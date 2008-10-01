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
#include "device-treeview.h"
#include "icons.h"
#include "main.h"

static GtkWidget *dialog;
static GtkWidget *in_args_table;
static GtkWidget *out_args_table;
static GtkWidget *device_label;
static GtkWidget *service_label;
static GtkWidget *action_label;
static GtkWidget *in_args_label;
static GtkWidget *out_args_label;
static GtkSizeGroup *static_labels_group;
static GtkSizeGroup *arguments_label_group;
static GtkSizeGroup *arguments_widget_group;

/*
 * 1. Gets rid of all the existing arguments
 * 2. resizes the table according to the number of new arguments
 * 3. Only show the label if there are any arguments
 */
static void
prepare_action_arguments_table (GtkContainer                  *table,
                                GUPnPServiceActionArgDirection direction,
                                GList                         *arguments)
{
        GList     *child_node;
        GtkWidget *label;

        for (child_node = gtk_container_get_children (table);
             child_node;
             child_node = child_node->next) {
                GtkWidget *widget;
                gchar     *name;

                widget = GTK_WIDGET (child_node->data);
                name = g_object_get_data (G_OBJECT (widget),
                                   "argument-name");
                if (name)
                        g_free (name);

                if (GTK_IS_LABEL (widget)) {
                        gtk_size_group_remove_widget (arguments_label_group,
                                                      widget);
                } else {
                        gtk_size_group_remove_widget (arguments_widget_group,
                                                      widget);
                }

                gtk_container_remove (table, widget);
        }

        if (direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN) {
                label = in_args_label;
        } else {
                label = out_args_label;
        }

        if (arguments) {
                gtk_table_resize (GTK_TABLE (table),
                                  g_list_length (arguments),
                                  2);
                gtk_widget_show (label);
        } else {
                /* No need to resize the table if it's not visible */
                gtk_widget_hide (label);
        }
}

static GtkWidget *
create_widget_for_argument (GUPnPServiceActionArgInfo *arg_info,
                            GUPnPServiceIntrospection *introspection)
{
        GtkWidget                           *widget;
        const GUPnPServiceStateVariableInfo *variable;
        GValue                               default_value;

        variable = gupnp_service_introspection_get_state_variable (
                        introspection,
                        arg_info->related_state_variable);

        memset (&default_value, 0, sizeof (GValue));
        if (variable->is_numeric) {
                GValue min, max, step;

                memset (&min, 0, sizeof (GValue));
                memset (&max, 0, sizeof (GValue));
                memset (&step, 0, sizeof (GValue));

                g_value_init (&min, G_TYPE_DOUBLE);
                g_value_init (&max, G_TYPE_DOUBLE);
                g_value_init (&step, G_TYPE_DOUBLE);
                g_value_init (&default_value, G_TYPE_DOUBLE);

                g_value_transform (&variable->minimum, &min);
                g_value_transform (&variable->maximum, &max);
                g_value_transform (&variable->step, &step);
                g_value_transform (&variable->default_value, &default_value);

                widget = gtk_spin_button_new_with_range (
                                g_value_get_double (&min),
                                g_value_get_double (&max),
                                g_value_get_double (&step));
                gtk_spin_button_set_value (
                                GTK_SPIN_BUTTON (widget),
                                g_value_get_double (&default_value));

                if (arg_info->direction ==
                    GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT)
                        gtk_widget_set_sensitive (widget, FALSE);

                g_value_unset (&min);
                g_value_unset (&max);
                g_value_unset (&step);
                g_value_unset (&default_value);
        } else if (variable->type == G_TYPE_BOOLEAN) {
                gboolean active;

                widget = gtk_check_button_new ();

                active = g_value_get_boolean (&variable->default_value);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                              active);

                if (arg_info->direction ==
                    GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT)
                        gtk_widget_set_sensitive (widget, FALSE);

        } else {
                const gchar *default_str;

                g_value_init (&default_value, G_TYPE_STRING);
                g_value_transform (&variable->default_value, &default_value);

                default_str = g_value_get_string (&default_value);

                /* Use a combobox if we are restricted to a list of values and
                 * it's an input argument */
                if (variable->allowed_values &&
                    arg_info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN) {
                        GList *node;
                        gint index = 0;

                        widget = gtk_combo_box_new_text ();
                        for (node = variable->allowed_values;
                             node;
                             node = node->next) {
                                gtk_combo_box_insert_text (
                                                GTK_COMBO_BOX (widget),
                                                index,
                                                (const char *) node->data);

                                if (default_str != NULL &&
                                    strcmp (default_str, node->data) == 0) {
                                        gtk_combo_box_set_active (
                                                        GTK_COMBO_BOX (widget),
                                                        index);
                                }

                                index++;
                        }
                } else {
                        GtkTextView   *text_view;
                        GtkTextBuffer *text_buffer;

                        text_view = GTK_TEXT_VIEW (gtk_text_view_new ());
                        widget = gtk_scrolled_window_new (NULL, NULL);
                        g_object_set (widget,
                                      "shadow-type", GTK_SHADOW_IN,
                                      "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                      "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                      NULL);

                        gtk_container_add (GTK_CONTAINER (widget),
                                           GTK_WIDGET (text_view));
                        gtk_text_view_set_wrap_mode (text_view, GTK_WRAP_CHAR);
                        text_buffer = gtk_text_view_get_buffer (text_view);

                        if (default_str != NULL)
                                gtk_text_buffer_set_text (text_buffer,
                                                          default_str,
                                                          -1);

                        if (arg_info->direction ==
                            GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT)
                                gtk_text_view_set_editable (text_view, FALSE);

                }

                g_value_unset (&default_value);
        }

        g_object_set_data (G_OBJECT (widget),
                           "argument-name",
                           g_strdup (arg_info->name));
        return widget;
}

