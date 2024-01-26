/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "compiler/nir/nir.h"
#include "main/context.h"
#include "main/blend.h"
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "main/shaderimage.h"
#include "main/teximage.h"
#include "program/prog_parameter.h"
#include "program/prog_instruction.h"
#include "main/framebuffer.h"
#include "main/shaderapi.h"

#include "isl/isl.h"

#include "brw_mipmap_tree.h"
#include "brw_batch.h"
#include "brw_tex.h"
#include "brw_fbo.h"
#include "brw_buffer_objects.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

static const uint32_t wb_mocs[] = {
   [7] = GFX7_MOCS_L3,
   [8] = BDW_MOCS_WB,
   [9] = SKL_MOCS_WB,
   [10] = CNL_MOCS_WB,
   [11] = ICL_MOCS_WB,
};

static const uint32_t pte_mocs[] = {
   [7] = GFX7_MOCS_L3,
   [8] = BDW_MOCS_PTE,
   [9] = SKL_MOCS_PTE,
   [10] = CNL_MOCS_PTE,
   [11] = ICL_MOCS_PTE,
};

uint32_t
brw_get_bo_mocs(const struct intel_device_info *devinfo, struct brw_bo *bo)
{
   return (bo && bo->external ? pte_mocs : wb_mocs)[devinfo->ver];
}

static void
get_isl_surf(struct brw_context *brw, struct brw_mipmap_tree *mt,
             GLenum target, struct isl_view *view,
             uint32_t *tile_x, uint32_t *tile_y,
             uint32_t *offset, struct isl_surf *surf)
{
   *surf = mt->surf;

   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const enum isl_dim_layout dim_layout =
      get_isl_dim_layout(devinfo, mt->surf.tiling, target);

   surf->dim = get_isl_surf_dim(target);

   if (surf->dim_layout == dim_layout)
      return;

   /* The layout of the specified texture target is not compatible with the
    * actual layout of the miptree structure in memory -- You're entering
    * dangerous territory, this can only possibly work if you only intended
    * to access a single level and slice of the texture, and the hardware
    * supports the tile offset feature in order to allow non-tile-aligned
    * base offsets, since we'll have to point the hardware to the first
    * texel of the level instead of relying on the usual base level/layer
    * controls.
    */
   assert(devinfo->has_surface_tile_offset);
   assert(view->levels == 1 && view->array_len == 1);
   assert(*tile_x == 0 && *tile_y == 0);

   *offset += brw_miptree_get_tile_offsets(mt, view->base_level,
                                           view->base_array_layer,
                                           tile_x, tile_y);

   /* Minify the logical dimensions of the texture. */
   const unsigned l = view->base_level - mt->first_level;
   surf->logical_level0_px.width = minify(surf->logical_level0_px.width, l);
   surf->logical_level0_px.height = surf->dim <= ISL_SURF_DIM_1D ? 1 :
      minify(surf->logical_level0_px.height, l);
   surf->logical_level0_px.depth = surf->dim <= ISL_SURF_DIM_2D ? 1 :
      minify(surf->logical_level0_px.depth, l);

   /* Only the base level and layer can be addressed with the overridden
    * layout.
    */
   surf->logical_level0_px.array_len = 1;
   surf->levels = 1;
   surf->dim_layout = dim_layout;

   /* The requested slice of the texture is now at the base level and
    * layer.
    */
   view->base_level = 0;
   view->base_array_layer = 0;
}

static void
brw_emit_surface_state(struct brw_context *brw,
                       struct brw_mipmap_tree *mt,
                       GLenum target, struct isl_view view,
                       enum isl_aux_usage aux_usage,
                       uint32_t *surf_offset, int surf_index,
                       unsigned reloc_flags)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t tile_x = mt->level[0].level_x;
   uint32_t tile_y = mt->level[0].level_y;
   uint32_t offset = mt->offset;

   struct isl_surf surf;

   get_isl_surf(brw, mt, target, &view, &tile_x, &tile_y, &offset, &surf);

   union isl_color_value clear_color = { .u32 = { 0, 0, 0, 0 } };

   struct brw_bo *aux_bo = NULL;
   struct isl_surf *aux_surf = NULL;
   uint64_t aux_offset = 0;
   struct brw_bo *clear_bo = NULL;
   uint64_t clear_offset = 0;

   if (aux_usage != ISL_AUX_USAGE_NONE) {
      aux_surf = &mt->aux_buf->surf;
      aux_bo = mt->aux_buf->bo;
      aux_offset = mt->aux_buf->offset;

      /* We only really need a clear color if we also have an auxiliary
       * surface.  Without one, it does nothing.
       */
      clear_color = brw_miptree_get_clear_color(mt, &clear_bo, &clear_offset);
   }

   void *state = brw_state_batch(brw,
                                 brw->isl_dev.ss.size,
                                 brw->isl_dev.ss.align,
                                 surf_offset);

   isl_surf_fill_state(&brw->isl_dev, state, .surf = &surf, .view = &view,
                       .address = brw_state_reloc(&brw->batch,
                                                  *surf_offset + brw->isl_dev.ss.addr_offset,
                                                  mt->bo, offset, reloc_flags),
                       .aux_surf = aux_surf, .aux_usage = aux_usage,
                       .aux_address = aux_offset,
                       .mocs = brw_get_bo_mocs(devinfo, mt->bo),
                       .clear_color = clear_color,
                       .use_clear_address = clear_bo != NULL,
                       .clear_address = clear_offset,
                       .x_offset_sa = tile_x, .y_offset_sa = tile_y);
   if (aux_surf) {
      /* On gfx7 and prior, the upper 20 bits of surface state DWORD 6 are the
       * upper 20 bits of the GPU address of the MCS buffer; the lower 12 bits
       * contain other control information.  Since buffer addresses are always
       * on 4k boundaries (and thus have their lower 12 bits zero), we can use
       * an ordinary reloc to do the necessary address translation.
       *
       * FIXME: move to the point of assignment.
       */
      assert((aux_offset & 0xfff) == 0);

      if (devinfo->ver >= 8) {
         uint64_t *aux_addr = state + brw->isl_dev.ss.aux_addr_offset;
         *aux_addr = brw_state_reloc(&brw->batch,
                                     *surf_offset +
                                     brw->isl_dev.ss.aux_addr_offset,
                                     aux_bo, *aux_addr,
                                     reloc_flags);
      } else {
         uint32_t *aux_addr = state + brw->isl_dev.ss.aux_addr_offset;
         *aux_addr = brw_state_reloc(&brw->batch,
                                     *surf_offset +
                                     brw->isl_dev.ss.aux_addr_offset,
                                     aux_bo, *aux_addr,
                                     reloc_flags);

      }
   }

   if (clear_bo != NULL) {
      /* Make sure the offset is aligned with a cacheline. */
      assert((clear_offset & 0x3f) == 0);
      uint64_t *clear_address =
            state + brw->isl_dev.ss.clear_color_state_offset;
      *clear_address = brw_state_reloc(&brw->batch,
                                       *surf_offset +
                                       brw->isl_dev.ss.clear_color_state_offset,
                                       clear_bo, *clear_address, reloc_flags);
   }
}

