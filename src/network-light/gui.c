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

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include <gmodule.h>

#include "gui.h"
#include "icons.h"
#include "upnp.h"
#include "main.h"

#define UI_FILE   "/org/gupnp/Tools/Network-Light/gupnp-network-light.ui"
#define ICON_FILE "/org/gupnp/Tools/Network-Light/pixmaps/network-light.png"
#define OFF_FILE  "/org/gupnp/Tools/Network-Light/pixmaps/network-light-off.png"
#define ON_FILE   "/org/gupnp/Tools/Network-Light/pixmaps/network-light-on.png"

static GtkBuilder *builder;
static GtkWidget  *main_window;
static GtkWidget  *about_dialog;
static GdkPixbuf  *on_pixbuf;
static GdkPixbuf  *off_pixbuf;
static gboolean    change_all = TRUE; // ui changes apply to all devices

void
on_light_status_menuitem_activate (GtkCheckMenuItem *menuitem,
                                   gpointer          user_data);
void
on_about_menuitem_activate (GtkMenuItem *menuitem,
                            gpointer     user_data);

void
on_increase_luminance_menuitem_activate (GtkMenuItem *menuitem,
                                         gpointer     user_data);

void
on_decrease_luminance_menuitem_activate (GtkMenuItem *menuitem,
                                         gpointer     user_data);
gboolean
on_main_window_button_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             gpointer        user_data);

gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data);

void
update_image (void)
{
        GtkWidget *image;
        GdkPixbuf *pixbuf;
        gfloat     alpha;

        image = GTK_WIDGET (gtk_builder_get_object (builder,
                                                    "light-bulb-image"));
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

G_MODULE_EXPORT
void
on_light_status_menuitem_activate (GtkCheckMenuItem *menuitem,
                                   gpointer          user_data)
{
        if (change_all) {
                set_all_status (gtk_check_menu_item_get_active (menuitem));
        } else {
                set_status (gtk_check_menu_item_get_active (menuitem));
        }
}

G_MODULE_EXPORT
void
on_about_menuitem_activate (GtkMenuItem *menuitem,
                            gpointer     user_data)
{
        gtk_widget_show (about_dialog);
}

G_MODULE_EXPORT
void
on_increase_luminance_menuitem_activate (GtkMenuItem *menuitem,
                                         gpointer     user_data)
{
        if (change_all) {
                set_all_load_level (get_load_level () + 20);
        } else {
                set_load_level (get_load_level () + 20);
        }
}

G_MODULE_EXPORT
void
on_decrease_luminance_menuitem_activate (GtkMenuItem *menuitem,
                                         gpointer     user_data)
{
        if (change_all) {
                set_all_load_level (get_load_level () - 20);
        } else {
                set_load_level (get_load_level () - 20);
        }
}

static void
prepare_popup (void)
{
        GtkWidget *status_menuitem;

        status_menuitem = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "light-status-menuitem"));
        g_assert (status_menuitem != NULL);

        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (status_menuitem),
                                        get_status ());
}

static void
on_main_window_right_clicked (GdkEventButton *event)
{
        GtkWidget *popup;

        popup = GTK_WIDGET (gtk_builder_get_object (builder, "popup-menu"));
        g_assert (popup != NULL);

        prepare_popup ();

#if GTK_CHECK_VERSION(3,22,0)
        gtk_menu_popup_at_pointer (GTK_MENU (popup), (GdkEvent *)event);
#else
        gtk_menu_popup (GTK_MENU (popup),
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        event->button,
                        event->time);
#endif
}

G_MODULE_EXPORT
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

G_MODULE_EXPORT
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
         gchar **argv[],
         gchar *name,
         gboolean exclusive)
{
        GdkPixbuf *icon_pixbuf;
        GError *error = NULL;

        change_all = !exclusive;

        gtk_init (argc, argv);
        init_icons ();

        builder = gtk_builder_new ();
        g_assert (builder != NULL);
        gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

        if (!gtk_builder_add_from_resource (builder, UI_FILE, &error)) {
                g_critical ("Unable to load the GUI file %s: %s",
                            UI_FILE,
                            error->message);

                g_error_free (error);
                return FALSE;
        }

        main_window = GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "main-window"));
        g_assert (main_window != NULL);

        if (name && (strlen(name) > 0)) {
            gtk_window_set_title (GTK_WINDOW (main_window), name);
        }

        about_dialog = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "about-dialog"));
        g_assert (about_dialog != NULL);

        on_pixbuf = gdk_pixbuf_new_from_resource (ON_FILE, NULL);
        if (on_pixbuf == NULL)
                return FALSE;

        off_pixbuf = gdk_pixbuf_new_from_resource (OFF_FILE, NULL);
        if (off_pixbuf == NULL) {
                g_object_unref (on_pixbuf);
                return FALSE;
        }

        icon_pixbuf = gdk_pixbuf_new_from_resource (ICON_FILE, NULL);
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

        gtk_builder_connect_signals (builder, NULL);
        g_signal_connect (G_OBJECT (about_dialog), "delete-event",
                          G_CALLBACK (gtk_widget_hide_on_delete), NULL);

        update_image ();

        gtk_widget_show_all (main_window);

        return TRUE;
}

void
deinit_ui (void)
{
        g_object_unref (builder);
        gtk_widget_destroy (main_window);
        gtk_widget_destroy (about_dialog);
        g_object_unref (on_pixbuf);
        g_object_unref (off_pixbuf);
        deinit_icons ();
}

