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


#ifndef BRW_UTIL_H
#define BRW_UTIL_H

#include "brw_context.h"
#include "main/framebuffer.h"

extern GLuint brw_translate_blend_factor( GLenum factor );
extern GLuint brw_translate_blend_equation( GLenum mode );

static inline float
brw_get_line_width(struct brw_context *brw)
{
   /* From the OpenGL 4.4 spec:
    *
    * "The actual width of non-antialiased lines is determined by rounding
    * the supplied width to the nearest integer, then clamping it to the
    * implementation-dependent maximum non-antialiased line width."
    */
   float line_width =
      CLAMP(!_mesa_is_multisample_enabled(&brw->ctx) && !brw->ctx.Line.SmoothFlag
            ? roundf(brw->ctx.Line.Width) : brw->ctx.Line.Width,
            0.125f, brw->ctx.Const.MaxLineWidth);

   if (!_mesa_is_multisample_enabled(&brw->ctx) && brw->ctx.Line.SmoothFlag && line_width < 1.5f) {
      /* For 1 pixel line thickness or less, the general
       * anti-aliasing algorithm gives up, and a garbage line is
       * generated.  Setting a Line Width of 0.0 specifies the
       * rasterization of the "thinnest" (one-pixel-wide),
       * non-antialiased lines.
       *
       * Lines rendered with zero Line Width are rasterized using
       * Grid Intersection Quantization rules as specified by
       * bspec section 6.3.12.1 Zero-Width (Cosmetic) Line
       * Rasterization.
       */
      line_width = 0.0f;
   }

   return line_width;
}

#endif
