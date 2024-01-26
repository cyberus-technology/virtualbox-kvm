/*
 * Copyright 2003 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "main/context.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/dd.h"

#include "brw_screen.h"
#include "brw_context.h"
#include "brw_defines.h"

int
brw_translate_shadow_compare_func(GLenum func)
{
   /* GL specifies the result of shadow comparisons as:
    *     1     if   ref <op> texel,
    *     0     otherwise.
    *
    * The hardware does:
    *     0     if texel <op> ref,
    *     1     otherwise.
    *
    * So, these look a bit strange because there's both a negation
    * and swapping of the arguments involved.
    */
   switch (func) {
   case GL_NEVER:
      return BRW_COMPAREFUNCTION_ALWAYS;
   case GL_LESS:
      return BRW_COMPAREFUNCTION_LEQUAL;
   case GL_LEQUAL:
      return BRW_COMPAREFUNCTION_LESS;
   case GL_GREATER:
      return BRW_COMPAREFUNCTION_GEQUAL;
   case GL_GEQUAL:
      return BRW_COMPAREFUNCTION_GREATER;
   case GL_NOTEQUAL:
      return BRW_COMPAREFUNCTION_EQUAL;
   case GL_EQUAL:
      return BRW_COMPAREFUNCTION_NOTEQUAL;
   case GL_ALWAYS:
      return BRW_COMPAREFUNCTION_NEVER;
   }

   unreachable("Invalid shadow comparison function.");
}

int
brw_translate_compare_func(GLenum func)
{
   switch (func) {
   case GL_NEVER:
      return BRW_COMPAREFUNCTION_NEVER;
   case GL_LESS:
      return BRW_COMPAREFUNCTION_LESS;
   case GL_LEQUAL:
      return BRW_COMPAREFUNCTION_LEQUAL;
   case GL_GREATER:
      return BRW_COMPAREFUNCTION_GREATER;
   case GL_GEQUAL:
      return BRW_COMPAREFUNCTION_GEQUAL;
   case GL_NOTEQUAL:
      return BRW_COMPAREFUNCTION_NOTEQUAL;
   case GL_EQUAL:
      return BRW_COMPAREFUNCTION_EQUAL;
   case GL_ALWAYS:
      return BRW_COMPAREFUNCTION_ALWAYS;
   }

   unreachable("Invalid comparison function.");
}

int
brw_translate_stencil_op(GLenum op)
{
   switch (op) {
   case GL_KEEP:
      return BRW_STENCILOP_KEEP;
   case GL_ZERO:
      return BRW_STENCILOP_ZERO;
   case GL_REPLACE:
      return BRW_STENCILOP_REPLACE;
   case GL_INCR:
      return BRW_STENCILOP_INCRSAT;
   case GL_DECR:
      return BRW_STENCILOP_DECRSAT;
   case GL_INCR_WRAP:
      return BRW_STENCILOP_INCR;
   case GL_DECR_WRAP:
      return BRW_STENCILOP_DECR;
   case GL_INVERT:
      return BRW_STENCILOP_INVERT;
   default:
      return BRW_STENCILOP_ZERO;
   }
}
