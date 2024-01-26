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
#include "vk_format.h"

#include "genxml/gen_macros.h"
#include "genxml/genX_pack.h"

#if GFX_VERx10 == 70
static int64_t
clamp_int64(int64_t x, int64_t min, int64_t max)
{
   if (x < min)
      return min;
   else if (x < max)
      return x;
   else
      return max;
}

void
gfx7_cmd_buffer_emit_scissor(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_framebuffer *fb = cmd_buffer->state.framebuffer;
   uint32_t count = cmd_buffer->state.gfx.dynamic.scissor.count;
   const VkRect2D *scissors = cmd_buffer->state.gfx.dynamic.scissor.scissors;

   /* Wa_1409725701:
    *    "The viewport-specific state used by the SF unit (SCISSOR_RECT) is
    *    stored as an array of up to 16 elements. The location of first
    *    element of the array, as specified by Pointer to SCISSOR_RECT, should
    *    be aligned to a 64-byte boundary.
    */
   uint32_t alignment = 64;
   struct anv_state scissor_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, count * 8, alignment);

   for (uint32_t i = 0; i < count; i++) {
      const VkRect2D *s = &scissors[i];

      /* Since xmax and ymax are inclusive, we have to have xmax < xmin or
       * ymax < ymin for empty clips.  In case clip x, y, width height are all
       * 0, the clamps below produce 0 for xmin, ymin, xmax, ymax, which isn't
       * what we want. Just special case empty clips and produce a canonical
       * empty clip. */
      static const struct GFX7_SCISSOR_RECT empty_scissor = {
         .ScissorRectangleYMin = 1,
         .ScissorRectangleXMin = 1,
         .ScissorRectangleYMax = 0,
         .ScissorRectangleXMax = 0
      };

      const int max = 0xffff;

      uint32_t y_min = s->offset.y;
      uint32_t x_min = s->offset.x;
      uint32_t y_max = s->offset.y + s->extent.height - 1;
      uint32_t x_max = s->offset.x + s->extent.width - 1;

      /* Do this math using int64_t so overflow gets clamped correctly. */
      if (cmd_buffer->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
         y_min = clamp_int64((uint64_t) y_min,
                             cmd_buffer->state.render_area.offset.y, max);
         x_min = clamp_int64((uint64_t) x_min,
                             cmd_buffer->state.render_area.offset.x, max);
         y_max = clamp_int64((uint64_t) y_max, 0,
                             cmd_buffer->state.render_area.offset.y +
                             cmd_buffer->state.render_area.extent.height - 1);
         x_max = clamp_int64((uint64_t) x_max, 0,
                             cmd_buffer->state.render_area.offset.x +
                             cmd_buffer->state.render_area.extent.width - 1);
      } else if (fb) {
         y_min = clamp_int64((uint64_t) y_min, 0, max);
         x_min = clamp_int64((uint64_t) x_min, 0, max);
         y_max = clamp_int64((uint64_t) y_max, 0, fb->height - 1);
         x_max = clamp_int64((uint64_t) x_max, 0, fb->width - 1);
      }

      struct GFX7_SCISSOR_RECT scissor = {
         .ScissorRectangleYMin = y_min,
         .ScissorRectangleXMin = x_min,
         .ScissorRectangleYMax = y_max,
         .ScissorRectangleXMax = x_max
      };

      if (s->extent.width <= 0 || s->extent.height <= 0) {
         GFX7_SCISSOR_RECT_pack(NULL, scissor_state.map + i * 8,
                                &empty_scissor);
      } else {
         GFX7_SCISSOR_RECT_pack(NULL, scissor_state.map + i * 8, &scissor);
      }
   }

   anv_batch_emit(&cmd_buffer->batch,
                  GFX7_3DSTATE_SCISSOR_STATE_POINTERS, ssp) {
      ssp.ScissorRectPointer = scissor_state.offset;
   }
}
#endif

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

   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_INDEX_BUFFER;
   if (GFX_VERx10 == 75)
      cmd_buffer->state.restart_index = restart_index_for_type(indexType);
   cmd_buffer->state.gfx.gfx7.index_buffer = buffer;
   cmd_buffer->state.gfx.gfx7.index_type = vk_to_intel_index_type(indexType);
   cmd_buffer->state.gfx.gfx7.index_offset = offset;
}

