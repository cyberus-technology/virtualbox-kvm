/*
 * Copyright (c) 2016 Intel Corporation
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
 */

/** @file hsw_queryobj.c
 *
 * Support for query buffer objects (GL_ARB_query_buffer_object) on Haswell+.
 */
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_batch.h"
#include "brw_buffer_objects.h"

/*
 * GPR0 = 80 * GPR0;
 */
static void
mult_gpr0_by_80(struct brw_context *brw)
{
   static const uint32_t maths[] = {
      MI_MATH_ALU2(LOAD, SRCA, R0),
      MI_MATH_ALU2(LOAD, SRCB, R0),
      MI_MATH_ALU0(ADD),
      MI_MATH_ALU2(STORE, R1, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R1),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(ADD),
      MI_MATH_ALU2(STORE, R1, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R1),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(ADD),
      MI_MATH_ALU2(STORE, R1, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R1),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(ADD),
      /* GPR1 = 16 * GPR0 */
      MI_MATH_ALU2(STORE, R1, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R1),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(ADD),
      MI_MATH_ALU2(STORE, R2, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R2),
      MI_MATH_ALU2(LOAD, SRCB, R2),
      MI_MATH_ALU0(ADD),
      /* GPR2 = 64 * GPR0 */
      MI_MATH_ALU2(STORE, R2, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R1),
      MI_MATH_ALU2(LOAD, SRCB, R2),
      MI_MATH_ALU0(ADD),
      /* GPR0 = 80 * GPR0 */
      MI_MATH_ALU2(STORE, R0, ACCU),
   };

   BEGIN_BATCH(1 + ARRAY_SIZE(maths));
   OUT_BATCH(HSW_MI_MATH | (1 + ARRAY_SIZE(maths) - 2));

   for (int m = 0; m < ARRAY_SIZE(maths); m++)
      OUT_BATCH(maths[m]);

   ADVANCE_BATCH();
}

/*
 * GPR0 = GPR0 & ((1ull << n) - 1);
 */
static void
keep_gpr0_lower_n_bits(struct brw_context *brw, uint32_t n)
{
   static const uint32_t maths[] = {
      MI_MATH_ALU2(LOAD, SRCA, R0),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(AND),
      MI_MATH_ALU2(STORE, R0, ACCU),
   };

   assert(n < 64);
   brw_load_register_imm64(brw, HSW_CS_GPR(1), (1ull << n) - 1);

   BEGIN_BATCH(1 + ARRAY_SIZE(maths));
   OUT_BATCH(HSW_MI_MATH | (1 + ARRAY_SIZE(maths) - 2));

   for (int m = 0; m < ARRAY_SIZE(maths); m++)
      OUT_BATCH(maths[m]);

   ADVANCE_BATCH();
}

/*
 * GPR0 = GPR0 << 30;
 */
static void
shl_gpr0_by_30_bits(struct brw_context *brw)
{
   /* First we mask 34 bits of GPR0 to prevent overflow */
   keep_gpr0_lower_n_bits(brw, 34);

   static const uint32_t shl_maths[] = {
      MI_MATH_ALU2(LOAD, SRCA, R0),
      MI_MATH_ALU2(LOAD, SRCB, R0),
      MI_MATH_ALU0(ADD),
      MI_MATH_ALU2(STORE, R0, ACCU),
   };

   const uint32_t outer_count = 5;
   const uint32_t inner_count = 6;
   STATIC_ASSERT(outer_count * inner_count == 30);
   const uint32_t cmd_len = 1 + inner_count * ARRAY_SIZE(shl_maths);
   const uint32_t batch_len = cmd_len * outer_count;

   BEGIN_BATCH(batch_len);

   /* We'll emit 5 commands, each shifting GPR0 left by 6 bits, for a total of
    * 30 left shifts.
    */
   for (int o = 0; o < outer_count; o++) {
      /* Submit one MI_MATH to shift left by 6 bits */
      OUT_BATCH(HSW_MI_MATH | (cmd_len - 2));
      for (int i = 0; i < inner_count; i++)
         for (int m = 0; m < ARRAY_SIZE(shl_maths); m++)
            OUT_BATCH(shl_maths[m]);
   }

   ADVANCE_BATCH();
}

