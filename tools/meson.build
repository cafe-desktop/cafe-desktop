executable('cafe-color-select',
  'cafe-color-select.c',
  c_args: [
    '-DLOCALE_DIR="@0@"'.format(join_paths(get_option('prefix'), get_option('localedir'))),
  ],
  dependencies: [ gtk_dep, libcafe_desktop_dep, ],
  include_directories: top_inc,
  install: true,
)

i18n.merge_file(
  input: 'cafe-color-select.desktop.in',
  output: 'cafe-color-select.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)
