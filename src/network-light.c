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
 * GNU Library General Public License for more details.
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

#include "network-light-gui.h"

#define DESCRIPTION_DOC "xml/network-light-desc.xml"
#define DIMMING_SERVICE "urn:schemas-upnp-org:service:Dimming:1"
#define SWITCH_SERVICE "urn:schemas-upnp-org:service:SwitchPower:1"

static GUPnPContext     *context;
static GUPnPRootDevice  *dev;
static GUPnPServiceInfo *switch_power;
static GUPnPServiceInfo *dimming;

static void
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

static void
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

static void
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

        gupnp_service_notify (service,
                              "Status",
                              G_TYPE_BOOLEAN,
                              get_status (),
                              NULL);
}

static void
on_query_status (GUPnPService *service,
                 const char   *variable_name,
                 GValue       *value,
                 gpointer      user_data)
{
        g_value_init (value, G_TYPE_BOOLEAN);
        g_value_set_boolean (value, get_status ());
}

static void
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

static void
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

static void
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

        gupnp_service_notify (service,
                              "LoadLevelStatus",
                              G_TYPE_UINT,
                              load_level,
                              NULL);
}

static void
on_query_load_level (GUPnPService *service,
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
        g_warning ("NOTIFY failed: %s\n", reason->message);
}

static gboolean
timeout (gpointer user_data)
{
        gupnp_service_notify (GUPNP_SERVICE (switch_power),
                              "Status",
                              G_TYPE_BOOLEAN,
                              get_status (),
                              NULL);

        gupnp_service_notify (GUPNP_SERVICE (dimming),
                              "LoadLevelStatus",
                              G_TYPE_UINT,
                              get_load_level (),
                              NULL);

        return FALSE;
}

static gboolean
init_upnp (void)
{
        GError *error;
        xmlDoc *doc = NULL;

        g_thread_init (NULL);

        error = NULL;
        context = gupnp_context_new (NULL, NULL, 0, &error);

        if (error) {
                g_error (error->message);
                g_error_free (error);

                return FALSE;
        }

        g_print ("Running on port %d\n", gupnp_context_get_port (context));

        /* Host current directory (for running uninstalled) */
        gupnp_context_host_path (context, ".", "");
        /* Host our xml dir (for running installed) */
        gupnp_context_host_path (context, DATA_DIR, "");

	/* Parse device description file */
	if (!g_file_test (DESCRIPTION_DOC, G_FILE_TEST_EXISTS))
	        doc = xmlParseFile (DATA_DIR "/" DESCRIPTION_DOC);
	else
	        doc = xmlParseFile (DESCRIPTION_DOC);

	if (doc == NULL) {
	        g_critical ("Unable to load the XML description file %s",
			    DESCRIPTION_DOC);
                return FALSE;
	}

        /* Create root device */
        dev = gupnp_root_device_new (context,
                                     doc,
                                     DESCRIPTION_DOC);

        /* Free doc when root device is destroyed */
        g_object_weak_ref (G_OBJECT (dev), (GWeakNotify) xmlFreeDoc, doc);

        switch_power = gupnp_device_info_get_service
                         (GUPNP_DEVICE_INFO (dev),
                         SWITCH_SERVICE);

        if (switch_power) {
                g_signal_connect (switch_power,
                                  "action-invoked::GetStatus",
                                  G_CALLBACK (on_get_status),
                                  NULL);
                g_signal_connect (switch_power,
                                  "action-invoked::GetTarget",
                                  G_CALLBACK (on_get_target),
                                  NULL);
                g_signal_connect (switch_power,
                                  "action-invoked::SetTarget",
                                  G_CALLBACK (on_set_target),
                                  NULL);

                g_signal_connect (switch_power,
                                  "query-variable::Status",
                                  G_CALLBACK (on_query_status),
                                  NULL);
                g_signal_connect (switch_power,
                                  "query-variable::Target",
                                  G_CALLBACK (on_query_status),
                                  NULL);

                g_signal_connect (switch_power,
                                  "notify-failed",
                                  G_CALLBACK (on_notify_failed),
                                  NULL);
        }

        dimming = gupnp_device_info_get_service
                         (GUPNP_DEVICE_INFO (dev),
                         DIMMING_SERVICE);

        if (dimming) {
                g_signal_connect (dimming,
                                  "action-invoked::GetLoadLevelStatus",
                                  G_CALLBACK (on_get_load_level_status),
                                  NULL);
                g_signal_connect (dimming,
                                  "action-invoked::GetLoadLevelTarget",
                                  G_CALLBACK (on_get_load_level_target),
                                  NULL);
                g_signal_connect (dimming,
                                  "action-invoked::SetLoadLevelTarget",
                                  G_CALLBACK (on_set_load_level_target),
                                  NULL);

                g_signal_connect (dimming,
                                  "query-variable::LoadLevelStatus",
                                  G_CALLBACK (on_query_load_level),
                                  NULL);
                g_signal_connect (dimming,
                                  "query-variable::LoadLevelTarget",
                                  G_CALLBACK (on_query_load_level),
                                  NULL);

                g_signal_connect (dimming,
                                  "notify-failed",
                                  G_CALLBACK (on_notify_failed),
                                  NULL);
        }

        g_timeout_add (5000, timeout, NULL);

        /* Run */
        gupnp_root_device_set_available (dev, TRUE);

        return TRUE;
}

static void
deinit_upnp (void)
{
        g_object_unref (switch_power);
        g_object_unref (dimming);
        g_object_unref (dev);
        g_object_unref (context);
}

int
main (int argc, char **argv)
{
        if (!init_ui (&argc, &argv)) {
                return -1;
        }

        if (!init_upnp ()) {
                return -2;
        }

        gtk_main ();

        deinit_ui ();
        deinit_upnp ();

        return 0;
}
