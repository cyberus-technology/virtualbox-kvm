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
#include "util/ralloc.h"
#include "ir.h"
#include "util/hash_table.h"

/**
 * \file varyings_test.cpp
 *
 * Test various aspects of linking shader stage inputs and outputs.
 */

namespace linker {
void
populate_consumer_input_sets(void *mem_ctx, exec_list *ir,
                             hash_table *consumer_inputs,
                             hash_table *consumer_interface_inputs,
                             ir_variable *consumer_inputs_with_locations[VARYING_SLOT_MAX]);

ir_variable *
get_matching_input(void *mem_ctx,
                   const ir_variable *output_var,
                   hash_table *consumer_inputs,
                   hash_table *consumer_interface_inputs,
                   ir_variable *consumer_inputs_with_locations[VARYING_SLOT_MAX]);
}

class link_varyings : public ::testing::Test {
public:
   virtual void SetUp();
   virtual void TearDown();

   char *interface_field_name(const glsl_type *iface, unsigned field = 0)
   {
      return ralloc_asprintf(mem_ctx,
                             "%s.%s",
                             iface->name,
                             iface->fields.structure[field].name);
   }

   void *mem_ctx;
   exec_list ir;
   hash_table *consumer_inputs;
   hash_table *consumer_interface_inputs;

   const glsl_type *simple_interface;
   ir_variable *junk[VARYING_SLOT_TESS_MAX];
};

void
link_varyings::SetUp()
{
   glsl_type_singleton_init_or_ref();

   this->mem_ctx = ralloc_context(NULL);
   this->ir.make_empty();

   this->consumer_inputs =
         _mesa_hash_table_create(NULL, _mesa_hash_string,
                                 _mesa_key_string_equal);

   this->consumer_interface_inputs =
         _mesa_hash_table_create(NULL, _mesa_hash_string,
                                 _mesa_key_string_equal);

   /* Needs to happen after glsl type initialization */
   static const glsl_struct_field f[] = {
      glsl_struct_field(glsl_type::vec(4), "v")
   };

   this->simple_interface =
      glsl_type::get_interface_instance(f,
                                        ARRAY_SIZE(f),
                                        GLSL_INTERFACE_PACKING_STD140,
                                        false,
                                        "simple_interface");
}

void
link_varyings::TearDown()
{
   ralloc_free(this->mem_ctx);
   this->mem_ctx = NULL;

   _mesa_hash_table_destroy(this->consumer_inputs, NULL);
   this->consumer_inputs = NULL;
   _mesa_hash_table_destroy(this->consumer_interface_inputs, NULL);
   this->consumer_interface_inputs = NULL;

   glsl_type_singleton_decref();
}

TEST_F(link_varyings, single_simple_input)
{
   ir_variable *const v =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "a",
                               ir_var_shader_in);


   ir.push_tail(v);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);

   hash_entry *entry = _mesa_hash_table_search(consumer_inputs, "a");
   EXPECT_EQ((void *) v, entry->data);
   EXPECT_EQ(1u, consumer_inputs->entries);
   EXPECT_TRUE(consumer_interface_inputs->entries == 0);
}

TEST_F(link_varyings, gl_ClipDistance)
{
   const glsl_type *const array_8_of_float =
      glsl_type::get_array_instance(glsl_type::vec(1), 8);

   ir_variable *const clipdistance =
      new(mem_ctx) ir_variable(array_8_of_float,
                               "gl_ClipDistance",
                               ir_var_shader_in);

   clipdistance->data.explicit_location = true;
   clipdistance->data.location = VARYING_SLOT_CLIP_DIST0;
   clipdistance->data.explicit_index = 0;

   ir.push_tail(clipdistance);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);

   EXPECT_EQ(clipdistance, junk[VARYING_SLOT_CLIP_DIST0]);
   EXPECT_TRUE(consumer_inputs->entries == 0);
   EXPECT_TRUE(consumer_interface_inputs->entries == 0);
}

