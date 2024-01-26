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

files_i915 = files(
  'i830_context.c',
  'i830_context.h',
  'i830_reg.h',
  'i830_state.c',
  'i830_texblend.c',
  'i830_texstate.c',
  'i830_vtbl.c',
  'i915_context.c',
  'i915_context.h',
  'i915_debug_fp.c',
  'i915_debug.h',
  'i915_fragprog.c',
  'i915_program.c',
  'i915_program.h',
  'i915_reg.h',
  'i915_state.c',
  'i915_tex_layout.c',
  'i915_texstate.c',
  'i915_vtbl.c',
  'intel_batchbuffer.c',
  'intel_batchbuffer.h',
  'intel_blit.c',
  'intel_blit.h',
  'intel_buffer_objects.c',
  'intel_buffer_objects.h',
  'intel_buffers.c',
  'intel_buffers.h',
  'intel_chipset.h',
  'intel_clear.c',
  'intel_clear.h',
  'intel_context.c',
  'intel_context.h',
  'intel_extensions.c',
  'intel_extensions.h',
  'intel_fbo.c',
  'intel_fbo.h',
  'intel_mipmap_tree.c',
  'intel_mipmap_tree.h',
  'intel_pixel_bitmap.c',
  'intel_pixel.c',
  'intel_pixel_copy.c',
  'intel_pixel_draw.c',
  'intel_pixel.h',
  'intel_pixel_read.c',
  'intel_reg.h',
  'intel_regions.c',
  'intel_regions.h',
  'intel_render.c',
  'intel_screen.c',
  'intel_screen.h',
  'intel_state.c',
  'intel_syncobj.c',
  'intel_tex.c',
  'intel_tex_copy.c',
  'intel_tex.h',
  'intel_tex_image.c',
  'intel_tex_layout.c',
  'intel_tex_layout.h',
  'intel_tex_obj.h',
  'intel_tex_subimage.c',
  'intel_tex_validate.c',
  'intel_tris.c',
  'intel_tris.h',
)

libi915 = static_library(
  'i915',
  files_i915,
  include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux, inc_dri_common, inc_util],
  c_args : [no_override_init_args],
  gnu_symbol_visibility : 'hidden',
  dependencies : [dep_libdrm, dep_libdrm_intel, idep_mesautil],
)
