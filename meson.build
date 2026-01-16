project('metaui', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'warning_level=2']
)

# Dependencies
wayland_client = dependency('wayland-client')
wayland_egl = dependency('wayland-egl')
egl = dependency('egl')
gl = dependency('gl')

# Generate wayland protocol
wayland_scanner = find_program('wayland-scanner')

# Download wlr-layer-shell protocol if not present
if not run_command('test', '-f', 'protocols/wlr-layer-shell-unstable-v1.xml', check: false).returncode() == 0
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
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@']
)

wlr_layer_shell_private_code = custom_target(
  'wlr-layer-shell-private-code.c',
  input: wlr_layer_shell_xml,
  output: 'wlr-layer-shell-unstable-v1-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@']
)

# Include directories
inc = include_directories('include')

# Library sources (header-only, but we need the protocol)
metaui_sources = [
  wlr_layer_shell_private_code
]

# Create a static library for the protocol
metaui_protocol_lib = static_library(
  'metaui-protocol',
  metaui_sources,
  dependencies: [wayland_client],
  include_directories: inc
)

metaui_protocol_dep = declare_dependency(
  link_with: metaui_protocol_lib,
  include_directories: inc,
  sources: [wlr_layer_shell_client_header]
)

# Test executable
test_exe = executable(
  'metaui-test',
  'test.cpp',
  dependencies: [
    wayland_client,
    wayland_egl,
    egl,
    gl,
    metaui_protocol_dep
  ],
  include_directories: inc,
  install: true
)

# Install headers
install_subdir('include/metaui', install_dir: 'include')
install_headers('include/metaui.hpp')

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  name: 'metaui',
  description: 'Modern C++ GUI Framework for Wayland',
  version: meson.project_version(),
  subdirs: 'metaui',
  requires: ['wayland-client', 'wayland-egl', 'egl', 'gl']
)
