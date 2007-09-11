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

#ifndef __GUPNP_NETWORK_LIGHT_UPNP_H__
#define __GUPNP_NETWORK_LIGHT_UPNP_H__

#include <libgupnp/gupnp.h>

void
notify_status_change            (gboolean       status);

void
notify_load_level_change        (gint           load_level);

gboolean
init_upnp                       (void);

void
deinit_upnp                     (void);

#endif /* __GUPNP_NETWORK_LIGHT_UPNP_H__ */