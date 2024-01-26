/*
 * Copyright (c) 2015 Intel Corporation
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

#include "common/intel_l3_config.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_batch.h"

/**
 * Calculate the desired L3 partitioning based on the current state of the
 * pipeline.  For now this simply returns the conservative defaults calculated
 * by get_default_l3_weights(), but we could probably do better by gathering
 * more statistics from the pipeline state (e.g. guess of expected URB usage
 * and bound surfaces), or by using feed-back from performance counters.
 */
static struct intel_l3_weights
get_pipeline_state_l3_weights(const struct brw_context *brw)
{
   const struct brw_stage_state *stage_states[] = {
      [MESA_SHADER_VERTEX] = &brw->vs.base,
      [MESA_SHADER_TESS_CTRL] = &brw->tcs.base,
      [MESA_SHADER_TESS_EVAL] = &brw->tes.base,
      [MESA_SHADER_GEOMETRY] = &brw->gs.base,
      [MESA_SHADER_FRAGMENT] = &brw->wm.base,
      [MESA_SHADER_COMPUTE] = &brw->cs.base
   };
   bool needs_dc = false, needs_slm = false;

   for (unsigned i = 0; i < ARRAY_SIZE(stage_states); i++) {
      const struct gl_program *prog =
         brw->ctx._Shader->CurrentProgram[stage_states[i]->stage];
      const struct brw_stage_prog_data *prog_data = stage_states[i]->prog_data;

      needs_dc |= (prog && (prog->sh.data->NumAtomicBuffers ||
                            prog->sh.data->NumShaderStorageBlocks ||
                            prog->info.num_images)) ||
         (prog_data && prog_data->total_scratch);
      needs_slm |= prog_data && prog_data->total_shared;
   }

   return intel_get_default_l3_weights(&brw->screen->devinfo,
                                       needs_dc, needs_slm);
}

/**
 * Program the hardware to use the specified L3 configuration.
 */
