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

#include "annotation_config.h"
#include "annotation_window.h"
#include "annotation_window_callbacks.h"
#include "background_window.h"
#include "bar.h"
#include "bar_callbacks.h"
#include "broken.h"
#include "cairo_functions.h"
#include "cursors.h"
#include "fill.h"
#include "font_config.h"
#include "input.h"
#include "iwb_loader.h"
#include "spline.h"
#include "text_window.h"
#include "utils.h"

#ifdef _WIN32
#include "windows_utils.h"
#endif

/* Internal data for the annotation window. */
AnnotateData *annotation_data;

/**
 * @brief Data structure passed to the savepoint background thread.
 * Contains the surface copy and filename for the save operation.
 */
typedef struct
{
  cairo_surface_t *surface_copy;
  gchar           *filename;
} SavepointThreadData;

/**
 * savepoint_thread_data_free:
 * @task_data: A pointer to a #SavepointThreadData struct.
 *
 * GDestroyNotify callback to free the thread data structure and its
 * contents (the surface copy and the duplicated filename).
 * This runs in the main thread *after* the worker thread is complete.
 */
static void
savepoint_thread_data_free (gpointer task_data)
{
  SavepointThreadData *data = (SavepointThreadData *) task_data;
  if (data)
    {
      if (data->surface_copy)
        {
          cairo_surface_destroy (data->surface_copy);
        }
      g_free (data->filename);
      g_free (data);
    }
}

/**
 * savepoint_worker_thread:
 * @task: The GTask executing this operation.
 * @source_object: (unused)
 * @task_data: A pointer to a #SavepointThreadData struct.
 * @cancellable: (unused)
 *
 * This function runs in a background thread (GTask).
 * It performs a single slow I/O operation: saving the surface as a PNG file.
 */
static void
savepoint_worker_thread (GTask *task,
                         gpointer source_object,
                         gpointer task_data,
                         GCancellable *cancellable)
{
  SavepointThreadData *data = (SavepointThreadData *) task_data;
  cairo_status_t       status;

  /* This is the slow I/O operation */
  status = cairo_surface_write_to_png (data->surface_copy, data->filename);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_warning ("(Thread) Failed to write savepoint PNG %s: %s",
                 data->filename, cairo_status_to_string (status));
    }
  else
    {
      g_debug ("(Thread) Savepoint stored in file: %s", data->filename);
    }
}

/**
 * get_pressure:
 * @ev: The #GdkEvent from which to extract the axis value.
 *
 * Extracts the pressure value from a GdkEvent, typically from a stylus
 * or touch device.
 *
 * This function queries the event for the %GDK_AXIS_PRESSURE value. If the
 * axis is available and successfully read, its value (usually between
 * 0.0 and 1.0) is returned.
 *
 * If the device does not support pressure or the axis value cannot be
 * read, the function safely returns a default value of 1.0, representing
 * full pressure.
 *
 * Returns: A gdouble representing the pressure, or 1.0 as a fallback.
 */
static gdouble
get_pressure (GdkEvent *ev)
{
  gdouble ret_value = 1.0;
  gdouble pressure  = ret_value;

  gboolean ret = gdk_event_get_axis (ev, GDK_AXIS_PRESSURE, &pressure);

  if (ret)
    {
      ret_value = pressure;
    }

  return ret_value;
}

/**
 * annotate_paint_context_new:
 * @type: The #AnnotatePaintType to assign to the new context.
 *
 * A simple constructor for creating a new #AnnotatePaintContext.
 *
 * This function allocates a new #AnnotatePaintContext structure and
 * initializes its `type` field with the provided value.
 *
 * Returns: (transfer full): A pointer to the newly allocated
 * #AnnotatePaintContext. The caller is responsible for freeing
 * this memory with g_free() when it is no longer needed.
 */
static AnnotatePaintContext *
annotate_paint_context_new (AnnotatePaintType type)
{
  AnnotatePaintContext *context = (AnnotatePaintContext *) NULL;
  context       = g_malloc ((gsize) sizeof (AnnotatePaintContext));
  context->type = type;

  return context;
}

/**
 * annotate_get_arrow_direction:
 * @devdata: The #AnnotateDeviceData containing the coordinate list.
 *
 * Calculates the direction of the end of a stroke for drawing an arrowhead.
 *
 * This function simplifies the user's drawn path into a temporary list of
 * significant points using build_meaningful_point_list(). It then
 * calculates the angle between the last two points of this simplified list.
 *
 * The temporary list is freed before the function returns. If the simplified
 * path contains fewer than two points, a direction cannot be determined,
 * and the function returns 0.0.
 *
 * Returns: A gdouble representing the direction of the stroke end in radians,
 * or 0.0 if a direction cannot be determined.
 */
static gdouble
annotate_get_arrow_direction (AnnotateDeviceData *devdata)
{
  GSList *list = devdata->coord_list;
  if (g_slist_length (list) < 2)
    {
      return 0.0;
    }

  AnnotatePoint *last_point = (AnnotatePoint *) list->data;

  /* meaningfull point at least half the line's thickness away. */
  gdouble min_distance = annotate_get_thickness () / 2.0;
  if (min_distance < 5.0)
    min_distance = 5.0;

  AnnotatePoint *old_point = NULL;
  GSList        *iter      = list->next;

  while (iter)
    {
      old_point = (AnnotatePoint *) iter->data;

      /* is the point far enough away? */
      gdouble distance = get_distance (last_point->x, last_point->y,
                                       old_point->x, old_point->y);
      if (distance > min_distance)
        {
          break;
        }

      iter = g_slist_next (iter);
    }

  if (old_point == NULL)
    {
      old_point = (AnnotatePoint *) g_slist_nth_data (list, 1);
    }

  /* Give the direction using the last two significant points. */
  return atan2 (last_point->y - old_point->y, last_point->x - old_point->x);
}

/**
 * select_color:
 *
 * Sets the appropriate Cairo source and operator based on the current tool.
 *
 * This is a helper function that configures the global Cairo context for
 * either drawing or erasing.
 *
 * If the current tool is not the eraser (e.g., pen, highlighter), it sets
 * the drawing operator to %CAIRO_OPERATOR_SOURCE and applies the color
 * stored in `annotation_data->color`.
 *
 * If the current tool is the eraser, it sets the operator to
 * %CAIRO_OPERATOR_CLEAR, which makes subsequent drawing operations erase
 * content by setting the alpha channel to 0.
 */
static void
select_color (void)
{
  cairo_t *annotation_cairo_context;
  annotation_cairo_context = annotation_data->annotation_cairo_context;

  if (! annotation_cairo_context)
    {
      return;
    }

  if (annotation_data->cur_context)
    {
      /* Pen or arrow tool. */
      if (annotation_data->cur_context->type != ANNOTATE_ERASER)
        {
          /* Select the color. */
          if (annotation_data->color)
            {
              g_debug ("Select color %s\n", annotation_data->color);
              cairo_set_source_rgba (annotation_cairo_context,
                                     (gdouble) annotation_data->r / 255.0,
                                     (gdouble) annotation_data->g / 255.0,
                                     (gdouble) annotation_data->b / 255.0,
                                     (gdouble) annotation_data->a / 255.0);
            }

          cairo_set_operator (annotation_cairo_context, CAIRO_OPERATOR_SOURCE);
        }
      else
        {
          /* It is the eraser tool. */
          g_debug ("Select transparent color to erase\n");
          cairo_set_operator (annotation_cairo_context, CAIRO_OPERATOR_CLEAR);
        }
    }
}

#ifdef _WIN32

/**
 * annotate_acquire_pointer_grab:
 *
 * Acquire the input pointer grab for the annotation window.
 *
 * This function calls `grab_pointer()` on the annotation window with
 * `GDK_ALL_EVENTS_MASK`, allowing the annotation system to capture
 * all input events (mouse, stylus, etc.) until the grab is released.
 *
 * It is used to ensure that all input events are directed to the
 * annotation window during interactive operations.
 **/
static void
annotate_acquire_pointer_grab (void)
{
  grab_pointer (annotation_data->annotation_window, GDK_ALL_EVENTS_MASK);
}

/**
 * annotate_release_pointer_grab:
 *
 * Releases the input pointer grab from the annotation window (Windows only).
 *
 * This function is a platform-specific helper for Windows that releases
 * a previously acquired pointer grab, allowing input events to be
 * processed by other windows.
 */
static void
annotate_release_pointer_grab (void)
{
  ungrab_pointer ();
}

#endif

/**
 * update_cursor:
 *
 * Applies the currently selected cursor to the annotation window.
 *
 * This function sets the cursor for the annotation window's underlying
 * GdkWindow to the one stored in `annotation_data->cursor`.
 *
 * On Windows, it includes a workaround that briefly releases and re-acquires
 * the pointer grab, which can be necessary to ensure the cursor update is
 * visually applied correctly while a grab is active.
 */
static void
update_cursor (void)
{
  GtkWidget *annotation_window = annotation_data->annotation_window;
  if (! annotation_window)
    {
      return;
    }

#ifdef _WIN32
  annotate_release_pointer_grab ();
#endif

  gdk_window_set_cursor (gtk_widget_get_window (annotation_window),
                         annotation_data->cursor);

#ifdef _WIN32
  annotate_acquire_pointer_grab ();
#endif
}

/**
 * disallocate_cursor:
 *
 * Safely unreferences the global cursor object.
 *
 * This function checks if a cursor is currently allocated in the global
 * annotation data. If it exists, it unreferences the GdkCursor object
 * and sets the pointer to NULL to prevent dangling pointers.
 */
static void
disallocate_cursor (void)
{
  if (annotation_data->cursor)
    {
      g_object_unref (annotation_data->cursor);
      annotation_data->cursor = (GdkCursor *) NULL;
    }
}

/**
 * annotate_acquire_input_grab:
 *
 * Acquires the input grab for the annotation window.
 *
 * This function attempts to capture all mouse and stylus events for the
 * annotation window. The implementation is platform-dependent. On Windows,
 * it uses a custom `grab_pointer` function. On other platforms like Linux,
 * it resets the window's input shape to its default state.
 */
static void
annotate_acquire_input_grab (void)
{
  GtkWidget *annotation_window = annotation_data->annotation_window;
#ifdef _WIN32
  grab_pointer (annotation_window, GDK_ALL_EVENTS_MASK);
#endif

#ifndef _WIN32
  /*
   * MACOSX; will do nothing.
   */
  gtk_widget_input_shape_combine_region (annotation_window, NULL);
#endif
}

/**
 * annotate_modify_color:
 * @devdata:  Device-specific data, including the previous point's pressure.
 * @data:     The main #AnnotateData application context.
 * @pressure: The current pressure value from the input device (0.0 to 1.0).
 *
 * Sets the source color for the Cairo context, adjusting the alpha channel
 * based on stylus pressure.
 *
 * This function creates a dynamic, pressure-sensitive stroke effect. It
 * smooths the pressure value by averaging it with the previous point's
 * pressure, applies a curve (square root) to make strokes more responsive
 * at low pressures, and combines it with a contrast factor. The final
 * calculated alpha is applied to the current base color.
 */
