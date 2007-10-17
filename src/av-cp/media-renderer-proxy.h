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

#ifndef __MEDIA_RENDERER_PROXY_H__
#define __MEDIA_RENDERER_PROXY_H__

#include <libgupnp/gupnp-device-proxy.h>

G_BEGIN_DECLS

GType
media_renderer_proxy_get_type (void) G_GNUC_CONST;

#define TYPE_MEDIA_RENDERER_PROXY \
                (media_renderer_proxy_get_type ())
#define MEDIA_RENDERER_PROXY(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 TYPE_MEDIA_RENDERER_PROXY, \
                 MediaRendererProxy))
#define MEDIA_RENDERER_PROXY_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 TYPE_MEDIA_RENDERER_PROXY, \
                 MediaRendererProxyClass))
#define IS_MEDIA_RENDERER_PROXY(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 TYPE_MEDIA_RENDERER_PROXY))
#define IS_MEDIA_RENDERER_PROXY_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 TYPE_MEDIA_RENDERER_PROXY))
#define MEDIA_RENDERER_PROXY_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 TYPE_MEDIA_RENDERER_PROXY, \
                 MediaRendererProxyClass))

typedef struct _MediaRendererProxyPrivate MediaRendererProxyPrivate;

typedef struct {
        GUPnPDeviceProxy parent;

        MediaRendererProxyPrivate *priv;
} MediaRendererProxy;

typedef struct {
        GUPnPDeviceProxyClass parent_class;

        /* future padding */
        void (* _gupnp_reserved1) (void);
        void (* _gupnp_reserved2) (void);
        void (* _gupnp_reserved3) (void);
        void (* _gupnp_reserved4) (void);
} MediaRendererProxyClass;

G_END_DECLS

#endif /* __MEDIA_RENDERER_PROXY_H__ */
