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

#include <gtk/gtk.h>
#include <gmodule.h>

#include "playlist-treeview.h"
#include "renderer-controls.h"
#include "renderer-combo.h"

#define SEC_PER_MIN 60
#define SEC_PER_HOUR 3600

GtkWidget *volume_scale;
GtkWidget *position_scale;
GtkWidget *play_button;
GtkWidget *pause_button;
GtkWidget *stop_button;
GtkWidget *next_button;
GtkWidget *prev_button;
GtkWidget *lenient_mode_menuitem;

static guint timeout_id;

void
on_play_button_clicked (GtkButton *button,
                        gpointer   user_data);
void
on_pause_button_clicked (GtkButton *button,
                         gpointer   user_data);

void
on_stop_button_clicked (GtkButton *button,
                        gpointer   user_data);

void
on_next_button_clicked (GtkButton *button,
                        gpointer   user_data);

void
on_previous_button_clicked (GtkButton *button,
                            gpointer   user_data);

void
on_clear_state_button_clicked (GtkButton *button,
                               gpointer   user_data);

gboolean
on_position_scale_value_changed (GtkRange *range,
                                 gpointer  user_data);

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
av_transport_action_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        const char *action_name;
        GError *error = NULL;
        GUPnPServiceProxyAction *action;
        const char *udn;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        action_name = (const char *) user_data;

        error = NULL;
        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));
        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        if (error != NULL) {
                g_warning ("Failed to send action '%s' to '%s': %s",
                           action_name,
                           udn,
                           error->message);

                goto out;
        }

        if (!gupnp_service_proxy_action_get_result (action, &error, NULL)) {

                g_warning ("Failed to send action '%s' to '%s': %s",
                           action_name,
                           udn,
                           error->message);
        }

out:
        g_clear_error (&error);
        g_object_unref (object);
}

static void
g_value_free (gpointer data)
{
       g_value_unset ((GValue *) data);
       g_slice_free (GValue, data);
}

static GList *
create_av_transport_args (char **additional_args, GList **out_values)
{
        GValue     *instance_id;
        gint        i;
        GList      *names = NULL, *values = NULL;

        instance_id = g_slice_alloc0 (sizeof (GValue));
        g_value_init (instance_id, G_TYPE_UINT);
        g_value_set_uint (instance_id, 0);

        names = g_list_append (names, g_strdup ("InstanceID"));
        values = g_list_append (values, instance_id);


        if (additional_args != NULL) {
                for (i = 0; additional_args[i]; i += 2) {
                        GValue *value;

                        value = g_slice_alloc0 (sizeof (GValue));
                        g_value_init (value, G_TYPE_STRING);
                        g_value_set_string (value, additional_args[i + 1]);
                        names = g_list_append (names,
                                               g_strdup (additional_args[i]));
                        values = g_list_append (values, value);
                }
        }

        *out_values = values;

        return names;
}

void
av_transport_send_action (const char *action_name, char *additional_args[])
{
        GUPnPServiceProxy *av_transport;
        GList             *names, *values;
        GUPnPServiceProxyAction *action;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                g_warning ("No renderer selected");
                return;
        }

        names = create_av_transport_args (additional_args, &values);
        action = gupnp_service_proxy_action_new_from_list (action_name,
                                                           names,
                                                           values);

        gupnp_service_proxy_call_action_async (av_transport,
                                               action,
                                               NULL,
                                               av_transport_action_cb,
                                               (char *) action_name);

        gupnp_service_proxy_action_unref (action);
        g_list_free_full (names, g_free);
        g_list_free_full (values, g_value_free);
}

static void
play (void)
{
        const char *args[] = { "Speed", "1", NULL };

        av_transport_send_action ("Play", (char **) args);
}

static void
set_av_transport_uri_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        SetAVTransportURIData *data = (SetAVTransportURIData *) user_data;
        GError *error = NULL;
        const char *udn;

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));

        GUPnPServiceProxyAction *action;
        action = gupnp_service_proxy_call_action_finish (
                GUPNP_SERVICE_PROXY (object),
                res,
                &error);

        // The above call will only catch issues with the HTTP transport, not
        // with the call itself. So we need to call an empty get on the action
        // as well to get any SOAP error
        if (error == NULL) {
                gupnp_service_proxy_action_get_result (action, &error, NULL);
        }
        if (error != NULL) {
                g_warning ("Failed to set URI '%s' on %s: %s",
                           gupnp_didl_lite_resource_get_uri (data->resource),
                           udn,
                           error->message);
        } else {
                long duration ;
                if (data->callback) {
                        data->callback ();
                }

                duration = gupnp_didl_lite_resource_get_duration
                                                        (data->resource);
                if (duration > 0) {
                        gtk_range_set_range (GTK_RANGE (position_scale),
                                             0.0,
                                             duration);
                }
        }

        g_clear_error (&error);
        set_av_transport_uri_data_free (data);
        g_object_unref (object);
}

