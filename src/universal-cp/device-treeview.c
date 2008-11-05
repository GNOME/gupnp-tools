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
#include "action-dialog.h"
#include "details-treeview.h"
#include "main.h"

static const char *default_details[] = {
        "Software", "GUPnP Universal Control Point",
        "Version", VERSION,
        "Author", "Zeeshan Ali (Khattak) <zeeshanak@gnome.org>", NULL};

static GtkWidget *treeview;
static GtkWidget *popup;
static GtkWidget *subscribe_menuitem;
static GtkWidget *action_menuitem;
static GtkWidget *separator;

static gboolean   expanded;

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
                GtkTreeIter      tmp;
                guint            icon_type;

                gtk_tree_model_get (model, iter,
                                    2, &info,
                                    5, &icon_type, -1);

                if (info) {
                        if (icon_type == ICON_DEVICE) {
                                const char *device_udn;

                                device_udn = gupnp_device_info_get_udn (info);

                                if (device_udn &&
                                    strcmp (device_udn, udn) == 0)
                                        found = TRUE;
                        }

                        g_object_unref (info);
                }

                if (found)
                        break;

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
       GtkTreeModel    *model;
       GtkTreeIter      root_iter;
       GtkTreeIter      iter;
       const char      *udn;

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
setup_device_popup (GtkWidget *popup)
{
        GUPnPServiceProxy *proxy;

        /* See if a service is selected */
        proxy = get_selected_service ();
        if (proxy != NULL) {
                g_object_set (subscribe_menuitem,
                              "visible",
                              TRUE,
                              "active",
                              gupnp_service_proxy_get_subscribed (proxy),
                              NULL);

                g_object_set (action_menuitem,
                              "visible",
                              FALSE,
                              NULL);
        } else {
                GUPnPServiceActionInfo *action;

                g_object_set (subscribe_menuitem,
                              "visible",
                              FALSE,
                              NULL);

                /* See if an action is selected */
                action = get_selected_action (NULL, NULL);
                g_object_set (action_menuitem,
                              "visible",
                              action != NULL,
                              NULL);
        }

        /* Separator should be visible only if either service or action row is
         * selected
         */
        g_object_set (separator,
                      "visible",
                      (proxy != NULL),
                      NULL);
        if (proxy)
                g_object_unref (proxy);
}

gboolean
on_device_treeview_button_release (GtkWidget      *widget,
                                   GdkEventButton *event,
                                   gpointer        user_data)
{
        if (event->type != GDK_BUTTON_RELEASE ||
            event->button != 3)
                return FALSE;

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

void
on_device_treeview_row_activate (GtkMenuItem *menuitem,
                                 gpointer     user_data)
{
        GUPnPServiceProxy         *proxy;
        GUPnPServiceIntrospection *introspection;

        /* See if a service is selected */
        proxy = get_selected_service ();
        if (proxy != NULL) {
                gboolean subscribed;

                subscribed = gupnp_service_proxy_get_subscribed (proxy);
                gupnp_service_proxy_set_subscribed (proxy, !subscribed);
        } else {
                GUPnPServiceActionInfo *action_info;

                /* See if an action is selected */
                action_info = get_selected_action (&proxy, &introspection);
                if (action_info != NULL) {
                        run_action_dialog (action_info,
                                           proxy,
                                           introspection);
                        g_object_unref (introspection);
                }
        }

        if (proxy != NULL)
                g_object_unref (proxy);
}

static void
on_something_selected (GtkTreeSelection *selection,
                       gpointer          user_data)
{
        GtkTreeModel *model;
        GtkTreeIter  iter;

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                guint icon_type;

                gtk_tree_model_get (model, &iter, 5, &icon_type, -1);

                /* We recognise things by how they look, don't we? */
                if (icon_type == ICON_DEVICE) {
                        GUPnPDeviceInfo *info;

                        gtk_tree_model_get (model, &iter, 2, &info, -1);
                        show_device_details (info);
                        g_object_unref (info);
                } else if (icon_type == ICON_SERVICE) {
                        GUPnPServiceInfo *info;

                        gtk_tree_model_get (model, &iter, 2, &info, -1);
                        show_service_details (info);
                        g_object_unref (info);
                } else if (icon_type == ICON_VARIABLE) {
                        GUPnPServiceStateVariableInfo *info;

                        gtk_tree_model_get (model, &iter, 4, &info, -1);
                        show_state_variable_details (info);
                } else if (icon_type == ICON_ACTION) {
                        GUPnPServiceActionInfo *info;

                        gtk_tree_model_get (model, &iter, 4, &info, -1);
                        show_action_details (info);
                } else if (icon_type == ICON_ACTION_ARG_IN ||
                           icon_type == ICON_ACTION_ARG_OUT) {
                        GUPnPServiceActionArgInfo *info;

                        gtk_tree_model_get (model, &iter, 4, &info, -1);
                        show_action_arg_details (info);
                } else
                        update_details (default_details);
        }

        else
                update_details (default_details);
}

