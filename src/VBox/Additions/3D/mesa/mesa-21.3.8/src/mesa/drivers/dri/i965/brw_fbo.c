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

#include "main/enums.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/teximage.h"
#include "main/image.h"
#include "main/condrender.h"
#include "util/hash_table.h"
#include "util/set.h"
#include "util/u_memory.h"

#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "brw_batch.h"
#include "brw_buffers.h"
#include "brw_blit.h"
#include "brw_fbo.h"
#include "brw_mipmap_tree.h"
#include "brw_image.h"
#include "brw_screen.h"
#include "brw_tex.h"
#include "brw_context.h"
#include "brw_defines.h"

#define FILE_DEBUG_FLAG DEBUG_FBO

/** Called by gl_renderbuffer::Delete() */
static void
brw_delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);

   assert(irb);

   brw_miptree_release(&irb->mt);
   brw_miptree_release(&irb->singlesample_mt);

   _mesa_delete_renderbuffer(ctx, rb);
}

/**
 * \brief Downsample a winsys renderbuffer from mt to singlesample_mt.
 *
 * If the miptree needs no downsample, then skip.
 */
void
brw_renderbuffer_downsample(struct brw_context *brw,
                              struct brw_renderbuffer *irb)
{
   if (!irb->need_downsample)
      return;
   brw_miptree_updownsample(brw, irb->mt, irb->singlesample_mt);
   irb->need_downsample = false;
}

/**
 * \brief Upsample a winsys renderbuffer from singlesample_mt to mt.
 *
 * The upsample is done unconditionally.
 */
void
brw_renderbuffer_upsample(struct brw_context *brw,
                          struct brw_renderbuffer *irb)
{
   assert(!irb->need_downsample);

   brw_miptree_updownsample(brw, irb->singlesample_mt, irb->mt);
}

/**
 * \see dd_function_table::MapRenderbuffer
 */
static void
brw_map_renderbuffer(struct gl_context *ctx,
                     struct gl_renderbuffer *rb,
                     GLuint x, GLuint y, GLuint w, GLuint h,
                     GLbitfield mode,
                     GLubyte **out_map,
                     GLint *out_stride,
                     bool flip_y)
{
   struct brw_context *brw = brw_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   struct brw_mipmap_tree *mt;
   void *map;
   ptrdiff_t stride;

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer), not an irb */
      GLint bpp = _mesa_get_format_bytes(rb->Format);
      GLint rowStride = srb->RowStride;
      *out_map = (GLubyte *) srb->Buffer + y * rowStride + x * bpp;
      *out_stride = rowStride;
      return;
   }

   brw_prepare_render(brw);

   /* The MapRenderbuffer API should always return a single-sampled mapping.
    * The case we are asked to map multisampled RBs is in glReadPixels() (or
    * swrast paths like glCopyTexImage()) from a window-system MSAA buffer,
    * and GL expects an automatic resolve to happen.
    *
    * If it's a color miptree, there is a ->singlesample_mt which wraps the
    * actual window system renderbuffer (which we may resolve to at any time),
    * while the miptree itself is our driver-private allocation.  If it's a
    * depth or stencil miptree, we have a private MSAA buffer and no shared
    * singlesample buffer, and since we don't expect anybody to ever actually
    * resolve it, we just make a temporary singlesample buffer now when we
    * have to.
    */
   if (rb->NumSamples > 1) {
      if (!irb->singlesample_mt) {
         irb->singlesample_mt =
            brw_miptree_create_for_renderbuffer(brw, irb->mt->format,
                                                rb->Width, rb->Height,
                                                1 /*num_samples*/);
         if (!irb->singlesample_mt)
            goto fail;
         irb->singlesample_mt_is_tmp = true;
         irb->need_downsample = true;
      }

      brw_renderbuffer_downsample(brw, irb);
      mt = irb->singlesample_mt;

      irb->need_map_upsample = mode & GL_MAP_WRITE_BIT;
   } else {
      mt = irb->mt;
   }

   /* For a window-system renderbuffer, we need to flip the mapping we receive
    * upside-down.  So we need to ask for a rectangle on flipped vertically, and
    * we then return a pointer to the bottom of it with a negative stride.
    */
   if (flip_y) {
      y = rb->Height - y - h;
   }

   brw_miptree_map(brw, mt, irb->mt_level, irb->mt_layer,
                   x, y, w, h, mode, &map, &stride);

   if (flip_y) {
      map += (h - 1) * stride;
      stride = -stride;
   }

   DBG("%s: rb %d (%s) mt mapped: (%d, %d) (%dx%d) -> %p/%"PRIdPTR"\n",
       __func__, rb->Name, _mesa_get_format_name(rb->Format),
       x, y, w, h, map, stride);

   *out_map = map;
   *out_stride = stride;
   return;

