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
#include "bar_callbacks.h"
#include "cairo_functions.h"
#include "recorder.h"
#include "recordingstudio.h"
#include "utils.h"


G_MODULE_EXPORT void
on_record_click (GtkToggleButton *toolbutton, gpointer func_data)
{
  g_debug ("on_recording_click\n");
  gboolean    grab_value = bar_data->grab;
  GtkBuilder *recordingstudio_window_gtk_builder;
  recordingstudio_window_gtk_builder =
      annotation_data->recordingstudio_window_gtk_builder;

  if (is_started ())
    {
      if (is_paused ())
        {
          resume_recorder ();

          /* Put the stop icon. */
          GtkWidget *imageWidget = GTK_WIDGET (gtk_builder_get_object (
              recordingstudio_window_gtk_builder, "media-playback-stop"));

          gtk_button_set_image ((GtkButton *) toolbutton, imageWidget);
          gtk_button_set_label ((GtkButton *) toolbutton, "Pause");
        }
      else
        {
          pause_recorder ();

          /* Put the record icon. */
          GtkWidget *imageWidget = GTK_WIDGET (
            gtk_builder_get_object (recordingstudio_window_gtk_builder,
                                    "media-record"));

          gtk_button_set_image ((GtkButton *) toolbutton, imageWidget);
          gtk_button_set_label ((GtkButton *) toolbutton, "Record");

          g_debug ("Screen recorder stopped");
        }
    }
  else
    {

      if (! is_recorder_available ())
        {
          GtkWidget *imageWidget = GTK_WIDGET (
              gtk_builder_get_object (recordingstudio_window_gtk_builder,
                                      "media-recorder-unavailable"));

          gtk_button_set_image ((GtkButton *) toolbutton, imageWidget);
          gtk_button_set_label ((GtkButton *) toolbutton, "Unavailable");

          visualize_missing_recorder_program_dialog (
              GTK_WINDOW (get_bar_widget ()),
              gettext ("In order to record with Ardesia you must install the "
                       "vlc program and add it to the PATH environment "
                       "variable"));

          /* Put an icon that remember that the tool is not available. */
          bar_data->grab = grab_value;
          start_tool (bar_data);
          return;
        }

      g_debug ("Starting screen recorder");

      /* The recording is not active. */
      gboolean status;
      status = start_save_video_dialog ((GtkButton *) toolbutton,
                                        GTK_WINDOW (get_bar_widget ()));

      if (status)
        {
          GtkWidget *imageWidget = GTK_WIDGET (
              gtk_builder_get_object (recordingstudio_window_gtk_builder,
                                      "media-playback-stop"));

          gtk_button_set_image ((GtkButton *) toolbutton, imageWidget);
          gtk_button_set_label ((GtkButton *) toolbutton, "Pause");
        }
    }
  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/**
 * Stop the recording completely instead of pausing
 * @param  toolbutton [description]
 * @param  func_data  [description]
 * @return            [description]
 */
G_MODULE_EXPORT void
on_stop_recording_click (GtkButton *toolbutton, gpointer func_data)
{
  g_debug ("on_stop_recording_click\n");
  gboolean grab_value      = bar_data->grab;
  annotate_release_grab ();
  bar_data->grab = FALSE;
  stop_recorder ();
  GtkBuilder *builder    = annotation_data->recordingstudio_window_gtk_builder;
  GObject    *record_obj = gtk_builder_get_object (builder, "record");
  GtkToggleButton *recordButton = GTK_TOGGLE_BUTTON (record_obj);
  
  /*
   * Block the "clicked" signal (on_record_click) before
   * changing the state programmatically, to avoid the spurious event.
   */
  g_signal_handlers_block_by_func(record_obj, G_CALLBACK(on_record_click), func_data);
  
  gtk_toggle_button_set_active (recordButton, FALSE); // This is now safe
  
  /* Re-enable the signal handler */
  g_signal_handlers_unblock_by_func(record_obj, G_CALLBACK(on_record_click), func_data);
  
  GObject *media_record_obj = gtk_builder_get_object (builder, "media-record");
  GtkWidget *imageWidget    = GTK_WIDGET (media_record_obj);
  gtk_button_set_image ((GtkButton *) recordButton, imageWidget);
  gtk_button_set_label ((GtkButton *) recordButton, "Record");
  bar_data->grab           = grab_value;
  start_tool (bar_data);
}


void
get_desktop_mouse_location (int *x, int *y)
{
  GdkScreen  *screen  = gdk_screen_get_default ();
  GdkWindow  *desktop = gdk_screen_get_root_window (screen);
  GdkDisplay *display = gdk_display_get_default ();
  GdkSeat    *seat    = gdk_display_get_default_seat (display);
  GdkDevice  *device  = gdk_seat_get_pointer (seat);
  gdk_window_get_device_position (desktop, device, x, y, NULL);
}

gboolean
move_cursor_window (gpointer data)
{
  if (annotation_data->is_cursor_visible == FALSE)
    {
      annotation_data->cursor_timer = 0;
      return G_SOURCE_REMOVE; // Stop timer
    }

  /* 1. Cast the gpointer to our state struct (La tua intuizione giusta) */
  CursorAnimState *state = (CursorAnimState *) data;
  
  /* 2. Get mouse position and move window (Tuo codice originale) */
  gint        x_screen, y_screen;
  GdkScreen  *screen  = gdk_screen_get_default ();
  GdkWindow  *desktop = gdk_screen_get_root_window (screen);
  GdkDisplay *display = gdk_display_get_default ();
  GdkSeat    *seat    = gdk_display_get_default_seat (display);
  GdkDevice  *device  = gdk_seat_get_pointer (seat);

  gdk_window_get_device_position (desktop, device, &x_screen, &y_screen, NULL);
      
  GtkWidget *cursor_window = annotation_data->cursor_window;
  gtk_window_move (GTK_WINDOW (cursor_window), x_screen - 32, y_screen - 32);

  /*
   * 3. Heavy work: Calculate contrast and update the state
   */
  int size = 16;
  GdkPixbuf *pb = gdk_pixbuf_get_from_window (desktop,
                                              x_screen - size/2,
                                              y_screen - size/2,
                                              size,
                                              size);
  if (pb)
    {
      int width = gdk_pixbuf_get_width (pb);
      int height = gdk_pixbuf_get_height (pb);
      int rowstride = gdk_pixbuf_get_rowstride (pb);
      int n_channels = gdk_pixbuf_get_n_channels (pb);
      unsigned char *pixels = gdk_pixbuf_get_pixels (pb);
      int sum = 0;

      for (int j = 0; j < height; j++)
        {
          unsigned char *p = pixels + j*rowstride;
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
              /* dark background → light cursor */
              state->r = 0.0; state->g = 1.0; state->b = 0.0; // Green
            }
	  else
	    {
              /* light background → dark cursor */
              state->r = 1.0; state->g = 1.0; state->b = 0.0; // Yellow
            }
      }
        g_object_unref (pb);
    }

  /* 4. Update position in state (centro della finestra 64x64) */
  state->x = 32;
  state->y = 32;

  /* 5. Force the cursor window to redraw (questo chiama on_draw_event) */
  if (annotation_data->is_cursor_visible)
    {
      gtk_widget_queue_draw(cursor_window);
    }

  return G_SOURCE_CONTINUE; // Continue timer
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
  gint x = state->x;
  gint y = state->y;

  /* Clear previous cursor */
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_paint (cr);

  /* Draw main cursor circle */
  cairo_save(cr);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgb (cr, r, g, b);
  cairo_translate (cr, x, y); // Use state position
  cairo_arc (cr, 0, 0, 5, 0, 2*M_PI);
  cairo_stroke_preserve (cr);
  cairo_fill (cr);

  /* Draw animated rings based on cursor_step */
  int step = annotation_data->cursor_step / 10;
  for (int i = 1; i <= step; i++)
    {
      cairo_set_line_width (cr, 2);
      cairo_set_source_rgb (cr, r, g, b);
      cairo_arc (cr, 0, 0, 5 + i*5, 0, 2*M_PI);
      cairo_stroke (cr);
    }
  cairo_restore(cr);

  /* Update step for next frame */
  annotation_data->cursor_step = (annotation_data->cursor_step + 1) % 60;
}

