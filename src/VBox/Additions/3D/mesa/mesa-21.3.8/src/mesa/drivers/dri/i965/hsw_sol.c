/*
 * Copyright © 2016 Intel Corporation
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

/**
 * An implementation of the transform feedback driver hooks for Haswell
 * and later hardware.  This uses MI_MATH to compute the number of vertices
 * written (for use by DrawTransformFeedback()) without any CPU<->GPU
 * synchronization which could stall.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "main/transformfeedback.h"

/**
 * We store several values in obj->prim_count_bo:
 *
 * [4x 32-bit values]: Final Number of Vertices Written
 * [4x 32-bit values]: Tally of Primitives Written So Far
 * [4x 64-bit values]: Starting SO_NUM_PRIMS_WRITTEN Counter Snapshots
 *
 * The first set of values is used by DrawTransformFeedback(), which
 * copies one of them into the 3DPRIM_VERTEX_COUNT register and performs
 * an indirect draw.  The other values are just temporary storage.
 */

#define TALLY_OFFSET (BRW_MAX_XFB_STREAMS * sizeof(uint32_t))
#define START_OFFSET (TALLY_OFFSET * 2)

/**
 * Store the SO_NUM_PRIMS_WRITTEN counters for each stream (4 uint64_t values)
 * to prim_count_bo.
 */
static void
save_prim_start_values(struct brw_context *brw,
                       struct brw_transform_feedback_object *obj)
{
   /* Flush any drawing so that the counters have the right values. */
   brw_emit_mi_flush(brw);

   /* Emit MI_STORE_REGISTER_MEM commands to write the values. */
   for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
      brw_store_register_mem64(brw, obj->prim_count_bo,
                               GFX7_SO_NUM_PRIMS_WRITTEN(i),
                               START_OFFSET + i * sizeof(uint64_t));
   }
}

/**
 * Compute the number of primitives written during our most recent
 * transform feedback activity (the current SO_NUM_PRIMS_WRITTEN value
 * minus the stashed "start" value), and add it to our running tally.
 *
 * If \p finalize is true, also compute the number of vertices written
 * (by multiplying by the number of vertices per primitive), and store
 * that to the "final" location.
 *
 * Otherwise, just overwrite the old tally with the new one.
 */
static void
tally_prims_written(struct brw_context *brw,
                    struct brw_transform_feedback_object *obj,
                    bool finalize)
{
   /* Flush any drawing so that the counters have the right values. */
   brw_emit_mi_flush(brw);

   for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
      /* GPR0 = Tally */
      brw_load_register_imm32(brw, HSW_CS_GPR(0) + 4, 0);
      brw_load_register_mem(brw, HSW_CS_GPR(0), obj->prim_count_bo,
                            TALLY_OFFSET + i * sizeof(uint32_t));
      if (!obj->base.Paused) {
         /* GPR1 = Start Snapshot */
         brw_load_register_mem64(brw, HSW_CS_GPR(1), obj->prim_count_bo,
                                 START_OFFSET + i * sizeof(uint64_t));
         /* GPR2 = Ending Snapshot */
         brw_load_register_reg64(brw, HSW_CS_GPR(2),
                                 GFX7_SO_NUM_PRIMS_WRITTEN(i));

         BEGIN_BATCH(9);
         OUT_BATCH(HSW_MI_MATH | (9 - 2));
         /* GPR1 = GPR2 (End) - GPR1 (Start) */
         OUT_BATCH(MI_MATH_ALU2(LOAD, SRCA, R2));
         OUT_BATCH(MI_MATH_ALU2(LOAD, SRCB, R1));
         OUT_BATCH(MI_MATH_ALU0(SUB));
         OUT_BATCH(MI_MATH_ALU2(STORE, R1, ACCU));
         /* GPR0 = GPR0 (Tally) + GPR1 (Diff) */
         OUT_BATCH(MI_MATH_ALU2(LOAD, SRCA, R0));
         OUT_BATCH(MI_MATH_ALU2(LOAD, SRCB, R1));
            OUT_BATCH(MI_MATH_ALU0(ADD));
         OUT_BATCH(MI_MATH_ALU2(STORE, R0, ACCU));
         ADVANCE_BATCH();
      }

      if (!finalize) {
         /* Write back the new tally */
         brw_store_register_mem32(brw, obj->prim_count_bo, HSW_CS_GPR(0),
                                  TALLY_OFFSET + i * sizeof(uint32_t));
      } else {
         /* Convert the number of primitives to the number of vertices. */
         if (obj->primitive_mode == GL_LINES) {
            /* Double R0 (R0 = R0 + R0) */
            BEGIN_BATCH(5);
            OUT_BATCH(HSW_MI_MATH | (5 - 2));
            OUT_BATCH(MI_MATH_ALU2(LOAD, SRCA, R0));
            OUT_BATCH(MI_MATH_ALU2(LOAD, SRCB, R0));
            OUT_BATCH(MI_MATH_ALU0(ADD));
            OUT_BATCH(MI_MATH_ALU2(STORE, R0, ACCU));
            ADVANCE_BATCH();
         } else if (obj->primitive_mode == GL_TRIANGLES) {
            /* Triple R0 (R1 = R0 + R0, R0 = R0 + R1) */
            BEGIN_BATCH(9);
            OUT_BATCH(HSW_MI_MATH | (9 - 2));
            OUT_BATCH(MI_MATH_ALU2(LOAD, SRCA, R0));
            OUT_BATCH(MI_MATH_ALU2(LOAD, SRCB, R0));
            OUT_BATCH(MI_MATH_ALU0(ADD));
            OUT_BATCH(MI_MATH_ALU2(STORE, R1, ACCU));
            OUT_BATCH(MI_MATH_ALU2(LOAD, SRCA, R0));
            OUT_BATCH(MI_MATH_ALU2(LOAD, SRCB, R1));
            OUT_BATCH(MI_MATH_ALU0(ADD));
            OUT_BATCH(MI_MATH_ALU2(STORE, R0, ACCU));
            ADVANCE_BATCH();
         }
         /* Store it to the final result */
         brw_store_register_mem32(brw, obj->prim_count_bo, HSW_CS_GPR(0),
                                  i * sizeof(uint32_t));
      }
   }
}

