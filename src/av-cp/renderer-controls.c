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
#include <gtk/gtk.h>

#include "playlist-treeview.h"
#include "renderer-controls.h"
#include "renderer-combo.h"

#define SEC_PER_MIN 60
#define SEC_PER_HOUR 3600

static GUPnPDIDLLiteParser *didl_parser;

GtkWidget *volume_vscale;
GtkWidget *position_hscale;
GtkWidget *play_button;
GtkWidget *pause_button;
GtkWidget *stop_button;
GtkWidget *next_button;
GtkWidget *prev_button;
GtkWidget *lenient_mode_menuitem;

static guint timeout_id;

typedef struct
{
  GCallback callback;

  GUPnPDIDLLiteResource *resource;
} SetAVTransportURIData;

static SetAVTransportURIData *
set_av_transport_uri_data_new (GCallback              callback,
                               GUPnPDIDLLiteResource *resource)
{
        SetAVTransportURIData *data;

        data = g_slice_new (SetAVTransportURIData);

        data->callback = callback;
        data->resource = resource; /* Steal the ref */

        return data;
}

static void
set_av_transport_uri_data_free (SetAVTransportURIData *data)
{
        g_object_unref (data->resource);
        g_slice_free (SetAVTransportURIData, data);
}

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

void
av_transport_send_action (char *action,
                          char *additional_args[])
{
        GUPnPServiceProxy *av_transport;
        GHashTable        *args;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");
                return;
        }

        args = create_av_transport_args_hash (additional_args);

        gupnp_service_proxy_begin_action_hash (av_transport,
                                               action,
                                               av_transport_action_cb,
                                               (char *) action,
                                               args);
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
        SetAVTransportURIData *data;
        GError                *error;

        data = (SetAVTransportURIData *) user_data;

        error = NULL;
        if (gupnp_service_proxy_end_action (av_transport,
                                            action,
                                            &error,
                                            NULL)) {
                long duration ;
                if (data->callback) {
                        data->callback ();
                }

                duration = gupnp_didl_lite_resource_get_duration
                                                        (data->resource);
                if (duration > 0) {
                        gtk_range_set_range (GTK_RANGE (position_hscale),
                                             0.0,
                                             duration);
                }
        } else {
                const char *udn;

                udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));

                g_warning ("Failed to set URI '%s' on %s: %s",
                           gupnp_didl_lite_resource_get_uri (data->resource),
                           udn,
                           error->message);

                g_error_free (error);
        }

        set_av_transport_uri_data_free (data);
        g_object_unref (av_transport);
}

gboolean
on_volume_vscale_value_changed (GtkRange *range,
                                gpointer  user_data);

void
set_volume_hscale (guint volume)
{
        g_signal_handlers_block_by_func (volume_vscale,
                                         on_volume_vscale_value_changed,
                                         NULL);

        gtk_range_set_value (GTK_RANGE (volume_vscale), volume);

        g_signal_handlers_unblock_by_func (volume_vscale,
                                           on_volume_vscale_value_changed,
                                           NULL);
}

static void
on_didl_item_available (GUPnPDIDLLiteParser *didl_parser,
                        xmlNode             *item_node,
                        gpointer             user_data)
{
        GUPnPDIDLLiteResource **resource;
        GUPnPServiceProxy      *av_transport;
        char                   *sink_protocol_info;
        gboolean                lenient_mode;

        resource = (GUPnPDIDLLiteResource **) user_data;

        av_transport = get_selected_av_transport (&sink_protocol_info);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");

                return;
        }

        lenient_mode = gtk_check_menu_item_get_active
                                (GTK_CHECK_MENU_ITEM (lenient_mode_menuitem));

        *resource = gupnp_didl_lite_object_get_compat_resource
                                                        (item_node,
                                                         sink_protocol_info,
                                                         lenient_mode);
        g_free (sink_protocol_info);
        g_object_unref (av_transport);
}

static GUPnPDIDLLiteResource *
find_compat_res_from_metadata (const char *metadata)
{
        GUPnPDIDLLiteResource *resource;
        GError                *error;

        resource = NULL;
        error = NULL;

        /* Assumption: metadata only contains a single didl object */
        gupnp_didl_lite_parser_parse_didl (didl_parser,
                                           metadata,
                                           on_didl_item_available,
                                           &resource,
                                           &error);
        if (error) {
                g_warning ("%s\n", error->message);

                g_error_free (error);
        }

        return resource;
}

