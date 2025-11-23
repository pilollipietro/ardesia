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
 * found_min_and_max:
 * @list: a #GSList of #AnnotatePoint elements.
 * @minx: (out): pointer to store the minimum X coordinate.
 * @miny: (out): pointer to store the minimum Y coordinate.
 * @maxx: (out): pointer to store the maximum X coordinate.
 * @maxy: (out): pointer to store the maximum Y coordinate.
 *
 * Iterates over a list of points to find the bounding coordinates of the set.
 * After the function returns, @minx/@miny contain the coordinates of the
 * top-left corner, and @maxx/@maxy contain the coordinates of the
 * bottom-right corner of the point set.
 *
 * This is useful for generating bounding boxes, rectangles, ellipses, or
 * for any operation that needs the spatial extent of the points.
 *
 * Note: the function assumes @list contains at least one point.
 */
static void
found_min_and_max (GSList *list,
                   gdouble *minx,
                   gdouble *miny,
                   gdouble *maxx,
                   gdouble *maxy)
{
  GSList        *node;
  AnnotatePoint *first_point;
  AnnotatePoint *cur_point;

  if (list == NULL)
    return;

  /* Initialize the min and max to the first point coordinates. */
  first_point = (AnnotatePoint *) list->data;
  *minx       = first_point->x;
  *miny       = first_point->y;
  *maxx       = first_point->x;
  *maxy       = first_point->y;

  /* Search the min and max coordinates using O(N) pointer traversal. */
  node = list->next;
  while (node)
    {
      cur_point = (AnnotatePoint *) node->data;
      *minx     = MIN (*minx, cur_point->x);
      *miny     = MIN (*miny, cur_point->y);
      *maxx     = MAX (*maxx, cur_point->x);
      *maxy     = MAX (*maxy, cur_point->y);

      node = node->next;
    }
}

/**
 * calculate_medium_pression:
 * @list: a #GSList of #AnnotatePoint elements.
 *
 * Computes the average pressure of all points in the list.
 *
 * Returns: the mean pressure as a gdouble. If the list is empty, returns 1.0.
 */
static gdouble
calculate_medium_pression (GSList *list)
{
  gdouble total_pressure = 0.0;
  guint   length         = 0;

  if (list == NULL)
    return 1.0;

  for (GSList *node = list; node != NULL; node = node->next)
    {
      AnnotatePoint *cur_point = (AnnotatePoint *) node->data;
      total_pressure += cur_point->pressure;
      length++;
    }

  if (length == 0)
    return 1.0;

  return total_pressure / length;
}

/**
 * is_similar_to_a_regular_polygon:
 * @list: a #GSList of #AnnotatePoint elements representing a closed path.
 * @pixel_tolerance: stroke thickness / drawing noise tolerance in pixels.
 *
 * Determines whether a hand-drawn polygon can be approximated as a regular
 * polygon by analyzing the uniformity of its side lengths.
 *
 * The input path is assumed to be closed, meaning that the first and last
 * points coincide (P0 == Pn). Therefore, a polygon with N sides is
 * represented by N+1 points.
 *
 * The algorithm is based on the coefficient of variation:
 *
 *   CV = σ / μ
 *
 * where μ is the mean side length and σ is the standard deviation.
 * This makes the metric scale-invariant and robust to resizing.
 *
 * Since the domain is freehand drawing, the threshold is dynamically
 * adjusted using:
 *
 *  - the number of sides (few sides → stricter, many sides → looser)
 *  - the stroke thickness / drawing noise (pixel_tolerance relative to μ)
 *
 * This compensates for geometric noise introduced by hand-drawn input
 * and avoids misclassifying elongated rectangles or distorted shapes
 * as regular polygons.
 *
 * Returns: %TRUE if the polygon is sufficiently close to a regular one,
 *          %FALSE otherwise.
 */
static gboolean
is_similar_to_a_regular_polygon (GSList  *list,
                                 gdouble  pixel_tolerance)
{
  if (! gslist_has_at_least (list, 6))
    {
      return FALSE;
    }

  /* Base threshold for coefficient of variation */
  const gdouble cv_threshold = 0.10;

  guint   num_sides = 0;
  gdouble sum       = 0.0;
  gdouble sum_sq    = 0.0;

  AnnotatePoint *first = (AnnotatePoint *) list->data;
  AnnotatePoint *prev  = first;

  /* Calculate sum and sum of squares for all sides including closing edge */
  for (GSList *node = list->next; node; node = node->next)
    {
      AnnotatePoint *curr = (AnnotatePoint *) node->data;

      gdouble dist = get_distance (prev->x, prev->y, curr->x, curr->y);

      sum += dist;
      sum_sq += dist * dist;
      num_sides++;

      prev = curr;
    }

  if (num_sides < 3)
    return FALSE;

  gdouble mean     = sum / num_sides;
  gdouble variance = (sum_sq / num_sides) - (mean * mean);

  if (variance < 0.0)
    variance = 0.0;

  gdouble std_dev = sqrt (variance);

  /* Coefficient of variation */
  gdouble cv = std_dev / mean;

  /* ---- Your fixes for the real problem ---- */

  /* Normalized noise due to stroke thickness */
  gdouble noise_ratio = pixel_tolerance / mean;

  /* Make tolerance stricter for few sides, looser for many sides */
  gdouble side_factor = 1.0 + (3.0 / num_sides);

  /* Adjust the threshold dynamically */
  gdouble adjusted_threshold = cv_threshold
                               * side_factor
                               * (1.0 + noise_ratio);

  return cv < adjusted_threshold;
}

