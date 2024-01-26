/*
 * Copyright © 2015 Intel Corporation
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

#include "compiler/brw_nir.h"
#include "compiler/glsl/ir_uniform.h"
#include "compiler/nir/nir_builder.h"
#include "brw_program.h"

static void
brw_nir_setup_glsl_builtin_uniform(nir_variable *var,
                                   const struct gl_program *prog,
                                   struct brw_stage_prog_data *stage_prog_data,
                                   bool is_scalar)
{
   const nir_state_slot *const slots = var->state_slots;
   assert(var->state_slots != NULL);

   unsigned uniform_index = var->data.driver_location / 4;
   for (unsigned int i = 0; i < var->num_state_slots; i++) {
      /* This state reference has already been setup by ir_to_mesa, but we'll
       * get the same index back here.
       */
      int index = _mesa_add_state_reference(prog->Parameters,
					    slots[i].tokens);

      /* Add each of the unique swizzles of the element as a parameter.
       * This'll end up matching the expected layout of the
       * array/matrix/structure we're trying to fill in.
       */
      int last_swiz = -1;
      for (unsigned j = 0; j < 4; j++) {
         int swiz = GET_SWZ(slots[i].swizzle, j);

         /* If we hit a pair of identical swizzles, this means we've hit the
          * end of the builtin variable.  In scalar mode, we should just quit
          * and move on to the next one.  In vec4, we need to continue and pad
          * it out to 4 components.
          */
         if (swiz == last_swiz && is_scalar)
            break;

         last_swiz = swiz;

         stage_prog_data->param[uniform_index++] =
            BRW_PARAM_PARAMETER(index, swiz);
      }
   }
}

static void
setup_vec4_image_param(uint32_t *params, uint32_t idx,
                       unsigned offset, unsigned n)
{
   assert(offset % sizeof(uint32_t) == 0);
   for (unsigned i = 0; i < n; ++i)
      params[i] = BRW_PARAM_IMAGE(idx, offset / sizeof(uint32_t) + i);

   for (unsigned i = n; i < 4; ++i)
      params[i] = BRW_PARAM_BUILTIN_ZERO;
}

static void
brw_setup_image_uniform_values(nir_variable *var,
                               struct brw_stage_prog_data *prog_data)
{
   unsigned param_start_index = var->data.driver_location / 4;
   uint32_t *param = &prog_data->param[param_start_index];
   unsigned num_images = MAX2(1, var->type->arrays_of_arrays_size());

   for (unsigned i = 0; i < num_images; i++) {
      const unsigned image_idx = var->data.binding + i;

      /* Upload the brw_image_param structure.  The order is expected to match
       * the BRW_IMAGE_PARAM_*_OFFSET defines.
       */
      setup_vec4_image_param(param + BRW_IMAGE_PARAM_OFFSET_OFFSET,
                             image_idx,
                             offsetof(brw_image_param, offset), 2);
      setup_vec4_image_param(param + BRW_IMAGE_PARAM_SIZE_OFFSET,
                             image_idx,
                             offsetof(brw_image_param, size), 3);
      setup_vec4_image_param(param + BRW_IMAGE_PARAM_STRIDE_OFFSET,
                             image_idx,
                             offsetof(brw_image_param, stride), 4);
      setup_vec4_image_param(param + BRW_IMAGE_PARAM_TILING_OFFSET,
                             image_idx,
                             offsetof(brw_image_param, tiling), 3);
      setup_vec4_image_param(param + BRW_IMAGE_PARAM_SWIZZLING_OFFSET,
                             image_idx,
                             offsetof(brw_image_param, swizzling), 2);
      param += BRW_IMAGE_PARAM_SIZE;
   }
}