void
annotate_modify_color (AnnotateDeviceData *devdata,
                       AnnotateData *data,
                       gdouble pressure)
{
  /* Pressure value is from 0 to 1; this value modify the RGBA gradient. */
  gdouble        old_pressure = pressure;
  AnnotatePoint *last_point;
  last_point = (AnnotatePoint *) g_slist_nth_data (devdata->coord_list, 0);
  if (devdata->coord_list != NULL)
    {
      old_pressure = last_point->pressure;
      if (last_point != NULL && pressure == old_pressure)
        {
          return;
        }
    }
  gdouble  new_alpha;
  gdouble  contrast = 1.5;
  cairo_t *annotation_cr;
  annotation_cr = data->annotation_cairo_context;
  guint r, g, b, a;
  r = data->r;
  g = data->g;
  b = data->b;
  a = data->a;

  if ((! annotation_cr) || (! data->color))
    {
      return;
    }
  if (pressure >= 1)
    {
      cairo_set_source_rgba (annotation_cr,
                             r / 255.0,
                             g / 255.0,
                             b / 255.0,
                             a / 255.0);
      return;
    }

  /*
   * Use a squareroot function to give an exponential curve.
   * This amplifies low pressure values,
   * making the stroke more visible at the start and end.
   */
  gdouble smoothed_pressure = (3 * pressure + old_pressure) / 4;
  gdouble curved_pressure   = sqrt (smoothed_pressure);

  /*
   * Calculate the final alpha value by combining the curved pressure
   * and contrast factor.
   */
  new_alpha = curved_pressure * contrast;

  /* Ensure the alpha value does not exceed the maximum of 1.0 */
  if (new_alpha > 1.0)
    {
      new_alpha = 1.0;
    }

  g_debug ("pressure %f, new_alpha %f", pressure, new_alpha);
  cairo_set_source_rgba (annotation_cr,
                         (gdouble) r / 255.0,
                         (gdouble) g / 255.0,
                         (gdouble) b / 255.0,
                         new_alpha * (gdouble) a / 255.0);
}

/**
 * annotate_draw_ellipse:
 * @devdata:  Device-specific data for color modification.
 * @x:        The x-coordinate of the top-left corner of the bounding box.
 * @y:        The y-coordinate of the top-left corner of the bounding box.
 * @width:    The width of the ellipse.
 * @height:   The height of the ellipse.
 * @pressure: The pressure value to apply for this shape.
 *
 * Draws a pressure-sensitive ellipse on the annotation context.
 *
 * The function first sets the drawing color and alpha based on pressure by
 * calling annotate_modify_color(). It then constructs the ellipse path
 * within the specified bounding box using Cairo transformations and an arc.
 */
static void
annotate_draw_ellipse (AnnotateDeviceData *devdata, gdouble x, gdouble y,
                       gdouble width, gdouble height, gdouble pressure)
{
  cairo_t *annotation_cairo_context;
  annotation_cairo_context = annotation_data->annotation_cairo_context;

  g_debug ("Draw ellipse: 2a=%f 2b=%f\n", width, height);

  annotate_modify_color (devdata, annotation_data, pressure);

  cairo_save (annotation_cairo_context);

  /* The ellipse is done as a 360 degree arc translated. */
  cairo_translate (annotation_cairo_context, x + width / 2.,
                   y + height / 2.);
  cairo_scale (annotation_cairo_context, width / 2., height / 2.);
  cairo_arc (annotation_cairo_context, 0., 0., 1., 0., 2 * M_PI);
  cairo_restore (annotation_cairo_context);
}

/**
 * annotate_draw_point:
 * @devdata:  Device-specific data for color modification.
 * @x:        The x-coordinate of the point.
 * @y:        The y-coordinate of the point.
 * @pressure: The pressure value to apply.
 *
 * Draws a single, pressure-sensitive point on the annotation context.
 *
 * The color is first adjusted for pressure. A point is then rendered by
 * drawing a zero-length line, which appears as a dot styled according to
 * the current line cap settings (e.g., `CAIRO_LINE_CAP_ROUND`).
 */
void
annotate_draw_point (AnnotateDeviceData *devdata,
                     gdouble x,
                     gdouble y,
                     gdouble pressure)
{
  cairo_save (annotation_data->annotation_cairo_context);
  /* Modify a little bit the color depending on pressure. */
  annotate_modify_color (devdata, annotation_data, pressure);
  cairo_move_to (annotation_data->annotation_cairo_context, x, y);
  cairo_line_to (annotation_data->annotation_cairo_context, x, y);
  cairo_restore (annotation_data->annotation_cairo_context);

  /* Compute dirty area */
  GtkWidget *annotation_window = get_annotation_window ();
  gdouble    thickness         = annotation_data->thickness;
  gdouble    padding           = thickness * 2.0;
  gdouble    dirty_rect_x      = x - padding;
  gdouble    dirty_rect_y      = y - padding;

  gdouble dirty_rect_width  = thickness + padding;
  gdouble dirty_rect_height = thickness + padding;

  gtk_widget_queue_draw_area (annotation_window,
                              (gint)dirty_rect_x,
                              (gint)dirty_rect_y,
                              (gint)dirty_rect_width,
                              (gint)dirty_rect_height);

}

/**
 * annotate_draw_line:
 * @devdata: Device data containing the last point of the stroke.
 * @x2:      The x-coordinate of the new point to draw to.
 * @y2:      The y-coordinate of the new point to draw to.
 * @stroke:  If %TRUE, a self-contained stroke is drawn from the last
 * point to the new point. If %FALSE, a line segment is simply
 * added to the current Cairo path without stroking it.
 *
 * Draws a line segment.
 *
 * Depending on the @stroke flag, this function either adds a line segment
 * to the current path (for building complex shapes) or immediately renders
 * a line from the previously recorded point to the new coordinates.
 */
void
annotate_draw_line (AnnotateDeviceData *devdata,
                    gdouble x2,
                    gdouble y2,
                    gboolean stroke)
{
  cairo_save (annotation_data->annotation_cairo_context);
  if (! stroke)
    {
      cairo_line_to (annotation_data->annotation_cairo_context, x2, y2);
    }
  else
    {
      AnnotatePoint *last_point;
      last_point = (AnnotatePoint *) g_slist_nth_data (devdata->coord_list, 0);
      if (last_point)
        {
          cairo_move_to (annotation_data->annotation_cairo_context,
                         last_point->x,
                         last_point->y);
        }
      else
        {
          cairo_move_to (annotation_data->annotation_cairo_context, x2, y2);
        }
      cairo_line_to (annotation_data->annotation_cairo_context, x2, y2);
      cairo_stroke (annotation_data->annotation_cairo_context);

      GtkWidget *annotation_window = get_annotation_window ();

      gdouble x1 = last_point ? last_point->x : x2;
      gdouble y1 = last_point ? last_point->y : y2;

      gdouble min_x = MIN (x1, x2);
      gdouble min_y = MIN (y1, y2);
      gdouble max_x = MAX (x1, x2);
      gdouble max_y = MAX (y1, y2);

      gint t = annotation_data->thickness;
      gint margin = t + 2;

      gtk_widget_queue_draw_area (annotation_window,
                                  (gint)(min_x - margin),
                                  (gint)(min_y - margin),
                                  (gint)((max_x - min_x) + 2 * margin),
                                  (gint)((max_y - min_y) + 2 * margin));

    }
  cairo_restore (annotation_data->annotation_cairo_context);
}

/**
 * annotate_draw_point_list:
 * @devdata: Device data for color modification.
 * @list:    A #GSList of #AnnotatePoint structs to be drawn.
 *
 * Constructs a continuous path from a list of points on the Cairo context.
 *
 * This function iterates through the @list, creating a single path of
 * connected line segments. The color/alpha is updated for each point based
 * on its pressure. Note that this function only builds the path; the caller
 * is responsible for stroking or filling it.
 */
void
annotate_draw_point_list (AnnotateDeviceData *devdata, GSList *list)
{
  cairo_save (annotation_data->annotation_cairo_context);
  if (list)
    {
      guint i      = 0;
      guint length = g_slist_length (list);
      for (i = 0; i < length; i = i + 1)
        {
          AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
          if (! point)
            {
              return;
            }

          if (length == 1)
            {
              /* It is a point. */
              annotate_draw_point (devdata,
                                   point->x,
                                   point->y,
                                   point->pressure);
              break;
            }
          annotate_modify_color (devdata, annotation_data, point->pressure);
          /* Draw line between the two points. */
          annotate_draw_line (devdata, point->x, point->y, FALSE);
        }
    }
  cairo_restore (annotation_data->annotation_cairo_context);
}

/**
 * annotate_draw_curve:
 * @devdata: Device data for color modification.
 * @list:    A dense #GSList of #AnnotatePoint structs representing the curve.
 *
 * Renders a smooth curve by constructing a path of cubic Bezier splines.
 *
 * This function is designed to render the dense point list generated by a
 * spline algorithm (like Catmull-Rom). It iterates through the @list,
 * taking points three at a time to form the control and anchor points
 * for a series of `cairo_curve_to` segments. This is used to
 * render a smoothed version of a freehand stroke.
 * The path is not stroked; the caller is responsible for rendering.
 *
 */
static void
annotate_draw_curve (AnnotateDeviceData *devdata, GSList *list)
{
  guint length = g_slist_length (list);

  if (list)
    {
      guint i = 0;
      for (i = 0; i < length; i = i + 3)
        {
          AnnotatePoint *first_point = NULL;
          first_point = (AnnotatePoint *) g_slist_nth_data (list,
                                                            i);
          if (! first_point)
            {
              return;
            }
          if (length == 1)
            {
              /* It is a point. */
              annotate_draw_point (devdata,
                                   first_point->x,
                                   first_point->y,
                                   first_point->pressure);
            }
          else
            {
              AnnotatePoint *second_point = NULL;
              second_point = (AnnotatePoint *) g_slist_nth_data (list,
                                                                 i + 1);
              if (! second_point)
                {
                  return;
                }
              else
                {
                  AnnotatePoint *third_point = NULL;
                  third_point = (AnnotatePoint *) g_slist_nth_data (list,
                                                                    i + 2);
                  if (! third_point)
                    {
                      /* draw line from first to second point */
                      annotate_draw_line (devdata,
                                          second_point->x,
                                          second_point->y,
                                          FALSE);
                      return;
                    }
                  annotate_modify_color (devdata,
                                         annotation_data,
                                         second_point->pressure);

                  cairo_curve_to (annotation_data->annotation_cairo_context,
                                  first_point->x,
                                  first_point->y,
                                  second_point->x,
                                  second_point->y,
                                  third_point->x,
                                  third_point->y);
                }
            }
        }
    }
}

/**
 * annotate_restore_surface:
 *
 * Restores the drawing canvas to a previous state from a savepoint.
 *
 * This function identifies the current savepoint based on the undo/redo
 * index, clears the current canvas, and paints the contents of the
 * savepoint's corresponding PNG file onto the annotation context.
 */
