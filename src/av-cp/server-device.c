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

#include "server-device.h"
#include "icons.h"

G_DEFINE_TYPE (AVCPMediaServer, av_cp_media_server, GUPNP_TYPE_DEVICE_PROXY)

struct _AVCPMediaServerPrivate {
        GdkPixbuf *icon;
};

enum
{
        PROP_ICON = 1,
        N_PROPERTIES
};

static GParamSpec *av_cp_media_server_properties[N_PROPERTIES] = { NULL, };

/* GObject overrides */
static void
av_cp_media_server_constructed (GObject *obj);

static void
av_cp_media_server_dispose (GObject *object)
{
        GObjectClass *parent_class =
                              G_OBJECT_CLASS (av_cp_media_server_parent_class);

        g_clear_object (&self->priv->icon);

        parent_class->dispose (object);
}

static void
av_cp_media_server_get_property (GObject    *obj,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *spec)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (obj);

        switch (property_id) {
        case PROP_ICON:
                g_value_set_object (value, self->priv->icon);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, spec);
                break;
        }
}

static void
av_cp_media_server_on_icon_updated (GUPnPDeviceInfo *info,
                                    GdkPixbuf       *icon);

static void
av_cp_media_server_class_init (AVCPMediaServerClass *klass)
{
        GObjectClass *obj_class = G_OBJECT_CLASS (klass);

        obj_class->constructed = av_cp_media_server_constructed;
        obj_class->get_property = av_cp_media_server_get_property;
        obj_class->dispose = av_cp_media_server_dispose;

        g_type_class_add_private (klass, sizeof (AVCPMediaServerPrivate));

        av_cp_media_server_properties[PROP_ICON] =
                g_param_spec_object ("icon",
                                     "icon",
                                     "icon",
                                     GDK_TYPE_PIXBUF,
                                     G_PARAM_STATIC_STRINGS |
                                     G_PARAM_READABLE);

        g_object_class_install_properties (obj_class,
                                           N_PROPERTIES,
                                           av_cp_media_server_properties);
}

static void
av_cp_media_server_init (AVCPMediaServer *self)
{
        self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                                  AV_CP_TYPE_MEDIA_SERVER,
                                                  AVCPMediaServerPrivate);
}

static void
av_cp_media_server_constructed (GObject *obj)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (obj);

        G_OBJECT_CLASS (av_cp_media_server_parent_class)->constructed (obj);

        schedule_icon_update (GUPNP_DEVICE_INFO (self),
                              av_cp_media_server_on_icon_updated);
}

static void
av_cp_media_server_on_icon_updated (GUPnPDeviceInfo *info,
                                    GdkPixbuf       *icon)
{
        AVCPMediaServer *self = AV_CP_MEDIA_SERVER (info);

        self->priv->icon = icon;
        g_object_notify (G_OBJECT (info), "icon");
}
