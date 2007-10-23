/*
 * Copyright (C) 2006, 2007 OpenedHand Ltd.
 * Copyright (C) 2007 Zeeshan Ali.
 *
 * Author: Jorn Baayen <jorn@openedhand.com>
 * Author: Zeeshan Ali <zeenix@gstreamer.net>
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

#ifndef __XML_UTIL_H__
#define __XML_UTIL_H__

#include <glib.h>
#include <glib-object.h>
#include <libxml/tree.h>
#include <stdarg.h>

/* GObject wrapper for xmlDoc, so that we can use refcounting and
 * weak references. */
GType
xml_doc_wrapper_get_type (void) G_GNUC_CONST;

#define TYPE_XML_DOC_WRAPPER \
                (xml_doc_wrapper_get_type ())
#define XML_DOC_WRAPPER(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 TYPE_XML_DOC_WRAPPER, \
                 XmlDocWrapper))
#define IS_XML_DOC_WRAPPER(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 TYPE_XML_DOC_WRAPPER))

typedef struct {
        GInitiallyUnowned parent;

        xmlDoc *doc;
} XmlDocWrapper;

typedef struct {
        GInitiallyUnownedClass parent_class;
} XmlDocWrapperClass;

XmlDocWrapper *
xml_doc_wrapper_new (xmlDoc *doc);

/* Misc utilities for inspecting xmlNodes */
xmlNode *
xml_util_get_element                    (xmlNode    *node,
                                         ...) G_GNUC_NULL_TERMINATED;

char *
xml_util_get_child_element_content      (xmlNode    *node,
                                         const char *child_name);

char *
xml_util_get_attribute_contents         (xmlNode    *node,
                                         const char *attribute_name);

#endif /* __XML_UTIL_H__ */