static uint32_t
gfx6_update_renderbuffer_surface(struct brw_context *brw,
                                 struct gl_renderbuffer *rb,
                                 unsigned unit,
                                 uint32_t surf_index)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   struct brw_mipmap_tree *mt = irb->mt;

   assert(brw_render_target_supported(brw, rb));

   mesa_format rb_format = _mesa_get_render_format(ctx, brw_rb_format(irb));
   if (unlikely(!brw->mesa_format_supports_render[rb_format])) {
      _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
                    __func__, _mesa_get_format_name(rb_format));
   }
   enum isl_format isl_format = brw->mesa_to_isl_render_format[rb_format];

   struct isl_view view = {
      .format = isl_format,
      .base_level = irb->mt_level - irb->mt->first_level,
      .levels = 1,
      .base_array_layer = irb->mt_layer,
      .array_len = MAX2(irb->layer_count, 1),
      .swizzle = ISL_SWIZZLE_IDENTITY,
      .usage = ISL_SURF_USAGE_RENDER_TARGET_BIT,
   };

   uint32_t offset;
   brw_emit_surface_state(brw, mt, mt->target, view,
                          brw->draw_aux_usage[unit],
                          &offset, surf_index,
                          RELOC_WRITE);
   return offset;
}

GLuint
translate_tex_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY_EXT:
      return BRW_SURFACE_1D;

   case GL_TEXTURE_RECTANGLE_NV:
      return BRW_SURFACE_2D;

   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      return BRW_SURFACE_2D;

   case GL_TEXTURE_3D:
      return BRW_SURFACE_3D;

   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      return BRW_SURFACE_CUBE;

   default:
      unreachable("not reached");
   }
}

uint32_t
brw_get_surface_tiling_bits(enum isl_tiling tiling)
{
   switch (tiling) {
   case ISL_TILING_X:
      return BRW_SURFACE_TILED;
   case ISL_TILING_Y0:
      return BRW_SURFACE_TILED | BRW_SURFACE_TILED_Y;
   default:
      return 0;
   }
}


uint32_t
brw_get_surface_num_multisamples(unsigned num_samples)
{
   if (num_samples > 1)
      return BRW_SURFACE_MULTISAMPLECOUNT_4;
   else
      return BRW_SURFACE_MULTISAMPLECOUNT_1;
}

/**
 * Compute the combination of DEPTH_TEXTURE_MODE and EXT_texture_swizzle
 * swizzling.
 */
int
brw_get_texture_swizzle(const struct gl_context *ctx,
                        const struct gl_texture_object *t)
{
   const struct gl_texture_image *img = t->Image[0][t->Attrib.BaseLevel];

   int swizzles[SWIZZLE_NIL + 1] = {
      SWIZZLE_X,
      SWIZZLE_Y,
      SWIZZLE_Z,
      SWIZZLE_W,
      SWIZZLE_ZERO,
      SWIZZLE_ONE,
      SWIZZLE_NIL
   };

   if (img->_BaseFormat == GL_DEPTH_COMPONENT ||
       img->_BaseFormat == GL_DEPTH_STENCIL) {
      GLenum depth_mode = t->Attrib.DepthMode;

      /* In ES 3.0, DEPTH_TEXTURE_MODE is expected to be GL_RED for textures
       * with depth component data specified with a sized internal format.
       * Otherwise, it's left at the old default, GL_LUMINANCE.
       */
      if (_mesa_is_gles3(ctx) &&
          img->InternalFormat != GL_DEPTH_COMPONENT &&
          img->InternalFormat != GL_DEPTH_STENCIL) {
         depth_mode = GL_RED;
      }

      switch (depth_mode) {
      case GL_ALPHA:
         swizzles[0] = SWIZZLE_ZERO;
         swizzles[1] = SWIZZLE_ZERO;
         swizzles[2] = SWIZZLE_ZERO;
         swizzles[3] = SWIZZLE_X;
         break;
      case GL_LUMINANCE:
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_X;
         swizzles[2] = SWIZZLE_X;
         swizzles[3] = SWIZZLE_ONE;
         break;
      case GL_INTENSITY:
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_X;
         swizzles[2] = SWIZZLE_X;
         swizzles[3] = SWIZZLE_X;
         break;
      case GL_RED:
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_ZERO;
         swizzles[2] = SWIZZLE_ZERO;
         swizzles[3] = SWIZZLE_ONE;
         break;
      }
   }

   GLenum datatype = _mesa_get_format_datatype(img->TexFormat);

   /* If the texture's format is alpha-only, force R, G, and B to
    * 0.0. Similarly, if the texture's format has no alpha channel,
    * force the alpha value read to 1.0. This allows for the
    * implementation to use an RGBA texture for any of these formats
    * without leaking any unexpected values.
    */
   switch (img->_BaseFormat) {
   case GL_ALPHA:
      swizzles[0] = SWIZZLE_ZERO;
      swizzles[1] = SWIZZLE_ZERO;
      swizzles[2] = SWIZZLE_ZERO;
      break;
   case GL_LUMINANCE:
      if (t->_IsIntegerFormat || datatype == GL_SIGNED_NORMALIZED) {
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_X;
         swizzles[2] = SWIZZLE_X;
         swizzles[3] = SWIZZLE_ONE;
      }
      break;
   case GL_LUMINANCE_ALPHA:
      if (datatype == GL_SIGNED_NORMALIZED) {
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_X;
         swizzles[2] = SWIZZLE_X;
         swizzles[3] = SWIZZLE_W;
      }
      break;
   case GL_INTENSITY:
      if (datatype == GL_SIGNED_NORMALIZED) {
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_X;
         swizzles[2] = SWIZZLE_X;
         swizzles[3] = SWIZZLE_X;
      }
      break;
   case GL_RED:
      if (img->TexFormat == MESA_FORMAT_R_SRGB8) {
         swizzles[0] = SWIZZLE_X;
         swizzles[1] = SWIZZLE_ZERO;
         swizzles[2] = SWIZZLE_ZERO;
         swizzles[3] = SWIZZLE_ONE;
         break;
      }
      FALLTHROUGH;
   case GL_RG:
   case GL_RGB:
      if (_mesa_get_format_bits(img->TexFormat, GL_ALPHA_BITS) > 0 ||
          img->TexFormat == MESA_FORMAT_RGB_DXT1 ||
          img->TexFormat == MESA_FORMAT_SRGB_DXT1)
         swizzles[3] = SWIZZLE_ONE;
      break;
   }

   return MAKE_SWIZZLE4(swizzles[GET_SWZ(t->Attrib._Swizzle, 0)],
                        swizzles[GET_SWZ(t->Attrib._Swizzle, 1)],
                        swizzles[GET_SWZ(t->Attrib._Swizzle, 2)],
                        swizzles[GET_SWZ(t->Attrib._Swizzle, 3)]);
}

/**
 * Convert an swizzle enumeration (i.e. SWIZZLE_X) to one of the Gfx7.5+
 * "Shader Channel Select" enumerations (i.e. HSW_SCS_RED).  The mappings are
 *
 * SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W, SWIZZLE_ZERO, SWIZZLE_ONE
 *         0          1          2          3             4            5
 *         4          5          6          7             0            1
 *   SCS_RED, SCS_GREEN,  SCS_BLUE, SCS_ALPHA,     SCS_ZERO,     SCS_ONE
 *
 * which is simply adding 4 then modding by 8 (or anding with 7).
 *
 * We then may need to apply workarounds for textureGather hardware bugs.
 */
static unsigned
swizzle_to_scs(GLenum swizzle, bool need_green_to_blue)
{
   unsigned scs = (swizzle + 4) & 7;

   return (need_green_to_blue && scs == HSW_SCS_GREEN) ? HSW_SCS_BLUE : scs;
}

