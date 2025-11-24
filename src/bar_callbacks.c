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
#include "bar.h"
#include "bar_callbacks.h"
#include "cairo_functions.h"
#include "color_selector.h"
#include "font_selector.h"
#include "info_dialog.h"
#include "iwb_saver.h"
#include "pdf_saver.h"
#include "preference_dialog.h"
#include "recorder.h"
#include "saver.h"
#include "share_confirmation_dialog.h"
#include "text_window.h"
#include "utils.h"

/**
 * on_bar_window_state_event:
 * @widget: The GTK widget (bar window) that received the event.
 * @event: The GDK window state event.
 * @func_data: Pointer to BarData structure.
 *
 * Handles window state changes for the bar (e.g., minimized/restore).
 * If the bar is iconified, releases the annotation lock.
 *
 * Returns: TRUE to stop further handling.
 */
G_MODULE_EXPORT gboolean
on_bar_window_state_event (GtkWidget *widget,
                           GdkEventWindowState *event,
                           gpointer func_data)
{
  g_debug ("on bar state event\n");
  BarData       *bar_data = (BarData *) func_data;
  GdkWindow     *win      = gtk_widget_get_window (widget);
  GdkWindowState state    = gdk_window_get_state (win);

  /* Track the minimized signals */
  if (state & GDK_WINDOW_STATE_ICONIFIED)
    {
      release_lock (bar_data);
    }

  return TRUE;
}

/**
 * on_bar_draw_event:
 * @widget: The GTK widget that requested redraw.
 * @cr: The Cairo context for drawing.
 * @user_data: Pointer to user data (BarData).
 *
 * Called when the bar needs to be redrawn.
 *
 * Returns: FALSE to propagate further drawing handling.
 */
G_MODULE_EXPORT gboolean
on_bar_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  g_debug ("bar draw event\n");
  return FALSE;
}

/**
 * on_bar_hide_event:
 * @widget: The GTK widget being hidden.
 * @user_data: Pointer to user data (BarData).
 *
 * Triggered when the bar window is hidden.
 */
void
on_bar_hide_event (GtkWidget *widget, gpointer user_data)
{
  g_debug ("bar hide event\n");
}

/**
 * on_bar_configure_event:
 * @widget: The GTK widget being configured.
 * @event: The configure event.
 * @func_data: Pointer to BarData.
 *
 * Called when the bar window is first configured or resized.
 * Sets default options and starts the currently active tool.
 *
 * Returns: TRUE to stop further event propagation.
 */
G_MODULE_EXPORT gboolean
on_bar_configure_event (GtkWidget *widget, GdkEvent *event, gpointer func_data)
{
  g_debug ("bar configure event (%d)\n", bar_data->screenshot_pending);
  if (! bar_data->screenshot_pending)
    {
      set_options (bar_data);
    }
  start_tool (bar_data);
  return TRUE;
}

/**
 * on_bar_quit:
 * @toolbutton: The quit tool button.
 * @func_data: Pointer to BarData.
 *
 * Handles the quit action. Stops recording, releases grabs,
 * exports IWB/PDF files, cleans up annotation data, and exits GTK main loop.
 *
 * Returns: FALSE.
 */
G_MODULE_EXPORT gboolean
on_bar_quit (GtkToolButton *toolbutton, gpointer func_data)
{
  g_debug ("on_bar_quit\n");

  BarData *bar_data = (BarData *) func_data;

  stop_recorder ();

  bar_data->grab = FALSE;
  /* Release grab. */
  annotate_release_grab ();

  export_iwb (get_iwb_filename ());
  quit_pdf_saver ();
  // start_share_dialog ();

  if (is_text_toggle_tool_button_active ())
    {
      annotation_data->text_tool = TRUE;
    }
  else
    {
      annotation_data->text_tool = FALSE;
    }

  /* handles removal of background and text data structures */
  annotate_quit ();

  /* Quit the gtk engine. */
  gtk_main_quit ();

  return FALSE;
}

