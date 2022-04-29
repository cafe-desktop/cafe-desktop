libcafe_desktopdir = $(includedir)/cafe-desktop-2.0/libcafe-desktop
libcafe_desktop_HEADERS = \
	cafe-desktop.h \
	cafe-desktop-utils.h \
	cafe-desktop-item.h \
	cafe-dconf.h \
	cafe-gsettings.h \
	cafe-bg.h			\
	cafe-bg-crossfade.h		\
	cafe-desktop-thumbnail.h	\
	cafe-rr.h			\
	cafe-languages.h		\
	cafe-rr-config.h		\
	cafe-rr-labeler.h \
	cafe-colorbutton.h \
	cafe-colorsel.h \
	cafe-hsv.h \
	cafe-colorseldialog.h

lib_LTLIBRARIES = libcafe-desktop-2.la

AM_CPPFLAGS =							\
	$(CAFE_DESKTOP_CFLAGS)					\
	$(XLIB_CFLAGS)						\
	$(DCONF_CFLAGS)						\
	-DG_LOG_DOMAIN=\"CafeDesktop\"				\
	-DCAFELOCALEDIR=\""$(localedir)\""			\
	-DPNP_IDS=\""$(PNP_IDS)"\"				\
	-DISO_CODES_PREFIX=\""$(ISO_CODES_PREFIX)"\"		\
	$(DISABLE_DEPRECATED_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

noinst_PROGRAMS = test-desktop-thumbnail test-ditem test test-languages

CLEANFILES =

introspection_sources = \
	cafe-desktop-utils.c	\
	cafe-desktop-thumbnail.c \
	cafe-dconf.c \
	cafe-gsettings.c \
	cafe-bg.c		\
	cafe-bg-crossfade.c	\
	display-name.c		\
	cafe-rr.c		\
	cafe-languages.c	\
	cafe-rr-config.c	\
	cafe-rr-output-info.c	\
	cafe-rr-labeler.c	\
	cafe-colorbutton.c \
	cafe-colorsel.c \
	cafe-hsv.c \
	cafe-colorseldialog.c \
	edid-parse.c

libcafe_desktop_2_la_SOURCES =		\
	$(introspection_sources)	\
	cafe-desktop-item.c		\
	cafe-rr-private.h		\
	edid.h				\
	private.h

libcafe_desktop_2_la_LIBADD =	\
	$(XLIB_LIBS)		\
	$(CAFE_DESKTOP_LIBS)	\
	$(DCONF_LIBS)

libcafe_desktop_2_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-export-symbols-regex "^cafe_.*" \
	-no-undefined

test_SOURCES = \
	test.c

test_LDADD = \
	libcafe-desktop-2.la		\
	$(CAFE_DESKTOP_LIBS)

test_desktop_thumbnail_LDADD = \
	$(lib_LTLIBRARIES) \
	$(CAFE_DESKTOP_LIBS) \
	$(XLIB_LIBS)


test_ditem_SOURCES = \
	test-ditem.c

test_ditem_LDADD = \
	libcafe-desktop-2.la		\
	$(XLIB_LIBS)			\
	$(CAFE_DESKTOP_LIBS)

test_languages_SOURCES = test-languages.c

test_languages_LDADD = \
	libcafe-desktop-2.la		\
	$(CAFE_DESKTOP_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cafe-desktop-2.0.pc

if USE_INTERNAL_PNP_IDS
pnpdatadir = $(datadir)/libcafe-desktop
pnpdata_DATA = pnp.ids
endif
pnpdata_DATA_dist = pnp.ids

check:
	test -s $(top_srcdir)/libcafe-desktop/pnp.ids

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

CafeDesktop-2.0.gir: libcafe-desktop-2.la
CafeDesktop_2_0_gir_INCLUDES = GObject-2.0 Ctk-3.0
CafeDesktop_2_0_gir_PACKAGES = cdk-pixbuf-2.0 glib-2.0 gobject-2.0 gio-2.0 ctk+-3.0
CafeDesktop_2_0_gir_CFLAGS = -DCAFE_DESKTOP_USE_UNSTABLE_API -I$(top_srcdir)
CafeDesktop_2_0_gir_LIBS = libcafe-desktop-2.la
CafeDesktop_2_0_gir_FILES = $(introspection_sources) $(libcafe_desktop_HEADERS)
CafeDesktop_2_0_gir_SCANNERFLAGS = --identifier-prefix=Cafe --symbol-prefix=cafe_
INTROSPECTION_SCANNER_ARGS += $(patsubst %,--c-include='libcafe-desktop/%',$(libcafe_desktop_HEADERS))
INTROSPECTION_GIRS += CafeDesktop-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

EXTRA_DIST = \
	meson.build \
	cafe-desktop.map \
	cafe-desktop-2.0.pc.in \
	cafe-desktop-2.0-uninstalled.pc.in \
	$(pnpdata_DATA_dist)

MAINTAINERCLEANFILES = \
	pnp.ids

-include $(top_srcdir)/git.mk
