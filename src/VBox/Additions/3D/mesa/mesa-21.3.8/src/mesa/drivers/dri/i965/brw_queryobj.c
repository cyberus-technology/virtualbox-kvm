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
 *
 */

/** @file brw_queryobj.c
 *
 * Support for query objects (GL_ARB_occlusion_query, GL_ARB_timer_query,
 * GL_EXT_transform_feedback, and friends).
 *
 * The hardware provides a PIPE_CONTROL command that can report the number of
 * fragments that passed the depth test, or the hardware timer.  They are
 * appropriately synced with the stage of the pipeline for our extensions'
 * needs.
 */
#include "main/queryobj.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_batch.h"

/* As best we know currently, the Gen HW timestamps are 36bits across
 * all platforms, which we need to account for when calculating a
 * delta to measure elapsed time.
 *
 * The timestamps read via glGetTimestamp() / brw_get_timestamp() sometimes
 * only have 32bits due to a kernel bug and so in that case we make sure to
 * treat all raw timestamps as 32bits so they overflow consistently and remain
 * comparable. (Note: the timestamps being passed here are not from the kernel
 * so we don't need to be taking the upper 32bits in this buggy kernel case we
 * are just clipping to 32bits here for consistency.)
 */
uint64_t
brw_raw_timestamp_delta(struct brw_context *brw, uint64_t time0, uint64_t time1)
{
   if (brw->screen->hw_has_timestamp == 2) {
      /* Kernel clips timestamps to 32bits in this case, so we also clip
       * PIPE_CONTROL timestamps for consistency.
       */
      return (uint32_t)time1 - (uint32_t)time0;
   } else {
      if (time0 > time1) {
         return (1ULL << 36) + time1 - time0;
      } else {
         return time1 - time0;
      }
   }
}

/**
 * Emit PIPE_CONTROLs to write the current GPU timestamp into a buffer.
 */
void
brw_write_timestamp(struct brw_context *brw, struct brw_bo *query_bo, int idx)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->ver == 6) {
      /* Emit Sandybridge workaround flush: */
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_CS_STALL |
                                  PIPE_CONTROL_STALL_AT_SCOREBOARD);
   }

   uint32_t flags = PIPE_CONTROL_WRITE_TIMESTAMP;

   if (devinfo->ver == 9 && devinfo->gt == 4)
      flags |= PIPE_CONTROL_CS_STALL;

   brw_emit_pipe_control_write(brw, flags,
                               query_bo, idx * sizeof(uint64_t), 0);
}

/**
 * Emit PIPE_CONTROLs to write the PS_DEPTH_COUNT register into a buffer.
 */
void
brw_write_depth_count(struct brw_context *brw, struct brw_bo *query_bo, int idx)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t flags = PIPE_CONTROL_WRITE_DEPTH_COUNT | PIPE_CONTROL_DEPTH_STALL;

   if (devinfo->ver == 9 && devinfo->gt == 4)
      flags |= PIPE_CONTROL_CS_STALL;

   if (devinfo->ver >= 10) {
      /* "Driver must program PIPE_CONTROL with only Depth Stall Enable bit set
       * prior to programming a PIPE_CONTROL with Write PS Depth Count Post sync
       * operation."
       */
      brw_emit_pipe_control_flush(brw, PIPE_CONTROL_DEPTH_STALL);
   }

   brw_emit_pipe_control_write(brw, flags,
                               query_bo, idx * sizeof(uint64_t), 0);
}

/**
 * Wait on the query object's BO and calculate the final result.
 */
static void
brw_queryobj_get_results(struct gl_context *ctx,
                         struct brw_query_object *query)
{
   struct brw_context *brw = brw_context(ctx);
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   int i;
   uint64_t *results;

   assert(devinfo->ver < 6);

   if (query->bo == NULL)
      return;