static void brw_update_texture_surface(struct gl_context *ctx,
                           unsigned unit,
                           uint32_t *surf_offset,
                           bool for_gather,
                           bool for_txf,
                           uint32_t plane)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_texture_object *obj = ctx->Texture.Unit[unit]._Current;

   if (obj->Target == GL_TEXTURE_BUFFER) {
      brw_update_buffer_texture_surface(ctx, unit, surf_offset);

   } else {
      struct brw_texture_object *intel_obj = brw_texture_object(obj);
      struct brw_mipmap_tree *mt = intel_obj->mt;

      if (plane > 0) {
         if (mt->plane[plane - 1] == NULL)
            return;
         mt = mt->plane[plane - 1];
      }

      struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
      /* If this is a view with restricted NumLayers, then our effective depth
       * is not just the miptree depth.
       */
      unsigned view_num_layers;
      if (obj->Immutable && obj->Target != GL_TEXTURE_3D) {
         view_num_layers = obj->Attrib.NumLayers;
      } else {
         view_num_layers = mt->surf.dim == ISL_SURF_DIM_3D ?
                              mt->surf.logical_level0_px.depth :
                              mt->surf.logical_level0_px.array_len;
      }

      /* Handling GL_ALPHA as a surface format override breaks 1.30+ style
       * texturing functions that return a float, as our code generation always
       * selects the .x channel (which would always be 0).
       */
      struct gl_texture_image *firstImage = obj->Image[0][obj->Attrib.BaseLevel];
      const bool alpha_depth = obj->Attrib.DepthMode == GL_ALPHA &&
         (firstImage->_BaseFormat == GL_DEPTH_COMPONENT ||
          firstImage->_BaseFormat == GL_DEPTH_STENCIL);
      const unsigned swizzle = (unlikely(alpha_depth) ? SWIZZLE_XYZW :
                                brw_get_texture_swizzle(&brw->ctx, obj));

      mesa_format mesa_fmt;
      if (firstImage->_BaseFormat == GL_DEPTH_STENCIL ||
          firstImage->_BaseFormat == GL_DEPTH_COMPONENT) {
         /* The format from intel_obj may be a combined depth stencil format
          * when we just want depth.  Pull it from the miptree instead.  This
          * is safe because texture views aren't allowed on depth/stencil.
          */
         mesa_fmt = mt->format;
      } else if (brw_miptree_has_etc_shadow(brw, mt)) {
         mesa_fmt = mt->shadow_mt->format;
      } else if (plane > 0) {
         mesa_fmt = mt->format;
      } else {
         mesa_fmt = intel_obj->_Format;
      }
      enum isl_format format = translate_tex_format(brw, mesa_fmt,
                                                    for_txf ? GL_DECODE_EXT :
                                                    sampler->Attrib.sRGBDecode);

      /* Implement gfx6 and gfx7 gather work-around */
      bool need_green_to_blue = false;
      if (for_gather) {
         if (devinfo->ver == 7 && (format == ISL_FORMAT_R32G32_FLOAT ||
                                   format == ISL_FORMAT_R32G32_SINT ||
                                   format == ISL_FORMAT_R32G32_UINT)) {
            format = ISL_FORMAT_R32G32_FLOAT_LD;
            need_green_to_blue = devinfo->is_haswell;
         } else if (devinfo->ver == 6) {
            /* Sandybridge's gather4 message is broken for integer formats.
             * To work around this, we pretend the surface is UNORM for
             * 8 or 16-bit formats, and emit shader instructions to recover
             * the real INT/UINT value.  For 32-bit formats, we pretend
             * the surface is FLOAT, and simply reinterpret the resulting
             * bits.
             */
            switch (format) {
            case ISL_FORMAT_R8_SINT:
            case ISL_FORMAT_R8_UINT:
               format = ISL_FORMAT_R8_UNORM;
               break;

            case ISL_FORMAT_R16_SINT:
            case ISL_FORMAT_R16_UINT:
               format = ISL_FORMAT_R16_UNORM;
               break;

            case ISL_FORMAT_R32_SINT:
            case ISL_FORMAT_R32_UINT:
               format = ISL_FORMAT_R32_FLOAT;
               break;

            default:
               break;
            }
         }
      }

      if (obj->StencilSampling && firstImage->_BaseFormat == GL_DEPTH_STENCIL) {
         if (devinfo->ver <= 7) {
            assert(mt->shadow_mt && !mt->stencil_mt->shadow_needs_update);
            mt = mt->shadow_mt;
         } else {
            mt = mt->stencil_mt;
         }
         format = ISL_FORMAT_R8_UINT;
      } else if (devinfo->ver <= 7 && mt->format == MESA_FORMAT_S_UINT8) {
         assert(mt->shadow_mt && !mt->shadow_needs_update);
         mt = mt->shadow_mt;
         format = ISL_FORMAT_R8_UINT;
      } else if (brw_miptree_needs_fake_etc(brw, mt)) {
         assert(mt->shadow_mt && !mt->shadow_needs_update);
         mt = mt->shadow_mt;
      }

      const int surf_index = surf_offset - &brw->wm.base.surf_offset[0];

      struct isl_view view = {
         .format = format,
         .base_level = obj->Attrib.MinLevel + obj->Attrib.BaseLevel,
         .levels = intel_obj->_MaxLevel - obj->Attrib.BaseLevel + 1,
         .base_array_layer = obj->Attrib.MinLayer,
         .array_len = view_num_layers,
         .swizzle = {
            .r = swizzle_to_scs(GET_SWZ(swizzle, 0), need_green_to_blue),
            .g = swizzle_to_scs(GET_SWZ(swizzle, 1), need_green_to_blue),
            .b = swizzle_to_scs(GET_SWZ(swizzle, 2), need_green_to_blue),
            .a = swizzle_to_scs(GET_SWZ(swizzle, 3), need_green_to_blue),
         },
         .usage = ISL_SURF_USAGE_TEXTURE_BIT,
      };

      /* On Ivy Bridge and earlier, we handle texture swizzle with shader
       * code.  The actual surface swizzle should be identity.
       */
      if (devinfo->verx10 <= 70)
         view.swizzle = ISL_SWIZZLE_IDENTITY;

      if (obj->Target == GL_TEXTURE_CUBE_MAP ||
          obj->Target == GL_TEXTURE_CUBE_MAP_ARRAY)
         view.usage |= ISL_SURF_USAGE_CUBE_BIT;

      enum isl_aux_usage aux_usage =
         brw_miptree_texture_aux_usage(brw, mt, format,
                                       brw->gfx9_astc5x5_wa_tex_mask);

      brw_emit_surface_state(brw, mt, mt->target, view, aux_usage,
                             surf_offset, surf_index,
                             0);
   }
}

void
brw_emit_buffer_surface_state(struct brw_context *brw,
                              uint32_t *out_offset,
                              struct brw_bo *bo,
                              unsigned buffer_offset,
                              enum isl_format format,
                              unsigned buffer_size,
                              unsigned pitch,
                              unsigned reloc_flags)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t *dw = brw_state_batch(brw,
                                  brw->isl_dev.ss.size,
                                  brw->isl_dev.ss.align,
                                  out_offset);

   isl_buffer_fill_state(&brw->isl_dev, dw,
                         .address = !bo ? buffer_offset :
                                    brw_state_reloc(&brw->batch,
                                                    *out_offset + brw->isl_dev.ss.addr_offset,
                                                    bo, buffer_offset,
                                                    reloc_flags),
                         .size_B = buffer_size,
                         .format = format,
                         .swizzle = ISL_SWIZZLE_IDENTITY,
                         .stride_B = pitch,
                         .mocs = brw_get_bo_mocs(devinfo, bo));
}

