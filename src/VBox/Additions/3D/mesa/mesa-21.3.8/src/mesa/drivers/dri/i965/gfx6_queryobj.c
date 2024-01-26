/*
 * Copyright © 2008 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *    Kenneth Graunke <kenneth@whitecape.org>
 */

/** @file gfx6_queryobj.c
 *
 * Support for query objects (GL_ARB_occlusion_query, GL_ARB_timer_query,
 * GL_EXT_transform_feedback, and friends) on platforms that support
 * hardware contexts (Gfx6+).
 */
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "perf/intel_perf_regs.h"
#include "brw_batch.h"
#include "brw_buffer_objects.h"

static inline void
set_query_availability(struct brw_context *brw, struct brw_query_object *query,
                       bool available)
{
   /* For platforms that support ARB_query_buffer_object, we write the
    * query availability for "pipelined" queries.
    *
    * Most counter snapshots are written by the command streamer, by
    * doing a CS stall and then MI_STORE_REGISTER_MEM.  For these
    * counters, the CS stall guarantees that the results will be
    * available when subsequent CS commands run.  So we don't need to
    * do any additional tracking.
    *
    * Other counters (occlusion queries and timestamp) are written by
    * PIPE_CONTROL, without a CS stall.  This means that we can't be
    * sure whether the writes have landed yet or not.  Performing a
    * PIPE_CONTROL with an immediate write will synchronize with
    * those earlier writes, so we write 1 when the value has landed.
    */
   if (brw->ctx.Extensions.ARB_query_buffer_object &&
       brw_is_query_pipelined(query)) {
      unsigned flags = PIPE_CONTROL_WRITE_IMMEDIATE;

      if (available) {
         /* Order available *after* the query results. */
         flags |= PIPE_CONTROL_FLUSH_ENABLE;
      } else {
         /* Make it unavailable *before* any pipelined reads. */
         flags |= PIPE_CONTROL_CS_STALL;
      }

      brw_emit_pipe_control_write(brw, flags,
                                  query->bo, 2 * sizeof(uint64_t),
                                  available);
   }
}

static void
write_primitives_generated(struct brw_context *brw,
                           struct brw_bo *query_bo, int stream, int idx)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   brw_emit_mi_flush(brw);

   if (devinfo->ver >= 7 && stream > 0) {
      brw_store_register_mem64(brw, query_bo,
                               GFX7_SO_PRIM_STORAGE_NEEDED(stream),
                               idx * sizeof(uint64_t));
   } else {
      brw_store_register_mem64(brw, query_bo, CL_INVOCATION_COUNT,
                               idx * sizeof(uint64_t));
   }
}

static void
write_xfb_primitives_written(struct brw_context *brw,
                             struct brw_bo *bo, int stream, int idx)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   brw_emit_mi_flush(brw);

   if (devinfo->ver >= 7) {
      brw_store_register_mem64(brw, bo, GFX7_SO_NUM_PRIMS_WRITTEN(stream),
                               idx * sizeof(uint64_t));
   } else {
      brw_store_register_mem64(brw, bo, GFX6_SO_NUM_PRIMS_WRITTEN,
                               idx * sizeof(uint64_t));
   }
}

static void
write_xfb_overflow_streams(struct gl_context *ctx,
                           struct brw_bo *bo, int stream, int count,
                           int idx)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   brw_emit_mi_flush(brw);

   for (int i = 0; i < count; i++) {
      int w_idx = 4 * i + idx;
      int g_idx = 4 * i + idx + 2;

      if (devinfo->ver >= 7) {
         brw_store_register_mem64(brw, bo,
                                  GFX7_SO_NUM_PRIMS_WRITTEN(stream + i),
                                  g_idx * sizeof(uint64_t));
         brw_store_register_mem64(brw, bo,
                                  GFX7_SO_PRIM_STORAGE_NEEDED(stream + i),
                                  w_idx * sizeof(uint64_t));
      } else {
         brw_store_register_mem64(brw, bo,
                                  GFX6_SO_NUM_PRIMS_WRITTEN,
                                  g_idx * sizeof(uint64_t));
         brw_store_register_mem64(brw, bo,
                                  GFX6_SO_PRIM_STORAGE_NEEDED,
                                  w_idx * sizeof(uint64_t));
      }
   }
}

static bool
check_xfb_overflow_streams(uint64_t *results, int count)
{
   bool overflow = false;

   for (int i = 0; i < count; i++) {
      uint64_t *result_i = &results[4 * i];

      if ((result_i[3] - result_i[2]) != (result_i[1] - result_i[0])) {
         overflow = true;
         break;
      }
   }

   return overflow;
}

