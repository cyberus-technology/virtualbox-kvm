/*
 * Copyright 2006 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include "drm-uapi/drm_fourcc.h"

#include "brw_batch.h"
#include "brw_image.h"
#include "brw_mipmap_tree.h"
#include "brw_tex.h"
#include "brw_blit.h"
#include "brw_fbo.h"

#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_state.h"

#include "main/enums.h"
#include "main/fbobject.h"
#include "main/formats.h"
#include "main/glformats.h"
#include "main/texcompress_etc.h"
#include "main/teximage.h"
#include "main/streaming-load-memcpy.h"

#include "util/format_srgb.h"
#include "util/u_memory.h"

#include "x86/common_x86_asm.h"

#define FILE_DEBUG_FLAG DEBUG_MIPTREE

static void *brw_miptree_map_raw(struct brw_context *brw,
                                 struct brw_mipmap_tree *mt,
                                 GLbitfield mode);

static void brw_miptree_unmap_raw(struct brw_mipmap_tree *mt);

/**
 * Return true if the format that will be used to access the miptree is
 * CCS_E-compatible with the miptree's linear/non-sRGB format.
 *
 * Why use the linear format? Well, although the miptree may be specified with
 * an sRGB format, the usage of that color space/format can be toggled. Since
 * our HW tends to support more linear formats than sRGB ones, we use this
 * format variant for check for CCS_E compatibility.
 */
static bool
format_ccs_e_compat_with_miptree(const struct intel_device_info *devinfo,
                                 const struct brw_mipmap_tree *mt,
                                 enum isl_format access_format)
{
   assert(mt->aux_usage == ISL_AUX_USAGE_CCS_E);

   mesa_format linear_format = _mesa_get_srgb_format_linear(mt->format);
   enum isl_format isl_format = brw_isl_format_for_mesa_format(linear_format);
   return isl_formats_are_ccs_e_compatible(devinfo, isl_format, access_format);
}

/* Determine if CCS_E is supported for a given platform and mesa format. */
static bool
format_supports_ccs_e(const struct brw_context *brw, mesa_format format)
{
   /* For now compression is only enabled for integer formats even though
    * there exist supported floating point formats also. This is a heuristic
    * decision based on current public benchmarks. In none of the cases these
    * formats provided any improvement but a few cases were seen to regress.
    * Hence these are left to to be enabled in the future when they are known
    * to improve things.
    */
   if (_mesa_get_format_datatype(format) == GL_FLOAT)
      return false;

   /* Many window system buffers are sRGB even if they are never rendered as
    * sRGB.  For those, we want CCS_E for when sRGBEncode is false.  When the
    * surface is used as sRGB, we fall back to CCS_D.
    */
   mesa_format linear_format = _mesa_get_srgb_format_linear(format);
   enum isl_format isl_format = brw_isl_format_for_mesa_format(linear_format);
   return isl_format_supports_ccs_e(&brw->screen->devinfo, isl_format);
}

/**
 * Determine depth format corresponding to a depth+stencil format,
 * for separate stencil.
 */
mesa_format
brw_depth_format_for_depthstencil_format(mesa_format format) {
   switch (format) {
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      return MESA_FORMAT_Z24_UNORM_X8_UINT;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return MESA_FORMAT_Z_FLOAT32;
   default:
      return format;
   }
}

static bool
create_mapping_table(GLenum target, unsigned first_level, unsigned last_level,
                     unsigned depth0, struct brw_mipmap_level *table)
{
   for (unsigned level = first_level; level <= last_level; level++) {
      const unsigned d =
         target == GL_TEXTURE_3D ? minify(depth0, level) : depth0;

      table[level].slice = calloc(d, sizeof(*table[0].slice));
      if (!table[level].slice)
         goto unwind;
   }

   return true;

unwind:
   for (unsigned level = first_level; level <= last_level; level++)
      free(table[level].slice);

   return false;
}

static bool
needs_separate_stencil(const struct brw_context *brw,
                       struct brw_mipmap_tree *mt,
                       mesa_format format)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (_mesa_get_format_base_format(format) != GL_DEPTH_STENCIL)
      return false;

   if (devinfo->must_use_separate_stencil)
      return true;

   return brw->has_separate_stencil && brw->has_hiz;
}

/**
 * Choose the aux usage for this miptree.  This function must be called fairly
 * late in the miptree create process after we have a tiling.
 */
static void
brw_miptree_choose_aux_usage(struct brw_context *brw,
                             struct brw_mipmap_tree *mt)
{
   assert(mt->aux_usage == ISL_AUX_USAGE_NONE);

   if (_mesa_is_format_color_format(mt->format)) {
      if (mt->surf.samples > 1) {
         mt->aux_usage = ISL_AUX_USAGE_MCS;
      } else if (!INTEL_DEBUG(DEBUG_NO_RBC) &&
                 format_supports_ccs_e(brw, mt->format)) {
         mt->aux_usage = ISL_AUX_USAGE_CCS_E;
      } else if (brw->mesa_format_supports_render[mt->format]) {
         mt->aux_usage = ISL_AUX_USAGE_CCS_D;
      }
   } else if (isl_surf_usage_is_depth(mt->surf.usage) && brw->has_hiz) {
      mt->aux_usage = ISL_AUX_USAGE_HIZ;
   }

   /* We can do fast-clear on all auxiliary surface types that are
    * allocated through the normal texture creation paths.
    */
   if (mt->aux_usage != ISL_AUX_USAGE_NONE)
      mt->supports_fast_clear = true;
}


/**
 * Choose an appropriate uncompressed format for a requested
 * compressed format, if unsupported.
 */
mesa_format
brw_lower_compressed_format(struct brw_context *brw, mesa_format format)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* No need to lower ETC formats on these platforms,
    * they are supported natively.
    */
   if (devinfo->ver >= 8 || devinfo->is_baytrail)
      return format;

   switch (format) {
   case MESA_FORMAT_ETC1_RGB8:
      return MESA_FORMAT_R8G8B8X8_UNORM;
   case MESA_FORMAT_ETC2_RGB8:
      return MESA_FORMAT_R8G8B8X8_UNORM;
   case MESA_FORMAT_ETC2_SRGB8:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
      return MESA_FORMAT_B8G8R8A8_SRGB;
   case MESA_FORMAT_ETC2_RGBA8_EAC:
   case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
      return MESA_FORMAT_R8G8B8A8_UNORM;
   case MESA_FORMAT_ETC2_R11_EAC:
      return MESA_FORMAT_R_UNORM16;
   case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
      return MESA_FORMAT_R_SNORM16;
   case MESA_FORMAT_ETC2_RG11_EAC:
      return MESA_FORMAT_RG_UNORM16;
   case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
      return MESA_FORMAT_RG_SNORM16;
   default:
      /* Non ETC1 / ETC2 format */
      return format;
   }
}

unsigned
brw_get_num_logical_layers(const struct brw_mipmap_tree *mt, unsigned level)
{
   if (mt->surf.dim == ISL_SURF_DIM_3D)
      return minify(mt->surf.logical_level0_px.depth, level);
   else
      return mt->surf.logical_level0_px.array_len;
}

UNUSED static unsigned
get_num_phys_layers(const struct isl_surf *surf, unsigned level)
{
   /* In case of physical dimensions one needs to consider also the layout.
    * See isl_calc_phys_level0_extent_sa().
    */
   if (surf->dim != ISL_SURF_DIM_3D)
      return surf->phys_level0_sa.array_len;

   if (surf->dim_layout == ISL_DIM_LAYOUT_GFX4_2D)
      return minify(surf->phys_level0_sa.array_len, level);

   return minify(surf->phys_level0_sa.depth, level);
}

/** \brief Assert that the level and layer are valid for the miptree. */
void
brw_miptree_check_level_layer(const struct brw_mipmap_tree *mt,
                              uint32_t level,
                              uint32_t layer)
{
   (void) mt;
   (void) level;
   (void) layer;

   assert(level >= mt->first_level);
   assert(level <= mt->last_level);
   assert(layer < get_num_phys_layers(&mt->surf, level));
}

static enum isl_aux_state **
create_aux_state_map(struct brw_mipmap_tree *mt,
                     enum isl_aux_state initial)
{
   const uint32_t levels = mt->last_level + 1;

   uint32_t total_slices = 0;
   for (uint32_t level = 0; level < levels; level++)
      total_slices += brw_get_num_logical_layers(mt, level);

   const size_t per_level_array_size = levels * sizeof(enum isl_aux_state *);

   /* We're going to allocate a single chunk of data for both the per-level
    * reference array and the arrays of aux_state.  This makes cleanup
    * significantly easier.
    */
   const size_t total_size = per_level_array_size +
                             total_slices * sizeof(enum isl_aux_state);
   void *data = malloc(total_size);
   if (data == NULL)
      return NULL;

   enum isl_aux_state **per_level_arr = data;
   enum isl_aux_state *s = data + per_level_array_size;
   for (uint32_t level = 0; level < levels; level++) {
      per_level_arr[level] = s;
      const unsigned level_layers = brw_get_num_logical_layers(mt, level);
      for (uint32_t a = 0; a < level_layers; a++)
         *(s++) = initial;
   }
   assert((void *)s == data + total_size);

   return per_level_arr;
}

static void
free_aux_state_map(enum isl_aux_state **state)
{
   free(state);
}

static bool
need_to_retile_as_linear(struct brw_context *brw, unsigned blt_pitch,
                         enum isl_tiling tiling, unsigned samples)
{
   if (samples > 1)
      return false;

   if (tiling == ISL_TILING_LINEAR)
      return false;

   if (blt_pitch >= 32768) {
      perf_debug("blt pitch %u too large to blit, falling back to untiled",
                 blt_pitch);
      return true;
   }

   return false;
}

static bool
need_to_retile_as_x(const struct brw_context *brw, uint64_t size,
                    enum isl_tiling tiling)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* If the BO is too large to fit in the aperture, we need to use the
    * BLT engine to support it.  Prior to Sandybridge, the BLT paths can't
    * handle Y-tiling, so we need to fall back to X.
    */
   if (devinfo->ver < 6 && size >= brw->max_gtt_map_object_size &&
       tiling == ISL_TILING_Y0)
      return true;

   return false;
}

static struct brw_mipmap_tree *
make_surface(struct brw_context *brw, GLenum target, mesa_format format,
             unsigned first_level, unsigned last_level,
             unsigned width0, unsigned height0, unsigned depth0,
             unsigned num_samples, isl_tiling_flags_t tiling_flags,
             isl_surf_usage_flags_t isl_usage_flags, uint32_t alloc_flags,
             unsigned row_pitch_B, struct brw_bo *bo)
{
   struct brw_mipmap_tree *mt = calloc(sizeof(*mt), 1);
   if (!mt)
      return NULL;

   if (!create_mapping_table(target, first_level, last_level, depth0,
                             mt->level)) {
      free(mt);
      return NULL;
   }

   mt->refcount = 1;

   if (target == GL_TEXTURE_CUBE_MAP ||
       target == GL_TEXTURE_CUBE_MAP_ARRAY)
      isl_usage_flags |= ISL_SURF_USAGE_CUBE_BIT;

   DBG("%s: %s %s %ux %u:%u:%u %d..%d <-- %p\n",
        __func__,
       _mesa_enum_to_string(target),
       _mesa_get_format_name(format),
       num_samples, width0, height0, depth0,
       first_level, last_level, mt);

   struct isl_surf_init_info init_info = {
      .dim = get_isl_surf_dim(target),
      .format = translate_tex_format(brw, format, false),
      .width = width0,
      .height = height0,
      .depth = target == GL_TEXTURE_3D ? depth0 : 1,
      .levels = last_level - first_level + 1,
      .array_len = target == GL_TEXTURE_3D ? 1 : depth0,
      .samples = num_samples,
      .row_pitch_B = row_pitch_B,
      .usage = isl_usage_flags,
      .tiling_flags = tiling_flags,
   };

   if (!isl_surf_init_s(&brw->isl_dev, &mt->surf, &init_info))
      goto fail;

   /* Depth surfaces are always Y-tiled and stencil is always W-tiled, although
    * on gfx7 platforms we also need to create Y-tiled copies of stencil for
    * texturing since the hardware can't sample from W-tiled surfaces. For
    * everything else, check for corner cases needing special treatment.
    */
   bool is_depth_stencil =
      mt->surf.usage & (ISL_SURF_USAGE_STENCIL_BIT | ISL_SURF_USAGE_DEPTH_BIT);
   if (!is_depth_stencil) {
      if (need_to_retile_as_linear(brw, brw_miptree_blt_pitch(mt),
                                   mt->surf.tiling, mt->surf.samples)) {
         init_info.tiling_flags = 1u << ISL_TILING_LINEAR;
         if (!isl_surf_init_s(&brw->isl_dev, &mt->surf, &init_info))
            goto fail;
      } else if (need_to_retile_as_x(brw, mt->surf.size_B, mt->surf.tiling)) {
         init_info.tiling_flags = 1u << ISL_TILING_X;
         if (!isl_surf_init_s(&brw->isl_dev, &mt->surf, &init_info))
            goto fail;
      }
   }

   /* In case of linear the buffer gets padded by fixed 64 bytes and therefore
    * the size may not be multiple of row_pitch.
    * See isl_apply_surface_padding().
    */
   if (mt->surf.tiling != ISL_TILING_LINEAR)
      assert(mt->surf.size_B % mt->surf.row_pitch_B == 0);

   if (!bo) {
      mt->bo = brw_bo_alloc_tiled(brw->bufmgr, "isl-miptree",
                                  mt->surf.size_B,
                                  BRW_MEMZONE_OTHER,
                                  isl_tiling_to_i915_tiling(
                                     mt->surf.tiling),
                                  mt->surf.row_pitch_B, alloc_flags);
      if (!mt->bo)
         goto fail;
   } else {
      mt->bo = bo;
   }

   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->target = target;
   mt->format = format;
   mt->aux_state = NULL;
   mt->cpp = isl_format_get_layout(mt->surf.format)->bpb / 8;
   mt->compressed = _mesa_is_format_compressed(format);
   mt->drm_modifier = DRM_FORMAT_MOD_INVALID;

   return mt;

fail:
   brw_miptree_release(&mt);
   return NULL;
}

