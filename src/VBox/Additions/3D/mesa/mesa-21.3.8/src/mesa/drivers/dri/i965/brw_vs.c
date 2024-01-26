/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "util/compiler.h"
#include "main/context.h"
#include "brw_context.h"
#include "brw_vs.h"
#include "brw_util.h"
#include "brw_state.h"
#include "program/prog_print.h"
#include "program/prog_parameter.h"
#include "compiler/brw_nir.h"
#include "brw_program.h"

#include "util/ralloc.h"

/**
 * Decide which set of clip planes should be used when clipping via
 * gl_Position or gl_ClipVertex.
 */
gl_clip_plane *
brw_select_clip_planes(struct gl_context *ctx)
{
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX]) {
      /* There is currently a GLSL vertex shader, so clip according to GLSL
       * rules, which means compare gl_ClipVertex (or gl_Position, if
       * gl_ClipVertex wasn't assigned) against the eye-coordinate clip planes
       * that were stored in EyeUserPlane at the time the clip planes were
       * specified.
       */
      return ctx->Transform.EyeUserPlane;
   } else {
      /* Either we are using fixed function or an ARB vertex program.  In
       * either case the clip planes are going to be compared against
       * gl_Position (which is in clip coordinates) so we have to clip using
       * _ClipUserPlane, which was transformed into clip coordinates by Mesa
       * core.
       */
      return ctx->Transform._ClipUserPlane;
   }
}

static GLbitfield64
brw_vs_outputs_written(struct brw_context *brw, struct brw_vs_prog_key *key,
                       GLbitfield64 user_varyings)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   GLbitfield64 outputs_written = user_varyings;

   if (devinfo->ver < 6) {
      /* Put dummy slots into the VUE for the SF to put the replaced
       * point sprite coords in.  We shouldn't need these dummy slots,
       * which take up precious URB space, but it would mean that the SF
       * doesn't get nice aligned pairs of input coords into output
       * coords, which would be a pain to handle.
       */
      for (unsigned i = 0; i < 8; i++) {
         if (key->point_coord_replace & (1 << i))
            outputs_written |= BITFIELD64_BIT(VARYING_SLOT_TEX0 + i);
      }

      /* if back colors are written, allocate slots for front colors too */
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         outputs_written |= BITFIELD64_BIT(VARYING_SLOT_COL0);
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC1))
         outputs_written |= BITFIELD64_BIT(VARYING_SLOT_COL1);
   }

   /* In order for legacy clipping to work, we need to populate the clip
    * distance varying slots whenever clipping is enabled, even if the vertex
    * shader doesn't write to gl_ClipDistance.
    */
   if (key->nr_userclip_plane_consts > 0) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0);
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1);
   }

   return outputs_written;
}

static bool
brw_codegen_vs_prog(struct brw_context *brw,
                    struct brw_program *vp,
                    struct brw_vs_prog_key *key)
{
   const struct brw_compiler *compiler = brw->screen->compiler;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   const GLuint *program;
   struct brw_vs_prog_data prog_data;
   struct brw_stage_prog_data *stage_prog_data = &prog_data.base.base;
   void *mem_ctx;
   bool start_busy = false;
   double start_time = 0;

   memset(&prog_data, 0, sizeof(prog_data));

   /* Use ALT floating point mode for ARB programs so that 0^0 == 1. */
   if (vp->program.info.is_arb_asm)
      stage_prog_data->use_alt_mode = true;

   mem_ctx = ralloc_context(NULL);

   nir_shader *nir = nir_shader_clone(mem_ctx, vp->program.nir);

   brw_assign_common_binding_table_offsets(devinfo, &vp->program,
                                           &prog_data.base.base, 0);

   if (!vp->program.info.is_arb_asm) {
      brw_nir_setup_glsl_uniforms(mem_ctx, nir, &vp->program,
                                  &prog_data.base.base,
                                  compiler->scalar_stage[MESA_SHADER_VERTEX]);
      if (brw->can_push_ubos) {
         brw_nir_analyze_ubo_ranges(compiler, nir, key,
                                    prog_data.base.base.ubo_ranges);
      }
   } else {
      brw_nir_setup_arb_uniforms(mem_ctx, nir, &vp->program,
                                 &prog_data.base.base);
   }

