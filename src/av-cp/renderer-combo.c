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

#include "renderer-combo.h"
#include "icons.h"

#define CONNECTION_MANAGER_V1 \
                        "urn:schemas-upnp-org:service:ConnectionManager:1"
#define CONNECTION_MANAGER_V2 \
                        "urn:schemas-upnp-org:service:ConnectionManager:2"
#define AV_TRANSPORT_V1 "urn:schemas-upnp-org:service:AVTransport:1"
#define AV_TRANSPORT_V2 "urn:schemas-upnp-org:service:AVTransport:2"

static GtkWidget *renderer_combo;

static GUPnPServiceProxy *
get_av_transport (GUPnPMediaRendererProxy *renderer)
{
        GUPnPServiceInfo *av_transport;

        av_transport = gupnp_device_info_get_service
                                        (GUPNP_DEVICE_INFO (renderer),
                                         AV_TRANSPORT_V1);
        if (av_transport == NULL) {
                av_transport = gupnp_device_info_get_service
                                                (GUPNP_DEVICE_INFO (renderer),
                                                 AV_TRANSPORT_V2);
        }

        return GUPNP_SERVICE_PROXY (av_transport);
}

GUPnPServiceProxy *
get_selected_av_transport (gchar ***protocols)
{
        GUPnPServiceProxy *av_transport;
        GtkComboBox       *combo;
        GtkTreeModel      *model;
        GtkTreeIter        iter;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return NULL;
        }

        if (protocols != NULL) {
                gtk_tree_model_get (model,
                                    &iter,
                                    4, protocols,
                                    -1);

                if (*protocols == NULL) {
                        return NULL;
                }
        }

        gtk_tree_model_get (model,
                            &iter,
                            3, &av_transport,
                            -1);

        return av_transport;
}

/* FIXME: implement this function
 */
PlaybackState
get_selected_renderer_state (void)
{
        PlaybackState state;
        GtkComboBox  *combo;
        GtkTreeModel *model;
        GtkTreeIter   iter;

        combo = GTK_COMBO_BOX (renderer_combo);
        model = gtk_combo_box_get_model (combo);
        g_assert (model != NULL);

        if (!gtk_combo_box_get_active_iter (combo, &iter)) {
                return PLAYBACK_STATE_UNKNOWN;
        }

        gtk_tree_model_get (model,
                            &iter,
                            5, &state,
                            -1);

        return state;
}

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

static PlaybackState
state_name_to_state (const char *state_name)
{
        PlaybackState state;

        if (strcmp ("STOPPED", state_name) == 0) {
                state = PLAYBACK_STATE_STOPPED;
        } else if (strcmp ("PLAYING", state_name) == 0) {
                state = PLAYBACK_STATE_PLAYING;
        } else if (strcmp ("PAUSED_PLAYBACK", state_name) == 0) {
                state = PLAYBACK_STATE_PAUSED;
        } else {
                state = PLAYBACK_STATE_UNKNOWN;
        }

        return state;
}

static void
set_state_by_name (const gchar *udn,
                   const gchar *state_name)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_combo_box_get_model
                (GTK_COMBO_BOX (renderer_combo));
        g_assert (model != NULL);

        if (find_renderer (model, udn, &iter)) {
                PlaybackState state;

                state = state_name_to_state (state_name),

                      gtk_list_store_set (GTK_LIST_STORE (model),
                                      &iter,
                                      5, state,
                                      -1);
        }
}

static void
on_device_icon_available (GUPnPDeviceInfo *info,
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
append_media_renderer_to_tree (GUPnPMediaRendererProxy *proxy,
                               GUPnPServiceProxy       *av_transport,
                               const char              *udn)
{
        GUPnPDeviceInfo  *info;
        GtkComboBox      *combo;
        GtkTreeModel     *model;
        GtkTreeIter       iter;
        char             *name;
        gboolean          was_empty;

        info = GUPNP_DEVICE_INFO (proxy);
        combo = GTK_COMBO_BOX (renderer_combo);

        name = gupnp_device_info_get_friendly_name (info);
        if (name == NULL)
                name = g_strdup (udn);

        model = gtk_combo_box_get_model (combo);

        if (gtk_combo_box_get_active (combo) == -1)
                was_empty = TRUE;
        else
                was_empty = FALSE;

        memset (&iter, 0, sizeof (iter));
        gtk_list_store_insert_with_values
                (GTK_LIST_STORE (model),
                 &iter,
                 -1,
                 0, get_icon_by_id (ICON_MEDIA_RENDERER),
                 1, name,
                 2, proxy,
                 3, av_transport,
                 5, PLAYBACK_STATE_UNKNOWN,
                 -1);

        schedule_icon_update (info, on_device_icon_available);

        if (was_empty)
                gtk_combo_box_set_active_iter (combo, &iter);

        g_free (name);
}

static GUPnPServiceProxy *
get_connection_manager (GUPnPMediaRendererProxy *proxy)
{
        GUPnPServiceInfo *cm;

        cm = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (proxy),
                                            CONNECTION_MANAGER_V1);
        if (cm == NULL) {
                cm = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (proxy),
                                                    CONNECTION_MANAGER_V2);
        }

        return GUPNP_SERVICE_PROXY (cm);
}

