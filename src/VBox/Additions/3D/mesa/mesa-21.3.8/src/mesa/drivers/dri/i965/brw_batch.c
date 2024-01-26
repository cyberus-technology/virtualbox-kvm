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

#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "brw_bufmgr.h"
#include "brw_buffers.h"
#include "brw_fbo.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "common/intel_decoder.h"
#include "common/intel_gem.h"

#include "util/hash_table.h"

#include <xf86drm.h>
#include "drm-uapi/i915_drm.h"

#define FILE_DEBUG_FLAG DEBUG_BUFMGR

/**
 * Target sizes of the batch and state buffers.  We create the initial
 * buffers at these sizes, and flush when they're nearly full.  If we
 * underestimate how close we are to the end, and suddenly need more space
 * in the middle of a draw, we can grow the buffers, and finish the draw.
 * At that point, we'll be over our target size, so the next operation
 * should flush.  Each time we flush the batch, we recreate both buffers
 * at the original target size, so it doesn't grow without bound.
 */
#define BATCH_SZ (20 * 1024)
#define STATE_SZ (16 * 1024)

static void
brw_batch_reset(struct brw_context *brw);
static void
brw_new_batch(struct brw_context *brw);

static unsigned
num_fences(struct brw_batch *batch)
{
   return util_dynarray_num_elements(&batch->exec_fences,
                                     struct drm_i915_gem_exec_fence);
}


static void
dump_validation_list(struct brw_batch *batch)
{
   fprintf(stderr, "Validation list (length %d):\n", batch->exec_count);

   for (int i = 0; i < batch->exec_count; i++) {
      uint64_t flags = batch->validation_list[i].flags;
      assert(batch->validation_list[i].handle ==
             batch->exec_bos[i]->gem_handle);
      fprintf(stderr, "[%2d]: %2d %-14s %p %s%-7s @ 0x%"PRIx64"%s (%"PRIu64"B)\n",
              i,
              batch->validation_list[i].handle,
              batch->exec_bos[i]->name,
              batch->exec_bos[i],
              (flags & EXEC_OBJECT_SUPPORTS_48B_ADDRESS) ? "(48b" : "(32b",
              (flags & EXEC_OBJECT_WRITE) ? " write)" : ")",
              (uint64_t)batch->validation_list[i].offset,
              (flags & EXEC_OBJECT_PINNED) ? " (pinned)" : "",
              batch->exec_bos[i]->size);
   }
}

static struct intel_batch_decode_bo
decode_get_bo(void *v_brw, bool ppgtt, uint64_t address)
{
   struct brw_context *brw = v_brw;
   struct brw_batch *batch = &brw->batch;

   for (int i = 0; i < batch->exec_count; i++) {
      struct brw_bo *bo = batch->exec_bos[i];
      /* The decoder zeroes out the top 16 bits, so we need to as well */
      uint64_t bo_address = bo->gtt_offset & (~0ull >> 16);

      if (address >= bo_address && address < bo_address + bo->size) {
         return (struct intel_batch_decode_bo) {
            .addr = bo_address,
            .size = bo->size,
            .map = brw_bo_map(brw, bo, MAP_READ),
         };
      }
   }

   return (struct intel_batch_decode_bo) { };
}

static unsigned
decode_get_state_size(void *v_brw, uint64_t address, uint64_t base_address)
{
   struct brw_context *brw = v_brw;
   struct brw_batch *batch = &brw->batch;
   unsigned size = (uintptr_t)
      _mesa_hash_table_u64_search(batch->state_batch_sizes,
                                  address - base_address);
   return size;
}

static void
init_reloc_list(struct brw_reloc_list *rlist, int count)
{
   rlist->reloc_count = 0;
   rlist->reloc_array_size = count;
   rlist->relocs = malloc(rlist->reloc_array_size *
                          sizeof(struct drm_i915_gem_relocation_entry));
}

void
brw_batch_init(struct brw_context *brw)
{
   struct brw_screen *screen = brw->screen;
   struct brw_batch *batch = &brw->batch;
   const struct intel_device_info *devinfo = &screen->devinfo;

   if (INTEL_DEBUG(DEBUG_BATCH)) {
      /* The shadow doesn't get relocs written so state decode fails. */
      batch->use_shadow_copy = false;
   } else
      batch->use_shadow_copy = !devinfo->has_llc;

   init_reloc_list(&batch->batch_relocs, 250);
   init_reloc_list(&batch->state_relocs, 250);

   batch->batch.map = NULL;
   batch->state.map = NULL;
   batch->exec_count = 0;
   batch->exec_array_size = 100;
   batch->exec_bos =
      malloc(batch->exec_array_size * sizeof(batch->exec_bos[0]));
   batch->validation_list =
      malloc(batch->exec_array_size * sizeof(batch->validation_list[0]));
   batch->contains_fence_signal = false;

   if (INTEL_DEBUG(DEBUG_BATCH)) {
      batch->state_batch_sizes =
         _mesa_hash_table_u64_create(NULL);

      const unsigned decode_flags =
         INTEL_BATCH_DECODE_FULL |
         (INTEL_DEBUG(DEBUG_COLOR) ? INTEL_BATCH_DECODE_IN_COLOR : 0) |
         INTEL_BATCH_DECODE_OFFSETS |
         INTEL_BATCH_DECODE_FLOATS;

      intel_batch_decode_ctx_init(&batch->decoder, devinfo, stderr,
                                  decode_flags, NULL, decode_get_bo,
                                  decode_get_state_size, brw);
      batch->decoder.max_vbo_decoded_lines = 100;
   }

   batch->use_batch_first =
      screen->kernel_features & KERNEL_ALLOWS_EXEC_BATCH_FIRST;

   /* PIPE_CONTROL needs a w/a but only on gfx6 */
   batch->valid_reloc_flags = EXEC_OBJECT_WRITE;
   if (devinfo->ver == 6)
      batch->valid_reloc_flags |= EXEC_OBJECT_NEEDS_GTT;

   brw_batch_reset(brw);
}

