/*
 * Copyright (C) 2007 Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <gmodule.h>

#include "gui.h"
#include "playlist-treeview.h"
#include "renderer-combo.h"
#include "renderer-controls.h"
#include "main.h"
#include "icons.h"

#define UI_FILE DATA_DIR "/gupnp-av-cp.ui"
#define ICON_FILE  "pixmaps/av-cp.png"

static GtkBuilder  *builder;
static GtkWidget *main_window;
static GtkWidget *about_dialog;
static GtkButton *rescan_button;

gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data);

G_MODULE_EXPORT
gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
        application_exit ();

        return TRUE;
}

static void
setup_icons (GtkBuilder *object)
{
        GdkPixbuf *icon_pixbuf;
        GtkWidget *volume_min;
        GtkWidget *volume_max;

        init_icons ();

        icon_pixbuf = load_pixbuf_file (ICON_FILE);
        g_assert (icon_pixbuf != NULL);

        gtk_window_set_icon (GTK_WINDOW (main_window), icon_pixbuf);
        gtk_window_set_icon (GTK_WINDOW (about_dialog), icon_pixbuf);
        gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (about_dialog),
                                   icon_pixbuf);
        g_object_unref (icon_pixbuf);

        volume_min = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "volume-min-image"));
        g_assert (volume_min != NULL);
        volume_max = GTK_WIDGET (gtk_builder_get_object (builder,
                                                         "volume-max-image"));
        g_assert (volume_max != NULL);

        gtk_image_set_from_pixbuf (GTK_IMAGE (volume_min),
                                   get_icon_by_id (ICON_MIN_VOLUME));
        gtk_image_set_from_pixbuf (GTK_IMAGE (volume_max),
                                   get_icon_by_id (ICON_MAX_VOLUME));
}

gboolean
init_ui (void)
{
        gint window_width, window_height;
        GError *error = NULL;
        double w = 0.0;
        double h = 0.0;

        builder = gtk_builder_new ();
        g_assert (builder != NULL);
        gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

        if (!gtk_builder_add_from_file (builder, UI_FILE, &error)) {
                g_critical ("Unable to load the GUI file %s: %s",
                            UI_FILE,
                            error->message);

                g_error_free (error);
                return FALSE;
        }

        main_window = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "main-window"));
        g_assert (main_window != NULL);
        about_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "about-dialog"));
        g_assert (about_dialog != NULL);
        rescan_button = GTK_BUTTON (gtk_builder_get_object (builder,
                                                            "rescan-button"));
        g_assert (rescan_button != NULL);

        /* 40% of the screen but don't get bigger than 1000x800 */
        w = gdk_screen_width () * 0.4;
        h = gdk_screen_height () * 0.4;

        /* Keep 5/4 aspect */
        if (w/h > 1.25) {
                h = w / 1.25;
        } else {
                w = h * 1.25;
        }

        window_width = CLAMP ((int) w, 10, 1000);
        window_height = CLAMP ((int) h, 10, 800);
        gtk_window_set_default_size (GTK_WINDOW (main_window),
                                     window_width,
                                     window_height);

        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (about_dialog),
                                      VERSION);

        gtk_builder_connect_signals (builder, NULL);

        setup_icons (builder);
        setup_playlist_treeview (builder);
        setup_renderer_controls (builder);
        setup_renderer_combo (builder);

        gtk_widget_show_all (main_window);

        return TRUE;
}

GtkButton *
get_rescan_button (void)
{
        return rescan_button;
}

void
deinit_ui (void)
{
        g_object_unref (builder);
        gtk_widget_destroy (main_window);
        gtk_widget_destroy (about_dialog);
        deinit_icons ();
}

