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

#include "broken.h"
#include "annotation_window.h"
#include "bar.h"
#include "utils.h"

/**
 * get_perpendicular_distance:
 * @p: The point to check.
 * @p1: The start point of the line segment.
 * @p2: The end point of the line segment.
 *
 * Helper for Douglas-Peucker. Calculates the perpendicular distance of a
 * point from the line segment connecting two other points.
 *
 * Returns: The perpendicular distance.
 */
static gdouble
get_perpendicular_distance (AnnotatePoint *p,
                            AnnotatePoint *p1,
                            AnnotatePoint *p2)
{
  gdouble dx = p2->x - p1->x;
  gdouble dy = p2->y - p1->y;
  gdouble mag_sq;
  gdouble t;

  mag_sq = dx * dx + dy * dy;

  if (mag_sq < 1e-9) /* Points are identical */
    {
      return get_distance (p->x, p->y, p1->x, p1->y);
    }

  t = ((p->x - p1->x) * dx + (p->y - p1->y) * dy) / mag_sq;

  if (t < 0.0)
    {
      return get_distance (p->x, p->y, p1->x, p1->y);
    }
  else if (t > 1.0)
    {
      return get_distance (p->x, p->y, p2->x, p2->y);
    }
  else
    {
      gdouble ix = p1->x + t * dx;
      gdouble iy = p1->y + t * dy;
      return get_distance (p->x, p->y, ix, iy);
    }
}

/**
 * copy_annotate_point:
 * @src: Source #AnnotatePoint.
 * @user_data: Unused.
 *
 * GCopyFunc to properly duplicate an AnnotatePoint for use with
 * g_slist_copy_deep.
 *
 * Returns: (transfer full): A new copy of the point.
 */
static gpointer
copy_annotate_point (gconstpointer src,
                     gpointer      user_data)
{
  return g_memdup2 (src, sizeof (AnnotatePoint));
}

/**
 * douglas_peucker_recursive:
 * @points: (transfer none): The list of points to simplify.
 * @epsilon: The simplification tolerance.
 *
 * The recursive part of the Douglas-Peucker algorithm.
 *
 * Returns: (transfer full): A new list with the simplified points.
 */
static GSList *
douglas_peucker_recursive (GSList *points, gdouble epsilon)
{
  gdouble max_dist   = 0.0;
  GSList *pivot_node = NULL;
  GSList *iter;
  guint   length;
  guint   pivot_index = 0;
  GSList *result      = NULL;

  length = g_slist_length (points);
  if (length < 3)
    {
      return g_slist_copy_deep (points, copy_annotate_point, NULL);
    }

  AnnotatePoint *first = points->data;
  AnnotatePoint *last  = g_slist_last (points)->data;

  iter = g_slist_next (points);
  for (guint i = 1; i < length - 1; i++)
    {
      gdouble dist = get_perpendicular_distance (iter->data, first, last);
      if (dist > max_dist)
        {
          max_dist    = dist;
          pivot_node  = iter;
          pivot_index = i;
        }
      iter = g_slist_next (iter);
    }

  if (max_dist > epsilon && pivot_node)
    {
      GSList *first_half  = NULL;
      GSList *second_half = NULL;
      GSList *res1, *res2;

      iter = points;
      for (guint i = 0; i <= pivot_index; i++)
        {
          first_half = g_slist_prepend (first_half, iter->data);
          iter       = g_slist_next (iter);
        }
      first_half = g_slist_reverse (first_half);

      iter = pivot_node;
      while (iter)
        {
          second_half = g_slist_prepend (second_half, iter->data);
          iter        = g_slist_next (iter);
        }
      second_half = g_slist_reverse (second_half);

      res1 = douglas_peucker_recursive (first_half, epsilon);
      res2 = douglas_peucker_recursive (second_half, epsilon);

      g_slist_free (first_half);
      g_slist_free (second_half);

      GSList *last_of_res1 = g_slist_last (res1);
      res1                 = g_slist_remove_link (res1, last_of_res1);
      g_free (last_of_res1->data);
      g_slist_free (last_of_res1);

      result = g_slist_concat (res1, res2);
    }
  else
    {
      result = g_slist_append (result, 
                               g_memdup2 (first, sizeof (AnnotatePoint)));
      result = g_slist_append (result, 
                               g_memdup2 (last, sizeof (AnnotatePoint)));
    }

  return result;
}

