project('xfce4-search-plugin', 'c',
  version : '0.1.0',
  meson_version : '>= 0.60.0',
  default_options : ['warning_level=2'])

cc = meson.get_compiler('c')

deps = [
  dependency('gtk+-3.0', version : '>= 3.22'),
  dependency('glib-2.0', version : '>= 2.56'),
  dependency('libxfce4panel-2.0', version : '>= 4.16'),
  dependency('libxfconf-0', required : true),
]

shared_module('search-plugin',
  'src/search-plugin-main.c',
  'src/search-plugin-ui.c',
  'src/search-plugin-apps.c',
  dependencies : deps,
  install : true,
  install_dir : join_paths(get_option('libdir'), 'xfce4', 'panel', 'plugins'),
  c_args : ['-DG_LOG_DOMAIN="xfce4-search-plugin"'])

install_data('data/search-plugin.desktop',
  install_dir : join_paths(get_option('datadir'), 'xfce4', 'panel', 'plugins'))
