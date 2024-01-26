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

/** \file gfx6_sol.c
 *
 * Code to initialize the binding table entries used by transform feedback.
 */

#include "main/bufferobj.h"
#include "main/macros.h"
#include "brw_context.h"
#include "brw_batch.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "main/transformfeedback.h"
#include "util/u_memory.h"

static void
gfx6_update_sol_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   bool xfb_active = _mesa_is_xfb_active_and_unpaused(ctx);
   struct gl_transform_feedback_object *xfb_obj;
   const struct gl_transform_feedback_info *linked_xfb_info = NULL;

   if (xfb_active) {
      /* BRW_NEW_TRANSFORM_FEEDBACK */
      xfb_obj = ctx->TransformFeedback.CurrentObject;
      linked_xfb_info = xfb_obj->program->sh.LinkedTransformFeedback;
   }

   for (int i = 0; i < BRW_MAX_SOL_BINDINGS; ++i) {
      const int surf_index = BRW_GFX6_SOL_BINDING_START + i;
      if (xfb_active && i < linked_xfb_info->NumOutputs) {
         unsigned buffer = linked_xfb_info->Outputs[i].OutputBuffer;
         unsigned buffer_offset =
            xfb_obj->Offset[buffer] / 4 +
            linked_xfb_info->Outputs[i].DstOffset;
         if (brw->programs[MESA_SHADER_GEOMETRY]) {
            brw_update_sol_surface(
               brw, xfb_obj->Buffers[buffer],
               &brw->gs.base.surf_offset[surf_index],
               linked_xfb_info->Outputs[i].NumComponents,
               linked_xfb_info->Buffers[buffer].Stride, buffer_offset);
         } else {
            brw_update_sol_surface(
               brw, xfb_obj->Buffers[buffer],
               &brw->ff_gs.surf_offset[surf_index],
               linked_xfb_info->Outputs[i].NumComponents,
               linked_xfb_info->Buffers[buffer].Stride, buffer_offset);
         }
      } else {
         if (!brw->programs[MESA_SHADER_GEOMETRY])
            brw->ff_gs.surf_offset[surf_index] = 0;
         else
            brw->gs.base.surf_offset[surf_index] = 0;
      }
   }

   brw->ctx.NewDriverState |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state gfx6_sol_surface = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_TRANSFORM_FEEDBACK,
   },
   .emit = gfx6_update_sol_surfaces,
};

/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_gs_upload_binding_table(struct brw_context *brw)
{
   uint32_t *bind;
   struct gl_context *ctx = &brw->ctx;
   const struct gl_program *prog;
   bool need_binding_table = false;

   /* We have two scenarios here:
    * 1) We are using a geometry shader only to implement transform feedback
    *    for a vertex shader (brw->programs[MESA_SHADER_GEOMETRY] == NULL).
    *    In this case, we only need surfaces for transform feedback in the
    *    GS stage.
    * 2) We have a user-provided geometry shader. In this case we may need
    *    surfaces for transform feedback and/or other stuff, like textures,
    *    in the GS stage.
    */

   if (!brw->programs[MESA_SHADER_GEOMETRY]) {
      /* BRW_NEW_VERTEX_PROGRAM */
      prog = ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];
      if (prog) {
         /* Skip making a binding table if we don't have anything to put in it */
         const struct gl_transform_feedback_info *linked_xfb_info =
            prog->sh.LinkedTransformFeedback;
         need_binding_table = linked_xfb_info->NumOutputs > 0;
      }
      if (!need_binding_table) {
         if (brw->ff_gs.bind_bo_offset != 0) {
            brw->ctx.NewDriverState |= BRW_NEW_BINDING_TABLE_POINTERS;
            brw->ff_gs.bind_bo_offset = 0;
         }
         return;
      }

      /* Might want to calculate nr_surfaces first, to avoid taking up so much
       * space for the binding table. Anyway, in this case we know that we only
       * use BRW_MAX_SOL_BINDINGS surfaces at most.
       */
      bind = brw_state_batch(brw, sizeof(uint32_t) * BRW_MAX_SOL_BINDINGS,
                             32, &brw->ff_gs.bind_bo_offset);

      /* BRW_NEW_SURFACES */
      memcpy(bind, brw->ff_gs.surf_offset,
             BRW_MAX_SOL_BINDINGS * sizeof(uint32_t));
   } else {
      /* BRW_NEW_GEOMETRY_PROGRAM */
      prog = ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY];
      if (prog) {
         /* Skip making a binding table if we don't have anything to put in it */
         struct brw_stage_prog_data *prog_data = brw->gs.base.prog_data;
         const struct gl_transform_feedback_info *linked_xfb_info =
            prog->sh.LinkedTransformFeedback;
         need_binding_table = linked_xfb_info->NumOutputs > 0 ||
                              prog_data->binding_table.size_bytes > 0;
      }
      if (!need_binding_table) {
         if (brw->gs.base.bind_bo_offset != 0) {
            brw->gs.base.bind_bo_offset = 0;
            brw->ctx.NewDriverState |= BRW_NEW_BINDING_TABLE_POINTERS;
         }
         return;
      }

      /* Might want to calculate nr_surfaces first, to avoid taking up so much
       * space for the binding table.
       */
      bind = brw_state_batch(brw, sizeof(uint32_t) * BRW_MAX_SURFACES,
                             32, &brw->gs.base.bind_bo_offset);

      /* BRW_NEW_SURFACES */
      memcpy(bind, brw->gs.base.surf_offset,
             BRW_MAX_SURFACES * sizeof(uint32_t));
   }

   brw->ctx.NewDriverState |= BRW_NEW_BINDING_TABLE_POINTERS;
}

