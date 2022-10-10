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

#include <stdlib.h>
#include <string.h>

#include <gmodule.h>
#include <glib/gi18n.h>

#include "details-treeview.h"
#include "gui.h"

static GtkWidget *treeview;
static GtkWidget *copy_value_menuitem;
static GtkWidget *popup;

gboolean
on_details_treeview_button_release (GtkWidget      *widget,
                                    GdkEventButton *event,
                                    gpointer        user_data);
void
on_details_treeview_row_activate (GtkMenuItem *menuitem,
                                  gpointer     user_data);

void
on_copy_all_details_activate (GtkMenuItem *menuitem,
                              gpointer     user_data);

static gboolean
get_selected_row (GtkTreeIter *iter)
{
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        return gtk_tree_selection_get_selected (selection, &model, iter);
}

G_MODULE_EXPORT
gboolean
on_details_treeview_button_release (GtkWidget      *widget,
                                    GdkEventButton *event,
                                    gpointer        user_data)
{
        if (event->type != GDK_BUTTON_RELEASE ||
            event->button != 3)
                return FALSE;

        /* Only show "Copy Value" menuitem when a row is selected */
        g_object_set (copy_value_menuitem,
                      "visible",
                      get_selected_row (NULL),
                      NULL);

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
        return TRUE;
}

G_MODULE_EXPORT
void
on_details_treeview_row_activate (GtkMenuItem *menuitem,
                                  gpointer     user_data)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (get_selected_row (&iter)) {
                char *val;

                gtk_tree_model_get (model, &iter, 1, &val, -1);
                if (val) {
                        GtkClipboard *clipboard;

                        clipboard = gtk_clipboard_get
                                                (GDK_SELECTION_CLIPBOARD);
                        g_assert (clipboard != NULL);
                        gtk_clipboard_set_text (clipboard, val, -1);
                        g_free (val);
                }
        }
}

G_MODULE_EXPORT
void
on_copy_all_details_activate (GtkMenuItem *menuitem,
                              gpointer     user_data)
{
        GtkClipboard *clipboard;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        char         *copied;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
        g_assert (clipboard != NULL);

        if (!gtk_tree_model_get_iter_first (model, &iter))
                return;

        copied = g_strdup ("");
        do {
                char *name;
                char *val;
                char *pair;

                gtk_tree_model_get (model, &iter,
                                    0, &name,
                                    1, &val, -1);
                if (!name)
                        continue;
                if (!val) {
                        g_free (name);
                        continue;
                }

                pair = g_strjoin (" ", name, val, NULL);

                g_free (val);
                g_free (name);

                val = copied;
                copied = g_strjoin ("\n", copied, pair, NULL);

                g_free (val);
                g_free (pair);
        } while (gtk_tree_model_iter_next (model, &iter));

        gtk_clipboard_set_text (clipboard, copied, -1);

        g_free (copied);
}

