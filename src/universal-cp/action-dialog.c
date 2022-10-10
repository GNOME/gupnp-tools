/*
 * Copyright (C) 2007 Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 * Copyright (C) 2011 Jens Georg <mail@jensge.org>
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 * Authors: Jens Georg <mail@jensge.org>
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
#include <glib/gi18n.h>

#include "gui.h"
#include "device-treeview.h"
#include "icons.h"
#include "main.h"

#include "action-dialog.h"

#define DEFAULT_TEXTVIEW_HEIGHT 55

static GtkWidget *dialog;
static GtkWidget *in_args_grid;
static GtkWidget *out_args_grid;
static GtkWidget *device_label;
static GtkWidget *service_label;
static GtkWidget *action_label;
static GtkWidget *in_args_expander;
static GtkWidget *out_args_expander;
static GtkSizeGroup *label_container_group;

void
on_action_invocation (GtkButton *button,
                      gpointer   user_data);

static void
on_expander_clicked (GObject    *expander,
                     GParamSpec *param_spec,
                     gpointer   user_data)
{
        /* Unfortunately set_size_request doesn't here */
        gtk_window_resize (GTK_WINDOW (user_data), 1, 1);
}

/*
 * 1. Gets rid of all the existing arguments
 * 2. Only show the label if there are any arguments
 */
