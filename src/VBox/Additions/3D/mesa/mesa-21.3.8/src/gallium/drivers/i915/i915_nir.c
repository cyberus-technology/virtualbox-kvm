/*
 * Copyright © 2021 Emma Anholt
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
 */

#include "compiler/nir/nir.h"
#include "compiler/nir/nir_builder.h"
#include "i915_fpc.h"

static bool
i915_sincos_filter(const nir_instr *instr, const void *data)
{
   if (instr->type != nir_instr_type_alu)
      return false;

   switch (nir_instr_as_alu(instr)->op) {
   case nir_op_fcos:
   case nir_op_fsin:
      return true;
   default:
      return false;
   }
}

/* Compute sin using a quadratic and quartic.  It gives continuity
 * that repeating the Taylor series lacks every 2*pi, and has
 * reduced error.
 *
 * The idea was described at:
 * https://web.archive.org/web/20100613230051/http://www.devmaster.net/forums/showthread.php?t=5784
 */
static nir_ssa_def *
i915_sincos_lower(nir_builder *b, nir_instr *instr, void *data)
{
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   nir_ssa_def *x = nir_ssa_for_alu_src(b, alu, 0);

   /* Reduce range from repeating about [-pi,pi] to [-1,1] */
   x = nir_fmul_imm(b, x, M_1_PI / 2.0);
   if (alu->op == nir_op_fsin)
      x = nir_fadd_imm(b, x, 0.5);
   else
      x = nir_fadd_imm(b, x, 0.75);
   x = nir_ffract(b, x);
   x = nir_fadd_imm(b, nir_fmul_imm(b, x, 2.0), -1.0);

   nir_ssa_def *x_absx = nir_fmul(b, x, nir_fabs(b, x));

   /* y is the first approximation of the result. */
   nir_ssa_def *y =
      nir_fadd(b, nir_fmul_imm(b, x, 4.0), nir_fmul_imm(b, x_absx, -4.0));

   /* improve the accuracy. */
   nir_ssa_def *y_absy = nir_fmul(b, y, nir_fabs(b, y));
   return nir_fadd(b, nir_fmul_imm(b, nir_fsub(b, y_absy, y), 0.225), y);
}

bool
i915_nir_lower_sincos(nir_shader *s)
{
   return nir_shader_lower_instructions(s, i915_sincos_filter,
                                        i915_sincos_lower, NULL);
}
