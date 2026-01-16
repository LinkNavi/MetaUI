project('metaui', ['c', 'cpp'],
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'c_std=c11', 'warning_level=2']
)

# Dependencies
wayland_client = dependency('wayland-client')
wayland_egl = dependency('wayland-egl')
wayland_protocols = dependency('wayland-protocols')
egl = dependency('egl')
gl = dependency('gl')

# Get wayland-protocols directory
wayland_protocols_dir = wayland_protocols.get_variable(pkgconfig: 'pkgdatadir')

# Generate wayland protocols
wayland_scanner = find_program('wayland-scanner')

# XDG Shell protocol (required by wlr-layer-shell)
xdg_shell_xml = wayland_protocols_dir / 'stable/xdg-shell/xdg-shell.xml'

xdg_shell_client_header = custom_target(
  'xdg-shell-client-protocol.h',
  input: xdg_shell_xml,
  output: 'xdg-shell-client-protocol.h',
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: get_option('includedir') / 'metaui'
)

xdg_shell_private_code = custom_target(
  'xdg-shell-private-code.c',
  input: xdg_shell_xml,
  output: 'xdg-shell-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
)

# Download wlr-layer-shell protocol if not present
fs = import('fs')
if not fs.exists('protocols/wlr-layer-shell-unstable-v1.xml')
  run_command('mkdir', '-p', 'protocols', check: false)
  run_command('wget', 
    'https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/raw/master/unstable/wlr-layer-shell-unstable-v1.xml',
    '-O', 'protocols/wlr-layer-shell-unstable-v1.xml',
    check: false)
endif

wlr_layer_shell_xml = files('protocols/wlr-layer-shell-unstable-v1.xml')

wlr_layer_shell_client_header = custom_target(
  'wlr-layer-shell-client-protocol.h',
  input: wlr_layer_shell_xml,
  output: 'wlr-layer-shell-unstable-v1-client-protocol.h',
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: get_option('includedir') / 'metaui'
)

wlr_layer_shell_private_code = custom_target(
  'wlr-layer-shell-private-code.c',
  input: wlr_layer_shell_xml,
  output: 'wlr-layer-shell-unstable-v1-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
)

# Include directories
inc = include_directories('include')
build_inc = include_directories('.')

# Library sources (protocol implementations)
metaui_sources = [
  xdg_shell_private_code,
  wlr_layer_shell_private_code
]

# Create a static library for the protocols
metaui_protocol_lib = static_library(
  'metaui-protocol',
  metaui_sources,
  dependencies: [wayland_client],
  include_directories: [inc, build_inc]
)

metaui_protocol_dep = declare_dependency(
  link_with: metaui_protocol_lib,
  include_directories: [inc, build_inc],
  sources: [xdg_shell_client_header, wlr_layer_shell_client_header]
)

# Build examples if they exist
if fs.exists('examples/test.cpp')
  test_exe = executable(
    'metaui-test',
    'examples/test.cpp',
    dependencies: [
      wayland_client,
      wayland_egl,
      egl,
      gl,
      metaui_protocol_dep
    ],
    include_directories: [inc, build_inc],
    install: true
  )
endif

# Build main.cpp if it exists in root
if fs.exists('main.cpp')
  main_exe = executable(
    'metaui-app',
    'main.cpp',
    dependencies: [
      wayland_client,
      wayland_egl,
      egl,
      gl,
      metaui_protocol_dep
    ],
    include_directories: [inc, build_inc],
    install: false
  )
endif

# Install headers
install_subdir('include/metaui', install_dir: get_option('includedir'))
install_headers('include/metaui.hpp')

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  name: 'metaui',
  description: 'Modern C++ GUI Framework for Wayland',
  version: meson.project_version(),
  subdirs: 'metaui',
  requires: ['wayland-client', 'wayland-egl', 'egl', 'gl', 'wayland-protocols']
)
