/*
 * Copyright (C) 2007 Zeeshan Ali.
 * Copyright (C) 2007 OpenedHand Ltd.
 * Copyright (C) 2013 Jens Georg <mail@jensge.org>
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *          Jorn Baayen <jorn@openedhand.com>
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

#include <config.h>

#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <glib/gi18n.h>

#include "gui.h"
#include "upnp.h"
#include "main.h"

static gboolean light_status;
static gint     light_load_level;

static int      upnp_port = 0;
static char   **interfaces = NULL;
static char    *name = NULL;
static gboolean exclusive;
static gboolean ipv4 = TRUE;
static gboolean ipv6 = TRUE;

// clang-format off
static GOptionEntry entries[] =
{
        { "port", 'p', 0, G_OPTION_ARG_INT, &upnp_port, N_("Network PORT to use for UPnP"), "PORT" },
        { "interface", 'i', 0, G_OPTION_ARG_STRING_ARRAY, &interfaces, N_("Network interfaces to use for UPnP communication"), "INTERFACE" },
        { "name", 'n', 0, G_OPTION_ARG_STRING, &name, N_("Friendly name for this UPnP light"), "NAME" },
        { "exclusive", 'x', 0, G_OPTION_ARG_NONE, &exclusive, N_("Apply change exclusively to this UPnP light"), NULL },
        { "v4", '4', 0, G_OPTION_ARG_NONE, &ipv4, N_("Use IPv4"), NULL },
        { "v6", '6', 0, G_OPTION_ARG_NONE, &ipv6, N_("Use IPv6"), NULL },
        { "no-v4",  0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &ipv4, N_("Do not use IPv4"), NULL },
        { "no-v6", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &ipv6, N_("Do not use IPv6"), NULL },
        { NULL }
};
// clang-format on

void
set_status (gboolean status)
{
        if (status != light_status) {
                light_status = status;
                update_image ();

                notify_status_change (status);
        }
}

gboolean
get_status (void)
{
        return light_status;
}

void
set_load_level (gint load_level)
{
        if (load_level != light_load_level) {
                light_load_level = CLAMP (load_level, 0, 100);
                update_image ();

                notify_load_level_change (light_load_level);
        }
}

gint
get_load_level (void)
{
        return light_load_level;
}

int
main (int argc, char **argv)
{
        GError *error = NULL;
        GOptionContext *context = NULL;

        /* Light is off in the beginning */
        light_status = FALSE;
        light_load_level = 100;

        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        context = g_option_context_new (_("- UPnP AV control point"));
        g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
        g_option_context_add_group (context, gtk_get_option_group (TRUE));

        if (!g_option_context_parse (context, &argc, &argv, &error)) {
                g_print (_("Could not parse options: %s\n"), error->message);

                g_option_context_free (context);

                return -4;
        }

        g_option_context_free (context);

        if (!init_ui (&argc, &argv, name, exclusive)) {
                return -1;
        }

        if (!init_upnp (interfaces, upnp_port, name, ipv4, ipv6)) {
                return -2;
        }

        gtk_main ();

        deinit_ui ();
        deinit_upnp ();

        return 0;
}
