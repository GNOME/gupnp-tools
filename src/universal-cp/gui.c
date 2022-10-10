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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <gmodule.h>

#include "gui.h"
#include "device-treeview.h"
#include "event-treeview.h"
#include "action-dialog.h"
#include "icons.h"
#include "main.h"

#define UI_FILE "/org/gupnp/Tools/Universal-CP/gupnp-universal-cp.ui"
#define ICON_FILE  "/org/gupnp/Tools/Universal-CP/pixmaps/universal-cp.png"

GtkBuilder *builder;
static GtkWidget *main_window;

gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data);

void
setup_treeview (GtkWidget    *treeview,
                GtkTreeModel *model,
                char         *headers[],
                int           render_index)
{
        GtkTreeSelection *selection;
        int i;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        for (i = 0; headers[i] != NULL; i++) {
                GtkCellRenderer   *renderer;
                GtkTreeViewColumn *column;

                column = gtk_tree_view_column_new ();
                renderer = gtk_cell_renderer_text_new ();
                gtk_tree_view_column_pack_end (column, renderer, FALSE);
                gtk_tree_view_column_set_title (column, headers[i]);
                gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "text", i + render_index);
                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);

                gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview),
                                             column,
                                             -1);
        }

        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 model);
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
}

static void
setup_treeviews (void)
{
        setup_details_treeview (builder);
        setup_event_treeview (builder);
        setup_device_treeview (builder);
}

G_MODULE_EXPORT
gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
        application_exit ();

        return TRUE;
}

gboolean
init_ui (gint   *argc,
         gchar **argv[])
{
        GtkWidget *about_dialog;
        GdkPixbuf *icon_pixbuf;
        GtkWidget *hpaned;
        GtkWidget *vpaned;
        gint       window_width, window_height;
        gint       position;
        GError    *error = NULL;
        double     w,h;

        gtk_init (argc, argv);

        builder = gtk_builder_new ();
        g_assert (builder != NULL);
        gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

        if (!gtk_builder_add_from_resource (builder, UI_FILE, &error)) {
                g_critical ("Unable to load the GUI file %s: %s",
                            UI_FILE,
                            error->message);

                g_error_free (error);
                return FALSE;
        }

        main_window = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "main-window"));
        about_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "about-dialog"));
        hpaned = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "main-window-hpaned"));
        vpaned = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "main-window-vpaned"));

        g_assert (main_window != NULL);
        g_assert (about_dialog != NULL);
        g_assert (hpaned != NULL);
        g_assert (vpaned != NULL);

        /* 80% of the screen but don't get bigger than 1000x800 */
        /* FIXME: Replace with proper functions */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        w = gdk_screen_width () * 0.8;
        h = gdk_screen_height () * 0.8;
        G_GNUC_END_IGNORE_DEPRECATIONS

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

        gtk_window_resize (GTK_WINDOW (main_window),
                                     window_width,
                                     window_height);


        init_icons ();

        icon_pixbuf = gdk_pixbuf_new_from_resource (ICON_FILE, NULL);
        if (icon_pixbuf == NULL) {
                return FALSE;
        }

        gtk_window_set_icon (GTK_WINDOW (main_window), icon_pixbuf);
        gtk_window_set_icon (GTK_WINDOW (about_dialog), icon_pixbuf);
        gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (about_dialog),
                                   icon_pixbuf);
        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (about_dialog),
                                      VERSION);

        g_object_unref (icon_pixbuf);

        gtk_builder_connect_signals (builder, NULL);

        setup_treeviews ();
        init_action_dialog (builder);

        gtk_widget_show_all (main_window);

        /* Give device treeview 40% of the main window */
        g_object_get (hpaned, "max-position", &position, NULL);
        position = position * 40 / 100;
        g_object_set (hpaned, "position", position, NULL);

        /* Give details treeview 60% of 60% of the main window */
        g_object_get (vpaned, "max-position", &position, NULL);
        position = position * 60 / 100;
        g_object_set (vpaned, "position", position, NULL);

        return TRUE;
}

void
deinit_ui (void)
{
        g_object_unref (builder);
        gtk_widget_destroy (main_window);
        deinit_action_dialog ();
        deinit_icons ();
}

