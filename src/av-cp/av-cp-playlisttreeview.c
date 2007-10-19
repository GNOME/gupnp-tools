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

#include "av-cp-playlisttreeview.h"
#include "icons.h"
#include "av-cp-gui.h"
#include "av-cp.h"

static GtkWidget *treeview;
static gboolean   expanded;

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

        store = gtk_tree_store_new
                                (3,
                                 GDK_TYPE_PIXBUF,    /* Icon             */
                                 G_TYPE_STRING,      /* Title            */
                                 G_TYPE_OBJECT,      /* Primary object   */
                                 G_TYPE_OBJECT);     /* Secondary object */

        return GTK_TREE_MODEL (store);
}

static void
setup_treeview_columns (GtkWidget *treeview)

{
        GtkTreeSelection *selection;
        GtkCellRenderer  *renderers[2];
        char             *headers[] = { "Icon", "Title"};
        char             *attribs[] = { "pixbuf", "text"};
        int               i;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        renderers[0] = gtk_cell_renderer_pixbuf_new ();
        renderers[1] = gtk_cell_renderer_text_new ();

        for (i = 0; i < 2; i++) {
                GtkTreeViewColumn *column;

                column = gtk_tree_view_column_new ();
                gtk_tree_view_column_pack_start (column, renderers[i], FALSE);
                gtk_tree_view_column_set_title (column, headers[i]);
                gtk_tree_view_column_add_attribute (column,
                                                    renderers[i],
                                                    attribs[i], i);
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

        treeview = glade_xml_get_widget (glade_xml, "playlist-treeview");
        g_assert (treeview != NULL);

        model = create_playlist_treemodel ();
        g_assert (model != NULL);

        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
        g_object_unref (model);

        setup_treeview_columns (treeview);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);
        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (on_item_selected),
                          NULL);

        expanded = FALSE;
}

static gboolean
find_media_server (GtkTreeModel *model,
                   const char   *udn,
                   GtkTreeIter  *iter)
{
        gboolean found = FALSE;
        gboolean more = TRUE;

        if (udn == NULL)
                return found;

        more = gtk_tree_model_iter_children (model, iter, NULL);

        while (more && !found) {
                GUPnPDeviceInfo *info;

                gtk_tree_model_get (model, iter,
                                    2, &info, -1);

                if (info) {
                        if (IS_MEDIA_SERVER_PROXY (info)) {
                                const char *device_udn;

                                device_udn = gupnp_device_info_get_udn (info);

                                if (device_udn &&
                                    strcmp (device_udn, udn) == 0)
                                        found = TRUE;
                        }

                        g_object_unref (info);
                }

                more = gtk_tree_model_iter_next (model, iter);
        }

        return found;
}

static void
append_media_server (MediaServerProxy *server,
                     GtkTreeModel     *model,
                     GtkTreeIter      *parent_iter)
{
        GUPnPDeviceInfo *info;
        char            *friendly_name;

        info = GUPNP_DEVICE_INFO (server);

        friendly_name = gupnp_device_info_get_friendly_name (info);
        if (friendly_name) {
                GtkTreeIter device_iter;
                GList      *child;

                gtk_tree_store_insert_with_values
                                (GTK_TREE_STORE (model),
                                 &device_iter, parent_iter, -1,
                                 0, get_icon_by_id (ICON_DEVICE),
                                 1, friendly_name,
                                 2, info,
                                 -1);
                g_free (friendly_name);

                /* Append the embedded devices */
                child = gupnp_device_info_list_devices (info);
                while (child) {
                        append_media_server (MEDIA_SERVER_PROXY (child->data),
                                             model,
                                             &device_iter);
                        g_object_unref (child->data);
                        child = g_list_delete_link (child, child);
                }

                /*schedule_icon_update (info);*/
                media_server_proxy_start_browsing (server, "0");
        }
}

void
add_media_server (MediaServerProxy *server)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        const char   *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (server));

        if (!find_media_server (model, udn, &iter)) {
                append_media_server (server, model, NULL);

                if (!expanded) {
                        gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));
                        expanded = TRUE;
                }
        }
}

void
remove_media_server (MediaServerProxy *server)
{
}

