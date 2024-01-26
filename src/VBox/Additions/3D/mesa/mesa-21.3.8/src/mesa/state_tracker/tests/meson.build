# Copyright © 2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

libmesa_st_test_common = static_library(
  'mesa_st_test_common',
  ['st_tests_common.cpp', ir_expression_operation_h],
  include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux],
  dependencies : [idep_gtest, idep_mesautil],
)

test(
  'st_format_test',
  executable(
    'st_format_test',
    ['st_format.c'],
    include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux],
    link_with : [
      libmesa_st_test_common, libmesa_gallium, libglapi, libgallium,
    ],
    dependencies : idep_mesautil,
  ),
  suite : ['st_mesa'],
)

test(
  'st_renumerate_test',
  executable(
    'st_renumerate_test',
    ['test_glsl_to_tgsi_lifetime.cpp', ir_expression_operation_h],
    include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux],
    link_with : [
      libmesa_st_test_common, libmesa_gallium, libglapi, libgallium,
    ],
    dependencies : [idep_mesautil, idep_gtest],
  ),
  suite : ['st_mesa'],
)

test(
  'st-array-merge-test',
  executable(
    'st_array_merge_test',
    ['test_glsl_to_tgsi_array_merge.cpp', ir_expression_operation_h],
    include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux],
    link_with : [
      libmesa_st_test_common, libmesa_gallium, libglapi, libgallium,
    ],
    dependencies : [idep_mesautil, idep_gtest],
  ),
  suite : ['st_mesa'],
)