/**
 * on_bar_info:
 * @toolbutton: The info tool button.
 * @func_data: Pointer to BarData.
 *
 * Opens the info dialog window while temporarily releasing the annotation grab.
 *
 * Returns: TRUE.
 */
G_MODULE_EXPORT gboolean
on_bar_info (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;

  /* Release grab. */
  annotate_release_grab ();

  /* Start the info dialog. */
  start_info_dialog (toolbutton, GTK_WINDOW (get_bar_widget ()));

  bar_data->grab = grab_value;
  start_tool (bar_data);
  return TRUE;
}

/**
 * on_bar_enter_notify_event:
 * @widget: The GTK widget for leave events.
 * @event: The GDK event data.
 * @func_data: Pointer to BarData.
 *
 * Handle mouse entering the bar window. Starts the active tool
 * or stops text editing if appropriate.
 *
 * Returns: TRUE.
 */
G_MODULE_EXPORT gboolean
on_bar_enter_notify_event (GtkWidget *widget,
                           GdkEvent *event,
                           gpointer func_data)
{
  g_debug ("bar enter notify event\n");
  if (is_text_toggle_tool_button_active ())
    {
      stop_text_widget ();
    }
  return TRUE;
}

/**
 * on_bar_leave_notify_event:
 * @widget: The GTK widget for enter events.
 * @event: The GDK event data.
 * @func_data: Pointer to BarData.
 *
 * Handle mouse leaving the bar window. Starts the active tool
 * or stops text editing if appropriate.
 *
 * Returns: TRUE.
 */
G_MODULE_EXPORT gboolean
on_bar_leave_notify_event (GtkWidget *widget,
                           GdkEvent *event,
                           gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  start_tool (bar_data);
  return TRUE;
}

/**
 * on_bar_pointer_activate:
 * @toolbutton: The tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activate the corresponding annotation tool (pointer, text, pen mode,
 * thickness, arrow, pencil, highlighter, filler, eraser, screenshot,
 * PDF page add, show/hide, recorder, fonts, preferences, undo/redo,
 * clear, color selector, predefined colors).
 *
 * Each function updates BarData, manages annotation grab, updates icons,
 * and triggers necessary drawing or dialogs.
 */
G_MODULE_EXPORT void
on_bar_pointer_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  release_lock (bar_data);
}

/**
 * on_bar_text_activate:
 * @toolbutton: The text tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activates the text annotation tool and locks the annotation context.
 */
G_MODULE_EXPORT void
on_bar_text_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  g_debug ("Text tool selected");
}

/**
 * on_bar_mode_activate:
 * @toolbutton: The mode tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Switches the bar to pen mode and updates the tool settings accordingly.
 */
G_MODULE_EXPORT void
on_bar_mode_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  take_pen_tool ();
  setup_bar_mode (toolbutton, bar_data);
}

/**
 * on_bar_thick_activate:
 * @toolbutton: The thickness tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Cycles through brush thicknesses (micro, thin, medium, thick) and updates
 * the icon on the tool button.
 */
G_MODULE_EXPORT void
on_bar_thick_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;

  if (bar_data->thickness == MICRO_THICKNESS)
    {
      g_debug ("Brush thickness set to thin");
      /* Set the thin icon. */
      select_thickness (toolbutton, "thin");
      bar_data->thickness = THIN_THICKNESS;
    }
  else if (bar_data->thickness == THIN_THICKNESS)
    {
      g_debug ("Brush thickness set to medium");
      /* Set the medium icon. */
      select_thickness (toolbutton, "medium");
      bar_data->thickness = MEDIUM_THICKNESS;
    }
  else if (bar_data->thickness == MEDIUM_THICKNESS)
    {
      g_debug ("Brush thickness set to thick");
      /* Set the thick icon. */
      select_thickness (toolbutton, "thick");
      bar_data->thickness = THICK_THICKNESS;
    }
  else if (bar_data->thickness == THICK_THICKNESS)
    {
      g_debug ("Brush thickness set to micro");
      /* Set the micro icon. */
      select_thickness (toolbutton, "micro");
      bar_data->thickness = MICRO_THICKNESS;
    }
}

