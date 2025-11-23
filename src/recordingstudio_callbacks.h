/*
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2025 Pilolli Pietro <pilolli.pietro@gmail.com>
 *
 * Ardesia is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ardesia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef RECORDING_STUDIO_HANDLERS_H
#define RECORDING_STUDIO_HANDLERS_H

#include <cairo.h>
#include <gtk/gtk.h>

/* Click handlers */
G_MODULE_EXPORT void on_record_click (GtkToggleButton *button,
                                      gpointer func_data);

G_MODULE_EXPORT void on_pause_click (GtkToggleButton *button,
                                     gpointer func_data);

G_MODULE_EXPORT void on_stop_click (GtkButton *button, gpointer func_data);

G_MODULE_EXPORT void on_clapperboard_click (GtkButton *button,
                                            gpointer func_data);

G_MODULE_EXPORT void on_clapperboard_release (GtkButton *button,
                                              gpointer func_data);

G_MODULE_EXPORT void on_cursor_click (GtkToggleButton *button,
                                      gpointer func_data);

G_MODULE_EXPORT void on_folder_click (GtkButton *button, gpointer func_data);

/* Draw event handler */
gboolean on_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data);

/* Window events */
G_MODULE_EXPORT void on_recordingstudio_window_destroy_event (
    GtkWidget *widget, GdkEvent *event, gpointer data);

G_MODULE_EXPORT gboolean on_recordingstudio_window_delete_event (
    GtkWidget *widget, GdkEvent *event, gpointer data);

#endif /* RECORDING_STUDIO_HANDLERS_H */