/* Number x is roundable to y. */
static gboolean
is_similar (gdouble x, gdouble y, gdouble pixel_tollerance)
{
  gdouble delta = fabs (x - y);

  if (delta <= pixel_tollerance)
    {
      return TRUE;
    }

  return FALSE;
}

static guint
count_points_along_horizontal (GSList *list,
                               gdouble x,
                               gdouble pixel_tollerance)
{
  guint i      = 0;
  guint length = g_slist_length (list);
  guint cnt    = 0;
  /* Search the min and max coordinates. */
  for (i = 1; i < length; i++)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) g_slist_nth_data (list, i);
      if (is_similar (cur_point->x, x, pixel_tollerance))
        {
          cnt++;
        }
    }
  return cnt;
}

static guint
count_points_along_vertical (GSList *list, gdouble y, gdouble pixel_tollerance)
{
  guint i      = 0;
  guint length = g_slist_length (list);
  guint cnt    = 0;
  /* Search the min and max coordinates. */
  for (i = 1; i < length; i++)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) g_slist_nth_data (list, i);
      if (is_similar (cur_point->y, y, pixel_tollerance))
        {
          cnt++;
        }
    }
  return cnt;
}

/* Take the list and found the minx miny maxx and maxy points. */
static void
found_min_and_max (GSList *list,
                   gdouble *minx,
                   gdouble *miny,
                   gdouble *maxx,
                   gdouble *maxy)
{
  guint i = 0;

  /* Initialize the min and max to the first point coordinates. */
  AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (list, i);
  *minx                      = first_point->x;
  *miny                      = first_point->y;
  *maxx                      = first_point->x;
  *maxy                      = first_point->y;

  guint length = g_slist_length (list);

  /* Search the min and max coordinates. */
  for (i = 1; i < length; i++)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) g_slist_nth_data (list, i);
      *minx                    = MIN (*minx, cur_point->x);
      *miny                    = MIN (*miny, cur_point->y);
      *maxx                    = MAX (*maxx, cur_point->x);
      *maxy                    = MAX (*maxy, cur_point->y);
    }
}

/*
 * The list of point is roundable to a rectangle
 * Note this algorithm found only the rectangle parallel to the axis.
 */
static gboolean
is_a_rectangle (GSList *list, gdouble pixel_tollerance)
{

  if (g_slist_length (list) != 4)
    {
      return FALSE;
    }
  else
    {
      AnnotatePoint *point0 = (AnnotatePoint *) g_slist_nth_data (list, 0);
      AnnotatePoint *point1 = (AnnotatePoint *) g_slist_nth_data (list, 1);
      AnnotatePoint *point2 = (AnnotatePoint *) g_slist_nth_data (list, 2);
      AnnotatePoint *point3 = (AnnotatePoint *) g_slist_nth_data (list, 3);

      if (! (is_similar (point0->x, point1->x, pixel_tollerance)))
        {
          return FALSE;
        }

      if (! (is_similar (point1->y, point2->y, pixel_tollerance)))
        {
          return FALSE;
        }

      if (! (is_similar (point2->x, point3->x, pixel_tollerance)))
        {
          return FALSE;
        }

      if (! (is_similar (point3->y, point0->y, pixel_tollerance)))
        {
          return FALSE;
        }
    }

  /* Postcondition: it is a rectangle. */
  return TRUE;
}

static gboolean
is_a_triangle (GSList *list, gdouble pixel_tollerance)
{
  gdouble minx = 0, miny = 0, maxx = 0, maxy = 0;
  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  // the * 3 adds some additional tolerance for wonky rectangles
  guint top    = count_points_along_vertical (list,
                                              miny,
                                              pixel_tollerance * 3);

  guint bottom = count_points_along_vertical (list,
                                              maxy,
                                              pixel_tollerance * 3);

  guint left   = count_points_along_horizontal (list,
                                                minx,
                                                pixel_tollerance * 3);

  guint right  = count_points_along_horizontal (list,
                                                maxx,
                                                pixel_tollerance * 3);

  g_debug ("triangle: %d %d %d %d", top, left, bottom, right);

  /* if one of the axis only has one point in it we will regard as a triangle */
  return (top == 1 || bottom == 1 || left == 1 || right == 1);
}

