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

#include "item-creation.h"
#include "main.h"

static const char *
guess_upnp_class_for_mime_type (const char *mime_type)
{
        const char *upnp_class = NULL;

        if (g_pattern_match_simple ("audio/*", mime_type)) {
                upnp_class = "object.item.audioItem.musicTrack";
        } else if (g_pattern_match_simple ("video/*", mime_type)) {
                upnp_class = "object.item.videoItem";
        } else if (g_pattern_match_simple ("image/*", mime_type)) {
                upnp_class = "object.item.imageItem";
        }

        return upnp_class;
}

static const char *
create_res_for_file (const char          *file_path,
                     GUPnPDIDLLiteObject *object)
{
        GUPnPDIDLLiteResource *res;
        GUPnPProtocolInfo *info;
        char *content_type;
        char *mime_type;
        const char *upnp_class;

        res = gupnp_didl_lite_object_add_resource (object);
        gupnp_didl_lite_resource_set_uri (res, "");

        content_type = g_content_type_guess (file_path, NULL, 0, NULL);
        mime_type = g_content_type_get_mime_type (content_type);
        upnp_class = guess_upnp_class_for_mime_type (mime_type);

        info = gupnp_protocol_info_new ();
        gupnp_protocol_info_set_mime_type (info, mime_type);
        gupnp_protocol_info_set_protocol (info, "*");

        gupnp_didl_lite_resource_set_protocol_info (res, info);

        g_object_unref (info);
        g_object_unref (res);
        g_free (mime_type);
        g_free (content_type);

        return upnp_class;
}

static char *
create_didl_for_file (const char *file_path,
                      const char *title,
                      const char *parent_id)
{
        GUPnPDIDLLiteWriter *writer;
        GUPnPDIDLLiteObject *item;
        char *didl;
        char *new_title;
        const char *upnp_class;

        writer = gupnp_didl_lite_writer_new (NULL);

        item = GUPNP_DIDL_LITE_OBJECT
                        (gupnp_didl_lite_writer_add_item (writer));
        gupnp_didl_lite_object_set_parent_id (item, parent_id);
        gupnp_didl_lite_object_set_id (item, "");
        gupnp_didl_lite_object_set_restricted (item, FALSE);

        if (title == NULL) {
                new_title = g_path_get_basename (file_path);
        } else {
                new_title = g_strdup (title);
        }
        gupnp_didl_lite_object_set_title (item, new_title);
        g_free (new_title);

        upnp_class = create_res_for_file (file_path, item);
        if (upnp_class == NULL) {
                g_critical ("Failed to guess UPnP class for file '%s'",
                            file_path);

                g_object_unref (writer);

                return NULL;
        } else
                gupnp_didl_lite_object_set_upnp_class (item, upnp_class);

        didl = gupnp_didl_lite_writer_get_string (writer);

        g_object_unref (item);
        g_object_unref (writer);

        return didl;
}

static void
didl_item_available_cb (GUPnPDIDLLiteParser *parser,
                        GUPnPDIDLLiteObject *object,
                        gpointer             user_data)
{
        GList *resources;
        GUPnPDIDLLiteResource *resource;
        const char **import_uri = (const char **) user_data;

        if (*import_uri != NULL) {
                /* This means we've already found importURI. */
                /* No need to search further. */
                return;
        }

        resources = gupnp_didl_lite_object_get_resources (object);
        if (resources == NULL) {
                return;
        }

        resource = (GUPnPDIDLLiteResource *) resources->data;

        *import_uri = gupnp_didl_lite_resource_get_import_uri(resource);
        if (*import_uri != NULL) {
                *import_uri = g_strdup (*import_uri);
        }

        while (resources) {
                g_object_unref ((GUPnPDIDLLiteResource *) resources->data);
                resources = g_list_delete_link (resources, resources);
        }
}

static char *
parse_result (const char *result)
{
        GUPnPDIDLLiteParser *parser;
        GError *error;
        const char *import_uri;

        parser = gupnp_didl_lite_parser_new ();

        g_signal_connect (parser,
                          "item-available",
                          G_CALLBACK (didl_item_available_cb),
                          &import_uri);

        import_uri = NULL;
        error = NULL;
        if (!gupnp_didl_lite_parser_parse_didl (parser, result, &error)) {
                g_critical ("Failed to parse result DIDL from MediaServer: %s",
                            error->message);

                g_error_free (error);
        }

        g_object_unref (parser);

        return g_strdup (import_uri);
}

static void
create_object_cb (GObject *object, GAsyncResult *res, gpointer user_data)
{
        GError *error = NULL;
        char *result;
        const char *import_uri;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        error = NULL;

        action = gupnp_service_proxy_call_action_finish (proxy, res, &error);
        if (error != NULL) {
                g_critical ("CreateObject call failed: %s", error->message);
                g_clear_error (&error);
                item_created (NULL);

                return;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "Result",
                                                    G_TYPE_STRING,
                                                    &result,
                                                    NULL)) {
                g_critical ("Failed to create new item on remote container: %s",
                            error->message);

                g_error_free (error);

                item_created (NULL);

                return;
        }

        if (result == NULL) {
                g_critical ("Failed to create new item on remote container."
                            "No reasons given by MediaServer.");

                item_created (NULL);

                return;
        }

        import_uri = parse_result (result);

        item_created (import_uri);

        g_free (result);
}

void
create_item (const char        *file_path,
             const char        *title,
             GUPnPServiceProxy *cds_proxy,
             const char        *container_id)
{
        char *didl;
        GUPnPServiceProxyAction *action;

        didl = create_didl_for_file (file_path, title, container_id);
        if (didl == NULL) {
                item_created (NULL);

                return;
        }

        action = gupnp_service_proxy_action_new ("CreateObject",
                                                 "ContainerID",
                                                 G_TYPE_STRING,
                                                 container_id,
                                                 "Elements",
                                                 G_TYPE_STRING,
                                                 didl,
                                                 NULL);

        gupnp_service_proxy_call_action_async (cds_proxy,
                                               action,
                                               NULL,
                                               create_object_cb,
                                               NULL);

        gupnp_service_proxy_action_unref (action);
}
