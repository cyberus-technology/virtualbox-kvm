/*
 * Copyright © 2017 Gert Wollny
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "st_tests_common.h"

#include "tgsi/tgsi_ureg.h"
#include "tgsi/tgsi_info.h"
#include "mesa/program/prog_instruction.h"
#include "gtest/gtest.h"

#include <utility>
#include <algorithm>
#include <iostream>

using std::vector;

using namespace tgsi_array_merge;
using ArrayLiveRangeMerge=testing::Test;

TEST_F(ArrayLiveRangeMerge, SimpleLiveRange)
{
   array_live_range a1(1, 10, 1, 5, WRITEMASK_X);
   array_live_range a2(2, 5, 6, 10, WRITEMASK_X);

   array_live_range::merge(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 10);
   EXPECT_EQ(a1.target_array_id(), 0);
   EXPECT_EQ(a1.used_components(), 1);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 6);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 1);
   EXPECT_EQ(a2.used_components(), 1);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a2.remap_one_swizzle(0), 0);
   EXPECT_EQ(a2.remap_one_swizzle(1), 1);
   EXPECT_EQ(a2.remap_one_swizzle(2), 2);
   EXPECT_EQ(a2.remap_one_swizzle(3), 3);
}

TEST_F(ArrayLiveRangeMerge, SimpleLiveRangeInverse)
{
   array_live_range a1(1, 5, 1, 5, WRITEMASK_X);
   array_live_range a2(2, 10, 6, 10, WRITEMASK_X);

   array_live_range::merge(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 5);
   EXPECT_EQ(a1.target_array_id(), 2);
   EXPECT_EQ(a1.used_components(), 1);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 0);
   EXPECT_EQ(a2.used_components(), 1);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a2.remap_one_swizzle(0), 0);
   EXPECT_EQ(a2.remap_one_swizzle(1), 1);
   EXPECT_EQ(a2.remap_one_swizzle(2), 2);
   EXPECT_EQ(a2.remap_one_swizzle(3), 3);
}


TEST_F(ArrayLiveRangeMerge, Interleave_x_xyz)
{
   array_live_range a1(1, 10, 1, 10, WRITEMASK_X);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_XYZ);

   array_live_range::interleave(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 10);
   EXPECT_EQ(a1.array_length(), 10u);
   EXPECT_EQ(a1.target_array_id(), 0);
   EXPECT_EQ(a1.used_components(), 4);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_XYZW);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 1);

   EXPECT_EQ(a2.remap_one_swizzle(0), 1);
   EXPECT_EQ(a2.remap_one_swizzle(1), 2);
   EXPECT_EQ(a2.remap_one_swizzle(2), 3);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);
}

TEST_F(ArrayLiveRangeMerge, Interleave_xyz_x)
{
   array_live_range a1(1, 10, 1, 10, WRITEMASK_XYZ);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_X);

   array_live_range::interleave(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 10);
   EXPECT_EQ(a1.array_length(), 10u);
   EXPECT_EQ(a1.target_array_id(), 0);
   EXPECT_EQ(a1.used_components(), 4);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_XYZW);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 1);

   EXPECT_EQ(a2.remap_one_swizzle(0), 3);
   EXPECT_EQ(a2.remap_one_swizzle(1), -1);
   EXPECT_EQ(a2.remap_one_swizzle(2), -1);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);
}


TEST_F(ArrayLiveRangeMerge, SimpleInterleave)
{
   array_live_range a1(1, 10, 1, 10, WRITEMASK_X);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_X);

   array_live_range::interleave(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 10);
   EXPECT_EQ(a1.array_length(), 10u);
   EXPECT_EQ(a1.target_array_id(), 0);
   EXPECT_EQ(a1.used_components(), 2);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 1);

   EXPECT_EQ(a2.remap_one_swizzle(0), 1);
   EXPECT_EQ(a2.remap_one_swizzle(1), -1);
   EXPECT_EQ(a2.remap_one_swizzle(2), -1);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);
}


TEST_F(ArrayLiveRangeMerge, SimpleInterleaveInverse)
{
   array_live_range a1(1, 8, 1, 10, WRITEMASK_X);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_X);

   array_live_range::interleave(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 10);
   EXPECT_EQ(a1.target_array_id(), 2);

   EXPECT_EQ(a1.remap_one_swizzle(0), 1);
   EXPECT_EQ(a1.remap_one_swizzle(1), -1);
   EXPECT_EQ(a1.remap_one_swizzle(2), -1);
   EXPECT_EQ(a1.remap_one_swizzle(3), -1);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.target_array_id(), 0);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.array_length(), 9u);
   EXPECT_EQ(a2.used_components(), 2);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_XY);
}


TEST_F(ArrayLiveRangeMerge, InterleaveRiveRangeExtend)
{
   array_live_range a1(1, 10, 2, 9, WRITEMASK_X);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_X);

   array_live_range::interleave(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 10);
   EXPECT_EQ(a1.array_length(), 10u);
   EXPECT_EQ(a1.target_array_id(), 0);
   EXPECT_EQ(a1.used_components(), 2);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 1);

   EXPECT_EQ(a2.remap_one_swizzle(0), 1);
   EXPECT_EQ(a2.remap_one_swizzle(1), -1);
   EXPECT_EQ(a2.remap_one_swizzle(2), -1);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);
}

TEST_F(ArrayLiveRangeMerge, InterleaveLiveRangeExtendInverse)
{
   array_live_range a1(1, 8, 2, 11, WRITEMASK_X);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_X);

   array_live_range::interleave(&a1, &a2);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 2);
   EXPECT_EQ(a1.end(), 11);
   EXPECT_EQ(a1.target_array_id(), 2);
   EXPECT_EQ(a1.used_components(), 1);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a1.remap_one_swizzle(0), 1);
   EXPECT_EQ(a1.remap_one_swizzle(1), -1);
   EXPECT_EQ(a1.remap_one_swizzle(2), -1);
   EXPECT_EQ(a1.remap_one_swizzle(3), -1);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 11);
   EXPECT_EQ(a2.target_array_id(), 0);
   EXPECT_EQ(a2.used_components(), 2);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a2.remap_one_swizzle(0), 0);
   EXPECT_EQ(a2.remap_one_swizzle(1), 1);
   EXPECT_EQ(a2.remap_one_swizzle(2), 2);
   EXPECT_EQ(a2.remap_one_swizzle(3), 3);
}

TEST_F(ArrayLiveRangeMerge, InterleaveChained)
{
   array_live_range a1(1, 8, 2, 11, WRITEMASK_X);
   array_live_range a2(2, 9, 1, 10, WRITEMASK_X);
   array_live_range a3(3, 10, 1, 10, WRITEMASK_X);

   array_live_range::interleave(&a1, &a2);
   array_live_range::interleave(&a2, &a3);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 2);
   EXPECT_EQ(a1.end(), 11);
   EXPECT_EQ(a1.target_array_id(), 2);
   EXPECT_EQ(a1.used_components(), 1);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a1.remap_one_swizzle(0), 2);
   EXPECT_EQ(a1.remap_one_swizzle(1), -1);
   EXPECT_EQ(a1.remap_one_swizzle(2), -1);
   EXPECT_EQ(a1.remap_one_swizzle(3), -1);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 11);
   EXPECT_EQ(a2.target_array_id(), 3);
   EXPECT_EQ(a2.used_components(), 2);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a2.remap_one_swizzle(0), 1);
   EXPECT_EQ(a2.remap_one_swizzle(1), 2);
   EXPECT_EQ(a2.remap_one_swizzle(2), -1);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);

   EXPECT_EQ(a3.array_id(), 3);
   EXPECT_EQ(a3.begin(), 1);
   EXPECT_EQ(a3.end(), 11);
   EXPECT_EQ(a3.target_array_id(), 0);
   EXPECT_EQ(a3.used_components(), 3);
   EXPECT_EQ(a3.access_mask(), WRITEMASK_XYZ);

   EXPECT_EQ(a3.remap_one_swizzle(0), 0);
   EXPECT_EQ(a3.remap_one_swizzle(1), 1);
   EXPECT_EQ(a3.remap_one_swizzle(2), 2);
   EXPECT_EQ(a3.remap_one_swizzle(3), 3);
}

TEST_F(ArrayLiveRangeMerge, MergeInterleaveChained)
{
   array_live_range a1(1, 8, 1, 5, WRITEMASK_X);
   array_live_range a2(2, 9, 6, 10, WRITEMASK_X);
   array_live_range a3(3, 10, 1, 10, WRITEMASK_X);

   array_live_range::merge(&a1, &a2);
   array_live_range::interleave(&a2, &a3);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 5);
   EXPECT_EQ(a1.target_array_id(), 2);
   EXPECT_EQ(a1.used_components(), 1);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a1.remap_one_swizzle(0), 1);
   EXPECT_EQ(a1.remap_one_swizzle(1), -1);
   EXPECT_EQ(a1.remap_one_swizzle(2), -1);
   EXPECT_EQ(a1.remap_one_swizzle(3), -1);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 3);
   EXPECT_EQ(a2.used_components(), 1);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a2.remap_one_swizzle(0), 1);
   EXPECT_EQ(a2.remap_one_swizzle(1), -1);
   EXPECT_EQ(a2.remap_one_swizzle(2), -1);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);

   EXPECT_EQ(a3.array_id(), 3);
   EXPECT_EQ(a3.begin(), 1);
   EXPECT_EQ(a3.end(), 10);
   EXPECT_EQ(a3.target_array_id(), 0);
   EXPECT_EQ(a3.used_components(), 2);
   EXPECT_EQ(a3.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a3.remap_one_swizzle(0), 0);
   EXPECT_EQ(a3.remap_one_swizzle(1), 1);
   EXPECT_EQ(a3.remap_one_swizzle(2), 2);
   EXPECT_EQ(a3.remap_one_swizzle(3), 3);
}

TEST_F(ArrayLiveRangeMerge, MergeMergeAndInterleave)
{
   array_live_range a1(1, 5, 1, 5, WRITEMASK_X);
   array_live_range a2(2, 4, 6, 7, WRITEMASK_X);
   array_live_range a3(3, 3, 1, 5, WRITEMASK_X);
   array_live_range a4(4, 2, 6, 8, WRITEMASK_X);

   array_live_range::merge(&a1, &a2);
   array_live_range::merge(&a3, &a4);
   array_live_range::interleave(&a1, &a3);

   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 8);
   EXPECT_EQ(a1.target_array_id(), 0);
   EXPECT_EQ(a1.used_components(), 2);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a1.remap_one_swizzle(0), 0);
   EXPECT_EQ(a1.remap_one_swizzle(1), 1);
   EXPECT_EQ(a1.remap_one_swizzle(2), 2);
   EXPECT_EQ(a1.remap_one_swizzle(3), 3);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 6);
   EXPECT_EQ(a2.end(), 7);
   EXPECT_EQ(a2.target_array_id(), 1);
   EXPECT_EQ(a2.used_components(), 1);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a2.remap_one_swizzle(0), 0);
   EXPECT_EQ(a2.remap_one_swizzle(1), 1);
   EXPECT_EQ(a2.remap_one_swizzle(2), 2);
   EXPECT_EQ(a2.remap_one_swizzle(3), 3);

   EXPECT_EQ(a3.array_id(), 3);
   EXPECT_EQ(a3.begin(), 1);
   EXPECT_EQ(a3.end(), 8);
   EXPECT_EQ(a3.target_array_id(), 1);
   EXPECT_EQ(a3.used_components(), 1);
   EXPECT_EQ(a3.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a3.remap_one_swizzle(0), 1);
   EXPECT_EQ(a3.remap_one_swizzle(1), -1);
   EXPECT_EQ(a3.remap_one_swizzle(2), -1);
   EXPECT_EQ(a3.remap_one_swizzle(3), -1);

   EXPECT_EQ(a4.array_id(), 4);
   EXPECT_EQ(a4.begin(), 6);
   EXPECT_EQ(a4.end(), 8);
   EXPECT_EQ(a4.target_array_id(), 3);
   EXPECT_EQ(a4.used_components(), 1);
   EXPECT_EQ(a4.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a4.remap_one_swizzle(0), 1);
   EXPECT_EQ(a4.remap_one_swizzle(1), -1);
   EXPECT_EQ(a4.remap_one_swizzle(2), -1);
   EXPECT_EQ(a4.remap_one_swizzle(3), -1);

}


TEST_F(ArrayLiveRangeMerge, MergeInterleaveMergeInterleaveChained)
{
   array_live_range a1(1, 8, 1, 5, WRITEMASK_X);
   array_live_range a2(2, 9, 6, 10, WRITEMASK_X);
   array_live_range a3(3, 10, 1, 10, WRITEMASK_X);
   array_live_range a4(4, 11, 11, 20, WRITEMASK_XY);
   array_live_range a5(5, 15, 5, 20, WRITEMASK_XY);

   array_live_range::merge(&a1, &a2);
   array_live_range::interleave(&a2, &a3);    // a2 -> a3
   array_live_range::merge(&a3, &a4);
   array_live_range::interleave(&a4, &a5);    // a4 -> a5


   EXPECT_EQ(a1.array_id(), 1);
   EXPECT_EQ(a1.begin(), 1);
   EXPECT_EQ(a1.end(), 5);
   EXPECT_EQ(a1.target_array_id(), 2);
   EXPECT_EQ(a1.used_components(), 1);
   EXPECT_EQ(a1.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a1.remap_one_swizzle(0), 3);
   EXPECT_EQ(a1.remap_one_swizzle(1), -1);
   EXPECT_EQ(a1.remap_one_swizzle(2), -1);
   EXPECT_EQ(a1.remap_one_swizzle(3), -1);

   EXPECT_EQ(a2.array_id(), 2);
   EXPECT_EQ(a2.begin(), 1);
   EXPECT_EQ(a2.end(), 10);
   EXPECT_EQ(a2.target_array_id(), 3);
   EXPECT_EQ(a2.used_components(), 1);
   EXPECT_EQ(a2.access_mask(), WRITEMASK_X);

   EXPECT_EQ(a2.remap_one_swizzle(0), 3);
   EXPECT_EQ(a2.remap_one_swizzle(1), -1);
   EXPECT_EQ(a2.remap_one_swizzle(2), -1);
   EXPECT_EQ(a2.remap_one_swizzle(3), -1);

   EXPECT_EQ(a3.array_id(), 3);
   EXPECT_EQ(a3.begin(), 1);
   EXPECT_EQ(a3.end(), 10);
   EXPECT_EQ(a3.target_array_id(), 4);
   EXPECT_EQ(a3.used_components(), 2);
   EXPECT_EQ(a3.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a3.remap_one_swizzle(0), 2);
   EXPECT_EQ(a3.remap_one_swizzle(1), 3);
   EXPECT_EQ(a3.remap_one_swizzle(2), -1);
   EXPECT_EQ(a3.remap_one_swizzle(3), -1);

   EXPECT_EQ(a4.array_id(), 4);
   EXPECT_EQ(a4.begin(), 1);
   EXPECT_EQ(a4.end(), 20);
   EXPECT_EQ(a4.target_array_id(), 5);
   EXPECT_EQ(a4.used_components(), 2);
   EXPECT_EQ(a4.access_mask(), WRITEMASK_XY);

   EXPECT_EQ(a4.remap_one_swizzle(0), 2);
   EXPECT_EQ(a4.remap_one_swizzle(1), 3);
   EXPECT_EQ(a4.remap_one_swizzle(2), -1);
   EXPECT_EQ(a4.remap_one_swizzle(3), -1);

   EXPECT_EQ(a5.array_id(), 5);
   EXPECT_EQ(a5.begin(), 1);
   EXPECT_EQ(a5.end(), 20);
   EXPECT_EQ(a5.target_array_id(), 0);
   EXPECT_EQ(a5.used_components(), 4);
   EXPECT_EQ(a5.access_mask(), WRITEMASK_XYZW);

   EXPECT_EQ(a5.remap_one_swizzle(0), 0);
   EXPECT_EQ(a5.remap_one_swizzle(1), 1);
   EXPECT_EQ(a5.remap_one_swizzle(2), 2);
   EXPECT_EQ(a5.remap_one_swizzle(3), 3);
}

using ArrayMergeTest=testing::Test;

TEST_F(ArrayMergeTest, ArrayMergeTwoSwizzles)
{
   vector<array_live_range> alt = {
      {1, 4, 1, 5, WRITEMASK_X},
      {2, 4, 2, 5, WRITEMASK_X},
   };

   int8_t expect_swizzle[] = {1, -1, -1, -1};
   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle},
   };

   vector<array_remapping> result(alt.size() + 1);

   get_array_remapping(2, &alt[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);

}

TEST_F(ArrayMergeTest, ArrayMergeFourSwizzles)
{
   vector<array_live_range> alt = {
      {1, 8, 1, 7, WRITEMASK_X},
      {2, 7, 2, 7, WRITEMASK_X},
      {3, 6, 3, 7, WRITEMASK_X},
      {4, 5, 4, 7, WRITEMASK_X},
   };
   int8_t expect_swizzle1[] = {1, -1, -1, -1};
   int8_t expect_swizzle2[] = {2, -1, -1, -1};
   int8_t expect_swizzle3[] = {3, -1, -1, -1};

   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle1},
      {1, expect_swizzle2},
      {1, expect_swizzle3},
   };

   vector<array_remapping> result(alt.size() + 1);

   get_array_remapping(4, &alt[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
   EXPECT_EQ(result[3], expect[2]);
   EXPECT_EQ(result[4], expect[3]);

}


TEST_F(ArrayMergeTest, SimpleChainMerge)
{
   vector<array_live_range> input = {
      {1, 3, 1, 5, WRITEMASK_XYZW},
      {2, 2, 6, 7, WRITEMASK_XYZW},
   };

   int8_t expect_swizzle[] = {0, 1, 2, 3};
   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle},
   };

   vector<array_remapping> result(3);
   get_array_remapping(2, &input[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
}

TEST_F(ArrayMergeTest, MergeAndInterleave)
{
   vector<array_live_range> input = {
      {1, 5, 1, 5, WRITEMASK_X},
      {2, 4, 6, 7, WRITEMASK_X},
      {3, 3, 1, 5, WRITEMASK_X},
      {4, 2, 6, 7, WRITEMASK_X},
   };

   int8_t expect_swizzle1[] = {0,  1,  2,  3};
   int8_t expect_swizzle2[] = {1, -1, -1, -1};
   int8_t expect_swizzle3[] = {1, -1, -1, -1};

   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle1},
      {1, expect_swizzle2},
      {1, expect_swizzle3}
   };
   vector<array_remapping> result(input.size() + 1);
   get_array_remapping(input.size(), &input[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
   EXPECT_EQ(result[3], expect[2]);
   EXPECT_EQ(result[4], expect[3]);
}

TEST_F(ArrayMergeTest, MergeAndInterleave2)
{
   vector<array_live_range> input = {
      {1, 5, 1, 5, WRITEMASK_X},
      {2, 4, 6, 7, WRITEMASK_X},
      {3, 3, 1, 8, WRITEMASK_XY},
      {4, 2, 6, 7, WRITEMASK_X},
   };

   int8_t expect_swizzle1[] = {0,  1,  2,  3};
   int8_t expect_swizzle2[] = {1,  2, -1, -1};
   int8_t expect_swizzle3[] = {3, -1, -1, -1};

   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle1},
      {1, expect_swizzle2},
      {1, expect_swizzle3}
   };
   vector<array_remapping> result(input.size() + 1);
   get_array_remapping(input.size(), &input[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
   EXPECT_EQ(result[3], expect[2]);
   EXPECT_EQ(result[4], expect[3]);
}


TEST_F(ArrayMergeTest, MergeAndInterleave3)
{
   vector<array_live_range> input = {
      {1, 5, 1, 5, WRITEMASK_X},
      {2, 4, 6, 7, WRITEMASK_XY},
      {3, 3, 1, 5, WRITEMASK_X}
   };

   int8_t expect_swizzle1[] = {0, 1, 2, 3};
   int8_t expect_swizzle2[] = {1, -1, -1, -1};

   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle1},
      {1, expect_swizzle2}
   };
   vector<array_remapping> result(input.size() + 1);
   get_array_remapping(input.size(), &input[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
   EXPECT_EQ(result[3], expect[2]);
}

TEST_F(ArrayMergeTest, MergeAndInterleave4)
{
   vector<array_live_range> input = {
      {1, 7, 1, 5, WRITEMASK_X},
      {2, 6, 6, 7, WRITEMASK_XY},
      {3, 5, 1, 5, WRITEMASK_X},
      {4, 4, 8, 9, WRITEMASK_XYZ},
      {5, 3, 8, 9, WRITEMASK_W},
      {6, 2, 10, 11, WRITEMASK_XYZW},
   };

   int8_t expect_swizzle1[] = {0, 1,  2,  3};
   int8_t expect_swizzle2[] = {1, -1, -1, -1};
   int8_t expect_swizzle3[] = {0, 1, 2, 3};
   int8_t expect_swizzle4[] = {-1, -1, -1, 3};
   int8_t expect_swizzle5[] = {0, 1, 2, 3};

   vector<array_remapping> expect = {
      {},
      {1, expect_swizzle1},
      {1, expect_swizzle2},
      {1, expect_swizzle3}, /* W from below will be interleaved in */
      {1, expect_swizzle4},
      {1, expect_swizzle5}
   };
   vector<array_remapping> result(input.size() + 1);
   get_array_remapping(input.size(), &input[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
   EXPECT_EQ(result[3], expect[2]);
   EXPECT_EQ(result[4], expect[3]);
   EXPECT_EQ(result[5], expect[4]);
   EXPECT_EQ(result[6], expect[5]);

}

TEST_F(ArrayMergeTest, MergeAndInterleave5)
{
   vector<array_live_range> input = {
      {1, 7, 1, 5, WRITEMASK_X},
      {2, 6, 1, 3, WRITEMASK_X},
      {3, 5, 4, 5, WRITEMASK_X},
      {4, 4, 6, 10, WRITEMASK_XY},
      {5, 8, 1, 10, WRITEMASK_XY}
   };

   /* 1. merge 3 into 2
    * 2. interleave 2 into 1 (x -> y) --- (y -> w)
    * 3. merge 4 into 1                 /
    * 4. interleave 1 into 5 (x,y - z,w)
    */

   /* swizzle1 holds the summary mask */
   int8_t expect_swizzle1[] = {2,  3, -1, -1};
   int8_t expect_swizzle2[] = {3, -1, -1, -1};
   int8_t expect_swizzle3[] = {3, -1, -1, -1};
   int8_t expect_swizzle4[] = {2,  3, -1, -1};

   vector<array_remapping> expect = {
      {5, expect_swizzle1},
      {5, expect_swizzle2},
      {5, expect_swizzle3},
      {5, expect_swizzle4},
      {}
   };
   vector<array_remapping> result(input.size() + 1);
   get_array_remapping(input.size(), &input[0], &result[0]);

   EXPECT_EQ(result[1], expect[0]);
   EXPECT_EQ(result[2], expect[1]);
   EXPECT_EQ(result[3], expect[2]);
   EXPECT_EQ(result[4], expect[3]);
   EXPECT_EQ(result[5], expect[4]);

}

/* Test two arrays life time simple */
TEST_F(LifetimeEvaluatorExactTest, TwoArraysSimple)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV , {MT(1, 1, WRITEMASK_XYZW)}, {MT(0, in0, "")}, {}, ARR()},
      { TGSI_OPCODE_MOV , {MT(2, 1, WRITEMASK_XYZW)}, {MT(0, in1, "")}, {}, ARR()},
      { TGSI_OPCODE_ADD , {MT(0,out0, WRITEMASK_XYZW)}, {MT(1,1,"xyzw"), MT(2,1,"xyzw")}, {}, ARR()},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1,2,0,2, WRITEMASK_XYZW}, {2,2,1,2, WRITEMASK_XYZW}}));
}