/* Calculate the media of the point pression. */
static gdouble
calculate_medium_pression (GSList *list)
{
  guint   i              = 0;
  gdouble total_pressure = 0;
  guint   length         = g_slist_length (list);

  if (length == 0)
    return 1.0;

  for (i = 0; i < length; i++)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) g_slist_nth_data (list, i);
      total_pressure           = total_pressure + cur_point->pressure;
    }

  return total_pressure / length;
}

/*
 * Check if a closed polygon (points in order) is convex.
 *
 * Returns TRUE if all cross products have the same sign (ignoring near-zero),
 * otherwise FALSE. This prevents star-shaped / self-intersecting polygons
 * from being misclassified as regular polygons.
 */
static gboolean
is_polygon_convex (GSList *points)
{
  guint n = g_slist_length (points);
  if (n < 3)
    return FALSE;

  gint          sign = 0; /* +1 or -1 when we detect a non-zero turn */
  const gdouble EPS  = 1e-9;

  for (guint i = 0; i < n; i++)
    {
      AnnotatePoint *A = g_slist_nth_data (points, i);
      AnnotatePoint *B = g_slist_nth_data (points, (i + 1) % n);
      AnnotatePoint *C = g_slist_nth_data (points, (i + 2) % n);

      gdouble v1x = B->x - A->x;
      gdouble v1y = B->y - A->y;
      gdouble v2x = C->x - B->x;
      gdouble v2y = C->y - B->y;

      /* cross product z-component */
      gdouble cross = v1x * v2y - v1y * v2x;

      if (fabs (cross) <= EPS)
        continue; /* collinear or tiny - ignore */

      if (cross > 0)
        {
          if (sign < 0)
            return FALSE;
          sign = 1;
        }
      else /* cross < 0 */
        {
          if (sign > 0)
            return FALSE;
          sign = -1;
        }
    }

  /* If sign never set (all collinear) consider it non-convex for our use */
  return (sign != 0);
}

/* The path described in list is similar to a regular polygon. */
static gboolean
is_similar_to_a_regular_polygon (GSList *list, gdouble pixel_tollerance)
{
  if (! is_polygon_convex (list))
    {
      return FALSE;
    }
  guint   i              = 0;
  gdouble ideal_distance = -1;
  gdouble total_distance = 0;

  guint          length    = g_slist_length (list);
  AnnotatePoint *old_point = (AnnotatePoint *) g_slist_nth_data (list, i);

  for (i = 1; i < length; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);

      gdouble distance     = get_distance (old_point->x,
                                           old_point->y,
                                           point->x,
                                           point->y);

      total_distance = total_distance + distance;
      old_point      = point;
    }

  ideal_distance = total_distance / length;

  i         = 0;
  old_point = (AnnotatePoint *) g_slist_nth_data (list, i);

  for (i = 1; i < length; i++)
    {
      AnnotatePoint *point     = (AnnotatePoint *) g_slist_nth_data (list, i);
      /* I have seen that a good compromise allow around 33% of error. */
      gdouble        threshold = ideal_distance / 3 + pixel_tollerance;

      gdouble        distance  = get_distance (point->x,
                                               point->y,
                                               old_point->x,
                                               old_point->y);

      if (! (is_similar (distance, ideal_distance, threshold)))
        {
          return FALSE;
        }

      old_point = point;
    }

  return TRUE;
}

/* Take a path and return the regular polygon path. */
static GSList *
extract_polygon (GSList *list)
{
  gdouble        cx          = -1;
  gdouble        cy          = -1;
  gdouble        radius      = -1;
  gdouble        minx        = -1;
  gdouble        miny        = -1;
  gdouble        maxx        = -1;
  gdouble        maxy        = -1;
  gdouble        angle_off   = M_PI / 2;
  gdouble        x1          = -1;
  gdouble        y1          = -1;
  guint          i           = 0;
  guint          length      = 0;
  gdouble        angle_step  = 0;
  AnnotatePoint *last_point  = NULL;
  AnnotatePoint *first_point = NULL;

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  cx         = (maxx + minx) / 2;
  cy         = (maxy + miny) / 2;
  radius     = ((maxx - minx) + (maxy - miny)) / 4;
  length     = g_slist_length (list);
  angle_step = 2 * M_PI / (length - 1);
  angle_off += angle_step / 2;

  for (i = 0; i < length - 1; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
      x1                   = radius * cos (angle_off) + cx;
      y1                   = radius * sin (angle_off) + cy;
      point->x             = x1;
      point->y             = y1;
      angle_off += angle_step;
    }

  last_point    = (AnnotatePoint *) g_slist_nth_data (list, length - 1);
  first_point   = (AnnotatePoint *) g_slist_nth_data (list, 0);
  last_point->x = first_point->x;
  last_point->y = first_point->y;

  return list;
}