static inline int
pipeline_target_to_index(int target)
{
   if (target == GL_GEOMETRY_SHADER_INVOCATIONS)
      return MAX_PIPELINE_STATISTICS - 1;
   else
      return target - GL_VERTICES_SUBMITTED_ARB;
}

static void
emit_pipeline_stat(struct brw_context *brw, struct brw_bo *bo,
                   int stream, int target, int idx)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* One source of confusion is the tessellation shader statistics. The
    * hardware has no statistics specific to the TE unit. Ideally we could have
    * the HS primitives for TESS_CONTROL_SHADER_PATCHES_ARB, and the DS
    * invocations as the register for TESS_CONTROL_SHADER_PATCHES_ARB.
    * Unfortunately we don't have HS primitives, we only have HS invocations.
    */

   /* Everything except GEOMETRY_SHADER_INVOCATIONS can be kept in a simple
    * lookup table
    */
   static const uint32_t target_to_register[] = {
      IA_VERTICES_COUNT,   /* VERTICES_SUBMITTED */
      IA_PRIMITIVES_COUNT, /* PRIMITIVES_SUBMITTED */
      VS_INVOCATION_COUNT, /* VERTEX_SHADER_INVOCATIONS */
      HS_INVOCATION_COUNT, /* TESS_CONTROL_SHADER_PATCHES */
      DS_INVOCATION_COUNT, /* TESS_EVALUATION_SHADER_INVOCATIONS */
      GS_PRIMITIVES_COUNT, /* GEOMETRY_SHADER_PRIMITIVES_EMITTED */
      PS_INVOCATION_COUNT, /* FRAGMENT_SHADER_INVOCATIONS */
      CS_INVOCATION_COUNT, /* COMPUTE_SHADER_INVOCATIONS */
      CL_INVOCATION_COUNT, /* CLIPPING_INPUT_PRIMITIVES */
      CL_PRIMITIVES_COUNT, /* CLIPPING_OUTPUT_PRIMITIVES */
      GS_INVOCATION_COUNT /* This one is special... */
   };
   STATIC_ASSERT(ARRAY_SIZE(target_to_register) == MAX_PIPELINE_STATISTICS);
   uint32_t reg = target_to_register[pipeline_target_to_index(target)];
   /* Gfx6 GS code counts full primitives, that is, it won't count individual
    * triangles in a triangle strip. Use CL_INVOCATION_COUNT for that.
    */
   if (devinfo->ver == 6 && target == GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB)
      reg = CL_INVOCATION_COUNT;
   assert(reg != 0);

   /* Emit a flush to make sure various parts of the pipeline are complete and
    * we get an accurate value
    */
   brw_emit_mi_flush(brw);

   brw_store_register_mem64(brw, bo, reg, idx * sizeof(uint64_t));
}


/**
 * Wait on the query object's BO and calculate the final result.
 */
static void
gfx6_queryobj_get_results(struct gl_context *ctx,
                          struct brw_query_object *query)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (query->bo == NULL)
      return;

   uint64_t *results = brw_bo_map(brw, query->bo, MAP_READ);
   switch (query->Base.Target) {
   case GL_TIME_ELAPSED:
      /* The query BO contains the starting and ending timestamps.
       * Subtract the two and convert to nanoseconds.
       */
      query->Base.Result = brw_raw_timestamp_delta(brw, results[0], results[1]);
      query->Base.Result = intel_device_info_timebase_scale(devinfo, query->Base.Result);
      break;

   case GL_TIMESTAMP:
      /* The query BO contains a single timestamp value in results[0]. */
      query->Base.Result = intel_device_info_timebase_scale(devinfo, results[0]);

      /* Ensure the scaled timestamp overflows according to
       * GL_QUERY_COUNTER_BITS
       */
      query->Base.Result &= (1ull << ctx->Const.QueryCounterBits.Timestamp) - 1;
      break;

   case GL_SAMPLES_PASSED_ARB:
      /* We need to use += rather than = here since some BLT-based operations
       * may have added additional samples to our occlusion query value.
       */
      query->Base.Result += results[1] - results[0];
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      if (results[0] != results[1])
         query->Base.Result = true;
      break;

   case GL_PRIMITIVES_GENERATED:
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
   case GL_VERTICES_SUBMITTED_ARB:
   case GL_PRIMITIVES_SUBMITTED_ARB:
   case GL_VERTEX_SHADER_INVOCATIONS_ARB:
   case GL_GEOMETRY_SHADER_INVOCATIONS:
   case GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB:
   case GL_CLIPPING_INPUT_PRIMITIVES_ARB:
   case GL_CLIPPING_OUTPUT_PRIMITIVES_ARB:
   case GL_COMPUTE_SHADER_INVOCATIONS_ARB:
   case GL_TESS_CONTROL_SHADER_PATCHES_ARB:
   case GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB:
      query->Base.Result = results[1] - results[0];
      break;

   case GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB:
      query->Base.Result = check_xfb_overflow_streams(results, 1);
      break;

   case GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB:
      query->Base.Result = check_xfb_overflow_streams(results, MAX_VERTEX_STREAMS);
      break;

   case GL_FRAGMENT_SHADER_INVOCATIONS_ARB:
      query->Base.Result = (results[1] - results[0]);
      /* Implement the "WaDividePSInvocationCountBy4:HSW,BDW" workaround:
       * "Invocation counter is 4 times actual.  WA: SW to divide HW reported
       *  PS Invocations value by 4."
       *
       * Prior to Haswell, invocation count was counted by the WM, and it
       * buggily counted invocations in units of subspans (2x2 unit). To get the
       * correct value, the CS multiplied this by 4. With HSW the logic moved,
       * and correctly emitted the number of pixel shader invocations, but,
       * whomever forgot to undo the multiply by 4.
       */
      if (devinfo->ver == 8 || devinfo->is_haswell)
         query->Base.Result /= 4;
      break;

   default:
      unreachable("Unrecognized query target in brw_queryobj_get_results()");
   }
   brw_bo_unmap(query->bo);

   /* Now that we've processed the data stored in the query's buffer object,
    * we can release it.
    */
   brw_bo_unreference(query->bo);
   query->bo = NULL;

   query->Base.Ready = true;
}

