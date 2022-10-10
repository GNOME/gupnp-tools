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

#include <string.h>
#include <stdlib.h>

#include <gmodule.h>

#include "playlist-treeview.h"
#include "renderer-combo.h"
#include "renderer-controls.h"
#include "icons.h"
#include "pretty-print.h"
#include "gui.h"
#include "server-device.h"
#include "search-dialog.h"
#include "didl-dialog.h"

#define CONTENT_DIR "urn:schemas-upnp-org:service:ContentDirectory"

#define ITEM_CLASS_IMAGE "object.item.imageItem"
#define ITEM_CLASS_AUDIO "object.item.audioItem"
#define ITEM_CLASS_VIDEO "object.item.videoItem"
#define ITEM_CLASS_TEXT  "object.item.textItem"

#define MAX_BROWSE 64

typedef gboolean (* RowCompareFunc) (GtkTreeModel *model,
                                     GtkTreeIter  *iter,
                                     gpointer      user_data);

static GtkWidget *treeview;
static GtkWidget *popup;
static GtkWidget *didl_dialog;
static gboolean   expanded;
static GHashTable *initial_notify;
static GtkDialog *search_dialog;

gboolean
on_playlist_treeview_button_release (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     gpointer        user_data);
void
on_playlist_treeview_item_selected (GtkTreeSelection *selection,
                                    gpointer          user_data);
void
on_playlist_row_expanded (GtkTreeView *tree_view,
                          GtkTreeIter *iter,
                          GtkTreePath *path,
                          gpointer     user_data);

void
on_playlist_row_collapsed (GtkTreeView *tree_view,
                           GtkTreeIter *iter,
                           GtkTreePath *path,
                           gpointer     user_data);

void
on_didl_menuitem_activate (GtkMenuItem *menuitem,
                           gpointer     user_data);

void
on_search_menu_item_activated (GtkMenuItem *menuitem,
                               gpointer     user_data);

gboolean
on_playlist_treeview_popup_menu (GtkWidget *widget, gpointer user_data);

static void
on_proxy_ready (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data);

typedef struct
{
        AVCPMediaServer *server;
        gchar *id;
        guint32 starting_index;
} BrowseData;

typedef struct
{
  MetadataFunc callback;

  gchar *id;

  gpointer user_data;
} BrowseMetadataData;

static BrowseData *
browse_data_new (AVCPMediaServer *server,
                 const char      *id,
                 guint32          starting_index)
{
        BrowseData *data;

        data = g_slice_new (BrowseData);
        data->server = g_object_ref (server);
        data->id = g_strdup (id);
        data->starting_index = starting_index;

        return data;
}

static void
browse_data_free (BrowseData *data)
{
        g_free (data->id);
        g_object_unref (data->server);
        g_slice_free (BrowseData, data);
}

static BrowseMetadataData *
browse_metadata_data_new (MetadataFunc callback,
                          const char  *id,
                          gpointer     user_data)
{
        BrowseMetadataData *data;

        data = g_slice_new (BrowseMetadataData);
        data->callback = callback;
        data->id = g_strdup (id);
        data->user_data = user_data;

        return data;
}

static void
browse_metadata_data_free (BrowseMetadataData *data)
{
        g_free (data->id);
        g_slice_free (BrowseMetadataData, data);
}

static void
browse (AVCPMediaServer *content_dir,
        const char      *container_id,
        guint32          starting_index,
        guint32          requested_count);

static void
do_popup_menu (GtkMenu *menu, GtkWidget *widget, GdkEventButton *event)
{
#if GTK_CHECK_VERSION(3,22,0)
        gtk_menu_popup_at_pointer (menu, (event != NULL) ? (GdkEvent *) event : gtk_get_current_event ());
#else
        int button = 0;
        int event_time;
        if (event) {
                button = event->button;
                event_time = event->time;
        } else {
                event_time = gtk_get_current_event_time ();
        }

        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, button, event_time);
#endif
}