void
remove_device (GUPnPDeviceInfo *info)
{
        GtkTreeModel *model;
        GtkTreeIter   root_iter;
        GtkTreeIter   iter;
        const char   *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (!gtk_tree_model_get_iter_first (model, &root_iter))
                return;

        if (find_device (model, udn, &root_iter, &iter)) {
                unschedule_icon_update (info);
                gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }
}

static void
on_state_variable_changed (GUPnPServiceProxy *proxy,
                           const char        *variable_name,
                           GValue            *value,
                           gpointer           user_data)
{
        GUPnPServiceInfo *info = GUPNP_SERVICE_INFO (proxy);
        GUPnPDeviceInfo  *device_info;
        GValue            str_value;
        char             *id;
        char             *friendly_name;
        char             *notified_at;
        struct tm        *tm;
        time_t            current_time;

        /* Get the parent device */
        device_info = get_service_device (info);
        g_return_if_fail (device_info != NULL);

        friendly_name = gupnp_device_info_get_friendly_name (device_info);
        g_object_unref (device_info);
        id = gupnp_service_info_get_id (info);
        /* We neither keep devices without friendlyname
         * nor services without an id
         */
        g_assert (friendly_name != NULL);
        g_assert (id != NULL);

        memset (&str_value, 0, sizeof (GValue));
        g_value_init (&str_value, G_TYPE_STRING);
        g_value_transform (value, &str_value);

        current_time = time (NULL);
        tm = localtime (&current_time);
        notified_at = g_strdup_printf ("%02d:%02d",
                                       tm->tm_hour,
                                       tm->tm_min);

        display_event (notified_at,
                       friendly_name,
                       id,
                       variable_name,
                       g_value_get_string (&str_value));

        g_free (notified_at);
        g_free (friendly_name);
        g_free (id);
        g_value_unset (&str_value);
}

static void
on_device_icon_available (GUPnPDeviceInfo *info,
                          GdkPixbuf       *icon)
{
        GtkTreeModel *model;
        GtkTreeIter   root_iter;
        GtkTreeIter   device_iter;
        const char   *udn;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (!gtk_tree_model_get_iter_first (model, &root_iter))
                return;

        if (find_device (model, udn, &root_iter, &device_iter)) {
                gtk_tree_store_set (GTK_TREE_STORE (model),
                                    &device_iter,
                                    0, icon,
                                    -1);
        } else {
                g_object_unref (icon);
        }
}

