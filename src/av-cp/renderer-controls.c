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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <gtk/gtk.h>

void
on_play_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
}

void
on_pause_button_clicked (GtkButton *button,
                         gpointer   user_data)
{
}

void
on_stop_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
}

void
on_next_button_clicked (GtkButton *button,
                        gpointer   user_data)
{
}

void
on_previous_button_clicked (GtkButton *button,
                            gpointer   user_data)
{
}

gboolean
on_position_hscale_change_value (GtkRange *range,
                                 GtkScrollType scroll,
                                 gdouble       value,
                                 gpointer      user_data)
{
        return TRUE;
}

gboolean
on_volume_vscale_change_value (GtkRange *range,
                               GtkScrollType scroll,
                               gdouble       value,
                               gpointer      user_data)
{
        return TRUE;
}