/**
 * on_bar_arrow_activate:
 * @toolbutton: The arrow tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activates the arrow annotation tool and sets its color to the current color.
 */
G_MODULE_EXPORT void
on_bar_arrow_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  gchar *color = g_strdup (bar_data->color);
  set_color (bar_data, color);
  g_free (color);
  g_debug ("Arrow tool selected");
}

/**
 * on_bar_pencil_activate:
 * @toolbutton: The pencil tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activates the pencil annotation tool and sets its color to the current color.
 */
G_MODULE_EXPORT void
on_bar_pencil_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  gchar *color = g_strdup (bar_data->color);
  set_color (bar_data, color);
  g_free (color);
  g_debug ("Pencil tool selected");
}

/**
 * on_bar_highlighter_activate:
 * @toolbutton: The highlighter tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activates the highlighter annotation tool and sets its color to the
 * current color.
 */
G_MODULE_EXPORT void
on_bar_highlighter_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  gchar *color = g_strdup (bar_data->color);
  set_color (bar_data, color);
  g_free (color);
  g_debug ("Highlighter tool selected");
}

/**
 * on_bar_filler_activate:
 * @toolbutton: The filler tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activates the filler annotation tool.
 */
G_MODULE_EXPORT void
on_bar_filler_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  annotate_select_filler ();
  g_debug ("Filler tool selected");
}

/**
 * on_bar_eraser_activate:
 * @toolbutton: The eraser tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Activates the eraser annotation tool.
 */
G_MODULE_EXPORT void
on_bar_eraser_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  annotate_select_eraser ();
  g_debug ("Eraser tool selected");
}

/**
 * on_bar_screenshot_activate:
 * @toolbutton: The screenshot tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Captures a screenshot of the annotation window. Temporarily releases grab
 * and restores it afterward.
 */
G_MODULE_EXPORT void
on_bar_screenshot_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;
  /* Release grab. */
  annotate_release_grab ();
  g_debug ("Taking screenshot");
  gdk_window_set_cursor (gtk_widget_get_window (get_annotation_window ()),
                         (GdkCursor *) NULL);
  start_save_image_dialog ();
  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/**
 * on_bar_add_pdf_activate:
 * @toolbutton: The add PDF tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Adds the current annotation content as a page to the PDF.
 */
G_MODULE_EXPORT void
on_bar_add_pdf_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;

  /* Release grab. */
  annotate_release_grab ();

  g_debug ("Exporting as PDF");
  add_pdf_page (GTK_WINDOW (get_bar_widget ()));
  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/**
 * on_bar_showhide_activate:
 * @toolButton: The show/hide annotations button clicked.
 * @func_data: Pointer to BarData.
 *
 * Toggles the visibility of annotations. Saves a snapshot when hiding,
 * restores it when showing, and updates the icon and tooltip accordingly.
 */