fail:
   *out_map = NULL;
   *out_stride = 0;
}

/**
 * \see dd_function_table::UnmapRenderbuffer
 */
static void
brw_unmap_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   struct brw_context *brw = brw_context(ctx);
   struct swrast_renderbuffer *srb = (struct swrast_renderbuffer *)rb;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   struct brw_mipmap_tree *mt;

   DBG("%s: rb %d (%s)\n", __func__,
       rb->Name, _mesa_get_format_name(rb->Format));

   if (srb->Buffer) {
      /* this is a malloc'd renderbuffer (accum buffer) */
      /* nothing to do */
      return;
   }

   if (rb->NumSamples > 1) {
      mt = irb->singlesample_mt;
   } else {
      mt = irb->mt;
   }

   brw_miptree_unmap(brw, mt, irb->mt_level, irb->mt_layer);

   if (irb->need_map_upsample) {
      brw_renderbuffer_upsample(brw, irb);
      irb->need_map_upsample = false;
   }

   if (irb->singlesample_mt_is_tmp)
      brw_miptree_release(&irb->singlesample_mt);
}


/**
 * Round up the requested multisample count to the next supported sample size.
 */
unsigned
brw_quantize_num_samples(struct brw_screen *intel, unsigned num_samples)
{
   const int *msaa_modes = brw_supported_msaa_modes(intel);
   int quantized_samples = 0;

   for (int i = 0; msaa_modes[i] != -1; ++i) {
      if (msaa_modes[i] >= num_samples)
         quantized_samples = msaa_modes[i];
      else
         break;
   }

   return quantized_samples;
}

static mesa_format
brw_renderbuffer_format(struct gl_context * ctx, GLenum internalFormat)
{
   struct brw_context *brw = brw_context(ctx);
   ASSERTED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   switch (internalFormat) {
   default:
      /* Use the same format-choice logic as for textures.
       * Renderbuffers aren't any different from textures for us,
       * except they're less useful because you can't texture with
       * them.
       */
      return ctx->Driver.ChooseTextureFormat(ctx, GL_TEXTURE_2D,
                                             internalFormat,
                                             GL_NONE, GL_NONE);
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* These aren't actual texture formats, so force them here. */
      if (brw->has_separate_stencil) {
         return MESA_FORMAT_S_UINT8;
      } else {
         assert(!devinfo->must_use_separate_stencil);
         return MESA_FORMAT_Z24_UNORM_S8_UINT;
      }
   }
}

static GLboolean
brw_alloc_private_renderbuffer_storage(struct gl_context *ctx,
                                       struct gl_renderbuffer *rb,
                                       GLenum internalFormat,
                                       GLuint width, GLuint height)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_screen *screen = brw->screen;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);

   assert(rb->Format != MESA_FORMAT_NONE);

   rb->NumSamples = brw_quantize_num_samples(screen, rb->NumSamples);
   rb->NumStorageSamples = rb->NumSamples;
   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_get_format_base_format(rb->Format);

   brw_miptree_release(&irb->mt);

   DBG("%s: %s: %s (%dx%d)\n", __func__,
       _mesa_enum_to_string(internalFormat),
       _mesa_get_format_name(rb->Format), width, height);

   if (width == 0 || height == 0)
      return true;

   irb->mt = brw_miptree_create_for_renderbuffer(brw, rb->Format,
                                                 width, height,
                                                 MAX2(rb->NumSamples, 1));
   if (!irb->mt)
      return false;

   irb->layer_count = 1;

   return true;
}

/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
brw_alloc_renderbuffer_storage(struct gl_context * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   rb->Format = brw_renderbuffer_format(ctx, internalFormat);
   return brw_alloc_private_renderbuffer_storage(ctx, rb, internalFormat, width, height);
}

static mesa_format
fallback_rgbx_to_rgba(struct brw_screen *screen, struct gl_renderbuffer *rb,
                      mesa_format original_format)
{
   mesa_format format = original_format;

   /* The base format and internal format must be derived from the user-visible
    * format (that is, the gl_config's format), even if we internally use
    * choose a different format for the renderbuffer. Otherwise, rendering may
    * use incorrect channel write masks.
    */
   rb->_BaseFormat = _mesa_get_format_base_format(original_format);
   rb->InternalFormat = rb->_BaseFormat;

   if (!screen->mesa_format_supports_render[original_format]) {
      /* The glRenderbufferStorage paths in core Mesa detect if the driver
       * does not support the user-requested format, and then searches for
       * a fallback format. The DRI code bypasses core Mesa, though. So we do
       * the fallbacks here.
       *
       * We must support MESA_FORMAT_R8G8B8X8 on Android because the Android
       * framework requires HAL_PIXEL_FORMAT_RGBX8888 winsys surfaces.
       */
      format = _mesa_format_fallback_rgbx_to_rgba(original_format);
      assert(screen->mesa_format_supports_render[format]);
   }
   return format;
}