static uint32_t
get_depth_format(struct anv_cmd_buffer *cmd_buffer)
{
   const struct anv_render_pass *pass = cmd_buffer->state.pass;
   const struct anv_subpass *subpass = cmd_buffer->state.subpass;

   if (!subpass->depth_stencil_attachment)
      return D16_UNORM;

   struct anv_render_pass_attachment *att =
      &pass->attachments[subpass->depth_stencil_attachment->attachment];

   switch (att->format) {
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_D16_UNORM_S8_UINT:
      return D16_UNORM;

   case VK_FORMAT_X8_D24_UNORM_PACK32:
   case VK_FORMAT_D24_UNORM_S8_UINT:
      return D24_UNORM_X8_UINT;

   case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return D32_FLOAT;

   default:
      return D16_UNORM;
   }
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
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_RENDER_TARGETS |
                                      ANV_CMD_DIRTY_DYNAMIC_LINE_WIDTH |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS |
                                      ANV_CMD_DIRTY_DYNAMIC_CULL_MODE |
                                      ANV_CMD_DIRTY_DYNAMIC_FRONT_FACE |
                                      ANV_CMD_DIRTY_DYNAMIC_DEPTH_BIAS_ENABLE |
                                      ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY)) {
      /* Take dynamic primitive topology in to account with
       *    3DSTATE_SF::MultisampleRasterizationMode
       */
      uint32_t ms_rast_mode = 0;

      if (cmd_buffer->state.gfx.pipeline->dynamic_states &
          ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY) {
         VkPrimitiveTopology primitive_topology =
            cmd_buffer->state.gfx.dynamic.primitive_topology;

         VkPolygonMode dynamic_raster_mode =
            genX(raster_polygon_mode)(cmd_buffer->state.gfx.pipeline,
                                      primitive_topology);

         ms_rast_mode =
            genX(ms_rasterization_mode)(pipeline, dynamic_raster_mode);
      }

      uint32_t sf_dw[GENX(3DSTATE_SF_length)];
      struct GENX(3DSTATE_SF) sf = {
         GENX(3DSTATE_SF_header),
         .DepthBufferSurfaceFormat = get_depth_format(cmd_buffer),
         .LineWidth = d->line_width,
         .GlobalDepthOffsetConstant = d->depth_bias.bias,
         .GlobalDepthOffsetScale = d->depth_bias.slope,
         .GlobalDepthOffsetClamp = d->depth_bias.clamp,
         .FrontWinding            = genX(vk_to_intel_front_face)[d->front_face],
         .CullMode                = genX(vk_to_intel_cullmode)[d->cull_mode],
         .GlobalDepthOffsetEnableSolid = d->depth_bias_enable,
         .GlobalDepthOffsetEnableWireframe = d->depth_bias_enable,
         .GlobalDepthOffsetEnablePoint = d->depth_bias_enable,
         .MultisampleRasterizationMode = ms_rast_mode,
      };
      GENX(3DSTATE_SF_pack)(NULL, sf_dw, &sf);

      anv_batch_emit_merge(&cmd_buffer->batch, sf_dw, pipeline->gfx7.sf);
   }

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
         ccp.ColorCalcStatePointer = cc_state.offset;
      }
   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_LINE_STIPPLE) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_LINE_STIPPLE), ls) {
         ls.LineStipplePattern = d->line_stipple.pattern;
         ls.LineStippleInverseRepeatCount =
            1.0f / MAX2(1, d->line_stipple.factor);
         ls.LineStippleRepeatCount = d->line_stipple.factor;
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
      uint32_t depth_stencil_dw[GENX(DEPTH_STENCIL_STATE_length)];

      struct GENX(DEPTH_STENCIL_STATE) depth_stencil = {
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
      GENX(DEPTH_STENCIL_STATE_pack)(NULL, depth_stencil_dw, &depth_stencil);

      struct anv_state ds_state =
         anv_cmd_buffer_merge_dynamic(cmd_buffer, depth_stencil_dw,
                                      pipeline->gfx7.depth_stencil_state,
                                      GENX(DEPTH_STENCIL_STATE_length), 64);

      anv_batch_emit(&cmd_buffer->batch,
                     GENX(3DSTATE_DEPTH_STENCIL_STATE_POINTERS), dsp) {
         dsp.PointertoDEPTH_STENCIL_STATE = ds_state.offset;
      }
   }

   if (cmd_buffer->state.gfx.gfx7.index_buffer &&
       cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
                                      ANV_CMD_DIRTY_INDEX_BUFFER |
                                      ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_RESTART_ENABLE)) {
      struct anv_buffer *buffer = cmd_buffer->state.gfx.gfx7.index_buffer;
      uint32_t offset = cmd_buffer->state.gfx.gfx7.index_offset;

#if GFX_VERx10 == 75
      anv_batch_emit(&cmd_buffer->batch, GFX75_3DSTATE_VF, vf) {
         vf.IndexedDrawCutIndexEnable  = d->primitive_restart_enable;
         vf.CutIndex                   = cmd_buffer->state.restart_index;
      }
#endif

      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_INDEX_BUFFER), ib) {
#if GFX_VERx10 != 75
         ib.CutIndexEnable        = d->primitive_restart_enable;
#endif
         ib.IndexFormat           = cmd_buffer->state.gfx.gfx7.index_type;
         ib.MOCS                  = anv_mocs(cmd_buffer->device,
                                             buffer->address.bo,
                                             ISL_SURF_USAGE_INDEX_BUFFER_BIT);

         ib.BufferStartingAddress = anv_address_add(buffer->address, offset);
         ib.BufferEndingAddress   = anv_address_add(buffer->address,
                                                    buffer->size);
      }
   }

   /* 3DSTATE_WM in the hope we can avoid spawning fragment shaders
    * threads or if we have dirty dynamic primitive topology state and
    * need to toggle 3DSTATE_WM::MultisampleRasterizationMode dynamically.
    */
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE ||
       cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY) {
      const uint8_t color_writes = cmd_buffer->state.gfx.dynamic.color_writes;

      bool dirty_color_blend =
         cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE;

      bool dirty_primitive_topology =
         cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY;

      VkPolygonMode dynamic_raster_mode;
      VkPrimitiveTopology primitive_topology =
         cmd_buffer->state.gfx.dynamic.primitive_topology;
      dynamic_raster_mode =
         genX(raster_polygon_mode)(cmd_buffer->state.gfx.pipeline,
                                   primitive_topology);

      if (dirty_color_blend || dirty_primitive_topology) {
         uint32_t dwords[GENX(3DSTATE_WM_length)];
         struct GENX(3DSTATE_WM) wm = {
            GENX(3DSTATE_WM_header),

            .ThreadDispatchEnable = pipeline->force_fragment_thread_dispatch ||
                                    color_writes,
            .MultisampleRasterizationMode =
               genX(ms_rasterization_mode)(pipeline, dynamic_raster_mode),
         };
         GENX(3DSTATE_WM_pack)(NULL, dwords, &wm);

         anv_batch_emit_merge(&cmd_buffer->batch, dwords, pipeline->gfx7.wm);
      }

   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_SAMPLE_LOCATIONS) {
      genX(emit_multisample)(&cmd_buffer->batch,
                             cmd_buffer->state.gfx.dynamic.sample_locations.samples,
                             cmd_buffer->state.gfx.dynamic.sample_locations.locations);
   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE ||
       cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_LOGIC_OP) {
      const uint8_t color_writes = cmd_buffer->state.gfx.dynamic.color_writes;
      bool dirty_color_blend =
         cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_COLOR_BLEND_STATE;

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
                                      pipeline->gfx7.blend_state, num_dwords, 64);
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_BLEND_STATE_POINTERS), bsp) {
         bsp.BlendStatePointer      = blend_states.offset;
      }
   }

   cmd_buffer->state.gfx.dirty = 0;
}

void
genX(cmd_buffer_enable_pma_fix)(struct anv_cmd_buffer *cmd_buffer,
                                bool enable)
{
   /* The NP PMA fix doesn't exist on gfx7 */
}
