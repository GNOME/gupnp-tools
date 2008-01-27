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

#include "playlist-treeview.h"
#include "renderer-combo.h"
#include "renderer-controls.h"
#include "icons.h"
#include "gui.h"
#include "main.h"

#define CONTENT_DIR_V1 "urn:schemas-upnp-org:service:ContentDirectory:1"
#define CONTENT_DIR_V2 "urn:schemas-upnp-org:service:ContentDirectory:2"

#define ITEM_CLASS_IMAGE "object.item.imageItem"
#define ITEM_CLASS_AUDIO "object.item.audioItem"
#define ITEM_CLASS_VIDEO "object.item.videoItem"
#define ITEM_CLASS_TEXT  "object.item.textItem"

typedef gboolean (* RowCompareFunc) (GtkTreeModel *model,
                                     GtkTreeIter  *iter,
                                     gpointer      user_data);

static GtkWidget *treeview;
static gboolean   expanded;

static GUPnPDIDLLiteParser *didl_parser;

static void
browse (GUPnPServiceProxy *content_dir, const char *container_id);

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
        PlaybackState state;

        state = get_selected_renderer_state ();

        if (state == PLAYBACK_STATE_PLAYING ||
            state == PLAYBACK_STATE_PAUSED) {
                char *id;
                char *uri;

                id = get_selected_item (&uri);

                if (id != NULL) {
                        set_av_transport_uri (id, uri, NULL);

                        g_free (uri);
                        g_free (id);
                }
       }
}