/* Test two arrays life time simple */
TEST_F(LifetimeEvaluatorExactTest, TwoArraysSimpleSwizzleX_Y)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV , {MT(1, 1, WRITEMASK_X)}, {MT(0, in0, "")}, {}, ARR()},
      { TGSI_OPCODE_MOV , {MT(2, 1, WRITEMASK_Y)}, {MT(0, in1, "")}, {}, ARR()},
      { TGSI_OPCODE_ADD , {MT(0,out0,1)}, {MT(1,1,"x"), MT(2,1,"y")}, {}, ARR()},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1, 2, 0, 2, WRITEMASK_X}, {2, 2, 1, 2, WRITEMASK_Y}}));
}

/* Test array written before loop and read inside, must survive the loop */
TEST_F(LifetimeEvaluatorExactTest, ArraysWriteBeforLoopReadInside)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV, {1}, {in1}, {}},
      { TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_X)}, {MT(0, in0, "")}, {}, ARR()},
      { TGSI_OPCODE_BGNLOOP },
      { TGSI_OPCODE_ADD, {MT(0,1, WRITEMASK_X)}, {MT(1,1,"x"), {MT(0,1, "x")}}, {}, ARR()},
      { TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_MOV, {out0}, {1}, {}},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1, 1, 1, 4, WRITEMASK_X}}));
}

