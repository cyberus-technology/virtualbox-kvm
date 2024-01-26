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
#include "brw_cs.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_program.h"
#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "program/prog_parameter.h"
#include "main/shaderapi.h"

static uint32_t
f_as_u32(float f)
{
   union fi fi = { .f = f };
   return fi.ui;
}

static uint32_t
brw_param_value(struct brw_context *brw,
                const struct gl_program *prog,
                const struct brw_stage_state *stage_state,
                uint32_t param)
{
   struct gl_context *ctx = &brw->ctx;

   switch (BRW_PARAM_DOMAIN(param)) {
   case BRW_PARAM_DOMAIN_BUILTIN:
      if (param == BRW_PARAM_BUILTIN_ZERO) {
         return 0;
      } else if (BRW_PARAM_BUILTIN_IS_CLIP_PLANE(param)) {
         gl_clip_plane *clip_planes = brw_select_clip_planes(ctx);
         unsigned idx = BRW_PARAM_BUILTIN_CLIP_PLANE_IDX(param);
         unsigned comp = BRW_PARAM_BUILTIN_CLIP_PLANE_COMP(param);
         return ((uint32_t *)clip_planes[idx])[comp];
      } else if (param >= BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X &&
                 param <= BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_W) {
         unsigned i = param - BRW_PARAM_BUILTIN_TESS_LEVEL_OUTER_X;
         return f_as_u32(ctx->TessCtrlProgram.patch_default_outer_level[i]);
      } else if (param == BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_X) {
         return f_as_u32(ctx->TessCtrlProgram.patch_default_inner_level[0]);
      } else if (param == BRW_PARAM_BUILTIN_TESS_LEVEL_INNER_Y) {
         return f_as_u32(ctx->TessCtrlProgram.patch_default_inner_level[1]);
      } else if (param >= BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_X &&
                 param <= BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_Z) {
         unsigned i = param - BRW_PARAM_BUILTIN_WORK_GROUP_SIZE_X;
         return brw->compute.group_size[i];
      } else {
         unreachable("Invalid param builtin");
      }

   case BRW_PARAM_DOMAIN_PARAMETER: {
      unsigned idx = BRW_PARAM_PARAMETER_IDX(param);
      unsigned offset = prog->Parameters->Parameters[idx].ValueOffset;
      unsigned comp = BRW_PARAM_PARAMETER_COMP(param);
      assert(idx < prog->Parameters->NumParameters);
      return prog->Parameters->ParameterValues[offset + comp].u;
   }

   case BRW_PARAM_DOMAIN_UNIFORM: {
      unsigned idx = BRW_PARAM_UNIFORM_IDX(param);
      assert(idx < prog->sh.data->NumUniformDataSlots);
      return prog->sh.data->UniformDataSlots[idx].u;
   }

   case BRW_PARAM_DOMAIN_IMAGE: {
      unsigned idx = BRW_PARAM_IMAGE_IDX(param);
      unsigned offset = BRW_PARAM_IMAGE_OFFSET(param);
      assert(offset < ARRAY_SIZE(stage_state->image_param));
      return ((uint32_t *)&stage_state->image_param[idx])[offset];
   }

   default:
      unreachable("Invalid param domain");
   }
}


void
brw_populate_constant_data(struct brw_context *brw,
                           const struct gl_program *prog,
                           const struct brw_stage_state *stage_state,
                           void *void_dst,
                           const uint32_t *param,
                           unsigned nr_params)
{
   uint32_t *dst = void_dst;
   for (unsigned i = 0; i < nr_params; i++)
      dst[i] = brw_param_value(brw, prog, stage_state, param[i]);
}


/**
 * Creates a streamed BO containing the push constants for the VS or GS on
 * gfx6+.
 *
 * Push constants are constant values (such as GLSL uniforms) that are
 * pre-loaded into a shader stage's register space at thread spawn time.
 *
 * Not all GLSL uniforms will be uploaded as push constants: The hardware has
 * a limitation of 32 or 64 EU registers (256 or 512 floats) per stage to be
 * uploaded as push constants, while GL 4.4 requires at least 1024 components
 * to be usable for the VS.  Plus, currently we always use pull constants
 * instead of push constants when doing variable-index array access.
 *
 * See brw_curbe.c for the equivalent gfx4/5 code.
 */
