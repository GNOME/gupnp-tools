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
#include <string.h>
#include <stdlib.h>

#include "av-cp-gui.h"
#include "av-cp-renderercombo.h"

#define MEDIA_RENDERER_V1 "urn:schemas-upnp-org:device:MediaRenderer:1"
#define MEDIA_RENDERER_V2 "urn:schemas-upnp-org:device:MediaRenderer:2"

static GUPnPContext      *context;
static GUPnPControlPoint *cp;

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
                           GUPnPDeviceProxy  *proxy)
{
        GUPnPDeviceInfo *info;
        const gchar     *type;

        info = GUPNP_DEVICE_INFO (proxy);
        type = gupnp_device_info_get_device_type (info);

        if (strcmp (MEDIA_RENDERER_V1, type) == 0 ||
            strcmp (MEDIA_RENDERER_V2, type) == 0) {
                add_media_renderer (proxy);
        }
}

static void
device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                             GUPnPDeviceProxy  *proxy)
{
        GUPnPDeviceInfo *info;
        const gchar     *type;

        info = GUPNP_DEVICE_INFO (proxy);
        type = gupnp_device_info_get_device_type (info);

        if (strcmp (MEDIA_RENDERER_V1, type) == 0 ||
            strcmp (MEDIA_RENDERER_V2, type) == 0) {
                remove_media_renderer (proxy);
        }
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
        gtk_main_quit ();

        deinit_upnp ();
        deinit_ui ();
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