static unsigned
buffer_texture_range_size(struct brw_context *brw,
                          struct gl_texture_object *obj)
{
   assert(obj->Target == GL_TEXTURE_BUFFER);
   const unsigned texel_size = _mesa_get_format_bytes(obj->_BufferObjectFormat);
   const unsigned buffer_size = (!obj->BufferObject ? 0 :
                                 obj->BufferObject->Size);
   const unsigned buffer_offset = MIN2(buffer_size, obj->BufferOffset);

   /* The ARB_texture_buffer_specification says:
    *
    *    "The number of texels in the buffer texture's texel array is given by
    *
    *       floor(<buffer_size> / (<components> * sizeof(<base_type>)),
    *
    *     where <buffer_size> is the size of the buffer object, in basic
    *     machine units and <components> and <base_type> are the element count
    *     and base data type for elements, as specified in Table X.1.  The
    *     number of texels in the texel array is then clamped to the
    *     implementation-dependent limit MAX_TEXTURE_BUFFER_SIZE_ARB."
    *
    * We need to clamp the size in bytes to MAX_TEXTURE_BUFFER_SIZE * stride,
    * so that when ISL divides by stride to obtain the number of texels, that
    * texel count is clamped to MAX_TEXTURE_BUFFER_SIZE.
    */
   return MIN3((unsigned)obj->BufferSize,
               buffer_size - buffer_offset,
               brw->ctx.Const.MaxTextureBufferSize * texel_size);
}

static void
emit_null_surface_state(struct brw_context *brw,
                        const struct gl_framebuffer *fb,
                        uint32_t *out_offset);

void
brw_update_buffer_texture_surface(struct gl_context *ctx,
                                  unsigned unit,
                                  uint32_t *surf_offset)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct brw_buffer_object *intel_obj =
      brw_buffer_object(tObj->BufferObject);
   const unsigned size = buffer_texture_range_size(brw, tObj);
   struct brw_bo *bo = NULL;
   mesa_format format = tObj->_BufferObjectFormat;
   const enum isl_format isl_format = brw_isl_format_for_mesa_format(format);
   int texel_size = _mesa_get_format_bytes(format);

   if (tObj->BufferObject == NULL) {
      emit_null_surface_state(brw, NULL, surf_offset);
      return;
   }

   if (intel_obj)
      bo = brw_bufferobj_buffer(brw, intel_obj, tObj->BufferOffset, size,
                                false);

   if (isl_format == ISL_FORMAT_UNSUPPORTED) {
      _mesa_problem(NULL, "bad format %s for texture buffer\n",
                    _mesa_get_format_name(format));
   }

   brw_emit_buffer_surface_state(brw, surf_offset, bo,
                                 tObj->BufferOffset,
                                 isl_format,
                                 size,
                                 texel_size,
                                 0);
}

/**
 * Set up a binding table entry for use by stream output logic (transform
 * feedback).
 *
 * buffer_size_minus_1 must be less than BRW_MAX_NUM_BUFFER_ENTRIES.
 */
void
brw_update_sol_surface(struct brw_context *brw,
                       struct gl_buffer_object *buffer_obj,
                       uint32_t *out_offset, unsigned num_vector_components,
                       unsigned stride_dwords, unsigned offset_dwords)
{
   struct brw_buffer_object *intel_bo = brw_buffer_object(buffer_obj);
   uint32_t offset_bytes = 4 * offset_dwords;
   struct brw_bo *bo = brw_bufferobj_buffer(brw, intel_bo,
                                            offset_bytes,
                                            buffer_obj->Size - offset_bytes,
                                            true);
   uint32_t *surf = brw_state_batch(brw, 6 * 4, 32, out_offset);
   uint32_t pitch_minus_1 = 4*stride_dwords - 1;
   size_t size_dwords = buffer_obj->Size / 4;
   uint32_t buffer_size_minus_1, width, height, depth, surface_format;

   /* FIXME: can we rely on core Mesa to ensure that the buffer isn't
    * too big to map using a single binding table entry?
    */
   assert((size_dwords - offset_dwords) / stride_dwords
          <= BRW_MAX_NUM_BUFFER_ENTRIES);

   if (size_dwords > offset_dwords + num_vector_components) {
      /* There is room for at least 1 transform feedback output in the buffer.
       * Compute the number of additional transform feedback outputs the
       * buffer has room for.
       */
      buffer_size_minus_1 =
         (size_dwords - offset_dwords - num_vector_components) / stride_dwords;
   } else {
      /* There isn't even room for a single transform feedback output in the
       * buffer.  We can't configure the binding table entry to prevent output
       * entirely; we'll have to rely on the geometry shader to detect
       * overflow.  But to minimize the damage in case of a bug, set up the
       * binding table entry to just allow a single output.
       */
      buffer_size_minus_1 = 0;
   }
   width = buffer_size_minus_1 & 0x7f;
   height = (buffer_size_minus_1 & 0xfff80) >> 7;
   depth = (buffer_size_minus_1 & 0x7f00000) >> 20;

   switch (num_vector_components) {
   case 1:
      surface_format = ISL_FORMAT_R32_FLOAT;
      break;
   case 2:
      surface_format = ISL_FORMAT_R32G32_FLOAT;
      break;
   case 3:
      surface_format = ISL_FORMAT_R32G32B32_FLOAT;
      break;
   case 4:
      surface_format = ISL_FORMAT_R32G32B32A32_FLOAT;
      break;
   default:
      unreachable("Invalid vector size for transform feedback output");
   }

   surf[0] = BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
      BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
      surface_format << BRW_SURFACE_FORMAT_SHIFT |
      BRW_SURFACE_RC_READ_WRITE;
   surf[1] = brw_state_reloc(&brw->batch,
                             *out_offset + 4, bo, offset_bytes, RELOC_WRITE);
   surf[2] = (width << BRW_SURFACE_WIDTH_SHIFT |
              height << BRW_SURFACE_HEIGHT_SHIFT);
   surf[3] = (depth << BRW_SURFACE_DEPTH_SHIFT |
              pitch_minus_1 << BRW_SURFACE_PITCH_SHIFT);
   surf[4] = 0;
   surf[5] = 0;
}

/* Creates a new WM constant buffer reflecting the current fragment program's
 * constants, if needed by the fragment program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
brw_upload_wm_pull_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->wm.base;
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct brw_program *fp =
      (struct brw_program *) brw->programs[MESA_SHADER_FRAGMENT];

   /* BRW_NEW_FS_PROG_DATA */
   struct brw_stage_prog_data *prog_data = brw->wm.base.prog_data;

   _mesa_shader_write_subroutine_indices(&brw->ctx, MESA_SHADER_FRAGMENT);
   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_pull_constants(brw, BRW_NEW_SURFACES, &fp->program,
                             stage_state, prog_data);
}

const struct brw_tracked_state brw_wm_pull_constants = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_FRAGMENT_PROGRAM |
             BRW_NEW_FS_PROG_DATA,
   },
   .emit = brw_upload_wm_pull_constants,
};

/**
 * Creates a null renderbuffer surface.
 *
 * This is used when the shader doesn't write to any color output.  An FB
 * write to target 0 will still be emitted, because that's how the thread is
 * terminated (and computed depth is returned), so we need to have the
 * hardware discard the target 0 color output..
 */