void
annotate_restore_surface (void)
{
  g_debug ("annotate window restore surface\n");

  if (annotation_data->annotation_cairo_context)
    {
      cairo_t *annotation_cr;
      annotation_cr = annotation_data->annotation_cairo_context;

      guint i = annotation_data->current_save_index;
      if (g_slist_length (annotation_data->savepoint_list) == i)
        {
          /* clear path and current point */
          cairo_new_path (annotation_cr);
          clear_cairo_context (annotation_cr);
          return;
        }

      AnnotateSavepoint *savepoint = (AnnotateSavepoint *) g_slist_nth_data (
          annotation_data->savepoint_list, i);

      if (! savepoint)
        {
          g_debug ("savepoint is FALSE\n");
          return;
        }

      cairo_save (annotation_cr);
      clear_cairo_context (annotation_cr);
      cairo_new_path (annotation_cr);
      cairo_set_operator (annotation_cr, CAIRO_OPERATOR_SOURCE);

      if (savepoint->filename)
        {
          g_debug ("load savepoint from filename %s\n", savepoint->filename);
          /* Load the file in the annotation surface. */
          cairo_surface_t *image_surface = cairo_image_surface_create_from_png (
              savepoint->filename);
          g_debug ("The save-point %s has been loaded from file\n",
                   savepoint->filename);

          if (image_surface)
            {
              cairo_set_source_surface (annotation_cr, image_surface, 0, 0);
              cairo_paint (annotation_cr);
              cairo_surface_destroy (image_surface);
            }
        }
      cairo_restore (annotation_cr);
    }

  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/**
 * rectify:
 * @devdata:     The device data containing the stroke to process.
 * @closed_path: %TRUE if the stroke is a closed shape.
 *
 * A shape-recognition helper that attempts to straighten a freehand stroke.
 *
 * It erases the original freehand stroke by restoring the last savepoint,
 * then analyzes the stroke's points to generate a simplified, straightened
 * version, which it then redraws. The device's coordinate list is updated
 * with the new, rectified points.
 */
static void
rectify (AnnotateDeviceData *devdata, gboolean closed_path)
{
  gdouble tollerance = annotate_get_thickness ();
  GSList *broken_list = broken (devdata->coord_list,
                                closed_path,
                                TRUE,
                                tollerance);

  g_debug ("rectify\n");

  /* Restore the surface without the last path handwritten. */
  annotate_restore_surface ();

  annotate_draw_point_list (devdata, broken_list);

  annotate_coord_dev_list_free (devdata);
  devdata->coord_list = broken_list;
}

/**
 * roundify:
 * @devdata:     The device data containing the stroke to process.
 * @closed_path: %TRUE if the stroke is a closed shape.
 *
 * A shape-recognition helper that attempts to smooth a freehand stroke.
 *
 * It erases the original stroke and analyzes its points. If the shape
 * resembles an ellipse, it draws a clean ellipse. Otherwise, it generates
 * a smooth spline curve and redraws the stroke. The device's
 * coordinate list is updated with the new points.
 */
static void
roundify (AnnotateDeviceData *devdata, gboolean closed_path)
{
  gdouble tollerance = annotate_get_thickness ();

  /* Build the meaningful point list with the standard deviation algorithm. */
  GSList *meaningful_point_list = (GSList *) NULL;

  /* Restore the surface without the last path handwritten. */
  annotate_restore_surface ();

  meaningful_point_list = build_meaningful_point_list (devdata->coord_list,
                                                       tollerance);

  annotate_coord_dev_list_free (devdata);

  if (g_slist_length (meaningful_point_list) < 4)
    {
      annotate_draw_point_list (devdata, meaningful_point_list);
      devdata->coord_list = meaningful_point_list;
    }
  else if ((closed_path) &&
           (is_similar_to_an_ellipse (meaningful_point_list, tollerance)))
    {
      GSList *rect_list = build_outbounded_rectangle (meaningful_point_list);

      if (rect_list)
        {
          /*
           * Identify the bounding rectangle of all the points and draws
           * the appropriate ellipse/circle.
           */
          gint           n    = g_slist_length (rect_list);
          gint           left = 0, right = 0, top = 0, bottom = 0;
          AnnotatePoint *point1 = NULL;
          point1 = (AnnotatePoint *) g_slist_nth_data (rect_list, 0);
          left   = point1->x;
          right  = point1->x;
          top    = point1->y;
          bottom = point1->y;
          for (int ii = 1; ii < n; ii++)
            {
              point1 = (AnnotatePoint *) g_slist_nth_data (rect_list, ii);
              if (point1->x < left)
                {
                  left = point1->x;
                }
              else if (point1->x > right)
                {
                  right = point1->x;
                }
              if (point1->y < top)
                {
                  top = point1->y;
                }
              else if (point1->y > bottom)
                {
                  bottom = point1->y;
                }
            }
          annotate_draw_ellipse (devdata, left, top, right - left, bottom - top,
                                 point1->pressure);

          g_slist_foreach (rect_list, (GFunc) g_free, NULL);
          g_slist_free (rect_list);
          devdata->coord_list = meaningful_point_list;
        }
    }

  else
    {
      GSList *splined_list = spline (meaningful_point_list);
      annotate_draw_curve (devdata, splined_list);

      annotate_coord_dev_list_free (devdata);
      devdata->coord_list = splined_list;
      g_slist_foreach (meaningful_point_list, (GFunc) g_free, (gpointer) NULL);
      g_slist_free (meaningful_point_list);
    }
}

/**
 * splinify:
 * @devdata: The device data containing the stroke to process.
 *
 * A shape-recognition helper that converts a freehand stroke into a smooth
 * curve.
 *
 * It erases the original stroke, calculates a new set of points
 * representing a spline, redraws the curve, and updates the device's
 * coordinate list.
 */
static void
splinify (AnnotateDeviceData *devdata)
{
  annotate_restore_surface ();
  GSList *splined_list = spline (devdata->coord_list);
  annotate_draw_curve (devdata, splined_list);
  annotate_coord_dev_list_free (devdata);
  devdata->coord_list = splined_list;
}

/**
 * create_annotation_window:
 * @workspace: (nullable): The #Workspace object, which may contain a
 * session file to load.
 * @commandline: The #CommandLine object containing startup options.
 *
 * Creates and initializes the main annotation window of the application.
 *
 * This function loads the user interface from a GtkBuilder file,
 * retrieves the top-level window widget, and connects all its signal
 * handlers. It also sets up extra signal handlers for hot-plugged
 * input devices. If a session file is provided in the @workspace, it
 * is loaded.
 *
 * Returns: (transfer none): A pointer to the newly created annotation
 * #GtkWidget, or %NULL on failure.
 */
GtkWidget *
create_annotation_window (Workspace *workspace, CommandLine *commandline)
{
  GtkWidget *widget = (GtkWidget *) NULL;
  GError    *error  = (GError *) NULL;

  annotate_init (NULL);

  /* Initialize the main window. */
  annotation_data->annotation_window_gtk_builder = gtk_builder_new ();

  GtkBuilder * annotation_window_gtk_builder =
    annotation_data->annotation_window_gtk_builder;

  annotation_data->is_opaque = commandline->is_opaque;
  annotation_data->paths     = NULL;

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file (annotation_window_gtk_builder,
                             ANNOTATION_UI_FILE, &error);

  if (error)
    {
      g_error ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      return widget;
    }

  GObject *annotation_obj;
  annotation_obj = gtk_builder_get_object (annotation_window_gtk_builder,
                                           "annotationWindow");
  widget         = GTK_WIDGET (annotation_obj);
  gtk_window_set_keep_above (GTK_WINDOW (widget), TRUE);

  annotation_data->annotation_window = widget;
  if (annotation_data->annotation_window == NULL)
    {
      g_warning ("Failed to create the annotation window");
      return NULL;
    }

  /* Connect all the callback from gtkbuilder xml file. */
  gtk_builder_connect_signals (annotation_window_gtk_builder,
                               (gpointer) annotation_data);

  /*
   * Connect some extra callbacks in order to handle
   * the hotplugged input devices.
   * After GDK 3.2 the GdkSeat has the device-added
   * and device-removed signals.
   */
  GdkSeat *seat = gdk_display_get_default_seat (gdk_display_get_default ());

  g_signal_connect (seat,
                    "device-added",
                    G_CALLBACK (on_device_added),
                    annotation_data);

  g_signal_connect (seat,
                    "device-removed",
                    G_CALLBACK (on_device_removed),
                    annotation_data);

  if (workspace->iwb_filename)
    {
      annotation_data->savepoint_list = load_iwb (workspace->iwb_filename);
    }
  return widget;
}

/**
 * make_annotation_window_transparent:
 *
 * Configures the annotation window for transparency.
 *
 * This function sets the window's visual to one that supports an alpha
 * channel (RGBA). It includes platform-specific workarounds, such as
 * using layered windows on Windows, to achieve click-through
 * transparency when not drawing.
 */
static void
make_annotation_window_transparent (void)
{
  if (! annotation_data->is_opaque)
    {
      GtkWidget *annotation_window = get_annotation_window ();
      /* This trys to set an alpha channel. */
      on_screen_changed (annotation_window,
                         NULL,
                         annotation_data);

      /* Put the opacity to 0 to avoid the initial flickering. */
      gtk_widget_set_opacity (annotation_window, 0.01);

#ifdef _WIN32
      /* @TODO Use RGBA colormap and avoid to use the layered window. */
      /* I use a layered window that use the black as transparent color. */
      set_layered_gdk_window_attributes (
          gtk_widget_get_window (annotation_window),
          RGB (0, 0, 0),
          0,
          LWA_COLORKEY);
#endif
    }
}

/**
 * position_annotation_window:
 * @x: The X coordinate for the window's top-left corner.
 * @y: The Y coordinate for the window's top-left corner.
 * @width: The initial width to request for the window.
 * @height: The initial height to request for the window.
 *
 * Positions, resizes, and shows the main annotation window.
 *
 * This function moves the window to the specified coordinates, sets its
 * initial size request, and ensures it stays on top of other windows.
 * It also calls a helper function to set up transparency before
 * finally showing the window on screen.
 **/
void
position_annotation_window (int x, int y, int width, int height)
{
  GtkWidget *annotation_window = get_annotation_window ();
  if (annotation_window != NULL)
    {
      g_debug ("setting annotation window position %d %d %d %d\n",
               x, y, width, height);
      gtk_window_move (GTK_WINDOW (annotation_window), x, y);

      gtk_window_set_keep_above (GTK_WINDOW (annotation_window),
                                 TRUE);

      make_annotation_window_transparent ();

      gtk_widget_set_size_request (annotation_window,
                                   width,
                                   height);

      gtk_widget_show_all (annotation_window);
    }
}

/**
 * create_savepoint_dir:
 *
 * Creates the temporary directory for storing undo/redo image snapshots.
 *
 * This function constructs a unique path in the system's temporary
 * directory (e.g., /tmp/ardesia/project_name/images) and creates it.
 * If a directory from a previous session exists, it is removed first.
 */
static void
create_savepoint_dir (void)
{
  const gchar *tmpdir       = g_get_tmp_dir ();
  gchar       *images       = "images";
  gchar       *project_name = get_project_name ();
  gchar *ardesia_tmp_dir = g_build_filename (tmpdir, PACKAGE_NAME, (gchar *) 0);

  gchar *project_tmp_dir = g_build_filename (ardesia_tmp_dir,
                                             project_name, (gchar *) 0);

  if (g_file_test (ardesia_tmp_dir, G_FILE_TEST_IS_DIR))
    {
      /* The folder already exist; I delete it. */
      rmdir_recursive (ardesia_tmp_dir);
    }

  annotation_data->savepoint_dir = g_build_filename (project_tmp_dir,
                                                     images,
                                                     (gchar *) 0);

  g_mkdir_with_parents (annotation_data->savepoint_dir, 0777);
  g_free (ardesia_tmp_dir);
  g_free (project_tmp_dir);
}

/**
 * delete_savepoint:
 * @savepoint: The #AnnotateSavepoint object to delete.
 *
 * Frees all resources associated with a single savepoint.
 *
 * This function removes the savepoint's snapshot PNG file from disk,
 * removes the corresponding node from the global savepoint list, and
 * frees the #AnnotateSavepoint struct itself.
 */
static void
delete_savepoint (AnnotateSavepoint *savepoint)
{
  if (savepoint)
    {
      if (savepoint->filename)
        {
          g_debug ("The save-point %s has been removed\n", savepoint->filename);
          g_remove (savepoint->filename);
          g_free (savepoint->filename);
          savepoint->filename = (gchar *) NULL;
        }
      GSList *savepoint_list = NULL;
      savepoint_list         = annotation_data->savepoint_list;
      annotation_data->savepoint_list = g_slist_remove (savepoint_list,
                                                        savepoint);
      g_free (savepoint);
      savepoint = (AnnotateSavepoint *) NULL;
    }
}

/**
 * annotate_redolist_free:
 *
 * Frees all savepoints in the "redo" list.
 *
 * When a new drawing action occurs after an undo, the "future" history
 * (the redo list) becomes invalid. This function clears that part of the
 * history by deleting all savepoints from the current undo position to
 * the head of the list.
 */
static void
annotate_redolist_free (void)
{
  if (! annotation_data || ! annotation_data->savepoint_list)
    return;

  guint   i            = annotation_data->current_save_index;
  GSList *current_node = annotation_data->savepoint_list;

  while (i > 0 && current_node != NULL)
    {
      delete_savepoint (current_node->data);
      current_node = annotation_data->savepoint_list;
      i--;
    }

  annotation_data->savepoint_list = current_node;
}

/**
 * annotate_savepoint_list_free:
 *
 * Frees the entire list of savepoints.
 *
 * This function iterates through all savepoints in the global list and calls
 * delete_savepoint() on each one to free its resources and remove it from
 * disk. This is typically called during application shutdown.
 */
static void
annotate_savepoint_list_free (void)
{
  g_slist_foreach (annotation_data->savepoint_list, (GFunc) delete_savepoint,
                   (gpointer) NULL);

  annotation_data->savepoint_list = (GSList *) NULL;
}

/* Delete the ardesia temporary directory */
static void
delete_ardesia_tmp_dir (void)
{
  gchar *ardesia_tmp_dir = g_build_filename (g_get_tmp_dir (),
                                             PACKAGE_NAME,
                                             (gchar *) 0);
  rmdir_recursive (ardesia_tmp_dir);
  g_free (ardesia_tmp_dir);
}

/**
 * draw_arrow_in_point:
 * @point:     The #AnnotatePoint where the arrowhead's tip should be.
 * @width:     The base width of the stroke, used to scale the arrowhead.
 * @direction: The direction in radians for the arrow to point.
 *
 * Draws a filled arrowhead at a specific point.
 *
 * This helper function calculates the vertices of a polygonal arrowhead
 * based on the given point, width, and direction, and then renders it
 * as a filled shape on the Cairo context.
 */
static void
draw_arrow_in_point (AnnotatePoint *point, gdouble width, gdouble direction)
{
  cairo_t *annotation_cairo_context;
  annotation_cairo_context = annotation_data->annotation_cairo_context;

  gdouble width_cos = width * cos (direction);
  gdouble width_sin = width * sin (direction);

  /* Vertex of the arrow. */
  gdouble arrow_head_0_x = point->x + width_cos;
  gdouble arrow_head_0_y = point->y + width_sin;

  /* Left point. */
  gdouble arrow_head_1_x = point->x - width_cos + width_sin;
  gdouble arrow_head_1_y = point->y - width_cos - width_sin;

  /* Origin. */
  gdouble arrow_head_2_x = point->x - 0.8 * width_cos;
  gdouble arrow_head_2_y = point->y - 0.8 * width_sin;

  /* Right point. */
  gdouble arrow_head_3_x = point->x - width_cos - width_sin;
  gdouble arrow_head_3_y = point->y + width_cos - width_sin;

  cairo_save (annotation_cairo_context);
  cairo_stroke (annotation_cairo_context);

  /* Initialize cairo properties. */
  cairo_set_line_join (annotation_cairo_context,
                       CAIRO_LINE_JOIN_MITER);

  cairo_set_operator (annotation_cairo_context,
                      CAIRO_OPERATOR_SOURCE);

  cairo_set_line_width (annotation_cairo_context, width);

  /* Draw the arrow. */
  cairo_move_to (annotation_cairo_context,
                 arrow_head_2_x,
                 arrow_head_2_y);

  cairo_line_to (annotation_cairo_context,
                 arrow_head_1_x,
                 arrow_head_1_y);

  cairo_line_to (annotation_cairo_context,
                 arrow_head_0_x,
                 arrow_head_0_y);

  cairo_line_to (annotation_cairo_context,
                 arrow_head_3_x,
                 arrow_head_3_y);

  cairo_close_path (annotation_cairo_context);
  cairo_fill_preserve (annotation_cairo_context);
  cairo_stroke (annotation_cairo_context);
  cairo_surface_flush (cairo_get_target (annotation_cairo_context));
  cairo_restore (annotation_cairo_context);

  g_debug ("with vertex at (x,y)= (%f : %f)\n", arrow_head_0_x, arrow_head_0_y);

  /* ---- Compute bounding box ---- */
  gdouble min_x = MIN (MIN (arrow_head_0_x, arrow_head_1_x),
                       MIN (arrow_head_2_x, arrow_head_3_x));
  gdouble max_x = MAX (MAX (arrow_head_0_x, arrow_head_1_x),
                       MAX (arrow_head_2_x, arrow_head_3_x));
  gdouble min_y = MIN (MIN (arrow_head_0_y, arrow_head_1_y),
                       MIN (arrow_head_2_y, arrow_head_3_y));
  gdouble max_y = MAX (MAX (arrow_head_0_y, arrow_head_1_y),
                       MAX (arrow_head_2_y, arrow_head_3_y));

  /* Add a small padding for antialiasing and line width */
  gdouble padding = width * 2.0;
  min_x -= padding;
  min_y -= padding;
  max_x += padding;
  max_y += padding;

  GtkWidget *annotation_window = get_annotation_window ();

  gtk_widget_queue_draw_area (annotation_window,
                              (int)min_x,
                              (int)min_y,
                              (int)(max_x - min_x),
                              (int)(max_y - min_y));
  
}

/**
 * annotate_configure_pen_options:
 * @data: The main #AnnotateData application context.
 *
 * Configures the Cairo context for a drawing operation based on the
 * current tool settings.
 *
 * This function acts as a bridge between the application's high-level
 * state (stored in @data) and the low-level Cairo drawing context. It
 * should be called immediately before a drawing or erasing stroke begins.
 *
 * It sets the line cap and join styles to `ROUND` for smooth strokes.
 * Based on the currently selected tool, it sets the appropriate Cairo
 * operator (`CAIRO_OPERATOR_CLEAR` for the eraser or `CAIRO_OPERATOR_SOURCE`
 * for pens) and line width. Finally, it applies the current drawing
 * color by calling the select_color() helper function.
 */
void
annotate_configure_pen_options (AnnotateData *data)
{
  cairo_t *annotation_cairo_context;
  annotation_cairo_context = data->annotation_cairo_context;

  if (annotation_cairo_context)
    {
      cairo_new_path (annotation_cairo_context);

      cairo_set_line_cap (annotation_cairo_context,
                          CAIRO_LINE_CAP_ROUND);

      cairo_set_line_join (annotation_cairo_context,
                           CAIRO_LINE_JOIN_ROUND);

      if (data->cur_context->type == ANNOTATE_ERASER)
        {
          data->cur_context = data->default_eraser;

          cairo_set_operator (annotation_cairo_context,
                              CAIRO_OPERATOR_CLEAR);

          /*
           * Make eraser slightly larger to overpaint and
           * clear anti-aliasing artifacts.
           */
          gdouble delta = 2.0;
          cairo_set_line_width (annotation_cairo_context,
                                annotate_get_thickness () + delta);
        }
      else
        {
          cairo_set_operator (annotation_cairo_context,
                              CAIRO_OPERATOR_SOURCE);

          cairo_set_line_width (annotation_cairo_context,
                                annotate_get_thickness ());
        }
    }
  select_color ();
}

/**
 * annotate_add_savepoint:
 *
 * Adds a save point for undo/redo functionality.
 *
 * This function should be called at the end of each user action that
 * modifies the annotation, such as a mouse button release, a fill
 * operation, or a clear screen action.
 *
 * It creates a new savepoint structure, copies the current content of the
 * annotation's Cairo context to a PNG file in the savepoint directory,
 * and prepends the new savepoint to the history list.
 *
 * Crucially, this function is "atomic": the undo/redo history is
 * modified (redo list cleared, new savepoint prepended) only after the
 * PNG file is successfully written. If any step fails, the history
 * remains unchanged, ensuring application state consistency.
 *
 * Postcondition:
 * - Redo history cleared
 * - New savepoint prepended to the savepoint list
 * - Current save index reset to 0
 * - Corresponding PNG file written to disk
 */
void
annotate_add_savepoint (void)
{
  AnnotateSavepoint *savepoint      = NULL;
  cairo_surface_t   *source_surface = NULL;
  int                w = 0, h = 0;
  guint              savepoint_index;

  g_return_if_fail (annotation_data != NULL);
  g_return_if_fail (annotation_data->annotation_cairo_context != NULL);

  source_surface = cairo_get_target (annotation_data->annotation_cairo_context);
  if (! source_surface)
    {
      g_warning ("Annotation context target is NULL");
      return;
    }

  get_context_size (annotation_data->annotation_cairo_context, &w, &h);
  if (w <= 0 || h <= 0)
    {
      g_warning ("Invalid annotation context size: %dx%d", w, h);
      return;
    }

  /* Create the savepoint metadata struct */
  savepoint = g_malloc0 (sizeof (AnnotateSavepoint));
  if (! savepoint)
    {
      g_warning ("Failed to allocate savepoint");
      return;
    }

  savepoint_index     = g_slist_length (annotation_data->savepoint_list) + 1;
  savepoint->filename = g_strdup_printf ("%s%s%s_%d_vellum.png",
                                         annotation_data->savepoint_dir,
                                         G_DIR_SEPARATOR_S,
                                         PACKAGE_NAME,
                                         savepoint_index);
  if (! savepoint->filename)
    {
      g_warning ("Failed to allocate filename for savepoint");
      g_free (savepoint);
      return;
    }

  /* Prepare data for the background thread */
  SavepointThreadData *thread_data = g_new0 (SavepointThreadData, 1);
  thread_data->filename            = g_strdup (savepoint->filename);

  /* Create the surface copy *in the main thread* */
  thread_data->surface_copy = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                          w,
                                                          h);

  if (cairo_surface_status (thread_data->surface_copy) != CAIRO_STATUS_SUCCESS)
    {
      g_warning ("Failed to create surface copy for thread");
      savepoint_thread_data_free (thread_data);
      g_free (savepoint->filename);
      g_free (savepoint);
      return;
    }

  /* Copy the content of the source surface to the new surface */
  cairo_t *cr_copy = cairo_create (thread_data->surface_copy);
  cairo_set_source_surface (cr_copy, source_surface, 0, 0);
  cairo_paint (cr_copy);
  cairo_destroy (cr_copy);

  /* Update history immediately */
  annotate_redolist_free ();

  annotation_data->savepoint_list =
      g_slist_prepend (annotation_data->savepoint_list, savepoint);

  annotation_data->current_save_index = 0;

  /* Create and run the background task (slow part) */
  GTask *task = g_task_new (NULL, NULL, NULL, NULL);
  g_task_set_task_data (task, thread_data, savepoint_thread_data_free);
  g_task_run_in_thread (task, savepoint_worker_thread);
  g_object_unref (task);

  g_debug ("Savepoint created: %s (saving in background)", savepoint->filename);
}