static void
get_protocol_info_cb (GUPnPServiceProxy       *cm,
                      GUPnPServiceProxyAction *action,
                      gpointer                 user_data)
{
        gchar  *sink_protocols;
        gchar **protocols;
        gchar  *udn;
        GError *error;

        udn = (gchar *) user_data;

        error = NULL;
        if (!gupnp_service_proxy_end_action (cm,
                                             action,
                                             &error,
                                             "Sink",
                                             G_TYPE_STRING,
                                             &sink_protocols,
                                             NULL)) {
                g_warning ("Failed to get sink protocol info from "
                           "media renderer '%s':%s\n",
                           udn,
                           error->message);
                g_error_free (error);

                goto return_point;
        }

        protocols = g_strsplit (sink_protocols,
                                ",",
                                0);

        if (protocols) {
                GtkTreeModel *model;
                GtkTreeIter   iter;

                model = gtk_combo_box_get_model
                                        (GTK_COMBO_BOX (renderer_combo));
                g_assert (model != NULL);

                if (find_renderer (model, udn, &iter)) {
                        gtk_list_store_set (GTK_LIST_STORE (model),
                                            &iter,
                                            4, protocols,
                                            -1);
                }

                g_strfreev (protocols);
        }

return_point:
        g_object_unref (cm);
        g_free (udn);
}

static void
get_transport_info_cb (GUPnPServiceProxy       *av_transport,
                      GUPnPServiceProxyAction *action,
                      gpointer                 user_data)
{
        gchar  *state_name;
        gchar  *udn;
        GError *error;

        udn = (gchar *) user_data;

        error = NULL;
        if (!gupnp_service_proxy_end_action (av_transport,
                                             action,
                                             &error,
                                             "CurrentTransportState",
                                             G_TYPE_STRING,
                                             &state_name,
                                             NULL)) {
                g_warning ("Failed to get transport info from media renderer"
                           " '%s':%s\n",
                           udn,
                           error->message);
                g_error_free (error);

                goto return_point;
        }

        if (state_name) {
                set_state_by_name (udn, state_name);

                g_free (state_name);
        }

return_point:
        g_object_unref (av_transport);
        g_free (udn);
}

void
add_media_renderer (GUPnPMediaRendererProxy *proxy)
{
        GtkTreeModel      *model;
        GtkTreeIter        iter;
        char              *udn;
        GUPnPServiceProxy *cm;
        GUPnPServiceProxy *av_transport;
        GError            *error;

        udn = (char *) gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));
        if (udn == NULL)
                return;

        cm = get_connection_manager (proxy);
        if (G_UNLIKELY (cm == NULL))
                return;

        av_transport = get_av_transport (proxy);
        if (av_transport == NULL) {
                g_object_unref (cm);

                return;
        }

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (renderer_combo));

        if (!find_renderer (model, udn, &iter))
                append_media_renderer_to_tree (proxy, av_transport, udn);

        udn = g_strdup (udn);

        error = NULL;
        gupnp_service_proxy_begin_action (cm,
                                          "GetProtocolInfo",
                                          get_protocol_info_cb,
                                          udn,
                                          &error,
                                          NULL);
        if (error) {
                g_warning ("Failed to get sink protocol info from "
                           "media renderer '%s':%s\n",
                           udn,
                           error->message);

                g_error_free (error);
                g_object_unref (cm);
                g_free (udn);
        }

        udn = g_strdup (udn);

        error = NULL;
        gupnp_service_proxy_begin_action (av_transport,
                                          "GetTransportInfo",
                                          get_transport_info_cb,
                                          udn,
                                          &error,
                                          "InstanceID", G_TYPE_UINT, 0,
                                          NULL);
        if (error) {
                g_warning ("Failed to get transport info from media renderer"
                           " '%s':%s\n",
                           udn,
                           error->message);

                g_error_free (error);
                g_object_unref (av_transport);
                g_free (udn);
        }
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

        store = gtk_list_store_new (6,
                                    GDK_TYPE_PIXBUF, /* Icon             */
                                    G_TYPE_STRING,   /* Name             */
                                    G_TYPE_OBJECT,   /* renderer proxy   */
                                    G_TYPE_OBJECT,   /* AVTranport proxy */
                                    G_TYPE_STRV,     /* ProtocolInfo     */
                                    G_TYPE_UINT);    /* AVTranport state */

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

