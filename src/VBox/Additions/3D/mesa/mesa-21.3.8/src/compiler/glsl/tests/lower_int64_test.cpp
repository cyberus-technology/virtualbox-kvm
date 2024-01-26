/*
 * Copyright © 2013 Intel Corporation
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
#include "util/compiler.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "ir.h"
#include "ir_builder.h"

using namespace ir_builder;

namespace lower_64bit {
void expand_source(ir_factory &body,
                   ir_rvalue *val,
                   ir_variable **expanded_src);

ir_dereference_variable *compact_destination(ir_factory &body,
                                             const glsl_type *type,
                                             ir_variable *result[4]);

ir_rvalue *lower_op_to_function_call(ir_instruction *base_ir,
                                     ir_expression *ir,
                                     ir_function_signature *callee);
};

class expand_source : public ::testing::Test {
public:
   virtual void SetUp();
   virtual void TearDown();

   exec_list instructions;
   ir_factory *body;
   ir_variable *expanded_src[4];
   void *mem_ctx;
};

void
expand_source::SetUp()
{
   glsl_type_singleton_init_or_ref();

   mem_ctx = ralloc_context(NULL);

   memset(expanded_src, 0, sizeof(expanded_src));
   instructions.make_empty();
   body = new ir_factory(&instructions, mem_ctx);
}

void
expand_source::TearDown()
{
   delete body;
   body = NULL;

   ralloc_free(mem_ctx);
   mem_ctx = NULL;

   glsl_type_singleton_decref();
}

static ir_dereference_variable *
create_variable(void *mem_ctx, const glsl_type *type)
{
   ir_variable *var = new(mem_ctx) ir_variable(type,
                                               "variable",
                                               ir_var_temporary);

   return new(mem_ctx) ir_dereference_variable(var);
}

static ir_expression *
create_expression(void *mem_ctx, const glsl_type *type)
{
   return new(mem_ctx) ir_expression(ir_unop_neg,
                                     create_variable(mem_ctx, type));
}

static void
check_expanded_source(const glsl_type *type,
                      ir_variable *expanded_src[4])
{
   const glsl_type *const expanded_type =
      type->base_type == GLSL_TYPE_UINT64
      ? glsl_type::uvec2_type :glsl_type::ivec2_type;

   for (int i = 0; i < type->vector_elements; i++) {
      EXPECT_EQ(expanded_type, expanded_src[i]->type);

      /* All elements that are part of the vector must be unique. */
      for (int j = i - 1; j >= 0; j--) {
         EXPECT_NE(expanded_src[i], expanded_src[j])
            << "    Element " << i << " is the same as element " << j;
      }
   }

   /* All elements that are not part of the vector must be the same as element
    * 0.  This is primarily for scalars (where every element is the same).
    */
   for (int i = type->vector_elements; i < 4; i++) {
      EXPECT_EQ(expanded_src[0], expanded_src[i])
         << "    Element " << i << " should be the same as element 0";
   }
}

static void
check_instructions(exec_list *instructions,
                   const glsl_type *type,
                   const ir_instruction *source)
{
   const glsl_type *const expanded_type =
      type->base_type == GLSL_TYPE_UINT64
      ? glsl_type::uvec2_type : glsl_type::ivec2_type;

   const ir_expression_operation unpack_opcode =
      type->base_type == GLSL_TYPE_UINT64
      ? ir_unop_unpack_uint_2x32 : ir_unop_unpack_int_2x32;

   ir_instruction *ir;

   /* The instruction list should contain IR to represent:
    *
    *    type tmp1;
    *    tmp1 = source;
    *    uvec2 tmp2;
    *    tmp2 = unpackUint2x32(tmp1.x);
    *    uvec2 tmp3;
    *    tmp3 = unpackUint2x32(tmp1.y);
    *    uvec2 tmp4;
    *    tmp4 = unpackUint2x32(tmp1.z);
    *    uvec2 tmp5;
    *    tmp5 = unpackUint2x32(tmp1.w);
    */
   ASSERT_FALSE(instructions->is_empty());
   ir = (ir_instruction *) instructions->pop_head();
   ir_variable *const tmp1 = ir->as_variable();
   EXPECT_EQ(ir_type_variable, ir->ir_type);
   EXPECT_EQ(type, tmp1->type) <<
      "    Got " <<
      tmp1->type->name <<
      ", expected " <<
      type->name;

   ASSERT_FALSE(instructions->is_empty());
   ir = (ir_instruction *) instructions->pop_head();
   ir_assignment *const assign1 = ir->as_assignment();
   EXPECT_EQ(ir_type_assignment, ir->ir_type);
   ASSERT_NE((void *)0, assign1);
   EXPECT_EQ(tmp1, assign1->lhs->variable_referenced());
   EXPECT_EQ(source, assign1->rhs);

   for (unsigned i = 0; i < type->vector_elements; i++) {
      ASSERT_FALSE(instructions->is_empty());
      ir = (ir_instruction *) instructions->pop_head();
      ir_variable *const tmp2 = ir->as_variable();
      EXPECT_EQ(ir_type_variable, ir->ir_type);
      EXPECT_EQ(expanded_type, tmp2->type);

      ASSERT_FALSE(instructions->is_empty());
      ir = (ir_instruction *) instructions->pop_head();
      ir_assignment *const assign2 = ir->as_assignment();
      EXPECT_EQ(ir_type_assignment, ir->ir_type);
      ASSERT_NE((void *)0, assign2);
      EXPECT_EQ(tmp2, assign2->lhs->variable_referenced());
      ir_expression *unpack = assign2->rhs->as_expression();
      ASSERT_NE((void *)0, unpack);
      EXPECT_EQ(unpack_opcode, unpack->operation);
      EXPECT_EQ(tmp1, unpack->operands[0]->variable_referenced());
   }

   EXPECT_TRUE(instructions->is_empty());
}

