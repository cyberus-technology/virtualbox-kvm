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

#include "genX_boilerplate.h"
#include "brw_defines.h"
#include "brw_state.h"

static unsigned
flags_to_post_sync_op(uint32_t flags)
{
   if (flags & PIPE_CONTROL_WRITE_IMMEDIATE)
      return WriteImmediateData;

   if (flags & PIPE_CONTROL_WRITE_DEPTH_COUNT)
      return WritePSDepthCount;

   if (flags & PIPE_CONTROL_WRITE_TIMESTAMP)
      return WriteTimestamp;

   return 0;
}

/**
 * Do the given flags have a Post Sync or LRI Post Sync operation?
 */
static enum pipe_control_flags
get_post_sync_flags(enum pipe_control_flags flags)
{
   flags &= PIPE_CONTROL_WRITE_IMMEDIATE |
            PIPE_CONTROL_WRITE_DEPTH_COUNT |
            PIPE_CONTROL_WRITE_TIMESTAMP |
            PIPE_CONTROL_LRI_POST_SYNC_OP;

   /* Only one "Post Sync Op" is allowed, and it's mutually exclusive with
    * "LRI Post Sync Operation".  So more than one bit set would be illegal.
    */
   assert(util_bitcount(flags) <= 1);

   return flags;
}

#define IS_COMPUTE_PIPELINE(brw) \
   (GFX_VER >= 7 && brw->last_pipeline == BRW_COMPUTE_PIPELINE)

/* Closed interval - GFX_VER \in [x, y] */
#define IS_GFX_VER_BETWEEN(x, y) (GFX_VER >= x && GFX_VER <= y)
#define IS_GFX_VERx10_BETWEEN(x, y) \
   (GFX_VERx10 >= x && GFX_VERx10 <= y)

/**
 * Emit a series of PIPE_CONTROL commands, taking into account any
 * workarounds necessary to actually accomplish the caller's request.
 *
 * Unless otherwise noted, spec quotations in this function come from:
 *
 * Synchronization of the 3D Pipeline > PIPE_CONTROL Command > Programming
 * Restrictions for PIPE_CONTROL.
 *
 * You should not use this function directly.  Use the helpers in
 * brw_pipe_control.c instead, which may split the pipe control further.
 */
void
genX(emit_raw_pipe_control)(struct brw_context *brw, uint32_t flags,
                            struct brw_bo *bo, uint32_t offset, uint64_t imm)
{
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;
   enum pipe_control_flags post_sync_flags = get_post_sync_flags(flags);
   enum pipe_control_flags non_lri_post_sync_flags =
      post_sync_flags & ~PIPE_CONTROL_LRI_POST_SYNC_OP;

   /* Recursive PIPE_CONTROL workarounds --------------------------------
    * (http://knowyourmeme.com/memes/xzibit-yo-dawg)
    *
    * We do these first because we want to look at the original operation,
    * rather than any workarounds we set.
    */
   if (GFX_VER == 6 && (flags & PIPE_CONTROL_RENDER_TARGET_FLUSH)) {
      /* Hardware workaround: SNB B-Spec says:
       *
       *    "[Dev-SNB{W/A}]: Before a PIPE_CONTROL with Write Cache Flush
       *     Enable = 1, a PIPE_CONTROL with any non-zero post-sync-op is
       *     required."
       */
      brw_emit_post_sync_nonzero_flush(brw);
   }

   if (GFX_VER == 9 && (flags & PIPE_CONTROL_VF_CACHE_INVALIDATE)) {
      /* The PIPE_CONTROL "VF Cache Invalidation Enable" bit description
       * lists several workarounds:
       *
       *    "Project: SKL, KBL, BXT
       *
       *     If the VF Cache Invalidation Enable is set to a 1 in a
       *     PIPE_CONTROL, a separate Null PIPE_CONTROL, all bitfields
       *     sets to 0, with the VF Cache Invalidation Enable set to 0
       *     needs to be sent prior to the PIPE_CONTROL with VF Cache
       *     Invalidation Enable set to a 1."
       */
      genX(emit_raw_pipe_control)(brw, 0, NULL, 0, 0);
   }

