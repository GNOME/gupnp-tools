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
#include <libsoup/soup.h>

#include "universal-cp-icons.h"
#include "universal-cp-devicetreeview.h"

#define PREFERED_DEPTH  32
#define PREFERED_WIDTH  22
#define PREFERED_HEIGHT 22

GdkPixbuf *icons[ICON_LAST];

/* For async downloads of icons */
static SoupSession *session;
static GList       *pending_gets;

typedef struct {
        GUPnPDeviceInfo *info;
        SoupMessage     *message;
        gchar           *mime_type;
        gint             width;
        gint             height;
} GetIconURLData;

static void
get_icon_url_data_free (GetIconURLData *data)
{
        pending_gets = g_list_remove (pending_gets, data);

        g_free (data->mime_type);
        g_slice_free (GetIconURLData, data);
}

static GdkPixbuf *
get_icon_from_message (SoupMessage    *msg,
                       GetIconURLData *data,
                       GError        **error)
{
        GdkPixbufLoader *loader;
        GdkPixbuf       *pixbuf;

        loader = gdk_pixbuf_loader_new_with_mime_type (data->mime_type, error);
        if (loader == NULL)
                return NULL;

        gdk_pixbuf_loader_write (loader,
                                 (guchar *) msg->response.body,
                                 msg->response.length,
                                 error);
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        if (pixbuf) {
                gfloat aspect_ratio;
                gint   height;

                /* Preserve the aspect-ratio of the original image */
                aspect_ratio = (gfloat) data->width / data->height;
                height = (gint) (PREFERED_WIDTH / aspect_ratio);
                pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                  PREFERED_WIDTH,
                                                  height,
                                                  GDK_INTERP_NEAREST);
        }

        gdk_pixbuf_loader_close (loader, NULL);

        return pixbuf;
}

/**
 * Icon downloaded.
 **/
static void
got_icon_url (SoupMessage    *msg,
              GetIconURLData *data)
{
        if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
                GdkPixbuf *pixbuf;
                GError    *error;

                error = NULL;
                pixbuf = get_icon_from_message (msg, data, &error);

                if (error) {
                        g_warning ("Failed to create icon for '%s': %s",
                                   gupnp_device_info_get_udn (data->info),
                                   error->message);
                        g_error_free (error);
                } else if (pixbuf) {
                        update_device_icon (data->info, pixbuf);
                } else {
                        g_warning ("Failed to create icon for '%s'",
                                   gupnp_device_info_get_udn (data->info));
                }
        }

        get_icon_url_data_free (data);
}

void
schedule_icon_update (GUPnPDeviceInfo *info)
{
        GetIconURLData *data;
        char           *icon_url;

        data = g_slice_new (GetIconURLData);

        icon_url = gupnp_device_info_get_icon_url
                        (info,
                         NULL,
                         PREFERED_DEPTH,
                         PREFERED_WIDTH,
                         PREFERED_HEIGHT,
                         TRUE,
                         &data->mime_type,
                         NULL,
                         &data->width,
                         &data->height);
        if (icon_url == NULL)
                return;

        data->message = soup_message_new (SOUP_METHOD_GET, icon_url);

        if (data->message == NULL) {
                g_warning ("Invalid URL icon for device '%s': %s",
                           gupnp_device_info_get_udn (info),
                           icon_url);

                g_free (icon_url);
                g_free (data->mime_type);
                g_slice_free (GetIconURLData, data);

                return;
        }

        data->info = info;

        soup_session_queue_message (session,
                                    data->message,
                                    (SoupMessageCallbackFn) got_icon_url,
                                    data);
        pending_gets = g_list_prepend (pending_gets, data);

        g_free (icon_url);
}

void
unschedule_icon_update (GUPnPDeviceInfo *info)
{
        while (pending_gets) {
                GetIconURLData *data;
                const char *udn1;
                const char *udn2;

                data = pending_gets->data;
                udn1 = gupnp_device_info_get_udn (info);
                udn2 = gupnp_device_info_get_udn (data->info);

                if (udn1 && udn2 && strcmp (udn1, udn2) == 0) {
                        soup_message_set_status (data->message,
                                                 SOUP_STATUS_CANCELLED);
                        soup_session_cancel_message (session,
                                                     data->message);

                        get_icon_url_data_free (data);
                }
        }
}

void
init_icons (GladeXML *glade_xml)
{
        GtkWidget *image;
        int        i;
        char      *file_names[] = {
                "pixmaps/devices.png",         /* ICON_DEVICES    */
                "pixmaps/device.png",          /* ICON_DEVICE     */
                "pixmaps/service.png",         /* ICON_SERVICE    */
                "pixmaps/state-variables.png", /* ICON_VARIABLES  */
                "pixmaps/state-variable.png",  /* ICON_VARIABLE   */
                "pixmaps/action.png",          /* ICON_ACTION     */
                "pixmaps/state-variable.png",  /* ICON_ACTION_ARG */
        };

        for (i = 0; i < ICON_LAST; i++) {
                /* Try to fetch the pixmap from the CWD first */
                icons[i] = gdk_pixbuf_new_from_file (file_names[i], NULL);
                if (icons[i] == NULL) {
                        char *pixmap_path;

                        /* Then Try to fetch it from the system path */
                        pixmap_path = g_strjoin ("/",
                                                 DATA_DIR,
                                                 file_names[i],
                                                 NULL);

                        icons[i] = gdk_pixbuf_new_from_file (pixmap_path, NULL);
                        g_free (pixmap_path);
                }

                g_assert (icons[i] != NULL);
        }

        image = glade_xml_get_widget (glade_xml, "device-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   icons[ICON_DEVICE]);

        image = glade_xml_get_widget (glade_xml, "service-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   icons[ICON_SERVICE]);

        image = glade_xml_get_widget (glade_xml, "action-image");
        g_assert (image != NULL);
        gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                                   icons[ICON_ACTION]);

        session = soup_session_async_new ();
        g_assert (session != NULL);

        pending_gets = NULL;
}

void
deinit_icons (void)
{
        int i;

        while (pending_gets) {
                GetIconURLData *data;

                data = pending_gets->data;

                soup_message_set_status (data->message,
                                         SOUP_STATUS_CANCELLED);
                soup_session_cancel_message (session, data->message);

                get_icon_url_data_free (data);
        }

        g_object_unref (session);

        for (i = 0; i < ICON_LAST; i++) {
                g_object_unref (icons[i]);
        }
}

