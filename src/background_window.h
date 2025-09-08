/*
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli.pietro@gmail.com>
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

#ifndef __BACKGROUND_WINDOW_H
#define __BACKGROUND_WINDOW_H

#include <gtk/gtk.h>

#define BACKGROUND_OPACITY 0.01

/* Structure that contains the info passed to the callbacks. */
typedef struct
{

  /* 0 no background, 1 color, 1 image*/
  gint type;

  /* Background colour selected. */
  gchar *color;

  /* Background image selected. */
  gchar *image;

  /* The background widget that represent the full window. */
  //  GtkWidget *background_window;

  /* cairo context to draw on the background window. */
  cairo_t *cr;

} BackgroundData;

extern BackgroundData *background_data;

BackgroundData *create_background_data ();

void destroy_background_data ();

void clear_background_context ();

/* Update the background image. */
void update_background_image (gchar *name);

/* Update the background colour. */
void update_background_color (gchar *rgba);

#endif
