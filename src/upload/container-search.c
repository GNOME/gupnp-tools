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
#include <libgupnp-av/gupnp-av.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"

static void
didl_object_available_cb (GUPnPDIDLLiteParser *parser,
                          xmlNode             *object_node,
                          gpointer             user_data)
{
        char **container_id = (char **) user_data;

        if (!gupnp_didl_lite_object_is_container (object_node)) {
                return;
        }

        if (!gupnp_didl_lite_object_get_restricted (object_node)) {
                char *title;

                /* Seems like we found a suitable container */
                *container_id = gupnp_didl_lite_object_get_id (object_node);
                title = gupnp_didl_lite_object_get_title (object_node);
                g_print ("Automatically choosing '%s' (%s) as destination.\n",
                         title,
                         *container_id);

                g_free (title);
        }
}

static char *
parse_result (const char *result)
{
        GUPnPDIDLLiteParser *parser;
        char *container_id;
        GError *error;

        parser = gupnp_didl_lite_parser_new ();

        container_id = NULL;
        error = NULL;
        if (!gupnp_didl_lite_parser_parse_didl (parser,
                                                result,
                                                didl_object_available_cb,
                                                &container_id,
                                                &error)) {
                g_critical ("Failed to parse result DIDL from MediaServer: %s",
                            error->message);

                g_error_free (error);
        }

        g_object_unref (parser);

        return container_id;
}

static void
browse_cb (GUPnPServiceProxy       *cds_proxy,
           GUPnPServiceProxyAction *action,
           gpointer                 user_data)
{
        GError *error;
        char *result;
        char *container_id;

        error = NULL;
        if (!gupnp_service_proxy_end_action (cds_proxy,
                                             action,
                                             &error,
                                             "Result",
                                                G_TYPE_STRING,
                                                &result,
                                             NULL)) {
                g_critical ("Failed to browse root container: %s",
                            error->message);

                g_error_free (error);

                application_exit ();

                return;
        }

        g_assert (result != NULL);

        container_id = parse_result (result);
        if (container_id == NULL) {
                g_critical ("Failed to find a suitable container for upload.");
                g_free (result);

                application_exit ();

                return;
        } else {
                container_found (container_id);
        }

        g_free (result);
}

void
search_container (GUPnPServiceProxy *cds_proxy)
{
        gupnp_service_proxy_begin_action (cds_proxy,
                                          "Browse",
                                          browse_cb,
                                          NULL,
                                          /* IN args */
                                          "ObjectID",
                                                G_TYPE_STRING,
                                                "0",
                                          "BrowseFlag",
                                                G_TYPE_STRING,
                                                "BrowseDirectChildren",
                                          "Filter",
                                                G_TYPE_STRING,
                                                "*",
                                          "StartingIndex",
                                                G_TYPE_UINT,
                                                0,
                                          "RequestedCount",
                                                G_TYPE_UINT,
                                                0,
                                          "SortCriteria",
                                                G_TYPE_STRING,
                                                "",
                                          NULL);
}