static void
brw_image_target_renderbuffer_storage(struct gl_context *ctx,
                                      struct gl_renderbuffer *rb,
                                      void *image_handle)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_renderbuffer *irb;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   __DRIimage *image;

   image = dri_screen->dri2.image->lookupEGLImage(dri_screen, image_handle,
                                                  dri_screen->loaderPrivate);
   if (image == NULL)
      return;

   if (image->planar_format && image->planar_format->nplanes > 1) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(planar buffers are not "
               "supported as render targets.)");
      return;
   }

   rb->Format = fallback_rgbx_to_rgba(brw->screen, rb, image->format);

   mesa_format chosen_format = rb->Format == image->format ?
      image->format : rb->Format;

   /* __DRIimage is opaque to the core so it has to be checked here */
   if (!brw->mesa_format_supports_render[chosen_format]) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glEGLImageTargetRenderbufferStorage(unsupported image format)");
      return;
   }

   irb = brw_renderbuffer(rb);
   brw_miptree_release(&irb->mt);

   /* Disable creation of the miptree's aux buffers because the driver exposes
    * no EGL API to manage them. That is, there is no API for resolving the aux
    * buffer's content to the main buffer nor for invalidating the aux buffer's
    * content.
    */
   irb->mt = brw_miptree_create_for_dri_image(brw, image, GL_TEXTURE_2D,
                                              rb->Format, false);
   if (!irb->mt)
      return;

   rb->Width = image->width;
   rb->Height = image->height;
   rb->NeedsFinishRenderTexture = true;
   irb->layer_count = 1;
}

/**
 * Called by _mesa_resize_framebuffer() for each hardware renderbuffer when a
 * window system framebuffer is resized.
 *
 * Any actual buffer reallocations for hardware renderbuffers (which would
 * have triggered _mesa_resize_framebuffer()) were done by
 * brw_process_dri2_buffer().
 */
static GLboolean
brw_alloc_window_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                         GLenum internalFormat, GLuint width, GLuint height)
{
   (void) ctx;
   assert(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;

   return true;
}

/** Dummy function for gl_renderbuffer::AllocStorage() */
static GLboolean
brw_nop_alloc_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                      GLenum internalFormat, GLuint width, GLuint height)
{
   (void) rb;
   (void) internalFormat;
   (void) width;
   (void) height;
   _mesa_problem(ctx, "brw_nop_alloc_storage should never be called.");
   return false;
}

/**
 * Create an brw_renderbuffer for a __DRIdrawable. This function is
 * unrelated to GL renderbuffers (that is, those created by
 * glGenRenderbuffers).
 *
 * \param num_samples must be quantized.
 */
struct brw_renderbuffer *
brw_create_winsys_renderbuffer(struct brw_screen *screen,
                               mesa_format format, unsigned num_samples)
{
   struct brw_renderbuffer *irb = CALLOC_STRUCT(brw_renderbuffer);
   if (!irb)
      return NULL;

   struct gl_renderbuffer *rb = &irb->Base.Base;
   irb->layer_count = 1;

   _mesa_init_renderbuffer(rb, 0);
   rb->ClassID = INTEL_RB_CLASS;
   rb->NumSamples = num_samples;
   rb->NumStorageSamples = num_samples;

   rb->Format = fallback_rgbx_to_rgba(screen, rb, format);

   /* intel-specific methods */
   rb->Delete = brw_delete_renderbuffer;
   rb->AllocStorage = brw_alloc_window_storage;

   return irb;
}

/**
 * Private window-system buffers (as opposed to ones shared with the display
 * server created with brw_create_winsys_renderbuffer()) are most similar in their
 * handling to user-created renderbuffers, but they have a resize handler that
 * may be called at brw_update_renderbuffers() time.
 *
 * \param num_samples must be quantized.
 */
struct brw_renderbuffer *
brw_create_private_renderbuffer(struct brw_screen *screen,
                                mesa_format format, unsigned num_samples)
{
   struct brw_renderbuffer *irb;

   irb = brw_create_winsys_renderbuffer(screen, format, num_samples);
   irb->Base.Base.AllocStorage = brw_alloc_private_renderbuffer_storage;

   return irb;
}

/**
 * Create a new renderbuffer object.
 * Typically called via glBindRenderbufferEXT().
 */