/*
 * GPR0 = GPR0 >> 2;
 *
 * Note that the upper 30 bits of GPR0 are lost!
 */
static void
shr_gpr0_by_2_bits(struct brw_context *brw)
{
   shl_gpr0_by_30_bits(brw);
   brw_load_register_reg(brw, HSW_CS_GPR(0), HSW_CS_GPR(0) + 4);
   brw_load_register_imm32(brw, HSW_CS_GPR(0) + 4, 0);
}

/*
 * GPR0 = (GPR0 == 0) ? 0 : 1;
 */
static void
gpr0_to_bool(struct brw_context *brw)
{
   static const uint32_t maths[] = {
      MI_MATH_ALU2(LOAD, SRCA, R0),
      MI_MATH_ALU1(LOAD0, SRCB),
      MI_MATH_ALU0(ADD),
      MI_MATH_ALU2(STOREINV, R0, ZF),
      MI_MATH_ALU2(LOAD, SRCA, R0),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(AND),
      MI_MATH_ALU2(STORE, R0, ACCU),
   };

   brw_load_register_imm64(brw, HSW_CS_GPR(1), 1ull);

   BEGIN_BATCH(1 + ARRAY_SIZE(maths));
   OUT_BATCH(HSW_MI_MATH | (1 + ARRAY_SIZE(maths) - 2));

   for (int m = 0; m < ARRAY_SIZE(maths); m++)
      OUT_BATCH(maths[m]);

   ADVANCE_BATCH();
}

static void
load_overflow_data_to_cs_gprs(struct brw_context *brw,
                              struct brw_query_object *query,
                              int idx)
{
   int offset = idx * sizeof(uint64_t) * 4;

   brw_load_register_mem64(brw, HSW_CS_GPR(1), query->bo, offset);

   offset += sizeof(uint64_t);
   brw_load_register_mem64(brw, HSW_CS_GPR(2), query->bo, offset);

   offset += sizeof(uint64_t);
   brw_load_register_mem64(brw, HSW_CS_GPR(3), query->bo, offset);

   offset += sizeof(uint64_t);
   brw_load_register_mem64(brw, HSW_CS_GPR(4), query->bo, offset);
}

/*
 * R3 = R4 - R3;
 * R1 = R2 - R1;
 * R1 = R3 - R1;
 * R0 = R0 | R1;
 */
static void
calc_overflow_for_stream(struct brw_context *brw)
{
   static const uint32_t maths[] = {
      MI_MATH_ALU2(LOAD, SRCA, R4),
      MI_MATH_ALU2(LOAD, SRCB, R3),
      MI_MATH_ALU0(SUB),
      MI_MATH_ALU2(STORE, R3, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R2),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(SUB),
      MI_MATH_ALU2(STORE, R1, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R3),
      MI_MATH_ALU2(LOAD, SRCB, R1),
      MI_MATH_ALU0(SUB),
      MI_MATH_ALU2(STORE, R1, ACCU),
      MI_MATH_ALU2(LOAD, SRCA, R1),
      MI_MATH_ALU2(LOAD, SRCB, R0),
      MI_MATH_ALU0(OR),
      MI_MATH_ALU2(STORE, R0, ACCU),
   };

   BEGIN_BATCH(1 + ARRAY_SIZE(maths));
   OUT_BATCH(HSW_MI_MATH | (1 + ARRAY_SIZE(maths) - 2));

   for (int m = 0; m < ARRAY_SIZE(maths); m++)
      OUT_BATCH(maths[m]);

   ADVANCE_BATCH();
}

static void
calc_overflow_to_gpr0(struct brw_context *brw, struct brw_query_object *query,
                       int count)
{
   brw_load_register_imm64(brw, HSW_CS_GPR(0), 0ull);

   for (int i = 0; i < count; i++) {
      load_overflow_data_to_cs_gprs(brw, query, i);
      calc_overflow_for_stream(brw);
   }
}

