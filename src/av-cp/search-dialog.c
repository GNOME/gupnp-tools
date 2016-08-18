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

/* DLNA recommends something between 10 and 30, let's just use 30
 * cf. 7.4.1.4.10.9 in the 2014 version of the architecture and protocols
 * guideline
 * dlna-guidelines-march-2014---part-1-1-architectures-and-protocols.pdf
 */
#define SEARCH_DIALOG_DEFAULT_SLICE 30

typedef struct _SearchTask SearchTask;

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
        SearchTask *task;
        GUPnPSearchCriteriaParser *parser;
        GRegex *position_re;
};

typedef struct _SearchDialogPrivate SearchDialogPrivate;
G_DEFINE_TYPE_WITH_PRIVATE (SearchDialog, search_dialog, GTK_TYPE_DIALOG)

void
search_dialog_on_search_activate (SearchDialog *self, GtkEntry *entry);

static void
search_dialog_finalize (GObject *object);

static void
search_dialog_dispose (GObject *object);

struct _SearchTask {
        AVCPMediaServer *server;
        char *search_expression;
        char *container_id;
        GtkListStore *target;
        int start;
        int count;
        int total;
        gboolean running;
        GUPnPDIDLLiteParser *parser;
        GError *error;
        GSourceFunc callback;
        gpointer user_data;
        GCancellable *cancellable;
};

static void
search_task_on_didl_object_available (GUPnPDIDLLiteParser *parser,
                                      GUPnPDIDLLiteObject *object,
                                      gpointer             user_data);

static SearchTask *
search_task_new (AVCPMediaServer *server,
                 GtkListStore *target,
                 const char *container_id,
                 const char *search_expression,
                 int count,
                 GSourceFunc callback,
                 gpointer user_data)
{
        SearchTask *task = g_new0 (SearchTask, 1);

        task->search_expression = g_strdup (search_expression);
        task->target = g_object_ref (target);
        task->server = g_object_ref (server);
        task->container_id = g_strdup (container_id);
        task->start = 0;
        task->count = count;
        task->total = -1;
        task->parser = gupnp_didl_lite_parser_new ();
        task->error = NULL;
        task->callback = callback;
        task->user_data = user_data;
        task->cancellable = g_cancellable_new ();

        g_signal_connect (G_OBJECT (task->parser),
                          "object-available",
                          G_CALLBACK (search_task_on_didl_object_available),
                          task);

        return task;
}

static void
search_task_free (SearchTask *task) {
        g_clear_object (&task->target);
        g_clear_object (&task->server);
        g_clear_object (&task->parser);
        g_free (task->search_expression);
        g_free (task->container_id);
        if (task->error != NULL) {
                g_error_free (task->error);
        }
        g_free (task);
}

static void
search_task_cancel (SearchTask *task) {
        g_cancellable_cancel (task->cancellable);
}

static gboolean
search_task_idle_callback (gpointer user_data)
{
        SearchTask *task = (SearchTask *)user_data;
        if (task->callback != NULL) {
            task->callback (task->user_data);
        }

        return FALSE;
}

static void
search_task_set_finished (SearchTask *task, GError *error)
{
        task->running = FALSE;
        task->error = error;

        g_idle_add (search_task_idle_callback, task);
}