   /* If the application has requested the query result, but this batch is
    * still contributing to it, flush it now so the results will be present
    * when mapped.
    */
   if (brw_batch_references(&brw->batch, query->bo))
      brw_batch_flush(brw);

   if (unlikely(brw->perf_debug)) {
      if (brw_bo_busy(query->bo)) {
         perf_debug("Stalling on the GPU waiting for a query object.\n");
      }
   }

   results = brw_bo_map(brw, query->bo, MAP_READ);
   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
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
      /* Loop over pairs of values from the BO, which are the PS_DEPTH_COUNT
       * value at the start and end of the batchbuffer.  Subtract them to
       * get the number of fragments which passed the depth test in each
       * individual batch, and add those differences up to get the number
       * of fragments for the entire query.
       *
       * Note that query->Base.Result may already be non-zero.  We may have
       * run out of space in the query's BO and allocated a new one.  If so,
       * this function was already called to accumulate the results so far.
       */
      for (i = 0; i < query->last_index; i++) {
         query->Base.Result += results[i * 2 + 1] - results[i * 2];
      }
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      /* If the starting and ending PS_DEPTH_COUNT from any of the batches
       * differ, then some fragments passed the depth test.
       */
      for (i = 0; i < query->last_index; i++) {
         if (results[i * 2 + 1] != results[i * 2]) {
            query->Base.Result = GL_TRUE;
            break;
         }
      }
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
}

/**
 * The NewQueryObject() driver hook.
 *
 * Allocates and initializes a new query object.
 */
static struct gl_query_object *
brw_new_query_object(struct gl_context *ctx, GLuint id)
{
   struct brw_query_object *query;

   query = calloc(1, sizeof(struct brw_query_object));

   query->Base.Id = id;
   query->Base.Result = 0;
   query->Base.Active = false;
   query->Base.Ready = true;

   return &query->Base;
}

/**
 * The DeleteQuery() driver hook.
 */
static void
brw_delete_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   brw_bo_unreference(query->bo);
   _mesa_delete_query(ctx, q);
}

/**
 * Gfx4-5 driver hook for glBeginQuery().
 *
 * Initializes driver structures and emits any GPU commands required to begin
 * recording data for the query.
 */
static void
brw_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver < 6);

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
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
      brw_bo_unreference(query->bo);
      query->bo =
         brw_bo_alloc(brw->bufmgr, "timer query", 4096, BRW_MEMZONE_OTHER);
      brw_write_timestamp(brw, query->bo, 0);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      /* For occlusion queries, we delay taking an initial sample until the
       * first drawing occurs in this batch.  See the reasoning in the comments
       * for brw_emit_query_begin() below.
       *
       * Since we're starting a new query, we need to be sure to throw away
       * any previous occlusion query results.
       */
      brw_bo_unreference(query->bo);
      query->bo = NULL;
      query->last_index = -1;

      brw->query.obj = query;

      /* Depth statistics on Gfx4 require strange workarounds, so we try to
       * avoid them when necessary.  They're required for occlusion queries,
       * so turn them on now.
       */
      brw->stats_wm++;
      brw->ctx.NewDriverState |= BRW_NEW_STATS_WM;
      break;

   default:
      unreachable("Unrecognized query target in brw_begin_query()");
   }
}

/**
 * Gfx4-5 driver hook for glEndQuery().
 *
 * Emits GPU commands to record a final query value, ending any data capturing.
 * However, the final result isn't necessarily available until the GPU processes
 * those commands.  brw_queryobj_get_results() processes the captured data to
 * produce the final result.
 */
