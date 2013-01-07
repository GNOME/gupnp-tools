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

#ifdef HAVE_GTK_SOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

#include "playlist-treeview.h"
#include "renderer-combo.h"
#include "renderer-controls.h"
#include "icons.h"
#include "pretty-print.h"
#include "gui.h"

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
static GtkWidget *didl_textview;
static gboolean   expanded;
static GHashTable *initial_notify;

typedef struct
{
  GUPnPServiceProxy *content_dir;

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
browse_data_new (GUPnPServiceProxy *content_dir,
                 const char        *id,
                 guint32            starting_index)
{
        BrowseData *data;

        data = g_slice_new (BrowseData);
        data->content_dir = g_object_ref (content_dir);
        data->id = g_strdup (id);
        data->starting_index = starting_index;

        return data;
}

static void
browse_data_free (BrowseData *data)
{
        g_free (data->id);
        g_object_unref (data->content_dir);
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
browse (GUPnPServiceProxy *content_dir,
        const char        *container_id,
        guint32            starting_index,
        guint32            requested_count);

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

        gtk_menu_popup (GTK_MENU (popup),
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        event->button,
                        event->time);

        return TRUE;
}

static void
on_item_selected (GtkTreeSelection *selection,
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
                                    /* childCount */
                                    G_TYPE_INT);

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
                GUPnPServiceProxy *content_dir;
                gchar             *id;
                gboolean           is_container;
                gint              child_count;

                gtk_tree_model_get (model, &child_iter,
                                    3, &content_dir,
                                    4, &id,
                                    5, &is_container,
                                    6, &child_count,
                                    -1);

                if (is_container && child_count != 0) {
                        browse (content_dir, id, 0, MAX_BROWSE);
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
        GtkTextBuffer *buffer;
        char *formatted;

        formatted = pretty_print_xml (metadata);

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (didl_textview));
        gtk_text_buffer_set_text (buffer, formatted, -1);
        gtk_widget_show (GTK_WIDGET (didl_textview));

        gtk_dialog_run (GTK_DIALOG (didl_dialog));

        g_free (formatted);
}

G_MODULE_EXPORT
void
on_didl_menuitem_activate (GtkMenuItem *menuitem,
                           gpointer     user_data)
{
        get_selected_object (display_metadata, NULL);
}

void
setup_playlist_treeview (GtkBuilder *builder)
{
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;

        initial_notify = g_hash_table_new (g_direct_hash, g_direct_equal);

        treeview = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "playlist-treeview"));
        g_assert (treeview != NULL);

        g_object_weak_ref (G_OBJECT (treeview),
                           (GWeakNotify) g_hash_table_destroy,
                           initial_notify);

        popup = GTK_WIDGET (gtk_builder_get_object (builder, "playlist-popup"));
        g_assert (popup != NULL);

        didl_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "didl-dialog"));
        didl_textview = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "didl-textview"));
#ifdef HAVE_GTK_SOURCEVIEW
        GtkSourceLanguageManager *manager =
                                gtk_source_language_manager_get_default ();
        GtkSourceLanguage *language =
                gtk_source_language_manager_guess_language (manager,
                                                            NULL,
                                                            "text/xml");

        GtkSourceBuffer *buffer = gtk_source_buffer_new_with_language (language);
        gtk_source_buffer_set_highlight_syntax (buffer, TRUE);
        gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (didl_textview),
                                               TRUE);

        gtk_text_view_set_buffer (GTK_TEXT_VIEW (didl_textview),
                                  GTK_TEXT_BUFFER (buffer));
#endif

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

        if (!is_container) {
                g_free (container_id);

                return found;
        }

        if (container_id == NULL)
                return found;

        if (strcmp (container_id, id) == 0) {
                found = TRUE;
        }

        g_free (container_id);

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

        if (find_container (content_dir,
                            model,
                            &container_iter,
                            container_id)) {
                /* Remove everyting under the container */
                if (gtk_tree_model_iter_has_child (model, &container_iter)) {
                        unpopulate_container (model, &container_iter);
                }

                /* Browse it again */
                browse (content_dir, container_id, 0, MAX_BROWSE);
        }
}