static void
emit_null_surface_state(struct brw_context *brw,
                        const struct gl_framebuffer *fb,
                        uint32_t *out_offset)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t *surf = brw_state_batch(brw,
                                    brw->isl_dev.ss.size,
                                    brw->isl_dev.ss.align,
                                    out_offset);

   /* Use the fb dimensions or 1x1x1 */
   const unsigned width   = fb ? _mesa_geometric_width(fb)   : 1;
   const unsigned height  = fb ? _mesa_geometric_height(fb)  : 1;
   const unsigned samples = fb ? _mesa_geometric_samples(fb) : 1;

   if (devinfo->ver != 6 || samples <= 1) {
      isl_null_fill_state(&brw->isl_dev, surf,
                          .size = isl_extent3d(width, height, 1));
      return;
   }

   /* On Gfx6, null render targets seem to cause GPU hangs when multisampling.
    * So work around this problem by rendering into dummy color buffer.
    *
    * To decrease the amount of memory needed by the workaround buffer, we
    * set its pitch to 128 bytes (the width of a Y tile).  This means that
    * the amount of memory needed for the workaround buffer is
    * (width_in_tiles + height_in_tiles - 1) tiles.
    *
    * Note that since the workaround buffer will be interpreted by the
    * hardware as an interleaved multisampled buffer, we need to compute
    * width_in_tiles and height_in_tiles by dividing the width and height
    * by 16 rather than the normal Y-tile size of 32.
    */
   unsigned width_in_tiles = ALIGN(width, 16) / 16;
   unsigned height_in_tiles = ALIGN(height, 16) / 16;
   unsigned pitch_minus_1 = 127;
   unsigned size_needed = (width_in_tiles + height_in_tiles - 1) * 4096;
   brw_get_scratch_bo(brw, &brw->wm.multisampled_null_render_target_bo,
                      size_needed);

   surf[0] = (BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
              ISL_FORMAT_B8G8R8A8_UNORM << BRW_SURFACE_FORMAT_SHIFT);
   surf[1] = brw_state_reloc(&brw->batch, *out_offset + 4,
                             brw->wm.multisampled_null_render_target_bo,
                             0, RELOC_WRITE);

   surf[2] = ((width - 1) << BRW_SURFACE_WIDTH_SHIFT |
              (height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   /* From Sandy bridge PRM, Vol4 Part1 p82 (Tiled Surface: Programming
    * Notes):
    *
    *     If Surface Type is SURFTYPE_NULL, this field must be TRUE
    */
   surf[3] = (BRW_SURFACE_TILED | BRW_SURFACE_TILED_Y |
              pitch_minus_1 << BRW_SURFACE_PITCH_SHIFT);
   surf[4] = BRW_SURFACE_MULTISAMPLECOUNT_4;
   surf[5] = 0;
}

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static uint32_t
gfx4_update_renderbuffer_surface(struct brw_context *brw,
                                 struct gl_renderbuffer *rb,
                                 unsigned unit,
                                 uint32_t surf_index)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   struct brw_mipmap_tree *mt = irb->mt;
   uint32_t *surf;
   uint32_t tile_x, tile_y;
   enum isl_format format;
   uint32_t offset;
   /* _NEW_BUFFERS */
   mesa_format rb_format = _mesa_get_render_format(ctx, brw_rb_format(irb));
   /* BRW_NEW_FS_PROG_DATA */

   if (rb->TexImage && !devinfo->has_surface_tile_offset) {
      brw_renderbuffer_get_tile_offsets(irb, &tile_x, &tile_y);

      if (tile_x != 0 || tile_y != 0) {
         /* Original gfx4 hardware couldn't draw to a non-tile-aligned
          * destination in a miptree unless you actually setup your renderbuffer
          * as a miptree and used the fragile lod/array_index/etc. controls to
          * select the image.  So, instead, we just make a new single-level
          * miptree and render into that.
          */
         brw_renderbuffer_move_to_temp(brw, irb, false);
         assert(irb->align_wa_mt);
         mt = irb->align_wa_mt;
      }
   }

   surf = brw_state_batch(brw, 6 * 4, 32, &offset);

   format = brw->mesa_to_isl_render_format[rb_format];
   if (unlikely(!brw->mesa_format_supports_render[rb_format])) {
      _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
                    __func__, _mesa_get_format_name(rb_format));
   }

   surf[0] = (BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
              format << BRW_SURFACE_FORMAT_SHIFT);

   /* reloc */
   assert(mt->offset % mt->cpp == 0);
   surf[1] = brw_state_reloc(&brw->batch, offset + 4, mt->bo,
                             mt->offset +
                             brw_renderbuffer_get_tile_offsets(irb,
                                                                 &tile_x,
                                                                 &tile_y),
                             RELOC_WRITE);

   surf[2] = ((rb->Width - 1) << BRW_SURFACE_WIDTH_SHIFT |
              (rb->Height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (brw_get_surface_tiling_bits(mt->surf.tiling) |
              (mt->surf.row_pitch_B - 1) << BRW_SURFACE_PITCH_SHIFT);

   surf[4] = brw_get_surface_num_multisamples(mt->surf.samples);

   assert(devinfo->has_surface_tile_offset || (tile_x == 0 && tile_y == 0));
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf[5] = ((tile_x / 4) << BRW_SURFACE_X_OFFSET_SHIFT |
              (tile_y / 2) << BRW_SURFACE_Y_OFFSET_SHIFT |
              (mt->surf.image_alignment_el.height == 4 ?
                  BRW_SURFACE_VERTICAL_ALIGN_ENABLE : 0));

   if (devinfo->ver < 6) {
      /* _NEW_COLOR */
      if (!ctx->Color.ColorLogicOpEnabled &&
          ctx->Color._AdvancedBlendMode == BLEND_NONE &&
          (ctx->Color.BlendEnabled & (1 << unit)))
         surf[0] |= BRW_SURFACE_BLEND_ENABLED;

      if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, unit, 0))
         surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_R_SHIFT;
      if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, unit, 1))
         surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_G_SHIFT;
      if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, unit, 2))
         surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_B_SHIFT;

      /* As mentioned above, disable writes to the alpha component when the
       * renderbuffer is XRGB.
       */
      if (ctx->DrawBuffer->Visual.alphaBits == 0 ||
          !GET_COLORMASK_BIT(ctx->Color.ColorMask, unit, 3)) {
         surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_A_SHIFT;
      }
   }

   return offset;
}

static void
update_renderbuffer_surfaces(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct gl_context *ctx = &brw->ctx;

   /* _NEW_BUFFERS | _NEW_COLOR */
   const struct gl_framebuffer *fb = ctx->DrawBuffer;

   /* Render targets always start at binding table index 0. */
   const unsigned rt_start = 0;

   uint32_t *surf_offsets = brw->wm.base.surf_offset;

   /* Update surfaces for drawing buffers */
   if (fb->_NumColorDrawBuffers >= 1) {
      for (unsigned i = 0; i < fb->_NumColorDrawBuffers; i++) {
         struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[i];

         if (brw_renderbuffer(rb)) {
            surf_offsets[rt_start + i] = devinfo->ver >= 6 ?
               gfx6_update_renderbuffer_surface(brw, rb, i, rt_start + i) :
               gfx4_update_renderbuffer_surface(brw, rb, i, rt_start + i);
         } else {
            emit_null_surface_state(brw, fb, &surf_offsets[rt_start + i]);
         }
      }
   } else {
      emit_null_surface_state(brw, fb, &surf_offsets[rt_start]);
   }

   /* The PIPE_CONTROL command description says:
    *
    * "Whenever a Binding Table Index (BTI) used by a Render Taget Message
    *  points to a different RENDER_SURFACE_STATE, SW must issue a Render
    *  Target Cache Flush by enabling this bit. When render target flush
    *  is set due to new association of BTI, PS Scoreboard Stall bit must
    *  be set in this packet."
   */
   if (devinfo->ver >= 11) {
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                  PIPE_CONTROL_STALL_AT_SCOREBOARD);
   }

   brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state brw_renderbuffer_surfaces = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_COLOR,
      .brw = BRW_NEW_BATCH,
   },
   .emit = update_renderbuffer_surfaces,
};

