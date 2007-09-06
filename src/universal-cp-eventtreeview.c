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

#include "universal-cp-eventtreeview.h"
#include "universal-cp-gui.h"

void
display_event (const char *notified_at,
               const char *friendly_name,
               const char *service_id,
               const char *variable_name,
               const char *value)
{
        GtkWidget        *treeview;
        GtkTreeModel     *model;
        GtkTreeIter       iter;

        treeview = glade_xml_get_widget (glade_xml, "event-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

        gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            0, notified_at,
                            1, friendly_name,
                            2, service_id,
                            3, variable_name,
                            4, value,
                            -1);
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

GtkTreeModel *
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


