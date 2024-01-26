/*
 * Copyright 2003 VMware, Inc.
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

#include "drm-uapi/drm_fourcc.h"
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/texobj.h"
#include "main/hash.h"
#include "main/fbobject.h"
#include "main/version.h"
#include "main/glthread.h"
#include "swrast/s_renderbuffer.h"
#include "util/ralloc.h"
#include "util/disk_cache.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "compiler/nir/nir.h"

#include "utils.h"
#include "util/disk_cache.h"
#include "util/driconf.h"
#include "util/u_cpu_detect.h"
#include "util/u_memory.h"

#include "common/intel_defines.h"

static const driOptionDescription brw_driconf[] = {
   DRI_CONF_SECTION_PERFORMANCE
      /* Options correspond to DRI_CONF_BO_REUSE_DISABLED,
       * DRI_CONF_BO_REUSE_ALL
       */
      DRI_CONF_OPT_E(bo_reuse, 1, 0, 1,
                     "Buffer object reuse",
                     DRI_CONF_ENUM(0, "Disable buffer object reuse")
                     DRI_CONF_ENUM(1, "Enable reuse of all sizes of buffer objects"))
      DRI_CONF_MESA_NO_ERROR(false)
      DRI_CONF_MESA_GLTHREAD(false)
   DRI_CONF_SECTION_END

   DRI_CONF_SECTION_QUALITY
      DRI_CONF_PRECISE_TRIG(false)

      DRI_CONF_OPT_I(clamp_max_samples, -1, 0, 0,
                     "Clamp the value of GL_MAX_SAMPLES to the "
                     "given integer. If negative, then do not clamp.")
   DRI_CONF_SECTION_END

   DRI_CONF_SECTION_DEBUG
      DRI_CONF_ALWAYS_FLUSH_BATCH(false)
      DRI_CONF_ALWAYS_FLUSH_CACHE(false)
      DRI_CONF_DISABLE_THROTTLING(false)
      DRI_CONF_FORCE_GLSL_EXTENSIONS_WARN(false)
      DRI_CONF_FORCE_GLSL_VERSION(0)
      DRI_CONF_DISABLE_GLSL_LINE_CONTINUATIONS(false)
      DRI_CONF_DISABLE_BLEND_FUNC_EXTENDED(false)
      DRI_CONF_DUAL_COLOR_BLEND_BY_LOCATION(false)
      DRI_CONF_ALLOW_EXTRA_PP_TOKENS(false)
      DRI_CONF_ALLOW_GLSL_EXTENSION_DIRECTIVE_MIDSHADER(false)
      DRI_CONF_ALLOW_GLSL_BUILTIN_VARIABLE_REDECLARATION(false)
      DRI_CONF_ALLOW_GLSL_CROSS_STAGE_INTERPOLATION_MISMATCH(false)
      DRI_CONF_ALLOW_HIGHER_COMPAT_VERSION(false)
      DRI_CONF_FORCE_COMPAT_PROFILE(false)
      DRI_CONF_FORCE_GLSL_ABS_SQRT(false)
      DRI_CONF_FORCE_GL_VENDOR()

      DRI_CONF_OPT_B(shader_precompile, true, "Perform code generation at shader link time.")
   DRI_CONF_SECTION_END

   DRI_CONF_SECTION_MISCELLANEOUS
      DRI_CONF_GLSL_ZERO_INIT(false)
      DRI_CONF_VS_POSITION_ALWAYS_INVARIANT(false)
      DRI_CONF_VS_POSITION_ALWAYS_PRECISE(false)
      DRI_CONF_ALLOW_RGB10_CONFIGS(false)
      DRI_CONF_ALLOW_RGB565_CONFIGS(true)
   DRI_CONF_SECTION_END
};

static char *
brw_driconf_get_xml(UNUSED const char *driver_name)
{
   return driGetOptionsXml(brw_driconf, ARRAY_SIZE(brw_driconf));
}

static const __DRIconfigOptionsExtension brw_config_options = {
   .base = { __DRI_CONFIG_OPTIONS, 2 },
   .xml = NULL,
   .getXml = brw_driconf_get_xml,
};

#include "brw_batch.h"
#include "brw_buffers.h"
#include "brw_bufmgr.h"
#include "brw_fbo.h"
#include "brw_mipmap_tree.h"
#include "brw_screen.h"
#include "brw_tex.h"
#include "brw_image.h"

#include "brw_context.h"

#include "drm-uapi/i915_drm.h"

/**
 * For debugging purposes, this returns a time in seconds.
 */
double
get_time(void)
{
   struct timespec tp;

   clock_gettime(CLOCK_MONOTONIC, &tp);

   return tp.tv_sec + tp.tv_nsec / 1000000000.0;
}

static const __DRItexBufferExtension brwTexBufferExtension = {
   .base = { __DRI_TEX_BUFFER, 3 },

   .setTexBuffer        = brw_set_texbuffer,
   .setTexBuffer2       = brw_set_texbuffer2,
   .releaseTexBuffer    = brw_release_texbuffer,
};

static void
brw_dri2_flush_with_flags(__DRIcontext *cPriv,
                            __DRIdrawable *dPriv,
                            unsigned flags,
                            enum __DRI2throttleReason reason)
{
   struct brw_context *brw = cPriv->driverPrivate;

   if (!brw)
      return;

   struct gl_context *ctx = &brw->ctx;

   _mesa_glthread_finish(ctx);

   FLUSH_VERTICES(ctx, 0, 0);

   if (flags & __DRI2_FLUSH_DRAWABLE)
      brw_resolve_for_dri2_flush(brw, dPriv);

   if (reason == __DRI2_THROTTLE_SWAPBUFFER)
      brw->need_swap_throttle = true;
   if (reason == __DRI2_THROTTLE_FLUSHFRONT)
      brw->need_flush_throttle = true;

   brw_batch_flush(brw);
}

/**
 * Provides compatibility with loaders that only support the older (version
 * 1-3) flush interface.
 *
 * That includes libGL up to Mesa 9.0, and the X Server at least up to 1.13.
 */
static void
brw_dri2_flush(__DRIdrawable *drawable)
{
   brw_dri2_flush_with_flags(drawable->driContextPriv, drawable,
                               __DRI2_FLUSH_DRAWABLE,
                               __DRI2_THROTTLE_SWAPBUFFER);
}

static const struct __DRI2flushExtensionRec brwFlushExtension = {
    .base = { __DRI2_FLUSH, 4 },

    .flush              = brw_dri2_flush,
    .invalidate         = dri2InvalidateDrawable,
    .flush_with_flags   = brw_dri2_flush_with_flags,
};

static const struct brw_image_format brw_image_formats[] = {
   { DRM_FORMAT_ABGR16161616F, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ABGR16161616F, 8 } } },

   { DRM_FORMAT_XBGR16161616F, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XBGR16161616F, 8 } } },

   { DRM_FORMAT_ARGB2101010, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ARGB2101010, 4 } } },

   { DRM_FORMAT_XRGB2101010, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XRGB2101010, 4 } } },

   { DRM_FORMAT_ABGR2101010, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ABGR2101010, 4 } } },

   { DRM_FORMAT_XBGR2101010, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XBGR2101010, 4 } } },

   { DRM_FORMAT_ARGB8888, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ARGB8888, 4 } } },

   { DRM_FORMAT_ABGR8888, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ABGR8888, 4 } } },

   { __DRI_IMAGE_FOURCC_SARGB8888, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_SARGB8, 4 } } },

   { __DRI_IMAGE_FOURCC_SXRGB8888, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_SXRGB8, 4 } } },

   { DRM_FORMAT_XRGB8888, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XRGB8888, 4 }, } },

   { DRM_FORMAT_XBGR8888, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XBGR8888, 4 }, } },

   { DRM_FORMAT_ARGB1555, __DRI_IMAGE_COMPONENTS_RGBA, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ARGB1555, 2 } } },

   { DRM_FORMAT_RGB565, __DRI_IMAGE_COMPONENTS_RGB, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_RGB565, 2 } } },

   { DRM_FORMAT_R8, __DRI_IMAGE_COMPONENTS_R, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 }, } },

   { DRM_FORMAT_R16, __DRI_IMAGE_COMPONENTS_R, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R16, 1 }, } },

   { DRM_FORMAT_GR88, __DRI_IMAGE_COMPONENTS_RG, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88, 2 }, } },

   { DRM_FORMAT_GR1616, __DRI_IMAGE_COMPONENTS_RG, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR1616, 2 }, } },

   { DRM_FORMAT_YUV410, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YUV411, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YUV420, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YUV422, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YUV444, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YVU410, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 2, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YVU411, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 2, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YVU420, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YVU422, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_YVU444, __DRI_IMAGE_COMPONENTS_Y_U_V, 3,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 2, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 } } },

   { DRM_FORMAT_NV12, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_GR88, 2 } } },

   { DRM_FORMAT_P010, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R16, 2 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_GR1616, 4 } } },

   { DRM_FORMAT_P012, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R16, 2 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_GR1616, 4 } } },

   { DRM_FORMAT_P016, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R16, 2 },
       { 1, 1, 1, __DRI_IMAGE_FORMAT_GR1616, 4 } } },

   { DRM_FORMAT_NV16, __DRI_IMAGE_COMPONENTS_Y_UV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8, 1 },
       { 1, 1, 0, __DRI_IMAGE_FORMAT_GR88, 2 } } },

   { DRM_FORMAT_AYUV, __DRI_IMAGE_COMPONENTS_AYUV, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_ABGR8888, 4 } } },

   { DRM_FORMAT_XYUV8888, __DRI_IMAGE_COMPONENTS_XYUV, 1,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_XBGR8888, 4 } } },

   /* For YUYV and UYVY buffers, we set up two overlapping DRI images
    * and treat them as planar buffers in the compositors.
    * Plane 0 is GR88 and samples YU or YV pairs and places Y into
    * the R component, while plane 1 is ARGB/ABGR and samples YUYV/UYVY
    * clusters and places pairs and places U into the G component and
    * V into A.  This lets the texture sampler interpolate the Y
    * components correctly when sampling from plane 0, and interpolate
    * U and V correctly when sampling from plane 1. */
   { DRM_FORMAT_YUYV, __DRI_IMAGE_COMPONENTS_Y_XUXV, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88, 2 },
       { 0, 1, 0, __DRI_IMAGE_FORMAT_ARGB8888, 4 } } },
   { DRM_FORMAT_UYVY, __DRI_IMAGE_COMPONENTS_Y_UXVX, 2,
     { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88, 2 },
       { 0, 1, 0, __DRI_IMAGE_FORMAT_ABGR8888, 4 } } }
};

static const struct {
   uint64_t modifier;
   unsigned since_ver;
} supported_modifiers[] = {
   { .modifier = DRM_FORMAT_MOD_LINEAR       , .since_ver = 1 },
   { .modifier = I915_FORMAT_MOD_X_TILED     , .since_ver = 1 },
   { .modifier = I915_FORMAT_MOD_Y_TILED     , .since_ver = 6 },
   { .modifier = I915_FORMAT_MOD_Y_TILED_CCS , .since_ver = 9 },
};

static bool
modifier_is_supported(const struct intel_device_info *devinfo,
                      const struct brw_image_format *fmt, int dri_format,
                      unsigned use, uint64_t modifier)
{
   const struct isl_drm_modifier_info *modinfo =
      isl_drm_modifier_get_info(modifier);
   int i;

   /* ISL had better know about the modifier */
   if (!modinfo)
      return false;

   if (devinfo->ver < 9 && (use & __DRI_IMAGE_USE_SCANOUT) &&
       !(modinfo->tiling == ISL_TILING_LINEAR ||
         modinfo->tiling == ISL_TILING_X))
      return false;

   if (modinfo->aux_usage == ISL_AUX_USAGE_CCS_E) {
      /* If INTEL_DEBUG=norbc is set, don't support any CCS_E modifiers */
      if (INTEL_DEBUG(DEBUG_NO_RBC))
         return false;

      /* CCS_E is not supported for planar images */
      if (fmt && fmt->nplanes > 1)
         return false;

      if (fmt) {
         assert(dri_format == 0);
         dri_format = fmt->planes[0].dri_format;
      }

      mesa_format format = driImageFormatToGLFormat(dri_format);
      /* Whether or not we support compression is based on the RGBA non-sRGB
       * version of the format.
       */
      format = _mesa_format_fallback_rgbx_to_rgba(format);
      format = _mesa_get_srgb_format_linear(format);
      if (!isl_format_supports_ccs_e(devinfo,
                                     brw_isl_format_for_mesa_format(format)))
         return false;
   }

   for (i = 0; i < ARRAY_SIZE(supported_modifiers); i++) {
      if (supported_modifiers[i].modifier != modifier)
         continue;

      return supported_modifiers[i].since_ver <= devinfo->ver;
   }

   return false;
}

