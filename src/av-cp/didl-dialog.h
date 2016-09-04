/*
 * Copyright (C) 2016 Jens Georg <mail@jensge.org>
 *
 * Authors: Jens Georg <mail@jensge.org>
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

#ifndef DIDL_DIALOG_H
#define DIDL_DIALOG_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgupnp/gupnp.h>

G_BEGIN_DECLS

GType
av_cp_didl_dialog_get_type (void);

#define AV_CP_TYPE_DIDL_DIALOG (av_cp_didl_dialog_get_type ())
#define AV_CP_DIDL_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 AV_CP_TYPE_DIDL_DIALOG, \
                 AVCPDidlDialog))
#define AV_CP_DIDL_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), \
            AV_CP_TYPE_DIDL_DIALOG, \
            AVCPDidlDialogClass))
#define AV_CP_IS_DIDL_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            AV_CP_TYPE_DIDL_DIALOG))
#define AV_CP_IS_DIDL_DIALOG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
            AV_CP_TYPE_DIDL_DIALOG))
#define AV_CP_DIDL_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
            AV_CP_TYPE_DIDL_DIALOG, \
            AVCPDidlDialogDeviceClass))

typedef struct _AVCPDidlDialog AVCPDidlDialog;
typedef struct _AVCPDidlDialogClass AVCPDidlDialogClass;

AVCPDidlDialog *av_cp_didl_dialog_new (void);
void av_cp_didl_dialog_set_xml (AVCPDidlDialog *self, const char *xml);

G_END_DECLS

#endif /* DIDL_DIALOG_H */
