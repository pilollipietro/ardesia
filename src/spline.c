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
 */

#include "spline.h"
#include "annotation_window.h"

/**
 * ctrl_coord:
 * @a, @b, @c: coordinates used to compute a Catmull–Rom Bézier control point
 * @f: scaling factor for the tangent
 *
 * Computes a single coordinate for a Bézier control point:
 *      a + f * (b - c)
 *
 * Used internally for both CP1 and CP2 of spline segments.
 *
 * Returns: the computed coordinate.
 */
static inline gdouble
ctrl_coord (gdouble a, gdouble b, gdouble c, gdouble f)
{
  return a + f * (b - c);
}

/**
 * make_segment:
 * @p: (transfer none): the endpoint of the Bézier segment (borrowed reference)
 * @cp1_x: X coordinate of the first control point
 * @cp1_y: Y coordinate of the first control point
 * @cp2_x: X coordinate of the second control point
 * @cp2_y: Y coordinate of the second control point
 *
 * Allocates and initializes a #SplineSegment structure. The segment contains:
 *
 *   • @p      — the Bézier endpoint (B3), borrowed from the input list
 *   • @cp1_x,
 *     @cp1_y  — first Bézier control point (CP1)
 *   • @cp2_x,
 *     @cp2_y  — second Bézier control point (CP2)
 *
 * The caller takes ownership of the returned #SplineSegment and must free it
 * (typically via spline_result_free()).
 *
 * Note: @p is *not* owned by the segment and must not be freed here.
 *
 * Returns: (transfer full): a newly allocated #SplineSegment.
 */
static SplineSegment *
make_segment (AnnotatePoint *p,
              double cp1_x, double cp1_y,
              double cp2_x, double cp2_y)
{
  SplineSegment *s = g_new0 (SplineSegment, 1);
  s->p             = p;
  s->cp1.x         = cp1_x;
  s->cp1.y         = cp1_y;
  s->cp2.x         = cp2_x;
  s->cp2.y         = cp2_y;
  return s;
}

/**
 * spline:
 * @points: (element-type AnnotatePoint): a #GSList of input points defining
 * the polyline to be interpolated, ordered from the oldest point to
 * the most recent.
 *
 * Generates a smooth curve from a list of points using a Catmull–Rom spline.
 * The function returns a #SplineResult structure containing:
 *
 *  • @start_point —  @start_point — first point of the spline (borrowed
 *    reference to the most recent input point).
 *
 *  • @segments — a #GSList of #SplineSegment structures; each segment holds:
 *      - seg->cp1: the first Bézier control point (CP1)
 *      - seg->cp2: the second Bézier control point (CP2)
 *      - seg->p  : the endpoint of the cubic Bézier curve (B3)
 *
 *    Segments are prepended during construction, so the list of segments
 *    is reversed relative to the iteration over the input points. As a result,
 *    @segments ordered from the most recent input point to the oldest point.
 *
 * The returned spline is suitable for drawing directly with
 * cairo_curve_to(), using the pattern:
 *
 *      seg->cp1, seg->cp2, seg->p
 *
 * Memory ownership:
 *  • The caller takes ownership of the returned #SplineResult and must free it
 *    using spline_result_free().
 *
 *  • Neither @start_point nor any segment endpoint (seg->p) is freed by
 *    spline_result_free(), because these are borrowed references to points
 *    from the caller-owned @points list.
 *
 * Returns: (transfer full): a newly allocated #SplineResult, or %NULL if
 * @points has fewer than two elements.
 */
