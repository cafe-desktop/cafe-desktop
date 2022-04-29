/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2011 Perberos <perberos@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "cafe-about.h"

/* get text macro, this should be on the common macros. or not?
 */
#ifndef cafe_gettext
#define cafe_gettext(package, locale, codeset) \
    bindtextdomain(package, locale); \
    bind_textdomain_codeset(package, codeset); \
    textdomain(package);
#endif

static void cafe_about_on_activate(GtkApplication* app)
{
    GList* list;

    list = ctk_application_get_windows(app);

    if (list)
    {
        ctk_window_present(GTK_WINDOW(list->data));
    }
    else
    {
        cafe_about_run();
    }
}

void cafe_about_run(void)
{
    cafe_about_dialog = (GtkAboutDialog*) ctk_about_dialog_new();

    ctk_window_set_default_icon_name(icon);
    ctk_about_dialog_set_logo_icon_name(cafe_about_dialog, icon);

    // name
    ctk_about_dialog_set_program_name(cafe_about_dialog, _(program_name));

    // version
    ctk_about_dialog_set_version(cafe_about_dialog, version);

    // credits and website
    ctk_about_dialog_set_copyright(cafe_about_dialog, _(copyright));
    ctk_about_dialog_set_website(cafe_about_dialog, website);

    /**
     * This generate a random message.
     * The comments index must not be more than comments_count - 1
     */
    ctk_about_dialog_set_comments(cafe_about_dialog, gettext(comments_array[g_random_int_range(0, comments_count - 1)]));

    ctk_about_dialog_set_authors(cafe_about_dialog, authors);
    ctk_about_dialog_set_artists(cafe_about_dialog, artists);
    ctk_about_dialog_set_documenters(cafe_about_dialog, documenters);
    /* Translators should localize the following string which will be
     * displayed in the about box to give credit to the translator(s). */
    ctk_about_dialog_set_translator_credits(cafe_about_dialog, _("translator-credits"));

    ctk_window_set_application(GTK_WINDOW(cafe_about_dialog), cafe_about_application);

    // start and destroy
    ctk_dialog_run((GtkDialog*) cafe_about_dialog);
    ctk_widget_destroy((GtkWidget*) cafe_about_dialog);
}

int main(int argc, char** argv)
{
    int status = 0;

    cafe_gettext(GETTEXT_PACKAGE, LOCALE_DIR, "UTF-8");

    GOptionContext* context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, command_entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, ctk_get_option_group(TRUE));
    g_option_context_parse(context, &argc, &argv, NULL);
    g_option_context_free(context);

    if (cafe_about_nogui == TRUE)
    {
        printf("%s %s\n", gettext(program_name), version);
    }
    else
    {
        ctk_init(&argc, &argv);

        cafe_about_application = ctk_application_new("org.cafe.about", 0);
        g_signal_connect(cafe_about_application, "activate", G_CALLBACK(cafe_about_on_activate), NULL);

        status = g_application_run(G_APPLICATION(cafe_about_application), argc, argv);

        g_object_unref(cafe_about_application);
    }

    return status;
}
