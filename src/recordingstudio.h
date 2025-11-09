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

#ifndef RECORDINGSTUDIO_H
#define RECORDINGSTUDIO_H

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "config.h"

/* Structure that contains the info passed to the callbacks. */
typedef struct
{
  gboolean         recording;
  gboolean         cursor_visible;
  gint             cursor_step;
  cairo_surface_t *cursor_surface;
} RecordingStudioData;

typedef struct
{
  gint    x; // Ultima posizione X (relativa alla finestra)
  gint    y; // Ultima posizione Y (relativa alla finestra)
  gdouble r; // Colore calcolato
  gdouble g;
  gdouble b;
} CursorAnimState;

#endif // RECORDINGSTUDIO_H
