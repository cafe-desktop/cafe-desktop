## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the CTK+ Makefiles in ctk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=cafe-desktop

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../ctk
DOC_SOURCE_DIR=../../../libcafe-desktop

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="CTK_DISABLE_DEPRECATED" 
SCAN_OPTIONS=--deprecated-guards="CAFE_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../cdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/ctk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/ctk/*.c
HFILE_GLOB=$(top_srcdir)/libcafe-desktop/*.h
CFILE_GLOB=$(top_srcdir)/libcafe-desktop/*.c

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=ctkdebug.h ctkintl.h
IGNORE_HFILES=

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/ctk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files=

# SGML files where gtk-doc abbreviations (#CtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. AM_CPPFLAGS=-I$(top_srcdir) -I$(top_builddir) $(CTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/ctk/$(ctktargetlib)
AM_CPPFLAGS=$(CAFE_DESKTOP_CFLAGS) -I$(top_srcdir)/libcafe-desktop -DCAFE_DESKTOP_USE_UNSTABLE_API
GTKDOC_LIBS=$(CAFE_DESKTOP_LIBS) $(top_builddir)/libcafe-desktop/libcafe-desktop-2.la

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += meson.build

-include $(top_srcdir)/git.mk
