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

#include <libgupnp/gupnp.h>
#include <string.h>
#include <stdlib.h>

#include "transfer.h"
#include "main.h"

typedef struct
{
        GUPnPServiceProxy *cds_proxy;
        guint              transfer_id;

        guint timeout_id;

        GUPnPServiceProxyAction *action;
} TrackTransferData;

static void
get_transfer_progress_cb (GObject *object,
                          GAsyncResult *result,
                          gpointer user_data)
{
        g_autoptr (GError) error = NULL;
        TrackTransferData *data;
        guint64 total = 0, length = 0;
        g_autofree gchar *status = NULL;
        GUPnPServiceProxyAction *action;

        data = (TrackTransferData *) user_data;

        total = length = 0;
        status = NULL;
        action = gupnp_service_proxy_call_action_finish (
                GUPNP_SERVICE_PROXY (object),
                result,
                &error);
        if (error == NULL) {
                gupnp_service_proxy_action_get_result (action, &error, NULL);
        }

        if (error != NULL) {
                g_critical ("Failed to track file transfer: %s",
                            error->message);
                g_clear_error (&error);

                transfer_completed ();

                return;
        }

        if (!gupnp_service_proxy_action_get_result (data->action,
                                                    &error,
                                                    "TransferStatus",
                                                    G_TYPE_STRING,
                                                    &status,
                                                    "TransferLength",
                                                    G_TYPE_UINT64,
                                                    &length,
                                                    "TransferTotal",
                                                    G_TYPE_UINT64,
                                                    &total,
                                                    NULL)) {
                g_critical ("Failed to track file transfer: %s",
                            error->message);

                g_error_free (error);

                transfer_completed ();

                return;
        }

        if (g_strcmp0 (status, "IN_PROGRESS") == 0) {
                if (total > 0) {
                        guint64 progress = length * 100 / total;

                        g_print ("\b\b\b%2" G_GUINT64_FORMAT "%%", progress);
                }

                data->action = NULL;

                g_free (status);
        } else {
                g_print ("\b\b\bDone.\n");

                g_free (status);
                g_source_remove (data->timeout_id);
                g_slice_free (TrackTransferData, data);

                transfer_completed ();
        }
}

static gboolean
track_transfer (gpointer user_data)
{
        TrackTransferData *data = (TrackTransferData *) user_data;

        if (data->action != NULL) {
                /* Previous action call still in progress */
                return TRUE;
        }

        data->action = gupnp_service_proxy_action_new ("GetTransferProgress",
                                                       "TransferID",
                                                       G_TYPE_UINT,
                                                       data->transfer_id,
                                                       NULL);

        gupnp_service_proxy_call_action_async (data->cds_proxy,
                                               data->action,
                                               NULL,
                                               get_transfer_progress_cb,
                                               data);

        gupnp_service_proxy_action_unref (data->action);

        return TRUE;
}

static void
start_tracking_transfer (GUPnPServiceProxy *cds_proxy,
                         guint              transfer_id)
{
        TrackTransferData *data;

        data = g_slice_new0 (TrackTransferData);
        data->cds_proxy = cds_proxy;
        data->transfer_id = transfer_id;

        /* Check progress every second */
        data->timeout_id = g_timeout_add_seconds (1,
                                                  track_transfer,
                                                  data);
}

static void
import_resource_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
        GError *error;
        guint   transfer_id;
        char   *file_name;
        GUPnPServiceProxyAction *action;
        GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (object);

        file_name = (gchar *) user_data;

        error = NULL;
        action = gupnp_service_proxy_call_action_finish (proxy, result, &error);
        if (error != NULL) {
                g_critical ("Failed to start file transfer: %s",
                            error->message);

                g_free (file_name);
                g_error_free (error);

                transfer_completed ();

                return;
        }

        if (!gupnp_service_proxy_action_get_result (action,
                                                    &error,
                                                    "TransferID",
                                                    G_TYPE_UINT,
                                                    &transfer_id,
                                                    NULL)) {
                g_critical ("Failed to start file transfer: %s",
                            error->message);

                g_free (file_name);
                g_error_free (error);

                transfer_completed ();

                return;
        }

        g_print ("Uploading %s: 00%%", file_name);
        start_tracking_transfer (GUPNP_SERVICE_PROXY (object), transfer_id);

        g_free (file_name);
}

void
start_transfer (const char        *file_path,
                const char        *dest_uri,
                GUPnPServiceProxy *cds_proxy,
                GUPnPContext      *context)
{
        char *source_uri;
        char *file_name;
        GUPnPServiceProxyAction *action;

        if (!g_path_is_absolute (file_path)) {
                g_critical ("Given file path '%s' is not absolute.", file_path);

                transfer_completed ();

                return;
        }

        /* Time to host the file */
        gupnp_context_host_path (context, file_path, file_path);

        source_uri = g_strdup_printf ("http://%s:%u%s",
                                      gssdp_client_get_host_ip (GSSDP_CLIENT (context)),
                                      gupnp_context_get_port (context),
                                      file_path);

        file_name = g_path_get_basename (file_path);
        action = gupnp_service_proxy_action_new ("ImportResource",
                                                 "SourceURI",
                                                 G_TYPE_STRING,
                                                 source_uri,
                                                 "DestinationURI",
                                                 G_TYPE_STRING,
                                                 dest_uri,
                                                 NULL);

        gupnp_service_proxy_call_action_async (cds_proxy,
                                               action,
                                               NULL,
                                               import_resource_cb,
                                               file_name);

        gupnp_service_proxy_action_unref (action);

        g_free (source_uri);
}