#define READ_ONCE(x) (*(volatile __typeof__(x) *)&(x))

static unsigned
add_exec_bo(struct brw_batch *batch, struct brw_bo *bo)
{
   assert(bo->bufmgr == batch->batch.bo->bufmgr);

   unsigned index = READ_ONCE(bo->index);

   if (index < batch->exec_count && batch->exec_bos[index] == bo)
      return index;

   /* May have been shared between multiple active batches */
   for (index = 0; index < batch->exec_count; index++) {
      if (batch->exec_bos[index] == bo)
         return index;
   }

   brw_bo_reference(bo);

   if (batch->exec_count == batch->exec_array_size) {
      batch->exec_array_size *= 2;
      batch->exec_bos =
         realloc(batch->exec_bos,
                 batch->exec_array_size * sizeof(batch->exec_bos[0]));
      batch->validation_list =
         realloc(batch->validation_list,
                 batch->exec_array_size * sizeof(batch->validation_list[0]));
   }

   batch->validation_list[batch->exec_count] =
      (struct drm_i915_gem_exec_object2) {
         .handle = bo->gem_handle,
         .offset = bo->gtt_offset,
         .flags = bo->kflags,
      };

   bo->index = batch->exec_count;
   batch->exec_bos[batch->exec_count] = bo;
   batch->aperture_space += bo->size;

   return batch->exec_count++;
}

static void
recreate_growing_buffer(struct brw_context *brw,
                        struct brw_growing_bo *grow,
                        const char *name, unsigned size,
                        enum brw_memory_zone memzone)
{
   struct brw_screen *screen = brw->screen;
   struct brw_batch *batch = &brw->batch;
   struct brw_bufmgr *bufmgr = screen->bufmgr;

   /* We can't grow buffers when using softpin, so just overallocate them. */
   if (brw_using_softpin(bufmgr))
      size *= 2;

   grow->bo = brw_bo_alloc(bufmgr, name, size, memzone);
   grow->bo->kflags |= can_do_exec_capture(screen) ? EXEC_OBJECT_CAPTURE : 0;
   grow->partial_bo = NULL;
   grow->partial_bo_map = NULL;
   grow->partial_bytes = 0;
   grow->memzone = memzone;

   if (batch->use_shadow_copy)
      grow->map = realloc(grow->map, grow->bo->size);
   else
      grow->map = brw_bo_map(brw, grow->bo, MAP_READ | MAP_WRITE);
}

static void
brw_batch_reset(struct brw_context *brw)
{
   struct brw_batch *batch = &brw->batch;

   if (batch->last_bo != NULL) {
      brw_bo_unreference(batch->last_bo);
      batch->last_bo = NULL;
   }
   batch->last_bo = batch->batch.bo;

   recreate_growing_buffer(brw, &batch->batch, "batchbuffer", BATCH_SZ,
                           BRW_MEMZONE_OTHER);
   batch->map_next = batch->batch.map;

   recreate_growing_buffer(brw, &batch->state, "statebuffer", STATE_SZ,
                           BRW_MEMZONE_DYNAMIC);

   /* Avoid making 0 a valid state offset - otherwise the decoder will try
    * and decode data when we use offset 0 as a null pointer.
    */
   batch->state_used = 1;

   add_exec_bo(batch, batch->batch.bo);
   assert(batch->batch.bo->index == 0);

   batch->needs_sol_reset = false;
   batch->state_base_address_emitted = false;

   if (batch->state_batch_sizes)
      _mesa_hash_table_u64_clear(batch->state_batch_sizes);

   /* Always add workaround_bo which contains a driver identifier to be
    * recorded in error states.
    */
   struct brw_bo *identifier_bo = brw->workaround_bo;
   if (identifier_bo)
      add_exec_bo(batch, identifier_bo);

   if (batch->contains_fence_signal)
      batch->contains_fence_signal = false;
}

static void
brw_batch_reset_and_clear_render_cache(struct brw_context *brw)
{
   brw_batch_reset(brw);
   brw_cache_sets_clear(brw);
}

void
brw_batch_save_state(struct brw_context *brw)
{
   brw->batch.saved.map_next = brw->batch.map_next;
   brw->batch.saved.batch_reloc_count = brw->batch.batch_relocs.reloc_count;
   brw->batch.saved.state_reloc_count = brw->batch.state_relocs.reloc_count;
   brw->batch.saved.exec_count = brw->batch.exec_count;
}

bool
brw_batch_saved_state_is_empty(struct brw_context *brw)
{
   struct brw_batch *batch = &brw->batch;
   return (batch->saved.map_next == batch->batch.map);
}

void
brw_batch_reset_to_saved(struct brw_context *brw)
{
   for (int i = brw->batch.saved.exec_count;
        i < brw->batch.exec_count; i++) {
      brw_bo_unreference(brw->batch.exec_bos[i]);
   }
   brw->batch.batch_relocs.reloc_count = brw->batch.saved.batch_reloc_count;
   brw->batch.state_relocs.reloc_count = brw->batch.saved.state_reloc_count;
   brw->batch.exec_count = brw->batch.saved.exec_count;

   brw->batch.map_next = brw->batch.saved.map_next;
   if (USED_BATCH(brw->batch) == 0)
      brw_new_batch(brw);
}