static struct gl_renderbuffer *
brw_new_renderbuffer(struct gl_context *ctx, GLuint name)
{
   struct brw_renderbuffer *irb;
   struct gl_renderbuffer *rb;

   irb = CALLOC_STRUCT(brw_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   rb = &irb->Base.Base;

   _mesa_init_renderbuffer(rb, name);
   rb->ClassID = INTEL_RB_CLASS;

   /* intel-specific methods */
   rb->Delete = brw_delete_renderbuffer;
   rb->AllocStorage = brw_alloc_renderbuffer_storage;
   /* span routines set in alloc_storage function */

   return rb;
}

static bool
brw_renderbuffer_update_wrapper(struct brw_context *brw,
                                struct brw_renderbuffer *irb,
                                struct gl_texture_image *image,
                                uint32_t layer,
                                bool layered)
{
   struct gl_renderbuffer *rb = &irb->Base.Base;
   struct brw_texture_image *intel_image = brw_texture_image(image);
   struct brw_mipmap_tree *mt = intel_image->mt;
   int level = image->Level;

   rb->AllocStorage = brw_nop_alloc_storage;

   /* adjust for texture view parameters */
   layer += image->TexObject->Attrib.MinLayer;
   level += image->TexObject->Attrib.MinLevel;

   brw_miptree_check_level_layer(mt, level, layer);
   irb->mt_level = level;
   irb->mt_layer = layer;

   if (!layered) {
      irb->layer_count = 1;
   } else if (mt->target != GL_TEXTURE_3D && image->TexObject->Attrib.NumLayers > 0) {
      irb->layer_count = image->TexObject->Attrib.NumLayers;
   } else {
      irb->layer_count = mt->surf.dim == ISL_SURF_DIM_3D ?
                            minify(mt->surf.logical_level0_px.depth, level) :
                            mt->surf.logical_level0_px.array_len;
   }

   brw_miptree_reference(&irb->mt, mt);

   brw_renderbuffer_set_draw_offset(irb);

   return true;
}

void
brw_renderbuffer_set_draw_offset(struct brw_renderbuffer *irb)
{
   unsigned int dst_x, dst_y;

   /* compute offset of the particular 2D image within the texture region */
   brw_miptree_get_image_offset(irb->mt, irb->mt_level, irb->mt_layer,
                                &dst_x, &dst_y);

   irb->draw_x = dst_x;
   irb->draw_y = dst_y;
}

/**
 * Called by glFramebufferTexture[123]DEXT() (and other places) to
 * prepare for rendering into texture memory.  This might be called
 * many times to choose different texture levels, cube faces, etc
 * before brw_finish_render_texture() is ever called.
 */
static void
brw_render_texture(struct gl_context * ctx,
                   struct gl_framebuffer *fb,
                   struct gl_renderbuffer_attachment *att)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_renderbuffer *rb = att->Renderbuffer;
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   struct gl_texture_image *image = rb->TexImage;
   struct brw_texture_image *intel_image = brw_texture_image(image);
   struct brw_mipmap_tree *mt = intel_image->mt;
   int layer;

   (void) fb;

   if (att->CubeMapFace > 0) {
      assert(att->Zoffset == 0);
      layer = att->CubeMapFace;
   } else {
      layer = att->Zoffset;
   }

   if (!intel_image->mt) {
      /* Fallback on drawing to a texture that doesn't have a miptree
       * (has a border, width/height 0, etc.)
       */
      _swrast_render_texture(ctx, fb, att);
      return;
   }

   brw_miptree_check_level_layer(mt, att->TextureLevel, layer);

   if (!brw_renderbuffer_update_wrapper(brw, irb, image, layer, att->Layered)) {
       _swrast_render_texture(ctx, fb, att);
       return;
   }

   DBG("Begin render %s texture tex=%u w=%d h=%d d=%d refcount=%d\n",
       _mesa_get_format_name(image->TexFormat),
       att->Texture->Name, image->Width, image->Height, image->Depth,
       rb->RefCount);
}


#define fbo_incomplete(fb, error_id, ...) do {                                          \
      static GLuint msg_id = 0;                                               \
      if (unlikely(ctx->Const.ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)) {    \
         _mesa_gl_debugf(ctx, &msg_id,                                        \
                         MESA_DEBUG_SOURCE_API,                               \
                         MESA_DEBUG_TYPE_OTHER,                               \
                         MESA_DEBUG_SEVERITY_MEDIUM,                          \
                         __VA_ARGS__);                                        \
      }                                                                       \
      DBG(__VA_ARGS__);                                                       \
      fb->_Status = error_id;                                                 \
   } while (0)

/**
 * Do additional "completeness" testing of a framebuffer object.
 */
