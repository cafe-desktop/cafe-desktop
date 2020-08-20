schemas_data = configuration_data()
schemas_data.set('GETTEXT_PACKAGE', meson.project_name())
schemas_data.set('datadir', cafedt_datadir)

schemas_in = [
  'org.cafe.background.gschema.xml',
]

schemas = []
foreach s: schemas_in
  schemas += configure_file(
    input: '@0@.in'.format(s),
    output: '@0@'.format(s),
    configuration: schemas_data,
  )
endforeach

schemas += [
  'org.cafe.accessibility-keyboard.gschema.xml',
  'org.cafe.accessibility-startup.gschema.xml',
  'org.cafe.applications-at-mobility.gschema.xml',
  'org.cafe.applications-at-visual.gschema.xml',
  'org.cafe.applications-browser.gschema.xml',
  'org.cafe.applications-calculator.gschema.xml',
  'org.cafe.applications-messenger.gschema.xml',
  'org.cafe.applications-office.gschema.xml',
  'org.cafe.applications-terminal.gschema.xml',
  'org.cafe.debug.gschema.xml',
  'org.cafe.file-views.gschema.xml',
  'org.cafe.interface.gschema.xml',
  'org.cafe.lockdown.gschema.xml',
  'org.cafe.sound.gschema.xml',
  'org.cafe.thumbnail-cache.gschema.xml',
  'org.cafe.thumbnailers.gschema.xml',
  'org.cafe.typing-break.gschema.xml',
]

install_data(schemas,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas'),
)