G_MODULE_EXPORT
gboolean
on_playlist_treeview_button_release (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     gpointer        user_data)
{
        GtkTreeSelection *selection;
        GtkTreeModel     *model;
        GtkTreeIter       iter;

        if (event->type != GDK_BUTTON_RELEASE ||
            event->button != 3)
                return FALSE;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        /* Only show the popup menu when a row is selected */
        if (!gtk_tree_selection_get_selected (selection, &model, &iter))
                return FALSE;

        do_popup_menu (GTK_MENU (popup), treeview, event);

        return TRUE;
}

G_MODULE_EXPORT
gboolean
on_playlist_treeview_popup_menu (GtkWidget *widget, gpointer user_data)
{
        do_popup_menu (GTK_MENU (popup), treeview, NULL);

        return TRUE;
}

G_MODULE_EXPORT
void
on_playlist_treeview_item_selected (GtkTreeSelection *selection,
                           gpointer          user_data)
{
        PlaybackState state;

        state = get_selected_renderer_state ();

        if (state == PLAYBACK_STATE_PLAYING ||
            state == PLAYBACK_STATE_PAUSED) {
                if (!get_selected_object ((MetadataFunc) set_av_transport_uri,
                                          NULL)) {
                        av_transport_send_action ("Stop", NULL);
                }
       }
}


G_MODULE_EXPORT
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
                AVCPMediaServer *server;
                gchar           *id;
                gboolean         is_container;
                gint             child_count;

                gtk_tree_model_get (model, &child_iter,
                                    2, &server,
                                    4, &id,
                                    5, &is_container,
                                    6, &child_count,
                                    -1);

                if (is_container && child_count != 0) {
                        browse (server, id, 0, MAX_BROWSE);
                }

                g_object_unref (server);
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

G_MODULE_EXPORT
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

static void display_metadata (const char *metadata,
                              gpointer    user_data)
{
        av_cp_didl_dialog_set_xml (AV_CP_DIDL_DIALOG (didl_dialog), metadata);

        gtk_dialog_run (GTK_DIALOG (didl_dialog));
}

G_MODULE_EXPORT
void
on_didl_menuitem_activate (GtkMenuItem *menuitem,
                           gpointer     user_data)
{
        get_selected_object (display_metadata, NULL);
}

G_MODULE_EXPORT
void
on_search_menu_item_activated (GtkMenuItem *menuitem,
                               gpointer     user_data)
{
    AVCPMediaServer   *server;
    GtkTreeSelection  *selection;
    GtkTreeModel      *model;
    GtkTreeIter        iter;
    char              *id = NULL;
    char              *title = NULL;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    g_assert (selection != NULL);

    if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
        return;
    }

    gtk_tree_model_get (model,
                        &iter,
                        1, &title,
                        2, &server,
                        4, &id,
                        -1);

    if (search_dialog == NULL) {
        search_dialog = search_dialog_new ();
    }

    search_dialog_set_server (SEARCH_DIALOG (search_dialog), server);
    search_dialog_set_container_id (SEARCH_DIALOG (search_dialog), id);
    search_dialog_set_container_title (SEARCH_DIALOG (search_dialog), title);

    search_dialog_run (SEARCH_DIALOG (search_dialog));
}

void
setup_playlist_treeview (GtkBuilder *builder)
{
        initial_notify = g_hash_table_new (g_direct_hash, g_direct_equal);

        treeview = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "playlist-treeview"));
        g_assert (treeview != NULL);

        g_object_weak_ref (G_OBJECT (treeview),
                           (GWeakNotify) g_hash_table_destroy,
                           initial_notify);

        popup = GTK_WIDGET (gtk_builder_get_object (builder, "playlist-popup"));
        g_assert (popup != NULL);
        gtk_menu_attach_to_widget (GTK_MENU (popup), treeview, NULL);

        didl_dialog = GTK_WIDGET (av_cp_didl_dialog_new ());
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

        if (info && GUPNP_IS_DEVICE_PROXY (info)) {
                found = g_strcmp0 (gupnp_device_info_get_udn (info), udn) == 0;
        }

        g_clear_object (&info);

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

        if (!is_container) {
                g_free (container_id);

                return found;
        }

        if (g_strcmp0 (container_id, id) == 0) {
                found = TRUE;
        }

        g_clear_pointer (&container_id, g_free);

        return found;
}

