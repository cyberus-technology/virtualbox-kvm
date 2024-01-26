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

#ifndef BRW_BLORP_H
#define BRW_BLORP_H

#include "blorp/blorp.h"
#include "brw_mipmap_tree.h"
#include "program/prog_instruction.h"

#ifdef __cplusplus
extern "C" {
#endif

void brw_blorp_init(struct brw_context *brw);

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
                        GLenum filter, bool mirror_x, bool mirror_y,
                        bool decode_srgb, bool encode_srgb);

void
brw_blorp_copy_miptrees(struct brw_context *brw,
                        struct brw_mipmap_tree *src_mt,
                        unsigned src_level, unsigned src_logical_layer,
                        struct brw_mipmap_tree *dst_mt,
                        unsigned dst_level, unsigned dst_logical_layer,
                        unsigned src_x, unsigned src_y,
                        unsigned dst_x, unsigned dst_y,
                        unsigned src_width, unsigned src_height);

void
brw_blorp_copy_buffers(struct brw_context *brw,
                       struct brw_bo *src_bo,
                       unsigned src_offset,
                       struct brw_bo *dst_bo,
                       unsigned dst_offset,
                       unsigned size);

bool
brw_blorp_upload_miptree(struct brw_context *brw,
                         struct brw_mipmap_tree *dst_mt,
                         mesa_format dst_format,
                         uint32_t level, uint32_t x, uint32_t y, uint32_t z,
                         uint32_t width, uint32_t height, uint32_t depth,
                         GLenum target, GLenum format, GLenum type,
                         const void *pixels,
                         const struct gl_pixelstore_attrib *packing);

bool
brw_blorp_download_miptree(struct brw_context *brw,
                           struct brw_mipmap_tree *src_mt,
                           mesa_format src_format, uint32_t src_swizzle,
                           uint32_t level, uint32_t x, uint32_t y, uint32_t z,
                           uint32_t width, uint32_t height, uint32_t depth,
                           GLenum target, GLenum format, GLenum type,
                           bool y_flip, const void *pixels,
                           const struct gl_pixelstore_attrib *packing);

void
brw_blorp_clear_color(struct brw_context *brw, struct gl_framebuffer *fb,
                      GLbitfield mask, bool partial_clear, bool encode_srgb);
void
brw_blorp_clear_depth_stencil(struct brw_context *brw,
                              struct gl_framebuffer *fb,
                              GLbitfield mask, bool partial_clear);

void
brw_blorp_resolve_color(struct brw_context *brw,
                        struct brw_mipmap_tree *mt,
                        unsigned level, unsigned layer,
                        enum isl_aux_op resolve_op);

void
brw_blorp_mcs_partial_resolve(struct brw_context *brw,
                              struct brw_mipmap_tree *mt,
                              uint32_t start_layer, uint32_t num_layers);

void
brw_hiz_exec(struct brw_context *brw, struct brw_mipmap_tree *mt,
             unsigned int level, unsigned int start_layer,
             unsigned int num_layers, enum isl_aux_op op);

void gfx4_blorp_exec(struct blorp_batch *batch,
                     const struct blorp_params *params);
void gfx45_blorp_exec(struct blorp_batch *batch,
                      const struct blorp_params *params);
void gfx5_blorp_exec(struct blorp_batch *batch,
                     const struct blorp_params *params);
void gfx6_blorp_exec(struct blorp_batch *batch,
                     const struct blorp_params *params);
void gfx7_blorp_exec(struct blorp_batch *batch,
                     const struct blorp_params *params);
void gfx75_blorp_exec(struct blorp_batch *batch,
                      const struct blorp_params *params);
void gfx8_blorp_exec(struct blorp_batch *batch,
                     const struct blorp_params *params);
void gfx9_blorp_exec(struct blorp_batch *batch,
                     const struct blorp_params *params);
void gfx11_blorp_exec(struct blorp_batch *batch,
                      const struct blorp_params *params);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BRW_BLORP_H */
