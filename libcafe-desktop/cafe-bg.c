/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

cafebg.c: Object for the desktop background.

Copyright (C) 2000 Eazel, Inc.
Copyright (C) 2007-2008 Red Hat, Inc.
Copyright (C) 2012 Jasmine Hassan <jasmine.aura@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this program; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
Boston, MA 02110-1301, USA.

Derived from eel-background.c and eel-gdk-pixbuf-extensions.c by
Darin Adler <darin@eazel.com> and Ramiro Estrugo <ramiro@eazel.com>

Authors: Soren Sandmann <sandmann@redhat.com>
	 Jasmine Hassan <jasmine.aura@gmail.com>

*/

#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

#include <cdk/cdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <cairo.h>

#define CAFE_DESKTOP_USE_UNSTABLE_API
#include <cafe-bg.h>
#include <cafe-bg-crossfade.h>

# include <cairo-xlib.h>

#define CAFE_BG_CACHE_DIR "cafe/background"

/* We keep the large pixbufs around if the next update
   in the slideshow is less than 60 seconds away */
#define KEEP_EXPENSIVE_CACHE_SECS 60

typedef struct _SlideShow SlideShow;
typedef struct _Slide Slide;

struct _Slide {
	double duration; /* in seconds */
	gboolean fixed;

	GSList* file1;
	GSList* file2; /* NULL if fixed is TRUE */
};

typedef struct _FileSize FileSize;
struct _FileSize {
	gint width;
	gint height;

	char* file;
};

/* This is the size of the CdkRGB dither matrix, in order to avoid
 * bad dithering when tiling the gradient
 */
#define GRADIENT_PIXMAP_TILE_SIZE 128
#define THUMBNAIL_SIZE 256

typedef struct FileCacheEntry FileCacheEntry;
#define CACHE_SIZE 4

/*
 *   Implementation of the CafeBG class
 */
struct _CafeBG {
	GObject		 parent_instance;
	char		*filename;
	CafeBGPlacement	 placement;
	CafeBGColorType	 color_type;
	CdkRGBA	 	 primary;
	CdkRGBA	 	 secondary;
	gboolean	 is_enabled;

	GFileMonitor* file_monitor;

	guint changed_id;
	guint transitioned_id;
	guint blow_caches_id;

	/* Cached information, only access through cache accessor functions */
	SlideShow* slideshow;
	time_t file_mtime;
	GdkPixbuf* pixbuf_cache;
	int timeout_id;

	GList* file_cache;
};

struct _CafeBGClass {
	GObjectClass parent_class;
};