static void
brw_validate_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct brw_renderbuffer *depthRb =
      brw_get_renderbuffer(fb, BUFFER_DEPTH);
   struct brw_renderbuffer *stencilRb =
      brw_get_renderbuffer(fb, BUFFER_STENCIL);
   struct brw_mipmap_tree *depth_mt = NULL, *stencil_mt = NULL;
   unsigned i;

   DBG("%s() on fb %p (%s)\n", __func__,
       fb, (fb == ctx->DrawBuffer ? "drawbuffer" :
            (fb == ctx->ReadBuffer ? "readbuffer" : "other buffer")));

   if (depthRb)
      depth_mt = depthRb->mt;
   if (stencilRb) {
      stencil_mt = stencilRb->mt;
      if (stencil_mt->stencil_mt)
         stencil_mt = stencil_mt->stencil_mt;
   }

   if (depth_mt && stencil_mt) {
      if (devinfo->ver >= 6) {
         const unsigned d_width = depth_mt->surf.phys_level0_sa.width;
         const unsigned d_height = depth_mt->surf.phys_level0_sa.height;
         const unsigned d_depth = depth_mt->surf.dim == ISL_SURF_DIM_3D ?
                                     depth_mt->surf.phys_level0_sa.depth :
                                     depth_mt->surf.phys_level0_sa.array_len;

         const unsigned s_width = stencil_mt->surf.phys_level0_sa.width;
         const unsigned s_height = stencil_mt->surf.phys_level0_sa.height;
         const unsigned s_depth = stencil_mt->surf.dim == ISL_SURF_DIM_3D ?
                                     stencil_mt->surf.phys_level0_sa.depth :
                                     stencil_mt->surf.phys_level0_sa.array_len;

         /* For gen >= 6, we are using the lod/minimum-array-element fields
          * and supporting layered rendering. This means that we must restrict
          * the depth & stencil attachments to match in various more retrictive
          * ways. (width, height, depth, LOD and layer)
          */
         if (d_width != s_width ||
             d_height != s_height ||
             d_depth != s_depth ||
             depthRb->mt_level != stencilRb->mt_level ||
             depthRb->mt_layer != stencilRb->mt_layer) {
            fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                           "FBO incomplete: depth and stencil must match in"
                           "width, height, depth, LOD and layer\n");
         }
      }
      if (depth_mt == stencil_mt) {
         /* For true packed depth/stencil (not faked on prefers-separate-stencil
          * hardware) we need to be sure they're the same level/layer, since
          * we'll be emitting a single packet describing the packed setup.
          */
         if (depthRb->mt_level != stencilRb->mt_level ||
             depthRb->mt_layer != stencilRb->mt_layer) {
            fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                           "FBO incomplete: depth image level/layer %d/%d != "
                           "stencil image %d/%d\n",
                           depthRb->mt_level,
                           depthRb->mt_layer,
                           stencilRb->mt_level,
                           stencilRb->mt_layer);
         }
      } else {
         if (!brw->has_separate_stencil) {
            fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                      "FBO incomplete: separate stencil unsupported\n");
         }
         if (stencil_mt->format != MESA_FORMAT_S_UINT8) {
            fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                      "FBO incomplete: separate stencil is %s "
                      "instead of S8\n",
                      _mesa_get_format_name(stencil_mt->format));
         }
         if (devinfo->ver < 7 && !brw_renderbuffer_has_hiz(depthRb)) {
            /* Before Gfx7, separate depth and stencil buffers can be used
             * only if HiZ is enabled. From the Sandybridge PRM, Volume 2,
             * Part 1, Bit 3DSTATE_DEPTH_BUFFER.SeparateStencilBufferEnable:
             *     [DevSNB]: This field must be set to the same value (enabled
             *     or disabled) as Hierarchical Depth Buffer Enable.
             */
            fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                          "FBO incomplete: separate stencil without HiZ\n");
         }
      }
   }

   for (i = 0; i < ARRAY_SIZE(fb->Attachment); i++) {
      struct gl_renderbuffer *rb;
      struct brw_renderbuffer *irb;

      if (fb->Attachment[i].Type == GL_NONE)
         continue;

      /* A supported attachment will have a Renderbuffer set either
       * from being a Renderbuffer or being a texture that got the
       * brw_wrap_texture() treatment.
       */
      rb = fb->Attachment[i].Renderbuffer;
      if (rb == NULL) {
         fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                       "FBO incomplete: attachment without "
                       "renderbuffer\n");
         continue;
      }

      if (fb->Attachment[i].Type == GL_TEXTURE) {
         if (rb->TexImage->Border) {
            fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                      "FBO incomplete: texture with border\n");
            continue;
         }
      }

      irb = brw_renderbuffer(rb);
      if (irb == NULL) {
         fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                   "FBO incomplete: software rendering renderbuffer\n");
         continue;
      }

     if (rb->Format == MESA_FORMAT_R_SRGB8) {
        fbo_incomplete(fb, GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
                       "FBO incomplete: Format not color renderable: %s\n",
                       _mesa_get_format_name(rb->Format));
        continue;
     }

      if (!brw_render_target_supported(brw, rb)) {
         fbo_incomplete(fb, GL_FRAMEBUFFER_UNSUPPORTED,
                   "FBO incomplete: Unsupported HW "
                   "texture/renderbuffer format attached: %s\n",
                   _mesa_get_format_name(brw_rb_format(irb)));
      }
   }
}