void
set_av_transport_uri (const char *metadata,
                      GCallback   callback)
{
        GUPnPServiceProxy     *av_transport;
        SetAVTransportURIData *data;
        GUPnPDIDLLiteResource *resource;
        const char            *uri;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");
                return;
        }

        resource = find_compat_res_from_metadata (metadata);
        if (resource == NULL) {
                g_warning ("no compatible URI found.");

                g_object_unref (av_transport);

                return;
        }

        data = set_av_transport_uri_data_new (callback, resource);
        uri = gupnp_didl_lite_resource_get_uri (resource);

        gupnp_service_proxy_begin_action (av_transport,
                                          "SetAVTransportURI",
                                          set_av_transport_uri_cb,
                                          data,
                                          "InstanceID",
                                          G_TYPE_UINT,
                                          0,
                                          "CurrentURI",
                                          G_TYPE_STRING,
                                          uri,
                                          "CurrentURIMetaData",
                                          G_TYPE_STRING,
                                          metadata,
                                          NULL);
}

void
on_play_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        PlaybackState state;

        state = get_selected_renderer_state ();

        if (state == PLAYBACK_STATE_STOPPED ||
            state == PLAYBACK_STATE_UNKNOWN) {
                get_selected_item ((GetSelectedItemCallback)
                                   set_av_transport_uri,
                                   play);
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
        select_next_object ();
}

void
on_previous_button_clicked (GtkButton *button,
                            gpointer   user_data)
{
        select_prev_object ();
}

void
on_clear_state_button_clicked (GtkButton *button,
                               gpointer   user_data)
{
        clear_selected_renderer_state ();
}

gboolean
on_position_hscale_value_changed (GtkRange *range,
                                  gpointer  user_data)
{
        char *args[] = { "Unit", "ABS_TIME", "Target", NULL, NULL };
        guint total_secs;
        guint hours;
        guint minutes;
        guint seconds;

        total_secs = (guint) gtk_range_get_value (range);
        hours = total_secs / 3600;
        minutes = (total_secs / 60) - (hours * 60);
        seconds = total_secs - (hours * 3600) - (minutes * 60);

        args[3] = g_strdup_printf ("%u:%02u:%02u",
                                   hours,
                                   minutes,
                                   seconds);

        if (args[3]) {
                av_transport_send_action ("Seek", args);
                g_free (args[3]);
        }

        return TRUE;
}

static gdouble
seconds_from_time (const char *time_str)
{
        char **tokens;
        gdouble seconds = -1.0;

        tokens = g_strsplit (time_str, ":", -1);
        if (tokens == NULL) {
                return -1.0;
        }

        if (tokens[0] == NULL ||
            tokens[1] == NULL ||
            tokens[2] == NULL) {
                goto return_point;
        }

        seconds = g_strtod (tokens[2], NULL);
        seconds += g_strtod (tokens[1], NULL) * SEC_PER_MIN;
        seconds += g_strtod (tokens[0], NULL) * SEC_PER_HOUR;

return_point:
        g_strfreev (tokens);

        return seconds;
}

void
set_position_hscale_duration (const char *duration_str)
{
        gdouble duration;

        duration = seconds_from_time (duration_str);
        if (duration > 0.0) {
                gtk_range_set_range (GTK_RANGE (position_hscale),
                                     0.0,
                                     duration);
        }
}

void
set_position_hscale_position (const char *position_str)
{
        gdouble position;

        position = seconds_from_time (position_str);
        if (position >= 0.0) {
                g_signal_handlers_block_by_func
                                        (position_hscale,
                                         on_position_hscale_value_changed,
                                         NULL);

                gtk_range_set_value (GTK_RANGE (position_hscale), position);

                g_signal_handlers_unblock_by_func
                                        (position_hscale,
                                         on_position_hscale_value_changed,
                                         NULL);
        }
}

static void
get_position_info_cb (GUPnPServiceProxy       *av_transport,
                      GUPnPServiceProxyAction *action,
                      gpointer                 user_data)
{
        gchar       *position;
        const gchar *udn;
        GError      *error;

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (av_transport));

        error = NULL;
        if (!gupnp_service_proxy_end_action (av_transport,
                                             action,
                                             &error,
                                             "AbsTime",
                                             G_TYPE_STRING,
                                             &position,
                                             NULL)) {
                g_warning ("Failed to get current media position"
                           "from media renderer '%s':%s\n",
                           udn,
                           error->message);
                g_error_free (error);

                goto return_point;
        }

        set_position_hscale_position (position);

return_point:
        g_object_unref (av_transport);
}

static gboolean
update_position (gpointer data)
{
        GUPnPServiceProxy *av_transport;
        const gchar       *udn;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                return FALSE;
        }

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (av_transport));

        gupnp_service_proxy_begin_action (av_transport,
                                          "GetPositionInfo",
                                          get_position_info_cb,
                                          NULL,
                                          "InstanceID", G_TYPE_UINT, 0,
                                          NULL);
        return TRUE;
}