/* Return the usual surface usage flags for the given format. */
static isl_surf_usage_flags_t
mt_surf_usage(mesa_format format)
{
   switch(_mesa_get_format_base_format(format)) {
   case GL_DEPTH_COMPONENT:
      return ISL_SURF_USAGE_DEPTH_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
   case GL_DEPTH_STENCIL:
      return ISL_SURF_USAGE_DEPTH_BIT | ISL_SURF_USAGE_STENCIL_BIT |
             ISL_SURF_USAGE_TEXTURE_BIT;
   case GL_STENCIL_INDEX:
      return ISL_SURF_USAGE_STENCIL_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
   default:
      return ISL_SURF_USAGE_RENDER_TARGET_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
   }
}

static struct brw_mipmap_tree *
miptree_create(struct brw_context *brw,
               GLenum target,
               mesa_format format,
               GLuint first_level,
               GLuint last_level,
               GLuint width0,
               GLuint height0,
               GLuint depth0,
               GLuint num_samples,
               enum brw_miptree_create_flags flags)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const uint32_t alloc_flags =
      (flags & MIPTREE_CREATE_BUSY || num_samples > 1) ? BO_ALLOC_BUSY : 0;
   isl_tiling_flags_t tiling_flags = ISL_TILING_ANY_MASK;

   /* TODO: This used to be because there wasn't BLORP to handle Y-tiling. */
   if (devinfo->ver < 6 && _mesa_is_format_color_format(format))
      tiling_flags &= ~ISL_TILING_Y0_BIT;

   mesa_format mt_fmt = format;
   if (!_mesa_is_format_color_format(format) && devinfo->ver >= 6) {
      /* Fix up the Z miptree format for how we're splitting out separate
       * stencil. Gfx7 expects there to be no stencil bits in its depth buffer.
       */
      mt_fmt = brw_depth_format_for_depthstencil_format(format);
   }

   struct brw_mipmap_tree *mt =
      make_surface(brw, target, mt_fmt, first_level, last_level,
                   width0, height0, depth0, num_samples,
                   tiling_flags, mt_surf_usage(mt_fmt),
                   alloc_flags, 0, NULL);

   if (mt == NULL)
      return NULL;

   if (brw_miptree_needs_fake_etc(brw, mt)) {
      mesa_format decomp_format = brw_lower_compressed_format(brw, format);
      mt->shadow_mt = make_surface(brw, target, decomp_format, first_level,
                                   last_level, width0, height0, depth0,
                                   num_samples, tiling_flags,
                                   mt_surf_usage(decomp_format),
                                   alloc_flags, 0, NULL);

      if (mt->shadow_mt == NULL) {
         brw_miptree_release(&mt);
         return NULL;
      }
   }

   if (needs_separate_stencil(brw, mt, format)) {
      mt->stencil_mt =
         make_surface(brw, target, MESA_FORMAT_S_UINT8, first_level, last_level,
                      width0, height0, depth0, num_samples,
                      ISL_TILING_W_BIT, mt_surf_usage(MESA_FORMAT_S_UINT8),
                      alloc_flags, 0, NULL);
      if (mt->stencil_mt == NULL) {
         brw_miptree_release(&mt);
         return NULL;
      }
   }

   if (!(flags & MIPTREE_CREATE_NO_AUX))
      brw_miptree_choose_aux_usage(brw, mt);

   return mt;
}

struct brw_mipmap_tree *
brw_miptree_create(struct brw_context *brw,
                   GLenum target,
                   mesa_format format,
                   GLuint first_level,
                   GLuint last_level,
                   GLuint width0,
                   GLuint height0,
                   GLuint depth0,
                   GLuint num_samples,
                   enum brw_miptree_create_flags flags)
{
   assert(num_samples > 0);

   struct brw_mipmap_tree *mt = miptree_create(
                                     brw, target, format,
                                     first_level, last_level,
                                     width0, height0, depth0, num_samples,
                                     flags);
   if (!mt)
      return NULL;

   mt->offset = 0;

   /* Create the auxiliary surface up-front. CCS_D, on the other hand, can only
    * compress clear color so we wait until an actual fast-clear to allocate
    * it.
    */
   if (mt->aux_usage != ISL_AUX_USAGE_CCS_D &&
       !brw_miptree_alloc_aux(brw, mt)) {
      mt->aux_usage = ISL_AUX_USAGE_NONE;
      mt->supports_fast_clear = false;
   }

   return mt;
}

struct brw_mipmap_tree *
brw_miptree_create_for_bo(struct brw_context *brw,
                          struct brw_bo *bo,
                          mesa_format format,
                          uint32_t offset,
                          uint32_t width,
                          uint32_t height,
                          uint32_t depth,
                          int pitch,
                          enum isl_tiling tiling,
                          enum brw_miptree_create_flags flags)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_mipmap_tree *mt;
   const GLenum target = depth > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
   const GLenum base_format = _mesa_get_format_base_format(format);

   if ((base_format == GL_DEPTH_COMPONENT ||
        base_format == GL_DEPTH_STENCIL)) {
      const mesa_format mt_fmt = (devinfo->ver < 6) ? format :
         brw_depth_format_for_depthstencil_format(format);
      mt = make_surface(brw, target, mt_fmt,
                        0, 0, width, height, depth, 1, ISL_TILING_Y0_BIT,
                        mt_surf_usage(mt_fmt),
                        0, pitch, bo);
      if (!mt)
         return NULL;

      brw_bo_reference(bo);

      if (!(flags & MIPTREE_CREATE_NO_AUX))
         brw_miptree_choose_aux_usage(brw, mt);

      return mt;
   } else if (format == MESA_FORMAT_S_UINT8) {
      mt = make_surface(brw, target, MESA_FORMAT_S_UINT8,
                        0, 0, width, height, depth, 1,
                        ISL_TILING_W_BIT,
                        mt_surf_usage(MESA_FORMAT_S_UINT8),
                        0, pitch, bo);
      if (!mt)
         return NULL;

      assert(bo->size >= mt->surf.size_B);

      brw_bo_reference(bo);
      return mt;
   }

   /* Nothing will be able to use this miptree with the BO if the offset isn't
    * aligned.
    */
   if (tiling != ISL_TILING_LINEAR)
      assert(offset % 4096 == 0);

   /* miptrees can't handle negative pitch.  If you need flipping of images,
    * that's outside of the scope of the mt.
    */
   assert(pitch >= 0);

   mt = make_surface(brw, target, format,
                     0, 0, width, height, depth, 1,
                     1lu << tiling,
                     mt_surf_usage(format),
                     0, pitch, bo);
   if (!mt)
      return NULL;

   brw_bo_reference(bo);
   mt->bo = bo;
   mt->offset = offset;

   if (!(flags & MIPTREE_CREATE_NO_AUX)) {
      brw_miptree_choose_aux_usage(brw, mt);

      /* Create the auxiliary surface up-front. CCS_D, on the other hand, can
       * only compress clear color so we wait until an actual fast-clear to
       * allocate it.
       */
      if (mt->aux_usage != ISL_AUX_USAGE_CCS_D &&
          !brw_miptree_alloc_aux(brw, mt)) {
         mt->aux_usage = ISL_AUX_USAGE_NONE;
         mt->supports_fast_clear = false;
      }
   }

   return mt;
}

static struct brw_mipmap_tree *
miptree_create_for_planar_image(struct brw_context *brw,
                                __DRIimage *image, GLenum target,
                                enum isl_tiling tiling)
{
   const struct brw_image_format *f = image->planar_format;
   struct brw_mipmap_tree *planar_mt = NULL;

   for (int i = 0; i < f->nplanes; i++) {
      const int index = f->planes[i].buffer_index;
      const uint32_t dri_format = f->planes[i].dri_format;
      const mesa_format format = driImageFormatToGLFormat(dri_format);
      const uint32_t width = image->width >> f->planes[i].width_shift;
      const uint32_t height = image->height >> f->planes[i].height_shift;

      /* Disable creation of the texture's aux buffers because the driver
       * exposes no EGL API to manage them. That is, there is no API for
       * resolving the aux buffer's content to the main buffer nor for
       * invalidating the aux buffer's content.
       */
      struct brw_mipmap_tree *mt =
         brw_miptree_create_for_bo(brw, image->bo, format,
                                   image->offsets[index],
                                   width, height, 1,
                                   image->strides[index],
                                   tiling,
                                   MIPTREE_CREATE_NO_AUX);
      if (mt == NULL) {
         brw_miptree_release(&planar_mt);
         return NULL;
      }

      mt->target = target;

      if (i == 0)
         planar_mt = mt;
      else
         planar_mt->plane[i - 1] = mt;
   }

   planar_mt->drm_modifier = image->modifier;

   return planar_mt;
}

static bool
create_ccs_buf_for_image(struct brw_context *brw,
                         __DRIimage *image,
                         struct brw_mipmap_tree *mt,
                         enum isl_aux_state initial_state)
{
   struct isl_surf temp_ccs_surf = {0,};

   /* CCS is only supported for very simple miptrees */
   assert(image->aux_offset != 0 && image->aux_pitch != 0);
   assert(image->tile_x == 0 && image->tile_y == 0);
   assert(mt->surf.samples == 1);
   assert(mt->surf.levels == 1);
   assert(mt->surf.logical_level0_px.depth == 1);
   assert(mt->surf.logical_level0_px.array_len == 1);
   assert(mt->first_level == 0);
   assert(mt->last_level == 0);

   /* We shouldn't already have a CCS */
   assert(!mt->aux_buf);

   if (!isl_surf_get_ccs_surf(&brw->isl_dev, &mt->surf, NULL,
                              &temp_ccs_surf, image->aux_pitch))
      return false;

   assert(image->aux_offset < image->bo->size);
   assert(temp_ccs_surf.size_B <= image->bo->size - image->aux_offset);

   mt->aux_buf = calloc(sizeof(*mt->aux_buf), 1);
   if (mt->aux_buf == NULL)
      return false;

   mt->aux_state = create_aux_state_map(mt, initial_state);
   if (!mt->aux_state) {
      free(mt->aux_buf);
      mt->aux_buf = NULL;
      return false;
   }

   /* On gfx10+ we start using an extra space in the aux buffer to store the
    * indirect clear color. However, if we imported an image from the window
    * system with CCS, we don't have the extra space at the end of the aux
    * buffer. So create a new bo here that will store that clear color.
    */
   if (brw->isl_dev.ss.clear_color_state_size > 0) {
      mt->aux_buf->clear_color_bo =
         brw_bo_alloc_tiled(brw->bufmgr, "clear_color_bo",
                            brw->isl_dev.ss.clear_color_state_size,
                            BRW_MEMZONE_OTHER, I915_TILING_NONE, 0,
                            BO_ALLOC_ZEROED);
      if (!mt->aux_buf->clear_color_bo) {
         free(mt->aux_buf);
         mt->aux_buf = NULL;
         return false;
      }
   }

   mt->aux_buf->bo = image->bo;
   brw_bo_reference(image->bo);

   mt->aux_buf->offset = image->aux_offset;
   mt->aux_buf->surf = temp_ccs_surf;

   return true;
}

struct brw_mipmap_tree *
brw_miptree_create_for_dri_image(struct brw_context *brw,
                                 __DRIimage *image, GLenum target,
                                 mesa_format format,
                                 bool allow_internal_aux)
{
   uint32_t bo_tiling, bo_swizzle;
   brw_bo_get_tiling(image->bo, &bo_tiling, &bo_swizzle);

   const struct isl_drm_modifier_info *mod_info =
      isl_drm_modifier_get_info(image->modifier);

   const enum isl_tiling tiling =
      mod_info ? mod_info->tiling : isl_tiling_from_i915_tiling(bo_tiling);

   if (image->planar_format && image->planar_format->nplanes > 1)
      return miptree_create_for_planar_image(brw, image, target, tiling);

   if (image->planar_format)
      assert(image->planar_format->planes[0].dri_format == image->dri_format);

   if (!brw->ctx.TextureFormatSupported[format]) {
      /* The texture storage paths in core Mesa detect if the driver does not
       * support the user-requested format, and then searches for a
       * fallback format. The DRIimage code bypasses core Mesa, though. So we
       * do the fallbacks here for important formats.
       *
       * We must support DRM_FOURCC_XBGR8888 textures because the Android
       * framework produces HAL_PIXEL_FORMAT_RGBX8888 winsys surfaces, which
       * the Chrome OS compositor consumes as dma_buf EGLImages.
       */
      format = _mesa_format_fallback_rgbx_to_rgba(format);
   }

   if (!brw->ctx.TextureFormatSupported[format])
      return NULL;

   enum brw_miptree_create_flags mt_create_flags = 0;

   /* If this image comes in from a window system, we have different
    * requirements than if it comes in via an EGL import operation.  Window
    * system images can use any form of auxiliary compression we wish because
    * they get "flushed" before being handed off to the window system and we
    * have the opportunity to do resolves.  Non window-system images, on the
    * other hand, have no resolve point so we can't have aux without a
    * modifier.
    */
   if (!allow_internal_aux)
      mt_create_flags |= MIPTREE_CREATE_NO_AUX;

   /* If we have a modifier which specifies aux, don't create one yet */
   if (mod_info && mod_info->aux_usage != ISL_AUX_USAGE_NONE)
      mt_create_flags |= MIPTREE_CREATE_NO_AUX;

   /* Disable creation of the texture's aux buffers because the driver exposes
    * no EGL API to manage them. That is, there is no API for resolving the aux
    * buffer's content to the main buffer nor for invalidating the aux buffer's
    * content.
    */
   struct brw_mipmap_tree *mt =
      brw_miptree_create_for_bo(brw, image->bo, format,
                                image->offset, image->width, image->height, 1,
                                image->pitch, tiling, mt_create_flags);
   if (mt == NULL)
      return NULL;

   mt->target = target;
   mt->level[0].level_x = image->tile_x;
   mt->level[0].level_y = image->tile_y;
   mt->drm_modifier = image->modifier;

   /* From "OES_EGL_image" error reporting. We report GL_INVALID_OPERATION
    * for EGL images from non-tile aligned sufaces in gfx4 hw and earlier which has
    * trouble resolving back to destination image due to alignment issues.
    */
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   if (!devinfo->has_surface_tile_offset) {
      uint32_t draw_x, draw_y;
      brw_miptree_get_tile_offsets(mt, 0, 0, &draw_x, &draw_y);

      if (draw_x != 0 || draw_y != 0) {
         _mesa_error(&brw->ctx, GL_INVALID_OPERATION, __func__);
         brw_miptree_release(&mt);
         return NULL;
      }
   }

   if (mod_info && mod_info->aux_usage != ISL_AUX_USAGE_NONE) {
      assert(mod_info->aux_usage == ISL_AUX_USAGE_CCS_E);

      mt->aux_usage = mod_info->aux_usage;
      /* If we are a window system buffer, then we can support fast-clears
       * even if the modifier doesn't support them by doing a partial resolve
       * as part of the flush operation.
       */
      mt->supports_fast_clear =
         allow_internal_aux || mod_info->supports_clear_color;

      /* We don't know the actual state of the surface when we get it but we
       * can make a pretty good guess based on the modifier.  What we do know
       * for sure is that it isn't in the AUX_INVALID state, so we just assume
       * a worst case of compression.
       */
      enum isl_aux_state initial_state =
         isl_drm_modifier_get_default_aux_state(image->modifier);

      if (!create_ccs_buf_for_image(brw, image, mt, initial_state)) {
         brw_miptree_release(&mt);
         return NULL;
      }
   }

   /* Don't assume coherency for imported EGLimages.  We don't know what
    * external clients are going to do with it.  They may scan it out.
    */
   image->bo->cache_coherent = false;

   return mt;
}