/**
 * Driver hook for glBeginQuery().
 *
 * Initializes driver structures and emits any GPU commands required to begin
 * recording data for the query.
 */
static void
gfx6_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Since we're starting a new query, we need to throw away old results. */
   brw_bo_unreference(query->bo);
   query->bo =
      brw_bo_alloc(brw->bufmgr, "query results", 4096, BRW_MEMZONE_OTHER);

   /* For ARB_query_buffer_object: The result is not available */
   set_query_availability(brw, query, false);

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED:
      /* For timestamp queries, we record the starting time right away so that
       * we measure the full time between BeginQuery and EndQuery.  There's
       * some debate about whether this is the right thing to do.  Our decision
       * is based on the following text from the ARB_timer_query extension:
       *
       * "(5) Should the extension measure total time elapsed between the full
       *      completion of the BeginQuery and EndQuery commands, or just time
       *      spent in the graphics library?
       *
       *  RESOLVED:  This extension will measure the total time elapsed
       *  between the full completion of these commands.  Future extensions
       *  may implement a query to determine time elapsed at different stages
       *  of the graphics pipeline."
       *
       * We write a starting timestamp now (at index 0).  At EndQuery() time,
       * we'll write a second timestamp (at index 1), and subtract the two to
       * obtain the time elapsed.  Notably, this includes time elapsed while
       * the system was doing other work, such as running other applications.
       */
      brw_write_timestamp(brw, query->bo, 0);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      brw_write_depth_count(brw, query->bo, 0);
      break;

   case GL_PRIMITIVES_GENERATED:
      write_primitives_generated(brw, query->bo, query->Base.Stream, 0);
      if (query->Base.Stream == 0)
         ctx->NewDriverState |= BRW_NEW_RASTERIZER_DISCARD;
      break;

   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      write_xfb_primitives_written(brw, query->bo, query->Base.Stream, 0);
      break;

   case GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB:
      write_xfb_overflow_streams(ctx, query->bo, query->Base.Stream, 1, 0);
      break;

   case GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB:
      write_xfb_overflow_streams(ctx, query->bo, 0, MAX_VERTEX_STREAMS, 0);
      break;

   case GL_VERTICES_SUBMITTED_ARB:
   case GL_PRIMITIVES_SUBMITTED_ARB:
   case GL_VERTEX_SHADER_INVOCATIONS_ARB:
   case GL_GEOMETRY_SHADER_INVOCATIONS:
   case GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB:
   case GL_FRAGMENT_SHADER_INVOCATIONS_ARB:
   case GL_CLIPPING_INPUT_PRIMITIVES_ARB:
   case GL_CLIPPING_OUTPUT_PRIMITIVES_ARB:
   case GL_COMPUTE_SHADER_INVOCATIONS_ARB:
   case GL_TESS_CONTROL_SHADER_PATCHES_ARB:
   case GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB:
      emit_pipeline_stat(brw, query->bo, query->Base.Stream, query->Base.Target, 0);
      break;

   default:
      unreachable("Unrecognized query target in brw_begin_query()");
   }
}