/* Set x-axis of the point. */
static void
point_put_x (gpointer current, gpointer value)
{
  AnnotatePoint *current_point = (AnnotatePoint *) current;
  gdouble       *valuex        = (gdouble *) value;
  current_point->x             = *valuex;
}

/* Set y-axis of the point. */
static void
point_put_y (gpointer current, gpointer value)
{
  AnnotatePoint *current_point = (AnnotatePoint *) current;
  gdouble       *value_y       = (gdouble *) value;
  current_point->y             = *value_y;
}

/* Return the degree of the rectangle between two point respect the axis. */
static gdouble
calculate_edge_degree (AnnotatePoint *point_a, AnnotatePoint *point_b)
{
  gdouble deltax       = fabs (point_a->x - point_b->x);
  gdouble deltay       = fabs (point_a->y - point_b->y);
  gdouble direction_ab = atan2 (deltay, deltax) / M_PI * 180;
  return direction_ab;
}

/*
 * straighten:
 *
 * Takes a list of AnnotatePoint structures and returns a new list
 * where minor deviations in direction are smoothed out.
 * Only significant points that exceed the degree threshold are kept,
 * and nearly horizontal or vertical lines are adjusted to exact
 * horizontal or vertical alignment.
 *
 * Parameters:
 * list - a GSList of AnnotatePoint* representing the input points.
 *
 * Returns:
 * A new GSList of AnnotatePoint* containing the straightened points.
 *
 * Note:
 * The original list is not modified. The returned list must be freed
 * by the caller when no longer needed.
 */
static GSList *
straighten (GSList *list)
{
  AnnotatePoint *inp_point        = NULL;
  AnnotatePoint *first_point      = NULL;
  AnnotatePoint *last_point       = NULL;
  AnnotatePoint *last_out_point   = NULL;
  gdouble        degree_threshold = 15;
  GSList        *list_out         = NULL;
  guint          length           = 0;
  guint          i;
  gdouble        direction;

  length = g_slist_length (list);

  /* Copy the first one point; it is a good point. */
  inp_point = (AnnotatePoint *) g_slist_nth_data (list, 0);

  first_point = allocate_point (inp_point->x,
                                inp_point->y,
                                inp_point->width,
                                inp_point->pressure);

  list_out = g_slist_prepend (list_out, first_point);

  for (i = 0; i < length - 2; i++)
    {
      AnnotatePoint *point_a = (AnnotatePoint *) g_slist_nth_data (list, i);
      AnnotatePoint *point_b = (AnnotatePoint *) g_slist_nth_data (list, i + 1);
      AnnotatePoint *point_c = (AnnotatePoint *) g_slist_nth_data (list, i + 2);

      gdouble direction_ab = calculate_edge_degree (point_a, point_b);
      gdouble direction_bc = calculate_edge_degree (point_b, point_c);
      gdouble delta_degree = fabs (direction_ab - direction_bc);

      if (delta_degree > degree_threshold)
        {
          /* Copy B it's a good point. */
          AnnotatePoint *point = allocate_point (point_b->x,
                                                 point_b->y,
                                                 point_b->width,
                                                 point_b->pressure);

          list_out = g_slist_prepend (list_out, point);
        }

      /*
       * Else: is three the difference degree is minor than
       * the threshold I neglegt B.
       */
    }

  /* Copy the last point; it is a good point. */
  last_point = (AnnotatePoint *) g_slist_nth_data (list, length - 1);

  last_out_point = allocate_point (last_point->x,
                                   last_point->y,
                                   last_point->width,
                                   last_point->pressure);

  list_out = g_slist_prepend (list_out, last_out_point);

  /* I reverse the list to preserve the initial order. */
  list_out = g_slist_reverse (list_out);

  length = g_slist_length (list_out);

  if (length != 2)
    {
      return list_out;
    }

  /* It is a segment! */
  direction = calculate_edge_degree (first_point, last_point);

  /* Is it is closed to 0 degree I draw an horizontal line. */
  if ((0 - degree_threshold <= direction) &&
      (direction <= 0 + degree_threshold))
    {
      /* y is the average */
      gdouble y = (first_point->y + last_point->y) / 2;
      /* Put this y for each element in the list. */
      g_slist_foreach (list_out, (GFunc) point_put_y, &y);
    }

  /* It is closed to 90 degree I draw a vertical line. */
  if ((90 - degree_threshold <= direction) &&
      (direction <= 90 + degree_threshold))
    {
      /* x is the average */
      gdouble x = (first_point->x + last_point->x) / 2;
      /* put this x for each element in the list. */
      g_slist_foreach (list_out, (GFunc) point_put_x, &x);
    }

  return list_out;
}

