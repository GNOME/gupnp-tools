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
#include <string.h>
#include <stdlib.h>
#include <config.h>

#include "control_point.h"
#include "item-creation.h"
#include "transfer.h"
#include "container-search.h"

static GMainLoop         *main_loop;
static GUPnPContext      *upnp_context;
static GUPnPServiceProxy *cds_proxy;

static char *file_path;
static char *udn;

static char *title = NULL;
static char *container_id = NULL;
static guint search_timeout = 0;

static GOptionEntry entries[] =
{
        { "search-timeout", 't', 0,
          G_OPTION_ARG_INT, &search_timeout,
          "Search Timeout", "seconds" },
        { "container-id", 'c', 0,
          G_OPTION_ARG_STRING, &container_id,
          "Destination Container ID", "CONTAINER_ID" },
        { "title", 'i', 0,
          G_OPTION_ARG_STRING, &title,
          "Title for item", "TITLE" },
        { NULL }
};

void
container_found (const char *container_id)
{
        /* Now create the item container */
        create_item (file_path,
                     title,
                     cds_proxy,
                     container_id,
                     upnp_context);
}

void
item_created (const char *import_uri) {
        start_transfer (file_path,
                        import_uri,
                        cds_proxy,
                        upnp_context);
}

void
target_cds_found (GUPnPServiceProxy *target_cds_proxy)
{
        cds_proxy = target_cds_proxy;

        if (container_id != NULL) {
                create_item (file_path,
                             title,
                             cds_proxy,
                             container_id,
                             upnp_context);
        } else {
                /* Find a suitable container */
                search_container (target_cds_proxy);
        }
}

void
application_exit (void)
{
        g_main_loop_quit (main_loop);

        deinit_control_point ();
}

gint
main (gint   argc,
      gchar *argv[])
{
        GError *error;

        error = NULL;
        GOptionContext *context;

        context = g_option_context_new ("- Upload file to UPnP MediaServer");
        g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
        if (!g_option_context_parse (context, &argc, &argv, &error))
        {
                g_print ("option parsing failed: %s\n", error->message);
                return -1;
        }

        if (argc < 3) {
                g_print ("Usage: %s FILE_PATH UDN\n", argv[0]);

                return -4;
        }

        file_path = argv[1];
        udn = argv[2];

        if (!g_thread_supported ()) {
                g_thread_init (NULL);
        }

        g_type_init ();

        error = NULL;
        upnp_context = gupnp_context_new (NULL, NULL, 0, &error);
        if (error) {
                g_critical (error->message);
                g_error_free (error);

                return -6;
        }

        if (!g_file_test (file_path,
                          G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
           g_critical ("File %s does not exist", file_path);

           return -5;
        }

        if (!init_control_point (upnp_context, udn, search_timeout)) {
           return -3;
        }

        main_loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (main_loop);

        g_object_unref (main_loop);
        g_object_unref (upnp_context);
        g_option_context_free (context);

        return 0;
}

