/*
 * Copyright © 2014 Intel Corporation
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

#include "brw_nir.h"
#include "brw_shader.h"
#include "dev/intel_debug.h"
#include "compiler/glsl_types.h"
#include "compiler/nir/nir_builder.h"
#include "util/u_math.h"

static bool
remap_tess_levels(nir_builder *b, nir_intrinsic_instr *intr,
                  GLenum primitive_mode)
{
   const int location = nir_intrinsic_base(intr);
   const unsigned component = nir_intrinsic_component(intr);
   bool out_of_bounds;

   if (location == VARYING_SLOT_TESS_LEVEL_INNER) {
      switch (primitive_mode) {
      case GL_QUADS:
         /* gl_TessLevelInner[0..1] lives at DWords 3-2 (reversed). */
         nir_intrinsic_set_base(intr, 0);
         nir_intrinsic_set_component(intr, 3 - component);
         out_of_bounds = false;
         break;
      case GL_TRIANGLES:
         /* gl_TessLevelInner[0] lives at DWord 4. */
         nir_intrinsic_set_base(intr, 1);
         out_of_bounds = component > 0;
         break;
      case GL_ISOLINES:
         out_of_bounds = true;
         break;
      default:
         unreachable("Bogus tessellation domain");
      }
   } else if (location == VARYING_SLOT_TESS_LEVEL_OUTER) {
      if (primitive_mode == GL_ISOLINES) {
         /* gl_TessLevelOuter[0..1] lives at DWords 6-7 (in order). */
         nir_intrinsic_set_base(intr, 1);
         nir_intrinsic_set_component(intr, 2 + nir_intrinsic_component(intr));
         out_of_bounds = component > 1;
      } else {
         /* Triangles use DWords 7-5 (reversed); Quads use 7-4 (reversed) */
         nir_intrinsic_set_base(intr, 1);
         nir_intrinsic_set_component(intr, 3 - nir_intrinsic_component(intr));
         out_of_bounds = component == 3 && primitive_mode == GL_TRIANGLES;
      }
   } else {
      return false;
   }

   if (out_of_bounds) {
      if (nir_intrinsic_infos[intr->intrinsic].has_dest) {
         b->cursor = nir_before_instr(&intr->instr);
         nir_ssa_def *undef = nir_ssa_undef(b, 1, 32);
         nir_ssa_def_rewrite_uses(&intr->dest.ssa, undef);
      }
      nir_instr_remove(&intr->instr);
   }

   return true;
}

static bool
is_input(nir_intrinsic_instr *intrin)
{
   return intrin->intrinsic == nir_intrinsic_load_input ||
          intrin->intrinsic == nir_intrinsic_load_per_vertex_input ||
          intrin->intrinsic == nir_intrinsic_load_interpolated_input;
}

static bool
is_output(nir_intrinsic_instr *intrin)
{
   return intrin->intrinsic == nir_intrinsic_load_output ||
          intrin->intrinsic == nir_intrinsic_load_per_vertex_output ||
          intrin->intrinsic == nir_intrinsic_store_output ||
          intrin->intrinsic == nir_intrinsic_store_per_vertex_output;
}


static bool
remap_patch_urb_offsets(nir_block *block, nir_builder *b,
                        const struct brw_vue_map *vue_map,
                        GLenum tes_primitive_mode)
{
   const bool is_passthrough_tcs = b->shader->info.name &&
      strcmp(b->shader->info.name, "passthrough TCS") == 0;

   nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
         continue;

      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);

      gl_shader_stage stage = b->shader->info.stage;

      if ((stage == MESA_SHADER_TESS_CTRL && is_output(intrin)) ||
          (stage == MESA_SHADER_TESS_EVAL && is_input(intrin))) {

         if (!is_passthrough_tcs &&
             remap_tess_levels(b, intrin, tes_primitive_mode))
            continue;

         int vue_slot = vue_map->varying_to_slot[intrin->const_index[0]];
         assert(vue_slot != -1);
         intrin->const_index[0] = vue_slot;

         nir_src *vertex = nir_get_io_vertex_index_src(intrin);
         if (vertex) {
            if (nir_src_is_const(*vertex)) {
               intrin->const_index[0] += nir_src_as_uint(*vertex) *
                                         vue_map->num_per_vertex_slots;
            } else {
               b->cursor = nir_before_instr(&intrin->instr);

               /* Multiply by the number of per-vertex slots. */
               nir_ssa_def *vertex_offset =
                  nir_imul(b,
                           nir_ssa_for_src(b, *vertex, 1),
                           nir_imm_int(b,
                                       vue_map->num_per_vertex_slots));

               /* Add it to the existing offset */
               nir_src *offset = nir_get_io_offset_src(intrin);
               nir_ssa_def *total_offset =
                  nir_iadd(b, vertex_offset,
                           nir_ssa_for_src(b, *offset, 1));

               nir_instr_rewrite_src(&intrin->instr, offset,
                                     nir_src_for_ssa(total_offset));
            }
         }
      }
   }
   return true;
}