static void
append_action_arguments (GList        *arguments,
                         GtkTreeStore *store,
                         GtkTreeIter  *action_iter)
{
        const GList *iter;

        for (iter = arguments; iter; iter = iter->next) {
                GUPnPServiceActionArgInfo *info;
                IconID                     icon_id;

                info = iter->data;

                if (info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN) {
                        icon_id = ICON_ACTION_ARG_IN;
                } else {
                        icon_id = ICON_ACTION_ARG_OUT;
                }

                gtk_tree_store_insert_with_values
                                (store,
                                 NULL, action_iter, -1,
                                 0, get_icon_by_id (icon_id),
                                 1, info->name,
                                 4, info,
                                 5, icon_id,
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

                gtk_tree_store_insert_with_values
                                (store, &action_iter,
                                 service_iter, -1,
                                 0, get_icon_by_id (ICON_ACTION),
                                 1, info->name,
                                 2, proxy,
                                 3, introspection,
                                 4, info,
                                 5, ICON_ACTION,
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
                                           service_iter, -1,
                                           0, get_icon_by_id (ICON_VARIABLES),
                                           1, "State variables",
                                           5, ICON_VARIABLES,
                                           -1);

        for (iter = variables; iter; iter = iter->next) {
                GUPnPServiceStateVariableInfo *info;

                info = iter->data;

                gtk_tree_store_insert_with_values
                                (store,
                                 NULL, &variables_iter, -1,
                                 0, get_icon_by_id (ICON_VARIABLE),
                                 1, info->name,
                                 2, proxy,
                                 4, info,
                                 5, ICON_VARIABLE,
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

        if (introspection == NULL) {
                gtk_tree_store_insert_with_values (store,
                                 NULL, service_iter, -1,
                                 0, get_icon_by_id (ICON_MISSING),
                                 1, "Information not available",
                                 5, ICON_MISSING,
                                 -1);

                return;
        }

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
got_introspection (GUPnPServiceInfo          *info,
                   GUPnPServiceIntrospection *introspection,
                   const GError              *error,
                   gpointer                   user_data)
{
        GtkTreeModel *model;
        GtkTreeIter  *service_iter;

        service_iter = (GtkTreeIter *) user_data;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        append_introspection (GUPNP_SERVICE_PROXY (info),
                              introspection,
                              GTK_TREE_STORE (model),
                              service_iter);

        g_slice_free (GtkTreeIter, service_iter);
        if (introspection != NULL) {
                g_object_unref (introspection);

                /* Services are subscribed to by default */
                gupnp_service_proxy_set_subscribed (GUPNP_SERVICE_PROXY (info), TRUE);
        }
}

static void
append_service_tree (GUPnPServiceInfo *info,
                     GtkTreeStore     *store,
                     GtkTreeIter      *device_iter)
{
        char *id;

        id = gupnp_service_info_get_id (info);
        if (id) {
                GtkTreeIter *service_iter;

                service_iter = g_slice_new (GtkTreeIter);
                gtk_tree_store_insert_with_values
                                (store,
                                 service_iter,
                                 device_iter, -1,
                                 0, get_icon_by_id (ICON_SERVICE),
                                 1, id,
                                 2, info,
                                 5, ICON_SERVICE,
                                 -1);

                gupnp_service_info_get_introspection_async (info,
                                                            got_introspection,
                                                            service_iter);

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

                gtk_tree_store_insert_with_values
                                (GTK_TREE_STORE (model),
                                 &device_iter, parent_iter, -1,
                                 0, get_icon_by_id (ICON_DEVICE),
                                 1, friendly_name,
                                 2, info,
                                 5, ICON_DEVICE,
                                 -1);
                g_free (friendly_name);

                schedule_icon_update (info, on_device_icon_available);

                /* Append the embedded devices */
                child = gupnp_device_info_list_devices (info);
                while (child) {
                        append_device_tree (GUPNP_DEVICE_INFO (child->data),
                                            model,
                                            &device_iter);
                        g_object_unref (child->data);
                        child = g_list_delete_link (child, child);
                }

                /* Append the services */
                child = gupnp_device_info_list_services (info);
                while (child) {
                        append_service_tree (GUPNP_SERVICE_INFO (child->data),
                                             GTK_TREE_STORE (model),
                                             &device_iter);
                        g_object_unref (child->data);
                        child = g_list_delete_link (child, child);
                }
        }
}

void
append_device (GUPnPDeviceInfo *info)
{
        GtkTreeModel *model;
        GtkTreeIter   root_iter;
        GtkTreeIter   iter;
        const char   *udn;

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

static GtkTreeModel *
create_device_treemodel (void)
{
        GtkTreeStore *store;

        store = gtk_tree_store_new (6,
                                    GDK_TYPE_PIXBUF, /* Icon                */
                                    G_TYPE_STRING,   /* Name                */
                                    G_TYPE_OBJECT,   /* Device/Service Info */
                                    G_TYPE_OBJECT,   /* Introspection       */
                                    G_TYPE_POINTER,  /* non-object          */
                                    G_TYPE_UINT);    /* icon type           */

        gtk_tree_store_insert_with_values (store, NULL, NULL, 0,
                                        0, get_icon_by_id (ICON_NETWORK),
                                        1, "UPnP Network",
                                        5, ICON_NETWORK,
                                        -1);
        return GTK_TREE_MODEL (store);
}

void
setup_device_treeview (GladeXML *glade_xml)
{
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;
        GtkCellRenderer   *renderer;
        GtkTreeViewColumn *column;
        char              *headers[] = { "Device", NULL};

        treeview = glade_xml_get_widget (glade_xml, "device-treeview");
        g_assert (treeview != NULL);
        popup = glade_xml_get_widget (glade_xml, "device-popup");
        g_assert (popup != NULL);

        g_object_weak_ref (G_OBJECT (treeview),
                           (GWeakNotify) gtk_widget_destroy,
                           popup);
        subscribe_menuitem = glade_xml_get_widget (glade_xml,
                                                   "subscribe-to-events");
        g_assert (subscribe_menuitem != NULL);
        action_menuitem = glade_xml_get_widget (glade_xml, "invoke-action");
        g_assert (action_menuitem != NULL);
        separator = glade_xml_get_widget (glade_xml, "device-popup-separator");
        g_assert (separator != NULL);

        model = create_device_treemodel ();
        g_assert (model != NULL);

        setup_treeview (treeview, model, headers, 1);
        g_object_unref (model);

        column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), 0);
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_add_attribute (column,
                                            renderer,
                                            "pixbuf", 0);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);
        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (on_something_selected),
                          NULL);

        expanded = FALSE;
}

GUPnPServiceProxy *
get_selected_service (void)
{
        GUPnPServiceProxy *proxy = NULL;
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;
        GtkTreeIter        iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                guint icon_type;

                gtk_tree_model_get (model, &iter,
                                    2, &proxy,
                                    5, &icon_type, -1);

                if (icon_type != ICON_SERVICE)
                        proxy = NULL;
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
        GtkTreeModel              *model;
        GtkTreeSelection          *selection;
        GtkTreeIter                iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                guint icon_type;

                gtk_tree_model_get (model, &iter,
                                    2, &proxy,
                                    3, &introspection,
                                    4, &action_info,
                                    5, &icon_type,
                                    -1);

                if (icon_type != ICON_ACTION) {
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
        gtk_tree_view_expand_all (GTK_TREE_VIEW (treeview));
}

void
on_collapse_devices_activate (GtkMenuItem *menuitem,
                              gpointer     user_data)
{
        gtk_tree_view_collapse_all (GTK_TREE_VIEW (treeview));
}