static uint64_t
tiling_to_modifier(uint32_t tiling)
{
   static const uint64_t map[] = {
      [I915_TILING_NONE]   = DRM_FORMAT_MOD_LINEAR,
      [I915_TILING_X]      = I915_FORMAT_MOD_X_TILED,
      [I915_TILING_Y]      = I915_FORMAT_MOD_Y_TILED,
   };

   assert(tiling < ARRAY_SIZE(map));

   return map[tiling];
}

static void
brw_image_warn_if_unaligned(__DRIimage *image, const char *func)
{
   uint32_t tiling, swizzle;
   brw_bo_get_tiling(image->bo, &tiling, &swizzle);

   if (tiling != I915_TILING_NONE && (image->offset & 0xfff)) {
      _mesa_warning(NULL, "%s: offset 0x%08x not on tile boundary",
                    func, image->offset);
   }
}

static const struct brw_image_format *
brw_image_format_lookup(int fourcc)
{
   for (unsigned i = 0; i < ARRAY_SIZE(brw_image_formats); i++) {
      if (brw_image_formats[i].fourcc == fourcc)
         return &brw_image_formats[i];
   }

   return NULL;
}

static bool
brw_image_get_fourcc(__DRIimage *image, int *fourcc)
{
   if (image->planar_format) {
      *fourcc = image->planar_format->fourcc;
      return true;
   }

   for (unsigned i = 0; i < ARRAY_SIZE(brw_image_formats); i++) {
      if (brw_image_formats[i].planes[0].dri_format == image->dri_format) {
         *fourcc = brw_image_formats[i].fourcc;
         return true;
      }
   }
   return false;
}

static __DRIimage *
brw_allocate_image(struct brw_screen *screen, int dri_format,
                   void *loaderPrivate)
{
    __DRIimage *image;

    image = calloc(1, sizeof *image);
    if (image == NULL)
       return NULL;

    image->screen = screen;
    image->dri_format = dri_format;
    image->offset = 0;

    image->format = driImageFormatToGLFormat(dri_format);
    if (dri_format != __DRI_IMAGE_FORMAT_NONE &&
        image->format == MESA_FORMAT_NONE) {
       free(image);
       return NULL;
    }

    image->internal_format = _mesa_get_format_base_format(image->format);
    image->driScrnPriv = screen->driScrnPriv;
    image->loader_private = loaderPrivate;

    return image;
}

/**
 * Sets up a DRIImage structure to point to a slice out of a miptree.
 */
static void
brw_setup_image_from_mipmap_tree(struct brw_context *brw, __DRIimage *image,
                                 struct brw_mipmap_tree *mt, GLuint level,
                                 GLuint zoffset)
{
   brw_miptree_make_shareable(brw, mt);

   brw_miptree_check_level_layer(mt, level, zoffset);

   image->width = minify(mt->surf.phys_level0_sa.width,
                         level - mt->first_level);
   image->height = minify(mt->surf.phys_level0_sa.height,
                          level - mt->first_level);
   image->pitch = mt->surf.row_pitch_B;

   image->offset = brw_miptree_get_tile_offsets(mt, level, zoffset,
                                                  &image->tile_x,
                                                  &image->tile_y);

   brw_bo_unreference(image->bo);
   image->bo = mt->bo;
   brw_bo_reference(mt->bo);
}

static __DRIimage *
brw_create_image_from_name(__DRIscreen *dri_screen,
                           int width, int height, int format,
                           int name, int pitch, void *loaderPrivate)
{
    struct brw_screen *screen = dri_screen->driverPrivate;
    __DRIimage *image;
    int cpp;

    image = brw_allocate_image(screen, format, loaderPrivate);
    if (image == NULL)
       return NULL;

    if (image->format == MESA_FORMAT_NONE)
       cpp = 1;
    else
       cpp = _mesa_get_format_bytes(image->format);

    image->width = width;
    image->height = height;
    image->pitch = pitch * cpp;
    image->bo = brw_bo_gem_create_from_name(screen->bufmgr, "image",
                                                  name);
    if (!image->bo) {
       free(image);
       return NULL;
    }
    image->modifier = tiling_to_modifier(image->bo->tiling_mode);

    return image;
}

static __DRIimage *
brw_create_image_from_renderbuffer(__DRIcontext *context,
                                   int renderbuffer, void *loaderPrivate)
{
   __DRIimage *image;
   struct brw_context *brw = context->driverPrivate;
   struct gl_context *ctx = &brw->ctx;
   struct gl_renderbuffer *rb;
   struct brw_renderbuffer *irb;

   rb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glRenderbufferExternalMESA");
      return NULL;
   }

   irb = brw_renderbuffer(rb);
   brw_miptree_make_shareable(brw, irb->mt);
   image = calloc(1, sizeof *image);
   if (image == NULL)
      return NULL;

   image->internal_format = rb->InternalFormat;
   image->format = rb->Format;
   image->modifier = tiling_to_modifier(
                        isl_tiling_to_i915_tiling(irb->mt->surf.tiling));
   image->offset = 0;
   image->driScrnPriv = context->driScreenPriv;
   image->loader_private = loaderPrivate;
   brw_bo_unreference(image->bo);
   image->bo = irb->mt->bo;
   brw_bo_reference(irb->mt->bo);
   image->width = rb->Width;
   image->height = rb->Height;
   image->pitch = irb->mt->surf.row_pitch_B;
   image->dri_format = driGLFormatToImageFormat(image->format);
   image->has_depthstencil = irb->mt->stencil_mt? true : false;

   rb->NeedsFinishRenderTexture = true;
   return image;
}

static __DRIimage *
brw_create_image_from_texture(__DRIcontext *context, int target,
                              unsigned texture, int zoffset,
                              int level,
                              unsigned *error,
                              void *loaderPrivate)
{
   __DRIimage *image;
   struct brw_context *brw = context->driverPrivate;
   struct gl_texture_object *obj;
   struct brw_texture_object *iobj;
   GLuint face = 0;

   obj = _mesa_lookup_texture(&brw->ctx, texture);
   if (!obj || obj->Target != target) {
      *error = __DRI_IMAGE_ERROR_BAD_PARAMETER;
      return NULL;
   }

   if (target == GL_TEXTURE_CUBE_MAP)
      face = zoffset;

   _mesa_test_texobj_completeness(&brw->ctx, obj);
   iobj = brw_texture_object(obj);
   if (!obj->_BaseComplete || (level > 0 && !obj->_MipmapComplete)) {
      *error = __DRI_IMAGE_ERROR_BAD_PARAMETER;
      return NULL;
   }

   if (level < obj->Attrib.BaseLevel || level > obj->_MaxLevel) {
      *error = __DRI_IMAGE_ERROR_BAD_MATCH;
      return NULL;
   }

   if (target == GL_TEXTURE_3D && obj->Image[face][level]->Depth < zoffset) {
      *error = __DRI_IMAGE_ERROR_BAD_MATCH;
      return NULL;
   }
   image = calloc(1, sizeof *image);
   if (image == NULL) {
      *error = __DRI_IMAGE_ERROR_BAD_ALLOC;
      return NULL;
   }

   image->internal_format = obj->Image[face][level]->InternalFormat;
   image->format = obj->Image[face][level]->TexFormat;
   image->modifier = tiling_to_modifier(
                        isl_tiling_to_i915_tiling(iobj->mt->surf.tiling));
   image->driScrnPriv = context->driScreenPriv;
   image->loader_private = loaderPrivate;
   brw_setup_image_from_mipmap_tree(brw, image, iobj->mt, level, zoffset);
   image->dri_format = driGLFormatToImageFormat(image->format);
   image->has_depthstencil = iobj->mt->stencil_mt? true : false;
   image->planar_format = iobj->planar_format;
   if (image->dri_format == __DRI_IMAGE_FORMAT_NONE) {
      *error = __DRI_IMAGE_ERROR_BAD_PARAMETER;
      free(image);
      return NULL;
   }

   *error = __DRI_IMAGE_ERROR_SUCCESS;
   return image;
}

static void
brw_destroy_image(__DRIimage *image)
{
   const __DRIscreen * driScreen = image->driScrnPriv;
   const __DRIimageLoaderExtension *imgLoader = driScreen->image.loader;
   const __DRIdri2LoaderExtension *dri2Loader = driScreen->dri2.loader;

   if (imgLoader && imgLoader->base.version >= 4 &&
         imgLoader->destroyLoaderImageState) {
      imgLoader->destroyLoaderImageState(image->loader_private);
   } else if (dri2Loader && dri2Loader->base.version >= 5 &&
         dri2Loader->destroyLoaderImageState) {
      dri2Loader->destroyLoaderImageState(image->loader_private);
   }

   brw_bo_unreference(image->bo);
   free(image);
}

enum modifier_priority {
   MODIFIER_PRIORITY_INVALID = 0,
   MODIFIER_PRIORITY_LINEAR,
   MODIFIER_PRIORITY_X,
   MODIFIER_PRIORITY_Y,
   MODIFIER_PRIORITY_Y_CCS,
};

const uint64_t priority_to_modifier[] = {
   [MODIFIER_PRIORITY_INVALID] = DRM_FORMAT_MOD_INVALID,
   [MODIFIER_PRIORITY_LINEAR] = DRM_FORMAT_MOD_LINEAR,
   [MODIFIER_PRIORITY_X] = I915_FORMAT_MOD_X_TILED,
   [MODIFIER_PRIORITY_Y] = I915_FORMAT_MOD_Y_TILED,
   [MODIFIER_PRIORITY_Y_CCS] = I915_FORMAT_MOD_Y_TILED_CCS,
};

static uint64_t
select_best_modifier(struct intel_device_info *devinfo,
                     int dri_format,
                     unsigned use,
                     const uint64_t *modifiers,
                     const unsigned count)
{
   enum modifier_priority prio = MODIFIER_PRIORITY_INVALID;

   for (int i = 0; i < count; i++) {
      if (!modifier_is_supported(devinfo, NULL, dri_format, use, modifiers[i]))
         continue;

      switch (modifiers[i]) {
      case I915_FORMAT_MOD_Y_TILED_CCS:
         prio = MAX2(prio, MODIFIER_PRIORITY_Y_CCS);
         break;
      case I915_FORMAT_MOD_Y_TILED:
         prio = MAX2(prio, MODIFIER_PRIORITY_Y);
         break;
      case I915_FORMAT_MOD_X_TILED:
         prio = MAX2(prio, MODIFIER_PRIORITY_X);
         break;
      case DRM_FORMAT_MOD_LINEAR:
         prio = MAX2(prio, MODIFIER_PRIORITY_LINEAR);
         break;
      case DRM_FORMAT_MOD_INVALID:
      default:
         break;
      }
   }

   return priority_to_modifier[prio];
}

static __DRIimage *
brw_create_image_common(__DRIscreen *dri_screen,
                        int width, int height, int format,
                        unsigned int use,
                        const uint64_t *modifiers,
                        unsigned count,
                        void *loaderPrivate)
{
   __DRIimage *image;
   struct brw_screen *screen = dri_screen->driverPrivate;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;
   bool ok;

   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (width != 64 || height != 64)
         return NULL;
      modifier = DRM_FORMAT_MOD_LINEAR;
   }

   if (use & __DRI_IMAGE_USE_LINEAR)
      modifier = DRM_FORMAT_MOD_LINEAR;

   if (modifier == DRM_FORMAT_MOD_INVALID) {
      if (modifiers) {
         /* User requested specific modifiers */
         modifier = select_best_modifier(&screen->devinfo, format, use,
                                         modifiers, count);
         if (modifier == DRM_FORMAT_MOD_INVALID)
            return NULL;
      } else {
         /* Historically, X-tiled was the default, and so lack of modifier means
          * X-tiled.
          */
         modifier = I915_FORMAT_MOD_X_TILED;
      }
   }

   image = brw_allocate_image(screen, format, loaderPrivate);
   if (image == NULL)
      return NULL;

   const struct isl_drm_modifier_info *mod_info =
      isl_drm_modifier_get_info(modifier);

   struct isl_surf surf;
   ok = isl_surf_init(&screen->isl_dev, &surf,
                      .dim = ISL_SURF_DIM_2D,
                      .format = brw_isl_format_for_mesa_format(image->format),
                      .width = width,
                      .height = height,
                      .depth = 1,
                      .levels = 1,
                      .array_len = 1,
                      .samples = 1,
                      .usage = ISL_SURF_USAGE_RENDER_TARGET_BIT |
                               ISL_SURF_USAGE_TEXTURE_BIT |
                               ISL_SURF_USAGE_STORAGE_BIT |
                               ((use & __DRI_IMAGE_USE_SCANOUT) ?
                                ISL_SURF_USAGE_DISPLAY_BIT : 0),
                      .tiling_flags = (1 << mod_info->tiling));
   assert(ok);
   if (!ok) {
      free(image);
      return NULL;
   }

   struct isl_surf aux_surf = {0,};
   if (mod_info->aux_usage == ISL_AUX_USAGE_CCS_E) {
      ok = isl_surf_get_ccs_surf(&screen->isl_dev, &surf, NULL, &aux_surf, 0);
      if (!ok) {
         free(image);
         return NULL;
      }
   } else {
      assert(mod_info->aux_usage == ISL_AUX_USAGE_NONE);
      aux_surf.size_B = 0;
   }

   /* We request that the bufmgr zero the buffer for us for two reasons:
    *
    *  1) If a buffer gets re-used from the pool, we don't want to leak random
    *     garbage from our process to some other.
    *
    *  2) For images with CCS_E, we want to ensure that the CCS starts off in
    *     a valid state.  A CCS value of 0 indicates that the given block is
    *     in the pass-through state which is what we want.
    */
   image->bo = brw_bo_alloc_tiled(screen->bufmgr, "image",
                                  surf.size_B + aux_surf.size_B,
                                  BRW_MEMZONE_OTHER,
                                  isl_tiling_to_i915_tiling(mod_info->tiling),
                                  surf.row_pitch_B, BO_ALLOC_ZEROED);
   if (image->bo == NULL) {
      free(image);
      return NULL;
   }
   image->width = width;
   image->height = height;
   image->pitch = surf.row_pitch_B;
   image->modifier = modifier;

   if (aux_surf.size_B) {
      image->aux_offset = surf.size_B;
      image->aux_pitch = aux_surf.row_pitch_B;
      image->aux_size = aux_surf.size_B;
   }

   return image;
}