enum {
	CHANGED,
	TRANSITIONED,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = {0};

G_DEFINE_TYPE(CafeBG, cafe_bg, G_TYPE_OBJECT)

static cairo_surface_t *make_root_pixmap     (CdkWindow  *window,
                                              gint        width,
                                              gint        height);

/* Pixbuf utils */
static void       pixbuf_average_value (GdkPixbuf  *pixbuf,
                                        CdkRGBA    *result);
static GdkPixbuf *pixbuf_scale_to_fit  (GdkPixbuf  *src,
					int         max_width,
					int         max_height);
static GdkPixbuf *pixbuf_scale_to_min  (GdkPixbuf  *src,
					int         min_width,
					int         min_height);

static void       pixbuf_draw_gradient (GdkPixbuf    *pixbuf,
					gboolean      horizontal,
					CdkRGBA     *c1,
					CdkRGBA     *c2,
					CdkRectangle *rect);

static void       pixbuf_tile          (GdkPixbuf  *src,
					GdkPixbuf  *dest);
static void       pixbuf_blend         (GdkPixbuf  *src,
					GdkPixbuf  *dest,
					int         src_x,
					int         src_y,
					int         width,
					int         height,
					int         dest_x,
					int         dest_y,
					double      alpha);

/* Thumbnail utilities */
static GdkPixbuf *create_thumbnail_for_filename (CafeDesktopThumbnailFactory *factory,
						 const char            *filename);
static gboolean   get_thumb_annotations (GdkPixbuf             *thumb,
					 int                   *orig_width,
					 int                   *orig_height);

/* Cache */
static GdkPixbuf *get_pixbuf_for_size  (CafeBG               *bg,
					gint                  num_monitor,
					int                   width,
					int                   height);
static void       clear_cache          (CafeBG               *bg);
static gboolean   is_different         (CafeBG               *bg,
					const char            *filename);
static time_t     get_mtime            (const char            *filename);
static GdkPixbuf *create_img_thumbnail (CafeBG               *bg,
					CafeDesktopThumbnailFactory *factory,
					CdkScreen             *screen,
					int                    dest_width,
					int                    dest_height,
					int		       frame_num);
static SlideShow * get_as_slideshow    (CafeBG               *bg,
					const char 	      *filename);
static Slide *     get_current_slide   (SlideShow 	      *show,
		   			double    	      *alpha);
static gboolean    slideshow_has_multiple_sizes (SlideShow *show);

static SlideShow *read_slideshow_file (const char *filename,
				       GError     **err);
static SlideShow *slideshow_ref       (SlideShow  *show);
static void       slideshow_unref     (SlideShow  *show);

static FileSize   *find_best_size      (GSList                *sizes,
					gint                   width,
					gint                   height);

static void
color_from_string (const char *string,
		   CdkRGBA   *colorp)
{
	/* If all else fails use black */
	cdk_rgba_parse (colorp, "#000000");

	if (!string)
		return;

	cdk_rgba_parse (colorp, string);
}

static char *
color_to_string (const CdkRGBA *color)
{
	return g_strdup_printf ("#%02x%02x%02x",
				((guint) (color->red * 65535)) >> 8,
				((guint) (color->green * 65535)) >> 8,
				((guint) (color->blue * 65535)) >> 8);
}

static gboolean
do_changed (CafeBG *bg)
{
	bg->changed_id = 0;

	g_signal_emit (G_OBJECT (bg), signals[CHANGED], 0);

	return FALSE;
}

static void
queue_changed (CafeBG *bg)
{
	if (bg->changed_id > 0) {
		g_source_remove (bg->changed_id);
	}

	bg->changed_id = g_timeout_add_full (G_PRIORITY_LOW,
					     100,
					     (GSourceFunc)do_changed,
					     bg,
					     NULL);
}

static gboolean
do_transitioned (CafeBG *bg)
{
	bg->transitioned_id = 0;

	if (bg->pixbuf_cache) {
		g_object_unref (bg->pixbuf_cache);
		bg->pixbuf_cache = NULL;
	}

	g_signal_emit (G_OBJECT (bg), signals[TRANSITIONED], 0);

	return FALSE;
}

static void
queue_transitioned (CafeBG *bg)
{
	if (bg->transitioned_id > 0) {
		g_source_remove (bg->transitioned_id);
	}

	bg->transitioned_id = g_timeout_add_full (G_PRIORITY_LOW,
						  100,
						  (GSourceFunc)do_transitioned,
						  bg,
						  NULL);
}

/* This function loads the user's preferences */
void
cafe_bg_load_from_preferences (CafeBG *bg)
{
	GSettings *settings;
	settings = g_settings_new (CAFE_BG_SCHEMA);

	cafe_bg_load_from_gsettings (bg, settings);
	g_object_unref (settings);

	/* Queue change to force background redraw */
	queue_changed (bg);
}

/* This function loads default system settings */
void
cafe_bg_load_from_system_preferences (CafeBG *bg)
{
	GSettings *settings;

	/* FIXME: we need to bind system settings instead of user but
	* that's currently impossible, not implemented yet.
	* Hence, reset to system default values.
	*/
	settings = g_settings_new (CAFE_BG_SCHEMA);

	cafe_bg_load_from_system_gsettings (bg, settings, FALSE);

	g_object_unref (settings);
}

/* This function loads (and optionally resets to) default system settings */
void
cafe_bg_load_from_system_gsettings (CafeBG    *bg,
				    GSettings *settings,
				    gboolean   reset_apply)
{
	GSettingsSchema *schema;
	gchar **keys;
	gchar **k;

	g_return_if_fail (CAFE_IS_BG (bg));
	g_return_if_fail (G_IS_SETTINGS (settings));

	g_settings_delay (settings);

	g_object_get (settings, "settings-schema", &schema, NULL);
	keys = g_settings_schema_list_keys (schema);
	g_settings_schema_unref (schema);

	for (k = keys; *k; k++) {
		g_settings_reset (settings, *k);
	}
	g_strfreev (keys);

	if (reset_apply) {
		/* Apply changes atomically. */
		g_settings_apply (settings);
	} else {
		cafe_bg_load_from_gsettings (bg, settings);
		g_settings_revert (settings);
	}
}

void
cafe_bg_load_from_gsettings (CafeBG    *bg,
			     GSettings *settings)
{
	char    *tmp;
	char    *filename;
	CafeBGColorType ctype;
	CdkRGBA c1, c2;
	CafeBGPlacement placement;

	g_return_if_fail (CAFE_IS_BG (bg));
	g_return_if_fail (G_IS_SETTINGS (settings));

	bg->is_enabled = g_settings_get_boolean (settings, CAFE_BG_KEY_DRAW_BACKGROUND);

	/* Filename */
	filename = NULL;
	tmp = g_settings_get_string (settings, CAFE_BG_KEY_PICTURE_FILENAME);
	if (tmp && *tmp != '\0') {
		/* FIXME: UTF-8 checks should go away.
		 * picture-filename is of type string, which can only be used for
		 * UTF-8 strings, and some filenames are not, dependending on the
		 * locale used.
		 * It would be better (and simpler) to change to a URI instead,
		 * as URIs are UTF-8 encoded strings.
		 */
		if (g_utf8_validate (tmp, -1, NULL) &&
		    g_file_test (tmp, G_FILE_TEST_EXISTS)) {
			filename = g_strdup (tmp);
		} else {
			filename = g_filename_from_utf8 (tmp, -1, NULL, NULL, NULL);
		}

		/* Fallback to default BG if the filename set is non-existent */
		if (filename != NULL && !g_file_test (filename, G_FILE_TEST_EXISTS)) {

			g_free (filename);

			g_settings_delay (settings);
			g_settings_reset (settings, CAFE_BG_KEY_PICTURE_FILENAME);
			filename = g_settings_get_string (settings, CAFE_BG_KEY_PICTURE_FILENAME);
			g_settings_revert (settings);

			//* Check if default background exists, also */
			if (filename != NULL && !g_file_test (filename, G_FILE_TEST_EXISTS)) {
				g_free (filename);
				filename = NULL;
			}
		}
	}
	g_free (tmp);

	/* Colors */
	tmp = g_settings_get_string (settings, CAFE_BG_KEY_PRIMARY_COLOR);
	color_from_string (tmp, &c1);
	g_free (tmp);

	tmp = g_settings_get_string (settings, CAFE_BG_KEY_SECONDARY_COLOR);
	color_from_string (tmp, &c2);
	g_free (tmp);

	/* Color type */
	ctype = g_settings_get_enum (settings, CAFE_BG_KEY_COLOR_TYPE);

	/* Placement */
	placement = g_settings_get_enum (settings, CAFE_BG_KEY_PICTURE_PLACEMENT);

	cafe_bg_set_color (bg, ctype, &c1, &c2);
	cafe_bg_set_placement (bg, placement);
	cafe_bg_set_filename (bg, filename);

	if (filename != NULL)
		g_free (filename);
}

void
cafe_bg_save_to_preferences (CafeBG *bg)
{
	GSettings *settings;
	settings = g_settings_new (CAFE_BG_SCHEMA);

	cafe_bg_save_to_gsettings (bg, settings);
	g_object_unref (settings);
}

void
cafe_bg_save_to_gsettings (CafeBG    *bg,
			   GSettings *settings)
{
	gchar *primary;
	gchar *secondary;

	g_return_if_fail (CAFE_IS_BG (bg));
	g_return_if_fail (G_IS_SETTINGS (settings));

	primary = color_to_string (&bg->primary);
	secondary = color_to_string (&bg->secondary);

	g_settings_delay (settings);

	g_settings_set_boolean (settings, CAFE_BG_KEY_DRAW_BACKGROUND, bg->is_enabled);
	g_settings_set_string (settings, CAFE_BG_KEY_PICTURE_FILENAME, bg->filename);
	g_settings_set_enum (settings, CAFE_BG_KEY_PICTURE_PLACEMENT, bg->placement);
	g_settings_set_string (settings, CAFE_BG_KEY_PRIMARY_COLOR, primary);
	g_settings_set_string (settings, CAFE_BG_KEY_SECONDARY_COLOR, secondary);
	g_settings_set_enum (settings, CAFE_BG_KEY_COLOR_TYPE, bg->color_type);

	/* Apply changes atomically. */
	g_settings_apply (settings);

	g_free (primary);
	g_free (secondary);
}


static void
cafe_bg_init (CafeBG *bg G_GNUC_UNUSED)
{
}

static void
cafe_bg_dispose (GObject *object)
{
	CafeBG *bg = CAFE_BG (object);

	if (bg->file_monitor) {
		g_object_unref (bg->file_monitor);
		bg->file_monitor = NULL;
	}

	clear_cache (bg);

	G_OBJECT_CLASS (cafe_bg_parent_class)->dispose (object);
}

static void
cafe_bg_finalize (GObject *object)
{
	CafeBG *bg = CAFE_BG (object);

	if (bg->changed_id != 0) {
		g_source_remove (bg->changed_id);
		bg->changed_id = 0;
	}

	if (bg->transitioned_id != 0) {
		g_source_remove (bg->transitioned_id);
		bg->transitioned_id = 0;
	}

	if (bg->blow_caches_id != 0) {
		g_source_remove (bg->blow_caches_id);
		bg->blow_caches_id = 0;
	}

	g_free (bg->filename);
	bg->filename = NULL;

	G_OBJECT_CLASS (cafe_bg_parent_class)->finalize (object);
}

static void
cafe_bg_class_init (CafeBGClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = cafe_bg_dispose;
	object_class->finalize = cafe_bg_finalize;

	signals[CHANGED] = g_signal_new ("changed",
					 G_OBJECT_CLASS_TYPE (object_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE, 0);

	signals[TRANSITIONED] = g_signal_new ("transitioned",
					 G_OBJECT_CLASS_TYPE (object_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL, NULL,
					 g_cclosure_marshal_VOID__VOID,
					 G_TYPE_NONE, 0);
}

CafeBG *
cafe_bg_new (void)
{
	return g_object_new (CAFE_TYPE_BG, NULL);
}

void
cafe_bg_set_color (CafeBG *bg,
		    CafeBGColorType type,
		    CdkRGBA *primary,
		    CdkRGBA *secondary)
{
	g_return_if_fail (bg != NULL);
	g_return_if_fail (primary != NULL);

	if (bg->color_type != type			||
	    !cdk_rgba_equal (&bg->primary, primary)			||
	    (secondary && !cdk_rgba_equal (&bg->secondary, secondary))) {
		bg->color_type = type;
		bg->primary = *primary;
		if (secondary) {
			bg->secondary = *secondary;
		}

		queue_changed (bg);
	}
}

void
cafe_bg_set_placement (CafeBG		*bg,
		       CafeBGPlacement	 placement)
{
	g_return_if_fail (bg != NULL);

	if (bg->placement != placement) {
		bg->placement = placement;

		queue_changed (bg);
	}
}

CafeBGPlacement
cafe_bg_get_placement (CafeBG *bg)
{
	g_return_val_if_fail (bg != NULL, -1);

	return bg->placement;
}

void
cafe_bg_get_color (CafeBG		*bg,
		   CafeBGColorType	*type,
		   CdkRGBA		*primary,
		   CdkRGBA		*secondary)
{
	g_return_if_fail (bg != NULL);

	if (type)
		*type = bg->color_type;

	if (primary)
		*primary = bg->primary;

	if (secondary)
		*secondary = bg->secondary;
}

void
cafe_bg_set_draw_background (CafeBG	*bg,
			     gboolean	 draw_background)
{
	g_return_if_fail (bg != NULL);

	if (bg->is_enabled != draw_background) {
		bg->is_enabled = draw_background;

		queue_changed (bg);
	}
}

gboolean
cafe_bg_get_draw_background (CafeBG *bg)
{
	g_return_val_if_fail (bg != NULL, FALSE);

	return bg->is_enabled;
}

const gchar *
cafe_bg_get_filename (CafeBG *bg)
{
	g_return_val_if_fail (bg != NULL, NULL);

	return bg->filename;
}

static inline gchar *
get_wallpaper_cache_dir ()
{
	return g_build_filename (g_get_user_cache_dir(), CAFE_BG_CACHE_DIR, NULL);
}

static inline gchar *
get_wallpaper_cache_prefix_name (gint                     num_monitor,
				 CafeBGPlacement          placement,
				 gint                     width,
				 gint                     height)
{
	return g_strdup_printf ("%i_%i_%i_%i", num_monitor, (gint) placement, width, height);
}

static char *
get_wallpaper_cache_filename (const char              *filename,
			      gint                     num_monitor,
			      CafeBGPlacement          placement,
			      gint                     width,
			      gint                     height)
{
	gchar *cache_filename;
	gchar *cache_prefix_name;
	gchar *md5_filename;
	gchar *cache_basename;
	gchar *cache_dir;

	md5_filename = g_compute_checksum_for_data (G_CHECKSUM_MD5, (const guchar *) filename,
						    strlen (filename));
	cache_prefix_name = get_wallpaper_cache_prefix_name (num_monitor, placement, width, height);
	cache_basename = g_strdup_printf ("%s_%s", cache_prefix_name, md5_filename);
	cache_dir = get_wallpaper_cache_dir ();
	cache_filename = g_build_filename (cache_dir, cache_basename, NULL);

	g_free (cache_prefix_name);
	g_free (md5_filename);
	g_free (cache_basename);
	g_free (cache_dir);

	return cache_filename;
}

static void
cleanup_cache_for_monitor (gchar *cache_dir,
			   gint   num_monitor)
{
	GDir            *g_cache_dir;
	gchar           *monitor_prefix;
	const gchar     *file;

	g_cache_dir = g_dir_open (cache_dir, 0, NULL);
	monitor_prefix = g_strdup_printf ("%i_", num_monitor);

	file = g_dir_read_name (g_cache_dir);
	while (file != NULL) {
		gchar *path = g_build_filename (cache_dir, file, NULL);

		/* purge files with same monitor id */
		if (g_str_has_prefix (file, monitor_prefix) &&
		    g_file_test (path, G_FILE_TEST_IS_REGULAR))
			g_unlink (path);

		g_free (path);

		file = g_dir_read_name (g_cache_dir);
	}

	g_free (monitor_prefix);
	g_dir_close (g_cache_dir);
}

static gboolean
cache_file_is_valid (const char *filename,
		     const char *cache_filename)
{
	time_t mtime;
	time_t cache_mtime;

	if (!g_file_test (cache_filename, G_FILE_TEST_IS_REGULAR))
		return FALSE;

	mtime = get_mtime (filename);
	cache_mtime = get_mtime (cache_filename);

	return (mtime < cache_mtime);
}

static void
refresh_cache_file (CafeBG     *bg,
		    GdkPixbuf  *new_pixbuf,
		    gint        num_monitor,
		    gint        width,
		    gint        height)
{
	gchar           *cache_filename;
	gchar           *cache_dir;

	if ((num_monitor == -1) || (width <= 300) || (height <= 300))
		return;

	cache_filename = get_wallpaper_cache_filename (bg->filename, num_monitor,
							bg->placement, width, height);
	cache_dir = get_wallpaper_cache_dir ();

	/* Only refresh scaled file on disk if useful (and don't cache slideshow) */
	if (!cache_file_is_valid (bg->filename, cache_filename)) {
		GdkPixbufFormat *format;

		format = gdk_pixbuf_get_file_info (bg->filename, NULL, NULL);

		if (format != NULL) {
			gchar *format_name;

			if (!g_file_test (cache_dir, G_FILE_TEST_IS_DIR)) {
				g_mkdir_with_parents (cache_dir, 0700);
			} else {
				cleanup_cache_for_monitor (cache_dir, num_monitor);
			}

			format_name = gdk_pixbuf_format_get_name (format);

			if (strcmp (format_name, "jpeg") == 0)
				gdk_pixbuf_save (new_pixbuf, cache_filename, format_name,
						 NULL, "quality", "100", NULL);
			else
				gdk_pixbuf_save (new_pixbuf, cache_filename, format_name,
						 NULL, NULL);

			g_free (format_name);
		}
	}

	g_free (cache_filename);
	g_free (cache_dir);
}

static void
file_changed (GFileMonitor     *file_monitor G_GNUC_UNUSED,
	      GFile            *child G_GNUC_UNUSED,
	      GFile            *other_file G_GNUC_UNUSED,
	      GFileMonitorEvent event_type G_GNUC_UNUSED,
	      gpointer          user_data)
{
	CafeBG *bg = CAFE_BG (user_data);

	clear_cache (bg);
	queue_changed (bg);
}

void
cafe_bg_set_filename (CafeBG	 *bg,
		      const char *filename)
{
	g_return_if_fail (bg != NULL);

	if (is_different (bg, filename)) {
		g_free (bg->filename);

		bg->filename = g_strdup (filename);
		bg->file_mtime = get_mtime (bg->filename);

		if (bg->file_monitor) {
			g_object_unref (bg->file_monitor);
			bg->file_monitor = NULL;
		}

		if (bg->filename) {
			GFile *f = g_file_new_for_path (bg->filename);

			bg->file_monitor = g_file_monitor_file (f, 0, NULL, NULL);
			g_signal_connect (bg->file_monitor, "changed",
					  G_CALLBACK (file_changed), bg);

			g_object_unref (f);
		}

		clear_cache (bg);

		queue_changed (bg);
	}
}

static void
draw_color_area (CafeBG       *bg,
		 GdkPixbuf    *dest,
		 CdkRectangle *rect)
{
	guint32 pixel;
	CdkRectangle extent;

        extent.x = 0;
        extent.y = 0;
        extent.width = gdk_pixbuf_get_width (dest);
        extent.height = gdk_pixbuf_get_height (dest);

	cdk_rectangle_intersect (rect, &extent, rect);

	switch (bg->color_type) {
	case CAFE_BG_COLOR_SOLID:
		/* not really a big deal to ignore the area of interest */
		pixel = ((guint) (bg->primary.red * 0xff) << 24)   |
			((guint) (bg->primary.green * 0xff) << 16) |
			((guint) (bg->primary.blue * 0xff) << 8)   |
			(0xff);

		gdk_pixbuf_fill (dest, pixel);
		break;

	case CAFE_BG_COLOR_H_GRADIENT:
		pixbuf_draw_gradient (dest, TRUE, &(bg->primary), &(bg->secondary), rect);
		break;

	case CAFE_BG_COLOR_V_GRADIENT:
		pixbuf_draw_gradient (dest, FALSE, &(bg->primary), &(bg->secondary), rect);
		break;

	default:
		break;
	}
}

static void
draw_color (CafeBG    *bg,
	    GdkPixbuf *dest)
{
	CdkRectangle rect;

	rect.x = 0;
	rect.y = 0;
	rect.width = gdk_pixbuf_get_width (dest);
	rect.height = gdk_pixbuf_get_height (dest);
	draw_color_area (bg, dest, &rect);
}

static void
draw_color_each_monitor (CafeBG    *bg,
			 GdkPixbuf *dest,
			 CdkScreen *screen)
{
	CdkDisplay *display;
	CdkRectangle rect;
	gint num_monitors;
	int monitor;

	display = cdk_screen_get_display (screen);
	num_monitors = cdk_display_get_n_monitors (display);
	for (monitor = 0; monitor < num_monitors; monitor++) {
		cdk_monitor_get_geometry (cdk_display_get_monitor (display, monitor), &rect);
		draw_color_area (bg, dest, &rect);
	}
}

static GdkPixbuf *
pixbuf_clip_to_fit (GdkPixbuf *src,
		    int        max_width,
		    int        max_height)
{
	int src_width, src_height;
	int w, h;
	int src_x, src_y;
	GdkPixbuf *pixbuf;

	src_width = gdk_pixbuf_get_width (src);
	src_height = gdk_pixbuf_get_height (src);

	if (src_width < max_width && src_height < max_height)
		return g_object_ref (src);

	w = MIN(src_width, max_width);
	h = MIN(src_height, max_height);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				 gdk_pixbuf_get_has_alpha (src),
				 8, w, h);

	src_x = (src_width - w) / 2;
	src_y = (src_height - h) / 2;
	gdk_pixbuf_copy_area (src,
			      src_x, src_y,
			      w, h,
			      pixbuf,
			      0, 0);
	return pixbuf;
}

static GdkPixbuf *
get_scaled_pixbuf (CafeBGPlacement  placement,
		   GdkPixbuf       *pixbuf,
		   int width, int height,
		   int *x, int *y,
		   int *w, int *h)
{
	GdkPixbuf *new;

#if 0
	g_print ("original_width: %d %d\n",
		 gdk_pixbuf_get_width (pixbuf),
		 gdk_pixbuf_get_height (pixbuf));
#endif

	switch (placement) {
	case CAFE_BG_PLACEMENT_SPANNED:
                new = pixbuf_scale_to_fit (pixbuf, width, height);
		break;
	case CAFE_BG_PLACEMENT_ZOOMED:
		new = pixbuf_scale_to_min (pixbuf, width, height);
		break;

	case CAFE_BG_PLACEMENT_FILL_SCREEN:
		new = gdk_pixbuf_scale_simple (pixbuf, width, height,
					       GDK_INTERP_BILINEAR);
		break;

	case CAFE_BG_PLACEMENT_SCALED:
		new = pixbuf_scale_to_fit (pixbuf, width, height);
		break;

	case CAFE_BG_PLACEMENT_CENTERED:
	case CAFE_BG_PLACEMENT_TILED:
	default:
		new = pixbuf_clip_to_fit (pixbuf, width, height);
		break;
	}

	*w = gdk_pixbuf_get_width (new);
	*h = gdk_pixbuf_get_height (new);
	*x = (width - *w) / 2;
	*y = (height - *h) / 2;

	return new;
}


static void
draw_image_area (CafeBG        *bg,
		 gint           num_monitor,
		 GdkPixbuf     *pixbuf,
		 GdkPixbuf     *dest,
		 CdkRectangle  *area)
{
	int dest_width = area->width;
	int dest_height = area->height;
	int x, y, w, h;
	GdkPixbuf *scaled;

	if (!pixbuf)
		return;

	scaled = get_scaled_pixbuf (bg->placement, pixbuf, dest_width, dest_height, &x, &y, &w, &h);

	switch (bg->placement) {
	case CAFE_BG_PLACEMENT_TILED:
		pixbuf_tile (scaled, dest);
		break;
	case CAFE_BG_PLACEMENT_ZOOMED:
	case CAFE_BG_PLACEMENT_CENTERED:
	case CAFE_BG_PLACEMENT_FILL_SCREEN:
	case CAFE_BG_PLACEMENT_SCALED:
		pixbuf_blend (scaled, dest, 0, 0, w, h, x + area->x, y + area->y, 1.0);
		break;
	case CAFE_BG_PLACEMENT_SPANNED:
		pixbuf_blend (scaled, dest, 0, 0, w, h, x, y, 1.0);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	refresh_cache_file (bg, scaled, num_monitor, dest_width, dest_height);

	g_object_unref (scaled);
}

static void
draw_image_for_thumb (CafeBG     *bg,
		      GdkPixbuf  *pixbuf,
		      GdkPixbuf  *dest)
{
	CdkRectangle rect;

	rect.x = 0;
	rect.y = 0;
	rect.width = gdk_pixbuf_get_width (dest);
	rect.height = gdk_pixbuf_get_height (dest);

	draw_image_area (bg, -1, pixbuf, dest, &rect);
}

static void
draw_once (CafeBG    *bg,
	   GdkPixbuf *dest,
	   gboolean   is_root)
{
	CdkRectangle rect;
	GdkPixbuf   *pixbuf;
	gint         monitor;

	/* whether we're drawing on root window or normal (Baul) window */
	monitor = (is_root) ? 0 : -1;

	rect.x = 0;
	rect.y = 0;
	rect.width = gdk_pixbuf_get_width (dest);
	rect.height = gdk_pixbuf_get_height (dest);

	pixbuf = get_pixbuf_for_size (bg, monitor, rect.width, rect.height);
	if (pixbuf) {
		draw_image_area (bg, monitor, pixbuf, dest, &rect);

		g_object_unref (pixbuf);
	}
}

static void
draw_each_monitor (CafeBG    *bg,
		   GdkPixbuf *dest,
		   CdkScreen *screen)
{
	CdkDisplay *display;

	display = cdk_screen_get_display (screen);
	gint num_monitors = cdk_display_get_n_monitors (display);
	gint monitor = 0;

	for (; monitor < num_monitors; monitor++) {
		CdkRectangle rect;
		GdkPixbuf *pixbuf;

		cdk_monitor_get_geometry (cdk_display_get_monitor (display, monitor), &rect);

		pixbuf = get_pixbuf_for_size (bg, monitor, rect.width, rect.height);
		if (pixbuf) {
			draw_image_area (bg, monitor, pixbuf, dest, &rect);

			g_object_unref (pixbuf);
		}
	}
}

void
cafe_bg_draw (CafeBG     *bg,
	       GdkPixbuf *dest,
	       CdkScreen *screen,
	       gboolean   is_root)
{
	if (!bg)
		return;

	if (is_root && (bg->placement != CAFE_BG_PLACEMENT_SPANNED)) {
		draw_color_each_monitor (bg, dest, screen);
		if (bg->filename) {
			draw_each_monitor (bg, dest, screen);
		}
	} else {
		draw_color (bg, dest);
		if (bg->filename) {
			draw_once (bg, dest, is_root);
		}
	}
}

gboolean
cafe_bg_has_multiple_sizes (CafeBG *bg)
{
	SlideShow *show;
	gboolean ret;

	g_return_val_if_fail (bg != NULL, FALSE);

	ret = FALSE;

	show = get_as_slideshow (bg, bg->filename);
	if (show) {
		ret = slideshow_has_multiple_sizes (show);
		slideshow_unref (show);
	}

	return ret;
}

static void
cafe_bg_get_pixmap_size (CafeBG   *bg,
			  int        width,
			  int        height,
			  int       *pixmap_width,
			  int       *pixmap_height)
{
	int dummy;

	if (!pixmap_width)
		pixmap_width = &dummy;
	if (!pixmap_height)
		pixmap_height = &dummy;

	*pixmap_width = width;
	*pixmap_height = height;

	if (!bg->filename) {
		switch (bg->color_type) {
		case CAFE_BG_COLOR_SOLID:
			*pixmap_width = 1;
			*pixmap_height = 1;
			break;

		case CAFE_BG_COLOR_H_GRADIENT:
		case CAFE_BG_COLOR_V_GRADIENT:
			break;
		}

		return;
	}
}

/**
 * cafe_bg_create_surface:
 * @bg: CafeBG
 * @window:
 * @width:
 * @height:
 * @root:
 *
 * Create a surface that can be set as background for @window. If @root is
 * TRUE, the surface created will be created by a temporary X server connection
 * so that if someone calls XKillClient on it, it won't affect the application
 * who created it.
 **/
cairo_surface_t *
cafe_bg_create_surface (CafeBG      *bg,
		 	CdkWindow   *window,
			int	     width,
			int	     height,
			gboolean     root)
{
	return cafe_bg_create_surface_scale (bg,
					     window,
					     width,
					     height,
					     1,
					     root);
}

/**
 * cafe_bg_create_surface_scale:
 * @bg: CafeBG
 * @window:
 * @width:
 * @height:
 * @scale:
 * @root:
 *
 * Create a scaled surface that can be set as background for @window. If @root is
 * TRUE, the surface created will be created by a temporary X server connection
 * so that if someone calls XKillClient on it, it won't affect the application
 * who created it.
 **/
cairo_surface_t *
cafe_bg_create_surface_scale (CafeBG      *bg,
			      CdkWindow   *window,
			      int          width,
			      int          height,
			      int          scale,
			      gboolean     root)
{
	int pm_width, pm_height;

	cairo_surface_t *surface;
	cairo_t *cr;

	g_return_val_if_fail (bg != NULL, NULL);
	g_return_val_if_fail (window != NULL, NULL);

	if (bg->pixbuf_cache &&
	    (gdk_pixbuf_get_width (bg->pixbuf_cache) != width ||
	     gdk_pixbuf_get_height (bg->pixbuf_cache) != height))
	{
		g_object_unref (bg->pixbuf_cache);
		bg->pixbuf_cache = NULL;
	}

	cafe_bg_get_pixmap_size (bg, width, height, &pm_width, &pm_height);

	if (root)
	{
		surface = make_root_pixmap (window, pm_width * scale, pm_height * scale);
	}
	else
	{
		surface = cdk_window_create_similar_surface (window, CAIRO_CONTENT_COLOR,
							     pm_width, pm_height);
	}

	cr = cairo_create (surface);
	cairo_scale (cr, (double)scale, (double)scale);

	if (!bg->filename && bg->color_type == CAFE_BG_COLOR_SOLID) {
		cdk_cairo_set_source_rgba (cr, &(bg->primary));
	}
	else
	{
		GdkPixbuf *pixbuf;

		pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
					 width, height);
		cafe_bg_draw (bg, pixbuf, cdk_window_get_screen (window), root);
		cdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
		g_object_unref (pixbuf);
	}

	cairo_paint (cr);

	cairo_destroy (cr);

	return surface;
}


/* determine if a background is darker or lighter than average, to help
 * clients know what colors to draw on top with
 */
gboolean
cafe_bg_is_dark (CafeBG *bg,
		  int      width,
		  int      height)
{
	CdkRGBA color;
	int intensity;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (bg != NULL, FALSE);

	if (bg->color_type == CAFE_BG_COLOR_SOLID) {
		color = bg->primary;
	} else {
		color.red = (bg->primary.red + bg->secondary.red) / 2;
		color.green = (bg->primary.green + bg->secondary.green) / 2;
		color.blue = (bg->primary.blue + bg->secondary.blue) / 2;
	}
	pixbuf = get_pixbuf_for_size (bg, -1, width, height);
	if (pixbuf) {
		CdkRGBA argb;
		guchar a, r, g, b;

		pixbuf_average_value (pixbuf, &argb);
		a = argb.alpha * 0xff;
		r = argb.red * 0xff;
		g = argb.green * 0xff;
		b = argb.blue * 0xff;

		color.red = (color.red * (0xFF - a) + r * 0x101 * a) / 0xFF;
		color.green = (color.green * (0xFF - a) + g * 0x101 * a) / 0xFF;
		color.blue = (color.blue * (0xFF - a) + b * 0x101 * a) / 0xFF;
		g_object_unref (pixbuf);
	}

	intensity = ((guint) (color.red * 65535) * 77 +
		     (guint) (color.green * 65535) * 150 +
		     (guint) (color.blue * 65535) * 28) >> 16;

	return intensity < 160; /* biased slightly to be dark */
}

/*
 * Create a persistent pixmap. We create a separate display
 * and set the closedown mode on it to RetainPermanent.
 */
static cairo_surface_t *
make_root_pixmap (CdkWindow *window, gint width, gint height)
{
	CdkScreen *screen = cdk_window_get_screen(window);
	char *disp_name = DisplayString (CDK_WINDOW_XDISPLAY (window));
	Display *display;
	Pixmap xpixmap;
	cairo_surface_t *surface;
	int depth;

	/* Desktop background pixmap should be created from dummy X client since most
	 * applications will try to kill it with XKillClient later when changing pixmap
	 */
	display = XOpenDisplay (disp_name);

	if (display == NULL) {
		g_warning ("Unable to open display '%s' when setting background pixmap\n",
		           (disp_name) ? disp_name : "NULL");
		return NULL;
	}

	depth = DefaultDepth (display, cdk_x11_screen_get_screen_number (screen));
	xpixmap = XCreatePixmap (display, CDK_WINDOW_XID (window), width, height, depth);

	XFlush (display);
	XSetCloseDownMode (display, RetainPermanent);
	XCloseDisplay (display);

	surface = cairo_xlib_surface_create (CDK_SCREEN_XDISPLAY (screen), xpixmap,
                                             CDK_VISUAL_XVISUAL (cdk_screen_get_system_visual (screen)),
        				     width, height);

	return surface;
}

static gboolean
get_original_size (const char *filename,
		   int        *orig_width,
		   int        *orig_height)
{
	gboolean result;

        if (gdk_pixbuf_get_file_info (filename, orig_width, orig_height))
		result = TRUE;
	else
		result = FALSE;

	return result;
}

static const char *
get_filename_for_size (CafeBG *bg, gint best_width, gint best_height)
{
	SlideShow *show;
	Slide *slide;
	FileSize *size;

	if (!bg->filename)
		return NULL;

	show = get_as_slideshow (bg, bg->filename);
	if (!show) {
		return bg->filename;
	}

	slide = get_current_slide (show, NULL);
	slideshow_unref (show);
	size = find_best_size (slide->file1, best_width, best_height);
	return size->file;
}

gboolean
cafe_bg_get_image_size (CafeBG	       *bg,
			 CafeDesktopThumbnailFactory *factory,
			 int                    best_width,
			 int                    best_height,
			 int		       *width,
			 int		       *height)
{
	GdkPixbuf *thumb;
	gboolean result = FALSE;
	const gchar *filename;

	g_return_val_if_fail (bg != NULL, FALSE);
	g_return_val_if_fail (factory != NULL, FALSE);

	if (!bg->filename)
		return FALSE;

	filename = get_filename_for_size (bg, best_width, best_height);
	thumb = create_thumbnail_for_filename (factory, filename);
	if (thumb) {
		if (get_thumb_annotations (thumb, width, height))
			result = TRUE;

		g_object_unref (thumb);
	}

	if (!result) {
		if (get_original_size (filename, width, height))
			result = TRUE;
	}

	return result;
}

static double
fit_factor (int from_width, int from_height,
	    int to_width,   int to_height)
{
	return MIN (to_width  / (double) from_width, to_height / (double) from_height);
}

/**
 * cafe_bg_create_thumbnail:
 *
 * Returns: (transfer full): a #GdkPixbuf showing the background as a thumbnail
 */
GdkPixbuf *
cafe_bg_create_thumbnail (CafeBG               *bg,
		           CafeDesktopThumbnailFactory *factory,
			   CdkScreen             *screen,
			   int                    dest_width,
			   int                    dest_height)
{
	GdkPixbuf *result;

	g_return_val_if_fail (bg != NULL, NULL);

	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, dest_width, dest_height);

	draw_color (bg, result);

	if (bg->filename) {
		GdkPixbuf *thumb;

		thumb = create_img_thumbnail (bg, factory, screen, dest_width, dest_height, -1);

		if (thumb) {
			draw_image_for_thumb (bg, thumb, result);
			g_object_unref (thumb);
		}
	}

	return result;
}

/**
 * cafe_bg_get_surface_from_root:
 * @screen: a #CdkScreen
 *
 * This function queries the _XROOTPMAP_ID property from
 * the root window associated with @screen to determine
 * the current root window background surface and returns
 * a copy of it. If the _XROOTPMAP_ID is not set, then
 * a black surface is returned.
 *
 * Return value: a #cairo_surface_t if successful or %NULL
 **/
cairo_surface_t *
cafe_bg_get_surface_from_root (CdkScreen *screen)
{
	int result;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guchar *data;
	Atom type;
	Display *display;
	int screen_num;
	cairo_surface_t *surface;
	cairo_surface_t *source_pixmap;
	int width, height;

	display = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (screen));
	screen_num = cdk_x11_screen_get_screen_number (screen);