void
gfx6_upload_push_constants(struct brw_context *brw,
                           const struct gl_program *prog,
                           const struct brw_stage_prog_data *prog_data,
                           struct brw_stage_state *stage_state)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;

   bool active = prog_data &&
      (stage_state->stage != MESA_SHADER_TESS_CTRL ||
       brw->programs[MESA_SHADER_TESS_EVAL]);

   if (active)
      _mesa_shader_write_subroutine_indices(ctx, stage_state->stage);

   if (!active || prog_data->nr_params == 0) {
      stage_state->push_const_size = 0;
   } else {
      /* Updates the ParamaterValues[i] pointers for all parameters of the
       * basic type of PROGRAM_STATE_VAR.
       */
      /* XXX: Should this happen somewhere before to get our state flag set? */
      if (prog)
         _mesa_load_state_parameters(ctx, prog->Parameters);

      int i;
      const int size = prog_data->nr_params * sizeof(gl_constant_value);
      gl_constant_value *param;
      if (devinfo->verx10 >= 75) {
         param = brw_upload_space(&brw->upload, size, 32,
                                  &stage_state->push_const_bo,
                                  &stage_state->push_const_offset);
      } else {
         param = brw_state_batch(brw, size, 32,
                                 &stage_state->push_const_offset);
      }

      STATIC_ASSERT(sizeof(gl_constant_value) == sizeof(float));

      /* _NEW_PROGRAM_CONSTANTS
       *
       * Also _NEW_TRANSFORM -- we may reference clip planes other than as a
       * side effect of dereferencing uniforms, so _NEW_PROGRAM_CONSTANTS
       * wouldn't be set for them.
       */
      brw_populate_constant_data(brw, prog, stage_state, param,
                                 prog_data->param,
                                 prog_data->nr_params);

      if (0) {
         fprintf(stderr, "%s constants:\n",
                 _mesa_shader_stage_to_string(stage_state->stage));
         for (i = 0; i < prog_data->nr_params; i++) {
            if ((i & 7) == 0)
               fprintf(stderr, "g%d: ",
                       prog_data->dispatch_grf_start_reg + i / 8);
            fprintf(stderr, "%8f ", param[i].f);
            if ((i & 7) == 7)
               fprintf(stderr, "\n");
         }
         if ((i & 7) != 0)
            fprintf(stderr, "\n");
         fprintf(stderr, "\n");
      }

      stage_state->push_const_size = ALIGN(prog_data->nr_params, 8) / 8;
      /* We can only push 32 registers of constants at a time. */

      /* From the SNB PRM (vol2, part 1, section 3.2.1.4: 3DSTATE_CONSTANT_VS:
       *
       *     "The sum of all four read length fields (each incremented to
       *      represent the actual read length) must be less than or equal to
       *      32"
       *
       * From the IVB PRM (vol2, part 1, section 3.2.1.3: 3DSTATE_CONSTANT_VS:
       *
       *     "The sum of all four read length fields must be less than or
       *      equal to the size of 64"
       *
       * The other shader stages all match the VS's limits.
       */
      assert(stage_state->push_const_size <= 32);
   }

   stage_state->push_constants_dirty = true;
}


/**
 * Creates a temporary BO containing the pull constant data for the shader
 * stage, and the SURFACE_STATE struct that points at it.
 *
 * Pull constants are GLSL uniforms (and other constant data) beyond what we
 * could fit as push constants, or that have variable-index array access
 * (which is easiest to support using pull constants, and avoids filling
 * register space with mostly-unused data).
 *
 * Compare this path to brw_curbe.c for gfx4/5 push constants, and
 * gfx6_vs_state.c for gfx6+ push constants.
 */
