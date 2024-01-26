/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@vmware.com>
 */
#include <stdbool.h>

/**
 * \file t_dd_dmatmp.h
 * Template for render stages which build and emit vertices directly
 * to fixed-size dma buffers.  Useful for rendering strips and other
 * native primitives where clipping and per-vertex tweaks such as
 * those in t_dd_tritmp.h are not required.
 *
 * Produces code for both inline triangles and indexed triangles.
 * Where various primitive types are unaccelerated by hardware, the
 * code attempts to fallback to other primitive types (quadstrips to
 * tristrips, lineloops to linestrips), or to indexed vertices.
 */

#if !HAVE_TRIANGLES || !HAVE_LINES || !HAVE_LINE_STRIPS || !HAVE_TRI_STRIPS || !HAVE_TRI_FANS
#error "must have lines, line strips, triangles, triangle fans, and triangle strips to use render template"
#endif

#if HAVE_QUAD_STRIPS || HAVE_QUADS || HAVE_ELTS
#error "ELTs, quads, and quad strips not supported by render template"
#endif


/**********************************************************************/
/*                  Render whole begin/end objects                    */
/**********************************************************************/

static inline void *TAG(emit_verts)(struct gl_context *ctx, GLuint start,
                                    GLuint count, void *buf)
{
   return EMIT_VERTS(ctx, start, count, buf);
}

/***********************************************************************
 *                    Render non-indexed primitives.
 ***********************************************************************/

static void TAG(render_points_verts)(struct gl_context *ctx,
                                     GLuint start,
                                     GLuint count,
                                     GLuint flags)
{
   if (HAVE_POINTS) {
      LOCAL_VARS;
      const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      unsigned currentsz;
      GLuint j, nr;

      INIT(GL_POINTS);

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8)
         currentsz = dmasz;

      for (j = 0; j < count; j += nr) {
         nr = MIN2(currentsz, count - j);
         TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
         currentsz = dmasz;
      }
   } else {
      unreachable("Cannot draw primitive; validate_render should have "
                  "prevented this");
   }
}

static void TAG(render_lines_verts)(struct gl_context *ctx,
                                    GLuint start,
                                    GLuint count,
                                    GLuint flags)
{
   LOCAL_VARS;
   const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS() & ~1;
   unsigned currentsz;
   GLuint j, nr;

   INIT(GL_LINES);

   /* Emit whole number of lines in total and in each buffer:
    */
   count -= count & 1;
   currentsz = GET_CURRENT_VB_MAX_VERTS();
   currentsz -= currentsz & 1;

   if (currentsz < 8)
      currentsz = dmasz;

   for (j = 0; j < count; j += nr) {
      nr = MIN2(currentsz, count - j);
      TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
      currentsz = dmasz;
   }
}


static void TAG(render_line_strip_verts)(struct gl_context *ctx,
                                         GLuint start,
                                         GLuint count,
                                         GLuint flags)
{
   LOCAL_VARS;
   const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
   unsigned currentsz;
   GLuint j, nr;

   INIT(GL_LINE_STRIP);

   currentsz = GET_CURRENT_VB_MAX_VERTS();
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = 0; j + 1 < count; j += nr - 1) {
      nr = MIN2(currentsz, count - j);
      TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
      currentsz = dmasz;
   }
 
   FLUSH();
}


static void TAG(render_line_loop_verts)(struct gl_context *ctx,
                                        GLuint start,
                                        GLuint count,
                                        GLuint flags)
{
   LOCAL_VARS;
   const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS() - 1;
   unsigned currentsz;
   GLuint j, nr;

   INIT(GL_LINE_STRIP);

   j = (flags & PRIM_BEGIN) ? 0 : 1;

   /* Ensure last vertex won't wrap buffers:
    */
   currentsz = GET_CURRENT_VB_MAX_VERTS();
   currentsz--;

   if (currentsz < 8)
      currentsz = dmasz;

   if (j + 1 < count) {
      for (/* empty */; j + 1 < count; j += nr - 1) {
         nr = MIN2(currentsz, count - j);

         if (j + nr >= count &&
             count > 1 &&
             (flags & PRIM_END)) {
            void *tmp;
            tmp = ALLOC_VERTS(nr+1);
            tmp = TAG(emit_verts)(ctx, start + j, nr, tmp);
            tmp = TAG(emit_verts)( ctx, start, 1, tmp );
            (void) tmp;
         } else {
            TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
            currentsz = dmasz;
         }
      }
   } else if (count > 1 && (flags & PRIM_END)) {
      void *tmp;
      tmp = ALLOC_VERTS(2);
      tmp = TAG(emit_verts)( ctx, start+1, 1, tmp );
      tmp = TAG(emit_verts)( ctx, start, 1, tmp );
      (void) tmp;
   }

   FLUSH();
}