/**
 * initialize_annotation_cairo_context:
 * @data: (inout): The main #AnnotateData application context, which will
 * hold the newly created Cairo context.
 *
 * Initializes the main Cairo drawing context for the annotation window if
 * it has not been created yet.
 *
 * The function is idempotent and will only perform the initialization once.
 * It creates a platform-specific Cairo surface matching the window's
 * dimensions. If a session history (`savepoint_list`) exists, it restores
 * the canvas from the last savepoint; otherwise, it clears the screen and
 * creates a new, blank initial savepoint before acquiring the input grab.
 **/
void
initialize_annotation_cairo_context (AnnotateData *data)
{
  GtkWidget *annotation_window = data->annotation_window;
  if (data->annotation_cairo_context == NULL)
    {
      g_debug ("initializing annotation cairo context\n");
      /* Initialize a transparent window. */
#ifdef _WIN32
      /* The hdc has depth 32 and the technology is DT_RASDISPLAY. */
      GdkWindow *window;
      HDC        hdc;
      window                   = gtk_widget_get_window (annotation_window);
      hdc                      = GetDC (GDK_WINDOW_HWND (window));
      /*
       * @TODO Use an HDC that support the ARGB32 format to support
       * the alpha channel; this fix the highlighter bug.
       * In the documentation is written that the now the resulting surface is
       * in RGB24 format.
       *
       */
      cairo_surface_t *surface = cairo_win32_surface_create (hdc);
      data->annotation_cairo_context = cairo_create (surface);
      cairo_surface_destroy (surface);
#else
      int width  = gtk_widget_get_allocated_width (annotation_window);
      int height = gtk_widget_get_allocated_height (annotation_window);
      if (data->annotation_cairo_context == NULL)
        {
          data->annotation_cairo_context = create_new_context (width,
                                                               height);
        }
      if (background_data->cr == NULL)
        {
          background_data->cr = create_new_context (width, height);
        }
#endif

      cairo_t *annotation_cr;
      annotation_cr = data->annotation_cairo_context;

      if (cairo_status (annotation_cr) != CAIRO_STATUS_SUCCESS)
        {
          g_printerr ("Failed to allocate the annotation cairo context");
          annotate_quit ();
          exit (EXIT_FAILURE);
        }
      cairo_set_operator (annotation_cr, CAIRO_OPERATOR_OVER);

      if (data->savepoint_list == NULL)
        {
          g_debug ("It has not savepoint; clear the screen");
          /* Clear the screen.  */
          annotate_clear_screen ();
          /* Create the first empty savepoint. */
          annotate_add_savepoint ();
        }
      else
        {
          g_debug ("It has savepoint; restore surface");
          annotate_restore_surface ();
        }

#ifndef _WIN32
      gtk_widget_set_opacity (annotation_window, 1.0);
#endif
      annotate_acquire_grab ();
    }
}

