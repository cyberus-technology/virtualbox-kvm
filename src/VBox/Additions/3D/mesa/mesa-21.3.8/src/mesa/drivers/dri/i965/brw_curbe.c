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

/** @file brw_curbe.c
 *
 * Push constant handling for gfx4/5.
 *
 * Push constants are constant values (such as GLSL uniforms) that are
 * pre-loaded into a shader stage's register space at thread spawn time.  On
 * gfx4 and gfx5, we create a blob in memory containing all the push constants
 * for all the stages in order.  At CMD_CONST_BUFFER time that blob is loaded
 * into URB space as a constant URB entry (CURBE) so that it can be accessed
 * quickly at thread setup time.  Each individual fixed function unit's state
 * (brw_vs_state.c for example) tells the hardware which subset of the CURBE
 * it wants in its register space, and we calculate those areas here under the
 * BRW_NEW_PUSH_CONSTANT_ALLOCATION state flag.  The brw_urb.c allocation will control
 * how many CURBEs can be loaded into the hardware at once before a pipeline
 * stall occurs at CMD_CONST_BUFFER time.
 *
 * On gfx6+, constant handling becomes a much simpler set of per-unit state.
 * See gfx6_upload_vec4_push_constants() in gfx6_vs_state.c for that code.
 */


#include "compiler/nir/nir.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/enums.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_statevars.h"
#include "util/bitscan.h"
#include "brw_batch.h"
#include "brw_buffer_objects.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_util.h"
#include "util/u_math.h"


/**
 * Partition the CURBE between the various users of constant values.
 *
 * If the users all fit within the previous allocatation, we avoid changing
 * the layout because that means reuploading all unit state and uploading new
 * constant buffers.
 */
static void calculate_curbe_offsets( struct brw_context *brw )
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FS_PROG_DATA */
   const GLuint nr_fp_regs = (brw->wm.base.prog_data->nr_params + 15) / 16;

   /* BRW_NEW_VS_PROG_DATA */
   const GLuint nr_vp_regs = (brw->vs.base.prog_data->nr_params + 15) / 16;
   GLuint nr_clip_regs = 0;
   GLuint total_regs;

   /* _NEW_TRANSFORM */
   if (ctx->Transform.ClipPlanesEnabled) {
      GLuint nr_planes = 6 + util_bitcount(ctx->Transform.ClipPlanesEnabled);
      nr_clip_regs = (nr_planes * 4 + 15) / 16;
   }


   total_regs = nr_fp_regs + nr_vp_regs + nr_clip_regs;

   /* The CURBE allocation size is limited to 32 512-bit units (128 EU
    * registers, or 1024 floats).  See CS_URB_STATE in the gfx4 or gfx5
    * (volume 1, part 1) PRMs.
    *
    * Note that in brw_fs.cpp we're only loading up to 16 EU registers of
    * values as push constants before spilling to pull constants, and in
    * brw_vec4.cpp we're loading up to 32 registers of push constants.  An EU
    * register is 1/2 of one of these URB entry units, so that leaves us 16 EU
    * regs for clip.
    */
   assert(total_regs <= 32);

   /* Lazy resize:
    */
   if (nr_fp_regs > brw->curbe.wm_size ||
       nr_vp_regs > brw->curbe.vs_size ||
       nr_clip_regs != brw->curbe.clip_size ||
       (total_regs < brw->curbe.total_size / 4 &&
        brw->curbe.total_size > 16)) {

      GLuint reg = 0;

      /* Calculate a new layout:
       */
      reg = 0;
      brw->curbe.wm_start = reg;
      brw->curbe.wm_size = nr_fp_regs; reg += nr_fp_regs;
      brw->curbe.clip_start = reg;
      brw->curbe.clip_size = nr_clip_regs; reg += nr_clip_regs;
      brw->curbe.vs_start = reg;
      brw->curbe.vs_size = nr_vp_regs; reg += nr_vp_regs;
      brw->curbe.total_size = reg;

      if (0)
         fprintf(stderr, "curbe wm %d+%d clip %d+%d vs %d+%d\n",
                 brw->curbe.wm_start,
                 brw->curbe.wm_size,
                 brw->curbe.clip_start,
                 brw->curbe.clip_size,
                 brw->curbe.vs_start,
                 brw->curbe.vs_size );

      brw->ctx.NewDriverState |= BRW_NEW_PUSH_CONSTANT_ALLOCATION;
   }
}