static __DRIimage *
brw_create_image(__DRIscreen *dri_screen,
                 int width, int height, int format,
                 unsigned int use,
                 void *loaderPrivate)
{
   return brw_create_image_common(dri_screen, width, height, format, use,
                                  NULL, 0, loaderPrivate);
}

static void *
brw_map_image(__DRIcontext *context, __DRIimage *image,
              int x0, int y0, int width, int height,
              unsigned int flags, int *stride, void **map_info)
{
   struct brw_context *brw = NULL;
   struct brw_bo *bo = NULL;
   void *raw_data = NULL;
   GLuint pix_w = 1;
   GLuint pix_h = 1;
   GLint pix_bytes = 1;

   if (!context || !image || !stride || !map_info || *map_info)
      return NULL;

   if (x0 < 0 || x0 >= image->width || width > image->width - x0)
      return NULL;

   if (y0 < 0 || y0 >= image->height || height > image->height - y0)
      return NULL;

   if (flags & MAP_INTERNAL_MASK)
      return NULL;

   brw = context->driverPrivate;
   bo = image->bo;

   assert(brw);
   assert(bo);

   /* DRI flags and GL_MAP.*_BIT flags are the same, so just pass them on. */
   raw_data = brw_bo_map(brw, bo, flags);
   if (!raw_data)
      return NULL;

   _mesa_get_format_block_size(image->format, &pix_w, &pix_h);
   pix_bytes = _mesa_get_format_bytes(image->format);

   assert(pix_w);
   assert(pix_h);
   assert(pix_bytes > 0);

   raw_data += (x0 / pix_w) * pix_bytes + (y0 / pix_h) * image->pitch;

   brw_bo_reference(bo);

   *stride = image->pitch;
   *map_info = bo;

   return raw_data;
}

static void
brw_unmap_image(UNUSED __DRIcontext *context, UNUSED __DRIimage *image,
                void *map_info)
{
   struct brw_bo *bo = map_info;

   brw_bo_unmap(bo);
   brw_bo_unreference(bo);
}

static __DRIimage *
brw_create_image_with_modifiers(__DRIscreen *dri_screen,
                                  int width, int height, int format,
                                  const uint64_t *modifiers,
                                  const unsigned count,
                                  void *loaderPrivate)
{
   return brw_create_image_common(dri_screen, width, height, format, 0,
                                  modifiers, count, loaderPrivate);
}

static __DRIimage *
brw_create_image_with_modifiers2(__DRIscreen *dri_screen,
                                 int width, int height, int format,
                                 const uint64_t *modifiers,
                                 const unsigned count, unsigned int use,
                                 void *loaderPrivate)
{
   return brw_create_image_common(dri_screen, width, height, format, use,
                                  modifiers, count, loaderPrivate);
}

static GLboolean
brw_query_image(__DRIimage *image, int attrib, int *value)
{
   switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      *value = image->pitch;
      return true;
   case __DRI_IMAGE_ATTRIB_HANDLE: {
      __DRIscreen *dri_screen = image->screen->driScrnPriv;
      uint32_t handle;
      if (brw_bo_export_gem_handle_for_device(image->bo,
                                              dri_screen->fd,
                                              &handle))
         return false;
      *value = handle;
      return true;
   }
   case __DRI_IMAGE_ATTRIB_NAME:
      return !brw_bo_flink(image->bo, (uint32_t *) value);
   case __DRI_IMAGE_ATTRIB_FORMAT:
      *value = image->dri_format;
      return true;
   case __DRI_IMAGE_ATTRIB_WIDTH:
      *value = image->width;
      return true;
   case __DRI_IMAGE_ATTRIB_HEIGHT:
      *value = image->height;
      return true;
   case __DRI_IMAGE_ATTRIB_COMPONENTS:
      if (image->planar_format == NULL)
         return false;
      *value = image->planar_format->components;
      return true;
   case __DRI_IMAGE_ATTRIB_FD:
      return !brw_bo_gem_export_to_prime(image->bo, value);
   case __DRI_IMAGE_ATTRIB_FOURCC:
      return brw_image_get_fourcc(image, value);
   case __DRI_IMAGE_ATTRIB_NUM_PLANES:
      if (isl_drm_modifier_has_aux(image->modifier)) {
         assert(!image->planar_format || image->planar_format->nplanes == 1);
         *value = 2;
      } else if (image->planar_format) {
         *value = image->planar_format->nplanes;
      } else {
         *value = 1;
      }
      return true;
   case __DRI_IMAGE_ATTRIB_OFFSET:
      *value = image->offset;
      return true;
   case __DRI_IMAGE_ATTRIB_MODIFIER_LOWER:
      *value = (image->modifier & 0xffffffff);
      return true;
   case __DRI_IMAGE_ATTRIB_MODIFIER_UPPER:
      *value = ((image->modifier >> 32) & 0xffffffff);
      return true;

  default:
      return false;
   }
}

static GLboolean
brw_query_format_modifier_attribs(__DRIscreen *dri_screen,
                                    uint32_t fourcc, uint64_t modifier,
                                    int attrib, uint64_t *value)
{
   struct brw_screen *screen = dri_screen->driverPrivate;
   const struct brw_image_format *f = brw_image_format_lookup(fourcc);

   if (!modifier_is_supported(&screen->devinfo, f, 0, 0, modifier))
      return false;

   switch (attrib) {
   case __DRI_IMAGE_FORMAT_MODIFIER_ATTRIB_PLANE_COUNT:
      *value = isl_drm_modifier_has_aux(modifier) ? 2 : f->nplanes;
      return true;

   default:
      return false;
   }
}

static __DRIimage *
brw_dup_image(__DRIimage *orig_image, void *loaderPrivate)
{
   __DRIimage *image;

   image = calloc(1, sizeof *image);
   if (image == NULL)
      return NULL;

   brw_bo_reference(orig_image->bo);
   image->screen          = orig_image->screen;
   image->bo              = orig_image->bo;
   image->internal_format = orig_image->internal_format;
   image->planar_format   = orig_image->planar_format;
   image->dri_format      = orig_image->dri_format;
   image->format          = orig_image->format;
   image->modifier        = orig_image->modifier;
   image->offset          = orig_image->offset;
   image->width           = orig_image->width;
   image->height          = orig_image->height;
   image->pitch           = orig_image->pitch;
   image->tile_x          = orig_image->tile_x;
   image->tile_y          = orig_image->tile_y;
   image->has_depthstencil = orig_image->has_depthstencil;
   image->driScrnPriv     = orig_image->driScrnPriv;
   image->loader_private  = loaderPrivate;
   image->aux_offset      = orig_image->aux_offset;
   image->aux_pitch       = orig_image->aux_pitch;

   memcpy(image->strides, orig_image->strides, sizeof(image->strides));
   memcpy(image->offsets, orig_image->offsets, sizeof(image->offsets));

   return image;
}

static GLboolean
brw_validate_usage(__DRIimage *image, unsigned int use)
{
   if (use & __DRI_IMAGE_USE_CURSOR) {
      if (image->width != 64 || image->height != 64)
         return GL_FALSE;
   }

   return GL_TRUE;
}

static __DRIimage *
brw_create_image_from_names(__DRIscreen *dri_screen,
                            int width, int height, int fourcc,
                            int *names, int num_names,
                            int *strides, int *offsets,
                            void *loaderPrivate)
{
    const struct brw_image_format *f = NULL;
    __DRIimage *image;
    int i, index;

    if (dri_screen == NULL || names == NULL || num_names != 1)
        return NULL;

    f = brw_image_format_lookup(fourcc);
    if (f == NULL)
        return NULL;

    image = brw_create_image_from_name(dri_screen, width, height,
                                       __DRI_IMAGE_FORMAT_NONE,
                                       names[0], strides[0],
                                       loaderPrivate);

   if (image == NULL)
      return NULL;

    image->planar_format = f;
    for (i = 0; i < f->nplanes; i++) {
        index = f->planes[i].buffer_index;
        image->offsets[index] = offsets[index];
        image->strides[index] = strides[index];
    }

    return image;
}

static __DRIimage *
brw_create_image_from_fds_common(__DRIscreen *dri_screen,
                                 int width, int height, int fourcc,
                                 uint64_t modifier, int *fds, int num_fds,
                                 int *strides, int *offsets,
                                 void *loaderPrivate)
{
   struct brw_screen *screen = dri_screen->driverPrivate;
   const struct brw_image_format *f;
   __DRIimage *image;
   int i, index;
   bool ok;

   if (fds == NULL || num_fds < 1)
      return NULL;

   f = brw_image_format_lookup(fourcc);
   if (f == NULL)
      return NULL;

   if (modifier != DRM_FORMAT_MOD_INVALID &&
       !modifier_is_supported(&screen->devinfo, f, 0, 0, modifier))
      return NULL;

   if (f->nplanes == 1)
      image = brw_allocate_image(screen, f->planes[0].dri_format,
                                   loaderPrivate);
   else
      image = brw_allocate_image(screen, __DRI_IMAGE_FORMAT_NONE,
                                   loaderPrivate);

   if (image == NULL)
      return NULL;

   image->width = width;
   image->height = height;
   image->pitch = strides[0];

   image->planar_format = f;

   if (modifier != DRM_FORMAT_MOD_INVALID) {
      const struct isl_drm_modifier_info *mod_info =
         isl_drm_modifier_get_info(modifier);
      uint32_t tiling = isl_tiling_to_i915_tiling(mod_info->tiling);
      image->bo = brw_bo_gem_create_from_prime_tiled(screen->bufmgr, fds[0],
                                                     tiling, strides[0]);
   } else {
      image->bo = brw_bo_gem_create_from_prime(screen->bufmgr, fds[0]);
   }

   if (image->bo == NULL) {
      free(image);
      return NULL;
   }

   /* We only support all planes from the same bo.
    * brw_bo_gem_create_from_prime() should return the same pointer for all
    * fds received here */
   for (i = 1; i < num_fds; i++) {
      struct brw_bo *aux = brw_bo_gem_create_from_prime(screen->bufmgr, fds[i]);
      brw_bo_unreference(aux);
      if (aux != image->bo) {
         brw_bo_unreference(image->bo);
         free(image);
         return NULL;
      }
   }

   if (modifier != DRM_FORMAT_MOD_INVALID)
      image->modifier = modifier;
   else
      image->modifier = tiling_to_modifier(image->bo->tiling_mode);

   const struct isl_drm_modifier_info *mod_info =
      isl_drm_modifier_get_info(image->modifier);

   int size = 0;
   struct isl_surf surf;
   for (i = 0; i < f->nplanes; i++) {
      index = f->planes[i].buffer_index;
      image->offsets[index] = offsets[index];
      image->strides[index] = strides[index];

      mesa_format format = driImageFormatToGLFormat(f->planes[i].dri_format);
      /* The images we will create are actually based on the RGBA non-sRGB
       * version of the format.
       */
      format = _mesa_format_fallback_rgbx_to_rgba(format);
      format = _mesa_get_srgb_format_linear(format);

      ok = isl_surf_init(&screen->isl_dev, &surf,
                         .dim = ISL_SURF_DIM_2D,
                         .format = brw_isl_format_for_mesa_format(format),
                         .width = image->width >> f->planes[i].width_shift,
                         .height = image->height >> f->planes[i].height_shift,
                         .depth = 1,
                         .levels = 1,
                         .array_len = 1,
                         .samples = 1,
                         .row_pitch_B = strides[index],
                         .usage = ISL_SURF_USAGE_RENDER_TARGET_BIT |
                                  ISL_SURF_USAGE_TEXTURE_BIT |
                                  ISL_SURF_USAGE_STORAGE_BIT,
                         .tiling_flags = (1 << mod_info->tiling));
      if (!ok) {
         brw_bo_unreference(image->bo);
         free(image);
         return NULL;
      }

      const int end = offsets[index] + surf.size_B;
      if (size < end)
         size = end;
   }

   if (mod_info->aux_usage == ISL_AUX_USAGE_CCS_E) {
      /* Even though we initialize surf in the loop above, we know that
       * anything with CCS_E will have exactly one plane so surf is properly
       * initialized when we get here.
       */
      assert(f->nplanes == 1);

      image->aux_offset = offsets[1];
      image->aux_pitch = strides[1];

      /* Scanout hardware requires that the CCS be placed after the main
       * surface in memory.  We consider any CCS that is placed any earlier in
       * memory to be invalid and reject it.
       *
       * At some point in the future, this restriction may be relaxed if the
       * hardware becomes less strict but we may need a new modifier for that.
       */
      assert(size > 0);
      if (image->aux_offset < size) {
         brw_bo_unreference(image->bo);
         free(image);
         return NULL;
      }

      struct isl_surf aux_surf = {0,};
      ok = isl_surf_get_ccs_surf(&screen->isl_dev, &surf, NULL, &aux_surf,
                                 image->aux_pitch);
      if (!ok) {
         brw_bo_unreference(image->bo);
         free(image);
         return NULL;
      }

      image->aux_size = aux_surf.size_B;

      const int end = image->aux_offset + aux_surf.size_B;
      if (size < end)
         size = end;
   } else {
      assert(mod_info->aux_usage == ISL_AUX_USAGE_NONE);
   }

   /* Check that the requested image actually fits within the BO. 'size'
    * is already relative to the offsets, so we don't need to add that. */
   if (image->bo->size == 0) {
      image->bo->size = size;
   } else if (size > image->bo->size) {
      brw_bo_unreference(image->bo);
      free(image);
      return NULL;
   }

   if (f->nplanes == 1) {
      image->offset = image->offsets[0];
      brw_image_warn_if_unaligned(image, __func__);
   }

   return image;
}

