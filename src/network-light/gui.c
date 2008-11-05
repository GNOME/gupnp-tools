/*
 * Copyright (C) 2007 Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
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
#include <stdlib.h>
#include <config.h>

#include "gui.h"
#include "icons.h"
#include "upnp.h"
#include "main.h"

#define GLADE_FILE DATA_DIR "/gupnp-network-light.glade"
#define ICON_FILE  "pixmaps/network-light-256x256.png"
#define OFF_FILE   "pixmaps/network-light-off.png"
#define ON_FILE    "pixmaps/network-light-on.png"

static GladeXML  *glade_xml;
static GtkWidget *main_window;
static GtkWidget *about_dialog;
static GdkPixbuf *on_pixbuf;
static GdkPixbuf *off_pixbuf;

void
update_image (void)
{
        GtkWidget *image;
        GdkPixbuf *pixbuf;
        gfloat     alpha;

        image = glade_xml_get_widget (glade_xml, "light-bulb-image");
        g_assert (image != NULL);

        if (get_status ()) {
                alpha = get_load_level () * 2.5;
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
on_light_status_menuitem_activate (GtkCheckMenuItem *menuitem,
                                   gpointer          user_data)
{
        set_all_status (gtk_check_menu_item_get_active (menuitem));
}

void
on_about_menuitem_activate (GtkMenuItem *menuitem,
                            gpointer     user_data)
{
        GtkWidget *about_dialog;

        about_dialog = glade_xml_get_widget (glade_xml, "about-dialog");
        g_assert (about_dialog != NULL);

        gtk_widget_show (about_dialog);
}

void
on_increase_luminance_menuitem_activate (GtkMenuItem *menuitem,
                                         gpointer     user_data)
{
        set_all_load_level (get_load_level () + 20);
}

void
on_decrease_luminance_menuitem_activate (GtkMenuItem *menuitem,
                                         gpointer     user_data)
{
        set_all_load_level (get_load_level () - 20);
}

static void
setup_popup ()
{
        GtkWidget *status_menuitem;
        GtkWidget *label;

        status_menuitem = glade_xml_get_widget (glade_xml,
                                                "light-status-menuitem");
        g_assert (status_menuitem != NULL);

        label = gtk_bin_get_child (GTK_BIN (status_menuitem));
        g_assert (label != NULL);

        gtk_label_set_markup (GTK_LABEL (label), "<b>On</b>");
}

static void
prepare_popup ()
{
        GtkWidget *status_menuitem;

        status_menuitem = glade_xml_get_widget (glade_xml,
                                                "light-status-menuitem");
        g_assert (status_menuitem != NULL);

        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (status_menuitem),
                                        get_status ());
}

static void
on_main_window_right_clicked (GdkEventButton *event)
{
        GtkWidget *popup;

        popup = glade_xml_get_widget (glade_xml, "popup-menu");
        g_assert (popup != NULL);

        prepare_popup ();

        gtk_menu_popup (GTK_MENU (popup),
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        event->button,
                        event->time);
}

gboolean
on_main_window_button_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             gpointer        user_data)
{
        if (event->type == GDK_BUTTON_RELEASE &&
            event->button == 3) {
                on_main_window_right_clicked (event);

                return TRUE;
        } else if (event->type == GDK_2BUTTON_PRESS &&
                   event->button == 1) {
                set_status (!get_status ());

                return TRUE;
        } else {
                return FALSE;
        }
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
        GdkPixbuf *icon_pixbuf;

        gtk_init (argc, argv);
        glade_init ();

        glade_xml = glade_xml_new (GLADE_FILE, NULL, NULL);
        if (glade_xml == NULL) {
                g_critical ("Unable to load the GUI file %s", GLADE_FILE);
                return FALSE;
        }

        main_window = glade_xml_get_widget (glade_xml, "main-window");
        g_assert (main_window != NULL);

        about_dialog = glade_xml_get_widget (glade_xml, "about-dialog");
        g_assert (about_dialog != NULL);

        on_pixbuf = load_pixbuf_file (ON_FILE);
        if (on_pixbuf == NULL)
                return FALSE;

        off_pixbuf = load_pixbuf_file (OFF_FILE);
        if (off_pixbuf == NULL) {
                g_object_unref (on_pixbuf);
                return FALSE;
        }

        icon_pixbuf = load_pixbuf_file (ICON_FILE);
        if (icon_pixbuf == NULL) {
                g_object_unref (on_pixbuf);
                g_object_unref (off_pixbuf);
                return FALSE;
        }

        gtk_window_set_icon (GTK_WINDOW (main_window), icon_pixbuf);
        gtk_window_set_icon (GTK_WINDOW (about_dialog), icon_pixbuf);
        gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (about_dialog),
                                   icon_pixbuf);
        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (about_dialog),
                                      VERSION);

        g_object_unref (icon_pixbuf);

        glade_xml_signal_autoconnect (glade_xml);

        setup_popup ();
        update_image ();

        gtk_widget_show_all (main_window);

        return TRUE;
}

void
deinit_ui (void)
{
        g_object_unref (glade_xml);
        gtk_widget_destroy (main_window);
        gtk_widget_destroy (about_dialog);
        g_object_unref (on_pixbuf);
        g_object_unref (off_pixbuf);
}