static void
search_task_on_search_ready (GObject *source, GAsyncResult *res, gpointer user_data)
{
        SearchTask *task  = (SearchTask *)user_data;
        GError *error = NULL;
        char *didl_xml = NULL;
        guint32 total = 0;
        guint32 returned = 0;
        gboolean result;
        gboolean finished = FALSE;

        result = av_cp_media_server_search_finish (AV_CP_MEDIA_SERVER (source),
                                                   res,
                                                   &didl_xml,
                                                   &total,
                                                   &returned,
                                                   &error);

        g_debug ("Received search slice result for %s with expression %s, result is %s",
                 task->container_id,
                 task->search_expression,
                 result ? "TRUE" : "FALSE");


        if (!result) {
                finished = TRUE;

                goto out;
        }

        if (g_cancellable_is_cancelled (task->cancellable)) {
                finished = TRUE;

                goto out;
        }

        /* Nothing returned by the server */
        if (returned == 0) {
                finished = TRUE;

                goto out;
        }

        if (didl_xml == NULL) {
                finished = TRUE;
                goto out;
        }

        gupnp_didl_lite_parser_parse_didl (task->parser, didl_xml, &error);
        if (error != NULL) {
                finished = TRUE;

                goto out;
        }

        if (total != 0) {
                task->total = total;
        }

        task->start += returned;

out:
        g_clear_pointer (&didl_xml, g_free);
        if (finished) {
                g_debug ("Finished search, error: %s",
                         error ? error->message : "none");
                search_task_set_finished (task, error);
        } else {
                g_debug ("Starting new slice %u/%u (total %u)",
                         task->start,
                         task->count,
                         task->total == -1 ? 0 : task->total);

                av_cp_media_server_search_async (task->server,
                                                 task->cancellable,
                                                 search_task_on_search_ready,
                                                 task->container_id,
                                                 task->search_expression,
                                                 task->start,
                                                 task->count,
                                                 task);
        }
}

static void
search_task_run (SearchTask *task) {
        if (task->running) {
                g_debug ("Search task is already running, not doing anything.");

                return;
        }

        g_debug ("Starting search task for %s with expression %s",
                 task->container_id,
                 task->search_expression);

        task->running = TRUE;

        av_cp_media_server_search_async (task->server,
                                         NULL,
                                         search_task_on_search_ready,
                                         task->container_id,
                                         task->search_expression,
                                         task->start,
                                         task->count,
                                         task);
}

#define ITEM_CLASS_IMAGE "object.item.imageItem"
#define ITEM_CLASS_AUDIO "object.item.audioItem"
#define ITEM_CLASS_VIDEO "object.item.videoItem"
#define ITEM_CLASS_TEXT  "object.item.textItem"
#define CONTAINER_CLASS "object.container"

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
        } else if (g_str_has_prefix (class_name, ITEM_CLASS_AUDIO)) {
                icon = get_icon_by_id (ICON_AUDIO_ITEM);
        } else if (g_str_has_prefix (class_name, ITEM_CLASS_VIDEO)) {
                icon = get_icon_by_id (ICON_VIDEO_ITEM);
        } else if (g_str_has_prefix (class_name, ITEM_CLASS_TEXT)) {
                icon = get_icon_by_id (ICON_TEXT_ITEM);
        } else if (g_str_has_prefix (class_name, CONTAINER_CLASS) ){
                icon = get_icon_by_id (ICON_CONTAINER);
        } else {
                icon = get_icon_by_id (ICON_FILE);
        }

        return icon;
}

static void
search_task_on_didl_object_available (GUPnPDIDLLiteParser *parser,
                                      GUPnPDIDLLiteObject *object,
                                      gpointer             user_data)
{
        SearchTask *task = (SearchTask *)user_data;
        GtkTreeIter iter;

        gtk_list_store_insert_with_values (task->target,
                                           &iter,
                                           -1,
                                           0, get_item_icon (object),
                                           1, gupnp_didl_lite_object_get_title (object),
                                           -1);
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
        object_class->dispose = search_dialog_dispose;
}

static void
search_dialog_init (SearchDialog *self)
{
        SearchDialogPrivate *priv = NULL;

        gtk_widget_init_template (GTK_WIDGET (self));
        priv = search_dialog_get_instance_private (self);

        priv->parser = gupnp_search_criteria_parser_new ();
}

static void
search_dialog_dispose (GObject *object)
{
        SearchDialog *self = SEARCH_DIALOG (object);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        GObjectClass *parent_class =
                              G_OBJECT_CLASS (search_dialog_parent_class);

        if (priv->task != NULL) {
                search_task_cancel (priv->task);
        }

        if (priv->pulse_timer != 0) {
                g_source_remove (priv->pulse_timer);
                priv->pulse_timer = 0;
        }

        g_clear_object (&priv->parser);

        if (parent_class->dispose != NULL) {
                parent_class->dispose (object);
        }
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
        g_clear_pointer (&priv->task, search_task_free);
        g_clear_pointer (&priv->position_re, g_regex_unref);

        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}