static void
add_timeout (void)
{
        if (timeout_id == 0) {
                timeout_id = g_timeout_add (1000,
                                            update_position,
                                            NULL);
        }
}

static void
remove_timeout (void)
{
        if (timeout_id != 0) {
                g_source_remove (timeout_id);
                timeout_id = 0;
        }
}

void
prepare_controls_for_state (PlaybackState state)
{
        gboolean play_possible;
        gboolean pause_possible;
        gboolean stop_possible;

        switch (state) {
        case PLAYBACK_STATE_STOPPED:
                play_possible = TRUE;
                pause_possible = TRUE;
                stop_possible = FALSE;

                remove_timeout ();

                break;

        case PLAYBACK_STATE_PAUSED:
                play_possible = TRUE;
                pause_possible = FALSE;
                stop_possible = TRUE;

                remove_timeout ();

                update_position (NULL);

                break;

        case PLAYBACK_STATE_PLAYING:
                play_possible = FALSE;
                pause_possible = TRUE;
                stop_possible = TRUE;

                /* Start tracking media position in playing state */
                add_timeout ();

                break;

        case PLAYBACK_STATE_TRANSITIONING:
                play_possible = FALSE;
                pause_possible = FALSE;
                stop_possible = FALSE;

                remove_timeout ();

                break;

       default:
                play_possible = TRUE;
                pause_possible = TRUE;
                stop_possible = TRUE;

                remove_timeout ();

                break;
        }

        /* Disable the seekbar when the state is stopped */
        gtk_widget_set_sensitive (position_hscale, stop_possible);
        if (!stop_possible) {
                g_signal_handlers_block_by_func
                                        (position_hscale,
                                         on_position_hscale_value_changed,
                                         NULL);

                gtk_range_set_value (GTK_RANGE (position_hscale), 0.0);

                g_signal_handlers_unblock_by_func
                                        (position_hscale,
                                         on_position_hscale_value_changed,
                                         NULL);
        }

        gtk_widget_set_sensitive (play_button, play_possible);
        gtk_widget_set_sensitive (pause_button, pause_possible);
        gtk_widget_set_sensitive (stop_button, stop_possible);
}

static void
set_volume_cb (GUPnPServiceProxy       *rendering_control,
               GUPnPServiceProxyAction *action,
               gpointer                 user_data)
{
        GError *error;

        error = NULL;
        if (!gupnp_service_proxy_end_action (rendering_control,
                                             action,
                                             &error,
                                             NULL)) {
                const char *udn;

                udn = gupnp_service_info_get_udn
                        (GUPNP_SERVICE_INFO (rendering_control));

                g_warning ("Failed to set volume of %s: %s",
                           udn,
                           error->message);

                g_error_free (error);

                /* Update the range according to the current volume */
                set_volume_hscale (get_selected_renderer_volume ());
        }

        g_object_unref (rendering_control);
}

gboolean
on_volume_vscale_value_changed (GtkRange *range,
                                gpointer  user_data)
{
        GUPnPServiceProxy *rendering_control;
        guint              desired_volume;

        rendering_control = get_selected_rendering_control ();
        if (rendering_control == NULL) {
                g_warning ("No renderer selected");
                return FALSE;
        }

        desired_volume = (guint) gtk_range_get_value (range);

        gupnp_service_proxy_begin_action (rendering_control,
                                          "SetVolume",
                                          set_volume_cb,
                                          NULL,
                                          "InstanceID",
                                          G_TYPE_UINT,
                                          0,
                                          "Channel",
                                          G_TYPE_STRING,
                                          "Master",
                                          "DesiredVolume",
                                          G_TYPE_UINT,
                                          desired_volume,
                                          NULL);

        return TRUE;
}

void
setup_renderer_controls (GtkBuilder *builder)
{
        volume_vscale = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "volume-vscale"));
        g_assert (volume_vscale != NULL);

        position_hscale = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "position-hscale"));
        g_assert (position_hscale != NULL);

        play_button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "play-button"));
        g_assert (play_button != NULL);

        pause_button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "pause-button"));
        g_assert (pause_button != NULL);

        stop_button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "stop-button"));
        g_assert (stop_button != NULL);

        next_button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "next-button"));
        g_assert (next_button != NULL);

        prev_button = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "previous-button"));
        g_assert (prev_button != NULL);

        lenient_mode_menuitem = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "lenient_mode_menuitem"));
        g_assert (lenient_mode_menuitem != NULL);

        didl_parser = gupnp_didl_lite_parser_new ();
        g_assert (didl_parser != NULL);
        timeout_id = 0;

        g_object_weak_ref (G_OBJECT (position_hscale),
                           (GWeakNotify) remove_timeout,
                           NULL);
}

