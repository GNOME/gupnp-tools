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

#define ITEM_CLASS_IMAGE "object.item.imageItem"
#define ITEM_CLASS_AUDIO "object.item.audioItem"
#define ITEM_CLASS_VIDEO "object.item.videoItem"
#define ITEM_CLASS_TEXT  "object.item.textItem"

typedef gboolean (* RowCompareFunc) (GtkTreeModel *model,
                                     GtkTreeIter  *iter,
                                     gpointer      user_data);

static GtkWidget *treeview;
static gboolean   expanded;

gboolean
on_playlist_treeview_button_release (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     gpointer        user_data)
{
        return FALSE;
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
                                (4,
                                 GDK_TYPE_PIXBUF,    /* Icon             */
                                 G_TYPE_STRING,      /* Title            */
                                 G_TYPE_OBJECT,      /* Primary object   */
                                 G_TYPE_OBJECT);     /* Secondary object */

        return GTK_TREE_MODEL (store);
}

static void
setup_treeview_columns (GtkWidget *treeview)

{
        GtkTreeSelection  *selection;
        GtkTreeViewColumn *column;
        GtkCellRenderer   *renderers[2];
        char              *headers[] = { "Icon", "Title"};
        char              *attribs[] = { "pixbuf", "text"};
        int                i;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        column = gtk_tree_view_column_new ();

        renderers[0] = gtk_cell_renderer_pixbuf_new ();
        renderers[1] = gtk_cell_renderer_text_new ();

        for (i = 0; i < 2; i++) {
                gtk_tree_view_column_pack_start (column, renderers[i], FALSE);
                gtk_tree_view_column_set_title (column, headers[i]);
                gtk_tree_view_column_add_attribute (column,
                                                    renderers[i],
                                                    attribs[i], i);
                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        }

        gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview),
                                     column,
                                     -1);

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
compare_media_server (GtkTreeModel *model,
                      GtkTreeIter  *iter,
                      gpointer      user_data)
{
        GUPnPDeviceInfo *info;
        const char      *udn = (const char *) user_data;
        gboolean         found = FALSE;

        if (udn == NULL)
                return found;

        gtk_tree_model_get (model, iter,
                            2, &info, -1);

        if (info) {
                if (IS_MEDIA_SERVER_PROXY (info)) {
                        const char *device_udn;

                        device_udn = gupnp_device_info_get_udn (info);

                        if (device_udn && strcmp (device_udn, udn) == 0) {
                                found = TRUE;
                        }
                }

                g_object_unref (info);
        }

        return found;
}

static gboolean
find_row (GtkTreeModel  *model,
          GtkTreeIter   *root_iter,
          GtkTreeIter   *iter,
          RowCompareFunc compare_func,
          gpointer       user_data,
          gboolean       recursive)
{
        gboolean found = FALSE;
        gboolean more = TRUE;

        more = gtk_tree_model_iter_children (model, iter, root_iter);

        while (more) {
                GtkTreeIter tmp;

                found = compare_func (model, iter, user_data);

                if (!found && recursive) {
                        found = find_row (model,
                                          iter,
                                          &tmp,
                                          compare_func,
                                          user_data,
                                          recursive);
                        if (found) {
                                *iter = tmp;
                                break;
                        }
                }

                if (found) {
                        /* Do this before the iter changes */
                        break;
                }

                more = gtk_tree_model_iter_next (model, iter);
        }

        return found;
}

static gboolean
compare_container (GtkTreeModel *model,
                   GtkTreeIter  *iter,
                   gpointer      user_data)
{
        DIDLLiteObject *object;
        const char     *id = (const char *) user_data;
        gboolean        found = FALSE;

        if (id == NULL)
                return found;

        gtk_tree_model_get (model, iter,
                            2, &object, -1);

        if (object) {
                if (IS_DIDL_LITE_OBJECT (object)) {
                        DIDLLiteObjectUPnPClass upnp_class;
                        char                   *container_id;

                        upnp_class = didl_lite_object_get_upnp_class (object);
                        container_id = didl_lite_object_get_id (object);

                        if (upnp_class ==
                            DIDL_LITE_OBJECT_UPNP_CLASS_CONTAINER &&
                            container_id &&
                            strcmp (container_id, id) == 0) {
                                found = TRUE;
                        }

                        g_free (container_id);
                }

                g_object_unref (object);
        }

        return found;
}

