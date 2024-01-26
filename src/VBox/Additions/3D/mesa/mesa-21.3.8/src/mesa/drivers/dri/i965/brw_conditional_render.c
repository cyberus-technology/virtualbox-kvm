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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Neil Roberts <neil@linux.intel.com>
 */

/** @file brw_conditional_render.c
 *
 * Support for conditional rendering based on query objects
 * (GL_NV_conditional_render, GL_ARB_conditional_render_inverted) on Gfx7+.
 */

#include "main/condrender.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_batch.h"

static void
set_predicate_enable(struct brw_context *brw,
                     bool value)
{
   if (value)
      brw->predicate.state = BRW_PREDICATE_STATE_RENDER;
   else
      brw->predicate.state = BRW_PREDICATE_STATE_DONT_RENDER;
}

static void
set_predicate_for_overflow_query(struct brw_context *brw,
                                 struct brw_query_object *query,
                                 int stream_start, int count)
{
   if (!can_do_mi_math_and_lrr(brw->screen)) {
      brw->predicate.state = BRW_PREDICATE_STATE_STALL_FOR_QUERY;
      return;
   }

   brw->predicate.state = BRW_PREDICATE_STATE_USE_BIT;

   /* Needed to ensure the memory is coherent for the MI_LOAD_REGISTER_MEM
    * command when loading the values into the predicate source registers for
    * conditional rendering.
    */
   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_FLUSH_ENABLE);

   hsw_overflow_result_to_gpr0(brw, query, count);
   brw_load_register_reg64(brw, MI_PREDICATE_SRC0, HSW_CS_GPR(0));
   brw_load_register_imm64(brw, MI_PREDICATE_SRC1, 0ull);
}

static void
set_predicate_for_occlusion_query(struct brw_context *brw,
                                  struct brw_query_object *query)
{
   if (!brw->predicate.supported) {
      brw->predicate.state = BRW_PREDICATE_STATE_STALL_FOR_QUERY;
      return;
   }

   brw->predicate.state = BRW_PREDICATE_STATE_USE_BIT;

   /* Needed to ensure the memory is coherent for the MI_LOAD_REGISTER_MEM
    * command when loading the values into the predicate source registers for
    * conditional rendering.
    */
   brw_emit_pipe_control_flush(brw, PIPE_CONTROL_FLUSH_ENABLE);

   brw_load_register_mem64(brw, MI_PREDICATE_SRC0, query->bo, 0 /* offset */);
   brw_load_register_mem64(brw, MI_PREDICATE_SRC1, query->bo, 8 /* offset */);
}

static void
set_predicate_for_result(struct brw_context *brw,
                         struct brw_query_object *query,
                         bool inverted)
{
   int load_op;

   assert(query->bo != NULL);

   switch (query->Base.Target) {
   case GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB:
      set_predicate_for_overflow_query(brw, query, 0, 1);
      break;
   case GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB:
      set_predicate_for_overflow_query(brw, query, 0, MAX_VERTEX_STREAMS);
      break;
   default:
      set_predicate_for_occlusion_query(brw, query);
   }

   if (brw->predicate.state == BRW_PREDICATE_STATE_USE_BIT) {
      if (inverted)
         load_op = MI_PREDICATE_LOADOP_LOAD;
      else
         load_op = MI_PREDICATE_LOADOP_LOADINV;

      BEGIN_BATCH(1);
      OUT_BATCH(GFX7_MI_PREDICATE |
                load_op |
                MI_PREDICATE_COMBINEOP_SET |
                MI_PREDICATE_COMPAREOP_SRCS_EQUAL);
      ADVANCE_BATCH();
   }
}

static void
brw_begin_conditional_render(struct gl_context *ctx,
                             struct gl_query_object *q,
                             GLenum mode)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *) q;
   bool inverted;

   switch (mode) {
   case GL_QUERY_WAIT:
   case GL_QUERY_NO_WAIT:
   case GL_QUERY_BY_REGION_WAIT:
   case GL_QUERY_BY_REGION_NO_WAIT:
      inverted = false;
      break;
   case GL_QUERY_WAIT_INVERTED:
   case GL_QUERY_NO_WAIT_INVERTED:
   case GL_QUERY_BY_REGION_WAIT_INVERTED:
   case GL_QUERY_BY_REGION_NO_WAIT_INVERTED:
      inverted = true;
      break;
   default:
      unreachable("Unexpected conditional render mode");
   }

   /* If there are already samples from a BLT operation or if the query object
    * is ready then we can avoid looking at the values in the buffer and just
    * decide whether to draw using the CPU without stalling.
    */
   if (query->Base.Result || query->Base.Ready)
      set_predicate_enable(brw, (query->Base.Result != 0) ^ inverted);
   else
      set_predicate_for_result(brw, query, inverted);
}

static void
brw_end_conditional_render(struct gl_context *ctx,
                           struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);

   /* When there is no longer a conditional render in progress it should
    * always render.
    */
   brw->predicate.state = BRW_PREDICATE_STATE_RENDER;
}

void
brw_init_conditional_render_functions(struct dd_function_table *functions)
{
   functions->BeginConditionalRender = brw_begin_conditional_render;
   functions->EndConditionalRender = brw_end_conditional_render;
}

bool
brw_check_conditional_render(struct brw_context *brw)
{
   if (brw->predicate.state == BRW_PREDICATE_STATE_STALL_FOR_QUERY) {
      perf_debug("Conditional rendering is implemented in software and may "
                 "stall.\n");
      return _mesa_check_conditional_render(&brw->ctx);
   }

   return brw->predicate.state != BRW_PREDICATE_STATE_DONT_RENDER;
}