/**
 * For a singlesample renderbuffer, this simply wraps the given BO with a
 * miptree.
 *
 * For a multisample renderbuffer, this wraps the window system's
 * (singlesample) BO with a singlesample miptree attached to the
 * brw_renderbuffer, then creates a multisample miptree attached to irb->mt
 * that will contain the actual rendering (which is lazily resolved to
 * irb->singlesample_mt).
 */
bool
brw_update_winsys_renderbuffer_miptree(struct brw_context *intel,
                                         struct brw_renderbuffer *irb,
                                         struct brw_mipmap_tree *singlesample_mt,
                                         uint32_t width, uint32_t height,
                                         uint32_t pitch)
{
   struct brw_mipmap_tree *multisample_mt = NULL;
   struct gl_renderbuffer *rb = &irb->Base.Base;
   mesa_format format = rb->Format;
   const unsigned num_samples = MAX2(rb->NumSamples, 1);

   /* Only the front and back buffers, which are color buffers, are allocated
    * through the image loader.
    */
   assert(_mesa_get_format_base_format(format) == GL_RGB ||
          _mesa_get_format_base_format(format) == GL_RGBA);

   assert(singlesample_mt);

   if (num_samples == 1) {
      brw_miptree_release(&irb->mt);
      irb->mt = singlesample_mt;

      assert(!irb->singlesample_mt);
   } else {
      brw_miptree_release(&irb->singlesample_mt);
      irb->singlesample_mt = singlesample_mt;

      if (!irb->mt ||
          irb->mt->surf.logical_level0_px.width != width ||
          irb->mt->surf.logical_level0_px.height != height) {
         multisample_mt = brw_miptree_create_for_renderbuffer(intel,
                                                              format,
                                                              width,
                                                              height,
                                                              num_samples);
         if (!multisample_mt)
            goto fail;

         irb->need_downsample = false;
         brw_miptree_release(&irb->mt);
         irb->mt = multisample_mt;
      }
   }
   return true;

fail:
   brw_miptree_release(&irb->mt);
   return false;
}

struct brw_mipmap_tree*
brw_miptree_create_for_renderbuffer(struct brw_context *brw,
                                    mesa_format format,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t num_samples)
{
   struct brw_mipmap_tree *mt;
   uint32_t depth = 1;
   GLenum target = num_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

   mt = brw_miptree_create(brw, target, format, 0, 0,
                           width, height, depth, num_samples,
                           MIPTREE_CREATE_BUSY);
   if (!mt)
      goto fail;

   return mt;

fail:
   brw_miptree_release(&mt);
   return NULL;
}

void
brw_miptree_reference(struct brw_mipmap_tree **dst,
                      struct brw_mipmap_tree *src)
{
   if (*dst == src)
      return;

   brw_miptree_release(dst);

   if (src) {
      src->refcount++;
      DBG("%s %p refcount now %d\n", __func__, src, src->refcount);
   }

   *dst = src;
}

static void
brw_miptree_aux_buffer_free(struct brw_miptree_aux_buffer *aux_buf)
{
   if (aux_buf == NULL)
      return;

   brw_bo_unreference(aux_buf->bo);
   brw_bo_unreference(aux_buf->clear_color_bo);

   free(aux_buf);
}

void
brw_miptree_release(struct brw_mipmap_tree **mt)
{
   if (!*mt)
      return;

   DBG("%s %p refcount will be %d\n", __func__, *mt, (*mt)->refcount - 1);
   if (--(*mt)->refcount <= 0) {
      GLuint i;

      DBG("%s deleting %p\n", __func__, *mt);

      brw_bo_unreference((*mt)->bo);
      brw_miptree_release(&(*mt)->stencil_mt);
      brw_miptree_release(&(*mt)->shadow_mt);
      brw_miptree_aux_buffer_free((*mt)->aux_buf);
      free_aux_state_map((*mt)->aux_state);

      brw_miptree_release(&(*mt)->plane[0]);
      brw_miptree_release(&(*mt)->plane[1]);

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
         free((*mt)->level[i].slice);
      }

      free(*mt);
   }
   *mt = NULL;
}


void
brw_get_image_dims(struct gl_texture_image *image,
                     int *width, int *height, int *depth)
{
   switch (image->TexObject->Target) {
   case GL_TEXTURE_1D_ARRAY:
      /* For a 1D Array texture the OpenGL API will treat the image height as
       * the number of array slices. For Intel hardware, we treat the 1D array
       * as a 2D Array with a height of 1. So, here we want to swap image
       * height and depth.
       */
      assert(image->Depth == 1);
      *width = image->Width;
      *height = 1;
      *depth = image->Height;
      break;
   case GL_TEXTURE_CUBE_MAP:
      /* For Cube maps, the mesa/main api layer gives us a depth of 1 even
       * though we really have 6 slices.
       */
      assert(image->Depth == 1);
      *width = image->Width;
      *height = image->Height;
      *depth = 6;
      break;
   default:
      *width = image->Width;
      *height = image->Height;
      *depth = image->Depth;
      break;
   }
}

/**
 * Can the image be pulled into a unified mipmap tree?  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
bool
brw_miptree_match_image(struct brw_mipmap_tree *mt,
                        struct gl_texture_image *image)
{
   struct brw_texture_image *brw_image = brw_texture_image(image);
   GLuint level = brw_image->base.Base.Level;
   int width, height, depth;

   /* glTexImage* choose the texture object based on the target passed in, and
    * objects can't change targets over their lifetimes, so this should be
    * true.
    */
   assert(image->TexObject->Target == mt->target);

   mesa_format mt_format = mt->format;
   if (mt->format == MESA_FORMAT_Z24_UNORM_X8_UINT && mt->stencil_mt)
      mt_format = MESA_FORMAT_Z24_UNORM_S8_UINT;
   if (mt->format == MESA_FORMAT_Z_FLOAT32 && mt->stencil_mt)
      mt_format = MESA_FORMAT_Z32_FLOAT_S8X24_UINT;

   if (_mesa_get_srgb_format_linear(image->TexFormat) !=
       _mesa_get_srgb_format_linear(mt_format))
      return false;

   brw_get_image_dims(image, &width, &height, &depth);

   if (mt->target == GL_TEXTURE_CUBE_MAP)
      depth = 6;

   if (level >= mt->surf.levels)
      return false;

   const unsigned level_depth =
      mt->surf.dim == ISL_SURF_DIM_3D ?
         minify(mt->surf.logical_level0_px.depth, level) :
         mt->surf.logical_level0_px.array_len;

   return width == minify(mt->surf.logical_level0_px.width, level) &&
          height == minify(mt->surf.logical_level0_px.height, level) &&
          depth == level_depth &&
          MAX2(image->NumSamples, 1) == mt->surf.samples;
}

void
brw_miptree_get_image_offset(const struct brw_mipmap_tree *mt,
                             GLuint level, GLuint slice,
                             GLuint *x, GLuint *y)
{
   if (level == 0 && slice == 0) {
      *x = mt->level[0].level_x;
      *y = mt->level[0].level_y;
      return;
   }

   uint32_t x_offset_sa, y_offset_sa, z_offset_sa, array_offset;

   /* Miptree itself can have an offset only if it represents a single
    * slice in an imported buffer object.
    * See brw_miptree_create_for_dri_image().
    */
   assert(mt->level[0].level_x == 0);
   assert(mt->level[0].level_y == 0);

   /* Given level is relative to level zero while the miptree may be
    * represent just a subset of all levels starting from 'first_level'.
    */
   assert(level >= mt->first_level);
   level -= mt->first_level;

   const unsigned z = mt->surf.dim == ISL_SURF_DIM_3D ? slice : 0;
   slice = mt->surf.dim == ISL_SURF_DIM_3D ? 0 : slice;
   isl_surf_get_image_offset_el(&mt->surf, level, slice, z,
                                &x_offset_sa, &y_offset_sa,
                                &z_offset_sa, &array_offset);

   *x = x_offset_sa;
   *y = y_offset_sa;
   assert(z_offset_sa == 0);
   assert(array_offset == 0);
}

/**
 * Compute the offset (in bytes) from the start of the BO to the given x
 * and y coordinate.  For tiled BOs, caller must ensure that x and y are
 * multiples of the tile size.
 */
uint32_t
brw_miptree_get_aligned_offset(const struct brw_mipmap_tree *mt,
                               uint32_t x, uint32_t y)
{
   int cpp = mt->cpp;
   uint32_t pitch = mt->surf.row_pitch_B;

   switch (mt->surf.tiling) {
   default:
      unreachable("not reached");
   case ISL_TILING_LINEAR:
      return y * pitch + x * cpp;
   case ISL_TILING_X:
      assert((x % (512 / cpp)) == 0);
      assert((y % 8) == 0);
      return y * pitch + x / (512 / cpp) * 4096;
   case ISL_TILING_Y0:
      assert((x % (128 / cpp)) == 0);
      assert((y % 32) == 0);
      return y * pitch + x / (128 / cpp) * 4096;
   }
}

/**
 * Rendering with tiled buffers requires that the base address of the buffer
 * be aligned to a page boundary.  For renderbuffers, and sometimes with
 * textures, we may want the surface to point at a texture image level that
 * isn't at a page boundary.
 *
 * This function returns an appropriately-aligned base offset
 * according to the tiling restrictions, plus any required x/y offset
 * from there.
 */
uint32_t
brw_miptree_get_tile_offsets(const struct brw_mipmap_tree *mt,
                             GLuint level, GLuint slice,
                             uint32_t *tile_x,
                             uint32_t *tile_y)
{
   uint32_t x, y;
   uint32_t mask_x, mask_y;

   isl_get_tile_masks(mt->surf.tiling, mt->cpp, &mask_x, &mask_y);
   brw_miptree_get_image_offset(mt, level, slice, &x, &y);

   *tile_x = x & mask_x;
   *tile_y = y & mask_y;

   return brw_miptree_get_aligned_offset(mt, x & ~mask_x, y & ~mask_y);
}

static void
brw_miptree_copy_slice_sw(struct brw_context *brw,
                          struct brw_mipmap_tree *src_mt,
                          unsigned src_level, unsigned src_layer,
                          struct brw_mipmap_tree *dst_mt,
                          unsigned dst_level, unsigned dst_layer,
                          unsigned width, unsigned height)
{
   void *src, *dst;
   ptrdiff_t src_stride, dst_stride;
   const unsigned cpp = (isl_format_get_layout(dst_mt->surf.format)->bpb / 8);

   brw_miptree_map(brw, src_mt,
                   src_level, src_layer,
                   0, 0,
                   width, height,
                   GL_MAP_READ_BIT | BRW_MAP_DIRECT_BIT,
                   &src, &src_stride);

   brw_miptree_map(brw, dst_mt,
                   dst_level, dst_layer,
                   0, 0,
                   width, height,
                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
                   BRW_MAP_DIRECT_BIT,
                   &dst, &dst_stride);

   DBG("sw blit %s mt %p %p/%"PRIdPTR" -> %s mt %p %p/%"PRIdPTR" (%dx%d)\n",
       _mesa_get_format_name(src_mt->format),
       src_mt, src, src_stride,
       _mesa_get_format_name(dst_mt->format),
       dst_mt, dst, dst_stride,
       width, height);

   int row_size = cpp * width;
   if (src_stride == row_size &&
       dst_stride == row_size) {
      memcpy(dst, src, row_size * height);
   } else {
      for (int i = 0; i < height; i++) {
         memcpy(dst, src, row_size);
         dst += dst_stride;
         src += src_stride;
      }
   }

   brw_miptree_unmap(brw, dst_mt, dst_level, dst_layer);
   brw_miptree_unmap(brw, src_mt, src_level, src_layer);

   /* Don't forget to copy the stencil data over, too.  We could have skipped
    * passing BRW_MAP_DIRECT_BIT, but that would have meant brw_miptree_map
    * shuffling the two data sources in/out of temporary storage instead of
    * the direct mapping we get this way.
    */
   if (dst_mt->stencil_mt) {
      assert(src_mt->stencil_mt);
      brw_miptree_copy_slice_sw(brw,
                                src_mt->stencil_mt, src_level, src_layer,
                                dst_mt->stencil_mt, dst_level, dst_layer,
                                width, height);
   }
}