/**
 * build_polygon:
 * @list: a #GSList of #AnnotatePoint elements representing a closed path.
 *
 * Creates a new list of points approximating a regular polygon derived
 * from the shape of the input path.
 *
 * The function computes the bounding box of the input points to estimate
 * the center and an effective radius (calculated as the average of the
 * semi-axes of the bounding box). Vertices are then placed uniformly
 * around a circle derived from this approximation.
 *
 * The function assumes that @list represents a closed path, where the
 * last point duplicates the first one. The closing point is explicitly
 * re-added so that the resulting list is also closed.
 *
 * The resulting list is built in reverse order due to the use of
 * g_slist_prepend().
 *
 * The input list is not modified. A new #GSList is allocated, and each
 * point inside it is a deep copy with updated coordinates. Width and
 * pressure fields are preserved from the original points.
 *
 * Returns: (transfer full): a newly allocated #GSList containing the
 *          points of the approximated regular polygon. The caller owns
 *          the list and must free both the nodes and the contained
 *          #AnnotatePoint structures.
 */
static GSList *
build_polygon (GSList *list)
{
  g_assert (gslist_has_at_least (list, 6));

  guint length = g_slist_length (list);

  gdouble minx = -1, miny = -1, maxx = -1, maxy = -1;
  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  gdouble cx     = (maxx + minx) / 2;
  gdouble cy     = (maxy + miny) / 2;
  gdouble radius = ((maxx - minx) + (maxy - miny)) / 4;

  /* Start angle: M_PI/2 puts first vertex at top.
   * Offset by half-step to ensure:
   * - Odd polygons (triangle, pentagon): point at top
   * - Even polygons (square, hexagon): flat edge at top
   */
  const gdouble angle_step = 2 * M_PI / (length - 1);
  gdouble       angle      = M_PI / 2 + angle_step / 2;

  GSList *out  = NULL;
  GSList *node = list;

  AnnotatePoint *first = NULL;
  for (guint i = 0; i < length - 1; i++)
    {
      gdouble x = radius * cos (angle) + cx;
      gdouble y = radius * sin (angle) + cy;

      AnnotatePoint *orig = (AnnotatePoint *) node->data;

      AnnotatePoint *p = allocate_point (x, y, orig->width, orig->pressure);
      if (i == 0)
        {
          first = p;
        }
      out = g_slist_prepend (out, p);

      angle += angle_step;
      node = node->next;
    }

  AnnotatePoint *last = allocate_point (first->x,
                                        first->y,
                                        first->width,
                                        first->pressure);

  out = g_slist_prepend (out, last);

  return out;
}

/**
 * straighten:
 * @list: a GSList of AnnotatePoint* representing a drawn path.
 * @close_path: if TRUE, also snaps first and last points if similar.
 * @threshold: distance in pixels to consider coordinates similar.
 *
 * Aligns points that have similar X or Y coordinates within a threshold.
 * Uses a linear scan to snap each coordinate to previously seen
 * canonical values.
 *
 * Returns: (transfer full): a new GSList of AnnotatePoint* representing
 * the straightened path.
 */
