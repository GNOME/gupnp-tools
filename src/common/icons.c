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

#include <string.h>
#include <stdlib.h>
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

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GetIconURLData, get_icon_url_data_free)

static GdkPixbuf *
get_icon_from_bytes (GBytes *icon_data,
                     const char *mime,
                     int source_width,
                     int source_height,
                     GError **error)
{
        g_autoptr (GdkPixbufLoader) loader;
        GdkPixbuf       *pixbuf;

        loader = gdk_pixbuf_loader_new_with_mime_type (mime, error);
        if (loader == NULL)
                return NULL;

        gdk_pixbuf_loader_write_bytes (loader, icon_data, error);
        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        if (pixbuf) {
                gfloat aspect_ratio;
                gint   height;

                /* Preserve the aspect-ratio of the original image */
                aspect_ratio = (gfloat) source_width / source_height;
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
on_got_icon (GObject *source, GAsyncResult *res, gpointer user_data)
{
        GError *error = NULL;
        g_autoptr (GTask) task = G_TASK (user_data);
        g_autofree char *mime = NULL;
        int width;
        int height;

        g_autoptr (GBytes) icon =
                gupnp_device_info_get_icon_finish (GUPNP_DEVICE_INFO (source),
                                                   res,
                                                   &mime,
                                                   NULL,
                                                   &width,
                                                   &height,
                                                   &error);

        if (error != NULL) {
                g_task_return_error (task, error);

                return;
        }

        GdkPixbuf *device_icon = icon == NULL ? NULL
                                              : get_icon_from_bytes (icon,
                                                                     mime,
                                                                     width,
                                                                     height,
                                                                     &error);

        if (error != NULL) {
                g_task_return_error (task, error);

                return;
        }

        g_task_return_pointer (task, device_icon, g_object_unref);
}

void
update_icon_async (GUPnPDeviceInfo *info,
                   GCancellable *cancellable,
                   GAsyncReadyCallback callback,
                   gpointer user_data)
{
        GTask *task = g_task_new (info, cancellable, callback, user_data);

        gupnp_device_info_get_icon_async (info,
                                          NULL,
                                          PREFERED_DEPTH,
                                          PREFERED_WIDTH,
                                          PREFERED_HEIGHT,
                                          TRUE,
                                          cancellable,
                                          on_got_icon,
                                          task);
}

GdkPixbuf *
update_icon_finish (GUPnPDeviceInfo *info, GAsyncResult *res, GError **error)
{
        g_return_val_if_fail (g_task_is_valid (res, info), NULL);

        return g_task_propagate_pointer (G_TASK (res), error);
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

        g_object_unref (download_session);

        for (i = 0; i < ICON_LAST; i++) {
                g_object_unref (icons[i]);
        }
        gupnp_tools_common_unregister_resource ();
}

