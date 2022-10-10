/*
 * Copyright (C) 2007 Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 * Copyright (C) 2013 Jens Georg <mail@jensge.org>
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *          Jens Georg <mail@jensge.org>
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

#include "config.h"

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <gmodule.h>
#include <glib/gi18n.h>

#include "gui.h"
#include "renderer-combo.h"
#include "playlist-treeview.h"
#include "server-device.h"

#define MEDIA_RENDERER "urn:schemas-upnp-org:device:MediaRenderer:1"
#define MEDIA_SERVER "urn:schemas-upnp-org:device:MediaServer:1"

static int upnp_port = 0;
static char **interfaces = NULL;
static char *user_agent = NULL;
static gboolean ipv4 = TRUE;
static gboolean ipv6 = TRUE;

// clang-format off
static GOptionEntry entries[] =
{
        { "port", 'p', 0, G_OPTION_ARG_INT, &upnp_port, N_("Network PORT to use for UPnP"), "PORT" },
        { "interface", 'i', 0, G_OPTION_ARG_STRING_ARRAY, &interfaces, N_("Network interfaces to use for UPnP communication"), "INTERFACE" },
        { "user-agent", 'u', 0, G_OPTION_ARG_STRING, &user_agent, N_("Application part of the User-Agent header to use for UPnP communication"), "USER-AGENT" },
        { "v4", '4', 0, G_OPTION_ARG_NONE, &ipv4, N_("Use the IPv4 protocol family"), NULL },
        { "v6", '6', 0, G_OPTION_ARG_NONE, &ipv6, N_("Use the IPv6 protocol family"), NULL },
        { "no-v4", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &ipv4, N_("Do not use the IPv4 protocol family"), NULL },
        { "no-v6", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &ipv6, N_("Do not use the IPv6 protocol family"), NULL },
        { NULL }
};
// clang-format on

static GUPnPContextManager *context_manager;

void application_exit (void);

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
on_rescan_button_clicked (GtkButton *button,
                          gpointer user_data)
{
        GUPnPContextManager *cm = GUPNP_CONTEXT_MANAGER (user_data);

        gupnp_context_manager_rescan_control_points (cm);
}

static void
on_context_available (GUPnPContextManager *cm,
                      GUPnPContext        *context,
                      gpointer             user_data)
{
        GUPnPControlPoint *dms_cp;
        GUPnPControlPoint *dmr_cp;

        dms_cp = gupnp_control_point_new (context, MEDIA_SERVER);
        dmr_cp = gupnp_control_point_new (context, MEDIA_RENDERER);

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

        /* Let context manager take care of the control point life cycle */
        gupnp_context_manager_manage_control_point (context_manager, dms_cp);
        gupnp_context_manager_manage_control_point (context_manager, dmr_cp);

        /* We don't need to keep our own references to the control points */
        g_object_unref (dms_cp);
        g_object_unref (dmr_cp);
}

static gboolean
init_upnp (int port)
{
        GUPnPContextFilter *context_filter;
        GtkButton *button;
        GUPnPResourceFactory *factory;
        GSocketFamily family = G_SOCKET_FAMILY_INVALID;

        factory = gupnp_resource_factory_get_default ();

        /* Work-around bgo#764498 */

        gupnp_resource_factory_register_resource_proxy_type (
                factory,
                "urn:schemas-upnp-org:device:MediaServer:1",
                AV_CP_TYPE_MEDIA_SERVER);

        gupnp_resource_factory_register_resource_proxy_type (
                factory,
                "urn:schemas-upnp-org:device:MediaServer:2",
                AV_CP_TYPE_MEDIA_SERVER);

        gupnp_resource_factory_register_resource_proxy_type (
                factory,
                "urn:schemas-upnp-org:device:MediaServer:3",
                AV_CP_TYPE_MEDIA_SERVER);

        gupnp_resource_factory_register_resource_proxy_type (
                factory,
                "urn:schemas-upnp-org:device:MediaServer:4",
                AV_CP_TYPE_MEDIA_SERVER);

        if (!(ipv4 && ipv6)) {
                if (ipv4) {
                        family = G_SOCKET_FAMILY_IPV4;
                } else if (ipv6) {
                        family = G_SOCKET_FAMILY_IPV6;
                }
        }

        context_manager =
                gupnp_context_manager_create_full (GSSDP_UDA_VERSION_1_0,
                                                   family,
                                                   port);
        g_assert (context_manager != NULL);

        if (interfaces != NULL) {
                context_filter = gupnp_context_manager_get_context_filter (
                        context_manager);
                gupnp_context_filter_add_entryv (context_filter, interfaces);
                gupnp_context_filter_set_enabled (context_filter, TRUE);
        }

        g_signal_connect (context_manager,
                          "context-available",
                          G_CALLBACK (on_context_available),
                          NULL);

        button = get_rescan_button ();
        g_signal_connect (G_OBJECT (button),
                          "clicked",
                          G_CALLBACK (on_rescan_button_clicked),
                          context_manager);

        return TRUE;
}

static void
deinit_upnp (void)
{
        g_object_unref (context_manager);
}

G_MODULE_EXPORT
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
        GError *error = NULL;
        GOptionContext *context = NULL;

        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        context = g_option_context_new (_("- UPnP AV control point"));
        g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
        g_option_context_add_group (context, gtk_get_option_group (TRUE));

        if (!g_option_context_parse (context, &argc, &argv, &error)) {
                g_print (_("Could not parse options: %s\n"), error->message);

                return -4;
        }

        if (user_agent != NULL) {
                g_set_prgname (user_agent);
        }

        if (!init_ui ()) {
           return -2;
        }

        if (!init_upnp (upnp_port)) {
           return -3;
        }

        gtk_main ();

        g_option_context_free (context);

        return 0;
}