/**
 * Try to do a glBlitFramebuffer using glCopyTexSubImage2D
 * We can do this when the dst renderbuffer is actually a texture and
 * there is no scaling, mirroring or scissoring.
 *
 * \return new buffer mask indicating the buffers left to blit using the
 *         normal path.
 */
static GLbitfield
brw_blit_framebuffer_with_blitter(struct gl_context *ctx,
                                  const struct gl_framebuffer *readFb,
                                  const struct gl_framebuffer *drawFb,
                                  GLint srcX0, GLint srcY0,
                                  GLint srcX1, GLint srcY1,
                                  GLint dstX0, GLint dstY0,
                                  GLint dstX1, GLint dstY1,
                                  GLbitfield mask)
{
   struct brw_context *brw = brw_context(ctx);

   /* Sync up the state of window system buffers.  We need to do this before
    * we go looking for the buffers.
    */
   brw_prepare_render(brw);

   if (mask & GL_COLOR_BUFFER_BIT) {
      unsigned i;
      struct gl_renderbuffer *src_rb = readFb->_ColorReadBuffer;
      struct brw_renderbuffer *src_irb = brw_renderbuffer(src_rb);

      if (!src_irb) {
         perf_debug("glBlitFramebuffer(): missing src renderbuffer.  "
                    "Falling back to software rendering.\n");
         return mask;
      }

      /* If the source and destination are the same size with no mirroring,
       * the rectangles are within the size of the texture and there is no
       * scissor, then we can probably use the blit engine.
       */
      if (!(srcX0 - srcX1 == dstX0 - dstX1 &&
            srcY0 - srcY1 == dstY0 - dstY1 &&
            srcX1 >= srcX0 &&
            srcY1 >= srcY0 &&
            srcX0 >= 0 && srcX1 <= readFb->Width &&
            srcY0 >= 0 && srcY1 <= readFb->Height &&
            dstX0 >= 0 && dstX1 <= drawFb->Width &&
            dstY0 >= 0 && dstY1 <= drawFb->Height &&
            !(ctx->Scissor.EnableFlags))) {
         perf_debug("glBlitFramebuffer(): non-1:1 blit.  "
                    "Falling back to software rendering.\n");
         return mask;
      }

      /* Blit to all active draw buffers.  We don't do any pre-checking,
       * because we assume that copying to MRTs is rare, and failure midway
       * through copying is even more rare.  Even if it was to occur, it's
       * safe to let meta start the copy over from scratch, because
       * glBlitFramebuffer completely overwrites the destination pixels, and
       * results are undefined if any destination pixels have a dependency on
       * source pixels.
       */
      for (i = 0; i < drawFb->_NumColorDrawBuffers; i++) {
         struct gl_renderbuffer *dst_rb = drawFb->_ColorDrawBuffers[i];
         struct brw_renderbuffer *dst_irb = brw_renderbuffer(dst_rb);

         if (!dst_irb) {
            perf_debug("glBlitFramebuffer(): missing dst renderbuffer.  "
                       "Falling back to software rendering.\n");
            return mask;
         }

         if (ctx->Color.sRGBEnabled &&
             _mesa_is_format_srgb(src_irb->mt->format) !=
             _mesa_is_format_srgb(dst_irb->mt->format)) {
            perf_debug("glBlitFramebuffer() with sRGB conversion cannot be "
                       "handled by BLT path.\n");
            return mask;
         }

         if (!brw_miptree_blit(brw,
                               src_irb->mt,
                               src_irb->mt_level, src_irb->mt_layer,
                               srcX0, srcY0, readFb->FlipY,
                               dst_irb->mt,
                               dst_irb->mt_level, dst_irb->mt_layer,
                               dstX0, dstY0, drawFb->FlipY,
                               dstX1 - dstX0, dstY1 - dstY0,
                               COLOR_LOGICOP_COPY)) {
            perf_debug("glBlitFramebuffer(): unknown blit failure.  "
                       "Falling back to software rendering.\n");
            return mask;
         }
      }

      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   return mask;
}

static void
brw_blit_framebuffer(struct gl_context *ctx,
                     struct gl_framebuffer *readFb,
                     struct gl_framebuffer *drawFb,
                     GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                     GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                     GLbitfield mask, GLenum filter)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* Page 679 of OpenGL 4.4 spec says:
    *    "Added BlitFramebuffer to commands affected by conditional rendering in
    *     section 10.10 (Bug 9562)."
    */
   if (!_mesa_check_conditional_render(ctx))
      return;

   if (devinfo->ver < 6) {
      /* On gfx4-5, try BLT first.
       *
       * Gfx4-5 have a single ring for both 3D and BLT operations, so there's
       * no inter-ring synchronization issues like on Gfx6+.  It is apparently
       * faster than using the 3D pipeline.  Original Gfx4 also has to rebase
       * and copy miptree slices in order to render to unaligned locations.
       */
      mask = brw_blit_framebuffer_with_blitter(ctx, readFb, drawFb,
                                               srcX0, srcY0, srcX1, srcY1,
                                               dstX0, dstY0, dstX1, dstY1,
                                               mask);
      if (mask == 0x0)
         return;
   }

   mask = brw_blorp_framebuffer(brw, readFb, drawFb,
                                srcX0, srcY0, srcX1, srcY1,
                                dstX0, dstY0, dstX1, dstY1,
                                mask, filter);
   if (mask == 0x0)
      return;

   /* brw_blorp_framebuffer should always be successful for color blits. */
   assert(!(mask & GL_COLOR_BUFFER_BIT));

   mask = _mesa_meta_BlitFramebuffer(ctx, readFb, drawFb,
                                     srcX0, srcY0, srcX1, srcY1,
                                     dstX0, dstY0, dstX1, dstY1,
                                     mask, filter);
   if (mask == 0x0)
      return;

   if (devinfo->ver >= 8 && (mask & GL_STENCIL_BUFFER_BIT)) {
      assert(!"Invalid blit");
   }

   _swrast_BlitFramebuffer(ctx, readFb, drawFb,
                           srcX0, srcY0, srcX1, srcY1,
                           dstX0, dstY0, dstX1, dstY1,
                           mask, filter);
}

