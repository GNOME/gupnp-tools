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

#ifndef __GUPNP_UNIVERSAL_CP_ICONS_H__
#define __GUPNP_UNIVERSAL_CP_ICONS_H__

#include <libgupnp/gupnp-control-point.h>
#include <gtk/gtk.h>

typedef enum
{
        ICON_FIRST = -1,
        ICON_DEVICE,
        ICON_SERVICE,
        ICON_VARIABLE,
        ICON_ACTION_ARG_IN,
        ICON_ACTION_ARG_OUT,
        ICON_MEDIA_RENDERER,
        ICON_MISSING,
        ICON_NETWORK,
        ICON_ACTION,
        ICON_VARIABLES,
        ICON_FILE,
        ICON_CONTAINER,
        ICON_AUDIO_ITEM,
        ICON_VIDEO_ITEM,
        ICON_IMAGE_ITEM,
        ICON_TEXT_ITEM,
        ICON_LAST
} IconID;

void
update_icon_async (GUPnPDeviceInfo *info,
                   GCancellable *cancellable,
                   GAsyncReadyCallback callback,
                   gpointer user_data);

GdkPixbuf *
update_icon_finish (GUPnPDeviceInfo *info, GAsyncResult *res, GError **error);

GdkPixbuf *
get_icon_by_id         (IconID icon_id);

GdkPixbuf *
load_pixbuf_file       (const char *file_name);

void
init_icons             (void);

void
deinit_icons           (void);

#endif /* __GUPNP_UNIVERSAL_CP_ICONS_H__ */