static __DRIimage *
brw_create_image_from_fds(__DRIscreen *dri_screen,
                          int width, int height, int fourcc,
                          int *fds, int num_fds, int *strides, int *offsets,
                          void *loaderPrivate)
{
   return brw_create_image_from_fds_common(dri_screen, width, height, fourcc,
                                           DRM_FORMAT_MOD_INVALID,
                                           fds, num_fds, strides, offsets,
                                           loaderPrivate);
}

static __DRIimage *
brw_create_image_from_dma_bufs2(__DRIscreen *dri_screen,
                                int width, int height,
                                int fourcc, uint64_t modifier,
                                int *fds, int num_fds,
                                int *strides, int *offsets,
                                enum __DRIYUVColorSpace yuv_color_space,
                                enum __DRISampleRange sample_range,
                                enum __DRIChromaSiting horizontal_siting,
                                enum __DRIChromaSiting vertical_siting,
                                unsigned *error,
                                void *loaderPrivate)
{
   __DRIimage *image;
   const struct brw_image_format *f = brw_image_format_lookup(fourcc);

   if (!f) {
      *error = __DRI_IMAGE_ERROR_BAD_MATCH;
      return NULL;
   }

   image = brw_create_image_from_fds_common(dri_screen, width, height,
                                            fourcc, modifier,
                                            fds, num_fds, strides, offsets,
                                            loaderPrivate);

   /*
    * Invalid parameters and any inconsistencies between are assumed to be
    * checked by the caller. Therefore besides unsupported formats one can fail
    * only in allocation.
    */
   if (!image) {
      *error = __DRI_IMAGE_ERROR_BAD_ALLOC;
      return NULL;
   }

   image->yuv_color_space = yuv_color_space;
   image->sample_range = sample_range;
   image->horizontal_siting = horizontal_siting;
   image->vertical_siting = vertical_siting;
   image->imported_dmabuf = true;

   *error = __DRI_IMAGE_ERROR_SUCCESS;
   return image;
}

static __DRIimage *
brw_create_image_from_dma_bufs(__DRIscreen *dri_screen,
                               int width, int height, int fourcc,
                               int *fds, int num_fds,
                               int *strides, int *offsets,
                               enum __DRIYUVColorSpace yuv_color_space,
                               enum __DRISampleRange sample_range,
                               enum __DRIChromaSiting horizontal_siting,
                               enum __DRIChromaSiting vertical_siting,
                               unsigned *error,
                               void *loaderPrivate)
{
   return brw_create_image_from_dma_bufs2(dri_screen, width, height,
                                          fourcc, DRM_FORMAT_MOD_INVALID,
                                          fds, num_fds, strides, offsets,
                                          yuv_color_space,
                                          sample_range,
                                          horizontal_siting,
                                          vertical_siting,
                                          error,
                                          loaderPrivate);
}

static bool
brw_image_format_is_supported(const struct intel_device_info *devinfo,
                                const struct brw_image_format *fmt)
{
   /* Currently, all formats with an brw_image_format are available on all
    * platforms so there's really nothing to check there.
    */

#ifndef NDEBUG
   if (fmt->nplanes == 1) {
      mesa_format format = driImageFormatToGLFormat(fmt->planes[0].dri_format);
      /* The images we will create are actually based on the RGBA non-sRGB
       * version of the format.
       */
      format = _mesa_format_fallback_rgbx_to_rgba(format);
      format = _mesa_get_srgb_format_linear(format);
      enum isl_format isl_format = brw_isl_format_for_mesa_format(format);
      assert(isl_format_supports_rendering(devinfo, isl_format));
   }
#endif

   return true;
}

static GLboolean
brw_query_dma_buf_formats(__DRIscreen *_screen, int max,
                            int *formats, int *count)
{
   struct brw_screen *screen = _screen->driverPrivate;
   int num_formats = 0, i;

   for (i = 0; i < ARRAY_SIZE(brw_image_formats); i++) {
      /* These formats are valid DRI formats but do not exist in drm_fourcc.h
       * in the Linux kernel. We don't want to accidentally advertise them
       * them through the EGL layer.
       */
      if (brw_image_formats[i].fourcc == __DRI_IMAGE_FOURCC_SARGB8888 ||
          brw_image_formats[i].fourcc == __DRI_IMAGE_FOURCC_SABGR8888 ||
          brw_image_formats[i].fourcc == __DRI_IMAGE_FOURCC_SXRGB8888)
         continue;

      if (!brw_image_format_is_supported(&screen->devinfo,
                                           &brw_image_formats[i]))
         continue;

      num_formats++;
      if (max == 0)
         continue;

      formats[num_formats - 1] = brw_image_formats[i].fourcc;
      if (num_formats >= max)
         break;
   }

   *count = num_formats;
   return true;
}

static GLboolean
brw_query_dma_buf_modifiers(__DRIscreen *_screen, int fourcc, int max,
                              uint64_t *modifiers,
                              unsigned int *external_only,
                              int *count)
{
   struct brw_screen *screen = _screen->driverPrivate;
   const struct brw_image_format *f;
   int num_mods = 0, i;

   f = brw_image_format_lookup(fourcc);
   if (f == NULL)
      return false;

   if (!brw_image_format_is_supported(&screen->devinfo, f))
      return false;

   for (i = 0; i < ARRAY_SIZE(supported_modifiers); i++) {
      uint64_t modifier = supported_modifiers[i].modifier;
      if (!modifier_is_supported(&screen->devinfo, f, 0, 0, modifier))
         continue;

      num_mods++;
      if (max == 0)
         continue;

      modifiers[num_mods - 1] = modifier;
      if (num_mods >= max)
        break;
   }

   if (external_only != NULL) {
      for (i = 0; i < num_mods && i < max; i++) {
         if (f->components == __DRI_IMAGE_COMPONENTS_Y_U_V ||
             f->components == __DRI_IMAGE_COMPONENTS_Y_UV ||
             f->components == __DRI_IMAGE_COMPONENTS_AYUV ||
             f->components == __DRI_IMAGE_COMPONENTS_XYUV ||
             f->components == __DRI_IMAGE_COMPONENTS_Y_XUXV ||
             f->components == __DRI_IMAGE_COMPONENTS_Y_UXVX) {
            external_only[i] = GL_TRUE;
         }
         else {
            external_only[i] = GL_FALSE;
         }
      }
   }

   *count = num_mods;
   return true;
}

static __DRIimage *
brw_from_planar(__DRIimage *parent, int plane, void *loaderPrivate)
{
    int width, height, offset, stride, size, dri_format;
    __DRIimage *image;

    if (parent == NULL)
       return NULL;

    width = parent->width;
    height = parent->height;

    const struct brw_image_format *f = parent->planar_format;

    if (f && plane < f->nplanes) {
       /* Use the planar format definition. */
       width >>= f->planes[plane].width_shift;
       height >>= f->planes[plane].height_shift;
       dri_format = f->planes[plane].dri_format;
       int index = f->planes[plane].buffer_index;
       offset = parent->offsets[index];
       stride = parent->strides[index];
       size = height * stride;
    } else if (plane == 0) {
       /* The only plane of a non-planar image: copy the parent definition
        * directly. */
       dri_format = parent->dri_format;
       offset = parent->offset;
       stride = parent->pitch;
       size = height * stride;
    } else if (plane == 1 && parent->modifier != DRM_FORMAT_MOD_INVALID &&
               isl_drm_modifier_has_aux(parent->modifier)) {
       /* Auxiliary plane */
       dri_format = parent->dri_format;
       offset = parent->aux_offset;
       stride = parent->aux_pitch;
       size = parent->aux_size;
    } else {
       return NULL;
    }

    if (offset + size > parent->bo->size) {
       _mesa_warning(NULL, "intel_from_planar: subimage out of bounds");
       return NULL;
    }

    image = brw_allocate_image(parent->screen, dri_format, loaderPrivate);
    if (image == NULL)
       return NULL;

    image->bo = parent->bo;
    brw_bo_reference(parent->bo);
    image->modifier = parent->modifier;

    image->width = width;
    image->height = height;
    image->pitch = stride;
    image->offset = offset;

    brw_image_warn_if_unaligned(image, __func__);

    return image;
}

static const __DRIimageExtension brwImageExtension = {
    .base = { __DRI_IMAGE, 19 },

    .createImageFromName                = brw_create_image_from_name,
    .createImageFromRenderbuffer        = brw_create_image_from_renderbuffer,
    .destroyImage                       = brw_destroy_image,
    .createImage                        = brw_create_image,
    .queryImage                         = brw_query_image,
    .dupImage                           = brw_dup_image,
    .validateUsage                      = brw_validate_usage,
    .createImageFromNames               = brw_create_image_from_names,
    .fromPlanar                         = brw_from_planar,
    .createImageFromTexture             = brw_create_image_from_texture,
    .createImageFromFds                 = brw_create_image_from_fds,
    .createImageFromDmaBufs             = brw_create_image_from_dma_bufs,
    .blitImage                          = NULL,
    .getCapabilities                    = NULL,
    .mapImage                           = brw_map_image,
    .unmapImage                         = brw_unmap_image,
    .createImageWithModifiers           = brw_create_image_with_modifiers,
    .createImageFromDmaBufs2            = brw_create_image_from_dma_bufs2,
    .queryDmaBufFormats                 = brw_query_dma_buf_formats,
    .queryDmaBufModifiers               = brw_query_dma_buf_modifiers,
    .queryDmaBufFormatModifierAttribs   = brw_query_format_modifier_attribs,
    .createImageWithModifiers2          = brw_create_image_with_modifiers2,
};

