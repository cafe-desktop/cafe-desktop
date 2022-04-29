/*
 * cafe-color.c: CAFE color selection tool
 *
 * Copyright (C) 2014 Stefano Karapetsas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors:
 *  Stefano Karapetsas <stefano@karapetsas.com>
 */

#include <config.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>
#include <libcafe-desktop/cafe-colorseldialog.h>
#include <libcafe-desktop/cafe-colorsel.h>

#define cafe_gettext(package, locale, codeset) \
    bindtextdomain(package, locale); \
    bind_textdomain_codeset(package, codeset); \
    textdomain(package);

gboolean
copy_color (GtkWidget *widget, GdkEvent  *event, CafeColorSelectionDialog *color_dialog)
{
    GdkColor color;
    gchar *color_string;

    cafe_color_selection_get_current_color (CAFE_COLOR_SELECTION (color_dialog->colorsel), &color);
    g_object_get (color_dialog->colorsel, "hex-string", &color_string, NULL);

    ctk_clipboard_set_text (ctk_clipboard_get (GDK_SELECTION_CLIPBOARD), color_string, -1);

    g_free (color_string);
    return 0;
}

int
main (int argc, char **argv)
{
    GtkWidget *color_dialog = NULL;
    GtkWidget *color_selection;
    GtkWidget *widget;

    cafe_gettext (GETTEXT_PACKAGE, LOCALE_DIR, "UTF-8");

    /* initialize GTK+ */
    ctk_init (&argc, &argv);
    ctk_window_set_default_icon_name ("ctk-select-color");

    color_dialog = cafe_color_selection_dialog_new (_("CAFE Color Selection"));
    color_selection = CAFE_COLOR_SELECTION_DIALOG (color_dialog)->colorsel;
    cafe_color_selection_set_has_palette (CAFE_COLOR_SELECTION (color_selection), TRUE);

    /* quit signal */
    g_signal_connect (color_dialog, "destroy", ctk_main_quit, NULL);

    widget = ctk_button_new_from_stock (GTK_STOCK_COPY);
    ctk_container_add (GTK_CONTAINER (ctk_dialog_get_action_area (GTK_DIALOG (color_dialog))), widget);
    g_signal_connect (widget, "button-release-event", G_CALLBACK (copy_color), color_dialog);

    widget = ctk_button_new_from_stock (GTK_STOCK_CLOSE);
    ctk_container_add (GTK_CONTAINER (ctk_dialog_get_action_area (GTK_DIALOG (color_dialog))), widget);
    g_signal_connect (widget, "button-release-event", ctk_main_quit, NULL);

    ctk_widget_show_all (color_dialog);
    ctk_widget_hide (CAFE_COLOR_SELECTION_DIALOG (color_dialog)->ok_button);
    ctk_widget_hide (CAFE_COLOR_SELECTION_DIALOG (color_dialog)->cancel_button);
    ctk_widget_hide (CAFE_COLOR_SELECTION_DIALOG (color_dialog)->help_button);

    /* start application */
    ctk_main ();
    return 0;
}