   if (GFX_VER == 9 && IS_COMPUTE_PIPELINE(brw) && post_sync_flags) {
      /* Project: SKL / Argument: LRI Post Sync Operation [23]
       *
       * "PIPECONTROL command with “Command Streamer Stall Enable” must be
       *  programmed prior to programming a PIPECONTROL command with "LRI
       *  Post Sync Operation" in GPGPU mode of operation (i.e when
       *  PIPELINE_SELECT command is set to GPGPU mode of operation)."
       *
       * The same text exists a few rows below for Post Sync Op.
       */
      genX(emit_raw_pipe_control)(brw, PIPE_CONTROL_CS_STALL, NULL, 0, 0);
   }

   /* "Flush Types" workarounds ---------------------------------------------
    * We do these now because they may add post-sync operations or CS stalls.
    */

   if (IS_GFX_VER_BETWEEN(8, 10) &&
       (flags & PIPE_CONTROL_VF_CACHE_INVALIDATE)) {
      /* Project: BDW, SKL+ (stopping at CNL) / Argument: VF Invalidate
       *
       * "'Post Sync Operation' must be enabled to 'Write Immediate Data' or
       *  'Write PS Depth Count' or 'Write Timestamp'."
       */
      if (!bo) {
         flags |= PIPE_CONTROL_WRITE_IMMEDIATE;
         post_sync_flags |= PIPE_CONTROL_WRITE_IMMEDIATE;
         non_lri_post_sync_flags |= PIPE_CONTROL_WRITE_IMMEDIATE;
         bo = brw->workaround_bo;
         offset = brw->workaround_bo_offset;
      }
   }

   if (GFX_VERx10 < 75 && (flags & PIPE_CONTROL_DEPTH_STALL)) {
      /* Project: PRE-HSW / Argument: Depth Stall
       *
       * "The following bits must be clear:
       *  - Render Target Cache Flush Enable ([12] of DW1)
       *  - Depth Cache Flush Enable ([0] of DW1)"
       */
      assert(!(flags & (PIPE_CONTROL_RENDER_TARGET_FLUSH |
                        PIPE_CONTROL_DEPTH_CACHE_FLUSH)));
   }

   if (GFX_VER >= 6 && (flags & PIPE_CONTROL_DEPTH_STALL)) {
      /* From the PIPE_CONTROL instruction table, bit 13 (Depth Stall Enable):
       *
       *    "This bit must be DISABLED for operations other than writing
       *     PS_DEPTH_COUNT."
       *
       * This seems like nonsense.  An Ivybridge workaround requires us to
       * emit a PIPE_CONTROL with a depth stall and write immediate post-sync
       * operation.  Gfx8+ requires us to emit depth stalls and depth cache
       * flushes together.  So, it's hard to imagine this means anything other
       * than "we originally intended this to be used for PS_DEPTH_COUNT".
       *
       * We ignore the supposed restriction and do nothing.
       */
   }

   if (GFX_VERx10 < 75 && (flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH)) {
      /* Project: PRE-HSW / Argument: Depth Cache Flush
       *
       * "Depth Stall must be clear ([13] of DW1)."
       */
      assert(!(flags & PIPE_CONTROL_DEPTH_STALL));
   }

   if (flags & (PIPE_CONTROL_RENDER_TARGET_FLUSH |
                PIPE_CONTROL_STALL_AT_SCOREBOARD)) {
      /* From the PIPE_CONTROL instruction table, bit 12 and bit 1:
       *
       *    "This bit must be DISABLED for End-of-pipe (Read) fences,
       *     PS_DEPTH_COUNT or TIMESTAMP queries."
       *
       * TODO: Implement end-of-pipe checking.
       */
      assert(!(post_sync_flags & (PIPE_CONTROL_WRITE_DEPTH_COUNT |
                                  PIPE_CONTROL_WRITE_TIMESTAMP)));
   }

   if (GFX_VER < 11 && (flags & PIPE_CONTROL_STALL_AT_SCOREBOARD)) {
      /* From the PIPE_CONTROL instruction table, bit 1:
       *
       *    "This bit is ignored if Depth Stall Enable is set.
       *     Further, the render cache is not flushed even if Write Cache
       *     Flush Enable bit is set."
       *
       * We assert that the caller doesn't do this combination, to try and
       * prevent mistakes.  It shouldn't hurt the GPU, though.
       *
       * We skip this check on Gfx11+ as the "Stall and Pixel Scoreboard"
       * and "Render Target Flush" combo is explicitly required for BTI
       * update workarounds.
       */
      assert(!(flags & (PIPE_CONTROL_DEPTH_STALL |
                        PIPE_CONTROL_RENDER_TARGET_FLUSH)));
   }

