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
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <config.h>

#include "universal-cp-icons.h"

GdkPixbuf *icons[ICON_LAST];

void
init_icons (GladeXML *glade_xml)
{
        GtkWidget *image;
        int        i;
        char      *file_names[] = {
                "pixmaps/devices.png",         /* ICON_DEVICES    */
                "pixmaps/device.png",          /* ICON_DEVICE     */
                "pixmaps/service.png",         /* ICON_SERVICE    */
                "pixmaps/state-variables.png", /* ICON_VARIABLES  */
                "pixmaps/state-variable.png",  /* ICON_VARIABLE   */
                "pixmaps/action.png",          /* ICON_ACTION     */
                "pixmaps/state-variable.png",  /* ICON_ACTION_ARG */
        };

        for (i = 0; i < ICON_LAST; i++) {
                /* Try to fetch the pixmap from the CWD first */
                icons[i] = gdk_pixbuf_new_from_file (file_names[i], NULL);
                if (icons[i] == NULL) {
                        char *pixmap_path;

                        /* Then Try to fetch it from the system path */
                        pixmap_path = g_strjoin ("/",
                                                 UI_DIR,
                                                 file_names[i],
                                                 NULL);

                        icons[i] = gdk_pixbuf_new_from_file (pixmap_path, NULL);
                        g_free (pixmap_path);
                }

                g_assert (icons[i] != NULL);
        }

        image = glade_xml_get_widget (glade_xml, "device-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   icons[ICON_DEVICE]);

        image = glade_xml_get_widget (glade_xml, "service-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   icons[ICON_SERVICE]);

        image = glade_xml_get_widget (glade_xml, "action-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   icons[ICON_ACTION]);
}

void
deinit_icons (void)
{
        int i;

        for (i = 0; i < ICON_LAST; i++) {
                g_object_unref (icons[i]);
        }
}

