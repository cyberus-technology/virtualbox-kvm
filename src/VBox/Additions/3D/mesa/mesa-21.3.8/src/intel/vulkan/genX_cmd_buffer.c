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

#include "anv_private.h"
#include "anv_measure.h"
#include "vk_format.h"
#include "vk_util.h"
#include "util/fast_idiv_by_const.h"

#include "common/intel_aux_map.h"
#include "common/intel_l3_config.h"
#include "genxml/gen_macros.h"
#include "genxml/genX_pack.h"
#include "genxml/gen_rt_pack.h"

#include "nir/nir_xfb_info.h"

/* We reserve :
 *    - GPR 14 for secondary command buffer returns
 *    - GPR 15 for conditional rendering
 */
#define MI_BUILDER_NUM_ALLOC_GPRS 14
#define __gen_get_batch_dwords anv_batch_emit_dwords
#define __gen_address_offset anv_address_add
#define __gen_get_batch_address(b, a) anv_batch_address(b, a)
#include "common/mi_builder.h"

static void genX(flush_pipeline_select)(struct anv_cmd_buffer *cmd_buffer,
                                        uint32_t pipeline);

static enum anv_pipe_bits
convert_pc_to_bits(struct GENX(PIPE_CONTROL) *pc) {
   enum anv_pipe_bits bits = 0;
   bits |= (pc->DepthCacheFlushEnable) ?  ANV_PIPE_DEPTH_CACHE_FLUSH_BIT : 0;
   bits |= (pc->DCFlushEnable) ?  ANV_PIPE_DATA_CACHE_FLUSH_BIT : 0;
#if GFX_VER >= 12
   bits |= (pc->TileCacheFlushEnable) ?  ANV_PIPE_TILE_CACHE_FLUSH_BIT : 0;
   bits |= (pc->HDCPipelineFlushEnable) ?  ANV_PIPE_HDC_PIPELINE_FLUSH_BIT : 0;
#endif
   bits |= (pc->RenderTargetCacheFlushEnable) ?  ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT : 0;
   bits |= (pc->StateCacheInvalidationEnable) ?  ANV_PIPE_STATE_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->ConstantCacheInvalidationEnable) ?  ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->TextureCacheInvalidationEnable) ?  ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->InstructionCacheInvalidateEnable) ?  ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->StallAtPixelScoreboard) ?  ANV_PIPE_STALL_AT_SCOREBOARD_BIT : 0;
   bits |= (pc->DepthStallEnable) ?  ANV_PIPE_DEPTH_STALL_BIT : 0;
   bits |= (pc->CommandStreamerStallEnable) ?  ANV_PIPE_CS_STALL_BIT : 0;
   return bits;
}

#define anv_debug_dump_pc(pc) \
   if (INTEL_DEBUG(DEBUG_PIPE_CONTROL)) { \
      fputs("pc: emit PC=( ", stderr); \
      anv_dump_pipe_bits(convert_pc_to_bits(&(pc))); \
      fprintf(stderr, ") reason: %s\n", __FUNCTION__); \
   }

static bool
is_render_queue_cmd_buffer(const struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_queue_family *queue_family = cmd_buffer->pool->queue_family;
   return (queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
}

void
genX(cmd_buffer_emit_state_base_address)(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_device *device = cmd_buffer->device;
   UNUSED const struct intel_device_info *devinfo = &device->info;
   uint32_t mocs = isl_mocs(&device->isl_dev, 0, false);

   /* If we are emitting a new state base address we probably need to re-emit
    * binding tables.
    */
   cmd_buffer->state.descriptors_dirty |= ~0;

   /* Emit a render target cache flush.
    *
    * This isn't documented anywhere in the PRM.  However, it seems to be
    * necessary prior to changing the surface state base adress.  Without
    * this, we get GPU hangs when using multi-level command buffers which
    * clear depth, reset state base address, and then go render stuff.
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
#if GFX_VER >= 12
      pc.HDCPipelineFlushEnable = true;
#else
      pc.DCFlushEnable = true;
#endif
      pc.RenderTargetCacheFlushEnable = true;
      pc.CommandStreamerStallEnable = true;
#if GFX_VER == 12
      /* Wa_1606662791:
       *
       *   Software must program PIPE_CONTROL command with "HDC Pipeline
       *   Flush" prior to programming of the below two non-pipeline state :
       *      * STATE_BASE_ADDRESS
       *      * 3DSTATE_BINDING_TABLE_POOL_ALLOC
       */
      if (devinfo->revision == 0 /* A0 */)
         pc.HDCPipelineFlushEnable = true;
#endif
      anv_debug_dump_pc(pc);
   }

#if GFX_VER == 12
   /* Wa_1607854226:
    *
    *  Workaround the non pipelined state not applying in MEDIA/GPGPU pipeline
    *  mode by putting the pipeline temporarily in 3D mode.
    */
   uint32_t gfx12_wa_pipeline = cmd_buffer->state.current_pipeline;
   genX(flush_pipeline_select_3d)(cmd_buffer);
#endif

   anv_batch_emit(&cmd_buffer->batch, GENX(STATE_BASE_ADDRESS), sba) {
      sba.GeneralStateBaseAddress = (struct anv_address) { NULL, 0 };
      sba.GeneralStateMOCS = mocs;
      sba.GeneralStateBaseAddressModifyEnable = true;

      sba.StatelessDataPortAccessMOCS = mocs;

      sba.SurfaceStateBaseAddress =
         anv_cmd_buffer_surface_base_address(cmd_buffer);
      sba.SurfaceStateMOCS = mocs;
      sba.SurfaceStateBaseAddressModifyEnable = true;

      sba.DynamicStateBaseAddress =
         (struct anv_address) { device->dynamic_state_pool.block_pool.bo, 0 };
      sba.DynamicStateMOCS = mocs;
      sba.DynamicStateBaseAddressModifyEnable = true;

      sba.IndirectObjectBaseAddress = (struct anv_address) { NULL, 0 };
      sba.IndirectObjectMOCS = mocs;
      sba.IndirectObjectBaseAddressModifyEnable = true;

      sba.InstructionBaseAddress =
         (struct anv_address) { device->instruction_state_pool.block_pool.bo, 0 };
      sba.InstructionMOCS = mocs;
      sba.InstructionBaseAddressModifyEnable = true;

#  if (GFX_VER >= 8)
      /* Broadwell requires that we specify a buffer size for a bunch of
       * these fields.  However, since we will be growing the BO's live, we
       * just set them all to the maximum.
       */
      sba.GeneralStateBufferSize       = 0xfffff;
      sba.IndirectObjectBufferSize     = 0xfffff;
      if (anv_use_softpin(device->physical)) {
         /* With softpin, we use fixed addresses so we actually know how big
          * our base addresses are.
          */
         sba.DynamicStateBufferSize    = DYNAMIC_STATE_POOL_SIZE / 4096;
         sba.InstructionBufferSize     = INSTRUCTION_STATE_POOL_SIZE / 4096;
      } else {
         sba.DynamicStateBufferSize    = 0xfffff;
         sba.InstructionBufferSize     = 0xfffff;
      }
      sba.GeneralStateBufferSizeModifyEnable    = true;
      sba.IndirectObjectBufferSizeModifyEnable  = true;
      sba.DynamicStateBufferSizeModifyEnable    = true;
      sba.InstructionBuffersizeModifyEnable     = true;
#  else
      /* On gfx7, we have upper bounds instead.  According to the docs,
       * setting an upper bound of zero means that no bounds checking is
       * performed so, in theory, we should be able to leave them zero.
       * However, border color is broken and the GPU bounds-checks anyway.
       * To avoid this and other potential problems, we may as well set it
       * for everything.
       */
      sba.GeneralStateAccessUpperBound =
         (struct anv_address) { .bo = NULL, .offset = 0xfffff000 };
      sba.GeneralStateAccessUpperBoundModifyEnable = true;
      sba.DynamicStateAccessUpperBound =
         (struct anv_address) { .bo = NULL, .offset = 0xfffff000 };
      sba.DynamicStateAccessUpperBoundModifyEnable = true;
      sba.InstructionAccessUpperBound =
         (struct anv_address) { .bo = NULL, .offset = 0xfffff000 };
      sba.InstructionAccessUpperBoundModifyEnable = true;
#  endif
#  if (GFX_VER >= 9)
      if (anv_use_softpin(device->physical)) {
         sba.BindlessSurfaceStateBaseAddress = (struct anv_address) {
            .bo = device->surface_state_pool.block_pool.bo,
            .offset = 0,
         };
         sba.BindlessSurfaceStateSize = (1 << 20) - 1;
      } else {
         sba.BindlessSurfaceStateBaseAddress = ANV_NULL_ADDRESS;
         sba.BindlessSurfaceStateSize = 0;
      }
      sba.BindlessSurfaceStateMOCS = mocs;
      sba.BindlessSurfaceStateBaseAddressModifyEnable = true;
#  endif
#  if (GFX_VER >= 10)
      sba.BindlessSamplerStateBaseAddress = (struct anv_address) { NULL, 0 };
      sba.BindlessSamplerStateMOCS = mocs;
      sba.BindlessSamplerStateBaseAddressModifyEnable = true;
      sba.BindlessSamplerStateBufferSize = 0;
#  endif
   }

#if GFX_VER == 12
   /* Wa_1607854226:
    *
    *  Put the pipeline back into its current mode.
    */
   if (gfx12_wa_pipeline != UINT32_MAX)
      genX(flush_pipeline_select)(cmd_buffer, gfx12_wa_pipeline);
#endif

   /* After re-setting the surface state base address, we have to do some
    * cache flusing so that the sampler engine will pick up the new
    * SURFACE_STATE objects and binding tables. From the Broadwell PRM,
    * Shared Function > 3D Sampler > State > State Caching (page 96):
    *
    *    Coherency with system memory in the state cache, like the texture
    *    cache is handled partially by software. It is expected that the
    *    command stream or shader will issue Cache Flush operation or
    *    Cache_Flush sampler message to ensure that the L1 cache remains
    *    coherent with system memory.
    *
    *    [...]
    *
    *    Whenever the value of the Dynamic_State_Base_Addr,
    *    Surface_State_Base_Addr are altered, the L1 state cache must be
    *    invalidated to ensure the new surface or sampler state is fetched
    *    from system memory.
    *
    * The PIPE_CONTROL command has a "State Cache Invalidation Enable" bit
    * which, according the PIPE_CONTROL instruction documentation in the
    * Broadwell PRM:
    *
    *    Setting this bit is independent of any other bit in this packet.
    *    This bit controls the invalidation of the L1 and L2 state caches
    *    at the top of the pipe i.e. at the parsing time.
    *
    * Unfortunately, experimentation seems to indicate that state cache
    * invalidation through a PIPE_CONTROL does nothing whatsoever in
    * regards to surface state and binding tables.  In stead, it seems that
    * invalidating the texture cache is what is actually needed.
    *
    * XXX:  As far as we have been able to determine through
    * experimentation, shows that flush the texture cache appears to be
    * sufficient.  The theory here is that all of the sampling/rendering
    * units cache the binding table in the texture cache.  However, we have
    * yet to be able to actually confirm this.
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.TextureCacheInvalidationEnable = true;
      pc.ConstantCacheInvalidationEnable = true;
      pc.StateCacheInvalidationEnable = true;
      anv_debug_dump_pc(pc);
   }
}

static void
add_surface_reloc(struct anv_cmd_buffer *cmd_buffer,
                  struct anv_state state, struct anv_address addr)
{
   VkResult result;

   if (anv_use_softpin(cmd_buffer->device->physical)) {
      result = anv_reloc_list_add_bo(&cmd_buffer->surface_relocs,
                                     &cmd_buffer->pool->alloc,
                                     addr.bo);
   } else {
      const struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;
      result = anv_reloc_list_add(&cmd_buffer->surface_relocs,
                                  &cmd_buffer->pool->alloc,
                                  state.offset + isl_dev->ss.addr_offset,
                                  addr.bo, addr.offset, NULL);
   }

   if (unlikely(result != VK_SUCCESS))
      anv_batch_set_error(&cmd_buffer->batch, result);
}

static void
add_surface_state_relocs(struct anv_cmd_buffer *cmd_buffer,
                         struct anv_surface_state state)
{
   const struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;

   assert(!anv_address_is_null(state.address));
   add_surface_reloc(cmd_buffer, state.state, state.address);

   if (!anv_address_is_null(state.aux_address)) {
      VkResult result =
         anv_reloc_list_add(&cmd_buffer->surface_relocs,
                            &cmd_buffer->pool->alloc,
                            state.state.offset + isl_dev->ss.aux_addr_offset,
                            state.aux_address.bo,
                            state.aux_address.offset,
                            NULL);
      if (result != VK_SUCCESS)
         anv_batch_set_error(&cmd_buffer->batch, result);
   }

   if (!anv_address_is_null(state.clear_address)) {
      VkResult result =
         anv_reloc_list_add(&cmd_buffer->surface_relocs,
                            &cmd_buffer->pool->alloc,
                            state.state.offset +
                            isl_dev->ss.clear_color_state_offset,
                            state.clear_address.bo,
                            state.clear_address.offset,
                            NULL);
      if (result != VK_SUCCESS)
         anv_batch_set_error(&cmd_buffer->batch, result);
   }
}

static bool
isl_color_value_requires_conversion(union isl_color_value color,
                                    const struct isl_surf *surf,
                                    const struct isl_view *view)
{
   if (surf->format == view->format && isl_swizzle_is_identity(view->swizzle))
      return false;

   uint32_t surf_pack[4] = { 0, 0, 0, 0 };
   isl_color_value_pack(&color, surf->format, surf_pack);

   uint32_t view_pack[4] = { 0, 0, 0, 0 };
   union isl_color_value swiz_color =
      isl_color_value_swizzle_inv(color, view->swizzle);
   isl_color_value_pack(&swiz_color, view->format, view_pack);

   return memcmp(surf_pack, view_pack, sizeof(surf_pack)) != 0;
}

static bool
anv_can_fast_clear_color_view(struct anv_device * device,
                              struct anv_image_view *iview,
                              VkImageLayout layout,
                              union isl_color_value clear_color,
                              uint32_t num_layers,
                              VkRect2D render_area)
{
   if (iview->planes[0].isl.base_array_layer >=
       anv_image_aux_layers(iview->image, VK_IMAGE_ASPECT_COLOR_BIT,
                            iview->planes[0].isl.base_level))
      return false;

   /* Start by getting the fast clear type.  We use the first subpass
    * layout here because we don't want to fast-clear if the first subpass
    * to use the attachment can't handle fast-clears.
    */
   enum anv_fast_clear_type fast_clear_type =
      anv_layout_to_fast_clear_type(&device->info, iview->image,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    layout);
   switch (fast_clear_type) {
   case ANV_FAST_CLEAR_NONE:
      return false;
   case ANV_FAST_CLEAR_DEFAULT_VALUE:
      if (!isl_color_value_is_zero(clear_color, iview->planes[0].isl.format))
         return false;
      break;
   case ANV_FAST_CLEAR_ANY:
      break;
   }

   /* Potentially, we could do partial fast-clears but doing so has crazy
    * alignment restrictions.  It's easier to just restrict to full size
    * fast clears for now.
    */
   if (render_area.offset.x != 0 ||
       render_area.offset.y != 0 ||
       render_area.extent.width != iview->vk.extent.width ||
       render_area.extent.height != iview->vk.extent.height)
      return false;

   /* On Broadwell and earlier, we can only handle 0/1 clear colors */
   if (GFX_VER <= 8 &&
       !isl_color_value_is_zero_one(clear_color, iview->planes[0].isl.format))
      return false;

   /* If the clear color is one that would require non-trivial format
    * conversion on resolve, we don't bother with the fast clear.  This
    * shouldn't be common as most clear colors are 0/1 and the most common
    * format re-interpretation is for sRGB.
    */
   if (isl_color_value_requires_conversion(clear_color,
                                           &iview->image->planes[0].primary_surface.isl,
                                           &iview->planes[0].isl)) {
      anv_perf_warn(VK_LOG_OBJS(&iview->vk.base),
                    "Cannot fast-clear to colors which would require "
                    "format conversion on resolve");
      return false;
   }

   /* We only allow fast clears to the first slice of an image (level 0,
    * layer 0) and only for the entire slice.  This guarantees us that, at
    * any given time, there is only one clear color on any given image at
    * any given time.  At the time of our testing (Jan 17, 2018), there
    * were no known applications which would benefit from fast-clearing
    * more than just the first slice.
    */
   if (iview->planes[0].isl.base_level > 0 ||
       iview->planes[0].isl.base_array_layer > 0) {
      anv_perf_warn(VK_LOG_OBJS(&iview->image->vk.base),
                    "Rendering with multi-lod or multi-layer framebuffer "
                    "with LOAD_OP_LOAD and baseMipLevel > 0 or "
                    "baseArrayLayer > 0.  Not fast clearing.");
      return false;
   }

   if (num_layers > 1) {
      anv_perf_warn(VK_LOG_OBJS(&iview->image->vk.base),
                    "Rendering to a multi-layer framebuffer with "
                    "LOAD_OP_CLEAR.  Only fast-clearing the first slice");
   }

   return true;
}

static bool
anv_can_hiz_clear_ds_view(struct anv_device *device,
                          struct anv_image_view *iview,
                          VkImageLayout layout,
                          VkImageAspectFlags clear_aspects,
                          float depth_clear_value,
                          VkRect2D render_area)
{
   /* We don't do any HiZ or depth fast-clears on gfx7 yet */
   if (GFX_VER == 7)
      return false;

   /* If we're just clearing stencil, we can always HiZ clear */
   if (!(clear_aspects & VK_IMAGE_ASPECT_DEPTH_BIT))
      return true;

   /* We must have depth in order to have HiZ */
   if (!(iview->image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT))
      return false;

   const enum isl_aux_usage clear_aux_usage =
      anv_layout_to_aux_usage(&device->info, iview->image,
                              VK_IMAGE_ASPECT_DEPTH_BIT,
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                              layout);
   if (!blorp_can_hiz_clear_depth(&device->info,
                                  &iview->image->planes[0].primary_surface.isl,
                                  clear_aux_usage,
                                  iview->planes[0].isl.base_level,
                                  iview->planes[0].isl.base_array_layer,
                                  render_area.offset.x,
                                  render_area.offset.y,
                                  render_area.offset.x +
                                  render_area.extent.width,
                                  render_area.offset.y +
                                  render_area.extent.height))
      return false;

   if (depth_clear_value != ANV_HZ_FC_VAL)
      return false;

   /* Only gfx9+ supports returning ANV_HZ_FC_VAL when sampling a fast-cleared
    * portion of a HiZ buffer. Testing has revealed that Gfx8 only supports
    * returning 0.0f. Gens prior to gfx8 do not support this feature at all.
    */
   if (GFX_VER == 8 && anv_can_sample_with_hiz(&device->info, iview->image))
      return false;

   /* If we got here, then we can fast clear */
   return true;
}

#define READ_ONCE(x) (*(volatile __typeof__(x) *)&(x))

#if GFX_VER == 12
static void
anv_image_init_aux_tt(struct anv_cmd_buffer *cmd_buffer,
                      const struct anv_image *image,
                      VkImageAspectFlagBits aspect,
                      uint32_t base_level, uint32_t level_count,
                      uint32_t base_layer, uint32_t layer_count)
{
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);

   const struct anv_surface *surface = &image->planes[plane].primary_surface;
   uint64_t base_address =
      anv_address_physical(anv_image_address(image, &surface->memory_range));

   const struct isl_surf *isl_surf = &image->planes[plane].primary_surface.isl;
   uint64_t format_bits = intel_aux_map_format_bits_for_isl_surf(isl_surf);

   /* We're about to live-update the AUX-TT.  We really don't want anyone else
    * trying to read it while we're doing this.  We could probably get away
    * with not having this stall in some cases if we were really careful but
    * it's better to play it safe.  Full stall the GPU.
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_END_OF_PIPE_SYNC_BIT,
                             "before update AUX-TT");
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   for (uint32_t a = 0; a < layer_count; a++) {
      const uint32_t layer = base_layer + a;

      uint64_t start_offset_B = UINT64_MAX, end_offset_B = 0;
      for (uint32_t l = 0; l < level_count; l++) {
         const uint32_t level = base_level + l;

         uint32_t logical_array_layer, logical_z_offset_px;
         if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
            logical_array_layer = 0;

            /* If the given miplevel does not have this layer, then any higher
             * miplevels won't either because miplevels only get smaller the
             * higher the LOD.
             */
            assert(layer < image->vk.extent.depth);
            if (layer >= anv_minify(image->vk.extent.depth, level))
               break;
            logical_z_offset_px = layer;
         } else {
            assert(layer < image->vk.array_layers);
            logical_array_layer = layer;
            logical_z_offset_px = 0;
         }

         uint64_t slice_start_offset_B, slice_end_offset_B;
         isl_surf_get_image_range_B_tile(isl_surf, level,
                                         logical_array_layer,
                                         logical_z_offset_px,
                                         &slice_start_offset_B,
                                         &slice_end_offset_B);

         start_offset_B = MIN2(start_offset_B, slice_start_offset_B);
         end_offset_B = MAX2(end_offset_B, slice_end_offset_B);
      }

      /* Aux operates 64K at a time */
      start_offset_B = align_down_u64(start_offset_B, 64 * 1024);
      end_offset_B = align_u64(end_offset_B, 64 * 1024);

      for (uint64_t offset = start_offset_B;
           offset < end_offset_B; offset += 64 * 1024) {
         uint64_t address = base_address + offset;

         uint64_t aux_entry_addr64, *aux_entry_map;
         aux_entry_map = intel_aux_map_get_entry(cmd_buffer->device->aux_map_ctx,
                                                 address, &aux_entry_addr64);

         assert(anv_use_softpin(cmd_buffer->device->physical));
         struct anv_address aux_entry_address = {
            .bo = NULL,
            .offset = aux_entry_addr64,
         };

         const uint64_t old_aux_entry = READ_ONCE(*aux_entry_map);
         uint64_t new_aux_entry =
            (old_aux_entry & INTEL_AUX_MAP_ADDRESS_MASK) | format_bits;

         if (isl_aux_usage_has_ccs(image->planes[plane].aux_usage))
            new_aux_entry |= INTEL_AUX_MAP_ENTRY_VALID_BIT;

         mi_store(&b, mi_mem64(aux_entry_address), mi_imm(new_aux_entry));
      }
   }

   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_AUX_TABLE_INVALIDATE_BIT,
                             "after update AUX-TT");
}
#endif /* GFX_VER == 12 */

/* Transitions a HiZ-enabled depth buffer from one layout to another. Unless
 * the initial layout is undefined, the HiZ buffer and depth buffer will
 * represent the same data at the end of this operation.
 */
static void
transition_depth_buffer(struct anv_cmd_buffer *cmd_buffer,
                        const struct anv_image *image,
                        uint32_t base_layer, uint32_t layer_count,
                        VkImageLayout initial_layout,
                        VkImageLayout final_layout,
                        bool will_full_fast_clear)
{
   const uint32_t depth_plane =
      anv_image_aspect_to_plane(image, VK_IMAGE_ASPECT_DEPTH_BIT);
   if (image->planes[depth_plane].aux_usage == ISL_AUX_USAGE_NONE)
      return;

#if GFX_VER == 12
   if ((initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ||
        initial_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) &&
       cmd_buffer->device->physical->has_implicit_ccs &&
       cmd_buffer->device->info.has_aux_map) {
      anv_image_init_aux_tt(cmd_buffer, image, VK_IMAGE_ASPECT_DEPTH_BIT,
                            0, 1, base_layer, layer_count);
   }
#endif

   /* If will_full_fast_clear is set, the caller promises to fast-clear the
    * largest portion of the specified range as it can.  For depth images,
    * that means the entire image because we don't support multi-LOD HiZ.
    */
   assert(image->planes[0].primary_surface.isl.levels == 1);
   if (will_full_fast_clear)
      return;

   const enum isl_aux_state initial_state =
      anv_layout_to_aux_state(&cmd_buffer->device->info, image,
                              VK_IMAGE_ASPECT_DEPTH_BIT,
                              initial_layout);
   const enum isl_aux_state final_state =
      anv_layout_to_aux_state(&cmd_buffer->device->info, image,
                              VK_IMAGE_ASPECT_DEPTH_BIT,
                              final_layout);

   const bool initial_depth_valid =
      isl_aux_state_has_valid_primary(initial_state);
   const bool initial_hiz_valid =
      isl_aux_state_has_valid_aux(initial_state);
   const bool final_needs_depth =
      isl_aux_state_has_valid_primary(final_state);
   const bool final_needs_hiz =
      isl_aux_state_has_valid_aux(final_state);

   /* Getting into the pass-through state for Depth is tricky and involves
    * both a resolve and an ambiguate.  We don't handle that state right now
    * as anv_layout_to_aux_state never returns it.
    */
   assert(final_state != ISL_AUX_STATE_PASS_THROUGH);

   if (final_needs_depth && !initial_depth_valid) {
      assert(initial_hiz_valid);
      anv_image_hiz_op(cmd_buffer, image, VK_IMAGE_ASPECT_DEPTH_BIT,
                       0, base_layer, layer_count, ISL_AUX_OP_FULL_RESOLVE);
   } else if (final_needs_hiz && !initial_hiz_valid) {
      assert(initial_depth_valid);
      anv_image_hiz_op(cmd_buffer, image, VK_IMAGE_ASPECT_DEPTH_BIT,
                       0, base_layer, layer_count, ISL_AUX_OP_AMBIGUATE);
   }
}

static inline bool
vk_image_layout_stencil_write_optimal(VkImageLayout layout)
{
   return layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
          layout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
          layout == VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR;
}

/* Transitions a HiZ-enabled depth buffer from one layout to another. Unless
 * the initial layout is undefined, the HiZ buffer and depth buffer will
 * represent the same data at the end of this operation.
 */
static void
transition_stencil_buffer(struct anv_cmd_buffer *cmd_buffer,
                          const struct anv_image *image,
                          uint32_t base_level, uint32_t level_count,
                          uint32_t base_layer, uint32_t layer_count,
                          VkImageLayout initial_layout,
                          VkImageLayout final_layout,
                          bool will_full_fast_clear)
{
#if GFX_VER == 7
   const uint32_t plane =
      anv_image_aspect_to_plane(image, VK_IMAGE_ASPECT_STENCIL_BIT);

   /* On gfx7, we have to store a texturable version of the stencil buffer in
    * a shadow whenever VK_IMAGE_USAGE_SAMPLED_BIT is set and copy back and
    * forth at strategic points. Stencil writes are only allowed in following
    * layouts:
    *
    *  - VK_IMAGE_LAYOUT_GENERAL
    *  - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    *  - VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    *  - VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
    *  - VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR
    *
    * For general, we have no nice opportunity to transition so we do the copy
    * to the shadow unconditionally at the end of the subpass. For transfer
    * destinations, we can update it as part of the transfer op. For the other
    * layouts, we delay the copy until a transition into some other layout.
    */
   if (anv_surface_is_valid(&image->planes[plane].shadow_surface) &&
       vk_image_layout_stencil_write_optimal(initial_layout) &&
       !vk_image_layout_stencil_write_optimal(final_layout)) {
      anv_image_copy_to_shadow(cmd_buffer, image,
                               VK_IMAGE_ASPECT_STENCIL_BIT,
                               base_level, level_count,
                               base_layer, layer_count);
   }
#elif GFX_VER == 12
   const uint32_t plane =
      anv_image_aspect_to_plane(image, VK_IMAGE_ASPECT_STENCIL_BIT);
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_NONE)
      return;

   if ((initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ||
        initial_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) &&
       cmd_buffer->device->physical->has_implicit_ccs &&
       cmd_buffer->device->info.has_aux_map) {
      anv_image_init_aux_tt(cmd_buffer, image, VK_IMAGE_ASPECT_STENCIL_BIT,
                            base_level, level_count, base_layer, layer_count);

      /* If will_full_fast_clear is set, the caller promises to fast-clear the
       * largest portion of the specified range as it can.
       */
      if (will_full_fast_clear)
         return;

      for (uint32_t l = 0; l < level_count; l++) {
         const uint32_t level = base_level + l;
         const VkRect2D clear_rect = {
            .offset.x = 0,
            .offset.y = 0,
            .extent.width = anv_minify(image->vk.extent.width, level),
            .extent.height = anv_minify(image->vk.extent.height, level),
         };

         uint32_t aux_layers =
            anv_image_aux_layers(image, VK_IMAGE_ASPECT_STENCIL_BIT, level);
         uint32_t level_layer_count =
            MIN2(layer_count, aux_layers - base_layer);

         /* From Bspec's 3DSTATE_STENCIL_BUFFER_BODY > Stencil Compression
          * Enable:
          *
          *    "When enabled, Stencil Buffer needs to be initialized via
          *    stencil clear (HZ_OP) before any renderpass."
          */
         anv_image_hiz_clear(cmd_buffer, image, VK_IMAGE_ASPECT_STENCIL_BIT,
                             level, base_layer, level_layer_count,
                             clear_rect, 0 /* Stencil clear value */);
      }
   }
#endif
}

#define MI_PREDICATE_SRC0    0x2400
#define MI_PREDICATE_SRC1    0x2408
#define MI_PREDICATE_RESULT  0x2418

static void
set_image_compressed_bit(struct anv_cmd_buffer *cmd_buffer,
                         const struct anv_image *image,
                         VkImageAspectFlagBits aspect,
                         uint32_t level,
                         uint32_t base_layer, uint32_t layer_count,
                         bool compressed)
{
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);

   /* We only have compression tracking for CCS_E */
   if (image->planes[plane].aux_usage != ISL_AUX_USAGE_CCS_E)
      return;

   for (uint32_t a = 0; a < layer_count; a++) {
      uint32_t layer = base_layer + a;
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
         sdi.Address = anv_image_get_compression_state_addr(cmd_buffer->device,
                                                            image, aspect,
                                                            level, layer);
         sdi.ImmediateData = compressed ? UINT32_MAX : 0;
      }
   }
}

static void
set_image_fast_clear_state(struct anv_cmd_buffer *cmd_buffer,
                           const struct anv_image *image,
                           VkImageAspectFlagBits aspect,
                           enum anv_fast_clear_type fast_clear)
{
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
      sdi.Address = anv_image_get_fast_clear_type_addr(cmd_buffer->device,
                                                       image, aspect);
      sdi.ImmediateData = fast_clear;
   }

   /* Whenever we have fast-clear, we consider that slice to be compressed.
    * This makes building predicates much easier.
    */
   if (fast_clear != ANV_FAST_CLEAR_NONE)
      set_image_compressed_bit(cmd_buffer, image, aspect, 0, 0, 1, true);
}

/* This is only really practical on haswell and above because it requires
 * MI math in order to get it correct.
 */
#if GFX_VERx10 >= 75
static void
anv_cmd_compute_resolve_predicate(struct anv_cmd_buffer *cmd_buffer,
                                  const struct anv_image *image,
                                  VkImageAspectFlagBits aspect,
                                  uint32_t level, uint32_t array_layer,
                                  enum isl_aux_op resolve_op,
                                  enum anv_fast_clear_type fast_clear_supported)
{
   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   const struct mi_value fast_clear_type =
      mi_mem32(anv_image_get_fast_clear_type_addr(cmd_buffer->device,
                                                  image, aspect));

