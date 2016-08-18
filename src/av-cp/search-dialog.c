/*
 * Copyright (C) 2016 Jens Georg <mail@jensge.org>
 *
 * Authors: Jens Georg <mail@jensge.org>
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

#include <glib/gi18n.h>
#include <libgupnp-av/gupnp-av.h>

#include <string.h>

#include "search-dialog.h"
#include "server-device.h"
#include "icons.h"

struct _SearchDialog {
        GtkDialog parent;
};

struct _SearchDialogClass {
        GtkDialogClass parent_class;
};

struct _SearchDialogPrivate {
        GtkListStore *search_dialog_liststore;
        GtkEntry *search_dialog_entry;
        char *id;
        char *title;
        AVCPMediaServer *server;
        guint pulse_timer;
};

typedef struct _SearchDialogPrivate SearchDialogPrivate;
G_DEFINE_TYPE_WITH_PRIVATE (SearchDialog, search_dialog, GTK_TYPE_DIALOG)

void
search_dialog_on_search_activate (SearchDialog *self, GtkEntry *entry);

static void
search_dialog_finalize (GObject *object);

#define ITEM_CLASS_IMAGE "object.item.imageItem"
#define ITEM_CLASS_AUDIO "object.item.audioItem"
#define ITEM_CLASS_VIDEO "object.item.videoItem"
#define ITEM_CLASS_TEXT  "object.item.textItem"

static GdkPixbuf *
get_item_icon (GUPnPDIDLLiteObject *object)
{
        GdkPixbuf  *icon;
        const char *class_name;

        class_name = gupnp_didl_lite_object_get_upnp_class (object);
        if (G_UNLIKELY (class_name == NULL)) {
                return get_icon_by_id (ICON_FILE);
        }

        if (g_str_has_prefix (class_name, ITEM_CLASS_IMAGE)) {
                icon = get_icon_by_id (ICON_IMAGE_ITEM);
        } else if (g_str_has_prefix (class_name,
                                    ITEM_CLASS_AUDIO)) {
                icon = get_icon_by_id (ICON_AUDIO_ITEM);
        } else if (g_str_has_prefix (class_name,
                                     ITEM_CLASS_VIDEO)) {
                icon = get_icon_by_id (ICON_VIDEO_ITEM);
        } else if (g_str_has_prefix (class_name,
                                     ITEM_CLASS_TEXT)) {
                icon = get_icon_by_id (ICON_TEXT_ITEM);
        } else {
                icon = get_icon_by_id (ICON_FILE);
        }

        return icon;
}


static void
search_dialog_class_init (SearchDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GError *error = NULL;
        gchar *data = NULL;
        gsize size = -1;
        GBytes *bytes;

        g_file_get_contents (DATA_DIR "/search-dialog.ui", &data, &size, &error);
        if (error != NULL) {
                g_critical ("Failed to load ui file: %s", error->message);
                g_error_free (error);

                return;
        }

        bytes = g_bytes_new_take (data, size);
        gtk_widget_class_set_template (widget_class, bytes);
        g_bytes_unref (bytes);

        gtk_widget_class_bind_template_child_private (widget_class,
                                                      SearchDialog,
                                                      search_dialog_liststore);
        gtk_widget_class_bind_template_child_private (widget_class,
                                                      SearchDialog,
                                                      search_dialog_entry);

        object_class->finalize = search_dialog_finalize;
}

static void
search_dialog_init (SearchDialog *self)
{
        gtk_widget_init_template (GTK_WIDGET (self));
}

static void
search_dialog_finalize (GObject *object)
{
        SearchDialog *self = SEARCH_DIALOG (object);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        GObjectClass *parent_class =
                              G_OBJECT_CLASS (search_dialog_parent_class);

        g_clear_pointer (&priv->id, g_free);
        g_clear_pointer (&priv->title, g_free);

        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}

static void
on_didl_object_available (GUPnPDIDLLiteParser *parser,
                          GUPnPDIDLLiteObject *object,
                          gpointer             user_data)
{
        SearchDialog *self = SEARCH_DIALOG (user_data);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        GtkTreeIter iter;

        gtk_list_store_insert_with_values (priv->search_dialog_liststore,
                                           &iter,
                                           -1,
                                           0, get_item_icon (object),
                                           1, gupnp_didl_lite_object_get_title (object),
                                           -1);

}

static void
search_dialog_on_search_done (GObject *source, GAsyncResult *res, gpointer user_data)
{
        SearchDialog *self = SEARCH_DIALOG (user_data);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);

        GError *error = NULL;
        char *xml = NULL;
        guint32 total = 0;
        guint32 returned = 0;
        GUPnPDIDLLiteParser *parser;


        if (!av_cp_media_server_search_finish (AV_CP_MEDIA_SERVER (source),
                                               res,
                                               &xml,
                                               &total,
                                               &returned,
                                               &error)) {
                GtkWidget *dialog = NULL;

                dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_CLOSE,
                                                 "Search failed: %s",
                                                 error->message);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);

                g_critical ("Failed to search: %s", error->message),
                g_error_free (error);

                goto out;
        }

        parser = gupnp_didl_lite_parser_new ();
        g_signal_connect (G_OBJECT (parser), "object-available",
                G_CALLBACK (on_didl_object_available),
                self);
        gupnp_didl_lite_parser_parse_didl (parser, xml, &error);
        g_free (xml);

out:
        g_clear_object (&parser);
        g_source_remove (priv->pulse_timer);
        gtk_entry_set_progress_fraction (priv->search_dialog_entry, 0.0);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->search_dialog_entry), TRUE);
}

void
search_dialog_set_server (SearchDialog *self, AVCPMediaServer *server)
{
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);

        priv->server = server;
}

void
search_dialog_set_container_id (SearchDialog *self, char *id)
{
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        g_free (priv->id);

        priv->id = id;
}

void
search_dialog_set_container_title (SearchDialog *self, char *title)
{
        char *name = NULL;
        char *window_title = NULL;

        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        g_free (priv->title);

        priv->title = title;
        name = gupnp_device_info_get_friendly_name
                                (GUPNP_DEVICE_INFO (priv->server));

        if (g_str_equal (priv->id, "0")) {
                window_title = g_strdup_printf (_("Searching on %s"),
                                         name);
        } else {
                window_title = g_strdup_printf (_("Searching in %s on %s"),
                                         title,
                                         name);
        }

        gtk_window_set_title (GTK_WINDOW (self), window_title);

        g_free (name);
        g_free (window_title);
}

static gboolean
pulse_timer (gpointer user_data)
{
        gtk_entry_progress_pulse (GTK_ENTRY (user_data));

        return TRUE;
}

G_MODULE_EXPORT
void
search_dialog_on_search_activate (SearchDialog *self, GtkEntry *entry)
{
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        gtk_list_store_clear (priv->search_dialog_liststore);
        gtk_widget_set_sensitive (GTK_WIDGET (entry), FALSE);
        priv->pulse_timer = g_timeout_add_seconds (1, pulse_timer, entry);

        av_cp_media_server_search_async (priv->server,
                                         NULL,
                                         search_dialog_on_search_done,
                                         priv->id,
                                         gtk_entry_get_text (entry),
                                         0,
                                         0,
                                         self);
}
