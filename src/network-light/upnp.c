/*
 * Copyright (C) 2007 Zeeshan Ali.
 * Copyright (C) 2007 OpenedHand Ltd.
 *
 * Author: Zeeshan Ali <zeenix@gstreamer.net>
 * Author: Jorn Baayen <jorn@openedhand.com>
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

#include <libgupnp/gupnp-root-device.h>
#include <libgupnp/gupnp-service.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <uuid/uuid.h>
#include <glib/gstdio.h>

#include "gui.h"
#include "upnp.h"
#include "main.h"

#define DESCRIPTION_DOC "xml/network-light-desc.xml"
#define DIMMING_SERVICE "urn:schemas-upnp-org:service:Dimming:1"
#define SWITCH_SERVICE "urn:schemas-upnp-org:service:SwitchPower:1"
#define NETWORK_LIGHT "urn:schemas-upnp-org:device:DimmableLight:1"

typedef struct
{
        GUPnPRootDevice *dev;
        GUPnPServiceInfo *switch_power;
        GUPnPServiceInfo *dimming;
} NetworkLight;

static GUPnPContextManager *context_manager;
static GHashTable *nl_hash;

/* Other network light services on the network */
static GList *switch_proxies;
static GList *dimming_proxies;

static xmlDoc *doc;
static char *desc_location;
static char *ext_desc_path;
static char uuid[37];

static NetworkLight *
network_light_new (GUPnPRootDevice  *dev,
                   GUPnPServiceInfo *switch_power,
                   GUPnPServiceInfo *dimming)
{
        NetworkLight *network_light;

        network_light = g_slice_new (NetworkLight);

        network_light->dev = dev;
        network_light->switch_power = switch_power;
        network_light->dimming = dimming;

        return network_light;
}

static void
network_light_free (NetworkLight *network_light)
{
        g_object_unref (network_light->dev);
        g_object_unref (network_light->switch_power);
        g_object_unref (network_light->dimming);

        g_slice_free (NetworkLight, network_light);
}

void
notify_status_change (gboolean status)
{
        GList *network_lights;
        GList *nl_node;

        network_lights = g_hash_table_get_values (nl_hash);

        for (nl_node = network_lights;
             nl_node != NULL;
             nl_node = nl_node->next) {
                NetworkLight *nl = (NetworkLight *) nl_node->data;

                gupnp_service_notify (GUPNP_SERVICE (nl->switch_power),
                                      "Status",
                                      G_TYPE_BOOLEAN,
                                      status,
                                      NULL);
        }
}

void
notify_load_level_change (gint load_level)
{
        GList *network_lights;
        GList *nl_node;

        network_lights = g_hash_table_get_values (nl_hash);

        for (nl_node = network_lights;
             nl_node != NULL;
             nl_node = nl_node->next) {
                NetworkLight *nl = (NetworkLight *) nl_node->data;

                gupnp_service_notify (GUPNP_SERVICE (nl->dimming),
                                      "LoadLevelStatus",
                                      G_TYPE_UINT,
                                      load_level,
                                      NULL);
        }
}

void
on_get_status (GUPnPService       *service,
               GUPnPServiceAction *action,
               gpointer            user_data)
{
        gupnp_service_action_set (action,
                                  "ResultStatus",
                                  G_TYPE_BOOLEAN,
                                  get_status (),
                                  NULL);

        gupnp_service_action_return (action);
}

void
on_get_target (GUPnPService       *service,
               GUPnPServiceAction *action,
               gpointer            user_data)
{
        gupnp_service_action_set (action,
                                  "RetTargetValue",
                                  G_TYPE_BOOLEAN,
                                  get_status (),
                                  NULL);

        gupnp_service_action_return (action);
}

void
on_set_target (GUPnPService       *service,
               GUPnPServiceAction *action,
               gpointer            user_data)
{
        gboolean status;

        gupnp_service_action_get (action,
                                  "NewTargetValue",
                                  G_TYPE_BOOLEAN,
                                  &status,
                                  NULL);
        gupnp_service_action_return (action);

        set_status (status);
}

void
on_query_status (GUPnPService *service,
                 const char   *variable_name,
                 GValue       *value,
                 gpointer      user_data)
{
        g_value_init (value, G_TYPE_BOOLEAN);
        g_value_set_boolean (value, get_status ());
}

