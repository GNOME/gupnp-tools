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

#include "xml-util.h"

G_BEGIN_DECLS

/* DIDL-Lite Generic Object related functions */
char *
didl_lite_object_get_upnp_class          (xmlNode        *object_node);

char *
didl_lite_object_get_id                  (xmlNode        *object_node);

char *
didl_lite_object_get_parent_id           (xmlNode        *object_node);

gboolean
didl_lite_object_get_never_playable      (xmlNode        *object_node);

char *
didl_lite_object_get_title               (xmlNode        *object_node);

char *
didl_lite_object_get_desc_type           (xmlNode        *object_node);

char *
didl_lite_object_get_desc_name_space     (xmlNode        *object_node);

char *
didl_lite_object_get_desc_contents       (xmlNode        *object_node);

gboolean
didl_lite_object_is_container            (xmlNode        *object_node);

gboolean
didl_lite_object_is_item                 (xmlNode        *object_node);

/* DIDL-Lite container Object functions */
gboolean
didl_lite_container_is_searchable        (xmlNode        *container_node);

/* DIDL-Lite item Object functions */
char *
didl_lite_item_get_ref_id                (xmlNode        *item_node);

char *
didl_lite_item_get_protocol_info         (xmlNode        *item_node);

char *
didl_lite_item_get_duration              (xmlNode        *item_node);

G_END_DECLS

#endif /* __DIDL_LITE_OBJECT_H__ */
