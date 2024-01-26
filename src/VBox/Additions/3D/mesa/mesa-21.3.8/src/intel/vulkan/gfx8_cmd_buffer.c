/*
 * Copyright © 2015 Intel Corporation
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

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "anv_private.h"

#include "genxml/gen_macros.h"
#include "genxml/genX_pack.h"
#include "common/intel_guardband.h"

#if GFX_VER == 8
void
gfx8_cmd_buffer_emit_viewport(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_framebuffer *fb = cmd_buffer->state.framebuffer;
   uint32_t count = cmd_buffer->state.gfx.dynamic.viewport.count;
   const VkViewport *viewports =
      cmd_buffer->state.gfx.dynamic.viewport.viewports;
   struct anv_state sf_clip_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, count * 64, 64);

   for (uint32_t i = 0; i < count; i++) {
      const VkViewport *vp = &viewports[i];

      /* The gfx7 state struct has just the matrix and guardband fields, the
       * gfx8 struct adds the min/max viewport fields. */
      struct GENX(SF_CLIP_VIEWPORT) sfv = {
         .ViewportMatrixElementm00 = vp->width / 2,
         .ViewportMatrixElementm11 = vp->height / 2,
         .ViewportMatrixElementm22 = vp->maxDepth - vp->minDepth,
         .ViewportMatrixElementm30 = vp->x + vp->width / 2,
         .ViewportMatrixElementm31 = vp->y + vp->height / 2,
         .ViewportMatrixElementm32 = vp->minDepth,
         .XMinClipGuardband = -1.0f,
         .XMaxClipGuardband = 1.0f,
         .YMinClipGuardband = -1.0f,
         .YMaxClipGuardband = 1.0f,
         .XMinViewPort = vp->x,
         .XMaxViewPort = vp->x + vp->width - 1,
         .YMinViewPort = MIN2(vp->y, vp->y + vp->height),
         .YMaxViewPort = MAX2(vp->y, vp->y + vp->height) - 1,
      };

      if (fb) {
         /* We can only calculate a "real" guardband clip if we know the
          * framebuffer at the time we emit the packet.  Otherwise, we have
          * fall back to a worst-case guardband of [-1, 1].
          */
         intel_calculate_guardband_size(fb->width, fb->height,
                                        sfv.ViewportMatrixElementm00,
                                        sfv.ViewportMatrixElementm11,
                                        sfv.ViewportMatrixElementm30,
                                        sfv.ViewportMatrixElementm31,
                                        &sfv.XMinClipGuardband,
                                        &sfv.XMaxClipGuardband,
                                        &sfv.YMinClipGuardband,
                                        &sfv.YMaxClipGuardband);
      }

      GENX(SF_CLIP_VIEWPORT_pack)(NULL, sf_clip_state.map + i * 64, &sfv);
   }

   anv_batch_emit(&cmd_buffer->batch,
                  GENX(3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP), clip) {
      clip.SFClipViewportPointer = sf_clip_state.offset;
   }
}

void
gfx8_cmd_buffer_emit_depth_viewport(struct anv_cmd_buffer *cmd_buffer,
                                    bool depth_clamp_enable)
{
   uint32_t count = cmd_buffer->state.gfx.dynamic.viewport.count;
   const VkViewport *viewports =
      cmd_buffer->state.gfx.dynamic.viewport.viewports;
   struct anv_state cc_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, count * 8, 32);

   for (uint32_t i = 0; i < count; i++) {
      const VkViewport *vp = &viewports[i];

      /* From the Vulkan spec:
       *
       *    "It is valid for minDepth to be greater than or equal to
       *    maxDepth."
       */
      float min_depth = MIN2(vp->minDepth, vp->maxDepth);
      float max_depth = MAX2(vp->minDepth, vp->maxDepth);

      struct GENX(CC_VIEWPORT) cc_viewport = {
         .MinimumDepth = depth_clamp_enable ? min_depth : 0.0f,
         .MaximumDepth = depth_clamp_enable ? max_depth : 1.0f,
      };

      GENX(CC_VIEWPORT_pack)(NULL, cc_state.map + i * 8, &cc_viewport);
   }

   anv_batch_emit(&cmd_buffer->batch,
                  GENX(3DSTATE_VIEWPORT_STATE_POINTERS_CC), cc) {
      cc.CCViewportPointer = cc_state.offset;
   }
}
#endif

