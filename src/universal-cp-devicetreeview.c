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
#include "universal-cp.h"

static const char *default_details[] = {
        "Software", "GUPnP Universal Control Point",
        "Version", VERSION,
        "Author", "Zeeshan Ali <zeenix@gstreamer.net>", NULL};

gboolean expanded;

gboolean
find_device (GtkTreeModel *model,
             const char   *udn,
             GtkTreeIter  *root_iter,
             GtkTreeIter  *iter)
{
        gboolean found = FALSE;
        gboolean more = TRUE;

        if (udn == NULL || root_iter == NULL)
                return found;

        more = gtk_tree_model_iter_children (model, iter, root_iter);

        while (more) {
                GUPnPDeviceInfo *info;
                GdkPixbuf       *pixbuf;
                GtkTreeIter     tmp;

                gtk_tree_model_get (model, iter,
                                    0, &pixbuf,
                                    2, &info, -1);

                if (info && pixbuf == icons[ICON_DEVICE]) {
                        const char *device_udn;

                        device_udn = gupnp_device_info_get_udn (info);

                        if (device_udn && strcmp (device_udn, udn) == 0) {
                                found = TRUE;
                                break;
                        }
                }

                if (info)
                        g_object_unref (G_OBJECT (info));
                if (pixbuf)
                        g_object_unref (G_OBJECT (pixbuf));

                /* recurse into embedded-devices */
                found = find_device (model, udn, iter, &tmp);
                if (found) {
                        *iter = tmp;
                        break;
                }

                more = gtk_tree_model_iter_next (model, iter);
        }

        return found;
}

GUPnPDeviceInfo *
get_service_device (GUPnPServiceInfo *service_info)
{
       GUPnPDeviceInfo *info = NULL;
       GtkWidget       *treeview;
       GtkTreeModel    *model;
       GtkTreeIter      root_iter;
       GtkTreeIter      iter;
       const char      *udn;

       treeview = glade_xml_get_widget (glade_xml, "device-treeview");
       g_assert (treeview != NULL);
       model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
       g_assert (model != NULL);

       if (!gtk_tree_model_get_iter_first (model, &root_iter))
               return NULL;

       udn = gupnp_service_info_get_udn (service_info);
       if (find_device (model, udn, &root_iter, &iter))
                gtk_tree_model_get (model, &iter, 2, &info, -1);

       return info;
}

static void
on_something_selected (GtkTreeSelection *selection,
                       gpointer          user_data)
{
        GtkTreeModel *model;
        GtkTreeIter  iter;

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                GdkPixbuf *pixbuf;

                gtk_tree_model_get (model, &iter, 0, &pixbuf, -1);
                g_assert (pixbuf != NULL);

                /* We recognise things by how they look, don't we? */
                if (pixbuf == icons[ICON_DEVICES] ||
                    pixbuf == icons[ICON_VARIABLES]) {
                        update_details (default_details);
                } else if (pixbuf == icons[ICON_DEVICE]) {
                        GUPnPDeviceInfo *info;

                        gtk_tree_model_get (model, &iter, 2, &info, -1);
                        show_device_details (info);
                        g_object_unref (G_OBJECT (info));
                } else if (pixbuf == icons[ICON_SERVICE]) {
                        GUPnPServiceInfo *info;

                        gtk_tree_model_get (model, &iter, 2, &info, -1);
                        show_service_details (info);
                        g_object_unref (G_OBJECT (info));
                } else if (pixbuf == icons[ICON_VARIABLE]) {
                        GUPnPServiceStateVariableInfo *info;

                        gtk_tree_model_get (model, &iter, 4, &info, -1);
                        show_state_variable_details (info);
                } else if (pixbuf == icons[ICON_ACTION]) {
                        GUPnPServiceActionInfo *info;

                        gtk_tree_model_get (model, &iter, 4, &info, -1);
                        show_action_details (info);
                } else if (pixbuf == icons[ICON_ACTION_ARG]) {
                        GUPnPServiceActionArgInfo *info;

                        gtk_tree_model_get (model, &iter, 4, &info, -1);
                        show_action_arg_details (info);
                }


                g_object_unref (G_OBJECT (pixbuf));
        }

        else
                update_details (default_details);
}

void
remove_device (GUPnPDeviceInfo *info)
{
        GtkWidget    *treeview;
        GtkTreeModel *model;
        GtkTreeIter   root_iter;
        GtkTreeIter   iter;
        const char   *udn;

        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (!gtk_tree_model_get_iter_first (model, &root_iter))
                return;

        if (find_device (model, udn, &root_iter, &iter))
                gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
}