   if (resolve_op == ISL_AUX_OP_FULL_RESOLVE) {
      /* In this case, we're doing a full resolve which means we want the
       * resolve to happen if any compression (including fast-clears) is
       * present.
       *
       * In order to simplify the logic a bit, we make the assumption that,
       * if the first slice has been fast-cleared, it is also marked as
       * compressed.  See also set_image_fast_clear_state.
       */
      const struct mi_value compression_state =
         mi_mem32(anv_image_get_compression_state_addr(cmd_buffer->device,
                                                       image, aspect,
                                                       level, array_layer));
      mi_store(&b, mi_reg64(MI_PREDICATE_SRC0), compression_state);
      mi_store(&b, compression_state, mi_imm(0));

      if (level == 0 && array_layer == 0) {
         /* If the predicate is true, we want to write 0 to the fast clear type
          * and, if it's false, leave it alone.  We can do this by writing
          *
          * clear_type = clear_type & ~predicate;
          */
         struct mi_value new_fast_clear_type =
            mi_iand(&b, fast_clear_type,
                        mi_inot(&b, mi_reg64(MI_PREDICATE_SRC0)));
         mi_store(&b, fast_clear_type, new_fast_clear_type);
      }
   } else if (level == 0 && array_layer == 0) {
      /* In this case, we are doing a partial resolve to get rid of fast-clear
       * colors.  We don't care about the compression state but we do care
       * about how much fast clear is allowed by the final layout.
       */
      assert(resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE);
      assert(fast_clear_supported < ANV_FAST_CLEAR_ANY);

      /* We need to compute (fast_clear_supported < image->fast_clear) */
      struct mi_value pred =
         mi_ult(&b, mi_imm(fast_clear_supported), fast_clear_type);
      mi_store(&b, mi_reg64(MI_PREDICATE_SRC0), mi_value_ref(&b, pred));

      /* If the predicate is true, we want to write 0 to the fast clear type
       * and, if it's false, leave it alone.  We can do this by writing
       *
       * clear_type = clear_type & ~predicate;
       */
      struct mi_value new_fast_clear_type =
         mi_iand(&b, fast_clear_type, mi_inot(&b, pred));
      mi_store(&b, fast_clear_type, new_fast_clear_type);
   } else {
      /* In this case, we're trying to do a partial resolve on a slice that
       * doesn't have clear color.  There's nothing to do.
       */
      assert(resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE);
      return;
   }

   /* Set src1 to 0 and use a != condition */
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(0));

   anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
      mip.CombineOperation = COMBINE_SET;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }
}
#endif /* GFX_VERx10 >= 75 */

#if GFX_VER <= 8
static void
anv_cmd_simple_resolve_predicate(struct anv_cmd_buffer *cmd_buffer,
                                 const struct anv_image *image,
                                 VkImageAspectFlagBits aspect,
                                 uint32_t level, uint32_t array_layer,
                                 enum isl_aux_op resolve_op,
                                 enum anv_fast_clear_type fast_clear_supported)
{
   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   struct mi_value fast_clear_type_mem =
      mi_mem32(anv_image_get_fast_clear_type_addr(cmd_buffer->device,
                                                      image, aspect));

   /* This only works for partial resolves and only when the clear color is
    * all or nothing.  On the upside, this emits less command streamer code
    * and works on Ivybridge and Bay Trail.
    */
   assert(resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE);
   assert(fast_clear_supported != ANV_FAST_CLEAR_ANY);

   /* We don't support fast clears on anything other than the first slice. */
   if (level > 0 || array_layer > 0)
      return;

   /* On gfx8, we don't have a concept of default clear colors because we
    * can't sample from CCS surfaces.  It's enough to just load the fast clear
    * state into the predicate register.
    */
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC0), fast_clear_type_mem);
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(0));
   mi_store(&b, fast_clear_type_mem, mi_imm(0));

   anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
      mip.CombineOperation = COMBINE_SET;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }
}
#endif /* GFX_VER <= 8 */

static void
anv_cmd_predicated_ccs_resolve(struct anv_cmd_buffer *cmd_buffer,
                               const struct anv_image *image,
                               enum isl_format format,
                               struct isl_swizzle swizzle,
                               VkImageAspectFlagBits aspect,
                               uint32_t level, uint32_t array_layer,
                               enum isl_aux_op resolve_op,
                               enum anv_fast_clear_type fast_clear_supported)
{
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);

#if GFX_VER >= 9
   anv_cmd_compute_resolve_predicate(cmd_buffer, image,
                                     aspect, level, array_layer,
                                     resolve_op, fast_clear_supported);
#else /* GFX_VER <= 8 */
   anv_cmd_simple_resolve_predicate(cmd_buffer, image,
                                    aspect, level, array_layer,
                                    resolve_op, fast_clear_supported);
#endif

   /* CCS_D only supports full resolves and BLORP will assert on us if we try
    * to do a partial resolve on a CCS_D surface.
    */
   if (resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE &&
       image->planes[plane].aux_usage == ISL_AUX_USAGE_CCS_D)
      resolve_op = ISL_AUX_OP_FULL_RESOLVE;

   anv_image_ccs_op(cmd_buffer, image, format, swizzle, aspect,
                    level, array_layer, 1, resolve_op, NULL, true);
}

static void
anv_cmd_predicated_mcs_resolve(struct anv_cmd_buffer *cmd_buffer,
                               const struct anv_image *image,
                               enum isl_format format,
                               struct isl_swizzle swizzle,
                               VkImageAspectFlagBits aspect,
                               uint32_t array_layer,
                               enum isl_aux_op resolve_op,
                               enum anv_fast_clear_type fast_clear_supported)
{
   assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
   assert(resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE);

#if GFX_VERx10 >= 75
   anv_cmd_compute_resolve_predicate(cmd_buffer, image,
                                     aspect, 0, array_layer,
                                     resolve_op, fast_clear_supported);

   anv_image_mcs_op(cmd_buffer, image, format, swizzle, aspect,
                    array_layer, 1, resolve_op, NULL, true);
#else
   unreachable("MCS resolves are unsupported on Ivybridge and Bay Trail");
#endif
}

void
genX(cmd_buffer_mark_image_written)(struct anv_cmd_buffer *cmd_buffer,
                                    const struct anv_image *image,
                                    VkImageAspectFlagBits aspect,
                                    enum isl_aux_usage aux_usage,
                                    uint32_t level,
                                    uint32_t base_layer,
                                    uint32_t layer_count)
{
   /* The aspect must be exactly one of the image aspects. */
   assert(util_bitcount(aspect) == 1 && (aspect & image->vk.aspects));

   /* The only compression types with more than just fast-clears are MCS,
    * CCS_E, and HiZ.  With HiZ we just trust the layout and don't actually
    * track the current fast-clear and compression state.  This leaves us
    * with just MCS and CCS_E.
    */
   if (aux_usage != ISL_AUX_USAGE_CCS_E &&
       aux_usage != ISL_AUX_USAGE_MCS)
      return;

   set_image_compressed_bit(cmd_buffer, image, aspect,
                            level, base_layer, layer_count, true);
}

static void
init_fast_clear_color(struct anv_cmd_buffer *cmd_buffer,
                      const struct anv_image *image,
                      VkImageAspectFlagBits aspect)
{
   assert(cmd_buffer && image);
   assert(image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV);

   set_image_fast_clear_state(cmd_buffer, image, aspect,
                              ANV_FAST_CLEAR_NONE);

   /* Initialize the struct fields that are accessed for fast-clears so that
    * the HW restrictions on the field values are satisfied.
    */
   struct anv_address addr =
      anv_image_get_clear_color_addr(cmd_buffer->device, image, aspect);

   if (GFX_VER >= 9) {
      const struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;
      const unsigned num_dwords = GFX_VER >= 10 ?
                                  isl_dev->ss.clear_color_state_size / 4 :
                                  isl_dev->ss.clear_value_size / 4;
      for (unsigned i = 0; i < num_dwords; i++) {
         anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
            sdi.Address = addr;
            sdi.Address.offset += i * 4;
            sdi.ImmediateData = 0;
         }
      }
   } else {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
         sdi.Address = addr;
         if (GFX_VERx10 >= 75) {
            /* Pre-SKL, the dword containing the clear values also contains
             * other fields, so we need to initialize those fields to match the
             * values that would be in a color attachment.
             */
            sdi.ImmediateData = ISL_CHANNEL_SELECT_RED   << 25 |
                                ISL_CHANNEL_SELECT_GREEN << 22 |
                                ISL_CHANNEL_SELECT_BLUE  << 19 |
                                ISL_CHANNEL_SELECT_ALPHA << 16;
         } else if (GFX_VER == 7) {
            /* On IVB, the dword containing the clear values also contains
             * other fields that must be zero or can be zero.
             */
            sdi.ImmediateData = 0;
         }
      }
   }
}

/* Copy the fast-clear value dword(s) between a surface state object and an
 * image's fast clear state buffer.
 */
static void
genX(copy_fast_clear_dwords)(struct anv_cmd_buffer *cmd_buffer,
                             struct anv_state surface_state,
                             const struct anv_image *image,
                             VkImageAspectFlagBits aspect,
                             bool copy_from_surface_state)
{
   assert(cmd_buffer && image);
   assert(image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV);

   struct anv_address ss_clear_addr = {
      .bo = cmd_buffer->device->surface_state_pool.block_pool.bo,
      .offset = surface_state.offset +
                cmd_buffer->device->isl_dev.ss.clear_value_offset,
   };
   const struct anv_address entry_addr =
      anv_image_get_clear_color_addr(cmd_buffer->device, image, aspect);
   unsigned copy_size = cmd_buffer->device->isl_dev.ss.clear_value_size;

#if GFX_VER == 7
   /* On gfx7, the combination of commands used here(MI_LOAD_REGISTER_MEM
    * and MI_STORE_REGISTER_MEM) can cause GPU hangs if any rendering is
    * in-flight when they are issued even if the memory touched is not
    * currently active for rendering.  The weird bit is that it is not the
    * MI_LOAD/STORE_REGISTER_MEM commands which hang but rather the in-flight
    * rendering hangs such that the next stalling command after the
    * MI_LOAD/STORE_REGISTER_MEM commands will catch the hang.
    *
    * It is unclear exactly why this hang occurs.  Both MI commands come with
    * warnings about the 3D pipeline but that doesn't seem to fully explain
    * it.  My (Jason's) best theory is that it has something to do with the
    * fact that we're using a GPU state register as our temporary and that
    * something with reading/writing it is causing problems.
    *
    * In order to work around this issue, we emit a PIPE_CONTROL with the
    * command streamer stall bit set.
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_CS_STALL_BIT,
                             "after copy_fast_clear_dwords. Avoid potential hang");
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);
#endif

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   if (copy_from_surface_state) {
      mi_memcpy(&b, entry_addr, ss_clear_addr, copy_size);
   } else {
      mi_memcpy(&b, ss_clear_addr, entry_addr, copy_size);

      /* Updating a surface state object may require that the state cache be
       * invalidated. From the SKL PRM, Shared Functions -> State -> State
       * Caching:
       *
       *    Whenever the RENDER_SURFACE_STATE object in memory pointed to by
       *    the Binding Table Pointer (BTP) and Binding Table Index (BTI) is
       *    modified [...], the L1 state cache must be invalidated to ensure
       *    the new surface or sampler state is fetched from system memory.
       *
       * In testing, SKL doesn't actually seem to need this, but HSW does.
       */
      anv_add_pending_pipe_bits(cmd_buffer,
                                ANV_PIPE_STATE_CACHE_INVALIDATE_BIT,
                                "after copy_fast_clear_dwords surface state update");
   }
}

/**
 * @brief Transitions a color buffer from one layout to another.
 *
 * See section 6.1.1. Image Layout Transitions of the Vulkan 1.0.50 spec for
 * more information.
 *
 * @param level_count VK_REMAINING_MIP_LEVELS isn't supported.
 * @param layer_count VK_REMAINING_ARRAY_LAYERS isn't supported. For 3D images,
 *                    this represents the maximum layers to transition at each
 *                    specified miplevel.
 */
static void
transition_color_buffer(struct anv_cmd_buffer *cmd_buffer,
                        const struct anv_image *image,
                        VkImageAspectFlagBits aspect,
                        const uint32_t base_level, uint32_t level_count,
                        uint32_t base_layer, uint32_t layer_count,
                        VkImageLayout initial_layout,
                        VkImageLayout final_layout,
                        uint64_t src_queue_family,
                        uint64_t dst_queue_family,
                        bool will_full_fast_clear)
{
   struct anv_device *device = cmd_buffer->device;
   const struct intel_device_info *devinfo = &device->info;
   /* Validate the inputs. */
   assert(cmd_buffer);
   assert(image && image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV);
   /* These values aren't supported for simplicity's sake. */
   assert(level_count != VK_REMAINING_MIP_LEVELS &&
          layer_count != VK_REMAINING_ARRAY_LAYERS);
   /* Ensure the subresource range is valid. */
   UNUSED uint64_t last_level_num = base_level + level_count;
   const uint32_t max_depth = anv_minify(image->vk.extent.depth, base_level);
   UNUSED const uint32_t image_layers = MAX2(image->vk.array_layers, max_depth);
   assert((uint64_t)base_layer + layer_count  <= image_layers);
   assert(last_level_num <= image->vk.mip_levels);
   /* If there is a layout transfer, the final layout cannot be undefined or
    * preinitialized (VUID-VkImageMemoryBarrier-newLayout-01198).
    */
   assert(initial_layout == final_layout ||
          (final_layout != VK_IMAGE_LAYOUT_UNDEFINED &&
           final_layout != VK_IMAGE_LAYOUT_PREINITIALIZED));
   const struct isl_drm_modifier_info *isl_mod_info =
      image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT
      ? isl_drm_modifier_get_info(image->vk.drm_format_mod)
      : NULL;

   const bool src_queue_external =
      src_queue_family == VK_QUEUE_FAMILY_FOREIGN_EXT ||
      src_queue_family == VK_QUEUE_FAMILY_EXTERNAL;

   const bool dst_queue_external =
      dst_queue_family == VK_QUEUE_FAMILY_FOREIGN_EXT ||
      dst_queue_family == VK_QUEUE_FAMILY_EXTERNAL;

   /* Simultaneous acquire and release on external queues is illegal. */
   assert(!src_queue_external || !dst_queue_external);

   /* Ownership transition on an external queue requires special action if the
    * image has a DRM format modifier because we store image data in
    * a driver-private bo which is inaccessible to the external queue.
    */
   const bool mod_acquire =
      src_queue_external &&
      image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;

   const bool mod_release =
      dst_queue_external &&
      image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;

   if (initial_layout == final_layout &&
       !mod_acquire && !mod_release) {
      /* No work is needed. */
       return;
   }

   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);

   if (anv_surface_is_valid(&image->planes[plane].shadow_surface) &&
       final_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      /* This surface is a linear compressed image with a tiled shadow surface
       * for texturing.  The client is about to use it in READ_ONLY_OPTIMAL so
       * we need to ensure the shadow copy is up-to-date.
       */
      assert(image->vk.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);
      assert(image->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT);
      assert(image->planes[plane].primary_surface.isl.tiling == ISL_TILING_LINEAR);
      assert(image->planes[plane].shadow_surface.isl.tiling != ISL_TILING_LINEAR);
      assert(isl_format_is_compressed(image->planes[plane].primary_surface.isl.format));
      assert(plane == 0);
      anv_image_copy_to_shadow(cmd_buffer, image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               base_level, level_count,
                               base_layer, layer_count);
   }

   if (base_layer >= anv_image_aux_layers(image, aspect, base_level))
      return;

   assert(image->planes[plane].primary_surface.isl.tiling != ISL_TILING_LINEAR);

   /* The following layouts are equivalent for non-linear images. */
   const bool initial_layout_undefined =
      initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ||
      initial_layout == VK_IMAGE_LAYOUT_PREINITIALIZED;

   bool must_init_fast_clear_state = false;
   bool must_init_aux_surface = false;

   if (initial_layout_undefined) {
      /* The subresource may have been aliased and populated with arbitrary
       * data.
       */
      must_init_fast_clear_state = true;
      must_init_aux_surface = true;
   } else if (mod_acquire) {
      /* The fast clear state lives in a driver-private bo, and therefore the
       * external/foreign queue is unaware of it.
       *
       * If this is the first time we are accessing the image, then the fast
       * clear state is uninitialized.
       *
       * If this is NOT the first time we are accessing the image, then the fast
       * clear state may still be valid and correct due to the resolve during
       * our most recent ownership release.  However, we do not track the aux
       * state with MI stores, and therefore must assume the worst-case: that
       * this is the first time we are accessing the image.
       */
      assert(image->planes[plane].fast_clear_memory_range.binding ==
              ANV_IMAGE_MEMORY_BINDING_PRIVATE);
      must_init_fast_clear_state = true;

      if (image->planes[plane].aux_surface.memory_range.binding ==
          ANV_IMAGE_MEMORY_BINDING_PRIVATE) {
         assert(isl_mod_info->aux_usage == ISL_AUX_USAGE_NONE);

         /* The aux surface, like the fast clear state, lives in
          * a driver-private bo.  We must initialize the aux surface for the
          * same reasons we must initialize the fast clear state.
          */
         must_init_aux_surface = true;
      } else {
         assert(isl_mod_info->aux_usage != ISL_AUX_USAGE_NONE);

         /* The aux surface, unlike the fast clear state, lives in
          * application-visible VkDeviceMemory and is shared with the
          * external/foreign queue. Therefore, when we acquire ownership of the
          * image with a defined VkImageLayout, the aux surface is valid and has
          * the aux state required by the modifier.
          */
         must_init_aux_surface = false;
      }
   }

#if GFX_VER == 12
   /* We do not yet support modifiers with aux on gen12. */
   assert(image->vk.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);

   if (initial_layout_undefined) {
      if (device->physical->has_implicit_ccs && devinfo->has_aux_map) {
         anv_image_init_aux_tt(cmd_buffer, image, aspect,
                               base_level, level_count,
                               base_layer, layer_count);
      }
   }
#else
   assert(!(device->physical->has_implicit_ccs && devinfo->has_aux_map));
#endif

   if (must_init_fast_clear_state) {
      if (base_level == 0 && base_layer == 0)
         init_fast_clear_color(cmd_buffer, image, aspect);
   }

   if (must_init_aux_surface) {
      assert(must_init_fast_clear_state);

      /* Initialize the aux buffers to enable correct rendering.  In order to
       * ensure that things such as storage images work correctly, aux buffers
       * need to be initialized to valid data.
       *
       * Having an aux buffer with invalid data is a problem for two reasons:
       *
       *  1) Having an invalid value in the buffer can confuse the hardware.
       *     For instance, with CCS_E on SKL, a two-bit CCS value of 2 is
       *     invalid and leads to the hardware doing strange things.  It
       *     doesn't hang as far as we can tell but rendering corruption can
       *     occur.
       *
       *  2) If this transition is into the GENERAL layout and we then use the
       *     image as a storage image, then we must have the aux buffer in the
       *     pass-through state so that, if we then go to texture from the
       *     image, we get the results of our storage image writes and not the
       *     fast clear color or other random data.
       *
       * For CCS both of the problems above are real demonstrable issues.  In
       * that case, the only thing we can do is to perform an ambiguate to
       * transition the aux surface into the pass-through state.
       *
       * For MCS, (2) is never an issue because we don't support multisampled
       * storage images.  In theory, issue (1) is a problem with MCS but we've
       * never seen it in the wild.  For 4x and 16x, all bit patters could, in
       * theory, be interpreted as something but we don't know that all bit
       * patterns are actually valid.  For 2x and 8x, you could easily end up
       * with the MCS referring to an invalid plane because not all bits of
       * the MCS value are actually used.  Even though we've never seen issues
       * in the wild, it's best to play it safe and initialize the MCS.  We
       * can use a fast-clear for MCS because we only ever touch from render
       * and texture (no image load store).
       */
      if (image->vk.samples == 1) {
         for (uint32_t l = 0; l < level_count; l++) {
            const uint32_t level = base_level + l;

            uint32_t aux_layers = anv_image_aux_layers(image, aspect, level);
            if (base_layer >= aux_layers)
               break; /* We will only get fewer layers as level increases */
            uint32_t level_layer_count =
               MIN2(layer_count, aux_layers - base_layer);

            /* If will_full_fast_clear is set, the caller promises to
             * fast-clear the largest portion of the specified range as it can.
             * For color images, that means only the first LOD and array slice.
             */
            if (level == 0 && base_layer == 0 && will_full_fast_clear) {
               base_layer++;
               level_layer_count--;
               if (level_layer_count == 0)
                  continue;
            }

            anv_image_ccs_op(cmd_buffer, image,
                             image->planes[plane].primary_surface.isl.format,
                             ISL_SWIZZLE_IDENTITY,
                             aspect, level, base_layer, level_layer_count,
                             ISL_AUX_OP_AMBIGUATE, NULL, false);

            if (image->planes[plane].aux_usage == ISL_AUX_USAGE_CCS_E) {
               set_image_compressed_bit(cmd_buffer, image, aspect,
                                        level, base_layer, level_layer_count,
                                        false);
            }
         }
      } else {
         if (image->vk.samples == 4 || image->vk.samples == 16) {
            anv_perf_warn(VK_LOG_OBJS(&image->vk.base),
                          "Doing a potentially unnecessary fast-clear to "
                          "define an MCS buffer.");
         }

         /* If will_full_fast_clear is set, the caller promises to fast-clear
          * the largest portion of the specified range as it can.
          */
         if (will_full_fast_clear)
            return;

         assert(base_level == 0 && level_count == 1);
         anv_image_mcs_op(cmd_buffer, image,
                          image->planes[plane].primary_surface.isl.format,
                          ISL_SWIZZLE_IDENTITY,
                          aspect, base_layer, layer_count,
                          ISL_AUX_OP_FAST_CLEAR, NULL, false);
      }
      return;
   }

   enum isl_aux_usage initial_aux_usage =
      anv_layout_to_aux_usage(devinfo, image, aspect, 0, initial_layout);
   enum isl_aux_usage final_aux_usage =
      anv_layout_to_aux_usage(devinfo, image, aspect, 0, final_layout);
   enum anv_fast_clear_type initial_fast_clear =
      anv_layout_to_fast_clear_type(devinfo, image, aspect, initial_layout);
   enum anv_fast_clear_type final_fast_clear =
      anv_layout_to_fast_clear_type(devinfo, image, aspect, final_layout);

   /* We must override the anv_layout_to_* functions because they are unaware of
    * acquire/release direction.
    */
   if (mod_acquire) {
      initial_aux_usage = isl_mod_info->aux_usage;
      initial_fast_clear = isl_mod_info->supports_clear_color ?
         initial_fast_clear : ANV_FAST_CLEAR_NONE;
   } else if (mod_release) {
      final_aux_usage = isl_mod_info->aux_usage;
      final_fast_clear = isl_mod_info->supports_clear_color ?
         final_fast_clear : ANV_FAST_CLEAR_NONE;
   }

   /* The current code assumes that there is no mixing of CCS_E and CCS_D.
    * We can handle transitions between CCS_D/E to and from NONE.  What we
    * don't yet handle is switching between CCS_E and CCS_D within a given
    * image.  Doing so in a performant way requires more detailed aux state
    * tracking such as what is done in i965.  For now, just assume that we
    * only have one type of compression.
    */
   assert(initial_aux_usage == ISL_AUX_USAGE_NONE ||
          final_aux_usage == ISL_AUX_USAGE_NONE ||
          initial_aux_usage == final_aux_usage);

   /* If initial aux usage is NONE, there is nothing to resolve */
   if (initial_aux_usage == ISL_AUX_USAGE_NONE)
      return;

   enum isl_aux_op resolve_op = ISL_AUX_OP_NONE;

   /* If the initial layout supports more fast clear than the final layout
    * then we need at least a partial resolve.
    */
   if (final_fast_clear < initial_fast_clear)
      resolve_op = ISL_AUX_OP_PARTIAL_RESOLVE;

   if (initial_aux_usage == ISL_AUX_USAGE_CCS_E &&
       final_aux_usage != ISL_AUX_USAGE_CCS_E)
      resolve_op = ISL_AUX_OP_FULL_RESOLVE;

   if (resolve_op == ISL_AUX_OP_NONE)
      return;

   /* Perform a resolve to synchronize data between the main and aux buffer.
    * Before we begin, we must satisfy the cache flushing requirement specified
    * in the Sky Lake PRM Vol. 7, "MCS Buffer for Render Target(s)":
    *
    *    Any transition from any value in {Clear, Render, Resolve} to a
    *    different value in {Clear, Render, Resolve} requires end of pipe
    *    synchronization.
    *
    * We perform a flush of the write cache before and after the clear and
    * resolve operations to meet this requirement.
    *
    * Unlike other drawing, fast clear operations are not properly
    * synchronized. The first PIPE_CONTROL here likely ensures that the
    * contents of the previous render or clear hit the render target before we
    * resolve and the second likely ensures that the resolve is complete before
    * we do any more rendering or clearing.
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
                             ANV_PIPE_END_OF_PIPE_SYNC_BIT,
                             "after transition RT");

   for (uint32_t l = 0; l < level_count; l++) {
      uint32_t level = base_level + l;

      uint32_t aux_layers = anv_image_aux_layers(image, aspect, level);
      if (base_layer >= aux_layers)
         break; /* We will only get fewer layers as level increases */
      uint32_t level_layer_count =
         MIN2(layer_count, aux_layers - base_layer);

      for (uint32_t a = 0; a < level_layer_count; a++) {
         uint32_t array_layer = base_layer + a;

         /* If will_full_fast_clear is set, the caller promises to fast-clear
          * the largest portion of the specified range as it can.  For color
          * images, that means only the first LOD and array slice.
          */
         if (level == 0 && array_layer == 0 && will_full_fast_clear)
            continue;

         if (image->vk.samples == 1) {
            anv_cmd_predicated_ccs_resolve(cmd_buffer, image,
                                           image->planes[plane].primary_surface.isl.format,
                                           ISL_SWIZZLE_IDENTITY,
                                           aspect, level, array_layer, resolve_op,
                                           final_fast_clear);
         } else {
            /* We only support fast-clear on the first layer so partial
             * resolves should not be used on other layers as they will use
             * the clear color stored in memory that is only valid for layer0.
             */
            if (resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE &&
                array_layer != 0)
               continue;

            anv_cmd_predicated_mcs_resolve(cmd_buffer, image,
                                           image->planes[plane].primary_surface.isl.format,
                                           ISL_SWIZZLE_IDENTITY,
                                           aspect, array_layer, resolve_op,
                                           final_fast_clear);
         }
      }
   }

   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
                             ANV_PIPE_END_OF_PIPE_SYNC_BIT,
                             "after transition RT");
}

