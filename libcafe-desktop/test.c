/*
 * test.c: general tests for libcafe-desktop
 *
 * Copyright (C) 2013-2014 Stefano Karapetsas
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
#include "cafe-desktop.h"
#include "cafe-colorbutton.h"

int
main (int argc, char **argv)
{
    CtkWindow *window = NULL;
    CtkWidget *widget = NULL;

    /* initialize CTK+ */
    ctk_init (&argc, &argv);

    /* create window */
    window = CTK_WINDOW (ctk_window_new (CTK_WINDOW_TOPLEVEL));

    ctk_window_set_title (window, "CAFE Desktop Test");

    /* create a CafeColorButton */
    widget = cafe_color_button_new ();

    /* add CafeColorButton to window */
    ctk_container_add (CTK_CONTAINER (window), widget);

    /* quit signal */
    g_signal_connect (CTK_WIDGET (window), "destroy", ctk_main_quit, NULL);

    ctk_widget_show_all (CTK_WIDGET (window));

    /* start application */
    ctk_main ();
    return 0;
}
