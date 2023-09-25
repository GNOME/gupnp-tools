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
#include <libgupnp-av/gupnp-av.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>

#include "container-search.h"
#include "main.h"

static void
didl_container_available_cb (GUPnPDIDLLiteParser *parser,
                             GUPnPDIDLLiteObject *container,
                             gpointer             user_data)
{
        char **container_id = (char **) user_data;

        if (!gupnp_didl_lite_object_get_restricted (container)) {
                /* Seems like we found a suitable container */
                *container_id = g_strdup (gupnp_didl_lite_object_get_id
                                                                (container));
                g_print ("Automatically choosing '%s' (%s) as destination.\n",
                         gupnp_didl_lite_object_get_title (container),
                         *container_id);
        }
}

static char *
parse_result (const char *result)
{
        GUPnPDIDLLiteParser *parser;
        char *container_id;
        GError *error;

        parser = gupnp_didl_lite_parser_new ();
        g_signal_connect (parser,
                          "container-available",
                          G_CALLBACK (didl_container_available_cb),
                          &container_id);

        container_id = NULL;
        error = NULL;
        if (!gupnp_didl_lite_parser_parse_didl (parser, result, &error)) {
                g_critical ("Failed to parse result DIDL from MediaServer: %s",
                            error->message);

                g_error_free (error);
        }

        g_object_unref (parser);

        return container_id;
}

static void
browse_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        g_autoptr (GError) error = NULL;
        g_autofree char *result = NULL;
        g_autofree char *container_id = NULL;
        GUPnPServiceProxyAction *action = NULL;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);

        if (error != NULL) {
                g_critical ("Failed to browse root container: %s",
                            error->message);
                application_exit ();

                return;
        }

        gupnp_service_proxy_action_get_result (action,
                                               &error,
                                               "Result",
                                               G_TYPE_STRING,
                                               &result,
                                               NULL);
        if (error != NULL) {
                g_critical ("Failed to browse root container: %s",
                            error->message);
                application_exit ();

                return;
        }

        g_assert (result != NULL);

        container_id = parse_result (result);
        if (container_id == NULL) {
                g_critical ("Failed to find a suitable container for upload.");

                application_exit ();
        } else {
                container_found (container_id);
        }
}

void
search_container (GUPnPServiceProxy *cds_proxy)
{
        GUPnPServiceProxyAction *action = NULL;
        action = gupnp_service_proxy_action_new ("Browse",
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

        gupnp_service_proxy_call_action_async (cds_proxy,
                                               action,
                                               NULL,
                                               browse_cb,
                                               NULL);

        gupnp_service_proxy_action_unref (action);
}