	result = XGetWindowProperty (display,
				     RootWindow (display, screen_num),
				     cdk_x11_get_xatom_by_name ("_XROOTPMAP_ID"),
				     0L, 1L, False, XA_PIXMAP,
				     &type, &format, &nitems, &bytes_after,
				     &data);
	surface = NULL;
	source_pixmap = NULL;

	if (result != Success || type != XA_PIXMAP ||
	    format != 32 || nitems != 1) {
		XFree (data);
		data = NULL;
	}

	if (data != NULL) {
		CdkDisplay *cdkdisplay;

		cdkdisplay = cdk_screen_get_display (screen);
		cdk_x11_display_error_trap_push (cdkdisplay);

		Pixmap xpixmap = *(Pixmap *) data;
		Window root_return;
		int x_ret, y_ret;
		unsigned int w_ret, h_ret, bw_ret, depth_ret;

		if (XGetGeometry (CDK_SCREEN_XDISPLAY (screen),
				  xpixmap,
				  &root_return,
				  &x_ret, &y_ret, &w_ret, &h_ret, &bw_ret, &depth_ret))
		{
			source_pixmap = cairo_xlib_surface_create (CDK_SCREEN_XDISPLAY (screen),
								   xpixmap,
								   CDK_VISUAL_XVISUAL (cdk_screen_get_system_visual (screen)),
								   w_ret, h_ret);
		}

		cdk_x11_display_error_trap_pop_ignored (cdkdisplay);
	}

