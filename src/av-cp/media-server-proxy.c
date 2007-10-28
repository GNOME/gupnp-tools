/*
 * Copyright (C) 2007 Zeeshan Ali <zeenix@gstreamer.net>
 *
 * Authors: Zeeshan Ali <zeenix@gstreamer.net>
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

#include <string.h>

#include <libgupnp/gupnp-service-proxy.h>

#include "media-server-proxy.h"
#include "didl-lite-parser.h"
#include "didl-lite-object.h"

#define CONNECTION_MANAGER_V1 "urn:schemas-upnp-org:service:ConnectionManager:1"
#define CONNECTION_MANAGER_V2 "urn:schemas-upnp-org:service:ConnectionManager:2"
#define CONTENT_DIR_V1        "urn:schemas-upnp-org:service:ContentDirectory:1"
#define CONTENT_DIR_V2        "urn:schemas-upnp-org:service:ContentDirectory:2"
#define AV_TRANSPORT_V1       "urn:schemas-upnp-org:service:AVTransport:1"
#define AV_TRANSPORT_V2       "urn:schemas-upnp-org:service:AVTransport:2"

G_DEFINE_TYPE (MediaServerProxy,
               media_server_proxy,
               GUPNP_TYPE_DEVICE_PROXY);

struct _MediaServerProxyPrivate {
        GUPnPServiceProxy *connection_manager;
        GUPnPServiceProxy *content_dir;
        GUPnPServiceProxy *av_transport;

        DIDLLiteParser *parser;
};

static void
on_didl_object_available (DIDLLiteParser *parser,
                          xmlNode        *object_node,
                          gpointer        user_data)
{
        MediaServerProxy *proxy;
        char             *id;

        proxy = MEDIA_SERVER_PROXY (user_data);

        if (!didl_lite_object_is_container (object_node))
                return;

        id = didl_lite_object_get_id (object_node);
        g_return_if_fail (id != NULL);

        /* Recurse into containers */
        media_server_proxy_start_browsing (proxy, id);

        g_free (id);
}

static void
media_server_proxy_init (MediaServerProxy *proxy)
{
        proxy->priv = G_TYPE_INSTANCE_GET_PRIVATE (proxy,
                                                   TYPE_MEDIA_SERVER_PROXY,
                                                   MediaServerProxyPrivate);

        proxy->priv->parser = didl_lite_parser_new ();
        g_signal_connect (proxy->priv->parser,
                          "didl-object-available",
                          G_CALLBACK (on_didl_object_available),
                          proxy);
}

static void
media_server_proxy_dispose (GObject *object)
{
        GObjectClass       *gobject_class;
        MediaServerProxy *proxy;

        proxy = MEDIA_SERVER_PROXY (object);

        if (proxy->priv->connection_manager) {
                g_object_unref (proxy->priv->connection_manager);
                proxy->priv->connection_manager = NULL;
        }

        if (proxy->priv->content_dir) {
                g_object_unref (proxy->priv->content_dir);
                proxy->priv->content_dir = NULL;
        }

        if (proxy->priv->av_transport) {
                g_object_unref (proxy->priv->av_transport);
                proxy->priv->av_transport = NULL;
        }

        if (proxy->priv->parser) {
                g_object_unref (proxy->priv->parser);
                proxy->priv->parser = NULL;
        }

        gobject_class = G_OBJECT_CLASS (media_server_proxy_parent_class);
        gobject_class->dispose (object);
}

static void
media_server_proxy_class_init (MediaServerProxyClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = media_server_proxy_dispose;

        g_type_class_add_private (klass, sizeof (MediaServerProxyPrivate));

}

static GUPnPServiceProxy *
get_content_dir (MediaServerProxy *proxy)
{
        GUPnPDeviceInfo  *info;
        GUPnPServiceInfo *content_dir;

        if (G_LIKELY (proxy->priv->content_dir))
                return proxy->priv->content_dir;

        info = GUPNP_DEVICE_INFO (proxy);

        /* Content Directory */
        content_dir = gupnp_device_info_get_service (info,
                                                     CONTENT_DIR_V1);
        if (content_dir == NULL) {
                        content_dir = gupnp_device_info_get_service
                                                      (info,
                                                       CONTENT_DIR_V2);
        }

        proxy->priv->content_dir = GUPNP_SERVICE_PROXY (content_dir);

        return proxy->priv->content_dir;
}

static inline void
on_browse_failure (GUPnPServiceInfo *info,
                   GError           *error)
{
        g_warning ("Failed to browse '%s': %s\n",
                   gupnp_service_info_get_location (info),
                   error->message);
        g_error_free (error);
}

void
browse_cb (GUPnPServiceProxy       *content_dir,
           GUPnPServiceProxyAction *action,
           gpointer                 user_data)
{
        MediaServerProxy *proxy;
        xmlDoc           *didl;
        char             *didl_xml;
        GError           *error;

        proxy = MEDIA_SERVER_PROXY (user_data);

        error = NULL;
        gupnp_service_proxy_end_action (content_dir,
                                        action,
                                        &error,
                                        /* OUT args */
                                        "Result",
                                        G_TYPE_STRING,
                                        &didl_xml,
                                        NULL);
        if (error) {
                on_browse_failure (GUPNP_SERVICE_INFO (content_dir), error);
                return;
        }

        didl = xmlParseMemory (didl_xml, strlen (didl_xml));
	if (didl == NULL) {
	        g_warning ("Parse error on XML DIDL-Light:\n'%s'", didl_xml);
	} else {
                didl_lite_parser_parse_didl (proxy->priv->parser, didl);
                xmlFreeDoc (didl);
        }

        g_free (didl_xml);
}

void
media_server_proxy_start_browsing (MediaServerProxy *proxy,
                                   const char       *object_id)
{
        GUPnPServiceProxy *content_dir;
        GError            *error;

        g_return_if_fail (proxy != NULL);
        g_return_if_fail (object_id != NULL);

        content_dir = get_content_dir (proxy);
        if (G_UNLIKELY (content_dir == NULL))
                return;

        error = NULL;
        gupnp_service_proxy_begin_action
                                (content_dir,
                                 "Browse",
                                 browse_cb,
                                 proxy,
                                 &error,
                                 /* IN args */
                                 "ObjectID",
                                 G_TYPE_STRING,
                                 object_id,
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
                                 G_TYPE_UINT, 0,
                                 "SortCriteria",
                                 G_TYPE_STRING,
                                 "",
                                 NULL);
        if (error)
                on_browse_failure (GUPNP_SERVICE_INFO (content_dir), error);
}


DIDLLiteParser *
media_server_proxy_get_parser (MediaServerProxy *proxy)
{
        return proxy->priv->parser;
}