void
brw_miptree_copy_slice(struct brw_context *brw,
                       struct brw_mipmap_tree *src_mt,
                       unsigned src_level, unsigned src_layer,
                       struct brw_mipmap_tree *dst_mt,
                       unsigned dst_level, unsigned dst_layer)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   mesa_format format = src_mt->format;
   unsigned width = minify(src_mt->surf.phys_level0_sa.width,
                           src_level - src_mt->first_level);
   unsigned height = minify(src_mt->surf.phys_level0_sa.height,
                            src_level - src_mt->first_level);

   assert(src_layer < get_num_phys_layers(&src_mt->surf,
                                          src_level - src_mt->first_level));

   assert(_mesa_get_srgb_format_linear(src_mt->format) ==
          _mesa_get_srgb_format_linear(dst_mt->format));

   DBG("validate blit mt %s %p %d,%d -> mt %s %p %d,%d (%dx%d)\n",
       _mesa_get_format_name(src_mt->format),
       src_mt, src_level, src_layer,
       _mesa_get_format_name(dst_mt->format),
       dst_mt, dst_level, dst_layer,
       width, height);

   if (devinfo->ver >= 6) {
      /* On gfx6 and above, we just use blorp.  It's faster than the blitter
       * and can handle everything without software fallbacks.
       */
      brw_blorp_copy_miptrees(brw,
                              src_mt, src_level, src_layer,
                              dst_mt, dst_level, dst_layer,
                              0, 0, 0, 0, width, height);

      if (src_mt->stencil_mt) {
         assert(dst_mt->stencil_mt);
         brw_blorp_copy_miptrees(brw,
                                 src_mt->stencil_mt, src_level, src_layer,
                                 dst_mt->stencil_mt, dst_level, dst_layer,
                                 0, 0, 0, 0, width, height);
      }
      return;
   }

   if (dst_mt->compressed) {
      unsigned int i, j;
      _mesa_get_format_block_size(dst_mt->format, &i, &j);
      height = ALIGN_NPOT(height, j) / j;
      width = ALIGN_NPOT(width, i) / i;
   }

   /* Gfx4-5 doesn't support separate stencil */
   assert(!src_mt->stencil_mt);

   uint32_t dst_x, dst_y, src_x, src_y;
   brw_miptree_get_image_offset(dst_mt, dst_level, dst_layer, &dst_x, &dst_y);
   brw_miptree_get_image_offset(src_mt, src_level, src_layer, &src_x, &src_y);

   DBG("validate blit mt %s %p %d,%d/%d -> mt %s %p %d,%d/%d (%dx%d)\n",
       _mesa_get_format_name(src_mt->format),
       src_mt, src_x, src_y, src_mt->surf.row_pitch_B,
       _mesa_get_format_name(dst_mt->format),
       dst_mt, dst_x, dst_y, dst_mt->surf.row_pitch_B,
       width, height);

   if (!brw_miptree_blit(brw,
                           src_mt, src_level, src_layer, 0, 0, false,
                           dst_mt, dst_level, dst_layer, 0, 0, false,
                           width, height, COLOR_LOGICOP_COPY)) {
      perf_debug("miptree validate blit for %s failed\n",
                 _mesa_get_format_name(format));

      brw_miptree_copy_slice_sw(brw,
                                src_mt, src_level, src_layer,
                                dst_mt, dst_level, dst_layer,
                                width, height);
   }
}

/**
 * Copies the image's current data to the given miptree, and associates that
 * miptree with the image.
 */
void
brw_miptree_copy_teximage(struct brw_context *brw,
                          struct brw_texture_image *brw_image,
                          struct brw_mipmap_tree *dst_mt)
{
   struct brw_mipmap_tree *src_mt = brw_image->mt;
   struct brw_texture_object *intel_obj =
      brw_texture_object(brw_image->base.Base.TexObject);
   int level = brw_image->base.Base.Level;
   const unsigned face = brw_image->base.Base.Face;
   unsigned start_layer, end_layer;

   if (intel_obj->base.Target == GL_TEXTURE_1D_ARRAY) {
      assert(face == 0);
      assert(brw_image->base.Base.Height);
      start_layer = 0;
      end_layer = brw_image->base.Base.Height - 1;
   } else if (face > 0) {
      start_layer = face;
      end_layer = face;
   } else {
      assert(brw_image->base.Base.Depth);
      start_layer = 0;
      end_layer = brw_image->base.Base.Depth - 1;
   }

   for (unsigned i = start_layer; i <= end_layer; i++) {
      brw_miptree_copy_slice(brw, src_mt, level, i, dst_mt, level, i);
   }

   brw_miptree_reference(&brw_image->mt, dst_mt);
   intel_obj->needs_validate = true;
}

static struct brw_miptree_aux_buffer *
brw_alloc_aux_buffer(struct brw_context *brw,
                       const struct isl_surf *aux_surf,
                       bool wants_memset,
                       uint8_t memset_value)
{
   struct brw_miptree_aux_buffer *buf = calloc(sizeof(*buf), 1);
   if (!buf)
      return false;

   uint64_t size = aux_surf->size_B;

   const bool has_indirect_clear = brw->isl_dev.ss.clear_color_state_size > 0;
   if (has_indirect_clear) {
      /* On CNL+, instead of setting the clear color in the SURFACE_STATE, we
       * will set a pointer to a dword somewhere that contains the color. So,
       * allocate the space for the clear color value here on the aux buffer.
       */
      buf->clear_color_offset = size;
      size += brw->isl_dev.ss.clear_color_state_size;
   }

   /* If the buffer needs to be initialised (requiring the buffer to be
    * immediately mapped to cpu space for writing), do not use the gpu access
    * flag which can cause an unnecessary delay if the backing pages happened
    * to be just used by the GPU.
    */
   const bool alloc_zeroed = wants_memset && memset_value == 0;
   const bool needs_memset =
      !alloc_zeroed && (wants_memset || has_indirect_clear);
   const uint32_t alloc_flags =
      alloc_zeroed ? BO_ALLOC_ZEROED : (needs_memset ? 0 : BO_ALLOC_BUSY);

   /* ISL has stricter set of alignment rules then the drm allocator.
    * Therefore one can pass the ISL dimensions in terms of bytes instead of
    * trying to recalculate based on different format block sizes.
    */
   buf->bo = brw_bo_alloc_tiled(brw->bufmgr, "aux-miptree", size,
                                BRW_MEMZONE_OTHER, I915_TILING_Y,
                                aux_surf->row_pitch_B, alloc_flags);
   if (!buf->bo) {
      free(buf);
      return NULL;
   }

   /* Initialize the bo to the desired value */
   if (needs_memset) {
      assert(!(alloc_flags & BO_ALLOC_BUSY));

      void *map = brw_bo_map(brw, buf->bo, MAP_WRITE | MAP_RAW);
      if (map == NULL) {
         brw_miptree_aux_buffer_free(buf);
         return NULL;
      }

      /* Memset the aux_surf portion of the BO. */
      if (wants_memset)
         memset(map, memset_value, aux_surf->size_B);

      /* Zero the indirect clear color to match ::fast_clear_color. */
      if (has_indirect_clear) {
         memset((char *)map + buf->clear_color_offset, 0,
                brw->isl_dev.ss.clear_color_state_size);
      }

      brw_bo_unmap(buf->bo);
   }

   if (has_indirect_clear) {
      buf->clear_color_bo = buf->bo;
      brw_bo_reference(buf->clear_color_bo);
   }

   buf->surf = *aux_surf;

   return buf;
}


/**
 * Helper for brw_miptree_alloc_aux() that sets
 * \c mt->level[level].has_hiz. Return true if and only if
 * \c has_hiz was set.
 */
static bool
brw_miptree_level_enable_hiz(struct brw_context *brw,
                             struct brw_mipmap_tree *mt,
                             uint32_t level)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(mt->aux_buf);
   assert(mt->surf.size_B > 0);

   if (devinfo->verx10 >= 75) {
      uint32_t width = minify(mt->surf.phys_level0_sa.width, level);
      uint32_t height = minify(mt->surf.phys_level0_sa.height, level);

      /* Disable HiZ for LOD > 0 unless the width is 8 aligned
       * and the height is 4 aligned. This allows our HiZ support
       * to fulfill Haswell restrictions for HiZ ops. For LOD == 0,
       * we can grow the width & height to allow the HiZ op to
       * force the proper size alignments.
       */
      if (level > 0 && ((width & 7) || (height & 3))) {
         DBG("mt %p level %d: HiZ DISABLED\n", mt, level);
         return false;
      }
   }

   DBG("mt %p level %d: HiZ enabled\n", mt, level);
   mt->level[level].has_hiz = true;
   return true;
}


/**
 * Allocate the initial aux surface for a miptree based on mt->aux_usage
 *
 * Since MCS, HiZ, and CCS_E can compress more than just clear color, we
 * create the auxiliary surfaces up-front.  CCS_D, on the other hand, can only
 * compress clear color so we wait until an actual fast-clear to allocate it.
 */
bool
brw_miptree_alloc_aux(struct brw_context *brw, struct brw_mipmap_tree *mt)
{
   assert(mt->aux_buf == NULL);

   /* Get the aux buf allocation parameters for this miptree. */
   enum isl_aux_state initial_state;
   uint8_t memset_value;
   struct isl_surf aux_surf = {0,};
   bool aux_surf_ok = false;

   switch (mt->aux_usage) {
   case ISL_AUX_USAGE_NONE:
      aux_surf.size_B = 0;
      aux_surf_ok = true;
      break;
   case ISL_AUX_USAGE_HIZ:
      initial_state = ISL_AUX_STATE_AUX_INVALID;
      memset_value = 0;
      aux_surf_ok = isl_surf_get_hiz_surf(&brw->isl_dev, &mt->surf, &aux_surf);
      break;
   case ISL_AUX_USAGE_MCS:
      /* From the Ivy Bridge PRM, Vol 2 Part 1 p326:
       *
       *     When MCS buffer is enabled and bound to MSRT, it is required that
       *     it is cleared prior to any rendering.
       *
       * Since we don't use the MCS buffer for any purpose other than
       * rendering, it makes sense to just clear it immediately upon
       * allocation.
       *
       * Note: the clear value for MCS buffers is all 1's, so we memset to
       * 0xff.
       */
      initial_state = ISL_AUX_STATE_CLEAR;
      memset_value = 0xFF;
      aux_surf_ok = isl_surf_get_mcs_surf(&brw->isl_dev, &mt->surf, &aux_surf);
      break;
   case ISL_AUX_USAGE_CCS_D:
   case ISL_AUX_USAGE_CCS_E:
      /* When CCS_E is used, we need to ensure that the CCS starts off in a
       * valid state.  From the Sky Lake PRM, "MCS Buffer for Render
       * Target(s)":
       *
       *    "If Software wants to enable Color Compression without Fast
       *    clear, Software needs to initialize MCS with zeros."
       *
       * A CCS value of 0 indicates that the corresponding block is in the
       * pass-through state which is what we want.
       *
       * For CCS_D, do the same thing. On gfx9+, this avoids having any
       * undefined bits in the aux buffer.
       */
      initial_state = ISL_AUX_STATE_PASS_THROUGH;
      memset_value = 0;
      aux_surf_ok =
         isl_surf_get_ccs_surf(&brw->isl_dev, &mt->surf, NULL, &aux_surf, 0);
      break;

   default:
      unreachable("Invalid aux usage");
   }

   /* We should have a valid aux_surf. */
   if (!aux_surf_ok)
      return false;

   /* No work is needed for a zero-sized auxiliary buffer. */
   if (aux_surf.size_B == 0)
      return true;

   /* Create the aux_state for the auxiliary buffer. */
   mt->aux_state = create_aux_state_map(mt, initial_state);
   if (mt->aux_state == NULL)
      return false;

   /* Allocate the auxiliary buffer. */
   const bool needs_memset = initial_state != ISL_AUX_STATE_AUX_INVALID;
   mt->aux_buf = brw_alloc_aux_buffer(brw, &aux_surf, needs_memset,
                                        memset_value);
   if (mt->aux_buf == NULL) {
      free_aux_state_map(mt->aux_state);
      mt->aux_state = NULL;
      return false;
   }

   /* Perform aux_usage-specific initialization. */
   if (mt->aux_usage == ISL_AUX_USAGE_HIZ) {
      for (unsigned level = mt->first_level; level <= mt->last_level; ++level)
         brw_miptree_level_enable_hiz(brw, mt, level);
   }

   return true;
}


/**
 * Can the miptree sample using the hiz buffer?
 */
bool
brw_miptree_sample_with_hiz(struct brw_context *brw,
                            struct brw_mipmap_tree *mt)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (!devinfo->has_sample_with_hiz) {
      return false;
   }

   if (!mt->aux_buf) {
      return false;
   }

   for (unsigned level = 0; level < mt->surf.levels; ++level) {
      if (!brw_miptree_level_has_hiz(mt, level))
         return false;
   }

   /* From the BDW PRM (Volume 2d: Command Reference: Structures
    *                   RENDER_SURFACE_STATE.AuxiliarySurfaceMode):
    *
    *  "If this field is set to AUX_HIZ, Number of Multisamples must be
    *   MULTISAMPLECOUNT_1, and Surface Type cannot be SURFTYPE_3D.
    *
    * There is no such blurb for 1D textures, but there is sufficient evidence
    * that this is broken on SKL+.
    */
   return (mt->surf.samples == 1 &&
           mt->target != GL_TEXTURE_3D &&
           mt->target != GL_TEXTURE_1D /* gfx9+ restriction */);
}

static bool
level_has_aux(const struct brw_mipmap_tree *mt, uint32_t level)
{
   return isl_aux_usage_has_hiz(mt->aux_usage) ?
          brw_miptree_level_has_hiz(mt, level) :
          mt->aux_usage != ISL_AUX_USAGE_NONE && mt->aux_buf;
}

/**
 * Does the miptree slice have hiz enabled?
 */
bool
brw_miptree_level_has_hiz(const struct brw_mipmap_tree *mt, uint32_t level)
{
   brw_miptree_check_level_layer(mt, level, 0);
   return mt->level[level].has_hiz;
}

static inline uint32_t
miptree_level_range_length(const struct brw_mipmap_tree *mt,
                           uint32_t start_level, uint32_t num_levels)
{
   assert(start_level >= mt->first_level);
   assert(start_level <= mt->last_level);

   if (num_levels == INTEL_REMAINING_LAYERS)
      num_levels = mt->last_level - start_level + 1;
   /* Check for overflow */
   assert(start_level + num_levels >= start_level);
   assert(start_level + num_levels <= mt->last_level + 1);

   return num_levels;
}

