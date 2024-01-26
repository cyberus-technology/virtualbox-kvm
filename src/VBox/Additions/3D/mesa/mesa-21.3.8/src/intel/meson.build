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

c_sse2_args = ['-msse2', '-mstackrealign']
inc_intel = include_directories('.')

subdir('genxml')
subdir('blorp')
subdir('dev')
subdir('isl')
subdir('common')
subdir('compiler')
subdir('perf')
if with_intel_tools
  subdir('tools')
endif
if get_option('vulkan-layers').contains('intel-nullhw')
  subdir('nullhw-layer')
endif
if with_intel_vk
  subdir('vulkan')
endif
if with_perfetto and (with_datasources.contains('intel') or with_datasources.contains('auto'))
  subdir('ds')
endif