static GdkPixbuf *
get_item_icon (GUPnPDIDLLiteObject *object)
{
        GdkPixbuf  *icon;
        const char *class_name;

        class_name = gupnp_didl_lite_object_get_upnp_class (object);
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

        return icon;
}

static gboolean
find_container (AVCPMediaServer   *server,
                GtkTreeModel      *model,
                GtkTreeIter       *container_iter,
                const char        *container_id)
{
        GtkTreeIter server_iter;
        const char *udn;

        udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (server));

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
update_container (AVCPMediaServer *server,
                  const char      *container_id)
{
        GtkTreeModel *model;
        GtkTreeIter   container_iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (find_container (server,
                            model,
                            &container_iter,
                            container_id)) {
                /* Remove everyting under the container */
                if (gtk_tree_model_iter_has_child (model, &container_iter)) {
                        unpopulate_container (model, &container_iter);
                }

                /* Browse it again */
                browse (server, container_id, 0, MAX_BROWSE);
        }
}

static void
update_container_child_count (AVCPMediaServer *server,
                              const char      *container_id)
{
        GtkTreeModel *model;
        GtkTreeIter   container_iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (find_container (server,
                            model,
                            &container_iter,
                            container_id)) {
                int child_count;

                child_count = gtk_tree_model_iter_n_children (model,
                                                              &container_iter);
                gtk_tree_store_set (GTK_TREE_STORE (model),
                                    &container_iter,
                                    6, child_count,
                                    -1);
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
        AVCPMediaServer *server = AV_CP_MEDIA_SERVER (user_data);

        /* Ignore initial event. It might have some old updates that happened
         * ages ago and cause duplicate entries in our list.
         */
        if (g_hash_table_contains (initial_notify, content_dir)) {
                g_hash_table_remove (initial_notify, content_dir);

                return;
        }

        tokens = g_strsplit (g_value_get_string (value), ",", 0);
        for (i = 0;
             tokens[i] != NULL && tokens[i+1] != NULL;
             i += 2) {
                update_container (server, tokens[i]);
        }

        g_strfreev (tokens);
}

static void
append_didl_object (GUPnPDIDLLiteObject *object,
                    BrowseData          *browse_data,
                    GtkTreeModel        *model,
                    GtkTreeIter         *server_iter)
{
        GtkTreeIter parent_iter;
        const char *id = NULL;
        const char *parent_id = NULL;
        const char *title = NULL;
        gboolean    is_container;
        gint        child_count;
        GdkPixbuf  *icon;

        id = gupnp_didl_lite_object_get_id (object);
        title = gupnp_didl_lite_object_get_title (object);
        parent_id = gupnp_didl_lite_object_get_parent_id (object);
        if (id == NULL || title == NULL || parent_id == NULL)
                return;

        is_container = GUPNP_IS_DIDL_LITE_CONTAINER (object);

        if (is_container) {
                GUPnPDIDLLiteContainer *container;
                icon = get_icon_by_id (ICON_CONTAINER);

                container = GUPNP_DIDL_LITE_CONTAINER (object);
                child_count = gupnp_didl_lite_container_get_child_count
                                                                (container);
        } else {
                child_count = 0;
                icon = get_item_icon (object);
        }

        /* Check if we browsed the root container. */
        if (strcmp (browse_data->id, "0") == 0) {
                parent_iter = *server_iter;
        } else if (!find_row (model,
                              server_iter,
                              &parent_iter,
                              compare_container,
                              (gpointer) parent_id,
                              TRUE))
                return;

        gtk_tree_store_insert_with_values (GTK_TREE_STORE (model),
                                           NULL, &parent_iter, -1,
                                           0, icon,
                                           1, title,
                                           2, browse_data->server,
                                           4, id,
                                           5, is_container,
                                           6, child_count,
                                           -1);
}

static void
update_device_icon (GUPnPDeviceInfo *info)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        AVCPMediaServer *server;
        const char   *udn;
        GdkPixbuf    *icon;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);
        server = AV_CP_MEDIA_SERVER (info);
        g_object_get (G_OBJECT (server), "icon", &icon, NULL);

        if (icon == NULL)
                return;

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

