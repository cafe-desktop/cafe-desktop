/* cafe-rr-labeler.h - Utility to label monitors to identify them
 * while they are being configured.
 *
 * Copyright 2008, Novell, Inc.
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
 * Author: Federico Mena-Quintero <federico@novell.com>
 */

#ifndef CAFE_RR_LABELER_H
#define CAFE_RR_LABELER_H

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error    CafeRR is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including caferr.h
#endif

#include "cafe-rr-config.h"

#define CAFE_TYPE_RR_LABELER            (cafe_rr_labeler_get_type ())
#define CAFE_RR_LABELER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_RR_LABELER, CafeRRLabeler))
#define CAFE_RR_LABELER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CAFE_TYPE_RR_LABELER, CafeRRLabelerClass))
#define CAFE_IS_RR_LABELER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_RR_LABELER))
#define CAFE_IS_RR_LABELER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CAFE_TYPE_RR_LABELER))
#define CAFE_RR_LABELER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CAFE_TYPE_RR_LABELER, CafeRRLabelerClass))

typedef struct _CafeRRLabeler CafeRRLabeler;
typedef struct _CafeRRLabelerClass CafeRRLabelerClass;
typedef struct _CafeRRLabelerPrivate CafeRRLabelerPrivate;

struct _CafeRRLabeler {
	GObject parent;

	/*< private >*/
	CafeRRLabelerPrivate *priv;
};

struct _CafeRRLabelerClass {
	GObjectClass parent_class;
};

GType cafe_rr_labeler_get_type (void);

CafeRRLabeler *cafe_rr_labeler_new (CafeRRConfig *config);

void cafe_rr_labeler_hide (CafeRRLabeler *labeler);

void cafe_rr_labeler_get_rgba_for_output (CafeRRLabeler *labeler, CafeRROutputInfo *output, GdkRGBA *color_out);

#endif