   /* PIPE_CONTROL page workarounds ------------------------------------- */

   if (IS_GFX_VER_BETWEEN(7, 8) &&
       (flags & PIPE_CONTROL_STATE_CACHE_INVALIDATE)) {
      /* From the PIPE_CONTROL page itself:
       *
       *    "IVB, HSW, BDW
       *     Restriction: Pipe_control with CS-stall bit set must be issued
       *     before a pipe-control command that has the State Cache
       *     Invalidate bit set."
       */
      flags |= PIPE_CONTROL_CS_STALL;
   }

   if (GFX_VERx10 == 75) {
      /* From the PIPE_CONTROL page itself:
       *
       *    "HSW - Programming Note: PIPECONTROL with RO Cache Invalidation:
       *     Prior to programming a PIPECONTROL command with any of the RO
       *     cache invalidation bit set, program a PIPECONTROL flush command
       *     with “CS stall” bit and “HDC Flush” bit set."
       *
       * TODO: Actually implement this.  What's an HDC Flush?
       */
   }

   if (flags & PIPE_CONTROL_FLUSH_LLC) {
      /* From the PIPE_CONTROL instruction table, bit 26 (Flush LLC):
       *
       *    "Project: ALL
       *     SW must always program Post-Sync Operation to "Write Immediate
       *     Data" when Flush LLC is set."
       *
       * For now, we just require the caller to do it.
       */
      assert(flags & PIPE_CONTROL_WRITE_IMMEDIATE);
   }

   /* "Post-Sync Operation" workarounds -------------------------------- */

   /* Project: All / Argument: Global Snapshot Count Reset [19]
    *
    * "This bit must not be exercised on any product.
    *  Requires stall bit ([20] of DW1) set."
    *
    * We don't use this, so we just assert that it isn't used.  The
    * PIPE_CONTROL instruction page indicates that they intended this
    * as a debug feature and don't think it is useful in production,
    * but it may actually be usable, should we ever want to.
    */
   assert((flags & PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET) == 0);

   if (flags & (PIPE_CONTROL_MEDIA_STATE_CLEAR |
                PIPE_CONTROL_INDIRECT_STATE_POINTERS_DISABLE)) {
      /* Project: All / Arguments:
       *
       * - Generic Media State Clear [16]
       * - Indirect State Pointers Disable [16]
       *
       *    "Requires stall bit ([20] of DW1) set."
       *
       * Also, the PIPE_CONTROL instruction table, bit 16 (Generic Media
       * State Clear) says:
       *
       *    "PIPECONTROL command with “Command Streamer Stall Enable” must be
       *     programmed prior to programming a PIPECONTROL command with "Media
       *     State Clear" set in GPGPU mode of operation"
       *
       * This is a subset of the earlier rule, so there's nothing to do.
       */
      flags |= PIPE_CONTROL_CS_STALL;
   }

   if (flags & PIPE_CONTROL_STORE_DATA_INDEX) {
      /* Project: All / Argument: Store Data Index
       *
       * "Post-Sync Operation ([15:14] of DW1) must be set to something other
       *  than '0'."
       *
       * For now, we just assert that the caller does this.  We might want to
       * automatically add a write to the workaround BO...
       */
      assert(non_lri_post_sync_flags != 0);
   }

   if (flags & PIPE_CONTROL_SYNC_GFDT) {
      /* Project: All / Argument: Sync GFDT
       *
       * "Post-Sync Operation ([15:14] of DW1) must be set to something other
       *  than '0' or 0x2520[13] must be set."
       *
       * For now, we just assert that the caller does this.
       */
      assert(non_lri_post_sync_flags != 0);
   }

   if (IS_GFX_VERx10_BETWEEN(60, 75) &&
       (flags & PIPE_CONTROL_TLB_INVALIDATE)) {
      /* Project: SNB, IVB, HSW / Argument: TLB inv
       *
       * "{All SKUs}{All Steppings}: Post-Sync Operation ([15:14] of DW1)
       *  must be set to something other than '0'."
       *
       * For now, we just assert that the caller does this.
       */
      assert(non_lri_post_sync_flags != 0);
   }