/*
 * Take a query and calculate whether there was overflow during transform
 * feedback. Store the result in the gpr0 register.
 */
void
hsw_overflow_result_to_gpr0(struct brw_context *brw,
                            struct brw_query_object *query,
                            int count)
{
   calc_overflow_to_gpr0(brw, query, count);
   gpr0_to_bool(brw);
}

static void
hsw_result_to_gpr0(struct gl_context *ctx, struct brw_query_object *query,
                   struct gl_buffer_object *buf, intptr_t offset,
                   GLenum pname, GLenum ptype)
{
   struct brw_context *brw = brw_context(ctx);
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   assert(query->bo);
   assert(pname != GL_QUERY_TARGET);

   if (pname == GL_QUERY_RESULT_AVAILABLE) {
      /* The query result availability is stored at offset 0 of the buffer. */
      brw_load_register_mem64(brw,
                              HSW_CS_GPR(0),
                              query->bo,
                              2 * sizeof(uint64_t));
      return;
   }

   if (pname == GL_QUERY_RESULT) {
      /* Since GL_QUERY_RESULT_NO_WAIT wasn't used, they want us to stall to
       * make sure the query is available.
       */
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_CS_STALL |
                                  PIPE_CONTROL_STALL_AT_SCOREBOARD);
   }

   if (query->Base.Target == GL_TIMESTAMP) {
      brw_load_register_mem64(brw,
                              HSW_CS_GPR(0),
                              query->bo,
                              0 * sizeof(uint64_t));
   } else if (query->Base.Target == GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB
              || query->Base.Target == GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB) {
      /* Don't do anything in advance here, since the math for this is a little
       * more complex.
       */
   } else {
      brw_load_register_mem64(brw,
                              HSW_CS_GPR(1),
                              query->bo,
                              0 * sizeof(uint64_t));
      brw_load_register_mem64(brw,
                              HSW_CS_GPR(2),
                              query->bo,
                              1 * sizeof(uint64_t));

      BEGIN_BATCH(5);
      OUT_BATCH(HSW_MI_MATH | (5 - 2));

      OUT_BATCH(MI_MATH_ALU2(LOAD, SRCA, R2));
      OUT_BATCH(MI_MATH_ALU2(LOAD, SRCB, R1));
      OUT_BATCH(MI_MATH_ALU0(SUB));
      OUT_BATCH(MI_MATH_ALU2(STORE, R0, ACCU));

      ADVANCE_BATCH();
   }

   switch (query->Base.Target) {
   case GL_FRAGMENT_SHADER_INVOCATIONS_ARB:
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
         shr_gpr0_by_2_bits(brw);
      break;
   case GL_TIME_ELAPSED:
   case GL_TIMESTAMP:
      mult_gpr0_by_80(brw);
      if (query->Base.Target == GL_TIMESTAMP) {
         keep_gpr0_lower_n_bits(brw, 36);
      }
      break;
   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      gpr0_to_bool(brw);
      break;
   case GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB:
      hsw_overflow_result_to_gpr0(brw, query, 1);
      break;
   case GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB:
      hsw_overflow_result_to_gpr0(brw, query, MAX_VERTEX_STREAMS);
      break;
   }
}

/*
 * Store immediate data into the user buffer using the requested size.
 */
static void
store_query_result_imm(struct brw_context *brw, struct brw_bo *bo,
                       uint32_t offset, GLenum ptype, uint64_t imm)
{
   switch (ptype) {
   case GL_INT:
   case GL_UNSIGNED_INT:
      brw_store_data_imm32(brw, bo, offset, imm);
      break;
   case GL_INT64_ARB:
   case GL_UNSIGNED_INT64_ARB:
      brw_store_data_imm64(brw, bo, offset, imm);
      break;
   default:
      unreachable("Unexpected result type");
   }
}