void
on_query_target (GUPnPService *service,
                 const char   *variable_name,
                 GValue       *value,
                 gpointer      user_data)
{
        g_value_init (value, G_TYPE_BOOLEAN);
        g_value_set_boolean (value, get_status ());
}

void
on_get_load_level_status (GUPnPService       *service,
                          GUPnPServiceAction *action,
                          gpointer            user_data)
{
        gupnp_service_action_set (action,
                                  "retLoadlevelStatus",
                                  G_TYPE_UINT,
                                  get_load_level (),
                                  NULL);

        gupnp_service_action_return (action);
}

void
on_get_load_level_target (GUPnPService       *service,
                          GUPnPServiceAction *action,
                          gpointer            user_data)
{
        gupnp_service_action_set (action,
                                  "retLoadlevelTarget",
                                  G_TYPE_UINT,
                                  get_load_level (),
                                  NULL);

        gupnp_service_action_return (action);
}

void
on_set_load_level_target (GUPnPService       *service,
                          GUPnPServiceAction *action,
                          gpointer            user_data)
{
        guint load_level;

        gupnp_service_action_get (action,
                                  "newLoadlevelTarget",
                                  G_TYPE_UINT,
                                  &load_level,
                                  NULL);
        gupnp_service_action_return (action);

        load_level = CLAMP (load_level, 0, 100);
        set_load_level (load_level);
}

void
on_query_load_level_status (GUPnPService *service,
                            const char   *variable_name,
                            GValue       *value,
                            gpointer      user_data)
{
        g_value_init (value, G_TYPE_UINT);
        g_value_set_uint (value, get_load_level ());
}

void
on_query_load_level_target (GUPnPService *service,
                            const char   *variable_name,
                            GValue       *value,
                            gpointer      user_data)
{
        g_value_init (value, G_TYPE_UINT);
        g_value_set_uint (value, get_load_level ());
}

static void
on_notify_failed (GUPnPService *service,
                  const GList  *callback_urls,
                  const GError *reason,
                  gpointer      user_data)
{
        GList   *url_node;
        GString *warning;

        warning = g_string_new (NULL);
        g_string_printf (warning,
                         "NOTIFY failed for the following client URLs:\n");
        for (url_node = (GList *) callback_urls;
             url_node;
             url_node = url_node->next) {
                g_string_append_printf (warning,
                                        "%s\n",
                                        (char *) url_node->data);
        }
        g_string_append_printf (warning, "Reason: %s", reason->message);

        g_warning ("%s", warning->str);
        g_string_free (warning, TRUE);
}

/* Copied from gupnp/libgupnp/xml-utils.c */
static xmlNode *
xml_util_get_element (xmlNode *node,
                      ...)
{
        va_list var_args;

        va_start (var_args, node);

        while (TRUE) {
                const char *arg;

                arg = va_arg (var_args, const char *);
                if (!arg)
                        break;

                for (node = node->children; node; node = node->next) {
                        if (node->name == NULL)
                                continue;

                        if (!strcmp (arg, (char *) node->name))
                                break;
                }

                if (!node)
                        break;
        }

        va_end (var_args);

        return node;
}

static void init_uuid ()
{
        uuid_t uuid_context;
        xmlNode *uuid_node;
        char *udn;

        uuid_generate (uuid_context);
        uuid_unparse (uuid_context, uuid);

        uuid_node = xml_util_get_element ((xmlNode *) doc,
                                          "root",
                                          "device",
                                          "UDN",
                                          NULL);
        if (uuid_node == NULL) {
                g_critical ("Failed to find UDN element"
                            "in device description");

                return;
        }

        udn = g_strdup_printf ("uuid:%s", uuid);

        xmlNodeSetContent (uuid_node, (unsigned char *) udn);

        g_free (udn);
}

void on_service_proxy_action_ret (GUPnPServiceProxy *proxy,
                                  GUPnPServiceProxyAction *action,
                                  gpointer user_data)
{
        GError *error = NULL;

        gupnp_service_proxy_end_action (proxy,
                                        action,
                                        &error,
                                        NULL);
        if (error) {
                GUPnPServiceInfo *info = GUPNP_SERVICE_INFO (proxy);

                g_warning ("Failed to call action \"%s\" on \"%s\": %s",
                           (char *) user_data,
                           gupnp_service_info_get_location (info),
                           error->message);

                g_error_free (error);
        }

        g_free (user_data);
}

