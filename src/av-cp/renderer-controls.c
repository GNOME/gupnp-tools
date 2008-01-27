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
#include <gtk/gtk.h>

#include "playlist-treeview.h"
#include "renderer-combo.h"

/* FIXME: display a dialog to report problems? */

static void
av_transport_action_cb (GUPnPServiceProxy       *av_transport,
                        GUPnPServiceProxyAction *action,
                        gpointer                 user_data)
{
        const char *action_name;
        GError *error;

        action_name = (const char *) user_data;

        error = NULL;
        if (!gupnp_service_proxy_end_action (av_transport,
                                             action,
                                             &error,
                                             NULL)) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to send action '%s' to '%s': %s",
                           action_name,
                           udn,
                           error->message);

                g_error_free (error);
        }

        g_object_unref (av_transport);
}

static void
g_value_free (gpointer data)
{
       g_value_unset ((GValue *) data);
       g_slice_free (GValue, data);
}

static GHashTable *
create_av_transport_args_hash (char **additional_args)
{
        GHashTable *args;
        GValue     *instance_id;
        gint        i;

        args = g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      NULL,
                                      g_value_free);

        instance_id = g_slice_alloc0 (sizeof (GValue));
        g_value_init (instance_id, G_TYPE_UINT);
        g_value_set_uint (instance_id, 0);

        g_hash_table_insert (args, "InstanceID", instance_id);

        if (additional_args != NULL) {
                for (i = 0; additional_args[i]; i += 2) {
                        GValue *value;

                        value = g_slice_alloc0 (sizeof (GValue));
                        g_value_init (value, G_TYPE_STRING);
                        g_value_set_string (value, additional_args[i + 1]);

                        g_hash_table_insert (args, additional_args[i], value);
                }
        }

        return args;
}

static void
av_transport_send_action (char *action,
                          char *additional_args[])
{
        GUPnPServiceProxy *av_transport;
        GHashTable        *args;
        GError            *error;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");
                return;
        }

        args = create_av_transport_args_hash (additional_args);

        error = NULL;
        gupnp_service_proxy_begin_action_hash (av_transport,
                                               action,
                                               av_transport_action_cb,
                                               (char *) action,
                                               &error,
                                               args);
        if (error) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to send action '%s' to '%s': %s",
                           action,
                           udn,
                           error->message);

                g_error_free (error);
                g_object_unref (av_transport);
        }

        g_hash_table_unref (args);
}

static void
play (void)
{
        char *args[] = { "Speed", "1", NULL };

        av_transport_send_action ("Play", args);
}

static void
set_av_transport_uri_cb (GUPnPServiceProxy       *av_transport,
                         GUPnPServiceProxyAction *action,
                         gpointer                 user_data)
{
        char   *uri;
        GError *error;

        uri = (char *) user_data;

        error = NULL;
        if (gupnp_service_proxy_end_action (av_transport,
                                            action,
                                            &error,
                                            NULL)) {
                play ();
        } else {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to set URI '%s' on %s: %s",
                           uri,
                           udn,
                           error->message);

                g_error_free (error);
        }

        g_free (uri);
        g_object_unref (av_transport);
}

static void
play_item (char *id, char *uri)
{
        GUPnPServiceProxy *av_transport;
        GError            *error;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");
                return;
        }

        error = NULL;
        gupnp_service_proxy_begin_action (av_transport,
                                          "SetAVTransportURI",
                                          set_av_transport_uri_cb,
                                          uri,
                                          &error,
                                          "InstanceID",
                                          G_TYPE_UINT,
                                          0,
                                          "CurrentURI",
                                          G_TYPE_STRING,
                                          uri,
                                          "CurrentURIMetaData",
                                          G_TYPE_STRING,
                                          "",
                                          NULL);
        if (error) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to set URI '%s' on %s: %s",
                           uri,
                           udn,
                           error->message);

                g_error_free (error);
                g_free (uri);
                g_object_unref (av_transport);
        }
}

void
on_play_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        PlaybackState state;

        state = get_selected_renderer_state ();

        if (state == PLAYBACK_STATE_STOPPED ||
            state == PLAYBACK_STATE_UNKNOWN) {
                char *id;
                char *uri;

                id = get_selected_item (&uri);

                if (id != NULL) {
                        play_item (id, uri);

                        g_free (id);
                }
        } else {
                play ();
        }
}

void
on_pause_button_clicked (GtkButton *button,
                         gpointer   user_data)
{
        av_transport_send_action ("Pause", NULL);
}

void
on_stop_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        av_transport_send_action ("Stop", NULL);
}

void
on_next_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
}

void
on_previous_button_clicked (GtkButton *button,
                            gpointer   user_data)
{
}

gboolean
on_position_hscale_change_value (GtkRange *range,
                                 GtkScrollType scroll,
                                 gdouble       value,
                                 gpointer      user_data)
{
        return TRUE;
}

gboolean
on_volume_vscale_change_value (GtkRange *range,
                               GtkScrollType scroll,
                               gdouble       value,
                               gpointer      user_data)
{
        return TRUE;
}
