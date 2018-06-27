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

#ifndef __RENDERER_CONTROL_H__
#define __RENDERER_CONTROL_H__

#include <libgupnp/gupnp.h>
#include <libgupnp-av/gupnp-av.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gtk/gtk.h>

#include "renderer-combo.h"

void
set_av_transport_uri            (const char *metadata,
                                 GCallback   callback);

void
set_volume_scale                (guint volume);

void
set_position_scale_duration     (const char *duration_str);

void
set_position_scale_position     (const char *position_str);

void
prepare_controls_for_state      (PlaybackState state);

void
av_transport_send_action        (const char *action,
                                 char *additional_args[]);

void
setup_renderer_controls         (GtkBuilder *builder);

#endif /* __RENDERER_CONTROL_H__ */