const struct brw_tracked_state gfx6_renderbuffer_surfaces = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_AUX_STATE,
   },
   .emit = update_renderbuffer_surfaces,
};

static void
update_renderbuffer_read_surfaces(struct brw_context *brw)
{
   const struct gl_context *ctx = &brw->ctx;

   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);

   if (wm_prog_data->has_render_target_reads &&
       !ctx->Extensions.EXT_shader_framebuffer_fetch) {
      /* _NEW_BUFFERS */
      const struct gl_framebuffer *fb = ctx->DrawBuffer;

      for (unsigned i = 0; i < fb->_NumColorDrawBuffers; i++) {
         struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[i];
         const struct brw_renderbuffer *irb = brw_renderbuffer(rb);
         const unsigned surf_index =
            wm_prog_data->binding_table.render_target_read_start + i;
         uint32_t *surf_offset = &brw->wm.base.surf_offset[surf_index];

         if (irb) {
            const enum isl_format format = brw->mesa_to_isl_render_format[
               _mesa_get_render_format(ctx, brw_rb_format(irb))];
            assert(isl_format_supports_sampling(&brw->screen->devinfo,
                                                format));

            /* Override the target of the texture if the render buffer is a
             * single slice of a 3D texture (since the minimum array element
             * field of the surface state structure is ignored by the sampler
             * unit for 3D textures on some hardware), or if the render buffer
             * is a 1D array (since shaders always provide the array index
             * coordinate at the Z component to avoid state-dependent
             * recompiles when changing the texture target of the
             * framebuffer).
             */
            const GLenum target =
               (irb->mt->target == GL_TEXTURE_3D &&
                irb->layer_count == 1) ? GL_TEXTURE_2D :
               irb->mt->target == GL_TEXTURE_1D_ARRAY ? GL_TEXTURE_2D_ARRAY :
               irb->mt->target;

            const struct isl_view view = {
               .format = format,
               .base_level = irb->mt_level - irb->mt->first_level,
               .levels = 1,
               .base_array_layer = irb->mt_layer,
               .array_len = irb->layer_count,
               .swizzle = ISL_SWIZZLE_IDENTITY,
               .usage = ISL_SURF_USAGE_TEXTURE_BIT,
            };

            enum isl_aux_usage aux_usage =
               brw_miptree_texture_aux_usage(brw, irb->mt, format,
                                             brw->gfx9_astc5x5_wa_tex_mask);
            if (brw->draw_aux_usage[i] == ISL_AUX_USAGE_NONE)
               aux_usage = ISL_AUX_USAGE_NONE;

            brw_emit_surface_state(brw, irb->mt, target, view, aux_usage,
                                   surf_offset, surf_index,
                                   0);

         } else {
            emit_null_surface_state(brw, fb, surf_offset);
         }
      }

      brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
   }
}

const struct brw_tracked_state brw_renderbuffer_read_surfaces = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_AUX_STATE |
             BRW_NEW_FS_PROG_DATA,
   },
   .emit = update_renderbuffer_read_surfaces,
};

static bool
is_depth_texture(struct brw_texture_object *iobj)
{
   GLenum base_format = _mesa_get_format_base_format(iobj->_Format);
   return base_format == GL_DEPTH_COMPONENT ||
          (base_format == GL_DEPTH_STENCIL && !iobj->base.StencilSampling);
}

static void
update_stage_texture_surfaces(struct brw_context *brw,
                              const struct gl_program *prog,
                              struct brw_stage_state *stage_state,
                              bool for_gather, uint32_t plane)
{
   if (!prog)
      return;

   struct gl_context *ctx = &brw->ctx;

   uint32_t *surf_offset = stage_state->surf_offset;

   /* BRW_NEW_*_PROG_DATA */
   if (for_gather)
      surf_offset += stage_state->prog_data->binding_table.gather_texture_start;
   else
      surf_offset += stage_state->prog_data->binding_table.plane_start[plane];

   unsigned num_samplers = BITSET_LAST_BIT(prog->info.textures_used);
   for (unsigned s = 0; s < num_samplers; s++) {
      surf_offset[s] = 0;

      if (BITSET_TEST(prog->info.textures_used, s)) {
         const unsigned unit = prog->SamplerUnits[s];
         const bool used_by_txf = BITSET_TEST(prog->info.textures_used_by_txf, s);
         struct gl_texture_object *obj = ctx->Texture.Unit[unit]._Current;
         struct brw_texture_object *iobj = brw_texture_object(obj);

         /* _NEW_TEXTURE */
         if (!obj)
            continue;

         if ((prog->ShadowSamplers & (1 << s)) && !is_depth_texture(iobj)) {
            /* A programming note for the sample_c message says:
             *
             *    "The Surface Format of the associated surface must be
             *     indicated as supporting shadow mapping as indicated in the
             *     surface format table."
             *
             * Accessing non-depth textures via a sampler*Shadow type is
             * undefined.  GLSL 4.50 page 162 says:
             *
             *    "If a shadow texture call is made to a sampler that does not
             *     represent a depth texture, then results are undefined."
             *
             * We give them a null surface (zeros) for undefined.  We've seen
             * GPU hangs with color buffers and sample_c, so we try and avoid
             * those with this hack.
             */
            emit_null_surface_state(brw, NULL, surf_offset + s);
         } else {
            brw_update_texture_surface(ctx, unit, surf_offset + s, for_gather,
                                       used_by_txf, plane);
         }
      }
   }
}


/**
 * Construct SURFACE_STATE objects for enabled textures.
 */
static void
brw_update_texture_surfaces(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* BRW_NEW_VERTEX_PROGRAM */
   struct gl_program *vs = brw->programs[MESA_SHADER_VERTEX];

   /* BRW_NEW_TESS_PROGRAMS */
   struct gl_program *tcs = brw->programs[MESA_SHADER_TESS_CTRL];
   struct gl_program *tes = brw->programs[MESA_SHADER_TESS_EVAL];

   /* BRW_NEW_GEOMETRY_PROGRAM */
   struct gl_program *gs = brw->programs[MESA_SHADER_GEOMETRY];

   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct gl_program *fs = brw->programs[MESA_SHADER_FRAGMENT];

   /* _NEW_TEXTURE */
   update_stage_texture_surfaces(brw, vs, &brw->vs.base, false, 0);
   update_stage_texture_surfaces(brw, tcs, &brw->tcs.base, false, 0);
   update_stage_texture_surfaces(brw, tes, &brw->tes.base, false, 0);
   update_stage_texture_surfaces(brw, gs, &brw->gs.base, false, 0);
   update_stage_texture_surfaces(brw, fs, &brw->wm.base, false, 0);

   /* emit alternate set of surface state for gather. this
    * allows the surface format to be overriden for only the
    * gather4 messages. */
   if (devinfo->ver < 8) {
      if (vs && vs->info.uses_texture_gather)
         update_stage_texture_surfaces(brw, vs, &brw->vs.base, true, 0);
      if (tcs && tcs->info.uses_texture_gather)
         update_stage_texture_surfaces(brw, tcs, &brw->tcs.base, true, 0);
      if (tes && tes->info.uses_texture_gather)
         update_stage_texture_surfaces(brw, tes, &brw->tes.base, true, 0);
      if (gs && gs->info.uses_texture_gather)
         update_stage_texture_surfaces(brw, gs, &brw->gs.base, true, 0);
      if (fs && fs->info.uses_texture_gather)
         update_stage_texture_surfaces(brw, fs, &brw->wm.base, true, 0);
   }

   if (fs) {
      update_stage_texture_surfaces(brw, fs, &brw->wm.base, false, 1);
      update_stage_texture_surfaces(brw, fs, &brw->wm.base, false, 2);
   }

   brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state brw_texture_surfaces = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_AUX_STATE |
             BRW_NEW_FRAGMENT_PROGRAM |
             BRW_NEW_FS_PROG_DATA |
             BRW_NEW_GEOMETRY_PROGRAM |
             BRW_NEW_GS_PROG_DATA |
             BRW_NEW_TESS_PROGRAMS |
             BRW_NEW_TCS_PROG_DATA |
             BRW_NEW_TES_PROG_DATA |
             BRW_NEW_TEXTURE_BUFFER |
             BRW_NEW_VERTEX_PROGRAM |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_update_texture_surfaces,
};

