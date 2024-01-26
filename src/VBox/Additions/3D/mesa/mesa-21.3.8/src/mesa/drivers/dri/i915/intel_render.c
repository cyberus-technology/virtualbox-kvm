/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware acceleration where possible.
 *
 */
#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/enums.h"

#include "math/m_xform.h"

#include "tnl/t_context.h"
#include "tnl/t_vertex.h"
#include "tnl/t_pipeline.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_tris.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1

#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_ELTS        0

static const uint32_t hw_prim[GL_POLYGON + 1] = {
   [GL_POINTS] = PRIM3D_POINTLIST,
   [GL_LINES ] = PRIM3D_LINELIST,
   [GL_LINE_LOOP] = PRIM3D_LINESTRIP,
   [GL_LINE_STRIP] = PRIM3D_LINESTRIP,
   [GL_TRIANGLES] = PRIM3D_TRILIST,
   [GL_TRIANGLE_STRIP] = PRIM3D_TRISTRIP,
   [GL_TRIANGLE_FAN] = PRIM3D_TRIFAN,
   [GL_QUADS] = 0,
   [GL_QUAD_STRIP] = 0,
   [GL_POLYGON] = PRIM3D_POLY,
};

static const GLenum reduced_prim[GL_POLYGON + 1] = {
   [GL_POINTS] = GL_POINTS,
   [GL_LINES] = GL_LINES,
   [GL_LINE_LOOP] = GL_LINES,
   [GL_LINE_STRIP] = GL_LINES,
   [GL_TRIANGLES] = GL_TRIANGLES,
   [GL_TRIANGLE_STRIP] = GL_TRIANGLES,
   [GL_TRIANGLE_FAN] = GL_TRIANGLES,
   [GL_QUADS] = GL_TRIANGLES,
   [GL_QUAD_STRIP] = GL_TRIANGLES,
   [GL_POLYGON] = GL_TRIANGLES,
};

static const int scale_prim[GL_POLYGON + 1] = {
   [GL_POINTS] = 1,
   [GL_LINES] = 1,
   [GL_LINE_LOOP] = 2,
   [GL_LINE_STRIP] = 2,
   [GL_TRIANGLES] = 1,
   [GL_TRIANGLE_STRIP] = 3,
   [GL_TRIANGLE_FAN] = 3,
   [GL_QUADS] = 0,              /* fallback case */
   [GL_QUAD_STRIP] = 0,         /* fallback case */
   [GL_POLYGON] = 3,
};


static void
intelDmaPrimitive(struct intel_context *intel, GLenum prim)
{
   if (0)
      fprintf(stderr, "%s %s\n", __func__, _mesa_enum_to_string(prim));
   INTEL_FIREVERTICES(intel);
   intel->vtbl.reduced_primitive_state(intel, reduced_prim[prim]);
   intel_set_prim(intel, hw_prim[prim]);
}

#define INTEL_NO_VBO_STATE_RESERVED 1500

static inline GLuint intel_get_vb_max(struct intel_context *intel)
{
   GLuint ret;

   if (intel->intelScreen->no_vbo) {
      ret = intel->batch.bo->size - INTEL_NO_VBO_STATE_RESERVED;
   } else
      ret = INTEL_VB_SIZE;
   ret /= (intel->vertex_size * 4);
   return ret;
}

static inline GLuint intel_get_current_max(struct intel_context *intel)
{
   GLuint ret;

   if (intel->intelScreen->no_vbo) {
      ret = intel_batchbuffer_space(intel);
      ret = ret <= INTEL_NO_VBO_STATE_RESERVED ? 0 : ret - INTEL_NO_VBO_STATE_RESERVED;
   } else
      ret = (INTEL_VB_SIZE - intel->prim.current_offset);

   return ret / (intel->vertex_size * 4);
}

#define LOCAL_VARS struct intel_context *intel = intel_context(ctx)
#define INIT( prim ) 				\
do {						\
   intelDmaPrimitive( intel, prim );		\
} while (0)

#define FLUSH() INTEL_FIREVERTICES(intel)

#define GET_SUBSEQUENT_VB_MAX_VERTS() intel_get_vb_max(intel)
#define GET_CURRENT_VB_MAX_VERTS() intel_get_current_max(intel)

#define ALLOC_VERTS(nr) intel_get_prim_space(intel, nr)

#define EMIT_VERTS( ctx, j, nr, buf ) \
  _tnl_emit_vertices_to_buffer(ctx, j, (j)+(nr), buf )

#define TAG(x) intel_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/

/* Heuristic to choose between the two render paths:
 */
static bool
choose_render(struct intel_context *intel, struct vertex_buffer *VB)
{
   int vertsz = intel->vertex_size;
   int cost_render = 0;
   int cost_fallback = 0;
   int nr_prims = 0;
   int nr_rprims = 0;
   int nr_rverts = 0;
   int rprim = intel->reduced_primitive;
   int i = 0;

   for (i = 0; i < VB->PrimitiveCount; i++) {
      GLuint prim = VB->Primitive[i].mode;
      GLuint length = VB->Primitive[i].count;

      if (!length)
         continue;

      nr_prims++;
      nr_rverts += length * scale_prim[prim & PRIM_MODE_MASK];

      if (reduced_prim[prim & PRIM_MODE_MASK] != rprim) {
         nr_rprims++;
         rprim = reduced_prim[prim & PRIM_MODE_MASK];
      }
   }

   /* One point for each generated primitive:
    */
   cost_render = nr_prims;
   cost_fallback = nr_rprims;

   /* One point for every 1024 dwords (4k) of dma:
    */
   cost_render += (vertsz * i) / 1024;
   cost_fallback += (vertsz * nr_rverts) / 1024;

   if (0)
      fprintf(stderr, "cost render: %d fallback: %d\n",
              cost_render, cost_fallback);

   if (cost_render > cost_fallback)
      return false;

   return true;
}


static GLboolean
intel_run_render(struct gl_context * ctx, struct tnl_pipeline_stage *stage)
{
   struct intel_context *intel = intel_context(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   intel->vtbl.render_prevalidate( intel );

   /* Don't handle clipping or indexed vertices.
    */
   if (intel->RenderIndex != 0 ||
       !intel_validate_render(ctx, VB) || !choose_render(intel, VB)) {
      return true;
   }

   tnl->clipspace.new_inputs |= VERT_BIT_POS;

   tnl->Driver.Render.Start(ctx);

   for (i = 0; i < VB->PrimitiveCount; i++) {
      GLuint prim = _tnl_translate_prim(&VB->Primitive[i]);
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
         continue;

      intel_render_tab_verts[prim & PRIM_MODE_MASK] (ctx, start,
                                                     length, prim);
   }

   tnl->Driver.Render.Finish(ctx);

   INTEL_FIREVERTICES(intel);

   return false;             /* finished the pipe */
}

static const struct tnl_pipeline_stage _intel_render_stage = {
   "intel render",
   NULL,
   NULL,
   NULL,
   NULL,
   intel_run_render             /* run */
};

const struct tnl_pipeline_stage *intel_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_vertex_program_stage,
#if 1
   &_intel_render_stage,        /* ADD: unclipped rastersetup-to-dma */
#endif
   &_tnl_render_stage,
   0,
};
