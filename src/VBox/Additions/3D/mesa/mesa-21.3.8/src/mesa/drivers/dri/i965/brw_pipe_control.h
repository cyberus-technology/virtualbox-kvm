/*
 * Copyright © 2017 Intel Corporation
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

#ifndef BRW_PIPE_CONTROL_DOT_H
#define BRW_PIPE_CONTROL_DOT_H

struct brw_context;
struct intel_device_info;
struct brw_bo;

/** @{
 *
 * PIPE_CONTROL operation, a combination MI_FLUSH and register write with
 * additional flushing control.
 *
 * The bits here are not the actual hardware values.  The actual values
 * shift around a bit per-generation, so we just have flags for each
 * potential operation, and use genxml to encode the actual packet.
 */
enum pipe_control_flags
{
   PIPE_CONTROL_FLUSH_LLC                       = (1 << 1),
   PIPE_CONTROL_LRI_POST_SYNC_OP                = (1 << 2),
   PIPE_CONTROL_STORE_DATA_INDEX                = (1 << 3),
   PIPE_CONTROL_CS_STALL                        = (1 << 4),
   PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET     = (1 << 5),
   PIPE_CONTROL_SYNC_GFDT                       = (1 << 6),
   PIPE_CONTROL_TLB_INVALIDATE                  = (1 << 7),
   PIPE_CONTROL_MEDIA_STATE_CLEAR               = (1 << 8),
   PIPE_CONTROL_WRITE_IMMEDIATE                 = (1 << 9),
   PIPE_CONTROL_WRITE_DEPTH_COUNT               = (1 << 10),
   PIPE_CONTROL_WRITE_TIMESTAMP                 = (1 << 11),
   PIPE_CONTROL_DEPTH_STALL                     = (1 << 12),
   PIPE_CONTROL_RENDER_TARGET_FLUSH             = (1 << 13),
   PIPE_CONTROL_INSTRUCTION_INVALIDATE          = (1 << 14),
   PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE        = (1 << 15),
   PIPE_CONTROL_INDIRECT_STATE_POINTERS_DISABLE = (1 << 16),
   PIPE_CONTROL_NOTIFY_ENABLE                   = (1 << 17),
   PIPE_CONTROL_FLUSH_ENABLE                    = (1 << 18),
   PIPE_CONTROL_DATA_CACHE_FLUSH                = (1 << 19),
   PIPE_CONTROL_VF_CACHE_INVALIDATE             = (1 << 20),
   PIPE_CONTROL_CONST_CACHE_INVALIDATE          = (1 << 21),
   PIPE_CONTROL_STATE_CACHE_INVALIDATE          = (1 << 22),
   PIPE_CONTROL_STALL_AT_SCOREBOARD             = (1 << 23),
   PIPE_CONTROL_DEPTH_CACHE_FLUSH               = (1 << 24),
};

#define PIPE_CONTROL_CACHE_FLUSH_BITS \
   (PIPE_CONTROL_DEPTH_CACHE_FLUSH | PIPE_CONTROL_DATA_CACHE_FLUSH | \
    PIPE_CONTROL_RENDER_TARGET_FLUSH)

#define PIPE_CONTROL_CACHE_INVALIDATE_BITS \
   (PIPE_CONTROL_STATE_CACHE_INVALIDATE | PIPE_CONTROL_CONST_CACHE_INVALIDATE | \
    PIPE_CONTROL_VF_CACHE_INVALIDATE | PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE | \
    PIPE_CONTROL_INSTRUCTION_INVALIDATE)

/** @} */

int brw_init_pipe_control(struct brw_context *brw,
                          const struct intel_device_info *info);
void brw_fini_pipe_control(struct brw_context *brw);

void brw_emit_pipe_control_flush(struct brw_context *brw, uint32_t flags);
void brw_emit_pipe_control_write(struct brw_context *brw, uint32_t flags,
                                 struct brw_bo *bo, uint32_t offset,
                                 uint64_t imm);
void brw_emit_end_of_pipe_sync(struct brw_context *brw, uint32_t flags);
void brw_emit_mi_flush(struct brw_context *brw);
void brw_emit_post_sync_nonzero_flush(struct brw_context *brw);
void brw_emit_depth_stall_flushes(struct brw_context *brw);
void gfx7_emit_vs_workaround_flush(struct brw_context *brw);
void gfx7_emit_cs_stall_flush(struct brw_context *brw);
void gfx7_emit_isp_disable(struct brw_context *brw);

#endif