static void
populate_action_arguments_table (GtkWidget                     *table,
                                 GList                         *arguments,
                                 GUPnPServiceActionArgDirection direction,
                                 GUPnPServiceIntrospection     *introspection)
{
        GList *arg_node;
        guint row;

        g_assert (introspection != NULL);

        prepare_action_arguments_table (GTK_CONTAINER (table),
                                        direction,
                                        arguments);

        /* FIXME: Is there a better way to shrink the dialog? */
        gtk_window_resize (GTK_WINDOW (dialog), 10, 10);

        row = 0;
        for (arg_node = arguments;
             arg_node;
             arg_node = arg_node->next) {
                GUPnPServiceActionArgInfo *arg_info;
                GtkWidget                 *label;
                GtkWidget                 *input_widget;

                arg_info = (GUPnPServiceActionArgInfo *) arg_node->data;

                /* First add the name */
                label = gtk_label_new (arg_info->name);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
                gtk_size_group_add_widget (arguments_label_group,
                                           label);
                gtk_table_attach (GTK_TABLE (table),
                                  label,
                                  0,
                                  1,
                                  row,
                                  row + 1,
                                  GTK_SHRINK,
                                  GTK_EXPAND | GTK_FILL,
                                  2,
                                  2);
                gtk_widget_show (label);

                /* Then the input widget */
                input_widget = create_widget_for_argument (arg_info,
                                                           introspection);
                gtk_size_group_add_widget (arguments_widget_group,
                                           input_widget);
                gtk_table_attach (GTK_TABLE (table),
                                  input_widget,
                                  1,
                                  2,
                                  row,
                                  row + 1,
                                  GTK_EXPAND | GTK_FILL,
                                  GTK_EXPAND | GTK_FILL,
                                  2,
                                  2);
                gtk_widget_show_all (input_widget);

                row += 1;
        }
}

static void
setup_action_dialog_labels (GUPnPServiceInfo       *service_info,
                            GUPnPServiceActionInfo *action_info)
{
        GUPnPDeviceInfo *device_info;
        gchar           *friendly_name;
        gchar           *id;

        device_info = get_service_device (service_info);
        friendly_name = gupnp_device_info_get_friendly_name (device_info);
        gtk_label_set_text (GTK_LABEL (device_label), friendly_name);
        g_free (friendly_name);

        id = gupnp_service_info_get_id (service_info);
        gtk_label_set_text (GTK_LABEL (service_label), id);
        g_free (id);

        gtk_label_set_text (GTK_LABEL (action_label), action_info->name);
}

static GList *
find_arguments_by_direction (GList                         *arguments,
                             GUPnPServiceActionArgDirection direction)
{
        GList *ret_list = NULL;
        GList *arg_node;

        for (arg_node = arguments;
             arg_node;
             arg_node = arg_node->next) {
                GUPnPServiceActionArgInfo *arg_info;

                arg_info = (GUPnPServiceActionArgInfo *) arg_node->data;
                if (arg_info->direction == direction) {
                        ret_list = g_list_append (ret_list, arg_node->data);
                }
        }

        return ret_list;
}