static void
setup_l3_config(struct brw_context *brw, const struct intel_l3_config *cfg)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const bool has_dc = cfg->n[INTEL_L3P_DC] || cfg->n[INTEL_L3P_ALL];
   const bool has_is = cfg->n[INTEL_L3P_IS] || cfg->n[INTEL_L3P_RO] ||
                       cfg->n[INTEL_L3P_ALL];
   const bool has_c = cfg->n[INTEL_L3P_C] || cfg->n[INTEL_L3P_RO] ||
                      cfg->n[INTEL_L3P_ALL];
   const bool has_t = cfg->n[INTEL_L3P_T] || cfg->n[INTEL_L3P_RO] ||
                      cfg->n[INTEL_L3P_ALL];
   const bool has_slm = cfg->n[INTEL_L3P_SLM];

   /* According to the hardware docs, the L3 partitioning can only be changed
    * while the pipeline is completely drained and the caches are flushed,
    * which involves a first PIPE_CONTROL flush which stalls the pipeline...
    */
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_DATA_CACHE_FLUSH |
                               PIPE_CONTROL_CS_STALL);

   /* ...followed by a second pipelined PIPE_CONTROL that initiates
    * invalidation of the relevant caches.  Note that because RO invalidation
    * happens at the top of the pipeline (i.e. right away as the PIPE_CONTROL
    * command is processed by the CS) we cannot combine it with the previous
    * stalling flush as the hardware documentation suggests, because that
    * would cause the CS to stall on previous rendering *after* RO
    * invalidation and wouldn't prevent the RO caches from being polluted by
    * concurrent rendering before the stall completes.  This intentionally
    * doesn't implement the SKL+ hardware workaround suggesting to enable CS
    * stall on PIPE_CONTROLs with the texture cache invalidation bit set for
    * GPGPU workloads because the previous and subsequent PIPE_CONTROLs
    * already guarantee that there is no concurrent GPGPU kernel execution
    * (see SKL HSD 2132585).
    */
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
                               PIPE_CONTROL_CONST_CACHE_INVALIDATE |
                               PIPE_CONTROL_INSTRUCTION_INVALIDATE |
                               PIPE_CONTROL_STATE_CACHE_INVALIDATE);

   /* Now send a third stalling flush to make sure that invalidation is
    * complete when the L3 configuration registers are modified.
    */
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_DATA_CACHE_FLUSH |
                               PIPE_CONTROL_CS_STALL);

   if (devinfo->ver >= 8) {
      assert(!cfg->n[INTEL_L3P_IS] && !cfg->n[INTEL_L3P_C] && !cfg->n[INTEL_L3P_T]);

      const unsigned imm_data = (
         (devinfo->ver < 11 && has_slm ? GFX8_L3CNTLREG_SLM_ENABLE : 0) |
         (devinfo->ver == 11 ? GFX11_L3CNTLREG_USE_FULL_WAYS : 0) |
         SET_FIELD(cfg->n[INTEL_L3P_URB], GFX8_L3CNTLREG_URB_ALLOC) |
         SET_FIELD(cfg->n[INTEL_L3P_RO], GFX8_L3CNTLREG_RO_ALLOC) |
         SET_FIELD(cfg->n[INTEL_L3P_DC], GFX8_L3CNTLREG_DC_ALLOC) |
         SET_FIELD(cfg->n[INTEL_L3P_ALL], GFX8_L3CNTLREG_ALL_ALLOC));

      /* Set up the L3 partitioning. */
      brw_load_register_imm32(brw, GFX8_L3CNTLREG, imm_data);
   } else {
      assert(!cfg->n[INTEL_L3P_ALL]);

      /* When enabled SLM only uses a portion of the L3 on half of the banks,
       * the matching space on the remaining banks has to be allocated to a
       * client (URB for all validated configurations) set to the
       * lower-bandwidth 2-bank address hashing mode.
       */
      const bool urb_low_bw = has_slm && !devinfo->is_baytrail;
      assert(!urb_low_bw || cfg->n[INTEL_L3P_URB] == cfg->n[INTEL_L3P_SLM]);

      /* Minimum number of ways that can be allocated to the URB. */
      const unsigned n0_urb = (devinfo->is_baytrail ? 32 : 0);
      assert(cfg->n[INTEL_L3P_URB] >= n0_urb);

      BEGIN_BATCH(7);
      OUT_BATCH(MI_LOAD_REGISTER_IMM | (7 - 2));

      /* Demote any clients with no ways assigned to LLC. */
      OUT_BATCH(GFX7_L3SQCREG1);
      OUT_BATCH((devinfo->is_haswell ? HSW_L3SQCREG1_SQGHPCI_DEFAULT :
                 devinfo->is_baytrail ? VLV_L3SQCREG1_SQGHPCI_DEFAULT :
                 IVB_L3SQCREG1_SQGHPCI_DEFAULT) |
                (has_dc ? 0 : GFX7_L3SQCREG1_CONV_DC_UC) |
                (has_is ? 0 : GFX7_L3SQCREG1_CONV_IS_UC) |
                (has_c ? 0 : GFX7_L3SQCREG1_CONV_C_UC) |
                (has_t ? 0 : GFX7_L3SQCREG1_CONV_T_UC));

      /* Set up the L3 partitioning. */
      OUT_BATCH(GFX7_L3CNTLREG2);
      OUT_BATCH((has_slm ? GFX7_L3CNTLREG2_SLM_ENABLE : 0) |
                SET_FIELD(cfg->n[INTEL_L3P_URB] - n0_urb, GFX7_L3CNTLREG2_URB_ALLOC) |
                (urb_low_bw ? GFX7_L3CNTLREG2_URB_LOW_BW : 0) |
                SET_FIELD(cfg->n[INTEL_L3P_ALL], GFX7_L3CNTLREG2_ALL_ALLOC) |
                SET_FIELD(cfg->n[INTEL_L3P_RO], GFX7_L3CNTLREG2_RO_ALLOC) |
                SET_FIELD(cfg->n[INTEL_L3P_DC], GFX7_L3CNTLREG2_DC_ALLOC));
      OUT_BATCH(GFX7_L3CNTLREG3);
      OUT_BATCH(SET_FIELD(cfg->n[INTEL_L3P_IS], GFX7_L3CNTLREG3_IS_ALLOC) |
                SET_FIELD(cfg->n[INTEL_L3P_C], GFX7_L3CNTLREG3_C_ALLOC) |
                SET_FIELD(cfg->n[INTEL_L3P_T], GFX7_L3CNTLREG3_T_ALLOC));

      ADVANCE_BATCH();

      if (can_do_hsw_l3_atomics(brw->screen)) {
         /* Enable L3 atomics on HSW if we have a DC partition, otherwise keep
          * them disabled to avoid crashing the system hard.
          */
         BEGIN_BATCH(5);
         OUT_BATCH(MI_LOAD_REGISTER_IMM | (5 - 2));
         OUT_BATCH(HSW_SCRATCH1);
         OUT_BATCH(has_dc ? 0 : HSW_SCRATCH1_L3_ATOMIC_DISABLE);
         OUT_BATCH(HSW_ROW_CHICKEN3);
         OUT_BATCH(REG_MASK(HSW_ROW_CHICKEN3_L3_ATOMIC_DISABLE) |
                   (has_dc ? 0 : HSW_ROW_CHICKEN3_L3_ATOMIC_DISABLE));
         ADVANCE_BATCH();
      }
   }
}

/**
 * Update the URB size in the context state for the specified L3
 * configuration.
 */
