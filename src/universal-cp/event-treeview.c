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

#include "event-treeview.h"
#include "gui.h"

#define MAX_VALUE_SIZE 128

static GtkWidget *treeview;
static GtkWidget *popup;
static GtkWidget *scrolled_window;
static GtkWidget *copy_event_menuitem;

gboolean
on_event_treeview_button_release (GtkWidget      *widget,
                                  GdkEventButton *event,
                                  gpointer        user_data);

void
on_event_treeview_row_activate (GtkMenuItem *menuitem,
                                gpointer     user_data);

void
on_copy_all_events_activate (GtkMenuItem *menuitem,
                             gpointer     user_data);

void
on_clear_event_log_activate (GtkMenuItem *menuitem,
                             gpointer     user_data);

void
on_event_log_activate (GtkCheckMenuItem *menuitem,
                       gpointer          user_data);

void
on_subscribe_to_events_activate (GtkCheckMenuItem *menuitem,
                                 gpointer          user_data);

static gboolean
on_query_tooltip (GtkWidget  *widget,
                  gint        x,
                  gint        y,
                  gboolean    keyboard_mode,
                  GtkTooltip *tooltip,
                  gpointer    user_data)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GtkTreePath  *path;
        gchar        *value;
        gboolean      ret = FALSE;

        gtk_tree_view_convert_widget_to_bin_window_coords
                                (GTK_TREE_VIEW (treeview),
                                 x,
                                 y,
                                 &x,
                                 &y);

        if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
                                            x,
                                            y,
                                            &path,
                                            NULL,
                                            NULL,
                                            NULL)) {
                return ret;
        }

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);

        if (G_UNLIKELY (!gtk_tree_model_get_iter (model, &iter, path))) {
                gtk_tree_path_free (path);

                return ret;
        }

        gtk_tree_model_get (model, &iter,
                            5, &value, -1);
        if (value) {
                if (strlen (value) != 0) {
                        ret = TRUE;
                        gtk_tooltip_set_text (tooltip, value);
                }

                g_free (value);
        }

        gtk_tree_path_free (path);

        return ret;
}

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

static void
setup_event_popup (GtkWidget *widget)
{
        /* Only show "Copy Value" menuitem when a row is selected */
        g_object_set (copy_event_menuitem,
                      "visible",
                      get_selected_row (NULL),
                      NULL);
}

G_MODULE_EXPORT
gboolean
on_event_treeview_button_release (GtkWidget      *widget,
                                  GdkEventButton *event,
                                  gpointer        user_data)
{
        if (event->type != GDK_BUTTON_RELEASE ||
            event->button != 3)
                return FALSE;

        setup_event_popup (popup);

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
on_event_treeview_row_activate (GtkMenuItem *menuitem,
                                gpointer     user_data)
{
        GtkClipboard *clipboard;
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
        g_assert (clipboard != NULL);

        if (get_selected_row (&iter)) {
                char *fields[5];
                char *event_str;

                gtk_tree_model_get (model, &iter,
                                    0, &fields[0],
                                    1, &fields[1],
                                    2, &fields[2],
                                    3, &fields[3],
                                    5, &fields[4], -1);
                if (G_UNLIKELY (!fields[0] ||
                                !fields[1] ||
                                !fields[2] ||
                                !fields[3] ||
                                !fields[4]))
                        return;

                event_str = g_strjoin (" ",
                                       fields[0],
                                       fields[1],
                                       fields[2],
                                       fields[3],
                                       fields[4], NULL);
                g_free (fields[0]);
                g_free (fields[1]);
                g_free (fields[2]);
                g_free (fields[3]);
                g_free (fields[4]);

                gtk_clipboard_set_text (clipboard, event_str, -1);

                g_free (event_str);
        }
}

G_MODULE_EXPORT
void
on_copy_all_events_activate (GtkMenuItem *menuitem,
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
                char *fields[5];
                char *event_str;
                char *tmp;

                gtk_tree_model_get (model, &iter,
                                    0, &fields[0],
                                    1, &fields[1],
                                    2, &fields[2],
                                    3, &fields[3],
                                    5, &fields[4], -1);
                if (G_UNLIKELY (!fields[0] ||
                                !fields[1] ||
                                !fields[2] ||
                                !fields[3] ||
                                !fields[4]))
                        continue;

                event_str = g_strjoin (" ",
                                       fields[0],
                                       fields[1],
                                       fields[2],
                                       fields[3],
                                       fields[4], NULL);
                g_free (fields[0]);
                g_free (fields[1]);
                g_free (fields[2]);
                g_free (fields[3]);
                g_free (fields[4]);

                tmp = copied;
                copied = g_strjoin ("\n", copied, event_str, NULL);

                g_free (tmp);
                g_free (event_str);
        } while (gtk_tree_model_iter_next (model, &iter));

        gtk_clipboard_set_text (clipboard, copied, -1);

        g_free (copied);
}