static void
brw_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver < 6);

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
      /* Write the final timestamp. */
      brw_write_timestamp(brw, query->bo, 1);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:

      /* No query->bo means that EndQuery was called after BeginQuery with no
       * intervening drawing. Rather than doing nothing at all here in this
       * case, we emit the query_begin and query_end state to the
       * hardware. This is to guarantee that waiting on the result of this
       * empty state will cause all previous queries to complete at all, as
       * required by the OpenGL 4.3 (Core Profile) spec, section 4.2.1:
       *
       *    "It must always be true that if any query object returns
       *     a result available of TRUE, all queries of the same type
       *     issued prior to that query must also return TRUE."
       */
      if (!query->bo) {
         brw_emit_query_begin(brw);
      }

      assert(query->bo);

      brw_emit_query_end(brw);

      brw->query.obj = NULL;

      brw->stats_wm--;
      brw->ctx.NewDriverState |= BRW_NEW_STATS_WM;
      break;

   default:
      unreachable("Unrecognized query target in brw_end_query()");
   }
}

/**
 * The Gfx4-5 WaitQuery() driver hook.
 *
 * Wait for a query result to become available and return it.  This is the
 * backing for glGetQueryObjectiv() with the GL_QUERY_RESULT pname.
 */
static void brw_wait_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;
   UNUSED const struct intel_device_info *devinfo =
      &brw_context(ctx)->screen->devinfo;

   assert(devinfo->ver < 6);

   brw_queryobj_get_results(ctx, query);
   query->Base.Ready = true;
}

/**
 * The Gfx4-5 CheckQuery() driver hook.
 *
 * Checks whether a query result is ready yet.  If not, flushes.
 * This is the backing for glGetQueryObjectiv()'s QUERY_RESULT_AVAILABLE pname.
 */
static void brw_check_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver < 6);

   /* From the GL_ARB_occlusion_query spec:
    *
    *     "Instead of allowing for an infinite loop, performing a
    *      QUERY_RESULT_AVAILABLE_ARB will perform a flush if the result is
    *      not ready yet on the first time it is queried.  This ensures that
    *      the async query will return true in finite time.
    */
   if (query->bo && brw_batch_references(&brw->batch, query->bo))
      brw_batch_flush(brw);

   if (query->bo == NULL || !brw_bo_busy(query->bo)) {
      brw_queryobj_get_results(ctx, query);
      query->Base.Ready = true;
   }
}

/**
 * Ensure there query's BO has enough space to store a new pair of values.
 *
 * If not, gather the existing BO's results and create a new buffer of the
 * same size.
 */
static void
ensure_bo_has_space(struct gl_context *ctx, struct brw_query_object *query)
{
   struct brw_context *brw = brw_context(ctx);
   UNUSED const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(devinfo->ver < 6);

   if (!query->bo || query->last_index * 2 + 1 >= 4096 / sizeof(uint64_t)) {

      if (query->bo != NULL) {
         /* The old query BO did not have enough space, so we allocated a new
          * one.  Gather the results so far (adding up the differences) and
          * release the old BO.
          */
         brw_queryobj_get_results(ctx, query);
      }

      query->bo = brw_bo_alloc(brw->bufmgr, "query", 4096, BRW_MEMZONE_OTHER);
      query->last_index = 0;
   }
}

/**
 * Record the PS_DEPTH_COUNT value (for occlusion queries) just before
 * primitive drawing.
 *
 * In a pre-hardware context world, the single PS_DEPTH_COUNT register is
 * shared among all applications using the GPU.  However, our query value
 * needs to only include fragments generated by our application/GL context.
 *
 * To accommodate this, we record PS_DEPTH_COUNT at the start and end of
 * each batchbuffer (technically, the first primitive drawn and flush time).
 * Subtracting each pair of values calculates the change in PS_DEPTH_COUNT
 * caused by a batchbuffer.  Since there is no preemption inside batches,
 * this is guaranteed to only measure the effects of our current application.
 *
 * Adding each of these differences (in case drawing is done over many batches)
 * produces the final expected value.
 *
 * In a world with hardware contexts, PS_DEPTH_COUNT is saved and restored
 * as part of the context state, so this is unnecessary, and skipped.
 */