void
brw_upload_pull_constants(struct brw_context *brw,
                          GLbitfield64 brw_new_constbuf,
                          const struct gl_program *prog,
                          struct brw_stage_state *stage_state,
                          const struct brw_stage_prog_data *prog_data)
{
   unsigned i;
   uint32_t surf_index = prog_data->binding_table.pull_constants_start;

   if (!prog_data->nr_pull_params) {
      if (stage_state->surf_offset[surf_index]) {
         stage_state->surf_offset[surf_index] = 0;
         brw->ctx.NewDriverState |= brw_new_constbuf;
      }
      return;
   }

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   _mesa_load_state_parameters(&brw->ctx, prog->Parameters);

   /* BRW_NEW_*_PROG_DATA | _NEW_PROGRAM_CONSTANTS */
   uint32_t size = prog_data->nr_pull_params * 4;
   struct brw_bo *const_bo = NULL;
   uint32_t const_offset;
   gl_constant_value *constants = brw_upload_space(&brw->upload, size, 64,
                                                   &const_bo, &const_offset);

   STATIC_ASSERT(sizeof(gl_constant_value) == sizeof(float));

   brw_populate_constant_data(brw, prog, stage_state, constants,
                              prog_data->pull_param,
                              prog_data->nr_pull_params);

   if (0) {
      for (i = 0; i < ALIGN(prog_data->nr_pull_params, 4) / 4; i++) {
         const gl_constant_value *row = &constants[i * 4];
         fprintf(stderr, "const surface %3d: %4.3f %4.3f %4.3f %4.3f\n",
                 i, row[0].f, row[1].f, row[2].f, row[3].f);
      }
   }

   brw_emit_buffer_surface_state(brw, &stage_state->surf_offset[surf_index],
                                 const_bo, const_offset,
                                 ISL_FORMAT_R32G32B32A32_FLOAT,
                                 size, 1, 0);

   brw_bo_unreference(const_bo);

   brw->ctx.NewDriverState |= brw_new_constbuf;
}

/**
 * Creates a region containing the push constants for the CS on gfx7+.
 *
 * Push constants are constant values (such as GLSL uniforms) that are
 * pre-loaded into a shader stage's register space at thread spawn time.
 *
 * For other stages, see brw_curbe.c:brw_upload_constant_buffer for the
 * equivalent gfx4/5 code and gfx6_vs_state.c:gfx6_upload_push_constants for
 * gfx6+.
 */
void
brw_upload_cs_push_constants(struct brw_context *brw,
                             const struct gl_program *prog,
                             const struct brw_cs_prog_data *cs_prog_data,
                             struct brw_stage_state *stage_state)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_stage_prog_data *prog_data =
      (struct brw_stage_prog_data*) cs_prog_data;

   /* Updates the ParamaterValues[i] pointers for all parameters of the
    * basic type of PROGRAM_STATE_VAR.
    */
   /* XXX: Should this happen somewhere before to get our state flag set? */
   _mesa_load_state_parameters(ctx, prog->Parameters);

   const struct brw_cs_dispatch_info dispatch =
      brw_cs_get_dispatch_info(&brw->screen->devinfo, cs_prog_data,
                               brw->compute.group_size);
   const unsigned push_const_size =
      brw_cs_push_const_total_size(cs_prog_data, dispatch.threads);

   if (push_const_size == 0) {
      stage_state->push_const_size = 0;
      return;
   }


   uint32_t *param =
      brw_state_batch(brw, ALIGN(push_const_size, 64),
                      64, &stage_state->push_const_offset);
   assert(param);

   STATIC_ASSERT(sizeof(gl_constant_value) == sizeof(float));

   if (cs_prog_data->push.cross_thread.size > 0) {
      uint32_t *param_copy = param;
      for (unsigned i = 0;
           i < cs_prog_data->push.cross_thread.dwords;
           i++) {
         assert(prog_data->param[i] != BRW_PARAM_BUILTIN_SUBGROUP_ID);
         param_copy[i] = brw_param_value(brw, prog, stage_state,
                                         prog_data->param[i]);
      }
   }

   if (cs_prog_data->push.per_thread.size > 0) {
      for (unsigned t = 0; t < dispatch.threads; t++) {
         unsigned dst =
            8 * (cs_prog_data->push.per_thread.regs * t +
                 cs_prog_data->push.cross_thread.regs);
         unsigned src = cs_prog_data->push.cross_thread.dwords;
         for ( ; src < prog_data->nr_params; src++, dst++) {
            if (prog_data->param[src] == BRW_PARAM_BUILTIN_SUBGROUP_ID) {
               param[dst] = t;
            } else {
               param[dst] = brw_param_value(brw, prog, stage_state,
                                            prog_data->param[src]);
            }
         }
      }
   }

   stage_state->push_const_size =
      cs_prog_data->push.cross_thread.regs +
      cs_prog_data->push.per_thread.regs;
}
