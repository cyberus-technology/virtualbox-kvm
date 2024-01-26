# Copyright © 2017 Intel Corporation

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

files_main_test = files('enum_strings.cpp')
link_main_test = []

if with_shared_glapi
  files_main_test += files(
    'dispatch_sanity.cpp',
    'mesa_formats.cpp',
    'mesa_extensions.cpp',
    'program_state_string.cpp',
  )
  link_main_test += libglapi
else
  files_main_test += files('stubs.cpp')
endif

test(
  'main-test',
  executable(
    'main_test',
    [files_main_test, main_dispatch_h],
    include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium],
    dependencies : [idep_gtest, dep_clock, dep_dl, dep_thread, idep_mesautil],
    link_with : [libmesa_classic, link_main_test],
  ),
  suite : ['mesa'],
)