   if (key->nr_userclip_plane_consts > 0) {
      brw_nir_lower_legacy_clipping(nir, key->nr_userclip_plane_consts,
                                    &prog_data.base.base);
   }

   if (key->copy_edgeflag)
      nir_lower_passthrough_edgeflags(nir);

   uint64_t outputs_written =
      brw_vs_outputs_written(brw, key, nir->info.outputs_written);

   brw_compute_vue_map(devinfo,
                       &prog_data.base.vue_map, outputs_written,
                       nir->info.separate_shader, 1);

   if (0) {
      _mesa_fprint_program_opt(stderr, &vp->program, PROG_PRINT_DEBUG, true);
   }

   if (unlikely(brw->perf_debug)) {
      start_busy = (brw->batch.last_bo &&
                    brw_bo_busy(brw->batch.last_bo));
      start_time = get_time();
   }

   if (INTEL_DEBUG(DEBUG_VS)) {
      if (vp->program.info.is_arb_asm)
         brw_dump_arb_asm("vertex", &vp->program);
   }


   /* Emit GFX4 code.
    */
   struct brw_compile_vs_params params = {
      .nir = nir,
      .key = key,
      .prog_data = &prog_data,
      .log_data = brw,
   };

   if (INTEL_DEBUG(DEBUG_SHADER_TIME)) {
      params.shader_time = true;
      params.shader_time_index =
         brw_get_shader_time_index(brw, &vp->program, ST_VS,
                                   !vp->program.info.is_arb_asm);
   }

   program = brw_compile_vs(compiler, mem_ctx, &params);
   if (program == NULL) {
      if (!vp->program.info.is_arb_asm) {
         vp->program.sh.data->LinkStatus = LINKING_FAILURE;
         ralloc_strcat(&vp->program.sh.data->InfoLog, params.error_str);
      }

      _mesa_problem(NULL, "Failed to compile vertex shader: %s\n", params.error_str);

      ralloc_free(mem_ctx);
      return false;
   }

   if (unlikely(brw->perf_debug)) {
      if (vp->compiled_once) {
         brw_debug_recompile(brw, MESA_SHADER_VERTEX, vp->program.Id,
                             &key->base);
      }
      if (start_busy && !brw_bo_busy(brw->batch.last_bo)) {
         perf_debug("VS compile took %.03f ms and stalled the GPU\n",
                    (get_time() - start_time) * 1000);
      }
      vp->compiled_once = true;
   }

   /* Scratch space is used for register spilling */
   brw_alloc_stage_scratch(brw, &brw->vs.base,
                           prog_data.base.base.total_scratch);

   /* The param and pull_param arrays will be freed by the shader cache. */
   ralloc_steal(NULL, prog_data.base.base.param);
   ralloc_steal(NULL, prog_data.base.base.pull_param);
   brw_upload_cache(&brw->cache, BRW_CACHE_VS_PROG,
                    key, sizeof(struct brw_vs_prog_key),
                    program, prog_data.base.base.program_size,
                    &prog_data, sizeof(prog_data),
                    &brw->vs.base.prog_offset, &brw->vs.base.prog_data);
   ralloc_free(mem_ctx);

   return true;
}

static bool
brw_vs_state_dirty(const struct brw_context *brw)
{
   return brw_state_dirty(brw,
                          _NEW_BUFFERS |
                          _NEW_LIGHT |
                          _NEW_POINT |
                          _NEW_POLYGON |
                          _NEW_TEXTURE |
                          _NEW_TRANSFORM,
                          BRW_NEW_VERTEX_PROGRAM |
                          BRW_NEW_VS_ATTRIB_WORKAROUNDS);
}