static void
on_proxy_ready (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data)
{
        GError *error = NULL;
        gboolean result = FALSE;

        result = g_async_initable_init_finish (G_ASYNC_INITABLE (source_object),
                                               res,
                                               &error);

        if (result == TRUE) {
                update_device_icon (GUPNP_DEVICE_INFO (source_object));
                browse (AV_CP_MEDIA_SERVER (source_object), "0", 0, MAX_BROWSE);
        }
}


static void
on_didl_object_available (GUPnPDIDLLiteParser *parser,
                          GUPnPDIDLLiteObject *object,
                          gpointer             user_data)
{
        BrowseData   *browse_data;
        GtkTreeModel *model;
        GtkTreeIter   server_iter;
        const char   *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        browse_data = (BrowseData *) user_data;

        udn = gupnp_device_info_get_udn
                                (GUPNP_DEVICE_INFO (browse_data->server));

        if (find_row (model,
                       NULL,
                       &server_iter,
                       compare_media_server,
                       (gpointer) udn,
                       FALSE)) {
                append_didl_object (object,
                                    browse_data,
                                    model,
                                    &server_iter);
        }

        return;
}

static void
browse_cb (GObject *object,
           GAsyncResult *result,
           gpointer user_data)
{
        BrowseData *data;
        char       *didl_xml;
        guint32     number_returned;
        guint32     total_matches;
        GError     *error;

        data = (BrowseData *) user_data;
        didl_xml = NULL;
        error = NULL;

        av_cp_media_server_browse_finish (AV_CP_MEDIA_SERVER (object),
                                          result,
                                          &didl_xml,
                                          &total_matches,
                                          &number_returned,
                                          &error);

        if (didl_xml) {
                GUPnPDIDLLiteParser *parser;
                gint32              remaining;
                gint32              batch_size;

                error = NULL;
                parser = gupnp_didl_lite_parser_new ();

                g_signal_connect (parser,
                                  "object-available",
                                  G_CALLBACK (on_didl_object_available),
                                  data);

                /* Only try to parse DIDL if server claims that there was a
                 * result */
                if (number_returned > 0)
                        if (!gupnp_didl_lite_parser_parse_didl (parser,
                                                                didl_xml,
                                                                &error)) {
                                g_warning ("Error while browsing %s: %s",
                                           data->id,
                                           error->message);
                                g_error_free (error);
                        }

                g_object_unref (parser);
                g_free (didl_xml);

                data->starting_index += number_returned;

                /* See if we have more objects to get */
                remaining = total_matches - data->starting_index;

                /* Keep browsing till we get each and every object */
                if ((remaining > 0 || total_matches == 0) && number_returned != 0) {
                        if (remaining > 0)
                                batch_size = MIN (remaining, MAX_BROWSE);
                        else
                                batch_size = MAX_BROWSE;

                        browse (AV_CP_MEDIA_SERVER (object),
                                data->id,
                                data->starting_index,
                                batch_size);
                } else
                        update_container_child_count (AV_CP_MEDIA_SERVER (object),
                                                      data->id);
        } else if (error) {
                g_warning ("Failed to browse '%s': %s",
                           gupnp_device_info_get_location (GUPNP_DEVICE_INFO (object)),
                           error->message);

                g_error_free (error);
        }

        browse_data_free (data);
}