/**
 * Does the renderbuffer have hiz enabled?
 */
bool
brw_renderbuffer_has_hiz(struct brw_renderbuffer *irb)
{
   return brw_miptree_level_has_hiz(irb->mt, irb->mt_level);
}

void
brw_renderbuffer_move_to_temp(struct brw_context *brw,
                                struct brw_renderbuffer *irb,
                                bool invalidate)
{
   struct gl_renderbuffer *rb =&irb->Base.Base;
   struct brw_texture_image *intel_image = brw_texture_image(rb->TexImage);
   struct brw_mipmap_tree *new_mt;
   int width, height, depth;

   brw_get_image_dims(rb->TexImage, &width, &height, &depth);

   assert(irb->align_wa_mt == NULL);
   new_mt = brw_miptree_create(brw, GL_TEXTURE_2D,
                               intel_image->base.Base.TexFormat,
                               0, 0,
                               width, height, 1,
                               irb->mt->surf.samples,
                               MIPTREE_CREATE_BUSY);

   if (!invalidate) {
      brw_miptree_copy_slice(brw, intel_image->mt,
                             intel_image->base.Base.Level, irb->mt_layer,
                             new_mt, 0, 0);
   }

   brw_miptree_reference(&irb->align_wa_mt, new_mt);
   brw_miptree_release(&new_mt);

   irb->draw_x = 0;
   irb->draw_y = 0;
}

void
brw_cache_sets_clear(struct brw_context *brw)
{
   hash_table_foreach(brw->render_cache, render_entry)
      _mesa_hash_table_remove(brw->render_cache, render_entry);

   set_foreach(brw->depth_cache, depth_entry)
      _mesa_set_remove(brw->depth_cache, depth_entry);
}

/**
 * Emits an appropriate flush for a BO if it has been rendered to within the
 * same batchbuffer as a read that's about to be emitted.
 *
 * The GPU has separate, incoherent caches for the render cache and the
 * sampler cache, along with other caches.  Usually data in the different
 * caches don't interact (e.g. we don't render to our driver-generated
 * immediate constant data), but for render-to-texture in FBOs we definitely
 * do.  When a batchbuffer is flushed, the kernel will ensure that everything
 * necessary is flushed before another use of that BO, but for reuse from
 * different caches within a batchbuffer, it's all our responsibility.
 */