static inline uint32_t
miptree_layer_range_length(const struct brw_mipmap_tree *mt, uint32_t level,
                           uint32_t start_layer, uint32_t num_layers)
{
   assert(level <= mt->last_level);

   const uint32_t total_num_layers = brw_get_num_logical_layers(mt, level);
   assert(start_layer < total_num_layers);
   if (num_layers == INTEL_REMAINING_LAYERS)
      num_layers = total_num_layers - start_layer;
   /* Check for overflow */
   assert(start_layer + num_layers >= start_layer);
   assert(start_layer + num_layers <= total_num_layers);

   return num_layers;
}

bool
brw_miptree_has_color_unresolved(const struct brw_mipmap_tree *mt,
                                 unsigned start_level, unsigned num_levels,
                                 unsigned start_layer, unsigned num_layers)
{
   assert(_mesa_is_format_color_format(mt->format));

   if (!mt->aux_buf)
      return false;

   /* Clamp the level range to fit the miptree */
   num_levels = miptree_level_range_length(mt, start_level, num_levels);

   for (uint32_t l = 0; l < num_levels; l++) {
      const uint32_t level = start_level + l;
      const uint32_t level_layers =
         miptree_layer_range_length(mt, level, start_layer, num_layers);
      for (unsigned a = 0; a < level_layers; a++) {
         enum isl_aux_state aux_state =
            brw_miptree_get_aux_state(mt, level, start_layer + a);
         assert(aux_state != ISL_AUX_STATE_AUX_INVALID);
         if (aux_state != ISL_AUX_STATE_PASS_THROUGH)
            return true;
      }
   }

   return false;
}

static void
brw_miptree_check_color_resolve(const struct brw_context *brw,
                                const struct brw_mipmap_tree *mt,
                                unsigned level, unsigned layer)
{
   if (!mt->aux_buf)
      return;

   /* Fast color clear is supported for mipmapped surfaces only on Gfx8+. */
   assert(brw->screen->devinfo.ver >= 8 ||
          (level == 0 && mt->first_level == 0 && mt->last_level == 0));

   /* Compression of arrayed msaa surfaces is supported. */
   if (mt->surf.samples > 1)
      return;

   /* Fast color clear is supported for non-msaa arrays only on Gfx8+. */
   assert(brw->screen->devinfo.ver >= 8 ||
          (layer == 0 &&
           mt->surf.logical_level0_px.depth == 1 &&
           mt->surf.logical_level0_px.array_len == 1));

   (void)level;
   (void)layer;
}

void
brw_miptree_prepare_access(struct brw_context *brw,
                           struct brw_mipmap_tree *mt,
                           uint32_t start_level, uint32_t num_levels,
                           uint32_t start_layer, uint32_t num_layers,
                           enum isl_aux_usage aux_usage,
                           bool fast_clear_supported)
{
   const uint32_t clamped_levels =
      miptree_level_range_length(mt, start_level, num_levels);
   for (uint32_t l = 0; l < clamped_levels; l++) {
      const uint32_t level = start_level + l;
      if (!level_has_aux(mt, level))
         continue;

      const uint32_t level_layers =
         miptree_layer_range_length(mt, level, start_layer, num_layers);
      for (uint32_t a = 0; a < level_layers; a++) {
         const uint32_t layer = start_layer + a;
         const enum isl_aux_state aux_state =
            brw_miptree_get_aux_state(mt, level, layer);
         const enum isl_aux_op aux_op =
            isl_aux_prepare_access(aux_state, aux_usage, fast_clear_supported);

         if (aux_op == ISL_AUX_OP_NONE) {
            /* Nothing to do here. */
         } else if (isl_aux_usage_has_mcs(mt->aux_usage)) {
            assert(aux_op == ISL_AUX_OP_PARTIAL_RESOLVE);
            brw_blorp_mcs_partial_resolve(brw, mt, layer, 1);
         } else if (isl_aux_usage_has_hiz(mt->aux_usage)) {
            brw_hiz_exec(brw, mt, level, layer, 1, aux_op);
         } else {
            assert(isl_aux_usage_has_ccs(mt->aux_usage));
            brw_miptree_check_color_resolve(brw, mt, level, layer);
            brw_blorp_resolve_color(brw, mt, level, layer, aux_op);
         }

         const enum isl_aux_state new_state =
            isl_aux_state_transition_aux_op(aux_state, mt->aux_usage, aux_op);
         brw_miptree_set_aux_state(brw, mt, level, layer, 1, new_state);
      }
   }
}

void
brw_miptree_finish_write(struct brw_context *brw,
                         struct brw_mipmap_tree *mt, uint32_t level,
                         uint32_t start_layer, uint32_t num_layers,
                         enum isl_aux_usage aux_usage)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (mt->format == MESA_FORMAT_S_UINT8 && devinfo->ver <= 7) {
      mt->shadow_needs_update = true;
   } else if (brw_miptree_has_etc_shadow(brw, mt)) {
      mt->shadow_needs_update = true;
   }

   if (!level_has_aux(mt, level))
      return;

   const uint32_t level_layers =
      miptree_layer_range_length(mt, level, start_layer, num_layers);

   for (uint32_t a = 0; a < level_layers; a++) {
      const uint32_t layer = start_layer + a;
      const enum isl_aux_state aux_state =
         brw_miptree_get_aux_state(mt, level, layer);
      const enum isl_aux_state new_aux_state =
         isl_aux_state_transition_write(aux_state, aux_usage, false);
      brw_miptree_set_aux_state(brw, mt, level, layer, 1, new_aux_state);
   }
}

enum isl_aux_state
brw_miptree_get_aux_state(const struct brw_mipmap_tree *mt,
                          uint32_t level, uint32_t layer)
{
   brw_miptree_check_level_layer(mt, level, layer);

   if (_mesa_is_format_color_format(mt->format)) {
      assert(mt->aux_buf != NULL);
      assert(mt->surf.samples == 1 ||
             mt->surf.msaa_layout == ISL_MSAA_LAYOUT_ARRAY);
   } else if (mt->format == MESA_FORMAT_S_UINT8) {
      unreachable("Cannot get aux state for stencil");
   } else {
      assert(brw_miptree_level_has_hiz(mt, level));
   }

   return mt->aux_state[level][layer];
}

void
brw_miptree_set_aux_state(struct brw_context *brw,
                          struct brw_mipmap_tree *mt, uint32_t level,
                          uint32_t start_layer, uint32_t num_layers,
                          enum isl_aux_state aux_state)
{
   num_layers = miptree_layer_range_length(mt, level, start_layer, num_layers);

   if (_mesa_is_format_color_format(mt->format)) {
      assert(mt->aux_buf != NULL);
      assert(mt->surf.samples == 1 ||
             mt->surf.msaa_layout == ISL_MSAA_LAYOUT_ARRAY);
   } else if (mt->format == MESA_FORMAT_S_UINT8) {
      unreachable("Cannot get aux state for stencil");
   } else {
      assert(brw_miptree_level_has_hiz(mt, level));
   }

   for (unsigned a = 0; a < num_layers; a++) {
      if (mt->aux_state[level][start_layer + a] != aux_state) {
         mt->aux_state[level][start_layer + a] = aux_state;
         brw->ctx.NewDriverState |= BRW_NEW_AUX_STATE;
      }
   }
}

/* On Gfx9 color buffers may be compressed by the hardware (lossless
 * compression). There are, however, format restrictions and care needs to be
 * taken that the sampler engine is capable for re-interpreting a buffer with
 * format different the buffer was originally written with.
 *
 * For example, SRGB formats are not compressible and the sampler engine isn't
 * capable of treating RGBA_UNORM as SRGB_ALPHA. In such a case the underlying
 * color buffer needs to be resolved so that the sampling surface can be
 * sampled as non-compressed (i.e., without the auxiliary MCS buffer being
 * set).
 */
static bool
can_texture_with_ccs(struct brw_context *brw,
                     struct brw_mipmap_tree *mt,
                     enum isl_format view_format)
{
   if (mt->aux_usage != ISL_AUX_USAGE_CCS_E)
      return false;

   if (!format_ccs_e_compat_with_miptree(&brw->screen->devinfo,
                                         mt, view_format)) {
      perf_debug("Incompatible sampling format (%s) for rbc (%s)\n",
                 isl_format_get_name(view_format),
                 _mesa_get_format_name(mt->format));
      return false;
   }

   return true;
}

enum isl_aux_usage
brw_miptree_texture_aux_usage(struct brw_context *brw,
                              struct brw_mipmap_tree *mt,
                              enum isl_format view_format,
                              enum gfx9_astc5x5_wa_tex_type astc5x5_wa_bits)
{
   assert(brw->screen->devinfo.ver == 9 || astc5x5_wa_bits == 0);

   /* On gfx9, ASTC 5x5 textures cannot live in the sampler cache along side
    * CCS or HiZ compressed textures.  See gfx9_apply_astc5x5_wa_flush() for
    * details.
    */
   if ((astc5x5_wa_bits & GFX9_ASTC5X5_WA_TEX_TYPE_ASTC5x5) &&
       mt->aux_usage != ISL_AUX_USAGE_MCS)
      return ISL_AUX_USAGE_NONE;

   switch (mt->aux_usage) {
   case ISL_AUX_USAGE_HIZ:
      if (brw_miptree_sample_with_hiz(brw, mt))
         return ISL_AUX_USAGE_HIZ;
      break;

   case ISL_AUX_USAGE_MCS:
      return ISL_AUX_USAGE_MCS;

   case ISL_AUX_USAGE_CCS_D:
   case ISL_AUX_USAGE_CCS_E:
      if (!mt->aux_buf) {
         assert(mt->aux_usage == ISL_AUX_USAGE_CCS_D);
         return ISL_AUX_USAGE_NONE;
      }

      /* If we don't have any unresolved color, report an aux usage of
       * ISL_AUX_USAGE_NONE.  This way, texturing won't even look at the
       * aux surface and we can save some bandwidth.
       */
      if (!brw_miptree_has_color_unresolved(mt, 0, INTEL_REMAINING_LEVELS,
                                              0, INTEL_REMAINING_LAYERS))
         return ISL_AUX_USAGE_NONE;

      if (can_texture_with_ccs(brw, mt, view_format))
         return ISL_AUX_USAGE_CCS_E;
      break;

   default:
      break;
   }

   return ISL_AUX_USAGE_NONE;
}

static bool
isl_formats_are_fast_clear_compatible(enum isl_format a, enum isl_format b)
{
   /* On gfx8 and earlier, the hardware was only capable of handling 0/1 clear
    * values so sRGB curve application was a no-op for all fast-clearable
    * formats.
    *
    * On gfx9+, the hardware supports arbitrary clear values.  For sRGB clear
    * values, the hardware interprets the floats, not as what would be
    * returned from the sampler (or written by the shader), but as being
    * between format conversion and sRGB curve application.  This means that
    * we can switch between sRGB and UNORM without having to whack the clear
    * color.
    */
   return isl_format_srgb_to_linear(a) == isl_format_srgb_to_linear(b);
}

void
brw_miptree_prepare_texture(struct brw_context *brw,
                            struct brw_mipmap_tree *mt,
                            enum isl_format view_format,
                            uint32_t start_level, uint32_t num_levels,
                            uint32_t start_layer, uint32_t num_layers,
                            enum gfx9_astc5x5_wa_tex_type astc5x5_wa_bits)
{
   enum isl_aux_usage aux_usage =
      brw_miptree_texture_aux_usage(brw, mt, view_format, astc5x5_wa_bits);

   bool clear_supported = aux_usage != ISL_AUX_USAGE_NONE;

   /* Clear color is specified as ints or floats and the conversion is done by
    * the sampler.  If we have a texture view, we would have to perform the
    * clear color conversion manually.  Just disable clear color.
    */
   if (!isl_formats_are_fast_clear_compatible(mt->surf.format, view_format))
      clear_supported = false;

   brw_miptree_prepare_access(brw, mt, start_level, num_levels,
                              start_layer, num_layers,
                              aux_usage, clear_supported);
}

void
brw_miptree_prepare_image(struct brw_context *brw, struct brw_mipmap_tree *mt)
{
   /* The data port doesn't understand any compression */
   brw_miptree_prepare_access(brw, mt, 0, INTEL_REMAINING_LEVELS,
                              0, INTEL_REMAINING_LAYERS,
                              ISL_AUX_USAGE_NONE, false);
}

enum isl_aux_usage
brw_miptree_render_aux_usage(struct brw_context *brw,
                             struct brw_mipmap_tree *mt,
                             enum isl_format render_format,
                             bool blend_enabled,
                             bool draw_aux_disabled)
{
   struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (draw_aux_disabled)
      return ISL_AUX_USAGE_NONE;

   switch (mt->aux_usage) {
   case ISL_AUX_USAGE_MCS:
      assert(mt->aux_buf);
      return ISL_AUX_USAGE_MCS;

   case ISL_AUX_USAGE_CCS_D:
   case ISL_AUX_USAGE_CCS_E:
      if (!mt->aux_buf) {
         assert(mt->aux_usage == ISL_AUX_USAGE_CCS_D);
         return ISL_AUX_USAGE_NONE;
      }

      /* gfx9+ hardware technically supports non-0/1 clear colors with sRGB
       * formats.  However, there are issues with blending where it doesn't
       * properly apply the sRGB curve to the clear color when blending.
       */
      if (devinfo->ver >= 9 && blend_enabled &&
          isl_format_is_srgb(render_format) &&
          !isl_color_value_is_zero_one(mt->fast_clear_color, render_format))
         return ISL_AUX_USAGE_NONE;

      if (mt->aux_usage == ISL_AUX_USAGE_CCS_E &&
          format_ccs_e_compat_with_miptree(&brw->screen->devinfo,
                                           mt, render_format))
         return ISL_AUX_USAGE_CCS_E;

      /* Otherwise, we have to fall back to CCS_D */
      return ISL_AUX_USAGE_CCS_D;

   default:
      return ISL_AUX_USAGE_NONE;
   }
}

void
brw_miptree_prepare_render(struct brw_context *brw,
                           struct brw_mipmap_tree *mt, uint32_t level,
                           uint32_t start_layer, uint32_t layer_count,
                           enum isl_aux_usage aux_usage)
{
   brw_miptree_prepare_access(brw, mt, level, 1, start_layer, layer_count,
                              aux_usage, aux_usage != ISL_AUX_USAGE_NONE);
}

void
brw_miptree_finish_render(struct brw_context *brw,
                          struct brw_mipmap_tree *mt, uint32_t level,
                          uint32_t start_layer, uint32_t layer_count,
                          enum isl_aux_usage aux_usage)
{
   assert(_mesa_is_format_color_format(mt->format));

