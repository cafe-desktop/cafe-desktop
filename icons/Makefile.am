
iconsdir = $(datadir)/icons/hicolor

png_icons = \
	16x16/apps/cafe-desktop.png \
	22x22/apps/cafe-desktop.png \
	24x24/apps/cafe-desktop.png \
	32x32/apps/cafe-desktop.png \
	48x48/apps/cafe-desktop.png \
	128x128/apps/cafe-desktop.png

nobase_dist_icons_DATA = \
	$(png_icons) \
	scalable/apps/cafe-desktop.svg \
	scalable/apps/cafe-desktop-symbolic.svg

ctk_update_icon_cache = ctk-update-icon-cache -f -t $(datadir)/icons/hicolor

EXTRA_DIST = \
	meson.build

.PHONY: build-png-icons clean-png-icons

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(ctk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(ctk_update_icon_cache)"; \
	fi

if HAVE_RSVG_CONVERT
build-png-icons:
	$(RSVG_CONVERT) -w 16 -h 16 scalable/apps/cafe-desktop.svg -o 16x16/apps/cafe-desktop.png
	$(RSVG_CONVERT) -w 22 -h 22 scalable/apps/cafe-desktop.svg -o 22x22/apps/cafe-desktop.png
	$(RSVG_CONVERT) -w 24 -h 24 scalable/apps/cafe-desktop.svg -o 24x24/apps/cafe-desktop.png
	$(RSVG_CONVERT) -w 32 -h 32 scalable/apps/cafe-desktop.svg -o 32x32/apps/cafe-desktop.png
	$(RSVG_CONVERT) -w 48 -h 48 scalable/apps/cafe-desktop.svg -o 48x48/apps/cafe-desktop.png
	$(RSVG_CONVERT) -w 128 -h 128 scalable/apps/cafe-desktop.svg -o 128x128/apps/cafe-desktop.png
endif

clean-png-icons:
	rm -f $(png_icons)
