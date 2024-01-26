/*
 * Copyright © 2013 Intel Corporation
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

#ifndef BRW_MULTISAMPLE_STATE_H
#define BRW_MULTISAMPLE_STATE_H

#include <stdint.h>

/**
 * Note: There are no standard multisample positions defined in OpenGL
 * specifications. Implementations have the freedom to pick the positions
 * which give plausible results. But the Vulkan specification does define
 * standard sample positions. So, we decided to pick the same pattern in
 * OpenGL as in Vulkan to keep it uniform across drivers and also to avoid
 * breaking applications which rely on this standard pattern.
 */

/**
 * 1x MSAA has a single sample at the center: (0.5, 0.5) -> (0x8, 0x8).
 *
 * 2x MSAA sample positions are (0.75, 0.75) and (0.25, 0.25):
 *   4 c
 * 4 1
 * c   0
 */
static const uint32_t
brw_multisample_positions_1x_2x = 0x008844cc;

/**
 * Sample positions:
 *   2 6 a e
 * 2   0
 * 6       1
 * a 2
 * e     3
 */
static const uint32_t
brw_multisample_positions_4x = 0xae2ae662;

/**
 * Sample positions:
 *
 * From the Ivy Bridge PRM, Vol2 Part1 p304 (3DSTATE_MULTISAMPLE:
 * Programming Notes):
 *     "When programming the sample offsets (for NUMSAMPLES_4 or _8 and
 *     MSRASTMODE_xxx_PATTERN), the order of the samples 0 to 3 (or 7
 *     for 8X) must have monotonically increasing distance from the
 *     pixel center. This is required to get the correct centroid
 *     computation in the device."
 *
 * Sample positions:
 *   1 3 5 7 9 b d f
 * 1               7
 * 3     3
 * 5         0
 * 7 5
 * 9             2
 * b       1
 * d   4
 * f           6
 */
static const uint32_t
brw_multisample_positions_8x[] = { 0x53d97b95, 0xf1bf173d };

/**
 * Sample positions:
 *
 *    0 1 2 3 4 5 6 7 8 9 a b c d e f
 * 0   15
 * 1                  9
 * 2         10
 * 3                        7
 * 4                               13
 * 5                1
 * 6        4
 * 7                          3
 * 8 12
 * 9                    0
 * a            2
 * b                            6
 * c     11
 * d                      5
 * e              8
 * f                             14
 */
static const uint32_t
brw_multisample_positions_16x[] = {
   0xc75a7599, 0xb3dbad36, 0x2c42816e, 0x10eff408
};

#endif /* BRW_MULTISAMPLE_STATE_H */
