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

#ifndef BRW_PIXEL_H
#define BRW_PIXEL_H

#include "main/mtypes.h"

void brw_init_pixel_functions(struct dd_function_table *functions);
bool brw_check_blit_fragment_ops(struct gl_context *ctx,
                                 bool src_alpha_is_one);

void brw_readpixels(struct gl_context *ctx,
                    GLint x, GLint y,
                    GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    const struct gl_pixelstore_attrib *pack,
                    GLvoid *pixels);

void brw_drawpixels(struct gl_context *ctx,
                    GLint x, GLint y,
                    GLsizei width, GLsizei height,
                    GLenum format,
                    GLenum type,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLvoid *pixels);

void brw_copypixels(struct gl_context *ctx,
                    GLint srcx, GLint srcy,
                    GLsizei width, GLsizei height,
                    GLint destx, GLint desty, GLenum type);

void brw_bitmap(struct gl_context *ctx,
                GLint x, GLint y,
                GLsizei width, GLsizei height,
                const struct gl_pixelstore_attrib *unpack,
                const GLubyte *pixels);

#endif