static void
flush_depth_and_render_caches(struct brw_context *brw, struct brw_bo *bo)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->ver >= 6) {
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                  PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                  PIPE_CONTROL_CS_STALL);

      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
                                  PIPE_CONTROL_CONST_CACHE_INVALIDATE);
   } else {
      brw_emit_mi_flush(brw);
   }

   brw_cache_sets_clear(brw);
}

void
brw_cache_flush_for_read(struct brw_context *brw, struct brw_bo *bo)
{
   if (_mesa_hash_table_search(brw->render_cache, bo) ||
       _mesa_set_search(brw->depth_cache, bo))
      flush_depth_and_render_caches(brw, bo);
}

static void *
format_aux_tuple(enum isl_format format, enum isl_aux_usage aux_usage)
{
   return (void *)(uintptr_t)((uint32_t)format << 8 | aux_usage);
}

void
brw_cache_flush_for_render(struct brw_context *brw, struct brw_bo *bo,
                           enum isl_format format,
                           enum isl_aux_usage aux_usage)
{
   if (_mesa_set_search(brw->depth_cache, bo))
      flush_depth_and_render_caches(brw, bo);

   /* Check to see if this bo has been used by a previous rendering operation
    * but with a different format or aux usage.  If it has, flush the render
    * cache so we ensure that it's only in there with one format or aux usage
    * at a time.
    *
    * Even though it's not obvious, this can easily happen in practice.
    * Suppose a client is blending on a surface with sRGB encode enabled on
    * gfx9.  This implies that you get AUX_USAGE_CCS_D at best.  If the client
    * then disables sRGB decode and continues blending we will flip on
    * AUX_USAGE_CCS_E without doing any sort of resolve in-between (this is
    * perfectly valid since CCS_E is a subset of CCS_D).  However, this means
    * that we have fragments in-flight which are rendering with UNORM+CCS_E
    * and other fragments in-flight with SRGB+CCS_D on the same surface at the
    * same time and the pixel scoreboard and color blender are trying to sort
    * it all out.  This ends badly (i.e. GPU hangs).
    *
    * To date, we have never observed GPU hangs or even corruption to be
    * associated with switching the format, only the aux usage.  However,
    * there are comments in various docs which indicate that the render cache
    * isn't 100% resilient to format changes.  We may as well be conservative
    * and flush on format changes too.  We can always relax this later if we
    * find it to be a performance problem.
    */
   struct hash_entry *entry = _mesa_hash_table_search(brw->render_cache, bo);
   if (entry && entry->data != format_aux_tuple(format, aux_usage))
      flush_depth_and_render_caches(brw, bo);
}

void
brw_render_cache_add_bo(struct brw_context *brw, struct brw_bo *bo,
                        enum isl_format format,
                        enum isl_aux_usage aux_usage)
{
#ifndef NDEBUG
   struct hash_entry *entry = _mesa_hash_table_search(brw->render_cache, bo);
   if (entry) {
      /* Otherwise, someone didn't do a flush_for_render and that would be
       * very bad indeed.
       */
      assert(entry->data == format_aux_tuple(format, aux_usage));
   }
#endif

   _mesa_hash_table_insert(brw->render_cache, bo,
                           format_aux_tuple(format, aux_usage));
}

void
brw_cache_flush_for_depth(struct brw_context *brw, struct brw_bo *bo)
{
   if (_mesa_hash_table_search(brw->render_cache, bo))
      flush_depth_and_render_caches(brw, bo);
}

void
brw_depth_cache_add_bo(struct brw_context *brw, struct brw_bo *bo)
{
   _mesa_set_add(brw->depth_cache, bo);
}

/**
 * Do one-time context initializations related to GL_EXT_framebuffer_object.
 * Hook in device driver functions.
 */
void
brw_fbo_init(struct brw_context *brw)
{
   struct dd_function_table *dd = &brw->ctx.Driver;
   dd->NewRenderbuffer = brw_new_renderbuffer;
   dd->MapRenderbuffer = brw_map_renderbuffer;
   dd->UnmapRenderbuffer = brw_unmap_renderbuffer;
   dd->RenderTexture = brw_render_texture;
   dd->ValidateFramebuffer = brw_validate_framebuffer;
   dd->BlitFramebuffer = brw_blit_framebuffer;
   dd->EGLImageTargetRenderbufferStorage =
      brw_image_target_renderbuffer_storage;

   brw->render_cache = _mesa_hash_table_create(brw->mem_ctx, _mesa_hash_pointer,
                                               _mesa_key_pointer_equal);
   brw->depth_cache = _mesa_set_create(brw->mem_ctx, _mesa_hash_pointer,
                                       _mesa_key_pointer_equal);
   util_dynarray_init(&brw->batch.exec_fences, NULL);
}