/**
 * get_annotation_window:
 *
 * Retrieves the main annotation window widget.
 *
 * Returns: (transfer none): the #GtkWidget representing the annotation
 * window. The returned widget is owned by the application and must not
 * be freed by the caller.
 */
GtkWidget *
get_annotation_window (void)
{
  return annotation_data->annotation_window;
}

/**
 * annotate_set_color:
 * @color: (transfer none): A string representing the new drawing color
 * (e.g., in hex format "RRGGBBAA").
 *
 * Sets the new global color for drawing operations.
 *
 * This function updates the color stored in the application's global
 * state (`annotation_data`). It safely handles memory by first freeing
 * any previously stored color string to prevent memory leaks.
 *
 * It then creates a **private, internal copy** of the @color string
 * passed as an argument, using g_strdup(). Because the function creates
 * its own copy, the **caller retains ownership** of the original string
 * and can (and must) free it if it was dynamically allocated.
 *
 * Since: 1.0
 */
void
annotate_set_color (gchar *color)
{
  if (annotation_data->color != NULL)
    {
      g_free (annotation_data->color);
      annotation_data->color = NULL;
    }
  annotation_data->color = g_strdup (color);
  guint r, g, b, a;
  assert (strlen (annotation_data->color) == 8);
  sscanf (annotation_data->color, "%02X%02X%02X%02X", &r, &g, &b, &a);
  annotation_data->r = r;
  annotation_data->g = g;
  annotation_data->b = b;
  annotation_data->a = a;
}

/**
 * annotate_set_rectifier:
 * @rectify: %TRUE to enable rectangle rectification, %FALSE to disable.
 *
 * Enable or disable the rectifier mode for annotations.
 *
 * When enabled, drawn shapes may be automatically adjusted or
 * rectified to cleaner rectangles depending on the current
 * drawing context.
 *
 * Since: 1.0
 */
void
annotate_set_rectifier (gboolean rectify)
{
  annotation_data->rectify = rectify;
}

/**
 * annotate_set_rounder:
 * @roundify: %TRUE to enable rounding mode, %FALSE to disable.
 *
 * Enables or disables the rounder mode for annotations.
 *
 * When enabled, drawn shapes may be automatically smoothed
 * into rounded forms instead of sharp edges.
 *
 * Since: 1.0
 */
void
annotate_set_rounder (gboolean roundify)
{
  annotation_data->roundify = roundify;
}

/**
 * annotate_set_arrow:
 * @arrow: %TRUE to enable arrow drawing, %FALSE to disable.
 *
 * Enables or disables arrow mode for annotations.
 *
 * When enabled, freehand strokes will automatically terminate with an
 * arrowhead.
 */
void
annotate_set_arrow (gboolean arrow)
{
  annotation_data->arrow = arrow;
}

/**
 * annotate_set_thickness:
 * @thickness: The base line thickness to use for annotation strokes.
 *
 * Sets the base line thickness for drawing operations.
 *
 * The actual stroke thickness may be modified by tool-specific
 * multipliers (e.g. for the eraser or highlighter) when rendering.
 */
void
annotate_set_thickness (gdouble thickness)
{
  annotation_data->thickness = thickness;
}

/**
 * annotate_get_thickness:
 *
 * Gets the effective line thickness for the current tool.
 *
 * This function returns the base thickness set with
 * annotate_set_thickness(), multiplied by a corrective factor
 * depending on the active tool:
 *
 * - Eraser: scaled by `eraser_multiplier`.
 * - Highlighter: scaled by `highlighter_multiplier`.
 * - Pen: scaled by `pen_multiplier`.
 *
 * Returns: (transfer none): the effective line thickness.
 */
gdouble
annotate_get_thickness (void)
{
  gfloat corrective_factor = 1.0;
  if (annotation_data->cur_context->type == ANNOTATE_ERASER)
    {
      corrective_factor = annotation_data->eraser_multiplier;
    }
  else if (annotation_data->cur_context->type == ANNOTATE_PEN)
    {
      if (annotation_data->a <= 128)
        {
          corrective_factor = annotation_data->highlighter_multiplier;
        }
      else
        {
          corrective_factor = annotation_data->pen_multiplier;
        }
    }
  return annotation_data->thickness * corrective_factor;
}

/**
 * annotate_coord_list_prepend:
 * @devdata:  The #AnnotateDeviceData to which the point will be added.
 * @x:        The x-coordinate of the new point.
 * @y:        The y-coordinate of the new point.
 * @width:    The width associated with the point (legacy, may not be used).
 * @pressure: The pressure value (0.0 to 1.0) for the point.
 *
 * Allocates a new #AnnotatePoint and prepends it to a device's coordinate
 * list.
 */
void
annotate_coord_list_prepend (AnnotateDeviceData *devdata, gdouble x, gdouble y,
                             gdouble width, gdouble pressure)
{
  AnnotatePoint *point = g_malloc ((gsize) sizeof (AnnotatePoint));
  point->x             = x;
  point->y             = y;
  point->width         = width;
  point->pressure      = pressure;
  devdata->coord_list  = g_slist_prepend (devdata->coord_list, point);
  g_debug ("add to coord list (%f, %f)", point->x, point->y);
}

/**
 * annotate_coord_dev_list_free:
 * @devdata: A pointer to an #AnnotateDeviceData structure whose
 * coordinate list will be freed.
 *
 * Frees the list of coordinates associated with a given input device.
 * This function releases each element of the coordinate list, clears
 * the GSList itself, and resets the device's length counter to zero.
 *
 * If no coordinate list exists for @devdata, only the length field is
 * reset. This ensures that the device data structure is left in a
 * consistent state regardless of its prior contents.
 **/
void
annotate_coord_dev_list_free (AnnotateDeviceData *devdata)
{
  if (devdata->coord_list)
    {
      g_slist_foreach (devdata->coord_list, (GFunc) g_free, (gpointer) NULL);
      g_slist_free (devdata->coord_list);
      devdata->coord_list = (GSList *) NULL;
    }
}

