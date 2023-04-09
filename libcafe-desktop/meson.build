libmdt_inc = include_directories('.')

headers = files(
  'cafe-desktop.h',
  'cafe-desktop-utils.h',
  'cafe-desktop-item.h',
  'cafe-dconf.h',
  'cafe-gsettings.h',
  'cafe-bg.h',
  'cafe-bg-crossfade.h',
  'cafe-desktop-thumbnail.h',
  'cafe-rr.h',
  'cafe-languages.h',
  'cafe-rr-config.h',
  'cafe-rr-labeler.h',
  'cafe-colorbutton.h',
  'cafe-colorsel.h',
  'cafe-colorseldialog.h',
)

install_headers(
  headers,
  subdir: 'cafe-desktop-2.0/libcafe-desktop',
)

sources = files(
  'cafe-desktop-item.c',
  'cafe-desktop-utils.c',
  'cafe-desktop-thumbnail.c',
  'cafe-dconf.c',
  'cafe-gsettings.c',
  'cafe-bg.c',
  'cafe-bg-crossfade.c',
  'display-name.c',
  'cafe-rr.c',
  'cafe-languages.c',
  'cafe-rr-config.c',
  'cafe-rr-output-info.c',
  'cafe-rr-labeler.c',
  'cafe-colorbutton.c',
  'cafe-colorsel.c',
  'cafe-colorseldialog.c',
  'edid-parse.c',
)

deps = [
  x11_dep,
  ctk_dep,
  m_dep,
  libmdt_dep,
  dconf_dep,
]

if get_option('pnp-ids-path') == 'internal'
  pnp_ids_path = join_paths(cafedt_pkgdatadir, 'pnp.ids')
  install_data('pnp.ids', install_dir: cafedt_pkgdatadir)
else
  pnp_ids_path = get_option('pnp-ids-path')
endif

cflags = [
  '-DCAFELOCALEDIR="@0@"'.format(cafedt_localedir),
  '-DG_LOG_DOMAIN="CafeDesktop"',
  '-DISO_CODES_PREFIX="@0@"'.format(iso_codes_prefix),
  '-DPNP_IDS="@0@"'.format(pnp_ids_path),
]

symbol_map = join_paths(meson.current_source_dir(), meson.project_name() + '.map')

test_ldflag = '-Wl,--version-script,' + symbol_map
ldflags = []

if cc.has_link_argument(test_ldflag)
  ldflags += test_ldflag
endif

libcafe_desktop = shared_library(
  'cafe-desktop-2',
  sources: sources,
  version: libversion,
  include_directories: top_inc,
  dependencies: deps,
  c_args: cflags,
  link_args: ldflags,
  link_depends: symbol_map,
  install: true,
  install_dir: cafedt_libdir,
)

libcafe_desktop_dep = declare_dependency(
  link_with: libcafe_desktop,
  include_directories: libmdt_inc,
  dependencies: deps,
)

pkg.generate(
  libraries: libcafe_desktop,
  version: cafedt_version,
  name: cafedt_api_name,
  description: 'Utility library for loading .desktop files',
  filebase: cafedt_api_name,
  subdirs: cafedt_api_name,
  requires: [
    'ctk+-3.0',
  ],
  requires_private: 'dconf',
  variables: 'exec_prefix=' + cafedt_libexecdir,
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

if enable_gir
  gir_sources = sources + headers

  gir_incs = [
    'GObject-2.0',
    'Ctk-3.0',
  ]

  gir_extra_args = [
    '--warn-all',
    '--add-include-path=@0@'.format(meson.current_source_dir()),
    '-DCAFE_DESKTOP_USE_UNSTABLE_API',
  ]

  gir_dir = join_paths(cafedt_datadir, 'gir-1.0')
  typelib_dir = join_paths(cafedt_libdir, 'girepository-1.0')

  gnome.generate_gir(
    libcafe_desktop,
    sources: gir_sources,
    nsversion: cafedt_api_version,
    namespace: cafedt_gir_ns,
    symbol_prefix: 'cafe_',
    identifier_prefix: 'Cafe',
    export_packages: cafedt_api_name,
    includes: gir_incs,
    extra_args: gir_extra_args,
    install: true,
    install_dir_gir: gir_dir,
    install_dir_typelib: typelib_dir,
  )
endif

test_ditem = executable('test-ditem',
  sources : 'test-ditem.c',
  dependencies : libcafe_desktop_dep,
  install : false,
)

test_color_button = executable('test-color-button',
  sources : 'test.c',
  dependencies : libcafe_desktop_dep,
  install : false,
)

test_languages = executable('test-languages',
  sources : 'test-languages.c',
  dependencies : libcafe_desktop_dep,
  install : false,
)