void
brw_nir_lower_vs_inputs(nir_shader *nir,
                        bool edgeflag_is_last,
                        const uint8_t *vs_attrib_wa_flags)
{
   /* Start with the location of the variable's base. */
   nir_foreach_shader_in_variable(var, nir)
      var->data.driver_location = var->data.location;

   /* Now use nir_lower_io to walk dereference chains.  Attribute arrays are
    * loaded as one vec4 or dvec4 per element (or matrix column), depending on
    * whether it is a double-precision type or not.
    */
   nir_lower_io(nir, nir_var_shader_in, type_size_vec4,
                nir_lower_io_lower_64bit_to_32);

   /* This pass needs actual constants */
   nir_opt_constant_folding(nir);

   nir_io_add_const_offset_to_base(nir, nir_var_shader_in);

   brw_nir_apply_attribute_workarounds(nir, vs_attrib_wa_flags);

   /* The last step is to remap VERT_ATTRIB_* to actual registers */

   /* Whether or not we have any system generated values.  gl_DrawID is not
    * included here as it lives in its own vec4.
    */
   const bool has_sgvs =
      BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_FIRST_VERTEX) ||
      BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BASE_INSTANCE) ||
      BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_VERTEX_ID_ZERO_BASE) ||
      BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_INSTANCE_ID);

   const unsigned num_inputs = util_bitcount64(nir->info.inputs_read);

   nir_foreach_function(function, nir) {
      if (!function->impl)
         continue;

      nir_builder b;
      nir_builder_init(&b, function->impl);

      nir_foreach_block(block, function->impl) {
         nir_foreach_instr_safe(instr, block) {
            if (instr->type != nir_instr_type_intrinsic)
               continue;

            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);

            switch (intrin->intrinsic) {
            case nir_intrinsic_load_first_vertex:
            case nir_intrinsic_load_base_instance:
            case nir_intrinsic_load_vertex_id_zero_base:
            case nir_intrinsic_load_instance_id:
            case nir_intrinsic_load_is_indexed_draw:
            case nir_intrinsic_load_draw_id: {
               b.cursor = nir_after_instr(&intrin->instr);

               /* gl_VertexID and friends are stored by the VF as the last
                * vertex element.  We convert them to load_input intrinsics at
                * the right location.
                */
               nir_intrinsic_instr *load =
                  nir_intrinsic_instr_create(nir, nir_intrinsic_load_input);
               load->src[0] = nir_src_for_ssa(nir_imm_int(&b, 0));

               nir_intrinsic_set_base(load, num_inputs);
               switch (intrin->intrinsic) {
               case nir_intrinsic_load_first_vertex:
                  nir_intrinsic_set_component(load, 0);
                  break;
               case nir_intrinsic_load_base_instance:
                  nir_intrinsic_set_component(load, 1);
                  break;
               case nir_intrinsic_load_vertex_id_zero_base:
                  nir_intrinsic_set_component(load, 2);
                  break;
               case nir_intrinsic_load_instance_id:
                  nir_intrinsic_set_component(load, 3);
                  break;
               case nir_intrinsic_load_draw_id:
               case nir_intrinsic_load_is_indexed_draw:
                  /* gl_DrawID and IsIndexedDraw are stored right after
                   * gl_VertexID and friends if any of them exist.
                   */
                  nir_intrinsic_set_base(load, num_inputs + has_sgvs);
                  if (intrin->intrinsic == nir_intrinsic_load_draw_id)
                     nir_intrinsic_set_component(load, 0);
                  else
                     nir_intrinsic_set_component(load, 1);
                  break;
               default:
                  unreachable("Invalid system value intrinsic");
               }

               load->num_components = 1;
               nir_ssa_dest_init(&load->instr, &load->dest, 1, 32, NULL);
               nir_builder_instr_insert(&b, &load->instr);

               nir_ssa_def_rewrite_uses(&intrin->dest.ssa,
                                        &load->dest.ssa);
               nir_instr_remove(&intrin->instr);
               break;
            }

            case nir_intrinsic_load_input: {
               /* Attributes come in a contiguous block, ordered by their
                * gl_vert_attrib value.  That means we can compute the slot
                * number for an attribute by masking out the enabled attributes
                * before it and counting the bits.
                */
               int attr = nir_intrinsic_base(intrin);
               uint64_t inputs_read = nir->info.inputs_read;
               int slot = -1;
               if (edgeflag_is_last) {
                  inputs_read &= ~BITFIELD64_BIT(VERT_ATTRIB_EDGEFLAG);
                  if (attr == VERT_ATTRIB_EDGEFLAG)
                     slot = num_inputs - 1;
               }
               if (slot == -1)
                  slot = util_bitcount64(inputs_read &
                                         BITFIELD64_MASK(attr));
               nir_intrinsic_set_base(intrin, slot);
               break;
            }

            default:
               break; /* Nothing to do */
            }
         }
      }
   }
}

void
brw_nir_lower_vue_inputs(nir_shader *nir,
                         const struct brw_vue_map *vue_map)
{
   nir_foreach_shader_in_variable(var, nir)
      var->data.driver_location = var->data.location;

   /* Inputs are stored in vec4 slots, so use type_size_vec4(). */
   nir_lower_io(nir, nir_var_shader_in, type_size_vec4,
                nir_lower_io_lower_64bit_to_32);

   /* This pass needs actual constants */
   nir_opt_constant_folding(nir);

   nir_io_add_const_offset_to_base(nir, nir_var_shader_in);

   nir_foreach_function(function, nir) {
      if (!function->impl)
         continue;

      nir_foreach_block(block, function->impl) {
         nir_foreach_instr(instr, block) {
            if (instr->type != nir_instr_type_intrinsic)
               continue;

            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);

            if (intrin->intrinsic == nir_intrinsic_load_input ||
                intrin->intrinsic == nir_intrinsic_load_per_vertex_input) {
               /* Offset 0 is the VUE header, which contains
                * VARYING_SLOT_LAYER [.y], VARYING_SLOT_VIEWPORT [.z], and
                * VARYING_SLOT_PSIZ [.w].
                */
               int varying = nir_intrinsic_base(intrin);
               int vue_slot;
               switch (varying) {
               case VARYING_SLOT_PSIZ:
                  nir_intrinsic_set_base(intrin, 0);
                  nir_intrinsic_set_component(intrin, 3);
                  break;

               default:
                  vue_slot = vue_map->varying_to_slot[varying];
                  assert(vue_slot != -1);
                  nir_intrinsic_set_base(intrin, vue_slot);
                  break;
               }
            }
         }
      }
   }
}

void
brw_nir_lower_tes_inputs(nir_shader *nir, const struct brw_vue_map *vue_map)
{
   nir_foreach_shader_in_variable(var, nir)
      var->data.driver_location = var->data.location;

   nir_lower_io(nir, nir_var_shader_in, type_size_vec4,
                nir_lower_io_lower_64bit_to_32);

   /* This pass needs actual constants */
   nir_opt_constant_folding(nir);

   nir_io_add_const_offset_to_base(nir, nir_var_shader_in);

   nir_foreach_function(function, nir) {
      if (function->impl) {
         nir_builder b;
         nir_builder_init(&b, function->impl);
         nir_foreach_block(block, function->impl) {
            remap_patch_urb_offsets(block, &b, vue_map,
                                    nir->info.tess.primitive_mode);
         }
      }
   }
}

/**
 * Convert interpolateAtOffset() offsets from [-0.5, +0.5] floating point
 * offsets to integer [-8, +7] offsets (in units of 1/16th of a pixel).
 *
 * We clamp to +7/16 on the upper end of the range, since +0.5 isn't
 * representable in a S0.4 value; a naive conversion would give us -8/16,
 * which is the opposite of what was intended.
 *
 * This is allowed by GL_ARB_gpu_shader5's quantization rules:
 *
 *    "Not all values of <offset> may be supported; x and y offsets may
 *     be rounded to fixed-point values with the number of fraction bits
 *     given by the implementation-dependent constant
 *     FRAGMENT_INTERPOLATION_OFFSET_BITS."
 */
static bool
lower_barycentric_at_offset(nir_builder *b, nir_instr *instr, void *data)
{
   if (instr->type != nir_instr_type_intrinsic)
      return false;

   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);

   if (intrin->intrinsic != nir_intrinsic_load_barycentric_at_offset)
      return false;

   b->cursor = nir_before_instr(instr);

   assert(intrin->src[0].ssa);
   nir_ssa_def *offset =
      nir_imin(b, nir_imm_int(b, 7),
               nir_f2i32(b, nir_fmul(b, nir_imm_float(b, 16),
                                     intrin->src[0].ssa)));

   nir_instr_rewrite_src(instr, &intrin->src[0], nir_src_for_ssa(offset));

   return true;
}