void
genX(cmd_buffer_enable_pma_fix)(struct anv_cmd_buffer *cmd_buffer, bool enable)
{
   if (cmd_buffer->state.pma_fix_enabled == enable)
      return;

   cmd_buffer->state.pma_fix_enabled = enable;

   /* According to the Broadwell PIPE_CONTROL documentation, software should
    * emit a PIPE_CONTROL with the CS Stall and Depth Cache Flush bits set
    * prior to the LRI.  If stencil buffer writes are enabled, then a Render
    * Cache Flush is also necessary.
    *
    * The Skylake docs say to use a depth stall rather than a command
    * streamer stall.  However, the hardware seems to violently disagree.
    * A full command streamer stall seems to be needed in both cases.
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DepthCacheFlushEnable = true;
      pc.CommandStreamerStallEnable = true;
      pc.RenderTargetCacheFlushEnable = true;
#if GFX_VER >= 12
      pc.TileCacheFlushEnable = true;

      /* Wa_1409600907: "PIPE_CONTROL with Depth Stall Enable bit must
       * be set with any PIPE_CONTROL with Depth Flush Enable bit set.
       */
      pc.DepthStallEnable = true;
#endif
   }

#if GFX_VER == 9

   uint32_t cache_mode;
   anv_pack_struct(&cache_mode, GENX(CACHE_MODE_0),
                   .STCPMAOptimizationEnable = enable,
                   .STCPMAOptimizationEnableMask = true);
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset   = GENX(CACHE_MODE_0_num);
      lri.DataDWord        = cache_mode;
   }

#elif GFX_VER == 8

   uint32_t cache_mode;
   anv_pack_struct(&cache_mode, GENX(CACHE_MODE_1),
                   .NPPMAFixEnable = enable,
                   .NPEarlyZFailsDisable = enable,
                   .NPPMAFixEnableMask = true,
                   .NPEarlyZFailsDisableMask = true);
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset   = GENX(CACHE_MODE_1_num);
      lri.DataDWord        = cache_mode;
   }

