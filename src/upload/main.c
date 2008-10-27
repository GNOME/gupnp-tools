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
#include "main.h"

static GMainLoop         *main_loop;
static GUPnPContext      *upnp_context;
static GUPnPServiceProxy *cds_proxy;

static GList *files = NULL;
static char *udn;

static char *title = NULL;
static char *dest_container = NULL;
static guint search_timeout = 0;

static GOptionEntry entries[] =
{
        { "search-timeout", 't', 0,
          G_OPTION_ARG_INT, &search_timeout,
          "Search Timeout", "seconds" },
        { "container-id", 'c', 0,
          G_OPTION_ARG_STRING, &dest_container,
          "Destination Container ID", "CONTAINER_ID" },
        { "title", 'i', 0,
          G_OPTION_ARG_STRING, &title,
          "Title for item", "TITLE" },
        { NULL }
};

static void
goto_next_file (void)
{
        files = g_list_next (files);

        if (files != NULL) {
                create_item ((char *) files->data,
                             title,
                             cds_proxy,
                             dest_container,
                             upnp_context);
        } else {
                /* Exit if there are no more files to upload */
                application_exit ();
        }
}

void
transfer_completed (void)
{
        goto_next_file ();
}

void
container_found (const char *container_id)
{
        dest_container = container_id;

        /* Now create the item container */
        create_item ((char *) files->data,
                     title,
                     cds_proxy,
                     dest_container,
                     upnp_context);
}

void
item_created (const char *import_uri) {
        if (import_uri == NULL) {
                /* Item creation failed, lets try next file */
                goto_next_file ();
        } else {
                start_transfer ((char *) files->data,
                                import_uri,
                                cds_proxy,
                                upnp_context);
        }
}

void
target_cds_found (GUPnPServiceProxy *target_cds_proxy)
{
        cds_proxy = target_cds_proxy;

        if (dest_container != NULL) {
                container_found (dest_container);
        } else {
                /* Find a suitable container */
                search_container (target_cds_proxy);
        }
}

void
application_exit (void)
{
        g_main_loop_quit (main_loop);
}

gint
main (gint   argc,
      gchar *argv[])
{
        GError *error;
        gint i;

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
                g_print ("Usage: %s UDN FILE_PATH...\n", argv[0]);

                return -4;
        }

        udn = argv[1];

        /* Get the list of files to upload */
        for (i = 2; i < argc; i++) {
                if (!g_file_test (argv[i],
                                  G_FILE_TEST_EXISTS |
                                  G_FILE_TEST_IS_REGULAR)) {
                        g_critical ("File %s does not exist", argv[i]);
                } else {
                        files = g_list_append (files, argv[i]);
                }
        }

        if (files == NULL) {
                return -5;
        }

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

        if (!init_control_point (upnp_context, udn, search_timeout)) {
           return -3;
        }

        main_loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (main_loop);

        /* Clean-up */
        g_main_loop_unref (main_loop);
        deinit_control_point ();
        g_object_unref (upnp_context);
        g_option_context_free (context);

        return 0;
}