static char *
get_actual_value (const char *value)
{
        char         *actual_value;
        guint         size;
        guint         i;

        size = strlen (value);
        actual_value = g_malloc (size + 1);
        /* Replace newlines with spaces in the actual value */
        for (i = 0; i <= size; i++) {
                if (value[i] == '\n')
                        actual_value[i] = ' ';
                else
                        actual_value[i] = value[i];
        }

        return actual_value;
}

static char *
get_display_value (const char *value)
{
        char         *display_value;
        size_t        size;

        /* Don't let the value displayed in treeview, grow indefinitely */
        size = strlen (value);
        if (size > MAX_VALUE_SIZE)
                size = MAX_VALUE_SIZE;

        display_value = g_memdup2 (value, size + 1);
        display_value[size] = '\0';

        return display_value;
}

void
display_event (const char *notified_at,
               const char *friendly_name,
               const char *service_id,
               const char *variable_name,
               const char *value)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GtkTreePath  *path;
        char         *actual_value;
        char         *display_value;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

        actual_value = get_actual_value (value);
        display_value = get_display_value (actual_value);

        gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            0, notified_at,
                            1, friendly_name,
                            2, service_id,
                            3, variable_name,
                            4, display_value,
                            5, actual_value,
                            -1);
        g_free (display_value);
        g_free (actual_value);

        path = gtk_tree_model_get_path (model, &iter);
        if (G_UNLIKELY (path == NULL))
                return;

        /* Don't let it scroll-down */
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (treeview),
                                      path,
                                      NULL,
                                      FALSE,
                                      0.0,
                                      0.0);

        gtk_tree_path_free (path);
}

static void
clear_event_treeview (void)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      more;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        more = gtk_tree_model_get_iter_first (model, &iter);

        while (more)
                more = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

G_MODULE_EXPORT
void
on_clear_event_log_activate (GtkMenuItem *menuitem,
                             gpointer     user_data)
{
        clear_event_treeview ();
}

static GtkTreeModel *
create_event_treemodel (void)
{
        GtkListStore *store;

        store = gtk_list_store_new (6,
                                    G_TYPE_STRING,  /* Time                  */
                                    G_TYPE_STRING,  /* Source Device         */
                                    G_TYPE_STRING,  /* Source Service        */
                                    G_TYPE_STRING,  /* State Variable (name) */
                                    G_TYPE_STRING,  /* Displayed Value       */
                                    G_TYPE_STRING); /* Actual Value          */

        return GTK_TREE_MODEL (store);
}

void
setup_event_treeview (GtkBuilder *builder)
{
        GtkTreeModel *model;
        char         *headers[6] = {_("Time"),
                                   _("Device"),
                                   _("Service"),
                                   _("State Variable"),
                                   _("Value"),
                                   NULL };

        treeview = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "event-treeview"));
        g_assert (treeview != NULL);
        copy_event_menuitem = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "copy-event-menuitem"));
        g_assert (copy_event_menuitem != NULL);
        popup = GTK_WIDGET (gtk_builder_get_object (builder, "event-popup"));
        g_assert (popup != NULL);

        scrolled_window = GTK_WIDGET (gtk_builder_get_object (
                                                builder,
                                                "event-scrolledwindow"));
        g_assert (scrolled_window != NULL);

        model = create_event_treemodel ();
        g_assert (model != NULL);

        setup_treeview (treeview, model, headers, 0);
        g_object_unref (model);

        g_object_set (treeview, "has-tooltip", TRUE, NULL);
        g_signal_connect (treeview,
                          "query-tooltip",
                          G_CALLBACK (on_query_tooltip),
                          NULL);
}

G_MODULE_EXPORT
void
on_event_log_activate (GtkCheckMenuItem *menuitem,
                       gpointer          user_data)
{
        g_object_set (scrolled_window,
                      "visible",
                      gtk_check_menu_item_get_active (menuitem),
                      NULL);
}

G_MODULE_EXPORT
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

                g_object_unref (proxy);
        }
}


