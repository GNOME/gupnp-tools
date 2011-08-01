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

#include <libgupnp/gupnp.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"

typedef struct
{
        GUPnPServiceProxy *cds_proxy;
        guint              transfer_id;

        guint timeout_id;

        GUPnPServiceProxyAction *action;
} TrackTransferData;

static void
get_transfer_progress_cb (GUPnPServiceProxy       *cds_proxy,
                          GUPnPServiceProxyAction *action,
                          gpointer                 user_data)
{
        GError *error;
        TrackTransferData *data;
        guint64 total, length;
        gchar *status;

        data = (TrackTransferData *) user_data;

        error = NULL;
        total = length = 0;
        status = NULL;
        if (!gupnp_service_proxy_end_action (cds_proxy,
                                             action,
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

        data->action = gupnp_service_proxy_begin_action (
                                        data->cds_proxy,
                                        "GetTransferProgress",
                                        get_transfer_progress_cb,
                                        data,
                                        "TransferID",
                                                G_TYPE_UINT,
                                                data->transfer_id,
                                        NULL);

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
import_resource_cb (GUPnPServiceProxy       *cds_proxy,
                    GUPnPServiceProxyAction *action,
                    gpointer                 user_data)
{
        GError *error;
        guint   transfer_id;
        char   *file_name;

        file_name = (gchar *) user_data;

        error = NULL;
        if (!gupnp_service_proxy_end_action (cds_proxy,
                                             action,
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
        start_tracking_transfer (cds_proxy, transfer_id);

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

        if (!g_path_is_absolute (file_path)) {
                g_critical ("Given file path '%s' is not absolute.", file_path);

                transfer_completed ();

                return;
        }

        /* Time to host the file */
        gupnp_context_host_path (context, file_path, file_path);

        source_uri = g_strdup_printf ("http://%s:%u%s",
                                      gupnp_context_get_host_ip (context),
                                      gupnp_context_get_port (context),
                                      file_path);

        file_name = g_path_get_basename (file_path);
        gupnp_service_proxy_begin_action (cds_proxy,
                                          "ImportResource",
                                          import_resource_cb,
                                          file_name,
                                          "SourceURI",
                                                G_TYPE_STRING,
                                                source_uri,
                                          "DestinationURI",
                                                G_TYPE_STRING,
                                                dest_uri,
                                          NULL);

        g_free (source_uri);
}