const struct brw_tracked_state brw_curbe_offsets = {
   .dirty = {
      .mesa = _NEW_TRANSFORM,
      .brw  = BRW_NEW_CONTEXT |
              BRW_NEW_BLORP |
              BRW_NEW_FS_PROG_DATA |
              BRW_NEW_VS_PROG_DATA,
   },
   .emit = calculate_curbe_offsets
};




/** Uploads the CS_URB_STATE packet.
 *
 * Just like brw_vs_state.c and brw_wm_state.c define a URB entry size and
 * number of entries for their stages, constant buffers do so using this state
 * packet.  Having multiple CURBEs in the URB at the same time allows the
 * hardware to avoid a pipeline stall between primitives using different
 * constant buffer contents.
 */
void brw_upload_cs_urb_state(struct brw_context *brw)
{
   BEGIN_BATCH(2);
   OUT_BATCH(CMD_CS_URB_STATE << 16 | (2-2));

   /* BRW_NEW_URB_FENCE */
   if (brw->urb.csize == 0) {
      OUT_BATCH(0);
   } else {
      /* BRW_NEW_URB_FENCE */
      assert(brw->urb.nr_cs_entries);
      OUT_BATCH((brw->urb.csize - 1) << 4 | brw->urb.nr_cs_entries);
   }
   ADVANCE_BATCH();
}

static const GLfloat fixed_plane[6][4] = {
   { 0,    0,   -1, 1 },
   { 0,    0,    1, 1 },
   { 0,   -1,    0, 1 },
   { 0,    1,    0, 1 },
   {-1,    0,    0, 1 },
   { 1,    0,    0, 1 }
};

/**
 * Gathers together all the uniform values into a block of memory to be
 * uploaded into the CURBE, then emits the state packet telling the hardware
 * the new location.
 */