static VkResult
genX(cmd_buffer_setup_attachments)(struct anv_cmd_buffer *cmd_buffer,
                                   const struct anv_render_pass *pass,
                                   const struct anv_framebuffer *framebuffer,
                                   const VkRenderPassBeginInfo *begin)
{
   struct anv_cmd_state *state = &cmd_buffer->state;

   vk_free(&cmd_buffer->pool->alloc, state->attachments);

   if (pass->attachment_count > 0) {
      state->attachments = vk_zalloc(&cmd_buffer->pool->alloc,
                                     pass->attachment_count *
                                          sizeof(state->attachments[0]),
                                     8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
      if (state->attachments == NULL) {
         /* Propagate VK_ERROR_OUT_OF_HOST_MEMORY to vkEndCommandBuffer */
         return anv_batch_set_error(&cmd_buffer->batch,
                                    VK_ERROR_OUT_OF_HOST_MEMORY);
      }
   } else {
      state->attachments = NULL;
   }

   const VkRenderPassAttachmentBeginInfoKHR *attach_begin =
      vk_find_struct_const(begin, RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR);
   if (begin && !attach_begin)
      assert(pass->attachment_count == framebuffer->attachment_count);

   for (uint32_t i = 0; i < pass->attachment_count; ++i) {
      if (attach_begin && attach_begin->attachmentCount != 0) {
         assert(attach_begin->attachmentCount == pass->attachment_count);
         ANV_FROM_HANDLE(anv_image_view, iview, attach_begin->pAttachments[i]);
         state->attachments[i].image_view = iview;
      } else if (framebuffer && i < framebuffer->attachment_count) {
         state->attachments[i].image_view = framebuffer->attachments[i];
      } else {
         state->attachments[i].image_view = NULL;
      }
   }

   if (begin) {
      for (uint32_t i = 0; i < pass->attachment_count; ++i) {
         const struct anv_render_pass_attachment *pass_att = &pass->attachments[i];
         struct anv_attachment_state *att_state = &state->attachments[i];
         VkImageAspectFlags att_aspects = vk_format_aspects(pass_att->format);
         VkImageAspectFlags clear_aspects = 0;
         VkImageAspectFlags load_aspects = 0;

         if (att_aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
            /* color attachment */
            if (pass_att->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
               clear_aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
            } else if (pass_att->load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
               load_aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
         } else {
            /* depthstencil attachment */
            if (att_aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
               if (pass_att->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                  clear_aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
               } else if (pass_att->load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
                  load_aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
               }
            }
            if (att_aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
               if (pass_att->stencil_load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                  clear_aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
               } else if (pass_att->stencil_load_op == VK_ATTACHMENT_LOAD_OP_LOAD) {
                  load_aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
               }
            }
         }

         att_state->current_layout = pass_att->initial_layout;
         att_state->current_stencil_layout = pass_att->stencil_initial_layout;
         att_state->pending_clear_aspects = clear_aspects;
         att_state->pending_load_aspects = load_aspects;
         if (clear_aspects)
            att_state->clear_value = begin->pClearValues[i];

         struct anv_image_view *iview = state->attachments[i].image_view;

         const uint32_t num_layers = iview->planes[0].isl.array_len;
         att_state->pending_clear_views = (1 << num_layers) - 1;

         /* This will be initialized after the first subpass transition. */
         att_state->aux_usage = ISL_AUX_USAGE_NONE;

         att_state->fast_clear = false;
         if (clear_aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
            assert(clear_aspects == VK_IMAGE_ASPECT_COLOR_BIT);
            att_state->fast_clear =
               anv_can_fast_clear_color_view(cmd_buffer->device, iview,
                                             pass_att->first_subpass_layout,
                                             vk_to_isl_color(att_state->clear_value.color),
                                             framebuffer->layers,
                                             begin->renderArea);
         } else if (clear_aspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
                                     VK_IMAGE_ASPECT_STENCIL_BIT)) {
            att_state->fast_clear =
               anv_can_hiz_clear_ds_view(cmd_buffer->device, iview,
                                         pass_att->first_subpass_layout,
                                         clear_aspects,
                                         att_state->clear_value.depthStencil.depth,
                                         begin->renderArea);
         }
      }
   }

   return VK_SUCCESS;
}

/**
 * Setup anv_cmd_state::attachments for vkCmdBeginRenderPass.
 */
static VkResult
genX(cmd_buffer_alloc_att_surf_states)(struct anv_cmd_buffer *cmd_buffer,
                                       const struct anv_render_pass *pass,
                                       const struct anv_subpass *subpass)
{
   const struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;
   struct anv_cmd_state *state = &cmd_buffer->state;

   /* Reserve one for the NULL state. */
   unsigned num_states = 1;
   for (uint32_t i = 0; i < subpass->attachment_count; i++) {
      uint32_t att = subpass->attachments[i].attachment;
      if (att == VK_ATTACHMENT_UNUSED)
         continue;

      assert(att < pass->attachment_count);
      if (!vk_format_is_color(pass->attachments[att].format))
         continue;

      const VkImageUsageFlagBits att_usage = subpass->attachments[i].usage;
      assert(util_bitcount(att_usage) == 1);

      if (att_usage == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ||
          att_usage == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
         num_states++;
   }

   const uint32_t ss_stride = align_u32(isl_dev->ss.size, isl_dev->ss.align);
   state->attachment_states =
      anv_state_stream_alloc(&cmd_buffer->surface_state_stream,
                             num_states * ss_stride, isl_dev->ss.align);
   if (state->attachment_states.map == NULL) {
      return anv_batch_set_error(&cmd_buffer->batch,
                                 VK_ERROR_OUT_OF_DEVICE_MEMORY);
   }

   struct anv_state next_state = state->attachment_states;
   next_state.alloc_size = isl_dev->ss.size;

   state->null_surface_state = next_state;
   next_state.offset += ss_stride;
   next_state.map += ss_stride;

   for (uint32_t i = 0; i < subpass->attachment_count; i++) {
      uint32_t att = subpass->attachments[i].attachment;
      if (att == VK_ATTACHMENT_UNUSED)
         continue;

      assert(att < pass->attachment_count);
      if (!vk_format_is_color(pass->attachments[att].format))
         continue;

      const VkImageUsageFlagBits att_usage = subpass->attachments[i].usage;
      assert(util_bitcount(att_usage) == 1);

      if (att_usage == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
         state->attachments[att].color.state = next_state;
      else if (att_usage == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
         state->attachments[att].input.state = next_state;
      else
         continue;

      next_state.offset += ss_stride;
      next_state.map += ss_stride;
   }

   assert(next_state.offset == state->attachment_states.offset +
                               state->attachment_states.alloc_size);

   return VK_SUCCESS;
}

VkResult
genX(BeginCommandBuffer)(
    VkCommandBuffer                             commandBuffer,
    const VkCommandBufferBeginInfo*             pBeginInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   /* If this is the first vkBeginCommandBuffer, we must *initialize* the
    * command buffer's state. Otherwise, we must *reset* its state. In both
    * cases we reset it.
    *
    * From the Vulkan 1.0 spec:
    *
    *    If a command buffer is in the executable state and the command buffer
    *    was allocated from a command pool with the
    *    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag set, then
    *    vkBeginCommandBuffer implicitly resets the command buffer, behaving
    *    as if vkResetCommandBuffer had been called with
    *    VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT not set. It then puts
    *    the command buffer in the recording state.
    */
   anv_cmd_buffer_reset(cmd_buffer);

   cmd_buffer->usage_flags = pBeginInfo->flags;

   /* VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT must be ignored for
    * primary level command buffers.
    *
    * From the Vulkan 1.0 spec:
    *
    *    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a
    *    secondary command buffer is considered to be entirely inside a render
    *    pass. If this is a primary command buffer, then this bit is ignored.
    */
   if (cmd_buffer->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
      cmd_buffer->usage_flags &= ~VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

   genX(cmd_buffer_emit_state_base_address)(cmd_buffer);

   /* We sometimes store vertex data in the dynamic state buffer for blorp
    * operations and our dynamic state stream may re-use data from previous
    * command buffers.  In order to prevent stale cache data, we flush the VF
    * cache.  We could do this on every blorp call but that's not really
    * needed as all of the data will get written by the CPU prior to the GPU
    * executing anything.  The chances are fairly high that they will use
    * blorp at least once per primary command buffer so it shouldn't be
    * wasted.
    *
    * There is also a workaround on gfx8 which requires us to invalidate the
    * VF cache occasionally.  It's easier if we can assume we start with a
    * fresh cache (See also genX(cmd_buffer_set_binding_for_gfx8_vb_flush).)
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_VF_CACHE_INVALIDATE_BIT,
                             "new cmd buffer");

   /* Re-emit the aux table register in every command buffer.  This way we're
    * ensured that we have the table even if this command buffer doesn't
    * initialize any images.
    */
   if (cmd_buffer->device->info.has_aux_map) {
      anv_add_pending_pipe_bits(cmd_buffer,
                                ANV_PIPE_AUX_TABLE_INVALIDATE_BIT,
                                "new cmd buffer with aux-tt");
   }

   /* We send an "Indirect State Pointers Disable" packet at
    * EndCommandBuffer, so all push contant packets are ignored during a
    * context restore. Documentation says after that command, we need to
    * emit push constants again before any rendering operation. So we
    * flag them dirty here to make sure they get emitted.
    */
   cmd_buffer->state.push_constants_dirty |= VK_SHADER_STAGE_ALL_GRAPHICS;

   VkResult result = VK_SUCCESS;
   if (cmd_buffer->usage_flags &
       VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
      assert(pBeginInfo->pInheritanceInfo);
      ANV_FROM_HANDLE(anv_render_pass, pass,
                      pBeginInfo->pInheritanceInfo->renderPass);
      struct anv_subpass *subpass =
         &pass->subpasses[pBeginInfo->pInheritanceInfo->subpass];
      ANV_FROM_HANDLE(anv_framebuffer, framebuffer,
                      pBeginInfo->pInheritanceInfo->framebuffer);

      cmd_buffer->state.pass = pass;
      cmd_buffer->state.subpass = subpass;

      /* This is optional in the inheritance info. */
      cmd_buffer->state.framebuffer = framebuffer;

      result = genX(cmd_buffer_setup_attachments)(cmd_buffer, pass,
                                                  framebuffer, NULL);
      if (result != VK_SUCCESS)
         return result;

      result = genX(cmd_buffer_alloc_att_surf_states)(cmd_buffer, pass,
                                                      subpass);
      if (result != VK_SUCCESS)
         return result;

      /* Record that HiZ is enabled if we can. */
      if (cmd_buffer->state.framebuffer) {
         const struct anv_image_view * const iview =
            anv_cmd_buffer_get_depth_stencil_view(cmd_buffer);

         if (iview && (iview->image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT)) {
            VkImageLayout layout =
                cmd_buffer->state.subpass->depth_stencil_attachment->layout;

            enum isl_aux_usage aux_usage =
               anv_layout_to_aux_usage(&cmd_buffer->device->info, iview->image,
                                       VK_IMAGE_ASPECT_DEPTH_BIT,
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                       layout);

            cmd_buffer->state.hiz_enabled = isl_aux_usage_has_hiz(aux_usage);
         }
      }

      cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_RENDER_TARGETS;
   }

#if GFX_VERx10 >= 75
   if (cmd_buffer->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      const VkCommandBufferInheritanceConditionalRenderingInfoEXT *conditional_rendering_info =
         vk_find_struct_const(pBeginInfo->pInheritanceInfo->pNext, COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT);

      /* If secondary buffer supports conditional rendering
       * we should emit commands as if conditional rendering is enabled.
       */
      cmd_buffer->state.conditional_render_enabled =
         conditional_rendering_info && conditional_rendering_info->conditionalRenderingEnable;
   }
#endif

   return result;
}

/* From the PRM, Volume 2a:
 *
 *    "Indirect State Pointers Disable
 *
 *    At the completion of the post-sync operation associated with this pipe
 *    control packet, the indirect state pointers in the hardware are
 *    considered invalid; the indirect pointers are not saved in the context.
 *    If any new indirect state commands are executed in the command stream
 *    while the pipe control is pending, the new indirect state commands are
 *    preserved.
 *
 *    [DevIVB+]: Using Invalidate State Pointer (ISP) only inhibits context
 *    restoring of Push Constant (3DSTATE_CONSTANT_*) commands. Push Constant
 *    commands are only considered as Indirect State Pointers. Once ISP is
 *    issued in a context, SW must initialize by programming push constant
 *    commands for all the shaders (at least to zero length) before attempting
 *    any rendering operation for the same context."
 *
 * 3DSTATE_CONSTANT_* packets are restored during a context restore,
 * even though they point to a BO that has been already unreferenced at
 * the end of the previous batch buffer. This has been fine so far since
 * we are protected by these scratch page (every address not covered by
 * a BO should be pointing to the scratch page). But on CNL, it is
 * causing a GPU hang during context restore at the 3DSTATE_CONSTANT_*
 * instruction.
 *
 * The flag "Indirect State Pointers Disable" in PIPE_CONTROL tells the
 * hardware to ignore previous 3DSTATE_CONSTANT_* packets during a
 * context restore, so the mentioned hang doesn't happen. However,
 * software must program push constant commands for all stages prior to
 * rendering anything. So we flag them dirty in BeginCommandBuffer.
 *
 * Finally, we also make sure to stall at pixel scoreboard to make sure the
 * constants have been loaded into the EUs prior to disable the push constants
 * so that it doesn't hang a previous 3DPRIMITIVE.
 */
static void
emit_isp_disable(struct anv_cmd_buffer *cmd_buffer)
{
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
         pc.StallAtPixelScoreboard = true;
         pc.CommandStreamerStallEnable = true;
         anv_debug_dump_pc(pc);
   }
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
         pc.IndirectStatePointersDisable = true;
         pc.CommandStreamerStallEnable = true;
         anv_debug_dump_pc(pc);
   }
}

VkResult
genX(EndCommandBuffer)(
    VkCommandBuffer                             commandBuffer)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return cmd_buffer->batch.status;

   anv_measure_endcommandbuffer(cmd_buffer);

   /* We want every command buffer to start with the PMA fix in a known state,
    * so we disable it at the end of the command buffer.
    */
   genX(cmd_buffer_enable_pma_fix)(cmd_buffer, false);

   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   emit_isp_disable(cmd_buffer);

   anv_cmd_buffer_end_batch_buffer(cmd_buffer);

   return VK_SUCCESS;
}

void
genX(CmdExecuteCommands)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCmdBuffers)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, primary, commandBuffer);

   assert(primary->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

   if (anv_batch_has_error(&primary->batch))
      return;

   /* The secondary command buffers will assume that the PMA fix is disabled
    * when they begin executing.  Make sure this is true.
    */
   genX(cmd_buffer_enable_pma_fix)(primary, false);

   /* The secondary command buffer doesn't know which textures etc. have been
    * flushed prior to their execution.  Apply those flushes now.
    */
   genX(cmd_buffer_apply_pipe_flushes)(primary);

   for (uint32_t i = 0; i < commandBufferCount; i++) {
      ANV_FROM_HANDLE(anv_cmd_buffer, secondary, pCmdBuffers[i]);

      assert(secondary->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      assert(!anv_batch_has_error(&secondary->batch));

#if GFX_VERx10 >= 75
      if (secondary->state.conditional_render_enabled) {
         if (!primary->state.conditional_render_enabled) {
            /* Secondary buffer is constructed as if it will be executed
             * with conditional rendering, we should satisfy this dependency
             * regardless of conditional rendering being enabled in primary.
             */
            struct mi_builder b;
            mi_builder_init(&b, &primary->device->info, &primary->batch);
            mi_store(&b, mi_reg64(ANV_PREDICATE_RESULT_REG),
                         mi_imm(UINT64_MAX));
         }
      }
#endif

      if (secondary->usage_flags &
          VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
         /* If we're continuing a render pass from the primary, we need to
          * copy the surface states for the current subpass into the storage
          * we allocated for them in BeginCommandBuffer.
          */
         struct anv_bo *ss_bo =
            primary->device->surface_state_pool.block_pool.bo;
         struct anv_state src_state = primary->state.attachment_states;
         struct anv_state dst_state = secondary->state.attachment_states;
         assert(src_state.alloc_size == dst_state.alloc_size);

         genX(cmd_buffer_so_memcpy)(primary,
                                    (struct anv_address) {
                                       .bo = ss_bo,
                                       .offset = dst_state.offset,
                                    },
                                    (struct anv_address) {
                                       .bo = ss_bo,
                                       .offset = src_state.offset,
                                    },
                                    src_state.alloc_size);
      }

      anv_cmd_buffer_add_secondary(primary, secondary);

      assert(secondary->perf_query_pool == NULL || primary->perf_query_pool == NULL ||
             secondary->perf_query_pool == primary->perf_query_pool);
      if (secondary->perf_query_pool)
         primary->perf_query_pool = secondary->perf_query_pool;

#if GFX_VERx10 == 120
      if (secondary->state.depth_reg_mode != ANV_DEPTH_REG_MODE_UNKNOWN)
         primary->state.depth_reg_mode = secondary->state.depth_reg_mode;
#endif
   }

   /* The secondary isn't counted in our VF cache tracking so we need to
    * invalidate the whole thing.
    */
   if (GFX_VER >= 8 && GFX_VER <= 9) {
      anv_add_pending_pipe_bits(primary,
                                ANV_PIPE_CS_STALL_BIT | ANV_PIPE_VF_CACHE_INVALIDATE_BIT,
                                "Secondary cmd buffer not tracked in VF cache");
   }

   /* The secondary may have selected a different pipeline (3D or compute) and
    * may have changed the current L3$ configuration.  Reset our tracking
    * variables to invalid values to ensure that we re-emit these in the case
    * where we do any draws or compute dispatches from the primary after the
    * secondary has returned.
    */
   primary->state.current_pipeline = UINT32_MAX;
   primary->state.current_l3_config = NULL;
   primary->state.current_hash_scale = 0;

   /* Each of the secondary command buffers will use its own state base
    * address.  We need to re-emit state base address for the primary after
    * all of the secondaries are done.
    *
    * TODO: Maybe we want to make this a dirty bit to avoid extra state base
    * address calls?
    */
   genX(cmd_buffer_emit_state_base_address)(primary);
}

/**
 * Program the hardware to use the specified L3 configuration.
 */
void
genX(cmd_buffer_config_l3)(struct anv_cmd_buffer *cmd_buffer,
                           const struct intel_l3_config *cfg)
{
   assert(cfg || GFX_VER >= 12);
   if (cfg == cmd_buffer->state.current_l3_config)
      return;

#if GFX_VER >= 11
   /* On Gfx11+ we use only one config, so verify it remains the same and skip
    * the stalling programming entirely.
    */
   assert(cfg == cmd_buffer->device->l3_config);
#else
   if (INTEL_DEBUG(DEBUG_L3)) {
      mesa_logd("L3 config transition: ");
      intel_dump_l3_config(cfg, stderr);
   }

   /* According to the hardware docs, the L3 partitioning can only be changed
    * while the pipeline is completely drained and the caches are flushed,
    * which involves a first PIPE_CONTROL flush which stalls the pipeline...
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DCFlushEnable = true;
      pc.PostSyncOperation = NoWrite;
      pc.CommandStreamerStallEnable = true;
      anv_debug_dump_pc(pc);
   }

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
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.TextureCacheInvalidationEnable = true;
      pc.ConstantCacheInvalidationEnable = true;
      pc.InstructionCacheInvalidateEnable = true;
      pc.StateCacheInvalidationEnable = true;
      pc.PostSyncOperation = NoWrite;
      anv_debug_dump_pc(pc);
   }

   /* Now send a third stalling flush to make sure that invalidation is
    * complete when the L3 configuration registers are modified.
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.DCFlushEnable = true;
      pc.PostSyncOperation = NoWrite;
      pc.CommandStreamerStallEnable = true;
      anv_debug_dump_pc(pc);
   }

   genX(emit_l3_config)(&cmd_buffer->batch, cmd_buffer->device, cfg);
#endif /* GFX_VER >= 11 */
   cmd_buffer->state.current_l3_config = cfg;
}

void
genX(cmd_buffer_apply_pipe_flushes)(struct anv_cmd_buffer *cmd_buffer)
{
   UNUSED const struct intel_device_info *devinfo = &cmd_buffer->device->info;
   enum anv_pipe_bits bits = cmd_buffer->state.pending_pipe_bits;

   if (unlikely(cmd_buffer->device->physical->always_flush_cache))
      bits |= ANV_PIPE_FLUSH_BITS | ANV_PIPE_INVALIDATE_BITS;
   else if (bits == 0)
      return;

   /*
    * From Sandybridge PRM, volume 2, "1.7.2 End-of-Pipe Synchronization":
    *
    *    Write synchronization is a special case of end-of-pipe
    *    synchronization that requires that the render cache and/or depth
    *    related caches are flushed to memory, where the data will become
    *    globally visible. This type of synchronization is required prior to
    *    SW (CPU) actually reading the result data from memory, or initiating
    *    an operation that will use as a read surface (such as a texture
    *    surface) a previous render target and/or depth/stencil buffer
    *
    *
    * From Haswell PRM, volume 2, part 1, "End-of-Pipe Synchronization":
    *
    *    Exercising the write cache flush bits (Render Target Cache Flush
    *    Enable, Depth Cache Flush Enable, DC Flush) in PIPE_CONTROL only
    *    ensures the write caches are flushed and doesn't guarantee the data
    *    is globally visible.
    *
    *    SW can track the completion of the end-of-pipe-synchronization by
    *    using "Notify Enable" and "PostSync Operation - Write Immediate
    *    Data" in the PIPE_CONTROL command.
    *
    * In other words, flushes are pipelined while invalidations are handled
    * immediately.  Therefore, if we're flushing anything then we need to
    * schedule an end-of-pipe sync before any invalidations can happen.
    */
   if (bits & ANV_PIPE_FLUSH_BITS)
      bits |= ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT;


   /* HSD 1209978178: docs say that before programming the aux table:
    *
    *    "Driver must ensure that the engine is IDLE but ensure it doesn't
    *    add extra flushes in the case it knows that the engine is already
    *    IDLE."
    */
   if (GFX_VER == 12 && (bits & ANV_PIPE_AUX_TABLE_INVALIDATE_BIT))
      bits |= ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT;

   /* If we're going to do an invalidate and we have a pending end-of-pipe
    * sync that has yet to be resolved, we do the end-of-pipe sync now.
    */
   if ((bits & ANV_PIPE_INVALIDATE_BITS) &&
       (bits & ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT)) {
      bits |= ANV_PIPE_END_OF_PIPE_SYNC_BIT;
      bits &= ~ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT;
   }

   /* Wa_1409226450, Wait for EU to be idle before pipe control which
    * invalidates the instruction cache
    */
   if (GFX_VER == 12 && (bits & ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT))
      bits |= ANV_PIPE_CS_STALL_BIT | ANV_PIPE_STALL_AT_SCOREBOARD_BIT;

   if ((GFX_VER >= 8 && GFX_VER <= 9) &&
       (bits & ANV_PIPE_CS_STALL_BIT) &&
       (bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT)) {
      /* If we are doing a VF cache invalidate AND a CS stall (it must be
       * both) then we can reset our vertex cache tracking.
       */
      memset(cmd_buffer->state.gfx.vb_dirty_ranges, 0,
             sizeof(cmd_buffer->state.gfx.vb_dirty_ranges));
      memset(&cmd_buffer->state.gfx.ib_dirty_range, 0,
             sizeof(cmd_buffer->state.gfx.ib_dirty_range));
   }

   /* Project: SKL / Argument: LRI Post Sync Operation [23]
    *
    * "PIPECONTROL command with “Command Streamer Stall Enable” must be
    *  programmed prior to programming a PIPECONTROL command with "LRI
    *  Post Sync Operation" in GPGPU mode of operation (i.e when
    *  PIPELINE_SELECT command is set to GPGPU mode of operation)."
    *
    * The same text exists a few rows below for Post Sync Op.
    *
    * On Gfx12 this is Wa_1607156449.
    */
   if (bits & ANV_PIPE_POST_SYNC_BIT) {
      if ((GFX_VER == 9 || (GFX_VER == 12 && devinfo->revision == 0 /* A0 */)) &&
          cmd_buffer->state.current_pipeline == GPGPU)
         bits |= ANV_PIPE_CS_STALL_BIT;
      bits &= ~ANV_PIPE_POST_SYNC_BIT;
   }

   if (bits & (ANV_PIPE_FLUSH_BITS | ANV_PIPE_STALL_BITS |
               ANV_PIPE_END_OF_PIPE_SYNC_BIT)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pipe) {
#if GFX_VER >= 12
         pipe.TileCacheFlushEnable = bits & ANV_PIPE_TILE_CACHE_FLUSH_BIT;
         pipe.HDCPipelineFlushEnable |= bits & ANV_PIPE_HDC_PIPELINE_FLUSH_BIT;
#else
         /* Flushing HDC pipeline requires DC Flush on earlier HW. */
         pipe.DCFlushEnable |= bits & ANV_PIPE_HDC_PIPELINE_FLUSH_BIT;
#endif
         pipe.DepthCacheFlushEnable = bits & ANV_PIPE_DEPTH_CACHE_FLUSH_BIT;
         pipe.DCFlushEnable |= bits & ANV_PIPE_DATA_CACHE_FLUSH_BIT;
         pipe.RenderTargetCacheFlushEnable =
            bits & ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT;

         /* Wa_1409600907: "PIPE_CONTROL with Depth Stall Enable bit must
          * be set with any PIPE_CONTROL with Depth Flush Enable bit set.
          */
#if GFX_VER >= 12
         pipe.DepthStallEnable =
            pipe.DepthCacheFlushEnable || (bits & ANV_PIPE_DEPTH_STALL_BIT);
#else
         pipe.DepthStallEnable = bits & ANV_PIPE_DEPTH_STALL_BIT;
#endif

         pipe.CommandStreamerStallEnable = bits & ANV_PIPE_CS_STALL_BIT;
         pipe.StallAtPixelScoreboard = bits & ANV_PIPE_STALL_AT_SCOREBOARD_BIT;

         /* From Sandybridge PRM, volume 2, "1.7.3.1 Writing a Value to Memory":
          *
          *    "The most common action to perform upon reaching a
          *    synchronization point is to write a value out to memory. An
          *    immediate value (included with the synchronization command) may
          *    be written."
          *
          *
          * From Broadwell PRM, volume 7, "End-of-Pipe Synchronization":
          *
          *    "In case the data flushed out by the render engine is to be
          *    read back in to the render engine in coherent manner, then the
          *    render engine has to wait for the fence completion before
          *    accessing the flushed data. This can be achieved by following
          *    means on various products: PIPE_CONTROL command with CS Stall
          *    and the required write caches flushed with Post-Sync-Operation
          *    as Write Immediate Data.
          *
          *    Example:
          *       - Workload-1 (3D/GPGPU/MEDIA)
          *       - PIPE_CONTROL (CS Stall, Post-Sync-Operation Write
          *         Immediate Data, Required Write Cache Flush bits set)
          *       - Workload-2 (Can use the data produce or output by
          *         Workload-1)
          */
         if (bits & ANV_PIPE_END_OF_PIPE_SYNC_BIT) {
            pipe.CommandStreamerStallEnable = true;
            pipe.PostSyncOperation = WriteImmediateData;
            pipe.Address = cmd_buffer->device->workaround_address;
         }

         /*
          * According to the Broadwell documentation, any PIPE_CONTROL with the
          * "Command Streamer Stall" bit set must also have another bit set,
          * with five different options:
          *
          *  - Render Target Cache Flush
          *  - Depth Cache Flush
          *  - Stall at Pixel Scoreboard
          *  - Post-Sync Operation
          *  - Depth Stall
          *  - DC Flush Enable
          *
          * I chose "Stall at Pixel Scoreboard" since that's what we use in
          * mesa and it seems to work fine. The choice is fairly arbitrary.
          */
         if (pipe.CommandStreamerStallEnable &&
             !pipe.RenderTargetCacheFlushEnable &&
             !pipe.DepthCacheFlushEnable &&
             !pipe.StallAtPixelScoreboard &&
             !pipe.PostSyncOperation &&
             !pipe.DepthStallEnable &&
             !pipe.DCFlushEnable)
            pipe.StallAtPixelScoreboard = true;
         anv_debug_dump_pc(pipe);
      }

      /* If a render target flush was emitted, then we can toggle off the bit
       * saying that render target writes are ongoing.
       */
      if (bits & ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT)
         bits &= ~(ANV_PIPE_RENDER_TARGET_BUFFER_WRITES);

      if (GFX_VERx10 == 75) {
         /* Haswell needs addition work-arounds:
          *
          * From Haswell PRM, volume 2, part 1, "End-of-Pipe Synchronization":
          *
          *    Option 1:
          *    PIPE_CONTROL command with the CS Stall and the required write
          *    caches flushed with Post-SyncOperation as Write Immediate Data
          *    followed by eight dummy MI_STORE_DATA_IMM (write to scratch
          *    spce) commands.
          *
          *    Example:
          *       - Workload-1
          *       - PIPE_CONTROL (CS Stall, Post-Sync-Operation Write
          *         Immediate Data, Required Write Cache Flush bits set)
          *       - MI_STORE_DATA_IMM (8 times) (Dummy data, Scratch Address)
          *       - Workload-2 (Can use the data produce or output by
          *         Workload-1)
          *
          * Unfortunately, both the PRMs and the internal docs are a bit
          * out-of-date in this regard.  What the windows driver does (and
          * this appears to actually work) is to emit a register read from the
          * memory address written by the pipe control above.
          *
          * What register we load into doesn't matter.  We choose an indirect
          * rendering register because we know it always exists and it's one
          * of the first registers the command parser allows us to write.  If
          * you don't have command parser support in your kernel (pre-4.2),
          * this will get turned into MI_NOOP and you won't get the
          * workaround.  Unfortunately, there's just not much we can do in
          * that case.  This register is perfectly safe to write since we
          * always re-load all of the indirect draw registers right before
          * 3DPRIMITIVE when needed anyway.
          */
         anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_MEM), lrm) {
            lrm.RegisterAddress  = 0x243C; /* GFX7_3DPRIM_START_INSTANCE */
            lrm.MemoryAddress = cmd_buffer->device->workaround_address;
         }
      }

      bits &= ~(ANV_PIPE_FLUSH_BITS | ANV_PIPE_STALL_BITS |
                ANV_PIPE_END_OF_PIPE_SYNC_BIT);
   }

   if (bits & ANV_PIPE_INVALIDATE_BITS) {
      /* From the SKL PRM, Vol. 2a, "PIPE_CONTROL",
       *
       *    "If the VF Cache Invalidation Enable is set to a 1 in a
       *    PIPE_CONTROL, a separate Null PIPE_CONTROL, all bitfields sets to
       *    0, with the VF Cache Invalidation Enable set to 0 needs to be sent
       *    prior to the PIPE_CONTROL with VF Cache Invalidation Enable set to
       *    a 1."
       *
       * This appears to hang Broadwell, so we restrict it to just gfx9.
       */
      if (GFX_VER == 9 && (bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT))
         anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pipe);

      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pipe) {
         pipe.StateCacheInvalidationEnable =
            bits & ANV_PIPE_STATE_CACHE_INVALIDATE_BIT;
         pipe.ConstantCacheInvalidationEnable =
            bits & ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT;
#if GFX_VER >= 12
         /* Invalidates the L3 cache part in which index & vertex data is loaded
          * when VERTEX_BUFFER_STATE::L3BypassDisable is set.
          */
         pipe.L3ReadOnlyCacheInvalidationEnable =
            bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT;
#endif
         pipe.VFCacheInvalidationEnable =
            bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT;
         pipe.TextureCacheInvalidationEnable =
            bits & ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT;
         pipe.InstructionCacheInvalidateEnable =
            bits & ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT;

         /* From the SKL PRM, Vol. 2a, "PIPE_CONTROL",
          *
          *    "When VF Cache Invalidate is set “Post Sync Operation” must be
          *    enabled to “Write Immediate Data” or “Write PS Depth Count” or
          *    “Write Timestamp”.
          */
         if (GFX_VER == 9 && pipe.VFCacheInvalidationEnable) {
            pipe.PostSyncOperation = WriteImmediateData;
            pipe.Address = cmd_buffer->device->workaround_address;
         }
         anv_debug_dump_pc(pipe);
      }

#if GFX_VER == 12
      if ((bits & ANV_PIPE_AUX_TABLE_INVALIDATE_BIT) &&
          cmd_buffer->device->info.has_aux_map) {
         anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
            lri.RegisterOffset = GENX(GFX_CCS_AUX_INV_num);
            lri.DataDWord = 1;
         }
      }
#endif

      bits &= ~ANV_PIPE_INVALIDATE_BITS;
   }

   cmd_buffer->state.pending_pipe_bits = bits;
}

static void
cmd_buffer_barrier(struct anv_cmd_buffer *cmd_buffer,
                   const VkDependencyInfoKHR *dep_info,
                   const char *reason)
{
   /* XXX: Right now, we're really dumb and just flush whatever categories
    * the app asks for.  One of these days we may make this a bit better
    * but right now that's all the hardware allows for in most areas.
    */
   VkAccessFlags2KHR src_flags = 0;
   VkAccessFlags2KHR dst_flags = 0;

   for (uint32_t i = 0; i < dep_info->memoryBarrierCount; i++) {
      src_flags |= dep_info->pMemoryBarriers[i].srcAccessMask;
      dst_flags |= dep_info->pMemoryBarriers[i].dstAccessMask;
   }

   for (uint32_t i = 0; i < dep_info->bufferMemoryBarrierCount; i++) {
      src_flags |= dep_info->pBufferMemoryBarriers[i].srcAccessMask;
      dst_flags |= dep_info->pBufferMemoryBarriers[i].dstAccessMask;
   }

   for (uint32_t i = 0; i < dep_info->imageMemoryBarrierCount; i++) {
      const VkImageMemoryBarrier2KHR *img_barrier =
         &dep_info->pImageMemoryBarriers[i];

      src_flags |= img_barrier->srcAccessMask;
      dst_flags |= img_barrier->dstAccessMask;

      ANV_FROM_HANDLE(anv_image, image, img_barrier->image);
      const VkImageSubresourceRange *range = &img_barrier->subresourceRange;

      uint32_t base_layer, layer_count;
      if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
         base_layer = 0;
         layer_count = anv_minify(image->vk.extent.depth, range->baseMipLevel);
      } else {
         base_layer = range->baseArrayLayer;
         layer_count = vk_image_subresource_layer_count(&image->vk, range);
      }
      const uint32_t level_count =
         vk_image_subresource_level_count(&image->vk, range);

      if (range->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
         transition_depth_buffer(cmd_buffer, image,
                                 base_layer, layer_count,
                                 img_barrier->oldLayout,
                                 img_barrier->newLayout,
                                 false /* will_full_fast_clear */);
      }

      if (range->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
         transition_stencil_buffer(cmd_buffer, image,
                                   range->baseMipLevel, level_count,
                                   base_layer, layer_count,
                                   img_barrier->oldLayout,
                                   img_barrier->newLayout,
                                   false /* will_full_fast_clear */);
      }

      if (range->aspectMask & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
         VkImageAspectFlags color_aspects =
            vk_image_expand_aspect_mask(&image->vk, range->aspectMask);
         anv_foreach_image_aspect_bit(aspect_bit, image, color_aspects) {
            transition_color_buffer(cmd_buffer, image, 1UL << aspect_bit,
                                    range->baseMipLevel, level_count,
                                    base_layer, layer_count,
                                    img_barrier->oldLayout,
                                    img_barrier->newLayout,
                                    img_barrier->srcQueueFamilyIndex,
                                    img_barrier->dstQueueFamilyIndex,
                                    false /* will_full_fast_clear */);
         }
      }
   }

   enum anv_pipe_bits bits =
      anv_pipe_flush_bits_for_access_flags(cmd_buffer->device, src_flags) |
      anv_pipe_invalidate_bits_for_access_flags(cmd_buffer->device, dst_flags);

   anv_add_pending_pipe_bits(cmd_buffer, bits, reason);
}

void genX(CmdPipelineBarrier2KHR)(
    VkCommandBuffer                             commandBuffer,
    const VkDependencyInfoKHR*                  pDependencyInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   cmd_buffer_barrier(cmd_buffer, pDependencyInfo, "pipe barrier");
}

static void
cmd_buffer_alloc_push_constants(struct anv_cmd_buffer *cmd_buffer)
{
   assert(anv_pipeline_is_primitive(cmd_buffer->state.gfx.pipeline));

   VkShaderStageFlags stages =
      cmd_buffer->state.gfx.pipeline->active_stages;

   /* In order to avoid thrash, we assume that vertex and fragment stages
    * always exist.  In the rare case where one is missing *and* the other
    * uses push concstants, this may be suboptimal.  However, avoiding stalls
    * seems more important.
    */
   stages |= VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

   if (stages == cmd_buffer->state.gfx.push_constant_stages)
      return;

   const unsigned push_constant_kb =
      cmd_buffer->device->info.max_constant_urb_size_kb;

   const unsigned num_stages =
      util_bitcount(stages & VK_SHADER_STAGE_ALL_GRAPHICS);
   unsigned size_per_stage = push_constant_kb / num_stages;

   /* Broadwell+ and Haswell gt3 require that the push constant sizes be in
    * units of 2KB.  Incidentally, these are the same platforms that have
    * 32KB worth of push constant space.
    */
   if (push_constant_kb == 32)
      size_per_stage &= ~1u;

   uint32_t kb_used = 0;
   for (int i = MESA_SHADER_VERTEX; i < MESA_SHADER_FRAGMENT; i++) {
      unsigned push_size = (stages & (1 << i)) ? size_per_stage : 0;
      anv_batch_emit(&cmd_buffer->batch,
                     GENX(3DSTATE_PUSH_CONSTANT_ALLOC_VS), alloc) {
         alloc._3DCommandSubOpcode  = 18 + i;
         alloc.ConstantBufferOffset = (push_size > 0) ? kb_used : 0;
         alloc.ConstantBufferSize   = push_size;
      }
      kb_used += push_size;
   }

   anv_batch_emit(&cmd_buffer->batch,
                  GENX(3DSTATE_PUSH_CONSTANT_ALLOC_PS), alloc) {
      alloc.ConstantBufferOffset = kb_used;
      alloc.ConstantBufferSize = push_constant_kb - kb_used;
   }

   cmd_buffer->state.gfx.push_constant_stages = stages;

   /* From the BDW PRM for 3DSTATE_PUSH_CONSTANT_ALLOC_VS:
    *
    *    "The 3DSTATE_CONSTANT_VS must be reprogrammed prior to
    *    the next 3DPRIMITIVE command after programming the
    *    3DSTATE_PUSH_CONSTANT_ALLOC_VS"
    *
    * Since 3DSTATE_PUSH_CONSTANT_ALLOC_VS is programmed as part of
    * pipeline setup, we need to dirty push constants.
    */
   cmd_buffer->state.push_constants_dirty |= VK_SHADER_STAGE_ALL_GRAPHICS;
}