static unsigned
count_uniform_storage_slots(const struct glsl_type *type)
{
   /* gl_uniform_storage can cope with one level of array, so if the
    * type is a composite type or an array where each element occupies
    * more than one slot than we need to recursively process it.
    */
   if (glsl_type_is_struct_or_ifc(type)) {
      unsigned location_count = 0;

      for (unsigned i = 0; i < glsl_get_length(type); i++) {
         const struct glsl_type *field_type = glsl_get_struct_field(type, i);

         location_count += count_uniform_storage_slots(field_type);
      }

      return location_count;
   }

   if (glsl_type_is_array(type)) {
      const struct glsl_type *element_type = glsl_get_array_element(type);

      if (glsl_type_is_array(element_type) ||
          glsl_type_is_struct_or_ifc(element_type)) {
         unsigned element_count = count_uniform_storage_slots(element_type);
         return element_count * glsl_get_length(type);
      }
   }

   return 1;
}

static void
brw_nir_setup_glsl_uniform(gl_shader_stage stage, nir_variable *var,
                           const struct gl_program *prog,
                           struct brw_stage_prog_data *stage_prog_data,
                           bool is_scalar)
{
   if (var->type->without_array()->is_sampler())
      return;

   if (var->type->without_array()->is_image()) {
      brw_setup_image_uniform_values(var, stage_prog_data);
      return;
   }

   /* The data for our (non-builtin) uniforms is stored in a series of
    * gl_uniform_storage structs for each subcomponent that
    * glGetUniformLocation() could name.  We know it's been set up in the same
    * order we'd walk the type, so walk the list of storage that matches the
    * range of slots covered by this variable.
    */
   unsigned uniform_index = var->data.driver_location / 4;
   unsigned num_slots = count_uniform_storage_slots(var->type);
   for (unsigned u = 0; u < num_slots; u++) {
      struct gl_uniform_storage *storage =
         &prog->sh.data->UniformStorage[var->data.location + u];

      /* We already handled samplers and images via the separate top-level
       * variables created by gl_nir_lower_samplers_as_deref(), but they're
       * still part of the structure's storage, and so we'll see them while
       * walking it to set up the other regular fields.  Just skip over them.
       */
      if (storage->builtin ||
          storage->type->is_sampler() ||
          storage->type->is_image())
         continue;

      gl_constant_value *components = storage->storage;
      unsigned vector_count = (MAX2(storage->array_elements, 1) *
                               storage->type->matrix_columns);
      unsigned vector_size = storage->type->vector_elements;
      unsigned max_vector_size = 4;
      if (storage->type->base_type == GLSL_TYPE_DOUBLE ||
          storage->type->base_type == GLSL_TYPE_UINT64 ||
          storage->type->base_type == GLSL_TYPE_INT64) {
         vector_size *= 2;
         if (vector_size > 4)
            max_vector_size = 8;
      }

      for (unsigned s = 0; s < vector_count; s++) {
         unsigned i;
         for (i = 0; i < vector_size; i++) {
            uint32_t idx = components - prog->sh.data->UniformDataSlots;
            stage_prog_data->param[uniform_index++] = BRW_PARAM_UNIFORM(idx);
            components++;
         }

         if (!is_scalar) {
            /* Pad out with zeros if needed (only needed for vec4) */
            for (; i < max_vector_size; i++) {
               stage_prog_data->param[uniform_index++] =
                  BRW_PARAM_BUILTIN_ZERO;
            }
         }
      }
   }
}

void
brw_nir_setup_glsl_uniforms(void *mem_ctx, nir_shader *shader,
                            const struct gl_program *prog,
                            struct brw_stage_prog_data *stage_prog_data,
                            bool is_scalar)
{
   unsigned nr_params = shader->num_uniforms / 4;
   stage_prog_data->nr_params = nr_params;
   stage_prog_data->param = rzalloc_array(mem_ctx, uint32_t, nr_params);

   nir_foreach_uniform_variable(var, shader) {
      /* UBO's, atomics and samplers don't take up space in the
         uniform file */
      if (var->interface_type != NULL || var->type->contains_atomic())
         continue;

      if (var->num_state_slots > 0) {
         brw_nir_setup_glsl_builtin_uniform(var, prog, stage_prog_data,
                                            is_scalar);
      } else {
         brw_nir_setup_glsl_uniform(shader->info.stage, var, prog,
                                    stage_prog_data, is_scalar);
      }
   }
}

