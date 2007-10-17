/*
 * Copyright (C) 2007 Zeeshan Ali <zeenix@gstreamer.net>
 * Copyright (C) 2006, 2007 OpenedHand Ltd.
 *
 * Author: Zeeshan Ali <zeenix@gstreamer.net>
 *         Jorn Baayen <jorn@openedhand.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include "av-resource-factory.h"
#include "media-renderer-proxy.h"
#include "media-server-proxy.h"

#define MEDIA_RENDERER_V1 "urn:schemas-upnp-org:device:MediaRenderer:1"
#define MEDIA_RENDERER_V2 "urn:schemas-upnp-org:device:MediaRenderer:2"
#define MEDIA_SERVER_V1 "urn:schemas-upnp-org:device:MediaServer:1"
#define MEDIA_SERVER_V2 "urn:schemas-upnp-org:device:MediaServer:2"

G_DEFINE_TYPE (AVResourceFactory,
               av_resource_factory,
               GUPNP_TYPE_RESOURCE_FACTORY);

static void
av_resource_factory_init (AVResourceFactory *factory)
{
}

static GUPnPDeviceProxy *
create_device_proxy (GUPnPResourceFactory *factory,
                     GUPnPContext         *context,
                     XmlDocWrapper        *doc,
                     xmlNode              *element,
                     const char           *udn,
                     const char           *location,
                     const SoupUri        *url_base)
{
        GUPnPDeviceProxy *proxy;
        const char       *device_type;
        GType             proxy_type = GUPNP_TYPE_DEVICE_PROXY;


        g_return_val_if_fail (GUPNP_IS_CONTEXT (context), NULL);
        g_return_val_if_fail (IS_XML_DOC_WRAPPER (doc), NULL);
        g_return_val_if_fail (element != NULL, NULL);
        g_return_val_if_fail (location != NULL, NULL);
        g_return_val_if_fail (url_base != NULL, NULL);

        device_type = xml_util_get_child_element_content_glib (element,
                                                               "deviceType");
        if (device_type) {
                if (strcmp (device_type, MEDIA_RENDERER_V1) == 0 ||
                    strcmp (device_type, MEDIA_RENDERER_V2) == 0) {
                        proxy_type = TYPE_MEDIA_RENDERER_PROXY;
                } else if (strcmp (device_type, MEDIA_SERVER_V1) == 0 ||
                           strcmp (device_type, MEDIA_SERVER_V2) == 0) {
                        proxy_type = TYPE_MEDIA_SERVER_PROXY;
                }
        }

        proxy = g_object_new (proxy_type,
                              "resource-factory", factory,
                              "context", context,
                              "location", location,
                              "udn", udn,
                              "url-base", url_base,
                              "document", doc,
                              "element", element,
                              NULL);

        return proxy;
}

/*static GUPnPServiceProxy *
create_service_proxy (AVResourceFactory *factory,
                      GUPnPContext         *context,
                      XmlDocWrapper        *doc,
                      xmlNode              *element,
                      const char           *udn,
                      const char           *service_type,
                      const char           *location,
                      const SoupUri        *url_base)
{
        GUPnPServiceProxy *proxy;

        g_return_val_if_fail (GUPNP_IS_CONTEXT (context), NULL);
        g_return_val_if_fail (IS_XML_DOC_WRAPPER (doc), NULL);
        g_return_val_if_fail (element != NULL, NULL);
        g_return_val_if_fail (location != NULL, NULL);
        g_return_val_if_fail (url_base != NULL, NULL);

        proxy = g_object_new (GUPNP_TYPE_SERVICE_PROXY,
                              "context", context,
                              "location", location,
                              "udn", udn,
                              "service-type", service_type,
                              "url-base", url_base,
                              "document", doc,
                              "element", element,
                              NULL);

        return proxy;
}*/

static void
av_resource_factory_class_init (AVResourceFactoryClass *klass)
{
        GUPnPResourceFactoryClass *parent_class;

        parent_class = GUPNP_RESOURCE_FACTORY_CLASS (klass);
        parent_class->create_device_proxy  = create_device_proxy;
        /*parent_class->create_service_proxy = create_service_proxy;*/
}

AVResourceFactory *
av_resource_factory_new (void)
{
        return g_object_new (TYPE_AV_RESOURCE_FACTORY, NULL);
}

