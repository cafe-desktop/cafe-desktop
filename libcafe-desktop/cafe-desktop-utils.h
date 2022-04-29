/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* cafe-ditem.h - Utilities for the CAFE Desktop

   Copyright (C) 1998 Tom Tromey
   All rights reserved.

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
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA  02110-1301, USA.  */
/*
  @NOTATION@
 */

#ifndef CAFE_DESKTOP_UTILS_H
#define CAFE_DESKTOP_UTILS_H

#ifndef CAFE_DESKTOP_USE_UNSTABLE_API
#error    cafe-desktop-utils is unstable API. You must define CAFE_DESKTOP_USE_UNSTABLE_API before including cafe-desktop-utils.h
#endif

#include <glib.h>
#include <cdk/cdk.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

/* prepend the terminal command to a vector */
void cafe_desktop_prepend_terminal_to_vector (int *argc, char ***argv);

/* replace cdk_spawn_command_line_on_screen, not available in CTK3 */
gboolean cafe_cdk_spawn_command_line_on_screen (GdkScreen *screen, const gchar *command, GError **error);

void
cafe_desktop_ctk_style_get_light_color (CtkStyleContext *style,
                                        CtkStateFlags    state,
                                        GdkRGBA         *color);

void
cafe_desktop_ctk_style_get_dark_color (CtkStyleContext *style,
                                       CtkStateFlags    state,
                                       GdkRGBA         *color);

G_END_DECLS

#endif /* CAFE_DESKTOP_UTILS_H */