void
brw_nir_lower_fs_inputs(nir_shader *nir,
                        const struct intel_device_info *devinfo,
                        const struct brw_wm_prog_key *key)
{
   nir_foreach_shader_in_variable(var, nir) {
      var->data.driver_location = var->data.location;

      /* Apply default interpolation mode.
       *
       * Everything defaults to smooth except for the legacy GL color
       * built-in variables, which might be flat depending on API state.
       */
      if (var->data.interpolation == INTERP_MODE_NONE) {
         const bool flat = key->flat_shade &&
            (var->data.location == VARYING_SLOT_COL0 ||
             var->data.location == VARYING_SLOT_COL1);

         var->data.interpolation = flat ? INTERP_MODE_FLAT
                                        : INTERP_MODE_SMOOTH;
      }

      /* On Ironlake and below, there is only one interpolation mode.
       * Centroid interpolation doesn't mean anything on this hardware --
       * there is no multisampling.
       */
      if (devinfo->ver < 6) {
         var->data.centroid = false;
         var->data.sample = false;
      }
   }

   nir_lower_io_options lower_io_options = nir_lower_io_lower_64bit_to_32;
   if (key->persample_interp)
      lower_io_options |= nir_lower_io_force_sample_interpolation;

   nir_lower_io(nir, nir_var_shader_in, type_size_vec4, lower_io_options);
   if (devinfo->ver >= 11)
      nir_lower_interpolation(nir, ~0);

   nir_shader_instructions_pass(nir, lower_barycentric_at_offset,
                                nir_metadata_block_index |
                                nir_metadata_dominance,
                                NULL);

   /* This pass needs actual constants */
   nir_opt_constant_folding(nir);

   nir_io_add_const_offset_to_base(nir, nir_var_shader_in);
}

void
brw_nir_lower_vue_outputs(nir_shader *nir)
{
   nir_foreach_shader_out_variable(var, nir) {
      var->data.driver_location = var->data.location;
   }

   nir_lower_io(nir, nir_var_shader_out, type_size_vec4,
                nir_lower_io_lower_64bit_to_32);
}

void
brw_nir_lower_tcs_outputs(nir_shader *nir, const struct brw_vue_map *vue_map,
                          GLenum tes_primitive_mode)
{
   nir_foreach_shader_out_variable(var, nir) {
      var->data.driver_location = var->data.location;
   }

   nir_lower_io(nir, nir_var_shader_out, type_size_vec4,
                nir_lower_io_lower_64bit_to_32);

   /* This pass needs actual constants */
   nir_opt_constant_folding(nir);

   nir_io_add_const_offset_to_base(nir, nir_var_shader_out);

   nir_foreach_function(function, nir) {
      if (function->impl) {
         nir_builder b;
         nir_builder_init(&b, function->impl);
         nir_foreach_block(block, function->impl) {
            remap_patch_urb_offsets(block, &b, vue_map, tes_primitive_mode);
         }
      }
   }
}

void
brw_nir_lower_fs_outputs(nir_shader *nir)
{
   nir_foreach_shader_out_variable(var, nir) {
      var->data.driver_location =
         SET_FIELD(var->data.index, BRW_NIR_FRAG_OUTPUT_INDEX) |
         SET_FIELD(var->data.location, BRW_NIR_FRAG_OUTPUT_LOCATION);
   }

   nir_lower_io(nir, nir_var_shader_out, type_size_dvec4, 0);
}

#define OPT(pass, ...) ({                                  \
   bool this_progress = false;                             \
   NIR_PASS(this_progress, nir, pass, ##__VA_ARGS__);      \
   if (this_progress)                                      \
      progress = true;                                     \
   this_progress;                                          \
})

void
brw_nir_optimize(nir_shader *nir, const struct brw_compiler *compiler,
                 bool is_scalar, bool allow_copies)
{
   bool progress;
   unsigned lower_flrp =
      (nir->options->lower_flrp16 ? 16 : 0) |
      (nir->options->lower_flrp32 ? 32 : 0) |
      (nir->options->lower_flrp64 ? 64 : 0);

   do {
      progress = false;
      OPT(nir_split_array_vars, nir_var_function_temp);
      OPT(nir_shrink_vec_array_vars, nir_var_function_temp);
      OPT(nir_opt_deref);
      OPT(nir_lower_vars_to_ssa);
      if (allow_copies) {
         /* Only run this pass in the first call to brw_nir_optimize.  Later
          * calls assume that we've lowered away any copy_deref instructions
          * and we don't want to introduce any more.
          */
         OPT(nir_opt_find_array_copies);
      }
      OPT(nir_opt_copy_prop_vars);
      OPT(nir_opt_dead_write_vars);
      OPT(nir_opt_combine_stores, nir_var_all);

      if (is_scalar) {
         OPT(nir_lower_alu_to_scalar, NULL, NULL);
      } else {
         OPT(nir_opt_shrink_vectors, true);
      }

      OPT(nir_copy_prop);

      if (is_scalar) {
         OPT(nir_lower_phis_to_scalar, false);
      }

      OPT(nir_copy_prop);
      OPT(nir_opt_dce);
      OPT(nir_opt_cse);
      OPT(nir_opt_combine_stores, nir_var_all);

      /* Passing 0 to the peephole select pass causes it to convert
       * if-statements that contain only move instructions in the branches
       * regardless of the count.
       *
       * Passing 1 to the peephole select pass causes it to convert
       * if-statements that contain at most a single ALU instruction (total)
       * in both branches.  Before Gfx6, some math instructions were
       * prohibitively expensive and the results of compare operations need an
       * extra resolve step.  For these reasons, this pass is more harmful
       * than good on those platforms.
       *
       * For indirect loads of uniforms (push constants), we assume that array
       * indices will nearly always be in bounds and the cost of the load is
       * low.  Therefore there shouldn't be a performance benefit to avoid it.
       * However, in vec4 tessellation shaders, these loads operate by
       * actually pulling from memory.
       */
      const bool is_vec4_tessellation = !is_scalar &&
         (nir->info.stage == MESA_SHADER_TESS_CTRL ||
          nir->info.stage == MESA_SHADER_TESS_EVAL);
      OPT(nir_opt_peephole_select, 0, !is_vec4_tessellation, false);
      OPT(nir_opt_peephole_select, 8, !is_vec4_tessellation,
          compiler->devinfo->ver >= 6);

      OPT(nir_opt_intrinsics);
      OPT(nir_opt_idiv_const, 32);
      OPT(nir_opt_algebraic);
      OPT(nir_opt_constant_folding);

      if (lower_flrp != 0) {
         if (OPT(nir_lower_flrp,
                 lower_flrp,
                 false /* always_precise */)) {
            OPT(nir_opt_constant_folding);
         }

         /* Nothing should rematerialize any flrps, so we only need to do this
          * lowering once.
          */
         lower_flrp = 0;
      }

      OPT(nir_opt_dead_cf);
      if (OPT(nir_opt_trivial_continues)) {
         /* If nir_opt_trivial_continues makes progress, then we need to clean
          * things up if we want any hope of nir_opt_if or nir_opt_loop_unroll
          * to make progress.
          */
         OPT(nir_copy_prop);
         OPT(nir_opt_dce);
      }
      OPT(nir_opt_if, false);
      OPT(nir_opt_conditional_discard);
      if (nir->options->max_unroll_iterations != 0) {
         OPT(nir_opt_loop_unroll);
      }
      OPT(nir_opt_remove_phis);
      OPT(nir_opt_gcm, false);
      OPT(nir_opt_undef);
      OPT(nir_lower_pack);
   } while (progress);

   /* Workaround Gfxbench unused local sampler variable which will trigger an
    * assert in the opt_large_constants pass.
    */
   OPT(nir_remove_dead_variables, nir_var_function_temp, NULL);
}