static void
brw_update_cs_texture_surfaces(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* BRW_NEW_COMPUTE_PROGRAM */
   struct gl_program *cs = brw->programs[MESA_SHADER_COMPUTE];

   /* _NEW_TEXTURE */
   update_stage_texture_surfaces(brw, cs, &brw->cs.base, false, 0);

   /* emit alternate set of surface state for gather. this
    * allows the surface format to be overriden for only the
    * gather4 messages.
    */
   if (devinfo->ver < 8) {
      if (cs && cs->info.uses_texture_gather)
         update_stage_texture_surfaces(brw, cs, &brw->cs.base, true, 0);
   }

   brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state brw_cs_texture_surfaces = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_COMPUTE_PROGRAM |
             BRW_NEW_AUX_STATE,
   },
   .emit = brw_update_cs_texture_surfaces,
};

static void
upload_buffer_surface(struct brw_context *brw,
                      struct gl_buffer_binding *binding,
                      uint32_t *out_offset,
                      enum isl_format format,
                      unsigned reloc_flags)
{
   if (!binding->BufferObject) {
      emit_null_surface_state(brw, NULL, out_offset);
   } else {
      ptrdiff_t size = binding->BufferObject->Size - binding->Offset;
      if (!binding->AutomaticSize)
         size = MIN2(size, binding->Size);

      if (size == 0) {
         emit_null_surface_state(brw, NULL, out_offset);
         return;
      }

      struct brw_buffer_object *iobj =
         brw_buffer_object(binding->BufferObject);
      struct brw_bo *bo =
         brw_bufferobj_buffer(brw, iobj, binding->Offset, size,
                              (reloc_flags & RELOC_WRITE) != 0);

      brw_emit_buffer_surface_state(brw, out_offset, bo, binding->Offset,
                                    format, size, 1, reloc_flags);
   }
}

void
brw_upload_ubo_surfaces(struct brw_context *brw, struct gl_program *prog,
                        struct brw_stage_state *stage_state,
                        struct brw_stage_prog_data *prog_data)
{
   struct gl_context *ctx = &brw->ctx;

   if (!prog || (prog->info.num_ubos == 0 &&
                 prog->info.num_ssbos == 0 &&
                 prog->info.num_abos == 0))
      return;

   if (prog->info.num_ubos) {
      assert(prog_data->binding_table.ubo_start < BRW_MAX_SURFACES);
      uint32_t *ubo_surf_offsets =
         &stage_state->surf_offset[prog_data->binding_table.ubo_start];

      for (int i = 0; i < prog->info.num_ubos; i++) {
         struct gl_buffer_binding *binding =
            &ctx->UniformBufferBindings[prog->sh.UniformBlocks[i]->Binding];
         upload_buffer_surface(brw, binding, &ubo_surf_offsets[i],
                               ISL_FORMAT_R32G32B32A32_FLOAT, 0);
      }
   }

   if (prog->info.num_ssbos || prog->info.num_abos) {
      assert(prog_data->binding_table.ssbo_start < BRW_MAX_SURFACES);
      uint32_t *ssbo_surf_offsets =
         &stage_state->surf_offset[prog_data->binding_table.ssbo_start];
      uint32_t *abo_surf_offsets = ssbo_surf_offsets + prog->info.num_ssbos;

      for (int i = 0; i < prog->info.num_abos; i++) {
         struct gl_buffer_binding *binding =
            &ctx->AtomicBufferBindings[prog->sh.AtomicBuffers[i]->Binding];
         upload_buffer_surface(brw, binding, &abo_surf_offsets[i],
                               ISL_FORMAT_RAW, RELOC_WRITE);
      }

      for (int i = 0; i < prog->info.num_ssbos; i++) {
         struct gl_buffer_binding *binding =
            &ctx->ShaderStorageBufferBindings[prog->sh.ShaderStorageBlocks[i]->Binding];

         upload_buffer_surface(brw, binding, &ssbo_surf_offsets[i],
                               ISL_FORMAT_RAW, RELOC_WRITE);
      }
   }

   stage_state->push_constants_dirty = true;
   brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
}

static void
brw_upload_wm_ubo_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_program *prog = ctx->FragmentProgram._Current;

   /* BRW_NEW_FS_PROG_DATA */
   brw_upload_ubo_surfaces(brw, prog, &brw->wm.base, brw->wm.base.prog_data);
}

const struct brw_tracked_state brw_wm_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_FS_PROG_DATA |
             BRW_NEW_UNIFORM_BUFFER,
   },
   .emit = brw_upload_wm_ubo_surfaces,
};

static void
brw_upload_cs_ubo_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];

   /* BRW_NEW_CS_PROG_DATA */
   brw_upload_ubo_surfaces(brw, prog, &brw->cs.base, brw->cs.base.prog_data);
}

const struct brw_tracked_state brw_cs_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_CS_PROG_DATA |
             BRW_NEW_UNIFORM_BUFFER,
   },
   .emit = brw_upload_cs_ubo_surfaces,
};

static void
brw_upload_cs_image_surfaces(struct brw_context *brw)
{
   /* _NEW_PROGRAM */
   const struct gl_program *cp = brw->programs[MESA_SHADER_COMPUTE];

   if (cp) {
      /* BRW_NEW_CS_PROG_DATA, BRW_NEW_IMAGE_UNITS, _NEW_TEXTURE */
      brw_upload_image_surfaces(brw, cp, &brw->cs.base,
                                brw->cs.base.prog_data);
   }
}

const struct brw_tracked_state brw_cs_image_surfaces = {
   .dirty = {
      .mesa = _NEW_TEXTURE | _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_CS_PROG_DATA |
             BRW_NEW_AUX_STATE |
             BRW_NEW_IMAGE_UNITS
   },
   .emit = brw_upload_cs_image_surfaces,
};

static uint32_t
get_image_format(struct brw_context *brw, mesa_format format, GLenum access)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   enum isl_format hw_format = brw_isl_format_for_mesa_format(format);
   if (access == GL_WRITE_ONLY || access == GL_NONE) {
      return hw_format;
   } else if (isl_has_matching_typed_storage_image_format(devinfo, hw_format)) {
      /* Typed surface reads support a very limited subset of the shader
       * image formats.  Translate it into the closest format the
       * hardware supports.
       */
      return isl_lower_storage_image_format(devinfo, hw_format);
   } else {
      /* The hardware doesn't actually support a typed format that we can use
       * so we have to fall back to untyped read/write messages.
       */
      return ISL_FORMAT_RAW;
   }
}

