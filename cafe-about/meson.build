date_exe = find_program('date')
cmd = run_command('sh', '-c', 'echo $SOURCE_DATE_EPOCH')
source_date_epoch = cmd.stdout().strip()
if source_date_epoch == ''
	source_date_epoch = run_command(date_exe, '+%s').stdout().strip()
endif
cafe_date = run_command(date_exe, '-u', '-d', '@' + source_date_epoch, '+%Y-%m-%d').stdout().strip()
cafe_year = run_command(date_exe, '-u', '-d', '@' + source_date_epoch, '+%Y').stdout().strip()

cafe_data = configuration_data()

cafe_data.set('CAFE_DATE', cafe_date)
cafe_data.set('CAFE_DATE_COMMENT_START', '<!--')
cafe_data.set('CAFE_DATE_COMMENT_END', '-->')
cafe_data.set('CAFE_PLATFORM', cafe_platform)
cafe_data.set('CAFE_MINOR', cafe_minor)
cafe_data.set('CAFE_MICRO', cafe_micro)

cafe_about_data = configuration_data()
cafe_about_data.set('RELEASE_YEAR', cafe_year)

executable('cafe-about',
  'cafe-about.c',
  c_args: [
    '-DLOCALE_DIR="@0@"'.format(join_paths(get_option('prefix'), get_option('localedir'))),
    '-DPIXMAPS_DIR="@0@"'.format(join_paths(get_option('prefix'), get_option('datadir'), 'pixmaps')),
  ],
  dependencies : ctk_dep,
  include_directories: top_inc,
  install: true,
)

i18n.merge_file(
  input: 'cafe-about.desktop.in',
  output: 'cafe-about.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

cafe_version_xml_in = configure_file(
  input: 'cafe-version.xml.in',
  output: 'cafe-version.xml',
  configuration: cafe_data,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'cafe-about'),
)

cafe_about_h_in = configure_file(
  input: 'cafe-about.h.in',
  output: 'cafe-about.h',
  configuration: cafe_about_data,
  install: false,
)
