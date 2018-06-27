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

#ifndef __RENDERERCOMBO_H__
#define __RENDERERCOMBO_H__

#include <libgupnp/gupnp.h>
#include <libgupnp-av/gupnp-av.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtk/gtk.h>

typedef enum
{
  PLAYBACK_STATE_UNKNOWN,
  PLAYBACK_STATE_TRANSITIONING,
  PLAYBACK_STATE_STOPPED,
  PLAYBACK_STATE_PAUSED,
  PLAYBACK_STATE_PLAYING
} PlaybackState;

GUPnPServiceProxy *
get_selected_av_transport       (gchar                 **sink_protocol_info);

GUPnPServiceProxy *
get_selected_rendering_control  (void);

guint
get_selected_renderer_volume    (void);

PlaybackState
get_selected_renderer_state     (void);

void
clear_selected_renderer_state   (void);

void
add_media_renderer              (GUPnPDeviceProxy *proxy);

void
remove_media_renderer           (GUPnPDeviceProxy *proxy);

void
setup_renderer_combo            (GtkBuilder       *builder);

void
on_renderer_combo_changed       (GtkComboBox *widget,
                                 gpointer     user_data);

#endif /* __RENDERERCOMBO_H__ */