void
brw_batch_free(struct brw_batch *batch)
{
   if (batch->use_shadow_copy) {
      free(batch->batch.map);
      free(batch->state.map);
   }

   for (int i = 0; i < batch->exec_count; i++) {
      brw_bo_unreference(batch->exec_bos[i]);
   }
   free(batch->batch_relocs.relocs);
   free(batch->state_relocs.relocs);
   free(batch->exec_bos);
   free(batch->validation_list);

   brw_bo_unreference(batch->last_bo);
   brw_bo_unreference(batch->batch.bo);
   brw_bo_unreference(batch->state.bo);
   if (batch->state_batch_sizes) {
      _mesa_hash_table_u64_destroy(batch->state_batch_sizes);
      intel_batch_decode_ctx_finish(&batch->decoder);
   }
}

/**
 * Finish copying the old batch/state buffer's contents to the new one
 * after we tried to "grow" the buffer in an earlier operation.
 */
static void
finish_growing_bos(struct brw_growing_bo *grow)
{
   struct brw_bo *old_bo = grow->partial_bo;
   if (!old_bo)
      return;

   memcpy(grow->map, grow->partial_bo_map, grow->partial_bytes);

   grow->partial_bo = NULL;
   grow->partial_bo_map = NULL;
   grow->partial_bytes = 0;

   brw_bo_unreference(old_bo);
}

static void
replace_bo_in_reloc_list(struct brw_reloc_list *rlist,
                         uint32_t old_handle, uint32_t new_handle)
{
   for (int i = 0; i < rlist->reloc_count; i++) {
      if (rlist->relocs[i].target_handle == old_handle)
         rlist->relocs[i].target_handle = new_handle;
   }
}

/**
 * Grow either the batch or state buffer to a new larger size.
 *
 * We can't actually grow buffers, so we allocate a new one, copy over
 * the existing contents, and update our lists to refer to the new one.
 *
 * Note that this is only temporary - each new batch recreates the buffers
 * at their original target size (BATCH_SZ or STATE_SZ).
 */
static void
grow_buffer(struct brw_context *brw,
            struct brw_growing_bo *grow,
            unsigned existing_bytes,
            unsigned new_size)
{
   struct brw_batch *batch = &brw->batch;
   struct brw_bufmgr *bufmgr = brw->bufmgr;
   struct brw_bo *bo = grow->bo;

   /* We can't grow buffers that are softpinned, as the growing mechanism
    * involves putting a larger buffer at the same gtt_offset...and we've
    * only allocated the smaller amount of VMA.  Without relocations, this
    * simply won't work.  This should never happen, however.
    */
   assert(!(bo->kflags & EXEC_OBJECT_PINNED));

   perf_debug("Growing %s - ran out of space\n", bo->name);

   if (grow->partial_bo) {
      /* We've already grown once, and now we need to do it again.
       * Finish our last grow operation so we can start a new one.
       * This should basically never happen.
       */
      perf_debug("Had to grow multiple times");
      finish_growing_bos(grow);
   }

   struct brw_bo *new_bo =
      brw_bo_alloc(bufmgr, bo->name, new_size, grow->memzone);

   /* Copy existing data to the new larger buffer */
   grow->partial_bo_map = grow->map;

   if (batch->use_shadow_copy) {
      /* We can't safely use realloc, as it may move the existing buffer,
       * breaking existing pointers the caller may still be using.  Just
       * malloc a new copy and memcpy it like the normal BO path.
       *
       * Use bo->size rather than new_size because the bufmgr may have
       * rounded up the size, and we want the shadow size to match.
       */
      grow->map = malloc(new_bo->size);
   } else {
      grow->map = brw_bo_map(brw, new_bo, MAP_READ | MAP_WRITE);
   }

   /* Try to put the new BO at the same GTT offset as the old BO (which
    * we're throwing away, so it doesn't need to be there).
    *
    * This guarantees that our relocations continue to work: values we've
    * already written into the buffer, values we're going to write into the
    * buffer, and the validation/relocation lists all will match.
    *
    * Also preserve kflags for EXEC_OBJECT_CAPTURE.
    */
   new_bo->gtt_offset = bo->gtt_offset;
   new_bo->index = bo->index;
   new_bo->kflags = bo->kflags;

   /* Batch/state buffers are per-context, and if we've run out of space,
    * we must have actually used them before, so...they will be in the list.
    */
   assert(bo->index < batch->exec_count);
   assert(batch->exec_bos[bo->index] == bo);

   /* Update the validation list to use the new BO. */
   batch->validation_list[bo->index].handle = new_bo->gem_handle;

   if (!batch->use_batch_first) {
      /* We're not using I915_EXEC_HANDLE_LUT, which means we need to go
       * update the relocation list entries to point at the new BO as well.
       * (With newer kernels, the "handle" is an offset into the validation
       * list, which remains unchanged, so we can skip this.)
       */
      replace_bo_in_reloc_list(&batch->batch_relocs,
                               bo->gem_handle, new_bo->gem_handle);
      replace_bo_in_reloc_list(&batch->state_relocs,
                               bo->gem_handle, new_bo->gem_handle);
   }

   /* Exchange the two BOs...without breaking pointers to the old BO.
    *
    * Consider this scenario:
    *
    * 1. Somebody calls brw_state_batch() to get a region of memory, and
    *    and then creates a brw_address pointing to brw->batch.state.bo.
    * 2. They then call brw_state_batch() a second time, which happens to
    *    grow and replace the state buffer.  They then try to emit a
    *    relocation to their first section of memory.
    *
    * If we replace the brw->batch.state.bo pointer at step 2, we would
    * break the address created in step 1.  They'd have a pointer to the
    * old destroyed BO.  Emitting a relocation would add this dead BO to
    * the validation list...causing /both/ statebuffers to be in the list,
    * and all kinds of disasters.
    *
    * This is not a contrived case - BLORP vertex data upload hits this.
    *
    * There are worse scenarios too.  Fences for GL sync objects reference
    * brw->batch.batch.bo.  If we replaced the batch pointer when growing,
    * we'd need to chase down every fence and update it to point to the
    * new BO.  Otherwise, it would refer to a "batch" that never actually
    * gets submitted, and would fail to trigger.
    *
    * To work around both of these issues, we transmutate the buffers in
    * place, making the existing struct brw_bo represent the new buffer,
    * and "new_bo" represent the old BO.  This is highly unusual, but it
    * seems like a necessary evil.
    *
    * We also defer the memcpy of the existing batch's contents.  Callers
    * may make multiple brw_state_batch calls, and retain pointers to the
    * old BO's map.  We'll perform the memcpy in finish_growing_bo() when
    * we finally submit the batch, at which point we've finished uploading
    * state, and nobody should have any old references anymore.
    *
    * To do that, we keep a reference to the old BO in grow->partial_bo,
    * and store the number of bytes to copy in grow->partial_bytes.  We
    * can monkey with the refcounts directly without atomics because these
    * are per-context BOs and they can only be touched by this thread.
    */
   assert(new_bo->refcount == 1);
   new_bo->refcount = bo->refcount;
   bo->refcount = 1;

   assert(list_is_empty(&bo->exports));
   assert(list_is_empty(&new_bo->exports));

   struct brw_bo tmp;
   memcpy(&tmp, bo, sizeof(struct brw_bo));
   memcpy(bo, new_bo, sizeof(struct brw_bo));
   memcpy(new_bo, &tmp, sizeof(struct brw_bo));

   list_inithead(&bo->exports);
   list_inithead(&new_bo->exports);

   grow->partial_bo = new_bo; /* the one reference of the OLD bo */
   grow->partial_bytes = existing_bytes;
}