   brw_miptree_finish_write(brw, mt, level, start_layer, layer_count,
                              aux_usage);
}

void
brw_miptree_prepare_depth(struct brw_context *brw,
                          struct brw_mipmap_tree *mt, uint32_t level,
                          uint32_t start_layer, uint32_t layer_count)
{
   brw_miptree_prepare_access(brw, mt, level, 1, start_layer, layer_count,
                              mt->aux_usage, mt->aux_buf != NULL);
}

void
brw_miptree_finish_depth(struct brw_context *brw,
                         struct brw_mipmap_tree *mt, uint32_t level,
                         uint32_t start_layer, uint32_t layer_count,
                         bool depth_written)
{
   if (depth_written) {
      brw_miptree_finish_write(brw, mt, level, start_layer, layer_count,
                               mt->aux_usage);
   }
}

void
brw_miptree_prepare_external(struct brw_context *brw,
                             struct brw_mipmap_tree *mt)
{
   enum isl_aux_usage aux_usage = ISL_AUX_USAGE_NONE;
   bool supports_fast_clear = false;

   const struct isl_drm_modifier_info *mod_info =
      isl_drm_modifier_get_info(mt->drm_modifier);

   if (mod_info && mod_info->aux_usage != ISL_AUX_USAGE_NONE) {
      /* CCS_E is the only supported aux for external images and it's only
       * supported on very simple images.
       */
      assert(mod_info->aux_usage == ISL_AUX_USAGE_CCS_E);
      assert(_mesa_is_format_color_format(mt->format));
      assert(mt->first_level == 0 && mt->last_level == 0);
      assert(mt->surf.logical_level0_px.depth == 1);
      assert(mt->surf.logical_level0_px.array_len == 1);
      assert(mt->surf.samples == 1);
      assert(mt->aux_buf != NULL);

      aux_usage = mod_info->aux_usage;
      supports_fast_clear = mod_info->supports_clear_color;
   }

   brw_miptree_prepare_access(brw, mt, 0, INTEL_REMAINING_LEVELS,
                              0, INTEL_REMAINING_LAYERS,
                              aux_usage, supports_fast_clear);
}

void
brw_miptree_finish_external(struct brw_context *brw,
                            struct brw_mipmap_tree *mt)
{
   if (!mt->aux_buf)
      return;

   /* We don't know the actual aux state of the aux surface.  The previous
    * owner could have given it to us in a number of different states.
    * Because we don't know the aux state, we reset the aux state to the
    * least common denominator of possible valid states.
    */
   enum isl_aux_state default_aux_state =
      isl_drm_modifier_get_default_aux_state(mt->drm_modifier);
   assert(mt->last_level == mt->first_level);
   brw_miptree_set_aux_state(brw, mt, 0, 0, INTEL_REMAINING_LAYERS,
                               default_aux_state);
}

/**
 * Make it possible to share the BO backing the given miptree with another
 * process or another miptree.
 *
 * Fast color clears are unsafe with shared buffers, so we need to resolve and
 * then discard the MCS buffer, if present.  We also set the no_ccs flag to
 * ensure that no MCS buffer gets allocated in the future.
 *
 * HiZ is similarly unsafe with shared buffers.
 */
void
brw_miptree_make_shareable(struct brw_context *brw,
                           struct brw_mipmap_tree *mt)
{
   /* MCS buffers are also used for multisample buffers, but we can't resolve
    * away a multisample MCS buffer because it's an integral part of how the
    * pixel data is stored.  Fortunately this code path should never be
    * reached for multisample buffers.
    */
   assert(mt->surf.msaa_layout == ISL_MSAA_LAYOUT_NONE ||
          mt->surf.samples == 1);

   brw_miptree_prepare_access(brw, mt, 0, INTEL_REMAINING_LEVELS,
                              0, INTEL_REMAINING_LAYERS,
                              ISL_AUX_USAGE_NONE, false);

   if (mt->aux_buf) {
      brw_miptree_aux_buffer_free(mt->aux_buf);
      mt->aux_buf = NULL;

      /* Make future calls of brw_miptree_level_has_hiz() return false. */
      for (uint32_t l = mt->first_level; l <= mt->last_level; ++l) {
         mt->level[l].has_hiz = false;
      }

      free(mt->aux_state);
      mt->aux_state = NULL;
      brw->ctx.NewDriverState |= BRW_NEW_AUX_STATE;
   }

   mt->aux_usage = ISL_AUX_USAGE_NONE;
   mt->supports_fast_clear = false;
}


/**
 * \brief Get pointer offset into stencil buffer.
 *
 * The stencil buffer is W tiled. Since the GTT is incapable of W fencing, we
 * must decode the tile's layout in software.
 *
 * See
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.2.1 W-Major Tile
 *     Format.
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.3 Tiling Algorithm
 *
 * Even though the returned offset is always positive, the return type is
 * signed due to
 *    commit e8b1c6d6f55f5be3bef25084fdd8b6127517e137
 *    mesa: Fix return type of  _mesa_get_format_bytes() (#37351)
 */
static intptr_t
brw_offset_S8(uint32_t stride, uint32_t x, uint32_t y, bool swizzled)
{
   uint32_t tile_size = 4096;
   uint32_t tile_width = 64;
   uint32_t tile_height = 64;
   uint32_t row_size = 64 * stride / 2; /* Two rows are interleaved. */

   uint32_t tile_x = x / tile_width;
   uint32_t tile_y = y / tile_height;

   /* The byte's address relative to the tile's base addres. */
   uint32_t byte_x = x % tile_width;
   uint32_t byte_y = y % tile_height;

   uintptr_t u = tile_y * row_size
               + tile_x * tile_size
               + 512 * (byte_x / 8)
               +  64 * (byte_y / 8)
               +  32 * ((byte_y / 4) % 2)
               +  16 * ((byte_x / 4) % 2)
               +   8 * ((byte_y / 2) % 2)
               +   4 * ((byte_x / 2) % 2)
               +   2 * (byte_y % 2)
               +   1 * (byte_x % 2);

   if (swizzled) {
      /* adjust for bit6 swizzling */
      if (((byte_x / 8) % 2) == 1) {
         if (((byte_y / 8) % 2) == 0) {
            u += 64;
         } else {
            u -= 64;
         }
      }
   }

   return u;
}

void
brw_miptree_updownsample(struct brw_context *brw,
                         struct brw_mipmap_tree *src,
                         struct brw_mipmap_tree *dst)
{
   unsigned src_w = src->surf.logical_level0_px.width;
   unsigned src_h = src->surf.logical_level0_px.height;
   unsigned dst_w = dst->surf.logical_level0_px.width;
   unsigned dst_h = dst->surf.logical_level0_px.height;

   brw_blorp_blit_miptrees(brw,
                           src, 0 /* level */, 0 /* layer */,
                           src->format, SWIZZLE_XYZW,
                           dst, 0 /* level */, 0 /* layer */, dst->format,
                           0, 0, src_w, src_h,
                           0, 0, dst_w, dst_h,
                           GL_NEAREST, false, false /*mirror x, y*/,
                           false, false);

   if (src->stencil_mt) {
      src_w = src->stencil_mt->surf.logical_level0_px.width;
      src_h = src->stencil_mt->surf.logical_level0_px.height;
      dst_w = dst->stencil_mt->surf.logical_level0_px.width;
      dst_h = dst->stencil_mt->surf.logical_level0_px.height;

      brw_blorp_blit_miptrees(brw,
                              src->stencil_mt, 0 /* level */, 0 /* layer */,
                              src->stencil_mt->format, SWIZZLE_XYZW,
                              dst->stencil_mt, 0 /* level */, 0 /* layer */,
                              dst->stencil_mt->format,
                              0, 0, src_w, src_h,
                              0, 0, dst_w, dst_h,
                              GL_NEAREST, false, false /*mirror x, y*/,
                              false, false /* decode/encode srgb */);
   }
}

void
brw_update_r8stencil(struct brw_context *brw,
                       struct brw_mipmap_tree *mt)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver >= 7);
   struct brw_mipmap_tree *src =
      mt->format == MESA_FORMAT_S_UINT8 ? mt : mt->stencil_mt;
   if (!src || devinfo->ver >= 8)
      return;

   assert(src->surf.size_B > 0);

   if (!mt->shadow_mt) {
      assert(devinfo->ver > 6); /* Handle MIPTREE_LAYOUT_GFX6_HIZ_STENCIL */
      mt->shadow_mt = make_surface(
                            brw,
                            src->target,
                            MESA_FORMAT_R_UINT8,
                            src->first_level, src->last_level,
                            src->surf.logical_level0_px.width,
                            src->surf.logical_level0_px.height,
                            src->surf.dim == ISL_SURF_DIM_3D ?
                               src->surf.logical_level0_px.depth :
                               src->surf.logical_level0_px.array_len,
                            src->surf.samples,
                            ISL_TILING_Y0_BIT,
                            ISL_SURF_USAGE_TEXTURE_BIT,
                            BO_ALLOC_BUSY, 0, NULL);
      assert(mt->shadow_mt);
   }

   if (src->shadow_needs_update == false)
      return;

   struct brw_mipmap_tree *dst = mt->shadow_mt;

   for (int level = src->first_level; level <= src->last_level; level++) {
      const unsigned depth = src->surf.dim == ISL_SURF_DIM_3D ?
         minify(src->surf.phys_level0_sa.depth, level) :
         src->surf.phys_level0_sa.array_len;

      for (unsigned layer = 0; layer < depth; layer++) {
         brw_blorp_copy_miptrees(brw,
                                 src, level, layer,
                                 dst, level, layer,
                                 0, 0, 0, 0,
                                 minify(src->surf.logical_level0_px.width,
                                        level),
                                 minify(src->surf.logical_level0_px.height,
                                        level));
      }
   }

   brw_cache_flush_for_read(brw, dst->bo);
   src->shadow_needs_update = false;
}

static void *
brw_miptree_map_raw(struct brw_context *brw,
                    struct brw_mipmap_tree *mt,
                    GLbitfield mode)
{
   struct brw_bo *bo = mt->bo;

   if (brw_batch_references(&brw->batch, bo))
      brw_batch_flush(brw);

   return brw_bo_map(brw, bo, mode);
}

static void
brw_miptree_unmap_raw(struct brw_mipmap_tree *mt)
{
   brw_bo_unmap(mt->bo);
}

static void
brw_miptree_unmap_map(struct brw_context *brw,
                      struct brw_mipmap_tree *mt,
                      struct brw_miptree_map *map,
                      unsigned int level, unsigned int slice)
{
   brw_miptree_unmap_raw(mt);
}

static void
brw_miptree_map_map(struct brw_context *brw,
                    struct brw_mipmap_tree *mt,
                    struct brw_miptree_map *map,
                    unsigned int level, unsigned int slice)
{
   unsigned int bw, bh;
   void *base;
   unsigned int image_x, image_y;
   intptr_t x = map->x;
   intptr_t y = map->y;

   /* For compressed formats, the stride is the number of bytes per
    * row of blocks.  brw_miptree_get_image_offset() already does
    * the divide.
    */
   _mesa_get_format_block_size(mt->format, &bw, &bh);
   assert(y % bh == 0);
   assert(x % bw == 0);
   y /= bh;
   x /= bw;

   brw_miptree_access_raw(brw, mt, level, slice,
                          map->mode & GL_MAP_WRITE_BIT);

   base = brw_miptree_map_raw(brw, mt, map->mode);

   if (base == NULL)
      map->ptr = NULL;
   else {
      base += mt->offset;

      /* Note that in the case of cube maps, the caller must have passed the
       * slice number referencing the face.
      */
      brw_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);
      x += image_x;
      y += image_y;

      map->stride = mt->surf.row_pitch_B;
      map->ptr = base + y * map->stride + x * mt->cpp;
   }

   DBG("%s: %d,%d %dx%d from mt %p (%s) "
       "%"PRIiPTR",%"PRIiPTR" = %p/%d\n", __func__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       x, y, map->ptr, map->stride);

   map->unmap = brw_miptree_unmap_map;
}

static void
brw_miptree_unmap_blit(struct brw_context *brw,
                       struct brw_mipmap_tree *mt,
                       struct brw_miptree_map *map,
                       unsigned int level,
                       unsigned int slice)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;

   brw_miptree_unmap_raw(map->linear_mt);

   if (map->mode & GL_MAP_WRITE_BIT) {
      if (devinfo->ver >= 6) {
         brw_blorp_copy_miptrees(brw, map->linear_mt, 0, 0,
                                 mt, level, slice,
                                 0, 0, map->x, map->y, map->w, map->h);
      } else {
         bool ok = brw_miptree_copy(brw,
                                    map->linear_mt, 0, 0, 0, 0,
                                    mt, level, slice, map->x, map->y,
                                    map->w, map->h);
         WARN_ONCE(!ok, "Failed to blit from linear temporary mapping");
      }
   }

   brw_miptree_release(&map->linear_mt);
}

/* Compute extent parameters for use with tiled_memcpy functions.
 * xs are in units of bytes and ys are in units of strides.
 */
static inline void
tile_extents(struct brw_mipmap_tree *mt, struct brw_miptree_map *map,
             unsigned int level, unsigned int slice, unsigned int *x1_B,
             unsigned int *x2_B, unsigned int *y1_el, unsigned int *y2_el)
{
   unsigned int block_width, block_height;
   unsigned int x0_el, y0_el;

   _mesa_get_format_block_size(mt->format, &block_width, &block_height);

   assert(map->x % block_width == 0);
   assert(map->y % block_height == 0);

   brw_miptree_get_image_offset(mt, level, slice, &x0_el, &y0_el);
   *x1_B = (map->x / block_width + x0_el) * mt->cpp;
   *y1_el = map->y / block_height + y0_el;
   *x2_B = (DIV_ROUND_UP(map->x + map->w, block_width) + x0_el) * mt->cpp;
   *y2_el = DIV_ROUND_UP(map->y + map->h, block_height) + y0_el;
}

