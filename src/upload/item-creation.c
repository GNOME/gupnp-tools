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
fill_res_for_file (const char            *file_path,
                   GUPnPDIDLLiteResource *res)
{
        char *content_type;

        memset (res, 0, sizeof (GUPnPDIDLLiteResource));
        content_type = g_content_type_guess (file_path, NULL, 0, NULL);
        res->mime_type = g_content_type_get_mime_type (content_type);

        res->uri = "";
        res->protocol = "*";
        res->network = "*";
        res->dlna_profile = "MP3"; /* FIXME */

        g_free (content_type);
}

static void
clear_res (GUPnPDIDLLiteResource *res)
{
        g_free (res->mime_type);
}

static char *
guess_upnp_class_for_mime_type (const char *mime_type)
{
        char *upnp_class = NULL;

        if (g_pattern_match_simple ("audio/*", mime_type)) {
                upnp_class = "object.item.audioItem.musicTrack";
        } else if (g_pattern_match_simple ("video/*", mime_type)) {
                upnp_class = "object.item.videoItem";
        } else if (g_pattern_match_simple ("image/*", mime_type)) {
                upnp_class = "object.item.imageItem";
        }

        return upnp_class;
}

static char *
create_didl_for_file (const char *file_path,
                      const char *title,
                      const char *parent_id)
{
        GUPnPDIDLLiteWriter *writer;
        GUPnPDIDLLiteResource res;
        char *didl;
        char *upnp_class;
        char *new_title;

        fill_res_for_file (file_path, &res);

        upnp_class = guess_upnp_class_for_mime_type (res.mime_type);
        if (upnp_class == NULL) {
                g_critical ("Failed to guess UPnP class for file '%s'",
                            file_path);

                return NULL;
        }

        if (title == NULL) {
                new_title = g_path_get_basename (file_path);
        } else {
                new_title = g_strdup (title);
        }

        writer = gupnp_didl_lite_writer_new ();

        gupnp_didl_lite_writer_start_didl_lite (writer,
                                                NULL,
                                                NULL,
                                                FALSE);
        gupnp_didl_lite_writer_start_item (writer,
                                           "",
                                           parent_id,
                                           NULL,
                                           FALSE);

        gupnp_didl_lite_writer_add_string (writer,
                                           "title",
                                           GUPNP_DIDL_LITE_WRITER_NAMESPACE_DC,
                                           NULL,
                                           new_title);
        gupnp_didl_lite_writer_add_string (
                                        writer,
                                        "class",
                                        GUPNP_DIDL_LITE_WRITER_NAMESPACE_UPNP,
                                        NULL,
                                        upnp_class);
        gupnp_didl_lite_writer_add_res (writer, &res);

        gupnp_didl_lite_writer_end_item (writer);
        gupnp_didl_lite_writer_end_didl_lite (writer);

        didl = g_strdup (gupnp_didl_lite_writer_get_string (writer));

        g_free (new_title);
        clear_res (&res);

        return didl;
}

static void
didl_item_available_cb (GUPnPDIDLLiteParser *parser,
                        xmlNode             *object_node,
                        gpointer             user_data)
{
        GList *resources;
        xmlNode *res_node;
        char **import_uri = (char **) user_data;

        if (*import_uri != NULL) {
                /* This means we've already found importURI. */
                /* No need to search further. */
                return;
        }

        resources = gupnp_didl_lite_object_get_property (object_node, "res");
        if (resources == NULL) {
                return;
        }

        res_node = (xmlNode *) resources->data;

        *import_uri = gupnp_didl_lite_property_get_attribute (res_node,
                                                              "importUri");
        if (*import_uri != NULL) {
                *import_uri = g_strdup (*import_uri);
        }
}

static char *
parse_result (const char *result)
{
        GUPnPDIDLLiteParser *parser;
        GError *error;
        char *import_uri;

        parser = gupnp_didl_lite_parser_new ();

        import_uri = NULL;

        error = NULL;
        if (!gupnp_didl_lite_parser_parse_didl (parser,
                                                result,
                                                didl_item_available_cb,
                                                &import_uri,
                                                &error)) {
                g_critical ("Failed to parse result DIDL from MediaServer: %s",
                            error->message);

                g_error_free (error);
        }

        g_object_unref (parser);

        return import_uri;
}

static void
create_object_cb (GUPnPServiceProxy       *cds_proxy,
                  GUPnPServiceProxyAction *action,
                  gpointer                 user_data)
{
        GError *error;
        char *result;
        char *import_uri;

        error = NULL;
        if (!gupnp_service_proxy_end_action (cds_proxy,
                                             action,
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

        didl = create_didl_for_file (file_path, title, container_id);
        if (didl == NULL) {
                item_created (NULL);

                return;
        }

        gupnp_service_proxy_begin_action (cds_proxy,
                                          "CreateObject",
                                          create_object_cb,
                                          NULL,
                                          "ContainerID",
                                                G_TYPE_STRING,
                                                container_id,
                                          "Elements",
                                                G_TYPE_STRING,
                                                didl,
                                          NULL);
}