const struct brw_tracked_state gfx6_gs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_GEOMETRY_PROGRAM |
             BRW_NEW_VERTEX_PROGRAM |
             BRW_NEW_SURFACES,
   },
   .emit = brw_gs_upload_binding_table,
};

struct gl_transform_feedback_object *
brw_new_transform_feedback(struct gl_context *ctx, GLuint name)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      CALLOC_STRUCT(brw_transform_feedback_object);
   if (!brw_obj)
      return NULL;

   _mesa_init_transform_feedback_object(&brw_obj->base, name);

   brw_obj->offset_bo =
      brw_bo_alloc(brw->bufmgr, "transform feedback offsets", 16,
                   BRW_MEMZONE_OTHER);
   brw_obj->prim_count_bo =
      brw_bo_alloc(brw->bufmgr, "xfb primitive counts", 16384,
                   BRW_MEMZONE_OTHER);

   return &brw_obj->base;
}

void
brw_delete_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   brw_bo_unreference(brw_obj->offset_bo);
   brw_bo_unreference(brw_obj->prim_count_bo);

   _mesa_delete_transform_feedback_object(ctx, obj);
}

/**
 * Tally the number of primitives generated so far.
 *
 * The buffer contains a series of pairs:
 * (<start0, start1, start2, start3>, <end0, end1, end2, end3>) ;
 * (<start0, start1, start2, start3>, <end0, end1, end2, end3>) ;
 *
 * For each stream, we subtract the pair of values (end - start) to get the
 * number of primitives generated during one section.  We accumulate these
 * values, adding them up to get the total number of primitives generated.
 *
 * Note that we expose one stream pre-Gfx7, so the above is just (start, end).
 */
static void
aggregate_transform_feedback_counter(
   struct brw_context *brw,
   struct brw_bo *bo,
   struct brw_transform_feedback_counter *counter)
{
   const unsigned streams = brw->ctx.Const.MaxVertexStreams;

   /* If the current batch is still contributing to the number of primitives
    * generated, flush it now so the results will be present when mapped.
    */
   if (brw_batch_references(&brw->batch, bo))
      brw_batch_flush(brw);

   if (unlikely(brw->perf_debug && brw_bo_busy(bo)))
      perf_debug("Stalling for # of transform feedback primitives written.\n");

   uint64_t *prim_counts = brw_bo_map(brw, bo, MAP_READ);
   prim_counts += counter->bo_start * streams;

