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

#include "renderercombo.h"
#include "icons.h"

static GtkWidget *renderer_combo;

static gboolean
find_renderer (GtkTreeModel *model,
               const char   *udn,
               GtkTreeIter  *iter)
{
        gboolean found = FALSE;
        gboolean more = TRUE;

        g_assert (udn != NULL);
        g_assert (iter != NULL);

        more = gtk_tree_model_get_iter_first (model, iter);

        while (more) {
                GUPnPDeviceInfo *info;

                gtk_tree_model_get (model,
                                    iter,
                                    2, &info,
                                    -1);

                if (info) {
                        const char *device_udn;

                        device_udn = gupnp_device_info_get_udn (info);

                        if (strcmp (device_udn, udn) == 0)
                                found = TRUE;

                        g_object_unref (info);
                }

                if (found)
                        break;

                more = gtk_tree_model_iter_next (model, iter);
        }

        return found;
}

void
update_device_icon (GUPnPDeviceInfo *info,
                    GdkPixbuf       *icon)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        const char   *udn;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (renderer_combo));
        g_assert (model != NULL);

        udn = gupnp_device_info_get_udn (info);

        if (find_renderer (model, udn, &iter)) {
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    0, icon,
                                    -1);
        }

        g_object_unref (icon);
}

void
add_media_renderer (GUPnPMediaRendererProxy *proxy)
{
        GUPnPDeviceInfo *info;
        GtkComboBox     *combo;
        GtkTreeModel    *model;
        GtkTreeIter      iter;
        const char      *udn;
        char            *name;

        info = GUPNP_DEVICE_INFO (proxy);
        combo = GTK_COMBO_BOX (renderer_combo);

        udn = gupnp_device_info_get_udn (info);
        if (udn == NULL)
                return;

        name = gupnp_device_info_get_friendly_name (info);
        if (name == NULL)
                name = g_strdup (udn);

        model = gtk_combo_box_get_model (combo);

        if (!find_renderer (model, udn, &iter)) {
                gboolean was_empty;

                if (gtk_combo_box_get_active (combo) == -1)
                        was_empty = TRUE;
                else
                        was_empty = FALSE;

                memset (&iter, 0, sizeof (iter));
                gtk_list_store_insert_with_values
                                (GTK_LIST_STORE (model),
                                 &iter,
                                 -1,
                                 0, get_icon_by_id (ICON_DEVICE),
                                 1, name,
                                 2, proxy,
                                 -1);

                schedule_icon_update (info);

                if (was_empty)
                        gtk_combo_box_set_active_iter (combo, &iter);
        }

        g_free (name);
}

void
remove_media_renderer (GUPnPMediaRendererProxy *proxy)
{
        GUPnPDeviceInfo *info;
        GtkComboBox     *combo;
        GtkTreeModel    *model;
        GtkTreeIter      iter;
        const char      *udn;

        info = GUPNP_DEVICE_INFO (proxy);
        combo = GTK_COMBO_BOX (renderer_combo);

        udn = gupnp_device_info_get_udn (info);
        if (udn == NULL)
                return;

        model = gtk_combo_box_get_model (combo);

        if (find_renderer (model, udn, &iter)) {
                unschedule_icon_update (info);
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
                gtk_combo_box_set_active (combo, 0);
        }
}

static GtkTreeModel *
create_renderer_treemodel (void)
{
        GtkListStore *store;

        store = gtk_list_store_new (3,
                                    GDK_TYPE_PIXBUF, /* Icon           */
                                    G_TYPE_STRING,   /* Name           */
                                    G_TYPE_OBJECT);  /* renderer proxy */

        return GTK_TREE_MODEL (store);
}

static void
setup_renderer_combo_text_cell (GtkWidget *renderer_combo)
{
        GtkCellRenderer *renderer;

        renderer = gtk_cell_renderer_text_new ();

        g_object_set (renderer,
                      "xalign", 0.0,
                      "xpad", 6,
                      NULL);
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (renderer_combo),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (renderer_combo),
                                       renderer,
                                       "text", 1);
}

static void
setup_renderer_combo_pixbuf_cell (GtkWidget *renderer_combo)
{
        GtkCellRenderer *renderer;

        renderer = gtk_cell_renderer_pixbuf_new ();
        g_object_set (renderer, "xalign", 0.0, NULL);

        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (renderer_combo),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (renderer_combo),
                                       renderer,
                                       "pixbuf", 0);
}

void
setup_renderer_combo (GladeXML *glade_xml)
{
        GtkTreeModel *model;

        renderer_combo = glade_xml_get_widget (glade_xml, "renderer-combobox");
        g_assert (renderer_combo != NULL);

        model = create_renderer_treemodel ();
        g_assert (model != NULL);

        gtk_combo_box_set_model (GTK_COMBO_BOX (renderer_combo), model);
        g_object_unref (model);

        setup_renderer_combo_pixbuf_cell (renderer_combo);
        setup_renderer_combo_text_cell (renderer_combo);
}

