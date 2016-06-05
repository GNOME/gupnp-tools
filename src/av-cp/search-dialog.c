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

#include "search-dialog.h"
#include "server-device.h"

struct _SearchDialog {
        GtkDialog parent;
};

struct _SearchDialogClass {
        GtkDialogClass parent_class;
};

struct _SearchDialogPrivate {
        char *id;
        AVCPMediaServer *server;
};

typedef struct _SearchDialogPrivate SearchDialogPrivate;
G_DEFINE_TYPE_WITH_PRIVATE (SearchDialog, search_dialog, GTK_TYPE_DIALOG)

void
search_dialog_on_search_activate (SearchDialog *self, GtkEntry *entry);

static void
search_dialog_class_init (SearchDialogClass *klass)
{
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
}

static void
search_dialog_init (SearchDialog *self)
{
        gtk_widget_init_template (GTK_WIDGET (self));
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

        priv->id = id;
}

G_MODULE_EXPORT
void
search_dialog_on_search_activate (SearchDialog *self, GtkEntry *entry)
{
        g_print ("==> %s\n", gtk_entry_get_text (entry));
}