#endif /* GFX_VER == 8 */

   /* After the LRI, a PIPE_CONTROL with both the Depth Stall and Depth Cache
    * Flush bits is often necessary.  We do it regardless because it's easier.
    * The render cache flush is also necessary if stencil writes are enabled.
    *
    * Again, the Skylake docs give a different set of flushes but the BDW
    * flushes seem to work just as well.
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DepthStallEnable = true;
      pc.DepthCacheFlushEnable = true;
      pc.RenderTargetCacheFlushEnable = true;
#if GFX_VER >= 12
      pc.TileCacheFlushEnable = true;
#endif
   }
}

UNUSED static bool
want_depth_pma_fix(struct anv_cmd_buffer *cmd_buffer)
{
   assert(GFX_VER == 8);

   /* From the Broadwell PRM Vol. 2c CACHE_MODE_1::NP_PMA_FIX_ENABLE:
    *
    *    SW must set this bit in order to enable this fix when following
    *    expression is TRUE.
    *
    *    3DSTATE_WM::ForceThreadDispatch != 1 &&
    *    !(3DSTATE_RASTER::ForceSampleCount != NUMRASTSAMPLES_0) &&
    *    (3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL) &&
    *    (3DSTATE_DEPTH_BUFFER::HIZ Enable) &&
    *    !(3DSTATE_WM::EDSC_Mode == EDSC_PREPS) &&
    *    (3DSTATE_PS_EXTRA::PixelShaderValid) &&
    *    !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
    *      3DSTATE_WM_HZ_OP::DepthBufferResolve ||
    *      3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
    *      3DSTATE_WM_HZ_OP::StencilBufferClear) &&
    *    (3DSTATE_WM_DEPTH_STENCIL::DepthTestEnable) &&
    *    (((3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
    *       3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
    *       3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
    *       3DSTATE_PS_BLEND::AlphaTestEnable ||
    *       3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable) &&
    *      3DSTATE_WM::ForceKillPix != ForceOff &&
    *      ((3DSTATE_WM_DEPTH_STENCIL::DepthWriteEnable &&
    *        3DSTATE_DEPTH_BUFFER::DEPTH_WRITE_ENABLE) ||
    *       (3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
    *        3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE &&
    *        3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE))) ||
    *     (3DSTATE_PS_EXTRA:: Pixel Shader Computed Depth mode != PSCDEPTH_OFF))
    */

   /* These are always true:
    *    3DSTATE_WM::ForceThreadDispatch != 1 &&
    *    !(3DSTATE_RASTER::ForceSampleCount != NUMRASTSAMPLES_0)
    */

   /* We only enable the PMA fix if we know for certain that HiZ is enabled.
    * If we don't know whether HiZ is enabled or not, we disable the PMA fix
    * and there is no harm.
    *
    * (3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL) &&
    * 3DSTATE_DEPTH_BUFFER::HIZ Enable
    */
   if (!cmd_buffer->state.hiz_enabled)
      return false;

   /* 3DSTATE_PS_EXTRA::PixelShaderValid */
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT))
      return false;

   /* !(3DSTATE_WM::EDSC_Mode == EDSC_PREPS) */
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
   if (wm_prog_data->early_fragment_tests)
      return false;

   /* We never use anv_pipeline for HiZ ops so this is trivially true:
    *    !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
    *      3DSTATE_WM_HZ_OP::DepthBufferResolve ||
    *      3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
    *      3DSTATE_WM_HZ_OP::StencilBufferClear)
    */

   /* 3DSTATE_WM_DEPTH_STENCIL::DepthTestEnable */
   if (!pipeline->depth_test_enable)
      return false;

   /* (((3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
    *    3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
    *    3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
    *    3DSTATE_PS_BLEND::AlphaTestEnable ||
    *    3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable) &&
    *   3DSTATE_WM::ForceKillPix != ForceOff &&
    *   ((3DSTATE_WM_DEPTH_STENCIL::DepthWriteEnable &&
    *     3DSTATE_DEPTH_BUFFER::DEPTH_WRITE_ENABLE) ||
    *    (3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
    *     3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE &&
    *     3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE))) ||
    *  (3DSTATE_PS_EXTRA:: Pixel Shader Computed Depth mode != PSCDEPTH_OFF))
    */
   return (pipeline->kill_pixel && (pipeline->writes_depth ||
                                    pipeline->writes_stencil)) ||
          wm_prog_data->computed_depth_mode != PSCDEPTH_OFF;
}

