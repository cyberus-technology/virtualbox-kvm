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

files_r200 = files(
  'radeon_buffer_objects.c',
  'radeon_buffer_objects.h',
  'radeon_cmdbuf.h',
  'radeon_common.c',
  'radeon_common.h',
  'radeon_common_context.c',
  'radeon_common_context.h',
  'radeon_debug.c',
  'radeon_debug.h',
  'radeon_dma.c',
  'radeon_dma.h',
  'radeon_fbo.c',
  'radeon_fog.c',
  'radeon_fog.h',
  'radeon_mipmap_tree.c',
  'radeon_mipmap_tree.h',
  'radeon_pixel_read.c',
  'radeon_queryobj.c',
  'radeon_queryobj.h',
  'radeon_span.c',
  'radeon_span.h',
  'radeon_tex_copy.c',
  'radeon_texture.c',
  'radeon_texture.h',
  'radeon_tile.c',
  'radeon_tile.h',
  'r200_blit.c',
  'r200_blit.h',
  'r200_cmdbuf.c',
  'r200_context.c',
  'r200_context.h',
  'r200_fragshader.c',
  'r200_ioctl.c',
  'r200_ioctl.h',
  'r200_maos.c',
  'r200_maos.h',
  'r200_reg.h',
  'r200_sanity.c',
  'r200_sanity.h',
  'r200_state.c',
  'r200_state.h',
  'r200_state_init.c',
  'r200_swtcl.c',
  'r200_swtcl.h',
  'r200_tcl.c',
  'r200_tcl.h',
  'r200_tex.c',
  'r200_tex.h',
  'r200_texstate.c',
  'r200_vertprog.c',
  'r200_vertprog.h',
  'radeon_chipset.h',
  'radeon_screen.c',
  'radeon_screen.h',
  'server/radeon_reg.h',
)

libr200 = static_library(
  'r200',
  files_r200,
  include_directories : [
    inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux, inc_dri_common, inc_util, include_directories('server'),
  ],
  c_args : ['-DRADEON_R200'],
  gnu_symbol_visibility : 'hidden',
  dependencies : [dep_libdrm, dep_libdrm_radeon, idep_mesautil],
)
