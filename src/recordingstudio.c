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

#include <gdk/gdk.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "annotation_window.h"
#include "background_window.h"
#include "bar.h"
#include "cairo_functions.h"
#include "recorder.h"
#include "recordingstudio.h"
#include "recordingstudio_callbacks.h"
#include "utils.h"

/**
 * setup_transparency:
 * @win: The #GtkWidget window to configure for transparency.
 *
 * Configures RGBA visual and enables transparency on the given window
 * if the screen compositor supports it.
 *
 * This function prepares the window for alpha blending by setting it
 * as app-paintable and applying an RGBA visual when available.
 */
static void
setup_transparency (GtkWidget *win)
{
  GdkScreen  *screen;
  GdkVisual  *visual;
  GdkDisplay *display;

  gtk_widget_set_app_paintable (win, TRUE);
  display = gtk_widget_get_display (win);
  screen  = gdk_display_get_default_screen (display);
  visual  = gdk_screen_get_rgba_visual (screen);

  if (visual != NULL && gdk_screen_is_composited (screen))
    {
      gtk_widget_set_visual (win, visual);
    }
}

/*
 * Draw the cursor with animated rings based on the current step.
 * Chooses a color that contrasts with the background.
 *
 * @param cr     Cairo context to draw on.
 * @param widget GTK widget that represents the drawing area.
 * @param state  The pre-calculated animation state.
 */
void
draw_video_cursor (cairo_t *cr, GtkWidget *widget, CursorAnimState *state)
{
  if (! annotation_data->is_cursor_visible)
    {
      cairo_set_source_rgba (cr, 0, 0, 0, 0);
      cairo_paint (cr);
      return;
    }
  gdouble r = state->r;
  gdouble g = state->g;
  gdouble b = state->b;
  gint    x = state->x;
  gint    y = state->y;

  /* Clear previous cursor */
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_paint (cr);

  /* Draw main cursor circle */
  cairo_save (cr);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgb (cr, r, g, b);
  cairo_translate (cr, x, y); // Use state position
  cairo_arc (cr, 0, 0, 5, 0, 2 * M_PI);
  cairo_stroke_preserve (cr);
  cairo_fill (cr);

  /* Draw animated rings based on cursor_step */
  int step = annotation_data->cursor_step / 10;
  for (int i = 1; i <= step; i++)
    {
      cairo_set_line_width (cr, 2);
      cairo_set_source_rgb (cr, r, g, b);
      cairo_arc (cr, 0, 0, 5 + i * 5, 0, 2 * M_PI);
      cairo_stroke (cr);
    }
  cairo_restore (cr);

  /* Update step for next frame */
  annotation_data->cursor_step = (annotation_data->cursor_step + 1) % 60;
}

/**
 * move_cursor_window:
 * @data: A pointer to #CursorAnimState.
 *
 * Timer callback that moves the cursor window to follow the mouse
 * position and updates animation state and colors dynamically.
 *
 * Returns: %G_SOURCE_CONTINUE to keep the timer running,
 *          %G_SOURCE_REMOVE to stop it.
 */
