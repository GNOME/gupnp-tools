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

static gboolean
protocol_equal_func (const gchar *protocol,
                     const gchar *uri,
                     const gchar *pattern)
{
        return g_pattern_match_simple (pattern, protocol);
}

char *
find_compatible_uri (gchar     **protocols,
                     GHashTable *resource_hash)
{
        guint i;
        char *uri = NULL;

        for (i = 0; protocols[i] && uri == NULL; i++) {
                uri = g_hash_table_find (resource_hash,
                                         (GHRFunc) protocol_equal_func,
                                         protocols[i]);
        }

        if (uri) {
                uri = g_strdup (uri);
        }

        return uri;
}

static void
set_state_cb (GUPnPServiceProxy       *av_transport,
              GUPnPServiceProxyAction *action,
              gpointer                 user_data)
{
        const char *state_name;
        GError *error;

        state_name = (const char *) user_data;

        error = NULL;
        if (!gupnp_service_proxy_end_action (av_transport,
                                             action,
                                             &error,
                                             NULL)) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to set state of '%s' to %s: %s",
                           udn,
                           state_name,
                           error->message);

                g_error_free (error);
        }

        g_object_unref (av_transport);
}

static void
play (void)
{
        GUPnPMediaRendererProxy *renderer;
        GUPnPServiceProxy       *av_transport;
        GError                  *error;

        renderer = get_selected_renderer (&av_transport, NULL);
        if (renderer == NULL) {
                g_warning ("No renderer selected");
                return;
        } else {
                g_object_unref (renderer);
        }

        error = NULL;
        gupnp_service_proxy_begin_action (av_transport,
                                          "Play",
                                          set_state_cb,
                                          "Playing",
                                          &error,
                                          "InstanceID", G_TYPE_UINT, 0,
                                          "Speed", G_TYPE_STRING, "1",
                                          NULL);
        if (error) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to Play '%s': %s",
                           udn,
                           error->message);

                g_error_free (error);
                g_object_unref (av_transport);
        }
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
play_item (char       *id,
           GHashTable *resource_hash)
{
        char                   **protocols;
        char                    *uri;
        GUPnPMediaRendererProxy *renderer;
        GUPnPServiceProxy       *av_transport;
        GError                  *error;

        renderer = get_selected_renderer (&av_transport,
                                          &protocols);
        if (renderer == NULL) {
                g_warning ("No renderer selected.");
                return;
        } else {
                g_object_unref (renderer);
        }

        uri = find_compatible_uri (protocols, resource_hash);
        g_strfreev (protocols);
        if (uri == NULL) {
                g_warning ("no compatible URI found.");
                g_object_unref (av_transport);
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
                char       *id;
                GHashTable *resource_hash;

                id = get_selected_item (&resource_hash);

                if (id != NULL) {
                        play_item (id, resource_hash);
                        g_free (id);
                }

                g_hash_table_unref (resource_hash);
        } else {
                play ();
        }
}

void
on_pause_button_clicked (GtkButton *button,
                         gpointer   user_data)
{
        GUPnPMediaRendererProxy *renderer;
        GUPnPServiceProxy       *av_transport;
        GError                  *error;

        renderer = get_selected_renderer (&av_transport, NULL);
        if (renderer == NULL) {
                g_warning ("No renderer selected.");
                return;
        } else {
                g_object_unref (renderer);
        }

        error = NULL;
        gupnp_service_proxy_begin_action (av_transport,
                                          "Pause",
                                          set_state_cb,
                                          "Paused",
                                          &error,
                                          "InstanceID", G_TYPE_UINT, 0,
                                          NULL);
        if (error) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to Pause '%s': %s",
                           udn,
                           error->message);

                g_error_free (error);
                g_object_unref (av_transport);
        }
}

void
on_stop_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        GUPnPMediaRendererProxy *renderer;
        GUPnPServiceProxy       *av_transport;
        GError                  *error;

        renderer = get_selected_renderer (&av_transport, NULL);
        if (renderer == NULL) {
                g_warning ("No renderer selected.");
                return;
        } else {
                g_object_unref (renderer);
        }

        error = NULL;
        gupnp_service_proxy_begin_action (av_transport,
                                          "Stop",
                                          set_state_cb,
                                          "Stopped",
                                          &error,
                                          "InstanceID", G_TYPE_UINT, 0,
                                          NULL);
        if (error) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to Pause '%s': %s",
                           udn,
                           error->message);

                g_error_free (error);
                g_object_unref (av_transport);
        }
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