/**
 * BeginTransformFeedback() driver hook.
 */
void
hsw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   brw_obj->primitive_mode = mode;

   /* Reset the SO buffer offsets to 0. */
   if (devinfo->ver >= 8) {
      brw_obj->zero_offsets = true;
   } else {
      BEGIN_BATCH(1 + 2 * BRW_MAX_XFB_STREAMS);
      OUT_BATCH(MI_LOAD_REGISTER_IMM | (1 + 2 * BRW_MAX_XFB_STREAMS - 2));
      for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
         OUT_BATCH(GFX7_SO_WRITE_OFFSET(i));
         OUT_BATCH(0);
      }
      ADVANCE_BATCH();
   }

   /* Zero out the initial tallies */
   brw_store_data_imm64(brw, brw_obj->prim_count_bo, TALLY_OFFSET,     0ull);
   brw_store_data_imm64(brw, brw_obj->prim_count_bo, TALLY_OFFSET + 8, 0ull);

   /* Store the new starting value of the SO_NUM_PRIMS_WRITTEN counters. */
   save_prim_start_values(brw, brw_obj);
}

/**
 * PauseTransformFeedback() driver hook.
 */
void
hsw_pause_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->is_haswell) {
      /* Flush any drawing so that the counters have the right values. */
      brw_emit_mi_flush(brw);

      /* Save the SOL buffer offset register values. */
      for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
         BEGIN_BATCH(3);
         OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
         OUT_BATCH(GFX7_SO_WRITE_OFFSET(i));
         OUT_RELOC(brw_obj->offset_bo, RELOC_WRITE, i * sizeof(uint32_t));
         ADVANCE_BATCH();
      }
   }

   /* Add any primitives written to our tally */
   tally_prims_written(brw, brw_obj, false);
}

/**
 * ResumeTransformFeedback() driver hook.
 */
void
hsw_resume_transform_feedback(struct gl_context *ctx,
                               struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   if (devinfo->is_haswell) {
      /* Reload the SOL buffer offset registers. */
      for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
         BEGIN_BATCH(3);
         OUT_BATCH(GFX7_MI_LOAD_REGISTER_MEM | (3 - 2));
         OUT_BATCH(GFX7_SO_WRITE_OFFSET(i));
         OUT_RELOC(brw_obj->offset_bo, RELOC_WRITE, i * sizeof(uint32_t));
         ADVANCE_BATCH();
      }
   }

   /* Store the new starting value of the SO_NUM_PRIMS_WRITTEN counters. */
   save_prim_start_values(brw, brw_obj);
}

/**
 * EndTransformFeedback() driver hook.
 */
void
hsw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Add any primitives written to our tally, convert it from the number
    * of primitives written to the number of vertices written, and store
    * it in the "final" location in the buffer which DrawTransformFeedback()
    * will use as the vertex count.
    */
   tally_prims_written(brw, brw_obj, true);
}
