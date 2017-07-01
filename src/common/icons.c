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

#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <libsoup/soup.h>

#include "icons.h"

#define PREFERED_DEPTH  32
#define PREFERED_WIDTH  22
#define PREFERED_HEIGHT 22

GdkPixbuf *icons[ICON_LAST];

/* For async downloads of icons */
static SoupSession *download_session;
static GList       *pending_gets;

typedef struct {
        GUPnPDeviceInfo *info;

        DeviceIconAvailableCallback callback;

        SoupMessage     *message;
        gchar           *mime_type;
        gint             width;
        gint             height;
} GetIconURLData;

static void
get_icon_url_data_free (GetIconURLData *data)
{
        g_object_unref (data->info);

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
                                 (guchar *) msg->response_body->data,
                                 msg->response_body->length,
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
                                                  GDK_INTERP_HYPER);
        }

        gdk_pixbuf_loader_close (loader, NULL);
        g_object_unref (loader);

        return pixbuf;
}

/**
 * Icon downloaded.
 **/
static void
got_icon_url (SoupSession    *session,
              SoupMessage    *msg,
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
                        data->callback (data->info, pixbuf);
                } else {
                        g_warning ("Failed to create icon for '%s'",
                                   gupnp_device_info_get_udn (data->info));
                }
        }

        pending_gets = g_list_remove (pending_gets, data);
        get_icon_url_data_free (data);
}

void
schedule_icon_update (GUPnPDeviceInfo            *info,
                      DeviceIconAvailableCallback callback)
{
        GetIconURLData *data;
        char           *icon_url;

        data = g_slice_new0 (GetIconURLData);
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
        if (icon_url == NULL) {
                g_free (data->mime_type);
                g_slice_free (GetIconURLData, data);

                return;
        }

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

        data->info = g_object_ref (info);
        data->callback = callback;

        pending_gets = g_list_prepend (pending_gets, data);
        soup_session_queue_message (download_session,
                                    data->message,
                                    (SoupSessionCallback) got_icon_url,
                                    data);

        g_free (icon_url);
}

void
unschedule_icon_update (GUPnPDeviceInfo *info)
{
        GList *gets;

        for (gets = pending_gets; gets; gets = gets->next) {
                GetIconURLData *data;
                const char *udn1;
                const char *udn2;

                data = gets->data;
                udn1 = gupnp_device_info_get_udn (info);
                udn2 = gupnp_device_info_get_udn (data->info);

                if (udn1 && udn2 && strcmp (udn1, udn2) == 0) {
                        soup_session_cancel_message (download_session,
                                                     data->message,
                                                     SOUP_STATUS_CANCELLED);
                        break;
                }
        }
}

GdkPixbuf *
get_icon_by_id (IconID icon_id)
{
        g_return_val_if_fail (icon_id > ICON_FIRST && icon_id < ICON_LAST, NULL);

        return icons[icon_id];
}

static GdkPixbuf *
get_pixbuf_from_theme (const char *icon_name)
{
        GdkScreen    *screen;
        GtkIconTheme *theme;
        GdkPixbuf    *pixbuf;
        GError       *error;

        screen = gdk_screen_get_default ();
        theme = gtk_icon_theme_get_for_screen (screen);

        error = NULL;
        pixbuf = gtk_icon_theme_load_icon (theme,
                                           icon_name,
                                           PREFERED_WIDTH,
                                           PREFERED_HEIGHT,
                                           &error);
        if (pixbuf == NULL) {
                g_warning ("Failed to load icon %s: %s",
                           icon_name,
                           error->message);
                g_error_free (error);

                error = NULL;
                pixbuf = gtk_icon_theme_load_icon (theme,
                                                   "image-missing",
                                                   PREFERED_WIDTH,
                                                   PREFERED_HEIGHT,
                                                   &error);
                if (pixbuf == NULL) {
                    g_critical ("Failed to load fallback icon: %s",
                                error->message);
                    g_error_free (error);
                }
        }

        return pixbuf;
}

GdkPixbuf *
load_pixbuf_file (const char *file_name)
{
        GdkPixbuf *pixbuf;
        char *path;

        path = g_build_path ("/", "/org/gupnp/Tools/Common", file_name, NULL);
        pixbuf = gdk_pixbuf_new_from_resource (path, NULL);
        if (pixbuf == NULL)
                g_critical ("failed to get image %s\n", path);

        g_free (path);

        return pixbuf;
}

extern void gupnp_tools_common_unregister_resource (void);
extern void gupnp_tools_common_register_resource (void);

void
init_icons (void)
{
        int   i, j;
        const char *file_names[] = {
                "pixmaps/upnp-device.png",          /* ICON_DEVICE         */
                "pixmaps/upnp-service.png",         /* ICON_SERVICE        */
                "pixmaps/upnp-state-variable.png",  /* ICON_VARIABLE       */
                "pixmaps/upnp-action-arg-in.png",   /* ICON_ACTION_ARG_IN  */
                "pixmaps/upnp-action-arg-out.png",  /* ICON_ACTION_ARG_OUT */
                "pixmaps/media-renderer.png"        /* ICON_MEDIA_RENDERER */
        };

        const char *theme_names[] = {
                "image-missing",               /* ICON_MISSING    */
                "network-workgroup",           /* ICON_NETWORK    */
                "system-run",                  /* ICON_ACTION     */
                "folder",                      /* ICON_VARIABLES  */
                "text-x-generic",              /* ICON_FILE       */
                "folder-remote",               /* ICON_CONTAINER  */
                "audio-x-generic",             /* ICON_AUDIO_ITEM */
                "video-x-generic",             /* ICON_VIDEO_ITEM */
                "image-x-generic",             /* ICON_IMAGE_ITEM */
                "text-x-generic",              /* ICON_TEXT_ITEM */
        };
        gupnp_tools_common_register_resource ();

        for (i = 0; i < ICON_MISSING; i++) {
                icons[i] = load_pixbuf_file (file_names[i]);
        }

        for (j = 0; i < ICON_LAST; i++, j++) {
                icons[i] = get_pixbuf_from_theme (theme_names[j]);
        }


        download_session = soup_session_new ();
        g_assert (download_session != NULL);

        pending_gets = NULL;
}

void
deinit_icons (void)
{
        int i;

        while (pending_gets) {
                GetIconURLData *data;

                data = pending_gets->data;

                soup_session_cancel_message (download_session,
                                             data->message,
                                             SOUP_STATUS_CANCELLED);
        }

        g_object_unref (download_session);

        for (i = 0; i < ICON_LAST; i++) {
                g_object_unref (icons[i]);
        }
        gupnp_tools_common_unregister_resource ();
}

