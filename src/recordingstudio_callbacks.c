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

#include "recordingstudio_callbacks.h"
#include "annotation_window.h"
#include "bar.h"
#include "cairo_functions.h"
#include "recorder.h"
#include "recorder_timer.h"
#include "recordingstudio.h"
#include "utils.h"

/**
 * timer_tick_callback:
 * @user_data: Unused.
 *
 * Called every second to update the timer display.
 * Only redraws if the timer is running.
 */
static gboolean
timer_tick_callback (gpointer user_data)
{
  if (! annotation_data->recordingstudio_options ||
      ! annotation_data->recordingstudio_options->timer_state)
    return TRUE;

  TimerState *state = annotation_data->recordingstudio_options->timer_state;

  if (state->running)
    {
      GtkBuilder *builder = annotation_data->recordingstudio_window_gtk_builder;
      GtkWidget  *timer_area = GTK_WIDGET (
          gtk_builder_get_object (builder, "timerDrawingArea"));

      if (timer_area)
        gtk_widget_queue_draw (timer_area);
    }

  return TRUE; // Keep timeout active
}

/**
 * on_record_click:
 * @button: The record toggle button.
 * @func_data: Unused user data.
 *
 * Handles the click on the record toggle button.
 *
 * If no recording is currently running, this callback checks for
 * recorder availability, opens the save dialog, and starts the
 * recording. It also updates the state of the record/pause/stop
 * buttons in the recording studio UI.
 */
G_MODULE_EXPORT void
on_record_click (GtkToggleButton *button, gpointer func_data)
{
  GtkWidget *ancestor = gtk_widget_get_ancestor (GTK_WIDGET(button),
                                                 GTK_TYPE_WINDOW);

  g_debug ("on_record_click\n");
  GtkBuilder *builder = annotation_data->recordingstudio_window_gtk_builder;
  annotation_data->recordingstudio_options->timer_tick_id =
      g_timeout_add_seconds (1, timer_tick_callback, NULL);
          
  if (! is_started ())
    {
      /* Check if recorder is available */
      if (! is_recorder_available ())
        {
          visualize_missing_recorder_program_dialog (
              GTK_WINDOW (ancestor),
              gettext ("In order to record with Ardesia you must install the "
                       "vlc program and add it to the PATH environment "
                       "variable"));

          GObject *imageObj = gtk_builder_get_object (builder,
                                                      "recorder_unavailable");

          gtk_button_set_image ((GtkButton *) button, GTK_WIDGET (imageObj));
          gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                                       gettext ("Unavailable"));
          return;
        }

      /* START recording */
      g_debug ("Starting recording\n");

      /* Open file chooser dialog */
      gboolean status = start_save_video_dialog ((GtkButton *) button,
                                                 GTK_WINDOW (ancestor));
      if (! status)
        {
          GObject *imageObj = gtk_builder_get_object (builder,
                                                      "recorder_unavailable");

          gtk_button_set_image ((GtkButton *) button, GTK_WIDGET (imageObj));
          gtk_widget_set_tooltip_text (GTK_WIDGET (button), gettext ("Error"));
          return;
        }

      timer_start (annotation_data->recordingstudio_options->timer_state);

      GObject *record_obj = gtk_builder_get_object (builder, "recordButton");
      gtk_widget_set_sensitive (GTK_WIDGET (record_obj), FALSE);

      GObject *pause_obj = gtk_builder_get_object (builder, "pauseButton");
      gtk_widget_set_sensitive (GTK_WIDGET (pause_obj), TRUE);

      GObject *stop_obj = gtk_builder_get_object (builder, "stopButton");
      gtk_widget_set_sensitive (GTK_WIDGET (stop_obj), TRUE);
    }
}

/**
 * on_pause_click:
 * @button: The pause toggle button.
 * @func_data: Unused user data.
 *
 * Handles the click on the pause toggle button.
 *
 * If the recording is running, it pauses it.
 * If the recording is already paused, it resumes it.
 */
