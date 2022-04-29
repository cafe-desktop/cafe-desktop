/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */
#include "config.h"
#include "private.h"
#include <string.h>
#include <glib.h>
#include <ctk/ctk.h>
#include <glib/gi18n-lib.h>
#include "cafe-colorsel.h"
#include "cafe-colorseldialog.h"

enum {
  PROP_0,
  PROP_COLOR_SELECTION,
  PROP_OK_BUTTON,
  PROP_CANCEL_BUTTON,
  PROP_HELP_BUTTON
};


/***************************/
/* CafeColorSelectionDialog */
/***************************/

static void cafe_color_selection_dialog_buildable_interface_init     (CtkBuildableIface *iface);
static GObject * cafe_color_selection_dialog_buildable_get_internal_child (CtkBuildable *buildable,
									  CtkBuilder   *builder,
									  const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (CafeColorSelectionDialog, cafe_color_selection_dialog,
           CTK_TYPE_DIALOG,
           G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                      cafe_color_selection_dialog_buildable_interface_init))

static CtkBuildableIface *parent_buildable_iface;

static void
cafe_color_selection_dialog_get_property (GObject         *object,
					 guint            prop_id,
					 GValue          *value,
					 GParamSpec      *pspec)
{
  CafeColorSelectionDialog *colorsel;

  colorsel = CAFE_COLOR_SELECTION_DIALOG (object);

  switch (prop_id)
    {
    case PROP_COLOR_SELECTION:
      g_value_set_object (value, colorsel->colorsel);
      break;
    case PROP_OK_BUTTON:
      g_value_set_object (value, colorsel->ok_button);
      break;
    case PROP_CANCEL_BUTTON:
      g_value_set_object (value, colorsel->cancel_button);
      break;
    case PROP_HELP_BUTTON:
      g_value_set_object (value, colorsel->help_button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cafe_color_selection_dialog_class_init (CafeColorSelectionDialogClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = cafe_color_selection_dialog_get_property;

  g_object_class_install_property (gobject_class,
				   PROP_COLOR_SELECTION,
				   g_param_spec_object ("color-selection",
						     _("Color Selection"),
						     _("The color selection embedded in the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_OK_BUTTON,
				   g_param_spec_object ("ok-button",
						     _("OK Button"),
						     _("The OK button of the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_CANCEL_BUTTON,
				   g_param_spec_object ("cancel-button",
						     _("Cancel Button"),
						     _("The cancel button of the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_HELP_BUTTON,
				   g_param_spec_object ("help-button",
						     _("Help Button"),
						     _("The help button of the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
}

static void
cafe_color_selection_dialog_init (CafeColorSelectionDialog *colorseldiag)
{
  CtkDialog *dialog = CTK_DIALOG (colorseldiag);

  _cafe_desktop_init_i18n ();

  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (dialog)), 2); /* 2 * 5 + 2 = 12 */
  ctk_container_set_border_width (CTK_CONTAINER (ctk_dialog_get_action_area (dialog)), 5);
  ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_action_area (dialog)), 6);

  colorseldiag->colorsel = cafe_color_selection_new ();
  ctk_container_set_border_width (CTK_CONTAINER (colorseldiag->colorsel), 5);
  cafe_color_selection_set_has_palette (CAFE_COLOR_SELECTION(colorseldiag->colorsel), FALSE);
  cafe_color_selection_set_has_opacity_control (CAFE_COLOR_SELECTION(colorseldiag->colorsel), FALSE);
  ctk_container_add (CTK_CONTAINER (ctk_dialog_get_content_area (CTK_DIALOG (colorseldiag))), colorseldiag->colorsel);
  ctk_widget_show (colorseldiag->colorsel);

  colorseldiag->cancel_button = ctk_dialog_add_button (CTK_DIALOG (colorseldiag),
                                                       CTK_STOCK_CANCEL,
                                                       CTK_RESPONSE_CANCEL);

  colorseldiag->ok_button = ctk_dialog_add_button (CTK_DIALOG (colorseldiag),
                                                   CTK_STOCK_OK,
                                                   CTK_RESPONSE_OK);

  ctk_widget_grab_default (colorseldiag->ok_button);

  colorseldiag->help_button = ctk_dialog_add_button (CTK_DIALOG (colorseldiag),
                                                     CTK_STOCK_HELP,
                                                     CTK_RESPONSE_HELP);

  ctk_widget_hide (colorseldiag->help_button);

  ctk_window_set_title (CTK_WINDOW (colorseldiag),
                        _("Color Selection"));

  //_ctk_dialog_set_ignore_separator (dialog, TRUE);
}

CtkWidget*
cafe_color_selection_dialog_new (const gchar *title)
{
  CafeColorSelectionDialog *colorseldiag;

  colorseldiag = g_object_new (CAFE_TYPE_COLOR_SELECTION_DIALOG, NULL);

  if (title)
    ctk_window_set_title (CTK_WINDOW (colorseldiag), title);

  ctk_window_set_resizable (CTK_WINDOW (colorseldiag), FALSE);

  return CTK_WIDGET (colorseldiag);
}

/**
 * cafe_color_selection_dialog_get_color_selection:
 * @colorsel: a #CafeColorSelectionDialog
 *
 * Retrieves the #CafeColorSelection widget embedded in the dialog.
 *
 * Returns: (transfer none): the embedded #CafeColorSelection
 *
 * Since: 1.9.1
 **/
CtkWidget*
cafe_color_selection_dialog_get_color_selection (CafeColorSelectionDialog *colorsel)
{
  g_return_val_if_fail (CAFE_IS_COLOR_SELECTION_DIALOG (colorsel), NULL);

  return colorsel->colorsel;
}

static void
cafe_color_selection_dialog_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = cafe_color_selection_dialog_buildable_get_internal_child;
}

static GObject *
cafe_color_selection_dialog_buildable_get_internal_child (CtkBuildable *buildable,
							 CtkBuilder   *builder,
							 const gchar  *childname)
{
    if (strcmp(childname, "ok_button") == 0)
	return G_OBJECT (CAFE_COLOR_SELECTION_DIALOG (buildable)->ok_button);
    else if (strcmp(childname, "cancel_button") == 0)
	return G_OBJECT (CAFE_COLOR_SELECTION_DIALOG (buildable)->cancel_button);
    else if (strcmp(childname, "help_button") == 0)
	return G_OBJECT (CAFE_COLOR_SELECTION_DIALOG(buildable)->help_button);
    else if (strcmp(childname, "color_selection") == 0)
	return G_OBJECT (CAFE_COLOR_SELECTION_DIALOG(buildable)->colorsel);

    return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}
