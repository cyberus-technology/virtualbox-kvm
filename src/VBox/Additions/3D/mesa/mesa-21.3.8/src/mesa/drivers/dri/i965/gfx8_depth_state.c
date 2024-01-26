/*
 * Copyright © 2011 Intel Corporation
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

#include "brw_batch.h"
#include "brw_mipmap_tree.h"
#include "brw_fbo.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "compiler/brw_eu_defines.h"
#include "brw_wm.h"
#include "main/framebuffer.h"

/**
 * Should we set the PMA FIX ENABLE bit?
 *
 * To avoid unnecessary depth related stalls, we need to set this bit.
 * However, there is a very complicated formula which governs when it
 * is legal to do so.  This function computes that.
 *
 * See the documenation for the CACHE_MODE_1 register, bit 11.
 */
static bool
pma_fix_enable(const struct brw_context *brw)
{
   const struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   /* _NEW_BUFFERS */
   struct brw_renderbuffer *depth_irb =
      brw_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);

   /* 3DSTATE_WM::ForceThreadDispatch is never used. */
   const bool wm_force_thread_dispatch = false;

   /* 3DSTATE_RASTER::ForceSampleCount is never used. */
   const bool raster_force_sample_count_nonzero = false;

   /* _NEW_BUFFERS:
    * 3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL &&
    * 3DSTATE_DEPTH_BUFFER::HIZ Enable
    */
   const bool hiz_enabled = depth_irb && brw_renderbuffer_has_hiz(depth_irb);

   /* 3DSTATE_WM::Early Depth/Stencil Control != EDSC_PREPS (2). */
   const bool edsc_not_preps = !wm_prog_data->early_fragment_tests;

   /* 3DSTATE_PS_EXTRA::PixelShaderValid is always true. */
   const bool pixel_shader_valid = true;

   /* !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
    *   3DSTATE_WM_HZ_OP::DepthBufferResolve ||
    *   3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
    *   3DSTATE_WM_HZ_OP::StencilBufferClear)
    *
    * HiZ operations are done outside of the normal state upload, so they're
    * definitely not happening now.
    */
   const bool in_hiz_op = false;

   /* _NEW_DEPTH:
    * DEPTH_STENCIL_STATE::DepthTestEnable
    */
   const bool depth_test_enabled = depth_irb && ctx->Depth.Test;

   /* _NEW_DEPTH:
    * 3DSTATE_WM_DEPTH_STENCIL::DepthWriteEnable &&
    * 3DSTATE_DEPTH_BUFFER::DEPTH_WRITE_ENABLE.
    */
   const bool depth_writes_enabled = brw_depth_writes_enabled(brw);

   /* _NEW_STENCIL:
    * !DEPTH_STENCIL_STATE::Stencil Buffer Write Enable ||
    * !3DSTATE_DEPTH_BUFFER::Stencil Buffer Enable ||
    * !3DSTATE_STENCIL_BUFFER::Stencil Buffer Enable
    */
   const bool stencil_writes_enabled = brw->stencil_write_enabled;

   /* 3DSTATE_PS_EXTRA::Pixel Shader Computed Depth Mode != PSCDEPTH_OFF */
   const bool ps_computes_depth =
      wm_prog_data->computed_depth_mode != BRW_PSCDEPTH_OFF;

   /* BRW_NEW_FS_PROG_DATA:     3DSTATE_PS_EXTRA::PixelShaderKillsPixels
    * BRW_NEW_FS_PROG_DATA:     3DSTATE_PS_EXTRA::oMask Present to RenderTarget
    * _NEW_MULTISAMPLE:         3DSTATE_PS_BLEND::AlphaToCoverageEnable
    * _NEW_COLOR:               3DSTATE_PS_BLEND::AlphaTestEnable
    * _NEW_BUFFERS:             3DSTATE_PS_BLEND::AlphaTestEnable
    *                           3DSTATE_PS_BLEND::AlphaToCoverageEnable
    *
    * 3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable is always false.
    * 3DSTATE_WM::ForceKillPix != ForceOff is always true.
    */
   const bool kill_pixel =
      wm_prog_data->uses_kill ||
      wm_prog_data->uses_omask ||
      _mesa_is_alpha_test_enabled(ctx) ||
      _mesa_is_alpha_to_coverage_enabled(ctx);

   /* The big formula in CACHE_MODE_1::NP PMA FIX ENABLE. */
   return !wm_force_thread_dispatch &&
          !raster_force_sample_count_nonzero &&
          hiz_enabled &&
          edsc_not_preps &&
          pixel_shader_valid &&
          !in_hiz_op &&
          depth_test_enabled &&
          (ps_computes_depth ||
           (kill_pixel && (depth_writes_enabled || stencil_writes_enabled)));
}

void
gfx8_write_pma_stall_bits(struct brw_context *brw, uint32_t pma_stall_bits)
{
   /* If we haven't actually changed the value, bail now to avoid unnecessary
    * pipeline stalls and register writes.
    */
   if (brw->pma_stall_bits == pma_stall_bits)
      return;

   brw->pma_stall_bits = pma_stall_bits;

   /* According to the PIPE_CONTROL documentation, software should emit a
    * PIPE_CONTROL with the CS Stall and Depth Cache Flush bits set prior
    * to the LRI.  If stencil buffer writes are enabled, then a Render Cache
    * Flush is also necessary.
    */
   const uint32_t render_cache_flush =
      brw->stencil_write_enabled ? PIPE_CONTROL_RENDER_TARGET_FLUSH : 0;
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_CS_STALL |
                               PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                               render_cache_flush);

   /* CACHE_MODE_1 is a non-privileged register. */
   brw_load_register_imm32(brw, GFX7_CACHE_MODE_1,
                           GFX8_HIZ_PMA_MASK_BITS |
                           pma_stall_bits );

   /* After the LRI, a PIPE_CONTROL with both the Depth Stall and Depth Cache
    * Flush bits is often necessary.  We do it regardless because it's easier.
    * The render cache flush is also necessary if stencil writes are enabled.
    */
   brw_emit_pipe_control_flush(brw,
                               PIPE_CONTROL_DEPTH_STALL |
                               PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                               render_cache_flush);

}

static void
gfx8_emit_pma_stall_workaround(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t bits = 0;

   if (devinfo->ver >= 9)
      return;

   if (pma_fix_enable(brw))
      bits |= GFX8_HIZ_NP_PMA_FIX_ENABLE | GFX8_HIZ_NP_EARLY_Z_FAILS_DISABLE;

   gfx8_write_pma_stall_bits(brw, bits);
}

const struct brw_tracked_state gfx8_pma_fix = {
   .dirty = {
      .mesa = _NEW_BUFFERS |
              _NEW_COLOR |
              _NEW_DEPTH |
              _NEW_MULTISAMPLE |
              _NEW_STENCIL,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_FS_PROG_DATA,
   },
   .emit = gfx8_emit_pma_stall_workaround
};