/**
 * annotate_push_context:
 * @cr: a #cairo_t context whose content will be composited
 *      onto the annotation window.
 *
 * Paints the given Cairo drawing context over the annotation window's
 * internal Cairo surface. This function captures the content of @cr
 * as a surface, applies it to the annotation window using the
 * %CAIRO_OPERATOR_ADD operator, and then updates the annotation
 * state by adding a new savepoint.
 *
 * The process includes:
 * - Clearing the current path of the annotation context.
 * - Setting the source surface from the input Cairo context.
 * - Painting it onto the annotation window surface.
 * - Stroking the path using the current line settings.
 * - Restoring the annotation Cairo context to its saved state.
 *
 * This is typically used to integrate external drawing (e.g., from
 * the text window) into the annotation layer.
 **/
void
annotate_push_context (cairo_t *cr)
{
  cairo_t *annotation_cairo_context =
      annotation_data->annotation_cairo_context;

  if (annotation_cairo_context == NULL)
  {
    g_warning ("Cannot push context on null annotation_cairo_context");
    return;
  }
  cairo_save (annotation_cairo_context);
  cairo_surface_t *source_surface = (cairo_surface_t *) NULL;
  g_debug ("The text window content has been painted over the "
           "annotation window\n");

  /* this clears the current path from the cairo context */
  cairo_new_path (annotation_cairo_context);
  /* this gets the target surface for the cairo context */
  source_surface = cairo_get_target (cr);

  cairo_set_operator (annotation_cairo_context,
                      CAIRO_OPERATOR_OVER);
 
  /*
   * Creates a pattern from surface at x,y on the context
   * at 0, left screen -> right screen, right screen disappears
   * at -1920, left screen -> disappears, right screen is good
   */
  cairo_set_source_surface (annotation_cairo_context,
                            source_surface,
                            0,
                            0);

  /* paints the current source everywhere in clip region. */
  cairo_paint (annotation_cairo_context);

  cairo_restore (annotation_cairo_context);

  annotate_add_savepoint ();
}

/**
 * annotate_select_pen:
 *
 * Select the default pen tool for annotations.
 *
 * This function sets the current drawing context to the default pen,
 * updates the internal paint type to `ANNOTATE_PEN`, and updates the
 * cursor to reflect the selected pen's color, thickness, and arrow style.
 *
 * It also disallocates any previous cursor before creating the new one.
 * A debug message is printed indicating the selected pen color.
 *
 * If no default pen exists, the function does nothing.
 **/
void
annotate_select_pen (void)
{
  g_debug ("The pen with color %s has been selected\n",
           annotation_data->color);

  if (annotation_data->default_pen)
    {
      annotation_data->cur_context    = annotation_data->default_pen;
      annotation_data->old_paint_type = ANNOTATE_PEN;

      disallocate_cursor ();

      set_pen_cursor (&annotation_data->cursor, annotate_get_thickness (),
                      annotation_data->color, annotation_data->arrow);

      update_cursor ();
    }
}

/**
 * annotate_select_filler:
 *
 * Select the default filler tool for annotations.
 *
 * This function sets the current drawing context to the default filler,
 * updates the internal paint type to `ANNOTATE_FILLER`, and updates the
 * cursor to reflect the filler tool with the selected pen color.
 *
 * It also disallocates any previous cursor before creating the new one.
 * A debug message is printed indicating the selected color.
 *
 * If no default filler exists, the function does nothing.
 **/
void
annotate_select_filler (void)
{
  g_debug ("Select filler with pen color %s\n", annotation_data->color);

  if (annotation_data->default_pen)
    {
      annotation_data->cur_context    = annotation_data->default_filler;
      annotation_data->old_paint_type = ANNOTATE_FILLER;

      disallocate_cursor ();

      set_filler_cursor (&annotation_data->cursor, annotation_data->color);

      update_cursor ();
    }
}

/**
 * annotate_select_eraser:
 *
 * Select the default eraser tool for annotations.
 *
 * This function sets the current drawing context to the default eraser,
 * updates the internal paint type to `ANNOTATE_ERASER`, and updates the
 * cursor to reflect the eraser tool with the current thickness.
 *
 * It also disallocates any previous cursor before creating the new one.
 * A debug message is printed to indicate that the eraser has been selected.
 **/
void
annotate_select_eraser (void)
{
  g_debug ("The eraser has been selected\n");

  annotation_data->cur_context    = annotation_data->default_eraser;
  annotation_data->old_paint_type = ANNOTATE_ERASER;

  disallocate_cursor ();

  set_eraser_cursor (&annotation_data->cursor, annotate_get_thickness ());

  update_cursor ();
}

/**
 * annotate_acquire_grab:
 *
 * Ensures an input grab is active on the annotation window.
 *
 * If a grab is not already active, this function acquires one, ensuring
 * that subsequent mouse/stylus events are captured by the application.
 * It is idempotent and safe to call multiple times.
 */
void
annotate_acquire_grab (void)
{
  if (! annotation_data->is_grabbed)
    {
      g_debug ("Acquire grab\n");
      annotate_acquire_input_grab ();
      annotation_data->is_grabbed = TRUE;
    }
}

/**
 * annotate_draw_arrow:
 * @devdata:  Device data containing the stroke's coordinate list.
 * @distance: The length of the last segment of the stroke.
 *
 * Draws an arrowhead at the end of the current stroke if conditions are met.
 *
 * The arrow is only drawn if the stroke is long enough. The direction is
 * calculated from the last few points of the stroke.
 */
void
annotate_draw_arrow (AnnotateDeviceData *devdata, gdouble distance)
{
  gdouble direction          = 0;
  gdouble pen_width          = annotate_get_thickness ();
  gdouble arrow_minimum_size = pen_width * 2;

  AnnotatePoint *point;
  point = (AnnotatePoint *) g_slist_nth_data (devdata->coord_list, 0);

  if (distance < arrow_minimum_size)
    {
      return;
    }

  g_debug ("Draw arrow: ");

  if (g_slist_length (devdata->coord_list) < 2)
    {
      /*
       * If it has length lesser then two then is a point and
       * it has no sense draw the arrow.
       */
      return;
    }

  /* Postcondition length >= 2 */
  direction = annotate_get_arrow_direction (devdata);

  g_debug ("Arrow direction %f\n", direction / M_PI * 180);

  draw_arrow_in_point (point, pen_width, direction);
}

/**
 * annotate_fill:
 * @devdata:  Device data (unused).
 * @data:     The main #AnnotateData application context.
 * @x:        The x-coordinate of the point to start the fill from.
 * @y:        The y-coordinate of the point to start the fill from.
 *
 * Performs a flood-fill operation starting at the given coordinates.
 *
 * This function uses the current color to fill a contiguous area on the
 * annotation canvas and creates a new savepoint for the undo history.
 */
void
annotate_fill (AnnotateDeviceData *devdata,
               AnnotateData *data,
               gdouble x,
               gdouble y)
{
  g_debug ("Fill\n");
  cairo_save (annotation_data->annotation_cairo_context);
  select_color ();
  fill (annotation_data, x, y);
  cairo_restore (annotation_data->annotation_cairo_context);
  annotate_add_savepoint ();
}

/**
 * annotate_shape_recognize:
 * @devdata:     The device data containing the stroke to process.
 * @closed_path: %TRUE if the stroke is a closed shape.
 *
 * Triggers the shape recognition and beautification process on a completed
 * stroke.
 *
 * Based on the currently active mode (`rectify` or `roundify`), this
 * function calls the appropriate helper to convert the freehand stroke
 * into a cleaner geometric shape (e.g., rectangle, ellipse, or spline).
 */
void
annotate_shape_recognize (AnnotateDeviceData *devdata, gboolean closed_path)
{
  if (annotation_data->rectify)
    {
      rectify (devdata, closed_path);
    }
  else if (annotation_data->roundify)
    {
      roundify (devdata, closed_path);
    }
  else if (closed_path)
    {
      splinify (devdata);
    }
}

/**
 * annotate_select_tool:
 * @data:         The main #AnnotateData application context.
 * @masterdevice: The master pointer device.
 * @slavedevice:  The physical device that generated the event (e.g., stylus).
 * @state:        The current modifier state.
 *
 * Selects the appropriate tool based on the physical device in use.
 *
 * This function is used for devices like tablets that have a separate
 * eraser tip. It checks if the source device is an eraser and selects the
 * eraser tool accordingly, otherwise defaulting to the previously used tool.
 */
void
annotate_select_tool (AnnotateData *data, GdkDevice *masterdevice,
                      GdkDevice *slavedevice, guint state)
{
  AnnotateDeviceData *masterdata = g_hash_table_lookup (data->devdatatable,
                                                        masterdevice);

  AnnotateDeviceData *slavedata = g_hash_table_lookup (data->devdatatable,
                                                       slavedevice);

  if (slavedevice)
    {
      if (gdk_device_get_source (slavedevice) == GDK_SOURCE_ERASER)
        {
          annotate_select_eraser ();
          data->old_paint_type = ANNOTATE_PEN;
        }
      else
        {
          if (data->old_paint_type == ANNOTATE_ERASER)
            {
              annotate_select_eraser ();
            }
          else if (data->old_paint_type == ANNOTATE_PEN)
            {
              annotate_select_pen ();
            }
          else
            {
              annotate_release_grab ();
            }
        }
    }
  else
    {
      g_debug ("Attempt to select non existent device!\n");
      data->cur_context = data->default_pen;
    }

  masterdata->lastslave = slavedevice;
  masterdata->state     = state;
  slavedata->state      = state;
}

/**
 * annotate_paint_context_free:
 * @context: The #AnnotatePaintContext to free.
 *
 * A simple destructor for an #AnnotatePaintContext object.
 */
void
annotate_paint_context_free (AnnotatePaintContext *context)
{
  if (context)
    {
      g_free (context);
      context = (AnnotatePaintContext *) NULL;
    }
}

void
destroy_text_config (TextConfig *cfg)
{
  if (cfg == NULL)
    return;
  g_free (cfg);
}

/**
 * annotate_quit:
 *
 * Clean up and free all resources used by the annotation system.
 *
 * This function finalizes the annotation session by saving the current
 * configuration state, releasing allocated memory, destroying GTK widgets,
 * freeing cairo contexts, cursors, and clearing temporary directories.
 *
 * The cleanup process includes:
 * - Destroying background and annotation data structures
 * - Releasing color strings, input devices, and savepoint data
 * - Destroying annotation and auxiliary windows (recording, background, font)
 * - Freeing cairo paths, paint contexts (pen, eraser, filler), and cursors
 * - Removing temporary directories used during annotation
 * - Freeing the associated monitor structure
 *
 * This should be called before shutting down the application to avoid
 * memory leaks and dangling resources.
 **/
