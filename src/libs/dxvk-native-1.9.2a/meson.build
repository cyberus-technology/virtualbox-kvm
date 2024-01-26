project('dxvk', ['c', 'cpp'], version : 'v1.9.2', meson_version : '>= 0.46')

cpu_family = target_machine.cpu_family()

add_project_arguments('-DNOMINMAX', language : 'cpp')

dxvk_compiler = meson.get_compiler('cpp')
dxvk_is_msvc = dxvk_compiler.get_id() == 'msvc'

# c++17 was added in 15.3, older version needs c++latest
if dxvk_is_msvc and dxvk_compiler.version().version_compare('<15.3')
  dxvk_cpp_std='c++latest'
else
  dxvk_cpp_std='c++17'
endif

dxvk_native = (not meson.is_cross_build() and not dxvk_is_msvc) or get_option('dxvk_native_force')

if dxvk_is_msvc
  add_project_arguments('/std:' + dxvk_cpp_std, language : 'cpp')
endif

if dxvk_compiler.get_id() == 'clang'
  if dxvk_compiler.has_argument('-Wno-unused-private-field')
    add_project_arguments('-Wno-unused-private-field', language: 'cpp')
  endif
  if dxvk_compiler.has_argument('-Wno-microsoft-exception-spec')
    add_project_arguments('-Wno-microsoft-exception-spec', language: 'cpp')
  endif
endif

if not dxvk_is_msvc
  add_project_arguments('-D_WIN32_WINNT=0xa00', language : 'cpp')

  if get_option('build_id') and dxvk_compiler.has_link_argument('-Wl,--build-id')
    add_global_link_arguments('-Wl,--build-id', language: 'cpp')
  endif

  # We need to set the section alignment for debug symbols to
  # work properly as well as avoiding a memcpy from the Wine loader.
  if dxvk_compiler.has_link_argument('-Wl,--file-alignment=4096')
    add_global_link_arguments('-Wl,--file-alignment=4096', language: 'cpp')
  endif

  # Wine's built-in back traces only work with dwarf2 symbols
  if get_option('debug') and target_machine.system() == 'windows'
    if dxvk_compiler.has_argument('-gstrict-dwarf') and dxvk_compiler.has_argument('-gdwarf-2')
      add_project_arguments('-gstrict-dwarf', '-gdwarf-2', language: ['c', 'cpp'])
    endif
  endif
endif

dxvk_include_path = include_directories('./include')

if (cpu_family == 'x86_64')
  dxvk_library_path = meson.source_root() + '/lib'
else
  dxvk_library_path = meson.source_root() + '/lib32'
endif

dxvk_extradep = [ ]

dxvk_platform = 'windows'
dxvk_wsi      = 'win32'
so_prefix     = 'dxvk_'

if not dxvk_native
  if dxvk_compiler.get_id() == 'msvc'
    wrc = find_program('rc')
  else
    add_global_link_arguments('-static', '-static-libgcc', language: 'c')
    add_global_link_arguments('-static', '-static-libgcc', '-static-libstdc++', language: 'cpp')

    if cpu_family == 'x86_64'
      wrc = find_program('x86_64-w64-mingw32-windres')
    elif cpu_family == 'x86'
      wrc = find_program('i686-w64-mingw32-windres')
    else
      error('Unsupported target architecture')
    endif
  endif

  if cpu_family == 'x86'
    if dxvk_compiler.has_link_argument('-Wl,--add-stdcall-alias')
      add_global_link_arguments('-Wl,--add-stdcall-alias', language: 'cpp')
    endif
    if dxvk_compiler.has_link_argument('-Wl,--enable-stdcall-fixup')
      add_global_link_arguments('-Wl,--enable-stdcall-fixup', language: 'cpp')
    endif
    if dxvk_compiler.has_argument('-msse') and dxvk_compiler.has_argument('-msse2')
      add_project_arguments('-msse', '-msse2', language: ['c', 'cpp'])
    endif
    if dxvk_compiler.has_argument('-mfpmath=sse')
      add_project_arguments('-mfpmath=sse', language: ['c', 'cpp'])
    endif
  endif

  lib_vulkan  = dxvk_compiler.find_library('vulkan-1', dirs : dxvk_library_path)
  lib_d3d9    = dxvk_compiler.find_library('d3d9')
  lib_d3d11   = dxvk_compiler.find_library('d3d11')
  lib_dxgi    = dxvk_compiler.find_library('dxgi')
  lib_d3dcompiler_43 = dxvk_compiler.find_library('d3dcompiler_43', dirs : dxvk_library_path)

  if dxvk_compiler.get_id() == 'msvc'
    lib_d3dcompiler_47 = dxvk_compiler.find_library('d3dcompiler')
  else
    lib_d3dcompiler_47 = dxvk_compiler.find_library('d3dcompiler_47')
  endif

  exe_ext = ''
  dll_ext = ''

  if dxvk_is_msvc
    res_ext = '.res'
  else
    res_ext = '.o'
  endif

  def_spec_ext = '.def'