void
set_all_status (gboolean status)
{
        GList *proxy_node;

        for (proxy_node = switch_proxies;
             proxy_node;
             proxy_node = g_list_next (proxy_node)) {
                GUPnPServiceProxy *proxy;
                char *action_name;

                proxy = GUPNP_SERVICE_PROXY (proxy_node->data);
                action_name = g_strdup ("SetTarget");

                gupnp_service_proxy_begin_action (proxy,
                                                  action_name,
                                                  on_service_proxy_action_ret,
                                                  action_name,
                                                  "NewTargetValue",
                                                  G_TYPE_BOOLEAN,
                                                  status,
                                                  NULL);
        }
}

void
set_all_load_level (gint load_level)
{
        GList *proxy_node;

        for (proxy_node = dimming_proxies;
             proxy_node;
             proxy_node = g_list_next (proxy_node)) {
                GUPnPServiceProxy *proxy;
                char *action_name;

                proxy = GUPNP_SERVICE_PROXY (proxy_node->data);
                action_name = g_strdup ("SetLoadLevelTarget");

                gupnp_service_proxy_begin_action (proxy,
                                                  action_name,
                                                  on_service_proxy_action_ret,
                                                  action_name,
                                                  "newLoadlevelTarget",
                                                  G_TYPE_UINT,
                                                  load_level,
                                                  NULL);
        }
}

static void
on_network_light_available (GUPnPControlPoint *cp,
                            GUPnPDeviceProxy  *light_proxy,
                            gpointer           user_data)
{
        GUPnPServiceProxy *switch_proxy;
        GUPnPServiceProxy *dimming_proxy;
        GUPnPServiceInfo  *info;

        info = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (light_proxy),
                                              SWITCH_SERVICE);
        switch_proxy = GUPNP_SERVICE_PROXY (info);

        if (switch_proxy) {
                switch_proxies = g_list_append (switch_proxies, switch_proxy);
        }

        info = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (light_proxy),
                                              DIMMING_SERVICE);
        dimming_proxy = GUPNP_SERVICE_PROXY (info);

        if (dimming_proxy) {
                dimming_proxies = g_list_append (dimming_proxies,
                                                 dimming_proxy);
        }
}

static gint compare_service (GUPnPServiceInfo *info1,
                             GUPnPServiceInfo *info2)
{
        const char *udn1;
        const char *udn2;

        udn1 = gupnp_service_info_get_udn (info1);
        udn2 = gupnp_service_info_get_udn (info2);

        return strcmp (udn1, udn2);
}

static void remove_service_from_list (GUPnPServiceInfo *info,
                                      GList           **list)
{
        GList *proxy_node;

        proxy_node = g_list_find_custom (*list,
                                         info,
                                         (GCompareFunc) compare_service);
        if (proxy_node) {
                g_object_unref (proxy_node->data);
                *list = g_list_remove (*list, proxy_node->data);
        }
}

static void
on_network_light_unavailable (GUPnPControlPoint *control_point,
                              GUPnPDeviceProxy  *light_proxy,
                              gpointer           user_data)
{
        GUPnPServiceInfo *info;

        info = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (light_proxy),
                                              SWITCH_SERVICE);
        if (info) {
                remove_service_from_list (info, &switch_proxies);
        }

        info = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (light_proxy),
                                              DIMMING_SERVICE);
        if (info) {
                remove_service_from_list (info, &dimming_proxies);
        }
}

static gboolean
init_server (GUPnPContext *context)
{
        NetworkLight *network_light;
        GUPnPRootDevice *dev;
        GUPnPServiceInfo *switch_power;
        GUPnPServiceInfo *dimming;
        GError *error = NULL;

        gupnp_context_host_path (context, desc_location, ext_desc_path);
        gupnp_context_host_path (context, DATA_DIR, "");

        /* Create root device */
        dev = gupnp_root_device_new_full (context,
                                          gupnp_resource_factory_get_default (),
                                          doc,
                                          ext_desc_path);

        switch_power = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (dev),
                                                      SWITCH_SERVICE);

        if (switch_power) {
                gupnp_service_signals_autoconnect
                                (GUPNP_SERVICE (switch_power),
                                 NULL,
                                 &error);

                g_signal_connect (switch_power,
                                  "notify-failed",
                                  G_CALLBACK (on_notify_failed),
                                  NULL);
        }

        dimming = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (dev),
                                                 DIMMING_SERVICE);

        if (dimming) {
                gupnp_service_signals_autoconnect (GUPNP_SERVICE (dimming),
                                                   NULL,
                                                   &error);

                g_signal_connect (dimming,
                                  "notify-failed",
                                  G_CALLBACK (on_notify_failed),
                                  NULL);
        }

        network_light = network_light_new (dev, switch_power, dimming);
        g_hash_table_insert (nl_hash, g_object_ref (context), network_light);

        /* Run */
        gupnp_root_device_set_available (dev, TRUE);

        g_print ("Attaching to IP/Host %s on port %d\n",
                 gupnp_context_get_host_ip (context),
                 gupnp_context_get_port (context));

        return TRUE;
}

