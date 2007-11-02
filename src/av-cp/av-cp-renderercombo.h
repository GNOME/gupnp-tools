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

#ifndef __AV_CP_RENDERERCOMBO_H__
#define __AV_CP_RENDERERCOMBO_H__

#include <libgupnp/gupnp.h>
#include <libgupnp-av/gupnp-av.h>
#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

void
update_device_icon              (GUPnPDeviceInfo *info,
                                 GdkPixbuf       *icon);
void
add_media_renderer              (GUPnPMediaRendererProxy *renderer);

void
remove_media_renderer           (GUPnPMediaRendererProxy *renderer);

void
setup_renderer_combo            (GladeXML                *glade_xml);

#endif /* __AV_CP_RENDERERCOMBO_H__ */
