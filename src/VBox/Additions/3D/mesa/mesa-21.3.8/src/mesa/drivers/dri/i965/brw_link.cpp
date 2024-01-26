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

#include "brw_context.h"
#include "compiler/brw_nir.h"
#include "brw_program.h"
#include "compiler/glsl/gl_nir.h"
#include "compiler/glsl/gl_nir_linker.h"
#include "compiler/glsl/ir.h"
#include "compiler/glsl/ir_optimization.h"
#include "compiler/glsl/program.h"
#include "compiler/nir/nir_serialize.h"
#include "program/program.h"
#include "main/glspirv.h"
#include "main/mtypes.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"

/**
 * Performs a compile of the shader stages even when we don't know
 * what non-orthogonal state will be set, in the hope that it reflects
 * the eventual NOS used, and thus allows us to produce link failures.
 */
static bool
brw_shader_precompile(struct gl_context *ctx,
                      struct gl_shader_program *sh_prog)
{
   struct gl_linked_shader *vs = sh_prog->_LinkedShaders[MESA_SHADER_VERTEX];
   struct gl_linked_shader *tcs = sh_prog->_LinkedShaders[MESA_SHADER_TESS_CTRL];
   struct gl_linked_shader *tes = sh_prog->_LinkedShaders[MESA_SHADER_TESS_EVAL];
   struct gl_linked_shader *gs = sh_prog->_LinkedShaders[MESA_SHADER_GEOMETRY];
   struct gl_linked_shader *fs = sh_prog->_LinkedShaders[MESA_SHADER_FRAGMENT];
   struct gl_linked_shader *cs = sh_prog->_LinkedShaders[MESA_SHADER_COMPUTE];

   if (fs && !brw_fs_precompile(ctx, fs->Program))
      return false;

   if (gs && !brw_gs_precompile(ctx, gs->Program))
      return false;

   if (tes && !brw_tes_precompile(ctx, sh_prog, tes->Program))
      return false;

   if (tcs && !brw_tcs_precompile(ctx, sh_prog, tcs->Program))
      return false;

   if (vs && !brw_vs_precompile(ctx, vs->Program))
      return false;

   if (cs && !brw_cs_precompile(ctx, cs->Program))
      return false;

   return true;
}

static void
brw_lower_packing_builtins(struct brw_context *brw,
                           exec_list *ir)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   /* Gens < 7 don't have instructions to convert to or from half-precision,
    * and Gens < 6 don't expose that functionality.
    */
   if (devinfo->ver != 6)
      return;

   lower_packing_builtins(ir, LOWER_PACK_HALF_2x16 | LOWER_UNPACK_HALF_2x16);
}

static void
process_glsl_ir(struct brw_context *brw,
                struct gl_shader_program *shader_prog,
                struct gl_linked_shader *shader)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;

   /* Temporary memory context for any new IR. */
   void *mem_ctx = ralloc_context(NULL);

   ralloc_adopt(mem_ctx, shader->ir);

   if (shader->Stage == MESA_SHADER_FRAGMENT) {
      lower_blend_equation_advanced(
         shader, ctx->Extensions.KHR_blend_equation_advanced_coherent);
   }

   /* lower_packing_builtins() inserts arithmetic instructions, so it
    * must precede lower_instructions().
    */
   brw_lower_packing_builtins(brw, shader->ir);
   do_mat_op_to_vec(shader->ir);

   unsigned instructions_to_lower = (DIV_TO_MUL_RCP |
                                     SUB_TO_ADD_NEG |
                                     EXP_TO_EXP2 |
                                     LOG_TO_LOG2 |
                                     DFREXP_DLDEXP_TO_ARITH);
   if (devinfo->ver < 7) {
      instructions_to_lower |= BIT_COUNT_TO_MATH |
                               EXTRACT_TO_SHIFTS |
                               INSERT_TO_SHIFTS |
                               REVERSE_TO_SHIFTS;
   }

   lower_instructions(shader->ir, instructions_to_lower);

   /* Pre-gfx6 HW can only nest if-statements 16 deep.  Beyond this,
    * if-statements need to be flattened.
    */
   if (devinfo->ver < 6)
      lower_if_to_cond_assign(shader->Stage, shader->ir, 16);

   do_vec_index_to_cond_assign(shader->ir);
   lower_vector_insert(shader->ir, true);
   lower_offset_arrays(shader->ir);
   lower_quadop_vector(shader->ir, false);

   validate_ir_tree(shader->ir);

   /* Now that we've finished altering the linked IR, reparent any live IR back
    * to the permanent memory context, and free the temporary one (discarding any
    * junk we optimized away).
    */
   reparent_ir(shader->ir, shader->ir);
   ralloc_free(mem_ctx);

   if (ctx->_Shader->Flags & GLSL_DUMP) {
      fprintf(stderr, "\n");
      if (shader->ir) {
         fprintf(stderr, "GLSL IR for linked %s program %d:\n",
                 _mesa_shader_stage_to_string(shader->Stage),
                 shader_prog->Name);
         _mesa_print_ir(stderr, shader->ir, NULL);
      } else {
         fprintf(stderr, "No GLSL IR for linked %s program %d (shader may be "
                 "from cache)\n", _mesa_shader_stage_to_string(shader->Stage),
                 shader_prog->Name);
      }
      fprintf(stderr, "\n");
   }
}

