/*
 * Copyright (C) 2016 Jens Georg <mail@jensge.org>
 *
 * Authors: Jens Georg <mail@jensge.org>
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

#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgupnp/gupnp.h>

G_BEGIN_DECLS

GType
av_cp_media_server_get_type (void);

#define AV_CP_TYPE_MEDIA_SERVER (av_cp_media_server_get_type ())
#define AV_CP_MEDIA_SERVER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 AV_CP_TYPE_MEDIA_SERVER, \
                 AVCPMediaServer))
#define AV_CP_MEDIA_SERVER_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), \
            AV_CP_TYPE_MEDIA_SERVER, \
            AVCPMediaServerClass))
#define AV_CP_IS_MEDIA_SERVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            AV_CP_TYPE_MEDIA_SERVER))
#define AV_CP_IS_MEDIA_SERVER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
            AV_CP_TYPE_MEDIA_SERVER))
#define AV_CP_MEDIA_SERVER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
            AV_CP_TYPE_MEDIA_SERVER, \
            AVCPMediaServerDeviceClass))

typedef struct _AVCPMediaServer AVCPMediaServer;
typedef struct _AVCPMediaServerClass AVCPMediaServerClass;
typedef struct _AVCPMediaServerPrivate AVCPMediaServerPrivate;

struct _AVCPMediaServer {
        GUPnPDeviceProxy parent;

        AVCPMediaServerPrivate *priv;
};

struct _AVCPMediaServerClass {
        GUPnPDeviceProxyClass parent_class;
};

gboolean
av_cp_media_server_ready (AVCPMediaServer *self);

G_END_DECLS

#endif /* MEDIA_SERVER_H */
