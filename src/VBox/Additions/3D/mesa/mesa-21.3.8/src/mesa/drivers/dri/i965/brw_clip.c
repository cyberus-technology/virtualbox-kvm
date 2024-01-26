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

#include "brw_batch.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_util.h"
#include "brw_state.h"
#include "compiler/brw_eu.h"

#include "util/ralloc.h"

static void
compile_clip_prog(struct brw_context *brw, struct brw_clip_prog_key *key)
{
   const unsigned *program;
   void *mem_ctx;
   unsigned program_size;

   mem_ctx = ralloc_context(NULL);

   struct brw_clip_prog_data prog_data;
   program = brw_compile_clip(brw->screen->compiler, mem_ctx, key, &prog_data,
                              &brw->vue_map_geom_out, &program_size);

   brw_upload_cache(&brw->cache,
                    BRW_CACHE_CLIP_PROG,
                    key, sizeof(*key),
                    program, program_size,
                    &prog_data, sizeof(prog_data),
                    &brw->clip.prog_offset, &brw->clip.prog_data);
   ralloc_free(mem_ctx);
}

/* Calculate interpolants for triangle and line rasterization.
 */
void
brw_upload_clip_prog(struct brw_context *brw)
{
   const struct intel_device_info *devinfo = &brw->screen->devinfo;
   struct gl_context *ctx = &brw->ctx;
   struct brw_clip_prog_key key;

   if (!brw_state_dirty(brw,
                        _NEW_BUFFERS |
                        _NEW_LIGHT |
                        _NEW_POLYGON |
                        _NEW_TRANSFORM,
                        BRW_NEW_BLORP |
                        BRW_NEW_FS_PROG_DATA |
                        BRW_NEW_REDUCED_PRIMITIVE |
                        BRW_NEW_VUE_MAP_GEOM_OUT))
      return;

   memset(&key, 0, sizeof(key));

   /* Populate the key:
    */

   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   if (wm_prog_data) {
      key.contains_flat_varying = wm_prog_data->contains_flat_varying;
      key.contains_noperspective_varying =
         wm_prog_data->contains_noperspective_varying;

      STATIC_ASSERT(sizeof(key.interp_mode) ==
                    sizeof(wm_prog_data->interp_mode));
      memcpy(key.interp_mode, wm_prog_data->interp_mode,
             sizeof(key.interp_mode));
   }

   /* BRW_NEW_REDUCED_PRIMITIVE */
   key.primitive = brw->reduced_primitive;
   /* BRW_NEW_VUE_MAP_GEOM_OUT */
   key.attrs = brw->vue_map_geom_out.slots_valid;

   /* _NEW_LIGHT */
   key.pv_first = (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION);
   /* _NEW_TRANSFORM (also part of VUE map)*/
   if (ctx->Transform.ClipPlanesEnabled)
      key.nr_userclip = util_logbase2(ctx->Transform.ClipPlanesEnabled) + 1;

   if (devinfo->ver == 5)
       key.clip_mode = BRW_CLIP_MODE_KERNEL_CLIP;
   else
       key.clip_mode = BRW_CLIP_MODE_NORMAL;

   /* _NEW_POLYGON */
   if (key.primitive == GL_TRIANGLES) {
      if (ctx->Polygon.CullFlag &&
          ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
         key.clip_mode = BRW_CLIP_MODE_REJECT_ALL;
      else {
         GLuint fill_front = BRW_CLIP_FILL_MODE_CULL;
         GLuint fill_back = BRW_CLIP_FILL_MODE_CULL;
         GLuint offset_front = 0;
         GLuint offset_back = 0;

         if (!ctx->Polygon.CullFlag ||
             ctx->Polygon.CullFaceMode != GL_FRONT) {
            switch (ctx->Polygon.FrontMode) {
            case GL_FILL:
               fill_front = BRW_CLIP_FILL_MODE_FILL;
               offset_front = 0;
               break;
            case GL_LINE:
               fill_front = BRW_CLIP_FILL_MODE_LINE;
               offset_front = ctx->Polygon.OffsetLine;
               break;
            case GL_POINT:
               fill_front = BRW_CLIP_FILL_MODE_POINT;
               offset_front = ctx->Polygon.OffsetPoint;
               break;
            }
         }

         if (!ctx->Polygon.CullFlag ||
             ctx->Polygon.CullFaceMode != GL_BACK) {
            switch (ctx->Polygon.BackMode) {
            case GL_FILL:
               fill_back = BRW_CLIP_FILL_MODE_FILL;
               offset_back = 0;
               break;
            case GL_LINE:
               fill_back = BRW_CLIP_FILL_MODE_LINE;
               offset_back = ctx->Polygon.OffsetLine;
               break;
            case GL_POINT:
               fill_back = BRW_CLIP_FILL_MODE_POINT;
               offset_back = ctx->Polygon.OffsetPoint;
               break;
            }
         }

         if (ctx->Polygon.BackMode != GL_FILL ||
             ctx->Polygon.FrontMode != GL_FILL) {
            key.do_unfilled = 1;

            /* Most cases the fixed function units will handle.  Cases where
             * one or more polygon faces are unfilled will require help:
             */
            key.clip_mode = BRW_CLIP_MODE_CLIP_NON_REJECTED;

            if (offset_back || offset_front) {
               /* _NEW_POLYGON, _NEW_BUFFERS */
               key.offset_units = ctx->Polygon.OffsetUnits * ctx->DrawBuffer->_MRD * 2;
               key.offset_factor = ctx->Polygon.OffsetFactor * ctx->DrawBuffer->_MRD;
               key.offset_clamp = ctx->Polygon.OffsetClamp * ctx->DrawBuffer->_MRD;
            }

            if (!brw->polygon_front_bit) {
               key.fill_ccw = fill_front;
               key.fill_cw = fill_back;
               key.offset_ccw = offset_front;
               key.offset_cw = offset_back;
               if (ctx->Light.Model.TwoSide &&
                   key.fill_cw != BRW_CLIP_FILL_MODE_CULL)
                  key.copy_bfc_cw = 1;
            } else {
               key.fill_cw = fill_front;
               key.fill_ccw = fill_back;
               key.offset_cw = offset_front;
               key.offset_ccw = offset_back;
               if (ctx->Light.Model.TwoSide &&
                   key.fill_ccw != BRW_CLIP_FILL_MODE_CULL)
                  key.copy_bfc_ccw = 1;
            }
         }
      }
   }

   if (!brw_search_cache(&brw->cache, BRW_CACHE_CLIP_PROG, &key, sizeof(key),
                         &brw->clip.prog_offset, &brw->clip.prog_data, true)) {
      compile_clip_prog( brw, &key );
   }
}