	width = WidthOfScreen (cdk_x11_screen_get_xscreen (screen));
	height = HeightOfScreen (cdk_x11_screen_get_xscreen (screen));

	if (source_pixmap) {
		cairo_t *cr;

		surface = cairo_surface_create_similar (source_pixmap,
							CAIRO_CONTENT_COLOR,
							width, height);

		cr = cairo_create (surface);
		cairo_set_source_surface (cr, source_pixmap, 0, 0);
		cairo_paint (cr);

		if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
			cairo_surface_destroy (surface);
			surface = NULL;
		}

		cairo_destroy (cr);
	}

	if (surface == NULL) {
		surface = cdk_window_create_similar_surface (cdk_screen_get_root_window (screen),
							     CAIRO_CONTENT_COLOR,
							     width, height);
	}

	if (source_pixmap != NULL)
		cairo_surface_destroy (source_pixmap);

	if (data != NULL)
		XFree (data);

	return surface;
}

/* Sets the "ESETROOT_PMAP_ID" property to later be used to free the pixmap,
 */
static void
cafe_bg_set_root_pixmap_id (CdkScreen       *screen,
			    Display         *display,
			    Pixmap           xpixmap)
{
	Window   xroot   = RootWindow (display, cdk_x11_screen_get_screen_number (screen));
	char    *atom_names[] = {"_XROOTPMAP_ID", "ESETROOT_PMAP_ID"};
	Atom     atoms[G_N_ELEMENTS(atom_names)] = {0};

	Atom     type;
	int      format;
	unsigned long nitems, after;
	unsigned char *data_root, *data_esetroot;

	/* Get atoms for both properties in an array, only if they exist.
	 * This method is to avoid multiple round-trips to Xserver
	 */
	if (XInternAtoms (display, atom_names, G_N_ELEMENTS(atom_names), True, atoms) &&
	    atoms[0] != None && atoms[1] != None) {
		int result;

		result = XGetWindowProperty (display, xroot, atoms[0], 0L, 1L,
		                             False, AnyPropertyType,
		                             &type, &format, &nitems, &after,
		                             &data_root);

		if (data_root != NULL && result == Success &&
		    type == XA_PIXMAP && format == 32 && nitems == 1) {
			result = XGetWindowProperty (display, xroot, atoms[1],
			                             0L, 1L, False,
			                             AnyPropertyType,
			                             &type, &format, &nitems,
			                             &after, &data_esetroot);

			if (data_esetroot != NULL && result == Success &&
			    type == XA_PIXMAP && format == 32 && nitems == 1) {
				CdkDisplay *cdkdisplay;

				Pixmap xrootpmap = *((Pixmap *) data_root);
				Pixmap esetrootpmap = *((Pixmap *) data_esetroot);

				cdkdisplay = cdk_screen_get_display (screen);
				cdk_x11_display_error_trap_push (cdkdisplay);
				if (xrootpmap && xrootpmap == esetrootpmap) {
					XKillClient (display, xrootpmap);
				}
				if (esetrootpmap && esetrootpmap != xrootpmap) {
					XKillClient (display, esetrootpmap);
				}
				cdk_x11_display_error_trap_pop_ignored (cdkdisplay);
			}
			if (data_esetroot != NULL) {
				XFree (data_esetroot);
			}
		}
		if (data_root != NULL) {
			XFree (data_root);
		}
	}

	/* Get atoms for both properties in an array, create them if needed.
	 * This method is to avoid multiple round-trips to Xserver
	 */
	if (!XInternAtoms (display, atom_names, G_N_ELEMENTS(atom_names), False, atoms) ||
	    atoms[0] == None || atoms[1] == None) {
	    g_warning ("Could not create atoms needed to set root pixmap id/properties.\n");
	    return;
	}

	/* Set new _XROOTMAP_ID and ESETROOT_PMAP_ID properties */
	XChangeProperty (display, xroot, atoms[0], XA_PIXMAP, 32,
			 PropModeReplace, (unsigned char *) &xpixmap, 1);

	XChangeProperty (display, xroot, atoms[1], XA_PIXMAP, 32,
			 PropModeReplace, (unsigned char *) &xpixmap, 1);
}