static int
brw_query_renderer_integer(__DRIscreen *dri_screen,
                           int param, unsigned int *value)
{
   const struct brw_screen *const screen =
      (struct brw_screen *) dri_screen->driverPrivate;

   switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] = 0x8086;
      return 0;
   case __DRI2_RENDERER_DEVICE_ID:
      value[0] = screen->deviceID;
      return 0;
   case __DRI2_RENDERER_ACCELERATED:
      value[0] = 1;
      return 0;
   case __DRI2_RENDERER_VIDEO_MEMORY: {
      /* Once a batch uses more than 75% of the maximum mappable size, we
       * assume that there's some fragmentation, and we start doing extra
       * flushing, etc.  That's the big cliff apps will care about.
       */
      const unsigned gpu_mappable_megabytes =
         screen->aperture_threshold / (1024 * 1024);

      const long system_memory_pages = sysconf(_SC_PHYS_PAGES);
      const long system_page_size = sysconf(_SC_PAGE_SIZE);

      if (system_memory_pages <= 0 || system_page_size <= 0)
         return -1;

      const uint64_t system_memory_bytes = (uint64_t) system_memory_pages
         * (uint64_t) system_page_size;

      const unsigned system_memory_megabytes =
         (unsigned) (system_memory_bytes / (1024 * 1024));

      value[0] = MIN2(system_memory_megabytes, gpu_mappable_megabytes);
      return 0;
   }
   case __DRI2_RENDERER_UNIFIED_MEMORY_ARCHITECTURE:
      value[0] = 1;
      return 0;
   case __DRI2_RENDERER_HAS_TEXTURE_3D:
      value[0] = 1;
      return 0;
   case __DRI2_RENDERER_HAS_CONTEXT_PRIORITY:
      value[0] = 0;
      if (brw_hw_context_set_priority(screen->bufmgr,
                                      0, INTEL_CONTEXT_HIGH_PRIORITY) == 0)
         value[0] |= __DRI2_RENDERER_HAS_CONTEXT_PRIORITY_HIGH;
      if (brw_hw_context_set_priority(screen->bufmgr,
                                      0, INTEL_CONTEXT_LOW_PRIORITY) == 0)
         value[0] |= __DRI2_RENDERER_HAS_CONTEXT_PRIORITY_LOW;
      /* reset to default last, just in case */
      if (brw_hw_context_set_priority(screen->bufmgr,
                                      0, INTEL_CONTEXT_MEDIUM_PRIORITY) == 0)
         value[0] |= __DRI2_RENDERER_HAS_CONTEXT_PRIORITY_MEDIUM;
      return 0;
   case __DRI2_RENDERER_HAS_FRAMEBUFFER_SRGB:
      value[0] = 1;
      return 0;
   default:
      return driQueryRendererIntegerCommon(dri_screen, param, value);
   }

   return -1;
}

static int
brw_query_renderer_string(__DRIscreen *dri_screen,
                          int param, const char **value)
{
   const struct brw_screen *screen =
      (struct brw_screen *) dri_screen->driverPrivate;

   switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] = brw_vendor_string;
      return 0;
   case __DRI2_RENDERER_DEVICE_ID:
      value[0] = brw_get_renderer_string(screen);
      return 0;
   default:
      break;
   }

   return -1;
}

static void
brw_set_cache_funcs(__DRIscreen *dri_screen,
                    __DRIblobCacheSet set, __DRIblobCacheGet get)
{
   const struct brw_screen *const screen =
      (struct brw_screen *) dri_screen->driverPrivate;

   if (!screen->disk_cache)
      return;

   disk_cache_set_callbacks(screen->disk_cache, set, get);
}

static const __DRI2rendererQueryExtension brwRendererQueryExtension = {
   .base = { __DRI2_RENDERER_QUERY, 1 },

   .queryInteger = brw_query_renderer_integer,
   .queryString = brw_query_renderer_string
};

static const __DRIrobustnessExtension dri2Robustness = {
   .base = { __DRI2_ROBUSTNESS, 1 }
};

static const __DRI2blobExtension brwBlobExtension = {
   .base = { __DRI2_BLOB, 1 },
   .set_cache_funcs = brw_set_cache_funcs
};

static const __DRImutableRenderBufferDriverExtension brwMutableRenderBufferExtension = {
   .base = { __DRI_MUTABLE_RENDER_BUFFER_DRIVER, 1 },
};

static const __DRIextension *screenExtensions[] = {
    &brwTexBufferExtension.base,
    &brwFenceExtension.base,
    &brwFlushExtension.base,
    &brwImageExtension.base,
    &brwRendererQueryExtension.base,
    &brwMutableRenderBufferExtension.base,
    &dri2ConfigQueryExtension.base,
    &dri2NoErrorExtension.base,
    &brwBlobExtension.base,
    NULL
};

static const __DRIextension *brwRobustScreenExtensions[] = {
    &brwTexBufferExtension.base,
    &brwFenceExtension.base,
    &brwFlushExtension.base,
    &brwImageExtension.base,
    &brwRendererQueryExtension.base,
    &brwMutableRenderBufferExtension.base,
    &dri2ConfigQueryExtension.base,
    &dri2Robustness.base,
    &dri2NoErrorExtension.base,
    &brwBlobExtension.base,
    NULL
};

static int
brw_get_param(struct brw_screen *screen, int param, int *value)
{
   int ret = 0;
   struct drm_i915_getparam gp;

   memset(&gp, 0, sizeof(gp));
   gp.param = param;
   gp.value = value;

   if (drmIoctl(screen->fd, DRM_IOCTL_I915_GETPARAM, &gp) == -1) {
      ret = -errno;
      if (ret != -EINVAL)
         _mesa_warning(NULL, "drm_i915_getparam: %d", ret);
   }

   return ret;
}

static bool
brw_get_boolean(struct brw_screen *screen, int param)
{
   int value = 0;
   return (brw_get_param(screen, param, &value) == 0) && value;
}

static int
brw_get_integer(struct brw_screen *screen, int param)
{
   int value = -1;

   if (brw_get_param(screen, param, &value) == 0)
      return value;

   return -1;
}

static void
brw_destroy_screen(__DRIscreen *sPriv)
{
   struct brw_screen *screen = sPriv->driverPrivate;

   brw_bufmgr_unref(screen->bufmgr);
   driDestroyOptionInfo(&screen->optionCache);

   disk_cache_destroy(screen->disk_cache);

   ralloc_free(screen);
   sPriv->driverPrivate = NULL;
}


/**
 * Create a gl_framebuffer and attach it to __DRIdrawable::driverPrivate.
 *
 *_This implements driDriverAPI::createNewDrawable, which the DRI layer calls
 * when creating a EGLSurface, GLXDrawable, or GLXPixmap. Despite the name,
 * this does not allocate GPU memory.
 */
static GLboolean
brw_create_buffer(__DRIscreen *dri_screen,
                  __DRIdrawable *driDrawPriv,
                  const struct gl_config *mesaVis, GLboolean isPixmap)
{
   struct brw_renderbuffer *rb;
   struct brw_screen *screen = (struct brw_screen *)
      dri_screen->driverPrivate;
   mesa_format rgbFormat;
   unsigned num_samples =
      brw_quantize_num_samples(screen, mesaVis->samples);

   if (isPixmap)
      return false;

   struct gl_framebuffer *fb = CALLOC_STRUCT(gl_framebuffer);
   if (!fb)
      return false;

   _mesa_initialize_window_framebuffer(fb, mesaVis);

   if (screen->winsys_msaa_samples_override != -1) {
      num_samples = screen->winsys_msaa_samples_override;
      fb->Visual.samples = num_samples;
   }

   if (mesaVis->redBits == 16 && mesaVis->alphaBits > 0 && mesaVis->floatMode) {
      rgbFormat = MESA_FORMAT_RGBA_FLOAT16;
   } else if (mesaVis->redBits == 16 && mesaVis->floatMode) {
      rgbFormat = MESA_FORMAT_RGBX_FLOAT16;
   } else if (mesaVis->redBits == 10 && mesaVis->alphaBits > 0) {
      rgbFormat = mesaVis->redMask == 0x3ff00000 ? MESA_FORMAT_B10G10R10A2_UNORM
                                                 : MESA_FORMAT_R10G10B10A2_UNORM;
   } else if (mesaVis->redBits == 10) {
      rgbFormat = mesaVis->redMask == 0x3ff00000 ? MESA_FORMAT_B10G10R10X2_UNORM
                                                 : MESA_FORMAT_R10G10B10X2_UNORM;
   } else if (mesaVis->redBits == 5) {
      rgbFormat = mesaVis->redMask == 0x1f ? MESA_FORMAT_R5G6B5_UNORM
                                           : MESA_FORMAT_B5G6R5_UNORM;
   } else if (mesaVis->alphaBits == 0) {
      rgbFormat = mesaVis->redMask == 0xff ? MESA_FORMAT_R8G8B8X8_SRGB
                                           : MESA_FORMAT_B8G8R8X8_SRGB;
      fb->Visual.sRGBCapable = true;
   } else if (mesaVis->sRGBCapable) {
      rgbFormat = mesaVis->redMask == 0xff ? MESA_FORMAT_R8G8B8A8_SRGB
                                           : MESA_FORMAT_B8G8R8A8_SRGB;
      fb->Visual.sRGBCapable = true;
   } else {
      rgbFormat = mesaVis->redMask == 0xff ? MESA_FORMAT_R8G8B8A8_SRGB
                                           : MESA_FORMAT_B8G8R8A8_SRGB;
      fb->Visual.sRGBCapable = true;
   }

   /* mesaVis->sRGBCapable was set, user is asking for sRGB */
   bool srgb_cap_set = mesaVis->redBits >= 8 && mesaVis->sRGBCapable;

   /* setup the hardware-based renderbuffers */
   rb = brw_create_winsys_renderbuffer(screen, rgbFormat, num_samples);
   _mesa_attach_and_own_rb(fb, BUFFER_FRONT_LEFT, &rb->Base.Base);
   rb->need_srgb = srgb_cap_set;

   if (mesaVis->doubleBufferMode) {
      rb = brw_create_winsys_renderbuffer(screen, rgbFormat, num_samples);
      _mesa_attach_and_own_rb(fb, BUFFER_BACK_LEFT, &rb->Base.Base);
      rb->need_srgb = srgb_cap_set;
   }

   /*
    * Assert here that the gl_config has an expected depth/stencil bit
    * combination: one of d24/s8, d16/s0, d0/s0. (See brw_init_screen(),
    * which constructs the advertised configs.)
    */
   if (mesaVis->depthBits == 24) {
      assert(mesaVis->stencilBits == 8);

      if (screen->devinfo.has_hiz_and_separate_stencil) {
         rb = brw_create_private_renderbuffer(screen,
                                                MESA_FORMAT_Z24_UNORM_X8_UINT,
                                                num_samples);
         _mesa_attach_and_own_rb(fb, BUFFER_DEPTH, &rb->Base.Base);
         rb = brw_create_private_renderbuffer(screen, MESA_FORMAT_S_UINT8,
                                                num_samples);
         _mesa_attach_and_own_rb(fb, BUFFER_STENCIL, &rb->Base.Base);
      } else {
         /*
          * Use combined depth/stencil. Note that the renderbuffer is
          * attached to two attachment points.
          */
         rb = brw_create_private_renderbuffer(screen,
                                                MESA_FORMAT_Z24_UNORM_S8_UINT,
                                                num_samples);
         _mesa_attach_and_own_rb(fb, BUFFER_DEPTH, &rb->Base.Base);
         _mesa_attach_and_reference_rb(fb, BUFFER_STENCIL, &rb->Base.Base);
      }
   }
   else if (mesaVis->depthBits == 16) {
      assert(mesaVis->stencilBits == 0);
      rb = brw_create_private_renderbuffer(screen, MESA_FORMAT_Z_UNORM16,
                                             num_samples);
      _mesa_attach_and_own_rb(fb, BUFFER_DEPTH, &rb->Base.Base);
   }
   else {
      assert(mesaVis->depthBits == 0);
      assert(mesaVis->stencilBits == 0);
   }

   /* now add any/all software-based renderbuffers we may need */
   _swrast_add_soft_renderbuffers(fb,
                                  false, /* never sw color */
                                  false, /* never sw depth */
                                  false, /* never sw stencil */
                                  mesaVis->accumRedBits > 0,
                                  false /* never sw alpha */);
   driDrawPriv->driverPrivate = fb;

   return true;
}

static void
brw_destroy_buffer(__DRIdrawable *driDrawPriv)
{
    struct gl_framebuffer *fb = driDrawPriv->driverPrivate;

    _mesa_reference_framebuffer(&fb, NULL);
}

static bool
brw_init_bufmgr(struct brw_screen *screen)
{
   __DRIscreen *dri_screen = screen->driScrnPriv;

   bool bo_reuse = false;
   int bo_reuse_mode = driQueryOptioni(&screen->optionCache, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
      break;
   case DRI_CONF_BO_REUSE_ALL:
      bo_reuse = true;
      break;
   }

   screen->bufmgr = brw_bufmgr_get_for_fd(&screen->devinfo, dri_screen->fd, bo_reuse);
   if (screen->bufmgr == NULL) {
      fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
              __func__, __LINE__);
      return false;
   }
   screen->fd = brw_bufmgr_get_fd(screen->bufmgr);

   if (!brw_get_boolean(screen, I915_PARAM_HAS_EXEC_NO_RELOC)) {
      fprintf(stderr, "[%s: %u] Kernel 3.9 required.\n", __func__, __LINE__);
      return false;
   }

   return true;
}