static void
unify_interfaces(struct shader_info **infos)
{
   struct shader_info *prev_info = NULL;

   for (unsigned i = MESA_SHADER_VERTEX; i < MESA_SHADER_FRAGMENT; i++) {
      if (!infos[i])
         continue;

      if (prev_info) {
         prev_info->outputs_written |= infos[i]->inputs_read &
            ~(VARYING_BIT_TESS_LEVEL_INNER | VARYING_BIT_TESS_LEVEL_OUTER);
         infos[i]->inputs_read |= prev_info->outputs_written &
            ~(VARYING_BIT_TESS_LEVEL_INNER | VARYING_BIT_TESS_LEVEL_OUTER);

         prev_info->patch_outputs_written |= infos[i]->patch_inputs_read;
         infos[i]->patch_inputs_read |= prev_info->patch_outputs_written;
      }
      prev_info = infos[i];
   }
}

static void
update_xfb_info(struct gl_transform_feedback_info *xfb_info,
                struct shader_info *info)
{
   if (!xfb_info)
      return;

   for (unsigned i = 0; i < xfb_info->NumOutputs; i++) {
      struct gl_transform_feedback_output *output = &xfb_info->Outputs[i];

      /* The VUE header contains three scalar fields packed together:
       * - gl_PointSize is stored in VARYING_SLOT_PSIZ.w
       * - gl_Layer is stored in VARYING_SLOT_PSIZ.y
       * - gl_ViewportIndex is stored in VARYING_SLOT_PSIZ.z
       */
      switch (output->OutputRegister) {
      case VARYING_SLOT_LAYER:
         assert(output->NumComponents == 1);
         output->OutputRegister = VARYING_SLOT_PSIZ;
         output->ComponentOffset = 1;
         break;
      case VARYING_SLOT_VIEWPORT:
         assert(output->NumComponents == 1);
         output->OutputRegister = VARYING_SLOT_PSIZ;
         output->ComponentOffset = 2;
         break;
      case VARYING_SLOT_PSIZ:
         assert(output->NumComponents == 1);
         output->ComponentOffset = 3;
         break;
      }

      info->outputs_written |= 1ull << output->OutputRegister;
   }
}