/**
 * cafe_bg_set_surface_as_root:
 * @screen: the #CdkScreen to change root background on
 * @surface: the #cairo_surface_t to set root background from.
 *   Must be an xlib surface backing a pixmap.
 *
 * Set the root pixmap, and properties pointing to it. We
 * do this atomically with a server grab to make sure that
 * we won't leak the pixmap if somebody else it setting
 * it at the same time. (This assumes that they follow the
 * same conventions we do).  @surface should come from a call
 * to cafe_bg_create_surface().
 **/
void
cafe_bg_set_surface_as_root (CdkScreen *screen, cairo_surface_t *surface)
{
	g_return_if_fail (screen != NULL);
	g_return_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_XLIB);

	/* Desktop background pixmap should be created from dummy X client since most
	 * applications will try to kill it with XKillClient later when changing pixmap
	 */
	Display    *display      = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (screen));
	Pixmap      pixmap_id    = cairo_xlib_surface_get_drawable (surface);
	Window      xroot        = RootWindow (display, cdk_x11_screen_get_screen_number (screen));

	XGrabServer (display);
	cafe_bg_set_root_pixmap_id (screen, display, pixmap_id);

	XSetWindowBackgroundPixmap (display, xroot, pixmap_id);
	XClearWindow (display, xroot);

	XFlush (display);
	XUngrabServer (display);
}

/**
 * cafe_bg_set_surface_as_root_with_crossfade:
 * @screen: the #CdkScreen to change root background on
 * @surface: the cairo xlib surface to set root background from
 *
 * Set the root pixmap, and properties pointing to it.
 * This function differs from cafe_bg_set_surface_as_root()
 * in that it adds a subtle crossfade animation from the
 * current root pixmap to the new one.
 *
 * Return value: (transfer full): a #CafeBGCrossfade object
 **/
CafeBGCrossfade *
cafe_bg_set_surface_as_root_with_crossfade (CdkScreen       *screen,
		 			    cairo_surface_t *surface)
{
	CdkWindow       *root_window;
	int              width, height;
	CafeBGCrossfade *fade;
	cairo_t         *cr;
	cairo_surface_t *old_surface;

	g_return_val_if_fail (screen != NULL, NULL);
	g_return_val_if_fail (surface != NULL, NULL);

	root_window = cdk_screen_get_root_window (screen);
	width       = cdk_window_get_width (root_window);
	height      = cdk_window_get_height (root_window);
	fade        = cafe_bg_crossfade_new (width, height);
	old_surface = cafe_bg_get_surface_from_root (screen);

	cafe_bg_crossfade_set_start_surface (fade, old_surface);
	cafe_bg_crossfade_set_end_surface (fade, surface);

	/* Before setting the surface as a root pixmap, let's have it draw
	 * the old stuff, just so it won't be noticable
	 * (crossfade will later get it back)
	 */
	cr = cairo_create (surface);
	cairo_set_source_surface (cr, old_surface, 0, 0);
	cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
	cairo_paint (cr);
	cairo_destroy (cr);
	cairo_surface_destroy (old_surface);

	cafe_bg_set_surface_as_root (screen, surface);
	cafe_bg_crossfade_start (fade, root_window);

	return fade;
}

/* Implementation of the pixbuf cache */
struct _SlideShow
{
	gint ref_count;
	double start_time;
	double total_duration;

	GQueue *slides;

	gboolean has_multiple_sizes;

	/* used during parsing */
	struct tm start_tm;
	GQueue *stack;
};

static double
now (void)
{
	const double microseconds_per_second = (double) G_USEC_PER_SEC;
	gint64 tv;

	tv = g_get_real_time ();

	return (double) (tv / microseconds_per_second);
}

static Slide *
get_current_slide (SlideShow *show,
		   double    *alpha)
{
	double delta = fmod (now() - show->start_time, show->total_duration);
	GList *list;
	double elapsed;
	int i;

	if (delta < 0)
		delta += show->total_duration;

	elapsed = 0;
	i = 0;
	for (list = show->slides->head; list != NULL; list = list->next) {
		Slide *slide = list->data;

		if (elapsed + slide->duration > delta) {
			if (alpha)
				*alpha = (delta - elapsed) / (double)slide->duration;
			return slide;
		}

		i++;
		elapsed += slide->duration;
	}

	/* this should never happen since we have slides and we should always
	 * find a current slide for the elapsed time since beginning -- we're
	 * looping with fmod() */
	g_assert_not_reached ();

	return NULL;
}

static GdkPixbuf *
blend (GdkPixbuf *p1,
       GdkPixbuf *p2,
       double alpha)
{
	GdkPixbuf *result = gdk_pixbuf_copy (p1);
	GdkPixbuf *tmp;

	if (gdk_pixbuf_get_width (p2) != gdk_pixbuf_get_width (p1) ||
            gdk_pixbuf_get_height (p2) != gdk_pixbuf_get_height (p1)) {
		tmp = gdk_pixbuf_scale_simple (p2,
					       gdk_pixbuf_get_width (p1),
					       gdk_pixbuf_get_height (p1),
					       GDK_INTERP_BILINEAR);
	}
        else {
		tmp = g_object_ref (p2);
	}

	pixbuf_blend (tmp, result, 0, 0, -1, -1, 0, 0, alpha);

        g_object_unref (tmp);

	return result;
}

typedef	enum {
	PIXBUF,
	SLIDESHOW,
	THUMBNAIL
} FileType;

struct FileCacheEntry
{
	FileType type;
	char *filename;
	union {
		GdkPixbuf *pixbuf;
		SlideShow *slideshow;
		GdkPixbuf *thumbnail;
	} u;
};

static void
file_cache_entry_delete (FileCacheEntry *ent)
{
	g_free (ent->filename);

	switch (ent->type) {
	case PIXBUF:
		g_object_unref (ent->u.pixbuf);
		break;
	case SLIDESHOW:
		slideshow_unref (ent->u.slideshow);
		break;
	case THUMBNAIL:
		g_object_unref (ent->u.thumbnail);
		break;
	}

	g_free (ent);
}

static void
bound_cache (CafeBG *bg)
{
      while (g_list_length (bg->file_cache) >= CACHE_SIZE) {
	      GList *last_link = g_list_last (bg->file_cache);
	      FileCacheEntry *ent = last_link->data;

	      file_cache_entry_delete (ent);

	      bg->file_cache = g_list_delete_link (bg->file_cache, last_link);
      }
}

static const FileCacheEntry *
file_cache_lookup (CafeBG *bg, FileType type, const char *filename)
{
	GList *list;

	for (list = bg->file_cache; list != NULL; list = list->next) {
		FileCacheEntry *ent = list->data;

		if (ent && ent->type == type &&
		    strcmp (ent->filename, filename) == 0) {
			return ent;
		}
	}

	return NULL;
}

static FileCacheEntry *
file_cache_entry_new (CafeBG *bg,
		      FileType type,
		      const char *filename)
{
	FileCacheEntry *ent = g_new0 (FileCacheEntry, 1);

	g_assert (!file_cache_lookup (bg, type, filename));

	ent->type = type;
	ent->filename = g_strdup (filename);

	bg->file_cache = g_list_prepend (bg->file_cache, ent);

	bound_cache (bg);

	return ent;
}

static void
file_cache_add_pixbuf (CafeBG *bg,
		       const char *filename,
		       GdkPixbuf *pixbuf)
{
	FileCacheEntry *ent = file_cache_entry_new (bg, PIXBUF, filename);
	ent->u.pixbuf = g_object_ref (pixbuf);
}

static void
file_cache_add_thumbnail (CafeBG *bg,
			  const char *filename,
			  GdkPixbuf *pixbuf)
{
	FileCacheEntry *ent = file_cache_entry_new (bg, THUMBNAIL, filename);
	ent->u.thumbnail = g_object_ref (pixbuf);
}

static void
file_cache_add_slide_show (CafeBG *bg,
			   const char *filename,
			   SlideShow *show)
{
	FileCacheEntry *ent = file_cache_entry_new (bg, SLIDESHOW, filename);
	ent->u.slideshow = slideshow_ref (show);
}

static GdkPixbuf *
load_from_cache_file (CafeBG     *bg,
		      const char *filename,
		      gint        num_monitor,
		      gint        best_width,
		      gint        best_height)
{
	GdkPixbuf *pixbuf = NULL;
	gchar *cache_filename;

	cache_filename = get_wallpaper_cache_filename (filename, num_monitor, bg->placement,
							best_width, best_height);

	if (cache_file_is_valid (filename, cache_filename))
		pixbuf = gdk_pixbuf_new_from_file (cache_filename, NULL);

	g_free (cache_filename);

	return pixbuf;
}

static GdkPixbuf *
get_as_pixbuf_for_size (CafeBG    *bg,
			const char *filename,
			gint         monitor,
			gint         best_width,
			gint         best_height)
{
	const FileCacheEntry *ent;
	if ((ent = file_cache_lookup (bg, PIXBUF, filename))) {
		return g_object_ref (ent->u.pixbuf);
	} else {
		GdkPixbuf *pixbuf = NULL;
		gchar *tmp = NULL;
		GdkPixbuf *tmp_pixbuf;

		/* Try to hit local cache first if relevant */
		if (monitor != -1)
			pixbuf = load_from_cache_file (bg, filename, monitor,
							best_width, best_height);

		if (!pixbuf) {
			GdkPixbufFormat *format;

			/* If scalable choose maximum size */
			format = gdk_pixbuf_get_file_info (filename, NULL, NULL);
			if (format != NULL)
				tmp = gdk_pixbuf_format_get_name (format);

			if (g_strcmp0 (tmp, "svg") == 0 &&
			    (best_width > 0 && best_height > 0) &&
			    (bg->placement == CAFE_BG_PLACEMENT_FILL_SCREEN ||
			     bg->placement == CAFE_BG_PLACEMENT_SCALED ||
			     bg->placement == CAFE_BG_PLACEMENT_ZOOMED))
			{
				pixbuf = gdk_pixbuf_new_from_file_at_size (filename,
									   best_width,
									   best_height, NULL);
			} else {
				pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
			}

			if (tmp != NULL)
				g_free (tmp);
		}

		if (pixbuf) {
			tmp_pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);
			g_object_unref (pixbuf);
			pixbuf = tmp_pixbuf;
			file_cache_add_pixbuf (bg, filename, pixbuf);
		}

		return pixbuf;
	}
}

static SlideShow *
get_as_slideshow (CafeBG *bg, const char *filename)
{
	const FileCacheEntry *ent;
	if ((ent = file_cache_lookup (bg, SLIDESHOW, filename))) {
		return slideshow_ref (ent->u.slideshow);
	}
	else {
		SlideShow *show = read_slideshow_file (filename, NULL);

		if (show)
			file_cache_add_slide_show (bg, filename, show);

		return show;
	}
}

