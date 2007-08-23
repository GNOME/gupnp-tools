/*
 * Copyright (C) 2007 Zeeshan Ali <zeenix@gstreamer.net>
 *
 * Authors: Zeeshan Ali <zeenix@gstreamer.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <config.h>

#include "universal-cp-gui.h"
#include "universal-cp-devicetreeview.h"
#include "universal-cp-actiondialog.h"
#include "universal-cp.h"

#define GLADE_FILE "gupnp-universal-cp.glade"

GladeXML  *glade_xml;
GdkPixbuf *icons[ICON_LAST];

static void
init_icons (void)
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
display_event (const char *notified_at,
               const char *friendly_name,
               const char *service_id,
               const char *variable_name,
               const char *value)
{
        GtkWidget        *treeview;
        GtkTreeModel     *model;

        treeview = glade_xml_get_widget (glade_xml, "event-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

        gtk_list_store_insert_with_values (GTK_LIST_STORE (model),
                                           NULL, -1,
                                           0, notified_at,
                                           1, friendly_name,
                                           2, service_id,
                                           3, variable_name,
                                           4, value,
                                           -1);
}

void
update_details (const char **tuples)
{
        GtkWidget    *treeview;
        GtkTreeModel *model;
        const char  **tuple;

        treeview = glade_xml_get_widget (glade_xml, "details-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        gtk_list_store_clear (GTK_LIST_STORE (model));

        for (tuple = tuples; *tuple; tuple = tuple + 2) {
                if (*(tuples + 1) == NULL)
                        continue;

                gtk_list_store_insert_with_values (
                                GTK_LIST_STORE (model),
                                NULL, -1,
                                0, *tuple,
                                1, *(tuple + 1), -1);
        }
}

static void
clear_event_treeview (void)
{
        GtkWidget    *treeview;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      more;

        treeview = glade_xml_get_widget (glade_xml, "event-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        more = gtk_tree_model_get_iter_first (model, &iter);

        while (more)
                more = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

void
on_clear_event_log_activate (GtkMenuItem *menuitem,
                             gpointer     user_data)
{
        clear_event_treeview ();
}

static GtkTreeModel *
create_details_treemodel (void)
{
        GtkListStore *store;

        store = gtk_list_store_new (2,
                                    G_TYPE_STRING,  /* Name */
                                    G_TYPE_STRING); /* Value */

        return GTK_TREE_MODEL (store);
}

static GtkTreeModel *
create_event_treemodel (void)
{
        GtkListStore *store;

        store = gtk_list_store_new (5,
                                    G_TYPE_STRING,  /* Time                  */
                                    G_TYPE_STRING,  /* Source Device         */
                                    G_TYPE_STRING,  /* Source Service        */
                                    G_TYPE_STRING,  /* State Variable (name) */
                                    G_TYPE_STRING); /* Value                 */

        return GTK_TREE_MODEL (store);
}

void
setup_treeview (GtkWidget    *treeview,
                GtkTreeModel *model,
                char         *headers[],
                int           render_index)
{
        int i;

        for (i = 0; headers[i] != NULL; i++) {
                GtkCellRenderer   *renderer;
                GtkTreeViewColumn *column;

                renderer = gtk_cell_renderer_text_new ();
                column = gtk_tree_view_column_new_with_attributes (
                                headers[i],
                                renderer,
                                "text", i + render_index,
                                NULL);
                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);

                gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview),
                                             column,
                                             -1);
        }

        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 model);
}

static void
setup_treeviews (void)
{
        GtkWidget        *treeviews[3];
        GtkTreeModel     *treemodels[3];
        int               i;
        char             *headers[3][6] = { {"Name",
                                         "Value",
                                         NULL },
                                        {"Time",
                                         "Device",
                                         "Service",
                                         "State Variable",
                                         "Value",
                                         NULL } ,
                                        {"Device",
                                         NULL} };

        treeviews[0] = glade_xml_get_widget (glade_xml,
                        "details-treeview");
        treeviews[1] = glade_xml_get_widget (glade_xml,
                        "event-treeview");
        treeviews[2] = glade_xml_get_widget (glade_xml,
                        "device-treeview");
        treemodels[0] = create_details_treemodel ();
        treemodels[1] = create_event_treemodel ();
        treemodels[2] = create_device_treemodel ();
        g_assert (treeviews[0] != NULL &&
                  treeviews[1] != NULL &&
                  treeviews[2] != NULL);

        for (i = 0; i < 2; i++)
                setup_treeview (treeviews[i], treemodels[i], headers[i], 0);

        setup_device_treeview (treeviews[2], treemodels[2], headers[2], 1);
}