static gboolean
search_dialog_on_search_task_done (gpointer user_data)
{
        SearchDialog *self = SEARCH_DIALOG (user_data);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);

        g_source_remove (priv->pulse_timer);
        gtk_entry_set_progress_fraction (priv->search_dialog_entry, 0);
        gtk_widget_set_sensitive (GTK_WIDGET (priv->search_dialog_entry), TRUE);

        /* Only show visible error if dialog is visible. If it's not visible,
         * it's likely to be a cancelled error */
        if (priv->task->error != NULL &&
            gtk_widget_is_visible (GTK_WIDGET (self))) {
                GtkWidget *dialog = NULL;

                dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_CLOSE,
                                                 "%s",
                                                 _("Search failed"));

                gtk_message_dialog_format_secondary_text
                                    (GTK_MESSAGE_DIALOG (dialog),
                                     _("Error message was: %s"),
                                     priv->task->error->message);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);

                g_critical ("Failed to search: %s", priv->task->error->message);
        }

        g_clear_pointer (&priv->task, search_task_free);

        return FALSE;
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

void
search_dialog_run (SearchDialog *self)
{
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        gtk_dialog_run (GTK_DIALOG (self));
        gtk_widget_hide (GTK_WIDGET (self));

        if (priv->task != NULL &&
            priv->task->running) {
                search_task_cancel (priv->task);
        }
}

static gboolean
pulse_timer (gpointer user_data)
{
        SearchDialog *self = SEARCH_DIALOG (user_data);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);
        if (priv->task->total == -1) {
                gtk_entry_progress_pulse (GTK_ENTRY (priv->search_dialog_entry));
        } else {
                gdouble progress = (gdouble) priv->task->start /
                                   (gdouble) priv->task->total;
                gtk_entry_set_progress_fraction
                                    (GTK_ENTRY (priv->search_dialog_entry),
                                     progress);
        }

        return TRUE;
}

G_MODULE_EXPORT
void
search_dialog_on_search_activate (SearchDialog *self, GtkEntry *entry)
{
        GError *error = NULL;
        const char *text = gtk_entry_get_text (entry);
        SearchDialogPrivate *priv = search_dialog_get_instance_private (self);

        gupnp_search_criteria_parser_parse_text (priv->parser, text, &error);
        if (error == NULL) {
                gtk_list_store_clear (priv->search_dialog_liststore);
                gtk_widget_set_sensitive (GTK_WIDGET (entry), FALSE);
                priv->pulse_timer = g_timeout_add_seconds (1, pulse_timer, self);

                g_clear_pointer (&priv->task, search_task_free);

                priv->task = search_task_new (priv->server,
                                              priv->search_dialog_liststore,
                                              priv->id,
                                              gtk_entry_get_text (entry),
                                              SEARCH_DIALOG_DEFAULT_SLICE,
                                              search_dialog_on_search_task_done,
                                              self);
                search_task_run (priv->task);
        } else {
                GtkWidget *dialog = NULL;
                GMatchInfo *info = NULL;
                char *position = NULL;

                if (priv->position_re == NULL) {
                        priv->position_re = g_regex_new ("([0-9]+)$", 0, 0, NULL);
                }

                if (!g_regex_match (priv->position_re, error->message, 0, &info)) {
                        position = g_strdup ("-1");
                } else {
                        position = g_match_info_fetch (info, 0);
                }

                g_match_info_free (info);

                dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_CLOSE,
                                                 "%s",
                                                 _("Search failed"));

                gtk_message_dialog_format_secondary_text
                                    (GTK_MESSAGE_DIALOG (dialog),
                                     _("Search criteria invalid: %s"),
                                     error->message);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);

                g_error_free (error);
                error = NULL;

                gtk_editable_set_position (GTK_EDITABLE (entry),
                                           atoi (position));
                g_free (position);

                return;
        }

}