TEST_F(link_varyings, gl_CullDistance)
{
   const glsl_type *const array_8_of_float =
      glsl_type::get_array_instance(glsl_type::vec(1), 8);

   ir_variable *const culldistance =
      new(mem_ctx) ir_variable(array_8_of_float,
                               "gl_CullDistance",
                               ir_var_shader_in);

   culldistance->data.explicit_location = true;
   culldistance->data.location = VARYING_SLOT_CULL_DIST0;
   culldistance->data.explicit_index = 0;

   ir.push_tail(culldistance);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);

   EXPECT_EQ(culldistance, junk[VARYING_SLOT_CULL_DIST0]);
   EXPECT_TRUE(consumer_inputs->entries == 0);
   EXPECT_TRUE(consumer_interface_inputs->entries == 0);
}

TEST_F(link_varyings, single_interface_input)
{
   ir_variable *const v =
      new(mem_ctx) ir_variable(simple_interface->fields.structure[0].type,
                               simple_interface->fields.structure[0].name,
                               ir_var_shader_in);

   v->init_interface_type(simple_interface);

   ir.push_tail(v);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);
   char *const full_name = interface_field_name(simple_interface);

   hash_entry *entry = _mesa_hash_table_search(consumer_interface_inputs,
                                               full_name);
   EXPECT_EQ((void *) v, entry->data);
   EXPECT_EQ(1u, consumer_interface_inputs->entries);
   EXPECT_TRUE(consumer_inputs->entries == 0);
}

TEST_F(link_varyings, one_interface_and_one_simple_input)
{
   ir_variable *const v =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "a",
                               ir_var_shader_in);


   ir.push_tail(v);

   ir_variable *const iface =
      new(mem_ctx) ir_variable(simple_interface->fields.structure[0].type,
                               simple_interface->fields.structure[0].name,
                               ir_var_shader_in);

   iface->init_interface_type(simple_interface);

   ir.push_tail(iface);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);

   char *const iface_field_name = interface_field_name(simple_interface);

   hash_entry *entry = _mesa_hash_table_search(consumer_interface_inputs,
                                               iface_field_name);
   EXPECT_EQ((void *) iface, entry->data);
   EXPECT_EQ(1u, consumer_interface_inputs->entries);

   entry = _mesa_hash_table_search(consumer_inputs, "a");
   EXPECT_EQ((void *) v, entry->data);
   EXPECT_EQ(1u, consumer_inputs->entries);
}

TEST_F(link_varyings, interface_field_doesnt_match_noninterface)
{
   char *const iface_field_name = interface_field_name(simple_interface);

   /* The input shader has a single input variable name "a.v"
    */
   ir_variable *const in_v =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               iface_field_name,
                               ir_var_shader_in);

   ir.push_tail(in_v);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);

   /* Create an output variable, "v", that is part of an interface block named
    * "a".  They should not match.
    */
   ir_variable *const out_v =
      new(mem_ctx) ir_variable(simple_interface->fields.structure[0].type,
                               simple_interface->fields.structure[0].name,
                               ir_var_shader_in);

   out_v->init_interface_type(simple_interface);

   ir_variable *const match =
      linker::get_matching_input(mem_ctx,
                                 out_v,
                                 consumer_inputs,
                                 consumer_interface_inputs,
                                 junk);

   EXPECT_EQ(NULL, match);
}

TEST_F(link_varyings, interface_field_doesnt_match_noninterface_vice_versa)
{
   char *const iface_field_name = interface_field_name(simple_interface);

   /* In input shader has a single variable, "v", that is part of an interface
    * block named "a".
    */
   ir_variable *const in_v =
      new(mem_ctx) ir_variable(simple_interface->fields.structure[0].type,
                               simple_interface->fields.structure[0].name,
                               ir_var_shader_in);

   in_v->init_interface_type(simple_interface);

   ir.push_tail(in_v);

   linker::populate_consumer_input_sets(mem_ctx,
                                        &ir,
                                        consumer_inputs,
                                        consumer_interface_inputs,
                                        junk);

   /* Create an output variable "a.v".  They should not match.
    */
   ir_variable *const out_v =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               iface_field_name,
                               ir_var_shader_out);

   ir_variable *const match =
      linker::get_matching_input(mem_ctx,
                                 out_v,
                                 consumer_inputs,
                                 consumer_interface_inputs,
                                 junk);

   EXPECT_EQ(NULL, match);
}