UNUSED static bool
want_stencil_pma_fix(struct anv_cmd_buffer *cmd_buffer)
{
   if (GFX_VER > 9)
      return false;
   assert(GFX_VER == 9);

   /* From the Skylake PRM Vol. 2c CACHE_MODE_1::STC PMA Optimization Enable:
    *
    *    Clearing this bit will force the STC cache to wait for pending
    *    retirement of pixels at the HZ-read stage and do the STC-test for
    *    Non-promoted, R-computed and Computed depth modes instead of
    *    postponing the STC-test to RCPFE.
    *
    *    STC_TEST_EN = 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
    *                  3DSTATE_WM_DEPTH_STENCIL::StencilTestEnable
    *
    *    STC_WRITE_EN = 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
    *                   (3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
    *                    3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE)
    *
    *    COMP_STC_EN = STC_TEST_EN &&
    *                  3DSTATE_PS_EXTRA::PixelShaderComputesStencil
    *
    *    SW parses the pipeline states to generate the following logical
    *    signal indicating if PMA FIX can be enabled.
    *
    *    STC_PMA_OPT =
    *       3DSTATE_WM::ForceThreadDispatch != 1 &&
    *       !(3DSTATE_RASTER::ForceSampleCount != NUMRASTSAMPLES_0) &&
    *       3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL &&
    *       3DSTATE_DEPTH_BUFFER::HIZ Enable &&
    *       !(3DSTATE_WM::EDSC_Mode == 2) &&
    *       3DSTATE_PS_EXTRA::PixelShaderValid &&
    *       !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
    *         3DSTATE_WM_HZ_OP::DepthBufferResolve ||
    *         3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
    *         3DSTATE_WM_HZ_OP::StencilBufferClear) &&
    *       (COMP_STC_EN || STC_WRITE_EN) &&
    *       ((3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
    *         3DSTATE_WM::ForceKillPix == ON ||
    *         3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
    *         3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
    *         3DSTATE_PS_BLEND::AlphaTestEnable ||
    *         3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable) ||
    *        (3DSTATE_PS_EXTRA::Pixel Shader Computed Depth mode != PSCDEPTH_OFF))
    */

   /* These are always true:
    *    3DSTATE_WM::ForceThreadDispatch != 1 &&
    *    !(3DSTATE_RASTER::ForceSampleCount != NUMRASTSAMPLES_0)
    */

   /* We only enable the PMA fix if we know for certain that HiZ is enabled.
    * If we don't know whether HiZ is enabled or not, we disable the PMA fix
    * and there is no harm.
    *
    * (3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL) &&
    * 3DSTATE_DEPTH_BUFFER::HIZ Enable
    */
   if (!cmd_buffer->state.hiz_enabled)
      return false;

   /* We can't possibly know if HiZ is enabled without the framebuffer */
   assert(cmd_buffer->state.framebuffer);

   /* HiZ is enabled so we had better have a depth buffer with HiZ */
   const struct anv_image_view *ds_iview =
      anv_cmd_buffer_get_depth_stencil_view(cmd_buffer);
   assert(ds_iview && ds_iview->image->planes[0].aux_usage == ISL_AUX_USAGE_HIZ);

   /* 3DSTATE_PS_EXTRA::PixelShaderValid */
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT))
      return false;

   /* !(3DSTATE_WM::EDSC_Mode == 2) */
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
   if (wm_prog_data->early_fragment_tests)
      return false;

   /* We never use anv_pipeline for HiZ ops so this is trivially true:
   *    !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
    *      3DSTATE_WM_HZ_OP::DepthBufferResolve ||
    *      3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
    *      3DSTATE_WM_HZ_OP::StencilBufferClear)
    */

   /* 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
    * 3DSTATE_WM_DEPTH_STENCIL::StencilTestEnable
    */
   const bool stc_test_en =
      (ds_iview->image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
      pipeline->stencil_test_enable;

   /* 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
    * (3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
    *  3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE)
    */
   const bool stc_write_en =
      (ds_iview->image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
      (cmd_buffer->state.gfx.dynamic.stencil_write_mask.front ||
       cmd_buffer->state.gfx.dynamic.stencil_write_mask.back) &&
      pipeline->writes_stencil;

   /* STC_TEST_EN && 3DSTATE_PS_EXTRA::PixelShaderComputesStencil */
   const bool comp_stc_en = stc_test_en && wm_prog_data->computed_stencil;

   /* COMP_STC_EN || STC_WRITE_EN */
   if (!(comp_stc_en || stc_write_en))
      return false;

   /* (3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
    *  3DSTATE_WM::ForceKillPix == ON ||
    *  3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
    *  3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
    *  3DSTATE_PS_BLEND::AlphaTestEnable ||
    *  3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable) ||
    * (3DSTATE_PS_EXTRA::Pixel Shader Computed Depth mode != PSCDEPTH_OFF)
    */
   return pipeline->kill_pixel ||
          wm_prog_data->computed_depth_mode != PSCDEPTH_OFF;
}

void
genX(cmd_buffer_flush_dynamic_state)(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   struct anv_dynamic_state *d = &cmd_buffer->state.gfx.dynamic;

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY) {
      uint32_t topology;
      if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
         topology = pipeline->topology;
      else
         topology = genX(vk_to_intel_primitive_type)[d->primitive_topology];

      cmd_buffer->state.gfx.primitive_topology = topology;

      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_VF_TOPOLOGY), vft) {
         vft.PrimitiveTopologyType = topology;
      }
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_DYNAMIC_LINE_WIDTH)) {
      uint32_t sf_dw[GENX(3DSTATE_SF_length)];
      struct GENX(3DSTATE_SF) sf = {
         GENX(3DSTATE_SF_header),
      };
#if GFX_VER == 8
      if (cmd_buffer->device->info.is_cherryview) {
         sf.CHVLineWidth = d->line_width;
      } else {
         sf.LineWidth = d->line_width;
      }
#else
      sf.LineWidth = d->line_width,
#endif
      GENX(3DSTATE_SF_pack)(NULL, sf_dw, &sf);
      anv_batch_emit_merge(&cmd_buffer->batch, sf_dw, pipeline->gfx8.sf);
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS |
                                      ANV_CMD_DIRTY_DYNAMIC_CULL_MODE |
                                      ANV_CMD_DIRTY_DYNAMIC_FRONT_FACE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY)) {
      /* Take dynamic primitive topology in to account with
       *    3DSTATE_RASTER::APIMode
       *    3DSTATE_RASTER::DXMultisampleRasterizationEnable
       *    3DSTATE_RASTER::AntialiasingEnable
       */
      uint32_t api_mode = 0;
      bool msaa_raster_enable = false;
      bool aa_enable = 0;

      if (cmd_buffer->state.gfx.pipeline->dynamic_states &
          ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY) {
         VkPrimitiveTopology primitive_topology =
            cmd_buffer->state.gfx.dynamic.primitive_topology;

         VkPolygonMode dynamic_raster_mode =
            genX(raster_polygon_mode)(cmd_buffer->state.gfx.pipeline,
                                      primitive_topology);

         genX(rasterization_mode)(
            dynamic_raster_mode, pipeline->line_mode, d->line_width,
            &api_mode, &msaa_raster_enable);

         aa_enable =
            anv_rasterization_aa_mode(dynamic_raster_mode,
                                      pipeline->line_mode);
      }

      uint32_t raster_dw[GENX(3DSTATE_RASTER_length)];
      struct GENX(3DSTATE_RASTER) raster = {
         GENX(3DSTATE_RASTER_header),
         .APIMode = api_mode,
         .DXMultisampleRasterizationEnable = msaa_raster_enable,
         .AntialiasingEnable = aa_enable,
         .GlobalDepthOffsetConstant = d->depth_bias.bias,
         .GlobalDepthOffsetScale = d->depth_bias.slope,
         .GlobalDepthOffsetClamp = d->depth_bias.clamp,
         .CullMode = genX(vk_to_intel_cullmode)[d->cull_mode],
         .FrontWinding = genX(vk_to_intel_front_face)[d->front_face],
         .GlobalDepthOffsetEnableSolid = d->depth_bias_enable,
         .GlobalDepthOffsetEnableWireframe = d->depth_bias_enable,
         .GlobalDepthOffsetEnablePoint = d->depth_bias_enable,
      };
      GENX(3DSTATE_RASTER_pack)(NULL, raster_dw, &raster);
      anv_batch_emit_merge(&cmd_buffer->batch, raster_dw,
                           pipeline->gfx8.raster);
   }

   /* Stencil reference values moved from COLOR_CALC_STATE in gfx8 to
    * 3DSTATE_WM_DEPTH_STENCIL in gfx9. That means the dirty bits gets split
    * across different state packets for gfx8 and gfx9. We handle that by
    * using a big old #if switch here.
    */