static VkResult
emit_binding_table(struct anv_cmd_buffer *cmd_buffer,
                   struct anv_cmd_pipeline_state *pipe_state,
                   struct anv_shader_bin *shader,
                   struct anv_state *bt_state)
{
   struct anv_subpass *subpass = cmd_buffer->state.subpass;
   uint32_t state_offset;

   struct anv_pipeline_bind_map *map = &shader->bind_map;
   if (map->surface_count == 0) {
      *bt_state = (struct anv_state) { 0, };
      return VK_SUCCESS;
   }

   *bt_state = anv_cmd_buffer_alloc_binding_table(cmd_buffer,
                                                  map->surface_count,
                                                  &state_offset);
   uint32_t *bt_map = bt_state->map;

   if (bt_state->map == NULL)
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;

   /* We only need to emit relocs if we're not using softpin.  If we are using
    * softpin then we always keep all user-allocated memory objects resident.
    */
   const bool need_client_mem_relocs =
      !anv_use_softpin(cmd_buffer->device->physical);
   struct anv_push_constants *push = &pipe_state->push_constants;

   for (uint32_t s = 0; s < map->surface_count; s++) {
      struct anv_pipeline_binding *binding = &map->surface_to_descriptor[s];

      struct anv_state surface_state;

      switch (binding->set) {
      case ANV_DESCRIPTOR_SET_NULL:
         bt_map[s] = 0;
         break;

      case ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS:
         /* Color attachment binding */
         assert(shader->stage == MESA_SHADER_FRAGMENT);
         if (binding->index < subpass->color_count) {
            const unsigned att =
               subpass->color_attachments[binding->index].attachment;

            /* From the Vulkan 1.0.46 spec:
             *
             *    "If any color or depth/stencil attachments are
             *    VK_ATTACHMENT_UNUSED, then no writes occur for those
             *    attachments."
             */
            if (att == VK_ATTACHMENT_UNUSED) {
               surface_state = cmd_buffer->state.null_surface_state;
            } else {
               surface_state = cmd_buffer->state.attachments[att].color.state;
            }
         } else {
            surface_state = cmd_buffer->state.null_surface_state;
         }

         assert(surface_state.map);
         bt_map[s] = surface_state.offset + state_offset;
         break;

      case ANV_DESCRIPTOR_SET_SHADER_CONSTANTS: {
         struct anv_state surface_state =
            anv_cmd_buffer_alloc_surface_state(cmd_buffer);

         struct anv_address constant_data = {
            .bo = cmd_buffer->device->instruction_state_pool.block_pool.bo,
            .offset = shader->kernel.offset +
                      shader->prog_data->const_data_offset,
         };
         unsigned constant_data_size = shader->prog_data->const_data_size;

         const enum isl_format format =
            anv_isl_format_for_descriptor_type(cmd_buffer->device,
                                               VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
         anv_fill_buffer_surface_state(cmd_buffer->device,
                                       surface_state, format,
                                       ISL_SURF_USAGE_CONSTANT_BUFFER_BIT,
                                       constant_data, constant_data_size, 1);

         assert(surface_state.map);
         bt_map[s] = surface_state.offset + state_offset;
         add_surface_reloc(cmd_buffer, surface_state, constant_data);
         break;
      }

      case ANV_DESCRIPTOR_SET_NUM_WORK_GROUPS: {
         /* This is always the first binding for compute shaders */
         assert(shader->stage == MESA_SHADER_COMPUTE && s == 0);

         struct anv_state surface_state =
            anv_cmd_buffer_alloc_surface_state(cmd_buffer);

         const enum isl_format format =
            anv_isl_format_for_descriptor_type(cmd_buffer->device,
                                               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
         anv_fill_buffer_surface_state(cmd_buffer->device, surface_state,
                                       format,
                                       ISL_SURF_USAGE_CONSTANT_BUFFER_BIT,
                                       cmd_buffer->state.compute.num_workgroups,
                                       12, 1);

         assert(surface_state.map);
         bt_map[s] = surface_state.offset + state_offset;
         if (need_client_mem_relocs) {
            add_surface_reloc(cmd_buffer, surface_state,
                              cmd_buffer->state.compute.num_workgroups);
         }
         break;
      }

      case ANV_DESCRIPTOR_SET_DESCRIPTORS: {
         /* This is a descriptor set buffer so the set index is actually
          * given by binding->binding.  (Yes, that's confusing.)
          */
         struct anv_descriptor_set *set =
            pipe_state->descriptors[binding->index];
         assert(set->desc_mem.alloc_size);
         assert(set->desc_surface_state.alloc_size);
         bt_map[s] = set->desc_surface_state.offset + state_offset;
         add_surface_reloc(cmd_buffer, set->desc_surface_state,
                           anv_descriptor_set_address(set));
         break;
      }

      default: {
         assert(binding->set < MAX_SETS);
         const struct anv_descriptor_set *set =
            pipe_state->descriptors[binding->set];
         if (binding->index >= set->descriptor_count) {
            /* From the Vulkan spec section entitled "DescriptorSet and
             * Binding Assignment":
             *
             *    "If the array is runtime-sized, then array elements greater
             *    than or equal to the size of that binding in the bound
             *    descriptor set must not be used."
             *
             * Unfortunately, the compiler isn't smart enough to figure out
             * when a dynamic binding isn't used so it may grab the whole
             * array and stick it in the binding table.  In this case, it's
             * safe to just skip those bindings that are OOB.
             */
            assert(binding->index < set->layout->descriptor_count);
            continue;
         }
         const struct anv_descriptor *desc = &set->descriptors[binding->index];

         switch (desc->type) {
         case VK_DESCRIPTOR_TYPE_SAMPLER:
            /* Nothing for us to do here */
            continue;

         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
            if (desc->image_view) {
               struct anv_surface_state sstate =
                  (desc->layout == VK_IMAGE_LAYOUT_GENERAL) ?
                  desc->image_view->planes[binding->plane].general_sampler_surface_state :
                  desc->image_view->planes[binding->plane].optimal_sampler_surface_state;
               surface_state = sstate.state;
               assert(surface_state.alloc_size);
               if (need_client_mem_relocs)
                  add_surface_state_relocs(cmd_buffer, sstate);
            } else {
               surface_state = cmd_buffer->device->null_surface_state;
            }
            break;
         }
         case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            assert(shader->stage == MESA_SHADER_FRAGMENT);
            assert(desc->image_view != NULL);
            if ((desc->image_view->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) == 0) {
               /* For depth and stencil input attachments, we treat it like any
                * old texture that a user may have bound.
                */
               assert(desc->image_view->n_planes == 1);
               struct anv_surface_state sstate =
                  (desc->layout == VK_IMAGE_LAYOUT_GENERAL) ?
                  desc->image_view->planes[0].general_sampler_surface_state :
                  desc->image_view->planes[0].optimal_sampler_surface_state;
               surface_state = sstate.state;
               assert(surface_state.alloc_size);
               if (need_client_mem_relocs)
                  add_surface_state_relocs(cmd_buffer, sstate);
            } else {
               /* For color input attachments, we create the surface state at
                * vkBeginRenderPass time so that we can include aux and clear
                * color information.
                */
               assert(binding->input_attachment_index < subpass->input_count);
               const unsigned subpass_att = binding->input_attachment_index;
               const unsigned att = subpass->input_attachments[subpass_att].attachment;
               surface_state = cmd_buffer->state.attachments[att].input.state;
            }
            break;

         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            if (desc->image_view) {
               struct anv_surface_state sstate =
                  binding->lowered_storage_surface
                  ? desc->image_view->planes[binding->plane].lowered_storage_surface_state
                  : desc->image_view->planes[binding->plane].storage_surface_state;
               surface_state = sstate.state;
               assert(surface_state.alloc_size);
               if (surface_state.offset == 0) {
                  mesa_loge("Bound a image to a descriptor where the "
                            "descriptor does not have NonReadable "
                            "set and the image does not have a "
                            "corresponding SPIR-V format enum.");
                  vk_debug_report(&cmd_buffer->device->physical->instance->vk,
                                  VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  &desc->image_view->vk.base,
                                  __LINE__, 0, "anv",
                                  "Bound a image to a descriptor where the "
                                  "descriptor does not have NonReadable "
                                  "set and the image does not have a "
                                  "corresponding SPIR-V format enum.");
               }
               if (surface_state.offset && need_client_mem_relocs)
                  add_surface_state_relocs(cmd_buffer, sstate);
            } else {
               surface_state = cmd_buffer->device->null_surface_state;
            }
            break;
         }

         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            if (desc->buffer_view) {
               surface_state = desc->buffer_view->surface_state;
               assert(surface_state.alloc_size);
               if (need_client_mem_relocs) {
                  add_surface_reloc(cmd_buffer, surface_state,
                                    desc->buffer_view->address);
               }
            } else {
               surface_state = cmd_buffer->device->null_surface_state;
            }
            break;

         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
            if (desc->buffer) {
               /* Compute the offset within the buffer */
               uint32_t dynamic_offset =
                  push->dynamic_offsets[binding->dynamic_offset_index];
               uint64_t offset = desc->offset + dynamic_offset;
               /* Clamp to the buffer size */
               offset = MIN2(offset, desc->buffer->size);
               /* Clamp the range to the buffer size */
               uint32_t range = MIN2(desc->range, desc->buffer->size - offset);

               /* Align the range for consistency */
               if (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
                  range = align_u32(range, ANV_UBO_ALIGNMENT);

               struct anv_address address =
                  anv_address_add(desc->buffer->address, offset);

               surface_state =
                  anv_state_stream_alloc(&cmd_buffer->surface_state_stream, 64, 64);
               enum isl_format format =
                  anv_isl_format_for_descriptor_type(cmd_buffer->device,
                                                     desc->type);

               isl_surf_usage_flags_t usage =
                  desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ?
                  ISL_SURF_USAGE_CONSTANT_BUFFER_BIT :
                  ISL_SURF_USAGE_STORAGE_BIT;

               anv_fill_buffer_surface_state(cmd_buffer->device, surface_state,
                                             format, usage, address, range, 1);
               if (need_client_mem_relocs)
                  add_surface_reloc(cmd_buffer, surface_state, address);
            } else {
               surface_state = cmd_buffer->device->null_surface_state;
            }
            break;
         }

         case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            if (desc->buffer_view) {
               surface_state = binding->lowered_storage_surface
                  ? desc->buffer_view->lowered_storage_surface_state
                  : desc->buffer_view->storage_surface_state;
               assert(surface_state.alloc_size);
               if (need_client_mem_relocs) {
                  add_surface_reloc(cmd_buffer, surface_state,
                                    desc->buffer_view->address);
               }
            } else {
               surface_state = cmd_buffer->device->null_surface_state;
            }
            break;

         default:
            assert(!"Invalid descriptor type");
            continue;
         }
         assert(surface_state.map);
         bt_map[s] = surface_state.offset + state_offset;
         break;
      }
      }
   }

   return VK_SUCCESS;
}

static VkResult
emit_samplers(struct anv_cmd_buffer *cmd_buffer,
              struct anv_cmd_pipeline_state *pipe_state,
              struct anv_shader_bin *shader,
              struct anv_state *state)
{
   struct anv_pipeline_bind_map *map = &shader->bind_map;
   if (map->sampler_count == 0) {
      *state = (struct anv_state) { 0, };
      return VK_SUCCESS;
   }

   uint32_t size = map->sampler_count * 16;
   *state = anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, size, 32);

   if (state->map == NULL)
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;

   for (uint32_t s = 0; s < map->sampler_count; s++) {
      struct anv_pipeline_binding *binding = &map->sampler_to_descriptor[s];
      const struct anv_descriptor *desc =
         &pipe_state->descriptors[binding->set]->descriptors[binding->index];

      if (desc->type != VK_DESCRIPTOR_TYPE_SAMPLER &&
          desc->type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
         continue;

      struct anv_sampler *sampler = desc->sampler;

      /* This can happen if we have an unfilled slot since TYPE_SAMPLER
       * happens to be zero.
       */
      if (sampler == NULL)
         continue;

      memcpy(state->map + (s * 16),
             sampler->state[binding->plane], sizeof(sampler->state[0]));
   }

   return VK_SUCCESS;
}

static uint32_t
flush_descriptor_sets(struct anv_cmd_buffer *cmd_buffer,
                      struct anv_cmd_pipeline_state *pipe_state,
                      const VkShaderStageFlags dirty,
                      struct anv_shader_bin **shaders,
                      uint32_t num_shaders)
{
   VkShaderStageFlags flushed = 0;

   VkResult result = VK_SUCCESS;
   for (uint32_t i = 0; i < num_shaders; i++) {
      if (!shaders[i])
         continue;

      gl_shader_stage stage = shaders[i]->stage;
      VkShaderStageFlags vk_stage = mesa_to_vk_shader_stage(stage);
      if ((vk_stage & dirty) == 0)
         continue;

      assert(stage < ARRAY_SIZE(cmd_buffer->state.samplers));
      result = emit_samplers(cmd_buffer, pipe_state, shaders[i],
                             &cmd_buffer->state.samplers[stage]);
      if (result != VK_SUCCESS)
         break;

      assert(stage < ARRAY_SIZE(cmd_buffer->state.binding_tables));
      result = emit_binding_table(cmd_buffer, pipe_state, shaders[i],
                                  &cmd_buffer->state.binding_tables[stage]);
      if (result != VK_SUCCESS)
         break;

      flushed |= vk_stage;
   }

   if (result != VK_SUCCESS) {
      assert(result == VK_ERROR_OUT_OF_DEVICE_MEMORY);

      result = anv_cmd_buffer_new_binding_table_block(cmd_buffer);
      if (result != VK_SUCCESS)
         return 0;

      /* Re-emit state base addresses so we get the new surface state base
       * address before we start emitting binding tables etc.
       */
      genX(cmd_buffer_emit_state_base_address)(cmd_buffer);

      /* Re-emit all active binding tables */
      flushed = 0;

      for (uint32_t i = 0; i < num_shaders; i++) {
         if (!shaders[i])
            continue;

         gl_shader_stage stage = shaders[i]->stage;

         result = emit_samplers(cmd_buffer, pipe_state, shaders[i],
                                &cmd_buffer->state.samplers[stage]);
         if (result != VK_SUCCESS) {
            anv_batch_set_error(&cmd_buffer->batch, result);
            return 0;
         }
         result = emit_binding_table(cmd_buffer, pipe_state, shaders[i],
                                     &cmd_buffer->state.binding_tables[stage]);
         if (result != VK_SUCCESS) {
            anv_batch_set_error(&cmd_buffer->batch, result);
            return 0;
         }

         flushed |= mesa_to_vk_shader_stage(stage);
      }
   }

   return flushed;
}

static void
cmd_buffer_emit_descriptor_pointers(struct anv_cmd_buffer *cmd_buffer,
                                    uint32_t stages)
{
   static const uint32_t sampler_state_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 43,
      [MESA_SHADER_TESS_CTRL]                   = 44, /* HS */
      [MESA_SHADER_TESS_EVAL]                   = 45, /* DS */
      [MESA_SHADER_GEOMETRY]                    = 46,
      [MESA_SHADER_FRAGMENT]                    = 47,
      [MESA_SHADER_COMPUTE]                     = 0,
   };

   static const uint32_t binding_table_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 38,
      [MESA_SHADER_TESS_CTRL]                   = 39,
      [MESA_SHADER_TESS_EVAL]                   = 40,
      [MESA_SHADER_GEOMETRY]                    = 41,
      [MESA_SHADER_FRAGMENT]                    = 42,
      [MESA_SHADER_COMPUTE]                     = 0,
   };

   anv_foreach_stage(s, stages) {
      assert(s < ARRAY_SIZE(binding_table_opcodes));
      assert(binding_table_opcodes[s] > 0);

      if (cmd_buffer->state.samplers[s].alloc_size > 0) {
         anv_batch_emit(&cmd_buffer->batch,
                        GENX(3DSTATE_SAMPLER_STATE_POINTERS_VS), ssp) {
            ssp._3DCommandSubOpcode = sampler_state_opcodes[s];
            ssp.PointertoVSSamplerState = cmd_buffer->state.samplers[s].offset;
         }
      }

      /* Always emit binding table pointers if we're asked to, since on SKL
       * this is what flushes push constants. */
      anv_batch_emit(&cmd_buffer->batch,
                     GENX(3DSTATE_BINDING_TABLE_POINTERS_VS), btp) {
         btp._3DCommandSubOpcode = binding_table_opcodes[s];
         btp.PointertoVSBindingTable = cmd_buffer->state.binding_tables[s].offset;
      }
   }
}

static struct anv_address
get_push_range_address(struct anv_cmd_buffer *cmd_buffer,
                       const struct anv_shader_bin *shader,
                       const struct anv_push_range *range)
{
   struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   switch (range->set) {
   case ANV_DESCRIPTOR_SET_DESCRIPTORS: {
      /* This is a descriptor set buffer so the set index is
       * actually given by binding->binding.  (Yes, that's
       * confusing.)
       */
      struct anv_descriptor_set *set =
         gfx_state->base.descriptors[range->index];
      return anv_descriptor_set_address(set);
   }

   case ANV_DESCRIPTOR_SET_PUSH_CONSTANTS: {
      if (gfx_state->base.push_constants_state.alloc_size == 0) {
         gfx_state->base.push_constants_state =
            anv_cmd_buffer_gfx_push_constants(cmd_buffer);
      }
      return (struct anv_address) {
         .bo = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
         .offset = gfx_state->base.push_constants_state.offset,
      };
   }

   case ANV_DESCRIPTOR_SET_SHADER_CONSTANTS:
      return (struct anv_address) {
         .bo = cmd_buffer->device->instruction_state_pool.block_pool.bo,
         .offset = shader->kernel.offset +
                   shader->prog_data->const_data_offset,
      };

   default: {
      assert(range->set < MAX_SETS);
      struct anv_descriptor_set *set =
         gfx_state->base.descriptors[range->set];
      const struct anv_descriptor *desc =
         &set->descriptors[range->index];

      if (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
         if (desc->buffer_view)
            return desc->buffer_view->address;
      } else {
         assert(desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
         if (desc->buffer) {
            const struct anv_push_constants *push =
               &gfx_state->base.push_constants;
            uint32_t dynamic_offset =
               push->dynamic_offsets[range->dynamic_offset_index];
            return anv_address_add(desc->buffer->address,
                                   desc->offset + dynamic_offset);
         }
      }

      /* For NULL UBOs, we just return an address in the workaround BO.  We do
       * writes to it for workarounds but always at the bottom.  The higher
       * bytes should be all zeros.
       */
      assert(range->length * 32 <= 2048);
      return (struct anv_address) {
         .bo = cmd_buffer->device->workaround_bo,
         .offset = 1024,
      };
   }
   }
}


/** Returns the size in bytes of the bound buffer
 *
 * The range is relative to the start of the buffer, not the start of the
 * range.  The returned range may be smaller than
 *
 *    (range->start + range->length) * 32;
 */
static uint32_t
get_push_range_bound_size(struct anv_cmd_buffer *cmd_buffer,
                          const struct anv_shader_bin *shader,
                          const struct anv_push_range *range)
{
   assert(shader->stage != MESA_SHADER_COMPUTE);
   const struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   switch (range->set) {
   case ANV_DESCRIPTOR_SET_DESCRIPTORS: {
      struct anv_descriptor_set *set =
         gfx_state->base.descriptors[range->index];
      assert(range->start * 32 < set->desc_mem.alloc_size);
      assert((range->start + range->length) * 32 <= set->desc_mem.alloc_size);
      return set->desc_mem.alloc_size;
   }

   case ANV_DESCRIPTOR_SET_PUSH_CONSTANTS:
      return (range->start + range->length) * 32;

   case ANV_DESCRIPTOR_SET_SHADER_CONSTANTS:
      return ALIGN(shader->prog_data->const_data_size, ANV_UBO_ALIGNMENT);

   default: {
      assert(range->set < MAX_SETS);
      struct anv_descriptor_set *set =
         gfx_state->base.descriptors[range->set];
      const struct anv_descriptor *desc =
         &set->descriptors[range->index];

      if (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
         if (!desc->buffer_view)
            return 0;

         if (range->start * 32 > desc->buffer_view->range)
            return 0;

         return desc->buffer_view->range;
      } else {
         if (!desc->buffer)
            return 0;

         assert(desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
         /* Compute the offset within the buffer */
         const struct anv_push_constants *push =
            &gfx_state->base.push_constants;
         uint32_t dynamic_offset =
            push->dynamic_offsets[range->dynamic_offset_index];
         uint64_t offset = desc->offset + dynamic_offset;
         /* Clamp to the buffer size */
         offset = MIN2(offset, desc->buffer->size);
         /* Clamp the range to the buffer size */
         uint32_t bound_range = MIN2(desc->range, desc->buffer->size - offset);

         /* Align the range for consistency */
         bound_range = align_u32(bound_range, ANV_UBO_ALIGNMENT);

         return bound_range;
      }
   }
   }
}

static void
cmd_buffer_emit_push_constant(struct anv_cmd_buffer *cmd_buffer,
                              gl_shader_stage stage,
                              struct anv_address *buffers,
                              unsigned buffer_count)
{
   const struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   const struct anv_graphics_pipeline *pipeline = gfx_state->pipeline;

   static const uint32_t push_constant_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 21,
      [MESA_SHADER_TESS_CTRL]                   = 25, /* HS */
      [MESA_SHADER_TESS_EVAL]                   = 26, /* DS */
      [MESA_SHADER_GEOMETRY]                    = 22,
      [MESA_SHADER_FRAGMENT]                    = 23,
      [MESA_SHADER_COMPUTE]                     = 0,
   };

   assert(stage < ARRAY_SIZE(push_constant_opcodes));
   assert(push_constant_opcodes[stage] > 0);

   anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CONSTANT_VS), c) {
      c._3DCommandSubOpcode = push_constant_opcodes[stage];

      if (anv_pipeline_has_stage(pipeline, stage)) {
         const struct anv_pipeline_bind_map *bind_map =
            &pipeline->shaders[stage]->bind_map;

#if GFX_VER >= 9
         /* This field exists since Gfx8.  However, the Broadwell PRM says:
          *
          *    "Constant Buffer Object Control State must be always programmed
          *    to zero."
          *
          * This restriction does not exist on any newer platforms.
          *
          * We only have one MOCS field for the whole packet, not one per
          * buffer.  We could go out of our way here to walk over all of the
          * buffers and see if any of them are used externally and use the
          * external MOCS.  However, the notion that someone would use the
          * same bit of memory for both scanout and a UBO is nuts.  Let's not
          * bother and assume it's all internal.
          */
         c.MOCS = isl_mocs(&cmd_buffer->device->isl_dev, 0, false);
#endif

#if GFX_VERx10 >= 75
         /* The Skylake PRM contains the following restriction:
          *
          *    "The driver must ensure The following case does not occur
          *     without a flush to the 3D engine: 3DSTATE_CONSTANT_* with
          *     buffer 3 read length equal to zero committed followed by a
          *     3DSTATE_CONSTANT_* with buffer 0 read length not equal to
          *     zero committed."
          *
          * To avoid this, we program the buffers in the highest slots.
          * This way, slot 0 is only used if slot 3 is also used.
          */
         assert(buffer_count <= 4);
         const unsigned shift = 4 - buffer_count;
         for (unsigned i = 0; i < buffer_count; i++) {
            const struct anv_push_range *range = &bind_map->push_ranges[i];

            /* At this point we only have non-empty ranges */
            assert(range->length > 0);

            /* For Ivy Bridge, make sure we only set the first range (actual
             * push constants)
             */
            assert((GFX_VERx10 >= 75) || i == 0);

            c.ConstantBody.ReadLength[i + shift] = range->length;
            c.ConstantBody.Buffer[i + shift] =
               anv_address_add(buffers[i], range->start * 32);
         }
#else
         /* For Ivy Bridge, push constants are relative to dynamic state
          * base address and we only ever push actual push constants.
          */
         if (bind_map->push_ranges[0].length > 0) {
            assert(buffer_count == 1);
            assert(bind_map->push_ranges[0].set ==
                   ANV_DESCRIPTOR_SET_PUSH_CONSTANTS);
            assert(buffers[0].bo ==
                   cmd_buffer->device->dynamic_state_pool.block_pool.bo);
            c.ConstantBody.ReadLength[0] = bind_map->push_ranges[0].length;
            c.ConstantBody.Buffer[0].bo = NULL;
            c.ConstantBody.Buffer[0].offset = buffers[0].offset;
         }
         assert(bind_map->push_ranges[1].length == 0);
         assert(bind_map->push_ranges[2].length == 0);
         assert(bind_map->push_ranges[3].length == 0);
#endif
      }
   }
}

#if GFX_VER >= 12
static void
cmd_buffer_emit_push_constant_all(struct anv_cmd_buffer *cmd_buffer,
                                  uint32_t shader_mask,
                                  struct anv_address *buffers,
                                  uint32_t buffer_count)
{
   if (buffer_count == 0) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CONSTANT_ALL), c) {
         c.ShaderUpdateEnable = shader_mask;
         c.MOCS = isl_mocs(&cmd_buffer->device->isl_dev, 0, false);
      }
      return;
   }

   const struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   const struct anv_graphics_pipeline *pipeline = gfx_state->pipeline;

   static const UNUSED uint32_t push_constant_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 21,
      [MESA_SHADER_TESS_CTRL]                   = 25, /* HS */
      [MESA_SHADER_TESS_EVAL]                   = 26, /* DS */
      [MESA_SHADER_GEOMETRY]                    = 22,
      [MESA_SHADER_FRAGMENT]                    = 23,
      [MESA_SHADER_COMPUTE]                     = 0,
   };

   gl_shader_stage stage = vk_to_mesa_shader_stage(shader_mask);
   assert(stage < ARRAY_SIZE(push_constant_opcodes));
   assert(push_constant_opcodes[stage] > 0);

   const struct anv_pipeline_bind_map *bind_map =
      &pipeline->shaders[stage]->bind_map;

   uint32_t *dw;
   const uint32_t buffer_mask = (1 << buffer_count) - 1;
   const uint32_t num_dwords = 2 + 2 * buffer_count;

   dw = anv_batch_emitn(&cmd_buffer->batch, num_dwords,
                        GENX(3DSTATE_CONSTANT_ALL),
                        .ShaderUpdateEnable = shader_mask,
                        .PointerBufferMask = buffer_mask,
                        .MOCS = isl_mocs(&cmd_buffer->device->isl_dev, 0, false));

   for (int i = 0; i < buffer_count; i++) {
      const struct anv_push_range *range = &bind_map->push_ranges[i];
      GENX(3DSTATE_CONSTANT_ALL_DATA_pack)(
         &cmd_buffer->batch, dw + 2 + i * 2,
         &(struct GENX(3DSTATE_CONSTANT_ALL_DATA)) {
            .PointerToConstantBuffer =
               anv_address_add(buffers[i], range->start * 32),
            .ConstantBufferReadLength = range->length,
         });
   }
}
#endif

static void
cmd_buffer_flush_push_constants(struct anv_cmd_buffer *cmd_buffer,
                                VkShaderStageFlags dirty_stages)
{
   VkShaderStageFlags flushed = 0;
   struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   const struct anv_graphics_pipeline *pipeline = gfx_state->pipeline;

#if GFX_VER >= 12
   uint32_t nobuffer_stages = 0;
#endif

   /* Compute robust pushed register access mask for each stage. */
   if (cmd_buffer->device->robust_buffer_access) {
      anv_foreach_stage(stage, dirty_stages) {
         if (!anv_pipeline_has_stage(pipeline, stage))
            continue;

         const struct anv_shader_bin *shader = pipeline->shaders[stage];
         const struct anv_pipeline_bind_map *bind_map = &shader->bind_map;
         struct anv_push_constants *push = &gfx_state->base.push_constants;

         push->push_reg_mask[stage] = 0;
         /* Start of the current range in the shader, relative to the start of
          * push constants in the shader.
          */
         unsigned range_start_reg = 0;
         for (unsigned i = 0; i < 4; i++) {
            const struct anv_push_range *range = &bind_map->push_ranges[i];
            if (range->length == 0)
               continue;

            unsigned bound_size =
               get_push_range_bound_size(cmd_buffer, shader, range);
            if (bound_size >= range->start * 32) {
               unsigned bound_regs =
                  MIN2(DIV_ROUND_UP(bound_size, 32) - range->start,
                       range->length);
               assert(range_start_reg + bound_regs <= 64);
               push->push_reg_mask[stage] |= BITFIELD64_RANGE(range_start_reg,
                                                              bound_regs);
            }

            cmd_buffer->state.push_constants_dirty |=
               mesa_to_vk_shader_stage(stage);

            range_start_reg += range->length;
         }
      }
   }

   /* Resets the push constant state so that we allocate a new one if
    * needed.
    */
   gfx_state->base.push_constants_state = ANV_STATE_NULL;

   anv_foreach_stage(stage, dirty_stages) {
      unsigned buffer_count = 0;
      flushed |= mesa_to_vk_shader_stage(stage);
      UNUSED uint32_t max_push_range = 0;

      struct anv_address buffers[4] = {};
      if (anv_pipeline_has_stage(pipeline, stage)) {
         const struct anv_shader_bin *shader = pipeline->shaders[stage];
         const struct anv_pipeline_bind_map *bind_map = &shader->bind_map;

         /* We have to gather buffer addresses as a second step because the
          * loop above puts data into the push constant area and the call to
          * get_push_range_address is what locks our push constants and copies
          * them into the actual GPU buffer.  If we did the two loops at the
          * same time, we'd risk only having some of the sizes in the push
          * constant buffer when we did the copy.
          */
         for (unsigned i = 0; i < 4; i++) {
            const struct anv_push_range *range = &bind_map->push_ranges[i];
            if (range->length == 0)
               break;

            buffers[i] = get_push_range_address(cmd_buffer, shader, range);
            max_push_range = MAX2(max_push_range, range->length);
            buffer_count++;
         }

         /* We have at most 4 buffers but they should be tightly packed */
         for (unsigned i = buffer_count; i < 4; i++)
            assert(bind_map->push_ranges[i].length == 0);
      }

#if GFX_VER >= 12
      /* If this stage doesn't have any push constants, emit it later in a
       * single CONSTANT_ALL packet.
       */
      if (buffer_count == 0) {
         nobuffer_stages |= 1 << stage;
         continue;
      }

      /* The Constant Buffer Read Length field from 3DSTATE_CONSTANT_ALL
       * contains only 5 bits, so we can only use it for buffers smaller than
       * 32.
       */
      if (max_push_range < 32) {
         cmd_buffer_emit_push_constant_all(cmd_buffer, 1 << stage,
                                           buffers, buffer_count);
         continue;
      }
#endif

      cmd_buffer_emit_push_constant(cmd_buffer, stage, buffers, buffer_count);
   }

#if GFX_VER >= 12
   if (nobuffer_stages)
      cmd_buffer_emit_push_constant_all(cmd_buffer, nobuffer_stages, NULL, 0);
#endif

   cmd_buffer->state.push_constants_dirty &= ~flushed;
}