   for (unsigned i = counter->bo_start; i + 1 < counter->bo_end; i += 2) {
      for (unsigned s = 0; s < streams; s++)
         counter->accum[s] += prim_counts[streams + s] - prim_counts[s];

      prim_counts += 2 * streams;
   }

   brw_bo_unmap(bo);

   /* We've already gathered up the old data; we can safely overwrite it now. */
   counter->bo_start = counter->bo_end = 0;
}

/**
 * Store the SO_NUM_PRIMS_WRITTEN counters for each stream (4 uint64_t values)
 * to prim_count_bo.
 *
 * If prim_count_bo is out of space, gather up the results so far into
 * prims_generated[] and allocate a new buffer with enough space.
 *
 * The number of primitives written is used to compute the number of vertices
 * written to a transform feedback stream, which is required to implement
 * DrawTransformFeedback().
 */
void
brw_save_primitives_written_counters(struct brw_context *brw,
                                     struct brw_transform_feedback_object *obj)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct gl_context *ctx = &brw->ctx;
   const int streams = ctx->Const.MaxVertexStreams;

   assert(obj->prim_count_bo != NULL);

   /* Check if there's enough space for a new pair of four values. */
   if ((obj->counter.bo_end + 2) * streams * sizeof(uint64_t) >=
       obj->prim_count_bo->size) {
      aggregate_transform_feedback_counter(brw, obj->prim_count_bo,
                                           &obj->previous_counter);
      aggregate_transform_feedback_counter(brw, obj->prim_count_bo,
                                           &obj->counter);
   }

   /* Flush any drawing so that the counters have the right values. */
   brw_emit_mi_flush(brw);

   /* Emit MI_STORE_REGISTER_MEM commands to write the values. */
   if (devinfo->ver >= 7) {
      for (int i = 0; i < streams; i++) {
         int offset = (streams * obj->counter.bo_end + i) * sizeof(uint64_t);
         brw_store_register_mem64(brw, obj->prim_count_bo,
                                  GFX7_SO_NUM_PRIMS_WRITTEN(i),
                                  offset);
      }
   } else {
      brw_store_register_mem64(brw, obj->prim_count_bo,
                               GFX6_SO_NUM_PRIMS_WRITTEN,
                               obj->counter.bo_end * sizeof(uint64_t));
   }

   /* Update where to write data to. */
   obj->counter.bo_end++;
}

static void
compute_vertices_written_so_far(struct brw_context *brw,
                                struct brw_transform_feedback_object *obj,
                                struct brw_transform_feedback_counter *counter,
                                uint64_t *vertices_written)
{
   const struct gl_context *ctx = &brw->ctx;
   unsigned vertices_per_prim = 0;

   switch (obj->primitive_mode) {
   case GL_POINTS:
      vertices_per_prim = 1;
      break;
   case GL_LINES:
      vertices_per_prim = 2;
      break;
   case GL_TRIANGLES:
      vertices_per_prim = 3;
      break;
   default:
      unreachable("Invalid transform feedback primitive mode.");
   }

   /* Get the number of primitives generated. */
   aggregate_transform_feedback_counter(brw, obj->prim_count_bo, counter);

   for (int i = 0; i < ctx->Const.MaxVertexStreams; i++) {
      vertices_written[i] = vertices_per_prim * counter->accum[i];
   }
}

/**
 * Compute the number of vertices written by the last transform feedback
 * begin/end block.
 */
static void
compute_xfb_vertices_written(struct brw_context *brw,
                             struct brw_transform_feedback_object *obj)
{
   if (obj->vertices_written_valid || !obj->base.EndedAnytime)
      return;

   compute_vertices_written_so_far(brw, obj, &obj->previous_counter,
                                   obj->vertices_written);
   obj->vertices_written_valid = true;
}

/**
 * GetTransformFeedbackVertexCount() driver hook.
 *
 * Returns the number of vertices written to a particular stream by the last
 * Begin/EndTransformFeedback block.  Used to implement DrawTransformFeedback().
 */