static unsigned
lower_bit_size_callback(const nir_instr *instr, UNUSED void *data)
{
   const struct brw_compiler *compiler = (const struct brw_compiler *) data;
   const struct intel_device_info *devinfo = compiler->devinfo;

   switch (instr->type) {
   case nir_instr_type_alu: {
      nir_alu_instr *alu = nir_instr_as_alu(instr);
      assert(alu->dest.dest.is_ssa);
      if (alu->dest.dest.ssa.bit_size >= 32)
         return 0;

      /* Note: nir_op_iabs and nir_op_ineg are not lowered here because the
       * 8-bit ABS or NEG instruction should eventually get copy propagated
       * into the MOV that does the type conversion.  This results in far
       * fewer MOV instructions.
       */
      switch (alu->op) {
      case nir_op_idiv:
      case nir_op_imod:
      case nir_op_irem:
      case nir_op_udiv:
      case nir_op_umod:
      case nir_op_fceil:
      case nir_op_ffloor:
      case nir_op_ffract:
      case nir_op_fround_even:
      case nir_op_ftrunc:
         return 32;
      case nir_op_frcp:
      case nir_op_frsq:
      case nir_op_fsqrt:
      case nir_op_fpow:
      case nir_op_fexp2:
      case nir_op_flog2:
      case nir_op_fsin:
      case nir_op_fcos:
         return devinfo->ver < 9 ? 32 : 0;
      case nir_op_isign:
         assert(!"Should have been lowered by nir_opt_algebraic.");
         return 0;
      default:
         if (nir_op_infos[alu->op].num_inputs >= 2 &&
             alu->dest.dest.ssa.bit_size == 8)
            return 16;

         if (nir_alu_instr_is_comparison(alu) &&
             alu->src[0].src.ssa->bit_size == 8)
            return 16;

         return 0;
      }
      break;
   }

   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      switch (intrin->intrinsic) {
      case nir_intrinsic_read_invocation:
      case nir_intrinsic_read_first_invocation:
      case nir_intrinsic_vote_feq:
      case nir_intrinsic_vote_ieq:
      case nir_intrinsic_shuffle:
      case nir_intrinsic_shuffle_xor:
      case nir_intrinsic_shuffle_up:
      case nir_intrinsic_shuffle_down:
      case nir_intrinsic_quad_broadcast:
      case nir_intrinsic_quad_swap_horizontal:
      case nir_intrinsic_quad_swap_vertical:
      case nir_intrinsic_quad_swap_diagonal:
         if (intrin->src[0].ssa->bit_size == 8)
            return 16;
         return 0;

      case nir_intrinsic_reduce:
      case nir_intrinsic_inclusive_scan:
      case nir_intrinsic_exclusive_scan:
         /* There are a couple of register region issues that make things
          * complicated for 8-bit types:
          *
          *    1. Only raw moves are allowed to write to a packed 8-bit
          *       destination.
          *    2. If we use a strided destination, the efficient way to do
          *       scan operations ends up using strides that are too big to
          *       encode in an instruction.
          *
          * To get around these issues, we just do all 8-bit scan operations
          * in 16 bits.  It's actually fewer instructions than what we'd have
          * to do if we were trying to do it in native 8-bit types and the
          * results are the same once we truncate to 8 bits at the end.
          */
         if (intrin->dest.ssa.bit_size == 8)
            return 16;
         return 0;

      default:
         return 0;
      }
      break;
   }

   case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
      if (phi->dest.ssa.bit_size == 8)
         return 16;
      return 0;
   }

   default:
      return 0;
   }
}

/* Does some simple lowering and runs the standard suite of optimizations
 *
 * This is intended to be called more-or-less directly after you get the
 * shader out of GLSL or some other source.  While it is geared towards i965,
 * it is not at all generator-specific except for the is_scalar flag.  Even
 * there, it is safe to call with is_scalar = false for a shader that is
 * intended for the FS backend as long as nir_optimize is called again with
 * is_scalar = true to scalarize everything prior to code gen.
 */