static void
cmd_buffer_emit_clip(struct anv_cmd_buffer *cmd_buffer)
{
   const uint32_t clip_states =
#if GFX_VER <= 7
      ANV_CMD_DIRTY_DYNAMIC_FRONT_FACE |
      ANV_CMD_DIRTY_DYNAMIC_CULL_MODE |
#endif
      ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY |
      ANV_CMD_DIRTY_DYNAMIC_VIEWPORT |
      ANV_CMD_DIRTY_PIPELINE;

   if ((cmd_buffer->state.gfx.dirty & clip_states) == 0)
      return;

   /* Take dynamic primitive topology in to account with
    *    3DSTATE_CLIP::ViewportXYClipTestEnable
    */
   bool xy_clip_test_enable = 0;

   if (cmd_buffer->state.gfx.pipeline->dynamic_states &
       ANV_CMD_DIRTY_DYNAMIC_PRIMITIVE_TOPOLOGY) {
      VkPrimitiveTopology primitive_topology =
         cmd_buffer->state.gfx.dynamic.primitive_topology;

      VkPolygonMode dynamic_raster_mode =
         genX(raster_polygon_mode)(cmd_buffer->state.gfx.pipeline,
                                   primitive_topology);

      xy_clip_test_enable = (dynamic_raster_mode == VK_POLYGON_MODE_FILL);
   }

#if GFX_VER <= 7
   const struct anv_dynamic_state *d = &cmd_buffer->state.gfx.dynamic;
#endif
   struct GENX(3DSTATE_CLIP) clip = {
      GENX(3DSTATE_CLIP_header),
#if GFX_VER <= 7
      .FrontWinding = genX(vk_to_intel_front_face)[d->front_face],
      .CullMode     = genX(vk_to_intel_cullmode)[d->cull_mode],
#endif
      .ViewportXYClipTestEnable = xy_clip_test_enable,
   };
   uint32_t dwords[GENX(3DSTATE_CLIP_length)];

   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   if (anv_pipeline_is_primitive(pipeline)) {
      const struct brw_vue_prog_data *last =
         anv_pipeline_get_last_vue_prog_data(pipeline);
      if (last->vue_map.slots_valid & VARYING_BIT_VIEWPORT) {
         clip.MaximumVPIndex =
            cmd_buffer->state.gfx.dynamic.viewport.count > 0 ?
            cmd_buffer->state.gfx.dynamic.viewport.count - 1 : 0;
      }
   }

   GENX(3DSTATE_CLIP_pack)(NULL, dwords, &clip);
   anv_batch_emit_merge(&cmd_buffer->batch, dwords,
                        pipeline->gfx7.clip);
}

static void
cmd_buffer_emit_streamout(struct anv_cmd_buffer *cmd_buffer)
{
   const struct anv_dynamic_state *d = &cmd_buffer->state.gfx.dynamic;
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;

#if GFX_VER == 7
#  define streamout_state_dw pipeline->gfx7.streamout_state
#else
#  define streamout_state_dw pipeline->gfx8.streamout_state
#endif

   uint32_t dwords[GENX(3DSTATE_STREAMOUT_length)];

   struct GENX(3DSTATE_STREAMOUT) so = {
      GENX(3DSTATE_STREAMOUT_header),
      .RenderingDisable = d->raster_discard,
   };
   GENX(3DSTATE_STREAMOUT_pack)(NULL, dwords, &so);
   anv_batch_emit_merge(&cmd_buffer->batch, dwords, streamout_state_dw);
}

void
genX(cmd_buffer_flush_state)(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   uint32_t *p;

   assert((pipeline->active_stages & VK_SHADER_STAGE_COMPUTE_BIT) == 0);

   genX(cmd_buffer_config_l3)(cmd_buffer, pipeline->base.l3_config);

   genX(cmd_buffer_emit_hashing_mode)(cmd_buffer, UINT_MAX, UINT_MAX, 1);

   genX(flush_pipeline_select_3d)(cmd_buffer);

   /* Apply any pending pipeline flushes we may have.  We want to apply them
    * now because, if any of those flushes are for things like push constants,
    * the GPU will read the state at weird times.
    */
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   uint32_t vb_emit = cmd_buffer->state.gfx.vb_dirty & pipeline->vb_used;
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE)
      vb_emit |= pipeline->vb_used;

   if (vb_emit) {
      const uint32_t num_buffers = __builtin_popcount(vb_emit);
      const uint32_t num_dwords = 1 + num_buffers * 4;

      p = anv_batch_emitn(&cmd_buffer->batch, num_dwords,
                          GENX(3DSTATE_VERTEX_BUFFERS));
      uint32_t i = 0;
      u_foreach_bit(vb, vb_emit) {
         struct anv_buffer *buffer = cmd_buffer->state.vertex_bindings[vb].buffer;
         uint32_t offset = cmd_buffer->state.vertex_bindings[vb].offset;

         /* If dynamic, use stride/size from vertex binding, otherwise use
          * stride/size that was setup in the pipeline object.
          */
         bool dynamic_stride = cmd_buffer->state.gfx.dynamic.dyn_vbo_stride;
         bool dynamic_size = cmd_buffer->state.gfx.dynamic.dyn_vbo_size;

         struct GENX(VERTEX_BUFFER_STATE) state;
         if (buffer) {
            uint32_t stride = dynamic_stride ?
               cmd_buffer->state.vertex_bindings[vb].stride : pipeline->vb[vb].stride;
            /* From the Vulkan spec (vkCmdBindVertexBuffers2EXT):
             *
             * "If pname:pSizes is not NULL then pname:pSizes[i] specifies
             * the bound size of the vertex buffer starting from the corresponding
             * elements of pname:pBuffers[i] plus pname:pOffsets[i]."
             */
            UNUSED uint32_t size = dynamic_size ?
               cmd_buffer->state.vertex_bindings[vb].size : buffer->size - offset;

            state = (struct GENX(VERTEX_BUFFER_STATE)) {
               .VertexBufferIndex = vb,

               .MOCS = anv_mocs(cmd_buffer->device, buffer->address.bo,
                                ISL_SURF_USAGE_VERTEX_BUFFER_BIT),
#if GFX_VER <= 7
               .BufferAccessType = pipeline->vb[vb].instanced ? INSTANCEDATA : VERTEXDATA,
               .InstanceDataStepRate = pipeline->vb[vb].instance_divisor,
#endif
               .AddressModifyEnable = true,
               .BufferPitch = stride,
               .BufferStartingAddress = anv_address_add(buffer->address, offset),
               .NullVertexBuffer = offset >= buffer->size,
#if GFX_VER >= 12
               .L3BypassDisable = true,
#endif

#if GFX_VER >= 8
               .BufferSize = size,
#else
               /* XXX: to handle dynamic offset for older gens we might want
                * to modify Endaddress, but there are issues when doing so:
                *
                * https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/7439
                */
               .EndAddress = anv_address_add(buffer->address, buffer->size - 1),
#endif
            };
         } else {
            state = (struct GENX(VERTEX_BUFFER_STATE)) {
               .VertexBufferIndex = vb,
               .NullVertexBuffer = true,
            };
         }

#if GFX_VER >= 8 && GFX_VER <= 9
         genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(cmd_buffer, vb,
                                                        state.BufferStartingAddress,
                                                        state.BufferSize);
#endif

         GENX(VERTEX_BUFFER_STATE_pack)(&cmd_buffer->batch, &p[1 + i * 4], &state);
         i++;
      }
   }

   cmd_buffer->state.gfx.vb_dirty &= ~vb_emit;

   uint32_t descriptors_dirty = cmd_buffer->state.descriptors_dirty &
                                pipeline->active_stages;
   if (!cmd_buffer->state.gfx.dirty && !descriptors_dirty &&
       !cmd_buffer->state.push_constants_dirty)
      return;

   if ((cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_XFB_ENABLE) ||
       (GFX_VER == 7 && (cmd_buffer->state.gfx.dirty &
                         ANV_CMD_DIRTY_PIPELINE))) {
      /* We don't need any per-buffer dirty tracking because you're not
       * allowed to bind different XFB buffers while XFB is enabled.
       */
      for (unsigned idx = 0; idx < MAX_XFB_BUFFERS; idx++) {
         struct anv_xfb_binding *xfb = &cmd_buffer->state.xfb_bindings[idx];
         anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_SO_BUFFER), sob) {
#if GFX_VER < 12
            sob.SOBufferIndex = idx;
#else
            sob._3DCommandOpcode = 0;
            sob._3DCommandSubOpcode = SO_BUFFER_INDEX_0_CMD + idx;
#endif

            if (cmd_buffer->state.xfb_enabled && xfb->buffer && xfb->size != 0) {
               sob.MOCS = anv_mocs(cmd_buffer->device, xfb->buffer->address.bo, 0);
               sob.SurfaceBaseAddress = anv_address_add(xfb->buffer->address,
                                                        xfb->offset);
#if GFX_VER >= 8
               sob.SOBufferEnable = true;
               sob.StreamOffsetWriteEnable = false;
               /* Size is in DWords - 1 */
               sob.SurfaceSize = DIV_ROUND_UP(xfb->size, 4) - 1;
#else
               /* We don't have SOBufferEnable in 3DSTATE_SO_BUFFER on Gfx7 so
                * we trust in SurfaceEndAddress = SurfaceBaseAddress = 0 (the
                * default for an empty SO_BUFFER packet) to disable them.
                */
               sob.SurfacePitch = pipeline->gfx7.xfb_bo_pitch[idx];
               sob.SurfaceEndAddress = anv_address_add(xfb->buffer->address,
                                                       xfb->offset + xfb->size);
#endif
            }
         }
      }

      /* CNL and later require a CS stall after 3DSTATE_SO_BUFFER */
      if (GFX_VER >= 10) {
         anv_add_pending_pipe_bits(cmd_buffer,
                                   ANV_PIPE_CS_STALL_BIT,
                                   "after 3DSTATE_SO_BUFFER call");
      }
   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE) {
      anv_batch_emit_batch(&cmd_buffer->batch, &pipeline->base.batch);

      /* Remove from dynamic state emission all of stuff that is baked into
       * the pipeline.
       */
      cmd_buffer->state.gfx.dirty &= ~pipeline->static_state_mask;

      /* If the pipeline changed, we may need to re-allocate push constant
       * space in the URB.
       */
      cmd_buffer_alloc_push_constants(cmd_buffer);
   }

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE)
      cmd_buffer->state.gfx.primitive_topology = pipeline->topology;

#if GFX_VER <= 7
   if (cmd_buffer->state.descriptors_dirty & VK_SHADER_STAGE_VERTEX_BIT ||
       cmd_buffer->state.push_constants_dirty & VK_SHADER_STAGE_VERTEX_BIT) {
      /* From the IVB PRM Vol. 2, Part 1, Section 3.2.1:
       *
       *    "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth
       *    stall needs to be sent just prior to any 3DSTATE_VS,
       *    3DSTATE_URB_VS, 3DSTATE_CONSTANT_VS,
       *    3DSTATE_BINDING_TABLE_POINTER_VS,
       *    3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one
       *    PIPE_CONTROL needs to be sent before any combination of VS
       *    associated 3DSTATE."
       */
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
         pc.DepthStallEnable  = true;
         pc.PostSyncOperation = WriteImmediateData;
         pc.Address           = cmd_buffer->device->workaround_address;
         anv_debug_dump_pc(pc);
      }
   }
#endif

   /* Render targets live in the same binding table as fragment descriptors */
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_RENDER_TARGETS)
      descriptors_dirty |= VK_SHADER_STAGE_FRAGMENT_BIT;

   /* We emit the binding tables and sampler tables first, then emit push
    * constants and then finally emit binding table and sampler table
    * pointers.  It has to happen in this order, since emitting the binding
    * tables may change the push constants (in case of storage images). After
    * emitting push constants, on SKL+ we have to emit the corresponding
    * 3DSTATE_BINDING_TABLE_POINTER_* for the push constants to take effect.
    */
   uint32_t dirty = 0;
   if (descriptors_dirty) {
      dirty = flush_descriptor_sets(cmd_buffer,
                                    &cmd_buffer->state.gfx.base,
                                    descriptors_dirty,
                                    pipeline->shaders,
                                    ARRAY_SIZE(pipeline->shaders));
      cmd_buffer->state.descriptors_dirty &= ~dirty;
   }

   if (dirty || cmd_buffer->state.push_constants_dirty) {
      /* Because we're pushing UBOs, we have to push whenever either
       * descriptors or push constants is dirty.
       */
      dirty |= cmd_buffer->state.push_constants_dirty;
      dirty &= ANV_STAGE_MASK & VK_SHADER_STAGE_ALL_GRAPHICS;
      cmd_buffer_flush_push_constants(cmd_buffer, dirty);
   }

   if (dirty)
      cmd_buffer_emit_descriptor_pointers(cmd_buffer, dirty);

   cmd_buffer_emit_clip(cmd_buffer);

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_RASTERIZER_DISCARD_ENABLE)
      cmd_buffer_emit_streamout(cmd_buffer);

   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_DYNAMIC_VIEWPORT)
      gfx8_cmd_buffer_emit_viewport(cmd_buffer);

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_DYNAMIC_VIEWPORT |
                                  ANV_CMD_DIRTY_PIPELINE)) {
      gfx8_cmd_buffer_emit_depth_viewport(cmd_buffer,
                                          pipeline->depth_clamp_enable);
   }

   if (cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_DYNAMIC_SCISSOR |
                                      ANV_CMD_DIRTY_RENDER_TARGETS))
      gfx7_cmd_buffer_emit_scissor(cmd_buffer);

   genX(cmd_buffer_flush_dynamic_state)(cmd_buffer);
}

static void
emit_vertex_bo(struct anv_cmd_buffer *cmd_buffer,
               struct anv_address addr,
               uint32_t size, uint32_t index)
{
   uint32_t *p = anv_batch_emitn(&cmd_buffer->batch, 5,
                                 GENX(3DSTATE_VERTEX_BUFFERS));

   GENX(VERTEX_BUFFER_STATE_pack)(&cmd_buffer->batch, p + 1,
      &(struct GENX(VERTEX_BUFFER_STATE)) {
         .VertexBufferIndex = index,
         .AddressModifyEnable = true,
         .BufferPitch = 0,
         .MOCS = addr.bo ? anv_mocs(cmd_buffer->device, addr.bo,
                                    ISL_SURF_USAGE_VERTEX_BUFFER_BIT) : 0,
         .NullVertexBuffer = size == 0,
#if GFX_VER >= 12
         .L3BypassDisable = true,
#endif
#if (GFX_VER >= 8)
         .BufferStartingAddress = addr,
         .BufferSize = size
#else
         .BufferStartingAddress = addr,
         .EndAddress = anv_address_add(addr, size),
#endif
      });

   genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(cmd_buffer,
                                                  index, addr, size);
}

static void
emit_base_vertex_instance_bo(struct anv_cmd_buffer *cmd_buffer,
                             struct anv_address addr)
{
   emit_vertex_bo(cmd_buffer, addr, addr.bo ? 8 : 0, ANV_SVGS_VB_INDEX);
}

static void
emit_base_vertex_instance(struct anv_cmd_buffer *cmd_buffer,
                          uint32_t base_vertex, uint32_t base_instance)
{
   if (base_vertex == 0 && base_instance == 0) {
      emit_base_vertex_instance_bo(cmd_buffer, ANV_NULL_ADDRESS);
   } else {
      struct anv_state id_state =
         anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, 8, 4);

      ((uint32_t *)id_state.map)[0] = base_vertex;
      ((uint32_t *)id_state.map)[1] = base_instance;

      struct anv_address addr = {
         .bo = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
         .offset = id_state.offset,
      };

      emit_base_vertex_instance_bo(cmd_buffer, addr);
   }
}

static void
emit_draw_index(struct anv_cmd_buffer *cmd_buffer, uint32_t draw_index)
{
   struct anv_state state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, 4, 4);

   ((uint32_t *)state.map)[0] = draw_index;

   struct anv_address addr = {
      .bo = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
      .offset = state.offset,
   };

   emit_vertex_bo(cmd_buffer, addr, 4, ANV_DRAWID_VB_INDEX);
}

static void
update_dirty_vbs_for_gfx8_vb_flush(struct anv_cmd_buffer *cmd_buffer,
                                   uint32_t access_type)
{
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   uint64_t vb_used = pipeline->vb_used;
   if (vs_prog_data->uses_firstvertex ||
       vs_prog_data->uses_baseinstance)
      vb_used |= 1ull << ANV_SVGS_VB_INDEX;
   if (vs_prog_data->uses_drawid)
      vb_used |= 1ull << ANV_DRAWID_VB_INDEX;

   genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)(cmd_buffer,
                                                       access_type == RANDOM,
                                                       vb_used);
}

ALWAYS_INLINE static void
cmd_buffer_emit_vertex_constants_and_flush(struct anv_cmd_buffer *cmd_buffer,
                                           const struct brw_vs_prog_data *vs_prog_data,
                                           uint32_t base_vertex,
                                           uint32_t base_instance,
                                           uint32_t draw_id,
                                           bool force_flush)
{
   bool emitted = false;
   if (vs_prog_data->uses_firstvertex ||
       vs_prog_data->uses_baseinstance) {
      emit_base_vertex_instance(cmd_buffer, base_vertex, base_instance);
      emitted = true;
   }
   if (vs_prog_data->uses_drawid) {
      emit_draw_index(cmd_buffer, draw_id);
      emitted = true;
   }
   /* Emitting draw index or vertex index BOs may result in needing
    * additional VF cache flushes.
    */
   if (emitted || force_flush)
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);
}

void genX(CmdDraw)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   const uint32_t count = (vertexCount *
                           instanceCount *
                           (pipeline->use_primitive_replication ?
                            1 : anv_subpass_view_count(cmd_buffer->state.subpass)));
   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_DRAW,
                        "draw", count);

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer, vs_prog_data,
                                              firstVertex, firstInstance, 0,
                                              true);

   /* Our implementation of VK_KHR_multiview uses instancing to draw the
    * different views.  We need to multiply instanceCount by the view count.
    */
   if (!pipeline->use_primitive_replication)
      instanceCount *= anv_subpass_view_count(cmd_buffer->state.subpass);

   anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
      prim.VertexAccessType         = SEQUENTIAL;
      prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
      prim.VertexCountPerInstance   = vertexCount;
      prim.StartVertexLocation      = firstVertex;
      prim.InstanceCount            = instanceCount;
      prim.StartInstanceLocation    = firstInstance;
      prim.BaseVertexLocation       = 0;
   }

   update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, SEQUENTIAL);
}

void genX(CmdDrawMultiEXT)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    drawCount,
    const VkMultiDrawInfoEXT                   *pVertexInfo,
    uint32_t                                    instanceCount,
    uint32_t                                    firstInstance,
    uint32_t                                    stride)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   const uint32_t count = (drawCount *
                           instanceCount *
                           (pipeline->use_primitive_replication ?
                            1 : anv_subpass_view_count(cmd_buffer->state.subpass)));
   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_DRAW,
                        "draw_multi", count);

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   /* Our implementation of VK_KHR_multiview uses instancing to draw the
    * different views.  We need to multiply instanceCount by the view count.
    */
   if (!pipeline->use_primitive_replication)
      instanceCount *= anv_subpass_view_count(cmd_buffer->state.subpass);

   uint32_t i = 0;
   vk_foreach_multi_draw(draw, i, pVertexInfo, drawCount, stride) {
      cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer, vs_prog_data,
                                                 draw->firstVertex,
                                                 firstInstance, i, !i);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
         prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
         prim.VertexAccessType         = SEQUENTIAL;
         prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
         prim.VertexCountPerInstance   = draw->vertexCount;
         prim.StartVertexLocation      = draw->firstVertex;
         prim.InstanceCount            = instanceCount;
         prim.StartInstanceLocation    = firstInstance;
         prim.BaseVertexLocation       = 0;
      }
   }

   update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, SEQUENTIAL);
}

void genX(CmdDrawIndexed)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   const uint32_t count = (indexCount *
                           instanceCount *
                           (pipeline->use_primitive_replication ?
                            1 : anv_subpass_view_count(cmd_buffer->state.subpass)));
   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_DRAW,
                        "draw indexed",
                        count);

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer, vs_prog_data, vertexOffset, firstInstance, 0, true);

   /* Our implementation of VK_KHR_multiview uses instancing to draw the
    * different views.  We need to multiply instanceCount by the view count.
    */
   if (!pipeline->use_primitive_replication)
      instanceCount *= anv_subpass_view_count(cmd_buffer->state.subpass);

   anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
      prim.VertexAccessType         = RANDOM;
      prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
      prim.VertexCountPerInstance   = indexCount;
      prim.StartVertexLocation      = firstIndex;
      prim.InstanceCount            = instanceCount;
      prim.StartInstanceLocation    = firstInstance;
      prim.BaseVertexLocation       = vertexOffset;
   }

   update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, RANDOM);
}

void genX(CmdDrawMultiIndexedEXT)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    drawCount,
    const VkMultiDrawIndexedInfoEXT            *pIndexInfo,
    uint32_t                                    instanceCount,
    uint32_t                                    firstInstance,
    uint32_t                                    stride,
    const int32_t                              *pVertexOffset)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   const uint32_t count = (drawCount *
                           instanceCount *
                           (pipeline->use_primitive_replication ?
                            1 : anv_subpass_view_count(cmd_buffer->state.subpass)));
   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_DRAW,
                        "draw indexed_multi",
                        count);

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   /* Our implementation of VK_KHR_multiview uses instancing to draw the
    * different views.  We need to multiply instanceCount by the view count.
    */
   if (!pipeline->use_primitive_replication)
      instanceCount *= anv_subpass_view_count(cmd_buffer->state.subpass);

   uint32_t i = 0;
   if (pVertexOffset) {
      if (vs_prog_data->uses_drawid) {
         bool emitted = true;
         if (vs_prog_data->uses_firstvertex ||
             vs_prog_data->uses_baseinstance) {
            emit_base_vertex_instance(cmd_buffer, *pVertexOffset, firstInstance);
            emitted = true;
         }
         vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
            if (vs_prog_data->uses_drawid) {
               emit_draw_index(cmd_buffer, i);
               emitted = true;
            }
            /* Emitting draw index or vertex index BOs may result in needing
             * additional VF cache flushes.
             */
            if (emitted)
               genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

            anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
               prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
               prim.VertexAccessType         = RANDOM;
               prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
               prim.VertexCountPerInstance   = draw->indexCount;
               prim.StartVertexLocation      = draw->firstIndex;
               prim.InstanceCount            = instanceCount;
               prim.StartInstanceLocation    = firstInstance;
               prim.BaseVertexLocation       = *pVertexOffset;
            }
            emitted = false;
         }
      } else {
         if (vs_prog_data->uses_firstvertex ||
             vs_prog_data->uses_baseinstance) {
            emit_base_vertex_instance(cmd_buffer, *pVertexOffset, firstInstance);
            /* Emitting draw index or vertex index BOs may result in needing
             * additional VF cache flushes.
             */
            genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);
         }
         vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
            anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
               prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
               prim.VertexAccessType         = RANDOM;
               prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
               prim.VertexCountPerInstance   = draw->indexCount;
               prim.StartVertexLocation      = draw->firstIndex;
               prim.InstanceCount            = instanceCount;
               prim.StartInstanceLocation    = firstInstance;
               prim.BaseVertexLocation       = *pVertexOffset;
            }
         }
      }
   } else {
      vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
         cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer, vs_prog_data,
                                                    draw->vertexOffset,
                                                    firstInstance, i, i != 0);

         anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
            prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
            prim.VertexAccessType         = RANDOM;
            prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
            prim.VertexCountPerInstance   = draw->indexCount;
            prim.StartVertexLocation      = draw->firstIndex;
            prim.InstanceCount            = instanceCount;
            prim.StartInstanceLocation    = firstInstance;
            prim.BaseVertexLocation       = draw->vertexOffset;
         }
      }
   }

   update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, RANDOM);
}

/* Auto-Draw / Indirect Registers */
#define GFX7_3DPRIM_END_OFFSET          0x2420
#define GFX7_3DPRIM_START_VERTEX        0x2430
#define GFX7_3DPRIM_VERTEX_COUNT        0x2434
#define GFX7_3DPRIM_INSTANCE_COUNT      0x2438
#define GFX7_3DPRIM_START_INSTANCE      0x243C
#define GFX7_3DPRIM_BASE_VERTEX         0x2440

void genX(CmdDrawIndirectByteCountEXT)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    instanceCount,
    uint32_t                                    firstInstance,
    VkBuffer                                    counterBuffer,
    VkDeviceSize                                counterBufferOffset,
    uint32_t                                    counterOffset,
    uint32_t                                    vertexStride)
{
#if GFX_VERx10 >= 75
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, counter_buffer, counterBuffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   /* firstVertex is always zero for this draw function */
   const uint32_t firstVertex = 0;

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_DRAW,
                        "draw indirect byte count",
                        instanceCount);

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   if (vs_prog_data->uses_firstvertex ||
       vs_prog_data->uses_baseinstance)
      emit_base_vertex_instance(cmd_buffer, firstVertex, firstInstance);
   if (vs_prog_data->uses_drawid)
      emit_draw_index(cmd_buffer, 0);

   /* Emitting draw index or vertex index BOs may result in needing
    * additional VF cache flushes.
    */
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   /* Our implementation of VK_KHR_multiview uses instancing to draw the
    * different views.  We need to multiply instanceCount by the view count.
    */
   if (!pipeline->use_primitive_replication)
      instanceCount *= anv_subpass_view_count(cmd_buffer->state.subpass);

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);
   struct mi_value count =
      mi_mem32(anv_address_add(counter_buffer->address,
                                   counterBufferOffset));
   if (counterOffset)
      count = mi_isub(&b, count, mi_imm(counterOffset));
   count = mi_udiv32_imm(&b, count, vertexStride);
   mi_store(&b, mi_reg32(GFX7_3DPRIM_VERTEX_COUNT), count);

   mi_store(&b, mi_reg32(GFX7_3DPRIM_START_VERTEX), mi_imm(firstVertex));
   mi_store(&b, mi_reg32(GFX7_3DPRIM_INSTANCE_COUNT), mi_imm(instanceCount));
   mi_store(&b, mi_reg32(GFX7_3DPRIM_START_INSTANCE), mi_imm(firstInstance));
   mi_store(&b, mi_reg32(GFX7_3DPRIM_BASE_VERTEX), mi_imm(0));

   anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.IndirectParameterEnable  = true;
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
      prim.VertexAccessType         = SEQUENTIAL;
      prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
   }

   update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, SEQUENTIAL);
#endif /* GFX_VERx10 >= 75 */
}

static void
load_indirect_parameters(struct anv_cmd_buffer *cmd_buffer,
                         struct anv_address addr,
                         bool indexed)
{
   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   mi_store(&b, mi_reg32(GFX7_3DPRIM_VERTEX_COUNT),
                mi_mem32(anv_address_add(addr, 0)));

   struct mi_value instance_count = mi_mem32(anv_address_add(addr, 4));
   unsigned view_count = anv_subpass_view_count(cmd_buffer->state.subpass);
   if (view_count > 1) {
#if GFX_VERx10 >= 75
      instance_count = mi_imul_imm(&b, instance_count, view_count);
#else
      anv_finishme("Multiview + indirect draw requires MI_MATH; "
                   "MI_MATH is not supported on Ivy Bridge");
#endif
   }
   mi_store(&b, mi_reg32(GFX7_3DPRIM_INSTANCE_COUNT), instance_count);

   mi_store(&b, mi_reg32(GFX7_3DPRIM_START_VERTEX),
                mi_mem32(anv_address_add(addr, 8)));

   if (indexed) {
      mi_store(&b, mi_reg32(GFX7_3DPRIM_BASE_VERTEX),
                   mi_mem32(anv_address_add(addr, 12)));
      mi_store(&b, mi_reg32(GFX7_3DPRIM_START_INSTANCE),
                   mi_mem32(anv_address_add(addr, 16)));
   } else {
      mi_store(&b, mi_reg32(GFX7_3DPRIM_START_INSTANCE),
                   mi_mem32(anv_address_add(addr, 12)));
      mi_store(&b, mi_reg32(GFX7_3DPRIM_BASE_VERTEX), mi_imm(0));
   }
}

void genX(CmdDrawIndirect)(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    _buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   for (uint32_t i = 0; i < drawCount; i++) {
      struct anv_address draw = anv_address_add(buffer->address, offset);

      if (vs_prog_data->uses_firstvertex ||
          vs_prog_data->uses_baseinstance)
         emit_base_vertex_instance_bo(cmd_buffer, anv_address_add(draw, 8));
      if (vs_prog_data->uses_drawid)
         emit_draw_index(cmd_buffer, i);

      /* Emitting draw index or vertex index BOs may result in needing
       * additional VF cache flushes.
       */
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      load_indirect_parameters(cmd_buffer, draw, false);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
         prim.IndirectParameterEnable  = true;
         prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
         prim.VertexAccessType         = SEQUENTIAL;
         prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
      }

      update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, SEQUENTIAL);

      offset += stride;
   }
}

void genX(CmdDrawIndexedIndirect)(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    _buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   genX(cmd_buffer_flush_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   for (uint32_t i = 0; i < drawCount; i++) {
      struct anv_address draw = anv_address_add(buffer->address, offset);

      /* TODO: We need to stomp base vertex to 0 somehow */
      if (vs_prog_data->uses_firstvertex ||
          vs_prog_data->uses_baseinstance)
         emit_base_vertex_instance_bo(cmd_buffer, anv_address_add(draw, 12));
      if (vs_prog_data->uses_drawid)
         emit_draw_index(cmd_buffer, i);

      /* Emitting draw index or vertex index BOs may result in needing
       * additional VF cache flushes.
       */
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      load_indirect_parameters(cmd_buffer, draw, true);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
         prim.IndirectParameterEnable  = true;
         prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
         prim.VertexAccessType         = RANDOM;
         prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
      }

      update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, RANDOM);

      offset += stride;
   }
}

static struct mi_value
prepare_for_draw_count_predicate(struct anv_cmd_buffer *cmd_buffer,
                                 struct mi_builder *b,
                                 struct anv_buffer *count_buffer,
                                 uint64_t countBufferOffset)
{
   struct anv_address count_address =
         anv_address_add(count_buffer->address, countBufferOffset);

   struct mi_value ret = mi_imm(0);

   if (cmd_buffer->state.conditional_render_enabled) {
#if GFX_VERx10 >= 75
      ret = mi_new_gpr(b);
      mi_store(b, mi_value_ref(b, ret), mi_mem32(count_address));
#endif
   } else {
      /* Upload the current draw count from the draw parameters buffer to
       * MI_PREDICATE_SRC0.
       */
      mi_store(b, mi_reg64(MI_PREDICATE_SRC0), mi_mem32(count_address));
      mi_store(b, mi_reg32(MI_PREDICATE_SRC1 + 4), mi_imm(0));
   }

   return ret;
}

static void
emit_draw_count_predicate(struct anv_cmd_buffer *cmd_buffer,
                          struct mi_builder *b,
                          uint32_t draw_index)
{
   /* Upload the index of the current primitive to MI_PREDICATE_SRC1. */
   mi_store(b, mi_reg32(MI_PREDICATE_SRC1), mi_imm(draw_index));

   if (draw_index == 0) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
         mip.LoadOperation    = LOAD_LOADINV;
         mip.CombineOperation = COMBINE_SET;
         mip.CompareOperation = COMPARE_SRCS_EQUAL;
      }
   } else {
      /* While draw_index < draw_count the predicate's result will be
       *  (draw_index == draw_count) ^ TRUE = TRUE
       * When draw_index == draw_count the result is
       *  (TRUE) ^ TRUE = FALSE
       * After this all results will be:
       *  (FALSE) ^ FALSE = FALSE
       */
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
         mip.LoadOperation    = LOAD_LOAD;
         mip.CombineOperation = COMBINE_XOR;
         mip.CompareOperation = COMPARE_SRCS_EQUAL;
      }
   }
}