void
run_action_dialog (GUPnPServiceActionInfo    *action_info,
                   GUPnPServiceProxy         *proxy,
                   GUPnPServiceIntrospection *introspection)
{
        GList *in_arguments;
        GList *out_arguments;

        setup_action_dialog_labels (GUPNP_SERVICE_INFO (proxy), action_info);

        in_arguments = find_arguments_by_direction (
                        action_info->arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN);
        populate_action_arguments_table (
                        in_args_table,
                        in_arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN,
                        introspection);

        out_arguments = find_arguments_by_direction (
                        action_info->arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT);
        populate_action_arguments_table (
                        out_args_table,
                        out_arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT,
                        introspection);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);

}

static void
g_value_free (gpointer data)
{
       g_value_unset ((GValue *) data);
       g_slice_free (GValue, data);
}

static GUPnPServiceActionArgInfo *
get_action_argument (GUPnPServiceActionInfo *action_info,
                     const gchar            *argument_name)
{
        GUPnPServiceActionArgInfo *arg_info = NULL;
        GList                     *arg_node;

        g_assert (argument_name != NULL);

        for (arg_node = action_info->arguments;
             arg_node;
             arg_node = arg_node->next) {
                GUPnPServiceActionArgInfo *info;

                info = (GUPnPServiceActionArgInfo *) arg_node->data;
                if (strcmp (info->name, argument_name) == 0) {
                        arg_info = info;
                        break;
                }
        }

        return arg_info;
}

static gchar *
get_action_arg_widget_info (GtkWidget                 *arg_widget,
                            GUPnPServiceIntrospection *introspection,
                            GUPnPServiceActionInfo    *action_info,
                            GValue                   **value)
{
        GUPnPServiceActionArgInfo           *arg_info;
        const GUPnPServiceStateVariableInfo *variable_info;
        gchar                               *arg_name;

        g_assert (value != NULL);

        arg_name = g_object_get_data (G_OBJECT (arg_widget),
                                      "argument-name");
        if (arg_name == NULL)
                return NULL;

        arg_info = get_action_argument (action_info, arg_name);
        variable_info = gupnp_service_introspection_get_state_variable (
                                        introspection,
                                        arg_info->related_state_variable);

        *value = g_slice_alloc0 (sizeof (GValue));
        g_value_init (*value, variable_info->type);

        if (arg_info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT)
                return arg_info->name;

        if (variable_info->is_numeric) {
                GValue double_value;
                gdouble num;

                memset (&double_value, 0, sizeof (GValue));
                g_value_init (&double_value, G_TYPE_DOUBLE);

                num = gtk_spin_button_get_value (GTK_SPIN_BUTTON (arg_widget));
                g_value_set_double (&double_value, num);
                g_value_transform (&double_value, *value);

                g_value_unset (&double_value);
        } else if (variable_info->type == G_TYPE_BOOLEAN) {
                gboolean active;

                active = gtk_toggle_button_get_active (
                                GTK_TOGGLE_BUTTON (arg_widget));
                g_value_set_boolean (*value,
                                     active);
        } else {
                GValue str_value;

                memset (&str_value, 0, sizeof (GValue));
                g_value_init (&str_value, G_TYPE_STRING);

                if (GTK_IS_SCROLLED_WINDOW (arg_widget)) {
                        GtkWidget     *text_view;
                        GtkTextBuffer *text_buffer;
                        GtkTextIter   start, end;
                        const gchar   *text;

                        text_view = gtk_bin_get_child (GTK_BIN (arg_widget));
                        text_buffer = gtk_text_view_get_buffer (
                                        GTK_TEXT_VIEW (text_view));
                        gtk_text_buffer_get_bounds (text_buffer,
                                                    &start,
                                                    &end);
                        text = gtk_text_buffer_get_text (text_buffer,
                                                         &start,
                                                         &end,
                                                         FALSE);
                        g_value_set_string (&str_value, text);
                }
                else {
                        gchar *text;

                        text = gtk_combo_box_get_active_text (
                                        GTK_COMBO_BOX (arg_widget));
                        g_value_set_string (&str_value, text);
                        g_free (text);
                }

                g_value_transform (&str_value, *value);

                g_value_unset (&str_value);
        }

        return arg_info->name;
}