static void
brw_miptree_unmap_tiled_memcpy(struct brw_context *brw,
                               struct brw_mipmap_tree *mt,
                               struct brw_miptree_map *map,
                               unsigned int level,
                               unsigned int slice)
{
   if (map->mode & GL_MAP_WRITE_BIT) {
      unsigned int x1, x2, y1, y2;
      tile_extents(mt, map, level, slice, &x1, &x2, &y1, &y2);

      char *dst = brw_miptree_map_raw(brw, mt, map->mode | MAP_RAW);
      dst += mt->offset;

      isl_memcpy_linear_to_tiled(
         x1, x2, y1, y2, dst, map->ptr, mt->surf.row_pitch_B, map->stride,
         brw->has_swizzling, mt->surf.tiling, ISL_MEMCPY);

      brw_miptree_unmap_raw(mt);
   }
   align_free(map->buffer);
   map->buffer = map->ptr = NULL;
}

/**
 * Determine which copy function to use for the given format combination
 *
 * The only two possible copy functions which are ever returned are a
 * direct memcpy and a RGBA <-> BGRA copy function.  Since RGBA -> BGRA and
 * BGRA -> RGBA are exactly the same operation (and memcpy is obviously
 * symmetric), it doesn't matter whether the copy is from the tiled image
 * to the untiled or vice versa.  The copy function required is the same in
 * either case so this function can be used.
 *
 * \param[in]  tiledFormat The format of the tiled image
 * \param[in]  format      The GL format of the client data
 * \param[in]  type        The GL type of the client data
 * \param[out] mem_copy    Will be set to one of either the standard
 *                         library's memcpy or a different copy function
 *                         that performs an RGBA to BGRA conversion
 * \param[out] cpp         Number of bytes per channel
 *
 * \return true if the format and type combination are valid
 */
isl_memcpy_type
brw_miptree_get_memcpy_type(mesa_format tiledFormat, GLenum format, GLenum type,
                            uint32_t *cpp)
{
   if (type == GL_UNSIGNED_INT_8_8_8_8_REV &&
       !(format == GL_RGBA || format == GL_BGRA))
      return ISL_MEMCPY_INVALID; /* Invalid type/format combination */

   if ((tiledFormat == MESA_FORMAT_L_UNORM8 && format == GL_LUMINANCE) ||
       (tiledFormat == MESA_FORMAT_A_UNORM8 && format == GL_ALPHA)) {
      *cpp = 1;
      return ISL_MEMCPY;
   } else if ((tiledFormat == MESA_FORMAT_B8G8R8A8_UNORM) ||
              (tiledFormat == MESA_FORMAT_B8G8R8X8_UNORM) ||
              (tiledFormat == MESA_FORMAT_B8G8R8A8_SRGB) ||
              (tiledFormat == MESA_FORMAT_B8G8R8X8_SRGB)) {
      *cpp = 4;
      if (format == GL_BGRA) {
         return ISL_MEMCPY;
      } else if (format == GL_RGBA) {
         return ISL_MEMCPY_BGRA8;
      }
   } else if ((tiledFormat == MESA_FORMAT_R8G8B8A8_UNORM) ||
              (tiledFormat == MESA_FORMAT_R8G8B8X8_UNORM) ||
              (tiledFormat == MESA_FORMAT_R8G8B8A8_SRGB) ||
              (tiledFormat == MESA_FORMAT_R8G8B8X8_SRGB)) {
      *cpp = 4;
      if (format == GL_BGRA) {
         /* Copying from RGBA to BGRA is the same as BGRA to RGBA so we can
          * use the same function.
          */
         return ISL_MEMCPY_BGRA8;
      } else if (format == GL_RGBA) {
         return ISL_MEMCPY;
      }
   }

   return ISL_MEMCPY_INVALID;
}

static void
brw_miptree_map_tiled_memcpy(struct brw_context *brw,
                             struct brw_mipmap_tree *mt,
                             struct brw_miptree_map *map,
                             unsigned int level, unsigned int slice)
{
   brw_miptree_access_raw(brw, mt, level, slice,
                          map->mode & GL_MAP_WRITE_BIT);

   unsigned int x1, x2, y1, y2;
   tile_extents(mt, map, level, slice, &x1, &x2, &y1, &y2);
   map->stride = ALIGN(_mesa_format_row_stride(mt->format, map->w), 16);

   /* The tiling and detiling functions require that the linear buffer
    * has proper 16-byte alignment (that is, its `x0` is 16-byte
    * aligned). Here we over-allocate the linear buffer by enough
    * bytes to get the proper alignment.
    */
   map->buffer = align_malloc(map->stride * (y2 - y1) + (x1 & 0xf), 16);
   map->ptr = (char *)map->buffer + (x1 & 0xf);
   assert(map->buffer);

   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      char *src = brw_miptree_map_raw(brw, mt, map->mode | MAP_RAW);
      src += mt->offset;

      const isl_memcpy_type copy_type =
#if defined(USE_SSE41)
         cpu_has_sse4_1 ? ISL_MEMCPY_STREAMING_LOAD :
#endif
         ISL_MEMCPY;

      isl_memcpy_tiled_to_linear(
         x1, x2, y1, y2, map->ptr, src, map->stride,
         mt->surf.row_pitch_B, brw->has_swizzling, mt->surf.tiling,
         copy_type);

      brw_miptree_unmap_raw(mt);
   }

   map->unmap = brw_miptree_unmap_tiled_memcpy;
}

static void
brw_miptree_map_blit(struct brw_context *brw,
                     struct brw_mipmap_tree *mt,
                     struct brw_miptree_map *map,
                     unsigned int level, unsigned int slice)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   map->linear_mt = make_surface(brw, GL_TEXTURE_2D, mt->format,
                                 0, 0, map->w, map->h, 1, 1,
                                 ISL_TILING_LINEAR_BIT,
                                 ISL_SURF_USAGE_RENDER_TARGET_BIT |
                                 ISL_SURF_USAGE_TEXTURE_BIT,
                                 0, 0, NULL);

   if (!map->linear_mt) {
      fprintf(stderr, "Failed to allocate blit temporary\n");
      goto fail;
   }
   map->stride = map->linear_mt->surf.row_pitch_B;

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      if (devinfo->ver >= 6) {
         brw_blorp_copy_miptrees(brw, mt, level, slice,
                                 map->linear_mt, 0, 0,
                                 map->x, map->y, 0, 0, map->w, map->h);
      } else {
         if (!brw_miptree_copy(brw,
                                 mt, level, slice, map->x, map->y,
                                 map->linear_mt, 0, 0, 0, 0,
                                 map->w, map->h)) {
            fprintf(stderr, "Failed to blit\n");
            goto fail;
         }
      }
   }

   map->ptr = brw_miptree_map_raw(brw, map->linear_mt, map->mode);

   DBG("%s: %d,%d %dx%d from mt %p (%s) %d,%d = %p/%d\n", __func__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       level, slice, map->ptr, map->stride);

   map->unmap = brw_miptree_unmap_blit;
   return;

fail:
   brw_miptree_release(&map->linear_mt);
   map->ptr = NULL;
   map->stride = 0;
}

/**
 * "Map" a buffer by copying it to an untiled temporary using MOVNTDQA.
 */
#if defined(USE_SSE41)
static void
brw_miptree_unmap_movntdqa(struct brw_context *brw,
                           struct brw_mipmap_tree *mt,
                           struct brw_miptree_map *map,
                           unsigned int level,
                           unsigned int slice)
{
   align_free(map->buffer);
   map->buffer = NULL;
   map->ptr = NULL;
}

static void
brw_miptree_map_movntdqa(struct brw_context *brw,
                         struct brw_mipmap_tree *mt,
                         struct brw_miptree_map *map,
                         unsigned int level, unsigned int slice)
{
   assert(map->mode & GL_MAP_READ_BIT);
   assert(!(map->mode & GL_MAP_WRITE_BIT));

   brw_miptree_access_raw(brw, mt, level, slice, false);

   DBG("%s: %d,%d %dx%d from mt %p (%s) %d,%d = %p/%d\n", __func__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       level, slice, map->ptr, map->stride);

   /* Map the original image */
   uint32_t image_x;
   uint32_t image_y;
   brw_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);
   image_x += map->x;
   image_y += map->y;

   void *src = brw_miptree_map_raw(brw, mt, map->mode);
   if (!src)
      return;

   src += mt->offset;

   src += image_y * mt->surf.row_pitch_B;
   src += image_x * mt->cpp;

   /* Due to the pixel offsets for the particular image being mapped, our
    * src pointer may not be 16-byte aligned.  However, if the pitch is
    * divisible by 16, then the amount by which it's misaligned will remain
    * consistent from row to row.
    */
   assert((mt->surf.row_pitch_B % 16) == 0);
   const int misalignment = ((uintptr_t) src) & 15;

   /* Create an untiled temporary buffer for the mapping. */
   const unsigned width_bytes = _mesa_format_row_stride(mt->format, map->w);

   map->stride = ALIGN(misalignment + width_bytes, 16);

   map->buffer = align_malloc(map->stride * map->h, 16);
   /* Offset the destination so it has the same misalignment as src. */
   map->ptr = map->buffer + misalignment;

   assert((((uintptr_t) map->ptr) & 15) == misalignment);

   for (uint32_t y = 0; y < map->h; y++) {
      void *dst_ptr = map->ptr + y * map->stride;
      void *src_ptr = src + y * mt->surf.row_pitch_B;

      _mesa_streaming_load_memcpy(dst_ptr, src_ptr, width_bytes);
   }

   brw_miptree_unmap_raw(mt);

   map->unmap = brw_miptree_unmap_movntdqa;
}
#endif

static void
brw_miptree_unmap_s8(struct brw_context *brw,
                     struct brw_mipmap_tree *mt,
                     struct brw_miptree_map *map,
                     unsigned int level,
                     unsigned int slice)
{
   if (map->mode & GL_MAP_WRITE_BIT) {
      unsigned int image_x, image_y;
      uint8_t *untiled_s8_map = map->ptr;
      uint8_t *tiled_s8_map = brw_miptree_map_raw(brw, mt, GL_MAP_WRITE_BIT);

      brw_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);

      for (uint32_t y = 0; y < map->h; y++) {
         for (uint32_t x = 0; x < map->w; x++) {
            ptrdiff_t offset = brw_offset_S8(mt->surf.row_pitch_B,
                                             image_x + x + map->x,
                                             image_y + y + map->y,
                                             brw->has_swizzling);
            tiled_s8_map[offset] = untiled_s8_map[y * map->w + x];
         }
      }

      brw_miptree_unmap_raw(mt);
   }

   free(map->buffer);
}

static void
brw_miptree_map_s8(struct brw_context *brw,
                   struct brw_mipmap_tree *mt,
                   struct brw_miptree_map *map,
                   unsigned int level, unsigned int slice)
{
   map->stride = map->w;
   map->buffer = map->ptr = malloc(map->stride * map->h);
   if (!map->buffer)
      return;

   brw_miptree_access_raw(brw, mt, level, slice,
                          map->mode & GL_MAP_WRITE_BIT);

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      uint8_t *untiled_s8_map = map->ptr;
      uint8_t *tiled_s8_map = brw_miptree_map_raw(brw, mt, GL_MAP_READ_BIT);
      unsigned int image_x, image_y;

      brw_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);

      for (uint32_t y = 0; y < map->h; y++) {
         for (uint32_t x = 0; x < map->w; x++) {
            ptrdiff_t offset = brw_offset_S8(mt->surf.row_pitch_B,
                                             x + image_x + map->x,
                                             y + image_y + map->y,
                                             brw->has_swizzling);
            untiled_s8_map[y * map->w + x] = tiled_s8_map[offset];
         }
      }

      brw_miptree_unmap_raw(mt);

      DBG("%s: %d,%d %dx%d from mt %p %d,%d = %p/%d\n", __func__,
          map->x, map->y, map->w, map->h,
          mt, map->x + image_x, map->y + image_y, map->ptr, map->stride);
   } else {
      DBG("%s: %d,%d %dx%d from mt %p = %p/%d\n", __func__,
          map->x, map->y, map->w, map->h,
          mt, map->ptr, map->stride);
   }

   map->unmap = brw_miptree_unmap_s8;
}

/**
 * Mapping functions for packed depth/stencil miptrees backed by real separate
 * miptrees for depth and stencil.
 *
 * On gfx7, and to support HiZ pre-gfx7, we have to have the stencil buffer
 * separate from the depth buffer.  Yet at the GL API level, we have to expose
 * packed depth/stencil textures and FBO attachments, and Mesa core expects to
 * be able to map that memory for texture storage and glReadPixels-type
 * operations.  We give Mesa core that access by mallocing a temporary and
 * copying the data between the actual backing store and the temporary.
 */
static void
brw_miptree_unmap_depthstencil(struct brw_context *brw,
                               struct brw_mipmap_tree *mt,
                               struct brw_miptree_map *map,
                               unsigned int level,
                               unsigned int slice)
{
   struct brw_mipmap_tree *z_mt = mt;
   struct brw_mipmap_tree *s_mt = mt->stencil_mt;
   bool map_z32f_x24s8 = mt->format == MESA_FORMAT_Z_FLOAT32;

   if (map->mode & GL_MAP_WRITE_BIT) {
      uint32_t *packed_map = map->ptr;
      uint8_t *s_map = brw_miptree_map_raw(brw, s_mt, GL_MAP_WRITE_BIT);
      uint32_t *z_map = brw_miptree_map_raw(brw, z_mt, GL_MAP_WRITE_BIT);
      unsigned int s_image_x, s_image_y;
      unsigned int z_image_x, z_image_y;

      brw_miptree_get_image_offset(s_mt, level, slice,
                                   &s_image_x, &s_image_y);
      brw_miptree_get_image_offset(z_mt, level, slice,
                                   &z_image_x, &z_image_y);

      for (uint32_t y = 0; y < map->h; y++) {
         for (uint32_t x = 0; x < map->w; x++) {
            ptrdiff_t s_offset = brw_offset_S8(s_mt->surf.row_pitch_B,
                                               x + s_image_x + map->x,
                                               y + s_image_y + map->y,
                                               brw->has_swizzling);
            ptrdiff_t z_offset = ((y + z_image_y + map->y) *
                                  (z_mt->surf.row_pitch_B / 4) +
                                  (x + z_image_x + map->x));

            if (map_z32f_x24s8) {
               z_map[z_offset] = packed_map[(y * map->w + x) * 2 + 0];
               s_map[s_offset] = packed_map[(y * map->w + x) * 2 + 1];
            } else {
               uint32_t packed = packed_map[y * map->w + x];
               s_map[s_offset] = packed >> 24;
               z_map[z_offset] = packed;
            }
         }
      }

      brw_miptree_unmap_raw(s_mt);
      brw_miptree_unmap_raw(z_mt);

      DBG("%s: %d,%d %dx%d from z mt %p (%s) %d,%d, s mt %p %d,%d = %p/%d\n",
          __func__,
          map->x, map->y, map->w, map->h,
          z_mt, _mesa_get_format_name(z_mt->format),
          map->x + z_image_x, map->y + z_image_y,
          s_mt, map->x + s_image_x, map->y + s_image_y,
          map->ptr, map->stride);
   }

   free(map->buffer);
}

