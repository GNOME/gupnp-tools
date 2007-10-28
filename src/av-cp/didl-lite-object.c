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

#define CONTAINER_CLASS_NAME "object.container"
#define ITEM_CLASS_NAME      "object.item"

gboolean
didl_lite_object_is_container (xmlNode *object_node)
{
        char     *class_name;
        gboolean is_container;

        class_name = didl_lite_object_get_upnp_class (object_node);
        g_return_val_if_fail (class_name != NULL, FALSE);

        if (0 == strncmp (class_name,
                          CONTAINER_CLASS_NAME,
                          strlen (CONTAINER_CLASS_NAME))) {
                is_container = TRUE;
        } else {
                is_container = FALSE;
        }

        g_free (class_name);

        return is_container;
}

gboolean
didl_lite_object_is_item (xmlNode *object_node)
{
        char     *class_name;
        gboolean is_item;

        class_name = didl_lite_object_get_upnp_class (object_node);
        g_return_val_if_fail (class_name != NULL, FALSE);

        if (0 == strncmp (class_name,
                          ITEM_CLASS_NAME,
                          strlen (ITEM_CLASS_NAME))) {
                is_item = TRUE;
        } else {
                is_item = FALSE;
        }

        g_free (class_name);

        return is_item;
}

char *
didl_lite_object_get_upnp_class (xmlNode *object_node)
{
        return xml_util_get_child_element_content (object_node,
                                                   "class");
}

char *
didl_lite_object_get_id (xmlNode *object_node)
{
        return xml_util_get_attribute_contents (object_node, "id");
}

char *
didl_lite_object_get_parent_id (xmlNode *object_node)
{
        return xml_util_get_attribute_contents (object_node,
                                                "parentID");
}

char *
didl_lite_object_get_title (xmlNode *object_node)
{
        return xml_util_get_child_element_content (object_node,
                                                   "title");
}