static void
update_out_action_argument_widget (GtkWidget                 *arg_widget,
                                   GUPnPServiceIntrospection *introspection,
                                   GUPnPServiceActionInfo    *action_info,
                                   GValue                    *value)
{
        GUPnPServiceActionArgInfo           *arg_info;
        const GUPnPServiceStateVariableInfo *variable_info;
        gchar                               *argument_name;

        g_assert (value != NULL);

        argument_name = g_object_get_data (G_OBJECT (arg_widget),
                                           "argument-name");

        if (argument_name == NULL)
                return;

        arg_info = get_action_argument (action_info, argument_name);
        variable_info = gupnp_service_introspection_get_state_variable (
                                        introspection,
                                        arg_info->related_state_variable);

        g_assert (arg_info->direction ==
                  GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT);

        if (variable_info->is_numeric) {
                GValue double_value;
                gdouble num;

                memset (&double_value, 0, sizeof (GValue));
                g_value_init (&double_value, G_TYPE_DOUBLE);

                g_value_transform (value, &double_value);
                num = g_value_get_double (&double_value);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (arg_widget), num);

                g_value_unset (&double_value);
        } else if (variable_info->type == G_TYPE_BOOLEAN) {
                gboolean active;

                active = g_value_get_boolean (value);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (arg_widget),
                                              active);
        } else {
                GValue         str_value;
                const gchar   *text;

                memset (&str_value, 0, sizeof (GValue));
                g_value_init (&str_value, G_TYPE_STRING);
                g_value_transform (value, &str_value);

                text = g_value_get_string (&str_value);
                if (text != NULL) {
                        GtkWidget     *text_view;
                        GtkTextBuffer *text_buffer;

                        text_view = gtk_bin_get_child (GTK_BIN (arg_widget));
                        text_buffer = gtk_text_view_get_buffer
                                                (GTK_TEXT_VIEW (text_view));
                        gtk_text_buffer_set_text (text_buffer, text, -1);
                }

                g_value_unset (&str_value);
        }
}

static GHashTable *
retrieve_in_action_arguments (GUPnPServiceIntrospection *introspection,
                              GUPnPServiceActionInfo    *action_info)
{
        GHashTable *in_args;
        GList      *arg_node;

        in_args = g_hash_table_new_full (g_str_hash,
                                         g_direct_equal,
                                         NULL,
                                         g_value_free);

        arg_node = gtk_container_get_children (GTK_CONTAINER (in_args_table));

        for (; arg_node; arg_node = arg_node->next) {
                GtkWidget *arg_widget;
                gchar     *name;
                GValue    *value;

                arg_widget = GTK_WIDGET (arg_node->data);

                name = get_action_arg_widget_info (arg_widget,
                                                   introspection,
                                                   action_info,
                                                   &value);
                if (name == NULL)
                        continue;

                g_hash_table_insert (in_args, name, value);
        }

        return in_args;
}

static GHashTable *
retrieve_out_action_arguments (GUPnPServiceIntrospection *introspection,
                               GUPnPServiceActionInfo    *action_info)
{
        GHashTable *out_args;
        GList      *arg_node;

        out_args = g_hash_table_new_full (g_str_hash,
                                          g_direct_equal,
                                          NULL,
                                          g_value_free);

        for (arg_node = action_info->arguments;
             arg_node;
             arg_node = arg_node->next) {
                GUPnPServiceActionArgInfo           *arg_info;
                const GUPnPServiceStateVariableInfo *variable_info;
                GValue                              *value;

                arg_info = (GUPnPServiceActionArgInfo *) arg_node->data;
                if (arg_info->direction ==
                    GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN)
                        continue;

                variable_info =
                        gupnp_service_introspection_get_state_variable (
                                        introspection,
                                        arg_info->related_state_variable);

                value = g_slice_alloc0 (sizeof (GValue));
                g_value_init (value, variable_info->type);
                g_hash_table_insert (out_args,
                                     arg_info->name,
                                     value);
        }

        return out_args;
}

