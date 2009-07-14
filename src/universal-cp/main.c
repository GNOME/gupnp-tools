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

#include <libgupnp/gupnp.h>
#include "gui.h"
#include <string.h>
#include <stdlib.h>

static GUPnPContextManager *context_manager;
static GHashTable *cp_hash;

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

static void
on_context_available (GUPnPContextManager *context_manager,
                      GUPnPContext        *context,
                      gpointer            *user_data)
{
        GUPnPControlPoint *cp;

        /* We're interested in everything */
        cp = gupnp_control_point_new (context, "upnp:rootdevice");

        g_hash_table_insert (cp_hash, g_object_ref (context), cp);

        g_signal_connect (cp,
                          "device-proxy-available",
                          G_CALLBACK (device_proxy_available_cb),
                          NULL);
        g_signal_connect (cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (device_proxy_unavailable_cb),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);
}

static void
on_context_unavailable (GUPnPContextManager *context_manager,
                        GUPnPContext        *context,
                        gpointer            *user_data)
{
        g_hash_table_remove (cp_hash, context);
}

static gboolean
context_equal (GUPnPContext *context1, GUPnPContext *context2)
{
        return g_ascii_strcasecmp (gupnp_context_get_name (context1),
                                   gupnp_context_get_name (context2)) == 0 &&
               g_ascii_strcasecmp (gupnp_context_get_host_ip (context1),
                                   gupnp_context_get_host_ip (context2)) == 0;
}

static gboolean
init_upnp (void)
{
        g_type_init ();

        cp_hash = g_hash_table_new_full (g_direct_hash,
                                         (GEqualFunc) context_equal,
                                         g_object_unref,
                                         g_object_unref);

        context_manager = gupnp_context_manager_new (NULL, 0);
        g_assert (context_manager != NULL);

        g_signal_connect (context_manager,
                          "context-available",
                          G_CALLBACK (on_context_available),
                          NULL);
        g_signal_connect (context_manager,
                          "context-unavailable",
                          G_CALLBACK (on_context_unavailable),
                          NULL);

        return TRUE;
}

static void
deinit_upnp (void)
{
        g_object_unref (context_manager);

        g_hash_table_unref (cp_hash);
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
        /* First thing initialize the thread system */
        g_thread_init (NULL);

        if (!init_ui (&argc, &argv)) {
           return -2;
        }

        if (!init_upnp ()) {
           return -3;
        }

        gtk_main ();

        return 0;
}

