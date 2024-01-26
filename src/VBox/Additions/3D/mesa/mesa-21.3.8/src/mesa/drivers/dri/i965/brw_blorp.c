/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "main/context.h"
#include "main/teximage.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/image.h"
#include "main/renderbuffer.h"
#include "main/glformats.h"

#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_meta_util.h"
#include "brw_state.h"
#include "brw_buffer_objects.h"
#include "brw_fbo.h"
#include "dev/intel_debug.h"

#define FILE_DEBUG_FLAG DEBUG_BLORP

static bool
brw_blorp_lookup_shader(struct blorp_batch *batch,
                        const void *key, uint32_t key_size,
                        uint32_t *kernel_out, void *prog_data_out)
{
   struct brw_context *brw = batch->driver_batch;
   return brw_search_cache(&brw->cache, BRW_CACHE_BLORP_PROG, key, key_size,
                           kernel_out, prog_data_out, true);
}

static bool
brw_blorp_upload_shader(struct blorp_batch *batch, uint32_t stage,
                        const void *key, uint32_t key_size,
                        const void *kernel, uint32_t kernel_size,
                        const struct brw_stage_prog_data *prog_data,
                        uint32_t prog_data_size,
                        uint32_t *kernel_out, void *prog_data_out)
{
   struct brw_context *brw = batch->driver_batch;
   brw_upload_cache(&brw->cache, BRW_CACHE_BLORP_PROG, key, key_size,
                    kernel, kernel_size, prog_data, prog_data_size,
                    kernel_out, prog_data_out);
   return true;
}

void
brw_blorp_init(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   blorp_init(&brw->blorp, brw, &brw->isl_dev);

   brw->blorp.compiler = brw->screen->compiler;

   switch (devinfo->ver) {
   case 4:
      if (devinfo->is_g4x) {
         brw->blorp.exec = gfx45_blorp_exec;
      } else {
         brw->blorp.exec = gfx4_blorp_exec;
      }
      break;
   case 5:
      brw->blorp.exec = gfx5_blorp_exec;
      break;
   case 6:
      brw->blorp.exec = gfx6_blorp_exec;
      break;
   case 7:
      if (devinfo->is_haswell) {
         brw->blorp.exec = gfx75_blorp_exec;
      } else {
         brw->blorp.exec = gfx7_blorp_exec;
      }
      break;
   case 8:
      brw->blorp.exec = gfx8_blorp_exec;
      break;
   case 9:
      brw->blorp.exec = gfx9_blorp_exec;
      break;
   case 11:
      brw->blorp.exec = gfx11_blorp_exec;
      break;

   default:
      unreachable("Invalid gen");
   }

   brw->blorp.lookup_shader = brw_blorp_lookup_shader;
   brw->blorp.upload_shader = brw_blorp_upload_shader;
}

static void
blorp_surf_for_miptree(struct brw_context *brw,
                       struct blorp_surf *surf,
                       const struct brw_mipmap_tree *mt,
                       enum isl_aux_usage aux_usage,
                       bool is_render_target,
                       unsigned *level,
                       unsigned start_layer, unsigned num_layers)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (mt->surf.msaa_layout == ISL_MSAA_LAYOUT_ARRAY) {
      const unsigned num_samples = mt->surf.samples;
      for (unsigned i = 0; i < num_layers; i++) {
         for (unsigned s = 0; s < num_samples; s++) {
            const unsigned phys_layer = (start_layer + i) * num_samples + s;
            brw_miptree_check_level_layer(mt, *level, phys_layer);
         }
      }
   } else {
      for (unsigned i = 0; i < num_layers; i++)
         brw_miptree_check_level_layer(mt, *level, start_layer + i);
   }

   *surf = (struct blorp_surf) {
      .surf = &mt->surf,
      .addr = (struct blorp_address) {
         .buffer = mt->bo,
         .offset = mt->offset,
         .reloc_flags = is_render_target ? EXEC_OBJECT_WRITE : 0,
         .mocs = brw_get_bo_mocs(devinfo, mt->bo),
      },
      .aux_usage = aux_usage,
      .tile_x_sa = mt->level[*level].level_x,
      .tile_y_sa = mt->level[*level].level_y,
   };

   if (surf->aux_usage == ISL_AUX_USAGE_HIZ &&
       !brw_miptree_level_has_hiz(mt, *level))
      surf->aux_usage = ISL_AUX_USAGE_NONE;

   if (surf->aux_usage != ISL_AUX_USAGE_NONE) {
      /* We only really need a clear color if we also have an auxiliary
       * surface.  Without one, it does nothing.
       */
      surf->clear_color =
         brw_miptree_get_clear_color(mt, (struct brw_bo **)
                                     &surf->clear_color_addr.buffer,
                                     &surf->clear_color_addr.offset);

      surf->aux_surf = &mt->aux_buf->surf;
      surf->aux_addr = (struct blorp_address) {
         .reloc_flags = is_render_target ? EXEC_OBJECT_WRITE : 0,
         .mocs = surf->addr.mocs,
      };

      surf->aux_addr.buffer = mt->aux_buf->bo;
      surf->aux_addr.offset = mt->aux_buf->offset;
   } else {
      surf->aux_addr = (struct blorp_address) {
         .buffer = NULL,
      };
      memset(&surf->clear_color, 0, sizeof(surf->clear_color));
   }
   assert((surf->aux_usage == ISL_AUX_USAGE_NONE) ==
          (surf->aux_addr.buffer == NULL));

   if (!is_render_target && brw->screen->devinfo.ver == 9)
      gfx9_apply_single_tex_astc5x5_wa(brw, mt->format, surf->aux_usage);

   /* ISL wants real levels, not offset ones. */
   *level -= mt->first_level;
}

static bool
brw_blorp_supports_dst_format(struct brw_context *brw, mesa_format format)
{
   /* If it's renderable, it's definitely supported. */
   if (brw->mesa_format_supports_render[format])
      return true;

   /* BLORP can't compress anything */
   if (_mesa_is_format_compressed(format))
      return false;

   /* No exotic formats such as GL_LUMINANCE_ALPHA */
   if (_mesa_get_format_bits(format, GL_RED_BITS) == 0 &&
       _mesa_get_format_bits(format, GL_DEPTH_BITS) == 0 &&
       _mesa_get_format_bits(format, GL_STENCIL_BITS) == 0)
      return false;

   return true;
}

