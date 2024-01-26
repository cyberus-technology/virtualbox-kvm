/*
 * Copyright © 2018 Intel Corporation
 * Copyright © 2021 Valve Corporation
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

#include <gtest/gtest.h>

#include "nir.h"
#include "nir_builder.h"

namespace {

class algebraic_test_base : public ::testing::Test {
protected:
   algebraic_test_base();
   ~algebraic_test_base();

   virtual void run_pass()=0;

   void test_op(nir_op op, nir_ssa_def *src0, nir_ssa_def *src1, nir_ssa_def *src2,
                nir_ssa_def *src3, const char *desc);

   void test_2src_op(nir_op op, int64_t src0, int64_t src1);

   nir_variable *res_var;
   nir_builder *b, _b;
};

algebraic_test_base::algebraic_test_base()
{
   glsl_type_singleton_init_or_ref();

   static const nir_shader_compiler_options options = { };
   _b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, &options, "opt_algebraic test");
   b = &_b;

   res_var = nir_local_variable_create(b->impl, glsl_int_type(), "res");
}

algebraic_test_base::~algebraic_test_base()
{
   ralloc_free(b->shader);

   glsl_type_singleton_decref();
}

void algebraic_test_base::test_op(nir_op op, nir_ssa_def *src0, nir_ssa_def *src1,
                                     nir_ssa_def *src2, nir_ssa_def *src3, const char *desc)
{
   nir_ssa_def *res_deref = &nir_build_deref_var(b, res_var)->dest.ssa;

   /* create optimized expression */
   nir_intrinsic_instr *optimized_instr = nir_build_store_deref(
      b, res_deref, nir_build_alu(b, op, src0, src1, src2, src3), 0x1);

   run_pass();
   b->cursor = nir_after_cf_list(&b->impl->body);

   /* create reference expression */
   nir_intrinsic_instr *ref_instr = nir_build_store_deref(
      b, res_deref, nir_build_alu(b, op, src0, src1, src2, src3), 0x1);

   /* test equality */
   nir_opt_constant_folding(b->shader);

   ASSERT_TRUE(nir_src_is_const(ref_instr->src[1]));
   ASSERT_TRUE(nir_src_is_const(optimized_instr->src[1]));

   int32_t ref = nir_src_as_int(ref_instr->src[1]);
   int32_t optimized = nir_src_as_int(optimized_instr->src[1]);

   EXPECT_EQ(ref, optimized) << "Test input: " << desc;

   /* reset shader */
   exec_list_make_empty(&nir_start_block(b->impl)->instr_list);
   b->cursor = nir_after_cf_list(&b->impl->body);
}

void algebraic_test_base::test_2src_op(nir_op op, int64_t src0, int64_t src1)
{
   char desc[128];
   snprintf(desc, sizeof(desc), "%s(%" PRId64 ", %" PRId64 ")", nir_op_infos[op].name, src0, src1);
   test_op(op, nir_imm_int(b, src0), nir_imm_int(b, src1), NULL, NULL, desc);
}

class nir_opt_algebraic_test : public algebraic_test_base {
protected:
   virtual void run_pass() {
      nir_opt_algebraic(b->shader);
   }
};

class nir_opt_idiv_const_test : public algebraic_test_base {
protected:
   virtual void run_pass() {
      nir_opt_idiv_const(b->shader, 8);
   }
};

TEST_F(nir_opt_algebraic_test, umod_pow2_src2)
{
   for (int i = 0; i <= 9; i++)
      test_2src_op(nir_op_umod, i, 4);
   test_2src_op(nir_op_umod, UINT32_MAX, 4);
}

TEST_F(nir_opt_algebraic_test, imod_pow2_src2)
{
   for (int i = -9; i <= 9; i++) {
      test_2src_op(nir_op_imod, i, 4);
      test_2src_op(nir_op_imod, i, -4);
      test_2src_op(nir_op_imod, i, INT32_MIN);
   }
   test_2src_op(nir_op_imod, INT32_MAX, 4);
   test_2src_op(nir_op_imod, INT32_MAX, -4);
   test_2src_op(nir_op_imod, INT32_MIN, 4);
   test_2src_op(nir_op_imod, INT32_MIN, -4);
   test_2src_op(nir_op_imod, INT32_MIN, INT32_MIN);
}

TEST_F(nir_opt_algebraic_test, irem_pow2_src2)
{
   for (int i = -9; i <= 9; i++) {
      test_2src_op(nir_op_irem, i, 4);
      test_2src_op(nir_op_irem, i, -4);
   }
   test_2src_op(nir_op_irem, INT32_MAX, 4);
   test_2src_op(nir_op_irem, INT32_MAX, -4);
   test_2src_op(nir_op_irem, INT32_MIN, 4);
   test_2src_op(nir_op_irem, INT32_MIN, -4);
}

TEST_F(nir_opt_idiv_const_test, umod)
{
   for (uint32_t d : {16u, 17u, 0u, UINT32_MAX}) {
      for (int i = 0; i <= 40; i++)
         test_2src_op(nir_op_umod, i, d);
      for (int i = 0; i < 20; i++)
         test_2src_op(nir_op_umod, UINT32_MAX - i, d);
   }
}

TEST_F(nir_opt_idiv_const_test, imod)
{
   for (int32_t d : {16, -16, 17, -17, 0, INT32_MIN, INT32_MAX}) {
      for (int i = -40; i <= 40; i++)
         test_2src_op(nir_op_imod, i, d);
      for (int i = 0; i < 20; i++)
         test_2src_op(nir_op_imod, INT32_MIN + i, d);
      for (int i = 0; i < 20; i++)
         test_2src_op(nir_op_imod, INT32_MAX - i, d);
   }
}

TEST_F(nir_opt_idiv_const_test, irem)
{
   for (int32_t d : {16, -16, 17, -17, 0, INT32_MIN, INT32_MAX}) {
      for (int i = -40; i <= 40; i++)
         test_2src_op(nir_op_irem, i, d);
      for (int i = 0; i < 20; i++)
         test_2src_op(nir_op_irem, INT32_MIN + i, d);
      for (int i = 0; i < 20; i++)
         test_2src_op(nir_op_irem, INT32_MAX - i, d);
   }
}

}