/**
 * build_meaningful_point_list:
 * @list_inp:          a GSList of AnnotatePoint representing a stroke or path
 * @pixel_tollerance:  threshold in pixels for determining meaningful deviation
 *
 * This function returns a new GSList containing a subset of points from
 * @list_inp, filtering out redundant points. It approximates the original
 * stroke by keeping only points that contribute significant visual deviation.
 * It uses the Ramer-Douglas-Peucker algorithm.
 *
 * Returns: (transfer full): A new, simplified list of #AnnotatePoint.
 * The caller is responsible for freeing the list and its points.
 **/
GSList *
build_meaningful_point_list (GSList *list_inp,
                             gdouble pixel_tollerance)
{
  GSList *simplified_list;
  
  if (g_slist_length (list_inp) < 3)
    {
      return g_slist_copy_deep (list_inp,
                                copy_annotate_point,
                                NULL);
    }

  simplified_list = douglas_peucker_recursive (list_inp, pixel_tollerance);

  /*
   * The pressure of the simplified points can be uneven. We recalculate a
   * medium pressure from the original list and apply it to all points
   * in the new list for a consistent stroke width.
   */
  if (simplified_list)
    {
      gdouble medium_pressure = calculate_medium_pression (list_inp);
      for (GSList *iter = simplified_list; iter; iter = g_slist_next (iter))
        {
          AnnotatePoint *p = iter->data;
          p->pressure      = medium_pressure;
        }
    }

  return simplified_list;
}

/**
 * build_outbounded_rectangle:
 * @list: a GSList of AnnotatePoint representing a stroke or path
 *
 * This function computes the axis-aligned bounding rectangle that fully
 * encloses all points in the input list @list.
 *
 * Algorithm:
 * 1. Determine the minimum and maximum x and y coordinates from all points
 * in the list using the helper function found_min_and_max().
 * 2. Create four AnnotatePoint instances representing the corners of the
 * bounding rectangle:
 * - Bottom-left (minx, miny)
 * - Top-left    (minx, maxy)
 * - Top-right   (maxx, maxy)
 * - Bottom-right(maxx, miny)
 * 3. Add an extra point equal to the first corner to "close" the rectangle
 * path for drawing purposes.
 * 4. Return a new GSList containing these AnnotatePoint objects in order.
 *
 * Notes:
 * - The width and pressure values are copied from the middle point of the
 * input list to maintain consistent visual properties.
 * - The caller is responsible for freeing the returned GSList and its points.
 *
 * Returns:
 * - A GSList of AnnotatePoint representing the bounding rectangle around
 * the input stroke/path.
 **/
GSList *
build_outbounded_rectangle (GSList *list)
{
  guint          length = g_slist_length (list);
  AnnotatePoint *point  = (AnnotatePoint *) g_slist_nth_data (list, length / 2);
  GSList        *ret_list = (GSList *) NULL;

  gdouble minx = 0;
  gdouble miny = 0;
  gdouble maxx = 0;
  gdouble maxy = 0;

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  AnnotatePoint *point3 = allocate_point (minx,
                                          maxy,
                                          point->width,
                                          point->pressure);

  ret_list = g_slist_prepend (ret_list, point3);

  AnnotatePoint *point2 = allocate_point (maxx,
                                          maxy,
                                          point->width,
                                          point->pressure);

  ret_list = g_slist_prepend (ret_list, point2);

  AnnotatePoint *point1 = allocate_point (maxx,
                                          miny,
                                          point->width,
                                          point->pressure);

  ret_list = g_slist_prepend (ret_list, point1);

  AnnotatePoint *point0 = allocate_point (minx, 
                                          miny,
                                          point->width,
                                          point->pressure);

  ret_list = g_slist_prepend (ret_list, point0);

  /* added in return point to close off. */
  AnnotatePoint *point4 = allocate_point (minx,
                                          maxy,
                                          point->width,
                                          point->pressure);

  ret_list = g_slist_prepend (ret_list, point4);

  return ret_list;
}