static void
browse_metadata_cb (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
        BrowseMetadataData *data;
        char               *metadata;
        GError             *error;

        data = (BrowseMetadataData *) user_data;

        metadata = NULL;
        error = NULL;

        av_cp_media_server_browse_metadata_finish (AV_CP_MEDIA_SERVER (object),
                                                   result,
                                                   &metadata,
                                                   &error);

        if (metadata != NULL) {
                data->callback (metadata, data->user_data);

                g_free (metadata);
        } else if (error) {
                g_warning ("Failed to get metadata for '%s': %s",
                           data->id,
                           error->message);

                g_error_free (error);
        }

        browse_metadata_data_free (data);
}

static void
browse (AVCPMediaServer *server,
        const char      *container_id,
        guint32          starting_index,
        guint32          requested_count)
{
        BrowseData *data;

        data = browse_data_new (server,
                                container_id,
                                starting_index);

        av_cp_media_server_browse_async (server,
                                         NULL,
                                         browse_cb,
                                         container_id,
                                         starting_index,
                                         requested_count,
                                         data);
}

static void
browse_metadata (AVCPMediaServer *server,
                 const char      *id,
                 MetadataFunc     callback,
                 gpointer         user_data)
{
        BrowseMetadataData *data;

        data = browse_metadata_data_new (callback, id, user_data);

        av_cp_media_server_browse_metadata_async (server,
                                                  NULL,
                                                  browse_metadata_cb,
                                                  id,
                                                  data);
}

static void
append_media_server (GUPnPDeviceProxy *proxy,
                     GtkTreeModel     *model,
                     GtkTreeIter      *parent_iter)
{
        GUPnPDeviceInfo   *info;
        char              *friendly_name;
        GUPnPServiceProxy *content_dir;

        info = GUPNP_DEVICE_INFO (proxy);

        friendly_name = gupnp_device_info_get_friendly_name (info);
        content_dir = av_cp_media_server_get_content_directory
                                (AV_CP_MEDIA_SERVER (info));

        if (G_LIKELY (friendly_name != NULL && content_dir != NULL)) {
                GtkTreeIter device_iter;
                GList      *child;

                gtk_tree_store_insert_with_values
                                (GTK_TREE_STORE (model),
                                 &device_iter, parent_iter, -1,
                                 0, get_icon_by_id (ICON_DEVICE),
                                 1, friendly_name,
                                 2, info,
                                 3, content_dir,
                                 4, "0",
                                 5, TRUE,
                                 -1);
                g_free (friendly_name);

                g_async_initable_init_async (G_ASYNC_INITABLE (proxy),
                                             G_PRIORITY_DEFAULT,
                                             NULL,
                                             on_proxy_ready,
                                             content_dir);

                /* Append the embedded devices */
                child = gupnp_device_info_list_devices (info);
                while (child) {
                        append_media_server (GUPNP_DEVICE_PROXY (child->data),
                                             model,
                                             &device_iter);
                        g_object_unref (child->data);
                        child = g_list_delete_link (child, child);
                }

                gupnp_service_proxy_add_notify (content_dir,
                                                "ContainerUpdateIDs",
                                                G_TYPE_STRING,
                                                on_container_update_ids,
                                                info);
                gupnp_service_proxy_set_subscribed (content_dir, TRUE);
                g_hash_table_insert (initial_notify, content_dir, content_dir);

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
                gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }
}

gboolean
get_selected_object (MetadataFunc callback,
                     gpointer     user_data)
{
        AVCPMediaServer   *server;
        GtkTreeSelection  *selection;
        GtkTreeModel      *model;
        GtkTreeIter        iter;
        char              *id = NULL;
        gboolean           ret = FALSE;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
                goto return_point;
        }

        gtk_tree_model_get (model,
                            &iter,
                            2, &server,
                            4, &id,
                            -1);

        browse_metadata (server, id, callback, user_data);

        ret = TRUE;

        if (id) {
                g_free (id);
        }

        g_object_unref (server);

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