void
on_event_log_activate (GtkCheckMenuItem *menuitem,
                       gpointer          user_data)
{
        GtkWidget *scrolled_window;

        scrolled_window = glade_xml_get_widget (glade_xml,
                                                "event-scrolledwindow");
        g_assert (scrolled_window != NULL);

        g_object_set (G_OBJECT (scrolled_window),
                      "visible",
                      gtk_check_menu_item_get_active (menuitem),
                      NULL);
}

void
on_subscribe_to_events_activate (GtkCheckMenuItem *menuitem,
                                 gpointer          user_data)
{
        GUPnPServiceProxy *proxy;
        gboolean           subscribed;

        proxy = get_selected_service ();
        if (proxy != NULL) {
                subscribed = gtk_check_menu_item_get_active (menuitem);
                gupnp_service_proxy_set_subscribed (proxy, subscribed);

                g_object_unref (G_OBJECT (proxy));
        }
}

static void
setup_device_popup (GtkWidget *popup)
{
        GUPnPServiceProxy *proxy;
        GtkWidget         *subscribe_menuitem;
        GtkWidget         *action_menuitem;
        GtkWidget         *separator;

        subscribe_menuitem = glade_xml_get_widget (glade_xml,
                                         "subscribe-to-events");
        g_assert (subscribe_menuitem != NULL);
        action_menuitem = glade_xml_get_widget (glade_xml,
                                         "invoke-action");
        g_assert (action_menuitem != NULL);
        separator = glade_xml_get_widget (glade_xml,
                                          "device-popup-separator");
        g_assert (separator != NULL);

        /* See if a service is selected */
        proxy = get_selected_service ();
        if (proxy != NULL) {
                g_object_set (G_OBJECT (subscribe_menuitem),
                              "visible",
                              TRUE,
                              "active",
                              gupnp_service_proxy_get_subscribed (proxy),
                              NULL);

                g_object_set (G_OBJECT (action_menuitem),
                              "visible",
                              FALSE,
                              NULL);
        } else {
                GUPnPServiceActionInfo *action;

                g_object_set (G_OBJECT (subscribe_menuitem),
                              "visible",
                              FALSE,
                              NULL);

                /* See if an action is selected */
                action = get_selected_action (NULL, NULL);
                g_object_set (G_OBJECT (action_menuitem),
                              "visible",
                              action != NULL,
                              NULL);
        }

        /* Separator should be visible only if either service or action row is
         * selected
         */
        g_object_set (G_OBJECT (separator),
                      "visible",
                      (proxy != NULL),
                      NULL);
        if (proxy)
                g_object_unref (G_OBJECT (proxy));
}

gboolean
on_device_treeview_button_release (GtkWidget      *widget,
                                   GdkEventButton *event,
                                   gpointer        user_data)
{
        GtkWidget *popup;

        if (event->type != GDK_BUTTON_RELEASE ||
            event->button != 3)
                return FALSE;

        popup = glade_xml_get_widget (glade_xml, "device-popup");
        g_assert (popup != NULL);

        setup_device_popup (popup);

        gtk_menu_popup (GTK_MENU (popup),
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        event->button,
                        event->time);
        return TRUE;
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
        GtkWidget *hpaned;
        GtkWidget *vpaned;
        gint       window_width, window_height;
        gchar     *glade_path = NULL;
        gint       position;

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
        hpaned = glade_xml_get_widget (glade_xml, "main-window-hpaned");
        vpaned = glade_xml_get_widget (glade_xml, "main-window-vpaned");
        g_assert (main_window != NULL);
        g_assert (hpaned != NULL);
        g_assert (vpaned != NULL);

        /* 80% of the screen but don't get bigger than 1000x800 */
        window_width = CLAMP ((gdk_screen_width () * 80 / 100), 10, 1000);
        window_height = CLAMP ((gdk_screen_height () * 80 / 100), 10, 800);
        gtk_window_set_default_size (GTK_WINDOW (main_window),
                                     window_width,
                                     window_height);

        glade_xml_signal_autoconnect (glade_xml);

        init_icons ();
        setup_treeviews ();
        init_action_dialog ();

        gtk_widget_show_all (main_window);

        /* Give device treeview 40% of the main window */
        g_object_get (G_OBJECT (hpaned), "max-position", &position, NULL);
        position = position * 40 / 100;
        g_object_set (G_OBJECT (hpaned), "position", position, NULL);

        /* Give details treeview 60% of 60% of the main window */
        g_object_get (G_OBJECT (vpaned), "max-position", &position, NULL);
        position = position * 60 / 100;
        g_object_set (G_OBJECT (vpaned), "position", position, NULL);

        expanded = FALSE;

        return TRUE;
}

void
deinit_ui (void)
{
        g_object_unref (glade_xml);
}

