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

#include <libgupnp/gupnp.h>
#include "gui.h"
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <gmodule.h>
#include <glib/gi18n.h>

static int upnp_port = 0;
static char **interfaces = NULL;
static gboolean ipv4 = TRUE;
static gboolean ipv6 = TRUE;
static GSSDPUDAVersion uda_version = GSSDP_UDA_VERSION_1_0;

static gboolean
parse_uda_version (const gchar *option_name,
                   const gchar *value,
                   gpointer data,
                   GError **error);

// clang-format off
static GOptionEntry entries[] =
{
        { "port", 'p', 0, G_OPTION_ARG_INT, &upnp_port, N_("Network PORT to use for UPnP"), "PORT" },
        { "interface", 'i', 0, G_OPTION_ARG_STRING_ARRAY, &interfaces, N_("Network interfaces to use for UPnP communication"), "INTERFACE" },
        { "v4", '4', 0, G_OPTION_ARG_NONE, &ipv4, N_("Use the IPv4 protocol family"), NULL },
        { "v6", '6', 0, G_OPTION_ARG_NONE, &ipv6, N_("Use the IPv6 protocol family"), NULL },
        { "no-v4", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &ipv4, N_("Do not use the IPv4 protocol family"), NULL },
        { "no-v6", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &ipv6, N_("Do not use the IPv6 protocol family"), NULL },
        { "uda-version", 'v', 0, G_OPTION_ARG_CALLBACK, parse_uda_version, N_("The UDA version to use"), "VERSION"},
        { NULL, 0, 0, 0, NULL, NULL, NULL }
};
// clang-format on

static gboolean
parse_uda_version (const gchar *option_name,
                   const gchar *value,
                   gpointer data,
                   GError **error)
{
        if (g_str_equal (value, "1.0")) {
                uda_version = GSSDP_UDA_VERSION_1_0;
        } else if (g_str_equal (value, "1.1")) {
                uda_version = GSSDP_UDA_VERSION_1_1;
        } else {
                g_set_error (error,
                             G_IO_ERROR,
                             G_IO_ERROR_INVALID_ARGUMENT,
                             "Invalid UDA version: %s", value);

                return FALSE;
        }

        return TRUE;
}

void
application_exit (void);

static GUPnPContextManager *context_manager;

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
on_context_available (GUPnPContextManager *manager,
                      GUPnPContext        *context,
                      gpointer             user_data)
{
        GUPnPControlPoint *cp;

        /* We're interested in everything */
        cp = gupnp_control_point_new (context, "upnp:rootdevice");

        g_signal_connect (cp,
                          "device-proxy-available",
                          G_CALLBACK (device_proxy_available_cb),
                          NULL);
        g_signal_connect (cp,
                          "device-proxy-unavailable",
                          G_CALLBACK (device_proxy_unavailable_cb),
                          NULL);

        gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

        /* Let context manager take care of the control point life cycle */
        gupnp_context_manager_manage_control_point (context_manager, cp);

        /* We don't need to keep our own reference to the control point */
        g_object_unref (cp);
}

static gboolean
init_upnp (void)
{
        GUPnPContextFilter *context_filter;

        GSocketFamily family = G_SOCKET_FAMILY_INVALID;

        if (!(ipv4 && ipv6)) {
                if (ipv4) {
                        family = G_SOCKET_FAMILY_IPV4;
                } else if (ipv6) {
                        family = G_SOCKET_FAMILY_IPV6;
                }
        }

        context_manager = gupnp_context_manager_create_full (uda_version,
                                                             family,
                                                             upnp_port);
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

        if (!init_ui (&argc, &argv)) {
           return -2;
        }

        if (!init_upnp ()) {
           return -3;
        }

        gtk_main ();

        return 0;
}