static GtkTreeModel *
create_playlist_treemodel (void)
{
        GtkTreeStore *store;

        store = gtk_tree_store_new (7,
                                    /* Icon */
                                    GDK_TYPE_PIXBUF,
                                    /* Title */
                                    G_TYPE_STRING,
                                    /* MediaServer */
                                    GUPNP_TYPE_DEVICE_PROXY,
                                    /* Content Directory */
                                    GUPNP_TYPE_SERVICE_PROXY,
                                    /* Id */
                                    G_TYPE_STRING,
                                    /* Is container? */
                                    G_TYPE_BOOLEAN,
                                    /* Resource Hashtable
                                     * key = Protocol Info
                                     * value = URI
                                     */
                                    G_TYPE_HASH_TABLE);

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

        didl_parser = gupnp_didl_lite_parser_new ();
        g_assert (didl_parser != NULL);
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
                if (GUPNP_IS_DEVICE_PROXY (info)) {
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
        const char     *id = (const char *) user_data;
        char           *container_id;
        gboolean        is_container;
        gboolean        found = FALSE;

        if (id == NULL)
                return found;

        gtk_tree_model_get (model, iter,
                            4, &container_id,
                            5, &is_container, -1);

        if (!is_container)
                return found;

        if (container_id == NULL)
                return found;

        if (strcmp (container_id, id) == 0) {
                found = TRUE;
        }

        g_free (container_id);

        return found;
}

static GdkPixbuf *
get_item_icon (xmlNode *object_node)
{
        GdkPixbuf *icon;
        char      *class_name;

        class_name = gupnp_didl_lite_object_get_upnp_class (object_node);
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

static GHashTable *
get_resource_hash (xmlNode *object_node)
{
   GHashTable *resource_hash;
   GList *resources;
   GList *iter;

   resources = gupnp_didl_lite_object_get_resources (object_node);

   if (resources == NULL)
           return NULL;

   resource_hash = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          g_free,
                                          g_free);

   for (iter = resources; iter; iter = iter->next) {
           xmlNode *res_node;
           char *uri;
           char *proto_info;

           res_node = (xmlNode *) iter->data;
           proto_info = gupnp_didl_lite_resource_get_protocol_info (res_node);
           if (proto_info == NULL)
                   continue;

           uri = gupnp_didl_lite_resource_get_contents (res_node);
           if (uri == NULL) {
                   g_free (proto_info);
                   continue;
           }

           g_hash_table_insert (resource_hash, proto_info, uri);
   }

   if (g_hash_table_size (resource_hash) == 0) {
           /* No point in keeping empty hash tables here */
           g_hash_table_destroy (resource_hash);
           resource_hash = NULL;
   }

   return resource_hash;
}

static void
append_didle_object (xmlNode           *object_node,
                     GUPnPServiceProxy *content_dir,
                     GtkTreeModel      *model,
                     GtkTreeIter       *server_iter)
{
        GtkTreeIter parent_iter;
        char       *id;
        char       *parent_id;
        char       *title;
        gboolean    is_container;
        GdkPixbuf  *icon;
        GHashTable *resource_hash;
        gint        position;

        id = gupnp_didl_lite_object_get_id (object_node);
        if (id == NULL)
                return;

        title = gupnp_didl_lite_object_get_title (object_node);
        if (title == NULL) {
                g_free (id);
                return;
        }

        parent_id = gupnp_didl_lite_object_get_parent_id (object_node);
        if (parent_id == NULL) {
                g_free (id);
                g_free (title);
                return;
        }

        is_container = gupnp_didl_lite_object_is_container (object_node);

        /* FIXME: The following code assumes that container is always
        * added to treeview before the objects under it. Although this
        * is currently always true but things might change after we
        * start to support container updates.
        */
        if (is_container) {
                position = 0;
                resource_hash = NULL;
                icon = get_icon_by_id (ICON_CONTAINER);

                /* Recurse */
                browse (content_dir, id);
        } else {
                position = -1;
                resource_hash = get_resource_hash (object_node);
                if (resource_hash == NULL) {
                        g_free (id);
                        g_free (title);
                        g_free (parent_id);
                        return;
                }

                icon = get_item_icon (object_node);
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

        gtk_tree_store_insert_with_values (GTK_TREE_STORE (model),
                                           NULL, &parent_iter, position,
                                           0, icon,
                                           1, title,
                                           3, content_dir,
                                           4, id,
                                           5, is_container,
                                           6, resource_hash,
                                           -1);

        g_free (parent_id);
        g_free (title);
        g_free (id);
}

static void
on_device_icon_available (GUPnPDeviceInfo *info,
                          GdkPixbuf       *icon)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        const char   *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (find_row (model,
                      NULL,
                      &iter,
                      compare_media_server,
                      (gpointer) udn,
                      FALSE)) {
                gtk_tree_store_set (GTK_TREE_STORE (model),
                                    &iter,
                                    0, icon,
                                    -1);
        }

        g_object_unref (icon);
}

static GUPnPServiceProxy *
get_content_dir (GUPnPDeviceProxy *proxy)
{
        GUPnPDeviceInfo  *info;
        GUPnPServiceInfo *content_dir;

        info = GUPNP_DEVICE_INFO (proxy);

        /* Content Directory */
        content_dir = gupnp_device_info_get_service (info,
                                                     CONTENT_DIR_V1);
        if (content_dir == NULL) {
                        content_dir = gupnp_device_info_get_service
                                                      (info,
                                                       CONTENT_DIR_V2);
        }

        return GUPNP_SERVICE_PROXY (content_dir);
}

static void
on_didl_object_available (GUPnPDIDLLiteParser *didl_parser,
                          xmlNode             *object_node,
                          gpointer             user_data)
{
        GUPnPServiceInfo *content_dir;
        GtkTreeModel     *model;
        GtkTreeIter       server_iter;
        const char       *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        content_dir = GUPNP_SERVICE_INFO (user_data);

        udn = gupnp_service_info_get_udn (content_dir);

        if (find_row (model,
                       NULL,
                       &server_iter,
                       compare_media_server,
                       (gpointer) udn,
                       FALSE)) {
                append_didle_object (object_node,
                                     GUPNP_SERVICE_PROXY (content_dir),
                                     model,
                                     &server_iter);
        }

        return;
}

static inline void
on_browse_failure (GUPnPServiceInfo *info,
                   GError           *error)
{
        g_warning ("Failed to browse '%s': %s\n",
                   gupnp_service_info_get_location (info),
                   error->message);
        g_error_free (error);
}

static void
browse_cb (GUPnPServiceProxy       *content_dir,
           GUPnPServiceProxyAction *action,
           gpointer                 user_data)
{
        char   *didl_xml;
        GError *error;

        error = NULL;
        gupnp_service_proxy_end_action (content_dir,
                                        action,
                                        &error,
                                        /* OUT args */
                                        "Result",
                                        G_TYPE_STRING,
                                        &didl_xml,
                                        NULL);
        if (error) {
                on_browse_failure (GUPNP_SERVICE_INFO (content_dir), error);
                return;
        }

        gupnp_didl_lite_parser_parse_didl (didl_parser,
                                           didl_xml,
                                           on_didl_object_available,
                                           content_dir);

        g_free (didl_xml);
}

static void
browse (GUPnPServiceProxy *content_dir, const char *container_id)
{
        GError *error;

        error = NULL;
        gupnp_service_proxy_begin_action
                                (content_dir,
                                 "Browse",
                                 browse_cb,
                                 NULL,
                                 &error,
                                 /* IN args */
                                 "ObjectID",
                                 G_TYPE_STRING,
                                 container_id,
                                 "BrowseFlag",
                                 G_TYPE_STRING,
                                 "BrowseDirectChildren",
                                 "Filter",
                                 G_TYPE_STRING,
                                 "*",
                                 "StartingIndex",
                                 G_TYPE_UINT,
                                 0,
                                 "RequestedCount",
                                 G_TYPE_UINT, 0,
                                 "SortCriteria",
                                 G_TYPE_STRING,
                                 "",
                                 NULL);
        if (error)
                on_browse_failure (GUPNP_SERVICE_INFO (content_dir), error);
}

static void
append_media_server (GUPnPDeviceProxy *proxy,
                     GtkTreeModel          *model,
                     GtkTreeIter           *parent_iter)
{
        GUPnPDeviceInfo   *info;
        GUPnPServiceProxy *content_dir;
        char              *friendly_name;

        info = GUPNP_DEVICE_INFO (proxy);

        content_dir = get_content_dir (proxy);
        friendly_name = gupnp_device_info_get_friendly_name (info);

        if (G_LIKELY (friendly_name && content_dir != NULL)) {
                GtkTreeIter device_iter;
                GList      *child;

                gtk_tree_store_insert_with_values
                                (GTK_TREE_STORE (model),
                                 &device_iter, parent_iter, -1,
                                 0, get_icon_by_id (ICON_DEVICE),
                                 1, friendly_name,
                                 2, info,
                                 3, content_dir,
                                 -1);
                g_free (friendly_name);

                /* Append the embedded devices */
                child = gupnp_device_info_list_devices (info);
                while (child) {
                        append_media_server (GUPNP_DEVICE_PROXY (child->data),
                                             model,
                                             &device_iter);
                        g_object_unref (child->data);
                        child = g_list_delete_link (child, child);
                }

                schedule_icon_update (info, on_device_icon_available);

                browse (content_dir, "0");
                g_object_unref (content_dir);
        }
}

void
add_media_server (GUPnPDeviceProxy *proxy)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        const char   *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));

        if (!find_row (model,
                       NULL,
                       &iter,
                       compare_media_server,
                       (gpointer) udn,
                       FALSE)) {

                append_media_server (proxy, model, NULL);

                if (!expanded) {
                        gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));
                        expanded = TRUE;
                }
        }
}