void
brw_vs_populate_key(struct brw_context *brw,
                    struct brw_vs_prog_key *key)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_VERTEX_PROGRAM */
   struct gl_program *prog = brw->programs[MESA_SHADER_VERTEX];
   struct brw_program *vp = (struct brw_program *) prog;
   const struct intel_device_info *devinfo = &brw->screen->devinfo;

   memset(key, 0, sizeof(*key));

   /* Just upload the program verbatim for now.  Always send it all
    * the inputs it asks for, whether they are varying or not.
    */

   /* _NEW_TEXTURE */
   brw_populate_base_prog_key(ctx, vp, &key->base);

   if (ctx->Transform.ClipPlanesEnabled != 0 &&
       (ctx->API == API_OPENGL_COMPAT || ctx->API == API_OPENGLES) &&
       vp->program.info.clip_distance_array_size == 0) {
      key->nr_userclip_plane_consts =
         util_logbase2(ctx->Transform.ClipPlanesEnabled) + 1;
   }

   if (devinfo->ver < 6) {
      /* _NEW_POLYGON */
      key->copy_edgeflag = (ctx->Polygon.FrontMode != GL_FILL ||
                            ctx->Polygon.BackMode != GL_FILL);

      /* _NEW_POINT */
      if (ctx->Point.PointSprite) {
         key->point_coord_replace = ctx->Point.CoordReplace & 0xff;
      }
   }

   if (prog->info.outputs_written &
       (VARYING_BIT_COL0 | VARYING_BIT_COL1 | VARYING_BIT_BFC0 |
        VARYING_BIT_BFC1)) {
      /* _NEW_LIGHT | _NEW_BUFFERS */
      key->clamp_vertex_color = ctx->Light._ClampVertexColor;
   }

   /* BRW_NEW_VS_ATTRIB_WORKAROUNDS */
   if (devinfo->verx10 <= 70) {
      memcpy(key->gl_attrib_wa_flags, brw->vb.attrib_wa_flags,
             sizeof(brw->vb.attrib_wa_flags));
   }
}

void
brw_upload_vs_prog(struct brw_context *brw)
{
   struct brw_vs_prog_key key;
   /* BRW_NEW_VERTEX_PROGRAM */
   struct brw_program *vp =
      (struct brw_program *) brw->programs[MESA_SHADER_VERTEX];

   if (!brw_vs_state_dirty(brw))
      return;

   brw_vs_populate_key(brw, &key);

   if (brw_search_cache(&brw->cache, BRW_CACHE_VS_PROG, &key, sizeof(key),
                        &brw->vs.base.prog_offset, &brw->vs.base.prog_data,
                        true))
      return;

   if (brw_disk_cache_upload_program(brw, MESA_SHADER_VERTEX))
      return;

   vp = (struct brw_program *) brw->programs[MESA_SHADER_VERTEX];
   vp->id = key.base.program_string_id;

   ASSERTED bool success = brw_codegen_vs_prog(brw, vp, &key);
   assert(success);
}

void
brw_vs_populate_default_key(const struct brw_compiler *compiler,
                            struct brw_vs_prog_key *key,
                            struct gl_program *prog)
{
   const struct intel_device_info *devinfo = compiler->devinfo;
   struct brw_program *bvp = brw_program(prog);

   memset(key, 0, sizeof(*key));

   brw_populate_default_base_prog_key(devinfo, bvp, &key->base);

   key->clamp_vertex_color =
      (prog->info.outputs_written &
       (VARYING_BIT_COL0 | VARYING_BIT_COL1 | VARYING_BIT_BFC0 |
        VARYING_BIT_BFC1));
}

bool
brw_vs_precompile(struct gl_context *ctx, struct gl_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_vs_prog_key key;
   uint32_t old_prog_offset = brw->vs.base.prog_offset;
   struct brw_stage_prog_data *old_prog_data = brw->vs.base.prog_data;
   bool success;

   struct brw_program *bvp = brw_program(prog);

   brw_vs_populate_default_key(brw->screen->compiler, &key, prog);

   success = brw_codegen_vs_prog(brw, bvp, &key);

   brw->vs.base.prog_offset = old_prog_offset;
   brw->vs.base.prog_data = old_prog_data;

   return success;
}
