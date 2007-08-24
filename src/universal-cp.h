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

#ifndef __GUPNP_UNIVERSAL_CP_H__
#define __GUPNP_UNIVERSAL_CP_H__

#include <libgupnp/gupnp-control-point.h>

void
on_state_variable_changed   (GUPnPServiceProxy             *proxy,
                             const char                    *variable_name,
                             GValue                        *value,
                             gpointer                       user_data);

void
show_action_arg_details     (GUPnPServiceActionArgInfo     *info);

void
show_action_details         (GUPnPServiceActionInfo        *info);

void
show_state_variable_details (GUPnPServiceStateVariableInfo *info);

void
show_service_details        (GUPnPServiceInfo              *info);

void
show_device_details         (GUPnPDeviceInfo               *info);

#endif /* __GUPNP_UNIVERSAL_CP_H__ */