static void
brw_upload_constant_buffer(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_PUSH_CONSTANT_ALLOCATION */
   const GLuint sz = brw->curbe.total_size;
   const GLuint bufsz = sz * 16 * sizeof(GLfloat);
   gl_constant_value *buf;
   GLuint i;
   gl_clip_plane *clip_planes;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct gl_program *fp = brw->programs[MESA_SHADER_FRAGMENT];

   /* BRW_NEW_VERTEX_PROGRAM */
   struct gl_program *vp = brw->programs[MESA_SHADER_VERTEX];

   if (sz == 0) {
      goto emit;
   }

   buf = brw_upload_space(&brw->upload, bufsz, 64,
                          &brw->curbe.curbe_bo, &brw->curbe.curbe_offset);

   STATIC_ASSERT(sizeof(gl_constant_value) == sizeof(float));

   /* fragment shader constants */
   if (brw->curbe.wm_size) {
      _mesa_load_state_parameters(ctx, fp->Parameters);

      /* BRW_NEW_PUSH_CONSTANT_ALLOCATION */
      GLuint offset = brw->curbe.wm_start * 16;

      /* BRW_NEW_FS_PROG_DATA | _NEW_PROGRAM_CONSTANTS: copy uniform values */
      brw_populate_constant_data(brw, fp, &brw->wm.base, &buf[offset],
                                 brw->wm.base.prog_data->param,
                                 brw->wm.base.prog_data->nr_params);
   }

   /* clipper constants */
   if (brw->curbe.clip_size) {
      GLuint offset = brw->curbe.clip_start * 16;
      GLbitfield mask;

      /* If any planes are going this way, send them all this way:
       */
      for (i = 0; i < 6; i++) {
         buf[offset + i * 4 + 0].f = fixed_plane[i][0];
         buf[offset + i * 4 + 1].f = fixed_plane[i][1];
         buf[offset + i * 4 + 2].f = fixed_plane[i][2];
         buf[offset + i * 4 + 3].f = fixed_plane[i][3];
      }

      /* Clip planes: _NEW_TRANSFORM plus _NEW_PROJECTION to get to
       * clip-space:
       */
      clip_planes = brw_select_clip_planes(ctx);
      mask = ctx->Transform.ClipPlanesEnabled;
      while (mask) {
         const int j = u_bit_scan(&mask);
         buf[offset + i * 4 + 0].f = clip_planes[j][0];
         buf[offset + i * 4 + 1].f = clip_planes[j][1];
         buf[offset + i * 4 + 2].f = clip_planes[j][2];
         buf[offset + i * 4 + 3].f = clip_planes[j][3];
         i++;
      }
   }

   /* vertex shader constants */
   if (brw->curbe.vs_size) {
      _mesa_load_state_parameters(ctx, vp->Parameters);

      GLuint offset = brw->curbe.vs_start * 16;

      /* BRW_NEW_VS_PROG_DATA | _NEW_PROGRAM_CONSTANTS: copy uniform values */
      brw_populate_constant_data(brw, vp, &brw->vs.base, &buf[offset],
                                 brw->vs.base.prog_data->param,
                                 brw->vs.base.prog_data->nr_params);
   }

   if (0) {
      for (i = 0; i < sz*16; i+=4)
         fprintf(stderr, "curbe %d.%d: %f %f %f %f\n", i/8, i&4,
                 buf[i+0].f, buf[i+1].f, buf[i+2].f, buf[i+3].f);
   }

   /* Because this provokes an action (ie copy the constants into the
    * URB), it shouldn't be shortcircuited if identical to the
    * previous time - because eg. the urb destination may have
    * changed, or the urb contents different to last time.
    *
    * Note that the data referred to is actually copied internally,
    * not just used in place according to passed pointer.
    *
    * It appears that the CS unit takes care of using each available
    * URB entry (Const URB Entry == CURBE) in turn, and issuing
    * flushes as necessary when doublebuffering of CURBEs isn't
    * possible.
    */

emit:
   /* BRW_NEW_URB_FENCE: From the gfx4 PRM, volume 1, section 3.9.8
    * (CONSTANT_BUFFER (CURBE Load)):
    *
    *     "Modifying the CS URB allocation via URB_FENCE invalidates any
    *      previous CURBE entries. Therefore software must subsequently
    *      [re]issue a CONSTANT_BUFFER command before CURBE data can be used
    *      in the pipeline."
    */
   BEGIN_BATCH(2);
   if (brw->curbe.total_size == 0) {
      OUT_BATCH((CMD_CONST_BUFFER << 16) | (2 - 2));
      OUT_BATCH(0);
   } else {
      OUT_BATCH((CMD_CONST_BUFFER << 16) | (1 << 8) | (2 - 2));
      OUT_RELOC(brw->curbe.curbe_bo, 0,
                (brw->curbe.total_size - 1) + brw->curbe.curbe_offset);
   }
   ADVANCE_BATCH();

   /* Work around a Broadwater/Crestline depth interpolator bug.  The
    * following sequence will cause GPU hangs:
    *
    * 1. Change state so that all depth related fields in CC_STATE are
    *    disabled, and in WM_STATE, only "PS Use Source Depth" is enabled.
    * 2. Emit a CONSTANT_BUFFER packet.
    * 3. Draw via 3DPRIMITIVE.
    *
    * The recommended workaround is to emit a non-pipelined state change after
    * emitting CONSTANT_BUFFER, in order to drain the windowizer pipeline.
    *
    * We arbitrarily choose 3DSTATE_GLOBAL_DEPTH_CLAMP_OFFSET (as it's small),
    * and always emit it when "PS Use Source Depth" is set.  We could be more
    * precise, but the additional complexity is probably not worth it.
    *
    * BRW_NEW_FRAGMENT_PROGRAM
    */
   if (devinfo->ver == 4 && !devinfo->is_g4x &&
       BITSET_TEST(fp->info.system_values_read, SYSTEM_VALUE_FRAG_COORD)) {
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_constant_buffer = {
   .dirty = {
      .mesa = _NEW_PROGRAM_CONSTANTS,
      .brw  = BRW_NEW_BATCH |
              BRW_NEW_BLORP |
              BRW_NEW_PUSH_CONSTANT_ALLOCATION |
              BRW_NEW_FRAGMENT_PROGRAM |
              BRW_NEW_FS_PROG_DATA |
              BRW_NEW_PSP | /* Implicit - hardware requires this, not used above */
              BRW_NEW_URB_FENCE |
              BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_upload_constant_buffer,
};
