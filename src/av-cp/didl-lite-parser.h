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

#ifndef __DIDL_LITE_PARSER_H__
#define __DIDL_LITE_PARSER_H__

#include <libxml/tree.h>
#include <glib-object.h>
#include "didl-lite-object.h"

G_BEGIN_DECLS

GType
didl_lite_parser_get_type (void) G_GNUC_CONST;

#define TYPE_DIDL_LITE_PARSER \
                (didl_lite_parser_get_type ())
#define DIDL_LITE_PARSER(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 TYPE_DIDL_LITE_PARSER, \
                 DIDLLiteParser))
#define DIDL_LITE_PARSER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 TYPE_DIDL_LITE_PARSER, \
                 DIDLLiteParserClass))
#define IS_DIDL_LITE_PARSER(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 TYPE_DIDL_LITE_PARSER))
#define IS_DIDL_LITE_PARSER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 TYPE_DIDL_LITE_PARSER))
#define DIDL_LITE_PARSER_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 TYPE_DIDL_LITE_PARSER, \
                 DIDLLiteParserClass))

typedef struct {
        GObject parent;

        gpointer gupnp_reserved;
} DIDLLiteParser;

typedef struct {
        GObjectClass parent_class;

        /* signals */
        void (* didl_object_available)      (DIDLLiteParser *parser,
                                             DIDLLiteObject *object);

        /* future padding */
        void (* _gupnp_reserved1) (void);
        void (* _gupnp_reserved2) (void);
} DIDLLiteParserClass;

DIDLLiteParser *
didl_lite_parser_new                   (void);

void
didl_lite_parser_parse_didl            (DIDLLiteParser        *parser,
                                        xmlDoc                *didl);

G_END_DECLS

#endif /* __DIDL_LITE_PARSER_H__ */
