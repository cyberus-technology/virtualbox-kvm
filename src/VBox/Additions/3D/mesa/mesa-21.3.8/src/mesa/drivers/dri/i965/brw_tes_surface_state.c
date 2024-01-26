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

#include "main/mtypes.h"
#include "program/prog_parameter.h"
#include "main/shaderapi.h"

#include "brw_context.h"
#include "brw_state.h"


/* Creates a new TES constant buffer reflecting the current TES program's
 * constants, if needed by the TES program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
brw_upload_tes_pull_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->tes.base;

   /* BRW_NEW_TESS_PROGRAMS */
   struct brw_program *dp =
      (struct brw_program *) brw->programs[MESA_SHADER_TESS_EVAL];

   if (!dp)
      return;

   /* BRW_NEW_TES_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->tes.base.prog_data;

   _mesa_shader_write_subroutine_indices(&brw->ctx, MESA_SHADER_TESS_EVAL);
   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_pull_constants(brw, BRW_NEW_TES_CONSTBUF, &dp->program,
                             stage_state, prog_data);
}

const struct brw_tracked_state brw_tes_pull_constants = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_TES_PROG_DATA |
             BRW_NEW_TESS_PROGRAMS,
   },
   .emit = brw_upload_tes_pull_constants,
};

static void
brw_upload_tes_ubo_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   /* _NEW_PROGRAM */
   struct gl_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_TESS_EVAL];

   /* BRW_NEW_TES_PROG_DATA */
   struct brw_stage_prog_data *prog_data = brw->tes.base.prog_data;

   brw_upload_ubo_surfaces(brw, prog, &brw->tes.base, prog_data);
}

const struct brw_tracked_state brw_tes_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_TES_PROG_DATA |
             BRW_NEW_UNIFORM_BUFFER,
   },
   .emit = brw_upload_tes_ubo_surfaces,
};

static void
brw_upload_tes_image_surfaces(struct brw_context *brw)
{
   /* BRW_NEW_TESS_PROGRAMS */
   const struct gl_program *tep = brw->programs[MESA_SHADER_TESS_EVAL];

   if (tep) {
      /* BRW_NEW_TES_PROG_DATA, BRW_NEW_IMAGE_UNITS */
      brw_upload_image_surfaces(brw, tep, &brw->tes.base,
                                brw->tes.base.prog_data);
   }
}

const struct brw_tracked_state brw_tes_image_surfaces = {
   .dirty = {
      .brw = BRW_NEW_BATCH |
             BRW_NEW_AUX_STATE |
             BRW_NEW_IMAGE_UNITS |
             BRW_NEW_TESS_PROGRAMS |
             BRW_NEW_TES_PROG_DATA,
   },
   .emit = brw_upload_tes_image_surfaces,
};
