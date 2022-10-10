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
#include <stdlib.h>
#include <string.h>

#include "control_point.h"
#include "item-creation.h"
#include "transfer.h"
#include "container-search.h"
#include "main.h"

static GMainLoop         *main_loop;
static GUPnPContext      *upnp_context;
static GUPnPServiceProxy *cds_proxy;

static GList *files = NULL;
static char *udn = NULL;
static char *interface = NULL;

static const char *title = NULL;
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
        { "interface", 'e', 0,
          G_OPTION_ARG_STRING, &interface,
          "Network interface to search MediaServer on", "INTERFACE" },
        { "udn", 'u', 0,
          G_OPTION_ARG_STRING, &udn,
          "UDN of the device to upload to", "UDN" },
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
                             dest_container);
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
        dest_container = g_strdup (container_id);

        /* Now create the item container */
        create_item ((char *) files->data,
                     title,
                     cds_proxy,
                     dest_container);
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
        if (main_loop != NULL)
                g_main_loop_quit (main_loop);
}

gint
main (gint   argc,
      gchar *argv[])
{
        g_autoptr (GError) error = NULL;
        gint i;
        g_autoptr (GOptionContext) context;

        context = g_option_context_new ("- Upload files to UPnP MediaServer");
        g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
        if (!g_option_context_parse (context, &argc, &argv, &error))
        {
                g_print ("option parsing failed: %s\n", error->message);
                return -1;
        }

        if (argc < 2) {
                g_autofree char *help = NULL;

                help = g_option_context_get_help (context, TRUE, NULL);
                g_print ("%s\n", help);

                return -4;
        }

        if (udn == NULL) {
                g_print ("Error: Please provide UDN for the target server\n");
                return -4;
        }

        /* Get the list of files to upload */
        for (i = 1; i < argc; i++) {
                if (!g_file_test (argv[i],
                                  G_FILE_TEST_EXISTS |
                                  G_FILE_TEST_IS_REGULAR)) {
                        g_printerr ("File %s does not exist\n", argv[i]);
                } else {
                        files = g_list_append (files, argv[i]);
                }
        }

        if (files == NULL) {
                return -5;
        }

        error = NULL;
        upnp_context = gupnp_context_new_full (interface,
                                               NULL,
                                               0,
                                               GSSDP_UDA_VERSION_1_0,
                                               &error);
        if (error) {
                g_printerr ("Error creating the GUPnP context: %s\n",
                            error->message);

                return -6;
        }

        g_print ("UPnP context created for interface %s (%s)\n",
                 gssdp_client_get_interface (GSSDP_CLIENT (upnp_context)),
                 gssdp_client_get_host_ip (GSSDP_CLIENT (upnp_context)));

        if (!init_control_point (upnp_context, udn, search_timeout)) {
           return -3;
        }

        main_loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (main_loop);

        /* Clean-up */
        g_clear_pointer (&main_loop, g_main_loop_unref);
        deinit_control_point ();
        g_object_unref (upnp_context);
        g_free (dest_container);
        g_free (interface);

        return 0;
}