G_MODULE_EXPORT void
on_bar_showhide_activate (GtkToolButton *toolButton, gpointer func_data)
{
  BarData *bar_data              = (BarData *) func_data;
  gboolean annotation_is_visible = bar_data->annotation_is_visible;

  GtkWidget *window = get_annotation_window ();

  if (annotation_is_visible)
    {
      /** currently annotations are visible so icon is the hidden **/
      if (window != NULL)
        {
          g_debug ("Annotations hidden");

          /* Save current drawing into a cairo surface */
          if (bar_data->snapshot_surface)
            {
              cairo_surface_destroy (bar_data->snapshot_surface);
              bar_data->snapshot_surface = NULL;
            }

          {
            GdkWindow *gdk_win = gtk_widget_get_window (window);
            gint       width   = gdk_window_get_width (gdk_win);
            gint       height  = gdk_window_get_height (gdk_win);

            bar_data->snapshot_surface =
              cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

            cairo_t *cr = cairo_create (bar_data->snapshot_surface);
            gdk_cairo_set_source_window (cr, gdk_win, 0, 0);
            cairo_paint (cr);
            cairo_destroy (cr);
          }

          annotate_clear_screen ();

          bar_data->annotation_is_visible = FALSE;

          /* Set the stop tool-tip. */
          gtk_tool_item_set_tooltip_text ((GtkToolItem *) toolButton,
                                          gettext ("Show Annotations"));

          /* Put the show icon. */
          GtkImage *icon = get_image_from_builder (gettext ("show"));
          gtk_tool_button_set_icon_widget (toolButton, (GtkWidget *) icon);
        }
    }
  else
    {
      /** currently annotations are hidden so icon is the showing icon **/
      if (window != NULL)
        {
          g_debug ("Annotations visible");

          /* Restore saved drawing if available */
          if (bar_data->snapshot_surface)
            {
              cairo_t *annotation_cr;
              annotation_cr = annotation_data->annotation_cairo_context;

              cairo_set_source_surface (annotation_cr,
                                        bar_data->snapshot_surface,
                                        0, 0);
              cairo_paint (annotation_cr);
              gtk_widget_queue_draw (annotation_window);
            }

          bar_data->annotation_is_visible = TRUE;

          /* Set the stop tool-tip. */
          gtk_tool_item_set_tooltip_text ((GtkToolItem *) toolButton,
                                          gettext ("Hide Annotations"));

          /* Put the hide icon. */
          GtkImage *icon = get_image_from_builder (gettext ("hide"));
          gtk_tool_button_set_icon_widget (toolButton, (GtkWidget *) icon);
        }
    }
}

/**
 * on_bar_recorder_activate:
 * @toolbutton: The recorder tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Shows or creates the recording studio window for video/screen recording.
 */
G_MODULE_EXPORT void
on_bar_recorder_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  GError *error = (GError *) NULL;
  // we want to show the recording studio window at this point
  if (annotation_data->recordingstudio_window == NULL)
    {
      g_debug ("Showing recording menu");
      if (annotation_data->recordingstudio_options == NULL)
        {
          annotation_data->recordingstudio_options =
            g_malloc0 ((gsize) sizeof (RecordingStudioData));
        }

      // create new window  /* Initialize the main window. */
      annotation_data->recordingstudio_window_gtk_builder = gtk_builder_new ();

      GtkBuilder *recording_studio_gtk_builder;

      recording_studio_gtk_builder =
        annotation_data->recordingstudio_window_gtk_builder;

      gtk_builder_add_from_file (recording_studio_gtk_builder,
                                 RECORDINGSTUDIO_UI_FILE,
                                 &error);

      if (error)
        {
          g_debug ("Failed to load builder file: %s", error->message);
          g_error_free (error);
          return;
        }

      GObject   *recordingstudio_obj;
      GtkWidget *recordingstudio_window;

      recordingstudio_obj =
        gtk_builder_get_object (recording_studio_gtk_builder,
                                "recordingstudio_window");

      recordingstudio_window = GTK_WIDGET (recordingstudio_obj);
      annotation_data->recordingstudio_window = recordingstudio_window;

      gtk_window_set_transient_for (GTK_WINDOW (recordingstudio_window),
                                    GTK_WINDOW (get_bar_widget ()));

      if (annotation_data->recordingstudio_window == NULL)
        {
          g_debug ("Failed to create the recording studio window");
          return;
        }

      gtk_builder_connect_signals (recording_studio_gtk_builder,
                                   (gpointer) annotation_data);

      gtk_widget_show (annotation_data->recordingstudio_window);

      if (! annotation_data->recordingstudio_options->timer_state)
        {
          annotation_data->recordingstudio_options->timer_state =
              timer_state_new ();
        }
    }
  else
    {
      // just show the window again
      gtk_widget_show (annotation_data->recordingstudio_window);
    }
}

