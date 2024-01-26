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

files_nouveau_vieux = files(
  'nouveau_array.c',
  'nouveau_array.h',
  'nouveau_bufferobj.c',
  'nouveau_bufferobj.h',
  'nouveau_context.c',
  'nouveau_context.h',
  'nouveau_driver.c',
  'nouveau_driver.h',
  'nouveau_fbo.c',
  'nouveau_fbo.h',
  'nouveau_gldefs.h',
  'nouveau_local.h',
  'nouveau_render.h',
  'nouveau_scratch.c',
  'nouveau_scratch.h',
  'nouveau_screen.c',
  'nouveau_screen.h',
  'nouveau_span.c',
  'nouveau_state.c',
  'nouveau_state.h',
  'nouveau_surface.c',
  'nouveau_surface.h',
  'nouveau_texture.c',
  'nouveau_texture.h',
  'nouveau_util.h',
  'nv01_2d.xml.h',
  'nv04_3d.xml.h',
  'nv04_context.c',
  'nv04_context.h',
  'nv04_driver.h',
  'nv04_render.c',
  'nv04_state_fb.c',
  'nv04_state_frag.c',
  'nv04_state_raster.c',
  'nv04_state_tex.c',
  'nv04_surface.c',
  'nv10_3d.xml.h',
  'nv10_context.c',
  'nv10_driver.h',
  'nv10_render.c',
  'nv10_state_fb.c',
  'nv10_state_frag.c',
  'nv10_state_polygon.c',
  'nv10_state_raster.c',
  'nv10_state_tex.c',
  'nv10_state_tnl.c',
  'nv20_3d.xml.h',
  'nv20_context.c',
  'nv20_driver.h',
  'nv20_render.c',
  'nv20_state_fb.c',
  'nv20_state_frag.c',
  'nv20_state_polygon.c',
  'nv20_state_raster.c',
  'nv20_state_tex.c',
  'nv20_state_tnl.c',
  'nv_m2mf.xml.h',
  'nv_object.xml.h',
)

libnouveau_vieux = static_library(
  'nouveau_vieux',
  files_nouveau_vieux,
  include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux, inc_dri_common, inc_util],
  gnu_symbol_visibility : 'hidden',
  dependencies : [dep_libdrm, dep_libdrm_nouveau, idep_mesautil],
)
