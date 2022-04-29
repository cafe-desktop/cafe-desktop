/*
 * GTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Cafe Library; see the file COPYING.LIB. If not,
 * write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the GTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CAFE_COLOR_BUTTON_H__
#define __CAFE_COLOR_BUTTON_H__

#include <glib.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS


/* The CafeColorButton widget is a simple color picker in a button.
 * The button displays a sample of the currently selected color.  When
 * the user clicks on the button, a color selection dialog pops up.
 * The color picker emits the "color_set" signal when the color is set.
 */

#define CAFE_TYPE_COLOR_BUTTON             (cafe_color_button_get_type ())
#define CAFE_COLOR_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_COLOR_BUTTON, CafeColorButton))
#define CAFE_COLOR_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_COLOR_BUTTON, CafeColorButtonClass))
#define CAFE_IS_COLOR_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_COLOR_BUTTON))
#define CAFE_IS_COLOR_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_COLOR_BUTTON))
#define CAFE_COLOR_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_COLOR_BUTTON, CafeColorButtonClass))

typedef struct _CafeColorButton          CafeColorButton;
typedef struct _CafeColorButtonClass     CafeColorButtonClass;
typedef struct _CafeColorButtonPrivate   CafeColorButtonPrivate;

struct _CafeColorButton {
  CtkButton button;

  /*< private >*/

  CafeColorButtonPrivate *priv;
};

struct _CafeColorButtonClass {
  CtkButtonClass parent_class;

  void (* color_set) (CafeColorButton *cp);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GType      cafe_color_button_get_type       (void) G_GNUC_CONST;
CtkWidget *cafe_color_button_new            (void);
CtkWidget *cafe_color_button_new_with_color (const GdkColor *color);
void       cafe_color_button_set_color      (CafeColorButton *color_button,
					    const GdkColor *color);
void       cafe_color_button_set_rgba       (CafeColorButton *color_button,
					     const GdkRGBA   *color);
void       cafe_color_button_set_alpha      (CafeColorButton *color_button,
					    guint16         alpha);
void       cafe_color_button_get_color      (CafeColorButton *color_button,
					    GdkColor       *color);
void       cafe_color_button_get_rgba       (CafeColorButton *color_button,
					     GdkRGBA         *color);
guint16    cafe_color_button_get_alpha      (CafeColorButton *color_button);
void       cafe_color_button_set_use_alpha  (CafeColorButton *color_button,
					    gboolean        use_alpha);
gboolean   cafe_color_button_get_use_alpha  (CafeColorButton *color_button);
void       cafe_color_button_set_title      (CafeColorButton *color_button,
					    const gchar    *title);
const gchar *cafe_color_button_get_title (CafeColorButton *color_button);


G_END_DECLS

#endif  /* __CAFE_COLOR_BUTTON_H__ */