void
brw_nir_setup_arb_uniforms(void *mem_ctx, nir_shader *shader,
                           struct gl_program *prog,
                           struct brw_stage_prog_data *stage_prog_data)
{
   struct gl_program_parameter_list *plist = prog->Parameters;

   unsigned nr_params = plist->NumParameters * 4;
   stage_prog_data->nr_params = nr_params;
   stage_prog_data->param = rzalloc_array(mem_ctx, uint32_t, nr_params);

   /* For ARB programs, prog_to_nir generates a single "parameters" variable
    * for all uniform data.  There may be additional sampler variables, and
    * an extra uniform from nir_lower_wpos_ytransform.
    */

   for (unsigned p = 0; p < plist->NumParameters; p++) {
      /* Parameters should be either vec4 uniforms or single component
       * constants; matrices and other larger types should have been broken
       * down earlier.
       */
      assert(plist->Parameters[p].Size <= 4);

      unsigned i;
      for (i = 0; i < plist->Parameters[p].Size; i++)
         stage_prog_data->param[4 * p + i] = BRW_PARAM_PARAMETER(p, i);
      for (; i < 4; i++)
         stage_prog_data->param[4 * p + i] = BRW_PARAM_BUILTIN_ZERO;
   }
}

static nir_ssa_def *
get_aoa_deref_offset(nir_builder *b,
                     nir_deref_instr *deref,
                     unsigned elem_size)
{
   unsigned array_size = elem_size;
   nir_ssa_def *offset = nir_imm_int(b, 0);

   while (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);

      /* This level's element size is the previous level's array size */
      nir_ssa_def *index = nir_ssa_for_src(b, deref->arr.index, 1);
      assert(deref->arr.index.ssa);
      offset = nir_iadd(b, offset,
                           nir_imul(b, index, nir_imm_int(b, array_size)));

      deref = nir_deref_instr_parent(deref);
      assert(glsl_type_is_array(deref->type));
      array_size *= glsl_get_length(deref->type);
   }

   /* Accessing an invalid surface index with the dataport can result in a
    * hang.  According to the spec "if the index used to select an individual
    * element is negative or greater than or equal to the size of the array,
    * the results of the operation are undefined but may not lead to
    * termination" -- which is one of the possible outcomes of the hang.
    * Clamp the index to prevent access outside of the array bounds.
    */
   return nir_umin(b, offset, nir_imm_int(b, array_size - elem_size));
}

