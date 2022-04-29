/* cafe-rr.h
 *
 * Copyright 2007, 2008, Red Hat, Inc.
 *
 * This file is part of the Cafe Library.
 *
 * The Cafe Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Cafe Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Cafe Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author: Soren Sandmann <sandmann@redhat.com>
 */
#ifndef CAFE_RR_H
#define CAFE_RR_H

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error    CafeRR is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including caferr.h
#endif

#include <glib.h>
#include <cdk/cdk.h>

typedef struct CafeRRScreenPrivate CafeRRScreenPrivate;
typedef struct CafeRROutput CafeRROutput;
typedef struct CafeRRCrtc CafeRRCrtc;
typedef struct CafeRRMode CafeRRMode;

typedef struct {
    GObject parent;

    CafeRRScreenPrivate* priv;
} CafeRRScreen;

typedef struct {
    GObjectClass parent_class;

        void (* changed) (void);
} CafeRRScreenClass;

typedef enum
{
    CAFE_RR_ROTATION_0 =	(1 << 0),
    CAFE_RR_ROTATION_90 =	(1 << 1),
    CAFE_RR_ROTATION_180 =	(1 << 2),
    CAFE_RR_ROTATION_270 =	(1 << 3),
    CAFE_RR_REFLECT_X =	(1 << 4),
    CAFE_RR_REFLECT_Y =	(1 << 5)
} CafeRRRotation;

/* Error codes */

#define CAFE_RR_ERROR (cafe_rr_error_quark ())

GQuark cafe_rr_error_quark (void);

typedef enum {
    CAFE_RR_ERROR_UNKNOWN,		/* generic "fail" */
    CAFE_RR_ERROR_NO_RANDR_EXTENSION,	/* RANDR extension is not present */
    CAFE_RR_ERROR_RANDR_ERROR,		/* generic/undescribed error from the underlying XRR API */
    CAFE_RR_ERROR_BOUNDS_ERROR,	/* requested bounds of a CRTC are outside the maximum size */
    CAFE_RR_ERROR_CRTC_ASSIGNMENT,	/* could not assign CRTCs to outputs */
    CAFE_RR_ERROR_NO_MATCHING_CONFIG,	/* none of the saved configurations matched the current configuration */
} CafeRRError;

#define CAFE_RR_CONNECTOR_TYPE_PANEL "Panel"  /* This is a laptop's built-in LCD */

#define CAFE_TYPE_RR_SCREEN                  (cafe_rr_screen_get_type())
#define CAFE_RR_SCREEN(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_RR_SCREEN, CafeRRScreen))
#define CAFE_IS_RR_SCREEN(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_RR_SCREEN))
#define CAFE_RR_SCREEN_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_RR_SCREEN, CafeRRScreenClass))
#define CAFE_IS_RR_SCREEN_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_RR_SCREEN))
#define CAFE_RR_SCREEN_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_RR_SCREEN, CafeRRScreenClass))

#define CAFE_TYPE_RR_OUTPUT (cafe_rr_output_get_type())
#define CAFE_TYPE_RR_CRTC   (cafe_rr_crtc_get_type())
#define CAFE_TYPE_RR_MODE   (cafe_rr_mode_get_type())

GType cafe_rr_screen_get_type (void);
GType cafe_rr_output_get_type (void);
GType cafe_rr_crtc_get_type (void);
GType cafe_rr_mode_get_type (void);

/* CafeRRScreen */
CafeRRScreen * cafe_rr_screen_new                (CdkScreen             *screen,
						    GError               **error);
CafeRROutput **cafe_rr_screen_list_outputs       (CafeRRScreen         *screen);
CafeRRCrtc **  cafe_rr_screen_list_crtcs         (CafeRRScreen         *screen);
CafeRRMode **  cafe_rr_screen_list_modes         (CafeRRScreen         *screen);
CafeRRMode **  cafe_rr_screen_list_clone_modes   (CafeRRScreen	  *screen);
void            cafe_rr_screen_set_size           (CafeRRScreen         *screen,
						    int                    width,
						    int                    height,
						    int                    mm_width,
						    int                    mm_height);
CafeRRCrtc *   cafe_rr_screen_get_crtc_by_id     (CafeRRScreen         *screen,
						    guint32                id);
gboolean        cafe_rr_screen_refresh            (CafeRRScreen         *screen,
						    GError               **error);
CafeRROutput * cafe_rr_screen_get_output_by_id   (CafeRRScreen         *screen,
						    guint32                id);
CafeRROutput * cafe_rr_screen_get_output_by_name (CafeRRScreen         *screen,
						    const char            *name);
