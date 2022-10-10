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

#include <stdlib.h>
#include <string.h>

#include "renderer-combo.h"
#include "renderer-controls.h"
#include "icons.h"

#define CONNECTION_MANAGER "urn:schemas-upnp-org:service:ConnectionManager"
#define AV_TRANSPORT "urn:schemas-upnp-org:service:AVTransport"
#define RENDERING_CONTROL "urn:schemas-upnp-org:service:RenderingControl"

static GtkWidget             *renderer_combo;
static GUPnPLastChangeParser *lc_parser;

static GUPnPServiceProxy *
get_av_transport (GUPnPDeviceProxy *renderer)
{
        GUPnPDeviceInfo  *info;
        GUPnPServiceInfo *av_transport;

        info = GUPNP_DEVICE_INFO (renderer);

        av_transport = gupnp_device_info_get_service (info, AV_TRANSPORT);

        return GUPNP_SERVICE_PROXY (av_transport);
}

GUPnPServiceProxy *
get_selected_av_transport (gchar **sink_protocol_info)
{
        GUPnPServiceProxy *av_transport;
        GtkComboBox       *combo;
        GtkTreeModel      *model;
        GtkTreeIter        iter;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return NULL;
        }

        if (sink_protocol_info != NULL) {
                gtk_tree_model_get (model,
                                    &iter,
                                    5, sink_protocol_info,
                                    -1);

                if (*sink_protocol_info == NULL) {
                        return NULL;
                }
        }

        gtk_tree_model_get (model,
                            &iter,
                            3, &av_transport,
                            -1);

        return av_transport;
}

GUPnPServiceProxy *
get_selected_rendering_control (void)
{
        GUPnPServiceProxy *rendering_control;
        GtkComboBox       *combo;
        GtkTreeModel      *model;
        GtkTreeIter        iter;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return NULL;
        }

        gtk_tree_model_get (model,
                            &iter,
                            4, &rendering_control,
                            -1);

        return rendering_control;
}

guint
get_selected_renderer_volume (void)
{
        GtkComboBox  *combo;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        guint         volume;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return 0;
        }

        gtk_tree_model_get (model,
                            &iter,
                            7, &volume,
                            -1);

        return volume;
}

PlaybackState
get_selected_renderer_state (void)
{
        PlaybackState state;
        GtkComboBox  *combo;
        GtkTreeModel *model;
        GtkTreeIter   iter;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return PLAYBACK_STATE_UNKNOWN;
        }

        gtk_tree_model_get (model,
                            &iter,
                            6, &state,
                            -1);

        return state;
}

static gboolean
find_renderer (GtkTreeModel *model,
               const char   *udn,
               GtkTreeIter  *iter)
{
        gboolean found = FALSE;
        gboolean more = TRUE;

        g_assert (udn != NULL);
        g_assert (iter != NULL);

        more = gtk_tree_model_get_iter_first (model, iter);

        while (more) {
                GUPnPDeviceInfo *info;

                gtk_tree_model_get (model,
                                    iter,
                                    2, &info,
                                    -1);

                if (info) {
                        const char *device_udn;

                        device_udn = gupnp_device_info_get_udn (info);

                        if (strcmp (device_udn, udn) == 0)
                                found = TRUE;

                        g_object_unref (info);
                }

                if (found)
                        break;

                more = gtk_tree_model_iter_next (model, iter);
        }

        return found;
}

static PlaybackState
state_name_to_state (const char *state_name)
{
        PlaybackState state;

        if (strcmp ("STOPPED", state_name) == 0) {
                state = PLAYBACK_STATE_STOPPED;
        } else if (strcmp ("PLAYING", state_name) == 0) {
                state = PLAYBACK_STATE_PLAYING;
        } else if (strcmp ("PAUSED_PLAYBACK", state_name) == 0) {
                state = PLAYBACK_STATE_PAUSED;
        } else if (strcmp ("TRANSITIONING", state_name) == 0) {
                state = PLAYBACK_STATE_TRANSITIONING;
        } else {
                state = PLAYBACK_STATE_UNKNOWN;
        }

        return state;
}