static void
update_container_child_count (GUPnPServiceProxy *content_dir,
                              const char        *container_id)
{
        GtkTreeModel *model;
        GtkTreeIter   container_iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (find_container (content_dir,
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
                update_container (content_dir, tokens[i]);
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
        const char *id;
        const char *parent_id;
        const char *title;
        gboolean    is_container;
        gint        child_count;
        GdkPixbuf  *icon;
        gint        position;

        id = gupnp_didl_lite_object_get_id (object);
        title = gupnp_didl_lite_object_get_title (object);
        parent_id = gupnp_didl_lite_object_get_parent_id (object);
        if (id == NULL || title == NULL || parent_id == NULL)
                return;

        is_container = GUPNP_IS_DIDL_LITE_CONTAINER (object);

        if (is_container) {
                GUPnPDIDLLiteContainer *container;
                position = 0;
                icon = get_icon_by_id (ICON_CONTAINER);

                container = GUPNP_DIDL_LITE_CONTAINER (object);
                child_count = gupnp_didl_lite_container_get_child_count
                                                                (container);
        } else {
                position = -1;
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
                                           NULL, &parent_iter, position,
                                           0, icon,
                                           1, title,
                                           3, browse_data->content_dir,
                                           4, id,
                                           5, is_container,
                                           6, child_count,
                                           -1);
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

        content_dir = gupnp_device_info_get_service (info, CONTENT_DIR);

        return GUPNP_SERVICE_PROXY (content_dir);
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

        udn = gupnp_service_info_get_udn
                                (GUPNP_SERVICE_INFO (browse_data->content_dir));

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
browse_cb (GUPnPServiceProxy       *content_dir,
           GUPnPServiceProxyAction *action,
           gpointer                 user_data)
{
        BrowseData *data;
        char       *didl_xml;
        guint32     number_returned;
        guint32     total_matches;
        GError     *error;

        data = (BrowseData *) user_data;
        didl_xml = NULL;
        error = NULL;

        gupnp_service_proxy_end_action (content_dir,
                                        action,
                                        &error,
                                        /* OUT args */
                                        "Result",
                                        G_TYPE_STRING,
                                        &didl_xml,
                                        "NumberReturned",
                                        G_TYPE_UINT,
                                        &number_returned,
                                        "TotalMatches",
                                        G_TYPE_UINT,
                                        &total_matches,
                                        NULL);
        if (didl_xml) {
                GUPnPDIDLLiteParser *parser;
                gint32              remaining;
                gint32              batch_size;
                GError              *error;

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

                        browse (content_dir,
                                data->id,
                                data->starting_index,
                                batch_size);
                } else
                        update_container_child_count (content_dir, data->id);
        } else if (error) {
                GUPnPServiceInfo *info;

                info = GUPNP_SERVICE_INFO (content_dir);
                g_warning ("Failed to browse '%s': %s",
                           gupnp_service_info_get_location (info),
                           error->message);

                g_error_free (error);
        }

        browse_data_free (data);
}

static void
browse_metadata_cb (GUPnPServiceProxy       *content_dir,
                    GUPnPServiceProxyAction *action,
                    gpointer                 user_data)
{
        BrowseMetadataData *data;
        char               *metadata;
        GError             *error;

        data = (BrowseMetadataData *) user_data;

        metadata = NULL;
        error = NULL;

        gupnp_service_proxy_end_action (content_dir,
                                        action,
                                        &error,
                                        /* OUT args */
                                        "Result",
                                        G_TYPE_STRING,
                                        &metadata,
                                        NULL);
        if (metadata) {
                data->callback (metadata, data->user_data);

                g_free (metadata);
        } else if (error) {
                g_warning ("Failed to get metadata for '%s': %s",
                           data->id,
                           error->message);

                g_error_free (error);
        }

        browse_metadata_data_free (data);
        g_object_unref (content_dir);
}

static void
browse (GUPnPServiceProxy *content_dir,
        const char        *container_id,
        guint32            starting_index,
        guint32            requested_count)
{
        BrowseData *data;

        data = browse_data_new (content_dir,
                                container_id,
                                starting_index);

        gupnp_service_proxy_begin_action
                                (content_dir,
                                 "Browse",
                                 browse_cb,
                                 data,
                                 /* IN args */
                                 "ObjectID",
                                 G_TYPE_STRING,
                                 container_id,
                                 "BrowseFlag",
                                 G_TYPE_STRING,
                                 "BrowseDirectChildren",
                                 "Filter",
                                 G_TYPE_STRING,
                                 "@childCount",
                                 "StartingIndex",
                                 G_TYPE_UINT,
                                 starting_index,
                                 "RequestedCount",
                                 G_TYPE_UINT,
                                 requested_count,
                                 "SortCriteria",
                                 G_TYPE_STRING,
                                 "",
                                 NULL);
}

static void
browse_metadata (GUPnPServiceProxy *content_dir,
                 const char        *id,
                 MetadataFunc       callback,
                 gpointer           user_data)
{
        BrowseMetadataData *data;

        data = browse_metadata_data_new (callback, id, user_data);

        gupnp_service_proxy_begin_action
                                (g_object_ref (content_dir),
                                 "Browse",
                                 browse_metadata_cb,
                                 data,
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
}

static void
append_media_server (GUPnPDeviceProxy *proxy,
                     GtkTreeModel     *model,
                     GtkTreeIter      *parent_iter)
{
        GUPnPDeviceInfo   *info;
        GUPnPServiceProxy *content_dir;
        char              *friendly_name;

        info = GUPNP_DEVICE_INFO (proxy);

        content_dir = get_content_dir (proxy);
        friendly_name = gupnp_device_info_get_friendly_name (info);

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

                browse (content_dir, "0", 0, MAX_BROWSE);
                gupnp_service_proxy_add_notify (content_dir,
                                                "ContainerUpdateIDs",
                                                G_TYPE_STRING,
                                                on_container_update_ids,
                                                NULL);
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
                unschedule_icon_update (info);
                gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }
}

gboolean
get_selected_object (MetadataFunc callback,
                     gpointer     user_data)
{
        GUPnPServiceProxy  *content_dir;
        GtkTreeSelection   *selection;
        GtkTreeModel       *model;
        GtkTreeIter         iter;
        char               *id = NULL;
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
                            -1);

        browse_metadata (g_object_ref (content_dir), id, callback, user_data);

        ret = TRUE;

        if (id) {
                g_free (id);
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