void
brw_preprocess_nir(const struct brw_compiler *compiler, nir_shader *nir,
                   const nir_shader *softfp64)
{
   const struct intel_device_info *devinfo = compiler->devinfo;
   UNUSED bool progress; /* Written by OPT */

   const bool is_scalar = compiler->scalar_stage[nir->info.stage];

   nir_validate_ssa_dominance(nir, "before brw_preprocess_nir");

   if (is_scalar) {
      OPT(nir_lower_alu_to_scalar, NULL, NULL);
   }

   if (nir->info.stage == MESA_SHADER_GEOMETRY)
      OPT(nir_lower_gs_intrinsics, 0);

   /* See also brw_nir_trig_workarounds.py */
   if (compiler->precise_trig &&
       !(devinfo->ver >= 10 || devinfo->is_kabylake))
      OPT(brw_nir_apply_trig_workarounds);

   if (devinfo->ver >= 12)
      OPT(brw_nir_clamp_image_1d_2d_array_sizes);

   const nir_lower_tex_options tex_options = {
      .lower_txp = ~0,
      .lower_txf_offset = true,
      .lower_rect_offset = true,
      .lower_txd_cube_map = true,
      .lower_txd_3d = devinfo->verx10 >= 125,
      .lower_txb_shadow_clamp = true,
      .lower_txd_shadow_clamp = true,
      .lower_txd_offset_clamp = true,
      .lower_tg4_offsets = true,
      .lower_txs_lod = true, /* Wa_14012320009 */
   };

   OPT(nir_lower_tex, &tex_options);
   OPT(nir_normalize_cubemap_coords);

   OPT(nir_lower_global_vars_to_local);

   OPT(nir_split_var_copies);
   OPT(nir_split_struct_vars, nir_var_function_temp);

   brw_nir_optimize(nir, compiler, is_scalar, true);

   OPT(nir_lower_doubles, softfp64, nir->options->lower_doubles_options);
   OPT(nir_lower_int64);

   OPT(nir_lower_bit_size, lower_bit_size_callback, (void *)compiler);

   if (is_scalar) {
      OPT(nir_lower_load_const_to_scalar);
   }

   /* Lower a bunch of stuff */
   OPT(nir_lower_var_copies);

   /* This needs to be run after the first optimization pass but before we
    * lower indirect derefs away
    */
   if (compiler->supports_shader_constants) {
      OPT(nir_opt_large_constants, NULL, 32);
   }

   OPT(nir_lower_system_values);
   OPT(nir_lower_compute_system_values, NULL);

   const nir_lower_subgroups_options subgroups_options = {
      .ballot_bit_size = 32,
      .ballot_components = 1,
      .lower_to_scalar = true,
      .lower_vote_trivial = !is_scalar,
      .lower_shuffle = true,
      .lower_quad_broadcast_dynamic = true,
      .lower_elect = true,
   };
   OPT(nir_lower_subgroups, &subgroups_options);

   OPT(nir_lower_clip_cull_distance_arrays);

   nir_variable_mode indirect_mask =
      brw_nir_no_indirect_mask(compiler, nir->info.stage);
   OPT(nir_lower_indirect_derefs, indirect_mask, UINT32_MAX);

   /* Even in cases where we can handle indirect temporaries via scratch, we
    * it can still be expensive.  Lower indirects on small arrays to
    * conditional load/stores.
    *
    * The threshold of 16 was chosen semi-arbitrarily.  The idea is that an
    * indirect on an array of 16 elements is about 30 instructions at which
    * point, you may be better off doing a send.  With a SIMD8 program, 16
    * floats is 1/8 of the entire register file.  Any array larger than that
    * is likely to cause pressure issues.  Also, this value is sufficiently
    * high that the benchmarks known to suffer from large temporary array
    * issues are helped but nothing else in shader-db is hurt except for maybe
    * that one kerbal space program shader.
    */
   if (is_scalar && !(indirect_mask & nir_var_function_temp))
      OPT(nir_lower_indirect_derefs, nir_var_function_temp, 16);

   /* Lower array derefs of vectors for SSBO and UBO loads.  For both UBOs and
    * SSBOs, our back-end is capable of loading an entire vec4 at a time and
    * we would like to take advantage of that whenever possible regardless of
    * whether or not the app gives us full loads.  This should allow the
    * optimizer to combine UBO and SSBO load operations and save us some send
    * messages.
    */
   OPT(nir_lower_array_deref_of_vec,
       nir_var_mem_ubo | nir_var_mem_ssbo,
       nir_lower_direct_array_deref_of_vec_load);

   /* Get rid of split copies */
   brw_nir_optimize(nir, compiler, is_scalar, false);
}

void
brw_nir_link_shaders(const struct brw_compiler *compiler,
                     nir_shader *producer, nir_shader *consumer)
{
   nir_lower_io_arrays_to_elements(producer, consumer);
   nir_validate_shader(producer, "after nir_lower_io_arrays_to_elements");
   nir_validate_shader(consumer, "after nir_lower_io_arrays_to_elements");

   const bool p_is_scalar = compiler->scalar_stage[producer->info.stage];
   const bool c_is_scalar = compiler->scalar_stage[consumer->info.stage];

   if (p_is_scalar && c_is_scalar) {
      NIR_PASS_V(producer, nir_lower_io_to_scalar_early, nir_var_shader_out);
      NIR_PASS_V(consumer, nir_lower_io_to_scalar_early, nir_var_shader_in);
      brw_nir_optimize(producer, compiler, p_is_scalar, false);
      brw_nir_optimize(consumer, compiler, c_is_scalar, false);
   }

   if (nir_link_opt_varyings(producer, consumer))
      brw_nir_optimize(consumer, compiler, c_is_scalar, false);

   NIR_PASS_V(producer, nir_remove_dead_variables, nir_var_shader_out, NULL);
   NIR_PASS_V(consumer, nir_remove_dead_variables, nir_var_shader_in, NULL);

   if (nir_remove_unused_varyings(producer, consumer)) {
      NIR_PASS_V(producer, nir_lower_global_vars_to_local);
      NIR_PASS_V(consumer, nir_lower_global_vars_to_local);

      /* The backend might not be able to handle indirects on
       * temporaries so we need to lower indirects on any of the
       * varyings we have demoted here.
       */
      NIR_PASS_V(producer, nir_lower_indirect_derefs,
                 brw_nir_no_indirect_mask(compiler, producer->info.stage),
                 UINT32_MAX);
      NIR_PASS_V(consumer, nir_lower_indirect_derefs,
                 brw_nir_no_indirect_mask(compiler, consumer->info.stage),
                 UINT32_MAX);

      brw_nir_optimize(producer, compiler, p_is_scalar, false);
      brw_nir_optimize(consumer, compiler, c_is_scalar, false);
   }

   NIR_PASS_V(producer, nir_lower_io_to_vector, nir_var_shader_out);
   NIR_PASS_V(producer, nir_opt_combine_stores, nir_var_shader_out);
   NIR_PASS_V(consumer, nir_lower_io_to_vector, nir_var_shader_in);

   if (producer->info.stage != MESA_SHADER_TESS_CTRL) {
      /* Calling lower_io_to_vector creates output variable writes with
       * write-masks.  On non-TCS outputs, the back-end can't handle it and we
       * need to call nir_lower_io_to_temporaries to get rid of them.  This,
       * in turn, creates temporary variables and extra copy_deref intrinsics
       * that we need to clean up.
       */
      NIR_PASS_V(producer, nir_lower_io_to_temporaries,
                 nir_shader_get_entrypoint(producer), true, false);
      NIR_PASS_V(producer, nir_lower_global_vars_to_local);
      NIR_PASS_V(producer, nir_split_var_copies);
      NIR_PASS_V(producer, nir_lower_var_copies);
   }
}

static bool
brw_nir_should_vectorize_mem(unsigned align_mul, unsigned align_offset,
                             unsigned bit_size,
                             unsigned num_components,
                             nir_intrinsic_instr *low,
                             nir_intrinsic_instr *high,
                             void *data)
{
   /* Don't combine things to generate 64-bit loads/stores.  We have to split
    * those back into 32-bit ones anyway and UBO loads aren't split in NIR so
    * we don't want to make a mess for the back-end.
    */
   if (bit_size > 32)
      return false;

   /* We can handle at most a vec4 right now.  Anything bigger would get
    * immediately split by brw_nir_lower_mem_access_bit_sizes anyway.
    */
   if (num_components > 4)
      return false;


   uint32_t align;
   if (align_offset)
      align = 1 << (ffs(align_offset) - 1);
   else
      align = align_mul;

   if (align < bit_size / 8)
      return false;

   return true;
}