#if GFX_VERx10 >= 75
static void
emit_draw_count_predicate_with_conditional_render(
                          struct anv_cmd_buffer *cmd_buffer,
                          struct mi_builder *b,
                          uint32_t draw_index,
                          struct mi_value max)
{
   struct mi_value pred = mi_ult(b, mi_imm(draw_index), max);
   pred = mi_iand(b, pred, mi_reg64(ANV_PREDICATE_RESULT_REG));

#if GFX_VER >= 8
   mi_store(b, mi_reg32(MI_PREDICATE_RESULT), pred);
#else
   /* MI_PREDICATE_RESULT is not whitelisted in i915 command parser
    * so we emit MI_PREDICATE to set it.
    */

   mi_store(b, mi_reg64(MI_PREDICATE_SRC0), pred);
   mi_store(b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(0));

   anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
      mip.CombineOperation = COMBINE_SET;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }
#endif
}
#endif

static void
emit_draw_count_predicate_cond(struct anv_cmd_buffer *cmd_buffer,
                               struct mi_builder *b,
                               uint32_t draw_index,
                               struct mi_value max)
{
#if GFX_VERx10 >= 75
   if (cmd_buffer->state.conditional_render_enabled) {
      emit_draw_count_predicate_with_conditional_render(
            cmd_buffer, b, draw_index, mi_value_ref(b, max));
   } else {
      emit_draw_count_predicate(cmd_buffer, b, draw_index);
   }
#else
   emit_draw_count_predicate(cmd_buffer, b, draw_index);
#endif
}

void genX(CmdDrawIndirectCount)(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    _buffer,
    VkDeviceSize                                offset,
    VkBuffer                                    _countBuffer,
    VkDeviceSize                                countBufferOffset,
    uint32_t                                    maxDrawCount,
    uint32_t                                    stride)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   ANV_FROM_HANDLE(anv_buffer, count_buffer, _countBuffer);
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;
   struct anv_graphics_pipeline *pipeline = cmd_state->gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   genX(cmd_buffer_flush_state)(cmd_buffer);

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);
   struct mi_value max =
      prepare_for_draw_count_predicate(cmd_buffer, &b,
                                       count_buffer, countBufferOffset);

   for (uint32_t i = 0; i < maxDrawCount; i++) {
      struct anv_address draw = anv_address_add(buffer->address, offset);

      emit_draw_count_predicate_cond(cmd_buffer, &b, i, max);

      if (vs_prog_data->uses_firstvertex ||
          vs_prog_data->uses_baseinstance)
         emit_base_vertex_instance_bo(cmd_buffer, anv_address_add(draw, 8));
      if (vs_prog_data->uses_drawid)
         emit_draw_index(cmd_buffer, i);

      /* Emitting draw index or vertex index BOs may result in needing
       * additional VF cache flushes.
       */
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      load_indirect_parameters(cmd_buffer, draw, false);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
         prim.IndirectParameterEnable  = true;
         prim.PredicateEnable          = true;
         prim.VertexAccessType         = SEQUENTIAL;
         prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
      }

      update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, SEQUENTIAL);

      offset += stride;
   }

   mi_value_unref(&b, max);
}

void genX(CmdDrawIndexedIndirectCount)(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    _buffer,
    VkDeviceSize                                offset,
    VkBuffer                                    _countBuffer,
    VkDeviceSize                                countBufferOffset,
    uint32_t                                    maxDrawCount,
    uint32_t                                    stride)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   ANV_FROM_HANDLE(anv_buffer, count_buffer, _countBuffer);
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;
   struct anv_graphics_pipeline *pipeline = cmd_state->gfx.pipeline;
   const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   genX(cmd_buffer_flush_state)(cmd_buffer);

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);
   struct mi_value max =
      prepare_for_draw_count_predicate(cmd_buffer, &b,
                                       count_buffer, countBufferOffset);

   for (uint32_t i = 0; i < maxDrawCount; i++) {
      struct anv_address draw = anv_address_add(buffer->address, offset);

      emit_draw_count_predicate_cond(cmd_buffer, &b, i, max);

      /* TODO: We need to stomp base vertex to 0 somehow */
      if (vs_prog_data->uses_firstvertex ||
          vs_prog_data->uses_baseinstance)
         emit_base_vertex_instance_bo(cmd_buffer, anv_address_add(draw, 12));
      if (vs_prog_data->uses_drawid)
         emit_draw_index(cmd_buffer, i);

      /* Emitting draw index or vertex index BOs may result in needing
       * additional VF cache flushes.
       */
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      load_indirect_parameters(cmd_buffer, draw, true);

      anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
         prim.IndirectParameterEnable  = true;
         prim.PredicateEnable          = true;
         prim.VertexAccessType         = RANDOM;
         prim.PrimitiveTopologyType    = cmd_buffer->state.gfx.primitive_topology;
      }

      update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer, RANDOM);

      offset += stride;
   }

   mi_value_unref(&b, max);
}

void genX(CmdBeginTransformFeedbackEXT)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstCounterBuffer,
    uint32_t                                    counterBufferCount,
    const VkBuffer*                             pCounterBuffers,
    const VkDeviceSize*                         pCounterBufferOffsets)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   assert(firstCounterBuffer < MAX_XFB_BUFFERS);
   assert(counterBufferCount <= MAX_XFB_BUFFERS);
   assert(firstCounterBuffer + counterBufferCount <= MAX_XFB_BUFFERS);

   /* From the SKL PRM Vol. 2c, SO_WRITE_OFFSET:
    *
    *    "Ssoftware must ensure that no HW stream output operations can be in
    *    process or otherwise pending at the point that the MI_LOAD/STORE
    *    commands are processed. This will likely require a pipeline flush."
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_CS_STALL_BIT,
                             "begin transform feedback");
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   for (uint32_t idx = 0; idx < MAX_XFB_BUFFERS; idx++) {
      /* If we have a counter buffer, this is a resume so we need to load the
       * value into the streamout offset register.  Otherwise, this is a begin
       * and we need to reset it to zero.
       */
      if (pCounterBuffers &&
          idx >= firstCounterBuffer &&
          idx - firstCounterBuffer < counterBufferCount &&
          pCounterBuffers[idx - firstCounterBuffer] != VK_NULL_HANDLE) {
         uint32_t cb_idx = idx - firstCounterBuffer;
         ANV_FROM_HANDLE(anv_buffer, counter_buffer, pCounterBuffers[cb_idx]);
         uint64_t offset = pCounterBufferOffsets ?
                           pCounterBufferOffsets[cb_idx] : 0;

         anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_MEM), lrm) {
            lrm.RegisterAddress  = GENX(SO_WRITE_OFFSET0_num) + idx * 4;
            lrm.MemoryAddress    = anv_address_add(counter_buffer->address,
                                                   offset);
         }
      } else {
         anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
            lri.RegisterOffset   = GENX(SO_WRITE_OFFSET0_num) + idx * 4;
            lri.DataDWord        = 0;
         }
      }
   }

   cmd_buffer->state.xfb_enabled = true;
   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_XFB_ENABLE;
}

void genX(CmdEndTransformFeedbackEXT)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstCounterBuffer,
    uint32_t                                    counterBufferCount,
    const VkBuffer*                             pCounterBuffers,
    const VkDeviceSize*                         pCounterBufferOffsets)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   assert(firstCounterBuffer < MAX_XFB_BUFFERS);
   assert(counterBufferCount <= MAX_XFB_BUFFERS);
   assert(firstCounterBuffer + counterBufferCount <= MAX_XFB_BUFFERS);

   /* From the SKL PRM Vol. 2c, SO_WRITE_OFFSET:
    *
    *    "Ssoftware must ensure that no HW stream output operations can be in
    *    process or otherwise pending at the point that the MI_LOAD/STORE
    *    commands are processed. This will likely require a pipeline flush."
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_CS_STALL_BIT,
                             "end transform feedback");
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   for (uint32_t cb_idx = 0; cb_idx < counterBufferCount; cb_idx++) {
      unsigned idx = firstCounterBuffer + cb_idx;

      /* If we have a counter buffer, this is a resume so we need to load the
       * value into the streamout offset register.  Otherwise, this is a begin
       * and we need to reset it to zero.
       */
      if (pCounterBuffers &&
          cb_idx < counterBufferCount &&
          pCounterBuffers[cb_idx] != VK_NULL_HANDLE) {
         ANV_FROM_HANDLE(anv_buffer, counter_buffer, pCounterBuffers[cb_idx]);
         uint64_t offset = pCounterBufferOffsets ?
                           pCounterBufferOffsets[cb_idx] : 0;

         anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_REGISTER_MEM), srm) {
            srm.MemoryAddress    = anv_address_add(counter_buffer->address,
                                                   offset);
            srm.RegisterAddress  = GENX(SO_WRITE_OFFSET0_num) + idx * 4;
         }
      }
   }

   cmd_buffer->state.xfb_enabled = false;
   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_XFB_ENABLE;
}

void
genX(cmd_buffer_flush_compute_state)(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_cmd_compute_state *comp_state = &cmd_buffer->state.compute;
   struct anv_compute_pipeline *pipeline = comp_state->pipeline;

   assert(pipeline->cs);

   genX(cmd_buffer_config_l3)(cmd_buffer, pipeline->base.l3_config);

   genX(flush_pipeline_select_gpgpu)(cmd_buffer);

   /* Apply any pending pipeline flushes we may have.  We want to apply them
    * now because, if any of those flushes are for things like push constants,
    * the GPU will read the state at weird times.
    */
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   if (cmd_buffer->state.compute.pipeline_dirty) {
      /* From the Sky Lake PRM Vol 2a, MEDIA_VFE_STATE:
       *
       *    "A stalling PIPE_CONTROL is required before MEDIA_VFE_STATE unless
       *    the only bits that are changed are scoreboard related: Scoreboard
       *    Enable, Scoreboard Type, Scoreboard Mask, Scoreboard * Delta. For
       *    these scoreboard related states, a MEDIA_STATE_FLUSH is
       *    sufficient."
       */
      anv_add_pending_pipe_bits(cmd_buffer,
                              ANV_PIPE_CS_STALL_BIT,
                              "flush compute state");
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      anv_batch_emit_batch(&cmd_buffer->batch, &pipeline->base.batch);

      /* The workgroup size of the pipeline affects our push constant layout
       * so flag push constants as dirty if we change the pipeline.
       */
      cmd_buffer->state.push_constants_dirty |= VK_SHADER_STAGE_COMPUTE_BIT;
   }

   if ((cmd_buffer->state.descriptors_dirty & VK_SHADER_STAGE_COMPUTE_BIT) ||
       cmd_buffer->state.compute.pipeline_dirty) {
      flush_descriptor_sets(cmd_buffer,
                            &cmd_buffer->state.compute.base,
                            VK_SHADER_STAGE_COMPUTE_BIT,
                            &pipeline->cs, 1);
      cmd_buffer->state.descriptors_dirty &= ~VK_SHADER_STAGE_COMPUTE_BIT;

#if GFX_VERx10 < 125
      uint32_t iface_desc_data_dw[GENX(INTERFACE_DESCRIPTOR_DATA_length)];
      struct GENX(INTERFACE_DESCRIPTOR_DATA) desc = {
         .BindingTablePointer =
            cmd_buffer->state.binding_tables[MESA_SHADER_COMPUTE].offset,
         .SamplerStatePointer =
            cmd_buffer->state.samplers[MESA_SHADER_COMPUTE].offset,
      };
      GENX(INTERFACE_DESCRIPTOR_DATA_pack)(NULL, iface_desc_data_dw, &desc);

      struct anv_state state =
         anv_cmd_buffer_merge_dynamic(cmd_buffer, iface_desc_data_dw,
                                      pipeline->interface_descriptor_data,
                                      GENX(INTERFACE_DESCRIPTOR_DATA_length),
                                      64);

      uint32_t size = GENX(INTERFACE_DESCRIPTOR_DATA_length) * sizeof(uint32_t);
      anv_batch_emit(&cmd_buffer->batch,
                     GENX(MEDIA_INTERFACE_DESCRIPTOR_LOAD), mid) {
         mid.InterfaceDescriptorTotalLength        = size;
         mid.InterfaceDescriptorDataStartAddress   = state.offset;
      }
#endif
   }

   if (cmd_buffer->state.push_constants_dirty & VK_SHADER_STAGE_COMPUTE_BIT) {
      comp_state->push_data =
         anv_cmd_buffer_cs_push_constants(cmd_buffer);

#if GFX_VERx10 < 125
      if (comp_state->push_data.alloc_size) {
         anv_batch_emit(&cmd_buffer->batch, GENX(MEDIA_CURBE_LOAD), curbe) {
            curbe.CURBETotalDataLength    = comp_state->push_data.alloc_size;
            curbe.CURBEDataStartAddress   = comp_state->push_data.offset;
         }
      }
#endif

      cmd_buffer->state.push_constants_dirty &= ~VK_SHADER_STAGE_COMPUTE_BIT;
   }

   cmd_buffer->state.compute.pipeline_dirty = false;

   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);
}

#if GFX_VER == 7

static VkResult
verify_cmd_parser(const struct anv_device *device,
                  int required_version,
                  const char *function)
{
   if (device->physical->cmd_parser_version < required_version) {
      return vk_errorf(device->physical, VK_ERROR_FEATURE_NOT_PRESENT,
                       "cmd parser version %d is required for %s",
                       required_version, function);
   } else {
      return VK_SUCCESS;
   }
}

#endif

static void
anv_cmd_buffer_push_base_group_id(struct anv_cmd_buffer *cmd_buffer,
                                  uint32_t baseGroupX,
                                  uint32_t baseGroupY,
                                  uint32_t baseGroupZ)
{
   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   struct anv_push_constants *push =
      &cmd_buffer->state.compute.base.push_constants;
   if (push->cs.base_work_group_id[0] != baseGroupX ||
       push->cs.base_work_group_id[1] != baseGroupY ||
       push->cs.base_work_group_id[2] != baseGroupZ) {
      push->cs.base_work_group_id[0] = baseGroupX;
      push->cs.base_work_group_id[1] = baseGroupY;
      push->cs.base_work_group_id[2] = baseGroupZ;

      cmd_buffer->state.push_constants_dirty |= VK_SHADER_STAGE_COMPUTE_BIT;
   }
}

void genX(CmdDispatch)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
   genX(CmdDispatchBase)(commandBuffer, 0, 0, 0, x, y, z);
}

#if GFX_VERx10 >= 125

static inline void
emit_compute_walker(struct anv_cmd_buffer *cmd_buffer,
                    const struct anv_compute_pipeline *pipeline, bool indirect,
                    const struct brw_cs_prog_data *prog_data,
                    uint32_t groupCountX, uint32_t groupCountY,
                    uint32_t groupCountZ)
{
   struct anv_cmd_compute_state *comp_state = &cmd_buffer->state.compute;
   const struct anv_shader_bin *cs_bin = pipeline->cs;
   bool predicate = cmd_buffer->state.conditional_render_enabled;

   const struct intel_device_info *devinfo = &pipeline->base.device->info;
   const struct brw_cs_dispatch_info dispatch =
      brw_cs_get_dispatch_info(devinfo, prog_data, NULL);

   anv_batch_emit(&cmd_buffer->batch, GENX(COMPUTE_WALKER), cw) {
      cw.IndirectParameterEnable        = indirect;
      cw.PredicateEnable                = predicate;
      cw.SIMDSize                       = dispatch.simd_size / 16;
      cw.IndirectDataStartAddress       = comp_state->push_data.offset;
      cw.IndirectDataLength             = comp_state->push_data.alloc_size;
      cw.LocalXMaximum                  = prog_data->local_size[0] - 1;
      cw.LocalYMaximum                  = prog_data->local_size[1] - 1;
      cw.LocalZMaximum                  = prog_data->local_size[2] - 1;
      cw.ThreadGroupIDXDimension        = groupCountX;
      cw.ThreadGroupIDYDimension        = groupCountY;
      cw.ThreadGroupIDZDimension        = groupCountZ;
      cw.ExecutionMask                  = dispatch.right_mask;

      cw.InterfaceDescriptor = (struct GENX(INTERFACE_DESCRIPTOR_DATA)) {
         .KernelStartPointer = cs_bin->kernel.offset,
         .SamplerStatePointer =
            cmd_buffer->state.samplers[MESA_SHADER_COMPUTE].offset,
         .BindingTablePointer =
            cmd_buffer->state.binding_tables[MESA_SHADER_COMPUTE].offset,
         .BindingTableEntryCount =
            1 + MIN2(pipeline->cs->bind_map.surface_count, 30),
         .NumberofThreadsinGPGPUThreadGroup = dispatch.threads,
         .SharedLocalMemorySize = encode_slm_size(GFX_VER,
                                                  prog_data->base.total_shared),
         .NumberOfBarriers = prog_data->uses_barrier,
      };
   }
}

#else /* #if GFX_VERx10 >= 125 */

static inline void
emit_gpgpu_walker(struct anv_cmd_buffer *cmd_buffer,
                  const struct anv_compute_pipeline *pipeline, bool indirect,
                  const struct brw_cs_prog_data *prog_data,
                  uint32_t groupCountX, uint32_t groupCountY,
                  uint32_t groupCountZ)
{
   bool predicate = (GFX_VER <= 7 && indirect) ||
      cmd_buffer->state.conditional_render_enabled;

   const struct intel_device_info *devinfo = &pipeline->base.device->info;
   const struct brw_cs_dispatch_info dispatch =
      brw_cs_get_dispatch_info(devinfo, prog_data, NULL);

   anv_batch_emit(&cmd_buffer->batch, GENX(GPGPU_WALKER), ggw) {
      ggw.IndirectParameterEnable      = indirect;
      ggw.PredicateEnable              = predicate;
      ggw.SIMDSize                     = dispatch.simd_size / 16;
      ggw.ThreadDepthCounterMaximum    = 0;
      ggw.ThreadHeightCounterMaximum   = 0;
      ggw.ThreadWidthCounterMaximum    = dispatch.threads - 1;
      ggw.ThreadGroupIDXDimension      = groupCountX;
      ggw.ThreadGroupIDYDimension      = groupCountY;
      ggw.ThreadGroupIDZDimension      = groupCountZ;
      ggw.RightExecutionMask           = dispatch.right_mask;
      ggw.BottomExecutionMask          = 0xffffffff;
   }

   anv_batch_emit(&cmd_buffer->batch, GENX(MEDIA_STATE_FLUSH), msf);
}

#endif /* #if GFX_VERx10 >= 125 */

static inline void
emit_cs_walker(struct anv_cmd_buffer *cmd_buffer,
               const struct anv_compute_pipeline *pipeline, bool indirect,
               const struct brw_cs_prog_data *prog_data,
               uint32_t groupCountX, uint32_t groupCountY,
               uint32_t groupCountZ)
{
#if GFX_VERx10 >= 125
   emit_compute_walker(cmd_buffer, pipeline, indirect, prog_data, groupCountX,
                       groupCountY, groupCountZ);
#else
   emit_gpgpu_walker(cmd_buffer, pipeline, indirect, prog_data, groupCountX,
                     groupCountY, groupCountZ);
#endif
}

void genX(CmdDispatchBase)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    baseGroupX,
    uint32_t                                    baseGroupY,
    uint32_t                                    baseGroupZ,
    uint32_t                                    groupCountX,
    uint32_t                                    groupCountY,
    uint32_t                                    groupCountZ)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_compute_pipeline *pipeline = cmd_buffer->state.compute.pipeline;
   const struct brw_cs_prog_data *prog_data = get_cs_prog_data(pipeline);

   anv_cmd_buffer_push_base_group_id(cmd_buffer, baseGroupX,
                                     baseGroupY, baseGroupZ);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_COMPUTE,
                        "compute",
                        groupCountX * groupCountY * groupCountZ *
                        prog_data->local_size[0] * prog_data->local_size[1] *
                        prog_data->local_size[2]);

   if (prog_data->uses_num_work_groups) {
      struct anv_state state =
         anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, 12, 4);
      uint32_t *sizes = state.map;
      sizes[0] = groupCountX;
      sizes[1] = groupCountY;
      sizes[2] = groupCountZ;
      cmd_buffer->state.compute.num_workgroups = (struct anv_address) {
         .bo = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
         .offset = state.offset,
      };

      /* The num_workgroups buffer goes in the binding table */
      cmd_buffer->state.descriptors_dirty |= VK_SHADER_STAGE_COMPUTE_BIT;
   }

   genX(cmd_buffer_flush_compute_state)(cmd_buffer);

   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);

   emit_cs_walker(cmd_buffer, pipeline, false, prog_data, groupCountX,
                  groupCountY, groupCountZ);
}

#define GPGPU_DISPATCHDIMX 0x2500
#define GPGPU_DISPATCHDIMY 0x2504
#define GPGPU_DISPATCHDIMZ 0x2508

void genX(CmdDispatchIndirect)(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    _buffer,
    VkDeviceSize                                offset)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   struct anv_compute_pipeline *pipeline = cmd_buffer->state.compute.pipeline;
   const struct brw_cs_prog_data *prog_data = get_cs_prog_data(pipeline);
   struct anv_address addr = anv_address_add(buffer->address, offset);
   UNUSED struct anv_batch *batch = &cmd_buffer->batch;

   anv_cmd_buffer_push_base_group_id(cmd_buffer, 0, 0, 0);

#if GFX_VER == 7
   /* Linux 4.4 added command parser version 5 which allows the GPGPU
    * indirect dispatch registers to be written.
    */
   if (verify_cmd_parser(cmd_buffer->device, 5,
                         "vkCmdDispatchIndirect") != VK_SUCCESS)
      return;
#endif

   anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_COMPUTE,
                        "compute indirect",
                        0);

   if (prog_data->uses_num_work_groups) {
      cmd_buffer->state.compute.num_workgroups = addr;

      /* The num_workgroups buffer goes in the binding table */
      cmd_buffer->state.descriptors_dirty |= VK_SHADER_STAGE_COMPUTE_BIT;
   }

   genX(cmd_buffer_flush_compute_state)(cmd_buffer);

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   struct mi_value size_x = mi_mem32(anv_address_add(addr, 0));
   struct mi_value size_y = mi_mem32(anv_address_add(addr, 4));
   struct mi_value size_z = mi_mem32(anv_address_add(addr, 8));

   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMX), size_x);
   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMY), size_y);
   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMZ), size_z);

#if GFX_VER <= 7
   /* predicate = (compute_dispatch_indirect_x_size == 0); */
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC0), size_x);
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(0));
   anv_batch_emit(batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
      mip.CombineOperation = COMBINE_SET;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }

   /* predicate |= (compute_dispatch_indirect_y_size == 0); */
   mi_store(&b, mi_reg32(MI_PREDICATE_SRC0), size_y);
   anv_batch_emit(batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
      mip.CombineOperation = COMBINE_OR;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }

   /* predicate |= (compute_dispatch_indirect_z_size == 0); */
   mi_store(&b, mi_reg32(MI_PREDICATE_SRC0), size_z);
   anv_batch_emit(batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
      mip.CombineOperation = COMBINE_OR;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }

   /* predicate = !predicate; */
   anv_batch_emit(batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
      mip.CombineOperation = COMBINE_OR;
      mip.CompareOperation = COMPARE_FALSE;
   }

#if GFX_VERx10 == 75
   if (cmd_buffer->state.conditional_render_enabled) {
      /* predicate &= !(conditional_rendering_predicate == 0); */
      mi_store(&b, mi_reg32(MI_PREDICATE_SRC0),
                   mi_reg32(ANV_PREDICATE_RESULT_REG));
      anv_batch_emit(batch, GENX(MI_PREDICATE), mip) {
         mip.LoadOperation    = LOAD_LOADINV;
         mip.CombineOperation = COMBINE_AND;
         mip.CompareOperation = COMPARE_SRCS_EQUAL;
      }
   }
#endif

#else /* GFX_VER > 7 */
   if (cmd_buffer->state.conditional_render_enabled)
      genX(cmd_emit_conditional_render_predicate)(cmd_buffer);
#endif

   emit_cs_walker(cmd_buffer, pipeline, true, prog_data, 0, 0, 0);
}

#if GFX_VERx10 >= 125
static void
calc_local_trace_size(uint8_t local_shift[3], const uint32_t global[3])
{
   unsigned total_shift = 0;
   memset(local_shift, 0, 3);

   bool progress;
   do {
      progress = false;
      for (unsigned i = 0; i < 3; i++) {
         assert(global[i] > 0);
         if ((1 << local_shift[i]) < global[i]) {
            progress = true;
            local_shift[i]++;
            total_shift++;
         }

         if (total_shift == 3)
            return;
      }
   } while(progress);

   /* Assign whatever's left to x */
   local_shift[0] += 3 - total_shift;
}

static struct GFX_RT_SHADER_TABLE
vk_sdar_to_shader_table(const VkStridedDeviceAddressRegionKHR *region)
{
   return (struct GFX_RT_SHADER_TABLE) {
      .BaseAddress = anv_address_from_u64(region->deviceAddress),
      .Stride = region->stride,
   };
}

static void
cmd_buffer_trace_rays(struct anv_cmd_buffer *cmd_buffer,
                      const VkStridedDeviceAddressRegionKHR *raygen_sbt,
                      const VkStridedDeviceAddressRegionKHR *miss_sbt,
                      const VkStridedDeviceAddressRegionKHR *hit_sbt,
                      const VkStridedDeviceAddressRegionKHR *callable_sbt,
                      bool is_indirect,
                      uint32_t launch_width,
                      uint32_t launch_height,
                      uint32_t launch_depth,
                      uint64_t launch_size_addr)
{
   struct anv_cmd_ray_tracing_state *rt = &cmd_buffer->state.rt;
   struct anv_ray_tracing_pipeline *pipeline = rt->pipeline;

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   /* If we have a known degenerate launch size, just bail */
   if (!is_indirect &&
       (launch_width == 0 || launch_height == 0 || launch_depth == 0))
      return;

   genX(cmd_buffer_config_l3)(cmd_buffer, pipeline->base.l3_config);
   genX(flush_pipeline_select_gpgpu)(cmd_buffer);

   cmd_buffer->state.rt.pipeline_dirty = false;

   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   /* Add these to the reloc list as they're internal buffers that don't
    * actually have relocs to pick them up manually.
    *
    * TODO(RT): This is a bit of a hack
    */
   anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
                         cmd_buffer->batch.alloc,
                         rt->scratch.bo);

   /* Allocate and set up our RT_DISPATCH_GLOBALS */
   struct anv_state rtdg_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                                         BRW_RT_PUSH_CONST_OFFSET +
                                         sizeof(struct anv_push_constants),
                                         64);

   struct GFX_RT_DISPATCH_GLOBALS rtdg = {
      .MemBaseAddress = (struct anv_address) {
         .bo = rt->scratch.bo,
         .offset = rt->scratch.layout.ray_stack_start,
      },
      .CallStackHandler =
         anv_shader_bin_get_bsr(cmd_buffer->device->rt_trivial_return, 0),
      .AsyncRTStackSize = rt->scratch.layout.ray_stack_stride / 64,
      .NumDSSRTStacks = rt->scratch.layout.stack_ids_per_dss,
      .MaxBVHLevels = BRW_RT_MAX_BVH_LEVELS,
      .Flags = RT_DEPTH_TEST_LESS_EQUAL,
      .HitGroupTable = vk_sdar_to_shader_table(hit_sbt),
      .MissGroupTable = vk_sdar_to_shader_table(miss_sbt),
      .SWStackSize = rt->scratch.layout.sw_stack_size / 64,
      .LaunchWidth = launch_width,
      .LaunchHeight = launch_height,
      .LaunchDepth = launch_depth,
      .CallableGroupTable = vk_sdar_to_shader_table(callable_sbt),
   };
   GFX_RT_DISPATCH_GLOBALS_pack(NULL, rtdg_state.map, &rtdg);

   /* Push constants go after the RT_DISPATCH_GLOBALS */
   assert(GFX_RT_DISPATCH_GLOBALS_length * 4 <= BRW_RT_PUSH_CONST_OFFSET);
   memcpy(rtdg_state.map + BRW_RT_PUSH_CONST_OFFSET,
          &cmd_buffer->state.rt.base.push_constants,
          sizeof(struct anv_push_constants));

   struct anv_address rtdg_addr = {
      .bo = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
      .offset = rtdg_state.offset,
   };

   uint8_t local_size_log2[3];
   uint32_t global_size[3] = {};
   if (is_indirect) {
      /* Pick a local size that's probably ok.  We assume most TraceRays calls
       * will use a two-dimensional dispatch size.  Worst case, our initial
       * dispatch will be a little slower than it has to be.
       */
      local_size_log2[0] = 2;
      local_size_log2[1] = 1;
      local_size_log2[2] = 0;

      struct mi_builder b;
      mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

      struct mi_value launch_size[3] = {
         mi_mem32(anv_address_from_u64(launch_size_addr + 0)),
         mi_mem32(anv_address_from_u64(launch_size_addr + 4)),
         mi_mem32(anv_address_from_u64(launch_size_addr + 8)),
      };

      /* Store the original launch size into RT_DISPATCH_GLOBALS
       *
       * TODO: Pull values from genX_bits.h once RT_DISPATCH_GLOBALS gets
       * moved into a genX version.
       */
      mi_store(&b, mi_mem32(anv_address_add(rtdg_addr, 52)),
               mi_value_ref(&b, launch_size[0]));
      mi_store(&b, mi_mem32(anv_address_add(rtdg_addr, 56)),
               mi_value_ref(&b, launch_size[1]));
      mi_store(&b, mi_mem32(anv_address_add(rtdg_addr, 60)),
               mi_value_ref(&b, launch_size[2]));

      /* Compute the global dispatch size */
      for (unsigned i = 0; i < 3; i++) {
         if (local_size_log2[i] == 0)
            continue;

         /* global_size = DIV_ROUND_UP(launch_size, local_size)
          *
          * Fortunately for us MI_ALU math is 64-bit and , mi_ushr32_imm
          * has the semantics of shifting the enture 64-bit value and taking
          * the bottom 32 so we don't have to worry about roll-over.
          */
         uint32_t local_size = 1 << local_size_log2[i];
         launch_size[i] = mi_iadd(&b, launch_size[i],
                                      mi_imm(local_size - 1));
         launch_size[i] = mi_ushr32_imm(&b, launch_size[i],
                                            local_size_log2[i]);
      }

      mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMX), launch_size[0]);
      mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMY), launch_size[1]);
      mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMZ), launch_size[2]);
   } else {
      uint32_t launch_size[3] = { launch_width, launch_height, launch_depth };
      calc_local_trace_size(local_size_log2, launch_size);

      for (unsigned i = 0; i < 3; i++) {
         /* We have to be a bit careful here because DIV_ROUND_UP adds to the
          * numerator value may overflow.  Cast to uint64_t to avoid this.
          */
         uint32_t local_size = 1 << local_size_log2[i];
         global_size[i] = DIV_ROUND_UP((uint64_t)launch_size[i], local_size);
      }
   }

   anv_batch_emit(&cmd_buffer->batch, GENX(COMPUTE_WALKER), cw) {
      cw.IndirectParameterEnable        = is_indirect;
      cw.PredicateEnable                = false;
      cw.SIMDSize                       = SIMD8;
      cw.LocalXMaximum                  = (1 << local_size_log2[0]) - 1;
      cw.LocalYMaximum                  = (1 << local_size_log2[1]) - 1;
      cw.LocalZMaximum                  = (1 << local_size_log2[2]) - 1;
      cw.ThreadGroupIDXDimension        = global_size[0];
      cw.ThreadGroupIDYDimension        = global_size[1];
      cw.ThreadGroupIDZDimension        = global_size[2];
      cw.ExecutionMask                  = 0xff;
      cw.EmitInlineParameter            = true;

      const gl_shader_stage s = MESA_SHADER_RAYGEN;
      struct anv_device *device = cmd_buffer->device;
      struct anv_state *surfaces = &cmd_buffer->state.binding_tables[s];
      struct anv_state *samplers = &cmd_buffer->state.samplers[s];
      cw.InterfaceDescriptor = (struct GENX(INTERFACE_DESCRIPTOR_DATA)) {
         .KernelStartPointer = device->rt_trampoline->kernel.offset,
         .SamplerStatePointer = samplers->offset,
         /* i965: DIV_ROUND_UP(CLAMP(stage_state->sampler_count, 0, 16), 4), */
         .SamplerCount = 0,
         .BindingTablePointer = surfaces->offset,
         .NumberofThreadsinGPGPUThreadGroup = 1,
         .BTDMode = true,
      };

      struct brw_rt_raygen_trampoline_params trampoline_params = {
         .rt_disp_globals_addr = anv_address_physical(rtdg_addr),
         .raygen_bsr_addr = raygen_sbt->deviceAddress,
         .is_indirect = is_indirect,
         .local_group_size_log2 = {
            local_size_log2[0],
            local_size_log2[1],
            local_size_log2[2],
         },
      };
      STATIC_ASSERT(sizeof(trampoline_params) == 32);
      memcpy(cw.InlineData, &trampoline_params, sizeof(trampoline_params));
   }
}

