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
#include <libsoup/soup.h>

#include "icons.h"

#define PREFERED_DEPTH  32
#define PREFERED_WIDTH  22
#define PREFERED_HEIGHT 22

/* provided by other modules */
/* FIXME: this should be done through a callback */
void
update_device_icon (GUPnPDeviceInfo *info,
                    GdkPixbuf       *icon);

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
                                                  GDK_INTERP_HYPER);
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

        pending_gets = g_list_remove (pending_gets, data);
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

        pending_gets = g_list_prepend (pending_gets, data);
        soup_session_queue_message (session,
                                    data->message,
                                    (SoupMessageCallbackFn) got_icon_url,
                                    data);

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
get_pixbuf_from_stock (const char *stock_id)
{
        GtkWidget *image;
        GdkPixbuf *pixbuf;

        image = gtk_image_new ();
        g_object_ref_sink (image);

        pixbuf = gtk_widget_render_icon (image,
                                         stock_id,
                                         GTK_ICON_SIZE_LARGE_TOOLBAR,
                                         "gupnp-universal-cp-icon");
        g_assert (pixbuf);
        pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                          PREFERED_WIDTH,
                                          PREFERED_HEIGHT,
                                          GDK_INTERP_HYPER);
        g_object_unref (image);

        return pixbuf;
}

static GdkPixbuf *
get_pixbuf_from_theme (const char *icon_name,
                       const char *fallback_icon_name)
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
                           fallback_icon_name,
                           error->message);
                g_error_free (error);

                pixbuf = gtk_icon_theme_load_icon (theme,
                                                   fallback_icon_name,
                                                   PREFERED_WIDTH,
                                                   PREFERED_HEIGHT,
                                                   &error);

                if (pixbuf == NULL) {
                        g_warning ("Failed to load icon %s: %s",
                                   fallback_icon_name,
                                   error->message);
                }
        }

        return pixbuf;
}

void
init_icons (void)
{
        int   i, j;
        char *file_names[] = {
                "pixmaps/upnp-device.png",          /* ICON_DEVICE         */
                "pixmaps/upnp-service.png",         /* ICON_SERVICE        */
                "pixmaps/upnp-state-variable.png",  /* ICON_VARIABLE       */
                "pixmaps/upnp-action-arg-in.png",   /* ICON_ACTION_ARG_IN  */
                "pixmaps/upnp-action-arg-out.png"   /* ICON_ACTION_ARG_OUT */
        };

        char *stock_ids[] = {
                GTK_STOCK_MISSING_IMAGE, /* ICON_MISSING   */
                GTK_STOCK_NETWORK,       /* ICON_NETWORK   */
                GTK_STOCK_EXECUTE,       /* ICON_ACTION    */
                GTK_STOCK_DIRECTORY,     /* ICON_VARIABLES */
                GTK_STOCK_FILE           /* ICON_FILE      */
        };

        char *theme_names[] = {
                "audio-volume-muted",          /* ICON_MIN_VOLUME */
                "audio-volume-high",           /* ICON_MAX_VOLUME */
                "folder-remote",               /* ICON_CONTAINER  */
        };

        char *fallback_theme_names[] = {
                "stock_volume-0",             /* ICON_MIN_VOLUME */
                "stock_volume-max",           /* ICON_MAX_VOLUME */
                "folder-remote",              /* ICON_CONTAINER  */
        };

        for (i = 0; i < ICON_MISSING; i++) {
                char *pixmap_path;

                pixmap_path = g_build_filename (DATA_DIR,
                                                file_names[i],
                                                NULL);

                icons[i] = gdk_pixbuf_new_from_file (pixmap_path, NULL);
                g_free (pixmap_path);

                g_assert (icons[i] != NULL);
        }

        for (j = 0; i < ICON_MIN_VOLUME; i++, j++) {
                icons[i] = get_pixbuf_from_stock (stock_ids[j]);
                g_assert (icons[i] != NULL);
        }

        for (j = 0; i < ICON_LAST; i++, j++) {
                icons[i] = get_pixbuf_from_theme (theme_names[j],
                                                  fallback_theme_names[j]);
                g_assert (icons[i] != NULL);
        }


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
        }

        g_object_unref (session);

        for (i = 0; i < ICON_LAST; i++) {
                g_object_unref (icons[i]);
        }
}

