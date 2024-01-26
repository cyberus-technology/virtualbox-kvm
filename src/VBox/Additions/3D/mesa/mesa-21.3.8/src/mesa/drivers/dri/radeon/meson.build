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

files_r100 = files(
  'radeon_buffer_objects.c',
  'radeon_buffer_objects.h',
  'radeon_cmdbuf.h',
  'radeon_common.c',
  'radeon_common_context.c',
  'radeon_common_context.h',
  'radeon_common.h',
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
  'radeon_blit.c',
  'radeon_blit.h',
  'radeon_context.c',
  'radeon_context.h',
  'radeon_chipset.h',
  'radeon_ioctl.c',
  'radeon_ioctl.h',
  'radeon_maos.c',
  'radeon_maos.h',
  'radeon_maos_vbtmp.h',
  'radeon_sanity.c',
  'radeon_sanity.h',
  'radeon_screen.c',
  'radeon_screen.h',
  'radeon_state.c',
  'radeon_state.h',
  'radeon_state_init.c',
  'radeon_swtcl.c',
  'radeon_swtcl.h',
  'radeon_tcl.c',
  'radeon_tcl.h',
  'radeon_tex.c',
  'radeon_tex.h',
  'radeon_texstate.c',
  'server/radeon_reg.h',
)

libr100 = static_library(
  'r100',
  files_r100,
  include_directories : [
    inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux, inc_dri_common, inc_util, include_directories('server'),
  ],
  c_args : ['-DRADEON_R100'],
  gnu_symbol_visibility : 'hidden',
  dependencies : [dep_libdrm, dep_libdrm_radeon, idep_mesautil],
)
