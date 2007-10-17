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
#include "av-cp-playlisttreeview.h"
#include "media-renderer-proxy.h"
#include "media-server-proxy.h"

#define MEDIA_RENDERER_V1 "urn:schemas-upnp-org:device:MediaRenderer:1"
#define MEDIA_RENDERER_V2 "urn:schemas-upnp-org:device:MediaRenderer:2"
#define MEDIA_SERVER_V1 "urn:schemas-upnp-org:device:MediaServer:1"
#define MEDIA_SERVER_V2 "urn:schemas-upnp-org:device:MediaServer:2"

static GUPnPContext      *context;
static GUPnPControlPoint *cp;

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
                           GUPnPDeviceProxy  *proxy)
{
        if (G_OBJECT_TYPE (proxy) == TYPE_MEDIA_RENDERER_PROXY) {
                add_media_renderer (MEDIA_RENDERER_PROXY (proxy));
        } else if (G_OBJECT_TYPE (proxy) == TYPE_MEDIA_SERVER_PROXY) {
                add_media_server (MEDIA_SERVER_PROXY (proxy));
        }
}

static void
device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                             GUPnPDeviceProxy  *proxy)
{
        if (G_OBJECT_TYPE (proxy) == TYPE_MEDIA_RENDERER_PROXY) {
                remove_media_renderer (MEDIA_RENDERER_PROXY (proxy));
        } else if (G_OBJECT_TYPE (proxy) == TYPE_MEDIA_SERVER_PROXY) {
                remove_media_server (MEDIA_SERVER_PROXY (proxy));
        }
}

static gboolean
init_upnp (void)
{
        GUPnPResourceFactory *factory;
        GError               *error;

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

        factory = gupnp_control_point_get_resource_factory (cp);
        g_assert (factory != NULL);
        gupnp_resource_factory_register_resource_proxy_type
                                        (factory,
                                         MEDIA_RENDERER_V1,
                                         TYPE_MEDIA_RENDERER_PROXY);
        gupnp_resource_factory_register_resource_proxy_type
                                        (factory,
                                         MEDIA_RENDERER_V2,
                                         TYPE_MEDIA_RENDERER_PROXY);
        gupnp_resource_factory_register_resource_proxy_type
                                        (factory,
                                         MEDIA_SERVER_V1,
                                         TYPE_MEDIA_SERVER_PROXY);
        gupnp_resource_factory_register_resource_proxy_type
                                        (factory,
                                         MEDIA_SERVER_V2,
                                         TYPE_MEDIA_SERVER_PROXY);

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