static
bool combine_all_barriers(nir_intrinsic_instr *a,
                          nir_intrinsic_instr *b,
                          void *data)
{
   /* Translation to backend IR will get rid of modes we don't care about, so
    * no harm in always combining them.
    *
    * TODO: While HW has only ACQUIRE|RELEASE fences, we could improve the
    * scheduling so that it can take advantage of the different semantics.
    */
   nir_intrinsic_set_memory_modes(a, nir_intrinsic_memory_modes(a) |
                                     nir_intrinsic_memory_modes(b));
   nir_intrinsic_set_memory_semantics(a, nir_intrinsic_memory_semantics(a) |
                                         nir_intrinsic_memory_semantics(b));
   nir_intrinsic_set_memory_scope(a, MAX2(nir_intrinsic_memory_scope(a),
                                          nir_intrinsic_memory_scope(b)));
   return true;
}

static void
brw_vectorize_lower_mem_access(nir_shader *nir,
                               const struct brw_compiler *compiler,
                               bool is_scalar,
                               bool robust_buffer_access)
{
   const struct intel_device_info *devinfo = compiler->devinfo;
   bool progress = false;

   if (is_scalar) {
      nir_load_store_vectorize_options options = {
         .modes = nir_var_mem_ubo | nir_var_mem_ssbo |
                  nir_var_mem_global | nir_var_mem_shared,
         .callback = brw_nir_should_vectorize_mem,
         .robust_modes = (nir_variable_mode)0,
      };

      if (robust_buffer_access) {
         options.robust_modes = nir_var_mem_ubo | nir_var_mem_ssbo |
                                nir_var_mem_global;
      }

      OPT(nir_opt_load_store_vectorize, &options);
   }

   OPT(brw_nir_lower_mem_access_bit_sizes, devinfo);

   while (progress) {
      progress = false;

      OPT(nir_lower_pack);
      OPT(nir_copy_prop);
      OPT(nir_opt_dce);
      OPT(nir_opt_cse);
      OPT(nir_opt_algebraic);
      OPT(nir_opt_constant_folding);
   }
}

static bool
nir_shader_has_local_variables(const nir_shader *nir)
{
   nir_foreach_function(func, nir) {
      if (func->impl && !exec_list_is_empty(&func->impl->locals))
         return true;
   }

   return false;
}

/* Prepare the given shader for codegen
 *
 * This function is intended to be called right before going into the actual
 * backend and is highly backend-specific.  Also, once this function has been
 * called on a shader, it will no longer be in SSA form so most optimizations
 * will not work.
 */
void
brw_postprocess_nir(nir_shader *nir, const struct brw_compiler *compiler,
                    bool is_scalar, bool debug_enabled,
                    bool robust_buffer_access)
{
   const struct intel_device_info *devinfo = compiler->devinfo;

   UNUSED bool progress; /* Written by OPT */

   OPT(nir_lower_bit_size, lower_bit_size_callback, (void *)compiler);

   OPT(brw_nir_lower_scoped_barriers);
   OPT(nir_opt_combine_memory_barriers, combine_all_barriers, NULL);

   do {
      progress = false;
      OPT(nir_opt_algebraic_before_ffma);
   } while (progress);

   if (devinfo->verx10 >= 125) {
      const nir_lower_idiv_options options = {
         .imprecise_32bit_lowering = false,
         .allow_fp16 = false
      };
      OPT(nir_lower_idiv, &options);
   }

   brw_nir_optimize(nir, compiler, is_scalar, false);

   if (is_scalar && nir_shader_has_local_variables(nir)) {
      OPT(nir_lower_vars_to_explicit_types, nir_var_function_temp,
          glsl_get_natural_size_align_bytes);
      OPT(nir_lower_explicit_io, nir_var_function_temp,
          nir_address_format_32bit_offset);
      brw_nir_optimize(nir, compiler, is_scalar, false);
   }

   brw_vectorize_lower_mem_access(nir, compiler, is_scalar,
                                  robust_buffer_access);

   if (OPT(nir_lower_int64))
      brw_nir_optimize(nir, compiler, is_scalar, false);

   if (devinfo->ver >= 6) {
      /* Try and fuse multiply-adds */
      OPT(brw_nir_opt_peephole_ffma);
   }

   if (OPT(nir_opt_comparison_pre)) {
      OPT(nir_copy_prop);
      OPT(nir_opt_dce);
      OPT(nir_opt_cse);

      /* Do the select peepehole again.  nir_opt_comparison_pre (combined with
       * the other optimization passes) will have removed at least one
       * instruction from one of the branches of the if-statement, so now it
       * might be under the threshold of conversion to bcsel.
       *
       * See brw_nir_optimize for the explanation of is_vec4_tessellation.
       */
      const bool is_vec4_tessellation = !is_scalar &&
         (nir->info.stage == MESA_SHADER_TESS_CTRL ||
          nir->info.stage == MESA_SHADER_TESS_EVAL);
      OPT(nir_opt_peephole_select, 0, is_vec4_tessellation, false);
      OPT(nir_opt_peephole_select, 1, is_vec4_tessellation,
          compiler->devinfo->ver >= 6);
   }

   do {
      progress = false;
      if (OPT(nir_opt_algebraic_late)) {
         /* At this late stage, anything that makes more constants will wreak
          * havok on the vec4 backend.  The handling of constants in the vec4
          * backend is not good.
          */
         if (is_scalar)
            OPT(nir_opt_constant_folding);

         OPT(nir_copy_prop);
         OPT(nir_opt_dce);
         OPT(nir_opt_cse);
      }
   } while (progress);


   OPT(brw_nir_lower_conversions);

   if (is_scalar)
      OPT(nir_lower_alu_to_scalar, NULL, NULL);

   while (OPT(nir_opt_algebraic_distribute_src_mods)) {
      OPT(nir_copy_prop);
      OPT(nir_opt_dce);
      OPT(nir_opt_cse);
   }

   OPT(nir_copy_prop);
   OPT(nir_opt_dce);
   OPT(nir_opt_move, nir_move_comparisons);
   OPT(nir_opt_dead_cf);

   OPT(nir_lower_bool_to_int32);
   OPT(nir_copy_prop);
   OPT(nir_opt_dce);

   OPT(nir_lower_locals_to_regs);

   if (unlikely(debug_enabled)) {
      /* Re-index SSA defs so we print more sensible numbers. */
      nir_foreach_function(function, nir) {
         if (function->impl)
            nir_index_ssa_defs(function->impl);
      }

      fprintf(stderr, "NIR (SSA form) for %s shader:\n",
              _mesa_shader_stage_to_string(nir->info.stage));
      nir_print_shader(nir, stderr);
   }

   nir_validate_ssa_dominance(nir, "before nir_convert_from_ssa");

   OPT(nir_convert_from_ssa, true);

   if (!is_scalar) {
      OPT(nir_move_vec_src_uses_to_dest);
      OPT(nir_lower_vec_to_movs, NULL, NULL);
   }

   OPT(nir_opt_dce);

   if (OPT(nir_opt_rematerialize_compares))
      OPT(nir_opt_dce);

   /* This is the last pass we run before we start emitting stuff.  It
    * determines when we need to insert boolean resolves on Gen <= 5.  We
    * run it last because it stashes data in instr->pass_flags and we don't
    * want that to be squashed by other NIR passes.
    */
   if (devinfo->ver <= 5)
      brw_nir_analyze_boolean_resolves(nir);

   nir_sweep(nir);

   if (unlikely(debug_enabled)) {
      fprintf(stderr, "NIR (final form) for %s shader:\n",
              _mesa_shader_stage_to_string(nir->info.stage));
      nir_print_shader(nir, stderr);
   }
}

