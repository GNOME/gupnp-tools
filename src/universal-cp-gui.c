/*
 * Copyright (C) 2007 Zeeshan Ali <zeenix@gstreamer.net>
 *
 * Authors: Zeeshan Ali <zeenix@gstreamer.net>
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

#include "universal-cp-gui.h"
#include "universal-cp-devicetreeview.h"
#include "universal-cp-eventtreeview.h"
#include "universal-cp-actiondialog.h"
#include "icons.h"
#include "universal-cp.h"

#define GLADE_FILE "gupnp-universal-cp.glade"

GladeXML *glade_xml;
static GtkWidget *main_window;

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
        setup_details_treeview (glade_xml);
        setup_event_treeview (glade_xml);
        setup_device_treeview (glade_xml);
}

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
        GtkWidget *hpaned;
        GtkWidget *vpaned;
        gint       window_width, window_height;
        gchar     *glade_path = NULL;
        gint       position;

        gtk_init (argc, argv);
        glade_init ();
        g_thread_init (NULL);

        /* Try to fetch the glade file from the CWD first */
        glade_path = GLADE_FILE;
        if (!g_file_test (glade_path, G_FILE_TEST_EXISTS)) {
                /* Then Try to fetch it from the system path */
                glade_path = DATA_DIR "/" GLADE_FILE;

                if (!g_file_test (glade_path, G_FILE_TEST_EXISTS))
                        glade_path = NULL;
        }

        if (glade_path == NULL) {
                g_critical ("Unable to load the GUI file %s", GLADE_FILE);
                return FALSE;
        }

        glade_xml = glade_xml_new (glade_path, NULL, NULL);
        if (glade_xml == NULL)
                return FALSE;

        main_window = glade_xml_get_widget (glade_xml, "main-window");
        about_dialog = glade_xml_get_widget (glade_xml, "about-dialog");
        hpaned = glade_xml_get_widget (glade_xml, "main-window-hpaned");
        vpaned = glade_xml_get_widget (glade_xml, "main-window-vpaned");

        g_assert (main_window != NULL);
        g_assert (about_dialog != NULL);
        g_assert (hpaned != NULL);
        g_assert (vpaned != NULL);

        /* 80% of the screen but don't get bigger than 1000x800 */
        window_width = CLAMP ((gdk_screen_width () * 80 / 100), 10, 1000);
        window_height = CLAMP ((gdk_screen_height () * 80 / 100), 10, 800);
        gtk_window_set_default_size (GTK_WINDOW (main_window),
                                     window_width,
                                     window_height);

        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (about_dialog),
                                      VERSION);

        glade_xml_signal_autoconnect (glade_xml);

        init_icons ();
        setup_treeviews ();
        init_action_dialog (glade_xml);

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
        g_object_unref (glade_xml);
        gtk_widget_destroy (main_window);
        deinit_action_dialog ();
        deinit_icons ();
}