SplineResult *
spline (GSList *points)
{
  g_assert (gslist_has_at_least (points, 2));

  gdouble tau    = 0.0;
  gdouble factor = (1.0 - tau) / 6.0;

  GSList       *ret    = NULL;
  GSList       *node   = points;
  GSList       *prev   = NULL;
  SplineResult *result = g_new0 (SplineResult, 1);

  /* prepare a "phantom" point before the first real point for
   * tangent calculation */

  AnnotatePoint *first_real  = node->data;
  AnnotatePoint *second_real = node->next->data;

  AnnotatePoint *pre_first = allocate_point (
      first_real->x - (second_real->x - first_real->x),
      first_real->y - (second_real->y - first_real->y),
      first_real->width,
      first_real->pressure);

  AnnotatePoint *post_last = NULL;
  AnnotatePoint *last_real = NULL;

  /* iterate over the list once to build segments and track last nodes */
  while (node)
    {
      AnnotatePoint *Pi  = node->data;
      AnnotatePoint *Pi1 = node->next ? node->next->data : NULL;

      /* find previous and next points for Catmull-Rom control calculation */
      AnnotatePoint *Pp = prev ? prev->data : pre_first;
      AnnotatePoint *Pn;
      Pn = Pi1 && node->next->next ? node->next->next->data : NULL;

      if (! Pn && Pi1)
        {
          /* last segment: create a "phantom" post_last point
           * to maintain smooth tangent
           */
          post_last = allocate_point (Pi1->x + (Pi1->x - Pi->x),
                                      Pi1->y + (Pi1->y - Pi->y),
                                      Pi1->width, Pi1->pressure);
          Pn        = post_last;
        }

      if (Pi1)
        {
          /* compute Bézier control points */
          gdouble cp1_x = Pi->x + factor * (Pi1->x - Pp->x);
          gdouble cp1_y = Pi->y + factor * (Pi1->y - Pp->y);

          gdouble cp2_x = Pi1->x - factor * (Pn->x - Pi->x);
          gdouble cp2_y = Pi1->y - factor * (Pn->y - Pi->y);

          /* NOTE:
           * Segments are stored in reverse (Pi1 → Pi), so CPs are
           * intentionally swapped when calling make_segment().
           * Do NOT reorder them.
           */
          SplineSegment *seg = make_segment (Pi, cp2_x, cp2_y, cp1_x, cp1_y);
          ret                = g_slist_prepend (ret, seg);
        }

      prev = node;
      node = node->next;
    }

  /* after loop, prev node points to the last real node */
  last_real = prev->data;

  if (pre_first)
    {
      g_free (pre_first);
    }
  if (post_last)
    {
      g_free (post_last);
    }

  result->start_point = last_real;
  result->segments    = ret;

  return result;
}

/**
 * spline_result_free:
 * @res: (transfer full): a #SplineResult returned by spline()
 *
 * Frees a #SplineResult structure previously allocated by spline().
 *
 * The function frees:
 * - all #SplineSegment structures stored in @res->segments
 * - the GSList that contains those segments
 * - the #SplineResult structure itself
 *
 * Important:
 * - `seg->p` (the endpoint of each segment) is **not** freed, because it
 *   refers to one of the original input points, owned by the caller.
 * - `res->start_point` is also **not** freed for the same reason: it is a
 *   borrowed reference to the first point of the input list.
 *
 * Does nothing if @res is %NULL.
 */
void
spline_result_free (SplineResult *res)
{
  if (! res)
    return;

  for (GSList *l = res->segments; l != NULL; l = l->next)
    {
      SplineSegment *seg = l->data;

      if (seg)
        {
          g_free (seg);
        }
    }

  g_slist_free (res->segments);
  g_free (res);
}

/**
 * spline_coord_list:
 * @res: (transfer none): a #SplineResult returned by spline()
 *
 * Converts a #SplineResult into a plain #GSList of points representing
 * the spline coordinates (P0, P1, P2, ...), copying the points.
 *
 * The list is constructed in reverse order of segment processing (most recent
 * point first, P_last), which is the standard chronological order for
 * the device coordinate list.
 *
 * Memory ownership:
 * - The caller takes ownership of the returned #GSList container and all
 * the #AnnotatePoint elements within it, created via g_memdup2.
 *
 * Returns: (transfer full): a new #GSList of copied AnnotatePoint elements.
 */
GSList *
spline_coord_list (SplineResult *res)
{
  if (! res)
    return NULL;

  GSList *new_list = NULL;

  if (res->start_point)
    {
      AnnotatePoint *p0_copy;
      p0_copy = (AnnotatePoint *) g_memdup2 (res->start_point,
                                             sizeof (AnnotatePoint));

      new_list = g_slist_prepend (new_list, p0_copy);
    }

  for (GSList *l = res->segments; l != NULL; l = l->next)
    {
      SplineSegment *seg = (SplineSegment *) l->data;

      if (! seg || ! seg->p)
        continue;

      AnnotatePoint *p_end_copy;
      p_end_copy = (AnnotatePoint *) g_memdup2 (seg->p,
                                                sizeof (AnnotatePoint));

      new_list = g_slist_prepend (new_list, p_end_copy);
    }
  return g_slist_reverse (new_list);
}
