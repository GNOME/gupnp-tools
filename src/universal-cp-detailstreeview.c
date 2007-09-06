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

#include "universal-cp-detailstreeview.h"
#include "universal-cp-gui.h"

static gboolean
get_selected_row (GtkTreeIter *iter)
{
        GtkWidget         *treeview;
        GtkTreeModel      *model;
        GtkTreeSelection  *selection;

        treeview = glade_xml_get_widget (glade_xml, "details-treeview");
        g_assert (treeview != NULL);
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        g_assert (model != NULL);
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        g_assert (selection != NULL);

        return gtk_tree_selection_get_selected (selection, &model, iter);
}

static void
setup_details_popup (GtkWidget *popup)
{
        GtkWidget *copy_value_menuitem;

        copy_value_menuitem = glade_xml_get_widget (glade_xml,
                                                    "copy-value-menuitem");
        g_assert (copy_value_menuitem != NULL);

        /* Only show "Copy Value" menuitem when a row is selected */
        g_object_set (copy_value_menuitem,
                      "visible",
                      get_selected_row (NULL),
                      NULL);
}

gboolean
on_details_treeview_button_release (GtkWidget      *widget,
                                    GdkEventButton *event,
                                    gpointer        user_data)
{
        GtkWidget *popup;

        if (event->type != GDK_BUTTON_RELEASE ||
            event->button != 3)
                return FALSE;

        popup = glade_xml_get_widget (glade_xml, "details-popup");
        g_assert (popup != NULL);

        setup_details_popup (popup);

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
on_details_treeview_row_activate (GtkMenuItem *menuitem,
                                  gpointer     user_data)
{
        GtkWidget    *treeview;
        GtkTreeModel *model;
        GtkTreeIter   iter;

        treeview = glade_xml_get_widget (glade_xml, "details-treeview");
        g_assert (treeview != NULL);
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

void
on_copy_all_details_activate (GtkMenuItem *menuitem,
                              gpointer     user_data)
{
        GtkClipboard *clipboard;
        GtkWidget    *treeview;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        char         *copied;

        treeview = glade_xml_get_widget (glade_xml, "details-treeview");
        g_assert (treeview != NULL);
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

GtkTreeModel *
create_details_treemodel (void)
{
        GtkListStore *store;

        store = gtk_list_store_new (2,
                                    G_TYPE_STRING,  /* Name */
                                    G_TYPE_STRING); /* Value */

        return GTK_TREE_MODEL (store);
}