/* FIXME: is there a better implementation possible? */
static gboolean
is_iter_active (GtkComboBox *combo,
                GtkTreeIter *iter)
{
        GtkTreeModel *model;
        GtkTreeIter   active_iter;
        GtkTreePath  *active_path;
        GtkTreePath  *path;
        gboolean      ret;

        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        path = gtk_tree_model_get_path (model, iter);
        g_assert (path != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &active_iter)) {
                gtk_tree_path_free (path);

                return FALSE;
        }

        active_path = gtk_tree_model_get_path (model, &active_iter);
        g_assert (active_path != NULL);

        ret = (gtk_tree_path_compare (path, active_path) == 0);

        gtk_tree_path_free (path);
        gtk_tree_path_free (active_path);

        return ret;
}

static void
set_state (GtkTreeModel *model,
           GtkTreeIter  *iter,
           PlaybackState state)
{
        gtk_list_store_set (GTK_LIST_STORE (model),
                            iter,
                            6, state,
                            -1);

        if (state == PLAYBACK_STATE_PLAYING ||
            state == PLAYBACK_STATE_PAUSED ||
            state == PLAYBACK_STATE_TRANSITIONING) {
                gtk_widget_set_sensitive (renderer_combo, FALSE);
        } else {
                gtk_widget_set_sensitive (renderer_combo, TRUE);
        }

        if (is_iter_active (GTK_COMBO_BOX (renderer_combo), iter)) {
                prepare_controls_for_state (state);
        }
}

static void
set_state_by_name (const gchar *udn,
                   const gchar *state_name)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_combo_box_get_model
                (GTK_COMBO_BOX (renderer_combo));
        g_assert (model != NULL);

        if (find_renderer (model, udn, &iter)) {
                PlaybackState state;

                state = state_name_to_state (state_name),

                set_state (model, &iter, state);
        }
}

static void
set_volume (const gchar *udn,
            guint        volume)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_combo_box_get_model
                (GTK_COMBO_BOX (renderer_combo));
        g_assert (model != NULL);

        if (find_renderer (model, udn, &iter)) {
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    7, volume,
                                    -1);

                if (is_iter_active (GTK_COMBO_BOX (renderer_combo), &iter)) {
                        set_volume_scale (volume);
                }
        }
}

static void
set_duration (const gchar *udn,
              const gchar *duration)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_combo_box_get_model
                (GTK_COMBO_BOX (renderer_combo));
        g_assert (model != NULL);

        if (find_renderer (model, udn, &iter)) {
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    8, duration,
                                    -1);

                if (is_iter_active (GTK_COMBO_BOX (renderer_combo), &iter)) {
                        set_position_scale_duration (duration);
                }
        }
}

static void
on_device_icon_available (GObject *source,
                          GAsyncResult *res,
                          gpointer user_data)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        const char   *udn;
        g_autoptr (GdkPixbuf) icon;
        g_autoptr (GError) error = NULL;

        icon = update_icon_finish (GUPNP_DEVICE_INFO (source), res, &error);
        if (error != NULL) {
                g_debug ("Failed to download icon: %s", error->message);
        }

        if (icon == NULL)
                return;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (renderer_combo));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (source));

        if (find_renderer (model, udn, &iter)) {
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    0, icon,
                                    -1);
        }

        g_object_set_data (source, "icon-download-cancellable", NULL);
}

static void
on_last_change (GUPnPServiceProxy *av_transport,
                const char        *variable_name,
                GValue            *value,
                gpointer           user_data)
{
       const char *last_change_xml;
       char       *state_name;
       char       *duration;
       GError     *error;

       last_change_xml = g_value_get_string (value);
       error = NULL;
       state_name = NULL;
       duration = NULL;

       if (gupnp_last_change_parser_parse_last_change (lc_parser,
                                                       0,
                                                       last_change_xml,
                                                       &error,
                                                       "TransportState",
                                                       G_TYPE_STRING,
                                                       &state_name,
                                                       "CurrentMediaDuration",
                                                       G_TYPE_STRING,
                                                       &duration,
                                                       NULL)) {
               const char *udn;

               udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO (av_transport));
               if (state_name != NULL) {
                       set_state_by_name (udn, state_name);
                       g_free (state_name);
               }

               if (duration != NULL) {
                       set_duration (udn, duration);
                       g_free (duration);
               }
       } else if (error) {
               g_warning ("%s\n", error->message);

               g_error_free (error);
       }
}

