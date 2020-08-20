/* cafe-rr-config.h
 * -*- c-basic-offset: 4 -*-
 *
 * Copyright 2007, 2008, Red Hat, Inc.
 * Copyright 2010 Giovanni Campagna
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author: Soren Sandmann <sandmann@redhat.com>
 */
#ifndef CAFE_RR_CONFIG_H
#define CAFE_RR_CONFIG_H

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error   cafe-rr-config.h is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including cafe-rr-config.h
#endif

#include "cafe-rr.h"
#include <glib.h>
#include <glib-object.h>

typedef struct MateRROutputInfoPrivate MateRROutputInfoPrivate;
typedef struct MateRRConfigPrivate MateRRConfigPrivate;

typedef struct
{
    GObject parent;

    /*< private >*/
    MateRROutputInfoPrivate *priv;
} MateRROutputInfo;

typedef struct
{
    GObjectClass parent_class;
} MateRROutputInfoClass;

#define CAFE_TYPE_RR_OUTPUT_INFO                  (cafe_rr_output_info_get_type())
#define CAFE_RR_OUTPUT_INFO(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_RR_OUTPUT_INFO, MateRROutputInfo))
#define CAFE_IS_RR_OUTPUT_INFO(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_RR_OUTPUT_INFO))
#define CAFE_RR_OUTPUT_INFO_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_RR_OUTPUT_INFO, MateRROutputInfoClass))
#define CAFE_IS_RR_OUTPUT_INFO_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_RR_OUTPUT_INFO))
#define CAFE_RR_OUTPUT_INFO_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_RR_OUTPUT_INFO, MateRROutputInfoClass))

GType cafe_rr_output_info_get_type (void);

char *cafe_rr_output_info_get_name (MateRROutputInfo *self);

gboolean cafe_rr_output_info_is_active  (MateRROutputInfo *self);
void     cafe_rr_output_info_set_active (MateRROutputInfo *self, gboolean active);


void cafe_rr_output_info_get_geometry (MateRROutputInfo *self, int *x, int *y, int *width, int *height);
void cafe_rr_output_info_set_geometry (MateRROutputInfo *self, int  x, int  y, int  width, int  height);

int  cafe_rr_output_info_get_refresh_rate (MateRROutputInfo *self);
void cafe_rr_output_info_set_refresh_rate (MateRROutputInfo *self, int rate);

MateRRRotation cafe_rr_output_info_get_rotation (MateRROutputInfo *self);
void            cafe_rr_output_info_set_rotation (MateRROutputInfo *self, MateRRRotation rotation);

gboolean cafe_rr_output_info_is_connected     (MateRROutputInfo *self);
void     cafe_rr_output_info_get_vendor       (MateRROutputInfo *self, gchar* vendor);
guint    cafe_rr_output_info_get_product      (MateRROutputInfo *self);
guint    cafe_rr_output_info_get_serial       (MateRROutputInfo *self);
double   cafe_rr_output_info_get_aspect_ratio (MateRROutputInfo *self);
char    *cafe_rr_output_info_get_display_name (MateRROutputInfo *self);

gboolean cafe_rr_output_info_get_primary (MateRROutputInfo *self);
void     cafe_rr_output_info_set_primary (MateRROutputInfo *self, gboolean primary);

int cafe_rr_output_info_get_preferred_width  (MateRROutputInfo *self);
int cafe_rr_output_info_get_preferred_height (MateRROutputInfo *self);

typedef struct
{
    GObject parent;

    /*< private >*/
    MateRRConfigPrivate *priv;
} MateRRConfig;

typedef struct
{
    GObjectClass parent_class;
} MateRRConfigClass;

#define CAFE_TYPE_RR_CONFIG                  (cafe_rr_config_get_type())
#define CAFE_RR_CONFIG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_RR_CONFIG, MateRRConfig))
#define CAFE_IS_RR_CONFIG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_RR_CONFIG))
#define CAFE_RR_CONFIG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_RR_CONFIG, MateRRConfigClass))
#define CAFE_IS_RR_CONFIG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_RR_CONFIG))
#define CAFE_RR_CONFIG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_RR_CONFIG, MateRRConfigClass))

GType               cafe_rr_config_get_type     (void);

MateRRConfig      *cafe_rr_config_new_current  (MateRRScreen  *screen,
						  GError        **error);
MateRRConfig      *cafe_rr_config_new_stored   (MateRRScreen  *screen,
						  GError        **error);
gboolean                cafe_rr_config_load_current (MateRRConfig  *self,
						      GError        **error);
gboolean                cafe_rr_config_load_filename (MateRRConfig  *self,
						       const gchar    *filename,
						       GError        **error);
gboolean            cafe_rr_config_match        (MateRRConfig  *config1,
						  MateRRConfig  *config2);
gboolean            cafe_rr_config_equal	 (MateRRConfig  *config1,
						  MateRRConfig  *config2);
gboolean            cafe_rr_config_save         (MateRRConfig  *configuration,
						  GError        **error);
void                cafe_rr_config_sanitize     (MateRRConfig  *configuration);
gboolean            cafe_rr_config_ensure_primary (MateRRConfig  *configuration);

gboolean	    cafe_rr_config_apply_with_time (MateRRConfig  *configuration,
						     MateRRScreen  *screen,
						     guint32         timestamp,
						     GError        **error);

gboolean            cafe_rr_config_apply_from_filename_with_time (MateRRScreen  *screen,
								   const char     *filename,
								   guint32         timestamp,
								   GError        **error);

gboolean            cafe_rr_config_applicable   (MateRRConfig  *configuration,
						  MateRRScreen  *screen,
						  GError        **error);

gboolean            cafe_rr_config_get_clone    (MateRRConfig  *configuration);
void                cafe_rr_config_set_clone    (MateRRConfig  *configuration, gboolean clone);
MateRROutputInfo **cafe_rr_config_get_outputs  (MateRRConfig  *configuration);

char *cafe_rr_config_get_backup_filename (void);
char *cafe_rr_config_get_intended_filename (void);

#endif
