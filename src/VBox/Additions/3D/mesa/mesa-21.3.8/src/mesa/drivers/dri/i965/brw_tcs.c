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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file brw_tcs.c
 *
 * Tessellation control shader state upload code.
 */

#include "brw_context.h"
#include "compiler/brw_nir.h"
#include "brw_program.h"
#include "brw_state.h"
#include "program/prog_parameter.h"
#include "nir_builder.h"

static bool
brw_codegen_tcs_prog(struct brw_context *brw, struct brw_program *tcp,
                     struct brw_program *tep, struct brw_tcs_prog_key *key)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_compiler *compiler = brw->screen->compiler;
   const struct intel_device_info *devinfo = compiler->devinfo;
   struct brw_stage_state *stage_state = &brw->tcs.base;
   nir_shader *nir;
   struct brw_tcs_prog_data prog_data;
   bool start_busy = false;
   double start_time = 0;

   void *mem_ctx = ralloc_context(NULL);
   if (tcp) {
      nir = nir_shader_clone(mem_ctx, tcp->program.nir);
   } else {
      const nir_shader_compiler_options *options =
         ctx->Const.ShaderCompilerOptions[MESA_SHADER_TESS_CTRL].NirOptions;
      nir = brw_nir_create_passthrough_tcs(mem_ctx, compiler, options, key);
   }

   memset(&prog_data, 0, sizeof(prog_data));

   if (tcp) {
      brw_assign_common_binding_table_offsets(devinfo, &tcp->program,
                                              &prog_data.base.base, 0);

      brw_nir_setup_glsl_uniforms(mem_ctx, nir, &tcp->program,
                                  &prog_data.base.base,
                                  compiler->scalar_stage[MESA_SHADER_TESS_CTRL]);
      if (brw->can_push_ubos) {
         brw_nir_analyze_ubo_ranges(compiler, nir, NULL,
                                    prog_data.base.base.ubo_ranges);
      }
   } else {
      /* Upload the Patch URB Header as the first two uniforms.
       * Do the annoying scrambling so the shader doesn't have to.
       */
      assert(nir->num_uniforms == 32);
      prog_data.base.base.param = rzalloc_array(mem_ctx, uint32_t, 8);
      prog_data.base.base.nr_params = 8;

      uint32_t *param = prog_data.base.base.param;
      for (int i = 0; i < 8; i++)
         param[i] = BRW_PARAM_BUILTIN_ZERO;

      if (key->tes_primitive_mode == GL_QUADS) {
         for (int i = 0; i < 4; i++)
            param[7 - i] = BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X + i;

         param[3] = BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_X;
         param[2] = BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_Y;
      } else if (key->tes_primitive_mode == GL_TRIANGLES) {
         for (int i = 0; i < 3; i++)
            param[7 - i] = BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X + i;

         param[4] = BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_X;
      } else {
         assert(key->tes_primitive_mode == GL_ISOLINES);
         param[7] = BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_Y;
         param[6] = BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X;
      }
   }

   int st_index = -1;
   if (INTEL_DEBUG(DEBUG_SHADER_TIME) && tep)
      st_index = brw_get_shader_time_index(brw, &tep->program, ST_TCS, true);

   if (unlikely(brw->perf_debug)) {
      start_busy = brw->batch.last_bo && brw_bo_busy(brw->batch.last_bo);
      start_time = get_time();
   }

   char *error_str;
   const unsigned *program =
      brw_compile_tcs(compiler, brw, mem_ctx, key, &prog_data, nir, st_index,
                      NULL, &error_str);
   if (program == NULL) {
      if (tep) {
         tep->program.sh.data->LinkStatus = LINKING_FAILURE;
         ralloc_strcat(&tep->program.sh.data->InfoLog, error_str);
      }

      _mesa_problem(NULL, "Failed to compile tessellation control shader: "
                    "%s\n", error_str);

      ralloc_free(mem_ctx);
      return false;
   }

   if (unlikely(brw->perf_debug)) {
      if (tcp) {
         if (tcp->compiled_once) {
            brw_debug_recompile(brw, MESA_SHADER_TESS_CTRL, tcp->program.Id,
                                &key->base);
         }
         tcp->compiled_once = true;
      }

      if (start_busy && !brw_bo_busy(brw->batch.last_bo)) {
         perf_debug("TCS compile took %.03f ms and stalled the GPU\n",
                    (get_time() - start_time) * 1000);
      }
   }

   /* Scratch space is used for register spilling */
   brw_alloc_stage_scratch(brw, stage_state,
                           prog_data.base.base.total_scratch);

   /* The param and pull_param arrays will be freed by the shader cache. */
   ralloc_steal(NULL, prog_data.base.base.param);
   ralloc_steal(NULL, prog_data.base.base.pull_param);
   brw_upload_cache(&brw->cache, BRW_CACHE_TCS_PROG,
                    key, sizeof(*key),
                    program, prog_data.base.base.program_size,
                    &prog_data, sizeof(prog_data),
                    &stage_state->prog_offset, &brw->tcs.base.prog_data);
   ralloc_free(mem_ctx);

   return true;
}