static bool
brw_detect_swizzling(struct brw_screen *screen)
{
   /* Broadwell PRM says:
    *
    *   "Before Gfx8, there was a historical configuration control field to
    *    swizzle address bit[6] for in X/Y tiling modes. This was set in three
    *    different places: TILECTL[1:0], ARB_MODE[5:4], and
    *    DISP_ARB_CTL[14:13].
    *
    *    For Gfx8 and subsequent generations, the swizzle fields are all
    *    reserved, and the CPU's memory controller performs all address
    *    swizzling modifications."
    */
   if (screen->devinfo.ver >= 8)
      return false;

   uint32_t tiling = I915_TILING_X;
   uint32_t swizzle_mode = 0;
   struct brw_bo *buffer =
      brw_bo_alloc_tiled(screen->bufmgr, "swizzle test", 32768,
                         BRW_MEMZONE_OTHER, tiling, 512, 0);
   if (buffer == NULL)
      return false;

   brw_bo_get_tiling(buffer, &tiling, &swizzle_mode);
   brw_bo_unreference(buffer);

   return swizzle_mode != I915_BIT_6_SWIZZLE_NONE;
}

static int
brw_detect_timestamp(struct brw_screen *screen)
{
   uint64_t dummy = 0, last = 0;
   int upper, lower, loops;

   /* On 64bit systems, some old kernels trigger a hw bug resulting in the
    * TIMESTAMP register being shifted and the low 32bits always zero.
    *
    * More recent kernels offer an interface to read the full 36bits
    * everywhere.
    */
   if (brw_reg_read(screen->bufmgr, TIMESTAMP | 1, &dummy) == 0)
      return 3;

   /* Determine if we have a 32bit or 64bit kernel by inspecting the
    * upper 32bits for a rapidly changing timestamp.
    */
   if (brw_reg_read(screen->bufmgr, TIMESTAMP, &last))
      return 0;

   upper = lower = 0;
   for (loops = 0; loops < 10; loops++) {
      /* The TIMESTAMP should change every 80ns, so several round trips
       * through the kernel should be enough to advance it.
       */
      if (brw_reg_read(screen->bufmgr, TIMESTAMP, &dummy))
         return 0;

      upper += (dummy >> 32) != (last >> 32);
      if (upper > 1) /* beware 32bit counter overflow */
         return 2; /* upper dword holds the low 32bits of the timestamp */

      lower += (dummy & 0xffffffff) != (last & 0xffffffff);
      if (lower > 1)
         return 1; /* timestamp is unshifted */

      last = dummy;
   }

   /* No advancement? No timestamp! */
   return 0;
}

 /**
 * Test if we can use MI_LOAD_REGISTER_MEM from an untrusted batchbuffer.
 *
 * Some combinations of hardware and kernel versions allow this feature,
 * while others don't.  Instead of trying to enumerate every case, just
 * try and write a register and see if works.
 */
static bool
brw_detect_pipelined_register(struct brw_screen *screen,
                                int reg, uint32_t expected_value, bool reset)
{
   if (screen->devinfo.no_hw)
      return false;

   struct brw_bo *results, *bo;
   uint32_t *batch;
   uint32_t offset = 0;
   void *map;
   bool success = false;

   /* Create a zero'ed temporary buffer for reading our results */
   results = brw_bo_alloc(screen->bufmgr, "registers", 4096, BRW_MEMZONE_OTHER);
   if (results == NULL)
      goto err;

   bo = brw_bo_alloc(screen->bufmgr, "batchbuffer", 4096, BRW_MEMZONE_OTHER);
   if (bo == NULL)
      goto err_results;

   map = brw_bo_map(NULL, bo, MAP_WRITE);
   if (!map)
      goto err_batch;

   batch = map;

   /* Write the register. */
   *batch++ = MI_LOAD_REGISTER_IMM | (3 - 2);
   *batch++ = reg;
   *batch++ = expected_value;

   /* Save the register's value back to the buffer. */
   *batch++ = MI_STORE_REGISTER_MEM | (3 - 2);
   *batch++ = reg;
   struct drm_i915_gem_relocation_entry reloc = {
      .offset = (char *) batch - (char *) map,
      .delta = offset * sizeof(uint32_t),
      .target_handle = results->gem_handle,
      .read_domains = I915_GEM_DOMAIN_INSTRUCTION,
      .write_domain = I915_GEM_DOMAIN_INSTRUCTION,
   };
   *batch++ = reloc.presumed_offset + reloc.delta;

   /* And afterwards clear the register */
   if (reset) {
      *batch++ = MI_LOAD_REGISTER_IMM | (3 - 2);
      *batch++ = reg;
      *batch++ = 0;
   }

   *batch++ = MI_BATCH_BUFFER_END;

   struct drm_i915_gem_exec_object2 exec_objects[2] = {
      {
         .handle = results->gem_handle,
      },
      {
         .handle = bo->gem_handle,
         .relocation_count = 1,
         .relocs_ptr = (uintptr_t) &reloc,
      }
   };

   struct drm_i915_gem_execbuffer2 execbuf = {
      .buffers_ptr = (uintptr_t) exec_objects,
      .buffer_count = 2,
      .batch_len = ALIGN((char *) batch - (char *) map, 8),
      .flags = I915_EXEC_RENDER,
   };

   /* Don't bother with error checking - if the execbuf fails, the
    * value won't be written and we'll just report that there's no access.
    */
   drmIoctl(screen->fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);

   /* Check whether the value got written. */
   void *results_map = brw_bo_map(NULL, results, MAP_READ);
   if (results_map) {
      success = *((uint32_t *)results_map + offset) == expected_value;
      brw_bo_unmap(results);
   }

err_batch:
   brw_bo_unreference(bo);
err_results:
   brw_bo_unreference(results);
err:
   return success;
}

static bool
brw_detect_pipelined_so(struct brw_screen *screen)
{
   const struct intel_device_info *devinfo = &screen->devinfo;

   /* Supposedly, Broadwell just works. */
   if (devinfo->ver >= 8)
      return true;

   if (devinfo->ver <= 6)
      return false;

   /* See the big explanation about command parser versions below */
   if (screen->cmd_parser_version >= (devinfo->is_haswell ? 7 : 2))
      return true;

   /* We use SO_WRITE_OFFSET0 since you're supposed to write it (unlike the
    * statistics registers), and we already reset it to zero before using it.
    */
   return brw_detect_pipelined_register(screen,
                                          GFX7_SO_WRITE_OFFSET(0),
                                          0x1337d0d0,
                                          false);
}

/**
 * Return array of MSAA modes supported by the hardware. The array is
 * zero-terminated and sorted in decreasing order.
 */
const int*
brw_supported_msaa_modes(const struct brw_screen  *screen)
{
   static const int gfx9_modes[] = {16, 8, 4, 2, 0, -1};
   static const int gfx8_modes[] = {8, 4, 2, 0, -1};
   static const int gfx7_modes[] = {8, 4, 0, -1};
   static const int gfx6_modes[] = {4, 0, -1};
   static const int gfx4_modes[] = {0, -1};

   if (screen->devinfo.ver >= 9) {
      return gfx9_modes;
   } else if (screen->devinfo.ver >= 8) {
      return gfx8_modes;
   } else if (screen->devinfo.ver >= 7) {
      return gfx7_modes;
   } else if (screen->devinfo.ver == 6) {
      return gfx6_modes;
   } else {
      return gfx4_modes;
   }
}

static unsigned
brw_loader_get_cap(const __DRIscreen *dri_screen, enum dri_loader_cap cap)
{
   if (dri_screen->dri2.loader && dri_screen->dri2.loader->base.version >= 4 &&
       dri_screen->dri2.loader->getCapability)
      return dri_screen->dri2.loader->getCapability(dri_screen->loaderPrivate, cap);

   if (dri_screen->image.loader && dri_screen->image.loader->base.version >= 2 &&
       dri_screen->image.loader->getCapability)
      return dri_screen->image.loader->getCapability(dri_screen->loaderPrivate, cap);

   return 0;
}

static bool
brw_allowed_format(__DRIscreen *dri_screen, mesa_format format)
{
   struct brw_screen *screen = dri_screen->driverPrivate;

   /* Expose only BGRA ordering if the loader doesn't support RGBA ordering. */
   bool allow_rgba_ordering = brw_loader_get_cap(dri_screen, DRI_LOADER_CAP_RGBA_ORDERING);
   if (!allow_rgba_ordering &&
       (format == MESA_FORMAT_R8G8B8A8_UNORM ||
        format == MESA_FORMAT_R8G8B8X8_UNORM ||
        format == MESA_FORMAT_R8G8B8A8_SRGB ||
        format == MESA_FORMAT_R8G8B8X8_SRGB))
      return false;

    /* Shall we expose 10 bpc formats? */
   bool allow_rgb10_configs = driQueryOptionb(&screen->optionCache,
                                              "allow_rgb10_configs");
   if (!allow_rgb10_configs &&
       (format == MESA_FORMAT_B10G10R10A2_UNORM ||
        format == MESA_FORMAT_B10G10R10X2_UNORM))
      return false;

   /* Shall we expose 565 formats? */
   bool allow_rgb565_configs = driQueryOptionb(&screen->optionCache,
                                               "allow_rgb565_configs");
   if (!allow_rgb565_configs && format == MESA_FORMAT_B5G6R5_UNORM)
      return false;

   /* Shall we expose fp16 formats? */
   bool allow_fp16_configs = brw_loader_get_cap(dri_screen, DRI_LOADER_CAP_FP16);
   if (!allow_fp16_configs &&
       (format == MESA_FORMAT_RGBA_FLOAT16 ||
        format == MESA_FORMAT_RGBX_FLOAT16))
      return false;

   return true;
}

