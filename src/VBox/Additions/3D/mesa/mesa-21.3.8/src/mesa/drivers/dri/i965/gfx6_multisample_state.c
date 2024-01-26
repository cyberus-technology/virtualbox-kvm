/*
 * Copyright © 2012 Intel Corporation
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

#include "brw_batch.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_multisample_state.h"
#include "main/framebuffer.h"

void
gfx6_get_sample_position(struct gl_context *ctx,
                         struct gl_framebuffer *fb,
                         GLuint index, GLfloat *result)
{
   uint8_t bits;

   switch (_mesa_geometric_samples(fb)) {
   case 1:
      result[0] = result[1] = 0.5f;
      return;
   case 2:
      bits = brw_multisample_positions_1x_2x >> (8 * index);
      break;
   case 4:
      bits = brw_multisample_positions_4x >> (8 * index);
      break;
   case 8:
      bits = brw_multisample_positions_8x[index >> 2] >> (8 * (index & 3));
      break;
   case 16:
      bits = brw_multisample_positions_16x[index >> 2] >> (8 * (index & 3));
      break;
   default:
      unreachable("Not implemented");
   }

   /* Convert from U0.4 back to a floating point coordinate. */
   result[0] = ((bits >> 4) & 0xf) / 16.0f;
   result[1] = (bits & 0xf) / 16.0f;
}