static bool
brw_nir_apply_sampler_key(nir_shader *nir,
                          const struct brw_compiler *compiler,
                          const struct brw_sampler_prog_key_data *key_tex)
{
   const struct intel_device_info *devinfo = compiler->devinfo;
   nir_lower_tex_options tex_options = {
      .lower_txd_clamp_bindless_sampler = true,
      .lower_txd_clamp_if_sampler_index_not_lt_16 = true,
   };

   /* Iron Lake and prior require lowering of all rectangle textures */
   if (devinfo->ver < 6)
      tex_options.lower_rect = true;

   /* Prior to Broadwell, our hardware can't actually do GL_CLAMP */
   if (devinfo->ver < 8) {
      tex_options.saturate_s = key_tex->gl_clamp_mask[0];
      tex_options.saturate_t = key_tex->gl_clamp_mask[1];
      tex_options.saturate_r = key_tex->gl_clamp_mask[2];
   }

   /* Prior to Haswell, we have to fake texture swizzle */
   for (unsigned s = 0; s < MAX_SAMPLERS; s++) {
      if (key_tex->swizzles[s] == SWIZZLE_NOOP)
         continue;

      tex_options.swizzle_result |= BITFIELD_BIT(s);
      for (unsigned c = 0; c < 4; c++)
         tex_options.swizzles[s][c] = GET_SWZ(key_tex->swizzles[s], c);
   }

   /* Prior to Haswell, we have to lower gradients on shadow samplers */
   tex_options.lower_txd_shadow = devinfo->verx10 <= 70;

   tex_options.lower_y_uv_external = key_tex->y_uv_image_mask;
   tex_options.lower_y_u_v_external = key_tex->y_u_v_image_mask;
   tex_options.lower_yx_xuxv_external = key_tex->yx_xuxv_image_mask;
   tex_options.lower_xy_uxvx_external = key_tex->xy_uxvx_image_mask;
   tex_options.lower_ayuv_external = key_tex->ayuv_image_mask;
   tex_options.lower_xyuv_external = key_tex->xyuv_image_mask;
   tex_options.bt709_external = key_tex->bt709_mask;
   tex_options.bt2020_external = key_tex->bt2020_mask;

   /* Setup array of scaling factors for each texture. */
   memcpy(&tex_options.scale_factors, &key_tex->scale_factors,
          sizeof(tex_options.scale_factors));

   return nir_lower_tex(nir, &tex_options);
}

static unsigned
get_subgroup_size(gl_shader_stage stage,
                  const struct brw_base_prog_key *key,
                  unsigned max_subgroup_size)
{
   switch (key->subgroup_size_type) {
   case BRW_SUBGROUP_SIZE_API_CONSTANT:
      /* We have to use the global constant size. */
      return BRW_SUBGROUP_SIZE;

   case BRW_SUBGROUP_SIZE_UNIFORM:
      /* It has to be uniform across all invocations but can vary per stage
       * if we want.  This gives us a bit more freedom.
       *
       * For compute, brw_nir_apply_key is called per-dispatch-width so this
       * is the actual subgroup size and not a maximum.  However, we only
       * invoke one size of any given compute shader so it's still guaranteed
       * to be uniform across invocations.
       */
      return max_subgroup_size;

   case BRW_SUBGROUP_SIZE_VARYING:
      /* The subgroup size is allowed to be fully varying.  For geometry
       * stages, we know it's always 8 which is max_subgroup_size so we can
       * return that.  For compute, brw_nir_apply_key is called once per
       * dispatch-width so max_subgroup_size is the real subgroup size.
       *
       * For fragment, we return 0 and let it fall through to the back-end
       * compiler.  This means we can't optimize based on subgroup size but
       * that's a risk the client took when it asked for a varying subgroup
       * size.
       */
      return stage == MESA_SHADER_FRAGMENT ? 0 : max_subgroup_size;

   case BRW_SUBGROUP_SIZE_REQUIRE_8:
   case BRW_SUBGROUP_SIZE_REQUIRE_16:
   case BRW_SUBGROUP_SIZE_REQUIRE_32:
      assert(stage == MESA_SHADER_COMPUTE);
      /* These enum values are expressly chosen to be equal to the subgroup
       * size that they require.
       */
      return key->subgroup_size_type;
   }

   unreachable("Invalid subgroup size type");
}

void
brw_nir_apply_key(nir_shader *nir,
                  const struct brw_compiler *compiler,
                  const struct brw_base_prog_key *key,
                  unsigned max_subgroup_size,
                  bool is_scalar)
{
   bool progress = false;

   OPT(brw_nir_apply_sampler_key, compiler, &key->tex);

   const nir_lower_subgroups_options subgroups_options = {
      .subgroup_size = get_subgroup_size(nir->info.stage, key,
                                         max_subgroup_size),
      .ballot_bit_size = 32,
      .ballot_components = 1,
      .lower_subgroup_masks = true,
   };
   OPT(nir_lower_subgroups, &subgroups_options);

   if (progress)
      brw_nir_optimize(nir, compiler, is_scalar, false);
}

enum brw_conditional_mod
brw_cmod_for_nir_comparison(nir_op op)
{
   switch (op) {
   case nir_op_flt:
   case nir_op_flt32:
   case nir_op_ilt:
   case nir_op_ilt32:
   case nir_op_ult:
   case nir_op_ult32:
      return BRW_CONDITIONAL_L;

   case nir_op_fge:
   case nir_op_fge32:
   case nir_op_ige:
   case nir_op_ige32:
   case nir_op_uge:
   case nir_op_uge32:
      return BRW_CONDITIONAL_GE;

   case nir_op_feq:
   case nir_op_feq32:
   case nir_op_ieq:
   case nir_op_ieq32:
   case nir_op_b32all_fequal2:
   case nir_op_b32all_iequal2:
   case nir_op_b32all_fequal3:
   case nir_op_b32all_iequal3:
   case nir_op_b32all_fequal4:
   case nir_op_b32all_iequal4:
      return BRW_CONDITIONAL_Z;

   case nir_op_fneu:
   case nir_op_fneu32:
   case nir_op_ine:
   case nir_op_ine32:
   case nir_op_b32any_fnequal2:
   case nir_op_b32any_inequal2:
   case nir_op_b32any_fnequal3:
   case nir_op_b32any_inequal3:
   case nir_op_b32any_fnequal4:
   case nir_op_b32any_inequal4:
      return BRW_CONDITIONAL_NZ;

   default:
      unreachable("Unsupported NIR comparison op");
   }
}

