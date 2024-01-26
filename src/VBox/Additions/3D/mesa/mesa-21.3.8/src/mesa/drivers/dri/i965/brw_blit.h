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

#ifndef BRW_BLIT_H
#define BRW_BLIT_H

#include "brw_context.h"

bool brw_miptree_blit_compatible_formats(mesa_format src, mesa_format dst);

bool brw_miptree_blit(struct brw_context *brw,
                      struct brw_mipmap_tree *src_mt,
                      int src_level, int src_slice,
                      uint32_t src_x, uint32_t src_y, bool src_flip,
                      struct brw_mipmap_tree *dst_mt,
                      int dst_level, int dst_slice,
                      uint32_t dst_x, uint32_t dst_y, bool dst_flip,
                      uint32_t width, uint32_t height,
                      enum gl_logicop_mode logicop);

bool brw_miptree_copy(struct brw_context *brw,
                      struct brw_mipmap_tree *src_mt,
                      int src_level, int src_slice,
                      uint32_t src_x, uint32_t src_y,
                      struct brw_mipmap_tree *dst_mt,
                      int dst_level, int dst_slice,
                      uint32_t dst_x, uint32_t dst_y,
                      uint32_t src_width, uint32_t src_height);

bool
brw_emit_immediate_color_expand_blit(struct brw_context *brw,
                                     GLuint cpp,
                                     GLubyte *src_bits, GLuint src_size,
                                     GLuint fg_color,
                                     GLshort dst_pitch,
                                     struct brw_bo *dst_buffer,
                                     GLuint dst_offset,
                                     enum isl_tiling dst_tiling,
                                     GLshort x, GLshort y,
                                     GLshort w, GLshort h,
                                     enum gl_logicop_mode logic_op);

#endif
