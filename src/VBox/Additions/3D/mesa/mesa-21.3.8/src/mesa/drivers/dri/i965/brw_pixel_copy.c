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

#include "main/image.h"
#include "main/state.h"
#include "main/stencil.h"
#include "main/mtypes.h"
#include "main/condrender.h"
#include "main/fbobject.h"
#include "drivers/common/meta.h"

#include "brw_context.h"
#include "brw_buffers.h"
#include "brw_mipmap_tree.h"
#include "brw_pixel.h"
#include "brw_fbo.h"
#include "brw_blit.h"
#include "brw_batch.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

/**
 * CopyPixels with the blitter.  Don't support zooming, pixel transfer, etc.
 */
static bool
do_blit_copypixels(struct gl_context * ctx,
                   GLint srcx, GLint srcy,
                   GLsizei width, GLsizei height,
                   GLint dstx, GLint dsty, GLenum type)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_framebuffer *read_fb = ctx->ReadBuffer;
   GLint orig_dstx;
   GLint orig_dsty;
   GLint orig_srcx;
   GLint orig_srcy;
   struct brw_renderbuffer *draw_irb = NULL;
   struct brw_renderbuffer *read_irb = NULL;

   /* Update draw buffer bounds */
   _mesa_update_state(ctx);

   brw_prepare_render(brw);

   switch (type) {
   case GL_COLOR:
      if (fb->_NumColorDrawBuffers != 1) {
         perf_debug("glCopyPixels() fallback: MRT\n");
         return false;
      }

      draw_irb = brw_renderbuffer(fb->_ColorDrawBuffers[0]);
      read_irb = brw_renderbuffer(read_fb->_ColorReadBuffer);
      break;
   case GL_DEPTH_STENCIL_EXT:
      draw_irb = brw_renderbuffer(fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      read_irb =
         brw_renderbuffer(read_fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      break;
   case GL_DEPTH:
      perf_debug("glCopyPixels() fallback: GL_DEPTH\n");
      return false;
   case GL_STENCIL:
      perf_debug("glCopyPixels() fallback: GL_STENCIL\n");
      return false;
   default:
      perf_debug("glCopyPixels(): Unknown type\n");
      return false;
   }

   if (!draw_irb) {
      perf_debug("glCopyPixels() fallback: missing draw buffer\n");
      return false;
   }

   if (!read_irb) {
      perf_debug("glCopyPixels() fallback: missing read buffer\n");
      return false;
   }

   if (draw_irb->mt->surf.samples > 1 || read_irb->mt->surf.samples > 1) {
      perf_debug("glCopyPixels() fallback: multisampled buffers\n");
      return false;
   }

   if (ctx->_ImageTransferState) {
      perf_debug("glCopyPixels(): Unsupported image transfer state\n");
      return false;
   }

   if (ctx->Depth.Test) {
      perf_debug("glCopyPixels(): Unsupported depth test state\n");
      return false;
   }

   if (brw->stencil_enabled) {
      perf_debug("glCopyPixels(): Unsupported stencil test state\n");
      return false;
   }

   if (ctx->Fog.Enabled ||
       ctx->Texture._MaxEnabledTexImageUnit != -1 ||
       _mesa_arb_fragment_program_enabled(ctx)) {
      perf_debug("glCopyPixels(): Unsupported fragment shader state\n");
      return false;
   }

   if (ctx->Color.AlphaEnabled ||
       ctx->Color.BlendEnabled) {
      perf_debug("glCopyPixels(): Unsupported blend state\n");
      return false;
   }

   if (GET_COLORMASK(ctx->Color.ColorMask, 0) != 0xf) {
      perf_debug("glCopyPixels(): Unsupported color mask state\n");
      return false;
   }

   if (ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F) {
      perf_debug("glCopyPixels(): Unsupported pixel zoom\n");
      return false;
   }

   brw_batch_flush(brw);

   /* Clip to destination buffer. */
   orig_dstx = dstx;
   orig_dsty = dsty;
   if (!_mesa_clip_to_region(fb->_Xmin, fb->_Ymin,
                             fb->_Xmax, fb->_Ymax,
                             &dstx, &dsty, &width, &height))
      goto out;
   /* Adjust src coords for our post-clipped destination origin */
   srcx += dstx - orig_dstx;
   srcy += dsty - orig_dsty;

   /* Clip to source buffer. */
   orig_srcx = srcx;
   orig_srcy = srcy;
   if (!_mesa_clip_to_region(0, 0,
                             read_fb->Width, read_fb->Height,
                             &srcx, &srcy, &width, &height))
      goto out;
   /* Adjust dst coords for our post-clipped source origin */
   dstx += srcx - orig_srcx;
   dsty += srcy - orig_srcy;

   if (!brw_miptree_blit(brw,
                           read_irb->mt, read_irb->mt_level, read_irb->mt_layer,
                           srcx, srcy, read_fb->FlipY,
                           draw_irb->mt, draw_irb->mt_level, draw_irb->mt_layer,
                           dstx, dsty, fb->FlipY,
                           width, height,
                           (ctx->Color.ColorLogicOpEnabled ?
                            ctx->Color._LogicOp : COLOR_LOGICOP_COPY))) {
      DBG("%s: blit failure\n", __func__);
      return false;
   }

   if (ctx->Query.CurrentOcclusionObject)
      ctx->Query.CurrentOcclusionObject->Result += width * height;

out:

   DBG("%s: success\n", __func__);
   return true;
}


void
brw_copypixels(struct gl_context *ctx,
               GLint srcx, GLint srcy,
               GLsizei width, GLsizei height,
               GLint destx, GLint desty, GLenum type)
{
   struct brw_context *brw = brw_context(ctx);

   DBG("%s\n", __func__);

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (brw->screen->devinfo.ver < 6 &&
       do_blit_copypixels(ctx, srcx, srcy, width, height, destx, desty, type))
      return;

   /* this will use swrast if needed */
   _mesa_meta_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type);
}