/**
 * Called by Gtk on draw event for cursor window
 * @param  widget    [description]
 * @param  cr        [description]
 * @param  user_data [description] (points to our CursorAnimState)
 * @return           [description]
 */
static gboolean
on_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  /* --- MODIFIED: Cast user_data to state and pass it on --- */
  CursorAnimState *state = (CursorAnimState *) user_data;
  draw_video_cursor (cr, widget, state);
  
  return FALSE;
}

static void
setup_transparency (GtkWidget *win)
{
  GdkScreen *screen;
  GdkVisual *visual;

  gtk_widget_set_app_paintable (win, TRUE);
  screen = gdk_screen_get_default ();
  visual = gdk_screen_get_rgba_visual (screen);

  if (visual != NULL && gdk_screen_is_composited (screen))
    {
      gtk_widget_set_visual (win, visual);
    }
}

/*
 * Manual creation of cursor window of 32x32 px size
 * @return [description]
 */
GtkWidget *
create_cursor_window (void)
{
  g_debug ("Creating cursor\n");
  gint       size   = 64;
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  
  CursorAnimState *state = g_new0(CursorAnimState, 1);
  GtkWindow *gtk_win = GTK_WINDOW (window);\
  state->r = 1.0; state->g = 1.0; state->b = 0.0; // Default Giallo
  state->x = size / 2;
  state->y = size / 2;
  
  /* Remove titlebar, resize controls etc */
  gtk_window_set_decorated (gtk_win, FALSE);

  GtkBuilder *builder = annotation_data->recordingstudio_window_gtk_builder;
  GtkWidget *recorder_window = GTK_WIDGET(
      gtk_builder_get_object(builder, "recordingstudio_window")
  );

  gtk_window_set_transient_for(gtk_win, GTK_WINDOW(recorder_window));

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

  g_object_set_data_full(G_OBJECT(window), "cursor-anim-state", state, g_free);
  
  setup_transparency (window);
  return window;
}