extern "C" GLboolean
brw_link_shader(struct gl_context *ctx, struct gl_shader_program *shProg)
{
   struct brw_context *brw = brw_context(ctx);
   const struct brw_compiler *compiler = brw->screen->compiler;
   unsigned int stage;
   struct shader_info *infos[MESA_SHADER_STAGES] = { 0, };

   if (shProg->data->LinkStatus == LINKING_SKIPPED)
      return GL_TRUE;

   for (stage = 0; stage < ARRAY_SIZE(shProg->_LinkedShaders); stage++) {
      struct gl_linked_shader *shader = shProg->_LinkedShaders[stage];
      if (!shader)
         continue;

      struct gl_program *prog = shader->Program;
      prog->Parameters = _mesa_new_parameter_list();

      if (!shader->spirv_data)
         process_glsl_ir(brw, shProg, shader);

      _mesa_copy_linked_program_data(shProg, shader);

      prog->ShadowSamplers = shader->shadow_samplers;

      bool debug_enabled =
         INTEL_DEBUG(intel_debug_flag_for_shader_stage(shader->Stage));

      if (debug_enabled && shader->ir) {
         fprintf(stderr, "GLSL IR for native %s shader %d:\n",
                 _mesa_shader_stage_to_string(shader->Stage), shProg->Name);
         _mesa_print_ir(stderr, shader->ir, NULL);
         fprintf(stderr, "\n\n");
      }

      prog->nir = brw_create_nir(brw, shProg, prog, (gl_shader_stage) stage,
                                 compiler->scalar_stage[stage]);
   }

   /* TODO: Verify if its feasible to split up the NIR linking work into a
    * per-stage part (that fill out information we need for the passes) and a
    * actual linking part, so that we could fold back brw_nir_lower_resources
    * back into brw_create_nir.
    */

   /* SPIR-V programs use a NIR linker */
   if (shProg->data->spirv) {
      static const gl_nir_linker_options opts = {
         .fill_parameters = false,
      };
      if (!gl_nir_link_spirv(ctx, shProg, &opts))
         return GL_FALSE;
   }

   for (stage = 0; stage < ARRAY_SIZE(shProg->_LinkedShaders); stage++) {
      struct gl_linked_shader *shader = shProg->_LinkedShaders[stage];
      if (!shader)
         continue;

      struct gl_program *prog = shader->Program;

      brw_nir_lower_resources(prog->nir, shProg, prog, &brw->screen->devinfo);

      NIR_PASS_V(prog->nir, brw_nir_lower_gl_images, prog);
   }

   /* Determine first and last stage. */
   unsigned first = MESA_SHADER_STAGES;
   unsigned last = 0;
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (!shProg->_LinkedShaders[i])
         continue;
      if (first == MESA_SHADER_STAGES)
         first = i;
      last = i;
   }

   /* Linking the stages in the opposite order (from fragment to vertex)
    * ensures that inter-shader outputs written to in an earlier stage
    * are eliminated if they are (transitively) not used in a later
    * stage.
    *
    * TODO: Look into Shadow of Mordor regressions on HSW and enable this for
    * all platforms. See: https://bugs.freedesktop.org/show_bug.cgi?id=103537
    */
    if (first != last && brw->screen->devinfo.ver >= 8) {
       int next = last;
       for (int i = next - 1; i >= 0; i--) {
          if (shProg->_LinkedShaders[i] == NULL)
             continue;

          brw_nir_link_shaders(compiler,
                               shProg->_LinkedShaders[i]->Program->nir,
                               shProg->_LinkedShaders[next]->Program->nir);
          next = i;
       }
    }

   for (stage = 0; stage < ARRAY_SIZE(shProg->_LinkedShaders); stage++) {
      struct gl_linked_shader *shader = shProg->_LinkedShaders[stage];
      if (!shader)
         continue;

      struct gl_program *prog = shader->Program;

      _mesa_update_shader_textures_used(shProg, prog);

      brw_shader_gather_info(prog->nir, prog);

      NIR_PASS_V(prog->nir, gl_nir_lower_atomics, shProg, false);
      NIR_PASS_V(prog->nir, nir_lower_atomics_to_ssbo);

      nir_sweep(prog->nir);

      infos[stage] = &prog->nir->info;

      update_xfb_info(prog->sh.LinkedTransformFeedback, infos[stage]);

      /* Make a pass over the IR to add state references for any built-in
       * uniforms that are used.  This has to be done now (during linking).
       * Code generation doesn't happen until the first time this shader is
       * used for rendering.  Waiting until then to generate the parameters is
       * too late.  At that point, the values for the built-in uniforms won't
       * get sent to the shader.
       */
      nir_foreach_uniform_variable(var, prog->nir) {
         const nir_state_slot *const slots = var->state_slots;
         for (unsigned int i = 0; i < var->num_state_slots; i++) {
            assert(slots != NULL);
            _mesa_add_state_reference(prog->Parameters, slots[i].tokens);
         }
      }
   }

   /* The linker tries to dead code eliminate unused varying components,
    * and make sure interfaces match.  But it isn't able to do so in all
    * cases.  So, explicitly make the interfaces match by OR'ing together
    * the inputs_read/outputs_written bitfields of adjacent stages.
    */
   if (!shProg->SeparateShader)
      unify_interfaces(infos);

   if ((ctx->_Shader->Flags & GLSL_DUMP) && shProg->Name != 0) {
      for (unsigned i = 0; i < shProg->NumShaders; i++) {
         const struct gl_shader *sh = shProg->Shaders[i];
         if (!sh)
            continue;

         fprintf(stderr, "GLSL %s shader %d source for linked program %d:\n",
                 _mesa_shader_stage_to_string(sh->Stage),
                 i, shProg->Name);
         fprintf(stderr, "%s", sh->Source);
         fprintf(stderr, "\n");
      }
   }

   if (brw->precompile && !brw_shader_precompile(ctx, shProg))
      return GL_FALSE;

   /* SPIR-V programs build its resource list from linked NIR shaders. */
   if (!shProg->data->spirv)
      build_program_resource_list(ctx, shProg, false);
   else
      nir_build_program_resource_list(ctx, shProg, true);

   for (stage = 0; stage < ARRAY_SIZE(shProg->_LinkedShaders); stage++) {
      struct gl_linked_shader *shader = shProg->_LinkedShaders[stage];
      if (!shader)
         continue;

      /* The GLSL IR won't be needed anymore. */
      ralloc_free(shader->ir);
      shader->ir = NULL;
   }

   return GL_TRUE;
}