static void
on_rendering_control_last_change (GUPnPServiceProxy *rendering_control,
                                  const char        *variable_name,
                                  GValue            *value,
                                  gpointer           user_data)
{
       const char *last_change_xml;
       guint       volume;
       GError     *error;

       last_change_xml = g_value_get_string (value);
       error = NULL;

       if (gupnp_last_change_parser_parse_last_change (lc_parser,
                                                       0,
                                                       last_change_xml,
                                                       &error,
                                                       "Volume",
                                                       G_TYPE_UINT,
                                                       &volume,
                                                       NULL)) {
                       const char *udn;

                       udn = gupnp_service_info_get_udn
                                        (GUPNP_SERVICE_INFO
                                                (rendering_control));
                       set_volume (udn, volume);
       } else if (error) {
               g_warning ("%s\n", error->message);

               g_error_free (error);
       }
}

void
clear_selected_renderer_state (void)
{
        GtkComboBox  *combo;
        GtkTreeModel *model;
        GtkTreeIter   iter;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (gtk_combo_box_get_active_iter (combo, &iter)) {
                set_state (model, &iter, PLAYBACK_STATE_UNKNOWN);
        }
}

static void
append_media_renderer_to_tree (GUPnPDeviceProxy  *proxy,
                               GUPnPServiceProxy *av_transport,
                               GUPnPServiceProxy *rendering_control,
                               const char        *udn)
{
        GUPnPDeviceInfo  *info;
        GtkComboBox      *combo;
        GtkTreeModel     *model;
        GtkTreeIter       iter;
        char             *name;
        gboolean          was_empty;

        info = GUPNP_DEVICE_INFO (proxy);
        combo = GTK_COMBO_BOX (renderer_combo);

        name = gupnp_device_info_get_friendly_name (info);
        if (name == NULL)
                name = g_strdup (udn);

        model = gtk_combo_box_get_model (combo);

        if (gtk_combo_box_get_active (combo) == -1)
                was_empty = TRUE;
        else
                was_empty = FALSE;

        memset (&iter, 0, sizeof (iter));
        gtk_list_store_insert_with_values
                (GTK_LIST_STORE (model),
                 &iter,
                 -1,
                 0, get_icon_by_id (ICON_MEDIA_RENDERER),
                 1, name,
                 2, proxy,
                 3, av_transport,
                 4, rendering_control,
                 6, PLAYBACK_STATE_UNKNOWN,
                 7, 0,
                 -1);

        gupnp_service_proxy_add_notify (av_transport,
                                        "LastChange",
                                        G_TYPE_STRING,
                                        on_last_change,
                                        NULL);
        gupnp_service_proxy_add_notify (rendering_control,
                                        "LastChange",
                                        G_TYPE_STRING,
                                        on_rendering_control_last_change,
                                        NULL);
        gupnp_service_proxy_set_subscribed (av_transport, TRUE);
        gupnp_service_proxy_set_subscribed (rendering_control, TRUE);

        GCancellable *cancellable = g_cancellable_new ();
        g_object_set_data_full (G_OBJECT (info),
                                "icon-download-cancellable",
                                cancellable,
                                g_object_unref);
        update_icon_async (info, cancellable, on_device_icon_available, NULL);

        if (was_empty)
                gtk_combo_box_set_active_iter (combo, &iter);

        g_free (name);
}

static GUPnPServiceProxy *
get_connection_manager (GUPnPDeviceProxy *proxy)
{
        GUPnPDeviceInfo  *info;
        GUPnPServiceInfo *cm;

        info = GUPNP_DEVICE_INFO (proxy);

        cm = gupnp_device_info_get_service (info, CONNECTION_MANAGER);

        return GUPNP_SERVICE_PROXY (cm);
}