TEST_F(expand_source, uint64_variable)
{
   const glsl_type *const type = glsl_type::uint64_t_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, u64vec2_variable)
{
   const glsl_type *const type = glsl_type::u64vec2_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, u64vec3_variable)
{
   const glsl_type *const type = glsl_type::u64vec3_type;

   /* Generate an operand that is a scalar variable dereference. */
   ir_variable *const var = new(mem_ctx) ir_variable(type,
                                                     "variable",
                                                     ir_var_temporary);

   ir_dereference_variable *const deref =
      new(mem_ctx) ir_dereference_variable(var);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, u64vec4_variable)
{
   const glsl_type *const type = glsl_type::u64vec4_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, int64_variable)
{
   const glsl_type *const type = glsl_type::int64_t_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, i64vec2_variable)
{
   const glsl_type *const type = glsl_type::i64vec2_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, i64vec3_variable)
{
   const glsl_type *const type = glsl_type::i64vec3_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, i64vec4_variable)
{
   const glsl_type *const type = glsl_type::i64vec4_type;
   ir_dereference_variable *const deref = create_variable(mem_ctx, type);

   lower_64bit::expand_source(*body, deref, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, deref);
}

TEST_F(expand_source, uint64_expression)
{
   const glsl_type *const type = glsl_type::uint64_t_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, u64vec2_expression)
{
   const glsl_type *const type = glsl_type::u64vec2_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, u64vec3_expression)
{
   const glsl_type *const type = glsl_type::u64vec3_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, u64vec4_expression)
{
   const glsl_type *const type = glsl_type::u64vec4_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, int64_expression)
{
   const glsl_type *const type = glsl_type::int64_t_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, i64vec2_expression)
{
   const glsl_type *const type = glsl_type::i64vec2_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, i64vec3_expression)
{
   const glsl_type *const type = glsl_type::i64vec3_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

TEST_F(expand_source, i64vec4_expression)
{
   const glsl_type *const type = glsl_type::i64vec4_type;
   ir_expression *const expr = create_expression(mem_ctx, type);

   lower_64bit::expand_source(*body, expr, expanded_src);

   check_expanded_source(type, expanded_src);
   check_instructions(&instructions, type, expr);
}

class compact_destination : public ::testing::Test {
public:
   virtual void SetUp();
   virtual void TearDown();

   exec_list instructions;
   ir_factory *body;
   ir_variable *expanded_src[4];
   void *mem_ctx;
};

void
compact_destination::SetUp()
{
   mem_ctx = ralloc_context(NULL);

   memset(expanded_src, 0, sizeof(expanded_src));
   instructions.make_empty();
   body = new ir_factory(&instructions, mem_ctx);
}

void
compact_destination::TearDown()
{
   delete body;
   body = NULL;

   ralloc_free(mem_ctx);
   mem_ctx = NULL;
}

TEST_F(compact_destination, uint64)
{
   const glsl_type *const type = glsl_type::uint64_t_type;

   for (unsigned i = 0; i < type->vector_elements; i++) {
      expanded_src[i] = new(mem_ctx) ir_variable(glsl_type::uvec2_type,
                                                 "result",
                                                 ir_var_temporary);
   }

   ir_dereference_variable *deref =
      lower_64bit::compact_destination(*body,
                                       type,
                                       expanded_src);

   ASSERT_EQ(ir_type_dereference_variable, deref->ir_type);
   EXPECT_EQ(type, deref->var->type) <<
      "    Got " <<
      deref->var->type->name <<
      ", expected " <<
      type->name;

   ir_instruction *ir;

   ASSERT_FALSE(instructions.is_empty());
   ir = (ir_instruction *) instructions.pop_head();
   ir_variable *const var = ir->as_variable();
   ASSERT_NE((void *)0, var);
   EXPECT_EQ(deref->var, var);

   for (unsigned i = 0; i < type->vector_elements; i++) {
      ASSERT_FALSE(instructions.is_empty());
      ir = (ir_instruction *) instructions.pop_head();
      ir_assignment *const assign = ir->as_assignment();
      ASSERT_NE((void *)0, assign);
      EXPECT_EQ(deref->var, assign->lhs->variable_referenced());
   }
}
