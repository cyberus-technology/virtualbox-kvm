/*
 * Copyright © 2020 Intel Corporation
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

class nir_opt_lower_returns_test : public ::testing::Test {
protected:
   nir_opt_lower_returns_test();
   ~nir_opt_lower_returns_test();

   nir_builder bld;

   nir_ssa_def *in_def;
};

nir_opt_lower_returns_test::nir_opt_lower_returns_test()
{
   glsl_type_singleton_init_or_ref();

   static const nir_shader_compiler_options options = { };
   bld = nir_builder_init_simple_shader(MESA_SHADER_VERTEX, &options, "lower returns test");

   nir_variable *var = nir_variable_create(bld.shader, nir_var_shader_in, glsl_int_type(), "in");
   in_def = nir_load_var(&bld, var);
}

nir_opt_lower_returns_test::~nir_opt_lower_returns_test()
{
   ralloc_free(bld.shader);
   glsl_type_singleton_decref();
}

nir_phi_instr *create_one_source_phi(nir_shader *shader, nir_block *pred,
                                     nir_ssa_def *def)
{
   nir_phi_instr *phi = nir_phi_instr_create(shader);

   nir_phi_instr_add_src(phi, pred, nir_src_for_ssa(def));

   nir_ssa_dest_init(&phi->instr, &phi->dest,
                     def->num_components, def->bit_size, NULL);

   return phi;
}

TEST_F(nir_opt_lower_returns_test, phis_after_loop)
{
   /* Test that after lowering of "return" the phis in block_5
    * have two sources, because block_2 will have block_5
    * as a successor.
    *
    *  block block_0:
    *  loop {
    *     block block_1:
    *     if ssa_2 {
    *       block block_2:
    *       return
    *       // succs: block_6
    *     } else {
    *       block block_3:
    *       break;
    *       // succs: block_5
    *     }
    *     block block_4:
    *  }
    *  block block_5:
    *  // preds: block_3
    *  vec1 32 ssa_4 = phi block_3: ssa_1
    *  vec1 32 ssa_5 = phi block_3: ssa_1
    *  // succs: block_6
    *  block block_6:
    */

   nir_loop *loop = nir_push_loop(&bld);

   nir_ssa_def *one = nir_imm_int(&bld, 1);

   nir_ssa_def *cmp_result = nir_ieq(&bld, in_def, one);
   nir_if *nif = nir_push_if(&bld, cmp_result);

   nir_jump(&bld, nir_jump_return);

   nir_push_else(&bld, NULL);

   nir_jump(&bld, nir_jump_break);

   nir_pop_if(&bld, NULL);

   nir_block *else_block = nir_if_last_else_block(nif);

   nir_pop_loop(&bld, loop);

   bld.cursor = nir_after_cf_node_and_phis(&loop->cf_node);

   nir_phi_instr *const phi_1 =
      create_one_source_phi(bld.shader, else_block, one);
   nir_builder_instr_insert(&bld, &phi_1->instr);

   nir_phi_instr *const phi_2 =
      create_one_source_phi(bld.shader, else_block, one);
   nir_builder_instr_insert(&bld, &phi_2->instr);

   ASSERT_TRUE(nir_lower_returns(bld.shader));
   EXPECT_EQ(phi_1->srcs.length(), 2);
   EXPECT_EQ(phi_2->srcs.length(), 2);

   nir_validate_shader(bld.shader, NULL);
}

TEST_F(nir_opt_lower_returns_test, phis_after_outer_loop)
{
   /* Test that after lowering of "return" the phis in block_7
    * have two sources, because block_6 will have a conditional break
    * inserted, which will add a new predcessor to block_7.
    *
    *  block block_0:
    *  loop {
    *     block block_1:
    *     loop {
    *        block block_2:
    *        if ssa_2 {
    *          block block_3:
    *          return
    *          // succs: block_8
    *        } else {
    *          block block_4:
    *          break;
    *          // succs: block_6
    *        }
    *        block block_5:
    *     }
    *     block block_6:
    *     break;
    *     // succs: block_7
    *  }
    *  block block_7:
    *  // preds: block_6
    *  vec1 32 ssa_4 = phi block_6: ssa_1
    *  vec1 32 ssa_5 = phi block_6: ssa_1
    *  // succs: block_8
    *  block block_8:
    */

   nir_loop *loop_outer = nir_push_loop(&bld);

   bld.cursor = nir_after_cf_list(&loop_outer->body);

   nir_loop *loop_inner = nir_push_loop(&bld);

   bld.cursor = nir_after_cf_list(&loop_inner->body);

   nir_ssa_def *one = nir_imm_int(&bld, 1);

   nir_ssa_def *cmp_result = nir_ieq(&bld, in_def, one);
   nir_push_if(&bld, cmp_result);

   nir_jump(&bld, nir_jump_return);

   nir_push_else(&bld, NULL);

   nir_jump(&bld, nir_jump_break);

   nir_pop_if(&bld, NULL);

   nir_pop_loop(&bld, loop_inner);

   bld.cursor = nir_after_cf_node_and_phis(&loop_inner->cf_node);

   nir_jump(&bld, nir_jump_break);

   nir_pop_loop(&bld, loop_outer);

   bld.cursor = nir_after_cf_node_and_phis(&loop_outer->cf_node);

   nir_phi_instr *const phi_1 =
      create_one_source_phi(bld.shader, nir_loop_last_block(loop_outer), one);
   nir_builder_instr_insert(&bld, &phi_1->instr);

   nir_phi_instr *const phi_2 =
      create_one_source_phi(bld.shader, nir_loop_last_block(loop_outer), one);
   nir_builder_instr_insert(&bld, &phi_2->instr);

   ASSERT_TRUE(nir_lower_returns(bld.shader));
   EXPECT_EQ(phi_1->srcs.length(), 2);
   EXPECT_EQ(phi_2->srcs.length(), 2);

   nir_validate_shader(bld.shader, NULL);
}