static GUPnPServiceProxy *
get_rendering_control (GUPnPDeviceProxy *proxy)
{
        GUPnPDeviceInfo  *info;
        GUPnPServiceInfo *rendering_control;

        info = GUPNP_DEVICE_INFO (proxy);

        rendering_control = gupnp_device_info_get_service (info,
                                                           RENDERING_CONTROL);

        return GUPNP_SERVICE_PROXY (rendering_control);
}

static void
get_protocol_info_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        gchar      *sink_protocol_info;
        const gchar *udn;
        GError *error = NULL;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));
        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        if (error != NULL) {
                g_warning ("Failed to get sink protocl info from "
                           "media renderer '%s': %s",
                           udn,
                           error->message);

                goto return_point;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "Sink",
                                                    G_TYPE_STRING,
                                                    &sink_protocol_info,
                                                    NULL)) {
                g_warning ("Failed to get sink protocol info from "
                           "media renderer '%s':%s\n",
                           udn,
                           error->message);

                goto return_point;
        }

        if (sink_protocol_info) {
                GtkTreeModel *model;
                GtkTreeIter   iter;

                model = gtk_combo_box_get_model
                                        (GTK_COMBO_BOX (renderer_combo));
                g_assert (model != NULL);

                if (find_renderer (model, udn, &iter)) {
                        gtk_list_store_set (GTK_LIST_STORE (model),
                                            &iter,
                                            5, sink_protocol_info,
                                            -1);
                }

                g_free (sink_protocol_info);
        }

return_point:
        g_clear_error (&error);
        g_object_unref (object);
}

static void
get_transport_info_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        gchar       *state_name;
        const gchar *udn;
        GError *error = NULL;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));
        if (error != NULL) {
                g_warning ("Failed to get transport info from media renderer"
                           " '%s':%s",
                           udn,
                           error->message);

                goto return_point;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "CurrentTransportState",
                                                    G_TYPE_STRING,
                                                    &state_name,
                                                    NULL)) {
                g_warning ("Failed to get transport info from media renderer"
                           " '%s':%s\n",
                           udn,
                           error->message);

                goto return_point;
        }

        if (state_name) {
                set_state_by_name (udn, state_name);

                g_free (state_name);
        }

return_point:
        g_clear_error (&error);
        g_object_unref (object);
}

static void
get_volume_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        guint        volume;
        const gchar *udn;
        GError *error = NULL;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));
        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        if (error != NULL) {
                g_warning ("Failed to get volume info from media renderer"
                           " '%s':%s",
                           udn,
                           error->message);

                goto return_point;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "CurrentVolume",
                                                    G_TYPE_UINT,
                                                    &volume,
                                                    NULL)) {
                g_warning ("Failed to get volume from media renderer"
                           " '%s':%s\n",
                           udn,
                           error->message);

                goto return_point;
        }

        set_volume (udn, volume);

return_point:
        g_clear_error (&error);
        g_object_unref (object);
}

static void

get_media_info_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        gchar       *duration;
        const gchar *udn;
        GError *error = NULL;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        udn = gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (object));
        if (error != NULL) {
                g_warning ("Failed to get media info from media renderer"
                           " '%s':%s",
                           udn,
                           error->message);

                goto return_point;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "MediaDuration",
                                                    G_TYPE_STRING,
                                                    &duration,
                                                    NULL)) {
                g_warning ("Failed to get current media duration"
                           "from media renderer '%s':%s\n",
                           udn,
                           error->message);

                goto return_point;
        }

        set_duration (udn, duration);
        g_free (duration);

return_point:
        g_clear_error (&error);
        g_object_unref (object);
}

