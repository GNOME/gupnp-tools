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
#include "xml-util.h"
#include "didl-lite-object.h"

G_DEFINE_TYPE (DIDLLiteObject,
               didl_lite_object,
               G_TYPE_OBJECT);

struct _DIDLLiteObjectPrivate {
        xmlDoc  *doc;
        xmlNode *element;
};

enum {
        PROP_0,
        PROP_ELEMENT
};

static void
didl_lite_object_init (DIDLLiteObject *object)
{
        object->priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (object,
                                         TYPE_DIDL_LITE_OBJECT,
                                         DIDLLiteObjectPrivate);
}

static void
didl_lite_object_set_property (GObject        *gobject,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
        DIDLLiteObject *object;

        object = DIDL_LITE_OBJECT (gobject);

        switch (property_id) {
        case PROP_ELEMENT:
                {
                        xmlNode *element;

                        element = g_value_get_pointer (value);

                        object->priv->doc = xmlNewDoc (element->doc->version);
                        object->priv->element = xmlDocCopyNodeList
                                                        (object->priv->doc,
                                                         element);
                        break;
                }
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
                break;
        }
}

static void
didl_lite_object_dispose (GObject *gobject)
{
        GObjectClass       *gobject_class;
        DIDLLiteObject     *object;

        object = DIDL_LITE_OBJECT (gobject);

        if (object->priv->doc) {
                xmlFreeDoc (object->priv->doc);
                object->priv->doc = NULL;
        }

        gobject_class = G_OBJECT_CLASS (didl_lite_object_parent_class);
        gobject_class->dispose (gobject);
}

static void
didl_lite_object_class_init (DIDLLiteObjectClass *klass)
{
        GObjectClass *gobject_class;

        gobject_class = G_OBJECT_CLASS (klass);

        gobject_class->dispose      = didl_lite_object_dispose;
        gobject_class->set_property = didl_lite_object_set_property;

        g_type_class_add_private (klass, sizeof (DIDLLiteObjectPrivate));

        /**
         * DIDLLiteObject:element
         *
         * Private property.
         *
         * Stability: Private
         **/
        g_object_class_install_property
                (gobject_class,
                 PROP_ELEMENT,
                 g_param_spec_pointer ("element",
                                       "Element",
                                       "The DIDL-Lite element related to this "
                                       "object",
                                       G_PARAM_WRITABLE |
                                       G_PARAM_CONSTRUCT_ONLY |
                                       G_PARAM_STATIC_NAME |
                                       G_PARAM_STATIC_NICK |
                                       G_PARAM_STATIC_BLURB));
}

DIDLLiteObject *
didl_lite_object_new (xmlNode *element)
{
        return g_object_new (TYPE_DIDL_LITE_OBJECT,
                             "element", element,
                             NULL);
}

DIDLLiteObjectUPnPClass
didl_lite_object_get_upnp_class (DIDLLiteObject *object)
{
        xmlNode *element;
        DIDLLiteObjectUPnPClass upnp_class;

        element = object->priv->element;

        if (0 == strcmp ((char *) element->name, "container")) {
                upnp_class = DIDL_LITE_OBJECT_UPNP_CLASS_CONTAINER;
        } else if (0 == strcmp ((char *) element->name, "item")) {
                upnp_class = DIDL_LITE_OBJECT_UPNP_CLASS_ITEM;
        } else {
                upnp_class = DIDL_LITE_OBJECT_UPNP_CLASS_UNKNOWN;
        }

        return upnp_class;
}

char *
didl_lite_object_get_upnp_class_name (DIDLLiteObject *object)
{
        return xml_util_get_child_element_content (object->priv->element,
                                                   "class");
}

char *
didl_lite_object_get_id (DIDLLiteObject *object)
{
        return xml_util_get_attribute_contents (object->priv->element, "id");
}

char *
didl_lite_object_get_parent_id (DIDLLiteObject *object)
{
        return xml_util_get_attribute_contents (object->priv->element,
                                                "parentID");
}

char *
didl_lite_object_get_title (DIDLLiteObject *object)
{
        return xml_util_get_child_element_content (object->priv->element,
                                                   "title");
}