gboolean
move_cursor_window (gpointer data)
{
  if (! annotation_data->is_cursor_visible)
    {
      annotation_data->cursor_timer = 0;
      return G_SOURCE_REMOVE; // Stop timer
    }

  /* Cast the gpointer to our state struct */
  CursorAnimState *state = (CursorAnimState *) data;

  GtkWidget *cursor_window = annotation_data->cursor_window;

  /* Get mouse position and move window */
  gint        x_screen, y_screen;
  GdkDisplay *display = gtk_widget_get_display (cursor_window);
  GdkScreen  *screen  = gdk_display_get_default_screen (display);
  GdkWindow  *desktop = gdk_screen_get_root_window (screen);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);
  GdkDevice  *device  = gdk_seat_get_pointer (seat);

  gdk_window_get_device_position (desktop, device, &x_screen, &y_screen, NULL);

  gtk_window_move (GTK_WINDOW (cursor_window), x_screen - 32, y_screen - 32);

  /*
   * Heavy work: Calculate contrast and update the state
   */
  int        size = 16;
  GdkPixbuf *pb   = gdk_pixbuf_get_from_window (desktop, x_screen - size / 2,
                                                y_screen - size / 2,
                                                size,
                                                size);
  if (pb)
    {
      int            width      = gdk_pixbuf_get_width (pb);
      int            height     = gdk_pixbuf_get_height (pb);
      int            rowstride  = gdk_pixbuf_get_rowstride (pb);
      int            n_channels = gdk_pixbuf_get_n_channels (pb);
      unsigned char *pixels     = gdk_pixbuf_get_pixels (pb);
      int            sum        = 0;

      for (int j = 0; j < height; j++)
        {
          unsigned char *p = pixels + j * rowstride;
          for (int i = 0; i < width; i++)
            {
              sum += p[0] + p[1] + p[2]; /* ignore alpha */
              p += n_channels;
            }
        }

      if (width > 0 && height > 0)
        {
          int avg_pixel = sum / (width * height * 3);
          if (avg_pixel < 128)
            {
              /* dark background → light cursor, green */
              state->r = 0.0;
              state->g = 1.0;
              state->b = 0.0;
            }
          else
            {
              /* light background → dark cursor, yellow */
              state->r = 1.0;
              state->g = 1.0;
              state->b = 0.0; // Yellow
            }
        }
      g_object_unref (pb);
    }

  /* Update position in state */
  state->x = 32;
  state->y = 32;

  /* Force the cursor window to redraw */
  if (annotation_data->is_cursor_visible)
    {
      gtk_widget_queue_draw (cursor_window);
    }

  return G_SOURCE_CONTINUE; // Continue timer
}

/**
 * create_cursor_window:
 *
 * Creates and initializes the animated cursor popup window.
 *
 * The window is transparent, follows the mouse pointer and hosts a
 * drawing area where the animated cursor is rendered.
 *
 * Returns: A newly created #GtkWidget pointer representing the cursor window.
 */
GtkWidget *
create_cursor_window (void)
{
  g_debug ("Creating cursor\n");
  gint       size   = 64;
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);

  CursorAnimState *state   = g_new0 (CursorAnimState, 1);
  GtkWindow       *gtk_win = GTK_WINDOW (window);
  state->r                 = 1.0;
  state->g                 = 1.0;
  state->b                 = 0.0;
  state->x                 = size / 2;
  state->y                 = size / 2;

  /* Remove titlebar, resize controls etc */
  gtk_window_set_decorated (gtk_win, FALSE);

  GtkBuilder *builder = annotation_data->recordingstudio_window_gtk_builder;
  GtkWidget  *recorder_window = GTK_WIDGET (
      gtk_builder_get_object (builder, "recordingstudio_window"));

  gtk_window_set_transient_for (gtk_win, GTK_WINDOW (recorder_window));

  /* Remove close box */
  gtk_window_set_deletable (gtk_win, FALSE);

  /* Remove from taskbar */
  gtk_window_set_skip_taskbar_hint (gtk_win, TRUE);

  /* Remove from pager */
  gtk_window_set_skip_pager_hint (gtk_win, TRUE);

  /* Sets initial size */
  gtk_window_set_default_size (gtk_win, size, size);

  /* Sets minimum size */
  gtk_widget_set_size_request (window, size, size);

  /* Cannot be resized by user */
  gtk_window_set_resizable (gtk_win, FALSE);

  GtkWidget *drawing_area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (window), drawing_area);

  g_signal_connect (G_OBJECT (drawing_area),
                    "draw",
                    G_CALLBACK (on_draw_event),
                    state);

  g_object_set_data_full (G_OBJECT (window),
                          "cursor-anim-state",
                          state,
                          g_free);

  setup_transparency (window);
  return window;
}