static enum isl_format
brw_blorp_to_isl_format(struct brw_context *brw, mesa_format format,
                        bool is_render_target)
{
   switch (format) {
   case MESA_FORMAT_NONE:
      return ISL_FORMAT_UNSUPPORTED;
   case MESA_FORMAT_S_UINT8:
      return ISL_FORMAT_R8_UINT;
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      return ISL_FORMAT_R24_UNORM_X8_TYPELESS;
   case MESA_FORMAT_Z_FLOAT32:
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return ISL_FORMAT_R32_FLOAT;
   case MESA_FORMAT_Z_UNORM16:
      return ISL_FORMAT_R16_UNORM;
   default:
      if (is_render_target) {
         assert(brw_blorp_supports_dst_format(brw, format));
         if (brw->mesa_format_supports_render[format]) {
            return brw->mesa_to_isl_render_format[format];
         } else {
            return brw_isl_format_for_mesa_format(format);
         }
      } else {
         /* Some destinations (is_render_target == true) are supported by
          * blorp even though we technically can't render to them.
          */
         return brw_isl_format_for_mesa_format(format);
      }
   }
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
static enum isl_channel_select
swizzle_to_scs(GLenum swizzle)
{
   return (enum isl_channel_select)((swizzle + 4) & 7);
}

/**
 * Note: if the src (or dst) is a 2D multisample array texture on Gfx7+ using
 * INTEL_MSAA_LAYOUT_UMS or INTEL_MSAA_LAYOUT_CMS, src_layer (dst_layer) is
 * the physical layer holding sample 0.  So, for example, if
 * src_mt->surf.samples == 4, then logical layer n corresponds to src_layer ==
 * 4*n.
 */
void
brw_blorp_blit_miptrees(struct brw_context *brw,
                        struct brw_mipmap_tree *src_mt,
                        unsigned src_level, unsigned src_layer,
                        mesa_format src_format, int src_swizzle,
                        struct brw_mipmap_tree *dst_mt,
                        unsigned dst_level, unsigned dst_layer,
                        mesa_format dst_format,
                        float src_x0, float src_y0,
                        float src_x1, float src_y1,
                        float dst_x0, float dst_y0,
                        float dst_x1, float dst_y1,
                        GLenum gl_filter, bool mirror_x, bool mirror_y,
                        bool decode_srgb, bool encode_srgb)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   DBG("%s from %dx %s mt %p %d %d (%f,%f) (%f,%f) "
       "to %dx %s mt %p %d %d (%f,%f) (%f,%f) (flip %d,%d)\n",
       __func__,
       src_mt->surf.samples, _mesa_get_format_name(src_mt->format), src_mt,
       src_level, src_layer, src_x0, src_y0, src_x1, src_y1,
       dst_mt->surf.samples, _mesa_get_format_name(dst_mt->format), dst_mt,
       dst_level, dst_layer, dst_x0, dst_y0, dst_x1, dst_y1,
       mirror_x, mirror_y);

   if (src_format == MESA_FORMAT_NONE)
      src_format = src_mt->format;

   if (dst_format == MESA_FORMAT_NONE)
      dst_format = dst_mt->format;

   if (!decode_srgb)
      src_format = _mesa_get_srgb_format_linear(src_format);

   if (!encode_srgb)
      dst_format = _mesa_get_srgb_format_linear(dst_format);

   /* When doing a multisample resolve of a GL_LUMINANCE32F or GL_INTENSITY32F
    * texture, the above code configures the source format for L32_FLOAT or
    * I32_FLOAT, and the destination format for R32_FLOAT.  On Sandy Bridge,
    * the SAMPLE message appears to handle multisampled L32_FLOAT and
    * I32_FLOAT textures incorrectly, resulting in blocky artifacts.  So work
    * around the problem by using a source format of R32_FLOAT.  This
    * shouldn't affect rendering correctness, since the destination format is
    * R32_FLOAT, so only the contents of the red channel matters.
    */
   if (devinfo->ver == 6 &&
       src_mt->surf.samples > 1 && dst_mt->surf.samples <= 1 &&
       src_mt->format == dst_mt->format &&
       (dst_format == MESA_FORMAT_L_FLOAT32 ||
        dst_format == MESA_FORMAT_I_FLOAT32)) {
      src_format = dst_format = MESA_FORMAT_R_FLOAT32;
   }

   enum blorp_filter blorp_filter;
   if (fabsf(dst_x1 - dst_x0) == fabsf(src_x1 - src_x0) &&
       fabsf(dst_y1 - dst_y0) == fabsf(src_y1 - src_y0)) {
      if (src_mt->surf.samples > 1 && dst_mt->surf.samples <= 1) {
         /* From the OpenGL ES 3.2 specification, section 16.2.1:
          *
          *    "If the read framebuffer is multisampled (its effective value
          *    of SAMPLE_BUFFERS is one) and the draw framebuffer is not (its
          *    value of SAMPLE_BUFFERS is zero), the samples corresponding to
          *    each pixel location in the source are converted to a single
          *    sample before being written to the destination.  The filter
          *    parameter is ignored. If the source formats are integer types
          *    or stencil values, a single sample’s value is selected for each
          *    pixel.  If the source formats are floating-point or normalized
          *    types, the sample values for each pixel are resolved in an
          *    implementation-dependent manner.  If the source formats are
          *    depth values, sample values are resolved in an implementation-
          *    dependent manner where the result will be between the minimum
          *    and maximum depth values in the pixel."
          *
          * For depth and stencil resolves, we choose to always use the value
          * at sample 0.
          */
         GLenum base_format = _mesa_get_format_base_format(src_mt->format);
         if (base_format == GL_DEPTH_COMPONENT ||
             base_format == GL_STENCIL_INDEX ||
             base_format == GL_DEPTH_STENCIL ||
             _mesa_is_format_integer(src_mt->format)) {
            /* The OpenGL ES 3.2 spec says:
             *
             *    "If the source formats are integer types or stencil values,
             *    a single sample's value is selected for each pixel."
             *
             * Just take sample 0 in this case.
             */
            blorp_filter = BLORP_FILTER_SAMPLE_0;
         } else {
            blorp_filter = BLORP_FILTER_AVERAGE;
         }
      } else {
         /* From the OpenGL 4.6 specification, section 18.3.1:
          *
          *    "If the source and destination dimensions are identical, no
          *    filtering is applied."
          *
          * Using BLORP_FILTER_NONE will also handle the upsample case by
          * replicating the one value in the source to all values in the
          * destination.
          */
         blorp_filter = BLORP_FILTER_NONE;
      }
   } else if (gl_filter == GL_LINEAR ||
              gl_filter == GL_SCALED_RESOLVE_FASTEST_EXT ||
              gl_filter == GL_SCALED_RESOLVE_NICEST_EXT) {
      blorp_filter = BLORP_FILTER_BILINEAR;
   } else {
      blorp_filter = BLORP_FILTER_NEAREST;
   }

   enum isl_format src_isl_format =
      brw_blorp_to_isl_format(brw, src_format, false);
   enum isl_aux_usage src_aux_usage =
      brw_miptree_texture_aux_usage(brw, src_mt, src_isl_format,
                                    0 /* The astc5x5 WA isn't needed */);
   /* We do format workarounds for some depth formats so we can't reliably
    * sample with HiZ.  One of these days, we should fix that.
    */
   if (src_aux_usage == ISL_AUX_USAGE_HIZ && src_mt->format != src_format)
      src_aux_usage = ISL_AUX_USAGE_NONE;
   const bool src_clear_supported =
      src_aux_usage != ISL_AUX_USAGE_NONE && src_mt->format == src_format;
   brw_miptree_prepare_access(brw, src_mt, src_level, 1, src_layer, 1,
                              src_aux_usage, src_clear_supported);

   enum isl_format dst_isl_format =
      brw_blorp_to_isl_format(brw, dst_format, true);
   enum isl_aux_usage dst_aux_usage =
      brw_miptree_render_aux_usage(brw, dst_mt, dst_isl_format, false, false);
   const bool dst_clear_supported = dst_aux_usage != ISL_AUX_USAGE_NONE;
   brw_miptree_prepare_access(brw, dst_mt, dst_level, 1, dst_layer, 1,
                              dst_aux_usage, dst_clear_supported);

   struct blorp_surf src_surf, dst_surf;
   blorp_surf_for_miptree(brw, &src_surf, src_mt, src_aux_usage, false,
                          &src_level, src_layer, 1);
   blorp_surf_for_miptree(brw, &dst_surf, dst_mt, dst_aux_usage, true,
                          &dst_level, dst_layer, 1);

   struct isl_swizzle src_isl_swizzle = {
      .r = swizzle_to_scs(GET_SWZ(src_swizzle, 0)),
      .g = swizzle_to_scs(GET_SWZ(src_swizzle, 1)),
      .b = swizzle_to_scs(GET_SWZ(src_swizzle, 2)),
      .a = swizzle_to_scs(GET_SWZ(src_swizzle, 3)),
   };

   struct blorp_batch batch;
   blorp_batch_init(&brw->blorp, &batch, brw, 0);
   blorp_blit(&batch, &src_surf, src_level, src_layer,
              src_isl_format, src_isl_swizzle,
              &dst_surf, dst_level, dst_layer,
              dst_isl_format, ISL_SWIZZLE_IDENTITY,
              src_x0, src_y0, src_x1, src_y1,
              dst_x0, dst_y0, dst_x1, dst_y1,
              blorp_filter, mirror_x, mirror_y);
   blorp_batch_finish(&batch);

   brw_miptree_finish_write(brw, dst_mt, dst_level, dst_layer, 1,
                            dst_aux_usage);
}