G_MODULE_EXPORT void
on_cursor_click (GtkToggleButton *toolbutton, gpointer func_data)
{
  g_debug ("on_cursor_click\n");
  GdkWindow *main_win = gtk_widget_get_window (annotation_data->annotation_window);
  if (!main_win) return;

  /*
   * vlc --screen-mouse-pointer does not work on linux so
   * instead what we want to do is show an image just under where
   * the mouse pointer is going to be
   */
   if (annotation_data->cursor_window == NULL)
     {
       /* Build cursor window. */
       g_debug ("Building cursor window\n");
       annotation_data->cursor_window = create_cursor_window ();

       /* Empty region for click through */
       const cairo_rectangle_int_t empty_rect = { 0, 0, 0, 0 };
       cairo_region_t *empty_region = cairo_region_create_rectangle (&empty_rect);
       g_object_set_data_full(G_OBJECT (annotation_data->cursor_window),
                                        "empty-input-region",
				        empty_region,
				        (GDestroyNotify)cairo_region_destroy);

       gtk_widget_show_all (annotation_data->cursor_window);

       cairo_region_t *region = g_object_get_data(
           G_OBJECT(annotation_data->cursor_window),
	   "empty-input-region");

       /* Apply the click-through shape AFTER displaying it */
       gtk_widget_input_shape_combine_region (annotation_data->cursor_window, region);

     }
  gboolean is_active = gtk_toggle_button_get_active(toolbutton);

  if (is_active)
    {
      g_debug("Cursor set to ACTIVE\n");
      gtk_widget_set_opacity (annotation_data->cursor_window, 1.0);
      annotation_data->is_cursor_visible = TRUE;

      if (annotation_data->cursor_timer == 0)
        {
          CursorAnimState *state = g_object_get_data(
              G_OBJECT (annotation_data->cursor_window), "cursor-anim-state");

          if (state)
            {
              move_cursor_window (state); // Chiamata iniziale
              annotation_data->cursor_timer = g_timeout_add (16, // 16ms ~= 60 FPS
                                                             move_cursor_window,
                                                             state);
            }
          else
            {
              g_warning("Could not retrieve 'cursor-anim-state' from cursor window!");
            }
        }
      gtk_button_set_label (GTK_BUTTON (toolbutton),
                            gettext("Enable Cursor Effect"));
    }
  else
    {
      g_debug("Cursor set to INACTIVE (hiding with opacity)\n");

      if (annotation_data->cursor_timer > 0)
        {
          g_source_remove (annotation_data->cursor_timer);
          annotation_data->cursor_timer = 0;
        }

      annotation_data->is_cursor_visible = FALSE;
      gtk_widget_set_opacity (annotation_data->cursor_window, 0.0);

      gtk_button_set_label (GTK_BUTTON (toolbutton),
                            gettext ("Disable Cursor Effect"));
    }
}

G_MODULE_EXPORT void
on_clapperboard_click (GtkToolButton *toolbutton, gpointer func_data)
{
  g_debug ("on_clapperboard_click");
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;
  annotate_release_grab ();
  GtkWidget *annotation_window = get_annotation_window ();
  if (annotation_data->clapperboard_cairo_context == NULL)
    {
      int width  = gtk_widget_get_allocated_width (annotation_window);
      int height = gtk_widget_get_allocated_height (annotation_window);
      annotation_data->clapperboard_cairo_context = create_new_context (width,
                                                                        height);
      load_color_onto_context (BLACK,
                               annotation_data->clapperboard_cairo_context);
    }

  annotation_data->is_clapperboard_visible = TRUE;

  /* Make the screen black and then go back to what it was before. */
  bar_data->grab = grab_value;
  start_tool (bar_data);
  gtk_widget_queue_draw (annotation_window);
  begin_clapperboard_countdown ();
}

G_MODULE_EXPORT void
on_new_click (GtkToolButton *toolbutton, gpointer func_data)
{
  g_debug ("on_new_click");
  stop_recorder ();
  GtkWidget *beginRecordingButton = GTK_WIDGET (gtk_builder_get_object (
      annotation_data->recordingstudio_window_gtk_builder, "record"));
  on_record_click ((GtkToggleButton *) beginRecordingButton, func_data);
}

G_MODULE_EXPORT void
on_recordingstudio_window_destroy_event (GtkWidget *widget,
                                         GdkEvent *event,
                                         gpointer data)
{

  g_debug ("recording studio window being destroyed\n");
}

G_MODULE_EXPORT gboolean
on_recordingstudio_window_delete_event (GtkWidget *widget,
                                        GdkEvent *event,
                                        gpointer data)
{
  gtk_widget_hide (widget);
  return TRUE;
}