void
genX(CmdTraceRaysKHR)(
    VkCommandBuffer                             commandBuffer,
    const VkStridedDeviceAddressRegionKHR*      pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pCallableShaderBindingTable,
    uint32_t                                    width,
    uint32_t                                    height,
    uint32_t                                    depth)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   cmd_buffer_trace_rays(cmd_buffer,
                         pRaygenShaderBindingTable,
                         pMissShaderBindingTable,
                         pHitShaderBindingTable,
                         pCallableShaderBindingTable,
                         false /* is_indirect */,
                         width, height, depth,
                         0 /* launch_size_addr */);
}

void
genX(CmdTraceRaysIndirectKHR)(
    VkCommandBuffer                             commandBuffer,
    const VkStridedDeviceAddressRegionKHR*      pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*      pCallableShaderBindingTable,
    VkDeviceAddress                             indirectDeviceAddress)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   cmd_buffer_trace_rays(cmd_buffer,
                         pRaygenShaderBindingTable,
                         pMissShaderBindingTable,
                         pHitShaderBindingTable,
                         pCallableShaderBindingTable,
                         true /* is_indirect */,
                         0, 0, 0, /* width, height, depth, */
                         indirectDeviceAddress);
}
#endif /* GFX_VERx10 >= 125 */

static void
genX(flush_pipeline_select)(struct anv_cmd_buffer *cmd_buffer,
                            uint32_t pipeline)
{
   UNUSED const struct intel_device_info *devinfo = &cmd_buffer->device->info;

   if (cmd_buffer->state.current_pipeline == pipeline)
      return;

#if GFX_VER >= 8 && GFX_VER < 10
   /* From the Broadwell PRM, Volume 2a: Instructions, PIPELINE_SELECT:
    *
    *   Software must clear the COLOR_CALC_STATE Valid field in
    *   3DSTATE_CC_STATE_POINTERS command prior to send a PIPELINE_SELECT
    *   with Pipeline Select set to GPGPU.
    *
    * The internal hardware docs recommend the same workaround for Gfx9
    * hardware too.
    */
   if (pipeline == GPGPU)
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CC_STATE_POINTERS), t);
#endif

#if GFX_VER == 9
   if (pipeline == _3D) {
      /* There is a mid-object preemption workaround which requires you to
       * re-emit MEDIA_VFE_STATE after switching from GPGPU to 3D.  However,
       * even without preemption, we have issues with geometry flickering when
       * GPGPU and 3D are back-to-back and this seems to fix it.  We don't
       * really know why.
       */
      anv_batch_emit(&cmd_buffer->batch, GENX(MEDIA_VFE_STATE), vfe) {
         vfe.MaximumNumberofThreads =
            devinfo->max_cs_threads * devinfo->subslice_total - 1;
         vfe.NumberofURBEntries     = 2;
         vfe.URBEntryAllocationSize = 2;
      }

      /* We just emitted a dummy MEDIA_VFE_STATE so now that packet is
       * invalid. Set the compute pipeline to dirty to force a re-emit of the
       * pipeline in case we get back-to-back dispatch calls with the same
       * pipeline and a PIPELINE_SELECT in between.
       */
      cmd_buffer->state.compute.pipeline_dirty = true;
   }
#endif

   /* From "BXML » GT » MI » vol1a GPU Overview » [Instruction]
    * PIPELINE_SELECT [DevBWR+]":
    *
    *   Project: DEVSNB+
    *
    *   Software must ensure all the write caches are flushed through a
    *   stalling PIPE_CONTROL command followed by another PIPE_CONTROL
    *   command to invalidate read only caches prior to programming
    *   MI_PIPELINE_SELECT command to change the Pipeline Select Mode.
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.RenderTargetCacheFlushEnable  = true;
      pc.DepthCacheFlushEnable         = true;
#if GFX_VER >= 12
      pc.HDCPipelineFlushEnable        = true;
#else
      pc.DCFlushEnable                 = true;
#endif
      pc.PostSyncOperation             = NoWrite;
      pc.CommandStreamerStallEnable    = true;
#if GFX_VER >= 12
      /* Wa_1409600907: "PIPE_CONTROL with Depth Stall Enable bit must be
       * set with any PIPE_CONTROL with Depth Flush Enable bit set.
       */
      pc.DepthStallEnable = true;
#endif
      anv_debug_dump_pc(pc);
   }

   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      pc.TextureCacheInvalidationEnable   = true;
      pc.ConstantCacheInvalidationEnable  = true;
      pc.StateCacheInvalidationEnable     = true;
      pc.InstructionCacheInvalidateEnable = true;
      pc.PostSyncOperation                = NoWrite;
      anv_debug_dump_pc(pc);
   }

   anv_batch_emit(&cmd_buffer->batch, GENX(PIPELINE_SELECT), ps) {
#if GFX_VER >= 9
      ps.MaskBits = GFX_VER >= 12 ? 0x13 : 3;
      ps.MediaSamplerDOPClockGateEnable = GFX_VER >= 12;
#endif
      ps.PipelineSelection = pipeline;
   }

#if GFX_VER == 9
   if (devinfo->is_geminilake) {
      /* Project: DevGLK
       *
       * "This chicken bit works around a hardware issue with barrier logic
       *  encountered when switching between GPGPU and 3D pipelines.  To
       *  workaround the issue, this mode bit should be set after a pipeline
       *  is selected."
       */
      anv_batch_write_reg(&cmd_buffer->batch, GENX(SLICE_COMMON_ECO_CHICKEN1), scec1) {
         scec1.GLKBarrierMode = pipeline == GPGPU ? GLK_BARRIER_MODE_GPGPU
                                                  : GLK_BARRIER_MODE_3D_HULL;
         scec1.GLKBarrierModeMask = 1;
      }
   }
#endif

   cmd_buffer->state.current_pipeline = pipeline;
}

void
genX(flush_pipeline_select_3d)(struct anv_cmd_buffer *cmd_buffer)
{
   genX(flush_pipeline_select)(cmd_buffer, _3D);
}

void
genX(flush_pipeline_select_gpgpu)(struct anv_cmd_buffer *cmd_buffer)
{
   genX(flush_pipeline_select)(cmd_buffer, GPGPU);
}

void
genX(cmd_buffer_emit_gfx7_depth_flush)(struct anv_cmd_buffer *cmd_buffer)
{
   if (GFX_VER >= 8)
      return;

   /* From the Haswell PRM, documentation for 3DSTATE_DEPTH_BUFFER:
    *
    *    "Restriction: Prior to changing Depth/Stencil Buffer state (i.e., any
    *    combination of 3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
    *    3DSTATE_STENCIL_BUFFER, 3DSTATE_HIER_DEPTH_BUFFER) SW must first
    *    issue a pipelined depth stall (PIPE_CONTROL with Depth Stall bit
    *    set), followed by a pipelined depth cache flush (PIPE_CONTROL with
    *    Depth Flush Bit set, followed by another pipelined depth stall
    *    (PIPE_CONTROL with Depth Stall Bit set), unless SW can otherwise
    *    guarantee that the pipeline from WM onwards is already flushed (e.g.,
    *    via a preceding MI_FLUSH)."
    */
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pipe) {
      pipe.DepthStallEnable = true;
      anv_debug_dump_pc(pipe);
   }
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pipe) {
      pipe.DepthCacheFlushEnable = true;
#if GFX_VER >= 12
      pipe.TileCacheFlushEnable = true;
#endif
      anv_debug_dump_pc(pipe);
   }
   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pipe) {
      pipe.DepthStallEnable = true;
      anv_debug_dump_pc(pipe);
   }
}

void
genX(cmd_buffer_emit_gfx12_depth_wa)(struct anv_cmd_buffer *cmd_buffer,
                                     const struct isl_surf *surf)
{
#if GFX_VERx10 == 120
   const bool fmt_is_d16 = surf->format == ISL_FORMAT_R16_UNORM;

   switch (cmd_buffer->state.depth_reg_mode) {
   case ANV_DEPTH_REG_MODE_HW_DEFAULT:
      if (!fmt_is_d16)
         return;
      break;
   case ANV_DEPTH_REG_MODE_D16:
      if (fmt_is_d16)
         return;
      break;
   case ANV_DEPTH_REG_MODE_UNKNOWN:
      break;
   }

   /* We'll change some CHICKEN registers depending on the depth surface
    * format. Do a depth flush and stall so the pipeline is not using these
    * settings while we change the registers.
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_DEPTH_CACHE_FLUSH_BIT |
                             ANV_PIPE_DEPTH_STALL_BIT |
                             ANV_PIPE_END_OF_PIPE_SYNC_BIT,
                             "Workaround: Stop pipeline for 14010455700");
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   /* Wa_14010455700
    *
    * To avoid sporadic corruptions “Set 0x7010[9] when Depth Buffer
    * Surface Format is D16_UNORM , surface type is not NULL & 1X_MSAA”.
    */
   anv_batch_write_reg(&cmd_buffer->batch, GENX(COMMON_SLICE_CHICKEN1), reg) {
      reg.HIZPlaneOptimizationdisablebit = fmt_is_d16 && surf->samples == 1;
      reg.HIZPlaneOptimizationdisablebitMask = true;
   }

   /* Wa_1806527549
    *
    * Set HIZ_CHICKEN (7018h) bit 13 = 1 when depth buffer is D16_UNORM.
    */
   anv_batch_write_reg(&cmd_buffer->batch, GENX(HIZ_CHICKEN), reg) {
      reg.HZDepthTestLEGEOptimizationDisable = fmt_is_d16;
      reg.HZDepthTestLEGEOptimizationDisableMask = true;
   }

   cmd_buffer->state.depth_reg_mode =
      fmt_is_d16 ? ANV_DEPTH_REG_MODE_D16 : ANV_DEPTH_REG_MODE_HW_DEFAULT;
#endif
}

/* From the Skylake PRM, 3DSTATE_VERTEX_BUFFERS:
 *
 *    "The VF cache needs to be invalidated before binding and then using
 *    Vertex Buffers that overlap with any previously bound Vertex Buffer
 *    (at a 64B granularity) since the last invalidation.  A VF cache
 *    invalidate is performed by setting the "VF Cache Invalidation Enable"
 *    bit in PIPE_CONTROL."
 *
 * This is implemented by carefully tracking all vertex and index buffer
 * bindings and flushing if the cache ever ends up with a range in the cache
 * that would exceed 4 GiB.  This is implemented in three parts:
 *
 *    1. genX(cmd_buffer_set_binding_for_gfx8_vb_flush)() which must be called
 *       every time a 3DSTATE_VERTEX_BUFFER packet is emitted and informs the
 *       tracking code of the new binding.  If this new binding would cause
 *       the cache to have a too-large range on the next draw call, a pipeline
 *       stall and VF cache invalidate are added to pending_pipeline_bits.
 *
 *    2. genX(cmd_buffer_apply_pipe_flushes)() resets the cache tracking to
 *       empty whenever we emit a VF invalidate.
 *
 *    3. genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)() must be called
 *       after every 3DPRIMITIVE and copies the bound range into the dirty
 *       range for each used buffer.  This has to be a separate step because
 *       we don't always re-bind all buffers and so 1. can't know which
 *       buffers are actually bound.
 */
void
genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(struct anv_cmd_buffer *cmd_buffer,
                                               int vb_index,
                                               struct anv_address vb_address,
                                               uint32_t vb_size)
{
   if (GFX_VER < 8 || GFX_VER > 9 ||
       !anv_use_softpin(cmd_buffer->device->physical))
      return;

   struct anv_vb_cache_range *bound, *dirty;
   if (vb_index == -1) {
      bound = &cmd_buffer->state.gfx.ib_bound_range;
      dirty = &cmd_buffer->state.gfx.ib_dirty_range;
   } else {
      assert(vb_index >= 0);
      assert(vb_index < ARRAY_SIZE(cmd_buffer->state.gfx.vb_bound_ranges));
      assert(vb_index < ARRAY_SIZE(cmd_buffer->state.gfx.vb_dirty_ranges));
      bound = &cmd_buffer->state.gfx.vb_bound_ranges[vb_index];
      dirty = &cmd_buffer->state.gfx.vb_dirty_ranges[vb_index];
   }

   if (vb_size == 0) {
      bound->start = 0;
      bound->end = 0;
      return;
   }

   assert(vb_address.bo && (vb_address.bo->flags & EXEC_OBJECT_PINNED));
   bound->start = intel_48b_address(anv_address_physical(vb_address));
   bound->end = bound->start + vb_size;
   assert(bound->end > bound->start); /* No overflow */

   /* Align everything to a cache line */
   bound->start &= ~(64ull - 1ull);
   bound->end = align_u64(bound->end, 64);

   /* Compute the dirty range */
   dirty->start = MIN2(dirty->start, bound->start);
   dirty->end = MAX2(dirty->end, bound->end);

   /* If our range is larger than 32 bits, we have to flush */
   assert(bound->end - bound->start <= (1ull << 32));
   if (dirty->end - dirty->start > (1ull << 32)) {
      anv_add_pending_pipe_bits(cmd_buffer,
                                ANV_PIPE_CS_STALL_BIT |
                                ANV_PIPE_VF_CACHE_INVALIDATE_BIT,
                                "vb > 32b range");
   }
}

void
genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)(struct anv_cmd_buffer *cmd_buffer,
                                                    uint32_t access_type,
                                                    uint64_t vb_used)
{
   if (GFX_VER < 8 || GFX_VER > 9 ||
       !anv_use_softpin(cmd_buffer->device->physical))
      return;

   if (access_type == RANDOM) {
      /* We have an index buffer */
      struct anv_vb_cache_range *bound = &cmd_buffer->state.gfx.ib_bound_range;
      struct anv_vb_cache_range *dirty = &cmd_buffer->state.gfx.ib_dirty_range;

      if (bound->end > bound->start) {
         dirty->start = MIN2(dirty->start, bound->start);
         dirty->end = MAX2(dirty->end, bound->end);
      }
   }

   uint64_t mask = vb_used;
   while (mask) {
      int i = u_bit_scan64(&mask);
      assert(i >= 0);
      assert(i < ARRAY_SIZE(cmd_buffer->state.gfx.vb_bound_ranges));
      assert(i < ARRAY_SIZE(cmd_buffer->state.gfx.vb_dirty_ranges));

      struct anv_vb_cache_range *bound, *dirty;
      bound = &cmd_buffer->state.gfx.vb_bound_ranges[i];
      dirty = &cmd_buffer->state.gfx.vb_dirty_ranges[i];

      if (bound->end > bound->start) {
         dirty->start = MIN2(dirty->start, bound->start);
         dirty->end = MAX2(dirty->end, bound->end);
      }
   }
}

/**
 * Update the pixel hashing modes that determine the balancing of PS threads
 * across subslices and slices.
 *
 * \param width Width bound of the rendering area (already scaled down if \p
 *              scale is greater than 1).
 * \param height Height bound of the rendering area (already scaled down if \p
 *               scale is greater than 1).
 * \param scale The number of framebuffer samples that could potentially be
 *              affected by an individual channel of the PS thread.  This is
 *              typically one for single-sampled rendering, but for operations
 *              like CCS resolves and fast clears a single PS invocation may
 *              update a huge number of pixels, in which case a finer
 *              balancing is desirable in order to maximally utilize the
 *              bandwidth available.  UINT_MAX can be used as shorthand for
 *              "finest hashing mode available".
 */
void
genX(cmd_buffer_emit_hashing_mode)(struct anv_cmd_buffer *cmd_buffer,
                                   unsigned width, unsigned height,
                                   unsigned scale)
{
#if GFX_VER == 9
   const struct intel_device_info *devinfo = &cmd_buffer->device->info;
   const unsigned slice_hashing[] = {
      /* Because all Gfx9 platforms with more than one slice require
       * three-way subslice hashing, a single "normal" 16x16 slice hashing
       * block is guaranteed to suffer from substantial imbalance, with one
       * subslice receiving twice as much work as the other two in the
       * slice.
       *
       * The performance impact of that would be particularly severe when
       * three-way hashing is also in use for slice balancing (which is the
       * case for all Gfx9 GT4 platforms), because one of the slices
       * receives one every three 16x16 blocks in either direction, which
       * is roughly the periodicity of the underlying subslice imbalance
       * pattern ("roughly" because in reality the hardware's
       * implementation of three-way hashing doesn't do exact modulo 3
       * arithmetic, which somewhat decreases the magnitude of this effect
       * in practice).  This leads to a systematic subslice imbalance
       * within that slice regardless of the size of the primitive.  The
       * 32x32 hashing mode guarantees that the subslice imbalance within a
       * single slice hashing block is minimal, largely eliminating this
       * effect.
       */
      _32x32,
      /* Finest slice hashing mode available. */
      NORMAL
   };
   const unsigned subslice_hashing[] = {
      /* 16x16 would provide a slight cache locality benefit especially
       * visible in the sampler L1 cache efficiency of low-bandwidth
       * non-LLC platforms, but it comes at the cost of greater subslice
       * imbalance for primitives of dimensions approximately intermediate
       * between 16x4 and 16x16.
       */
      _16x4,
      /* Finest subslice hashing mode available. */
      _8x4
   };
   /* Dimensions of the smallest hashing block of a given hashing mode.  If
    * the rendering area is smaller than this there can't possibly be any
    * benefit from switching to this mode, so we optimize out the
    * transition.
    */
   const unsigned min_size[][2] = {
         { 16, 4 },
         { 8, 4 }
   };
   const unsigned idx = scale > 1;

   if (cmd_buffer->state.current_hash_scale != scale &&
       (width > min_size[idx][0] || height > min_size[idx][1])) {
      anv_add_pending_pipe_bits(cmd_buffer,
                                ANV_PIPE_CS_STALL_BIT |
                                ANV_PIPE_STALL_AT_SCOREBOARD_BIT,
                                "change pixel hash mode");
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      anv_batch_write_reg(&cmd_buffer->batch, GENX(GT_MODE), gt) {
         gt.SliceHashing = (devinfo->num_slices > 1 ? slice_hashing[idx] : 0);
         gt.SliceHashingMask = (devinfo->num_slices > 1 ? -1 : 0);
         gt.SubsliceHashing = subslice_hashing[idx];
         gt.SubsliceHashingMask = -1;
      }

      cmd_buffer->state.current_hash_scale = scale;
   }
#endif
}

static void
cmd_buffer_emit_depth_stencil(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_device *device = cmd_buffer->device;
   const struct anv_image_view *iview =
      anv_cmd_buffer_get_depth_stencil_view(cmd_buffer);
   const struct anv_image *image = iview ? iview->image : NULL;

   /* FIXME: Width and Height are wrong */

   genX(cmd_buffer_emit_gfx7_depth_flush)(cmd_buffer);

   uint32_t *dw = anv_batch_emit_dwords(&cmd_buffer->batch,
                                        device->isl_dev.ds.size / 4);
   if (dw == NULL)
      return;

   struct isl_depth_stencil_hiz_emit_info info = { };

   if (iview)
      info.view = &iview->planes[0].isl;

   if (image && (image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT)) {
      const uint32_t depth_plane =
         anv_image_aspect_to_plane(image, VK_IMAGE_ASPECT_DEPTH_BIT);
      const struct anv_surface *depth_surface =
         &image->planes[depth_plane].primary_surface;
      const struct anv_address depth_address =
         anv_image_address(image, &depth_surface->memory_range);

      info.depth_surf = &depth_surface->isl;

      info.depth_address =
         anv_batch_emit_reloc(&cmd_buffer->batch,
                              dw + device->isl_dev.ds.depth_offset / 4,
                              depth_address.bo, depth_address.offset);
      info.mocs =
         anv_mocs(device, depth_address.bo, ISL_SURF_USAGE_DEPTH_BIT);

      const uint32_t ds =
         cmd_buffer->state.subpass->depth_stencil_attachment->attachment;
      info.hiz_usage = cmd_buffer->state.attachments[ds].aux_usage;
      if (info.hiz_usage != ISL_AUX_USAGE_NONE) {
         assert(isl_aux_usage_has_hiz(info.hiz_usage));

         const struct anv_surface *hiz_surface =
            &image->planes[depth_plane].aux_surface;
         const struct anv_address hiz_address =
            anv_image_address(image, &hiz_surface->memory_range);

         info.hiz_surf = &hiz_surface->isl;

         info.hiz_address =
            anv_batch_emit_reloc(&cmd_buffer->batch,
                                 dw + device->isl_dev.ds.hiz_offset / 4,
                                 hiz_address.bo, hiz_address.offset);

         info.depth_clear_value = ANV_HZ_FC_VAL;
      }
   }

   if (image && (image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT)) {
      const uint32_t stencil_plane =
         anv_image_aspect_to_plane(image, VK_IMAGE_ASPECT_STENCIL_BIT);
      const struct anv_surface *stencil_surface =
         &image->planes[stencil_plane].primary_surface;
      const struct anv_address stencil_address =
         anv_image_address(image, &stencil_surface->memory_range);

      info.stencil_surf = &stencil_surface->isl;

      info.stencil_aux_usage = image->planes[stencil_plane].aux_usage;
      info.stencil_address =
         anv_batch_emit_reloc(&cmd_buffer->batch,
                              dw + device->isl_dev.ds.stencil_offset / 4,
                              stencil_address.bo, stencil_address.offset);
      info.mocs =
         anv_mocs(device, stencil_address.bo, ISL_SURF_USAGE_STENCIL_BIT);
   }

   isl_emit_depth_stencil_hiz_s(&device->isl_dev, dw, &info);

   if (info.depth_surf)
      genX(cmd_buffer_emit_gfx12_depth_wa)(cmd_buffer, info.depth_surf);

   if (GFX_VER >= 12) {
      cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
      genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

      /* Wa_1408224581
       *
       * Workaround: Gfx12LP Astep only An additional pipe control with
       * post-sync = store dword operation would be required.( w/a is to
       * have an additional pipe control after the stencil state whenever
       * the surface state bits of this state is changing).
       */
      anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
         pc.PostSyncOperation = WriteImmediateData;
         pc.Address = cmd_buffer->device->workaround_address;
      }
   }
   cmd_buffer->state.hiz_enabled = isl_aux_usage_has_hiz(info.hiz_usage);
}

/**
 * This ANDs the view mask of the current subpass with the pending clear
 * views in the attachment to get the mask of views active in the subpass
 * that still need to be cleared.
 */
static inline uint32_t
get_multiview_subpass_clear_mask(const struct anv_cmd_state *cmd_state,
                                 const struct anv_attachment_state *att_state)
{
   return cmd_state->subpass->view_mask & att_state->pending_clear_views;
}

static inline bool
do_first_layer_clear(const struct anv_cmd_state *cmd_state,
                     const struct anv_attachment_state *att_state)
{
   if (!cmd_state->subpass->view_mask)
      return true;

   uint32_t pending_clear_mask =
      get_multiview_subpass_clear_mask(cmd_state, att_state);

   return pending_clear_mask & 1;
}

static inline bool
current_subpass_is_last_for_attachment(const struct anv_cmd_state *cmd_state,
                                       uint32_t att_idx)
{
   const uint32_t last_subpass_idx =
      cmd_state->pass->attachments[att_idx].last_subpass_idx;
   const struct anv_subpass *last_subpass =
      &cmd_state->pass->subpasses[last_subpass_idx];
   return last_subpass == cmd_state->subpass;
}

static void
cmd_buffer_begin_subpass(struct anv_cmd_buffer *cmd_buffer,
                         uint32_t subpass_id)
{
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;
   struct anv_render_pass *pass = cmd_state->pass;
   struct anv_subpass *subpass = &pass->subpasses[subpass_id];
   cmd_state->subpass = subpass;

   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_RENDER_TARGETS;

   /* Our implementation of VK_KHR_multiview uses instancing to draw the
    * different views.  If the client asks for instancing, we need to use the
    * Instance Data Step Rate to ensure that we repeat the client's
    * per-instance data once for each view.  Since this bit is in
    * VERTEX_BUFFER_STATE on gfx7, we need to dirty vertex buffers at the top
    * of each subpass.
    */
   if (GFX_VER == 7)
      cmd_buffer->state.gfx.vb_dirty |= ~0;

   /* It is possible to start a render pass with an old pipeline.  Because the
    * render pass and subpass index are both baked into the pipeline, this is
    * highly unlikely.  In order to do so, it requires that you have a render
    * pass with a single subpass and that you use that render pass twice
    * back-to-back and use the same pipeline at the start of the second render
    * pass as at the end of the first.  In order to avoid unpredictable issues
    * with this edge case, we just dirty the pipeline at the start of every
    * subpass.
    */
   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_PIPELINE;

   /* Accumulate any subpass flushes that need to happen before the subpass */
   anv_add_pending_pipe_bits(cmd_buffer,
                             cmd_buffer->state.pass->subpass_flushes[subpass_id],
                             "begin subpass deps/attachments");

   VkRect2D render_area = cmd_buffer->state.render_area;
   struct anv_framebuffer *fb = cmd_buffer->state.framebuffer;

   bool is_multiview = subpass->view_mask != 0;

   for (uint32_t i = 0; i < subpass->attachment_count; ++i) {
      const uint32_t a = subpass->attachments[i].attachment;
      if (a == VK_ATTACHMENT_UNUSED)
         continue;

      assert(a < cmd_state->pass->attachment_count);
      struct anv_attachment_state *att_state = &cmd_state->attachments[a];

      struct anv_image_view *iview = cmd_state->attachments[a].image_view;
      const struct anv_image *image = iview->image;

      VkImageLayout target_layout = subpass->attachments[i].layout;
      VkImageLayout target_stencil_layout =
         subpass->attachments[i].stencil_layout;

      uint32_t level = iview->planes[0].isl.base_level;
      uint32_t width = anv_minify(iview->image->vk.extent.width, level);
      uint32_t height = anv_minify(iview->image->vk.extent.height, level);
      bool full_surface_draw =
         render_area.offset.x == 0 && render_area.offset.y == 0 &&
         render_area.extent.width == width &&
         render_area.extent.height == height;

      uint32_t base_layer, layer_count;
      if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
         base_layer = 0;
         layer_count = anv_minify(iview->image->vk.extent.depth, level);
      } else {
         base_layer = iview->planes[0].isl.base_array_layer;
         layer_count = fb->layers;
      }

      if (image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
         bool will_full_fast_clear =
            (att_state->pending_clear_aspects & VK_IMAGE_ASPECT_COLOR_BIT) &&
            att_state->fast_clear && full_surface_draw;

         assert(image->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT);
         transition_color_buffer(cmd_buffer, image, VK_IMAGE_ASPECT_COLOR_BIT,
                                 level, 1, base_layer, layer_count,
                                 att_state->current_layout, target_layout,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 will_full_fast_clear);
         att_state->aux_usage =
            anv_layout_to_aux_usage(&cmd_buffer->device->info, image,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                    target_layout);
      }

      if (image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
         bool will_full_fast_clear =
            (att_state->pending_clear_aspects & VK_IMAGE_ASPECT_DEPTH_BIT) &&
            att_state->fast_clear && full_surface_draw;

         transition_depth_buffer(cmd_buffer, image,
                                 base_layer, layer_count,
                                 att_state->current_layout, target_layout,
                                 will_full_fast_clear);
         att_state->aux_usage =
            anv_layout_to_aux_usage(&cmd_buffer->device->info, image,
                                    VK_IMAGE_ASPECT_DEPTH_BIT,
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                    target_layout);
      }

      if (image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
         bool will_full_fast_clear =
            (att_state->pending_clear_aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
            att_state->fast_clear && full_surface_draw;

         transition_stencil_buffer(cmd_buffer, image,
                                   level, 1, base_layer, layer_count,
                                   att_state->current_stencil_layout,
                                   target_stencil_layout,
                                   will_full_fast_clear);
      }
      att_state->current_layout = target_layout;
      att_state->current_stencil_layout = target_stencil_layout;

      if (att_state->pending_clear_aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
         assert(att_state->pending_clear_aspects == VK_IMAGE_ASPECT_COLOR_BIT);

         /* Multi-planar images are not supported as attachments */
         assert(image->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT);
         assert(image->n_planes == 1);

         uint32_t base_clear_layer = iview->planes[0].isl.base_array_layer;
         uint32_t clear_layer_count = fb->layers;

         if (att_state->fast_clear &&
             do_first_layer_clear(cmd_state, att_state)) {
            /* We only support fast-clears on the first layer */
            assert(level == 0 && base_layer == 0);

            union isl_color_value clear_color = {};
            anv_clear_color_from_att_state(&clear_color, att_state, iview);
            if (iview->image->vk.samples == 1) {
               anv_image_ccs_op(cmd_buffer, image,
                                iview->planes[0].isl.format,
                                iview->planes[0].isl.swizzle,
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                0, 0, 1, ISL_AUX_OP_FAST_CLEAR,
                                &clear_color,
                                false);
            } else {
               anv_image_mcs_op(cmd_buffer, image,
                                iview->planes[0].isl.format,
                                iview->planes[0].isl.swizzle,
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                0, 1, ISL_AUX_OP_FAST_CLEAR,
                                &clear_color,
                                false);
            }
            base_clear_layer++;
            clear_layer_count--;
            if (is_multiview)
               att_state->pending_clear_views &= ~1;

            if (isl_color_value_is_zero(clear_color,
                                        iview->planes[0].isl.format)) {
               /* This image has the auxiliary buffer enabled. We can mark the
                * subresource as not needing a resolve because the clear color
                * will match what's in every RENDER_SURFACE_STATE object when
                * it's being used for sampling.
                */
               set_image_fast_clear_state(cmd_buffer, iview->image,
                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                          ANV_FAST_CLEAR_DEFAULT_VALUE);
            } else {
               set_image_fast_clear_state(cmd_buffer, iview->image,
                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                          ANV_FAST_CLEAR_ANY);
            }
         }

         /* From the VkFramebufferCreateInfo spec:
          *
          * "If the render pass uses multiview, then layers must be one and each
          *  attachment requires a number of layers that is greater than the
          *  maximum bit index set in the view mask in the subpasses in which it
          *  is used."
          *
          * So if multiview is active we ignore the number of layers in the
          * framebuffer and instead we honor the view mask from the subpass.
          */
         if (is_multiview) {
            assert(image->n_planes == 1);
            uint32_t pending_clear_mask =
               get_multiview_subpass_clear_mask(cmd_state, att_state);

            u_foreach_bit(layer_idx, pending_clear_mask) {
               uint32_t layer =
                  iview->planes[0].isl.base_array_layer + layer_idx;

               anv_image_clear_color(cmd_buffer, image,
                                     VK_IMAGE_ASPECT_COLOR_BIT,
                                     att_state->aux_usage,
                                     iview->planes[0].isl.format,
                                     iview->planes[0].isl.swizzle,
                                     level, layer, 1,
                                     render_area,
                                     vk_to_isl_color(att_state->clear_value.color));
            }

            att_state->pending_clear_views &= ~pending_clear_mask;
         } else if (clear_layer_count > 0) {
            assert(image->n_planes == 1);
            anv_image_clear_color(cmd_buffer, image, VK_IMAGE_ASPECT_COLOR_BIT,
                                  att_state->aux_usage,
                                  iview->planes[0].isl.format,
                                  iview->planes[0].isl.swizzle,
                                  level, base_clear_layer, clear_layer_count,
                                  render_area,
                                  vk_to_isl_color(att_state->clear_value.color));
         }
      } else if (att_state->pending_clear_aspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
                                                     VK_IMAGE_ASPECT_STENCIL_BIT)) {
         if (att_state->fast_clear &&
             (att_state->pending_clear_aspects & VK_IMAGE_ASPECT_DEPTH_BIT)) {
            /* We currently only support HiZ for single-LOD images */
            assert(isl_aux_usage_has_hiz(iview->image->planes[0].aux_usage));
            assert(iview->planes[0].isl.base_level == 0);
            assert(iview->planes[0].isl.levels == 1);
         }

         if (is_multiview) {
            uint32_t pending_clear_mask =
              get_multiview_subpass_clear_mask(cmd_state, att_state);

            u_foreach_bit(layer_idx, pending_clear_mask) {
               uint32_t layer =
                  iview->planes[0].isl.base_array_layer + layer_idx;

               if (att_state->fast_clear) {
                  anv_image_hiz_clear(cmd_buffer, image,
                                      att_state->pending_clear_aspects,
                                      level, layer, 1, render_area,
                                      att_state->clear_value.depthStencil.stencil);
               } else {
                  anv_image_clear_depth_stencil(cmd_buffer, image,
                                                att_state->pending_clear_aspects,
                                                att_state->aux_usage,
                                                level, layer, 1, render_area,
                                                att_state->clear_value.depthStencil.depth,
                                                att_state->clear_value.depthStencil.stencil);
               }
            }

            att_state->pending_clear_views &= ~pending_clear_mask;
         } else {
            if (att_state->fast_clear) {
               anv_image_hiz_clear(cmd_buffer, image,
                                   att_state->pending_clear_aspects,
                                   level, base_layer, layer_count,
                                   render_area,
                                   att_state->clear_value.depthStencil.stencil);
            } else {
               anv_image_clear_depth_stencil(cmd_buffer, image,
                                             att_state->pending_clear_aspects,
                                             att_state->aux_usage,
                                             level, base_layer, layer_count,
                                             render_area,
                                             att_state->clear_value.depthStencil.depth,
                                             att_state->clear_value.depthStencil.stencil);
            }
         }
      } else  {
         assert(att_state->pending_clear_aspects == 0);
      }

      /* If multiview is enabled, then we are only done clearing when we no
       * longer have pending layers to clear, or when we have processed the
       * last subpass that uses this attachment.
       */
      if (!is_multiview ||
          att_state->pending_clear_views == 0 ||
          current_subpass_is_last_for_attachment(cmd_state, a)) {
         att_state->pending_clear_aspects = 0;
      }

      att_state->pending_load_aspects = 0;
   }

   /* We've transitioned all our images possibly fast clearing them.  Now we
    * can fill out the surface states that we will use as render targets
    * during actual subpass rendering.
    */
   VkResult result = genX(cmd_buffer_alloc_att_surf_states)(cmd_buffer,
                                                            pass, subpass);
   if (result != VK_SUCCESS)
      return;

   isl_null_fill_state(&cmd_buffer->device->isl_dev,
                       cmd_state->null_surface_state.map,
                       .size = isl_extent3d(fb->width, fb->height, fb->layers));

   for (uint32_t i = 0; i < subpass->attachment_count; ++i) {
      const uint32_t att = subpass->attachments[i].attachment;
      if (att == VK_ATTACHMENT_UNUSED)
         continue;

      assert(att < cmd_state->pass->attachment_count);
      struct anv_render_pass_attachment *pass_att = &pass->attachments[att];
      struct anv_attachment_state *att_state = &cmd_state->attachments[att];
      struct anv_image_view *iview = att_state->image_view;

      if (!vk_format_is_color(pass_att->format))
         continue;

      const VkImageUsageFlagBits att_usage = subpass->attachments[i].usage;
      assert(util_bitcount(att_usage) == 1);

      struct anv_surface_state *surface_state;
      isl_surf_usage_flags_t isl_surf_usage;
      enum isl_aux_usage isl_aux_usage;
      if (att_usage == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
         surface_state = &att_state->color;
         isl_surf_usage = ISL_SURF_USAGE_RENDER_TARGET_BIT;
         isl_aux_usage = att_state->aux_usage;
      } else if (att_usage == VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
         surface_state = &att_state->input;
         isl_surf_usage = ISL_SURF_USAGE_TEXTURE_BIT;
         isl_aux_usage =
            anv_layout_to_aux_usage(&cmd_buffer->device->info, iview->image,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                    att_state->current_layout);
      } else {
         continue;
      }

      /* We had better have a surface state when we get here */
      assert(surface_state->state.map);

      union isl_color_value clear_color = { .u32 = { 0, } };
      if (pass_att->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR &&
          att_state->fast_clear)
         anv_clear_color_from_att_state(&clear_color, att_state, iview);

      anv_image_fill_surface_state(cmd_buffer->device,
                                   iview->image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   &iview->planes[0].isl,
                                   isl_surf_usage,
                                   isl_aux_usage,
                                   &clear_color,
                                   0,
                                   surface_state,
                                   NULL);

      add_surface_state_relocs(cmd_buffer, *surface_state);

      if (GFX_VER < 10 &&
          pass_att->load_op == VK_ATTACHMENT_LOAD_OP_LOAD &&
          iview->image->planes[0].aux_usage != ISL_AUX_USAGE_NONE &&
          iview->planes[0].isl.base_level == 0 &&
          iview->planes[0].isl.base_array_layer == 0) {
         genX(copy_fast_clear_dwords)(cmd_buffer, surface_state->state,
                                      iview->image,
                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                      false /* copy to ss */);
      }
   }

