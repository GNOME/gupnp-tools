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

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>
#include <string.h>
#include <stdlib.h>

#include "gui.h"
#include "renderer-combo.h"
#include "playlist-treeview.h"

#define MEDIA_RENDERER "urn:schemas-upnp-org:device:MediaRenderer:*"
#define MEDIA_SERVER "urn:schemas-upnp-org:device:MediaServer:*"

static GUPnPContext      *context;
static GUPnPControlPoint *cp;

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
                           GUPnPDeviceProxy  *proxy)
{
        const char *type;

        type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));

        if (g_pattern_match_simple (MEDIA_RENDERER, type)) {
                add_media_renderer (proxy);
        } else if (g_pattern_match_simple (MEDIA_SERVER, type)) {
                add_media_server (proxy);
        }
}

static void
device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                             GUPnPDeviceProxy  *proxy)
{
        const char *type;

        type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));

        if (g_pattern_match_simple (MEDIA_RENDERER, type)) {
                remove_media_renderer (proxy);
        } else if (g_pattern_match_simple (MEDIA_SERVER, type)) {
                remove_media_server (proxy);
        }
}

static gboolean
init_upnp (void)
{
        GError *error = NULL;

        g_type_init ();

        context = gupnp_context_new (NULL, NULL, 0, &error);
        if (error) {
                g_printerr ("Error creating the GUPnP context: %s\n",
			    error->message);
                g_error_free (error);

                return FALSE;
        }

        /* We're interested in everything */
        cp = gupnp_control_point_new (context, "ssdp:all");

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
        /* First thing initialze the thread system */
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