   if (GFX_VER >= 7 && (flags & PIPE_CONTROL_TLB_INVALIDATE)) {
      /* Project: IVB+ / Argument: TLB inv
       *
       *    "Requires stall bit ([20] of DW1) set."
       *
       * Also, from the PIPE_CONTROL instruction table:
       *
       *    "Project: SKL+
       *     Post Sync Operation or CS stall must be set to ensure a TLB
       *     invalidation occurs.  Otherwise no cycle will occur to the TLB
       *     cache to invalidate."
       *
       * This is not a subset of the earlier rule, so there's nothing to do.
       */
      flags |= PIPE_CONTROL_CS_STALL;
   }

   if (GFX_VER == 9 && devinfo->gt == 4) {
      /* TODO: The big Skylake GT4 post sync op workaround */
   }

   /* "GPGPU specific workarounds" (both post-sync and flush) ------------ */

   if (IS_COMPUTE_PIPELINE(brw)) {
      if (GFX_VER >= 9 && (flags & PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE)) {
         /* Project: SKL+ / Argument: Tex Invalidate
          * "Requires stall bit ([20] of DW) set for all GPGPU Workloads."
          */
         flags |= PIPE_CONTROL_CS_STALL;
      }

      if (GFX_VER == 8 && (post_sync_flags ||
                           (flags & (PIPE_CONTROL_NOTIFY_ENABLE |
                                     PIPE_CONTROL_DEPTH_STALL |
                                     PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                     PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                                     PIPE_CONTROL_DATA_CACHE_FLUSH)))) {
         /* Project: BDW / Arguments:
          *
          * - LRI Post Sync Operation   [23]
          * - Post Sync Op              [15:14]
          * - Notify En                 [8]
          * - Depth Stall               [13]
          * - Render Target Cache Flush [12]
          * - Depth Cache Flush         [0]
          * - DC Flush Enable           [5]
          *
          *    "Requires stall bit ([20] of DW) set for all GPGPU and Media
          *     Workloads."
          *
          * (The docs have separate table rows for each bit, with essentially
          * the same workaround text.  We've combined them here.)
          */
         flags |= PIPE_CONTROL_CS_STALL;

         /* Also, from the PIPE_CONTROL instruction table, bit 20:
          *
          *    "Project: BDW
          *     This bit must be always set when PIPE_CONTROL command is
          *     programmed by GPGPU and MEDIA workloads, except for the cases
          *     when only Read Only Cache Invalidation bits are set (State
          *     Cache Invalidation Enable, Instruction cache Invalidation
          *     Enable, Texture Cache Invalidation Enable, Constant Cache
          *     Invalidation Enable). This is to WA FFDOP CG issue, this WA
          *     need not implemented when FF_DOP_CG is disable via "Fixed
          *     Function DOP Clock Gate Disable" bit in RC_PSMI_CTRL register."
          *
          * It sounds like we could avoid CS stalls in some cases, but we
          * don't currently bother.  This list isn't exactly the list above,
          * either...
          */
      }
   }

   /* Implement the WaCsStallAtEveryFourthPipecontrol workaround on IVB, BYT:
    *
    * "Every 4th PIPE_CONTROL command, not counting the PIPE_CONTROL with
    *  only read-cache-invalidate bit(s) set, must have a CS_STALL bit set."
    *
    * Note that the kernel does CS stalls between batches, so we only need
    * to count them within a batch.  We currently naively count every 4, and
    * don't skip the ones with only read-cache-invalidate bits set.  This
    * may or may not be a problem...
    */
   if (GFX_VERx10 == 70) {
      if (flags & PIPE_CONTROL_CS_STALL) {
         /* If we're doing a CS stall, reset the counter and carry on. */
         brw->pipe_controls_since_last_cs_stall = 0;
      }

      /* If this is the fourth pipe control without a CS stall, do one now. */
      if (++brw->pipe_controls_since_last_cs_stall == 4) {
         brw->pipe_controls_since_last_cs_stall = 0;
         flags |= PIPE_CONTROL_CS_STALL;
      }
   }

   /* "Stall" workarounds ----------------------------------------------
    * These have to come after the earlier ones because we may have added
    * some additional CS stalls above.
    */