void
brw_blorp_copy_miptrees(struct brw_context *brw,
                        struct brw_mipmap_tree *src_mt,
                        unsigned src_level, unsigned src_layer,
                        struct brw_mipmap_tree *dst_mt,
                        unsigned dst_level, unsigned dst_layer,
                        unsigned src_x, unsigned src_y,
                        unsigned dst_x, unsigned dst_y,
                        unsigned src_width, unsigned src_height)
{
   DBG("%s from %dx %s mt %p %d %d (%d,%d) %dx%d"
       "to %dx %s mt %p %d %d (%d,%d)\n",
       __func__,
       src_mt->surf.samples, _mesa_get_format_name(src_mt->format), src_mt,
       src_level, src_layer, src_x, src_y, src_width, src_height,
       dst_mt->surf.samples, _mesa_get_format_name(dst_mt->format), dst_mt,
       dst_level, dst_layer, dst_x, dst_y);

   enum isl_aux_usage src_aux_usage, dst_aux_usage;
   bool src_clear_supported, dst_clear_supported;

   switch (src_mt->aux_usage) {
   case ISL_AUX_USAGE_HIZ:
      if (brw_miptree_sample_with_hiz(brw, src_mt)) {
         src_aux_usage = src_mt->aux_usage;
         src_clear_supported = true;
      } else {
         src_aux_usage = ISL_AUX_USAGE_NONE;
         src_clear_supported = false;
      }
      break;
   case ISL_AUX_USAGE_MCS:
   case ISL_AUX_USAGE_CCS_E:
      src_aux_usage = src_mt->aux_usage;
      src_clear_supported = false;
      break;
   default:
      src_aux_usage = ISL_AUX_USAGE_NONE;
      src_clear_supported = false;
      break;
   }

   switch (dst_mt->aux_usage) {
   case ISL_AUX_USAGE_MCS:
   case ISL_AUX_USAGE_CCS_E:
      dst_aux_usage = dst_mt->aux_usage;
      dst_clear_supported = false;
      break;
   default:
      dst_aux_usage = ISL_AUX_USAGE_NONE;
      dst_clear_supported = false;
      break;
   }

   brw_miptree_prepare_access(brw, src_mt, src_level, 1, src_layer, 1,
                              src_aux_usage, src_clear_supported);
   brw_miptree_prepare_access(brw, dst_mt, dst_level, 1, dst_layer, 1,
                              dst_aux_usage, dst_clear_supported);

   struct blorp_surf src_surf, dst_surf;
   blorp_surf_for_miptree(brw, &src_surf, src_mt, src_aux_usage, false,
                          &src_level, src_layer, 1);
   blorp_surf_for_miptree(brw, &dst_surf, dst_mt, dst_aux_usage, true,
                          &dst_level, dst_layer, 1);

   /* The hardware seems to have issues with having a two different format
    * views of the same texture in the sampler cache at the same time.  It's
    * unclear exactly what the issue is but it hurts glCopyImageSubData
    * particularly badly because it does a lot of format reinterprets.  We
    * badly need better understanding of the issue and a better fix but this
    * works for now and fixes CTS tests.
    *
    * TODO: Remove this hack!
    */
   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_CS_STALL |
                                    PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE);

   struct blorp_batch batch;
   blorp_batch_init(&brw->blorp, &batch, brw, 0);
   blorp_copy(&batch, &src_surf, src_level, src_layer,
              &dst_surf, dst_level, dst_layer,
              src_x, src_y, dst_x, dst_y, src_width, src_height);
   blorp_batch_finish(&batch);

   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_CS_STALL |
                                    PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE);

   brw_miptree_finish_write(brw, dst_mt, dst_level, dst_layer, 1,
                            dst_aux_usage);
}

void
brw_blorp_copy_buffers(struct brw_context *brw,
                       struct brw_bo *src_bo,
                       unsigned src_offset,
                       struct brw_bo *dst_bo,
                       unsigned dst_offset,
                       unsigned size)
{
   DBG("%s %d bytes from %p[%d] to %p[%d]",
       __func__, size, src_bo, src_offset, dst_bo, dst_offset);

   struct blorp_batch batch;
   struct blorp_address src = { .buffer = src_bo, .offset = src_offset };
   struct blorp_address dst = { .buffer = dst_bo, .offset = dst_offset };

   blorp_batch_init(&brw->blorp, &batch, brw, 0);
   blorp_buffer_copy(&batch, src, dst, size);
   blorp_batch_finish(&batch);
}


static struct brw_mipmap_tree *
find_miptree(GLbitfield buffer_bit, struct brw_renderbuffer *irb)
{
   struct brw_mipmap_tree *mt = irb->mt;
   if (buffer_bit == GL_STENCIL_BUFFER_BIT && mt->stencil_mt)
      mt = mt->stencil_mt;
   return mt;
}

static int
blorp_get_texture_swizzle(const struct brw_renderbuffer *irb)
{
   return irb->Base.Base._BaseFormat == GL_RGB ?
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE) :
      SWIZZLE_XYZW;
}

static void
do_blorp_blit(struct brw_context *brw, GLbitfield buffer_bit,
              struct brw_renderbuffer *src_irb, mesa_format src_format,
              struct brw_renderbuffer *dst_irb, mesa_format dst_format,
              GLfloat srcX0, GLfloat srcY0, GLfloat srcX1, GLfloat srcY1,
              GLfloat dstX0, GLfloat dstY0, GLfloat dstX1, GLfloat dstY1,
              GLenum filter, bool mirror_x, bool mirror_y)
{
   const struct gl_context *ctx = &brw->ctx;

   /* Find source/dst miptrees */
   struct brw_mipmap_tree *src_mt = find_miptree(buffer_bit, src_irb);
   struct brw_mipmap_tree *dst_mt = find_miptree(buffer_bit, dst_irb);

   const bool do_srgb = ctx->Color.sRGBEnabled;

   /* Do the blit */
   brw_blorp_blit_miptrees(brw,
                           src_mt, src_irb->mt_level, src_irb->mt_layer,
                           src_format, blorp_get_texture_swizzle(src_irb),
                           dst_mt, dst_irb->mt_level, dst_irb->mt_layer,
                           dst_format,
                           srcX0, srcY0, srcX1, srcY1,
                           dstX0, dstY0, dstX1, dstY1,
                           filter, mirror_x, mirror_y,
                           do_srgb, do_srgb);

   dst_irb->need_downsample = true;
}