static void
brw_miptree_map_depthstencil(struct brw_context *brw,
                             struct brw_mipmap_tree *mt,
                             struct brw_miptree_map *map,
                             unsigned int level, unsigned int slice)
{
   struct brw_mipmap_tree *z_mt = mt;
   struct brw_mipmap_tree *s_mt = mt->stencil_mt;
   bool map_z32f_x24s8 = mt->format == MESA_FORMAT_Z_FLOAT32;
   int packed_bpp = map_z32f_x24s8 ? 8 : 4;

   map->stride = map->w * packed_bpp;
   map->buffer = map->ptr = malloc(map->stride * map->h);
   if (!map->buffer)
      return;

   brw_miptree_access_raw(brw, z_mt, level, slice,
                          map->mode & GL_MAP_WRITE_BIT);
   brw_miptree_access_raw(brw, s_mt, level, slice,
                          map->mode & GL_MAP_WRITE_BIT);

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      uint32_t *packed_map = map->ptr;
      uint8_t *s_map = brw_miptree_map_raw(brw, s_mt, GL_MAP_READ_BIT);
      uint32_t *z_map = brw_miptree_map_raw(brw, z_mt, GL_MAP_READ_BIT);
      unsigned int s_image_x, s_image_y;
      unsigned int z_image_x, z_image_y;

      brw_miptree_get_image_offset(s_mt, level, slice,
                                   &s_image_x, &s_image_y);
      brw_miptree_get_image_offset(z_mt, level, slice,
                                   &z_image_x, &z_image_y);

      for (uint32_t y = 0; y < map->h; y++) {
         for (uint32_t x = 0; x < map->w; x++) {
            int map_x = map->x + x, map_y = map->y + y;
            ptrdiff_t s_offset = brw_offset_S8(s_mt->surf.row_pitch_B,
                                                 map_x + s_image_x,
                                                 map_y + s_image_y,
                                                 brw->has_swizzling);
            ptrdiff_t z_offset = ((map_y + z_image_y) *
                                  (z_mt->surf.row_pitch_B / 4) +
                                  (map_x + z_image_x));
            uint8_t s = s_map[s_offset];
            uint32_t z = z_map[z_offset];

            if (map_z32f_x24s8) {
               packed_map[(y * map->w + x) * 2 + 0] = z;
               packed_map[(y * map->w + x) * 2 + 1] = s;
            } else {
               packed_map[y * map->w + x] = (s << 24) | (z & 0x00ffffff);
            }
         }
      }

      brw_miptree_unmap_raw(s_mt);
      brw_miptree_unmap_raw(z_mt);

      DBG("%s: %d,%d %dx%d from z mt %p %d,%d, s mt %p %d,%d = %p/%d\n",
          __func__,
          map->x, map->y, map->w, map->h,
          z_mt, map->x + z_image_x, map->y + z_image_y,
          s_mt, map->x + s_image_x, map->y + s_image_y,
          map->ptr, map->stride);
   } else {
      DBG("%s: %d,%d %dx%d from mt %p = %p/%d\n", __func__,
          map->x, map->y, map->w, map->h,
          mt, map->ptr, map->stride);
   }

   map->unmap = brw_miptree_unmap_depthstencil;
}

/**
 * Create and attach a map to the miptree at (level, slice). Return the
 * attached map.
 */
static struct brw_miptree_map*
brw_miptree_attach_map(struct brw_mipmap_tree *mt,
                       unsigned int level,
                       unsigned int slice,
                       unsigned int x,
                       unsigned int y,
                       unsigned int w,
                       unsigned int h,
                       GLbitfield mode)
{
   struct brw_miptree_map *map = calloc(1, sizeof(*map));

   if (!map)
      return NULL;

   assert(mt->level[level].slice[slice].map == NULL);
   mt->level[level].slice[slice].map = map;

   map->mode = mode;
   map->x = x;
   map->y = y;
   map->w = w;
   map->h = h;

   return map;
}

/**
 * Release the map at (level, slice).
 */
static void
brw_miptree_release_map(struct brw_mipmap_tree *mt,
                         unsigned int level,
                         unsigned int slice)
{
   struct brw_miptree_map **map;

   map = &mt->level[level].slice[slice].map;
   free(*map);
   *map = NULL;
}

static bool
can_blit_slice(struct brw_mipmap_tree *mt,
               const struct brw_miptree_map *map)
{
   /* See brw_miptree_blit() for details on the 32k pitch limit. */
   const unsigned src_blt_pitch = brw_miptree_blt_pitch(mt);
   const unsigned dst_blt_pitch = ALIGN(map->w * mt->cpp, 64);
   return src_blt_pitch < 32768 && dst_blt_pitch < 32768;
}

static bool
use_blitter_to_map(struct brw_context *brw,
                   struct brw_mipmap_tree *mt,
                   const struct brw_miptree_map *map)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->has_llc &&
      /* It's probably not worth swapping to the blit ring because of
       * all the overhead involved.
       */
       !(map->mode & GL_MAP_WRITE_BIT) &&
       !mt->compressed &&
       (mt->surf.tiling == ISL_TILING_X ||
        /* Prior to Sandybridge, the blitter can't handle Y tiling */
        (devinfo->ver >= 6 && mt->surf.tiling == ISL_TILING_Y0) ||
        /* Fast copy blit on skl+ supports all tiling formats. */
        devinfo->ver >= 9) &&
       can_blit_slice(mt, map))
      return true;

   if (mt->surf.tiling != ISL_TILING_LINEAR &&
       mt->bo->size >= brw->max_gtt_map_object_size) {
      assert(can_blit_slice(mt, map));
      return true;
   }

   return false;
}

/**
 * Parameter \a out_stride has type ptrdiff_t not because the buffer stride may
 * exceed 32 bits but to diminish the likelihood subtle bugs in pointer
 * arithmetic overflow.
 *
 * If you call this function and use \a out_stride, then you're doing pointer
 * arithmetic on \a out_ptr. The type of \a out_stride doesn't prevent all
 * bugs.  The caller must still take care to avoid 32-bit overflow errors in
 * all arithmetic expressions that contain buffer offsets and pixel sizes,
 * which usually have type uint32_t or GLuint.
 */
void
brw_miptree_map(struct brw_context *brw,
                struct brw_mipmap_tree *mt,
                unsigned int level,
                unsigned int slice,
                unsigned int x,
                unsigned int y,
                unsigned int w,
                unsigned int h,
                GLbitfield mode,
                void **out_ptr,
                ptrdiff_t *out_stride)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_miptree_map *map;

   assert(mt->surf.samples == 1);

   map = brw_miptree_attach_map(mt, level, slice, x, y, w, h, mode);
   if (!map){
      *out_ptr = NULL;
      *out_stride = 0;
      return;
   }

   if (mt->format == MESA_FORMAT_S_UINT8) {
      brw_miptree_map_s8(brw, mt, map, level, slice);
   } else if (mt->stencil_mt && !(mode & BRW_MAP_DIRECT_BIT)) {
      brw_miptree_map_depthstencil(brw, mt, map, level, slice);
   } else if (use_blitter_to_map(brw, mt, map)) {
      brw_miptree_map_blit(brw, mt, map, level, slice);
   } else if (mt->surf.tiling != ISL_TILING_LINEAR && devinfo->ver > 4) {
      brw_miptree_map_tiled_memcpy(brw, mt, map, level, slice);
#if defined(USE_SSE41)
   } else if (!(mode & GL_MAP_WRITE_BIT) &&
              !mt->compressed && cpu_has_sse4_1 &&
              (mt->surf.row_pitch_B % 16 == 0)) {
      brw_miptree_map_movntdqa(brw, mt, map, level, slice);
#endif
   } else {
      if (mt->surf.tiling != ISL_TILING_LINEAR)
         perf_debug("brw_miptree_map: mapping via gtt");
      brw_miptree_map_map(brw, mt, map, level, slice);
   }

   *out_ptr = map->ptr;
   *out_stride = map->stride;

   if (map->ptr == NULL)
      brw_miptree_release_map(mt, level, slice);
}

void
brw_miptree_unmap(struct brw_context *brw,
                  struct brw_mipmap_tree *mt,
                  unsigned int level,
                  unsigned int slice)
{
   struct brw_miptree_map *map = mt->level[level].slice[slice].map;

   assert(mt->surf.samples == 1);

   if (!map)
      return;

   DBG("%s: mt %p (%s) level %d slice %d\n", __func__,
       mt, _mesa_get_format_name(mt->format), level, slice);

   if (map->unmap)
      map->unmap(brw, mt, map, level, slice);

   brw_miptree_release_map(mt, level, slice);
}

enum isl_surf_dim
get_isl_surf_dim(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      return ISL_SURF_DIM_1D;

   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
      return ISL_SURF_DIM_2D;

   case GL_TEXTURE_3D:
      return ISL_SURF_DIM_3D;
   }

   unreachable("Invalid texture target");
}

enum isl_dim_layout
get_isl_dim_layout(const struct intel_device_info *devinfo,
                   enum isl_tiling tiling, GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      return (devinfo->ver >= 9 && tiling == ISL_TILING_LINEAR ?
              ISL_DIM_LAYOUT_GFX9_1D : ISL_DIM_LAYOUT_GFX4_2D);

   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
      return ISL_DIM_LAYOUT_GFX4_2D;

   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      return (devinfo->ver == 4 ? ISL_DIM_LAYOUT_GFX4_3D :
              ISL_DIM_LAYOUT_GFX4_2D);

   case GL_TEXTURE_3D:
      return (devinfo->ver >= 9 ?
              ISL_DIM_LAYOUT_GFX4_2D : ISL_DIM_LAYOUT_GFX4_3D);
   }

   unreachable("Invalid texture target");
}

bool
brw_miptree_set_clear_color(struct brw_context *brw,
                            struct brw_mipmap_tree *mt,
                            union isl_color_value clear_color)
{
   if (memcmp(&mt->fast_clear_color, &clear_color, sizeof(clear_color)) != 0) {
      mt->fast_clear_color = clear_color;
      if (mt->aux_buf->clear_color_bo) {
         /* We can't update the clear color while the hardware is still using
          * the previous one for a resolve or sampling from it. Make sure that
          * there are no pending commands at this point.
          */
         brw_emit_pipe_control_flush(brw, PIPE_CONTROL_CS_STALL);
         for (int i = 0; i < 4; i++) {
            brw_store_data_imm32(brw, mt->aux_buf->clear_color_bo,
                                 mt->aux_buf->clear_color_offset + i * 4,
                                 mt->fast_clear_color.u32[i]);
         }
         brw_emit_pipe_control_flush(brw, PIPE_CONTROL_STATE_CACHE_INVALIDATE);
      }
      brw->ctx.NewDriverState |= BRW_NEW_AUX_STATE;
      return true;
   }
   return false;
}

union isl_color_value
brw_miptree_get_clear_color(const struct brw_mipmap_tree *mt,
                            struct brw_bo **clear_color_bo,
                            uint64_t *clear_color_offset)
{
   assert(mt->aux_buf);

   *clear_color_bo = mt->aux_buf->clear_color_bo;
   *clear_color_offset = mt->aux_buf->clear_color_offset;
   return mt->fast_clear_color;
}

static void
brw_miptree_update_etc_shadow(struct brw_context *brw,
                              struct brw_mipmap_tree *mt,
                              unsigned int level,
                              unsigned int slice,
                              int level_w,
                              int level_h)
{
   ptrdiff_t etc_stride, shadow_stride;
   void *mptr, *sptr;
   struct brw_mipmap_tree *smt = mt->shadow_mt;

   assert(brw_miptree_has_etc_shadow(brw, mt));

   brw_miptree_map(brw, mt, level, slice, 0, 0, level_w, level_h,
                   GL_MAP_READ_BIT, &mptr, &etc_stride);
   brw_miptree_map(brw, smt, level, slice, 0, 0, level_w, level_h,
                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT,
                   &sptr, &shadow_stride);

   if (mt->format == MESA_FORMAT_ETC1_RGB8) {
      _mesa_etc1_unpack_rgba8888(sptr, shadow_stride, mptr, etc_stride,
                                 level_w, level_h);
   } else {
      /* destination and source images must have the same swizzle */
      bool is_bgra = (smt->format == MESA_FORMAT_B8G8R8A8_SRGB);
      _mesa_unpack_etc2_format(sptr, shadow_stride, mptr, etc_stride,
                               level_w, level_h, mt->format, is_bgra);
   }

   brw_miptree_unmap(brw, mt, level, slice);
   brw_miptree_unmap(brw, smt, level, slice);
}

void
brw_miptree_update_etc_shadow_levels(struct brw_context *brw,
                                     struct brw_mipmap_tree *mt)
{
   struct brw_mipmap_tree *smt;
   int num_slices;

   assert(mt);
   assert(mt->surf.size_B > 0);
   assert(brw_miptree_has_etc_shadow(brw, mt));

   smt = mt->shadow_mt;
   num_slices = smt->surf.logical_level0_px.array_len;

   for (int level = smt->first_level; level <= smt->last_level; level++) {
      int level_w = minify(smt->surf.logical_level0_px.width,
                           level - smt->first_level);
      int level_h = minify(smt->surf.logical_level0_px.height,
                           level - smt->first_level);

      for (unsigned int slice = 0; slice < num_slices; slice++) {
         brw_miptree_update_etc_shadow(brw, mt, level, slice, level_w,
                                       level_h);
      }
   }

   mt->shadow_needs_update = false;
}
