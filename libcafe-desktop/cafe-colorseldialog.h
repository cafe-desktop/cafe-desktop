/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CAFE_COLOR_SELECTION_DIALOG_H__
#define __CAFE_COLOR_SELECTION_DIALOG_H__

#include <glib.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CAFE_TYPE_COLOR_SELECTION_DIALOG            (cafe_color_selection_dialog_get_type ())
#define CAFE_COLOR_SELECTION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFE_TYPE_COLOR_SELECTION_DIALOG, CafeColorSelectionDialog))
#define CAFE_COLOR_SELECTION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CAFE_TYPE_COLOR_SELECTION_DIALOG, CafeColorSelectionDialogClass))
#define CAFE_IS_COLOR_SELECTION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFE_TYPE_COLOR_SELECTION_DIALOG))
#define CAFE_IS_COLOR_SELECTION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CAFE_TYPE_COLOR_SELECTION_DIALOG))
#define CAFE_COLOR_SELECTION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CAFE_TYPE_COLOR_SELECTION_DIALOG, CafeColorSelectionDialogClass))


typedef struct _CafeColorSelectionDialog       CafeColorSelectionDialog;
typedef struct _CafeColorSelectionDialogClass  CafeColorSelectionDialogClass;


struct _CafeColorSelectionDialog
{
  GtkDialog parent_instance;

  GtkWidget *colorsel;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkWidget *help_button;
};

struct _CafeColorSelectionDialogClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/* ColorSelectionDialog */
GType      cafe_color_selection_dialog_get_type            (void) G_GNUC_CONST;
GtkWidget* cafe_color_selection_dialog_new                 (const gchar *title);
GtkWidget* cafe_color_selection_dialog_get_color_selection (CafeColorSelectionDialog *colorsel);


G_END_DECLS

#endif /* __CAFE_COLOR_SELECTION_DIALOG_H__ */
