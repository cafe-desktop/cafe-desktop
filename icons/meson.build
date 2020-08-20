icons = [
  '16x16/apps/cafe-desktop.png',
  '22x22/apps/cafe-desktop.png',
  '24x24/apps/cafe-desktop.png',
  '32x32/apps/cafe-desktop.png',
  '48x48/apps/cafe-desktop.png',
  '128x128/apps/cafe-desktop.png',
  'scalable/apps/cafe-desktop.svg',
  'scalable/apps/cafe-desktop-symbolic.svg',
]

foreach i : icons
  dir = i.split('/')
  install_data(i,
    install_dir: join_paths('share/icons/hicolor', dir[0], dir[1]),
  )
endforeach
