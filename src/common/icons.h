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

#ifndef __GUPNP_UNIVERSAL_CP_ICONS_H__
#define __GUPNP_UNIVERSAL_CP_ICONS_H__

#include <libgupnp/gupnp-control-point.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

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
        ICON_MIN_VOLUME,
        ICON_MAX_VOLUME,
        ICON_CONTAINER,
        ICON_AUDIO_ITEM,
        ICON_VIDEO_ITEM,
        ICON_IMAGE_ITEM,
        ICON_TEXT_ITEM,
        ICON_LAST
} IconID;

typedef void (* DeviceIconAvailableCallback) (GUPnPDeviceInfo *info,
                                              GdkPixbuf       *icon);

void
schedule_icon_update   (GUPnPDeviceInfo            *info,
                        DeviceIconAvailableCallback callback);

void
unschedule_icon_update (GUPnPDeviceInfo *info);

GdkPixbuf *
get_icon_by_id         (IconID icon_id);

GdkPixbuf *
load_pixbuf_file       (const char *file_name);

void
init_icons             (void);

void
deinit_icons           (void);

#endif /* __GUPNP_UNIVERSAL_CP_ICONS_H__ */
