/*
 * Copyright © 2017 Intel Corporation
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

#include "anv_nir.h"
#include "nir/nir_builder.h"
#include "compiler/brw_compiler.h"

static bool
anv_nir_add_base_work_group_id_instr(nir_builder *b,
                                     nir_instr *instr,
                                     UNUSED void *cb_data)
{
   if (instr->type != nir_instr_type_intrinsic)
      return false;

   nir_intrinsic_instr *load_id = nir_instr_as_intrinsic(instr);
   if (load_id->intrinsic != nir_intrinsic_load_workgroup_id)
      return false;

   b->cursor = nir_after_instr(&load_id->instr);

   nir_ssa_def *load_base =
      nir_load_push_constant(b, 3, 32, nir_imm_int(b, 0),
                             .base = offsetof(struct anv_push_constants, cs.base_work_group_id),
                             .range = 3 * sizeof(uint32_t));

   nir_ssa_def *id = nir_iadd(b, &load_id->dest.ssa, load_base);

   nir_ssa_def_rewrite_uses_after(&load_id->dest.ssa, id, id->parent_instr);
   return true;
}

bool
anv_nir_add_base_work_group_id(nir_shader *shader)
{
   assert(shader->info.stage == MESA_SHADER_COMPUTE);

   return nir_shader_instructions_pass(shader,
                                       anv_nir_add_base_work_group_id_instr,
                                       nir_metadata_block_index |
                                       nir_metadata_dominance,
                                       NULL);
}