/**
 * Driver hook for glEndQuery().
 *
 * Emits GPU commands to record a final query value, ending any data capturing.
 * However, the final result isn't necessarily available until the GPU processes
 * those commands.  brw_queryobj_get_results() processes the captured data to
 * produce the final result.
 */
static void
gfx6_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED:
      brw_write_timestamp(brw, query->bo, 1);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      brw_write_depth_count(brw, query->bo, 1);
      break;

   case GL_PRIMITIVES_GENERATED:
      write_primitives_generated(brw, query->bo, query->Base.Stream, 1);
      if (query->Base.Stream == 0)
         ctx->NewDriverState |= BRW_NEW_RASTERIZER_DISCARD;
      break;

   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      write_xfb_primitives_written(brw, query->bo, query->Base.Stream, 1);
      break;

   case GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB:
      write_xfb_overflow_streams(ctx, query->bo, query->Base.Stream, 1, 1);
      break;

   case GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB:
      write_xfb_overflow_streams(ctx, query->bo, 0, MAX_VERTEX_STREAMS, 1);
      break;

      /* calculate overflow here */
   case GL_VERTICES_SUBMITTED_ARB:
   case GL_PRIMITIVES_SUBMITTED_ARB:
   case GL_VERTEX_SHADER_INVOCATIONS_ARB:
   case GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB:
   case GL_FRAGMENT_SHADER_INVOCATIONS_ARB:
   case GL_COMPUTE_SHADER_INVOCATIONS_ARB:
   case GL_CLIPPING_INPUT_PRIMITIVES_ARB:
   case GL_CLIPPING_OUTPUT_PRIMITIVES_ARB:
   case GL_GEOMETRY_SHADER_INVOCATIONS:
   case GL_TESS_CONTROL_SHADER_PATCHES_ARB:
   case GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB:
      emit_pipeline_stat(brw, query->bo,
                         query->Base.Stream, query->Base.Target, 1);
      break;

   default:
      unreachable("Unrecognized query target in brw_end_query()");
   }

   /* The current batch contains the commands to handle EndQuery(),
    * but they won't actually execute until it is flushed.
    */
   query->flushed = false;

   /* For ARB_query_buffer_object: The result is now available */
   set_query_availability(brw, query, true);
}

/**
 * Flush the batch if it still references the query object BO.
 */
static void
flush_batch_if_needed(struct brw_context *brw, struct brw_query_object *query)
{
   /* If the batch doesn't reference the BO, it must have been flushed
    * (for example, due to being full).  Record that it's been flushed.
    */
   query->flushed = query->flushed ||
                    !brw_batch_references(&brw->batch, query->bo);

   if (!query->flushed)
      brw_batch_flush(brw);
}

/**
 * The WaitQuery() driver hook.
 *
 * Wait for a query result to become available and return it.  This is the
 * backing for glGetQueryObjectiv() with the GL_QUERY_RESULT pname.
 */
static void gfx6_wait_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* If the application has requested the query result, but this batch is
    * still contributing to it, flush it now to finish that work so the
    * result will become available (eventually).
    */
   flush_batch_if_needed(brw, query);

   gfx6_queryobj_get_results(ctx, query);
}

/**
 * The CheckQuery() driver hook.
 *
 * Checks whether a query result is ready yet.  If not, flushes.
 * This is the backing for glGetQueryObjectiv()'s QUERY_RESULT_AVAILABLE pname.
 */
static void gfx6_check_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* If query->bo is NULL, we've already gathered the results - this is a
    * redundant CheckQuery call.  Ignore it.
    */
   if (query->bo == NULL)
      return;

   /* From the GL_ARB_occlusion_query spec:
    *
    *     "Instead of allowing for an infinite loop, performing a
    *      QUERY_RESULT_AVAILABLE_ARB will perform a flush if the result is
    *      not ready yet on the first time it is queried.  This ensures that
    *      the async query will return true in finite time.
    */
   flush_batch_if_needed(brw, query);

   if (!brw_bo_busy(query->bo)) {
      gfx6_queryobj_get_results(ctx, query);
   }
}

static void
gfx6_query_counter(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;
   brw_query_counter(ctx, q);
   set_query_availability(brw, query, true);
}

/* Initialize Gfx6+-specific query object functions. */
void gfx6_init_queryobj_functions(struct dd_function_table *functions)
{
   functions->BeginQuery = gfx6_begin_query;
   functions->EndQuery = gfx6_end_query;
   functions->CheckQuery = gfx6_check_query;
   functions->WaitQuery = gfx6_wait_query;
   functions->QueryCounter = gfx6_query_counter;
}
