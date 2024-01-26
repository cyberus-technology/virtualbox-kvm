/*
 * Copyright © 2014 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <sys/errno.h>

#include "main/condrender.h"
#include "main/mtypes.h"
#include "main/state.h"
#include "brw_context.h"
#include "brw_draw.h"
#include "brw_state.h"
#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "brw_defines.h"


static void
brw_dispatch_compute_common(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   bool fail_next;

   if (!_mesa_check_conditional_render(ctx))
      return;

   if (ctx->NewState)
      _mesa_update_state(ctx);

   brw_validate_textures(brw);

   brw_predraw_resolve_inputs(brw, false, NULL);

   /* Flush the batch if the batch/state buffers are nearly full.  We can
    * grow them if needed, but this is not free, so we'd like to avoid it.
    */
   brw_batch_require_space(brw, 600);
   brw_require_statebuffer_space(brw, 2500);
   brw_batch_save_state(brw);
   fail_next = brw_batch_saved_state_is_empty(brw);

 retry:
   brw->batch.no_wrap = true;
   brw_upload_compute_state(brw);

   brw->vtbl.emit_compute_walker(brw);

   brw->batch.no_wrap = false;

   if (!brw_batch_has_aperture_space(brw, 0)) {
      if (!fail_next) {
         brw_batch_reset_to_saved(brw);
         brw_batch_flush(brw);
         fail_next = true;
         goto retry;
      } else {
         int ret = brw_batch_flush(brw);
         WARN_ONCE(ret == -ENOSPC,
                   "i965: Single compute shader dispatch "
                   "exceeded available aperture space\n");
      }
   }

   /* Now that we know we haven't run out of aperture space, we can safely
    * reset the dirty bits.
    */
   brw_compute_state_finished(brw);

   if (brw->always_flush_batch)
      brw_batch_flush(brw);

   brw_program_cache_check_size(brw);

   /* Note: since compute shaders can't write to framebuffers, there's no need
    * to call brw_postdraw_set_buffers_need_resolve().
    */
}

static void
brw_dispatch_compute(struct gl_context *ctx, const GLuint *num_groups) {
   struct brw_context *brw = brw_context(ctx);

   brw->compute.num_work_groups_bo = NULL;
   brw->compute.num_work_groups = num_groups;
   brw->compute.group_size = NULL;
   ctx->NewDriverState |= BRW_NEW_CS_WORK_GROUPS;

   brw_dispatch_compute_common(ctx);
}

static void
brw_dispatch_compute_indirect(struct gl_context *ctx, GLintptr indirect)
{
   struct brw_context *brw = brw_context(ctx);
   static const GLuint indirect_group_counts[3] = { 0, 0, 0 };
   struct gl_buffer_object *indirect_buffer = ctx->DispatchIndirectBuffer;
   struct brw_bo *bo =
      brw_bufferobj_buffer(brw,
                           brw_buffer_object(indirect_buffer),
                           indirect, 3 * sizeof(GLuint), false);

   brw->compute.num_work_groups_bo = bo;
   brw->compute.num_work_groups_offset = indirect;
   brw->compute.num_work_groups = indirect_group_counts;
   brw->compute.group_size = NULL;
   ctx->NewDriverState |= BRW_NEW_CS_WORK_GROUPS;

   brw_dispatch_compute_common(ctx);
}

static void
brw_dispatch_compute_group_size(struct gl_context *ctx,
                                const GLuint *num_groups,
                                const GLuint *group_size)
{
   struct brw_context *brw = brw_context(ctx);

   brw->compute.num_work_groups_bo = NULL;
   brw->compute.num_work_groups = num_groups;
   brw->compute.group_size = group_size;
   ctx->NewDriverState |= BRW_NEW_CS_WORK_GROUPS;

   brw_dispatch_compute_common(ctx);
}

void
brw_init_compute_functions(struct dd_function_table *functions)
{
   functions->DispatchCompute = brw_dispatch_compute;
   functions->DispatchComputeIndirect = brw_dispatch_compute_indirect;
   functions->DispatchComputeGroupSize = brw_dispatch_compute_group_size;
}