G_MODULE_EXPORT void
on_pause_click (GtkToggleButton *button, gpointer func_data)
{
  g_debug ("on_pause_click\n");

  if (is_started () && ! is_paused ())
    {
      /* PAUSE recording */
      pause_recorder ();
      g_debug ("Pausing recording\n");
      if (annotation_data->recordingstudio_options->timer_state)
        timer_stop (annotation_data->recordingstudio_options->timer_state);
    }
  else if (is_paused ())
    {
      /* RESUME recording (unpause) */
      g_debug ("Resuming from pause\n");
      resume_recorder ();
      if (annotation_data->recordingstudio_options->timer_state)
        timer_start (annotation_data->recordingstudio_options->timer_state);
    }
}

/**
 * on_stop_click:
 * @button: The stop button.
 * @func_data: Unused user data.
 *
 * Handles the click on the stop button.
 *
 * Stops the recording process and resets the recording studio UI
 * to its idle state, restoring the initial sensitivity and toggle
 * states of the controls.
 *
 * Signal handlers are temporarily blocked to avoid recursive
 * activation when resetting toggle states.
 */
G_MODULE_EXPORT void
on_stop_click (GtkButton *button, gpointer func_data)
{
  g_debug ("on_stop_click\n");

  /* Stop recorder */
  stop_recorder ();

  if (annotation_data->recordingstudio_options->timer_state)
    {
      timer_reset (annotation_data->recordingstudio_options->timer_state);

      GtkWidget *timer_area = GTK_WIDGET (
          gtk_builder_get_object (
              annotation_data->recordingstudio_window_gtk_builder,
              "timerDrawingArea"
           )
      );

      if (timer_area)
        gtk_widget_queue_draw (timer_area);
    }

  /* Reset UI to idle state */
  GtkBuilder *builder = annotation_data->recordingstudio_window_gtk_builder;
  GtkWidget *record =
      GTK_WIDGET (gtk_builder_get_object (builder, "recordButton"));

  GtkWidget *pause  =
      GTK_WIDGET (gtk_builder_get_object (builder, "pauseButton"));

  GtkWidget *stop = GTK_WIDGET (button);

  g_signal_handlers_block_matched (record,
                                   G_SIGNAL_MATCH_FUNC,
                                   0, /* signal id - any */
                                   0, /* detail - none */
                                   NULL, /* closure - none */
                                   G_CALLBACK (on_record_click),
                                   NULL);

  g_signal_handlers_block_matched (pause,
                                   G_SIGNAL_MATCH_FUNC,
                                   0,
                                   0,
                                   NULL,
                                   G_CALLBACK (on_pause_click),
                                   NULL);

  g_signal_handlers_block_matched (stop,
                                   G_SIGNAL_MATCH_FUNC,
                                   0,
                                   0,
                                   NULL,
                                   G_CALLBACK (on_stop_click),
                                   NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (record), FALSE);
  gtk_widget_set_sensitive (record, TRUE);

  gtk_widget_set_sensitive (pause, FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pause), FALSE);

  gtk_widget_set_sensitive (stop, FALSE);

  g_signal_handlers_unblock_matched (record, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
                                     G_CALLBACK (on_record_click), NULL);

  g_signal_handlers_unblock_matched (pause, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
                                     G_CALLBACK (on_pause_click), NULL);

  g_signal_handlers_unblock_matched (stop, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
                                     G_CALLBACK (on_stop_click), NULL);
}

/**
 * on_clapperboard_release:
 * @button: The clapperboard button.
 * @func_data: Unused user data.
 *
 * Stops and clears the clapperboard visual effect.
 *
 * Destroys the temporary Cairo context used to render the
 * clapperboard overlay and triggers a redraw of the annotation
 * window.
 */
G_MODULE_EXPORT void
on_clapperboard_release (GtkButton *button, gpointer func_data)
{
  if (annotation_data->clapperboard_cairo_context == NULL)
    {
      return;
    }
  cairo_destroy (annotation_data->clapperboard_cairo_context);
  annotation_data->clapperboard_cairo_context = NULL;
  gtk_widget_queue_draw (annotation_window);
  return;
}

