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

#include "spline.h"
#include "annotation_window.h"
#include "utils.h"

/**
 * spline:
 * @list: A #GSList of #AnnotatePoint structs representing a polyline.
 *
 * Computes a smooth, C1-continuous curve that INTERPOLATES (passes through)
 * a given set of points.
 *
 * This implementation uses the principles of Catmull-Rom splines to generate
 * control points for cubic Bézier curves. This creates a visually smooth
 * result that is faithful to the user's input, without requiring external
 * heavy dependencies like GSL.
 *
 * Returns: (transfer full) (nullable): A new #GSList containing the
 * points required to draw the smoothed curve. The list is composed of
 * sequential triplets: (control point 1, control point 2, endpoint),
 * intended for use with cairo_curve_to(). The caller is responsible for
 * freeing this list and its contents.
 **/

/**
 * spline_from_points:
 * @points: (element-type AnnotatePoint): A #GSList of points to interpolate.
 *
 * Converts a list of points into a smooth curve using a Catmull-Rom spline.
 * The output is a new list of points that can be rendered as a series of
 * cubic Bézier curves. The format is [P0, CP1_0, CP2_0, P1, CP1_1, ...].
 *
 * This function is optimized to be O(n) by iterating backwards over the
 * input list and using g_slist_prepend() to build the output list directly,
 * avoiding a costly final reversal.
 *
 * Returns: (transfer full) (element-type AnnotatePoint): A new #GSList
 * containing the points for the spline, or %NULL if there are
 * fewer than two input points. The caller is responsible for
 * freeing this list and its data.
 */
GSList *
spline (GSList *points)
{
  GSList        *ret = NULL;
  guint          length;
  AnnotatePoint *first_point;

  length = g_slist_length (points);

  if (length < 2)
    {
      return NULL;
    }

  /*
   * Iterate backwards from the second-to-last point to the first. This allows
   * us to build the final list in the correct order using g_slist_prepend(),
   * which is an efficient O(1) operation.
   */
  for (gint i = length - 2; i >= 0; i--)
    {
      AnnotatePoint *p0, *p1, *p2, *p3;
      AnnotatePoint *control_point_1, *control_point_2, *end_point;
      gdouble        cp1_x, cp1_y, cp2_x, cp2_y;
      gdouble        cp1_pressure, cp2_pressure, cp1_width, cp2_width;

      p1 = g_slist_nth_data (points, i);
      p2 = g_slist_nth_data (points, i + 1);

      /* Duplicate endpoints to handle boundary conditions */
      p0 = (i > 0) ? g_slist_nth_data (points, i - 1) : p1;
      p3 = (i < length - 2) ? g_slist_nth_data (points, i + 2) : p2;

      /* Calculate Bézier control points from Catmull-Rom tangents */
      cp1_x = p1->x + (p2->x - p0->x) / 6.0;
      cp1_y = p1->y + (p2->y - p0->y) / 6.0;
      cp2_x = p2->x - (p3->x - p1->x) / 6.0;
      cp2_y = p2->y - (p3->y - p1->y) / 6.0;

      /* Interpolate attributes for the control points */
      cp1_pressure = p1->pressure + (p2->pressure - p1->pressure) / 3.0;
      cp2_pressure = p1->pressure + (p2->pressure - p1->pressure) * 2.0 / 3.0;
      cp1_width    = p1->width + (p2->width - p1->width) / 3.0;
      cp2_width    = p1->width + (p2->width - p1->width) * 2.0 / 3.0;

      control_point_1 = allocate_point (cp1_x, cp1_y, cp1_width, cp1_pressure);
      control_point_2 = allocate_point (cp2_x, cp2_y, cp2_width, cp2_pressure);
      end_point       = allocate_point (p2->x, p2->y, p2->width, p2->pressure);

      /* Prepend the segment's points (end, cp2, cp1) to the list head */
      ret = g_slist_prepend (ret, end_point);
      ret = g_slist_prepend (ret, control_point_2);
      ret = g_slist_prepend (ret, control_point_1);
    }

  /*
   * The loop has built the curve segments. Now, prepend the very first
   * point of the path to complete the list.
   */
  first_point = g_slist_nth_data (points, 0);
  ret = g_slist_prepend (ret,
                         allocate_point (first_point->x, first_point->y,
                                         first_point->width,
                                         first_point->pressure));

  return ret;
}