GSList *
straighten (GSList *list, gboolean close_path, gdouble threshold)
{
  if (! list)
    return NULL;

  GSList        *result      = NULL;
  AnnotatePoint *first_point = NULL;
  AnnotatePoint *last_point  = NULL;

  GArray *canonical_x = g_array_new (FALSE, FALSE, sizeof (gdouble));
  GArray *canonical_y = g_array_new (FALSE, FALSE, sizeof (gdouble));

  for (GSList *l = list; l; l = l->next)
    {
      AnnotatePoint *orig = (AnnotatePoint *) l->data;
      gdouble        x    = orig->x;
      gdouble        y    = orig->y;

      /* Snap X to canonical value if similar */
      gboolean found_x = FALSE;
      for (guint i = 0; i < canonical_x->len; i++)
        {
          gdouble canon = g_array_index (canonical_x, gdouble, i);
          if (fabs (x - canon) <= threshold)
            {
              x       = canon;
              found_x = TRUE;
              break;
            }
        }
      if (! found_x)
        g_array_append_val (canonical_x, x);

      /* Snap Y to canonical value if similar */
      gboolean found_y = FALSE;
      for (guint i = 0; i < canonical_y->len; i++)
        {
          gdouble canon = g_array_index (canonical_y, gdouble, i);
          if (fabs (y - canon) <= threshold)
            {
              y       = canon;
              found_y = TRUE;
              break;
            }
        }
      if (! found_y)
        g_array_append_val (canonical_y, y);

      AnnotatePoint *copy = allocate_point (x, y, orig->width, orig->pressure);
      result              = g_slist_prepend (result, copy);

      if (! first_point)
        first_point = copy;
      last_point = copy;
    }

  g_array_free (canonical_x, TRUE);
  g_array_free (canonical_y, TRUE);

  /* Snap first and last points together if close_path is enabled */
  if (close_path && first_point && last_point && first_point != last_point)
    {
      if (fabs (first_point->x - last_point->x) <= threshold)
        {
          gdouble avg_x  = (first_point->x + last_point->x) / 2;
          first_point->x = avg_x;
          last_point->x  = avg_x;
        }

      if (fabs (first_point->y - last_point->y) <= threshold)
        {
          gdouble avg_y  = (first_point->y + last_point->y) / 2;
          first_point->y = avg_y;
          last_point->y  = avg_y;
        }
    }

  return result;
}

/**
 * simplify_douglas_peucker:
 * @points: a GSList of AnnotatePoint* representing a polyline
 * @epsilon: distance threshold in pixels for simplification
 *
 * Simplifies a polyline using the Douglas-Peucker algorithm.
 * Points that deviate less than @epsilon from the line connecting
 * their neighbors are discarded, while significant points are retained.
 * This reduces the number of points while preserving the overall shape.
 *
 * Returns: (transfer full): a new GSList containing deep-copied points
 *          representing the simplified path. The caller owns the list.
 *
 * Notes:
 * - The original list is not modified.
 * - Useful for performance optimization or smoothing noisy input.
 */
GSList *
simplify_douglas_peucker (GSList *points, gdouble epsilon)
{
  g_assert (gslist_has_at_least (points, 4));
  guint n = g_slist_length (points);

  AnnotatePoint **arr  = g_new (AnnotatePoint *, n);
  GSList         *iter = points;
  for (guint i = 0; i < n; i++, iter = iter->next)
    {
      arr[i] = (AnnotatePoint *) iter->data;
    }

  gboolean *keep = g_new0 (gboolean, n);
  keep[0]        = TRUE;
  keep[n - 1]    = TRUE;

  typedef struct
  {
    guint start, end;
  } Interval;

  Interval *stack = g_malloc (sizeof (Interval) * n * 2);
  int       top   = 0;
  stack[top++]    = (Interval){ 0, n - 1 };

  while (top > 0)
    {
      Interval cur   = stack[--top];
      guint    start = cur.start;
      guint    end   = cur.end;
      if (end <= start + 1)
        continue;

      AnnotatePoint *p0 = arr[start];
      AnnotatePoint *p1 = arr[end];

      gdouble max_dist = -1.0;
      guint   pivot    = start;

      for (guint i = start + 1; i < end; i++)
        {
          AnnotatePoint *pi   = arr[i];
          gdouble        dx   = p1->x - p0->x;
          gdouble        dy   = p1->y - p0->y;
          gdouble        len2 = dx * dx + dy * dy;
          gdouble        d;
          if (len2 == 0)
            d = sqrt ((pi->x - p0->x) * (pi->x - p0->x) +
                      (pi->y - p0->y) * (pi->y - p0->y));
          else
            d = fabs (dy * (pi->x - p0->x) - dx * (pi->y - p0->y)) /
                sqrt (len2);

          if (d > max_dist)
            {
              max_dist = d;
              pivot    = i;
            }
        }

      if (max_dist > epsilon)
        {
          keep[pivot]  = TRUE;
          stack[top++] = (Interval){ start, pivot };
          stack[top++] = (Interval){ pivot, end };
        }
    }

  GSList *out = NULL;
  for (guint i = 0; i < n; i++)
    {
      if (keep[i])
        {
          out = g_slist_prepend (out,
                                 g_memdup2 (arr[i], sizeof (AnnotatePoint)));
        }
    }

  g_free (arr);
  g_free (keep);
  g_free (stack);
  return out;
}

