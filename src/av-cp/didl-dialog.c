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

#include <config.h>

#include "didl-dialog.h"
#include "pretty-print.h"

#include <gtk/gtk.h>

#ifdef HAVE_GTK_SOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

#define DIALOG_RESOURCE_PATH "/org/gupnp/Tools/AV-CP/didl-lite-dialog.ui"

struct _AVCPDidlDialog {
        GtkDialog parent;
};

struct _AVCPDidlDialogClass {
        GtkDialogClass parent_class;
};

struct _AVCPDidlDialogPrivate {
    GtkWidget *didl_textview;
};
typedef struct _AVCPDidlDialogPrivate AVCPDidlDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AVCPDidlDialog, av_cp_didl_dialog, GTK_TYPE_DIALOG)

/* GObject overrides */
static void av_cp_didl_dialog_constructed (GObject *object);

static void
av_cp_didl_dialog_class_init (AVCPDidlDialogClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        gtk_widget_class_set_template_from_resource (widget_class,
                                                     DIALOG_RESOURCE_PATH);

        gtk_widget_class_bind_template_child_private (widget_class,
                                                      AVCPDidlDialog,
                                                      didl_textview);

        object_class->constructed = av_cp_didl_dialog_constructed;
}

static void
av_cp_didl_dialog_init (AVCPDidlDialog *self)
{
        gtk_widget_init_template (GTK_WIDGET (self));
}

static void
av_cp_didl_dialog_constructed (GObject *object)
{
        GObjectClass *parent_class = NULL;

        parent_class = G_OBJECT_CLASS (av_cp_didl_dialog_parent_class);
        if (parent_class->constructed != NULL) {
            parent_class->constructed (object);
        }

        g_signal_connect (object, "delete-event",
                          G_CALLBACK (gtk_widget_hide_on_delete), NULL);

#ifdef HAVE_GTK_SOURCEVIEW
        {
                AVCPDidlDialog *self = AV_CP_DIDL_DIALOG (object);
                AVCPDidlDialogPrivate *priv;

                priv = av_cp_didl_dialog_get_instance_private (self);

                GtkSourceLanguageManager *manager =
                                gtk_source_language_manager_get_default ();
                GtkSourceLanguage *language =
                    gtk_source_language_manager_guess_language (manager,
                                                                NULL,
                                                                "text/xml");

                GtkSourceBuffer *buffer = gtk_source_buffer_new_with_language (language);
                gtk_source_buffer_set_highlight_syntax (buffer, TRUE);
                gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (priv->didl_textview),
                                                       TRUE);

                gtk_text_view_set_buffer (GTK_TEXT_VIEW (priv->didl_textview),
                                          GTK_TEXT_BUFFER (buffer));
        }
#endif
}

AVCPDidlDialog *
av_cp_didl_dialog_new (void)
{
        GtkSettings *settings = gtk_settings_get_default ();
        int use_header;

        g_object_get (G_OBJECT (settings),
                                "gtk-dialogs-use-header",
                                &use_header,
                                NULL);

        return g_object_new (AV_CP_TYPE_DIDL_DIALOG,
                             "use-header-bar", use_header == 1 ? TRUE : FALSE,
                             NULL);
}

void
av_cp_didl_dialog_set_xml (AVCPDidlDialog *self, const char *xml)
{
        GtkTextBuffer *buffer;
        char *formatted;
        AVCPDidlDialogPrivate *priv;

        priv = av_cp_didl_dialog_get_instance_private (self);

        formatted = pretty_print_xml (xml);

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->didl_textview));
        gtk_text_buffer_set_text (buffer, formatted, -1);
        g_free (formatted);
}
