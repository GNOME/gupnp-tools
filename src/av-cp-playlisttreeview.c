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
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <config.h>

#include "av-cp-playlisttreeview.h"
#include "av-cp-gui.h"
#include "av-cp.h"

static GtkWidget *treeview;

gboolean
on_playlist_treeview_button_release (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     gpointer        user_data)
{
        return TRUE;
}

void
on_playlist_treeview_row_activate (GtkMenuItem *menuitem,
                                   gpointer     user_data)
{
}

static void
on_item_selected (GtkTreeSelection *selection,
                  gpointer          user_data)
{
}

static GtkTreeModel *
create_playlist_treemodel (void)
{
        GtkTreeStore *store;

        store = gtk_tree_store_new (4,
                                    G_TYPE_STRING,   /* Title             */
                                    G_TYPE_STRING,   /* Description       */
                                    G_TYPE_OBJECT,   /* MediaServer proxy */
                                    G_TYPE_OBJECT);  /* ContentDir proxy  */

        return GTK_TREE_MODEL (store);
}

static void
setup_treeview_text_columns (GtkWidget *treeview,
                             char      *headers[])

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
                                                    "text", i);
                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);

                gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview),
                                             column,
                                             -1);
        }

        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
}

void
setup_playlist_treeview (GladeXML *glade_xml)
{
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;
        char              *headers[] = { "Name", "Description", NULL};

        treeview = glade_xml_get_widget (glade_xml, "playlist-treeview");
        g_assert (treeview != NULL);

        model = create_playlist_treemodel ();
        g_assert (model != NULL);

        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
        g_object_unref (model);

        setup_treeview_text_columns (treeview, headers);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);
        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (on_item_selected),
                          NULL);
}

