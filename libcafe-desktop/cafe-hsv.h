/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 *
 * Modified to work internally in cafe-desktop by Pablo Barciela 2019
 */

#ifndef __CAFE_HSV_H__
#define __CAFE_HSV_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CAFE_TYPE_HSV            (cafe_hsv_get_type ())
#define CAFE_HSV(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_HSV, CafeHSV))
#define CAFE_HSV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_HSV, CafeHSVClass))
#define CAFE_IS_HSV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_HSV))
#define CAFE_IS_HSV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_HSV))
#define CAFE_HSV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_HSV, CafeHSVClass))


typedef struct _CafeHSV              CafeHSV;
typedef struct _CafeHSVPrivate       CafeHSVPrivate;
typedef struct _CafeHSVClass         CafeHSVClass;

struct _CafeHSV
{
  GtkWidget parent_instance;

  /*< private >*/
  CafeHSVPrivate *priv;
};

struct _CafeHSVClass
{
  GtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (CafeHSV         *hsv);

  /* Keybindings */
  void (* move)    (CafeHSV         *hsv,
                    GtkDirectionType type);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GType      cafe_hsv_get_type     (void) G_GNUC_CONST;
GtkWidget* cafe_hsv_new          (void);
void       cafe_hsv_set_color    (CafeHSV    *hsv,
				  double      h,
				  double      s,
				  double      v);
void       cafe_hsv_get_color    (CafeHSV    *hsv,
				  gdouble    *h,
				  gdouble    *s,
				  gdouble    *v);
void       cafe_hsv_set_metrics  (CafeHSV    *hsv,
				  gint        size,
				  gint        ring_width);
void       cafe_hsv_get_metrics  (CafeHSV    *hsv,
				  gint       *size,
				  gint       *ring_width);
gboolean   cafe_hsv_is_adjusting (CafeHSV    *hsv);

G_END_DECLS

#endif /* __CAFE_HSV_H__ */

