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

#include "main/enums.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/fbobject.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/readpix.h"
#include "main/state.h"
#include "main/glformats.h"
#include "program/prog_instruction.h"
#include "drivers/common/meta.h"

#include "brw_context.h"
#include "brw_blorp.h"
#include "brw_screen.h"
#include "brw_batch.h"
#include "brw_buffers.h"
#include "brw_fbo.h"
#include "brw_mipmap_tree.h"
#include "brw_pixel.h"
#include "brw_buffer_objects.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

/**
 * \brief A fast path for glReadPixels
 *
 * This fast path is taken when the source format is BGRA, RGBA,
 * A or L and when the texture memory is X- or Y-tiled.  It downloads
 * the source data by directly mapping the memory without a GTT fence.
 * This then needs to be de-tiled on the CPU before presenting the data to
 * the user in the linear fasion.
 *
 * This is a performance win over the conventional texture download path.
 * In the conventional texture download path, the texture is either mapped
 * through the GTT or copied to a linear buffer with the blitter before
 * handing off to a software path.  This allows us to avoid round-tripping
 * through the GPU (in the case where we would be blitting) and do only a
 * single copy operation.
 */
static bool
brw_readpixels_tiled_memcpy(struct gl_context *ctx,
                            GLint xoffset, GLint yoffset,
                            GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            GLvoid * pixels,
                            const struct gl_pixelstore_attrib *pack)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* This path supports reading from color buffers only */
   if (rb == NULL)
      return false;

   struct brw_renderbuffer *irb = brw_renderbuffer(rb);
   int dst_pitch;

   /* The miptree's buffer. */
   struct brw_bo *bo;

   uint32_t cpp;
   isl_memcpy_type copy_type;

   /* This fastpath is restricted to specific renderbuffer types:
    * a 2D BGRA, RGBA, L8 or A8 texture. It could be generalized to support
    * more types.
    */
   if (!devinfo->has_llc ||
       !(type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8_REV) ||
       pixels == NULL ||
       pack->BufferObj ||
       pack->Alignment > 4 ||
       pack->SkipPixels > 0 ||
       pack->SkipRows > 0 ||
       (pack->RowLength != 0 && pack->RowLength != width) ||
       pack->SwapBytes ||
       pack->LsbFirst ||
       pack->Invert)
      return false;

   /* Only a simple blit, no scale, bias or other mapping. */
   if (ctx->_ImageTransferState)
      return false;

   /* It is possible that the renderbuffer (or underlying texture) is
    * multisampled.  Since ReadPixels from a multisampled buffer requires a
    * multisample resolve, we can't handle this here
    */
   if (rb->NumSamples > 1)
      return false;

   /* We can't handle copying from RGBX or BGRX because the tiled_memcpy
    * function doesn't set the last channel to 1. Note this checks BaseFormat
    * rather than TexFormat in case the RGBX format is being simulated with an
    * RGBA format.
    */
   if (rb->_BaseFormat == GL_RGB)
      return false;

   copy_type = brw_miptree_get_memcpy_type(rb->Format, format, type, &cpp);
   if (copy_type == ISL_MEMCPY_INVALID)
      return false;

   if (!irb->mt ||
       (irb->mt->surf.tiling != ISL_TILING_X &&
        irb->mt->surf.tiling != ISL_TILING_Y0)) {
      /* The algorithm is written only for X- or Y-tiled memory. */
      return false;
   }

   /* tiled_to_linear() assumes that if the object is swizzled, it is using
    * I915_BIT6_SWIZZLE_9_10 for X and I915_BIT6_SWIZZLE_9 for Y.  This is only
    * true on gfx5 and above.
    *
    * The killer on top is that some gfx4 have an L-shaped swizzle mode, where
    * parts of the memory aren't swizzled at all. Userspace just can't handle
    * that.
    */
   if (devinfo->ver < 5 && brw->has_swizzling)
      return false;

   /* Since we are going to read raw data to the miptree, we need to resolve
    * any pending fast color clears before we start.
    */
   brw_miptree_access_raw(brw, irb->mt, irb->mt_level, irb->mt_layer, false);

   bo = irb->mt->bo;

   if (brw_batch_references(&brw->batch, bo)) {
      perf_debug("Flushing before mapping a referenced bo.\n");
      brw_batch_flush(brw);
   }

   void *map = brw_bo_map(brw, bo, MAP_READ | MAP_RAW);
   if (map == NULL) {
      DBG("%s: failed to map bo\n", __func__);
      return false;
   }

   unsigned slice_offset_x, slice_offset_y;
   brw_miptree_get_image_offset(irb->mt, irb->mt_level, irb->mt_layer,
                                  &slice_offset_x, &slice_offset_y);
   xoffset += slice_offset_x;
   yoffset += slice_offset_y;

   dst_pitch = _mesa_image_row_stride(pack, width, format, type);

   /* For a window-system renderbuffer, the buffer is actually flipped
    * vertically, so we need to handle that.  Since the detiling function
    * can only really work in the forwards direction, we have to be a
    * little creative.  First, we compute the Y-offset of the first row of
    * the renderbuffer (in renderbuffer coordinates).  We then match that
    * with the last row of the client's data.  Finally, we give
    * tiled_to_linear a negative pitch so that it walks through the
    * client's data backwards as it walks through the renderbufer forwards.
    */
   if (ctx->ReadBuffer->FlipY) {
      yoffset = rb->Height - yoffset - height;
      pixels += (ptrdiff_t) (height - 1) * dst_pitch;
      dst_pitch = -dst_pitch;
   }

   /* We postponed printing this message until having committed to executing
    * the function.
    */
   DBG("%s: x,y=(%d,%d) (w,h)=(%d,%d) format=0x%x type=0x%x "
       "mesa_format=0x%x tiling=%d "
       "pack=(alignment=%d row_length=%d skip_pixels=%d skip_rows=%d)\n",
       __func__, xoffset, yoffset, width, height,
       format, type, rb->Format, irb->mt->surf.tiling,
       pack->Alignment, pack->RowLength, pack->SkipPixels,
       pack->SkipRows);

   isl_memcpy_tiled_to_linear(
      xoffset * cpp, (xoffset + width) * cpp,
      yoffset, yoffset + height,
      pixels,
      map + irb->mt->offset,
      dst_pitch, irb->mt->surf.row_pitch_B,
      brw->has_swizzling,
      irb->mt->surf.tiling,
      copy_type
   );

   brw_bo_unmap(bo);
   return true;
}

