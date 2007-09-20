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

#include <libgupnp/gupnp-control-point.h>
#include "universal-cp-gui.h"
#include <string.h>
#include <stdlib.h>

static GUPnPContext      *context;
static GUPnPControlPoint *cp;

void
on_state_variable_changed (GUPnPServiceProxy *proxy,
                           const char        *variable_name,
                           GValue            *value,
                           gpointer           user_data)
{
        GUPnPServiceInfo *info = GUPNP_SERVICE_INFO (proxy);
        GUPnPDeviceInfo  *device_info;
        GValue            str_value;
        char             *id;
        char             *friendly_name;
        char             *notified_at;
        struct tm        *tm;
        time_t            current_time;

        /* Get the parent device */
        device_info = get_service_device (info);
        g_return_if_fail (device_info != NULL);

        friendly_name = gupnp_device_info_get_friendly_name (device_info);
        g_object_unref (device_info);
        id = gupnp_service_info_get_id (info);
        /* We neither keep devices without friendlyname
         * nor services without an id
         */
        g_assert (friendly_name != NULL);
        g_assert (id != NULL);

        memset (&str_value, 0, sizeof (GValue));
        g_value_init (&str_value, G_TYPE_STRING);
        g_value_transform (value, &str_value);

        current_time = time (NULL);
        tm = localtime (&current_time);
        notified_at = g_strdup_printf ("%02d:%02d",
                                       tm->tm_hour,
                                       tm->tm_min);

        display_event (notified_at,
                       friendly_name,
                       id,
                       variable_name,
                       g_value_get_string (&str_value));

        g_free (notified_at);
        g_free (friendly_name);
        g_free (id);
        g_value_unset (&str_value);
}