static void
update_default_image_param(struct brw_context *brw,
                           struct gl_image_unit *u,
                           struct brw_image_param *param)
{
   memset(param, 0, sizeof(*param));
   /* Set the swizzling shifts to all-ones to effectively disable swizzling --
    * See emit_address_calculation() in brw_fs_surface_builder.cpp for a more
    * detailed explanation of these parameters.
    */
   param->swizzling[0] = 0xff;
   param->swizzling[1] = 0xff;
}

static void
update_buffer_image_param(struct brw_context *brw,
                          struct gl_image_unit *u,
                          struct brw_image_param *param)
{
   const unsigned size = buffer_texture_range_size(brw, u->TexObj);
   update_default_image_param(brw, u, param);

   param->size[0] = size / _mesa_get_format_bytes(u->_ActualFormat);
   param->stride[0] = _mesa_get_format_bytes(u->_ActualFormat);
}

static void
update_image_surface(struct brw_context *brw,
                     struct gl_image_unit *u,
                     GLenum access,
                     uint32_t *surf_offset,
                     struct brw_image_param *param)
{
   if (_mesa_is_image_unit_valid(&brw->ctx, u)) {
      struct gl_texture_object *obj = u->TexObj;
      const unsigned format = get_image_format(brw, u->_ActualFormat, access);
      const bool written = (access != GL_READ_ONLY && access != GL_NONE);

      if (obj->Target == GL_TEXTURE_BUFFER) {
         const unsigned texel_size = (format == ISL_FORMAT_RAW ? 1 :
                                      _mesa_get_format_bytes(u->_ActualFormat));
         const unsigned buffer_size = buffer_texture_range_size(brw, obj);
         struct brw_bo *const bo = !obj->BufferObject ? NULL :
            brw_bufferobj_buffer(brw, brw_buffer_object(obj->BufferObject),
                                 obj->BufferOffset, buffer_size, written);

         brw_emit_buffer_surface_state(
            brw, surf_offset, bo, obj->BufferOffset,
            format, buffer_size, texel_size,
            written ? RELOC_WRITE : 0);

         update_buffer_image_param(brw, u, param);

      } else {
         struct brw_texture_object *intel_obj = brw_texture_object(obj);
         struct brw_mipmap_tree *mt = intel_obj->mt;

         unsigned base_layer, num_layers;
         if (u->Layered) {
            if (obj->Target == GL_TEXTURE_3D) {
               base_layer = 0;
               num_layers = minify(mt->surf.logical_level0_px.depth, u->Level);
            } else {
               assert(obj->Immutable || obj->Attrib.MinLayer == 0);
               base_layer = obj->Attrib.MinLayer;
               num_layers = obj->Immutable ?
                                obj->Attrib.NumLayers :
                                mt->surf.logical_level0_px.array_len;
            }
         } else {
            base_layer = obj->Attrib.MinLayer + u->_Layer;
            num_layers = 1;
         }

         struct isl_view view = {
            .format = format,
            .base_level = obj->Attrib.MinLevel + u->Level,
            .levels = 1,
            .base_array_layer = base_layer,
            .array_len = num_layers,
            .swizzle = ISL_SWIZZLE_IDENTITY,
            .usage = ISL_SURF_USAGE_STORAGE_BIT,
         };

         if (format == ISL_FORMAT_RAW) {
            brw_emit_buffer_surface_state(
               brw, surf_offset, mt->bo, mt->offset,
               format, mt->bo->size - mt->offset, 1 /* pitch */,
               written ? RELOC_WRITE : 0);

         } else {
            const int surf_index = surf_offset - &brw->wm.base.surf_offset[0];
            assert(!brw_miptree_has_color_unresolved(mt,
                                                     view.base_level, 1,
                                                     view.base_array_layer,
                                                     view.array_len));
            brw_emit_surface_state(brw, mt, mt->target, view,
                                   ISL_AUX_USAGE_NONE,
                                   surf_offset, surf_index,
                                   written ? RELOC_WRITE : 0);
         }

         isl_surf_fill_image_param(&brw->isl_dev, param, &mt->surf, &view);
      }

   } else {
      emit_null_surface_state(brw, NULL, surf_offset);
      update_default_image_param(brw, u, param);
   }
}

void
brw_upload_image_surfaces(struct brw_context *brw,
                          const struct gl_program *prog,
                          struct brw_stage_state *stage_state,
                          struct brw_stage_prog_data *prog_data)
{
   assert(prog);
   struct gl_context *ctx = &brw->ctx;

   if (prog->info.num_images) {
      for (unsigned i = 0; i < prog->info.num_images; i++) {
         struct gl_image_unit *u = &ctx->ImageUnits[prog->sh.ImageUnits[i]];
         const unsigned surf_idx = prog_data->binding_table.image_start + i;

         update_image_surface(brw, u, prog->sh.ImageAccess[i],
                              &stage_state->surf_offset[surf_idx],
                              &stage_state->image_param[i]);
      }

      brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
      /* This may have changed the image metadata dependent on the context
       * image unit state and passed to the program as uniforms, make sure
       * that push and pull constants are reuploaded.
       */
      brw->NewGLState |= _NEW_PROGRAM_CONSTANTS;
   }
}

static void
brw_upload_wm_image_surfaces(struct brw_context *brw)
{
   /* BRW_NEW_FRAGMENT_PROGRAM */
   const struct gl_program *wm = brw->programs[MESA_SHADER_FRAGMENT];

   if (wm) {
      /* BRW_NEW_FS_PROG_DATA, BRW_NEW_IMAGE_UNITS, _NEW_TEXTURE */
      brw_upload_image_surfaces(brw, wm, &brw->wm.base,
                                brw->wm.base.prog_data);
   }
}

const struct brw_tracked_state brw_wm_image_surfaces = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_AUX_STATE |
             BRW_NEW_FRAGMENT_PROGRAM |
             BRW_NEW_FS_PROG_DATA |
             BRW_NEW_IMAGE_UNITS
   },
   .emit = brw_upload_wm_image_surfaces,
};

static void
brw_upload_cs_work_groups_surface(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   /* BRW_NEW_CS_PROG_DATA */
   const struct brw_cs_prog_data *cs_prog_data =
      brw_cs_prog_data(brw->cs.base.prog_data);

   if (prog && cs_prog_data->uses_num_work_groups) {
      const unsigned surf_idx =
         cs_prog_data->binding_table.work_groups_start;
      uint32_t *surf_offset = &brw->cs.base.surf_offset[surf_idx];
      struct brw_bo *bo;
      uint32_t bo_offset;

      if (brw->compute.num_work_groups_bo == NULL) {
         bo = NULL;
         brw_upload_data(&brw->upload,
                         (void *)brw->compute.num_work_groups,
                         3 * sizeof(GLuint),
                         sizeof(GLuint),
                         &bo,
                         &bo_offset);
      } else {
         bo = brw->compute.num_work_groups_bo;
         bo_offset = brw->compute.num_work_groups_offset;
      }

      brw_emit_buffer_surface_state(brw, surf_offset,
                                    bo, bo_offset,
                                    ISL_FORMAT_RAW,
                                    3 * sizeof(GLuint), 1,
                                    RELOC_WRITE);

      /* The state buffer now holds a reference to our upload, drop ours. */
      if (bo != brw->compute.num_work_groups_bo)
         brw_bo_unreference(bo);

      brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
   }
}

const struct brw_tracked_state brw_cs_work_groups_surface = {
   .dirty = {
      .brw = BRW_NEW_CS_PROG_DATA |
             BRW_NEW_CS_WORK_GROUPS
   },
   .emit = brw_upload_cs_work_groups_surface,
};
