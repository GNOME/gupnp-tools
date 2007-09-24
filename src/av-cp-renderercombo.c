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

#include <libgupnp/gupnp-control-point.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>

#include "av-cp-renderercombo.h"

static GtkWidget *renderer_combo;

void
add_media_renderer (GUPnPDeviceProxy *renderer)
{
        GUPnPDeviceInfo *info;
        GtkTreeModel    *model;
        char            *name;

        info = GUPNP_DEVICE_INFO (renderer);

        name = gupnp_device_info_get_friendly_name (info);
        if (name == NULL)
                name = g_strdup (gupnp_device_info_get_udn (info));
        if (name == NULL)
                return;

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (renderer_combo));
        gtk_list_store_insert_with_values (GTK_LIST_STORE (model),
                                           NULL,
                                           -1,
                                           0, name,
                                           1, renderer,
                                           -1);
        g_free (name);
}

void
remove_media_renderer (GUPnPDeviceProxy *renderer)
{
}

static GtkTreeModel *
create_renderer_treemodel (void)
{
        GtkListStore *store;

        store = gtk_list_store_new (2,
                                    G_TYPE_STRING,   /* Name           */
                                    G_TYPE_OBJECT);  /* renderer proxy */

        return GTK_TREE_MODEL (store);
}

static void
setup_renderer_combo_text_cell (GtkWidget *renderer_combo)
{
        GtkCellRenderer *renderer;

        renderer = gtk_cell_renderer_text_new ();

        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (renderer_combo),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (renderer_combo),
                                       renderer,
                                       "text", 0);
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

        setup_renderer_combo_text_cell (renderer_combo);
}