void
show_action_arg_details (GUPnPServiceActionArgInfo *info)
{
        char *details[32];
        int   i = 0;

        details[i++] = "Name";
        details[i++] = g_strdup (info->name);
        details[i++] = "Direction";
        if (info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN)
                details[i++] = g_strdup ("in");
        else
                details[i++] = g_strdup ("out");
        details[i++] = "Related State Variable";
        details[i++] = g_strdup (info->related_state_variable);
        details[i++] = "Is Return Value";
        details[i++] = g_strdup (info->retval? "Yes": "No");
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_action_details (GUPnPServiceActionInfo *info)
{
        char *details[32];
        int   i = 0;

        details[i++] = "Name";
        details[i++] = g_strdup (info->name);
        details[i++] = "Number of Arguments";
        details[i++] = g_strdup_printf ("%u",
                                        g_list_length (info->arguments));
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_state_variable_details (GUPnPServiceStateVariableInfo *info)
{
        char  *details[32];
        GValue str_value;
        int    i = 0;

        memset (&str_value, 0, sizeof (GValue));
        g_value_init (&str_value, G_TYPE_STRING);

        details[i++] = "Name";
        details[i++] = g_strdup (info->name);
        details[i++] = "Send Events";
        details[i++] = g_strdup (info->send_events? "Yes": "No");
        details[i++] = "GType";
        details[i++] = g_strdup (g_type_name (info->type));

        details[i++] = "Default Value";
        g_value_transform (&info->default_value,
                           &str_value);
        details[i++] = g_value_dup_string (&str_value);

        if (info->is_numeric) {
                details[i++] = "Minimum";
                g_value_transform (&info->minimum,
                                   &str_value);
                details[i++] = g_value_dup_string (&str_value);
                details[i++] = "Maximum";
                g_value_transform (&info->maximum,
                                   &str_value);
                details[i++] = g_value_dup_string (&str_value);
                details[i++] = "Step";
                g_value_transform (&info->step,
                                   &str_value);
                details[i++] = g_value_dup_string (&str_value);
        } else if (info->allowed_values) {
                GList *iter;
                char **valuesv;
                int    j;

                valuesv =
                        g_malloc (sizeof (char *) *
                                  (g_list_length (info->allowed_values) + 1));

                for (j = 0, iter = info->allowed_values;
                     iter;
                     iter = iter->next, j++) {
                        valuesv[j] = (char *) iter->data;
                }
                valuesv[j] = NULL;

                details[i++] = "Allowed Values";
                details[i++] = g_strjoinv (", ", valuesv);
        }

        g_value_unset (&str_value);
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_service_details (GUPnPServiceInfo *info)
{
        char          *details[32];
        const SoupUri *uri;
        const char    *str;
        int            i = 0;

        details[i++] = "Location";
        str = gupnp_service_info_get_location (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = "UDN";
        str = gupnp_service_info_get_udn (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = "Type";
        str = gupnp_service_info_get_service_type (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = "Base URL";
        uri = gupnp_service_info_get_url_base (info);
        if (uri)
                details[i++] = soup_uri_to_string (uri, FALSE);

        details[i++] = "Service ID";
        details[i++] = gupnp_service_info_get_id (info);
        details[i++] = "Service URL";
        details[i++] = gupnp_service_info_get_scpd_url (info);
        details[i++] = "Control URL";
        details[i++] = gupnp_service_info_get_control_url (info);
        details[i++] = "Event Subscription URL";
        details[i++] = gupnp_service_info_get_event_subscription_url (info);
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_device_details (GUPnPDeviceInfo *info)
{
        char          *details[32];
        const SoupUri *uri;
        const char    *str;
        int            i = 0;

        details[i++] = "Location";
        str = gupnp_device_info_get_location (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = "UDN";
        str = gupnp_device_info_get_udn (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = "Type";
        str = gupnp_device_info_get_device_type (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = "Base URL";
        uri = gupnp_device_info_get_url_base (info);
        if (uri)
                details[i++] = soup_uri_to_string (uri, FALSE);

        details[i++] = "Friendly Name";
        details[i++] = gupnp_device_info_get_friendly_name (info);
        details[i++] = "Manufacturer";
        details[i++] = gupnp_device_info_get_manufacturer (info);
        details[i++] = "Manufacturer URL";
        details[i++] = gupnp_device_info_get_manufacturer_url (info);
        details[i++] = "Model Description";
        details[i++] = gupnp_device_info_get_model_description (info);
        details[i++] = "Model Name";
        details[i++] = gupnp_device_info_get_model_name (info);
        details[i++] = "Model Number";
        details[i++] = gupnp_device_info_get_model_number (info);
        details[i++] = "Model URL";
        details[i++] = gupnp_device_info_get_model_url (info);
        details[i++] = "Serial Number";
        details[i++] = gupnp_device_info_get_serial_number (info);
        details[i++] = "UPC";
        details[i++] = gupnp_device_info_get_upc (info);
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
                           GUPnPDeviceProxy  *proxy)
{
        append_device (GUPNP_DEVICE_INFO (proxy));
}

static void
device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                             GUPnPDeviceProxy  *proxy)
{
        remove_device (GUPNP_DEVICE_INFO (proxy));
}

static gboolean
init_upnp (void)
{
        GError *error;

        g_type_init ();

        error = NULL;
        context = gupnp_context_new (NULL, NULL, 0, &error);
        if (error) {
                g_critical (error->message);
                g_error_free (error);

                return FALSE;
        }

        /* We're interested in everything */
        cp = gupnp_control_point_new (context, "upnp:rootdevice");

        g_signal_connect (cp,
                          "device-proxy-available",
                          G_CALLBACK (device_proxy_available_cb),
                          NULL);
        g_signal_connect (cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (device_proxy_unavailable_cb),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

        return TRUE;
}

static void
deinit_upnp (void)
{
        g_object_unref (cp);
        g_object_unref (context);
}

void
application_exit (void)
{
        deinit_upnp ();
        deinit_ui ();

        gtk_main_quit ();
}

gint
main (gint   argc,
      gchar *argv[])
{
        if (!init_ui (&argc, &argv)) {
           return -2;
        }

        if (!init_upnp ()) {
           return -3;
        }

        gtk_main ();

        return 0;
}

