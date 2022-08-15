/* cafe-bg.h -

   Copyright (C) 2007 Red Hat, Inc.
   Copyright (C) 2012 Jasmine Hassan <jasmine.aura@gmail.com>

   This file is part of the Cafe Library.

   The Cafe Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Cafe Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Cafe Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA  02110-1301, USA.

   Authors: Soren Sandmann <sandmann@redhat.com>
	    Jasmine Hassan <jasmine.aura@gmail.com>
*/

#ifndef __CAFE_BG_H__
#define __CAFE_BG_H__

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error    CafeBG is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including cafe-bg.h
#endif

#include <glib.h>
#include <cdk/cdk.h>
#include <gio/gio.h>
#include "cafe-desktop-thumbnail.h"
#include "cafe-bg-crossfade.h"

G_BEGIN_DECLS

#define CAFE_TYPE_BG            (cafe_bg_get_type ())
#define CAFE_BG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_BG, CafeBG))
#define CAFE_BG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CAFE_TYPE_BG, CafeBGClass))
#define CAFE_IS_BG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_BG))
#define CAFE_IS_BG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CAFE_TYPE_BG))
#define CAFE_BG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CAFE_TYPE_BG, CafeBGClass))

#define CAFE_BG_SCHEMA "org.cafe.background"

/* whether to draw the desktop bg */
#define CAFE_BG_KEY_DRAW_BACKGROUND	"draw-background"

/* whether Baul or cafe-settings-daemon draw the desktop */
#define CAFE_BG_KEY_SHOW_DESKTOP	"show-desktop-icons"

/* whether to fade when changing background (By Baul/m-s-d) */
#define CAFE_BG_KEY_BACKGROUND_FADE	"background-fade"

#define CAFE_BG_KEY_PRIMARY_COLOR	"primary-color"
#define CAFE_BG_KEY_SECONDARY_COLOR	"secondary-color"
#define CAFE_BG_KEY_COLOR_TYPE		"color-shading-type"
#define CAFE_BG_KEY_PICTURE_PLACEMENT	"picture-options"
#define CAFE_BG_KEY_PICTURE_OPACITY	"picture-opacity"
#define CAFE_BG_KEY_PICTURE_FILENAME	"picture-filename"

typedef struct _CafeBG CafeBG;
typedef struct _CafeBGClass CafeBGClass;

typedef enum {
	CAFE_BG_COLOR_SOLID,
	CAFE_BG_COLOR_H_GRADIENT,
	CAFE_BG_COLOR_V_GRADIENT
} CafeBGColorType;

typedef enum {
	CAFE_BG_PLACEMENT_TILED,
	CAFE_BG_PLACEMENT_ZOOMED,
	CAFE_BG_PLACEMENT_CENTERED,
	CAFE_BG_PLACEMENT_SCALED,
	CAFE_BG_PLACEMENT_FILL_SCREEN,
	CAFE_BG_PLACEMENT_SPANNED
} CafeBGPlacement;

GType            cafe_bg_get_type              (void);
CafeBG *         cafe_bg_new                   (void);
void             cafe_bg_load_from_preferences (CafeBG               *bg);
void             cafe_bg_load_from_system_preferences  (CafeBG       *bg);
void             cafe_bg_load_from_system_gsettings    (CafeBG       *bg,
							GSettings    *settings,
							gboolean      reset_apply);
void             cafe_bg_load_from_gsettings   (CafeBG               *bg,
						GSettings            *settings);
void             cafe_bg_save_to_preferences   (CafeBG               *bg);
void             cafe_bg_save_to_gsettings     (CafeBG               *bg,
						GSettings            *settings);

/* Setters */
void             cafe_bg_set_filename          (CafeBG               *bg,
						 const char            *filename);
void             cafe_bg_set_placement         (CafeBG               *bg,
						 CafeBGPlacement       placement);
void             cafe_bg_set_color             (CafeBG               *bg,
						 CafeBGColorType       type,
						 CdkRGBA              *primary,
						 CdkRGBA              *secondary);
void		 cafe_bg_set_draw_background   (CafeBG		     *bg,
						gboolean	      draw_background);
/* Getters */
gboolean	 cafe_bg_get_draw_background   (CafeBG		     *bg);
CafeBGPlacement  cafe_bg_get_placement         (CafeBG               *bg);
void		 cafe_bg_get_color             (CafeBG               *bg,
						 CafeBGColorType      *type,
						 CdkRGBA              *primary,
						 CdkRGBA              *secondary);
const gchar *    cafe_bg_get_filename          (CafeBG               *bg);

/* Drawing and thumbnailing */
void             cafe_bg_draw                  (CafeBG               *bg,
						 GdkPixbuf             *dest,
						 CdkScreen	       *screen,
                                                 gboolean               is_root);

cairo_surface_t *cafe_bg_create_surface        (CafeBG               *bg,
						CdkWindow            *window,
						int                   width,
						int                   height,
						gboolean              root);

cairo_surface_t *cafe_bg_create_surface_scale  (CafeBG               *bg,
						CdkWindow            *window,
						int                   width,
						int                   height,
						int                   scale,
						gboolean              root);

gboolean         cafe_bg_get_image_size        (CafeBG               *bg,
						 CafeDesktopThumbnailFactory *factory,
                                                 int                    best_width,
                                                 int                    best_height,
						 int                   *width,
						 int                   *height);
GdkPixbuf *      cafe_bg_create_thumbnail      (CafeBG               *bg,
						 CafeDesktopThumbnailFactory *factory,
						 CdkScreen             *screen,
						 int                    dest_width,
						 int                    dest_height);
gboolean         cafe_bg_is_dark               (CafeBG               *bg,
                                                 int                    dest_width,
						 int                    dest_height);
gboolean         cafe_bg_has_multiple_sizes    (CafeBG               *bg);
gboolean         cafe_bg_changes_with_time     (CafeBG               *bg);
GdkPixbuf *      cafe_bg_create_frame_thumbnail (CafeBG              *bg,
						 CafeDesktopThumbnailFactory *factory,
						 CdkScreen             *screen,
						 int                    dest_width,
						 int                    dest_height,
						 int                    frame_num);

/* Set a surface as root - not a CafeBG method. At some point
 * if we decide to stabilize the API then we may want to make
 * these object methods, drop cafe_bg_create_surface, etc.
 */
void             cafe_bg_set_surface_as_root   (CdkScreen            *screen,
						cairo_surface_t    *surface);
CafeBGCrossfade *cafe_bg_set_surface_as_root_with_crossfade (CdkScreen       *screen,
							     cairo_surface_t *surface);
cairo_surface_t *cafe_bg_get_surface_from_root (CdkScreen *screen);

G_END_DECLS

#endif
