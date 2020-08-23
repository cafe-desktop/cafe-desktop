/* cafe-rr-output-info.c
 * -*- c-basic-offset: 4 -*-
 *
 * Copyright 2010 Giovanni Campagna
 *
 * This file is part of the Cafe Desktop Library.
 *
 * The Cafe Desktop Library is free software; you can redistribute it and/or
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
 * License along with the Cafe Desktop Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define CAFE_DESKTOP_USE_UNSTABLE_API

#include <config.h>

#include "cafe-rr-config.h"

#include "edid.h"
#include "cafe-rr-private.h"

G_DEFINE_TYPE_WITH_PRIVATE (CafeRROutputInfo, cafe_rr_output_info, G_TYPE_OBJECT)

static void
cafe_rr_output_info_init (CafeRROutputInfo *self)
{
    self->priv = cafe_rr_output_info_get_instance_private (self);

    self->priv->name = NULL;
    self->priv->on = FALSE;
    self->priv->display_name = NULL;
}

static void
cafe_rr_output_info_finalize (GObject *gobject)
{
    CafeRROutputInfo *self = CAFE_RR_OUTPUT_INFO (gobject);

    g_free (self->priv->name);
    g_free (self->priv->display_name);

    G_OBJECT_CLASS (cafe_rr_output_info_parent_class)->finalize (gobject);
}

static void
cafe_rr_output_info_class_init (CafeRROutputInfoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = cafe_rr_output_info_finalize;
}

/**
 * cafe_rr_output_info_get_name:
 *
 * Returns: (transfer none): the output name
 */
char *cafe_rr_output_info_get_name (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), NULL);

    return self->priv->name;
}

/**
 * cafe_rr_output_info_is_active:
 *
 * Returns: whether there is a CRTC assigned to this output (i.e. a signal is being sent to it)
 */
gboolean cafe_rr_output_info_is_active (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), FALSE);

    return self->priv->on;
}

void cafe_rr_output_info_set_active (CafeRROutputInfo *self, gboolean active)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));

    self->priv->on = active;
}

/**
 * cafe_rr_output_info_get_geometry:
 * @self: a #CafeRROutputInfo
 * @x: (out) (allow-none):
 * @y: (out) (allow-none):
 * @width: (out) (allow-none):
 * @height: (out) (allow-none):
 */
void cafe_rr_output_info_get_geometry (CafeRROutputInfo *self, int *x, int *y, int *width, int *height)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));

    if (x)
	*x = self->priv->x;
    if (y)
	*y = self->priv->y;
    if (width)
	*width = self->priv->width;
    if (height)
	*height = self->priv->height;
}

/**
 * cafe_rr_output_info_set_geometry:
 * @self: a #CafeRROutputInfo
 * @x: x offset for monitor
 * @y: y offset for monitor
 * @width: monitor width
 * @height: monitor height
 *
 * Set the geometry for the monitor connected to the specified output.
 */
void cafe_rr_output_info_set_geometry (CafeRROutputInfo *self, int  x, int  y, int  width, int  height)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));

    self->priv->x = x;
    self->priv->y = y;
    self->priv->width = width;
    self->priv->height = height;
}

int cafe_rr_output_info_get_refresh_rate (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->rate;
}

void cafe_rr_output_info_set_refresh_rate (CafeRROutputInfo *self, int rate)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));

    self->priv->rate = rate;
}

CafeRRRotation cafe_rr_output_info_get_rotation (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), CAFE_RR_ROTATION_0);

    return self->priv->rotation;
}

void cafe_rr_output_info_set_rotation (CafeRROutputInfo *self, CafeRRRotation rotation)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));

    self->priv->rotation = rotation;
}

/**
 * cafe_rr_output_info_is_connected:
 *
 * Returns: whether the output is physically connected to a monitor
 */
gboolean cafe_rr_output_info_is_connected (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), FALSE);

    return self->priv->connected;
}

/**
 * cafe_rr_output_info_get_vendor:
 * @self: a #CafeRROutputInfo
 * @vendor: (out caller-allocates) (array fixed-size=4):
 */
void cafe_rr_output_info_get_vendor (CafeRROutputInfo *self, gchar* vendor)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));
    g_return_if_fail (vendor != NULL);

    vendor[0] = self->priv->vendor[0];
    vendor[1] = self->priv->vendor[1];
    vendor[2] = self->priv->vendor[2];
    vendor[3] = self->priv->vendor[3];
}

guint cafe_rr_output_info_get_product (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->product;
}

guint cafe_rr_output_info_get_serial (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->serial;
}

double cafe_rr_output_info_get_aspect_ratio (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->aspect;
}

/**
 * cafe_rr_output_info_get_display_name:
 *
 * Returns: (transfer none): the display name of this output
 */
char *cafe_rr_output_info_get_display_name (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), NULL);

    return self->priv->display_name;
}

gboolean cafe_rr_output_info_get_primary (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), FALSE);

    return self->priv->primary;
}

void cafe_rr_output_info_set_primary (CafeRROutputInfo *self, gboolean primary)
{
    g_return_if_fail (CAFE_IS_RR_OUTPUT_INFO (self));

    self->priv->primary = primary;
}

int cafe_rr_output_info_get_preferred_width (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->pref_width;
}

int cafe_rr_output_info_get_preferred_height (CafeRROutputInfo *self)
{
    g_return_val_if_fail (CAFE_IS_RR_OUTPUT_INFO (self), 0);

    return self->priv->pref_height;
}