/* Test array written conditionally in loop must survive the whole loop */
TEST_F(LifetimeEvaluatorExactTest, ArraysConditionalWriteInNestedLoop)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV, {1}, {in1}, {}},
      { TGSI_OPCODE_BGNLOOP },
      {   TGSI_OPCODE_BGNLOOP },
      {     TGSI_OPCODE_IF, {}, {1}, {}},
      {       TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_Z)}, {MT(0, in0, "")}, {}, ARR()},
      {     TGSI_OPCODE_ENDIF },
      {     TGSI_OPCODE_ADD, {MT(0,1, WRITEMASK_X)}, {MT(1,1,"z"), {MT(0,1, "x")}}, {}, ARR()},
      {   TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_MOV, {out0}, {1}, {}},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1, 1, 1, 8, WRITEMASK_Z}}));
}

/* Test array read conditionally in loop before write must
 * survive the whole loop
 */
TEST_F(LifetimeEvaluatorExactTest, ArraysConditionalReadBeforeWriteInNestedLoop)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV, {1}, {in1}, {}},
      { TGSI_OPCODE_BGNLOOP },
      {   TGSI_OPCODE_BGNLOOP },
      {     TGSI_OPCODE_IF, {}, {1}, {}},
      {     TGSI_OPCODE_ADD, {MT(0,1, WRITEMASK_X)}, {MT(1,1,"z"), {MT(0,1, "x")}}, {}, ARR()},
      {     TGSI_OPCODE_ENDIF },
      {       TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_Z)}, {MT(0, in0, "")}, {}, ARR()},
      {   TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_MOV, {out0}, {1}, {}},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1, 1, 1, 8, WRITEMASK_Z}}));
}