void
annotate_quit (void)
{
  /* destroy data structures of other contexts. */
  if (background_data)
    {
      destroy_background_data ();
    }

  if (text_config)
    {
      destroy_text_config (text_config);
      text_config = NULL;
    }

  if (annotation_data)
    {
      annotation_config_save_state (annotation_data);
      if (annotation_data->color)
        {
          g_free (annotation_data->color);
          annotation_data->color = NULL;
        }

      /* Destroy cursors. */
      disallocate_cursor ();
      cursors_main_quit ();

      if (annotation_data->font != NULL)
        {
          pango_font_description_free (annotation_data->font);
          annotation_data->font = NULL;
        }
      if (annotation_data->annotation_window)
        {
          gtk_widget_destroy (annotation_data->annotation_window);
          annotation_data->annotation_window = (GtkWidget *) NULL;
        }

      if (annotation_data->recordingstudio_options)
        {
          g_free (annotation_data->recordingstudio_options);
        }

      if (annotation_data->recordingstudio_window)
        {
          gtk_widget_destroy (annotation_data->recordingstudio_window);
          annotation_data->recordingstudio_window = (GtkWidget *) NULL;
        }

      remove_input_devices (annotation_data);
      annotate_savepoint_list_free ();

      delete_ardesia_tmp_dir ();

      if (annotation_data->savepoint_dir)
        {
          g_free (annotation_data->savepoint_dir);
          annotation_data->savepoint_dir = (gchar *) NULL;
        }

      for (GList *l = annotation_data->paths; l != NULL; l = l->next)
        {
          cairo_path_t *path = (cairo_path_t *) l->data;
          cairo_path_destroy (path);
        }
      g_list_free (annotation_data->paths);
      annotation_data->paths = NULL;

      if (annotation_data->default_pen)
        {
          annotate_paint_context_free (annotation_data->default_pen);
        }

      if (annotation_data->default_eraser)
        {
          annotate_paint_context_free (annotation_data->default_eraser);
        }

      if (annotation_data->default_filler)
        {
          annotate_paint_context_free (annotation_data->default_filler);
        }

      if (annotation_data->background_selection_window)
        {
          gtk_widget_destroy (annotation_data->background_selection_window);
          annotation_data->background_selection_window = NULL;
        }
      /* Destroy cairo object. */
      cairo_destroy (annotation_data->annotation_cairo_context);

      if (annotation_data->clapperboard_cairo_context)
        {
          cairo_destroy (annotation_data->clapperboard_cairo_context);
        }
      if (annotation_data->font_window)
        {
          gtk_widget_destroy (annotation_data->font_window);
          annotation_data->font_window = NULL;
        }
      if (annotation_data->monitor != NULL)
        {
          g_free (annotation_data->monitor);
          annotation_data->monitor = NULL;
        }
      g_free (annotation_data);
    }
}

/**
 * annotate_release_input_grab:
 *
 * Releases the input grab, allowing events to pass through the window.
 *
 * The implementation is platform-specific and may involve modifying the
 * window's input shape to make it 'click-through'.
 */
void
annotate_release_input_grab (void)
{
  g_debug ("annotate_release_input_grab\n");
  ungrab_pointer ();
  /*
   * @TODO implement correctly gtk_widget_input_shape_combine_mask
   * in the quartz gdkwindow or use an equivalent native function;
   * the current implementation in macosx this does not do nothing.
   */
#ifndef _WIN32
  /*
   * This allows the mouse event to be passed below the transparent annotation;
   * at the moment this call works only on Linux
   */

  const cairo_rectangle_int_t ann_rect = { 0, 0, 0, 0 };
  cairo_region_t             *r = cairo_region_create_rectangle (&ann_rect);
  gtk_widget_input_shape_combine_region (annotation_data->annotation_window, r);
  cairo_region_destroy (r);

#else
  /*
   * @TODO WIN32 implement correctly gtk_widget_input_shape_combine_mask
   * in the win32 gdkwindow or use an equivalent native function.
   * Now in the gtk implementation the gtk_widget_input_shape_combine_mask
   * call the gtk_widget_shape_combine_mask that is not the desired behaviour.
   *
   */

#endif
}

/**
 * annotate_release_grab:
 *
 * Releases the input grab if it is currently active.
 *
 * This function checks if the system has an input grab and calls the
 * appropriate helper to release it, updating the internal application state.
 */
void
annotate_release_grab (void)
{
  g_debug ("releasing grab (is_grabbed=%d)\n", annotation_data->is_grabbed);
  if (annotation_data->is_grabbed)
    {
      g_debug ("Release grab\n");
      annotate_release_input_grab ();
      annotation_data->is_grabbed = FALSE;
    }
}

/**
 * annotate_undo:
 *
 * Undo the last annotation action by reverting to the previous save point.
 *
 * This function moves the savepoint index forward in the list of saved
 * annotation states and restores the corresponding surface. It has no effect
 * if there are no save points, or if the current savepoint is already the
 * latest in the list.
 **/
void
annotate_undo (void)
{
  g_debug ("Undo\n");

  if (annotation_data->savepoint_list)
    {
      if (annotation_data->current_save_index !=
          g_slist_length (annotation_data->savepoint_list) - 1)
        {
          annotation_data->current_save_index += 1;
          annotate_restore_surface ();
        }
    }
}

/**
 * annotate_redo:
 *
 * Redo an annotation action by moving back to a more recent save point.
 *
 * This function decreases the savepoint index in the list of saved annotation
 * states and restores the corresponding surface.
 * It has no effect if there are no save points, or if the current savepoint
 * is already the oldest in the list.
 **/
void
annotate_redo (void)
{
  g_debug ("Redo\n");

  if (annotation_data->savepoint_list)
    {
      if (annotation_data->current_save_index != 0)
        {
          annotation_data->current_save_index -= 1;
          annotate_restore_surface ();
        }
    }
}

/**
 * annotate_clear_screen:
 *
 * Clear the annotation window by resetting the Cairo drawing context.
 *
 * This function clears any existing drawings in the annotation window's
 * Cairo context and sets the operator to `CAIRO_OPERATOR_SOURCE`.
 * It then queues a redraw of the annotation window, triggering an expose
 * event so that the cleared surface is visually updated.
 *
 * If no Cairo context exists, the function does nothing.
 **/
void
annotate_clear_screen (void)
{
  g_debug ("Clear annotation window\n");

  if (annotation_data->annotation_cairo_context)
    {
      /* clear existing cairo context */
      cairo_new_path (annotation_data->annotation_cairo_context);
      clear_cairo_context (annotation_data->annotation_cairo_context);
      cairo_set_operator (annotation_data->annotation_cairo_context,
                          CAIRO_OPERATOR_SOURCE);

      /* call for a redraw; it generates an expose event. */
      gtk_widget_queue_draw (annotation_data->annotation_window);
    }
}

/**
 * create_annotation_data:
 *
 * Allocates and initializes the main #AnnotateData struct.
 *
 * This function is called once at startup. It allocates the global
 * `annotation_data` struct and sets all its fields to their initial
 * default values.
 */
static void
create_annotation_data (void)
{
  annotation_data = g_malloc0 ((gsize) sizeof (AnnotateData));

  annotation_data->color = NULL;
  gchar *color           = g_strdup ("FFFF0088");
  annotate_set_color (color);
  g_free (color);

  /* Initialize the data structure. */
  annotation_data->is_background_visible      = FALSE;
  annotation_data->is_text_editor_visible     = FALSE;
  annotation_data->is_annotation_visible      = TRUE;
  annotation_data->is_window_covering_toolbar = TRUE; // err on side of caution
  annotation_data->is_opaque                  = FALSE;

  annotation_data->annotation_cairo_context = (cairo_t *) NULL;
  annotation_data->savepoint_list           = (GSList *) NULL;
  annotation_data->current_save_index       = 0;
  annotation_data->cursor                   = (GdkCursor *) NULL;
  annotation_data->devdatatable             = (GHashTable *) NULL;

  annotation_data->is_grabbed     = FALSE;
  annotation_data->arrow          = FALSE;
  annotation_data->rectify        = FALSE;
  annotation_data->roundify       = FALSE;
  annotation_data->old_paint_type = ANNOTATE_POINTER;

  annotation_data->default_pen = annotate_paint_context_new (ANNOTATE_PEN);

  annotation_data->default_eraser = annotate_paint_context_new (
      ANNOTATE_ERASER);

  annotation_data->default_filler = annotate_paint_context_new (
      ANNOTATE_FILLER);

  annotation_data->cur_context = annotation_data->default_pen;

  annotation_data->monitor = NULL;

  annotation_data->recordingstudio_window_gtk_builder = NULL;
  annotation_data->recordingstudio_window             = NULL;
  annotation_data->recordingstudio_options            = NULL;

  annotation_data->clapperboard_cairo_context = NULL;
  annotation_data->is_clapperboard_visible    = FALSE;

  annotation_data->cursor_window_gtk_builder = NULL;
  annotation_data->cursor_window             = NULL;
  annotation_data->is_cursor_visible         = FALSE;
  annotation_data->cursor_timer              = 0;
  annotation_data->cursor_step               = 0;

  annotation_data->background_selection_window     = NULL;
  annotation_data->background_selection_container  = NULL;
  annotation_data->background_button_data          = NULL;
  annotation_data->background_button_last_selected = BACKGROUND_NONE_SELECTED;

  annotation_data->pen_multiplier         = 1;
  annotation_data->eraser_multiplier      = 3;
  annotation_data->highlighter_multiplier = 3;

  annotation_data->font_window = NULL;
  annotation_data->font        = NULL;

  /*
   * We create background data objects at the same time to be safe
   */
  background_data = create_background_data ();
}

/**
 * annotate_init:
 * @monitor: the #Monitor where annotations will be drawn
 *
 * Initializes the annotation system and prepares it for use.
 *
 * This function sets up the core annotation data structures, loads
 * the saved configuration state, and initializes input devices and
 * cursors. It also ensures that a savepoint directory exists for
 * storing annotation snapshots.
 *
 * Typical usage is to call this function once during application
 * startup before any annotation actions are performed.
 **/
void
annotate_init (Monitor *monitor)
{
  cursors_main ();

  /* Setup AnnotateData object. */
  create_annotation_data ();

  annotation_config_load_state (annotation_data);

  /* Initialize the pen context. */
  annotation_data->monitor = monitor;

  setup_input_devices (annotation_data);

  create_savepoint_dir ();
}

/**
 * annotation_window_button_press:
 * @ev:   The #GdkEventButton for the mouse button press.
 * @data: A pointer to the main #AnnotateData struct.
 *
 * Handles a button press event in the annotation window.
 *
 * This function initializes the drawing context if needed, acquires the
 * input grab, computes the pressure, updates the cursor, and starts a new
 * stroke by storing the initial point.
 *
 * Returns: %TRUE if the event was handled, %FALSE otherwise.
 */
