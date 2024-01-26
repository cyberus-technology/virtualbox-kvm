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

# TODO: android?

files_libintel_dev = files(
  'intel_debug.c',
  'intel_debug.h',
  'intel_device_info.c',
  'intel_device_info.h',
)

libintel_dev = static_library(
  ['intel_dev'],
  [files_libintel_dev, sha1_h],
  include_directories : [inc_include, inc_src, inc_intel],
  dependencies : idep_mesautil,
  c_args : [no_override_init_args],
  gnu_symbol_visibility : 'hidden',
)

if with_tests
  test('intel_device_info_test',
       executable(
         'intel_device_info_test',
         'intel_device_info_test.c',
         include_directories : [inc_include, inc_src],
         link_with : libintel_dev,
       ),
       suite : ['intel'],
      )
endif

if with_intel_tools
  intel_dev_info = executable(
    'intel_dev_info',
    files('intel_dev_info.c'),
    dependencies : [dep_libdrm, dep_dl, dep_thread, dep_m],
    include_directories : [inc_include, inc_src, inc_intel],
    link_with : libintel_dev,
    c_args : [no_override_init_args],
    gnu_symbol_visibility : 'hidden',
    install : true
  )
endif
