/*
 * Copyright © 2009 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "compiler/brw_eu_defines.h"
#include "brw_util.h"
#include "brw_batch.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"

bool
brw_is_drawing_points(const struct brw_context *brw)
{
   /* Determine if the primitives *reaching the SF* are points */
   /* _NEW_POLYGON */
   if (brw->ctx.Polygon.FrontMode == GL_POINT ||
       brw->ctx.Polygon.BackMode == GL_POINT) {
      return true;
   }

   if (brw->gs.base.prog_data) {
      /* BRW_NEW_GS_PROG_DATA */
      return brw_gs_prog_data(brw->gs.base.prog_data)->output_topology ==
             _3DPRIM_POINTLIST;
   } else if (brw->tes.base.prog_data) {
      /* BRW_NEW_TES_PROG_DATA */
      return brw_tes_prog_data(brw->tes.base.prog_data)->output_topology ==
             BRW_TESS_OUTPUT_TOPOLOGY_POINT;
   } else {
      /* BRW_NEW_PRIMITIVE */
      return brw->primitive == _3DPRIM_POINTLIST;
   }
}

bool
brw_is_drawing_lines(const struct brw_context *brw)
{
   /* Determine if the primitives *reaching the SF* are points */
   /* _NEW_POLYGON */
   if (brw->ctx.Polygon.FrontMode == GL_LINE ||
       brw->ctx.Polygon.BackMode == GL_LINE) {
      return true;
   }

   if (brw->gs.base.prog_data) {
      /* BRW_NEW_GS_PROG_DATA */
      return brw_gs_prog_data(brw->gs.base.prog_data)->output_topology ==
             _3DPRIM_LINESTRIP;
   } else if (brw->tes.base.prog_data) {
      /* BRW_NEW_TES_PROG_DATA */
      return brw_tes_prog_data(brw->tes.base.prog_data)->output_topology ==
             BRW_TESS_OUTPUT_TOPOLOGY_LINE;
   } else {
      /* BRW_NEW_PRIMITIVE */
      switch (brw->primitive) {
      case _3DPRIM_LINELIST:
      case _3DPRIM_LINESTRIP:
      case _3DPRIM_LINELOOP:
         return true;
      }
   }
   return false;
}