static void
append_action_arguments (GList        *arguments,
                         GtkTreeStore *store,
                         GtkTreeIter  *action_iter)
{
        const GList *iter;

        for (iter = arguments; iter; iter = iter->next) {
                GUPnPServiceActionArgInfo *info;

                info = iter->data;

                gtk_tree_store_insert_with_values (store,
                                                   NULL, action_iter, -1,
                                                   0, icons[ICON_ACTION_ARG],
                                                   1, info->name,
                                                   4, info,
                                                   -1);
        }
}

static void
append_actions (GUPnPServiceProxy         *proxy,
                GUPnPServiceIntrospection *introspection,
                const GList               *actions,
                GtkTreeStore              *store,
                GtkTreeIter               *service_iter)
{
        const GList *iter;

        for (iter = actions; iter; iter = iter->next) {
                GUPnPServiceActionInfo *info;
                GtkTreeIter             action_iter;

                info = iter->data;

                gtk_tree_store_insert_with_values (store, &action_iter,
                                                   service_iter, -1,
                                                   0, icons[ICON_ACTION],
                                                   1, info->name,
                                                   2, proxy,
                                                   3, introspection,
                                                   4, info,
                                                   -1);
                append_action_arguments (info->arguments,
                                         store,
                                         &action_iter);
        }
}

static void
append_state_variables (GUPnPServiceProxy *proxy,
                        const GList       *variables,
                        GtkTreeStore      *store,
                        GtkTreeIter       *service_iter)
{
        const GList *iter;
        GtkTreeIter  variables_iter;

        gtk_tree_store_insert_with_values (store,
                                           &variables_iter,
                                           service_iter,
                                           -1,
                                           0, icons[ICON_VARIABLES],
                                           1, "State variables", -1);

        for (iter = variables; iter; iter = iter->next) {
                GUPnPServiceStateVariableInfo *info;

                info = iter->data;

                gtk_tree_store_insert_with_values (store,
                                                   NULL, &variables_iter, -1,
                                                   0, icons[ICON_VARIABLE],
                                                   1, info->name,
                                                   2, proxy,
                                                   4, info,
                                                   -1);

                /* Set-up event notitications for variable */
                gupnp_service_proxy_add_notify (proxy,
                                                info->name,
                                                info->type,
                                                on_state_variable_changed,
                                                NULL);
        }
}

static void
append_introspection (GUPnPServiceProxy         *proxy,
                      GUPnPServiceIntrospection *introspection,
                      GtkTreeStore              *store,
                      GtkTreeIter               *service_iter)
{
        const GList *list;

        list = gupnp_service_introspection_list_state_variables (
                        introspection);
        if (list)
                append_state_variables (proxy,
                                        list,
                                        store,
                                        service_iter);

        list = gupnp_service_introspection_list_actions (introspection);
        if (list)
                append_actions (proxy,
                                introspection,
                                list,
                                store,
                                service_iter);
}

static void
append_service_tree (GUPnPServiceInfo *info,
                     GtkTreeStore     *store,
                     GtkTreeIter      *device_iter)
{
        char *id;

        id = gupnp_service_info_get_id (info);
        if (id) {
                GUPnPServiceIntrospection *introspection;
                GtkTreeIter                service_iter;

                gtk_tree_store_insert_with_values (store,
                                                   &service_iter,
                                                   device_iter, -1,
                                                   0, icons[ICON_SERVICE],
                                                   1, id,
                                                   2, info, -1);

                introspection = gupnp_service_info_get_introspection (info,
                                                                      NULL);
                if (introspection) {
                        append_introspection (GUPNP_SERVICE_PROXY (info),
                                              introspection,
                                              store,
                                              &service_iter);
                        g_object_unref (introspection);
                }

                g_free (id);
        }
}

static void
append_device_tree (GUPnPDeviceInfo *info,
                    GtkTreeModel    *model,
                    GtkTreeIter     *parent_iter)
{
        char *friendly_name;

        friendly_name = gupnp_device_info_get_friendly_name (info);
        if (friendly_name) {
                GtkTreeIter device_iter;
                GList *child;

                gtk_tree_store_insert_with_values (GTK_TREE_STORE (model),
                                                   &device_iter, parent_iter, -1,
                                                   0, icons[ICON_DEVICE],
                                                   1, friendly_name,
                                                   2, info, -1);
                g_free (friendly_name);

                /* Append the embedded devices */
                for (child = gupnp_device_info_list_devices (info);
                     child;
                     child = g_list_next (child)) {
                        append_device_tree (GUPNP_DEVICE_INFO (child->data),
                                            model,
                                            &device_iter);
                }

                /* Append the services */
                for (child = gupnp_device_info_list_services (info);
                     child;
                     child = g_list_next (child)) {
                        append_service_tree (GUPNP_SERVICE_INFO (child->data),
                                             GTK_TREE_STORE (model),
                                             &device_iter);
                }
        }
}