#if GFX_VER == 8
   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_DYNAMIC_BLEND_CONSTANTS |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_REFERENCE)) {
      struct anv_state cc_state =
         anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                                            GENX(COLOR_CALC_STATE_length) * 4,
                                            64);
      struct GENX(COLOR_CALC_STATE) cc = {
         .BlendConstantColorRed = d->blend_constants[0],
         .BlendConstantColorGreen = d->blend_constants[1],
         .BlendConstantColorBlue = d->blend_constants[2],
         .BlendConstantColorAlpha = d->blend_constants[3],
         .StencilReferenceValue = d->stencil_reference.front & 0xff,
         .BackfaceStencilReferenceValue = d->stencil_reference.back & 0xff,
      };
      GENX(COLOR_CALC_STATE_pack)(NULL, cc_state.map, &cc);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CC_STATE_POINTERS), ccp) {
         ccp.ColorCalcStatePointer        = cc_state.offset;
         ccp.ColorCalcStatePointerValid   = true;
      }
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_RENDER_TARGETS |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_COMPARE_MASK |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_WRITE_MASK |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_TEST_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_WRITE_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_COMPARE_OP |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_TEST_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_OP)) {
      uint32_t wm_depth_stencil_dw[GENX(3DSTATE_WM_DEPTH_STENCIL_length)];

      struct GENX(3DSTATE_WM_DEPTH_STENCIL wm_depth_stencil) = {
         GENX(3DSTATE_WM_DEPTH_STENCIL_header),

         .StencilTestMask = d->stencil_compare_mask.front & 0xff,
         .StencilWriteMask = d->stencil_write_mask.front & 0xff,

         .BackfaceStencilTestMask = d->stencil_compare_mask.back & 0xff,
         .BackfaceStencilWriteMask = d->stencil_write_mask.back & 0xff,

         .StencilBufferWriteEnable =
            (d->stencil_write_mask.front || d->stencil_write_mask.back) &&
            d->stencil_test_enable,

         .DepthTestEnable = d->depth_test_enable,
         .DepthBufferWriteEnable = d->depth_test_enable && d->depth_write_enable,
         .DepthTestFunction = genX(vk_to_intel_compare_op)[d->depth_compare_op],
         .StencilTestEnable = d->stencil_test_enable,
         .StencilFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.front.fail_op],
         .StencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[d->stencil_op.front.pass_op],
         .StencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.front.depth_fail_op],
         .StencilTestFunction = genX(vk_to_intel_compare_op)[d->stencil_op.front.compare_op],
         .BackfaceStencilFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.back.fail_op],
         .BackfaceStencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[d->stencil_op.back.pass_op],
         .BackfaceStencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.back.depth_fail_op],
         .BackfaceStencilTestFunction = genX(vk_to_intel_compare_op)[d->stencil_op.back.compare_op],
      };
      GENX(3DSTATE_WM_DEPTH_STENCIL_pack)(NULL, wm_depth_stencil_dw,
                                          &wm_depth_stencil);

      anv_batch_emit_merge(&cmd_buffer->batch, wm_depth_stencil_dw,
                           pipeline->gfx8.wm_depth_stencil);

      genX(cmd_buffer_enable_pma_fix)(cmd_buffer,
                                      want_depth_pma_fix(cmd_buffer));
   }