/**
 * is_similar_to_an_ellipse:
 * @list: a GSList of AnnotatePoint representing a closed path
 * @pixel_tollerance: a tolerance threshold in pixels
 *
 * Determines if the path represented by the points in @list is similar
 * to an ellipse, using the geometric definition of an ellipse:
 * - For an ellipse, the sum of the distances from any point on the
 * ellipse to the two foci is constant.
 *
 * Algorithm:
 * -  Compute the bounding box of the path (minx, miny, maxx, maxy).
 * -  Compute the semi-axes a (horizontal) and b (vertical) of the ellipse.
 * -  Compute the focal distance c = sqrt(|a^2 - b^2|).
 * -  Determine the coordinates of the two foci (f1, f2) depending on the
 * orientation (horizontal or vertical).
 * -  Compute the sum of distances from the first point (minx, miny) to
 * both foci. This serves as the "ideal sum" for a perfect ellipse.
 * -  Iterate over all points in the path, compute the sum of distances to
 * the foci, and check if the difference from the ideal sum exceeds
 * the tolerance.
 * -  If any point violates the tolerance, the path is not considered
 * similar to an ellipse; otherwise, it is.
 *
 * Notes:
 * - The tolerance is increased slightly based on the average semi-axis
 * length to account for larger ellipses.
 * - Uses Euclidean distance (get_distance function) for computation.
 *
 * Returns:
 * - TRUE if the path approximates an ellipse within the given tolerance.
 * - FALSE otherwise.
 **/
gboolean
is_similar_to_an_ellipse (GSList *list, gdouble pixel_tollerance)
{
  guint   i          = 0;
  gdouble minx       = 0;
  gdouble miny       = 0;
  gdouble maxx       = 0;
  gdouble maxy       = 0;
  gdouble tollerance = 0;

  /* Semi x-axis */
  gdouble a = 0;

  /* Semi y-axis */
  gdouble b = 0;

  gdouble c = 0.0;

  /* x coordinate of the origin */
  gdouble originx = 0;

  /* y coordinate of the origin */
  gdouble originy = 0;

  /* x coordinate of focus1 */
  gdouble f1x = 0;

  /* y coordinate of focus1 */
  gdouble f1y = 0;

  /* x coordinate of focus2 */
  gdouble f2x = 0;

  /* y coordinate of focus2 */
  gdouble f2y = 0;

  gdouble distance_p1f1 = 0;
  gdouble distance_p1f2 = 0;
  gdouble sump1         = 0;

  gdouble aq = 0;
  gdouble bq = 0;

  guint length = g_slist_length (list);

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  a = (maxx - minx) / 2;
  b = (maxy - miny) / 2;

  aq = pow (a, 2);
  bq = pow (b, 2);

  /*
   * If in one point the sum of the distance by focus F1 and F2 differer more
   * than the tolerance value the curve line will not be considered an ellipse.
   */
  tollerance = pixel_tollerance + (a + b) / 2;

  originx = minx + a;
  originy = miny + b;

  if (aq > bq)
    {
      c   = sqrt (aq - bq);
      /* F1 (x0-c,y0) */
      f1x = originx - c;
      f1y = originy;
      /* F2 (x0+c,y0) */
      f2x = originx + c;
      f2y = originy;
    }
  else
    {
      c   = sqrt (bq - aq);
      /* F1 (x0, y0-c) */
      f1x = originx;
      f1y = originy - c;
      /* F2 (x0, y0+c) */
      f2x = originx;
      f2y = originy + c;
    }

  distance_p1f1 = get_distance (minx, miny, f1x, f1y);
  distance_p1f2 = get_distance (minx, miny, f2x, f2y);
  sump1         = distance_p1f1 + distance_p1f2;

  /*
   * In the ellipse the sum of the distance
   * (p,f1)+distance (p,f2) must be constant.
   */

  for (i = 0; i < length; i++)
    {
      AnnotatePoint *point      = (AnnotatePoint *) g_slist_nth_data (list, i);
      gdouble        distancef1 = get_distance (point->x, point->y, f1x, f1y);
      gdouble        distancef2 = get_distance (point->x, point->y, f2x, f2y);
      gdouble        sum        = distancef1 + distancef2;
      gdouble        difference = fabs (sum - sump1);

      if (difference > tollerance)
        {
          /*
           * The sum is too different from the ideal one;
           * I do not approximate the shape to an ellipse.
           */
          return FALSE;
        }
    }

  return TRUE;
}

