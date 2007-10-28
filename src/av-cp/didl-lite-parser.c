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
#include "didl-lite-parser.h"
#include "didl-lite-object.h"
#include "xml-util.h"

G_DEFINE_TYPE (DIDLLiteParser,
               didl_lite_parser,
               G_TYPE_OBJECT);

enum {
        DIDL_OBJECT_AVAILABLE,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

static void
didl_lite_parser_init (DIDLLiteParser *didl)
{
}

static void
didl_lite_parser_dispose (GObject *object)
{
        GObjectClass   *gobject_class;
        DIDLLiteParser *parser;

        parser = DIDL_LITE_PARSER (object);

        gobject_class = G_OBJECT_CLASS (didl_lite_parser_parent_class);
        gobject_class->dispose (object);
}

static void
didl_lite_parser_class_init (DIDLLiteParserClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = didl_lite_parser_dispose;

        /**
         * DIDLLiteParser::didl-object-available
         * @didl: The #GUPnPDIDLLiteParser that received the signal
         * @object_node: The now available object node
         *
         * The ::didl-object-available signal is emitted whenever a new
         * DIDL-Lite object node is available.
         **/
        signals[DIDL_OBJECT_AVAILABLE] =
                g_signal_new ("didl-object-available",
                              TYPE_DIDL_LITE_PARSER,
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (DIDLLiteParserClass,
                                               didl_object_available),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__POINTER,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_POINTER);
}

DIDLLiteParser *
didl_lite_parser_new (void)
{
        return g_object_new (TYPE_DIDL_LITE_PARSER, NULL);
}

void
didl_lite_parser_parse_didl (DIDLLiteParser *parser,
                             xmlDoc         *didl)
{
        XmlDocWrapper *wrapper;
        xmlNode       *element;

        wrapper = xml_doc_wrapper_new (didl);

        /* Get a pointer to root element */
        element = xml_util_get_element ((xmlNode *) didl,
                                        "DIDL-Lite",
                                        NULL);
        for (element = element->children; element; element = element->next) {
                g_signal_emit (parser,
                               signals[DIDL_OBJECT_AVAILABLE],
                               0,
                               element);
        }

        g_object_ref_sink (wrapper);
        g_object_unref (wrapper);
}

