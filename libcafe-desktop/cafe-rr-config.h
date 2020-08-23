/* cafe-rr-config.h
 * -*- c-basic-offset: 4 -*-
 *
 * Copyright 2007, 2008, Red Hat, Inc.
 * Copyright 2010 Giovanni Campagna
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
#ifndef CAFE_RR_CONFIG_H
#define CAFE_RR_CONFIG_H

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error   cafe-rr-config.h is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including cafe-rr-config.h
#endif

#include "cafe-rr.h"
#include <glib.h>
#include <glib-object.h>

typedef struct CafeRROutputInfoPrivate CafeRROutputInfoPrivate;
typedef struct CafeRRConfigPrivate CafeRRConfigPrivate;

typedef struct
{
    GObject parent;

    /*< private >*/
    CafeRROutputInfoPrivate *priv;
} CafeRROutputInfo;

typedef struct
{
    GObjectClass parent_class;
} CafeRROutputInfoClass;

#define CAFE_TYPE_RR_OUTPUT_INFO                  (cafe_rr_output_info_get_type())
#define CAFE_RR_OUTPUT_INFO(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_RR_OUTPUT_INFO, CafeRROutputInfo))
#define CAFE_IS_RR_OUTPUT_INFO(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_RR_OUTPUT_INFO))
#define CAFE_RR_OUTPUT_INFO_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_RR_OUTPUT_INFO, CafeRROutputInfoClass))
#define CAFE_IS_RR_OUTPUT_INFO_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_RR_OUTPUT_INFO))
#define CAFE_RR_OUTPUT_INFO_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_RR_OUTPUT_INFO, CafeRROutputInfoClass))

GType cafe_rr_output_info_get_type (void);

char *cafe_rr_output_info_get_name (CafeRROutputInfo *self);

gboolean cafe_rr_output_info_is_active  (CafeRROutputInfo *self);
void     cafe_rr_output_info_set_active (CafeRROutputInfo *self, gboolean active);


void cafe_rr_output_info_get_geometry (CafeRROutputInfo *self, int *x, int *y, int *width, int *height);
void cafe_rr_output_info_set_geometry (CafeRROutputInfo *self, int  x, int  y, int  width, int  height);

int  cafe_rr_output_info_get_refresh_rate (CafeRROutputInfo *self);
void cafe_rr_output_info_set_refresh_rate (CafeRROutputInfo *self, int rate);

CafeRRRotation cafe_rr_output_info_get_rotation (CafeRROutputInfo *self);
void            cafe_rr_output_info_set_rotation (CafeRROutputInfo *self, CafeRRRotation rotation);

gboolean cafe_rr_output_info_is_connected     (CafeRROutputInfo *self);
void     cafe_rr_output_info_get_vendor       (CafeRROutputInfo *self, gchar* vendor);
guint    cafe_rr_output_info_get_product      (CafeRROutputInfo *self);
guint    cafe_rr_output_info_get_serial       (CafeRROutputInfo *self);
double   cafe_rr_output_info_get_aspect_ratio (CafeRROutputInfo *self);
char    *cafe_rr_output_info_get_display_name (CafeRROutputInfo *self);

gboolean cafe_rr_output_info_get_primary (CafeRROutputInfo *self);
void     cafe_rr_output_info_set_primary (CafeRROutputInfo *self, gboolean primary);

int cafe_rr_output_info_get_preferred_width  (CafeRROutputInfo *self);
int cafe_rr_output_info_get_preferred_height (CafeRROutputInfo *self);

typedef struct
{
    GObject parent;

    /*< private >*/
    CafeRRConfigPrivate *priv;
} CafeRRConfig;

typedef struct
{
    GObjectClass parent_class;
} CafeRRConfigClass;

#define CAFE_TYPE_RR_CONFIG                  (cafe_rr_config_get_type())
#define CAFE_RR_CONFIG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_RR_CONFIG, CafeRRConfig))
#define CAFE_IS_RR_CONFIG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_RR_CONFIG))
#define CAFE_RR_CONFIG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_RR_CONFIG, CafeRRConfigClass))
#define CAFE_IS_RR_CONFIG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_RR_CONFIG))
#define CAFE_RR_CONFIG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_RR_CONFIG, CafeRRConfigClass))

GType               cafe_rr_config_get_type     (void);

CafeRRConfig      *cafe_rr_config_new_current  (CafeRRScreen  *screen,
						  GError        **error);
CafeRRConfig      *cafe_rr_config_new_stored   (CafeRRScreen  *screen,
						  GError        **error);
gboolean                cafe_rr_config_load_current (CafeRRConfig  *self,
						      GError        **error);
gboolean                cafe_rr_config_load_filename (CafeRRConfig  *self,
						       const gchar    *filename,
						       GError        **error);
gboolean            cafe_rr_config_match        (CafeRRConfig  *config1,
						  CafeRRConfig  *config2);
gboolean            cafe_rr_config_equal	 (CafeRRConfig  *config1,
						  CafeRRConfig  *config2);
gboolean            cafe_rr_config_save         (CafeRRConfig  *configuration,
						  GError        **error);
void                cafe_rr_config_sanitize     (CafeRRConfig  *configuration);
gboolean            cafe_rr_config_ensure_primary (CafeRRConfig  *configuration);

gboolean	    cafe_rr_config_apply_with_time (CafeRRConfig  *configuration,
						     CafeRRScreen  *screen,
						     guint32         timestamp,
						     GError        **error);

gboolean            cafe_rr_config_apply_from_filename_with_time (CafeRRScreen  *screen,
								   const char     *filename,
								   guint32         timestamp,
								   GError        **error);

gboolean            cafe_rr_config_applicable   (CafeRRConfig  *configuration,
						  CafeRRScreen  *screen,
						  GError        **error);

gboolean            cafe_rr_config_get_clone    (CafeRRConfig  *configuration);
void                cafe_rr_config_set_clone    (CafeRRConfig  *configuration, gboolean clone);
CafeRROutputInfo **cafe_rr_config_get_outputs  (CafeRRConfig  *configuration);

char *cafe_rr_config_get_backup_filename (void);
char *cafe_rr_config_get_intended_filename (void);

#endif