static void TAG(render_triangles_verts)(struct gl_context *ctx,
                                        GLuint start,
                                        GLuint count,
                                        GLuint flags)
{
   LOCAL_VARS;
   const unsigned dmasz = (GET_SUBSEQUENT_VB_MAX_VERTS() / 3) * 3;
   unsigned currentsz;
   GLuint j, nr;

   INIT(GL_TRIANGLES);

   currentsz = (GET_CURRENT_VB_MAX_VERTS() / 3) * 3;

   /* Emit whole number of tris in total.  dmasz is already a multiple
    * of 3.
    */
   count -= count % 3;

   if (currentsz < 8)
      currentsz = dmasz;

   for (j = 0; j < count; j += nr) {
      nr = MIN2(currentsz, count - j);
      TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
      currentsz = dmasz;
   }
}



static void TAG(render_tri_strip_verts)(struct gl_context *ctx,
                                        GLuint start,
                                        GLuint count,
                                        GLuint flags)
{
   LOCAL_VARS;
   GLuint j, nr;
   const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS() & ~1;
   unsigned currentsz;

   INIT(GL_TRIANGLE_STRIP);

   currentsz = GET_CURRENT_VB_MAX_VERTS();

   if (currentsz < 8)
      currentsz = dmasz;

   /* From here on emit even numbers of tris when wrapping over buffers:
    */
   currentsz -= (currentsz & 1);

   for (j = 0; j + 2 < count; j += nr - 2) {
      nr = MIN2(currentsz, count - j);
      TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
      currentsz = dmasz;
   }

   FLUSH();
}

static void TAG(render_tri_fan_verts)(struct gl_context *ctx,
                                      GLuint start,
                                      GLuint count,
                                      GLuint flags)
{
   LOCAL_VARS;
   GLuint j, nr;
   const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
   unsigned currentsz;

   INIT(GL_TRIANGLE_FAN);

   currentsz = GET_CURRENT_VB_MAX_VERTS();
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = 1; j + 1 < count; j += nr - 2) {
      void *tmp;
      nr = MIN2(currentsz, count - j + 1);
      tmp = ALLOC_VERTS(nr);
      tmp = TAG(emit_verts)(ctx, start, 1, tmp);
      tmp = TAG(emit_verts)(ctx, start + j, nr - 1, tmp);
      (void) tmp;
      currentsz = dmasz;
   }

   FLUSH();
}


static void TAG(render_poly_verts)(struct gl_context *ctx,
                                   GLuint start,
                                   GLuint count,
                                   GLuint flags)
{
   if (HAVE_POLYGONS) {
      LOCAL_VARS;
      GLuint j, nr;
      const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      unsigned currentsz;

      INIT(GL_POLYGON);

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8) {
         currentsz = dmasz;
      }

      for (j = 1; j + 1 < count; j += nr - 2) {
         void *tmp;
         nr = MIN2(currentsz, count - j + 1);
         tmp = ALLOC_VERTS(nr);
         tmp = TAG(emit_verts)(ctx, start, 1, tmp);
         tmp = TAG(emit_verts)(ctx, start + j, nr - 1, tmp);
         (void) tmp;
         currentsz = dmasz;
      }

      FLUSH();
   } else if (ctx->Light.ShadeModel == GL_SMOOTH ||
              ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION) {
      TAG(render_tri_fan_verts)( ctx, start, count, flags );
   } else {
      unreachable("Cannot draw primitive; validate_render should have "
                  "prevented this");
   }
}