static void
display_action_out_arguments (GHashTable *out_args)
{
        GUPnPServiceIntrospection *introspection;
        GUPnPServiceActionInfo    *action_info;
        GList                     *arg_node;

        action_info = get_selected_action (NULL, &introspection);
        if (action_info == NULL)
                return;

        arg_node = gtk_container_get_children (GTK_CONTAINER (out_args_table));

        for (; arg_node; arg_node = arg_node->next) {
                GtkWidget *arg_widget;
                gchar     *name;
                GValue    *value;

                arg_widget = GTK_WIDGET (arg_node->data);

                name = get_action_arg_widget_info (arg_widget,
                                                   introspection,
                                                   action_info,
                                                   &value);
                if (name == NULL)
                        continue;

                value = g_hash_table_lookup (out_args, name);
                update_out_action_argument_widget (arg_widget,
                                                   introspection,
                                                   action_info,
                                                   value);
        }

        g_object_unref (introspection);
}

static void
on_action_complete (GUPnPServiceProxy       *proxy,
                    GUPnPServiceProxyAction *action,
                    gpointer                 user_data)
{
        GUPnPServiceIntrospection *introspection;
        GUPnPServiceActionInfo    *action_info;
        GHashTable                *out_args;
        GError                    *error = NULL;

        action_info = get_selected_action (NULL, &introspection);
        if (action_info == NULL)
                return;

        out_args = retrieve_out_action_arguments (introspection, action_info);

        gupnp_service_proxy_end_action_hash (proxy,
                                             action,
                                             &error,
                                             out_args);
        if (error) {
                GtkWidget *error_dialog;

                error_dialog = gtk_message_dialog_new (dialog,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_ERROR,
                                                       GTK_BUTTONS_CLOSE,
                                                       "%s\n\n%s %s (%d)",
                                                       "Action failed.",
                                                       "Error: ",
                                                       error->message,
                                                       error->code);

                gtk_dialog_run (error_dialog);
                gtk_widget_destroy (error_dialog);

                g_error_free (error);
        } else {
                display_action_out_arguments (out_args);
        }

        g_hash_table_destroy (out_args);
        g_object_unref (introspection);
}

void
on_action_invocation (GtkButton *button,
                      gpointer   user_data)
{
        GUPnPServiceProxy         *proxy;
        GUPnPServiceIntrospection *introspection;
        GUPnPServiceActionInfo    *action_info;
        GHashTable                *in_args;

        action_info = get_selected_action (&proxy, &introspection);
        if (action_info == NULL)
                return;

        in_args = retrieve_in_action_arguments (introspection,
                                                action_info);

        gupnp_service_proxy_begin_action_hash (proxy,
                                              action_info->name,
                                              on_action_complete,
                                              NULL,
                                              in_args);

        g_hash_table_destroy (in_args);
        g_object_unref (proxy);
        g_object_unref (introspection);
}

void
init_action_dialog (GladeXML *glade_xml)
{
        GtkWidget *image;

        arguments_label_group =
                gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
        arguments_widget_group =
                gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
        static_labels_group =
                gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

        /* Dialog box and tables */
        dialog = glade_xml_get_widget (glade_xml, "action-invocation-dialog");
        g_assert (dialog != NULL);
        in_args_table = glade_xml_get_widget (glade_xml,
                                              "in-action-arguments-table");
        g_assert (in_args_table != NULL);
        out_args_table = glade_xml_get_widget (glade_xml,
                                               "out-action-arguments-table");
        g_assert (out_args_table != NULL);

        /* All the labels */
        in_args_label = glade_xml_get_widget (glade_xml,
                                              "in-action-arguments-label");
        g_assert (in_args_label != NULL);
        out_args_label = glade_xml_get_widget (glade_xml,
                                              "out-action-arguments-label");
        g_assert (out_args_label != NULL);
        device_label = glade_xml_get_widget (glade_xml, "device-label");
        g_assert (device_label != NULL);
        gtk_size_group_add_widget (static_labels_group, device_label);
        service_label = glade_xml_get_widget (glade_xml, "service-label");
        g_assert (service_label != NULL);
        gtk_size_group_add_widget (static_labels_group, service_label);
        action_label = glade_xml_get_widget (glade_xml, "action-label");
        g_assert (action_label != NULL);
        gtk_size_group_add_widget (static_labels_group, action_label);

        /* the images */
        image = glade_xml_get_widget (glade_xml, "device-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   get_icon_by_id (ICON_DEVICE));

        image = glade_xml_get_widget (glade_xml, "service-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   get_icon_by_id (ICON_SERVICE));

        image = glade_xml_get_widget (glade_xml, "action-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   get_icon_by_id (ICON_ACTION));
}

void
deinit_action_dialog (void)
{
        gtk_widget_destroy (dialog);
}