void
brw_batch_require_space(struct brw_context *brw, GLuint sz)
{
   struct brw_batch *batch = &brw->batch;

   const unsigned batch_used = USED_BATCH(*batch) * 4;
   if (batch_used + sz >= BATCH_SZ && !batch->no_wrap) {
      brw_batch_flush(brw);
   } else if (batch_used + sz >= batch->batch.bo->size) {
      const unsigned new_size =
         MIN2(batch->batch.bo->size + batch->batch.bo->size / 2,
              MAX_BATCH_SIZE);
      grow_buffer(brw, &batch->batch, batch_used, new_size);
      batch->map_next = (void *) batch->batch.map + batch_used;
      assert(batch_used + sz < batch->batch.bo->size);
   }
}

/**
 * Called when starting a new batch buffer.
 */
static void
brw_new_batch(struct brw_context *brw)
{
   /* Unreference any BOs held by the previous batch, and reset counts. */
   for (int i = 0; i < brw->batch.exec_count; i++) {
      brw_bo_unreference(brw->batch.exec_bos[i]);
      brw->batch.exec_bos[i] = NULL;
   }
   brw->batch.batch_relocs.reloc_count = 0;
   brw->batch.state_relocs.reloc_count = 0;
   brw->batch.exec_count = 0;
   brw->batch.aperture_space = 0;

   brw_bo_unreference(brw->batch.state.bo);

   /* Create a new batchbuffer and reset the associated state: */
   brw_batch_reset_and_clear_render_cache(brw);

   /* If the kernel supports hardware contexts, then most hardware state is
    * preserved between batches; we only need to re-emit state that is required
    * to be in every batch.  Otherwise we need to re-emit all the state that
    * would otherwise be stored in the context (which for all intents and
    * purposes means everything).
    */
   if (brw->hw_ctx == 0) {
      brw->ctx.NewDriverState |= BRW_NEW_CONTEXT;
      brw_upload_invariant_state(brw);
   }

   brw->ctx.NewDriverState |= BRW_NEW_BATCH;

   brw->ib.index_size = -1;

   /* We need to periodically reap the shader time results, because rollover
    * happens every few seconds.  We also want to see results every once in a
    * while, because many programs won't cleanly destroy our context, so the
    * end-of-run printout may not happen.
    */
   if (INTEL_DEBUG(DEBUG_SHADER_TIME))
      brw_collect_and_report_shader_time(brw);

   brw_batch_maybe_noop(brw);
}

/**
 * Called from brw_batch_flush before emitting MI_BATCHBUFFER_END and
 * sending it off.
 *
 * This function can emit state (say, to preserve registers that aren't saved
 * between batches).
 */
static void
brw_finish_batch(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   brw->batch.no_wrap = true;

   /* Capture the closing pipeline statistics register values necessary to
    * support query objects (in the non-hardware context world).
    */
   brw_emit_query_end(brw);

   /* Work around L3 state leaks into contexts set MI_RESTORE_INHIBIT which
    * assume that the L3 cache is configured according to the hardware
    * defaults.  On Kernel 4.16+, we no longer need to do this.
    */
   if (devinfo->ver >= 7 &&
       !(brw->screen->kernel_features & KERNEL_ALLOWS_CONTEXT_ISOLATION))
      gfx7_restore_default_l3_config(brw);

   if (devinfo->is_haswell) {
      /* From the Haswell PRM, Volume 2b, Command Reference: Instructions,
       * 3DSTATE_CC_STATE_POINTERS > "Note":
       *
       * "SW must program 3DSTATE_CC_STATE_POINTERS command at the end of every
       *  3D batch buffer followed by a PIPE_CONTROL with RC flush and CS stall."
       *
       * From the example in the docs, it seems to expect a regular pipe control
       * flush here as well. We may have done it already, but meh.
       *
       * See also WaAvoidRCZCounterRollover.
       */
      brw_emit_mi_flush(brw);
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (2 - 2));
      OUT_BATCH(brw->cc.state_offset | 1);
      ADVANCE_BATCH();
      brw_emit_pipe_control_flush(brw, PIPE_CONTROL_RENDER_TARGET_FLUSH |
                                       PIPE_CONTROL_CS_STALL);
   }

   /* Do not restore push constant packets during context restore. */
   if (devinfo->ver >= 7)
      gfx7_emit_isp_disable(brw);

   /* Emit MI_BATCH_BUFFER_END to finish our batch.  Note that execbuf2
    * requires our batch size to be QWord aligned, so we pad it out if
    * necessary by emitting an extra MI_NOOP after the end.
    */
   brw_batch_require_space(brw, 8);
   *brw->batch.map_next++ = MI_BATCH_BUFFER_END;
   if (USED_BATCH(brw->batch) & 1) {
      *brw->batch.map_next++ = MI_NOOP;
   }

   brw->batch.no_wrap = false;
}