void
add_media_renderer (GUPnPDeviceProxy *proxy)
{
        GtkTreeModel      *model;
        GtkTreeIter        iter;
        const char        *udn;
        GUPnPServiceProxy *cm;
        GUPnPServiceProxy *av_transport;
        GUPnPServiceProxy *rendering_control;

        udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));
        if (udn == NULL)
                return;

        cm = get_connection_manager (proxy);
        if (G_UNLIKELY (cm == NULL))
                return;

        av_transport = get_av_transport (proxy);
        if (av_transport == NULL)
                goto no_av_transport;

        rendering_control = get_rendering_control (proxy);
        if (rendering_control == NULL)
                goto no_rendering_control;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (renderer_combo));

        if (!find_renderer (model, udn, &iter))
                append_media_renderer_to_tree (proxy,
                                               av_transport,
                                               rendering_control,
                                               udn);

        GUPnPServiceProxyAction *action;

        action = gupnp_service_proxy_action_new ("GetProtocolInfo", NULL);
        gupnp_service_proxy_call_action_async (g_object_ref (cm),
                                               action,
                                               NULL,
                                               get_protocol_info_cb,
                                               NULL);
        gupnp_service_proxy_action_unref (action);

        action = gupnp_service_proxy_action_new ("GetTransportInfo",
                                                 "InstanceID",
                                                 G_TYPE_UINT,
                                                 0,
                                                 NULL);
        gupnp_service_proxy_call_action_async (g_object_ref (av_transport),
                                               action,
                                               NULL,
                                               get_transport_info_cb,
                                               NULL);
        gupnp_service_proxy_action_unref (action);

        action = gupnp_service_proxy_action_new ("GetMediaInfo",
                                                 "InstanceID",
                                                 G_TYPE_UINT,
                                                 0,
                                                 NULL);
        gupnp_service_proxy_call_action_async (g_object_ref (av_transport),
                                               action,
                                               NULL,
                                               get_media_info_cb,
                                               NULL);
        gupnp_service_proxy_action_unref (action);

        action = gupnp_service_proxy_action_new ("GetVolume",
                                                 "InstanceID",
                                                 G_TYPE_UINT,
                                                 0,
                                                 "Channel",
                                                 G_TYPE_STRING,
                                                 "Master",
                                                 NULL);
        gupnp_service_proxy_call_action_async (g_object_ref (rendering_control),
                                               action,
                                               NULL,
                                               get_volume_cb,
                                               NULL);
        gupnp_service_proxy_action_unref (action);

        g_object_unref (rendering_control);
no_rendering_control:
        g_object_unref (av_transport);
no_av_transport:
        g_object_unref (cm);
}

void
remove_media_renderer (GUPnPDeviceProxy *proxy)
{
        GUPnPDeviceInfo *info;
        GtkComboBox     *combo;
        GtkTreeModel    *model;
        GtkTreeIter      iter;
        const char      *udn;

        info = GUPNP_DEVICE_INFO (proxy);
        combo = GTK_COMBO_BOX (renderer_combo);

        udn = gupnp_device_info_get_udn (info);
        if (udn == NULL)
                return;

        model = gtk_combo_box_get_model (combo);

        if (find_renderer (model, udn, &iter)) {
                GCancellable *cancellable =
                        g_object_get_data (G_OBJECT (info),
                                           "icon-download-cancellable");
                if (cancellable != NULL) {
                        g_cancellable_cancel (cancellable);

                        g_object_set_data (G_OBJECT (info),
                                           "icon-download-cancellable",
                                           NULL);
                }
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
                gtk_combo_box_set_active (combo, 0);
        }
}

G_MODULE_EXPORT
void
on_renderer_combo_changed (GtkComboBox *widget,
                           gpointer     user_data)
{
        GtkComboBox  *combo;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        PlaybackState state;
        guint         volume;

        combo = GTK_COMBO_BOX (widget);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return;
        }

        gtk_tree_model_get (model,
                            &iter,
                            6, &state,
                            7, &volume,
                            -1);
        set_volume_scale (volume);
        prepare_controls_for_state (state);
}

void
setup_renderer_combo (GtkBuilder *builder)
{
        lc_parser = gupnp_last_change_parser_new ();
        renderer_combo = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "renderer-combobox"));
}

