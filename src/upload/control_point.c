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

#include <config.h>

#include <libgupnp/gupnp.h>
#include <string.h>
#include <stdlib.h>

#include "control_point.h"
#include "main.h"

#define CDS "urn:schemas-upnp-org:service:ContentDirectory"

static GUPnPControlPoint *cp;
static GUPnPServiceProxy *cds_proxy = NULL;

static guint timeout_id = 0;

static void
device_proxy_unavailable_cb (GUPnPControlPoint *control_point,
                             GUPnPDeviceProxy  *proxy)
{
        /* We don't need to check if it's our proxy */
        application_exit ();
}

static void
device_proxy_available_cb (GUPnPControlPoint *control_point,
                           GUPnPDeviceProxy  *proxy)
{
        GUPnPDeviceInfo *info;

        info = GUPNP_DEVICE_INFO (proxy);

        cds_proxy = GUPNP_SERVICE_PROXY (
                        gupnp_device_info_get_service (info, CDS));
        if (G_UNLIKELY (cds_proxy == NULL)) {
                g_critical ("MediaServer '%s' doesn't provide ContentDirectory"
                            "service", gupnp_device_info_get_udn (info));

                application_exit ();

                return;
        }

        if (timeout_id > 0) {
                /* No need to timeout anymore */
                g_source_remove (timeout_id);
        }

        /* No need to bother searching anymore */
        g_signal_handlers_disconnect_by_func (
                                        cp,
                                        G_CALLBACK (device_proxy_available_cb),
                                        NULL);

        /* But we still want to know when our device goes away */
        g_signal_connect (cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (device_proxy_unavailable_cb),
                          NULL);

        target_cds_found (cds_proxy);
}

gboolean
init_control_point (GUPnPContext *context,
                    const char   *udn,
                    guint         timeout)
{
        /* We're interested in everything at this stage */
        cp = gupnp_control_point_new (context, udn);

        g_signal_connect (cp,
                          "device-proxy-available",
                          G_CALLBACK (device_proxy_available_cb),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

        if (timeout > 0) {
                timeout_id =
                        g_timeout_add_seconds (timeout,
                                               (GSourceFunc) application_exit,
                                               NULL);
        }

        return TRUE;
}

void
deinit_control_point (void)
{
        if (cds_proxy != NULL) {
                g_object_unref (cds_proxy);
        }

        g_object_unref (cp);
}

