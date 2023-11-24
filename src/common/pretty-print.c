/*
 * Copyright (C) 2011 Jens Georg <mail@jensge.org>
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

#include <config.h>

#include "pretty-print.h"

#include <libxml/globals.h>
#include <libxml/xmlreader.h>

#include <string.h>

char *
pretty_print_xml (const char *xml)
{
        xmlDocPtr doc;
        int old_value;
        char *text;
        int length;

        doc = xmlReadMemory (xml,
                             strlen (xml),
                             NULL,
                             NULL,
                             XML_PARSE_NONET | XML_PARSE_RECOVER);

        if (!doc)
                return NULL;

        old_value = xmlIndentTreeOutput;
        xmlIndentTreeOutput = 1;
        xmlDocDumpFormatMemoryEnc (doc,
                                   (xmlChar **) &text,
                                   &length,
                                   "UTF-8",
                                   1);
        xmlIndentTreeOutput = old_value;
        xmlFreeDoc (doc);

        return text;
}