static void
set_predicate(struct brw_context *brw, struct brw_bo *query_bo)
{
   brw_load_register_imm64(brw, MI_PREDICATE_SRC1, 0ull);

   /* Load query availability into SRC0 */
   brw_load_register_mem64(brw, MI_PREDICATE_SRC0, query_bo,
                           2 * sizeof(uint64_t));

   /* predicate = !(query_availability == 0); */
   BEGIN_BATCH(1);
   OUT_BATCH(GFX7_MI_PREDICATE |
             MI_PREDICATE_LOADOP_LOADINV |
             MI_PREDICATE_COMBINEOP_SET |
             MI_PREDICATE_COMPAREOP_SRCS_EQUAL);
   ADVANCE_BATCH();
}

/*
 * Store data from the register into the user buffer using the requested size.
 * The write also enables the predication to prevent writing the result if the
 * query has not finished yet.
 */
static void
store_query_result_reg(struct brw_context *brw, struct brw_bo *bo,
                       uint32_t offset, GLenum ptype, uint32_t reg,
                       const bool pipelined)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   uint32_t cmd_size = devinfo->ver >= 8 ? 4 : 3;
   uint32_t dwords = (ptype == GL_INT || ptype == GL_UNSIGNED_INT) ? 1 : 2;
   assert(devinfo->ver >= 6);

   BEGIN_BATCH(dwords * cmd_size);
   for (int i = 0; i < dwords; i++) {
      OUT_BATCH(MI_STORE_REGISTER_MEM |
                (pipelined ? MI_STORE_REGISTER_MEM_PREDICATE : 0) |
                (cmd_size - 2));
      OUT_BATCH(reg + 4 * i);
      if (devinfo->ver >= 8) {
         OUT_RELOC64(bo, RELOC_WRITE, offset + 4 * i);
      } else {
         OUT_RELOC(bo, RELOC_WRITE | RELOC_NEEDS_GGTT, offset + 4 * i);
      }
   }
   ADVANCE_BATCH();
}

static void
hsw_store_query_result(struct gl_context *ctx, struct gl_query_object *q,
                       struct gl_buffer_object *buf, intptr_t offset,
                       GLenum pname, GLenum ptype)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;
   struct brw_buffer_object *bo = brw_buffer_object(buf);
   const bool pipelined = brw_is_query_pipelined(query);

   if (pname == GL_QUERY_TARGET) {
      store_query_result_imm(brw, bo->buffer, offset, ptype,
                             query->Base.Target);
      return;
   } else if (pname == GL_QUERY_RESULT_AVAILABLE && !pipelined) {
      store_query_result_imm(brw, bo->buffer, offset, ptype, 1ull);
   } else if (query->bo) {
      /* The query bo still around. Therefore, we:
       *
       *  1. Compute the current result in GPR0
       *  2. Set the command streamer predicate based on query availability
       *  3. (With predication) Write GPR0 to the requested buffer
       */
      hsw_result_to_gpr0(ctx, query, buf, offset, pname, ptype);
      if (pipelined)
         set_predicate(brw, query->bo);
      store_query_result_reg(brw, bo->buffer, offset, ptype, HSW_CS_GPR(0),
                             pipelined);
   } else {
      /* The query bo is gone, so the query must have been processed into
       * client memory. In this case we can fill the buffer location with the
       * requested data using MI_STORE_DATA_IMM.
       */
      switch (pname) {
      case GL_QUERY_RESULT_AVAILABLE:
         store_query_result_imm(brw, bo->buffer, offset, ptype, 1ull);
         break;
      case GL_QUERY_RESULT_NO_WAIT:
      case GL_QUERY_RESULT:
         store_query_result_imm(brw, bo->buffer, offset, ptype,
                                q->Result);
         break;
      default:
         unreachable("Unexpected result type");
      }
   }

}

/* Initialize hsw+-specific query object functions. */
void hsw_init_queryobj_functions(struct dd_function_table *functions)
{
   gfx6_init_queryobj_functions(functions);
   functions->StoreQueryResult = hsw_store_query_result;
}