GLsizei
brw_get_transform_feedback_vertex_count(struct gl_context *ctx,
                                        struct gl_transform_feedback_object *obj,
                                        GLuint stream)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   assert(obj->EndedAnytime);
   assert(stream < ctx->Const.MaxVertexStreams);

   compute_xfb_vertices_written(brw, brw_obj);
   return brw_obj->vertices_written[stream];
}

void
brw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                             struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   const struct gl_program *prog;
   const struct gl_transform_feedback_info *linked_xfb_info;
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) xfb_obj;

   assert(brw->screen->devinfo.ver == 6);

   if (ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY]) {
      /* BRW_NEW_GEOMETRY_PROGRAM */
      prog = ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY];
   } else {
      /* BRW_NEW_VERTEX_PROGRAM */
      prog = ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];
   }
   linked_xfb_info = prog->sh.LinkedTransformFeedback;

   /* Compute the maximum number of vertices that we can write without
    * overflowing any of the buffers currently being used for feedback.
    */
   brw_obj->max_index
      = _mesa_compute_max_transform_feedback_vertices(ctx, xfb_obj,
                                                      linked_xfb_info);

   /* Initialize the SVBI 0 register to zero and set the maximum index. */
   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
   OUT_BATCH(0); /* SVBI 0 */
   OUT_BATCH(0); /* starting index */
   OUT_BATCH(brw_obj->max_index);
   ADVANCE_BATCH();

   /* Initialize the rest of the unused streams to sane values.  Otherwise,
    * they may indicate that there is no room to write data and prevent
    * anything from happening at all.
    */
   for (int i = 1; i < 4; i++) {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
      OUT_BATCH(i << SVB_INDEX_SHIFT);
      OUT_BATCH(0); /* starting index */
      OUT_BATCH(0xffffffff);
      ADVANCE_BATCH();
   }

   /* Store the starting value of the SO_NUM_PRIMS_WRITTEN counters. */
   brw_save_primitives_written_counters(brw, brw_obj);

   brw_obj->primitive_mode = mode;
}

void
brw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Store the ending value of the SO_NUM_PRIMS_WRITTEN counters. */
   if (!obj->Paused)
      brw_save_primitives_written_counters(brw, brw_obj);

   /* We've reached the end of a transform feedback begin/end block.  This
    * means that future DrawTransformFeedback() calls will need to pick up the
    * results of the current counter, and that it's time to roll back the
    * current primitive counter to zero.
    */
   brw_obj->previous_counter = brw_obj->counter;
   brw_reset_transform_feedback_counter(&brw_obj->counter);

   /* EndTransformFeedback() means that we need to update the number of
    * vertices written.  Since it's only necessary if DrawTransformFeedback()
    * is called and it means mapping a buffer object, we delay computing it
    * until it's absolutely necessary to try and avoid stalls.
    */
   brw_obj->vertices_written_valid = false;
}

void
brw_pause_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Store the temporary ending value of the SO_NUM_PRIMS_WRITTEN counters.
    * While this operation is paused, other transform feedback actions may
    * occur, which will contribute to the counters.  We need to exclude that
    * from our counts.
    */
   brw_save_primitives_written_counters(brw, brw_obj);
}

void
brw_resume_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Reload SVBI 0 with the count of vertices written so far. */
   uint64_t svbi;
   compute_vertices_written_so_far(brw, brw_obj, &brw_obj->counter, &svbi);

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
   OUT_BATCH(0); /* SVBI 0 */
   OUT_BATCH((uint32_t) svbi); /* starting index */
   OUT_BATCH(brw_obj->max_index);
   ADVANCE_BATCH();

   /* Initialize the rest of the unused streams to sane values.  Otherwise,
    * they may indicate that there is no room to write data and prevent
    * anything from happening at all.
    */
   for (int i = 1; i < 4; i++) {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
      OUT_BATCH(i << SVB_INDEX_SHIFT);
      OUT_BATCH(0); /* starting index */
      OUT_BATCH(0xffffffff);
      ADVANCE_BATCH();
   }

   /* Store the new starting value of the SO_NUM_PRIMS_WRITTEN counters. */
   brw_save_primitives_written_counters(brw, brw_obj);
}
