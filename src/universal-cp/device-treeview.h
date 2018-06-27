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

#ifndef __UNIVERSAL_CP_DEVICETREEVIEW_H__
#define __UNIVERSAL_CP_DEVICETREEVIEW_H__

#include <string.h>
#include <stdlib.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

gboolean
find_device             (GtkTreeModel              *model,
                         const char                *udn,
                         GtkTreeIter               *root_iter,
                         GtkTreeIter               *iter);

GUPnPDeviceInfo *
get_service_device      (GUPnPServiceInfo          *service_info);

void
remove_device           (GUPnPDeviceInfo           *info);

void
append_device           (GUPnPDeviceInfo            *info);

void
setup_device_treeview   (GtkBuilder                 *builder);

GUPnPServiceProxy *
get_selected_service    (void);

GUPnPServiceActionInfo *
get_selected_action     (GUPnPServiceProxy         **ret_proxy,
                         GUPnPServiceIntrospection **ret_introspection);

#endif /* __UNIVERSAL_CP_DEVICETREEVIEW_H__ */