void
update_details (const char **tuples)
{
        GtkTreeModel *model;
        const char  **tuple;

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

void
show_action_arg_details (GUPnPServiceActionArgInfo *info)
{
        char *details[32];
        int   i = 0;

        details[i++] = _("Name");
        details[i++] = g_strdup (info->name);
        details[i++] = _("Direction");
        if (info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN)
                details[i++] = g_strdup ("in");
        else
                details[i++] = g_strdup ("out");
        details[i++] = _("Related State Variable");
        details[i++] = g_strdup (info->related_state_variable);
        details[i++] = _("Is Return Value");
        details[i++] = g_strdup (info->retval? _("Yes"): _("No"));
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_action_details (GUPnPServiceActionInfo *info)
{
        char *details[32];
        int   i = 0;

        details[i++] = _("Name");
        details[i++] = g_strdup (info->name);
        details[i++] = _("Number of Arguments");
        details[i++] = g_strdup_printf ("%u",
                                        g_list_length (info->arguments));
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_state_variable_details (GUPnPServiceStateVariableInfo *info)
{
        char  *details[32];
        GValue str_value;
        int    i = 0;

        memset (&str_value, 0, sizeof (GValue));
        g_value_init (&str_value, G_TYPE_STRING);

        details[i++] = _("Name");
        details[i++] = g_strdup (info->name);
        details[i++] = _("Send Events");
        details[i++] = g_strdup (info->send_events? _("Yes"): _("No"));
        details[i++] = _("GType");
        details[i++] = g_strdup (g_type_name (info->type));

        details[i++] = _("Default Value");
        g_value_transform (&info->default_value,
                           &str_value);
        details[i++] = g_value_dup_string (&str_value);

        if (info->is_numeric) {
                details[i++] = _("Minimum");
                g_value_transform (&info->minimum,
                                   &str_value);
                details[i++] = g_value_dup_string (&str_value);
                details[i++] = _("Maximum");
                g_value_transform (&info->maximum,
                                   &str_value);
                details[i++] = g_value_dup_string (&str_value);
                details[i++] = _("Step");
                g_value_transform (&info->step,
                                   &str_value);
                details[i++] = g_value_dup_string (&str_value);
        } else if (info->allowed_values) {
                GList *iter;
                char **valuesv;
                int    j;

                valuesv =
                        g_malloc (sizeof (char *) *
                                  (g_list_length (info->allowed_values) + 1));

                for (j = 0, iter = info->allowed_values;
                     iter;
                     iter = iter->next, j++) {
                        valuesv[j] = (char *) iter->data;
                }
                valuesv[j] = NULL;

                details[i++] = _("Allowed Values");
                details[i++] = g_strjoinv (", ", valuesv);
        }

        g_value_unset (&str_value);
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_service_details (GUPnPServiceInfo *info)
{
        char          *details[32];
        const GUri *uri;
        const char    *str;
        int            i = 0;

        details[i++] = _("Location");
        str = gupnp_service_info_get_location (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = _("UDN");
        str = gupnp_service_info_get_udn (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = _("Type");
        str = gupnp_service_info_get_service_type (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = _("Base URL");
        uri = gupnp_service_info_get_url_base (info);
        if (uri)
                details[i++] = g_uri_to_string ((GUri *) uri);

        details[i++] = _("Service ID");
        details[i++] = gupnp_service_info_get_id (info);
        details[i++] = _("Service URL");
        details[i++] = gupnp_service_info_get_scpd_url (info);
        details[i++] = _("Control URL");
        details[i++] = gupnp_service_info_get_control_url (info);
        details[i++] = _("Event Subscription URL");
        details[i++] = gupnp_service_info_get_event_subscription_url (info);
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
}

void
show_device_details (GUPnPDeviceInfo *info)
{
        char          *details[32];
        const GUri *uri;
        const char    *str;
        int            i = 0;

        details[i++] = _("Location");
        str = gupnp_device_info_get_location (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = _("UDN");
        str = gupnp_device_info_get_udn (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = _("Type");
        str = gupnp_device_info_get_device_type (info);
        if (str)
                details[i++] = g_strdup (str);

        details[i++] = _("Base URL");
        uri = gupnp_device_info_get_url_base (info);
        if (uri)
                details[i++] = g_uri_to_string ((GUri *) uri);

        details[i++] = _("Friendly Name");
        details[i++] = gupnp_device_info_get_friendly_name (info);
        details[i++] = _("Manufacturer");
        details[i++] = gupnp_device_info_get_manufacturer (info);
        details[i++] = _("Manufacturer URL");
        details[i++] = gupnp_device_info_get_manufacturer_url (info);
        details[i++] = _("Model Description");
        details[i++] = gupnp_device_info_get_model_description (info);
        details[i++] = _("Model Name");
        details[i++] = gupnp_device_info_get_model_name (info);
        details[i++] = _("Model Number");
        details[i++] = gupnp_device_info_get_model_number (info);
        details[i++] = _("Model URL");
        details[i++] = gupnp_device_info_get_model_url (info);
        details[i++] = _("Serial Number");
        details[i++] = gupnp_device_info_get_serial_number (info);
        details[i++] = _("UPC");
        details[i++] = gupnp_device_info_get_upc (info);
        details[i++] = _("Presentation URL");
        details[i++] = gupnp_device_info_get_presentation_url (info);
        details[i] = NULL;

        update_details ((const char **) details);

        /* Only free the values */
        for (i = 1; details[i - 1]; i += 2) {
                if (details[i])
                        g_free (details[i]);
        }
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

void
setup_details_treeview (GtkBuilder *builder)
{
        GtkTreeModel *model;
        char         *headers[3] = { _("Name"),
                                     _("Value"),
                                     NULL };

        treeview = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "details-treeview"));
        g_assert (treeview != NULL);
        copy_value_menuitem = GTK_WIDGET (gtk_builder_get_object (builder,
                                                     "copy-value-menuitem"));
        g_assert (copy_value_menuitem != NULL);
        popup = GTK_WIDGET (gtk_builder_get_object (builder, "details-popup"));
        g_assert (popup != NULL);

        model = create_details_treemodel ();
        g_assert (model != NULL);

        setup_treeview (treeview, model, headers, 0);
        g_object_unref (model);
}