static bool
try_blorp_blit(struct brw_context *brw,
               const struct gl_framebuffer *read_fb,
               const struct gl_framebuffer *draw_fb,
               GLfloat srcX0, GLfloat srcY0, GLfloat srcX1, GLfloat srcY1,
               GLfloat dstX0, GLfloat dstY0, GLfloat dstX1, GLfloat dstY1,
               GLenum filter, GLbitfield buffer_bit)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;

   /* Sync up the state of window system buffers.  We need to do this before
    * we go looking for the buffers.
    */
   brw_prepare_render(brw);

   bool mirror_x, mirror_y;
   if (brw_meta_mirror_clip_and_scissor(ctx, read_fb, draw_fb,
                                        &srcX0, &srcY0, &srcX1, &srcY1,
                                        &dstX0, &dstY0, &dstX1, &dstY1,
                                        &mirror_x, &mirror_y))
      return true;

   /* Find buffers */
   struct brw_renderbuffer *src_irb;
   struct brw_renderbuffer *dst_irb;
   struct brw_mipmap_tree *src_mt;
   struct brw_mipmap_tree *dst_mt;
   switch (buffer_bit) {
   case GL_COLOR_BUFFER_BIT:
      src_irb = brw_renderbuffer(read_fb->_ColorReadBuffer);
      for (unsigned i = 0; i < draw_fb->_NumColorDrawBuffers; ++i) {
         dst_irb = brw_renderbuffer(draw_fb->_ColorDrawBuffers[i]);
	 if (dst_irb)
            do_blorp_blit(brw, buffer_bit,
                          src_irb, src_irb->Base.Base.Format,
                          dst_irb, dst_irb->Base.Base.Format,
                          srcX0, srcY0, srcX1, srcY1,
                          dstX0, dstY0, dstX1, dstY1,
                          filter, mirror_x, mirror_y);
      }
      break;
   case GL_DEPTH_BUFFER_BIT:
      src_irb =
         brw_renderbuffer(read_fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      dst_irb =
         brw_renderbuffer(draw_fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      src_mt = find_miptree(buffer_bit, src_irb);
      dst_mt = find_miptree(buffer_bit, dst_irb);

      /* We also can't handle any combined depth-stencil formats because we
       * have to reinterpret as a color format.
       */
      if (_mesa_get_format_base_format(src_mt->format) == GL_DEPTH_STENCIL ||
          _mesa_get_format_base_format(dst_mt->format) == GL_DEPTH_STENCIL)
         return false;

      do_blorp_blit(brw, buffer_bit, src_irb, MESA_FORMAT_NONE,
                    dst_irb, MESA_FORMAT_NONE, srcX0, srcY0,
                    srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                    filter, mirror_x, mirror_y);
      break;
   case GL_STENCIL_BUFFER_BIT:
      /* Blorp doesn't support combined depth stencil which is all we have
       * prior to gfx6.
       */
      if (devinfo->ver < 6)
         return false;

      src_irb =
         brw_renderbuffer(read_fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      dst_irb =
         brw_renderbuffer(draw_fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      do_blorp_blit(brw, buffer_bit, src_irb, MESA_FORMAT_NONE,
                    dst_irb, MESA_FORMAT_NONE, srcX0, srcY0,
                    srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                    filter, mirror_x, mirror_y);
      break;
   default:
      unreachable("not reached");
   }

   return true;
}

static void
apply_y_flip(int *y0, int *y1, int height)
{
   int tmp = height - *y0;
   *y0 = height - *y1;
   *y1 = tmp;
}

bool
brw_blorp_copytexsubimage(struct brw_context *brw,
                          struct gl_renderbuffer *src_rb,
                          struct gl_texture_image *dst_image,
                          int slice,
                          int srcX0, int srcY0,
                          int dstX0, int dstY0,
                          int width, int height)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_renderbuffer *src_irb = brw_renderbuffer(src_rb);
   struct brw_texture_image *intel_image = brw_texture_image(dst_image);

   /* No pixel transfer operations (zoom, bias, mapping), just a blit */
   if (brw->ctx._ImageTransferState)
      return false;

   /* Sync up the state of window system buffers.  We need to do this before
    * we go looking at the src renderbuffer's miptree.
    */
   brw_prepare_render(brw);

   struct brw_mipmap_tree *src_mt = src_irb->mt;
   struct brw_mipmap_tree *dst_mt = intel_image->mt;

   /* We can't handle any combined depth-stencil formats because we have to
    * reinterpret as a color format.
    */
   if (_mesa_get_format_base_format(src_mt->format) == GL_DEPTH_STENCIL ||
       _mesa_get_format_base_format(dst_mt->format) == GL_DEPTH_STENCIL)
      return false;

   if (!brw_blorp_supports_dst_format(brw, dst_image->TexFormat))
      return false;

   /* Source clipping shouldn't be necessary, since copytexsubimage (in
    * src/mesa/main/teximage.c) calls _mesa_clip_copytexsubimage() which
    * takes care of it.
    *
    * Destination clipping shouldn't be necessary since the restrictions on
    * glCopyTexSubImage prevent the user from specifying a destination rectangle
    * that falls outside the bounds of the destination texture.
    * See error_check_subtexture_dimensions().
    */

   int srcY1 = srcY0 + height;
   int srcX1 = srcX0 + width;
   int dstX1 = dstX0 + width;
   int dstY1 = dstY0 + height;

   /* Account for the fact that in the system framebuffer, the origin is at
    * the lower left.
    */
   bool mirror_y = ctx->ReadBuffer->FlipY;
   if (mirror_y)
      apply_y_flip(&srcY0, &srcY1, src_rb->Height);

   /* Account for face selection and texture view MinLayer */
   int dst_slice = slice + dst_image->TexObject->Attrib.MinLayer + dst_image->Face;
   int dst_level = dst_image->Level + dst_image->TexObject->Attrib.MinLevel;

   brw_blorp_blit_miptrees(brw,
                           src_mt, src_irb->mt_level, src_irb->mt_layer,
                           src_rb->Format, blorp_get_texture_swizzle(src_irb),
                           dst_mt, dst_level, dst_slice,
                           dst_image->TexFormat,
                           srcX0, srcY0, srcX1, srcY1,
                           dstX0, dstY0, dstX1, dstY1,
                           GL_NEAREST, false, mirror_y,
                           false, false);

   /* If we're copying to a packed depth stencil texture and the source
    * framebuffer has separate stencil, we need to also copy the stencil data
    * over.
    */
   src_rb = ctx->ReadBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   if (_mesa_get_format_bits(dst_image->TexFormat, GL_STENCIL_BITS) > 0 &&
       src_rb != NULL) {
      src_irb = brw_renderbuffer(src_rb);
      src_mt = src_irb->mt;

      if (src_mt->stencil_mt)
         src_mt = src_mt->stencil_mt;
      if (dst_mt->stencil_mt)
         dst_mt = dst_mt->stencil_mt;

      if (src_mt != dst_mt) {
         brw_blorp_blit_miptrees(brw,
                                 src_mt, src_irb->mt_level, src_irb->mt_layer,
                                 src_mt->format,
                                 blorp_get_texture_swizzle(src_irb),
                                 dst_mt, dst_level, dst_slice,
                                 dst_mt->format,
                                 srcX0, srcY0, srcX1, srcY1,
                                 dstX0, dstY0, dstX1, dstY1,
                                 GL_NEAREST, false, mirror_y,
                                 false, false);
      }
   }

   return true;
}


GLbitfield
brw_blorp_framebuffer(struct brw_context *brw,
                      struct gl_framebuffer *readFb,
                      struct gl_framebuffer *drawFb,
                      GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter)
{
   static GLbitfield buffer_bits[] = {
      GL_COLOR_BUFFER_BIT,
      GL_DEPTH_BUFFER_BIT,
      GL_STENCIL_BUFFER_BIT,
   };

   for (unsigned int i = 0; i < ARRAY_SIZE(buffer_bits); ++i) {
      if ((mask & buffer_bits[i]) &&
       try_blorp_blit(brw, readFb, drawFb,
                      srcX0, srcY0, srcX1, srcY1,
                      dstX0, dstY0, dstX1, dstY1,
                      filter, buffer_bits[i])) {
         mask &= ~buffer_bits[i];
      }
   }

   /* try_blorp_blit should always be successful for color blits. */
   assert(!(mask & GL_COLOR_BUFFER_BIT));
   return mask;
}

static struct brw_bo *
blorp_get_client_bo(struct brw_context *brw,
                    unsigned w, unsigned h, unsigned d,
                    GLenum target, GLenum format, GLenum type,
                    const void *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    uint32_t *offset_out, uint32_t *row_stride_out,
                    uint32_t *image_stride_out, bool read_only)
{
   /* Account for SKIP_PIXELS, SKIP_ROWS, ALIGNMENT, and SKIP_IMAGES */
   const GLuint dims = _mesa_get_texture_dimensions(target);
   const uint32_t first_pixel = _mesa_image_offset(dims, packing, w, h,
                                                   format, type, 0, 0, 0);
   const uint32_t last_pixel =  _mesa_image_offset(dims, packing, w, h,
                                                   format, type,
                                                   d - 1, h - 1, w);
   const uint32_t stride = _mesa_image_row_stride(packing, w, format, type);
   const uint32_t size = last_pixel - first_pixel;

   *row_stride_out = stride;
   *image_stride_out = _mesa_image_image_stride(packing, w, h, format, type);

   if (packing->BufferObj) {
      const uint32_t offset = first_pixel + (intptr_t)pixels;

      if (!read_only) {
         const int32_t cpp = _mesa_bytes_per_pixel(format, type);
         assert(cpp > 0);

         if ((offset % cpp) || (stride % cpp)) {
            perf_debug("Bad PBO alignment; fallback to CPU mapping\n");
            return NULL;
         }
      }

      /* This is a user-provided PBO. We just need to get the BO out */
      struct brw_buffer_object *intel_pbo =
         brw_buffer_object(packing->BufferObj);
      struct brw_bo *bo =
         brw_bufferobj_buffer(brw, intel_pbo, offset, size, !read_only);

      /* We take a reference to the BO so that the caller can just always
       * unref without having to worry about whether it's a user PBO or one
       * we created.
       */
      brw_bo_reference(bo);

      *offset_out = offset;
      return bo;
   } else {
      /* Someone should have already checked that there is data to upload. */
      assert(pixels);

      /* Creating a temp buffer currently only works for upload */
      assert(read_only);

      /* This is not a user-provided PBO.  Instead, pixels is a pointer to CPU
       * data which we need to copy into a BO.
       */
      struct brw_bo *bo =
         brw_bo_alloc(brw->bufmgr, "tmp_tex_subimage_src", size,
                      BRW_MEMZONE_OTHER);
      if (bo == NULL) {
         perf_debug("%s: temp bo creation failed: size = %u\n", __func__,
                    size);
         return NULL;
      }

      if (brw_bo_subdata(bo, 0, size, pixels + first_pixel)) {
         perf_debug("%s: temp bo upload failed\n", __func__);
         brw_bo_unreference(bo);
         return NULL;
      }

      *offset_out = 0;
      return bo;
   }
}

/* Consider all the restrictions and determine the format of the source. */
static mesa_format
blorp_get_client_format(struct brw_context *brw,
                        GLenum format, GLenum type,
                        const struct gl_pixelstore_attrib *packing)
{
   if (brw->ctx._ImageTransferState)
      return MESA_FORMAT_NONE;

   if (packing->SwapBytes || packing->LsbFirst || packing->Invert) {
      perf_debug("%s: unsupported gl_pixelstore_attrib\n", __func__);
      return MESA_FORMAT_NONE;
   }

   if (format != GL_RED &&
       format != GL_RG &&
       format != GL_RGB &&
       format != GL_BGR &&
       format != GL_RGBA &&
       format != GL_BGRA &&
       format != GL_ALPHA &&
       format != GL_RED_INTEGER &&
       format != GL_RG_INTEGER &&
       format != GL_RGB_INTEGER &&
       format != GL_BGR_INTEGER &&
       format != GL_RGBA_INTEGER &&
       format != GL_BGRA_INTEGER) {
      perf_debug("%s: %s not supported", __func__,
                 _mesa_enum_to_string(format));
      return MESA_FORMAT_NONE;
   }

   return _mesa_tex_format_from_format_and_type(&brw->ctx, format, type);
}

bool
brw_blorp_upload_miptree(struct brw_context *brw,
                         struct brw_mipmap_tree *dst_mt,
                         mesa_format dst_format,
                         uint32_t level, uint32_t x, uint32_t y, uint32_t z,
                         uint32_t width, uint32_t height, uint32_t depth,
                         GLenum target, GLenum format, GLenum type,
                         const void *pixels,
                         const struct gl_pixelstore_attrib *packing)
{
   const mesa_format src_format =
      blorp_get_client_format(brw, format, type, packing);
   if (src_format == MESA_FORMAT_NONE)
      return false;

   if (!brw->mesa_format_supports_render[dst_format]) {
      perf_debug("%s: can't use %s as render target\n", __func__,
                 _mesa_get_format_name(dst_format));
      return false;
   }

   uint32_t src_offset, src_row_stride, src_image_stride;
   struct brw_bo *src_bo =
      blorp_get_client_bo(brw, width, height, depth,
                          target, format, type, pixels, packing,
                          &src_offset, &src_row_stride,
                          &src_image_stride, true);
   if (src_bo == NULL)
      return false;

   /* Now that source is offset to correct starting point, adjust the
    * given dimensions to treat 1D arrays as 2D.
    */
   if (target == GL_TEXTURE_1D_ARRAY) {
      assert(depth == 1);
      assert(z == 0);
      depth = height;
      height = 1;
      z = y;
      y = 0;
      src_image_stride = src_row_stride;
   }

   brw_miptree_check_level_layer(dst_mt, level, z + depth - 1);

   bool result = false;

   /* Blit slice-by-slice creating a single-slice miptree for each layer. Even
    * in case of linear buffers hardware wants image arrays to be aligned by
    * four rows. This way hardware only gets one image at a time and any
    * source alignment will do.
    */
   for (unsigned i = 0; i < depth; ++i) {
      struct brw_mipmap_tree *src_mt =
         brw_miptree_create_for_bo(brw, src_bo, src_format,
                                   src_offset + i * src_image_stride,
                                   width, height, 1,
                                   src_row_stride,
                                   ISL_TILING_LINEAR, 0);

      if (!src_mt) {
         perf_debug("%s: miptree creation for src failed\n", __func__);
         goto err;
      }

      /* In case exact match is needed, copy using equivalent UINT formats
       * preventing hardware from changing presentation for SNORM -1.
       */
      if (src_mt->format == dst_format) {
         brw_blorp_copy_miptrees(brw, src_mt, 0, 0,
                                 dst_mt, level, z + i,
                                 0, 0, x, y, width, height);
      } else {
         brw_blorp_blit_miptrees(brw, src_mt, 0, 0,
                                 src_format, SWIZZLE_XYZW,
                                 dst_mt, level, z + i,
                                 dst_format,
                                 0, 0, width, height,
                                 x, y, x + width, y + height,
                                 GL_NEAREST, false, false, false, false);
      }

      brw_miptree_release(&src_mt);
   }

   result = true;

err:
   brw_bo_unreference(src_bo);

   return result;
}

bool
brw_blorp_download_miptree(struct brw_context *brw,
                           struct brw_mipmap_tree *src_mt,
                           mesa_format src_format, uint32_t src_swizzle,
                           uint32_t level, uint32_t x, uint32_t y, uint32_t z,
                           uint32_t width, uint32_t height, uint32_t depth,
                           GLenum target, GLenum format, GLenum type,
                           bool y_flip, const void *pixels,
                           const struct gl_pixelstore_attrib *packing)
{
   const mesa_format dst_format =
      blorp_get_client_format(brw, format, type, packing);
   if (dst_format == MESA_FORMAT_NONE)
      return false;

   if (!brw->mesa_format_supports_render[dst_format]) {
      perf_debug("%s: can't use %s as render target\n", __func__,
                 _mesa_get_format_name(dst_format));
      return false;
   }

   /* We can't fetch from LUMINANCE or intensity as that would require a
    * non-trivial swizzle.
    */
   switch (_mesa_get_format_base_format(src_format)) {
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
      return false;
   default:
      break;
   }

   /* This pass only works for PBOs */
   assert(packing->BufferObj);

   uint32_t dst_offset, dst_row_stride, dst_image_stride;
   struct brw_bo *dst_bo =
      blorp_get_client_bo(brw, width, height, depth,
                          target, format, type, pixels, packing,
                          &dst_offset, &dst_row_stride,
                          &dst_image_stride, false);
   if (dst_bo == NULL)
      return false;

   /* Now that source is offset to correct starting point, adjust the
    * given dimensions to treat 1D arrays as 2D.
    */
   if (target == GL_TEXTURE_1D_ARRAY) {
      assert(depth == 1);
      assert(z == 0);
      depth = height;
      height = 1;
      z = y;
      y = 0;
      dst_image_stride = dst_row_stride;
   }

   brw_miptree_check_level_layer(src_mt, level, z + depth - 1);

   int y0 = y;
   int y1 = y + height;
   if (y_flip) {
      apply_y_flip(&y0, &y1, minify(src_mt->surf.phys_level0_sa.height,
                                    level - src_mt->first_level));
   }

   bool result = false;

   /* Blit slice-by-slice creating a single-slice miptree for each layer. Even
    * in case of linear buffers hardware wants image arrays to be aligned by
    * four rows. This way hardware only gets one image at a time and any
    * source alignment will do.
    */
   for (unsigned i = 0; i < depth; ++i) {
      struct brw_mipmap_tree *dst_mt =
         brw_miptree_create_for_bo(brw, dst_bo, dst_format,
                                   dst_offset + i * dst_image_stride,
                                   width, height, 1,
                                   dst_row_stride,
                                   ISL_TILING_LINEAR, 0);

      if (!dst_mt) {
         perf_debug("%s: miptree creation for src failed\n", __func__);
         goto err;
      }

      /* In case exact match is needed, copy using equivalent UINT formats
       * preventing hardware from changing presentation for SNORM -1.
       */
      if (dst_mt->format == src_format && !y_flip &&
          src_swizzle == SWIZZLE_XYZW) {
         brw_blorp_copy_miptrees(brw, src_mt, level, z + i,
                                 dst_mt, 0, 0,
                                 x, y, 0, 0, width, height);
      } else {
         brw_blorp_blit_miptrees(brw, src_mt, level, z + i,
                                 src_format, src_swizzle,
                                 dst_mt, 0, 0, dst_format,
                                 x, y0, x + width, y1,
                                 0, 0, width, height,
                                 GL_NEAREST, false, y_flip, false, false);
      }

      brw_miptree_release(&dst_mt);
   }

   result = true;

   /* As we implement PBO transfers by binding the user-provided BO as a
    * fake framebuffer and rendering to it.  This breaks the invariant of the
    * GL that nothing is able to render to a BO, causing nondeterministic
    * corruption issues because the render cache is not coherent with a
    * number of other caches that the BO could potentially be bound to
    * afterwards.
    *
    * This could be solved in the same way that we guarantee texture
    * coherency after a texture is attached to a framebuffer and
    * rendered to, but that would involve checking *all* BOs bound to
    * the pipeline for the case we need to emit a cache flush due to
    * previous rendering to any of them -- Including vertex, index,
    * uniform, atomic counter, shader image, transform feedback,
    * indirect draw buffers, etc.
    *
    * That would increase the per-draw call overhead even though it's
    * very unlikely that any of the BOs bound to the pipeline has been
    * rendered to via a PBO at any point, so it seems better to just
    * flush here unconditionally.
    */
   brw_emit_mi_flush(brw);

err:
   brw_bo_unreference(dst_bo);

   return result;
}

static bool
set_write_disables(const struct brw_renderbuffer *irb,
                   const unsigned color_mask, uint8_t *color_write_disable)
{
   /* Format information in the renderbuffer represents the requirements
    * given by the client. There are cases where the backing miptree uses,
    * for example, RGBA to represent RGBX. Since the client is only expecting
    * RGB we can treat alpha as not used and write whatever we like into it.
    */
   const GLenum base_format = irb->Base.Base._BaseFormat;
   const int components = _mesa_components_in_format(base_format);
   assert(components > 0);
   *color_write_disable = ~color_mask & BITFIELD_MASK(components);
   return *color_write_disable;
}

static void
do_single_blorp_clear(struct brw_context *brw, struct gl_framebuffer *fb,
                      struct gl_renderbuffer *rb, unsigned buf,
                      bool partial_clear, bool encode_srgb)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   uint32_t x0, x1, y0, y1;

   mesa_format format = irb->Base.Base.Format;
   if (!encode_srgb)
      format = _mesa_get_srgb_format_linear(format);
   enum isl_format isl_format = brw->mesa_to_isl_render_format[format];

   x0 = fb->_Xmin;
   x1 = fb->_Xmax;
   if (fb->FlipY) {
      y0 = rb->Height - fb->_Ymax;
      y1 = rb->Height - fb->_Ymin;
   } else {
      y0 = fb->_Ymin;
      y1 = fb->_Ymax;
   }

   /* If the clear region is empty, just return. */
   if (x0 == x1 || y0 == y1)
      return;

   bool can_fast_clear = !partial_clear;

   if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
      can_fast_clear = false;

   uint8_t color_write_disable = 0;
   if (set_write_disables(irb, GET_COLORMASK(ctx->Color.ColorMask, buf),
                          &color_write_disable))
      can_fast_clear = false;

   /* We store clear colors as floats or uints as needed.  If there are
    * texture views in play, the formats will not properly be respected
    * during resolves because the resolve operations only know about the
    * miptree and not the renderbuffer.
    */
   if (irb->Base.Base.Format != irb->mt->format)
      can_fast_clear = false;

   if (!irb->mt->supports_fast_clear ||
       !brw_is_color_fast_clear_compatible(brw, irb->mt, &ctx->Color.ClearColor))
      can_fast_clear = false;

   /* Surface state can only record one fast clear color value. Therefore
    * unless different levels/layers agree on the color it can be used to
    * represent only single level/layer. Here it will be reserved for the
    * first slice (level 0, layer 0).
    */
   if (irb->layer_count > 1 || irb->mt_level || irb->mt_layer)
      can_fast_clear = false;

   unsigned level = irb->mt_level;
   const unsigned num_layers = fb->MaxNumLayers ? irb->layer_count : 1;

   /* If the MCS buffer hasn't been allocated yet, we need to allocate it now.
    */
   if (can_fast_clear && !irb->mt->aux_buf) {
      assert(irb->mt->aux_usage == ISL_AUX_USAGE_CCS_D);
      if (!brw_miptree_alloc_aux(brw, irb->mt)) {
         /* We're out of memory. Fall back to a non-fast clear. */
         can_fast_clear = false;
      }
   }

   if (can_fast_clear) {
      const enum isl_aux_state aux_state =
         brw_miptree_get_aux_state(irb->mt, irb->mt_level, irb->mt_layer);
      union isl_color_value clear_color =
         brw_meta_convert_fast_clear_color(brw, irb->mt,
                                           &ctx->Color.ClearColor);

      /* If the buffer is already in ISL_AUX_STATE_CLEAR and the clear color
       * hasn't changed, the clear is redundant and can be skipped.
       */
      if (!brw_miptree_set_clear_color(brw, irb->mt, clear_color) &&
          aux_state == ISL_AUX_STATE_CLEAR) {
         return;
      }

      DBG("%s (fast) to mt %p level %d layers %d+%d\n", __FUNCTION__,
          irb->mt, irb->mt_level, irb->mt_layer, num_layers);

      /* We can't setup the blorp_surf until we've allocated the MCS above */
      struct blorp_surf surf;
      blorp_surf_for_miptree(brw, &surf, irb->mt, irb->mt->aux_usage, true,
                             &level, irb->mt_layer, num_layers);

      /* Ivybrigde PRM Vol 2, Part 1, "11.7 MCS Buffer for Render Target(s)":
       *
       *    "Any transition from any value in {Clear, Render, Resolve} to a
       *    different value in {Clear, Render, Resolve} requires end of pipe
       *    synchronization."
       *
       * In other words, fast clear ops are not properly synchronized with
       * other drawing.  We need to use a PIPE_CONTROL to ensure that the
       * contents of the previous draw hit the render target before we resolve
       * and again afterwards to ensure that the resolve is complete before we
       * do any more regular drawing.
       */
      brw_emit_end_of_pipe_sync(brw, PIPE_CONTROL_RENDER_TARGET_FLUSH);

      struct blorp_batch batch;
      blorp_batch_init(&brw->blorp, &batch, brw, 0);
      blorp_fast_clear(&batch, &surf, isl_format_srgb_to_linear(isl_format),
                       ISL_SWIZZLE_IDENTITY,
                       level, irb->mt_layer, num_layers, x0, y0, x1, y1);
      blorp_batch_finish(&batch);

      brw_emit_end_of_pipe_sync(brw, PIPE_CONTROL_RENDER_TARGET_FLUSH);

      /* Now that the fast clear has occurred, put the buffer in
       * INTEL_FAST_CLEAR_STATE_CLEAR so that we won't waste time doing
       * redundant clears.
       */
      brw_miptree_set_aux_state(brw, irb->mt, irb->mt_level,
                                irb->mt_layer, num_layers,
                                ISL_AUX_STATE_CLEAR);
   } else {
      DBG("%s (slow) to mt %p level %d layer %d+%d\n", __FUNCTION__,
          irb->mt, irb->mt_level, irb->mt_layer, num_layers);

      enum isl_aux_usage aux_usage =
         brw_miptree_render_aux_usage(brw, irb->mt, isl_format, false, false);
      brw_miptree_prepare_render(brw, irb->mt, level, irb->mt_layer,
                                 num_layers, aux_usage);

      struct blorp_surf surf;
      blorp_surf_for_miptree(brw, &surf, irb->mt, aux_usage, true,
                             &level, irb->mt_layer, num_layers);

      union isl_color_value clear_color;
      memcpy(clear_color.f32, ctx->Color.ClearColor.f, sizeof(float) * 4);

      struct blorp_batch batch;
      blorp_batch_init(&brw->blorp, &batch, brw, 0);
      blorp_clear(&batch, &surf, isl_format, ISL_SWIZZLE_IDENTITY,
                  level, irb->mt_layer, num_layers,
                  x0, y0, x1, y1,
                  clear_color, color_write_disable);
      blorp_batch_finish(&batch);

      brw_miptree_finish_render(brw, irb->mt, level, irb->mt_layer,
                                num_layers, aux_usage);
   }

   return;
}

void
brw_blorp_clear_color(struct brw_context *brw, struct gl_framebuffer *fb,
                      GLbitfield mask, bool partial_clear, bool encode_srgb)
{
   for (unsigned buf = 0; buf < fb->_NumColorDrawBuffers; buf++) {
      struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[buf];
      struct brw_renderbuffer *irb = brw_renderbuffer(rb);

      /* Only clear the buffers present in the provided mask */
      if (((1 << fb->_ColorDrawBufferIndexes[buf]) & mask) == 0)
         continue;

      /* If this is an ES2 context or GL_ARB_ES2_compatibility is supported,
       * the framebuffer can be complete with some attachments missing.  In
       * this case the _ColorDrawBuffers pointer will be NULL.
       */
      if (rb == NULL)
         continue;

      do_single_blorp_clear(brw, fb, rb, buf, partial_clear, encode_srgb);
      irb->need_downsample = true;
   }

   return;
}

void
brw_blorp_clear_depth_stencil(struct brw_context *brw,
                              struct gl_framebuffer *fb,
                              GLbitfield mask, bool partial_clear)
{
   const struct gl_context *ctx = &brw->ctx;
   struct gl_renderbuffer *depth_rb =
      fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencil_rb =
      fb->Attachment[BUFFER_STENCIL].Renderbuffer;

   if (!depth_rb || ctx->Depth.Mask == GL_FALSE)
      mask &= ~BUFFER_BIT_DEPTH;

   if (!stencil_rb || (ctx->Stencil.WriteMask[0] & 0xff) == 0)
      mask &= ~BUFFER_BIT_STENCIL;

   if (!(mask & (BUFFER_BITS_DEPTH_STENCIL)))
      return;

   uint32_t x0, x1, y0, y1, rb_height;
   if (depth_rb) {
      rb_height = depth_rb->Height;
      if (stencil_rb) {
         assert(depth_rb->Width == stencil_rb->Width);
         assert(depth_rb->Height == stencil_rb->Height);
      }
   } else {
      assert(stencil_rb);
      rb_height = stencil_rb->Height;
   }

   x0 = fb->_Xmin;
   x1 = fb->_Xmax;
   if (fb->FlipY) {
      y0 = rb_height - fb->_Ymax;
      y1 = rb_height - fb->_Ymin;
   } else {
      y0 = fb->_Ymin;
      y1 = fb->_Ymax;
   }

   /* If the clear region is empty, just return. */
   if (x0 == x1 || y0 == y1)
      return;

   uint32_t level = 0, start_layer = 0, num_layers;
   struct blorp_surf depth_surf, stencil_surf;

   struct brw_mipmap_tree *depth_mt = NULL;
   if (mask & BUFFER_BIT_DEPTH) {
      struct brw_renderbuffer *irb = brw_renderbuffer(depth_rb);
      depth_mt = find_miptree(GL_DEPTH_BUFFER_BIT, irb);

      level = irb->mt_level;
      start_layer = irb->mt_layer;
      num_layers = fb->MaxNumLayers ? irb->layer_count : 1;

      brw_miptree_prepare_depth(brw, depth_mt, level, start_layer, num_layers);

      unsigned depth_level = level;
      blorp_surf_for_miptree(brw, &depth_surf, depth_mt, depth_mt->aux_usage,
                             true, &depth_level, start_layer, num_layers);
      assert(depth_level == level);
   }

   uint8_t stencil_mask = 0;
   struct brw_mipmap_tree *stencil_mt = NULL;
   if (mask & BUFFER_BIT_STENCIL) {
      struct brw_renderbuffer *irb = brw_renderbuffer(stencil_rb);
      stencil_mt = find_miptree(GL_STENCIL_BUFFER_BIT, irb);

      if (mask & BUFFER_BIT_DEPTH) {
         assert(level == irb->mt_level);
         assert(start_layer == irb->mt_layer);
         assert(num_layers == fb->MaxNumLayers ? irb->layer_count : 1);
      }

      level = irb->mt_level;
      start_layer = irb->mt_layer;
      num_layers = fb->MaxNumLayers ? irb->layer_count : 1;

      stencil_mask = ctx->Stencil.WriteMask[0] & 0xff;

      brw_miptree_prepare_access(brw, stencil_mt, level, 1,
                                 start_layer, num_layers,
                                 ISL_AUX_USAGE_NONE, false);

      unsigned stencil_level = level;
      blorp_surf_for_miptree(brw, &stencil_surf, stencil_mt,
                             ISL_AUX_USAGE_NONE, true,
                             &stencil_level, start_layer, num_layers);
   }

   assert((mask & BUFFER_BIT_DEPTH) || stencil_mask);

   struct blorp_batch batch;
   blorp_batch_init(&brw->blorp, &batch, brw, 0);
   blorp_clear_depth_stencil(&batch, &depth_surf, &stencil_surf,
                             level, start_layer, num_layers,
                             x0, y0, x1, y1,
                             (mask & BUFFER_BIT_DEPTH), ctx->Depth.Clear,
                             stencil_mask, ctx->Stencil.Clear);
   blorp_batch_finish(&batch);

   if (mask & BUFFER_BIT_DEPTH) {
      brw_miptree_finish_depth(brw, depth_mt, level,
                               start_layer, num_layers, true);
   }

   if (stencil_mask) {
      brw_miptree_finish_write(brw, stencil_mt, level,
                               start_layer, num_layers,
                               ISL_AUX_USAGE_NONE);
   }
}

void
brw_blorp_resolve_color(struct brw_context *brw, struct brw_mipmap_tree *mt,
                        unsigned level, unsigned layer,
                        enum isl_aux_op resolve_op)
{
   DBG("%s to mt %p level %u layer %u\n", __FUNCTION__, mt, level, layer);

   const mesa_format format = _mesa_get_srgb_format_linear(mt->format);

   struct blorp_surf surf;
   blorp_surf_for_miptree(brw, &surf, mt, mt->aux_usage, true,
                          &level, layer, 1 /* num_layers */);

   /* Ivybrigde PRM Vol 2, Part 1, "11.7 MCS Buffer for Render Target(s)":
    *
    *    "Any transition from any value in {Clear, Render, Resolve} to a
    *    different value in {Clear, Render, Resolve} requires end of pipe
    *    synchronization."
    *
    * In other words, fast clear ops are not properly synchronized with
    * other drawing.  We need to use a PIPE_CONTROL to ensure that the
    * contents of the previous draw hit the render target before we resolve
    * and again afterwards to ensure that the resolve is complete before we
    * do any more regular drawing.
    */
   brw_emit_end_of_pipe_sync(brw, PIPE_CONTROL_RENDER_TARGET_FLUSH);


   struct blorp_batch batch;
   blorp_batch_init(&brw->blorp, &batch, brw, 0);
   blorp_ccs_resolve(&batch, &surf, level, layer, 1,
                     brw_blorp_to_isl_format(brw, format, true),
                     resolve_op);
   blorp_batch_finish(&batch);

   /* See comment above */
   brw_emit_end_of_pipe_sync(brw, PIPE_CONTROL_RENDER_TARGET_FLUSH);
}

void
brw_blorp_mcs_partial_resolve(struct brw_context *brw,
                              struct brw_mipmap_tree *mt,
                              uint32_t start_layer, uint32_t num_layers)
{
   DBG("%s to mt %p layers %u-%u\n", __FUNCTION__, mt,
       start_layer, start_layer + num_layers - 1);

   assert(mt->aux_usage == ISL_AUX_USAGE_MCS);

   const mesa_format format = _mesa_get_srgb_format_linear(mt->format);
   enum isl_format isl_format = brw_blorp_to_isl_format(brw, format, true);

   struct blorp_surf surf;
   uint32_t level = 0;
   blorp_surf_for_miptree(brw, &surf, mt, ISL_AUX_USAGE_MCS, true,
                          &level, start_layer, num_layers);

   struct blorp_batch batch;
   blorp_batch_init(&brw->blorp, &batch, brw, 0);
   blorp_mcs_partial_resolve(&batch, &surf, isl_format,
                             start_layer, num_layers);
   blorp_batch_finish(&batch);
}

/**
 * Perform a HiZ or depth resolve operation.
 *
 * For an overview of HiZ ops, see the following sections of the Sandy Bridge
 * PRM, Volume 1, Part 2:
 *   - 7.5.3.1 Depth Buffer Clear
 *   - 7.5.3.2 Depth Buffer Resolve
 *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
 */
void
brw_hiz_exec(struct brw_context *brw, struct brw_mipmap_tree *mt,
             unsigned int level, unsigned int start_layer,
             unsigned int num_layers, enum isl_aux_op op)
{
   assert(brw_miptree_level_has_hiz(mt, level));
   assert(op != ISL_AUX_OP_NONE);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const char *opname = NULL;

   switch (op) {
   case ISL_AUX_OP_FULL_RESOLVE:
      opname = "depth resolve";
      break;
   case ISL_AUX_OP_AMBIGUATE:
      opname = "hiz ambiguate";
      break;
   case ISL_AUX_OP_FAST_CLEAR:
      opname = "depth clear";
      break;
   case ISL_AUX_OP_PARTIAL_RESOLVE:
   case ISL_AUX_OP_NONE:
      unreachable("Invalid HiZ op");
   }

   DBG("%s %s to mt %p level %d layers %d-%d\n",
       __func__, opname, mt, level, start_layer, start_layer + num_layers - 1);

   /* The following stalls and flushes are only documented to be required for
    * HiZ clear operations.  However, they also seem to be required for
    * resolve operations.
    */
   if (devinfo->ver == 6) {
      /* From the Sandy Bridge PRM, volume 2 part 1, page 313:
       *
       *   "If other rendering operations have preceded this clear, a
       *   PIPE_CONTROL with write cache flush enabled and Z-inhibit
       *   disabled must be issued before the rectangle primitive used for
       *   the depth buffer clear operation.
       */
       brw_emit_pipe_control_flush(brw,
                                   PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                   PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                   PIPE_CONTROL_CS_STALL);
   } else if (devinfo->ver >= 7) {
      /*
       * From the Ivybridge PRM, volume 2, "Depth Buffer Clear":
       *
       *   If other rendering operations have preceded this clear, a
       *   PIPE_CONTROL with depth cache flush enabled, Depth Stall bit
       *   enabled must be issued before the rectangle primitive used for
       *   the depth buffer clear operation.
       *
       * Same applies for Gfx8 and Gfx9.
       *
       * In addition, from the Ivybridge PRM, volume 2, 1.10.4.1
       * PIPE_CONTROL, Depth Cache Flush Enable:
       *
       *   This bit must not be set when Depth Stall Enable bit is set in
       *   this packet.
       *
       * This is confirmed to hold for real, HSW gets immediate gpu hangs.
       *
       * Therefore issue two pipe control flushes, one for cache flush and
       * another for depth stall.
       */
       brw_emit_pipe_control_flush(brw,
                                   PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                   PIPE_CONTROL_CS_STALL);

       brw_emit_pipe_control_flush(brw, PIPE_CONTROL_DEPTH_STALL);
   }

   assert(mt->aux_usage == ISL_AUX_USAGE_HIZ && mt->aux_buf);

   struct blorp_surf surf;
   blorp_surf_for_miptree(brw, &surf, mt, ISL_AUX_USAGE_HIZ, true,
                          &level, start_layer, num_layers);

   struct blorp_batch batch;
   blorp_batch_init(&brw->blorp, &batch, brw,
                    BLORP_BATCH_NO_UPDATE_CLEAR_COLOR);
   blorp_hiz_op(&batch, &surf, level, start_layer, num_layers, op);
   blorp_batch_finish(&batch);

   /* The following stalls and flushes are only documented to be required for
    * HiZ clear operations.  However, they also seem to be required for
    * resolve operations.
    */
   if (devinfo->ver == 6) {
      /* From the Sandy Bridge PRM, volume 2 part 1, page 314:
       *
       *     "DevSNB, DevSNB-B{W/A}]: Depth buffer clear pass must be
       *     followed by a PIPE_CONTROL command with DEPTH_STALL bit set
       *     and Then followed by Depth FLUSH'
      */
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_DEPTH_STALL);

      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                  PIPE_CONTROL_CS_STALL);
   } else if (devinfo->ver >= 8) {
      /*
       * From the Broadwell PRM, volume 7, "Depth Buffer Clear":
       *
       *    "Depth buffer clear pass using any of the methods (WM_STATE,
       *    3DSTATE_WM or 3DSTATE_WM_HZ_OP) must be followed by a
       *    PIPE_CONTROL command with DEPTH_STALL bit and Depth FLUSH bits
       *    "set" before starting to render.  DepthStall and DepthFlush are
       *    not needed between consecutive depth clear passes nor is it
       *    required if the depth clear pass was done with
       *    'full_surf_clear' bit set in the 3DSTATE_WM_HZ_OP."
       *
       *  TODO: Such as the spec says, this could be conditional.
       */
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                  PIPE_CONTROL_DEPTH_STALL);

   }
}