void
brw_emit_query_begin(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_query_object *query = brw->query.obj;

   /* Skip if we're not doing any queries, or we've already recorded the
    * initial query value for this batchbuffer.
    */
   if (!query || brw->query.begin_emitted)
      return;

   ensure_bo_has_space(ctx, query);

   brw_write_depth_count(brw, query->bo, query->last_index * 2);

   brw->query.begin_emitted = true;
}

/**
 * Called at batchbuffer flush to get an ending PS_DEPTH_COUNT
 * (for non-hardware context platforms).
 *
 * See the explanation in brw_emit_query_begin().
 */
void
brw_emit_query_end(struct brw_context *brw)
{
   struct brw_query_object *query = brw->query.obj;

   if (!brw->query.begin_emitted)
      return;

   brw_write_depth_count(brw, query->bo, query->last_index * 2 + 1);

   brw->query.begin_emitted = false;
   query->last_index++;
}

/**
 * Driver hook for glQueryCounter().
 *
 * This handles GL_TIMESTAMP queries, which perform a pipelined read of the
 * current GPU time.  This is unlike GL_TIME_ELAPSED, which measures the
 * time while the query is active.
 */
void
brw_query_counter(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *) q;

   assert(q->Target == GL_TIMESTAMP);

   brw_bo_unreference(query->bo);
   query->bo =
      brw_bo_alloc(brw->bufmgr, "timestamp query", 4096, BRW_MEMZONE_OTHER);
   brw_write_timestamp(brw, query->bo, 0);

   query->flushed = false;
}

/**
 * Read the TIMESTAMP register immediately (in a non-pipelined fashion).
 *
 * This is used to implement the GetTimestamp() driver hook.
 */
static uint64_t
brw_get_timestamp(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint64_t result = 0;

   switch (brw->screen->hw_has_timestamp) {
   case 3: /* New kernel, always full 36bit accuracy */
      brw_reg_read(brw->bufmgr, TIMESTAMP | 1, &result);
      break;
   case 2: /* 64bit kernel, result is left-shifted by 32bits, losing 4bits */
      brw_reg_read(brw->bufmgr, TIMESTAMP, &result);
      result = result >> 32;
      break;
   case 1: /* 32bit kernel, result is 36bit wide but may be inaccurate! */
      brw_reg_read(brw->bufmgr, TIMESTAMP, &result);
      break;
   }

   /* Scale to nanosecond units */
   result = intel_device_info_timebase_scale(devinfo, result);

   /* Ensure the scaled timestamp overflows according to
    * GL_QUERY_COUNTER_BITS.  Technically this isn't required if
    * querying GL_TIMESTAMP via glGetInteger but it seems best to keep
    * QueryObject and GetInteger timestamps consistent.
    */
   result &= (1ull << ctx->Const.QueryCounterBits.Timestamp) - 1;
   return result;
}

/**
 * Is this type of query written by PIPE_CONTROL?
 */
bool
brw_is_query_pipelined(struct brw_query_object *query)
{
   switch (query->Base.Target) {
   case GL_TIMESTAMP:
   case GL_TIME_ELAPSED:
   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      return true;

   case GL_PRIMITIVES_GENERATED:
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
   case GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB:
   case GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB:
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
      return false;

   default:
      unreachable("Unrecognized query target in is_query_pipelined()");
   }
}

/* Initialize query object functions used on all generations. */
void brw_init_common_queryobj_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = brw_new_query_object;
   functions->DeleteQuery = brw_delete_query;
   functions->GetTimestamp = brw_get_timestamp;
}

/* Initialize Gfx4/5-specific query object functions. */
void gfx4_init_queryobj_functions(struct dd_function_table *functions)
{
   functions->BeginQuery = brw_begin_query;
   functions->EndQuery = brw_end_query;
   functions->CheckQuery = brw_check_query;
   functions->WaitQuery = brw_wait_query;
   functions->QueryCounter = brw_query_counter;
}
