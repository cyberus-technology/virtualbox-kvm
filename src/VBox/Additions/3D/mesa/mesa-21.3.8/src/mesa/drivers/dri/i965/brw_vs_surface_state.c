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

#include "main/mtypes.h"
#include "program/prog_parameter.h"
#include "main/shaderapi.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_buffer_objects.h"


/* Creates a new VS constant buffer reflecting the current VS program's
 * constants, if needed by the VS program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
brw_upload_vs_pull_constants(struct brw_context *brw)
{
   struct brw_stage_state *stage_state = &brw->vs.base;

   /* BRW_NEW_VERTEX_PROGRAM */
   struct brw_program *vp =
      (struct brw_program *) brw->programs[MESA_SHADER_VERTEX];

   /* BRW_NEW_VS_PROG_DATA */
   const struct brw_stage_prog_data *prog_data = brw->vs.base.prog_data;

   _mesa_shader_write_subroutine_indices(&brw->ctx, MESA_SHADER_VERTEX);
   /* _NEW_PROGRAM_CONSTANTS */
   brw_upload_pull_constants(brw, BRW_NEW_VS_CONSTBUF, &vp->program,
                             stage_state, prog_data);
}

const struct brw_tracked_state brw_vs_pull_constants = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_VERTEX_PROGRAM |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_upload_vs_pull_constants,
};

static void
brw_upload_vs_ubo_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* _NEW_PROGRAM */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];

   /* BRW_NEW_VS_PROG_DATA */
   brw_upload_ubo_surfaces(brw, prog, &brw->vs.base, brw->vs.base.prog_data);
}

const struct brw_tracked_state brw_vs_ubo_surfaces = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_UNIFORM_BUFFER |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_upload_vs_ubo_surfaces,
};

static void
brw_upload_vs_image_surfaces(struct brw_context *brw)
{
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_program *vp = brw->programs[MESA_SHADER_VERTEX];

   if (vp) {
      /* BRW_NEW_VS_PROG_DATA, BRW_NEW_IMAGE_UNITS, _NEW_TEXTURE */
      brw_upload_image_surfaces(brw, vp, &brw->vs.base,
                                brw->vs.base.prog_data);
   }
}

const struct brw_tracked_state brw_vs_image_surfaces = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_AUX_STATE |
             BRW_NEW_IMAGE_UNITS |
             BRW_NEW_VERTEX_PROGRAM |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_upload_vs_image_surfaces,
};
