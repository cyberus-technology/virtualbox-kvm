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

#include "compiler/nir/nir.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/state.h"

#include "brw_batch.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_util.h"
#include "brw_state.h"
#include "compiler/brw_eu.h"

#include "util/ralloc.h"

static void
compile_sf_prog(struct brw_context *brw, struct brw_sf_prog_key *key)
{
   const unsigned *program;
   void *mem_ctx;
   unsigned program_size;

   mem_ctx = ralloc_context(NULL);

   struct brw_sf_prog_data prog_data;
   program = brw_compile_sf(brw->screen->compiler, mem_ctx, key, &prog_data,
                            &brw->vue_map_geom_out, &program_size);

   brw_upload_cache(&brw->cache, BRW_CACHE_SF_PROG,
                    key, sizeof(*key),
                    program, program_size,
                    &prog_data, sizeof(prog_data),
                    &brw->sf.prog_offset, &brw->sf.prog_data);
   ralloc_free(mem_ctx);
}

/* Calculate interpolants for triangle and line rasterization.
 */
void
brw_upload_sf_prog(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_sf_prog_key key;

   if (!brw_state_dirty(brw,
                        _NEW_BUFFERS |
                        _NEW_HINT |
                        _NEW_LIGHT |
                        _NEW_POINT |
                        _NEW_POLYGON |
                        _NEW_PROGRAM |
                        _NEW_TRANSFORM,
                        BRW_NEW_BLORP |
                        BRW_NEW_FS_PROG_DATA |
                        BRW_NEW_REDUCED_PRIMITIVE |
                        BRW_NEW_VUE_MAP_GEOM_OUT))
      return;

   /* _NEW_BUFFERS */
   bool flip_y = ctx->DrawBuffer->FlipY;

   memset(&key, 0, sizeof(key));

   /* Populate the key, noting state dependencies:
    */
   /* BRW_NEW_VUE_MAP_GEOM_OUT */
   key.attrs = brw->vue_map_geom_out.slots_valid;

   /* BRW_NEW_REDUCED_PRIMITIVE */
   switch (brw->reduced_primitive) {
   case GL_TRIANGLES:
      /* NOTE: We just use the edgeflag attribute as an indicator that
       * unfilled triangles are active.  We don't actually do the
       * edgeflag testing here, it is already done in the clip
       * program.
       */
      if (key.attrs & BITFIELD64_BIT(VARYING_SLOT_EDGE))
         key.primitive = BRW_SF_PRIM_UNFILLED_TRIS;
      else
         key.primitive = BRW_SF_PRIM_TRIANGLES;
      break;
   case GL_LINES:
      key.primitive = BRW_SF_PRIM_LINES;
      break;
   case GL_POINTS:
      key.primitive = BRW_SF_PRIM_POINTS;
      break;
   }

   /* _NEW_TRANSFORM */
   key.userclip_active = (ctx->Transform.ClipPlanesEnabled != 0);

   /* _NEW_POINT */
   key.do_point_sprite = ctx->Point.PointSprite;
   if (key.do_point_sprite) {
      key.point_sprite_coord_replace = ctx->Point.CoordReplace & 0xff;
   }
   if (brw->programs[MESA_SHADER_FRAGMENT]->info.inputs_read &
       BITFIELD64_BIT(VARYING_SLOT_PNTC)) {
      key.do_point_coord = 1;
   }

   /*
    * Window coordinates in a FBO are inverted, which means point
    * sprite origin must be inverted, too.
    */
   if ((ctx->Point.SpriteOrigin == GL_LOWER_LEFT) == flip_y)
      key.sprite_origin_lower_left = true;

   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *wm_prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   if (wm_prog_data) {
      key.contains_flat_varying = wm_prog_data->contains_flat_varying;

      STATIC_ASSERT(sizeof(key.interp_mode) ==
                    sizeof(wm_prog_data->interp_mode));
      memcpy(key.interp_mode, wm_prog_data->interp_mode,
             sizeof(key.interp_mode));
   }

   /* _NEW_LIGHT | _NEW_PROGRAM */
   key.do_twoside_color = _mesa_vertex_program_two_side_enabled(ctx);

   /* _NEW_POLYGON */
   if (key.do_twoside_color) {
      /* If we're rendering to a FBO, we have to invert the polygon
       * face orientation, just as we invert the viewport in
       * sf_unit_create_from_key().
       */
      key.frontface_ccw = brw->polygon_front_bit != flip_y;
   }

   if (!brw_search_cache(&brw->cache, BRW_CACHE_SF_PROG, &key, sizeof(key),
                         &brw->sf.prog_offset, &brw->sf.prog_data, true)) {
      compile_sf_prog( brw, &key );
   }
}