/**
 * build_meaningful_point_list:
 * @list_inp: a GSList of AnnotatePoint* representing a stroke or path
 * @pixel_tolerance: threshold in pixels for determining meaningful deviation
 *
 * Filters the input list to retain only points that contribute
 * significant visual change to the path. Simplification is performed
 * using a curvature-based approach (similar to Douglas-Peucker).
 *
 * The function also normalizes the pressure of all retained points
 * to the average pressure of the original path, ensuring consistent
 * stroke width.
 *
 * Returns: (transfer full): a new GSList of AnnotatePoint* containing only
 *          meaningful points. The caller owns the list.
 *
 * Notes:
 * - If the input list has fewer than three points, a deep copy of the
 *   original list is returned without simplification.
 */
GSList *
build_meaningful_point_list (GSList *list_inp,
                             gdouble pixel_tolerance)
{
  g_assert (gslist_has_at_least (list_inp, 4));
  GSList *simplified_list;

  simplified_list = simplify_douglas_peucker (list_inp, pixel_tolerance);

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
  assert (gslist_has_at_least (list, 2));

  AnnotatePoint *point    = (AnnotatePoint *) list->next->data;
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

  /*
   * Add a duplicate of the first corner to close the rectangle path.
   * This ensures that drawing routines treating the points as a closed
   * polygon will render a complete rectangle without gaps.
   */
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
 * @pixel_tolerance: a tolerance threshold in pixels
 *
 * Determines whether the path described by @list approximates an ellipse.
 * An ellipse is defined geometrically as a set of points for which the sum
 * of distances to two foci is constant.
 *
 * Algorithm:
 * Compute the bounding box of the path (minx, miny, maxx, maxy).
 * Compute the semi-axes a (horizontal) and b (vertical) of the ellipse.
 * Compute focal distance c = sqrt(|a^2 - b^2|) and the coordinates of
 *    the two foci depending on orientation.
 * Compute the sum of distances from the first point to both foci (ideal sum).
 * Iterate over all points and check if the deviation from the ideal sum
 *    exceeds the tolerance.
 * Return FALSE if any point violates the tolerance, TRUE otherwise.
 *
 * Notes:
 * - Tolerance is increased proportionally to the average semi-axis length
 *   to allow minor drawing inaccuracies in larger shapes.
 * - Uses Euclidean distance for calculations (get_distance function).
 *
 * Returns: %TRUE if the path approximates an ellipse within the given
 *          tolerance,
 *          %FALSE otherwise.
 */
gboolean
is_similar_to_an_ellipse (GSList *list, gdouble pixel_tolerance)
{
  if (! gslist_has_at_least (list, 5))
    {
      return FALSE;
    }

  gdouble minx      = 0;
  gdouble miny      = 0;
  gdouble maxx      = 0;
  gdouble maxy      = 0;
  gdouble tolerance = 0;

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

  found_min_and_max (list, &minx, &miny, &maxx, &maxy);

  a = (maxx - minx) / 2;
  b = (maxy - miny) / 2;

  aq = pow (a, 2);
  bq = pow (b, 2);

  /*
   * Increase tolerance proportionally to the size of the ellipse
   * (average of semi-axes a and b) to allow for minor drawing inaccuracies
   * in larger shapes.
   */
  tolerance = pixel_tolerance + (a + b) / 2;

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

  gdouble sum;
  gdouble difference;

  for (GSList *node = list; node != NULL; node = node->next)
    {
      AnnotatePoint *point = (AnnotatePoint *) node->data;

      gdouble distancef1 = get_distance (point->x, point->y, f1x, f1y);
      gdouble distancef2 = get_distance (point->x, point->y, f2x, f2y);
      sum                = distancef1 + distancef2;
      difference         = fabs (sum - sump1);

      if (difference > tolerance)
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
 * @pixel_tolerance: tolerance in pixels used by detectors / simplification
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
                      gdouble pixel_tolerance)
{ 
  /* It is similar to regular a polygon. */
  if (close_path &&
      is_similar_to_a_regular_polygon (list_inp, pixel_tolerance))
    {
      g_debug ("polygon detected");
      return build_polygon (list_inp);
    }
  return straighten (list_inp, close_path, pixel_tolerance);
}

/**
 * broken:
 * @list_inp: a GSList of AnnotatePoint representing the original path
 * @close_path: whether the path should be considered closed
 * @rectify: whether to further smooth/rectify the meaningful points
 * @pixel_tolerance: threshold in pixels for determining meaningful deviation
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
        gdouble pixel_tolerance)
{
  GSList *meaningful_points = build_meaningful_point_list (list_inp,
                                                           pixel_tolerance);

  if (meaningful_points && rectify)
    {
      GSList *rectified_list = build_rectified_list (meaningful_points,
                                                     close_path,
                                                     pixel_tolerance);

      /* Free the meaningful_point_list after it's been processed. */
      g_slist_foreach (meaningful_points, (GFunc) g_free, NULL);
      g_slist_free (meaningful_points);

      return rectified_list;
    }

  return meaningful_points;
}