G_MODULE_EXPORT
gboolean
on_volume_scale_value_changed (GtkRange *range,
                               gpointer  user_data);

void
set_volume_scale (guint volume)
{
        g_signal_handlers_block_by_func (volume_scale,
                                         on_volume_scale_value_changed,
                                         NULL);

        gtk_range_set_value (GTK_RANGE (volume_scale), volume);

        g_signal_handlers_unblock_by_func (volume_scale,
                                           on_volume_scale_value_changed,
                                           NULL);
}

static void
on_didl_object_available (GUPnPDIDLLiteParser *parser,
                          GUPnPDIDLLiteObject *object,
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
                                                        (object,
                                                         sink_protocol_info,
                                                         lenient_mode);
        g_free (sink_protocol_info);
        g_object_unref (av_transport);
}

static GUPnPDIDLLiteResource *
find_compat_res_from_metadata (const char *metadata)
{
        GUPnPDIDLLiteParser   *parser;
        GUPnPDIDLLiteResource *resource;
        GError                *error;

        parser = gupnp_didl_lite_parser_new ();
        resource = NULL;
        error = NULL;

        g_signal_connect (parser,
                          "object-available",
                          G_CALLBACK (on_didl_object_available),
                          &resource);

        /* Assumption: metadata only contains a single didl object */
        gupnp_didl_lite_parser_parse_didl (parser, metadata, &error);
        if (error) {
                g_warning ("%s\n", error->message);

                g_error_free (error);
        }

        g_object_unref (parser);

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
        GUPnPServiceProxyAction *action;

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

        action = gupnp_service_proxy_action_new ("SetAVTransportURI",
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

        gupnp_service_proxy_call_action_async (av_transport,
                                               action,
                                               NULL,
                                               set_av_transport_uri_cb,
                                               data);
        gupnp_service_proxy_action_unref (action);
}

G_MODULE_EXPORT
void
on_play_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        PlaybackState state;

        state = get_selected_renderer_state ();

        if (state == PLAYBACK_STATE_STOPPED ||
            state == PLAYBACK_STATE_UNKNOWN) {
                get_selected_object ((MetadataFunc) set_av_transport_uri, play);
        } else {
                play ();
        }
}

G_MODULE_EXPORT
void
on_pause_button_clicked (GtkButton *button,
                         gpointer   user_data)
{
        av_transport_send_action ("Pause", NULL);
}

G_MODULE_EXPORT
void
on_stop_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        av_transport_send_action ("Stop", NULL);
}

G_MODULE_EXPORT
void
on_next_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
        select_next_object ();
}

G_MODULE_EXPORT
void
on_previous_button_clicked (GtkButton *button,
                            gpointer   user_data)
{
        select_prev_object ();
}

G_MODULE_EXPORT
void
on_clear_state_button_clicked (GtkButton *button,
                               gpointer   user_data)
{
        clear_selected_renderer_state ();
}

G_MODULE_EXPORT
gboolean
on_position_scale_value_changed (GtkRange *range,
                                 gpointer  user_data)
{
        char *args[] = { (char *) "Unit", (char *) "ABS_TIME", (char *)"Target", NULL, NULL };
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
                av_transport_send_action ("Seek", (char **)args);
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
set_position_scale_duration (const char *duration_str)
{
        gdouble duration;

        duration = seconds_from_time (duration_str);
        if (duration > 0.0) {
                gtk_range_set_range (GTK_RANGE (position_scale),
                                     0.0,
                                     duration);
        }
}

void
set_position_scale_position (const char *position_str)
{
        gdouble position;

        position = seconds_from_time (position_str);
        if (position >= 0.0) {
                g_signal_handlers_block_by_func
                                        (position_scale,
                                         on_position_scale_value_changed,
                                         NULL);

                gtk_range_set_value (GTK_RANGE (position_scale), position);

                g_signal_handlers_unblock_by_func
                                        (position_scale,
                                         on_position_scale_value_changed,
                                         NULL);
        }
}

static void
get_position_info_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        gchar       *position;
        gchar       *duration;
        const gchar *udn;
        GError *error = NULL;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));

        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        if (error != NULL) {
                g_warning ("Failed to get current media position"
                           "from media renderer '%s':%s\n",
                           udn,
                           error->message);

                goto return_point;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "AbsTime",
                                                    G_TYPE_STRING,
                                                    &position,
                                                    "TrackDuration",
                                                    G_TYPE_STRING,
                                                    &duration,
                                                    NULL)) {
                g_warning ("Failed to get current media position"
                           "from media renderer '%s':%s\n",
                           udn,
                           error->message);

                goto return_point;
        }

        set_position_scale_position (position);
        set_position_scale_duration (duration);
        char *tooltip = g_strdup_printf ("%s/%s", position, duration);
        gtk_widget_set_tooltip_text (GTK_WIDGET (position_scale), tooltip);
        gtk_widget_set_has_tooltip (GTK_WIDGET (position_scale), TRUE);
        g_free (tooltip);
        g_free (position);
        g_free (duration);