static GdkPixbuf *
get_as_thumbnail (CafeBG *bg, CafeDesktopThumbnailFactory *factory, const char *filename)
{
	const FileCacheEntry *ent;
	if ((ent = file_cache_lookup (bg, THUMBNAIL, filename))) {
		return g_object_ref (ent->u.thumbnail);
	}
	else {
		GdkPixbuf *thumb = create_thumbnail_for_filename (factory, filename);

		if (thumb)
			file_cache_add_thumbnail (bg, filename, thumb);

		return thumb;
	}
}

static gboolean
blow_expensive_caches (gpointer data)
{
	CafeBG *bg = data;
	GList *list;

	bg->blow_caches_id = 0;

	if (bg->file_cache) {
		for (list = bg->file_cache; list != NULL; list = list->next) {
			FileCacheEntry *ent = list->data;

			if (ent->type == PIXBUF) {
				file_cache_entry_delete (ent);
				bg->file_cache = g_list_delete_link (bg->file_cache,
								     list);
			}
		}
	}

	if (bg->pixbuf_cache) {
		g_object_unref (bg->pixbuf_cache);
		bg->pixbuf_cache = NULL;
	}

	return FALSE;
}

static void
blow_expensive_caches_in_idle (CafeBG *bg)
{
	if (bg->blow_caches_id == 0) {
		bg->blow_caches_id =
			g_idle_add (blow_expensive_caches,
				    bg);
	}
}


static gboolean
on_timeout (gpointer data)
{
	CafeBG *bg = data;

	bg->timeout_id = 0;

	queue_transitioned (bg);

	return FALSE;
}

static double
get_slide_timeout (Slide   *slide)
{
	double timeout;
	if (slide->fixed) {
		timeout = slide->duration;
	} else {
		/* Maybe the number of steps should be configurable? */

		/* In the worst case we will do a fade from 0 to 256, which mean
		 * we will never use more than 255 steps, however in most cases
		 * the first and last value are similar and users can't percieve
		 * changes in pixel values as small as 1/255th. So, lets not waste
		 * CPU cycles on transitioning to often.
		 *
		 * 64 steps is enough for each step to be just detectable in a 16bit
		 * color mode in the worst case, so we'll use this as an approximation
		 * of whats detectable.
		 */
		timeout = slide->duration / 64.0;
	}
	return timeout;
}

static void
ensure_timeout (CafeBG *bg,
		Slide   *slide)
{
	if (!bg->timeout_id) {
		double timeout = get_slide_timeout (slide);

		/* G_MAXUINT means "only one slide" */
		if (timeout < G_MAXUINT) {
			bg->timeout_id = g_timeout_add_full (
				G_PRIORITY_LOW,
				timeout * 1000, on_timeout, bg, NULL);
		}

	}
}

static time_t
get_mtime (const char *filename)
{
	time_t     mtime;

	mtime = (time_t)-1;

	if (filename) {
		GFile     *file;
		GFileInfo *info;

		file = g_file_new_for_path (filename);
		info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
					  G_FILE_QUERY_INFO_NONE, NULL, NULL);
		if (info) {
			mtime = g_file_info_get_attribute_uint64 (info,
								  G_FILE_ATTRIBUTE_TIME_MODIFIED);
			g_object_unref (info);
		}
		g_object_unref (file);
	}

	return mtime;
}

static GdkPixbuf *
scale_thumbnail (CafeBGPlacement placement,
		 const char *filename,
		 GdkPixbuf *thumb,
		 CdkScreen *screen,
		 int	    dest_width,
		 int	    dest_height)
{
	int o_width;
	int o_height;

	if (placement != CAFE_BG_PLACEMENT_TILED &&
	    placement != CAFE_BG_PLACEMENT_CENTERED) {

		/* In this case, the pixbuf will be scaled to fit the screen anyway,
		 * so just return the pixbuf here
		 */
		return g_object_ref (thumb);
	}

	if (get_thumb_annotations (thumb, &o_width, &o_height)		||
	    (filename && get_original_size (filename, &o_width, &o_height))) {

		int scr_height = HeightOfScreen (cdk_x11_screen_get_xscreen (screen));
		int scr_width = WidthOfScreen (cdk_x11_screen_get_xscreen (screen));
		int thumb_width = gdk_pixbuf_get_width (thumb);
		int thumb_height = gdk_pixbuf_get_height (thumb);
		double screen_to_dest = fit_factor (scr_width, scr_height,
						    dest_width, dest_height);
		double thumb_to_orig  = fit_factor (thumb_width, thumb_height,
						    o_width, o_height);
		double f = thumb_to_orig * screen_to_dest;
		int new_width, new_height;

		new_width = floor (thumb_width * f + 0.5);
		new_height = floor (thumb_height * f + 0.5);

		if (placement == CAFE_BG_PLACEMENT_TILED) {
			/* Heuristic to make sure tiles don't become so small that
			 * they turn into a blur.
			 *
			 * This is strictly speaking incorrect, but the resulting
			 * thumbnail gives a much better idea what the background
			 * will actually look like.
			 */

			if ((new_width < 32 || new_height < 32) &&
			    (new_width < o_width / 4 || new_height < o_height / 4)) {
				new_width = o_width / 4;
				new_height = o_height / 4;
			}
		}

		thumb = gdk_pixbuf_scale_simple (thumb, new_width, new_height,
						 GDK_INTERP_BILINEAR);
	}
	else
		g_object_ref (thumb);

	return thumb;
}

/* frame_num determines which slide to thumbnail.
 * -1 means 'current slide'.
 */
static GdkPixbuf *
create_img_thumbnail (CafeBG                      *bg,
		      CafeDesktopThumbnailFactory *factory,
		      CdkScreen                    *screen,
		      int                           dest_width,
		      int                           dest_height,
		      int                           frame_num)
{
	if (bg->filename) {
		GdkPixbuf *thumb;

		thumb = get_as_thumbnail (bg, factory, bg->filename);

		if (thumb) {
			GdkPixbuf *result;
			result = scale_thumbnail (bg->placement,
						  bg->filename,
						  thumb,
						  screen,
						  dest_width,
						  dest_height);
			g_object_unref (thumb);
			return result;
		}
		else {
			SlideShow *show = get_as_slideshow (bg, bg->filename);

			if (show) {
				double alpha;
				Slide *slide;

				if (frame_num == -1)
					slide = get_current_slide (show, &alpha);
				else
					slide = g_queue_peek_nth (show->slides, frame_num);

				if (slide->fixed) {
					GdkPixbuf *tmp;
					FileSize *fs;
					fs = find_best_size (slide->file1, dest_width, dest_height);
					tmp = get_as_thumbnail (bg, factory, fs->file);
					if (tmp) {
						thumb = scale_thumbnail (bg->placement,
									 fs->file,
									 tmp,
									 screen,
									 dest_width,
									 dest_height);
						g_object_unref (tmp);
					}
				}
				else {
					FileSize *fs1, *fs2;
					GdkPixbuf *p1, *p2;
					fs1 = find_best_size (slide->file1, dest_width, dest_height);
					p1 = get_as_thumbnail (bg, factory, fs1->file);

					fs2 = find_best_size (slide->file2, dest_width, dest_height);
					p2 = get_as_thumbnail (bg, factory, fs2->file);

					if (p1 && p2) {
						GdkPixbuf *thumb1, *thumb2;

						thumb1 = scale_thumbnail (bg->placement,
									  fs1->file,
									  p1,
									  screen,
									  dest_width,
									  dest_height);

						thumb2 = scale_thumbnail (bg->placement,
									  fs2->file,
									  p2,
									  screen,
									  dest_width,
									  dest_height);

						thumb = blend (thumb1, thumb2, alpha);

						g_object_unref (thumb1);
						g_object_unref (thumb2);
					}
					if (p1)
						g_object_unref (p1);
					if (p2)
						g_object_unref (p2);
				}

				ensure_timeout (bg, slide);

				slideshow_unref (show);
			}
		}

		return thumb;
	}

	return NULL;
}

/*
 * Find the FileSize that best matches the given size.
 * Do two passes; the first pass only considers FileSizes
 * that are larger than the given size.
 * We are looking for the image that best matches the aspect ratio.
 * When two images have the same aspect ratio, prefer the one whose
 * width is closer to the given width.
 */
static FileSize *
find_best_size (GSList *sizes, gint width, gint height)
{
	GSList *s;
	gdouble a, d, distance;
	FileSize *best = NULL;
	gint pass;

	a = width/(gdouble)height;
	distance = 10000.0;

	for (pass = 0; pass < 2; pass++) {
		for (s = sizes; s; s = s->next) {
			FileSize *size = s->data;

			if (pass == 0 && (size->width < width || size->height < height))
				continue;

			d = fabs (a - size->width/(gdouble)size->height);
			if (d < distance) {
				distance = d;
				best = size;
			}
			else if (d == distance) {
				if (abs (size->width - width) < abs (best->width - width)) {
					best = size;
				}
			}
		}

		if (best)
			break;
	}

	return best;
}

static GdkPixbuf *
get_pixbuf_for_size (CafeBG *bg,
		     gint monitor,
		     gint best_width,
		     gint best_height)
{
	guint time_until_next_change;
	gboolean hit_cache = FALSE;

	/* only hit the cache if the aspect ratio matches */
	if (bg->pixbuf_cache) {
		int width, height;
		width = gdk_pixbuf_get_width (bg->pixbuf_cache);
		height = gdk_pixbuf_get_height (bg->pixbuf_cache);
		hit_cache = 0.2 > fabs ((best_width / (double)best_height) - (width / (double)height));
		if (!hit_cache) {
			g_object_unref (bg->pixbuf_cache);
			bg->pixbuf_cache = NULL;
		}
	}

	if (!hit_cache && bg->filename) {
		bg->file_mtime = get_mtime (bg->filename);

		bg->pixbuf_cache = get_as_pixbuf_for_size (bg, bg->filename, monitor,
							   best_width, best_height);
		time_until_next_change = G_MAXUINT;
		if (!bg->pixbuf_cache) {
			SlideShow *show = get_as_slideshow (bg, bg->filename);

			if (show) {
				double alpha;
				Slide *slide;

				slideshow_ref (show);

				slide = get_current_slide (show, &alpha);
				time_until_next_change = (guint)get_slide_timeout (slide);
				if (slide->fixed) {
					FileSize *size = find_best_size (slide->file1,
									 best_width, best_height);
					bg->pixbuf_cache =
						get_as_pixbuf_for_size (bg, size->file, monitor,
									best_width, best_height);
				} else {
					FileSize *size;
					GdkPixbuf *p1, *p2;

					size = find_best_size (slide->file1,
								best_width, best_height);
					p1 = get_as_pixbuf_for_size (bg, size->file, monitor,
								     best_width, best_height);

					size = find_best_size (slide->file2,
								best_width, best_height);
					p2 = get_as_pixbuf_for_size (bg, size->file, monitor,
								     best_width, best_height);

					if (p1 && p2)
						bg->pixbuf_cache = blend (p1, p2, alpha);
					if (p1)
						g_object_unref (p1);
					if (p2)
						g_object_unref (p2);
				}

				ensure_timeout (bg, slide);

				slideshow_unref (show);
			}
		}

		/* If the next slideshow step is a long time away then
		   we blow away the expensive stuff (large pixbufs) from
		   the cache */
		if (time_until_next_change > KEEP_EXPENSIVE_CACHE_SECS)
		    blow_expensive_caches_in_idle (bg);
	}

	if (bg->pixbuf_cache)
		g_object_ref (bg->pixbuf_cache);

	return bg->pixbuf_cache;
}