void
brw_tcs_populate_key(struct brw_context *brw,
                     struct brw_tcs_prog_key *key)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const struct brw_compiler *compiler = brw->screen->compiler;
   struct brw_program *tcp =
      (struct brw_program *) brw->programs[MESA_SHADER_TESS_CTRL];
   struct brw_program *tep =
      (struct brw_program *) brw->programs[MESA_SHADER_TESS_EVAL];
   struct gl_program *tes_prog = &tep->program;

   uint64_t per_vertex_slots = tes_prog->info.inputs_read;
   uint32_t per_patch_slots = tes_prog->info.patch_inputs_read;

   memset(key, 0, sizeof(*key));

   if (tcp) {
      struct gl_program *prog = &tcp->program;
      per_vertex_slots |= prog->info.outputs_written;
      per_patch_slots |= prog->info.patch_outputs_written;
   }

   if (devinfo->ver < 8 || !tcp || compiler->use_tcs_8_patch)
      key->input_vertices = brw->ctx.TessCtrlProgram.patch_vertices;
   key->outputs_written = per_vertex_slots;
   key->patch_outputs_written = per_patch_slots;

   /* We need to specialize our code generation for tessellation levels
    * based on the domain the DS is expecting to tessellate.
    */
   key->tes_primitive_mode = tep->program.info.tess.primitive_mode;
   key->quads_workaround = devinfo->ver < 9 &&
                           tep->program.info.tess.primitive_mode == GL_QUADS &&
                           tep->program.info.tess.spacing == TESS_SPACING_EQUAL;

   if (tcp) {
      /* _NEW_TEXTURE */
      brw_populate_base_prog_key(&brw->ctx, tcp, &key->base);
   }
}

void
brw_upload_tcs_prog(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->tcs.base;
   struct brw_tcs_prog_key key;
   /* BRW_NEW_TESS_PROGRAMS */
   struct brw_program *tcp =
      (struct brw_program *) brw->programs[MESA_SHADER_TESS_CTRL];
   ASSERTED struct brw_program *tep =
      (struct brw_program *) brw->programs[MESA_SHADER_TESS_EVAL];
   assert(tep);

   if (!brw_state_dirty(brw,
                        _NEW_TEXTURE,
                        BRW_NEW_PATCH_PRIMITIVE |
                        BRW_NEW_TESS_PROGRAMS))
      return;

   brw_tcs_populate_key(brw, &key);

   if (brw_search_cache(&brw->cache, BRW_CACHE_TCS_PROG, &key, sizeof(key),
                        &stage_state->prog_offset, &brw->tcs.base.prog_data,
                        true))
      return;

   if (brw_disk_cache_upload_program(brw, MESA_SHADER_TESS_CTRL))
      return;

   tcp = (struct brw_program *) brw->programs[MESA_SHADER_TESS_CTRL];
   if (tcp)
      tcp->id = key.base.program_string_id;

   ASSERTED bool success = brw_codegen_tcs_prog(brw, tcp, tep, &key);
   assert(success);
}

void
brw_tcs_populate_default_key(const struct brw_compiler *compiler,
                             struct brw_tcs_prog_key *key,
                             struct gl_shader_program *sh_prog,
                             struct gl_program *prog)
{
   const struct intel_device_info *devinfo = compiler->devinfo;
   struct brw_program *btcp = brw_program(prog);
   const struct gl_linked_shader *tes =
      sh_prog->_LinkedShaders[MESA_SHADER_TESS_EVAL];

   memset(key, 0, sizeof(*key));

   brw_populate_default_base_prog_key(devinfo, btcp, &key->base);

   /* Guess that the input and output patches have the same dimensionality. */
   if (devinfo->ver < 8 || compiler->use_tcs_8_patch)
      key->input_vertices = prog->info.tess.tcs_vertices_out;

   if (tes) {
      key->tes_primitive_mode = tes->Program->info.tess.primitive_mode;
      key->quads_workaround = devinfo->ver < 9 &&
                              tes->Program->info.tess.primitive_mode == GL_QUADS &&
                              tes->Program->info.tess.spacing == TESS_SPACING_EQUAL;
   } else {
      key->tes_primitive_mode = GL_TRIANGLES;
   }

   key->outputs_written = prog->nir->info.outputs_written;
   key->patch_outputs_written = prog->nir->info.patch_outputs_written;
}

bool
brw_tcs_precompile(struct gl_context *ctx,
                   struct gl_shader_program *shader_prog,
                   struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   const struct brw_compiler *compiler = brw->screen->compiler;
   struct brw_tcs_prog_key key;
   uint32_t old_prog_offset = brw->tcs.base.prog_offset;
   struct brw_stage_prog_data *old_prog_data = brw->tcs.base.prog_data;
   bool success;

   struct brw_program *btcp = brw_program(prog);
   const struct gl_linked_shader *tes =
      shader_prog->_LinkedShaders[MESA_SHADER_TESS_EVAL];
   struct brw_program *btep = tes ? brw_program(tes->Program) : NULL;

   brw_tcs_populate_default_key(compiler, &key, shader_prog, prog);

   success = brw_codegen_tcs_prog(brw, btcp, btep, &key);

   brw->tcs.base.prog_offset = old_prog_offset;
   brw->tcs.base.prog_data = old_prog_data;

   return success;
}
