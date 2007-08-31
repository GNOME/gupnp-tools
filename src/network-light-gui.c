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

#define OFF_FILE "pixmaps/network-light-off.png"
#define ON_FILE  "pixmaps/network-light-on.png"

static GtkWidget *main_window;
static GdkPixbuf *on_pixbuf;
static GdkPixbuf *off_pixbuf;
static GtkWidget *image;

static gboolean light_status;
static guint    light_load_level;

static void
update_image (void)
{
        GdkPixbuf *pixbuf;
        gfloat     alpha;

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
init_ui (gint   *argc,
         gchar **argv[])
{
        gtk_init (argc, argv);

        main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        g_object_connect (main_window,
                          "signal::delete-event",
                          gtk_main_quit,
                          main_window,
                          NULL);

        on_pixbuf = gdk_pixbuf_new_from_file (ON_FILE, NULL);
        off_pixbuf = gdk_pixbuf_new_from_file (OFF_FILE, NULL);
        if (off_pixbuf == NULL || on_pixbuf == NULL) {
                g_warning ("failed to get image %s\n",
                           (on_pixbuf == NULL)? ON_FILE: OFF_FILE);

                return FALSE;
        }

        image = gtk_image_new_from_pixbuf (off_pixbuf);
        gtk_container_add (GTK_CONTAINER (main_window), image);

        /* Put window into the center of the screen */
        gtk_window_set_position (GTK_WINDOW (main_window), GTK_WIN_POS_CENTER);

        gtk_widget_show_all (main_window);

        /* Light is off in the beginning */
        light_status = FALSE;
        light_load_level = 100;

        return TRUE;
}