static void
throttle(struct brw_context *brw)
{
   /* Wait for the swapbuffers before the one we just emitted, so we
    * don't get too many swaps outstanding for apps that are GPU-heavy
    * but not CPU-heavy.
    *
    * We're using intelDRI2Flush (called from the loader before
    * swapbuffer) and glFlush (for front buffer rendering) as the
    * indicator that a frame is done and then throttle when we get
    * here as we prepare to render the next frame.  At this point for
    * round trips for swap/copy and getting new buffers are done and
    * we'll spend less time waiting on the GPU.
    *
    * Unfortunately, we don't have a handle to the batch containing
    * the swap, and getting our hands on that doesn't seem worth it,
    * so we just use the first batch we emitted after the last swap.
    */
   if (brw->need_swap_throttle && brw->throttle_batch[0]) {
      if (brw->throttle_batch[1]) {
         if (!brw->disable_throttling) {
            brw_bo_wait_rendering(brw->throttle_batch[1]);
         }
         brw_bo_unreference(brw->throttle_batch[1]);
      }
      brw->throttle_batch[1] = brw->throttle_batch[0];
      brw->throttle_batch[0] = NULL;
      brw->need_swap_throttle = false;
      /* Throttling here is more precise than the throttle ioctl, so skip it */
      brw->need_flush_throttle = false;
   }

   if (brw->need_flush_throttle) {
      drmCommandNone(brw->screen->fd, DRM_I915_GEM_THROTTLE);
      brw->need_flush_throttle = false;
   }
}

static int
execbuffer(int fd,
           struct brw_batch *batch,
           uint32_t ctx_id,
           int used,
           int in_fence,
           int *out_fence,
           int flags)
{
   struct drm_i915_gem_execbuffer2 execbuf = {
      .buffers_ptr = (uintptr_t) batch->validation_list,
      .buffer_count = batch->exec_count,
      .batch_start_offset = 0,
      .batch_len = used,
      .flags = flags,
      .rsvd1 = ctx_id, /* rsvd1 is actually the context ID */
   };

   unsigned long cmd = DRM_IOCTL_I915_GEM_EXECBUFFER2;

   if (in_fence != -1) {
      execbuf.rsvd2 = in_fence;
      execbuf.flags |= I915_EXEC_FENCE_IN;
   }

   if (out_fence != NULL) {
      cmd = DRM_IOCTL_I915_GEM_EXECBUFFER2_WR;
      *out_fence = -1;
      execbuf.flags |= I915_EXEC_FENCE_OUT;
   }

   if (num_fences(batch)) {
      execbuf.flags |= I915_EXEC_FENCE_ARRAY;
      execbuf.num_cliprects = num_fences(batch);
      execbuf.cliprects_ptr =
         (uintptr_t)util_dynarray_begin(&batch->exec_fences);
   }


   int ret = drmIoctl(fd, cmd, &execbuf);
   if (ret != 0)
      ret = -errno;

   for (int i = 0; i < batch->exec_count; i++) {
      struct brw_bo *bo = batch->exec_bos[i];

      bo->idle = false;
      bo->index = -1;

      /* Update brw_bo::gtt_offset */
      if (batch->validation_list[i].offset != bo->gtt_offset) {
         DBG("BO %d migrated: 0x%" PRIx64 " -> 0x%" PRIx64 "\n",
             bo->gem_handle, bo->gtt_offset,
             (uint64_t)batch->validation_list[i].offset);
         assert(!(bo->kflags & EXEC_OBJECT_PINNED));
         bo->gtt_offset = batch->validation_list[i].offset;
      }
   }

   if (ret == 0 && out_fence != NULL)
      *out_fence = execbuf.rsvd2 >> 32;

   return ret;
}