/* Test array written conditionally in loop must survive the whole loop */
TEST_F(LifetimeEvaluatorExactTest, ArraysConditionalWriteInNestedLoop2)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV, {1}, {in1}, {}},
      { TGSI_OPCODE_BGNLOOP },
      {   TGSI_OPCODE_BGNLOOP },
      {     TGSI_OPCODE_IF, {}, {1}, {}},
      {       TGSI_OPCODE_BGNLOOP },
      {         TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_Z)}, {MT(0, in0, "")}, {}, ARR()},
      {       TGSI_OPCODE_ENDLOOP },
      {     TGSI_OPCODE_ENDIF },
      {     TGSI_OPCODE_ADD, {MT(0,1, WRITEMASK_X)}, {MT(1,1,"z"), {MT(0,1, "x")}}, {}, ARR()},
      {   TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_MOV, {out0}, {1}, {}},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1, 1, 1, 10, WRITEMASK_Z}}));
}


/* Test distinct loops */
TEST_F(LifetimeEvaluatorExactTest, ArraysReadWriteInSeparateScopes)
{
   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV, {1}, {in1}, {}},
      { TGSI_OPCODE_BGNLOOP },
      {   TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_W)}, {MT(0, in0, "")}, {}, ARR()},
      { TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_BGNLOOP },
      {   TGSI_OPCODE_ADD, {MT(0,1, WRITEMASK_X)}, {MT(1,1,"w"), {MT(0,1, "x")}}, {}, ARR()},
      { TGSI_OPCODE_ENDLOOP },
      { TGSI_OPCODE_MOV, {out0}, {1}, {}},
      { TGSI_OPCODE_END}
   };
   run (code, array_lt_expect({{1, 1, 2, 6, WRITEMASK_W}}));
}

