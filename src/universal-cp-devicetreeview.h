/*
 * Copyright (C) 2007 Zeeshan Ali <zeenix@gstreamer.net>
 *
 * Authors: Zeeshan Ali <zeenix@gstreamer.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __UNIVERSAL_CP_DEVICETREEVIEW_H__
#define __UNIVERSAL_CP_DEVICETREEVIEW_H__

#include <string.h>
#include <stdlib.h>
#include <config.h>

extern gboolean expanded;

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

GtkTreeModel *
create_device_treemodel (void);

void
setup_device_treeview   (GtkWidget                  *treeview,
                         GtkTreeModel               *model,
                         char                       *headers[],
                         int                         render_index);

GUPnPServiceProxy *
get_selected_service    (void);

GUPnPServiceActionInfo *
get_selected_action     (GUPnPServiceProxy         **ret_proxy,
                         GUPnPServiceIntrospection **ret_introspection);

#endif /* __UNIVERSAL_CP_DEVICETREEVIEW_H__ */