static int
submit_batch(struct brw_context *brw, int in_fence_fd, int *out_fence_fd)
{
   struct brw_batch *batch = &brw->batch;
   int ret = 0;

   if (batch->use_shadow_copy) {
      void *bo_map = brw_bo_map(brw, batch->batch.bo, MAP_WRITE);
      memcpy(bo_map, batch->batch.map, 4 * USED_BATCH(*batch));

      bo_map = brw_bo_map(brw, batch->state.bo, MAP_WRITE);
      memcpy(bo_map, batch->state.map, batch->state_used);
   }

   brw_bo_unmap(batch->batch.bo);
   brw_bo_unmap(batch->state.bo);

   if (!brw->screen->devinfo.no_hw) {
      /* The requirement for using I915_EXEC_NO_RELOC are:
       *
       *   The addresses written in the objects must match the corresponding
       *   reloc.gtt_offset which in turn must match the corresponding
       *   execobject.offset.
       *
       *   Any render targets written to in the batch must be flagged with
       *   EXEC_OBJECT_WRITE.
       *
       *   To avoid stalling, execobject.offset should match the current
       *   address of that object within the active context.
       */
      int flags = I915_EXEC_NO_RELOC | I915_EXEC_RENDER;

      if (batch->needs_sol_reset)
         flags |= I915_EXEC_GEN7_SOL_RESET;

      /* Set statebuffer relocations */
      const unsigned state_index = batch->state.bo->index;
      if (state_index < batch->exec_count &&
          batch->exec_bos[state_index] == batch->state.bo) {
         struct drm_i915_gem_exec_object2 *entry =
            &batch->validation_list[state_index];
         assert(entry->handle == batch->state.bo->gem_handle);
         entry->relocation_count = batch->state_relocs.reloc_count;
         entry->relocs_ptr = (uintptr_t) batch->state_relocs.relocs;
      }

      /* Set batchbuffer relocations */
      struct drm_i915_gem_exec_object2 *entry = &batch->validation_list[0];
      assert(entry->handle == batch->batch.bo->gem_handle);
      entry->relocation_count = batch->batch_relocs.reloc_count;
      entry->relocs_ptr = (uintptr_t) batch->batch_relocs.relocs;

      if (batch->use_batch_first) {
         flags |= I915_EXEC_BATCH_FIRST | I915_EXEC_HANDLE_LUT;
      } else {
         /* Move the batch to the end of the validation list */
         struct drm_i915_gem_exec_object2 tmp;
         struct brw_bo *tmp_bo;
         const unsigned index = batch->exec_count - 1;

         tmp = *entry;
         *entry = batch->validation_list[index];
         batch->validation_list[index] = tmp;

         tmp_bo = batch->exec_bos[0];
         batch->exec_bos[0] = batch->exec_bos[index];
         batch->exec_bos[index] = tmp_bo;
      }

      ret = execbuffer(brw->screen->fd, batch, brw->hw_ctx,
                       4 * USED_BATCH(*batch),
                       in_fence_fd, out_fence_fd, flags);

      throttle(brw);
   }

   if (INTEL_DEBUG(DEBUG_BATCH)) {
      intel_print_batch(&batch->decoder, batch->batch.map,
                        4 * USED_BATCH(*batch),
                        batch->batch.bo->gtt_offset, false);
   }

   if (brw->ctx.Const.ResetStrategy == GL_LOSE_CONTEXT_ON_RESET_ARB)
      brw_check_for_reset(brw);

   if (ret != 0) {
      fprintf(stderr, "i965: Failed to submit batchbuffer: %s\n",
              strerror(-ret));
      abort();
   }

   return ret;
}

/**
 * The in_fence_fd is ignored if -1.  Otherwise this function takes ownership
 * of the fd.
 *
 * The out_fence_fd is ignored if NULL. Otherwise, the caller takes ownership
 * of the returned fd.
 */
int
_brw_batch_flush_fence(struct brw_context *brw,
                               int in_fence_fd, int *out_fence_fd,
                               const char *file, int line)
{
   int ret;

   if (USED_BATCH(brw->batch) == 0 && !brw->batch.contains_fence_signal)
      return 0;

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!brw->batch.no_wrap);

   brw_finish_batch(brw);
   brw_upload_finish(&brw->upload);

   finish_growing_bos(&brw->batch.batch);
   finish_growing_bos(&brw->batch.state);

   if (brw->throttle_batch[0] == NULL) {
      brw->throttle_batch[0] = brw->batch.batch.bo;
      brw_bo_reference(brw->throttle_batch[0]);
   }

   if (INTEL_DEBUG(DEBUG_BATCH | DEBUG_SUBMIT)) {
      int bytes_for_commands = 4 * USED_BATCH(brw->batch);
      int bytes_for_state = brw->batch.state_used;
      fprintf(stderr, "%19s:%-3d: Batchbuffer flush with %5db (%0.1f%%) (pkt),"
              " %5db (%0.1f%%) (state), %4d BOs (%0.1fMb aperture),"
              " %4d batch relocs, %4d state relocs\n", file, line,
              bytes_for_commands, 100.0f * bytes_for_commands / BATCH_SZ,
              bytes_for_state, 100.0f * bytes_for_state / STATE_SZ,
              brw->batch.exec_count,
              (float) (brw->batch.aperture_space / (1024 * 1024)),
              brw->batch.batch_relocs.reloc_count,
              brw->batch.state_relocs.reloc_count);

      dump_validation_list(&brw->batch);
   }

   ret = submit_batch(brw, in_fence_fd, out_fence_fd);

   if (INTEL_DEBUG(DEBUG_SYNC)) {
      fprintf(stderr, "waiting for idle\n");
      brw_bo_wait_rendering(brw->batch.batch.bo);
   }

   /* Start a new batch buffer. */
   brw_new_batch(brw);

   return ret;
}

void
brw_batch_maybe_noop(struct brw_context *brw)
{
   if (!brw->frontend_noop || USED_BATCH(brw->batch) != 0)
      return;

   BEGIN_BATCH(1);
   OUT_BATCH(MI_BATCH_BUFFER_END);
   ADVANCE_BATCH();
}

bool
brw_batch_references(struct brw_batch *batch, struct brw_bo *bo)
{
   unsigned index = READ_ONCE(bo->index);
   if (index < batch->exec_count && batch->exec_bos[index] == bo)
      return true;

   for (int i = 0; i < batch->exec_count; i++) {
      if (batch->exec_bos[i] == bo)
         return true;
   }
   return false;
}

/*  This is the only way buffers get added to the validate list.
 */
