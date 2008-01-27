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

#ifndef __PLAYLISTTREEVIEW_H__
#define __PLAYLISTTREEVIEW_H__

#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libgupnp-av/gupnp-av.h>

void
setup_playlist_treeview         (GladeXML               *glade_xml);

void
add_media_server                (GUPnPDeviceProxy  *proxy);

void
remove_media_server             (GUPnPDeviceProxy  *proxy);

char *
get_selected_item               (char **uri);

#endif /* __PLAYLISTTREEVIEW_H__ */