static void
update_urb_size(struct brw_context *brw, const struct intel_l3_config *cfg)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const unsigned sz = intel_get_l3_config_urb_size(devinfo, cfg);

   if (brw->urb.size != sz) {
      brw->urb.size = sz;
      brw->ctx.NewDriverState |= BRW_NEW_URB_SIZE;

      /* If we change the total URB size, reset the individual stage sizes to
       * zero so that, even if there is no URB size change, gfx7_upload_urb
       * still re-emits 3DSTATE_URB_*.
       */
      brw->urb.vsize = 0;
      brw->urb.gsize = 0;
      brw->urb.hsize = 0;
      brw->urb.dsize = 0;
   }
}

void
brw_emit_l3_state(struct brw_context *brw)
{
   const struct intel_l3_weights w = get_pipeline_state_l3_weights(brw);
   const float dw = intel_diff_l3_weights(w, intel_get_l3_config_weights(brw->l3.config));
   /* The distance between any two compatible weight vectors cannot exceed two
    * due to the triangle inequality.
    */
   const float large_dw_threshold = 2.0;
   /* Somewhat arbitrary, simply makes sure that there will be no repeated
    * transitions to the same L3 configuration, could probably do better here.
    */
   const float small_dw_threshold = 0.5;
   /* If we're emitting a new batch the caches should already be clean and the
    * transition should be relatively cheap, so it shouldn't hurt much to use
    * the smaller threshold.  Otherwise use the larger threshold so that we
    * only reprogram the L3 mid-batch if the most recently programmed
    * configuration is incompatible with the current pipeline state.
    */
   const float dw_threshold = (brw->ctx.NewDriverState & BRW_NEW_BATCH ?
                               small_dw_threshold : large_dw_threshold);

   if (dw > dw_threshold && can_do_pipelined_register_writes(brw->screen)) {
      const struct intel_l3_config *const cfg =
         intel_get_l3_config(&brw->screen->devinfo, w);

      setup_l3_config(brw, cfg);
      update_urb_size(brw, cfg);
      brw->l3.config = cfg;

      if (INTEL_DEBUG(DEBUG_L3)) {
         fprintf(stderr, "L3 config transition (%f > %f): ", dw, dw_threshold);
         intel_dump_l3_config(cfg, stderr);
      }
   }
}

const struct brw_tracked_state gfx7_l3_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_CS_PROG_DATA |
             BRW_NEW_FS_PROG_DATA |
             BRW_NEW_GS_PROG_DATA |
             BRW_NEW_TCS_PROG_DATA |
             BRW_NEW_TES_PROG_DATA |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_emit_l3_state
};

/**
 * Hack to restore the default L3 configuration.
 *
 * This will be called at the end of every batch in order to reset the L3
 * configuration to the default values for the time being until the kernel is
 * fixed.  Until kernel commit 6702cf16e0ba8b0129f5aa1b6609d4e9c70bc13b
 * (included in v4.1) we would set the MI_RESTORE_INHIBIT bit when submitting
 * batch buffers for the default context used by the DDX, which meant that any
 * context state changed by the GL would leak into the DDX, the assumption
 * being that the DDX would initialize any state it cares about manually.  The
 * DDX is however not careful enough to program an L3 configuration
 * explicitly, and it makes assumptions about it (URB size) which won't hold
 * and cause it to misrender if we let our L3 set-up to leak into the DDX.
 *
 * Since v4.1 of the Linux kernel the default context is saved and restored
 * normally, so it's far less likely for our L3 programming to interfere with
 * other contexts -- In fact restoring the default L3 configuration at the end
 * of the batch will be redundant most of the time.  A kind of state leak is
 * still possible though if the context making assumptions about L3 state is
 * created immediately after our context was active (e.g. without the DDX
 * default context being scheduled in between) because at present the DRM
 * doesn't fully initialize the contents of newly created contexts and instead
 * sets the MI_RESTORE_INHIBIT flag causing it to inherit the state from the
 * last active context.
 *
 * It's possible to realize such a scenario if, say, an X server (or a GL
 * application using an outdated non-L3-aware Mesa version) is started while
 * another GL application is running and happens to have modified the L3
 * configuration, or if no X server is running at all and a GL application
 * using a non-L3-aware Mesa version is started after another GL application
 * ran and modified the L3 configuration -- The latter situation can actually
 * be reproduced easily on IVB in our CI system.
 */
void
gfx7_restore_default_l3_config(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct intel_l3_config *const cfg = intel_get_default_l3_config(devinfo);

   if (cfg != brw->l3.config &&
       can_do_pipelined_register_writes(brw->screen)) {
      setup_l3_config(brw, cfg);
      update_urb_size(brw, cfg);
      brw->l3.config = cfg;
   }
}
