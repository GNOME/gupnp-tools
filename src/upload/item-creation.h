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

#ifndef __ITEM_CREATION_H__
#define __ITEM_CREATION_H__

#include <libgupnp/gupnp.h>

void
create_item                     (const char              *file_path,
                                 const char              *title,
                                 GUPnPServiceProxy       *cds_proxy,
                                 const char              *container_id,
                                 GUPnPContext            *context);

#endif /* __ITEM_CREATION_H__ */

