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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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
#include "main/image.h"
#include "main/glformats.h"
#include "main/mtypes.h"
#include "main/condrender.h"
#include "main/fbobject.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/bufferobj.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"

#include "brw_context.h"
#include "brw_screen.h"
#include "brw_blit.h"
#include "brw_buffers.h"
#include "brw_fbo.h"
#include "brw_mipmap_tree.h"
#include "brw_pixel.h"
#include "brw_buffer_objects.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static bool
do_blit_drawpixels(struct gl_context * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *unpack,
                   const GLvoid * pixels)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_buffer_object *src = brw_buffer_object(unpack->BufferObj);
   GLuint src_offset;
   struct brw_bo *src_buffer;

   DBG("%s\n", __func__);

   if (!brw_check_blit_fragment_ops(ctx, false))
      return false;

   if (ctx->DrawBuffer->_NumColorDrawBuffers != 1) {
      DBG("%s: fallback due to MRT\n", __func__);
      return false;
   }

   brw_prepare_render(brw);

   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
   struct brw_renderbuffer *irb = brw_renderbuffer(rb);

   mesa_format src_format = _mesa_format_from_format_and_type(format, type);
   if (_mesa_format_is_mesa_array_format(src_format))
      src_format = _mesa_format_from_array_format(src_format);
   mesa_format dst_format = irb->mt->format;

   /* We can safely discard sRGB encode/decode for the DrawPixels interface */
   src_format = _mesa_get_srgb_format_linear(src_format);
   dst_format = _mesa_get_srgb_format_linear(dst_format);

   if (!brw_miptree_blit_compatible_formats(src_format, dst_format)) {
      DBG("%s: bad format for blit\n", __func__);
      return false;
   }

   if (unpack->SwapBytes || unpack->LsbFirst ||
       unpack->SkipPixels || unpack->SkipRows) {
      DBG("%s: bad packing params\n", __func__);
      return false;
   }

   int src_stride = _mesa_image_row_stride(unpack, width, format, type);
   bool src_flip = false;
   /* Mesa flips the src_stride for unpack->Invert, but we want our mt to have
    * a normal src_stride.
    */
   if (unpack->Invert) {
      src_stride = -src_stride;
      src_flip = true;
   }

   src_offset = (GLintptr)pixels;
   src_offset += _mesa_image_offset(2, unpack, width, height,
                                    format, type, 0, 0, 0);

   src_buffer = brw_bufferobj_buffer(brw, src, src_offset,
                                     height * src_stride, false);

   struct brw_mipmap_tree *pbo_mt =
      brw_miptree_create_for_bo(brw,
                                  src_buffer,
                                  irb->mt->format,
                                  src_offset,
                                  width, height, 1,
                                  src_stride,
                                  ISL_TILING_LINEAR,
                                  MIPTREE_CREATE_DEFAULT);
   if (!pbo_mt)
      return false;

   if (!brw_miptree_blit(brw,
                           pbo_mt, 0, 0,
                           0, 0, src_flip,
                           irb->mt, irb->mt_level, irb->mt_layer,
                           x, y, ctx->DrawBuffer->FlipY,
                           width, height, COLOR_LOGICOP_COPY)) {
      DBG("%s: blit failed\n", __func__);
      brw_miptree_release(&pbo_mt);
      return false;
   }

   brw_miptree_release(&pbo_mt);

   if (ctx->Query.CurrentOcclusionObject)
      ctx->Query.CurrentOcclusionObject->Result += width * height;

   DBG("%s: success\n", __func__);
   return true;
}

void
brw_drawpixels(struct gl_context *ctx,
               GLint x, GLint y,
               GLsizei width, GLsizei height,
               GLenum format,
               GLenum type,
               const struct gl_pixelstore_attrib *unpack,
               const GLvoid *pixels)
{
   struct brw_context *brw = brw_context(ctx);

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (format == GL_STENCIL_INDEX) {
      _swrast_DrawPixels(ctx, x, y, width, height, format, type,
                         unpack, pixels);
      return;
   }

   if (brw->screen->devinfo.ver < 6 &&
       unpack->BufferObj) {
      if (do_blit_drawpixels(ctx, x, y, width, height, format, type, unpack,
                             pixels)) {
         return;
      }

      perf_debug("%s: fallback to generic code in PBO case\n", __func__);
   }

   _mesa_meta_DrawPixels(ctx, x, y, width, height, format, type,
                         unpack, pixels);
}