void
remove_media_server (GUPnPDeviceProxy *proxy)
{
        GUPnPDeviceInfo *info;
        GtkTreeModel    *model;
        GtkTreeIter      iter;
        const char      *udn;

        info = GUPNP_DEVICE_INFO (proxy);

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (find_row (model,
                      NULL,
                      &iter,
                      compare_media_server,
                      (gpointer) udn,
                      FALSE)) {
                unschedule_icon_update (info);
                gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }
}

static gboolean
protocol_equal_func (const gchar *protocol,
                     const gchar *uri,
                     const gchar *pattern)
{
        return g_pattern_match_simple (pattern, protocol);
}

static char *
find_compatible_uri (GHashTable *resource_hash)
{
        GUPnPServiceProxy *av_transport;
        char             **protocols;
        char              *uri = NULL;
        guint              i;

        av_transport = get_selected_av_transport (&protocols);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");

                return NULL;
        }

        for (i = 0; protocols[i] && uri == NULL; i++) {
                uri = g_hash_table_find (resource_hash,
                                         (GHRFunc) protocol_equal_func,
                                         protocols[i]);
        }

        if (uri) {
                uri = g_strdup (uri);
        }

        if (protocols) {
                g_strfreev (protocols);
        }

        g_object_unref (av_transport);

        return uri;
}

char *
get_selected_item (char **uri)
{
        GtkTreeSelection*selection;
        GtkTreeModel    *model;
        GtkTreeIter      iter;
        GHashTable      *resource_hash;
        gboolean         is_container;
        char            *id;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
                return NULL;
        }

        gtk_tree_model_get (model,
                            &iter,
                            4, &id,
                            5, &is_container,
                            6, &resource_hash,
                            -1);

        if (is_container) {
                goto abnormal_return;
        }

        *uri = find_compatible_uri (resource_hash);
        if (*uri == NULL) {
                g_warning ("no compatible URI found.");

                goto abnormal_return;
        }

        goto normal_return;

abnormal_return:
        if (id) {
                g_free (id);
                id = NULL;
        }

normal_return:
        g_hash_table_unref (resource_hash);

        return id;
}