void            cafe_rr_screen_get_ranges         (CafeRRScreen         *screen,
						    int                   *min_width,
						    int                   *max_width,
						    int                   *min_height,
						    int                   *max_height);
void            cafe_rr_screen_get_timestamps     (CafeRRScreen         *screen,
						    guint32               *change_timestamp_ret,
						    guint32               *config_timestamp_ret);

void            cafe_rr_screen_set_primary_output (CafeRRScreen         *screen,
                                                    CafeRROutput         *output);

/* CafeRROutput */
guint32         cafe_rr_output_get_id             (CafeRROutput         *output);
const char *    cafe_rr_output_get_name           (CafeRROutput         *output);
gboolean        cafe_rr_output_is_connected       (CafeRROutput         *output);
int             cafe_rr_output_get_size_inches    (CafeRROutput         *output);
int             cafe_rr_output_get_width_mm       (CafeRROutput         *outout);
int             cafe_rr_output_get_height_mm      (CafeRROutput         *output);
const guint8 *  cafe_rr_output_get_edid_data      (CafeRROutput         *output);
CafeRRCrtc **  cafe_rr_output_get_possible_crtcs (CafeRROutput         *output);
CafeRRMode *   cafe_rr_output_get_current_mode   (CafeRROutput         *output);
CafeRRCrtc *   cafe_rr_output_get_crtc           (CafeRROutput         *output);
const char *    cafe_rr_output_get_connector_type (CafeRROutput         *output);
gboolean        cafe_rr_output_is_laptop          (CafeRROutput         *output);
void            cafe_rr_output_get_position       (CafeRROutput         *output,
						    int                   *x,
						    int                   *y);
gboolean        cafe_rr_output_can_clone          (CafeRROutput         *output,
						    CafeRROutput         *clone);
CafeRRMode **  cafe_rr_output_list_modes         (CafeRROutput         *output);
CafeRRMode *   cafe_rr_output_get_preferred_mode (CafeRROutput         *output);
gboolean        cafe_rr_output_supports_mode      (CafeRROutput         *output,
						    CafeRRMode           *mode);
gboolean        cafe_rr_output_get_is_primary     (CafeRROutput         *output);

/* CafeRRMode */
guint32         cafe_rr_mode_get_id               (CafeRRMode           *mode);
guint           cafe_rr_mode_get_width            (CafeRRMode           *mode);
guint           cafe_rr_mode_get_height           (CafeRRMode           *mode);
int             cafe_rr_mode_get_freq             (CafeRRMode           *mode);

/* CafeRRCrtc */
guint32         cafe_rr_crtc_get_id               (CafeRRCrtc           *crtc);

#ifndef CAFE_DISABLE_DEPRECATED
gboolean        cafe_rr_crtc_set_config           (CafeRRCrtc           *crtc,
						    int                    x,
						    int                    y,
						    CafeRRMode           *mode,
						    CafeRRRotation        rotation,
						    CafeRROutput        **outputs,
						    int                    n_outputs,
						    GError               **error);
#endif

gboolean        cafe_rr_crtc_set_config_with_time (CafeRRCrtc           *crtc,
						    guint32                timestamp,
						    int                    x,
						    int                    y,
						    CafeRRMode           *mode,
						    CafeRRRotation        rotation,
						    CafeRROutput        **outputs,
						    int                    n_outputs,
						    GError               **error);
gboolean        cafe_rr_crtc_can_drive_output     (CafeRRCrtc           *crtc,
						    CafeRROutput         *output);
CafeRRMode *   cafe_rr_crtc_get_current_mode     (CafeRRCrtc           *crtc);
void            cafe_rr_crtc_get_position         (CafeRRCrtc           *crtc,
						    int                   *x,
						    int                   *y);
CafeRRRotation cafe_rr_crtc_get_current_rotation (CafeRRCrtc           *crtc);
CafeRRRotation cafe_rr_crtc_get_rotations        (CafeRRCrtc           *crtc);
gboolean        cafe_rr_crtc_supports_rotation    (CafeRRCrtc           *crtc,
						    CafeRRRotation        rotation);

gboolean        cafe_rr_crtc_get_gamma            (CafeRRCrtc           *crtc,
						    int                   *size,
						    unsigned short       **red,
						    unsigned short       **green,
						    unsigned short       **blue);
void            cafe_rr_crtc_set_gamma            (CafeRRCrtc           *crtc,
						    int                    size,
						    unsigned short        *red,
						    unsigned short        *green,
						    unsigned short        *blue);
#endif /* CAFE_RR_H */