void
append_device (GUPnPDeviceInfo *info)
{
        GtkWidget    *treeview;
        GtkTreeModel *model;
        GtkTreeIter   root_iter;
        GtkTreeIter   iter;
        const char   *udn;

        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (!gtk_tree_model_get_iter_first (model, &root_iter))
                return;

        if (!find_device (model, udn, &root_iter, &iter)) {

                append_device_tree (info, model, &root_iter);

                if (!expanded) {
                        GtkTreePath *first_row;

                        first_row = gtk_tree_model_get_path (
                                        model,
                                        &root_iter);
                        expanded = gtk_tree_view_expand_row (
                                        GTK_TREE_VIEW (treeview),
                                        first_row,
                                        FALSE);
                }
        }
}

GtkTreeModel *
create_device_treemodel (void)
{
        GtkTreeStore *store;

        store = gtk_tree_store_new (5,
                                    GDK_TYPE_PIXBUF, /* Icon                */
                                    G_TYPE_STRING,   /* Name                */
                                    G_TYPE_OBJECT,   /* Device/Service Info */
                                    G_TYPE_OBJECT,   /* Introspection       */
                                    G_TYPE_POINTER); /* non-object          */

        gtk_tree_store_insert_with_values (store, NULL, NULL, 0,
                                        0, icons[ICON_DEVICES],
                                        1, "UPnP Devices",
                                        -1);
        return GTK_TREE_MODEL (store);
}

void
setup_device_treeview (GtkWidget    *treeview,
                       GtkTreeModel *model,
                       char         *headers[],
                       int           render_index)
{
        GtkTreeSelection  *selection;
        GtkCellRenderer   *renderer;
        GtkTreeViewColumn *column;

        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes (
                        "icon",
                        renderer,
                        "pixbuf", 0,
                        NULL);
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview),
                                     column,
                                     0);

        setup_treeview (treeview, model, headers, render_index);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);
        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (on_something_selected),
                          NULL);
}

GUPnPServiceProxy *
get_selected_service (void)
{
        GUPnPServiceProxy *proxy = NULL;
        GtkWidget         *treeview;
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;
        GtkTreeIter        iter;

        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                GdkPixbuf *pixbuf;

                gtk_tree_model_get (model, &iter,
                                    0, &pixbuf,
                                    2, &proxy, -1);

                g_assert (pixbuf != NULL);

                if (pixbuf != icons[ICON_SERVICE])
                        proxy = NULL;
                g_object_unref (G_OBJECT (pixbuf));
        }

        return proxy;
}

GUPnPServiceActionInfo *
get_selected_action (GUPnPServiceProxy         **ret_proxy,
                     GUPnPServiceIntrospection **ret_introspection)
{
        GUPnPServiceProxy         *proxy;
        GUPnPServiceIntrospection *introspection;
        GUPnPServiceActionInfo    *action_info = NULL;
        GtkWidget                 *treeview;
        GtkTreeModel              *model;
        GtkTreeSelection          *selection;
        GtkTreeIter                iter;

        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                GdkPixbuf *pixbuf;

                gtk_tree_model_get (model, &iter,
                                    0, &pixbuf,
                                    2, &proxy,
                                    3, &introspection,
                                    4, &action_info, -1);

                g_assert (pixbuf != NULL);

                if (pixbuf != icons[ICON_ACTION]) {
                        if (proxy != NULL) {
                                g_object_unref (proxy);
                                proxy = NULL;
                        }

                        if (introspection != NULL) {
                                g_object_unref (introspection);
                                introspection = NULL;
                        }

                        action_info = NULL;
                }
                g_object_unref (G_OBJECT (pixbuf));

                if (ret_proxy)
                        *ret_proxy = proxy;
                else if (proxy != NULL)
                        g_object_unref (proxy);

                if (ret_introspection)
                        *ret_introspection = introspection;
                else if (introspection != NULL)
                        g_object_unref (introspection);
        }

        return action_info;
}

void
on_expand_devices_activate (GtkMenuItem *menuitem,
                            gpointer     user_data)
{
        GtkWidget *treeview;

        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);

        gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));
}

void
on_collapse_devices_activate (GtkMenuItem *menuitem,
                              gpointer     user_data)
{
        GtkWidget *treeview;
        
        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);

        gtk_tree_view_collapse_all (GTK_TREE_VIEW (treeview));
}