static __DRIconfig**
brw_screen_make_configs(__DRIscreen *dri_screen)
{
   static const mesa_format formats[] = {
      MESA_FORMAT_B5G6R5_UNORM,
      MESA_FORMAT_B8G8R8A8_UNORM,
      MESA_FORMAT_B8G8R8X8_UNORM,

      MESA_FORMAT_B8G8R8A8_SRGB,
      MESA_FORMAT_B8G8R8X8_SRGB,

      /* For 10 bpc, 30 bit depth framebuffers. */
      MESA_FORMAT_B10G10R10A2_UNORM,
      MESA_FORMAT_B10G10R10X2_UNORM,

      MESA_FORMAT_RGBA_FLOAT16,
      MESA_FORMAT_RGBX_FLOAT16,

      /* The 32-bit RGBA format must not precede the 32-bit BGRA format.
       * Likewise for RGBX and BGRX.  Otherwise, the GLX client and the GLX
       * server may disagree on which format the GLXFBConfig represents,
       * resulting in swapped color channels.
       *
       * The problem, as of 2017-05-30:
       * When matching a GLXFBConfig to a __DRIconfig, GLX ignores the channel
       * order and chooses the first __DRIconfig with the expected channel
       * sizes. Specifically, GLX compares the GLXFBConfig's and __DRIconfig's
       * __DRI_ATTRIB_{CHANNEL}_SIZE but ignores __DRI_ATTRIB_{CHANNEL}_MASK.
       *
       * EGL does not suffer from this problem. It correctly compares the
       * channel masks when matching EGLConfig to __DRIconfig.
       */

      /* Required by Android, for HAL_PIXEL_FORMAT_RGBA_8888. */
      MESA_FORMAT_R8G8B8A8_UNORM,
      MESA_FORMAT_R8G8B8A8_SRGB,

      /* Required by Android, for HAL_PIXEL_FORMAT_RGBX_8888. */
      MESA_FORMAT_R8G8B8X8_UNORM,
      MESA_FORMAT_R8G8B8X8_SRGB,
   };

   /* __DRI_ATTRIB_SWAP_COPY is not supported due to page flipping. */
   static const GLenum back_buffer_modes[] = {
      __DRI_ATTRIB_SWAP_UNDEFINED, __DRI_ATTRIB_SWAP_NONE
   };

   static const uint8_t singlesample_samples[1] = {0};

   struct brw_screen *screen = dri_screen->driverPrivate;
   const struct intel_device_info *devinfo = &screen->devinfo;
   uint8_t depth_bits[4], stencil_bits[4];
   __DRIconfig **configs = NULL;

   unsigned num_formats = ARRAY_SIZE(formats);

   /* Generate singlesample configs, each without accumulation buffer
    * and with EGL_MUTABLE_RENDER_BUFFER_BIT_KHR.
    */
   for (unsigned i = 0; i < num_formats; i++) {
      __DRIconfig **new_configs;
      int num_depth_stencil_bits = 1;

      if (!brw_allowed_format(dri_screen, formats[i]))
         continue;

      /* Starting with DRI2 protocol version 1.1 we can request a depth/stencil
       * buffer that has a different number of bits per pixel than the color
       * buffer, gen >= 6 supports this.
       */
      depth_bits[0] = 0;
      stencil_bits[0] = 0;

      if (formats[i] == MESA_FORMAT_B5G6R5_UNORM) {
         if (devinfo->ver >= 8) {
            depth_bits[num_depth_stencil_bits] = 16;
            stencil_bits[num_depth_stencil_bits] = 0;
            num_depth_stencil_bits++;
         }
         if (devinfo->ver >= 6) {
             depth_bits[num_depth_stencil_bits] = 24;
             stencil_bits[num_depth_stencil_bits] = 8;
             num_depth_stencil_bits++;
         }
      } else {
         depth_bits[num_depth_stencil_bits] = 24;
         stencil_bits[num_depth_stencil_bits] = 8;
         num_depth_stencil_bits++;
      }

      new_configs = driCreateConfigs(formats[i],
                                     depth_bits,
                                     stencil_bits,
                                     num_depth_stencil_bits,
                                     back_buffer_modes, 2,
                                     singlesample_samples, 1,
                                     false, false);
      configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate the minimum possible set of configs that include an
    * accumulation buffer.
    */
   for (unsigned i = 0; i < num_formats; i++) {
      __DRIconfig **new_configs;

      if (!brw_allowed_format(dri_screen, formats[i]))
         continue;

      if (formats[i] == MESA_FORMAT_B5G6R5_UNORM) {
         if (devinfo->ver >= 8) {
            depth_bits[0] = 16;
            stencil_bits[0] = 0;
         } else if (devinfo->ver >= 6) {
            depth_bits[0] = 24;
            stencil_bits[0] = 8;
         } else {
            depth_bits[0] = 0;
            stencil_bits[0] = 0;
         }
      } else {
         depth_bits[0] = 24;
         stencil_bits[0] = 8;
      }

      new_configs = driCreateConfigs(formats[i],
                                     depth_bits, stencil_bits, 1,
                                     back_buffer_modes, 1,
                                     singlesample_samples, 1,
                                     true, false);
      configs = driConcatConfigs(configs, new_configs);
   }

   /* Generate multisample configs.
    *
    * This loop breaks early, and hence is a no-op, on gen < 6.
    *
    * Multisample configs must follow the singlesample configs in order to
    * work around an X server bug present in 1.12. The X server chooses to
    * associate the first listed RGBA888-Z24S8 config, regardless of its
    * sample count, with the 32-bit depth visual used for compositing.
    *
    * Only doublebuffer configs with GLX_SWAP_UNDEFINED_OML behavior are
    * supported.  Singlebuffer configs are not supported because no one wants
    * them.
    */
   for (unsigned i = 0; i < num_formats; i++) {
      if (devinfo->ver < 6)
         break;

      if (!brw_allowed_format(dri_screen, formats[i]))
         continue;

      __DRIconfig **new_configs;
      const int num_depth_stencil_bits = 2;
      int num_msaa_modes = 0;
      const uint8_t *multisample_samples = NULL;

      depth_bits[0] = 0;
      stencil_bits[0] = 0;

      if (formats[i] == MESA_FORMAT_B5G6R5_UNORM && devinfo->ver >= 8) {
         depth_bits[1] = 16;
         stencil_bits[1] = 0;
      } else {
         depth_bits[1] = 24;
         stencil_bits[1] = 8;
      }

      if (devinfo->ver >= 9) {
         static const uint8_t multisample_samples_gfx9[] = {2, 4, 8, 16};
         multisample_samples = multisample_samples_gfx9;
         num_msaa_modes = ARRAY_SIZE(multisample_samples_gfx9);
      } else if (devinfo->ver == 8) {
         static const uint8_t multisample_samples_gfx8[] = {2, 4, 8};
         multisample_samples = multisample_samples_gfx8;
         num_msaa_modes = ARRAY_SIZE(multisample_samples_gfx8);
      } else if (devinfo->ver == 7) {
         static const uint8_t multisample_samples_gfx7[] = {4, 8};
         multisample_samples = multisample_samples_gfx7;
         num_msaa_modes = ARRAY_SIZE(multisample_samples_gfx7);
      } else if (devinfo->ver == 6) {
         static const uint8_t multisample_samples_gfx6[] = {4};
         multisample_samples = multisample_samples_gfx6;
         num_msaa_modes = ARRAY_SIZE(multisample_samples_gfx6);
      }

      new_configs = driCreateConfigs(formats[i],
                                     depth_bits,
                                     stencil_bits,
                                     num_depth_stencil_bits,
                                     back_buffer_modes, 1,
                                     multisample_samples,
                                     num_msaa_modes,
                                     false, false);
      configs = driConcatConfigs(configs, new_configs);
   }

   if (configs == NULL) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   return configs;
}

static void
set_max_gl_versions(struct brw_screen *screen)
{
   __DRIscreen *dri_screen = screen->driScrnPriv;
   const bool has_astc = screen->devinfo.ver >= 9;

   switch (screen->devinfo.ver) {
   case 11:
   case 10:
   case 9:
   case 8:
      dri_screen->max_gl_core_version = 46;
      dri_screen->max_gl_compat_version = 30;
      dri_screen->max_gl_es1_version = 11;
      dri_screen->max_gl_es2_version = has_astc ? 32 : 31;
      break;
   case 7:
      dri_screen->max_gl_core_version = 33;
      if (can_do_pipelined_register_writes(screen)) {
         dri_screen->max_gl_core_version = 42;
         if (screen->devinfo.is_haswell && can_do_compute_dispatch(screen))
            dri_screen->max_gl_core_version = 43;
         if (screen->devinfo.is_haswell && can_do_mi_math_and_lrr(screen))
            dri_screen->max_gl_core_version = 45;
      }
      dri_screen->max_gl_compat_version = 30;
      dri_screen->max_gl_es1_version = 11;
      dri_screen->max_gl_es2_version = screen->devinfo.is_haswell ? 31 : 30;
      break;
   case 6:
      dri_screen->max_gl_core_version = 33;
      dri_screen->max_gl_compat_version = 30;
      dri_screen->max_gl_es1_version = 11;
      dri_screen->max_gl_es2_version = 30;
      break;
   case 5:
   case 4:
      dri_screen->max_gl_core_version = 0;
      dri_screen->max_gl_compat_version = 21;
      dri_screen->max_gl_es1_version = 11;
      dri_screen->max_gl_es2_version = 20;
      break;
   default:
      unreachable("unrecognized brw_screen::gen");
   }

   /* OpenGL 3.3+ requires GL_ARB_blend_func_extended.  Don't advertise those
    * versions if driconf disables the extension.
    */
   if (driQueryOptionb(&screen->optionCache, "disable_blend_func_extended")) {
      dri_screen->max_gl_core_version =
         MIN2(32, dri_screen->max_gl_core_version);
      dri_screen->max_gl_compat_version =
         MIN2(32, dri_screen->max_gl_compat_version);
   }

   /* Using the `allow_higher_compat_version` option during context creation
    * means that an application that doesn't request a specific version can be
    * given a version higher than 3.0.  However, an application still cannot
    * request a higher version.  For that to work, max_gl_compat_version must
    * be set.
    */
   if (dri_screen->max_gl_compat_version < dri_screen->max_gl_core_version) {
      if (driQueryOptionb(&screen->optionCache, "allow_higher_compat_version"))
         dri_screen->max_gl_compat_version = dri_screen->max_gl_core_version;
   }
}

static void
shader_debug_log_mesa(void *data, unsigned *msg_id, const char *fmt, ...)
{
   struct brw_context *brw = (struct brw_context *)data;
   va_list args;

   va_start(args, fmt);
   _mesa_gl_vdebugf(&brw->ctx, msg_id,
                    MESA_DEBUG_SOURCE_SHADER_COMPILER,
                    MESA_DEBUG_TYPE_OTHER,
                    MESA_DEBUG_SEVERITY_NOTIFICATION, fmt, args);
   va_end(args);
}

static void
shader_perf_log_mesa(void *data, unsigned *msg_id, const char *fmt, ...)
{
   struct brw_context *brw = (struct brw_context *)data;

   va_list args;
   va_start(args, fmt);

   if (INTEL_DEBUG(DEBUG_PERF)) {
      va_list args_copy;
      va_copy(args_copy, args);
      vfprintf(stderr, fmt, args_copy);
      va_end(args_copy);
   }

   if (brw->perf_debug) {
      _mesa_gl_vdebugf(&brw->ctx, msg_id,
                       MESA_DEBUG_SOURCE_SHADER_COMPILER,
                       MESA_DEBUG_TYPE_PERFORMANCE,
                       MESA_DEBUG_SEVERITY_MEDIUM, fmt, args);
   }
   va_end(args);
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the struct gl_config supported by this driver
 */
static const
__DRIconfig **brw_init_screen(__DRIscreen *dri_screen)
{
   struct brw_screen *screen;

   util_cpu_detect();

   if (dri_screen->image.loader) {
   } else if (dri_screen->dri2.loader->base.version <= 2 ||
       dri_screen->dri2.loader->getBuffersWithFormat == NULL) {
      fprintf(stderr,
              "\nERROR!  DRI2 loader with getBuffersWithFormat() "
              "support required\n");
      return NULL;
   }

   /* Allocate the private area */
   screen = rzalloc(NULL, struct brw_screen);
   if (!screen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return NULL;
   }
   /* parse information in __driConfigOptions */
   driOptionCache options;
   memset(&options, 0, sizeof(options));

   driParseOptionInfo(&options, brw_driconf, ARRAY_SIZE(brw_driconf));
   driParseConfigFiles(&screen->optionCache, &options, dri_screen->myNum,
                       "i965", NULL, NULL, NULL, 0, NULL, 0);
   driDestroyOptionCache(&options);

   screen->driScrnPriv = dri_screen;
   dri_screen->driverPrivate = (void *) screen;

   if (!intel_get_device_info_from_fd(dri_screen->fd, &screen->devinfo))
      return NULL;

   const struct intel_device_info *devinfo = &screen->devinfo;
   screen->deviceID = devinfo->chipset_id;

   if (devinfo->ver >= 12) {
      fprintf(stderr, "gfx12 and newer are not supported on i965\n");
      return NULL;
   }

   if (!brw_init_bufmgr(screen))
       return NULL;

   brw_process_intel_debug_variable();

   if (INTEL_DEBUG(DEBUG_SHADER_TIME) && devinfo->ver < 7) {
      fprintf(stderr,
              "shader_time debugging requires gfx7 (Ivybridge) or better.\n");
      intel_debug &= ~DEBUG_SHADER_TIME;
   }

   if (brw_get_integer(screen, I915_PARAM_MMAP_GTT_VERSION) >= 1) {
      /* Theorectically unlimited! At least for individual objects...
       *
       * Currently the entire (global) address space for all GTT maps is
       * limited to 64bits. That is all objects on the system that are
       * setup for GTT mmapping must fit within 64bits. An attempt to use
       * one that exceeds the limit with fail in brw_bo_map_gtt().
       *
       * Long before we hit that limit, we will be practically limited by
       * that any single object must fit in physical memory (RAM). The upper
       * limit on the CPU's address space is currently 48bits (Skylake), of
       * which only 39bits can be physical memory. (The GPU itself also has
       * a 48bit addressable virtual space.) We can fit over 32 million
       * objects of the current maximum allocable size before running out
       * of mmap space.
       */
      screen->max_gtt_map_object_size = UINT64_MAX;
   } else {
      /* Estimate the size of the mappable aperture into the GTT.  There's an
       * ioctl to get the whole GTT size, but not one to get the mappable subset.
       * It turns out it's basically always 256MB, though some ancient hardware
       * was smaller.
       */
      uint32_t gtt_size = 256 * 1024 * 1024;

      /* We don't want to map two objects such that a memcpy between them would
       * just fault one mapping in and then the other over and over forever.  So
       * we would need to divide the GTT size by 2.  Additionally, some GTT is
       * taken up by things like the framebuffer and the ringbuffer and such, so
       * be more conservative.
       */
      screen->max_gtt_map_object_size = gtt_size / 4;
   }

   screen->aperture_threshold = devinfo->aperture_bytes * 3 / 4;

   screen->hw_has_swizzling = brw_detect_swizzling(screen);
   screen->hw_has_timestamp = brw_detect_timestamp(screen);

   isl_device_init(&screen->isl_dev, &screen->devinfo,
                   screen->hw_has_swizzling);

   /* Gfx7-7.5 kernel requirements / command parser saga:
    *
    * - pre-v3.16:
    *   Haswell and Baytrail cannot use any privileged batchbuffer features.
    *
    *   Ivybridge has aliasing PPGTT on by default, which accidentally marks
    *   all batches secure, allowing them to use any feature with no checking.
    *   This is effectively equivalent to a command parser version of
    *   \infinity - everything is possible.
    *
    *   The command parser does not exist, and querying the version will
    *   return -EINVAL.
    *
    * - v3.16:
    *   The kernel enables the command parser by default, for systems with
    *   aliasing PPGTT enabled (Ivybridge and Haswell).  However, the
    *   hardware checker is still enabled, so Haswell and Baytrail cannot
    *   do anything.
    *
    *   Ivybridge goes from "everything is possible" to "only what the
    *   command parser allows" (if the user boots with i915.cmd_parser=0,
    *   then everything is possible again).  We can only safely use features
    *   allowed by the supported command parser version.
    *
    *   Annoyingly, I915_PARAM_CMD_PARSER_VERSION reports the static version
    *   implemented by the kernel, even if it's turned off.  So, checking
    *   for version > 0 does not mean that you can write registers.  We have
    *   to try it and see.  The version does, however, indicate the age of
    *   the kernel.
    *
    *   Instead of matching the hardware checker's behavior of converting
    *   privileged commands to MI_NOOP, it makes execbuf2 start returning
    *   -EINVAL, making it dangerous to try and use privileged features.
    *
    *   Effective command parser versions:
    *   - Haswell:   0 (reporting 1, writes don't work)
    *   - Baytrail:  0 (reporting 1, writes don't work)
    *   - Ivybridge: 1 (enabled) or infinite (disabled)
    *
    * - v3.17:
    *   Baytrail aliasing PPGTT is enabled, making it like Ivybridge:
    *   effectively version 1 (enabled) or infinite (disabled).
    *
    * - v3.19: f1f55cc0556031c8ee3fe99dae7251e78b9b653b
    *   Command parser v2 supports predicate writes.
    *
    *   - Haswell:   0 (reporting 1, writes don't work)
    *   - Baytrail:  2 (enabled) or infinite (disabled)
    *   - Ivybridge: 2 (enabled) or infinite (disabled)
    *
    *   So version >= 2 is enough to know that Ivybridge and Baytrail
    *   will work.  Haswell still can't do anything.
    *
    * - v4.0: Version 3 happened.  Largely not relevant.
    *
    * - v4.1: 6702cf16e0ba8b0129f5aa1b6609d4e9c70bc13b
    *   L3 config registers are properly saved and restored as part
    *   of the hardware context.  We can approximately detect this point
    *   in time by checking if I915_PARAM_REVISION is recognized - it
    *   landed in a later commit, but in the same release cycle.
    *
    * - v4.2: 245054a1fe33c06ad233e0d58a27ec7b64db9284
    *   Command parser finally gains secure batch promotion.  On Haswell,
    *   the hardware checker gets disabled, which finally allows it to do
    *   privileged commands.
    *
    *   I915_PARAM_CMD_PARSER_VERSION reports 3.  Effective versions:
    *   - Haswell:   3 (enabled) or 0 (disabled)
    *   - Baytrail:  3 (enabled) or infinite (disabled)
    *   - Ivybridge: 3 (enabled) or infinite (disabled)
    *
    *   Unfortunately, detecting this point in time is tricky, because
    *   no version bump happened when this important change occurred.
    *   On Haswell, if we can write any register, then the kernel is at
    *   least this new, and we can start trusting the version number.
    *
    * - v4.4: 2bbe6bbb0dc94fd4ce287bdac9e1bd184e23057b and
    *   Command parser reaches version 4, allowing access to Haswell
    *   atomic scratch and chicken3 registers.  If version >= 4, we know
    *   the kernel is new enough to support privileged features on all
    *   hardware.  However, the user might have disabled it...and the
    *   kernel will still report version 4.  So we still have to guess
    *   and check.
    *
    * - v4.4: 7b9748cb513a6bef4af87b79f0da3ff7e8b56cd8
    *   Command parser v5 whitelists indirect compute shader dispatch
    *   registers, needed for OpenGL 4.3 and later.
    *
    * - v4.8:
    *   Command parser v7 lets us use MI_MATH on Haswell.
    *
    *   Additionally, the kernel begins reporting version 0 when
    *   the command parser is disabled, allowing us to skip the
    *   guess-and-check step on Haswell.  Unfortunately, this also
    *   means that we can no longer use it as an indicator of the
    *   age of the kernel.
    */
   if (brw_get_param(screen, I915_PARAM_CMD_PARSER_VERSION,
                       &screen->cmd_parser_version) < 0) {
      /* Command parser does not exist - getparam is unrecognized */
      screen->cmd_parser_version = 0;
   }

   /* Kernel 4.13 retuired for exec object capture */
   if (brw_get_boolean(screen, I915_PARAM_HAS_EXEC_CAPTURE)) {
      screen->kernel_features |= KERNEL_ALLOWS_EXEC_CAPTURE;
   }

   if (brw_get_boolean(screen, I915_PARAM_HAS_EXEC_BATCH_FIRST)) {
      screen->kernel_features |= KERNEL_ALLOWS_EXEC_BATCH_FIRST;
   }

   if (!brw_detect_pipelined_so(screen)) {
      /* We can't do anything, so the effective version is 0. */
      screen->cmd_parser_version = 0;
   } else {
      screen->kernel_features |= KERNEL_ALLOWS_SOL_OFFSET_WRITES;
   }

   if (devinfo->ver >= 8 || screen->cmd_parser_version >= 2)
      screen->kernel_features |= KERNEL_ALLOWS_PREDICATE_WRITES;

   /* Haswell requires command parser version 4 in order to have L3
    * atomic scratch1 and chicken3 bits
    */
   if (devinfo->is_haswell && screen->cmd_parser_version >= 4) {
      screen->kernel_features |=
         KERNEL_ALLOWS_HSW_SCRATCH1_AND_ROW_CHICKEN3;
   }

   /* Haswell requires command parser version 6 in order to write to the
    * MI_MATH GPR registers, and version 7 in order to use
    * MI_LOAD_REGISTER_REG (which all users of MI_MATH use).
    */
   if (devinfo->ver >= 8 ||
       (devinfo->is_haswell && screen->cmd_parser_version >= 7)) {
      screen->kernel_features |= KERNEL_ALLOWS_MI_MATH_AND_LRR;
   }

   /* Gfx7 needs at least command parser version 5 to support compute */
   if (devinfo->ver >= 8 || screen->cmd_parser_version >= 5)
      screen->kernel_features |= KERNEL_ALLOWS_COMPUTE_DISPATCH;

   if (brw_get_boolean(screen, I915_PARAM_HAS_CONTEXT_ISOLATION))
      screen->kernel_features |= KERNEL_ALLOWS_CONTEXT_ISOLATION;

   const char *force_msaa = getenv("INTEL_FORCE_MSAA");
   if (force_msaa) {
      screen->winsys_msaa_samples_override =
         brw_quantize_num_samples(screen, atoi(force_msaa));
      printf("Forcing winsys sample count to %d\n",
             screen->winsys_msaa_samples_override);
   } else {
      screen->winsys_msaa_samples_override = -1;
   }

   set_max_gl_versions(screen);

   /* Notification of GPU resets requires hardware contexts and a kernel new
    * enough to support DRM_IOCTL_I915_GET_RESET_STATS.  If the ioctl is
    * supported, calling it with a context of 0 will either generate EPERM or
    * no error.  If the ioctl is not supported, it always generate EINVAL.
    * Use this to determine whether to advertise the __DRI2_ROBUSTNESS
    * extension to the loader.
    *
    * Don't even try on pre-Gfx6, since we don't attempt to use contexts there.
    */
   if (devinfo->ver >= 6) {
      struct drm_i915_reset_stats stats;
      memset(&stats, 0, sizeof(stats));

      const int ret = drmIoctl(screen->fd, DRM_IOCTL_I915_GET_RESET_STATS, &stats);

      screen->has_context_reset_notification =
         (ret != -1 || errno != EINVAL);
   }

   dri_screen->extensions = !screen->has_context_reset_notification
      ? screenExtensions : brwRobustScreenExtensions;

   screen->compiler = brw_compiler_create(screen, devinfo);
   screen->compiler->shader_debug_log = shader_debug_log_mesa;
   screen->compiler->shader_perf_log = shader_perf_log_mesa;

   /* Changing the meaning of constant buffer pointers from a dynamic state
    * offset to an absolute address is only safe if the kernel isolates other
    * contexts from our changes.
    */
   screen->compiler->constant_buffer_0_is_relative = devinfo->ver < 8 ||
      !(screen->kernel_features & KERNEL_ALLOWS_CONTEXT_ISOLATION);

   screen->compiler->glsl_compiler_options[MESA_SHADER_VERTEX].PositionAlwaysInvariant = driQueryOptionb(&screen->optionCache, "vs_position_always_invariant");
   screen->compiler->glsl_compiler_options[MESA_SHADER_TESS_EVAL].PositionAlwaysPrecise = driQueryOptionb(&screen->optionCache, "vs_position_always_precise");

   screen->compiler->supports_pull_constants = true;
   screen->compiler->compact_params = true;
   screen->compiler->lower_variable_group_size = true;

   screen->has_exec_fence =
     brw_get_boolean(screen, I915_PARAM_HAS_EXEC_FENCE);

   brw_screen_init_surface_formats(screen);

   if (INTEL_DEBUG(DEBUG_BATCH | DEBUG_SUBMIT)) {
      unsigned int caps = brw_get_integer(screen, I915_PARAM_HAS_SCHEDULER);
      if (caps) {
         fprintf(stderr, "Kernel scheduler detected: %08x\n", caps);
         if (caps & I915_SCHEDULER_CAP_PRIORITY)
            fprintf(stderr, "  - User priority sorting enabled\n");
         if (caps & I915_SCHEDULER_CAP_PREEMPTION)
            fprintf(stderr, "  - Preemption enabled\n");
      }
   }

   brw_disk_cache_init(screen);

   return (const __DRIconfig**) brw_screen_make_configs(dri_screen);
}

struct brw_buffer {
   __DRIbuffer base;
   struct brw_bo *bo;
};

static __DRIbuffer *
brw_allocate_buffer(__DRIscreen *dri_screen,
                    unsigned attachment, unsigned format,
                    int width, int height)
{
   struct brw_screen *screen = dri_screen->driverPrivate;

   assert(attachment == __DRI_BUFFER_FRONT_LEFT ||
          attachment == __DRI_BUFFER_BACK_LEFT);

   struct brw_buffer *buffer = calloc(1, sizeof *buffer);
   if (buffer == NULL)
      return NULL;

   /* The front and back buffers are color buffers, which are X tiled. GFX9+
    * supports Y tiled and compressed buffers, but there is no way to plumb that
    * through to here. */
   uint32_t pitch;
   int cpp = format / 8;
   buffer->bo = brw_bo_alloc_tiled_2d(screen->bufmgr,
                                      __func__,
                                      width,
                                      height,
                                      cpp,
                                      BRW_MEMZONE_OTHER,
                                      I915_TILING_X, &pitch,
                                      BO_ALLOC_BUSY);

   if (buffer->bo == NULL) {
      free(buffer);
      return NULL;
   }

   brw_bo_flink(buffer->bo, &buffer->base.name);

   buffer->base.attachment = attachment;
   buffer->base.cpp = cpp;
   buffer->base.pitch = pitch;

   return &buffer->base;
}

static void
brw_release_buffer(UNUSED __DRIscreen *dri_screen, __DRIbuffer *_buffer)
{
   struct brw_buffer *buffer = (struct brw_buffer *) _buffer;

   brw_bo_unreference(buffer->bo);
   free(buffer);
}

static const struct __DriverAPIRec brw_driver_api = {
   .InitScreen           = brw_init_screen,
   .DestroyScreen        = brw_destroy_screen,
   .CreateContext        = brw_create_context,
   .DestroyContext       = brw_destroy_context,
   .CreateBuffer         = brw_create_buffer,
   .DestroyBuffer        = brw_destroy_buffer,
   .MakeCurrent          = brw_make_current,
   .UnbindContext        = brw_unbind_context,
   .AllocateBuffer       = brw_allocate_buffer,
   .ReleaseBuffer        = brw_release_buffer
};

static const struct __DRIDriverVtableExtensionRec brw_vtable = {
   .base = { __DRI_DRIVER_VTABLE, 1 },
   .vtable = &brw_driver_api,
};

static const __DRIextension *brw_driver_extensions[] = {
    &driCoreExtension.base,
    &driImageDriverExtension.base,
    &driDRI2Extension.base,
    &brw_vtable.base,
    &brw_config_options.base,
    NULL
};

PUBLIC const __DRIextension **__driDriverGetExtensions_i965(void)
{
   globalDriverAPI = &brw_driver_api;

   return brw_driver_extensions;
}