static bool
brw_readpixels_blorp(struct gl_context *ctx,
                     unsigned x, unsigned y,
                     unsigned w, unsigned h,
                     GLenum format, GLenum type, const void *pixels,
                     const struct gl_pixelstore_attrib *packing)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   if (!rb)
      return false;

   struct brw_renderbuffer *irb = brw_renderbuffer(rb);

   /* _mesa_get_readpixels_transfer_ops() includes the cases of read
    * color clamping along with the ctx->_ImageTransferState.
    */
   if (_mesa_get_readpixels_transfer_ops(ctx, rb->Format, format,
                                         type, GL_FALSE))
      return false;

   GLenum dst_base_format = _mesa_unpack_format_to_base_format(format);
   if (_mesa_need_rgb_to_luminance_conversion(rb->_BaseFormat,
                                              dst_base_format))
      return false;

   unsigned swizzle;
   if (irb->Base.Base._BaseFormat == GL_RGB) {
      swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
   } else {
      swizzle = SWIZZLE_XYZW;
   }

   return brw_blorp_download_miptree(brw, irb->mt, rb->Format, swizzle,
                                     irb->mt_level, x, y, irb->mt_layer,
                                     w, h, 1, GL_TEXTURE_2D, format, type,
                                     ctx->ReadBuffer->FlipY, pixels, packing);
}

void
brw_readpixels(struct gl_context *ctx,
               GLint x, GLint y, GLsizei width, GLsizei height,
               GLenum format, GLenum type,
               const struct gl_pixelstore_attrib *pack, GLvoid *pixels)
{
   bool ok;

   struct brw_context *brw = brw_context(ctx);
   bool dirty;

   DBG("%s\n", __func__);

   /* Reading pixels wont dirty the front buffer, so reset the dirty
    * flag after calling brw_prepare_render().
    */
   dirty = brw->front_buffer_dirty;
   brw_prepare_render(brw);
   brw->front_buffer_dirty = dirty;

   if (pack->BufferObj) {
      if (brw_readpixels_blorp(ctx, x, y, width, height,
                               format, type, pixels, pack))
         return;

      perf_debug("%s: fallback to CPU mapping in PBO case\n", __func__);
   }

   ok = brw_readpixels_tiled_memcpy(ctx, x, y, width, height,
                                    format, type, pixels, pack);
   if(ok)
      return;

   /* Update Mesa state before calling _mesa_readpixels().
    * XXX this may not be needed since ReadPixels no longer uses the
    * span code.
    */

   if (ctx->NewState)
      _mesa_update_state(ctx);

   _mesa_readpixels(ctx, x, y, width, height, format, type, pack, pixels);

   /* There's an brw_prepare_render() call in intelSpanRenderStart(). */
   brw->front_buffer_dirty = dirty;
}
