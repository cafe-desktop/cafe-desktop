bin_PROGRAMS = cafe-about
cafe_about_SOURCES = cafe-about.c cafe-about.h
cafe_about_CFLAGS = $(WARN_CFLAGS) $(CAFE_ABOUT_CFLAGS)
cafe_about_LDADD = $(CAFE_ABOUT_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(CAFE_ABOUT_CFLAGS) \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	-DPIXMAPS_DIR=\"$(datadir)/pixmaps/\"

desktopdir = $(datadir)/applications
desktop_in_files = cafe-about.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
cafe-about.desktop: cafe-about.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --keyword= --keyword=Name --keyword=Comment --template $< -d $(top_srcdir)/po -o $@

versiondir = $(datadir)/cafe-about
version_DATA = cafe-version.xml

EXTRA_DIST = \
	$(desktop_in_files) \
	meson.build \
	cafe-about.h.in \
	cafe-version.xml.in

CLEANFILES = $(desktop_DATA) $(bin_PROGRAMS) $(version_DATA) cafe-about.h

#-include $(top_srcdir)/git.mk