/**
 * on_clapperboard_click:
 * @button: The clapperboard button.
 * @func_data: Unused user data.
 *
 * Draws a visual clapperboard overlay on the annotation window.
 *
 * The effect consists of red corner markers drawn on a dedicated
 * Cairo surface and composited over the annotation window during
 * redraw. The overlay remains visible until the button is released.
 */
G_MODULE_EXPORT void
on_clapperboard_click (GtkButton *button, gpointer func_data)
{
  const gdouble line_width = 8.0;
  const gdouble opacity    = 0.8;
  gint          height = gtk_widget_get_allocated_height (annotation_window);
  const gdouble corner_size = height * 0.07;
  int           width = gtk_widget_get_allocated_width (annotation_window);

  if (annotation_data->clapperboard_cairo_context == NULL)
    {
      annotation_data->clapperboard_cairo_context = create_new_context (width,
                                                                        height);
    }

  cairo_set_operator (annotation_data->clapperboard_cairo_context,
                      CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba (annotation_data->clapperboard_cairo_context,
                         1.0, 0.0, 0.0, opacity);

  cairo_set_line_width (annotation_data->clapperboard_cairo_context,
                        line_width);

  cairo_set_line_cap (annotation_data->clapperboard_cairo_context,
                      CAIRO_LINE_CAP_SQUARE);

  cairo_move_to (annotation_data->clapperboard_cairo_context, 0, corner_size);
  cairo_line_to (annotation_data->clapperboard_cairo_context, 0, 0);
  cairo_line_to (annotation_data->clapperboard_cairo_context, corner_size, 0);

  cairo_move_to (annotation_data->clapperboard_cairo_context,
                 width - corner_size, 0);

  cairo_line_to (annotation_data->clapperboard_cairo_context, width, 0);

  cairo_line_to (annotation_data->clapperboard_cairo_context,
                 width, corner_size);

  cairo_move_to (annotation_data->clapperboard_cairo_context,
                 0, height - corner_size);

  cairo_line_to (annotation_data->clapperboard_cairo_context, 0, height);

  cairo_line_to (annotation_data->clapperboard_cairo_context,
                 corner_size, height);

  cairo_move_to (annotation_data->clapperboard_cairo_context,
                 width - corner_size, height);

  cairo_line_to (annotation_data->clapperboard_cairo_context, width, height);

  cairo_line_to (annotation_data->clapperboard_cairo_context,
                 width, height - corner_size);

  cairo_stroke (annotation_data->clapperboard_cairo_context);

  gtk_widget_queue_draw (annotation_window);
  return;
}

/**
 * on_draw_event:
 * @widget: The drawing area widget.
 * @cr: The Cairo drawing context.
 * @user_data: Pointer to the CursorAnimState.
 *
 * Draw callback for the cursor window drawing area.
 *
 * Delegates rendering to draw_video_cursor() using the provided
 * animation state.
 *
 * Returns: %FALSE to propagate the draw event further.
 */
gboolean
on_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  CursorAnimState *state = (CursorAnimState *) user_data;
  draw_video_cursor (cr, widget, state);
  return FALSE;
}

/**
 * on_cursor_click:
 * @button: The cursor toggle button.
 * @func_data: Unused user data.
 *
 * Toggles the animated cursor overlay.
 *
 * When enabled, creates and shows the cursor window and starts the
 * periodic animation timer.
 * When disabled, stops the timer and hides the cursor window.
 */
