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

inc_wayland_drm = include_directories('.')

wayland_drm_protocol_c = custom_target(
  'wayland-drm-protocol.c',
  input : 'wayland-drm.xml',
  output : 'wayland-drm-protocol.c',
  command : [prog_wl_scanner, wl_scanner_arg, '@INPUT@', '@OUTPUT@'],
)

wayland_drm_client_protocol_h = custom_target(
  'wayland-drm-client-protocol.h',
  input : 'wayland-drm.xml',
  output : 'wayland-drm-client-protocol.h',
  command : [prog_wl_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)

wayland_drm_server_protocol_h = custom_target(
  'wayland-drm-server-protocol.h',
  input : 'wayland-drm.xml',
  output : 'wayland-drm-server-protocol.h',
  command : [prog_wl_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

libwayland_drm = static_library(
  'wayland_drm',
  ['wayland-drm.c', wayland_drm_protocol_c, wayland_drm_server_protocol_h],
  gnu_symbol_visibility : 'hidden',
  dependencies : [dep_wayland_server],
  build_by_default : false,
)

# linux-dmabuf isn't part of wayland-drm, but this happens to be the only
# place which is a) guaranteed to be built when building either or both
# of EGL and Vulkan WSI, and b) guaranteed to be included before both,
# so the targets are actually defined by the time the files come to be
# used.
#
# Moving these to a src/wsi/ was suggested but shot down, so put these
# here for now as the maybe-least-bad solution.
linux_dmabuf_unstable_v1_protocol_c = custom_target(
  'linux-dmabuf-unstable-v1-protocol.c',
  input : wayland_dmabuf_xml,
  output : 'linux-dmabuf-unstable-v1-protocol.c',
  command : [prog_wl_scanner, wl_scanner_arg, '@INPUT@', '@OUTPUT@'],
)

linux_dmabuf_unstable_v1_client_protocol_h = custom_target(
  'linux-dmabuf-unstable-v1-client-protocol.h',
  input : wayland_dmabuf_xml,
  output : 'linux-dmabuf-unstable-v1-client-protocol.h',
  command : [prog_wl_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)
