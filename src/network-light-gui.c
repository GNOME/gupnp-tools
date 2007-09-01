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

#include "network-light-gui.h"

#define GLADE_FILE "gupnp-network-light.glade"
#define OFF_FILE   "pixmaps/network-light-off.png"
#define ON_FILE    "pixmaps/network-light-on.png"

static GladeXML  *glade_xml;
static GdkPixbuf *on_pixbuf;
static GdkPixbuf *off_pixbuf;

static gboolean light_status;
static guint    light_load_level;

static void
update_image (void)
{
        GtkWidget *image;
        GdkPixbuf *pixbuf;
        gfloat     alpha;

        image = glade_xml_get_widget (glade_xml, "light-bulb-image");
        g_assert (image != NULL);

        if (light_status) {
                alpha = light_load_level * 2.5;
        } else {
                alpha = 0.0;
        }

        pixbuf = gdk_pixbuf_copy (off_pixbuf);
        gdk_pixbuf_composite (on_pixbuf,
                              pixbuf,
                              0, 0,
                              gdk_pixbuf_get_width (pixbuf),
                              gdk_pixbuf_get_height (pixbuf),
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_HYPER,
                              (int) alpha);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
        g_object_unref (pixbuf);
}

void
set_status (gboolean status)
{
        if (status != light_status) {
                light_status = status;
                update_image ();
        }
}

gboolean
get_status (void)
{
        return light_status;
}

void
set_load_level (guint load_level)
{
        if (load_level != light_load_level) {
                light_load_level = load_level, 0, 100;
                update_image ();
        }
}

guint
get_load_level (void)
{
        return light_load_level;
}

gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
        gtk_main_quit ();
        return TRUE;
}

gboolean
init_ui (gint   *argc,
         gchar **argv[])
{
        GtkWidget *main_window;
        gchar     *glade_path = NULL;

        gtk_init (argc, argv);
        glade_init ();

        /* Try to fetch the glade file from the CWD first */
        glade_path = GLADE_FILE;
        if (!g_file_test (glade_path, G_FILE_TEST_EXISTS)) {
                /* Then Try to fetch it from the system path */
                glade_path = UI_DIR "/" GLADE_FILE;

                if (!g_file_test (glade_path, G_FILE_TEST_EXISTS))
                        glade_path = NULL;
        }

        if (glade_path == NULL) {
                g_critical ("Unable to load the GUI file %s", GLADE_FILE);
                return FALSE;
        }

        glade_xml = glade_xml_new (glade_path, NULL, NULL);
        if (glade_xml == NULL)
                return FALSE;

        main_window = glade_xml_get_widget (glade_xml, "main-window");
        g_assert (main_window != NULL);

        on_pixbuf = gdk_pixbuf_new_from_file (ON_FILE, NULL);
        off_pixbuf = gdk_pixbuf_new_from_file (OFF_FILE, NULL);
        if (off_pixbuf == NULL || on_pixbuf == NULL) {
                g_warning ("failed to get image %s\n",
                           (on_pixbuf == NULL)? ON_FILE: OFF_FILE);

                return FALSE;
        }

        glade_xml_signal_autoconnect (glade_xml);

        /* Light is off in the beginning */
        light_status = FALSE;
        light_load_level = 100;
        update_image ();

        gtk_widget_show_all (main_window);

        return TRUE;
}

void
deinit_ui (void)
{
        g_object_unref (glade_xml);
}