gboolean
annotation_window_button_press (GdkEventButton *ev, AnnotateData *data)
{
  if (data->is_text_editor_visible)
    {
      return FALSE;
    }
  GdkDevice *master = gdk_event_get_device ((GdkEvent *) ev);
  gdouble    x      = ev->x;
  gdouble    y      = ev->y;

  /* Get the data for this device. */
  GHashTable         *devdatatable = data->devdatatable;
  AnnotateDeviceData *masterdata;
  masterdata = g_hash_table_lookup (devdatatable, master);

  gdouble pressure = 1.0;

  if (data->cur_context == data->default_filler)
    {
      return FALSE;
    }

  if (! data->is_grabbed)
    {
      g_debug ("on_button_press: initialising cairo\n");
      initialize_annotation_cairo_context (data);
      if (! data->is_grabbed)
        {
          g_printerr ("on_button_press: initialising cairo failed\n");
          return FALSE;
        }
    }

  if (! ev)
    {
      g_printerr ("Device '%s': Invalid event; I ungrab all\n",
                  gdk_device_get_name (master));
      annotate_release_grab ();
      return FALSE;
    }

  g_debug ("Device '%s': Button %i Down at (x,y)= (%f : %f)\n",
           gdk_device_get_name (master), ev->button, x, y);

#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    {
      /* The point is inside the ardesia bar then ungrab. */
      annotate_release_grab ();
      return FALSE;
    }
#endif

  pressure = get_pressure ((GdkEvent *) ev);

  if (pressure <= 0)
    {
      return FALSE;
    }

  /* Acquires the grab capability. */
  initialize_annotation_cairo_context (data);

  annotate_configure_pen_options (data);

  annotate_coord_dev_list_free (masterdata);
  annotate_draw_point (masterdata, x, y, pressure);

  gdouble thickness = annotate_get_thickness ();

  annotate_coord_list_prepend (masterdata,
                               x,
                               y,
                               thickness,
                               pressure);

  return TRUE;
}

/**
 * annotation_window_mouse_move:
 * @ev:   The #GdkEventMotion for the mouse motion.
 * @data: A pointer to the main #AnnotateData struct.
 *
 * Handle mouse or stylus motion events in the annotation window.
 *
 * This function updates the current stroke by drawing lines between
 * successive points, applies pressure sensitivity, modifies color if
 * necessary.
 *
 * Returns: %TRUE if the event was handled, %FALSE otherwise.
 */
gboolean
annotation_window_mouse_move (GdkEventMotion *ev, AnnotateData *data)
{
  if (data->is_text_editor_visible)
    {
      return FALSE;
    }
  GdkDevice *master = gdk_event_get_device ((GdkEvent *) ev);
  if (! ev)
    {
      g_printerr ("Device '%s': Invalid event; I ungrab all\n",
                  gdk_device_get_name (master));
      annotate_release_grab ();
      return FALSE;
    }

  GdkDevice *slave = gdk_event_get_source_device ((GdkEvent *) ev);
  if (slave == NULL)
    {
      g_warning ("Could not find slave device.");
      return FALSE;
    }
  GHashTable *devdatatable = data->devdatatable;

  /* Get the data for this device. */
  AnnotateDeviceData *masterdata = g_hash_table_lookup (devdatatable, master);
  if (masterdata == NULL)
    {
      g_warning ("Could not find device data for master pointer.");
      return FALSE;
    }

  AnnotateDeviceData *slavedata = g_hash_table_lookup (devdatatable, slave);

  if (slavedata == NULL)
    {
      g_warning ("Could not find device data for slave pointer.");
      return FALSE;
    }

  if (data->cur_context == data->default_filler)
    {
      return FALSE;
    }

  if (ev->state != masterdata->state ||
      ev->state != slavedata->state ||
      masterdata->lastslave != slave)
    {
      annotate_select_tool (data, master, slave, ev->state);
    }

  gdouble pressure = 1.0;

  if (! data->is_grabbed)
    {
      return FALSE;
    }

  g_debug ("Device '%s': Move at (x,y)= (%f : %f)\n",
           gdk_device_get_name (master), ev->x, ev->y);

#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    {

      g_debug ("Device '%s': Move on the bar then ungrab\n",
               gdk_device_get_name (master));

      /* The point is inside the ardesia bar then ungrab. */
      annotate_release_grab ();
      return FALSE;
    }
#endif

  /* Only the first 5 buttons allowed. */
  if (! (ev->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK |
                      GDK_BUTTON4_MASK | GDK_BUTTON5_MASK)))
    {
      return TRUE;
    }

  initialize_annotation_cairo_context (data);

  annotate_configure_pen_options (data);

  gdouble thickness = annotate_get_thickness ();

  if (data->cur_context->type != ANNOTATE_ERASER)
    {
      pressure = get_pressure ((GdkEvent *) ev);

      if (pressure <= 0)
        {
          return FALSE;
        }

      /*
       * If the point is already selected and higher pressure then
       * print else jump it.
       */
      if (masterdata->coord_list)
        {
          AnnotatePoint *last_point = (AnnotatePoint *) g_slist_nth_data (
              masterdata->coord_list, 0);

          gdouble distance;
          distance = get_distance (last_point->x,
                                   last_point->y,
                                   ev->x,
                                   ev->y);
 
          if (distance < thickness)
            {
              /* Seems that you are uprising the pen. */
              if (pressure <= last_point->pressure)
                {
                  /* Jump the point you are uprising the hand. */
                  return FALSE;
                }
              else // pressure >= last_point->pressure
                {
                  /* Seems that you are pressing the pen more. */
                  annotate_modify_color (masterdata, data, pressure);
                  annotate_draw_line (masterdata, ev->x, ev->y, TRUE);
                  /*
                   * Store the new pressure without allocate
                   * a new coordinate.
                   */
                  last_point->pressure = pressure;
                  return TRUE;
                }
            }
        }
    }

  annotate_draw_line (masterdata, ev->x, ev->y, TRUE);
  annotate_coord_list_prepend (masterdata,
                               ev->x,
                               ev->y,
                               thickness,
                               pressure);

  return TRUE;
}

/**
 * save_closed_path:
 *
 * Saves a copy of the current Cairo path to a list for later use.
 *
 * This is used to store complex, closed paths that might need to be
 * redrawn or manipulated later.
 */
void
save_closed_path (void)
{
  cairo_t      *annotation_cr = annotation_data->annotation_cairo_context;
  cairo_path_t *path_copy     = cairo_copy_path (annotation_cr);
  annotation_data->paths = g_list_append (annotation_data->paths, path_copy);
}

/**
 * annotation_window_button_release:
 * @ev:   The #GdkEventButton for the mouse button release.
 * @data: A pointer to the main #AnnotateData struct.
 *
 * Handle a button release event in the annotation window.
 *
 * This function completes the current stroke, checks if the path should
 * be closed based on proximity to the starting point, draws arrows if
 * needed, updates the stroke on the Cairo context, and creates a new
 * undo savepoint.
 *
 * Returns: %TRUE if the event was handled, %FALSE otherwise.
 */
gboolean
annotation_window_button_release (GdkEventButton *ev, AnnotateData *data)
{
  if (data->is_text_editor_visible)
    {
      return FALSE;
    }
  GdkDevice *master = gdk_event_get_device ((GdkEvent *) ev);

  /* Get the data for this device. */
  AnnotateDeviceData *masterdata = g_hash_table_lookup (data->devdatatable,
                                                        master);

  guint length = g_slist_length (masterdata->coord_list);

  if (! data->is_grabbed)
    {
      return FALSE;
    }

  if (! ev)
    {
      g_error ("Device '%s': Invalid event; I ungrab all\n",
               gdk_device_get_name (master));

      annotate_release_grab ();
      return FALSE;
    }

  g_debug ("Device '%s': Button %i Up at (x,y)= (%.2f : %.2f)\n",
           gdk_device_get_name (master), ev->button, ev->x, ev->y);

  /*
   * This was required to stop if from permanently holding on the screen
   * over the ardesia bar.
   */
  if (inside_bar_window (ev->x_root, ev->y_root))
    /* Point is in the ardesia bar. */
    {
      /* The last point was outside the bar then ungrab. */
      annotate_release_grab ();
      return FALSE;
    }

  if (data->cur_context == data->default_filler)
    {
      annotate_fill (masterdata, data, ev->x, ev->y);
      take_pen_tool ();
      annotate_select_pen ();
      return TRUE;
    }

  initialize_annotation_cairo_context (data);

  if (length > 2)
    {
      AnnotatePoint *first_point = (AnnotatePoint *) g_slist_nth_data (
          masterdata->coord_list, length - 1);
      AnnotatePoint *last_point;
      last_point = (AnnotatePoint *) g_slist_nth_data (masterdata->coord_list,
                                                       0);

      gdouble distance = get_distance (ev->x,
                                       ev->y,
                                       first_point->x,
                                       first_point->y);

      gdouble pressure = last_point->pressure;
      annotate_modify_color (masterdata, data, pressure);

      gdouble thickness = annotate_get_thickness ();
      gdouble gap = distance - thickness;
      const gdouble snap_tolerance = 20.0;
      gboolean closed_path = (gap < snap_tolerance);

      /*
       * If the distance between two point lesser than tolerance
       * they are the same point for me.
       */
      if (! closed_path)
        {
          /* Different point. */
          annotate_draw_line (masterdata, ev->x, ev->y, TRUE);
          annotate_coord_list_prepend (masterdata,
                                       ev->x,
                                       ev->y,
                                       annotate_get_thickness (),
                                       pressure);
        }
      else
        {
          /* Rounded to be the same point. */
          annotate_draw_line (masterdata, first_point->x, first_point->y, TRUE);

          annotate_coord_list_prepend (masterdata,
                                       first_point->x,
                                       first_point->y,
                                       annotate_get_thickness (),
                                       pressure);
        }

      if (data->cur_context->type != ANNOTATE_ERASER)
        {
          annotate_shape_recognize (masterdata, closed_path);

          /* If is selected an arrow type then I draw the arrow. */
          if (! closed_path && data->arrow)
            {
              /* Print arrow at the end of the path. */
              annotate_draw_arrow (masterdata, distance);
            }
        }
      if (closed_path)
        {
          cairo_close_path (data->annotation_cairo_context);
          save_closed_path ();
        }
    }

  cairo_stroke (data->annotation_cairo_context);

  annotate_add_savepoint ();

  return TRUE;
}

/**
 * annotation_window_change:
 * @width: The new width for the drawing surfaces.
 * @height: The new height for the drawing surfaces.
 *
 * A callback function for window configuration or resize events, used to
 * lazily initialize the background drawing context.
 *
 * If the background's Cairo context (`background_data->cr`) has not yet
 * been created, this function allocates it with the specified dimensions.
 **/
void
annotation_window_change (int width, int height)
{
  if (background_data->cr == NULL)
    {
      background_data->cr = create_new_context (width, height);
    }
}

/**
 * initialize_font:
 *
 * Initializes the default font for text annotations.
 *
 * This function attempts to load a font configuration from the user's
 * settings. If no configuration is found, it falls back to a default
 * Pango font description. The result is stored in `annotation_data->font`.
 */
void
initialize_font (void)
{
  annotation_data->font = font_config_load ();

  if (annotation_data->font == NULL)
    {
      annotation_data->font = pango_font_description_new ();
      pango_font_description_set_size (annotation_data->font,
                                       32 * PANGO_SCALE);
    }
}
