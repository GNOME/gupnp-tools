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

#ifndef __UPNP_H__
#define __UPNP_H__

#include <libgupnp/gupnp.h>

void
notify_status_change            (gboolean       status);

void
notify_load_level_change        (gint           load_level);

gboolean
init_upnp                       (gchar **interfaces, guint port, gchar *name, gboolean ipv4, gboolean ipv6);

void
deinit_upnp                     (void);

void
set_all_status (gboolean status);

void
set_all_load_level (gint load_level);

#endif /* __UPNP_H__ */