/**
 * on_bar_fonts_clicked:
 * @toolbutton: The fonts tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Opens the font selector dialog.
 */
G_MODULE_EXPORT void
on_bar_fonts_clicked (GtkToolButton *toolbutton, gpointer func_data)
{
  create_font_selector_window (GTK_WINDOW (get_bar_widget ()));
  show_font_selector_window ();
}

/**
 * on_bar_preferences_activate:
 * @toolbutton: The preferences tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Opens the bar preference window or shows the background selection window
 * if already created.
 */
G_MODULE_EXPORT void
on_bar_preferences_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  if (annotation_data->background_selection_window != NULL)
    {
      gtk_widget_show_all (annotation_data->background_selection_window);
    }
  else
    {
      create_bar_preference_window (GTK_WINDOW (get_bar_widget ()));
    }
}

/**
 * on_bar_undo_activate:
 * @toolbutton: The undo tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Undoes the last annotation action.
 */
G_MODULE_EXPORT void
on_bar_undo_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  annotate_undo ();
}

/**
 * on_bar_redo_activate:
 * @toolbutton: The redo tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Redoes the last undone annotation action.
 */
G_MODULE_EXPORT void
on_bar_redo_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  annotate_redo ();
}

/**
 * on_bar_clear_activate:
 * @toolbutton: The clear tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Clears the annotation screen and adds a savepoint for undo/redo.
 */
G_MODULE_EXPORT void
on_bar_clear_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  g_debug ("Screen has been cleared");
  annotate_clear_screen ();
  annotate_add_savepoint ();
}

/**
 * on_bar_color_activate:
 * @toolbutton: The color selector toggle tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Opens the color selector dialog, updates the annotation color if a valid
 * color is chosen, and restores the previous grab state.
 */
G_MODULE_EXPORT void
on_bar_color_activate (GtkToggleToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  gchar   *new_color  = "";

  if (! gtk_toggle_tool_button_get_active (toolbutton))
    {
      return;
    }

  /* Release grab. */
  annotate_release_grab ();

  bar_data->grab = FALSE;
  gdk_window_set_cursor (gtk_widget_get_window (get_annotation_window ()),
                         (GdkCursor *) NULL);
  new_color = start_color_selector_dialog (GTK_TOOL_BUTTON (toolbutton),
                                           GTK_WINDOW (get_bar_widget ()),
                                           bar_data->color);

  if (new_color) // if it is a valid color
    {
      set_color (bar_data, new_color);
      g_free (new_color);
    }

  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/**
 * on_bar_blue_activate:
 * @toolbutton: The blue color tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Sets the annotation color to blue.
 */
G_MODULE_EXPORT void
on_bar_blue_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gchar   *color    = g_strdup (BLUE);
  set_color (bar_data, color);
  g_free (color);
}

/**
 * on_bar_red_activate:
 * @toolbutton: The red color tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Sets the annotation color to red.
 */
G_MODULE_EXPORT void
on_bar_red_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gchar   *color    = g_strdup (RED);
  set_color (bar_data, color);
  g_free (color);
}

/**
 * on_bar_green_activate:
 * @toolbutton: The green color tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Sets the annotation color to green.
 */
G_MODULE_EXPORT void
on_bar_green_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gchar   *color    = g_strdup (GREEN);
  set_color (bar_data, color);
  g_free (color);
}

/**
 * on_bar_yellow_activate:
 * @toolbutton: The yellow color tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Sets the annotation color to yellow.
 */
G_MODULE_EXPORT void
on_bar_yellow_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gchar   *color    = g_strdup (YELLOW);
  set_color (bar_data, color);
  g_free (color);
}

/**
 * on_bar_white_activate:
 * @toolbutton: The white color tool button clicked.
 * @func_data: Pointer to BarData.
 *
 * Sets the annotation color to white.
 */
G_MODULE_EXPORT void
on_bar_white_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  gchar   *color    = g_strdup (WHITE);
  set_color (bar_data, color);
  g_free (color);
}