/*
 * build_rectified_list:
 * @list_inp:        input GSList of AnnotatePoint (assumed ordered subpath)
 * @close_path:      TRUE if the subpath is closed (shape),
 * FALSE for open strokes
 * @pixel_tollerance: tolerance in pixels used by detectors / simplification
 *
 * Returns a new GSList with rectified points. The function copies input points
 * (so returned list elements are newly allocated via allocate_point ()) and
 * either:
 * - recognizes and returns a geometric shape (rectangle, triangle, polygon)
 * - or returns a straightened / simplified polyline.
 *
 * Important: this function does NOT try to be overly aggressive. Regular
 * polygon extraction is only performed for convex shapes (prevents stars
 * being forced into regular polygons).
 */
GSList *
build_rectified_list (GSList *list_inp,
                      gboolean close_path,
                      gdouble pixel_tollerance)
{
  GSList *ret_list = (GSList *) NULL;
  if (! close_path || g_slist_length (list_inp) <= 3)
    {
      g_debug ("straightening");
      /* Try to make straighten. */
      ret_list = straighten (list_inp);
      return ret_list;
    }
  guint length = g_slist_length (list_inp);
  guint i      = 0;

  /* Copy the input list. */
  for (i = 0; i < length; i++)
    {
      AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list_inp, i);

      AnnotatePoint *point_copy = allocate_point (point->x,
                                                  point->y,
                                                  point->width,
                                                  point->pressure);

      ret_list = g_slist_prepend (ret_list, point_copy);
    }

  /* I reverse the list to preserve the initial order. */
  ret_list = g_slist_reverse (ret_list);

  /* It is similar to regular a polygon. */
  if (is_similar_to_a_regular_polygon (ret_list, pixel_tollerance))
    {
      g_debug ("extracted as polygon");
      ret_list = extract_polygon (ret_list);
      return ret_list;
    }
  ret_list      = straighten (ret_list);
  guint npoints = g_slist_length (ret_list);
  if (is_a_rectangle (ret_list, pixel_tollerance))
    {
      /* It is a rectangle. */
      GSList *rect_list = build_outbounded_rectangle (ret_list);
      g_slist_foreach (ret_list, (GFunc) g_free, NULL);
      g_slist_free (ret_list);
      ret_list = rect_list;
      return ret_list;
    }
  if (is_a_triangle (ret_list, pixel_tollerance))
    {
      g_debug ("straightening triangle");
      ret_list = straighten (ret_list);
      return ret_list;
    }
  if (npoints > 8 && ! is_polygon_convex (ret_list))
    {
      // circle time
      ret_list = extract_polygon (ret_list);
      return ret_list;
    }
  return ret_list;
}

/**
 * broken:
 * @list_inp: a GSList of AnnotatePoint representing the original path
 * @close_path: whether the path should be considered closed
 * @rectify: whether to further smooth/rectify the meaningful points
 * @pixel_tollerance: tolerance in pixels for determining meaningful points
 *
 * Processes a list of points and returns a "recognized" or simplified path.
 *
 * Steps:
 * -  Calls build_meaningful_point_list() to extract only the points that
 * contribute significantly to the path shape (removes small deviations).
 * -  If @rectify is TRUE, calls build_rectified_list() on the meaningful points
 * to further smooth or regularize the path.
 * -  Frees the intermediate meaningful points list if rectification is applied.
 * -  Returns the processed list of points, either rectified or just the
 * meaningful points.
 *
 * Returns:
 * - A GSList of AnnotatePoint representing the simplified/smoothed path.
 * - The caller is responsible for freeing the returned list.
 */
GSList *
broken (GSList *list_inp,
        gboolean close_path,
        gboolean rectify,
        gdouble pixel_tollerance)
{
  GSList *meaningful_points = build_meaningful_point_list (list_inp,
                                                           pixel_tollerance);

  if (meaningful_points && rectify)
    {
        GSList *rectified_list = build_rectified_list (meaningful_points,
                                                       close_path,
                                                       pixel_tollerance);

      /* Free the meaningful_point_list after it's been processed. */
      g_slist_foreach (meaningful_points, (GFunc) g_free, NULL);
      g_slist_free (meaningful_points);

      return rectified_list;
    }

  return meaningful_points;
}
