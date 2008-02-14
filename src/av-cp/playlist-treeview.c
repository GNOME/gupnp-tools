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

#define CONTENT_DIR "urn:schemas-upnp-org:service:ContentDirectory:*"

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

typedef struct
{
  GetSelectedItemCallback callback;

  gchar *uri;
  gchar *id;

  gpointer user_data;
} BrowseMetadataData;

static BrowseMetadataData *
browse_metadata_data_new (GetSelectedItemCallback callback,
                          const char             *id,
                          const char             *uri,
                          gpointer                user_data)
{
        BrowseMetadataData *data;

        data = g_slice_new (BrowseMetadataData);
        data->callback = callback;
        data->id = g_strdup (id);
        data->uri = g_strdup (uri);
        data->user_data = user_data;

        return data;
}

static void
browse_metadata_data_free (BrowseMetadataData *data)
{
        g_free (data->id);
        g_free (data->uri);
        g_slice_free (BrowseMetadataData, data);
}

static void
browse (GUPnPServiceProxy *content_dir, const char *container_id);

gboolean
on_playlist_treeview_button_release (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     gpointer        user_data)
{
        return FALSE;
}

static void
on_item_selected (GtkTreeSelection *selection,
                  gpointer          user_data)
{
        PlaybackState state;

        state = get_selected_renderer_state ();

        if (state == PLAYBACK_STATE_PLAYING ||
            state == PLAYBACK_STATE_PAUSED) {
                if (!get_selected_item ((GetSelectedItemCallback)
                                         set_av_transport_uri,
                                        NULL)) {
                        av_transport_send_action ("Stop", NULL);
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
on_playlist_row_expanded (GtkTreeView *tree_view,
                          GtkTreeIter *iter,
                          GtkTreePath *path,
                          gpointer     user_data)
{
        GtkTreeModel *model;
        GtkTreeIter   child_iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (!gtk_tree_model_iter_children (model, &child_iter, iter)) {
                return;
        }

        do {
                GUPnPServiceProxy *content_dir;
                gchar             *id;
                gboolean           is_container;

                gtk_tree_model_get (model, &child_iter,
                                    3, &content_dir,
                                    4, &id,
                                    5, &is_container,
                                    -1);

                if (is_container) {
                        browse (content_dir, id);
                }

                g_object_unref (content_dir);
                g_free (id);
        } while (gtk_tree_model_iter_next (model, &child_iter));
}

static void
unpopulate_container (GtkTreeModel *model,
                      GtkTreeIter  *container_iter)
{
        GtkTreeIter child_iter;

        if (!gtk_tree_model_iter_children (model,
                                           &child_iter,
                                           container_iter)) {
                return;
        }

        while (gtk_tree_store_remove(GTK_TREE_STORE (model),
                                     &child_iter));
}

void
on_playlist_row_collapsed (GtkTreeView *tree_view,
                           GtkTreeIter *iter,
                           GtkTreePath *path,
                           gpointer     user_data)
{
        GtkTreeModel *model;
        GtkTreeIter   child_iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (!gtk_tree_model_iter_children (model, &child_iter, iter)) {
                return;
        }

        do {
                unpopulate_container (model, &child_iter);
        } while (gtk_tree_model_iter_next (model, &child_iter));
}

static gboolean
tree_selection_func (GtkTreeSelection *selection,
                     GtkTreeModel     *model,
                     GtkTreePath      *path,
                     gboolean          path_currently_selected,
                     gpointer          data)
{
        GtkTreeIter iter;
        gboolean    is_container;

        if (path_currently_selected) {
                return TRUE;
        }

        if (!gtk_tree_model_get_iter (model, &iter, path)) {
                return FALSE;
        }

        gtk_tree_model_get (model,
                            &iter,
                            5, &is_container,
                            -1);

        /* Let it be selected if it's an item */
        return !is_container;
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
        gtk_tree_selection_set_select_function (selection,
                                                tree_selection_func,
                                                NULL,
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

static gboolean
find_container (GUPnPServiceProxy *content_dir,
                GtkTreeModel      *model,
                GtkTreeIter       *container_iter,
                const char        *container_id)
{
        GtkTreeIter server_iter;
        const char *udn;

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (content_dir));

        if (!find_row (model,
                       NULL,
                       &server_iter,
                       compare_media_server,
                       (gpointer) udn,
                       FALSE)) {
                return FALSE;
        } else if (!find_row (model,
                              &server_iter,
                              container_iter,
                              compare_container,
                              (gpointer) container_id,
                              TRUE)) {
                return FALSE;
        } else {
                return TRUE;
        }

}

static void
update_container (GUPnPServiceProxy *content_dir,
                  const char        *container_id)
{
        GtkTreeModel *model;
        GtkTreeIter   container_iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (!find_container (content_dir,
                             model,
                             &container_iter,
                             container_id)) {
                return;
        } else if (gtk_tree_model_iter_has_child (model, &container_iter)) {
                /* Remove everyting under the container */
                unpopulate_container (model, &container_iter);
                /* Browse it again */
                browse (content_dir, container_id);
        }
}

static void
on_container_update_ids (GUPnPServiceProxy *content_dir,
                         const char        *variable,
                         GValue            *value,
                         gpointer           user_data)
{
        char **tokens;
        guint  i;

        tokens = g_strsplit (g_value_get_string (value), ",", 0);
        for (i = 0;
             tokens[i] != NULL && tokens[i+1] != NULL;
             i += 2) {
                update_container (content_dir, tokens[i]);
        }

        g_strfreev (tokens);
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

        if (is_container) {
                position = 0;
                resource_hash = NULL;
                icon = get_icon_by_id (ICON_CONTAINER);
        } else {
                position = -1;
                resource_hash =
                        gupnp_didl_lite_object_get_resource_hash (object_node);
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
        GList            *types;
        GList            *type;
        GUPnPServiceInfo *content_dir = NULL;

        info = GUPNP_DEVICE_INFO (proxy);

        types = gupnp_device_info_list_service_types (info);
        for (type = types;
             type;
             type = type->next) {
                if (g_pattern_match_simple (CONTENT_DIR,
                                            (char *) type->data)) {
                        content_dir = gupnp_device_info_get_service
                                                      (info,
                                                       (char *) type->data);
                }

                g_free (type->data);
        }

        if (types) {
                g_list_free (types);
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

static inline void
on_browse_metadata_failure (BrowseMetadataData *data,
                            GError             *error)
{
        g_warning ("Failed to get metadata for '%s': %s\n",
                   data->uri,
                   error->message);
        g_error_free (error);
        browse_metadata_data_free (data);
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
        } else {
                gupnp_didl_lite_parser_parse_didl (didl_parser,
                                                   didl_xml,
                                                   on_didl_object_available,
                                                   content_dir);

                g_free (didl_xml);
        }

        g_object_unref (content_dir);
}

static void
browse (GUPnPServiceProxy *content_dir, const char *container_id)
{
        GError *error;

        g_object_ref (content_dir);

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
        if (error) {
                on_browse_failure (GUPNP_SERVICE_INFO (content_dir), error);
                g_object_unref (content_dir);
        }
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
                                 5, TRUE,
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
                gupnp_service_proxy_add_notify (content_dir,
                                                "ContainerUpdateIDs",
                                                G_TYPE_STRING,
                                                on_container_update_ids,
                                                NULL);
                gupnp_service_proxy_set_subscribed (content_dir, TRUE);

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
is_transport_compat (const gchar *renderer_protocol,
                     const gchar *renderer_host,
                     const gchar *item_protocol,
                     const gchar *item_host)
{
        if (g_ascii_strcasecmp (renderer_protocol, item_protocol) != 0 &&
            g_ascii_strcasecmp (renderer_protocol, "*") != 0) {
                return FALSE;
        } else if (g_ascii_strcasecmp ("INTERNAL", renderer_protocol) == 0 &&
                   g_ascii_strcasecmp (renderer_host, item_host) != 0) {
                   /* Host must be the same in case of INTERNAL protocol */
                        return FALSE;
        } else {
                return TRUE;
        }
}

static gboolean
is_content_format_compat (const gchar *renderer_content_format,
                          const gchar *item_content_format)
{
        if (g_ascii_strcasecmp (renderer_content_format, "*") != 0 &&
            g_ascii_strcasecmp (renderer_content_format,
                                item_content_format) != 0) {
                return FALSE;
        } else {
                return TRUE;
        }
}

static gchar *
get_dlna_pn (gchar **additional_info_fields)
{
        gchar *pn = NULL;
        gint   i;

        for (i = 0; additional_info_fields[i]; i++) {
                pn = g_strstr_len (additional_info_fields[i],
                                   strlen (additional_info_fields[i]),
                                   "DLNA.ORG_PN=");
                if (pn != NULL) {
                        pn += 12; /* end of "DLNA.ORG_PN=" */

                        break;
                }
        }

        return pn;
}

static gboolean
is_additional_info_compat (const gchar *renderer_additional_info,
                           const gchar *item_additional_info)
{
        gchar  **renderer_tokens;
        gchar  **item_tokens;
        gchar   *renderer_pn;
        gchar   *item_pn;
        gboolean ret = FALSE;

        if (g_ascii_strcasecmp (renderer_additional_info, "*") == 0) {
                return TRUE;
        }

        renderer_tokens = g_strsplit (renderer_additional_info, ";", -1);
        if (renderer_tokens == NULL) {
                return FALSE;
        }

        item_tokens = g_strsplit (item_additional_info, ";", -1);
        if (item_tokens == NULL) {
                goto no_item_tokens;
        }

        renderer_pn = get_dlna_pn (renderer_tokens);
        item_pn = get_dlna_pn (item_tokens);
        if (renderer_pn == NULL || item_pn == NULL) {
                goto no_renderer_pn;
        }

        if (g_ascii_strcasecmp (renderer_pn, item_pn) == 0) {
                ret = TRUE;
        }

no_renderer_pn:
        g_strfreev (item_tokens);
no_item_tokens:
        g_strfreev (renderer_tokens);

        return ret;
}


static gboolean
protocol_equal_func (const gchar *item_protocol,
                     const gchar *item_uri,
                     const gchar *renderer_protocol)
{
        gchar **item_proto_tokens;
        gchar **renderer_proto_tokens;
        gboolean ret = FALSE;

        item_proto_tokens = g_strsplit (item_protocol,
                                        ":",
                                        4);
        renderer_proto_tokens = g_strsplit (renderer_protocol,
                                            ":",
                                            4);
        if (item_proto_tokens[0] == NULL ||
            item_proto_tokens[1] == NULL ||
            item_proto_tokens[2] == NULL ||
            item_proto_tokens[3] == NULL ||
            renderer_proto_tokens[0] == NULL ||
            renderer_proto_tokens[1] == NULL ||
            renderer_proto_tokens[2] == NULL ||
            renderer_proto_tokens[3] == NULL) {
                goto return_point;
        }

        if (is_transport_compat (renderer_proto_tokens[0],
                                 renderer_proto_tokens[2],
                                 item_proto_tokens[0],
                                 item_proto_tokens[1]) &&
            is_content_format_compat (renderer_proto_tokens[2],
                                      item_proto_tokens[2]) &&
            is_additional_info_compat (renderer_proto_tokens[3],
                                       item_proto_tokens[3])) {
                ret = TRUE;
        }

return_point:
        g_strfreev (renderer_proto_tokens);
        g_strfreev (item_proto_tokens);

        return ret;
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

void
browse_metadata_cb (GUPnPServiceProxy       *content_dir,
                    GUPnPServiceProxyAction *action,
                    gpointer                 user_data)
{
        BrowseMetadataData *data;
        char               *metadata;
        GError             *error;

        data = (BrowseMetadataData *) user_data;

        error = NULL;
        gupnp_service_proxy_end_action (content_dir,
                                        action,
                                        &error,
                                        /* OUT args */
                                        "Result",
                                        G_TYPE_STRING,
                                        &metadata,
                                        NULL);
        if (error) {
                on_browse_metadata_failure (data, error);
        } else {
                data->callback (data->uri, metadata, data->user_data);

                browse_metadata_data_free (data);
                g_free (metadata);
        }

        g_object_unref (content_dir);
}

gboolean
get_selected_item (GetSelectedItemCallback callback,
                   gpointer                user_data)
{
        GUPnPServiceProxy  *content_dir;
        GtkTreeSelection   *selection;
        GtkTreeModel       *model;
        GtkTreeIter         iter;
        GHashTable         *resource_hash;
        gboolean            is_container;
        BrowseMetadataData *data;
        char               *id = NULL;
        char               *uri = NULL;
        GError             *error;
        gboolean            ret = FALSE;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
                goto return_point;
        }

        gtk_tree_model_get (model,
                            &iter,
                            3, &content_dir,
                            4, &id,
                            5, &is_container,
                            6, &resource_hash,
                            -1);

        if (is_container) {
                goto free_and_return;
        }

        uri = find_compatible_uri (resource_hash);
        if (uri == NULL) {
                g_warning ("no compatible URI found.");

                goto free_and_return;
        }

        data = browse_metadata_data_new (callback, id, uri, user_data);

        error = NULL;
        gupnp_service_proxy_begin_action
                                (g_object_ref (content_dir),
                                 "Browse",
                                 browse_metadata_cb,
                                 data,
                                 &error,
                                 /* IN args */
                                 "ObjectID",
                                 G_TYPE_STRING,
                                 data->id,
                                 "BrowseFlag",
                                 G_TYPE_STRING,
                                 "BrowseMetadata",
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
        if (error) {
                on_browse_metadata_failure (data, error);
                g_object_unref (content_dir);
        } else {
                ret = TRUE;
        }

free_and_return:
        if (id) {
                g_free (id);
        }

        if (resource_hash) {
                g_hash_table_unref (resource_hash);
        }

        g_object_unref (content_dir);

return_point:
        return ret;
}

void
select_next_object (void)
{
        GtkTreeSelection *selection;
        GtkTreeModel     *model;
        GtkTreeIter       iter;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
                return;
        }

        if (gtk_tree_model_iter_next (model, &iter)) {
                gtk_tree_selection_select_iter (selection, &iter);
        }
}

void
select_prev_object (void)
{
        GtkTreeSelection *selection;
        GtkTreeModel     *model;
        GtkTreePath      *path;
        GtkTreeIter       iter;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
                return;
        }

        path = gtk_tree_model_get_path (model, &iter);
        if (path == NULL) {
                return;
        }

        if (gtk_tree_path_prev (path)) {
                gtk_tree_selection_select_path (selection, path);
        }

        gtk_tree_path_free (path);
}