static void
prepare_action_arguments_grid (GtkContainer                  *grid,
                               GUPnPServiceActionArgDirection direction,
                               GList                         *arguments)
{
        GList     *child_node, *children;

        /* reset expander state */
        gtk_expander_set_expanded (GTK_EXPANDER (in_args_expander),  TRUE);
        gtk_expander_set_expanded (GTK_EXPANDER (out_args_expander), FALSE);

        children = gtk_container_get_children (grid);
        for (child_node = children;
             child_node;
             child_node = child_node->next) {
                gtk_widget_destroy (GTK_WIDGET (child_node->data));
        }
        g_list_free (children);

        if (direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN) {
                gtk_widget_set_visible (in_args_expander,
                                        arguments != NULL);
        } else {
                gtk_widget_set_visible (out_args_expander,
                                        arguments != NULL);
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

                        widget = gtk_combo_box_text_new ();
                        for (node = variable->allowed_values;
                             node;
                             node = node->next) {
                                gtk_combo_box_text_insert (
                                                GTK_COMBO_BOX_TEXT (widget),
                                                index,
                                                NULL,
                                                (const char *) node->data);

                                if (node == variable->allowed_values ||
                                    (default_str != NULL &&
                                     strcmp (default_str, node->data) == 0)) {
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
                                      "min-content-height", DEFAULT_TEXTVIEW_HEIGHT,
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

        g_object_set_data_full (G_OBJECT (widget),
                                "argument-name",
                                g_strdup (arg_info->name),
                                g_free);
        return widget;
}

static void
populate_action_arguments_grid (GtkWidget                     *grid,
                                GList                         *arguments,
                                GUPnPServiceActionArgDirection direction,
                                GUPnPServiceIntrospection     *introspection)
{
        GList *arg_node;
        GtkWidget *last_aligner_grid = NULL;

        g_assert (introspection != NULL);

        prepare_action_arguments_grid (GTK_CONTAINER (grid),
                                       direction,
                                       arguments);

        gtk_widget_set_size_request (dialog, 0, 0);

        for (arg_node = arguments;
             arg_node;
             arg_node = arg_node->next) {
                GUPnPServiceActionArgInfo *arg_info;
                GtkWidget                 *aligner_grid;
                GtkWidget                 *label;
                GtkWidget                 *input_widget;

                arg_info = (GUPnPServiceActionArgInfo *) arg_node->data;

                /* First add the name */
                /* GTK_ALIGN_START seems to have a bug in a size group:
                 * use a container for alignment */
                aligner_grid = gtk_grid_new ();
                gtk_grid_attach_next_to (GTK_GRID (grid),
                                         aligner_grid,
                                         last_aligner_grid,
                                         GTK_POS_BOTTOM,
                                         1, 1);
                last_aligner_grid = aligner_grid;

                label = gtk_label_new (arg_info->name);
                gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
                gtk_widget_set_vexpand (label, TRUE);
                gtk_widget_set_halign (label, GTK_ALIGN_START);
                gtk_grid_attach (GTK_GRID (aligner_grid), label,
                                 0, 0, 1, 1);
                gtk_size_group_add_widget (label_container_group,
                                           aligner_grid);
                gtk_widget_show_all (aligner_grid);

                /* Then the input widget */
                input_widget = create_widget_for_argument (arg_info,
                                                           introspection);
                gtk_widget_set_hexpand (input_widget, TRUE);
                gtk_grid_attach_next_to (GTK_GRID (grid),
                                         input_widget,
                                         aligner_grid,
                                         GTK_POS_RIGHT,
                                         1,1);
                gtk_widget_show_all (input_widget);
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
        GList *in_arguments = NULL;
        GList *out_arguments = NULL;

        setup_action_dialog_labels (GUPNP_SERVICE_INFO (proxy), action_info);

        in_arguments = find_arguments_by_direction (
                        action_info->arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN);
        populate_action_arguments_grid (
                        in_args_grid,
                        in_arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_IN,
                        introspection);

        out_arguments = find_arguments_by_direction (
                        action_info->arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT);
        populate_action_arguments_grid (
                        out_args_grid,
                        out_arguments,
                        GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT,
                        introspection);

        /* If the action only has out arguments, open the out expander */
        if (!in_arguments)
                gtk_expander_set_expanded (GTK_EXPANDER (out_args_expander), TRUE);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);

        g_list_free (in_arguments);
        g_list_free (out_arguments);
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

        if (arg_info->direction == GUPNP_SERVICE_ACTION_ARG_DIRECTION_OUT) {
                *value = NULL;
                return arg_info->name;
        }

        *value = g_slice_alloc0 (sizeof (GValue));
        g_value_init (*value, variable_info->type);

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

                        text = gtk_combo_box_text_get_active_text (
                                        GTK_COMBO_BOX_TEXT (arg_widget));
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

static void
retrieve_in_action_arguments (GUPnPServiceIntrospection *introspection,
                              GUPnPServiceActionInfo    *action_info,
                              GList                     **in_names,
                              GList                     **in_values)
{
        GList      *arg_node;

        arg_node = gtk_container_get_children (GTK_CONTAINER (in_args_grid));

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

                *in_names = g_list_append (*in_names, name);
                *in_values = g_list_append (*in_values, value);
        }
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
        GList                     *it;

        action_info = get_selected_action (NULL, &introspection);
        if (action_info == NULL)
                return;

        arg_node = gtk_container_get_children (GTK_CONTAINER (out_args_grid));

        for (it = arg_node; it; it = it->next) {
                GtkWidget *arg_widget;
                gchar     *name;
                GValue    *value = NULL;

                arg_widget = GTK_WIDGET (it->data);

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
        g_list_free (arg_node);
}

static void
on_action_complete (GObject *object, GAsyncResult *result, gpointer user_data)
{
        GUPnPServiceIntrospection *introspection;
        GUPnPServiceActionInfo    *action_info;
        GHashTable                *out_args;
        GError                    *error = NULL;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);
        GUPnPServiceProxyAction *action;

        action_info = get_selected_action (NULL, &introspection);
        if (action_info == NULL)
                return;

        out_args = retrieve_out_action_arguments (introspection, action_info);

        action = gupnp_service_proxy_call_action_finish (proxy, result, &error);
        if (error != NULL) {
                GtkWidget *error_dialog;

                error_dialog =
                        gtk_message_dialog_new (
                                        GTK_WINDOW (dialog),
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Action failed.\n\nError %d: %s"),
                                        error->code,
                                        error->message);

                gtk_dialog_run (GTK_DIALOG (error_dialog));
                gtk_widget_destroy (error_dialog);

                goto out;
        }

        gupnp_service_proxy_action_get_result_hash (action, out_args, &error);
        if (error != NULL) {
                GtkWidget *error_dialog;

                error_dialog = gtk_message_dialog_new (
                        GTK_WINDOW (dialog),
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR,
                        GTK_BUTTONS_CLOSE,
                        _ ("Action failed.\n\nError %d: %s"),
                        error->code,
                        error->message);

                gtk_dialog_run (GTK_DIALOG (error_dialog));
                gtk_widget_destroy (error_dialog);
        } else {
                display_action_out_arguments (out_args);
        }

out:
        g_clear_error (&error);
        g_hash_table_destroy (out_args);
        g_object_unref (introspection);
}

G_MODULE_EXPORT
void
on_action_invocation (GtkButton *button,
                      gpointer   user_data)
{
        GUPnPServiceProxy         *proxy;
        GUPnPServiceIntrospection *introspection;
        GUPnPServiceActionInfo    *action_info;
        GList                     *in_names = NULL, *in_values = NULL;
        GUPnPServiceProxyAction *action;

        action_info = get_selected_action (&proxy, &introspection);
        if (action_info == NULL)
                return;

        retrieve_in_action_arguments (introspection,
                                      action_info,
                                      &in_names,
                                      &in_values);

        action = gupnp_service_proxy_action_new_from_list (action_info->name,
                                                           in_names,
                                                           in_values);

        gupnp_service_proxy_call_action_async (proxy,
                                               action,
                                               NULL,
                                               on_action_complete,
                                               NULL);
        gupnp_service_proxy_action_unref (action);

        g_list_free (in_names);
        g_list_free_full (in_values, g_value_free);

        g_object_unref (proxy);
        g_object_unref (introspection);
}

void
init_action_dialog (GtkBuilder *builder)
{
        GtkWidget *image;

        label_container_group =
                gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

        /* Dialog box and grids */
        dialog = GTK_WIDGET (gtk_builder_get_object (
                                        builder,
                                        "action-invocation-dialog"));
        g_assert (dialog != NULL);
        in_args_grid = GTK_WIDGET (gtk_builder_get_object (
                                        builder,
                                        "in-action-arguments-grid"));
        g_assert (in_args_grid != NULL);
        out_args_grid = GTK_WIDGET (gtk_builder_get_object (
                                        builder,
                                        "out-action-arguments-grid"));
        g_assert (out_args_grid != NULL);

        /* All the labels */
        in_args_expander = GTK_WIDGET (gtk_builder_get_object (
                                        builder,
                                        "in-action-arguments-expander"));
        g_assert (in_args_expander != NULL);
        out_args_expander = GTK_WIDGET (gtk_builder_get_object (
                                        builder,
                                        "out-action-arguments-expander"));
        g_assert (out_args_expander != NULL);
        device_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "device-label"));
        g_assert (device_label != NULL);
        service_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                            "service-label"));
        g_assert (service_label != NULL);
        action_label = GTK_WIDGET (gtk_builder_get_object (builder,
                                                           "action-label"));
        g_assert (action_label != NULL);

        /* the images */
        image = GTK_WIDGET (gtk_builder_get_object (builder, "device-image"));
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   get_icon_by_id (ICON_DEVICE));

        image = GTK_WIDGET (gtk_builder_get_object (builder, "service-image"));
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   get_icon_by_id (ICON_SERVICE));

        image = GTK_WIDGET (gtk_builder_get_object (builder, "action-image"));
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   get_icon_by_id (ICON_ACTION));

        g_signal_connect (in_args_expander,
                          "notify::expanded",
                          G_CALLBACK (on_expander_clicked),
                          dialog);

        g_signal_connect (out_args_expander,
                          "notify::expanded",
                          G_CALLBACK (on_expander_clicked),
                          dialog);
}

void
deinit_action_dialog (void)
{
        g_object_unref (label_container_group);
        gtk_widget_destroy (dialog);
}