else
  add_project_arguments('-DDXVK_NATIVE', language : 'cpp')

  dxvk_wsi      = get_option('dxvk_native_wsi')
  dxvk_platform = target_machine.system()

  if cpu_family == 'x86'
    if dxvk_compiler.has_argument('-msse') and dxvk_compiler.has_argument('-msse2')
      add_project_arguments('-msse', '-msse2', language: 'c')
      add_project_arguments('-msse', '-msse2', language: 'cpp')
    endif
    if dxvk_compiler.has_argument('-mfpmath=sse')
      add_project_arguments('-mfpmath=sse', language: ['c', 'cpp'])
    endif
  endif

  if dxvk_platform == 'windows'
    dxvk_include_path = include_directories('./include', './include/native/')
    lib_vulkan  = dxvk_compiler.find_library('vulkan-1', dirs : dxvk_library_path)
    lib_sdl2    = dxvk_compiler.find_library('SDL2',     dirs : dxvk_library_path)
    wrc = find_program('rc')
  else
    dxvk_include_path = include_directories('./include', './include/native/', './include/native/windows', './include/native/directx')
    lib_vulkan  = dxvk_compiler.find_library('vulkan')
    lib_sdl2    = dxvk_compiler.find_library('SDL2')
    wrc = find_program('echo')
    so_prefix = 'libdxvk_'
  endif

  res_ext = ''
  exe_ext = ''
  dll_ext = ''
  def_spec_ext = '.def'
endif

add_project_arguments('-DDXVK_WSI_'      + dxvk_wsi.to_upper(),      language : 'cpp')
add_project_arguments('-DDXVK_PLATFORM_' + dxvk_platform.to_upper(), language : 'cpp')

glsl_compiler = find_program('glslangValidator')
glsl_args = [ '-V', '--vn', '@BASENAME@', '@INPUT@', '-o', '@OUTPUT@' ]
if run_command(glsl_compiler, [ '--quiet', '--version' ]).returncode() == 0
    glsl_args += [ '--quiet' ]
endif
glsl_generator = generator(glsl_compiler,
  output    : [ '@BASENAME@.h' ],
  arguments : glsl_args)

if dxvk_is_msvc
  wrc_generator = generator(wrc,
  output    : [ '@BASENAME@' + res_ext ],
  arguments : [ '/fo', '@OUTPUT@', '@INPUT@' ])
elif dxvk_native
  wrc_generator = generator(wrc,
  output    : [ '@BASENAME@_ignored.h' ],
  arguments : [ 'Ignoring: ', '@INPUT@' ])
else
  wrc_generator = generator(wrc,
  output    : [ '@BASENAME@' + res_ext ],
  arguments : [ '-i', '@INPUT@', '-o', '@OUTPUT@' ])
endif

dxvk_version = vcs_tag(
  command: ['git', 'describe', '--dirty=+'],
  input:  'version.h.in',
  output: 'version.h')

subdir('src')

enable_tests = get_option('enable_tests')

if enable_tests
  subdir('tests')
endif
