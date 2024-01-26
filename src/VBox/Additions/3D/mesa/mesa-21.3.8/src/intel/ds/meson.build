# Copyright © 2020-2021 Collabora, Ltd.
# Author: Antonio Caggiano <antonio.caggiano@collabora.com>
#
# SPDX-License-Identifier: MIT

pps_intel_sources = [
  'intel_pps_perf.cc',
  'intel_pps_driver.cc',
]


pps_intel_lib = static_library(
  'pps-intel',
  sources: pps_intel_sources,
  include_directories: [inc_tool, inc_src, inc_include],
  link_with: [libintel_perf, libintel_dev],
  dependencies: [dep_perfetto, dep_libdrm, idep_mesautil],
  cpp_args: '-std=c++17'
)

compile_args_pps_intel = ['-DPPS_INTEL']

pps_intel_dep = declare_dependency(
  link_with: pps_intel_lib,
  include_directories: [inc_tool, inc_include],
  compile_args: compile_args_pps_intel,
)

pps_datasources += pps_intel_dep
if not with_datasources.contains('intel')
  with_datasources += 'intel'
endif
