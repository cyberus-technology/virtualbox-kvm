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

/**
 * 3DSTATE_SAMPLE_PATTERN
 */
void
gfx8_emit_3dstate_sample_pattern(struct brw_context *brw)
{
   BEGIN_BATCH(9);
   OUT_BATCH(_3DSTATE_SAMPLE_PATTERN << 16 | (9 - 2));

   /* 16x MSAA */
   OUT_BATCH(brw_multisample_positions_16x[0]); /* positions  3,  2,  1,  0 */
   OUT_BATCH(brw_multisample_positions_16x[1]); /* positions  7,  6,  5,  4 */
   OUT_BATCH(brw_multisample_positions_16x[2]); /* positions 11, 10,  9,  8 */
   OUT_BATCH(brw_multisample_positions_16x[3]); /* positions 15, 14, 13, 12 */

   /* 8x MSAA */
   OUT_BATCH(brw_multisample_positions_8x[1]); /* sample positions 7654 */
   OUT_BATCH(brw_multisample_positions_8x[0]); /* sample positions 3210 */

   /* 4x MSAA */
   OUT_BATCH(brw_multisample_positions_4x);

   /* 1x and 2x MSAA */
   OUT_BATCH(brw_multisample_positions_1x_2x);
   ADVANCE_BATCH();
}
