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
#include <signal.h>

#define DESCRIPTION_DOC "xml/network-light-desc.xml"
#define DIMMING_SERVICE "urn:schemas-upnp-org:service:Dimming:1"
#define SWITCH_SERVICE "urn:schemas-upnp-org:service:SwitchPower:1"

GMainLoop *main_loop;

static void
interrupt_signal_handler (int signum)
{
        g_main_loop_quit (main_loop);
}

static void
on_get_status(GUPnPService       *service,
              GUPnPServiceAction *action,
              gpointer            user_data)
{
        GList *locales;

        g_print ("The \"GetStatus\" action was invoked.\n");

        g_print ("\tLocales: ");
        locales = gupnp_service_action_get_locales (action);
        while (locales) {
                g_print ("%s", (char *) locales->data);
                g_free (locales->data);
                locales = g_list_delete_link (locales, locales);
                if (locales)
                        g_print (", ");
        }
        g_print ("\n");

        gupnp_service_action_set (action,
                                  "ResultStatus", G_TYPE_BOOLEAN, TRUE,
                                  NULL);

        gupnp_service_action_return (action);
}

static void
on_query_status (GUPnPService *service,
                 const char   *variable_name,
                 GValue       *value,
                 gpointer      user_data)
{
        g_value_init (value, G_TYPE_BOOLEAN);
        g_value_set_uint (value, FALSE);
}

static void
on_notify_failed (GUPnPService *service,
                  const GList  *callback_urls,
                  const GError *reason,
                  gpointer      user_data)
{
        g_print ("NOTIFY failed: %s\n", reason->message);
}

static gboolean
timeout (gpointer user_data)
{
        gupnp_service_notify (GUPNP_SERVICE (user_data),
                              "Status",
                              G_TYPE_BOOLEAN,
                              FALSE,
                              NULL);

        return FALSE;
}

int
main (int argc, char **argv)
{
        GError *error;
        GUPnPContext *context;
        xmlDoc *doc;
        GUPnPRootDevice *dev;
        GUPnPServiceInfo *switch_power;
        struct sigaction sig_action;

        g_thread_init (NULL);
        g_type_init ();
        setlocale (LC_ALL, "");

        error = NULL;
        context = gupnp_context_new (NULL, NULL, 0, &error);
        if (error) {
                g_error (error->message);
                g_error_free (error);

                return 1;
        }

        g_print ("Running on port %d\n", gupnp_context_get_port (context));

        /* Host current directory */
        gupnp_context_host_path (context, ".", "");

        /* Parse device description file */
        doc = xmlParseFile (DESCRIPTION_DOC);

        /* Create root device */
        dev = gupnp_root_device_new (context,
                                     doc,
                                     DESCRIPTION_DOC);

        /* Free doc when root device is destroyed */
        g_object_weak_ref (G_OBJECT (dev), (GWeakNotify) xmlFreeDoc, doc);

        /* Implement Browse action on ContentDirectory if available */
        switch_power = gupnp_device_info_get_service
                         (GUPNP_DEVICE_INFO (dev),
                         SWITCH_SERVICE);

        if (switch_power) {
                g_signal_connect (switch_power,
                                  "action-invoked::GetStatus",
                                  G_CALLBACK (on_get_status),
                                  NULL);

                g_signal_connect (switch_power,
                                  "query-variable::Status",
                                  G_CALLBACK (on_query_status),
                                  NULL);

                g_signal_connect (switch_power,
                                  "notify-failed",
                                  G_CALLBACK (on_notify_failed),
                                  NULL);

                g_timeout_add (5000, timeout, switch_power);
        }

        /* Run */
        gupnp_root_device_set_available (dev, TRUE);

        main_loop = g_main_loop_new (NULL, FALSE);

        /* Hook the handler for SIGTERM */
        memset (&sig_action, 0, sizeof (sig_action));
        sig_action.sa_handler = interrupt_signal_handler;
        sigaction (SIGINT, &sig_action, NULL);

        g_main_loop_run (main_loop);
        g_main_loop_unref (main_loop);

        if (switch_power)
                g_object_unref (switch_power);

        g_object_unref (dev);
        g_object_unref (context);

        return 0;
}