static gboolean
is_different (CafeBG    *bg,
	      const char *filename)
{
	if (!filename && bg->filename) {
		return TRUE;
	}
	else if (filename && !bg->filename) {
		return TRUE;
	}
	else if (!filename && !bg->filename) {
		return FALSE;
	}
	else {
		time_t mtime = get_mtime (filename);

		if (mtime != bg->file_mtime)
			return TRUE;

		if (strcmp (filename, bg->filename) != 0)
			return TRUE;

		return FALSE;
	}
}

static void
clear_cache (CafeBG *bg)
{
	GList *list;

	if (bg->file_cache) {
		for (list = bg->file_cache; list != NULL; list = list->next) {
			FileCacheEntry *ent = list->data;

			file_cache_entry_delete (ent);
		}
		g_list_free (bg->file_cache);
		bg->file_cache = NULL;
	}

	if (bg->pixbuf_cache) {
		g_object_unref (bg->pixbuf_cache);

		bg->pixbuf_cache = NULL;
	}

	if (bg->timeout_id) {
		g_source_remove (bg->timeout_id);

		bg->timeout_id = 0;
	}
}

/* Pixbuf utilities */
static void
pixbuf_average_value (GdkPixbuf *pixbuf,
                      CdkRGBA   *result)
{
	guint64 a_total, r_total, g_total, b_total;
	guint row, column;
	int row_stride;
	const guchar *pixels, *p;
	int r, g, b;
	guint64 dividend;
	guint width, height;
	gdouble dd;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	row_stride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	/* iterate through the pixbuf, counting up each component */
	a_total = 0;
	r_total = 0;
	g_total = 0;
	b_total = 0;

	if (gdk_pixbuf_get_has_alpha (pixbuf)) {
		for (row = 0; row < height; row++) {
			p = pixels + (row * row_stride);
			for (column = 0; column < width; column++) {
				int a;

				r = *p++;
				g = *p++;
				b = *p++;
				a = *p++;

				a_total += a;
				r_total += r * a;
				g_total += g * a;
				b_total += b * a;
			}
		}
		dividend = height * width * 0xFF;
		a_total *= 0xFF;
	} else {
		for (row = 0; row < height; row++) {
			p = pixels + (row * row_stride);
			for (column = 0; column < width; column++) {
				r = *p++;
				g = *p++;
				b = *p++;

				r_total += r;
				g_total += g;
				b_total += b;
			}
		}
		dividend = height * width;
		a_total = dividend * 0xFF;
	}

	dd = dividend * 0xFF;
	result->alpha = a_total / dd;
	result->red = r_total / dd;
	result->green = g_total / dd;
	result->blue = b_total / dd;
}

static GdkPixbuf *
pixbuf_scale_to_fit (GdkPixbuf *src, int max_width, int max_height)
{
	double factor;
	int src_width, src_height;
	int new_width, new_height;

	src_width = gdk_pixbuf_get_width (src);
	src_height = gdk_pixbuf_get_height (src);

	factor = MIN (max_width  / (double) src_width, max_height / (double) src_height);

	new_width  = floor (src_width * factor + 0.5);
	new_height = floor (src_height * factor + 0.5);

	return gdk_pixbuf_scale_simple (src, new_width, new_height, GDK_INTERP_BILINEAR);
}

static GdkPixbuf *
pixbuf_scale_to_min (GdkPixbuf *src, int min_width, int min_height)
{
	double factor;
	int src_width, src_height;
	int new_width, new_height;
	GdkPixbuf *dest;

	src_width = gdk_pixbuf_get_width (src);
	src_height = gdk_pixbuf_get_height (src);

	factor = MAX (min_width / (double) src_width, min_height / (double) src_height);

	new_width = floor (src_width * factor + 0.5);
	new_height = floor (src_height * factor + 0.5);

	dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			       gdk_pixbuf_get_has_alpha (src),
			       8, min_width, min_height);
	if (!dest)
		return NULL;

	/* crop the result */
	gdk_pixbuf_scale (src, dest,
			  0, 0,
			  min_width, min_height,
			  (new_width - min_width) / -2,
			  (new_height - min_height) / -2,
			  factor,
			  factor,
			  GDK_INTERP_BILINEAR);
	return dest;
}

static guchar *
create_gradient (const CdkRGBA *primary,
		 const CdkRGBA *secondary,
		 int	         n_pixels)
{
	guchar *result = g_malloc (n_pixels * 3);
	int i;

	for (i = 0; i < n_pixels; ++i) {
		double ratio = (i + 0.5) / n_pixels;

		result[3 * i + 0] = (guchar) ((primary->red * (1 - ratio) + secondary->red * ratio) * 0x100);
		result[3 * i + 1] = (guchar) ((primary->green * (1 - ratio) + secondary->green * ratio) * 0x100);
		result[3 * i + 2] = (guchar) ((primary->blue * (1 - ratio) + secondary->blue * ratio) * 0x100);
	}

	return result;
}

static void
pixbuf_draw_gradient (GdkPixbuf    *pixbuf,
		      gboolean      horizontal,
		      CdkRGBA      *primary,
		      CdkRGBA      *secondary,
		      CdkRectangle *rect)
{
	int width;
	int height;
	int rowstride;
	guchar *dst;
	int n_channels = 3;

	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	width = rect->width;
	height = rect->height;
	dst = gdk_pixbuf_get_pixels (pixbuf) + rect->x * n_channels + rowstride * rect->y;

	if (horizontal) {
		guchar *gradient = create_gradient (primary, secondary, width);
		int copy_bytes_per_row = width * n_channels;
		int i;

		for (i = 0; i < height; i++) {
			guchar *d;
			d = dst + rowstride * i;
			memcpy (d, gradient, copy_bytes_per_row);
		}
		g_free (gradient);
	} else {
		guchar *gb, *gradient;
		int i;

		gradient = create_gradient (primary, secondary, height);
		for (i = 0; i < height; i++) {
			int j;
			guchar *d;

			d = dst + rowstride * i;
			gb = gradient + n_channels * i;
			for (j = width; j > 0; j--) {
				int k;

				for (k = 0; k < n_channels; k++) {
					*(d++) = gb[k];
				}
			}
		}

		g_free (gradient);
	}
}

static void
pixbuf_blend (GdkPixbuf *src,
	      GdkPixbuf *dest,
	      int	 src_x,
	      int	 src_y,
	      int	 src_width,
	      int        src_height,
	      int	 dest_x,
	      int	 dest_y,
	      double	 alpha)
{
	int dest_width = gdk_pixbuf_get_width (dest);
	int dest_height = gdk_pixbuf_get_height (dest);
	int offset_x = dest_x - src_x;
	int offset_y = dest_y - src_y;

	if (src_width < 0)
		src_width = gdk_pixbuf_get_width (src);

	if (src_height < 0)
		src_height = gdk_pixbuf_get_height (src);

	if (dest_x < 0)
		dest_x = 0;

	if (dest_y < 0)
		dest_y = 0;

	if (dest_x + src_width > dest_width) {
		src_width = dest_width - dest_x;
	}

	if (dest_y + src_height > dest_height) {
		src_height = dest_height - dest_y;
	}

	gdk_pixbuf_composite (src, dest,
			      dest_x, dest_y,
			      src_width, src_height,
			      offset_x, offset_y,
			      1, 1, GDK_INTERP_NEAREST,
			      alpha * 0xFF + 0.5);
}

static void
pixbuf_tile (GdkPixbuf *src, GdkPixbuf *dest)
{
	int x, y;
	int tile_width, tile_height;
	int dest_width = gdk_pixbuf_get_width (dest);
	int dest_height = gdk_pixbuf_get_height (dest);

	tile_width = gdk_pixbuf_get_width (src);
	tile_height = gdk_pixbuf_get_height (src);

	for (y = 0; y < dest_height; y += tile_height) {
		for (x = 0; x < dest_width; x += tile_width) {
			pixbuf_blend (src, dest, 0, 0,
				      tile_width, tile_height, x, y, 1.0);
		}
	}
}

static gboolean stack_is (SlideShow *parser, const char *s1, ...);

/* Parser for fading background */
static void
handle_start_element (GMarkupParseContext *context G_GNUC_UNUSED,
		      const gchar         *name,
		      const gchar        **attr_names,
		      const gchar        **attr_values,
		      gpointer             user_data,
		      GError             **err G_GNUC_UNUSED)
{
	SlideShow *parser = user_data;
	gint i;

	if (strcmp (name, "static") == 0 || strcmp (name, "transition") == 0) {
		Slide *slide = g_new0 (Slide, 1);

		if (strcmp (name, "static") == 0)
			slide->fixed = TRUE;

		g_queue_push_tail (parser->slides, slide);
	}
	else if (strcmp (name, "size") == 0) {
		Slide *slide = parser->slides->tail->data;
		FileSize *size = g_new0 (FileSize, 1);
		for (i = 0; attr_names[i]; i++) {
			if (strcmp (attr_names[i], "width") == 0)
				size->width = atoi (attr_values[i]);
			else if (strcmp (attr_names[i], "height") == 0)
				size->height = atoi (attr_values[i]);
		}
		if (parser->stack->tail &&
		    (strcmp (parser->stack->tail->data, "file") == 0 ||
		     strcmp (parser->stack->tail->data, "from") == 0)) {
			slide->file1 = g_slist_prepend (slide->file1, size);
		}
		else if (parser->stack->tail &&
			 strcmp (parser->stack->tail->data, "to") == 0) {
			slide->file2 = g_slist_prepend (slide->file2, size);
		}
		else
			g_free (size);
	}
	g_queue_push_tail (parser->stack, g_strdup (name));
}

static void
handle_end_element (GMarkupParseContext *context G_GNUC_UNUSED,
		    const gchar         *name G_GNUC_UNUSED,
		    gpointer             user_data,
		    GError             **err G_GNUC_UNUSED)
{
	SlideShow *parser = user_data;

	g_free (g_queue_pop_tail (parser->stack));
}

static gboolean
stack_is (SlideShow *parser,
	  const char *s1,
	  ...)
{
	GList *stack = NULL;
	const char *s;
	GList *l1, *l2;
	va_list args;

	stack = g_list_prepend (stack, (gpointer)s1);

	va_start (args, s1);

	s = va_arg (args, const char *);
	while (s) {
		stack = g_list_prepend (stack, (gpointer)s);
		s = va_arg (args, const char *);
	}

	va_end (args);

	l1 = stack;
	l2 = parser->stack->head;

	while (l1 && l2) {
		if (strcmp (l1->data, l2->data) != 0) {
			g_list_free (stack);
			return FALSE;
		}

		l1 = l1->next;
		l2 = l2->next;
	}

	g_list_free (stack);

	return (!l1 && !l2);
}

static int
parse_int (const char *text)
{
	return strtol (text, NULL, 0);
}

