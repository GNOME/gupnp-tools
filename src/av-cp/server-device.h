/*
 * Copyright (C) 2016 Jens Georg <mail@jensge.org>
 *
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

#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgupnp/gupnp.h>

G_BEGIN_DECLS

GType
av_cp_media_server_get_type (void);

#define AV_CP_TYPE_MEDIA_SERVER (av_cp_media_server_get_type ())
#define AV_CP_MEDIA_SERVER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 AV_CP_TYPE_MEDIA_SERVER, \
                 AVCPMediaServer))
#define AV_CP_MEDIA_SERVER_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), \
            AV_CP_TYPE_MEDIA_SERVER, \
            AVCPMediaServerClass))
#define AV_CP_IS_MEDIA_SERVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            AV_CP_TYPE_MEDIA_SERVER))
#define AV_CP_IS_MEDIA_SERVER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
            AV_CP_TYPE_MEDIA_SERVER))
#define AV_CP_MEDIA_SERVER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
            AV_CP_TYPE_MEDIA_SERVER, \
            AVCPMediaServerDeviceClass))

typedef struct _AVCPMediaServer AVCPMediaServer;
typedef struct _AVCPMediaServerClass AVCPMediaServerClass;
typedef struct _AVCPMediaServerPrivate AVCPMediaServerPrivate;

struct _AVCPMediaServer {
        GUPnPDeviceProxy parent;
};

struct _AVCPMediaServerClass {
        GUPnPDeviceProxyClass parent_class;
};


GUPnPServiceProxy *
av_cp_media_server_get_content_directory (AVCPMediaServer *self);

void
av_cp_media_server_browse_async (AVCPMediaServer     *self,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 const char          *container_id,
                                 guint32              starting_index,
                                 guint32              requested_count,
                                 gpointer             user_data);

gboolean
av_cp_media_server_browse_finish (AVCPMediaServer  *self,
                                  GAsyncResult     *result,
                                  char            **didl_xml,
                                  guint32          *total_matches,
                                  guint32          *number_returned,
                                  GError          **error);

void
av_cp_media_server_browse_metadata_async (AVCPMediaServer     *self,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          const char          *container_id,
                                          gpointer             user_data);

gboolean
av_cp_media_server_browse_metadata_finish (AVCPMediaServer  *self,
                                           GAsyncResult     *result,
                                           char            **didl_xml,
                                           GError          **error);

void
av_cp_media_server_search_async (AVCPMediaServer     *self,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 const char          *container_id,
                                 const char          *search_criteria,
                                 const char          *filter,
                                 guint32              starting_index,
                                 guint32              requested_count,
                                 gpointer             user_data);
gboolean
av_cp_media_server_search_finish (AVCPMediaServer  *self,
                                  GAsyncResult     *result,
                                  char            **didl_xml,
                                  guint32          *total_matches,
                                  guint32          *number_returned,
                                  GError          **error);

char const * const *
av_cp_media_server_get_search_caps (AVCPMediaServer *self);

G_END_DECLS

#endif /* MEDIA_SERVER_H */