#else
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_BLEND_CONSTANTS) {
      struct anv_state cc_state =
         anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                                            GENX(COLOR_CALC_STATE_length) * 4,
                                            64);
      struct GENX(COLOR_CALC_STATE) cc = {
         .BlendConstantColorRed = d->blend_constants[0],
         .BlendConstantColorGreen = d->blend_constants[1],
         .BlendConstantColorBlue = d->blend_constants[2],
         .BlendConstantColorAlpha = d->blend_constants[3],
      };
      GENX(COLOR_CALC_STATE_pack)(NULL, cc_state.map, &cc);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CC_STATE_POINTERS), ccp) {
         ccp.ColorCalcStatePointer = cc_state.offset;
         ccp.ColorCalcStatePointerValid = true;
      }
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_RENDER_TARGETS |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_COMPARE_MASK |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_WRITE_MASK |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_REFERENCE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_TEST_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_WRITE_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_COMPARE_OP |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_TEST_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_STENCIL_OP)) {
      uint32_t dwords[GENX(3DSTATE_WM_DEPTH_STENCIL_length)];
      struct GENX(3DSTATE_WM_DEPTH_STENCIL) wm_depth_stencil = {
         GENX(3DSTATE_WM_DEPTH_STENCIL_header),

         .StencilTestMask = d->stencil_compare_mask.front & 0xff,
         .StencilWriteMask = d->stencil_write_mask.front & 0xff,

         .BackfaceStencilTestMask = d->stencil_compare_mask.back & 0xff,
         .BackfaceStencilWriteMask = d->stencil_write_mask.back & 0xff,

         .StencilReferenceValue = d->stencil_reference.front & 0xff,
         .BackfaceStencilReferenceValue = d->stencil_reference.back & 0xff,

         .StencilBufferWriteEnable =
            (d->stencil_write_mask.front || d->stencil_write_mask.back) &&
            d->stencil_test_enable,

         .DepthTestEnable = d->depth_test_enable,
         .DepthBufferWriteEnable = d->depth_test_enable && d->depth_write_enable,
         .DepthTestFunction = genX(vk_to_intel_compare_op)[d->depth_compare_op],
         .StencilTestEnable = d->stencil_test_enable,
         .StencilFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.front.fail_op],
         .StencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[d->stencil_op.front.pass_op],
         .StencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.front.depth_fail_op],
         .StencilTestFunction = genX(vk_to_intel_compare_op)[d->stencil_op.front.compare_op],
         .BackfaceStencilFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.back.fail_op],
         .BackfaceStencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[d->stencil_op.back.pass_op],
         .BackfaceStencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[d->stencil_op.back.depth_fail_op],
         .BackfaceStencilTestFunction = genX(vk_to_intel_compare_op)[d->stencil_op.back.compare_op],

      };
      GENX(3DSTATE_WM_DEPTH_STENCIL_pack)(NULL, dwords, &wm_depth_stencil);

      anv_batch_emit_merge(&cmd_buffer->batch, dwords,
                           pipeline->gfx9.wm_depth_stencil);

      genX(cmd_buffer_enable_pma_fix)(cmd_buffer,
                                      want_stencil_pma_fix(cmd_buffer));
   }