static void
handle_text (GMarkupParseContext *context G_GNUC_UNUSED,
	     const gchar         *text,
	     gsize                text_len G_GNUC_UNUSED,
	     gpointer             user_data,
	     GError             **err G_GNUC_UNUSED)
{
	SlideShow *parser = user_data;
	FileSize *fs;
	gint i;

	g_return_if_fail (parser != NULL);
	g_return_if_fail (parser->slides != NULL);

	Slide *slide = parser->slides->tail ? parser->slides->tail->data : NULL;

	if (stack_is (parser, "year", "starttime", "background", NULL)) {
		parser->start_tm.tm_year = parse_int (text) - 1900;
	}
	else if (stack_is (parser, "month", "starttime", "background", NULL)) {
		parser->start_tm.tm_mon = parse_int (text) - 1;
	}
	else if (stack_is (parser, "day", "starttime", "background", NULL)) {
		parser->start_tm.tm_mday = parse_int (text);
	}
	else if (stack_is (parser, "hour", "starttime", "background", NULL)) {
		parser->start_tm.tm_hour = parse_int (text) - 1;
	}
	else if (stack_is (parser, "minute", "starttime", "background", NULL)) {
		parser->start_tm.tm_min = parse_int (text);
	}
	else if (stack_is (parser, "second", "starttime", "background", NULL)) {
		parser->start_tm.tm_sec = parse_int (text);
	}
	else if (stack_is (parser, "duration", "static", "background", NULL) ||
		 stack_is (parser, "duration", "transition", "background", NULL)) {
		g_return_if_fail (slide != NULL);

		slide->duration = g_strtod (text, NULL);
		parser->total_duration += slide->duration;
	}
	else if (stack_is (parser, "file", "static", "background", NULL) ||
		 stack_is (parser, "from", "transition", "background", NULL)) {
		g_return_if_fail (slide != NULL);

		for (i = 0; text[i]; i++) {
			if (!g_ascii_isspace (text[i]))
				break;
		}
		if (text[i] == 0)
			return;
		fs = g_new (FileSize, 1);
		fs->width = -1;
		fs->height = -1;
		fs->file = g_strdup (text);
		slide->file1 = g_slist_prepend (slide->file1, fs);
		if (slide->file1->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
	else if (stack_is (parser, "size", "file", "static", "background", NULL) ||
		 stack_is (parser, "size", "from", "transition", "background", NULL)) {
		g_return_if_fail (slide != NULL);

		fs = slide->file1->data;
		fs->file = g_strdup (text);
		if (slide->file1->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
	else if (stack_is (parser, "to", "transition", "background", NULL)) {
		g_return_if_fail (slide != NULL);

		for (i = 0; text[i]; i++) {
			if (!g_ascii_isspace (text[i]))
				break;
		}
		if (text[i] == 0)
			return;
		fs = g_new (FileSize, 1);
		fs->width = -1;
		fs->height = -1;
		fs->file = g_strdup (text);
		slide->file2 = g_slist_prepend (slide->file2, fs);
		if (slide->file2->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
	else if (stack_is (parser, "size", "to", "transition", "background", NULL)) {
		g_return_if_fail (slide != NULL);

		fs = slide->file2->data;
		fs->file = g_strdup (text);
		if (slide->file2->next != NULL)
			parser->has_multiple_sizes = TRUE;
	}
}

static SlideShow *
slideshow_ref (SlideShow *show)
{
	show->ref_count++;
	return show;
}

static void
slideshow_unref (SlideShow *show)
{
	GList *list;
	GSList *slist;
	FileSize *size;

	show->ref_count--;
	if (show->ref_count > 0)
		return;

	for (list = show->slides->head; list != NULL; list = list->next) {
		Slide *slide = list->data;

		for (slist = slide->file1; slist != NULL; slist = slist->next) {
			size = slist->data;
			g_free (size->file);
			g_free (size);
		}
		g_slist_free (slide->file1);

		for (slist = slide->file2; slist != NULL; slist = slist->next) {
			size = slist->data;
			g_free (size->file);
			g_free (size);
		}
		g_slist_free (slide->file2);

		g_free (slide);
	}

	g_queue_free (show->slides);

	g_list_foreach (show->stack->head, (GFunc) g_free, NULL);
	g_queue_free (show->stack);

	g_free (show);
}

static void
dump_bg (SlideShow *show G_GNUC_UNUSED)
{
#if 0
	GList *list;
	GSList *slist;

	for (list = show->slides->head; list != NULL; list = list->next)
	{
		Slide *slide = list->data;

		g_print ("\nSlide: %s\n", slide->fixed? "fixed" : "transition");
		g_print ("duration: %f\n", slide->duration);
		g_print ("File1:\n");
		for (slist = slide->file1; slist != NULL; slist = slist->next) {
			FileSize *size = slist->data;
			g_print ("\t%s (%dx%d)\n",
				 size->file, size->width, size->height);
		}
		g_print ("File2:\n");
		for (slist = slide->file2; slist != NULL; slist = slist->next) {
			FileSize *size = slist->data;
			g_print ("\t%s (%dx%d)\n",
				 size->file, size->width, size->height);
		}
	}
#endif
}

static void
threadsafe_localtime (time_t time, struct tm *tm)
{
	struct tm *res;

	G_LOCK_DEFINE_STATIC (localtime_mutex);

	G_LOCK (localtime_mutex);

	res = localtime (&time);
	if (tm) {
		*tm = *res;
	}

	G_UNLOCK (localtime_mutex);
}

static SlideShow *
read_slideshow_file (const char *filename,
		     GError     **err)
{
	GMarkupParser parser = {
		handle_start_element,
		handle_end_element,
		handle_text,
		NULL, /* passthrough */
		NULL, /* error */
	};

	GFile *file;
	char *contents = NULL;
	gsize len;
	SlideShow *show = NULL;
	GMarkupParseContext *context = NULL;

	if (!filename)
		return NULL;

	file = g_file_new_for_path (filename);
	if (!g_file_load_contents (file, NULL, &contents, &len, NULL, NULL)) {
		g_object_unref (file);
		return NULL;
	}
	g_object_unref (file);

	show = g_new0 (SlideShow, 1);
	show->ref_count = 1;
	threadsafe_localtime ((time_t)0, &show->start_tm);
	show->stack = g_queue_new ();
	show->slides = g_queue_new ();

	context = g_markup_parse_context_new (&parser, 0, show, NULL);

	if (!g_markup_parse_context_parse (context, contents, len, err)) {
		slideshow_unref (show);
		show = NULL;
	}


	if (show) {
		if (!g_markup_parse_context_end_parse (context, err)) {
			slideshow_unref (show);
			show = NULL;
		}
	}

	g_markup_parse_context_free (context);

	if (show) {
		time_t t;
		int len;

		t = mktime (&show->start_tm);

		show->start_time = (double)t;

		dump_bg (show);

		len = g_queue_get_length (show->slides);

		/* no slides, that's not a slideshow */
		if (len == 0) {
			slideshow_unref (show);
			show = NULL;
		/* one slide, there's no transition */
		} else if (len == 1) {
			Slide *slide = show->slides->head->data;
			slide->duration = show->total_duration = G_MAXUINT;
		}
	}

	g_free (contents);

	return show;
}

/* Thumbnail utilities */
static GdkPixbuf *
create_thumbnail_for_filename (CafeDesktopThumbnailFactory *factory,
			       const char            *filename)
{
	char *thumb;
	time_t mtime;
	GdkPixbuf *result = NULL;
	char *uri;

	mtime = get_mtime (filename);

	if (mtime == (time_t)-1)
		return NULL;

	uri = g_filename_to_uri (filename, NULL, NULL);

	if (uri == NULL)
		return NULL;

	thumb = cafe_desktop_thumbnail_factory_lookup (factory, uri, mtime);

	if (thumb) {
		result = gdk_pixbuf_new_from_file (thumb, NULL);
		g_free (thumb);
	}
	else {
		GdkPixbuf *orig;

		orig = gdk_pixbuf_new_from_file (filename, NULL);
		if (orig) {
			int orig_width = gdk_pixbuf_get_width (orig);
			int orig_height = gdk_pixbuf_get_height (orig);

			result = pixbuf_scale_to_fit (orig, THUMBNAIL_SIZE, THUMBNAIL_SIZE);


			g_object_set_data_full (G_OBJECT (result), "cafe-thumbnail-height",
						g_strdup_printf ("%d", orig_height), g_free);
			g_object_set_data_full (G_OBJECT (result), "cafe-thumbnail-width",
						g_strdup_printf ("%d", orig_width), g_free);

			g_object_unref (orig);

			cafe_desktop_thumbnail_factory_save_thumbnail (factory, result, uri, mtime);
		}
		else {
			cafe_desktop_thumbnail_factory_create_failed_thumbnail (factory, uri, mtime);
		}
	}

	g_free (uri);

	return result;
}

static gboolean
get_thumb_annotations (GdkPixbuf *thumb,
		       int	 *orig_width,
		       int	 *orig_height)
{
	char *end;
	const char *wstr, *hstr;

	wstr = gdk_pixbuf_get_option (thumb, "tEXt::Thumb::Image::Width");
	hstr = gdk_pixbuf_get_option (thumb, "tEXt::Thumb::Image::Height");

	if (hstr && wstr) {
		*orig_width = strtol (wstr, &end, 10);
		if (*end != 0)
			return FALSE;

		*orig_height = strtol (hstr, &end, 10);
		if (*end != 0)
			return FALSE;

		return TRUE;
	}

	return FALSE;
}

static gboolean
slideshow_has_multiple_sizes (SlideShow *show)
{
	return show->has_multiple_sizes;
}

/*
 * Returns whether the background is a slideshow.
 */
gboolean
cafe_bg_changes_with_time (CafeBG *bg)
{
	SlideShow *show;

	g_return_val_if_fail (bg != NULL, FALSE);

	if (!bg->filename)
		return FALSE;

	show = get_as_slideshow (bg, bg->filename);
	if (show)
		return g_queue_get_length (show->slides) > 1;

	return FALSE;
}

/**
 * cafe_bg_create_frame_thumbnail:
 *
 * Creates a thumbnail for a certain frame, where 'frame' is somewhat
 * vaguely defined as 'suitable point to show while single-stepping
 * through the slideshow'.
 *
 * Returns: (transfer full): the newly created thumbnail or
 * or NULL if frame_num is out of bounds.
 */
GdkPixbuf *
cafe_bg_create_frame_thumbnail (CafeBG			*bg,
				 CafeDesktopThumbnailFactory	*factory,
				 CdkScreen			*screen,
				 int				 dest_width,
				 int				 dest_height,
				 int				 frame_num)
{
	SlideShow *show;
	GdkPixbuf *result;
        GList *l;
        int i, skipped;
        gboolean found;

	g_return_val_if_fail (bg != NULL, FALSE);

	show = get_as_slideshow (bg, bg->filename);

	if (!show)
		return NULL;


	if (frame_num < 0 || frame_num >= g_queue_get_length (show->slides))
		return NULL;

	i = 0;
	skipped = 0;
	found = FALSE;
	for (l = show->slides->head; l; l = l->next) {
		Slide *slide = l->data;
		if (!slide->fixed) {
			skipped++;
			continue;
		}
		if (i == frame_num) {
			found = TRUE;
			break;
		}
		i++;
	}
	if (!found)
		return NULL;


	result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, dest_width, dest_height);

	draw_color (bg, result);

	if (bg->filename) {
		GdkPixbuf *thumb;

		thumb = create_img_thumbnail (bg, factory, screen,
					      dest_width, dest_height,
					      frame_num + skipped);

		if (thumb) {
			draw_image_for_thumb (bg, thumb, result);
			g_object_unref (thumb);
		}
	}

	return result;
}