static void TAG(render_quad_strip_verts)(struct gl_context *ctx,
                                         GLuint start,
                                         GLuint count,
                                         GLuint flags)
{
   GLuint j, nr;

   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      LOCAL_VARS;
      const unsigned dmasz = GET_SUBSEQUENT_VB_MAX_VERTS() & ~1;
      unsigned currentsz;

      /* Emit smooth-shaded quadstrips as tristrips:
       */
      FLUSH();
      INIT(GL_TRIANGLE_STRIP);

      /* Emit whole number of quads in total, and in each buffer.
       */
      currentsz = GET_CURRENT_VB_MAX_VERTS();
      currentsz -= currentsz & 1;
      count -= count & 1;

      if (currentsz < 8)
         currentsz = dmasz;

      for (j = 0; j + 3 < count; j += nr - 2) {
         nr = MIN2(currentsz, count - j);
         TAG(emit_verts)(ctx, start + j, nr, ALLOC_VERTS(nr));
         currentsz = dmasz;
      }

      FLUSH();
   } else {
      unreachable("Cannot draw primitive; validate_render should have "
                  "prevented this");
   }
}


static void TAG(render_quads_verts)(struct gl_context *ctx,
                                    GLuint start,
                                    GLuint count,
                                    GLuint flags)
{
   if (ctx->Light.ShadeModel == GL_SMOOTH ||
       ctx->Light.ProvokingVertex == GL_LAST_VERTEX_CONVENTION) {
      LOCAL_VARS;
      GLuint j;

      /* Emit whole number of quads in total. */
      count -= count & 3;

      /* Hardware doesn't have a quad primitive type -- try to simulate it using
       * triangle primitive.  This is a win for gears, but is it useful in the
       * broader world?
       */
      INIT(GL_TRIANGLES);

      for (j = 0; j + 3 < count; j += 4) {
         void *tmp = ALLOC_VERTS(6);
         /* Send v0, v1, v3
          */
         tmp = EMIT_VERTS(ctx, start + j,     2, tmp);
         tmp = EMIT_VERTS(ctx, start + j + 3, 1, tmp);
         /* Send v1, v2, v3
          */
         tmp = EMIT_VERTS(ctx, start + j + 1, 3, tmp);
         (void) tmp;
      }
   } else {
      unreachable("Cannot draw primitive");
   }
}

static void TAG(render_noop)(struct gl_context *ctx,
                             GLuint start,
                             GLuint count,
                             GLuint flags)
{
   (void) ctx;
   (void) start;
   (void) count;
   (void) flags;
}

static const tnl_render_func TAG(render_tab_verts)[GL_POLYGON+2] =
{
   TAG(render_points_verts),
   TAG(render_lines_verts),
   TAG(render_line_loop_verts),
   TAG(render_line_strip_verts),
   TAG(render_triangles_verts),
   TAG(render_tri_strip_verts),
   TAG(render_tri_fan_verts),
   TAG(render_quads_verts),
   TAG(render_quad_strip_verts),
   TAG(render_poly_verts),
   TAG(render_noop),
};

/* Pre-check the primitives in the VB to prevent the need for
 * fallbacks later on.
 */
static bool TAG(validate_render)(struct gl_context *ctx,
                                 struct vertex_buffer *VB)
{
   GLint i;

   if (VB->ClipOrMask & ~CLIP_CULL_BIT)
      return false;

   if (VB->Elts)
      return false;

   for (i = 0 ; i < VB->PrimitiveCount ; i++) {
      GLuint prim = VB->Primitive[i].mode;
      GLuint count = VB->Primitive[i].count;
      bool ok = false;

      if (!count)
         continue;

      switch (prim & PRIM_MODE_MASK) {
      case GL_POINTS:
         ok = HAVE_POINTS;
         break;
      case GL_LINES:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
         ok = !ctx->Line.StippleFlag;
         break;
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
         ok = true;
         break;
      case GL_POLYGON:
         ok = (HAVE_POLYGONS) || ctx->Light.ShadeModel == GL_SMOOTH ||
              ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION;
         break;
      case GL_QUAD_STRIP:
         ok = VB->Elts || ctx->Light.ShadeModel == GL_SMOOTH;
         break;
      case GL_QUADS:
         ok = ctx->Light.ShadeModel == GL_SMOOTH ||
              ctx->Light.ProvokingVertex == GL_LAST_VERTEX_CONVENTION;
         break;
      default:
         break;
      }
      
      if (!ok) {
/*          fprintf(stderr, "not ok %s\n", _mesa_enum_to_string(prim & PRIM_MODE_MASK)); */
         return false;
      }
   }

   return true;
}

