bin_PROGRAMS = cafe-color-select
bin_SCRIPTS =

AM_CPPFLAGS = \
	-I$(top_srcdir)

cafe_color_select_SOURCES = \
	cafe-color-select.c

cafe_color_select_CFLAGS = \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	$(WARN_CFLAGS) \
	$(CAFE_DESKTOP_CFLAGS)

cafe_color_select_LDADD = \
	$(top_builddir)/libcafe-desktop/libcafe-desktop-2.la \
	$(CAFE_DESKTOP_LIBS)

desktopdir = $(datadir)/applications
desktop_in_files = cafe-color-select.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
cafe-color-select.desktop: cafe-color-select.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	$(desktop_in_files) \
	meson.build

CLEANFILES = $(desktop_DATA) $(bin_PROGRAMS)