void
brw_nir_lower_gl_images(nir_shader *shader,
                        const struct gl_program *prog)
{
   /* We put image uniforms at the end */
   nir_foreach_uniform_variable(var, shader) {
      if (!var->type->contains_image())
         continue;

      /* GL Only allows arrays of arrays of images */
      assert(var->type->without_array()->is_image());
      const unsigned num_images = MAX2(1, var->type->arrays_of_arrays_size());

      var->data.driver_location = shader->num_uniforms;
      shader->num_uniforms += num_images * BRW_IMAGE_PARAM_SIZE * 4;
   }

   nir_function_impl *impl = nir_shader_get_entrypoint(shader);

   nir_builder b;
   nir_builder_init(&b, impl);

   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         if (instr->type != nir_instr_type_intrinsic)
            continue;

         nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
         switch (intrin->intrinsic) {
         case nir_intrinsic_image_deref_load:
         case nir_intrinsic_image_deref_store:
         case nir_intrinsic_image_deref_atomic_add:
         case nir_intrinsic_image_deref_atomic_imin:
         case nir_intrinsic_image_deref_atomic_umin:
         case nir_intrinsic_image_deref_atomic_imax:
         case nir_intrinsic_image_deref_atomic_umax:
         case nir_intrinsic_image_deref_atomic_and:
         case nir_intrinsic_image_deref_atomic_or:
         case nir_intrinsic_image_deref_atomic_xor:
         case nir_intrinsic_image_deref_atomic_exchange:
         case nir_intrinsic_image_deref_atomic_comp_swap:
         case nir_intrinsic_image_deref_size:
         case nir_intrinsic_image_deref_samples:
         case nir_intrinsic_image_deref_load_raw_intel:
         case nir_intrinsic_image_deref_store_raw_intel: {
            nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            nir_variable *var = nir_deref_instr_get_variable(deref);

            struct gl_uniform_storage *storage =
               &prog->sh.data->UniformStorage[var->data.location];
            const unsigned image_var_idx =
               storage->opaque[shader->info.stage].index;

            b.cursor = nir_before_instr(&intrin->instr);
            nir_ssa_def *index = nir_iadd(&b, nir_imm_int(&b, image_var_idx),
                                          get_aoa_deref_offset(&b, deref, 1));
            nir_rewrite_image_intrinsic(intrin, index, false);
            break;
         }

         case nir_intrinsic_image_deref_load_param_intel: {
            nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            nir_variable *var = nir_deref_instr_get_variable(deref);
            const unsigned num_images =
               MAX2(1, var->type->arrays_of_arrays_size());

            b.cursor = nir_instr_remove(&intrin->instr);

            const unsigned param = nir_intrinsic_base(intrin);
            nir_ssa_def *offset =
               get_aoa_deref_offset(&b, deref, BRW_IMAGE_PARAM_SIZE * 4);
            offset = nir_iadd(&b, offset, nir_imm_int(&b, param * 16));

            nir_intrinsic_instr *load =
               nir_intrinsic_instr_create(b.shader,
                                          nir_intrinsic_load_uniform);
            nir_intrinsic_set_base(load, var->data.driver_location);
            nir_intrinsic_set_range(load, num_images * BRW_IMAGE_PARAM_SIZE * 4);
            load->src[0] = nir_src_for_ssa(offset);
            load->num_components = intrin->dest.ssa.num_components;
            nir_ssa_dest_init(&load->instr, &load->dest,
                              intrin->dest.ssa.num_components,
                              intrin->dest.ssa.bit_size, NULL);
            nir_builder_instr_insert(&b, &load->instr);

            nir_ssa_def_rewrite_uses(&intrin->dest.ssa,
                                     &load->dest.ssa);
            break;
         }

         default:
            break;
         }
      }
   }
}

void
brw_nir_lower_legacy_clipping(nir_shader *nir, int nr_userclip_plane_consts,
                              struct brw_stage_prog_data *prog_data)
{
   if (nr_userclip_plane_consts == 0)
      return;

   nir_function_impl *impl = nir_shader_get_entrypoint(nir);

   nir_lower_clip_vs(nir, (1 << nr_userclip_plane_consts) - 1, true, false,
                     NULL);
   nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);

   const unsigned clip_plane_base = nir->num_uniforms;

   assert(nir->num_uniforms == prog_data->nr_params * 4);
   const unsigned num_clip_floats = 4 * nr_userclip_plane_consts;
   uint32_t *clip_param =
      brw_stage_prog_data_add_params(prog_data, num_clip_floats);
   nir->num_uniforms += num_clip_floats * sizeof(float);
   assert(nir->num_uniforms == prog_data->nr_params * 4);

   for (unsigned i = 0; i < num_clip_floats; i++)
      clip_param[i] = BRW_PARAM_BUILTIN_CLIP_PLANE(i / 4, i % 4);

   nir_builder b;
   nir_builder_init(&b, impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         if (instr->type != nir_instr_type_intrinsic)
            continue;

         nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
         if (intrin->intrinsic != nir_intrinsic_load_user_clip_plane)
            continue;

         b.cursor = nir_before_instr(instr);

         nir_intrinsic_instr *load =
            nir_intrinsic_instr_create(nir, nir_intrinsic_load_uniform);
         load->num_components = 4;
         load->src[0] = nir_src_for_ssa(nir_imm_int(&b, 0));
         nir_ssa_dest_init(&load->instr, &load->dest, 4, 32, NULL);
         nir_intrinsic_set_base(load, clip_plane_base + 4 * sizeof(float) *
                                      nir_intrinsic_ucp_id(intrin));
         nir_intrinsic_set_range(load, 4 * sizeof(float));
         nir_builder_instr_insert(&b, &load->instr);

         nir_ssa_def_rewrite_uses(&intrin->dest.ssa,
                                  &load->dest.ssa);
         nir_instr_remove(instr);
      }
   }
}
