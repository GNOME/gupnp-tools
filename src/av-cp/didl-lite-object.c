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
        return didl_lite_object_get_value (object_node, "class");
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
        return didl_lite_object_get_value (object_node, "title");
}

char *
didl_lite_object_get_value (xmlNode    *object_node,
                            const char *value_id)
{
        return xml_util_get_child_element_content (object_node, value_id);
}

gboolean
didl_lite_object_get_restricted (xmlNode *object_node)
{
        return xml_util_get_boolean_attribute (object_node, "restricted");
}

gboolean
didl_lite_object_get_never_playable (xmlNode *object_node)
{
        return xml_util_get_boolean_attribute (object_node, "neverPlayable");
}

GList *
didl_lite_object_get_descriptors (xmlNode *object_node)
{
        return xml_util_get_child_elements_by_name (object_node, "desc");
}

GList *
didl_lite_object_get_resources (xmlNode *object_node)
{
        return xml_util_get_child_elements_by_name (object_node, "res");
}

gboolean
didl_lite_container_is_searchable (xmlNode *container_node)
{
        return xml_util_get_boolean_attribute (container_node, "searchable");
}

guint
didl_lite_container_get_child_count (xmlNode *container_node)
{
        return xml_util_get_uint_attribute (container_node, "childCount");
}

char *
didl_lite_item_get_ref_id (xmlNode *item_node)
{
        return xml_util_get_attribute_contents (item_node, "refID");
}

char *
didl_lite_descriptor_get_type (xmlNode *desc_node)
{
        return xml_util_get_attribute_contents (desc_node, "type");
}

char *
didl_lite_descriptor_get_name_space (xmlNode *desc_node)
{
        return xml_util_get_attribute_contents (desc_node, "nameSpace");
}

char *
didl_lite_resource_get_contents (xmlNode *res_node)
{
        return xml_util_get_element_content (res_node);
}

char *
didl_lite_resource_get_import_uri (xmlNode *res_node)
{
        return xml_util_get_attribute_contents (res_node, "importUri");
}

char *
didl_lite_resource_get_protocol_info (xmlNode *res_node)
{
        return xml_util_get_attribute_contents (res_node, "protocolInfo");
}

guint
didl_lite_resource_get_size (xmlNode *res_node)
{
        return xml_util_get_uint_attribute (res_node, "size");
}

char *
didl_lite_resource_get_duration (xmlNode *res_node)
{
        return xml_util_get_attribute_contents (res_node, "duration");
}

guint
didl_lite_resource_get_bitrate (xmlNode *res_node)
{
        return xml_util_get_uint_attribute (res_node, "bitrate");
}

guint
didl_lite_resource_get_sample_frequency (xmlNode *res_node)
{
        return xml_util_get_uint_attribute (res_node, "frequency");
}

guint
didl_lite_resource_get_bits_per_sample (xmlNode *res_node)
{
        return xml_util_get_uint_attribute (res_node, "bitsPerSample");
}

guint
didl_lite_resource_get_nr_audio_channels (xmlNode *res_node)
{
        return xml_util_get_uint_attribute (res_node, "nrAudioChannels");
}

char *
didl_lite_resource_get_resolution (xmlNode *res_node)
{
        return xml_util_get_attribute_contents (res_node, "resolution");
}

guint
didl_lite_resource_get_color_depth (xmlNode *res_node)
{
        return xml_util_get_uint_attribute (res_node, "colorDepth");
}

char *
didl_lite_resource_get_protection (xmlNode *res_node)
{
        return xml_util_get_attribute_contents (res_node, "protection");
}

