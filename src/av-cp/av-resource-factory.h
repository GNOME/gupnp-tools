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

#ifndef __AV_RESOURCE_FACTORY_H__
#define __AV_RESOURCE_FACTORY_H__

#include <libgupnp/gupnp-resource-factory.h>

G_BEGIN_DECLS

GType
av_resource_factory_get_type (void) G_GNUC_CONST;

#define TYPE_AV_RESOURCE_FACTORY \
                (av_resource_factory_get_type ())
#define AV_RESOURCE_FACTORY(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 TYPE_AV_RESOURCE_FACTORY, \
                 AVResourceFactory))
#define AV_RESOURCE_FACTORY_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 TYPE_AV_RESOURCE_FACTORY, \
                 AVResourceFactoryClass))
#define IS_AV_RESOURCE_FACTORY(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 TYPE_AV_RESOURCE_FACTORY))
#define IS_AV_RESOURCE_FACTORY_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 TYPE_AV_RESOURCE_FACTORY))
#define AV_RESOURCE_FACTORY_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 TYPE_AV_RESOURCE_FACTORY, \
                 AVResourceFactoryClass))

typedef struct {
        GUPnPResourceFactory parent;

        gpointer _gupnp_reserved;
} AVResourceFactory;

typedef struct {
        GUPnPResourceFactoryClass parent_class;

        /* future padding */
        void (* _gupnp_reserved1) (void);
        void (* _gupnp_reserved2) (void);
        void (* _gupnp_reserved3) (void);
        void (* _gupnp_reserved4) (void);
} AVResourceFactoryClass;

AVResourceFactory *
av_resource_factory_new (void);

G_END_DECLS

#endif /* __AV_RESOURCE_FACTORY_H__ */
