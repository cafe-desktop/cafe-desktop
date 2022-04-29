project(
  'cafe-desktop',
  'c',
  version: '1.24.1',
  license: [ 'GPL-2', 'FDL-1.1', 'LGPL-2' ],
)

cafedt_version = meson.project_version()
version_array = cafedt_version.split('.')
cafe_major_version = version_array[0].to_int()
cafe_minor_version = version_array[0].to_int()
cafe_micro_version = version_array[0].to_int()
cafe_platform = 1
cafe_minor = 24
cafe_micro = 0

cafedt_api_version = '2.0'
cafedt_api_name = '@0@-@1@'.format(meson.project_name(), cafedt_api_version)

cafedt_gir_ns = 'CafeDesktop'
cafedt_gir_version = '2.0'

cafedt_gettext_package = meson.project_name()

cafedt_prefix = get_option('prefix')
cafedt_bindir = join_paths(cafedt_prefix, get_option('bindir'))
cafedt_datadir = join_paths(cafedt_prefix, get_option('datadir'))
cafedt_includedir = join_paths(cafedt_prefix, get_option('includedir'))
cafedt_libdir = join_paths(cafedt_prefix, get_option('libdir'))
cafedt_libexecdir = join_paths(cafedt_prefix, get_option('libexecdir'))
cafedt_localedir = join_paths(cafedt_prefix, get_option('localedir'))
cafedt_mandir = join_paths(cafedt_prefix, get_option('mandir'))

cafedt_pkgdatadir = join_paths(cafedt_datadir, 'libcafe-desktop')

# options
enable_gir = get_option('introspection')
enable_ctk_doc = get_option('gtk-doc')
enable_startup_notification = get_option('startup-notification')

# Before making a release, the LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
current = 17
revision = 4
age = 1

libversion = '@0@.@1@.@2@'.format(current, age, revision)

cc = meson.get_compiler('c')
nm = find_program('nm')

config_h = configuration_data()

# i18n
config_h.set_quoted('GETTEXT_PACKAGE', cafedt_gettext_package)
config_h.set_quoted('LOCALEDIR', cafedt_localedir)

common_flags = [
  '-DHAVE_CONFIG_H',
  '-I' + meson.build_root(),
  '-DPACKAGE_VERSION="@0@"'.format(cafedt_version),
]

add_project_arguments(common_flags, language: 'c')

ctk_dep = dependency('ctk+-3.0', version: '>= 3.22.0')
m_dep = cc.find_library('m', required: false)
dconf_dep = dependency('dconf', version: '>= 0.13.4')
x11_dep = dependency('x11', required: true)
randr_dep = dependency('xrandr', version: '>= 1.3', required: false)
config_h.set('HAVE_RANDR', randr_dep.found())
iso_codes = dependency('iso-codes')
iso_codes_prefix = iso_codes.get_pkgconfig_variable('prefix')
libmdt_dep = [
  dependency('gdk-pixbuf-2.0'),
  dependency('gobject-2.0'),
  dependency('glib-2.0', version: '>= 2.50.0'),
  dependency('gio-2.0', version: '>= 2.26.0'),
  dependency('libstartup-notification-1.0', version: '>= 0.5',
    required: enable_startup_notification ),
  randr_dep,
  iso_codes,
]

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

po_dir = join_paths(meson.source_root(), 'po')

top_inc = include_directories('.')

subdir('icons')
subdir('libcafe-desktop')
subdir('man')
if enable_ctk_doc
  subdir('docs/reference/cafe-desktop')
endif
subdir('po')
subdir('tools')
subdir('schemas')
if get_option('cafe-about')
  subdir('cafe-about')
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
)

meson.add_install_script('utils/post_install.py')

test('libcafe-desktop symbols check',
  find_program('abi-check'),
  env: [
    'LIB=@0@'.format(libcafe_desktop.full_path()),
    'NM=@0@'.format(nm.path()),
    'ABI=@0@/libcafe-desktop/cafe-desktop.abi'.format(meson.source_root())
  ]
)
