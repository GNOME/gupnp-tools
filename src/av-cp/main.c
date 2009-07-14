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

#define MEDIA_RENDERER "urn:schemas-upnp-org:device:MediaRenderer:1"
#define MEDIA_SERVER "urn:schemas-upnp-org:device:MediaServer:1"

static GUPnPContextManager *context_manager;
static GList *control_points;
static GList *contexts;

static gboolean
context_equal (GUPnPContext *context1, GUPnPContext *context2)
{
        return g_ascii_strcasecmp (gupnp_context_get_name (context1),
                                   gupnp_context_get_name (context2)) == 0 &&
               g_ascii_strcasecmp (gupnp_context_get_host_ip (context1),
                                   gupnp_context_get_host_ip (context2)) == 0;
}

static void
dms_proxy_available_cb (GUPnPControlPoint *cp,
                        GUPnPDeviceProxy  *proxy)
{
        add_media_server (proxy);
}

static void
dmr_proxy_available_cb (GUPnPControlPoint *cp,
                        GUPnPDeviceProxy  *proxy)
{
        add_media_renderer (proxy);
}

static void
dms_proxy_unavailable_cb (GUPnPControlPoint *cp,
                          GUPnPDeviceProxy  *proxy)
{
        remove_media_server (proxy);
}

static void
dmr_proxy_unavailable_cb (GUPnPControlPoint *cp,
                          GUPnPDeviceProxy  *proxy)
{
        remove_media_renderer (proxy);
}

static void
on_context_available (GUPnPContextManager *context_manager,
                      GUPnPContext        *context,
                      gpointer            *user_data)
{
        GUPnPControlPoint *dms_cp;
        GUPnPControlPoint *dmr_cp;

        contexts = g_list_append (contexts, g_object_ref (context));

        dms_cp = gupnp_control_point_new (context, MEDIA_SERVER);
        dmr_cp = gupnp_control_point_new (context, MEDIA_RENDERER);

        control_points = g_list_append (control_points, dms_cp);
        control_points = g_list_append (control_points, dmr_cp);

        g_signal_connect (dms_cp,
                          "device-proxy-available",
                          G_CALLBACK (dms_proxy_available_cb),
                          NULL);
        g_signal_connect (dmr_cp,
                          "device-proxy-available",
                          G_CALLBACK (dmr_proxy_available_cb),
                          NULL);
        g_signal_connect (dms_cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (dms_proxy_unavailable_cb),
                          NULL);
        g_signal_connect (dmr_cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (dmr_proxy_unavailable_cb),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (dms_cp),
                                           TRUE);
        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (dmr_cp),
                                           TRUE);
}

static void
on_context_unavailable (GUPnPContextManager *context_manager,
                        GUPnPContext        *context,
                        gpointer            *user_data)
{
        GList *l = control_points;

        while (l) {
                GUPnPControlPoint *cp = GUPNP_CONTROL_POINT (l->data);

                if (context_equal (context,
                                   gupnp_control_point_get_context (cp))) {
                        GList *next = l->next;

                        control_points = g_list_delete_link (control_points, l);
                        l = next;

                        g_object_unref (cp);
                } else {
                        l = l->next;
                }
        }

        contexts = g_list_remove (contexts, context);
        g_object_unref (context);
}

static gboolean
init_upnp (void)
{
        g_type_init ();

        control_points = NULL;
        contexts = NULL;

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

        while (control_points) {
                g_object_unref (control_points->data);

                control_points = g_list_delete_link (control_points,
                                                     control_points);
        }

        while (contexts) {
                g_object_unref (contexts->data);

                contexts = g_list_delete_link (contexts, contexts);
        }
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

