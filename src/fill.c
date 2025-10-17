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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fill.h"
#include "utils.h"

/**
 * fill:
 * @annotation_data: (nullable): a pointer to #AnnotateData
 * @x: X coordinate of the starting point
 * @y: Y coordinate of the starting point
 *
 * Performs a fill operation starting from the specified point (@x, @y)
 * within a closed path in the provided annotation data. The function
 * fills the interior of the shape starting at the given point using
 * the current annotation color.
 **/
void
fill (AnnotateData *annotation_data,
      gdouble x,
      gdouble y)
{
  if (! annotation_data || ! annotation_data->color)
    return;

  cairo_t *cr = annotation_data->annotation_cairo_context;

  guint r, g, b, a;
  if (sscanf (annotation_data->color, "%02X%02X%02X%02X", &r, &g, &b, &a) != 4)
    {
      g_debug ("Invalid color format: %s", annotation_data->color);
      cairo_destroy (cr);
      return;
    }

  double fr = r / 255.0;
  double fg = g / 255.0;
  double fb = b / 255.0;
  double fa = a / 255.0;

  for (GList *l = annotation_data->paths; l != NULL; l = l->next)
    {
      cairo_path_t *path = (cairo_path_t *) l->data;

      cairo_new_path (cr);
      cairo_append_path (cr, path);

      if (cairo_in_fill (cr, x, y))
        {
          g_debug ("found closed path to fill");
          cairo_set_source_rgba (cr, fr, fg, fb, fa);
	  gdouble padding = 2.0;

          gdouble x1, y1, x2, y2;
	  cairo_path_extents (cr, &x1, &y1, &x2, &y2);

	  x1 -= padding;
	  y1 -= padding;
	  x2 += padding;
	  y2 += padding;

	  gtk_widget_queue_draw_area (
	      GTK_WIDGET (annotation_data->annotation_window),
              (int)x1,
	      (int)y1,
              (int)(x2 - x1),
              (int)(y2 - y1)
	  );

          cairo_fill (cr);
          return;
        }
    }

  g_debug ("No path contains point (%f,%f)", x, y);
}
