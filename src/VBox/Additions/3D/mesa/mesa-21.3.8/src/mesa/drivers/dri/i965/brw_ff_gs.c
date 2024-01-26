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

#include "main/macros.h"
#include "main/enums.h"
#include "main/transformfeedback.h"

#include "brw_batch.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_ff_gs.h"
#include "util/ralloc.h"

static void
compile_ff_gs_prog(struct brw_context *brw,
		   struct brw_ff_gs_prog_key *key)
{
   const GLuint *program;
   void *mem_ctx;
   GLuint program_size;

   mem_ctx = ralloc_context(NULL);

   struct brw_ff_gs_prog_data prog_data;
   program = brw_compile_ff_gs_prog(brw->screen->compiler, mem_ctx, key,
                                    &prog_data,
                                    &brw_vue_prog_data(brw->vs.base.prog_data)->vue_map,
                                    &program_size);

   brw_upload_cache(&brw->cache, BRW_CACHE_FF_GS_PROG,
                    key, sizeof(*key),
                    program, program_size,
                    &prog_data, sizeof(prog_data),
                    &brw->ff_gs.prog_offset, &brw->ff_gs.prog_data);
   ralloc_free(mem_ctx);
}

static bool
brw_ff_gs_state_dirty(const struct brw_context *brw)
{
   return brw_state_dirty(brw,
                          _NEW_LIGHT,
                          BRW_NEW_PRIMITIVE |
                          BRW_NEW_TRANSFORM_FEEDBACK |
                          BRW_NEW_VS_PROG_DATA);
}

static void
brw_ff_gs_populate_key(struct brw_context *brw,
                       struct brw_ff_gs_prog_key *key)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   static const unsigned swizzle_for_offset[4] = {
      BRW_SWIZZLE4(0, 1, 2, 3),
      BRW_SWIZZLE4(1, 2, 3, 3),
      BRW_SWIZZLE4(2, 3, 3, 3),
      BRW_SWIZZLE4(3, 3, 3, 3)
   };

   struct gl_context *ctx = &brw->ctx;

   assert(devinfo->ver < 7);

   memset(key, 0, sizeof(*key));

   /* BRW_NEW_VS_PROG_DATA (part of VUE map) */
   key->attrs = brw_vue_prog_data(brw->vs.base.prog_data)->vue_map.slots_valid;

   /* BRW_NEW_PRIMITIVE */
   key->primitive = brw->primitive;

   /* _NEW_LIGHT */
   key->pv_first = (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION);
   if (key->primitive == _3DPRIM_QUADLIST && ctx->Light.ShadeModel != GL_FLAT) {
      /* Provide consistent primitive order with brw_set_prim's
       * optimization of single quads to trifans.
       */
      key->pv_first = true;
   }

   if (devinfo->ver == 6) {
      /* On Gfx6, GS is used for transform feedback. */
      /* BRW_NEW_TRANSFORM_FEEDBACK */
      if (_mesa_is_xfb_active_and_unpaused(ctx)) {
         const struct gl_program *prog =
            ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];
         const struct gl_transform_feedback_info *linked_xfb_info =
            prog->sh.LinkedTransformFeedback;
         int i;

         /* Make sure that the VUE slots won't overflow the unsigned chars in
          * key->transform_feedback_bindings[].
          */
         STATIC_ASSERT(BRW_VARYING_SLOT_COUNT <= 256);

         /* Make sure that we don't need more binding table entries than we've
          * set aside for use in transform feedback.  (We shouldn't, since we
          * set aside enough binding table entries to have one per component).
          */
         assert(linked_xfb_info->NumOutputs <= BRW_MAX_SOL_BINDINGS);

         key->need_gs_prog = true;
         key->num_transform_feedback_bindings = linked_xfb_info->NumOutputs;
         for (i = 0; i < key->num_transform_feedback_bindings; ++i) {
            key->transform_feedback_bindings[i] =
               linked_xfb_info->Outputs[i].OutputRegister;
            key->transform_feedback_swizzles[i] =
               swizzle_for_offset[linked_xfb_info->Outputs[i].ComponentOffset];
         }
      }
   } else {
      /* Pre-gfx6, GS is used to transform QUADLIST, QUADSTRIP, and LINELOOP
       * into simpler primitives.
       */
      key->need_gs_prog = (brw->primitive == _3DPRIM_QUADLIST ||
                           brw->primitive == _3DPRIM_QUADSTRIP ||
                           brw->primitive == _3DPRIM_LINELOOP);
   }
}

/* Calculate interpolants for triangle and line rasterization.
 */
void
brw_upload_ff_gs_prog(struct brw_context *brw)
{
   struct brw_ff_gs_prog_key key;

   if (!brw_ff_gs_state_dirty(brw))
      return;

   /* Populate the key:
    */
   brw_ff_gs_populate_key(brw, &key);

   if (brw->ff_gs.prog_active != key.need_gs_prog) {
      brw->ctx.NewDriverState |= BRW_NEW_FF_GS_PROG_DATA;
      brw->ff_gs.prog_active = key.need_gs_prog;
   }

   if (brw->ff_gs.prog_active) {
      if (!brw_search_cache(&brw->cache, BRW_CACHE_FF_GS_PROG, &key,
                            sizeof(key), &brw->ff_gs.prog_offset,
                            &brw->ff_gs.prog_data, true)) {
         compile_ff_gs_prog(brw, &key);
      }
   }
}