static uint64_t
emit_reloc(struct brw_batch *batch,
           struct brw_reloc_list *rlist, uint32_t offset,
           struct brw_bo *target, int32_t target_offset,
           unsigned int reloc_flags)
{
   assert(target != NULL);

   if (target->kflags & EXEC_OBJECT_PINNED) {
      brw_use_pinned_bo(batch, target, reloc_flags & RELOC_WRITE);
      return intel_canonical_address(target->gtt_offset + target_offset);
   }

   unsigned int index = add_exec_bo(batch, target);
   struct drm_i915_gem_exec_object2 *entry = &batch->validation_list[index];

   if (rlist->reloc_count == rlist->reloc_array_size) {
      rlist->reloc_array_size *= 2;
      rlist->relocs = realloc(rlist->relocs,
                              rlist->reloc_array_size *
                              sizeof(struct drm_i915_gem_relocation_entry));
   }

   if (reloc_flags & RELOC_32BIT) {
      /* Restrict this buffer to the low 32 bits of the address space.
       *
       * Altering the validation list flags restricts it for this batch,
       * but we also alter the BO's kflags to restrict it permanently
       * (until the BO is destroyed and put back in the cache).  Buffers
       * may stay bound across batches, and we want keep it constrained.
       */
      target->kflags &= ~EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
      entry->flags &= ~EXEC_OBJECT_SUPPORTS_48B_ADDRESS;

      /* RELOC_32BIT is not an EXEC_OBJECT_* flag, so get rid of it. */
      reloc_flags &= ~RELOC_32BIT;
   }

   if (reloc_flags)
      entry->flags |= reloc_flags & batch->valid_reloc_flags;

   rlist->relocs[rlist->reloc_count++] =
      (struct drm_i915_gem_relocation_entry) {
         .offset = offset,
         .delta = target_offset,
         .target_handle = batch->use_batch_first ? index : target->gem_handle,
         .presumed_offset = entry->offset,
      };

   /* Using the old buffer offset, write in what the right data would be, in
    * case the buffer doesn't move and we can short-circuit the relocation
    * processing in the kernel
    */
   return entry->offset + target_offset;
}

void
brw_use_pinned_bo(struct brw_batch *batch, struct brw_bo *bo,
                  unsigned writable_flag)
{
   assert(bo->kflags & EXEC_OBJECT_PINNED);
   assert((writable_flag & ~EXEC_OBJECT_WRITE) == 0);

   unsigned int index = add_exec_bo(batch, bo);
   struct drm_i915_gem_exec_object2 *entry = &batch->validation_list[index];
   assert(entry->offset == bo->gtt_offset);

   if (writable_flag)
      entry->flags |= EXEC_OBJECT_WRITE;
}

uint64_t
brw_batch_reloc(struct brw_batch *batch, uint32_t batch_offset,
                struct brw_bo *target, uint32_t target_offset,
                unsigned int reloc_flags)
{
   assert(batch_offset <= batch->batch.bo->size - sizeof(uint32_t));

   return emit_reloc(batch, &batch->batch_relocs, batch_offset,
                     target, target_offset, reloc_flags);
}

uint64_t
brw_state_reloc(struct brw_batch *batch, uint32_t state_offset,
                struct brw_bo *target, uint32_t target_offset,
                unsigned int reloc_flags)
{
   assert(state_offset <= batch->state.bo->size - sizeof(uint32_t));

   return emit_reloc(batch, &batch->state_relocs, state_offset,
                     target, target_offset, reloc_flags);
}

/**
 * Reserve some space in the statebuffer, or flush.
 *
 * This is used to estimate when we're near the end of the batch,
 * so we can flush early.
 */
void
brw_require_statebuffer_space(struct brw_context *brw, int size)
{
   if (brw->batch.state_used + size >= STATE_SZ)
      brw_batch_flush(brw);
}

/**
 * Allocates a block of space in the batchbuffer for indirect state.
 */
void *
brw_state_batch(struct brw_context *brw,
                int size,
                int alignment,
                uint32_t *out_offset)
{
   struct brw_batch *batch = &brw->batch;

   assert(size < batch->state.bo->size);

   uint32_t offset = ALIGN(batch->state_used, alignment);

   if (offset + size >= STATE_SZ && !batch->no_wrap) {
      brw_batch_flush(brw);
      offset = ALIGN(batch->state_used, alignment);
   } else if (offset + size >= batch->state.bo->size) {
      const unsigned new_size =
         MIN2(batch->state.bo->size + batch->state.bo->size / 2,
              MAX_STATE_SIZE);
      grow_buffer(brw, &batch->state, batch->state_used, new_size);
      assert(offset + size < batch->state.bo->size);
   }

   if (INTEL_DEBUG(DEBUG_BATCH)) {
      _mesa_hash_table_u64_insert(batch->state_batch_sizes,
                                  offset, (void *) (uintptr_t) size);
   }

   batch->state_used = offset + size;

   *out_offset = offset;
   return batch->state.map + (offset >> 2);
}

void
brw_batch_data(struct brw_context *brw,
                       const void *data, GLuint bytes)
{
   assert((bytes & 3) == 0);
   brw_batch_require_space(brw, bytes);
   memcpy(brw->batch.map_next, data, bytes);
   brw->batch.map_next += bytes >> 2;
}

static void
load_sized_register_mem(struct brw_context *brw,
                        uint32_t reg,
                        struct brw_bo *bo,
                        uint32_t offset,
                        int size)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   int i;

   /* MI_LOAD_REGISTER_MEM only exists on Gfx7+. */
   assert(devinfo->ver >= 7);

   if (devinfo->ver >= 8) {
      BEGIN_BATCH(4 * size);
      for (i = 0; i < size; i++) {
         OUT_BATCH(GFX7_MI_LOAD_REGISTER_MEM | (4 - 2));
         OUT_BATCH(reg + i * 4);
         OUT_RELOC64(bo, 0, offset + i * 4);
      }
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3 * size);
      for (i = 0; i < size; i++) {
         OUT_BATCH(GFX7_MI_LOAD_REGISTER_MEM | (3 - 2));
         OUT_BATCH(reg + i * 4);
         OUT_RELOC(bo, 0, offset + i * 4);
      }
      ADVANCE_BATCH();
   }
}

