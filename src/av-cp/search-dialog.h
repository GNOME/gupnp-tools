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

#ifndef SEARCH_DIALOG_H
#define SEARCH_DIALOG_H

#include <gtk/gtk.h>

#include "server-device.h"

#define SEARCH_DIALOG_TYPE (search_dialog_get_type ())
#define SEARCH_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                            SEARCH_DIALOG_TYPE, \
                            SearchDialog))

typedef struct _SearchDialog SearchDialog;
typedef struct _SearchDialogClass SearchDialogClass;

GType search_dialog_get_type (void);

GtkDialog *
search_dialog_new ();

void
search_dialog_set_server (SearchDialog *self, AVCPMediaServer *server);

void
search_dialog_set_container_id (SearchDialog *self, char *id);

void
search_dialog_set_container_title (SearchDialog *self, char *title);

void
search_dialog_run (SearchDialog *self);

#endif /* SEARCH_DIALOG_H */