static GdkPixbuf *
get_item_icon (DIDLLiteObject *object)
{
        GdkPixbuf *icon;
        char      *class_name;

        class_name = didl_lite_object_get_upnp_class_name (object);
        if (G_UNLIKELY (class_name == NULL)) {
                return get_icon_by_id (ICON_FILE);
        }

        if (0 == strncmp (class_name,
                          ITEM_CLASS_IMAGE,
                          strlen (ITEM_CLASS_IMAGE))) {
                icon = get_icon_by_id (ICON_IMAGE_ITEM);
        } else if (0 == strncmp (class_name,
                                 ITEM_CLASS_AUDIO,
                                 strlen (ITEM_CLASS_AUDIO))) {
                icon = get_icon_by_id (ICON_AUDIO_ITEM);
        } else if (0 == strncmp (class_name,
                                 ITEM_CLASS_VIDEO,
                                 strlen (ITEM_CLASS_VIDEO))) {
                icon = get_icon_by_id (ICON_VIDEO_ITEM);
        } else if (0 == strncmp (class_name,
                                 ITEM_CLASS_TEXT,
                                 strlen (ITEM_CLASS_TEXT))) {
                icon = get_icon_by_id (ICON_TEXT_ITEM);
        } else {
                icon = get_icon_by_id (ICON_FILE);
        }

        g_free (class_name);

        return icon;
}

static void
append_didle_object (DIDLLiteObject         *object,
                     DIDLLiteObjectUPnPClass upnp_class,
                     MediaServerProxy       *server,
                     GtkTreeModel           *model,
                     GtkTreeIter            *server_iter)
{
        GtkTreeIter parent_iter;
        char       *parent_id;
        char       *title;
        GdkPixbuf  *icon;
        gint        position;

        title = didl_lite_object_get_title (object);
        if (title == NULL)
                return;

        parent_id = didl_lite_object_get_parent_id (object);
        if (parent_id == NULL) {
                g_free (title);
                return;
        }

        if (!find_row (model,
                       server_iter,
                       &parent_iter,
                       compare_container,
                       (gpointer) parent_id,
                       TRUE)) {
                /* Assume parent to be the root container if parentID is
                 * unknown */
                parent_iter = *server_iter;
        }

        /* FIXME: The following code assumes that container is always
        * added to treeview before the objects under it. Although this
        * is currently always true but things might change after we
        * start to support container updates.
        */
        if (upnp_class == DIDL_LITE_OBJECT_UPNP_CLASS_CONTAINER) {
                position = 0;
                icon = get_icon_by_id (ICON_CONTAINER);
        } else {
                position = -1;
                icon = get_item_icon (object);
        }

        gtk_tree_store_insert_with_values (GTK_TREE_STORE (model),
                                           NULL, &parent_iter, position,
                                           0, icon,
                                           1, title,
                                           2, object,
                                           3, server,
                                           -1);

        g_free (parent_id);
        g_free (title);
}

static void
on_didl_object_available (DIDLLiteParser *parser,
                          DIDLLiteObject *object,
                          gpointer        user_data)
{
        MediaServerProxy       *server;
        DIDLLiteObjectUPnPClass upnp_class;
        GtkTreeModel           *model;
        GtkTreeIter             server_iter;
        const char             *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        server = MEDIA_SERVER_PROXY (user_data);
        upnp_class = didl_lite_object_get_upnp_class (object);

        udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (server));

        if (find_row (model,
                       NULL,
                       &server_iter,
                       compare_media_server,
                       (gpointer) udn,
                       FALSE)) {
                append_didle_object (object,
                                     upnp_class,
                                     server,
                                     model,
                                     &server_iter);
        }

        return;
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
                DIDLLiteParser *parser;
                GtkTreeIter     device_iter;
                GList          *child;

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

                parser = media_server_proxy_get_parser (server);
                g_signal_connect (parser,
                                  "didl-object-available",
                                  G_CALLBACK (on_didl_object_available),
                                  server);
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

        if (!find_row (model,
                       NULL,
                       &iter,
                       compare_media_server,
                       (gpointer) udn,
                       FALSE)) {

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