void
brw_load_register_mem(struct brw_context *brw,
                      uint32_t reg,
                      struct brw_bo *bo,
                      uint32_t offset)
{
   load_sized_register_mem(brw, reg, bo, offset, 1);
}

void
brw_load_register_mem64(struct brw_context *brw,
                        uint32_t reg,
                        struct brw_bo *bo,
                        uint32_t offset)
{
   load_sized_register_mem(brw, reg, bo, offset, 2);
}

/*
 * Write an arbitrary 32-bit register to a buffer via MI_STORE_REGISTER_MEM.
 */
void
brw_store_register_mem32(struct brw_context *brw,
                         struct brw_bo *bo, uint32_t reg, uint32_t offset)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver >= 6);

   if (devinfo->ver >= 8) {
      BEGIN_BATCH(4);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (4 - 2));
      OUT_BATCH(reg);
      OUT_RELOC64(bo, RELOC_WRITE, offset);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
      OUT_BATCH(reg);
      OUT_RELOC(bo, RELOC_WRITE | RELOC_NEEDS_GGTT, offset);
      ADVANCE_BATCH();
   }
}

/*
 * Write an arbitrary 64-bit register to a buffer via MI_STORE_REGISTER_MEM.
 */
void
brw_store_register_mem64(struct brw_context *brw,
                         struct brw_bo *bo, uint32_t reg, uint32_t offset)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver >= 6);

   /* MI_STORE_REGISTER_MEM only stores a single 32-bit value, so to
    * read a full 64-bit register, we need to do two of them.
    */
   if (devinfo->ver >= 8) {
      BEGIN_BATCH(8);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (4 - 2));
      OUT_BATCH(reg);
      OUT_RELOC64(bo, RELOC_WRITE, offset);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (4 - 2));
      OUT_BATCH(reg + sizeof(uint32_t));
      OUT_RELOC64(bo, RELOC_WRITE, offset + sizeof(uint32_t));
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(6);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
      OUT_BATCH(reg);
      OUT_RELOC(bo, RELOC_WRITE | RELOC_NEEDS_GGTT, offset);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
      OUT_BATCH(reg + sizeof(uint32_t));
      OUT_RELOC(bo, RELOC_WRITE | RELOC_NEEDS_GGTT, offset + sizeof(uint32_t));
      ADVANCE_BATCH();
   }
}

/*
 * Write a 32-bit register using immediate data.
 */
void
brw_load_register_imm32(struct brw_context *brw, uint32_t reg, uint32_t imm)
{
   assert(brw->screen->devinfo.ver >= 6);

   BEGIN_BATCH(3);
   OUT_BATCH(MI_LOAD_REGISTER_IMM | (3 - 2));
   OUT_BATCH(reg);
   OUT_BATCH(imm);
   ADVANCE_BATCH();
}

/*
 * Write a 64-bit register using immediate data.
 */
void
brw_load_register_imm64(struct brw_context *brw, uint32_t reg, uint64_t imm)
{
   assert(brw->screen->devinfo.ver >= 6);

   BEGIN_BATCH(5);
   OUT_BATCH(MI_LOAD_REGISTER_IMM | (5 - 2));
   OUT_BATCH(reg);
   OUT_BATCH(imm & 0xffffffff);
   OUT_BATCH(reg + 4);
   OUT_BATCH(imm >> 32);
   ADVANCE_BATCH();
}

/*
 * Copies a 32-bit register.
 */
void
brw_load_register_reg(struct brw_context *brw, uint32_t dest, uint32_t src)
{
   assert(brw->screen->devinfo.verx10 >= 75);

   BEGIN_BATCH(3);
   OUT_BATCH(MI_LOAD_REGISTER_REG | (3 - 2));
   OUT_BATCH(src);
   OUT_BATCH(dest);
   ADVANCE_BATCH();
}

/*
 * Copies a 64-bit register.
 */
void
brw_load_register_reg64(struct brw_context *brw, uint32_t dest, uint32_t src)
{
   assert(brw->screen->devinfo.verx10 >= 75);

   BEGIN_BATCH(6);
   OUT_BATCH(MI_LOAD_REGISTER_REG | (3 - 2));
   OUT_BATCH(src);
   OUT_BATCH(dest);
   OUT_BATCH(MI_LOAD_REGISTER_REG | (3 - 2));
   OUT_BATCH(src + sizeof(uint32_t));
   OUT_BATCH(dest + sizeof(uint32_t));
   ADVANCE_BATCH();
}

/*
 * Write 32-bits of immediate data to a GPU memory buffer.
 */
void
brw_store_data_imm32(struct brw_context *brw, struct brw_bo *bo,
                     uint32_t offset, uint32_t imm)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver >= 6);

   BEGIN_BATCH(4);
   OUT_BATCH(MI_STORE_DATA_IMM | (4 - 2));
   if (devinfo->ver >= 8)
      OUT_RELOC64(bo, RELOC_WRITE, offset);
   else {
      OUT_BATCH(0); /* MBZ */
      OUT_RELOC(bo, RELOC_WRITE, offset);
   }
   OUT_BATCH(imm);
   ADVANCE_BATCH();
}

/*
 * Write 64-bits of immediate data to a GPU memory buffer.
 */
void
brw_store_data_imm64(struct brw_context *brw, struct brw_bo *bo,
                     uint32_t offset, uint64_t imm)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver >= 6);

   BEGIN_BATCH(5);
   OUT_BATCH(MI_STORE_DATA_IMM | (5 - 2));
   if (devinfo->ver >= 8)
      OUT_RELOC64(bo, RELOC_WRITE, offset);
   else {
      OUT_BATCH(0); /* MBZ */
      OUT_RELOC(bo, RELOC_WRITE, offset);
   }
   OUT_BATCH(imm & 0xffffffffu);
   OUT_BATCH(imm >> 32);
   ADVANCE_BATCH();
}