return_point:
        g_clear_error (&error);
        g_object_unref (object);
}

static gboolean
update_position (gpointer data)
{
        GUPnPServiceProxy *av_transport;
        GUPnPServiceProxyAction *action;

        av_transport = get_selected_av_transport (NULL);
        if (av_transport == NULL) {
                return FALSE;
        }

        action = gupnp_service_proxy_action_new ("GetPositionInfo",
                                                 "InstanceID",
                                                 G_TYPE_UINT,
                                                 0,
                                                 NULL);

        gupnp_service_proxy_call_action_async (av_transport,
                                               action,
                                               NULL,
                                               get_position_info_cb,
                                               NULL);

        gupnp_service_proxy_action_unref (action);

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
                pause_possible = FALSE;
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

       case PLAYBACK_STATE_UNKNOWN:
       default:
                play_possible = TRUE;
                pause_possible = TRUE;
                stop_possible = TRUE;

                remove_timeout ();

                break;
        }

        /* Disable the seekbar when the state is stopped */
        gtk_widget_set_sensitive (position_scale, stop_possible);
        if (!stop_possible) {
                g_signal_handlers_block_by_func
                                        (position_scale,
                                         on_position_scale_value_changed,
                                         NULL);

                gtk_range_set_value (GTK_RANGE (position_scale), 0.0);

                g_signal_handlers_unblock_by_func
                                        (position_scale,
                                         on_position_scale_value_changed,
                                         NULL);
        }

        gtk_widget_set_sensitive (play_button, play_possible);
        gtk_widget_set_sensitive (pause_button, pause_possible);
        gtk_widget_set_sensitive (stop_button, stop_possible);
}

static void
set_volume_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        GError *error;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);
        GUPnPServiceProxyAction *action;

        error = NULL;
        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);

        if (error == NULL) {
                // No transport error, check for SOAP error
                gupnp_service_proxy_action_get_result (action, &error, NULL);
        }

        if (error != NULL) {
                const char *udn;

                udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));

                g_warning ("Failed to set volume of %s: %s",
                           udn,
                           error->message);

                g_error_free (error);

                /* Update the range according to the current volume */
                set_volume_scale (get_selected_renderer_volume ());
        }

        g_object_unref (object);
}

G_MODULE_EXPORT
gboolean
on_volume_scale_value_changed (GtkRange *range,
                               gpointer  user_data)
{
        GUPnPServiceProxy *rendering_control;
        guint              desired_volume;
        GUPnPServiceProxyAction *action;

        rendering_control = get_selected_rendering_control ();
        if (rendering_control == NULL) {
                g_warning ("No renderer selected");
                return FALSE;
        }

        desired_volume = (guint) gtk_range_get_value (range);

        action = gupnp_service_proxy_action_new ("SetVolume",
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

        gupnp_service_proxy_call_action_async (rendering_control,
                                               action,
                                               NULL,
                                               set_volume_cb,
                                               NULL);

        gupnp_service_proxy_action_unref (action);

        return TRUE;
}

void
setup_renderer_controls (GtkBuilder *builder)
{
        volume_scale = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "volume-scale"));
        g_assert (volume_scale != NULL);

        position_scale = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "position-scale"));
        g_assert (position_scale != NULL);

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

        timeout_id = 0;

        g_object_weak_ref (G_OBJECT (position_scale),
                           (GWeakNotify) remove_timeout,
                           NULL);
}

