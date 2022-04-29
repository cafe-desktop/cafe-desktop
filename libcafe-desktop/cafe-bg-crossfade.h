/* cafe-bg-crossfade.h - fade window background between two pixmaps

   Copyright 2008, Red Hat, Inc.

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
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
   Floor, Boston, MA 02110-1301 US.

   Author: Ray Strode <rstrode@redhat.com>
*/

#ifndef __CAFE_BG_CROSSFADE_H__
#define __CAFE_BG_CROSSFADE_H__

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error    CafeBGCrossfade is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including cafe-bg-crossfade.h
#endif

#include <glib.h>
#include <cdk/cdk.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CAFE_TYPE_BG_CROSSFADE            (cafe_bg_crossfade_get_type ())
#define CAFE_BG_CROSSFADE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_BG_CROSSFADE, CafeBGCrossfade))
#define CAFE_BG_CROSSFADE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CAFE_TYPE_BG_CROSSFADE, CafeBGCrossfadeClass))
#define CAFE_IS_BG_CROSSFADE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_BG_CROSSFADE))
#define CAFE_IS_BG_CROSSFADE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CAFE_TYPE_BG_CROSSFADE))
#define CAFE_BG_CROSSFADE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CAFE_TYPE_BG_CROSSFADE, CafeBGCrossfadeClass))

typedef struct _CafeBGCrossfadePrivate CafeBGCrossfadePrivate;
typedef struct _CafeBGCrossfade CafeBGCrossfade;
typedef struct _CafeBGCrossfadeClass CafeBGCrossfadeClass;

struct _CafeBGCrossfade
{
    GObject parent_object;

    CafeBGCrossfadePrivate *priv;
};

struct _CafeBGCrossfadeClass
{
    GObjectClass parent_class;

    void (* finished) (CafeBGCrossfade *fade, GdkWindow *window);
};

GType             cafe_bg_crossfade_get_type (void);
CafeBGCrossfade   *cafe_bg_crossfade_new (int width, int height);

gboolean          cafe_bg_crossfade_set_start_surface (CafeBGCrossfade *fade,
                                                       cairo_surface_t *surface);
gboolean          cafe_bg_crossfade_set_end_surface (CafeBGCrossfade *fade,
                                                     cairo_surface_t *surface);

void              cafe_bg_crossfade_start (CafeBGCrossfade *fade,
                                           GdkWindow        *window);
void              cafe_bg_crossfade_start_widget (CafeBGCrossfade *fade,
                                                  CtkWidget       *widget);
gboolean          cafe_bg_crossfade_is_started (CafeBGCrossfade *fade);
void              cafe_bg_crossfade_stop (CafeBGCrossfade *fade);

G_END_DECLS

#endif