#if GFX_VER >= 11
   /* The PIPE_CONTROL command description says:
    *
    *    "Whenever a Binding Table Index (BTI) used by a Render Taget Message
    *     points to a different RENDER_SURFACE_STATE, SW must issue a Render
    *     Target Cache Flush by enabling this bit. When render target flush
    *     is set due to new association of BTI, PS Scoreboard Stall bit must
    *     be set in this packet."
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
                             ANV_PIPE_STALL_AT_SCOREBOARD_BIT,
                             "change RT");
#endif

   cmd_buffer_emit_depth_stencil(cmd_buffer);
}

static enum blorp_filter
vk_to_blorp_resolve_mode(VkResolveModeFlagBitsKHR vk_mode)
{
   switch (vk_mode) {
   case VK_RESOLVE_MODE_SAMPLE_ZERO_BIT_KHR:
      return BLORP_FILTER_SAMPLE_0;
   case VK_RESOLVE_MODE_AVERAGE_BIT_KHR:
      return BLORP_FILTER_AVERAGE;
   case VK_RESOLVE_MODE_MIN_BIT_KHR:
      return BLORP_FILTER_MIN_SAMPLE;
   case VK_RESOLVE_MODE_MAX_BIT_KHR:
      return BLORP_FILTER_MAX_SAMPLE;
   default:
      return BLORP_FILTER_NONE;
   }
}

static void
cmd_buffer_end_subpass(struct anv_cmd_buffer *cmd_buffer)
{
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;
   struct anv_subpass *subpass = cmd_state->subpass;
   uint32_t subpass_id = anv_get_subpass_id(&cmd_buffer->state);
   struct anv_framebuffer *fb = cmd_buffer->state.framebuffer;

   /* We are done with the previous subpass and all rendering directly to that
    * subpass is now complete.  Zero out all the surface states so we don't
    * accidentally use them between now and the next subpass.
    */
   for (uint32_t i = 0; i < cmd_state->pass->attachment_count; ++i) {
      memset(&cmd_state->attachments[i].color, 0,
             sizeof(cmd_state->attachments[i].color));
      memset(&cmd_state->attachments[i].input, 0,
             sizeof(cmd_state->attachments[i].input));
   }
   cmd_state->null_surface_state = ANV_STATE_NULL;
   cmd_state->attachment_states = ANV_STATE_NULL;

   for (uint32_t i = 0; i < subpass->attachment_count; ++i) {
      const uint32_t a = subpass->attachments[i].attachment;
      if (a == VK_ATTACHMENT_UNUSED)
         continue;

      assert(a < cmd_state->pass->attachment_count);
      struct anv_attachment_state *att_state = &cmd_state->attachments[a];
      struct anv_image_view *iview = att_state->image_view;

      assert(util_bitcount(subpass->attachments[i].usage) == 1);
      if (subpass->attachments[i].usage ==
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
         /* We assume that if we're ending a subpass, we did do some rendering
          * so we may end up with compressed data.
          */
         genX(cmd_buffer_mark_image_written)(cmd_buffer, iview->image,
                                             VK_IMAGE_ASPECT_COLOR_BIT,
                                             att_state->aux_usage,
                                             iview->planes[0].isl.base_level,
                                             iview->planes[0].isl.base_array_layer,
                                             fb->layers);
      } else if (subpass->attachments[i].usage ==
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
         /* We may be writing depth or stencil so we need to mark the surface.
          * Unfortunately, there's no way to know at this point whether the
          * depth or stencil tests used will actually write to the surface.
          *
          * Even though stencil may be plane 1, it always shares a base_level
          * with depth.
          */
         const struct isl_view *ds_view = &iview->planes[0].isl;
         if (iview->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
            genX(cmd_buffer_mark_image_written)(cmd_buffer, iview->image,
                                                VK_IMAGE_ASPECT_DEPTH_BIT,
                                                att_state->aux_usage,
                                                ds_view->base_level,
                                                ds_view->base_array_layer,
                                                fb->layers);
         }
         if (iview->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
            /* Even though stencil may be plane 1, it always shares a
             * base_level with depth.
             */
            genX(cmd_buffer_mark_image_written)(cmd_buffer, iview->image,
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                                ISL_AUX_USAGE_NONE,
                                                ds_view->base_level,
                                                ds_view->base_array_layer,
                                                fb->layers);
         }
      }
   }

   if (subpass->has_color_resolve) {
      /* We are about to do some MSAA resolves.  We need to flush so that the
       * result of writes to the MSAA color attachments show up in the sampler
       * when we blit to the single-sampled resolve target.
       */
      anv_add_pending_pipe_bits(cmd_buffer,
                                ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT |
                                ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT,
                                "MSAA resolve");

      for (uint32_t i = 0; i < subpass->color_count; ++i) {
         uint32_t src_att = subpass->color_attachments[i].attachment;
         uint32_t dst_att = subpass->resolve_attachments[i].attachment;

         if (dst_att == VK_ATTACHMENT_UNUSED)
            continue;

         assert(src_att < cmd_buffer->state.pass->attachment_count);
         assert(dst_att < cmd_buffer->state.pass->attachment_count);

         if (cmd_buffer->state.attachments[dst_att].pending_clear_aspects) {
            /* From the Vulkan 1.0 spec:
             *
             *    If the first use of an attachment in a render pass is as a
             *    resolve attachment, then the loadOp is effectively ignored
             *    as the resolve is guaranteed to overwrite all pixels in the
             *    render area.
             */
            cmd_buffer->state.attachments[dst_att].pending_clear_aspects = 0;
         }

         struct anv_image_view *src_iview = cmd_state->attachments[src_att].image_view;
         struct anv_image_view *dst_iview = cmd_state->attachments[dst_att].image_view;

         const VkRect2D render_area = cmd_buffer->state.render_area;

         enum isl_aux_usage src_aux_usage =
            cmd_buffer->state.attachments[src_att].aux_usage;
         enum isl_aux_usage dst_aux_usage =
            cmd_buffer->state.attachments[dst_att].aux_usage;

         assert(src_iview->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT &&
                dst_iview->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT);

         anv_image_msaa_resolve(cmd_buffer,
                                src_iview->image, src_aux_usage,
                                src_iview->planes[0].isl.base_level,
                                src_iview->planes[0].isl.base_array_layer,
                                dst_iview->image, dst_aux_usage,
                                dst_iview->planes[0].isl.base_level,
                                dst_iview->planes[0].isl.base_array_layer,
                                VK_IMAGE_ASPECT_COLOR_BIT,
                                render_area.offset.x, render_area.offset.y,
                                render_area.offset.x, render_area.offset.y,
                                render_area.extent.width,
                                render_area.extent.height,
                                fb->layers, BLORP_FILTER_NONE);
      }
   }

   if (subpass->ds_resolve_attachment) {
      /* We are about to do some MSAA resolves.  We need to flush so that the
       * result of writes to the MSAA depth attachments show up in the sampler
       * when we blit to the single-sampled resolve target.
       */
      anv_add_pending_pipe_bits(cmd_buffer,
                              ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT |
                              ANV_PIPE_DEPTH_CACHE_FLUSH_BIT,
                              "MSAA resolve");

      uint32_t src_att = subpass->depth_stencil_attachment->attachment;
      uint32_t dst_att = subpass->ds_resolve_attachment->attachment;

      assert(src_att < cmd_buffer->state.pass->attachment_count);
      assert(dst_att < cmd_buffer->state.pass->attachment_count);

      if (cmd_buffer->state.attachments[dst_att].pending_clear_aspects) {
         /* From the Vulkan 1.0 spec:
          *
          *    If the first use of an attachment in a render pass is as a
          *    resolve attachment, then the loadOp is effectively ignored
          *    as the resolve is guaranteed to overwrite all pixels in the
          *    render area.
          */
         cmd_buffer->state.attachments[dst_att].pending_clear_aspects = 0;
      }

      struct anv_image_view *src_iview = cmd_state->attachments[src_att].image_view;
      struct anv_image_view *dst_iview = cmd_state->attachments[dst_att].image_view;

      const VkRect2D render_area = cmd_buffer->state.render_area;

      struct anv_attachment_state *src_state =
         &cmd_state->attachments[src_att];
      struct anv_attachment_state *dst_state =
         &cmd_state->attachments[dst_att];

      if ((src_iview->image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) &&
          subpass->depth_resolve_mode != VK_RESOLVE_MODE_NONE_KHR) {

         /* MSAA resolves sample from the source attachment.  Transition the
          * depth attachment first to get rid of any HiZ that we may not be
          * able to handle.
          */
         transition_depth_buffer(cmd_buffer, src_iview->image,
                                 src_iview->planes[0].isl.base_array_layer,
                                 fb->layers,
                                 src_state->current_layout,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 false /* will_full_fast_clear */);
         src_state->aux_usage =
            anv_layout_to_aux_usage(&cmd_buffer->device->info, src_iview->image,
                                    VK_IMAGE_ASPECT_DEPTH_BIT,
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
         src_state->current_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

         /* MSAA resolves write to the resolve attachment as if it were any
          * other transfer op.  Transition the resolve attachment accordingly.
          */
         VkImageLayout dst_initial_layout = dst_state->current_layout;

         /* If our render area is the entire size of the image, we're going to
          * blow it all away so we can claim the initial layout is UNDEFINED
          * and we'll get a HiZ ambiguate instead of a resolve.
          */
         if (dst_iview->image->vk.image_type != VK_IMAGE_TYPE_3D &&
             render_area.offset.x == 0 && render_area.offset.y == 0 &&
             render_area.extent.width == dst_iview->vk.extent.width &&
             render_area.extent.height == dst_iview->vk.extent.height)
            dst_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;

         transition_depth_buffer(cmd_buffer, dst_iview->image,
                                 dst_iview->planes[0].isl.base_array_layer,
                                 fb->layers,
                                 dst_initial_layout,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 false /* will_full_fast_clear */);
         dst_state->aux_usage =
            anv_layout_to_aux_usage(&cmd_buffer->device->info, dst_iview->image,
                                    VK_IMAGE_ASPECT_DEPTH_BIT,
                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
         dst_state->current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

         enum blorp_filter filter =
            vk_to_blorp_resolve_mode(subpass->depth_resolve_mode);

         anv_image_msaa_resolve(cmd_buffer,
                                src_iview->image, src_state->aux_usage,
                                src_iview->planes[0].isl.base_level,
                                src_iview->planes[0].isl.base_array_layer,
                                dst_iview->image, dst_state->aux_usage,
                                dst_iview->planes[0].isl.base_level,
                                dst_iview->planes[0].isl.base_array_layer,
                                VK_IMAGE_ASPECT_DEPTH_BIT,
                                render_area.offset.x, render_area.offset.y,
                                render_area.offset.x, render_area.offset.y,
                                render_area.extent.width,
                                render_area.extent.height,
                                fb->layers, filter);
      }

      if ((src_iview->image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
          subpass->stencil_resolve_mode != VK_RESOLVE_MODE_NONE_KHR) {

         src_state->current_stencil_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
         dst_state->current_stencil_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

         enum isl_aux_usage src_aux_usage = ISL_AUX_USAGE_NONE;
         const uint32_t plane =
            anv_image_aspect_to_plane(dst_iview->image, VK_IMAGE_ASPECT_STENCIL_BIT);
         enum isl_aux_usage dst_aux_usage =
            dst_iview->image->planes[plane].aux_usage;

         enum blorp_filter filter =
            vk_to_blorp_resolve_mode(subpass->stencil_resolve_mode);

         anv_image_msaa_resolve(cmd_buffer,
                                src_iview->image, src_aux_usage,
                                src_iview->planes[0].isl.base_level,
                                src_iview->planes[0].isl.base_array_layer,
                                dst_iview->image, dst_aux_usage,
                                dst_iview->planes[0].isl.base_level,
                                dst_iview->planes[0].isl.base_array_layer,
                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                render_area.offset.x, render_area.offset.y,
                                render_area.offset.x, render_area.offset.y,
                                render_area.extent.width,
                                render_area.extent.height,
                                fb->layers, filter);
      }
   }

#if GFX_VER == 7
   /* On gfx7, we have to store a texturable version of the stencil buffer in
    * a shadow whenever VK_IMAGE_USAGE_SAMPLED_BIT is set and copy back and
    * forth at strategic points. Stencil writes are only allowed in following
    * layouts:
    *
    *  - VK_IMAGE_LAYOUT_GENERAL
    *  - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    *  - VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    *  - VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
    *  - VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR
    *
    * For general, we have no nice opportunity to transition so we do the copy
    * to the shadow unconditionally at the end of the subpass. For transfer
    * destinations, we can update it as part of the transfer op. For the other
    * layouts, we delay the copy until a transition into some other layout.
    */
   if (subpass->depth_stencil_attachment) {
      uint32_t a = subpass->depth_stencil_attachment->attachment;
      assert(a != VK_ATTACHMENT_UNUSED);

      struct anv_attachment_state *att_state = &cmd_state->attachments[a];
      struct anv_image_view *iview = cmd_state->attachments[a].image_view;;
      const struct anv_image *image = iview->image;

      if (image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
         const uint32_t plane =
            anv_image_aspect_to_plane(image, VK_IMAGE_ASPECT_STENCIL_BIT);

         if (anv_surface_is_valid(&image->planes[plane].shadow_surface) &&
             att_state->current_stencil_layout == VK_IMAGE_LAYOUT_GENERAL) {
            assert(image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT);
            anv_image_copy_to_shadow(cmd_buffer, image,
                                     VK_IMAGE_ASPECT_STENCIL_BIT,
                                     iview->planes[plane].isl.base_level, 1,
                                     iview->planes[plane].isl.base_array_layer,
                                     fb->layers);
         }
      }
   }
#endif /* GFX_VER == 7 */

   for (uint32_t i = 0; i < subpass->attachment_count; ++i) {
      const uint32_t a = subpass->attachments[i].attachment;
      if (a == VK_ATTACHMENT_UNUSED)
         continue;

      if (cmd_state->pass->attachments[a].last_subpass_idx != subpass_id)
         continue;

      assert(a < cmd_state->pass->attachment_count);
      struct anv_attachment_state *att_state = &cmd_state->attachments[a];
      struct anv_image_view *iview = cmd_state->attachments[a].image_view;
      const struct anv_image *image = iview->image;

      /* Transition the image into the final layout for this render pass */
      VkImageLayout target_layout =
         cmd_state->pass->attachments[a].final_layout;
      VkImageLayout target_stencil_layout =
         cmd_state->pass->attachments[a].stencil_final_layout;

      uint32_t base_layer, layer_count;
      if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
         base_layer = 0;
         layer_count = anv_minify(iview->image->vk.extent.depth,
                                  iview->planes[0].isl.base_level);
      } else {
         base_layer = iview->planes[0].isl.base_array_layer;
         layer_count = fb->layers;
      }

      if (image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
         assert(image->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT);
         transition_color_buffer(cmd_buffer, image, VK_IMAGE_ASPECT_COLOR_BIT,
                                 iview->planes[0].isl.base_level, 1,
                                 base_layer, layer_count,
                                 att_state->current_layout, target_layout,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 false /* will_full_fast_clear */);
      }

      if (image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
         transition_depth_buffer(cmd_buffer, image,
                                 base_layer, layer_count,
                                 att_state->current_layout, target_layout,
                                 false /* will_full_fast_clear */);
      }

      if (image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
         transition_stencil_buffer(cmd_buffer, image,
                                   iview->planes[0].isl.base_level, 1,
                                   base_layer, layer_count,
                                   att_state->current_stencil_layout,
                                   target_stencil_layout,
                                   false /* will_full_fast_clear */);
      }
   }

   /* Accumulate any subpass flushes that need to happen after the subpass.
    * Yes, they do get accumulated twice in the NextSubpass case but since
    * genX_CmdNextSubpass just calls end/begin back-to-back, we just end up
    * ORing the bits in twice so it's harmless.
    */
   anv_add_pending_pipe_bits(cmd_buffer,
                             cmd_buffer->state.pass->subpass_flushes[subpass_id + 1],
                             "end subpass deps/attachments");
}

void genX(CmdBeginRenderPass2)(
    VkCommandBuffer                             commandBuffer,
    const VkRenderPassBeginInfo*                pRenderPassBeginInfo,
    const VkSubpassBeginInfoKHR*                pSubpassBeginInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_render_pass, pass, pRenderPassBeginInfo->renderPass);
   ANV_FROM_HANDLE(anv_framebuffer, framebuffer, pRenderPassBeginInfo->framebuffer);
   VkResult result;

   if (!is_render_queue_cmd_buffer(cmd_buffer)) {
      assert(!"Trying to start a render pass on non-render queue!");
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_UNKNOWN);
      return;
   }

   cmd_buffer->state.framebuffer = framebuffer;
   cmd_buffer->state.pass = pass;
   cmd_buffer->state.render_area = pRenderPassBeginInfo->renderArea;

   anv_measure_beginrenderpass(cmd_buffer);

   result = genX(cmd_buffer_setup_attachments)(cmd_buffer, pass,
                                               framebuffer,
                                               pRenderPassBeginInfo);
   if (result != VK_SUCCESS) {
      assert(anv_batch_has_error(&cmd_buffer->batch));
      return;
   }

   genX(flush_pipeline_select_3d)(cmd_buffer);

   cmd_buffer_begin_subpass(cmd_buffer, 0);
}

void genX(CmdNextSubpass2)(
    VkCommandBuffer                             commandBuffer,
    const VkSubpassBeginInfoKHR*                pSubpassBeginInfo,
    const VkSubpassEndInfoKHR*                  pSubpassEndInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   assert(cmd_buffer->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

   uint32_t prev_subpass = anv_get_subpass_id(&cmd_buffer->state);
   cmd_buffer_end_subpass(cmd_buffer);
   cmd_buffer_begin_subpass(cmd_buffer, prev_subpass + 1);
}

void genX(CmdEndRenderPass2)(
    VkCommandBuffer                             commandBuffer,
    const VkSubpassEndInfoKHR*                  pSubpassEndInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   if (anv_batch_has_error(&cmd_buffer->batch))
      return;

   cmd_buffer_end_subpass(cmd_buffer);

   cmd_buffer->state.hiz_enabled = false;

   /* Remove references to render pass specific state. This enables us to
    * detect whether or not we're in a renderpass.
    */
   cmd_buffer->state.framebuffer = NULL;
   cmd_buffer->state.pass = NULL;
   cmd_buffer->state.subpass = NULL;
}

void
genX(cmd_emit_conditional_render_predicate)(struct anv_cmd_buffer *cmd_buffer)
{
#if GFX_VERx10 >= 75
   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   mi_store(&b, mi_reg64(MI_PREDICATE_SRC0),
                mi_reg32(ANV_PREDICATE_RESULT_REG));
   mi_store(&b, mi_reg64(MI_PREDICATE_SRC1), mi_imm(0));

   anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
      mip.CombineOperation = COMBINE_SET;
      mip.CompareOperation = COMPARE_SRCS_EQUAL;
   }
#endif
}

#if GFX_VERx10 >= 75
void genX(CmdBeginConditionalRenderingEXT)(
   VkCommandBuffer                             commandBuffer,
   const VkConditionalRenderingBeginInfoEXT*   pConditionalRenderingBegin)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, pConditionalRenderingBegin->buffer);
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;
   struct anv_address value_address =
      anv_address_add(buffer->address, pConditionalRenderingBegin->offset);

   const bool isInverted = pConditionalRenderingBegin->flags &
                           VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT;

   cmd_state->conditional_render_enabled = true;

   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   struct mi_builder b;
   mi_builder_init(&b, &cmd_buffer->device->info, &cmd_buffer->batch);

   /* Section 19.4 of the Vulkan 1.1.85 spec says:
    *
    *    If the value of the predicate in buffer memory changes
    *    while conditional rendering is active, the rendering commands
    *    may be discarded in an implementation-dependent way.
    *    Some implementations may latch the value of the predicate
    *    upon beginning conditional rendering while others
    *    may read it before every rendering command.
    *
    * So it's perfectly fine to read a value from the buffer once.
    */
   struct mi_value value =  mi_mem32(value_address);

   /* Precompute predicate result, it is necessary to support secondary
    * command buffers since it is unknown if conditional rendering is
    * inverted when populating them.
    */
   mi_store(&b, mi_reg64(ANV_PREDICATE_RESULT_REG),
                isInverted ? mi_uge(&b, mi_imm(0), value) :
                             mi_ult(&b, mi_imm(0), value));
}

void genX(CmdEndConditionalRenderingEXT)(
	VkCommandBuffer                             commandBuffer)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;

   cmd_state->conditional_render_enabled = false;
}
#endif

/* Set of stage bits for which are pipelined, i.e. they get queued
 * by the command streamer for later execution.
 */
#define ANV_PIPELINE_STAGE_PIPELINED_BITS \
   ~(VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR | \
     VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR | \
     VK_PIPELINE_STAGE_2_HOST_BIT_KHR | \
     VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT)

void genX(CmdSetEvent2KHR)(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     _event,
    const VkDependencyInfoKHR*                  pDependencyInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_event, event, _event);

   VkPipelineStageFlags2KHR src_stages = 0;

   for (uint32_t i = 0; i < pDependencyInfo->memoryBarrierCount; i++)
      src_stages |= pDependencyInfo->pMemoryBarriers[i].srcStageMask;
   for (uint32_t i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++)
      src_stages |= pDependencyInfo->pBufferMemoryBarriers[i].srcStageMask;
   for (uint32_t i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++)
      src_stages |= pDependencyInfo->pImageMemoryBarriers[i].srcStageMask;

   cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      if (src_stages & ANV_PIPELINE_STAGE_PIPELINED_BITS) {
         pc.StallAtPixelScoreboard = true;
         pc.CommandStreamerStallEnable = true;
      }

      pc.DestinationAddressType  = DAT_PPGTT,
      pc.PostSyncOperation       = WriteImmediateData,
      pc.Address = (struct anv_address) {
         cmd_buffer->device->dynamic_state_pool.block_pool.bo,
         event->state.offset
      };
      pc.ImmediateData           = VK_EVENT_SET;
      anv_debug_dump_pc(pc);
   }
}

void genX(CmdResetEvent2KHR)(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     _event,
    VkPipelineStageFlags2KHR                    stageMask)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_event, event, _event);

   cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
   genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);

   anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
      if (stageMask & ANV_PIPELINE_STAGE_PIPELINED_BITS) {
         pc.StallAtPixelScoreboard = true;
         pc.CommandStreamerStallEnable = true;
      }

      pc.DestinationAddressType  = DAT_PPGTT;
      pc.PostSyncOperation       = WriteImmediateData;
      pc.Address = (struct anv_address) {
         cmd_buffer->device->dynamic_state_pool.block_pool.bo,
         event->state.offset
      };
      pc.ImmediateData           = VK_EVENT_RESET;
      anv_debug_dump_pc(pc);
   }
}

void genX(CmdWaitEvents2KHR)(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    const VkDependencyInfoKHR*                  pDependencyInfos)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

#if GFX_VER >= 8
   for (uint32_t i = 0; i < eventCount; i++) {
      ANV_FROM_HANDLE(anv_event, event, pEvents[i]);

      anv_batch_emit(&cmd_buffer->batch, GENX(MI_SEMAPHORE_WAIT), sem) {
         sem.WaitMode            = PollingMode,
         sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD,
         sem.SemaphoreDataDword  = VK_EVENT_SET,
         sem.SemaphoreAddress = (struct anv_address) {
            cmd_buffer->device->dynamic_state_pool.block_pool.bo,
            event->state.offset
         };
      }
   }
#else
   anv_finishme("Implement events on gfx7");
#endif

   cmd_buffer_barrier(cmd_buffer, pDependencyInfos, "wait event");
}

VkResult genX(CmdSetPerformanceOverrideINTEL)(
    VkCommandBuffer                             commandBuffer,
    const VkPerformanceOverrideInfoINTEL*       pOverrideInfo)
{
   ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);

   switch (pOverrideInfo->type) {
   case VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL: {
#if GFX_VER >= 9
      anv_batch_write_reg(&cmd_buffer->batch, GENX(CS_DEBUG_MODE2), csdm2) {
         csdm2._3DRenderingInstructionDisable = pOverrideInfo->enable;
         csdm2.MediaInstructionDisable = pOverrideInfo->enable;
         csdm2._3DRenderingInstructionDisableMask = true;
         csdm2.MediaInstructionDisableMask = true;
      }
#else
      anv_batch_write_reg(&cmd_buffer->batch, GENX(INSTPM), instpm) {
         instpm._3DRenderingInstructionDisable = pOverrideInfo->enable;
         instpm.MediaInstructionDisable = pOverrideInfo->enable;
         instpm._3DRenderingInstructionDisableMask = true;
         instpm.MediaInstructionDisableMask = true;
      }
#endif
      break;
   }

   case VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL:
      if (pOverrideInfo->enable) {
         /* FLUSH ALL THE THINGS! As requested by the MDAPI team. */
         anv_add_pending_pipe_bits(cmd_buffer,
                                   ANV_PIPE_FLUSH_BITS |
                                   ANV_PIPE_INVALIDATE_BITS,
                                   "perf counter isolation");
         genX(cmd_buffer_apply_pipe_flushes)(cmd_buffer);
      }
      break;

   default:
      unreachable("Invalid override");
   }

   return VK_SUCCESS;
}

VkResult genX(CmdSetPerformanceStreamMarkerINTEL)(
    VkCommandBuffer                             commandBuffer,
    const VkPerformanceStreamMarkerInfoINTEL*   pMarkerInfo)
{
   /* TODO: Waiting on the register to write, might depend on generation. */

   return VK_SUCCESS;
}

void genX(cmd_emit_timestamp)(struct anv_batch *batch,
                              struct anv_bo *bo,
                              uint32_t offset) {
   anv_batch_emit(batch, GENX(PIPE_CONTROL), pc) {
      pc.CommandStreamerStallEnable = true;
      pc.PostSyncOperation       = WriteTimestamp;
      pc.Address = (struct anv_address) {bo, offset};
      anv_debug_dump_pc(pc);
   }
}
