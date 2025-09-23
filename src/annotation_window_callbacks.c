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

/*
 * Functions for handling various (GTK+)-Events.
 */

#include "annotation_window.h"
#include "annotation_window_callbacks.h"
#include "background_window.h"
#include "bar.h"
#include "cairo_functions.h"
#include "input.h"
#include "text_window.h"
#include "text_window_callbacks.h"
#include "utils.h"

/* On configure event. */
G_MODULE_EXPORT gboolean
on_configure (GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  AnnotateData  *data  = (AnnotateData *) user_data;
  GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (widget));

  g_debug ("Annotation window get configure event (%d,%d,%d,%d)\n",
           gtk_widget_get_allocated_width (widget),
           gtk_widget_get_allocated_height (widget),
	   state,
	   gtk_widget_is_focus (widget));
  if (! data->is_grabbed)
    {
      return FALSE;
    }

  int width  = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);
  if (data->is_background_visible == TRUE)
    {
      annotation_window_change (width, height);
      gtk_widget_set_opacity (data->annotation_window, 1.0);
    }
  if (data->is_annotation_visible == TRUE)
    {
      annotation_window_change (width, height);
    }
  if (data->is_text_editor_visible == TRUE)
    {
      annotation_window_change (width, height);
    }
  return TRUE;
}

G_MODULE_EXPORT gboolean
on_keypress (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  AnnotateData *data   = (AnnotateData *) user_data;
  GdkEventKey  *ev     = (GdkEventKey *) event;
  gboolean      retval = FALSE;

  g_debug ("Annotation on_keypress event (%d, %d)\n", ev->type, ev->keyval);

  if (data->is_text_editor_visible == TRUE)
    {
      retval = on_text_window_key_press_event (widget, event, text_data);
    }

  gtk_widget_queue_draw (widget);
  return retval;
}

G_MODULE_EXPORT gboolean
on_keyrelease (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{

  GdkEventKey *ev = (GdkEventKey *) event;
  g_debug ("Annotation on_keyrelease event (%d, %d)\n", ev->type, ev->keyval);
  return FALSE;
}

/** When the window changes z-order or state */
G_MODULE_EXPORT gboolean
on_window_state_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  return FALSE;
}

/*
 * On screen changed.
 * Required to make window transparent
 */
G_MODULE_EXPORT void
on_screen_changed (GtkWidget *widget,
		   GdkScreen *previous_screen,
		   gpointer user_data)
{
  g_debug ("Annotation window get screen-changed event\n");

  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (widget));
  GdkVisual *visual = gdk_screen_get_rgba_visual (screen);

  if (visual == NULL)
    {
      visual = gdk_screen_get_system_visual (screen);
    }

  gtk_widget_set_visual (widget, visual);
}

/*
 * Expose event: this occurs when the window is shown.
 */
G_MODULE_EXPORT gboolean
on_expose (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  AnnotateData *annotation_data = (AnnotateData *) user_data;

  g_debug ("Annotation window get draw event (grab: %d)\n", bar_data->grab);
  gboolean use_paint = TRUE;
  clear_cairo_context (cr); // blank the current window for repainting

  gint ann_width = 0, ann_height = 0, ann_x = 0, ann_y = 0;
  gtk_window_get_position (GTK_WINDOW (annotation_data->annotation_window),
                           &ann_x, &ann_y);
  gtk_window_get_size (GTK_WINDOW (annotation_data->annotation_window),
                       &ann_width, &ann_height);
  cairo_rectangle (cr, ann_x, ann_y, ann_width, ann_height);
  use_paint = TRUE;

  if (annotation_data->is_background_visible)
    {
      /* draw background layer on context cr */
      if (background_data->cr)
        {
          draw_cairo_context (cr, background_data->cr, use_paint);
        }
    }

  if (annotation_data->is_annotation_visible)
    {
      g_debug ("annotation_window_callbacks\n");
      /* draw annotation layer on context cr */
      initialize_annotation_cairo_context (annotation_data);
      draw_cairo_context (cr,
		          annotation_data->annotation_cairo_context,
			  use_paint);
    }

  if (annotation_data->is_text_editor_visible)
    {
      if (text_data->cr)
        {
          /* draw the text editor layer */
          draw_cairo_context (cr, text_data->cr, use_paint);
        }
    }

  /* draw clapperboard on top of everything else */
  if (annotation_data->is_clapperboard_visible)
    {
      if (annotation_data->clapperboard_cairo_context)
        {
          draw_cairo_context (cr,
			      annotation_data->clapperboard_cairo_context,
			      use_paint);
        }
    }

  return TRUE;
}

/*
 * Event-Handlers to perform the drawing.
 */

/* This is called when the button is pushed. */
G_MODULE_EXPORT gboolean
on_button_press (GtkWidget *win, GdkEventButton *ev, gpointer user_data)
{
  g_debug ("annotation_window:: on_button_press\n");
  AnnotateData *data   = (AnnotateData *) user_data;
  gboolean      retval = FALSE;

  if (data->is_annotation_visible == TRUE &&
      data->is_text_editor_visible == FALSE)
    {
      retval = annotation_window_button_press (ev, data);
    }

  return retval;
}

/* This shots when the pointer is moving. */
G_MODULE_EXPORT gboolean
on_motion_notify (GtkWidget *win, GdkEventMotion *ev, gpointer user_data)
{

  AnnotateData *data   = (AnnotateData *) user_data;
  gboolean      retval = FALSE;
  if (data->is_annotation_visible == TRUE &&
      data->is_text_editor_visible == FALSE)
    {
      retval = annotation_window_mouse_move (ev, data);
    }
  return retval;
}

/* This shots when the button is released. */
G_MODULE_EXPORT gboolean
on_button_release (GtkWidget *win, GdkEventButton *ev, gpointer user_data)
{
  g_debug ("annotation_window::on_button_release\n");
  AnnotateData *data   = (AnnotateData *) user_data;
  gboolean      retval = FALSE;
  if (data->is_text_editor_visible == TRUE)
    {
      retval = on_text_window_button_release (win, ev, text_data);
    }
  else if (data->is_annotation_visible == TRUE)
    {
      retval = annotation_window_button_release (ev, data);
    }
  return retval;
}

/* On device added. */
void
on_device_removed (GdkDeviceManager *device_manager,
		   GdkDevice *device,
		   gpointer user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;
  g_debug ("device '%s' removed\n", gdk_device_get_name (device));
  remove_input_device (device, data);
}

/* On device removed. */
void
on_device_added (GdkDeviceManager *device_manager,
		 GdkDevice *device,
		 gpointer user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;
  g_debug ("device '%s' added\n", gdk_device_get_name (device));
  add_input_device (device, data);
}
