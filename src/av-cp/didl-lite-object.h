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

#ifndef __DIDL_LITE_OBJECT_H__
#define __DIDL_LITE_OBJECT_H__

#include <glib-object.h>

#include "xml-util.h"

G_BEGIN_DECLS

typedef enum
{
  DIDL_LITE_OBJECT_UPNP_CLASS_UNKNOWN,
  DIDL_LITE_OBJECT_UPNP_CLASS_CONTAINER,
  DIDL_LITE_OBJECT_UPNP_CLASS_ITEM,
} DIDLLiteObjectUPnPClass;

GType
didl_lite_object_get_type (void) G_GNUC_CONST;

#define TYPE_DIDL_LITE_OBJECT \
                (didl_lite_object_get_type ())
#define DIDL_LITE_OBJECT(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 TYPE_DIDL_LITE_OBJECT, \
                 DIDLLiteObject))
#define DIDL_LITE_OBJECT_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 TYPE_DIDL_LITE_OBJECT, \
                 DIDLLiteObjectClass))
#define IS_DIDL_LITE_OBJECT(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 TYPE_DIDL_LITE_OBJECT))
#define IS_DIDL_LITE_OBJECT_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 TYPE_DIDL_LITE_OBJECT))
#define DIDL_LITE_OBJECT_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 TYPE_DIDL_LITE_OBJECT, \
                 DIDLLiteObjectClass))

typedef struct _DIDLLiteObjectPrivate DIDLLiteObjectPrivate;

typedef struct {
        GObject parent;

        DIDLLiteObjectPrivate *priv;
} DIDLLiteObject;

typedef struct {
        GObjectClass parent_class;

        /* future padding */
        void (* _gupnp_reserved1) (void);
        void (* _gupnp_reserved2) (void);
        void (* _gupnp_reserved3) (void);
        void (* _gupnp_reserved4) (void);
} DIDLLiteObjectClass;

DIDLLiteObject *
didl_lite_object_new                     (XmlDocWrapper         *wrapper,
                                          xmlNode               *element);

/* DIDL-Lite Generic Object related functions */
DIDLLiteObjectUPnPClass
didl_lite_object_get_upnp_class          (DIDLLiteObject        *object);

char *
didl_lite_object_get_upnp_class_name     (DIDLLiteObject        *object);

char *
didl_lite_object_get_id                  (DIDLLiteObject        *object);

char *
didl_lite_object_get_parent_id           (DIDLLiteObject        *object);

gboolean
didl_lite_object_get_never_playable      (DIDLLiteObject        *object);

char *
didl_lite_object_get_title               (DIDLLiteObject        *object);

char *
didl_lite_object_get_desc_type           (DIDLLiteObject        *object);

char *
didl_lite_object_get_desc_name_space     (DIDLLiteObject        *object);

char *
didl_lite_object_get_desc_contents       (DIDLLiteObject        *object);

/* DIDL-Lite container Object functions */
gboolean
didl_lite_object_is_container_searchable (DIDLLiteObject        *object);

/* DIDL-Lite item Object functions */
char *
didl_lite_object_get_item_ref_id         (DIDLLiteObject        *object);

char *
didl_lite_object_get_item_protocol_info  (DIDLLiteObject        *object);

char *
didl_lite_object_get_item_duration       (DIDLLiteObject        *object);

G_END_DECLS

#endif /* __DIDL_LITE_OBJECT_H__ */