G_MODULE_EXPORT void
on_cursor_click (GtkToggleButton *button, gpointer func_data)
{
  g_debug ("on_cursor_click\n");

  /* Create cursor window if needed */
  if (annotation_data->cursor_window == NULL)
    {
      g_debug ("Building cursor window\n");
      annotation_data->cursor_window = create_cursor_window ();

      /* Empty region for click through */
      const cairo_rectangle_int_t empty_rect = { 0, 0, 0, 0 };
      cairo_region_t             *empty_region;
      empty_region = cairo_region_create_rectangle (&empty_rect);

      g_object_set_data_full (G_OBJECT (annotation_data->cursor_window),
                              "empty-input-region",
                              empty_region,
                              (GDestroyNotify) cairo_region_destroy);

      gtk_widget_show_all (annotation_data->cursor_window);

      cairo_region_t *region = g_object_get_data (
          G_OBJECT (annotation_data->cursor_window), "empty-input-region");

      gtk_widget_input_shape_combine_region (annotation_data->cursor_window,
                                             region);

    }

  gboolean is_active = gtk_toggle_button_get_active (button);

  if (is_active)
    {
      /* ENABLE cursor effect */
      g_debug ("Cursor effect enabled\n");
      gtk_widget_set_opacity (annotation_data->cursor_window, 1.0);
      annotation_data->is_cursor_visible = TRUE;

      if (annotation_data->cursor_timer == 0)
        {
          CursorAnimState *state = g_object_get_data (
              G_OBJECT (annotation_data->cursor_window), "cursor-anim-state");

          if (state)
            {
              move_cursor_window (state);
              gint timer_frequency          = 16; /* 16ms ~= 60 FPS */
              annotation_data->cursor_timer = g_timeout_add (timer_frequency,
                                                             move_cursor_window,
                                                             state);
            }
        }
    }
  else
    {
      /* DISABLE cursor effect */
      g_debug ("Cursor effect disabled\n");

      if (annotation_data->cursor_timer > 0)
        {
          g_source_remove (annotation_data->cursor_timer);
          annotation_data->cursor_timer = 0;
        }

      annotation_data->is_cursor_visible = FALSE;
      gtk_widget_set_opacity (annotation_data->cursor_window, 0.0);
    }
}

/**
 * on_recordingstudio_window_destroy:
 * @widget: The recording studio window.
 * @event: The GdkEvent associated with the destroy signal.
 * @data: Unused user data.
 *
 * Handles the destroy of the recording studio window.
 *
 * Currently only logs the event for debugging purposes.
 */
G_MODULE_EXPORT void
on_recordingstudio_window_destroy (GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data)
{
  g_debug ("Recording studio window being destroyed\n");
  // Stop timer tick
  if (annotation_data->recordingstudio_options->timer_tick_id > 0)
    {
      g_source_remove (annotation_data->recordingstudio_options->timer_tick_id);
      annotation_data->recordingstudio_options->timer_tick_id = 0;
    }

  // Free timer state
  if (annotation_data->recordingstudio_options->timer_state)
    {
      timer_state_free (annotation_data->recordingstudio_options->timer_state);
      annotation_data->recordingstudio_options->timer_state = NULL;
    }
  g_free (annotation_data->recordingstudio_options);
}

/**
 * on_recordingstudio_window_delete_event:
 * @widget: The recording studio window.
 * @event: The GdkEvent associated with the delete event.
 * @data: Unused user data.
 *
 * Handles the delete-event of the recording studio window.
 *
 * Instead of destroying the window, this callback hides it so that
 * it can be shown again without recreating it.
 *
 * Returns: %TRUE to stop further processing and prevent destruction.
 */
G_MODULE_EXPORT gboolean
on_recordingstudio_window_delete_event (GtkWidget *widget,
                                        GdkEvent  *event,
                                        gpointer   data)
{
  gtk_widget_hide (widget);
  return TRUE;
}

/**
 * on_timer_draw:
 * @widget: The timer drawing area.
 * @cr: Cairo context.
 * @user_data: Unused.
 *
 * Draw callback for the timer display.
 * or HH:MM:SS when running/paused.
 */
G_MODULE_EXPORT gboolean
on_timer_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
  cairo_paint (cr);

  if (! annotation_data->recordingstudio_options ||
      ! annotation_data->recordingstudio_options->timer_state)
    return FALSE;

  timer_draw_overlay (cr, annotation_data->recordingstudio_options->timer_state,
                      10, 10, 48);

  return FALSE;
}