class ArrayRemapTest: public MesaTestWithMemCtx {

public:
   void run (const vector<FakeCodeline>& code,
	     const vector<FakeCodeline>& expect,
	     vector<unsigned> array_sizes,
	     vector<array_remapping>& remapping) const;


};

TEST_F(ArrayRemapTest, ApplyMerge)
{
   vector<unsigned> array_sizes{0, 12, 11, 10, 9, 8, 7};

   int8_t set_swizzle3[] = {1, -1, -1, -1};
   int8_t set_swizzle5[] = {3, -1, -1, -1};
   int8_t set_no_reswizzle[] = {0, 1, 2, 3};

   vector<array_remapping> remapping = {
      {},
      array_remapping(),
      {1, set_no_reswizzle},
      {1, set_swizzle3},
      {1, set_no_reswizzle},
      {1, set_swizzle5},
      {1, set_no_reswizzle}
   };

   const vector<FakeCodeline> code = {
      { TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_X)}, {MT(0, in0, "x")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(2, 2, WRITEMASK_XY)}, {MT(0, in0, "xy")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(3, 3, WRITEMASK_X)}, {MT(0, in0, "x")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(4, 4, WRITEMASK_XYZ)}, {MT(0, in0, "xyz")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(5, 5, WRITEMASK_X)}, {MT(0, in0, "x")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(6, 6, WRITEMASK_XYZW)}, {MT(0, in0, "xyzw")}, {}, ARR()},

      { TGSI_OPCODE_ADD, {MT(0, out0, WRITEMASK_X)}, {MT(1, 1, "x"), MT(0, in0, "y")}, {}, ARR()},
      { TGSI_OPCODE_ADD, {MT(0, out0, WRITEMASK_YZ)}, {MT(2, 2, "xy"), MT(0, in0, "yz")}, {}, ARR()},
      { TGSI_OPCODE_MUL, {MT(0, out0, WRITEMASK_W)}, {MT(3, 3, "x"), MT(0, in0, "x")}, {}, ARR()},
      { TGSI_OPCODE_ADD, {MT(0, out1, WRITEMASK_XYZ)}, {MT(4, 4, "xyz"), MT(0, in0, "xyz")}, {}, ARR()},
      { TGSI_OPCODE_MAD, {MT(0, out1, WRITEMASK_W)}, {MT(5, 5, "x"), MT(3, 1, "x"), MT(1, 1, "x")}, {}, ARR()},
      { TGSI_OPCODE_ADD, {MT(0, out2, WRITEMASK_XYZW)}, {MT(6, 6, "xyzw"), MT(0, in0, "xyzw")}, {}, ARR()},

      { TGSI_OPCODE_END}
   };

   const vector<FakeCodeline> expect = {
      { TGSI_OPCODE_MOV, {MT(1, 1, WRITEMASK_X)}, {MT(0, in0, "x")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(1, 2, WRITEMASK_XY)}, {MT(0, in0, "xy")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(1, 3, WRITEMASK_Y)}, {MT(0, in0, "xx")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(1, 4, WRITEMASK_XYZ)}, {MT(0, in0, "xyz")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(1, 5, WRITEMASK_W)}, {MT(0, in0, "xxxx")}, {}, ARR()},
      { TGSI_OPCODE_MOV, {MT(1, 6, WRITEMASK_XYZW)}, {MT(0, in0, "xyzw")}, {}, ARR()},

      { TGSI_OPCODE_ADD, {MT(0, out0, WRITEMASK_X)}, {MT(1, 1, "x"), MT(0, in0, "y")}, {}, ARR()},
      { TGSI_OPCODE_ADD, {MT(0, out0, WRITEMASK_YZ)}, {MT(1, 2, "xy"), MT(0, in0, "yz")}, {}, ARR()},
      { TGSI_OPCODE_MUL, {MT(0, out0, WRITEMASK_W)}, {MT(1, 3, "y"), MT(0, in0, "xx")}, {}, ARR()},
      { TGSI_OPCODE_ADD, {MT(0, out1, WRITEMASK_XYZ)}, {MT(1, 4, "xyz"), MT(0, in0, "xyz")}, {}, ARR()},
      { TGSI_OPCODE_MAD, {MT(0, out1, WRITEMASK_W)}, {MT(1, 5, "w"), MT(1, 1, "yyyy"), MT(1, 1, "xxxx")}, {}, ARR()},
      { TGSI_OPCODE_ADD, {MT(0, out2, WRITEMASK_XYZW)}, {MT(1, 6, "xyzw"), MT(0, in0, "xyzw")}, {}, ARR()},
      { TGSI_OPCODE_END}
   };

   run(code, expect, array_sizes, remapping);

}

void ArrayRemapTest::run (const vector<FakeCodeline>& code,
			  const vector<FakeCodeline>& expect,
			  vector<unsigned> array_sizes,
			  vector<array_remapping>& remapping) const
{
   FakeShader input(code);
   FakeShader expect_shader(expect);
   exec_list *program = input.get_program(mem_ctx);

   int n_arrays = remap_arrays(array_sizes.size() - 1, &array_sizes[0],
	 program, &remapping[0]);

   EXPECT_EQ(n_arrays, expect_shader.get_num_arrays());

   FakeShader remapped_program(program);

   ASSERT_EQ(remapped_program.length(), expect_shader.length());

   for (size_t i = 0; i < expect_shader.length(); i++) {
      EXPECT_EQ(remapped_program.line(i), expect_shader.line(i));
   }

}
