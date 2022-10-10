/*
 * Copyright (C) 2011 Nokia Corporation.
 * Copyright (C) 2014 Jens Georg <mail@jensge.org>
 *
 * Author: Jens Georg <jensg@openismus.com>
 *                    <mail@jensge.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <glib.h>
#include <libgssdp/gssdp.h>
#include <stdlib.h>
#include <string.h>

static char *interface = NULL;
static char *target = NULL;
static char *message_type = NULL;
static int timeout = 0;
static int rescan_interval = 0;

typedef struct _GSSDPDiscover {
        GMainLoop *main_loop;
        GSSDPClient *client;
        GSSDPResourceBrowser *browser;
} GSSDPDiscover;

static GOptionEntry entries[] =
{
        { "interface", 'i', 0, G_OPTION_ARG_STRING, &interface, "Network INTERFACE to use", "INTERFACE" },
        { "target", 't', 0, G_OPTION_ARG_STRING, &target, "SSDP TARGET to search for (default: ssdp:all)", "TARGET" },
        { "timeout", 'n', 0, G_OPTION_ARG_INT, &timeout, "TIME in seconds to wait for replies before exiting", "TIME" },
        { "rescan-interval", 'r', 0, G_OPTION_ARG_INT, &rescan_interval, "TIME in seconds to wait before sending another discovery request", "TIME" },
        { "message-type", 'm', 0, G_OPTION_ARG_STRING, &message_type, "TYPE of message (available,unavailable,all)", "TYPE" },
        { NULL }
};

static gboolean
on_force_rescan_timeout (GSSDPDiscover *discover) {
        gssdp_resource_browser_set_active (discover->browser,
                                           FALSE);
        gssdp_resource_browser_set_active (discover->browser,
                                           TRUE);
        return TRUE;
}

static void
on_resource_available (GSSDPResourceBrowser *browser,
                       const char           *usn,
                       GList                *locations)
{
        GList *l;

        g_print ("resource available\n"
                 "  USN:      %s\n",
                 usn);

        for (l = locations; l; l = l->next)
                g_print ("  Location: %s\n", (char *) l->data);
}

static void
on_resource_unavailable (GSSDPResourceBrowser *browser,
                         const char           *usn)
{
        g_print ("resource unavailable\n"
                 "  USN:      %s\n",
                 usn);
}


int main (int argc, char *argv[]) {
        GError *error = NULL;
        GOptionContext *context;
        GSSDPDiscover discover;

        context = g_option_context_new ("- discover devices using SSDP");
        g_option_context_add_main_entries (context, entries, NULL);

        if (!g_option_context_parse (context, &argc, &argv, &error)) {
                g_warning ("Failed to parse options: %s", error->message);
                g_error_free (error);
                exit (1);
        }

        g_option_context_free (context);

        discover.client = gssdp_client_new_full (interface,
                                                 NULL,
                                                 0,
                                                 GSSDP_UDA_VERSION_1_0,
                                                 &error);
        if (error != NULL) {
                g_warning ("Failed to create GSSDP client: %s", error->message);
                g_error_free (error);
                exit (1);
        }

        g_print ("Using network interface %s\n",
                 gssdp_client_get_interface (discover.client));

        if (target) {
                g_print ("Scanning for resources matching %s\n", target);
                discover.browser = gssdp_resource_browser_new
                                        (discover.client, target);
        } else {
                g_print ("Scanning for all resources\n");
                discover.browser = gssdp_resource_browser_new
                                        (discover.client, "ssdp:all");
        }

        if (message_type == NULL) {
                message_type = g_strdup ("available");
        } else {
                if (strncmp (message_type, "available", 9) != 0 &&
                    strncmp (message_type, "all", 3) != 0 &&
                    strncmp (message_type, "unavailable", 11) != 0) {
                        g_print ("Invalid message type. Use \"all\", ");
                        g_print ("\"available\" or \"unavailable\"\n");
                        exit (1);
                }
        }

        if (strncmp (message_type, "available", 9) == 0 ||
            strncmp (message_type, "all", 3) == 0) {
                g_signal_connect (discover.browser,
                                  "resource-available",
                                  G_CALLBACK (on_resource_available),
                                  &discover);
        }

        if (strncmp (message_type, "unavailable", 11) == 0 ||
            strncmp (message_type, "all", 3) == 0) {
                g_signal_connect (discover.browser,
                                  "resource-unavailable",
                                  G_CALLBACK (on_resource_unavailable),
                                  &discover);
        }

        g_print ("Showing \"%s\" messages\n", message_type);

        discover.main_loop = g_main_loop_new (NULL, FALSE);

        gssdp_resource_browser_set_active (discover.browser, TRUE);

        if (timeout > 0)
                g_timeout_add_seconds (timeout,
                                       (GSourceFunc) g_main_loop_quit,
                                       discover.main_loop);

        if (rescan_interval > 0 &&
            (rescan_interval < timeout || timeout == 0))
                g_timeout_add_seconds (rescan_interval,
                                       (GSourceFunc) on_force_rescan_timeout,
                                       &discover);

        g_main_loop_run (discover.main_loop);

        g_main_loop_unref (discover.main_loop);
        g_object_unref (discover.browser);
        g_object_unref (discover.client);

        g_free (interface);
        g_free (target);
        g_free (message_type);

        return 0;
}