static gboolean
prepare_desc ()
{
        doc = xmlParseFile (DATA_DIR "/" DESCRIPTION_DOC);
        if (doc == NULL) {
                g_critical ("Unable to load the XML description file %s",
                            DESCRIPTION_DOC);
                return FALSE;
        }

        /* create and set the UUID in the xmlDoc */
        init_uuid ();

        /* saving the xml file to the temporal location with the uuid name */
        desc_location = g_strdup_printf ("%s/gupnp-network-light-%s.xml",
                                         g_get_tmp_dir (),
                                         uuid);
        g_assert (desc_location != NULL);

        if (xmlSaveFile (desc_location, doc) < 0) {
                g_print ("Error saving description file to %s.\n",
                         desc_location);

                g_free (desc_location);
                xmlFreeDoc (doc);

                return FALSE;
        }

        ext_desc_path = g_strdup_printf ("/%s.xml", uuid);

        return TRUE;
}

static gboolean
init_client (GUPnPContext *context)
{
        GUPnPControlPoint *cp;

        cp = gupnp_control_point_new (context, NETWORK_LIGHT);

        g_signal_connect (cp,
                          "device-proxy-available",
                          G_CALLBACK (on_network_light_available),
                          NULL);
        g_signal_connect (cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (on_network_light_unavailable),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

        /* Let context manager take care of the control point life cycle */
        gupnp_context_manager_manage_control_point (context_manager, cp);

        /* We don't need to keep our own references to the control points */
        g_object_unref (cp);

        return TRUE;
}

static void
on_context_available (GUPnPContextManager *context_manager,
                      GUPnPContext        *context,
                      gpointer            *user_data)
{
        /* Initialize client-side stuff */
        init_client (context);

        /* Then the server-side stuff */
        init_server (context);
}

static void
on_context_unavailable (GUPnPContextManager *context_manager,
                        GUPnPContext        *context,
                        gpointer             user_data)
{
        g_print ("Dettaching from IP/Host %s and port %d\n",
                 gupnp_context_get_host_ip (context),
                 gupnp_context_get_port (context));

        g_hash_table_remove (nl_hash, context);
}

static gboolean
context_equal (GUPnPContext *context1, GUPnPContext *context2)
{
        return g_ascii_strcasecmp (gupnp_context_get_host_ip (context1),
                                   gupnp_context_get_host_ip (context2)) == 0;
}

gboolean
init_upnp (void)
{
        GError *error = NULL;

        switch_proxies = NULL;
        dimming_proxies = NULL;

        nl_hash = g_hash_table_new_full (g_direct_hash,
                                         (GEqualFunc) context_equal,
                                         g_object_unref,
                                         (GDestroyNotify) network_light_free);

        if (!prepare_desc (&error)) {
                return FALSE;
        }

        context_manager = gupnp_context_manager_new (NULL, 0);
        g_assert (context_manager != NULL);

        g_signal_connect (context_manager,
                          "context-available",
                          on_context_available,
                          NULL);
        g_signal_connect (context_manager,
                          "context-unavailable",
                          on_context_unavailable,
                          NULL);

        return TRUE;
}

void
deinit_upnp (void)
{
        g_object_unref (context_manager);

        g_hash_table_unref (nl_hash);

        g_list_foreach (switch_proxies, (GFunc) g_object_unref, NULL);
        g_list_foreach (dimming_proxies, (GFunc) g_object_unref, NULL);

        /* Free descriptiont doc */
        xmlFreeDoc (doc);

        if (g_remove (desc_location) != 0)
                g_warning ("error removing %s\n", desc_location);
        g_free (desc_location);
        g_free (ext_desc_path);
}

