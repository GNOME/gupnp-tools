// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright 2022 Jens Georg <mail@jensge.org>

#include <config.h>

#include <glib-unix.h>
#include <glib.h>
#include <libgupnp/gupnp.h>

#include <iso646.h>

void
on_variable_notify (GUPnPServiceProxy *proxy,
                    const char *variable,
                    GValue *value,
                    gpointer user_data)
{
        g_autoptr (GDateTime) dt = g_date_time_new_now_local ();
        g_autofree char *timestr = g_date_time_format_iso8601 (dt);

        g_print ("%s|%s|%s|%s|%s\n",
                 timestr,
                 gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (user_data)),
                 gupnp_service_info_get_id (GUPNP_SERVICE_INFO (user_data)),
                 variable,
                 g_value_get_string (value));
}

void
on_introspection (GObject *source, GAsyncResult *res, gpointer user_data)
{
        g_autoptr (GError) error = NULL;

        g_autoptr (GUPnPServiceIntrospection) is =
                gupnp_service_info_introspect_finish (
                        GUPNP_SERVICE_INFO (source),
                        res,
                        &error);

        g_autofree char *id =
                gupnp_service_info_get_id (GUPNP_SERVICE_INFO (source));

        if (error != NULL) {
                g_warning ("Failed to introspect service proxy %s: %s",
                           id,
                           error->message);

                return;
        } else {
                g_info ("Got introspection for %s", id);
        }

        const GList *state_variables =
                gupnp_service_introspection_list_state_variables (is);
        for (const GList *it = state_variables; it != NULL;
             it = g_list_next (it)) {
                GUPnPServiceStateVariableInfo *info = it->data;
                if (not info->send_events)
                        continue;

                g_info ("Subscribing to %s (type: %s)",
                        info->name,
                        g_type_name (info->type));

                gupnp_service_proxy_add_notify (GUPNP_SERVICE_PROXY (source),
                                                info->name,
                                                G_TYPE_STRING,
                                                on_variable_notify,
                                                source);
        }
}

void
subscribe_to_proxy (GUPnPServiceProxy *sp, gpointer user_data)
{
        g_object_ref (sp);
        g_autofree char *id =
                gupnp_service_info_get_id (GUPNP_SERVICE_INFO (sp));

        g_info ("Found new service proxy %s", id);
        gupnp_service_info_introspect_async (GUPNP_SERVICE_INFO (sp),
                                             NULL,
                                             on_introspection,
                                             NULL);
        gupnp_service_proxy_set_subscribed (sp, TRUE);
}

void
on_proxy_available (GUPnPControlPoint *cp,
                    GUPnPDeviceProxy *dp,
                    gpointer user_data)
{
        g_autofree char *id =
                gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (dp));

        g_info ("New device %s, type %s at %s",
                id,
                gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (dp)),
                gupnp_device_info_get_location (GUPNP_DEVICE_INFO (dp)));

        GList *services =
                gupnp_device_info_list_services (GUPNP_DEVICE_INFO (dp));

        g_list_foreach ((GList *) services, (GFunc) subscribe_to_proxy, NULL);

        // Shallow-free the list, so we keep a reference to each service
        g_list_free_full (services, (GDestroyNotify) g_object_unref);

        g_object_ref (dp);
}

void
on_proxy_unavailable (GUPnPControlPoint *cp,
                      GUPnPDeviceProxy *dp,
                      gpointer user_data)
{
        g_autofree char *id =
                gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (dp));

        g_info ("Lost service proxy %s", id);

        // Dropping the reference we added in on_proxy_available
        g_object_unref (dp);
}

void
on_context_available (GUPnPContextManager *cm,
                      GUPnPContext *ctx,
                      gpointer user_data)
{

        g_info ("New context: %s",
                gssdp_client_get_host_ip (GSSDP_CLIENT (ctx)));
        g_autoptr (GUPnPControlPoint) cp =
                gupnp_control_point_new (ctx, "upnp:rootdevice");

        g_signal_connect (cp,
                          "device-proxy-available",
                          G_CALLBACK (on_proxy_available),
                          NULL);

        g_signal_connect (cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (on_proxy_unavailable),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

        gupnp_context_manager_manage_control_point (cm, cp);
}

void
on_context_unavailable (GUPnPContextManager *cm,
                        GUPnPContext *ctx,
                        gpointer user_data)
{
        g_info ("Context gone: %s",
                gssdp_client_get_host_ip (GSSDP_CLIENT (ctx)));
}

int
main (int argc, char *argv[])
{
        g_autoptr (GUPnPContextManager) cm = gupnp_context_manager_create (0);
        g_autoptr (GMainLoop) loop = g_main_loop_new (NULL, FALSE);

        g_signal_connect (cm,
                          "context-available",
                          G_CALLBACK (on_context_available),
                          NULL);

        g_signal_connect (cm,
                          "context-unavailable",
                          G_CALLBACK (on_context_unavailable),
                          NULL);

        g_unix_signal_add (SIGINT, (GSourceFunc) g_main_loop_quit, loop);
        g_unix_signal_add (SIGTERM, (GSourceFunc) g_main_loop_quit, loop);

        g_main_loop_run (loop);

        return 0;
}