uint32_t
brw_aop_for_nir_intrinsic(const nir_intrinsic_instr *atomic)
{
   switch (atomic->intrinsic) {
#define AOP_CASE(atom) \
   case nir_intrinsic_image_atomic_##atom:            \
   case nir_intrinsic_bindless_image_atomic_##atom:   \
   case nir_intrinsic_ssbo_atomic_##atom:             \
   case nir_intrinsic_shared_atomic_##atom:           \
   case nir_intrinsic_global_atomic_##atom

   AOP_CASE(add): {
      unsigned src_idx;
      switch (atomic->intrinsic) {
      case nir_intrinsic_image_atomic_add:
      case nir_intrinsic_bindless_image_atomic_add:
         src_idx = 3;
         break;
      case nir_intrinsic_ssbo_atomic_add:
         src_idx = 2;
         break;
      case nir_intrinsic_shared_atomic_add:
      case nir_intrinsic_global_atomic_add:
         src_idx = 1;
         break;
      default:
         unreachable("Invalid add atomic opcode");
      }

      if (nir_src_is_const(atomic->src[src_idx])) {
         int64_t add_val = nir_src_as_int(atomic->src[src_idx]);
         if (add_val == 1)
            return BRW_AOP_INC;
         else if (add_val == -1)
            return BRW_AOP_DEC;
      }
      return BRW_AOP_ADD;
   }

   AOP_CASE(imin):         return BRW_AOP_IMIN;
   AOP_CASE(umin):         return BRW_AOP_UMIN;
   AOP_CASE(imax):         return BRW_AOP_IMAX;
   AOP_CASE(umax):         return BRW_AOP_UMAX;
   AOP_CASE(and):          return BRW_AOP_AND;
   AOP_CASE(or):           return BRW_AOP_OR;
   AOP_CASE(xor):          return BRW_AOP_XOR;
   AOP_CASE(exchange):     return BRW_AOP_MOV;
   AOP_CASE(comp_swap):    return BRW_AOP_CMPWR;

#undef AOP_CASE
#define AOP_CASE(atom) \
   case nir_intrinsic_ssbo_atomic_##atom:          \
   case nir_intrinsic_shared_atomic_##atom:        \
   case nir_intrinsic_global_atomic_##atom

   AOP_CASE(fmin):         return BRW_AOP_FMIN;
   AOP_CASE(fmax):         return BRW_AOP_FMAX;
   AOP_CASE(fcomp_swap):   return BRW_AOP_FCMPWR;
   AOP_CASE(fadd):         return BRW_AOP_FADD;

#undef AOP_CASE

   default:
      unreachable("Unsupported NIR atomic intrinsic");
   }
}

enum brw_reg_type
brw_type_for_nir_type(const struct intel_device_info *devinfo,
                      nir_alu_type type)
{
   switch (type) {
   case nir_type_uint:
   case nir_type_uint32:
      return BRW_REGISTER_TYPE_UD;
   case nir_type_bool:
   case nir_type_int:
   case nir_type_bool32:
   case nir_type_int32:
      return BRW_REGISTER_TYPE_D;
   case nir_type_float:
   case nir_type_float32:
      return BRW_REGISTER_TYPE_F;
   case nir_type_float16:
      return BRW_REGISTER_TYPE_HF;
   case nir_type_float64:
      return BRW_REGISTER_TYPE_DF;
   case nir_type_int64:
      return devinfo->ver < 8 ? BRW_REGISTER_TYPE_DF : BRW_REGISTER_TYPE_Q;
   case nir_type_uint64:
      return devinfo->ver < 8 ? BRW_REGISTER_TYPE_DF : BRW_REGISTER_TYPE_UQ;
   case nir_type_int16:
      return BRW_REGISTER_TYPE_W;
   case nir_type_uint16:
      return BRW_REGISTER_TYPE_UW;
   case nir_type_int8:
      return BRW_REGISTER_TYPE_B;
   case nir_type_uint8:
      return BRW_REGISTER_TYPE_UB;
   default:
      unreachable("unknown type");
   }

   return BRW_REGISTER_TYPE_F;
}

nir_shader *
brw_nir_create_passthrough_tcs(void *mem_ctx, const struct brw_compiler *compiler,
                               const nir_shader_compiler_options *options,
                               const struct brw_tcs_prog_key *key)
{
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_TESS_CTRL,
                                                  options, "passthrough TCS");
   ralloc_steal(mem_ctx, b.shader);
   nir_shader *nir = b.shader;
   nir_variable *var;
   nir_ssa_def *load;
   nir_ssa_def *zero = nir_imm_int(&b, 0);
   nir_ssa_def *invoc_id = nir_load_invocation_id(&b);

   nir->info.inputs_read = key->outputs_written &
      ~(VARYING_BIT_TESS_LEVEL_INNER | VARYING_BIT_TESS_LEVEL_OUTER);
   nir->info.outputs_written = key->outputs_written;
   nir->info.tess.tcs_vertices_out = key->input_vertices;
   nir->num_uniforms = 8 * sizeof(uint32_t);

   var = nir_variable_create(nir, nir_var_uniform, glsl_vec4_type(), "hdr_0");
   var->data.location = 0;
   var = nir_variable_create(nir, nir_var_uniform, glsl_vec4_type(), "hdr_1");
   var->data.location = 1;

   /* Write the patch URB header. */
   for (int i = 0; i <= 1; i++) {
      load = nir_load_uniform(&b, 4, 32, zero, .base = i * 4 * sizeof(uint32_t));

      nir_store_output(&b, load, zero,
                       .base = VARYING_SLOT_TESS_LEVEL_INNER - i,
                       .write_mask = WRITEMASK_XYZW);
   }

   /* Copy inputs to outputs. */
   uint64_t varyings = nir->info.inputs_read;

   while (varyings != 0) {
      const int varying = ffsll(varyings) - 1;

      load = nir_load_per_vertex_input(&b, 4, 32, invoc_id, zero, .base = varying);

      nir_store_per_vertex_output(&b, load, invoc_id, zero,
                                  .base = varying,
                                  .write_mask = WRITEMASK_XYZW);

      varyings &= ~BITFIELD64_BIT(varying);
   }

   nir_validate_shader(nir, "in brw_nir_create_passthrough_tcs");

   brw_preprocess_nir(compiler, nir, NULL);

   return nir;
}
