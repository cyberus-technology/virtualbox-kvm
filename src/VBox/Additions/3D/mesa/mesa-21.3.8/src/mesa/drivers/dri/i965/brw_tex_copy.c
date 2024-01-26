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

#include "main/mtypes.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/fbobject.h"

#include "drivers/common/meta.h"

#include "brw_screen.h"
#include "brw_mipmap_tree.h"
#include "brw_fbo.h"
#include "brw_tex.h"
#include "brw_context.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE


static void
brw_copytexsubimage(struct gl_context *ctx, GLuint dims,
                    struct gl_texture_image *texImage,
                    GLint xoffset, GLint yoffset, GLint slice,
                    struct gl_renderbuffer *rb,
                    GLint x, GLint y,
                    GLsizei width, GLsizei height)
{
   struct brw_context *brw = brw_context(ctx);

   /* Try BLORP first.  It can handle almost everything. */
   if (brw_blorp_copytexsubimage(brw, rb, texImage, slice, x, y,
                                 xoffset, yoffset, width, height))
      return;

   /* Finally, fall back to meta.  This will likely be slow. */
   perf_debug("%s - fallback to swrast\n", __func__);
   _mesa_meta_CopyTexSubImage(ctx, dims, texImage,
                              xoffset, yoffset, slice,
                              rb, x, y, width, height);
}


void
brw_init_texture_copy_image_functions(struct dd_function_table *functions)
{
   functions->CopyTexSubImage = brw_copytexsubimage;
}