#endif

#if GFX_VER >= 12
   if(cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                     ANV_CMD_DIRTY_DYNAMIC_DEPTH_BOUNDS |
                                     ANV_CMD_DIRTY_DYNAMIC_DEPTH_BOUNDS_TEST_ENABLE)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_DEPTH_BOUNDS), db) {
         db.DepthBoundsTestValueModifyDisable = false;
         db.DepthBoundsTestEnableModifyDisable = false;
         db.DepthBoundsTestEnable = d->depth_bounds_test_enable;
         db.DepthBoundsTestMinValue = d->depth_bounds.min;
         db.DepthBoundsTestMaxValue = d->depth_bounds.max;
      }
   }
#endif

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_LINE_STIPPLE) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_LINE_STIPPLE), ls) {
         ls.LineStipplePattern = d->line_stipple.pattern;
         ls.LineStippleInverseRepeatCount =
            1.0f / MAX2(1, d->line_stipple.factor);
         ls.LineStippleRepeatCount = d->line_stipple.factor;
      }
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_INDEX_BUFFER |
                                      ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_RESTART_ENABLE)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_VF), vf) {
         vf.IndexedDrawCutIndexEnable  = d->primitive_restart_enable;
         vf.CutIndex                   = cmd_buffer->state.restart_index;
      }
   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_SAMPLE_LOCATIONS) {
      genX(emit_sample_pattern)(&cmd_buffer->batch,
                                cmd_buffer->state.gfx.dynamic.sample_locations.samples,
                                cmd_buffer->state.gfx.dynamic.sample_locations.locations);
   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE ||
       cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP) {
      const uint8_t color_writes = cmd_buffer->state.gfx.dynamic.color_writes;
      /* 3DSTATE_WM in the hope we can avoid spawning fragment shaders
       * threads.
       */
      bool dirty_color_blend =
         cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE;

      if (dirty_color_blend) {
         uint32_t dwords[MAX2(GENX(3DSTATE_WM_length),
                              GENX(3DSTATE_PS_BLEND_length))];
         struct GENX(3DSTATE_WM) wm = {
            GENX(3DSTATE_WM_header),

            .ForceThreadDispatchEnable = (pipeline->force_fragment_thread_dispatch ||
                                          !color_writes) ? ForceON : 0,
         };
         GENX(3DSTATE_WM_pack)(NULL, dwords, &wm);

         anv_batch_emit_merge(&cmd_buffer->batch, dwords, pipeline->gfx8.wm);

         /* 3DSTATE_PS_BLEND to be consistent with the rest of the
          * BLEND_STATE_ENTRY.
          */
         struct GENX(3DSTATE_PS_BLEND) ps_blend = {
            GENX(3DSTATE_PS_BLEND_header),
            .HasWriteableRT = color_writes != 0,
         };
         GENX(3DSTATE_PS_BLEND_pack)(NULL, dwords, &ps_blend);
         anv_batch_emit_merge(&cmd_buffer->batch, dwords, pipeline->gfx8.ps_blend);
      }

      /* Blend states of each RT */
      uint32_t surface_count = 0;
      struct anv_pipeline_bind_map *map;
      if (anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT)) {
         map = &pipeline->shaders[MESA_SHADER_FRAGMENT]->bind_map;
         surface_count = map->surface_count;
      }

      uint32_t blend_dws[GENX(BLEND_STATE_length) +
                         MAX_RTS * GENX(BLEND_STATE_ENTRY_length)];
      uint32_t *dws = blend_dws;
      memset(blend_dws, 0, sizeof(blend_dws));

      /* Skip this part */
      dws += GENX(BLEND_STATE_length);

      bool dirty_logic_op =
         cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP;

      for (uint32_t i = 0; i < surface_count; i++) {
         struct anv_pipeline_binding *binding = &map->surface_to_descriptor[i];
         bool write_disabled =
            dirty_color_blend && (color_writes & (1u << binding->index)) == 0;
         struct GENX(BLEND_STATE_ENTRY) entry = {
            .WriteDisableAlpha = write_disabled,
            .WriteDisableRed   = write_disabled,
            .WriteDisableGreen = write_disabled,
            .WriteDisableBlue  = write_disabled,
            .LogicOpFunction =
               dirty_logic_op ? genX(vk_to_intel_logic_op)[d->logic_op] : 0,
         };
         GENX(BLEND_STATE_ENTRY_pack)(NULL, dws, &entry);
         dws += GENX(BLEND_STATE_ENTRY_length);
      }

      uint32_t num_dwords = GENX(BLEND_STATE_length) +
         GENX(BLEND_STATE_ENTRY_length) * surface_count;

      struct anv_state blend_states =
         anv_cmd_buffer_merge_dynamic(cmd_buffer, blend_dws,
                                      pipeline->gfx8.blend_state, num_dwords, 64);
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_BLEND_STATE_POINTERS), bsp) {
         bsp.BlendStatePointer      = blend_states.offset;
         bsp.BlendStatePointerValid = true;
      }
   }