   if (GFX_VER < 9 && (flags & PIPE_CONTROL_CS_STALL)) {
      /* Project: PRE-SKL, VLV, CHV
       *
       * "[All Stepping][All SKUs]:
       *
       *  One of the following must also be set:
       *
       *  - Render Target Cache Flush Enable ([12] of DW1)
       *  - Depth Cache Flush Enable ([0] of DW1)
       *  - Stall at Pixel Scoreboard ([1] of DW1)
       *  - Depth Stall ([13] of DW1)
       *  - Post-Sync Operation ([13] of DW1)
       *  - DC Flush Enable ([5] of DW1)"
       *
       * If we don't already have one of those bits set, we choose to add
       * "Stall at Pixel Scoreboard".  Some of the other bits require a
       * CS stall as a workaround (see above), which would send us into
       * an infinite recursion of PIPE_CONTROLs.  "Stall at Pixel Scoreboard"
       * appears to be safe, so we choose that.
       */
      const uint32_t wa_bits = PIPE_CONTROL_RENDER_TARGET_FLUSH |
                               PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                               PIPE_CONTROL_WRITE_IMMEDIATE |
                               PIPE_CONTROL_WRITE_DEPTH_COUNT |
                               PIPE_CONTROL_WRITE_TIMESTAMP |
                               PIPE_CONTROL_STALL_AT_SCOREBOARD |
                               PIPE_CONTROL_DEPTH_STALL |
                               PIPE_CONTROL_DATA_CACHE_FLUSH;
      if (!(flags & wa_bits))
         flags |= PIPE_CONTROL_STALL_AT_SCOREBOARD;
   }

   /* Emit --------------------------------------------------------------- */

   brw_batch_emit(brw, GENX(PIPE_CONTROL), pc) {
   #if GFX_VER >= 9
      pc.FlushLLC = 0;
   #endif
   #if GFX_VER >= 7
      pc.LRIPostSyncOperation = NoLRIOperation;
      pc.PipeControlFlushEnable = flags & PIPE_CONTROL_FLUSH_ENABLE;
      pc.DCFlushEnable = flags & PIPE_CONTROL_DATA_CACHE_FLUSH;
   #endif
   #if GFX_VER >= 6
      pc.StoreDataIndex = 0;
      pc.CommandStreamerStallEnable = flags & PIPE_CONTROL_CS_STALL;
      pc.GlobalSnapshotCountReset =
         flags & PIPE_CONTROL_GLOBAL_SNAPSHOT_COUNT_RESET;
      pc.TLBInvalidate = flags & PIPE_CONTROL_TLB_INVALIDATE;
      pc.GenericMediaStateClear = flags & PIPE_CONTROL_MEDIA_STATE_CLEAR;
      pc.StallAtPixelScoreboard = flags & PIPE_CONTROL_STALL_AT_SCOREBOARD;
      pc.RenderTargetCacheFlushEnable =
         flags & PIPE_CONTROL_RENDER_TARGET_FLUSH;
      pc.DepthCacheFlushEnable = flags & PIPE_CONTROL_DEPTH_CACHE_FLUSH;
      pc.StateCacheInvalidationEnable =
         flags & PIPE_CONTROL_STATE_CACHE_INVALIDATE;
      pc.VFCacheInvalidationEnable = flags & PIPE_CONTROL_VF_CACHE_INVALIDATE;
      pc.ConstantCacheInvalidationEnable =
         flags & PIPE_CONTROL_CONST_CACHE_INVALIDATE;
   #else
      pc.WriteCacheFlush = flags & PIPE_CONTROL_RENDER_TARGET_FLUSH;
   #endif
      pc.PostSyncOperation = flags_to_post_sync_op(flags);
      pc.DepthStallEnable = flags & PIPE_CONTROL_DEPTH_STALL;
      pc.InstructionCacheInvalidateEnable =
         flags & PIPE_CONTROL_INSTRUCTION_INVALIDATE;
      pc.NotifyEnable = flags & PIPE_CONTROL_NOTIFY_ENABLE;
   #if GFX_VERx10 >= 45
      pc.IndirectStatePointersDisable =
         flags & PIPE_CONTROL_INDIRECT_STATE_POINTERS_DISABLE;
   #endif
   #if GFX_VER >= 6
      pc.TextureCacheInvalidationEnable =
         flags & PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE;
   #elif GFX_VER == 5 || GFX_VERx10 == 45
      pc.TextureCacheFlushEnable =
         flags & PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE;
   #endif
      pc.Address = ggtt_bo(bo, offset);
      if (GFX_VER < 7 && bo)
         pc.DestinationAddressType = DAT_GGTT;
      pc.ImmediateData = imm;
   }
}
