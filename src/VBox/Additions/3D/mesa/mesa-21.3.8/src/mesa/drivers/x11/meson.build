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

gl_link_with = []
if with_shared_glapi
  gl_link_with += libglapi
endif

libgl = shared_library(
  'GL',
  files(
    'fakeglx.c', 'glxapi.c', 'xfonts.c', 'xm_api.c', 'xm_buffer.c', 'xm_dd.c',
    'xm_line.c', 'xm_tri.c',
  ),
  include_directories : [
    inc_include, inc_src, inc_mesa, inc_mapi, inc_gallium, inc_gallium_aux
  ],
  link_with : [libmesa_classic, libglapi_static, gl_link_with],
  dependencies : [idep_mesautil, dep_x11, dep_xext, dep_xcb, dep_thread],
  version : '1.6.0',
  install : true,
)