#if GFX_VER >= 11
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_SHADING_RATE) {
      struct anv_state cps_states = ANV_STATE_NULL;

#if GFX_VER >= 12
      uint32_t count = cmd_buffer->state.gfx.dynamic.viewport.count;
      cps_states =
         anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                                            GENX(CPS_STATE_length) * 4 * count,
                                            32);
#endif /* GFX_VER >= 12 */

      genX(emit_shading_rate)(&cmd_buffer->batch, pipeline, cps_states,
                              &cmd_buffer->state.gfx.dynamic);
   }
#endif /* GFX_VER >= 11 */

   cmd_buffer->state.gfx.dirty = 0;
}

static uint32_t vk_to_intel_index_type(VkIndexType type)
{
   switch (type) {
   case VK_INDEX_TYPE_UINT8_EXT:
      return INDEX_BYTE;
   case VK_INDEX_TYPE_UINT16:
      return INDEX_WORD;
   case VK_INDEX_TYPE_UINT32:
      return INDEX_DWORD;
   default:
      unreachable("invalid index type");
   }
}

static uint32_t restart_index_for_type(VkIndexType type)
{
   switch (type) {
   case VK_INDEX_TYPE_UINT8_EXT:
      return UINT8_MAX;
   case VK_INDEX_TYPE_UINT16:
      return UINT16_MAX;
   case VK_INDEX_TYPE_UINT32:
      return UINT32_MAX;
   default:
      unreachable("invalid index type");
   }
}

void genX(CmdBindIndexBuffer)(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    _buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);

   cmd_buffer->state.restart_index = restart_index_for_type(indexType);

   anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_INDEX_BUFFER), ib) {
      ib.IndexFormat           = vk_to_intel_index_type(indexType);
      ib.MOCS                  = anv_mocs(cmd_buffer->device,
                                          buffer->address.bo,
                                          ISL_SURF_USAGE_INDEX_BUFFER_BIT);
#if GFX_VER >= 12
      ib.L3BypassDisable       = true;
#endif
      ib.BufferStartingAddress = anv_address_add(buffer->address, offset);
      ib.BufferSize            = buffer->size - offset;
   }

   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_INDEX_BUFFER;
}
