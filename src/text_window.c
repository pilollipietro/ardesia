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

#include "annotation_window.h"
#include "bar_callbacks.h"
#include "cairo_functions.h"
#include "keyboard.h"
#include "text_window.h"
#include "text_window_callbacks.h"
#include "utils.h"

#ifdef _WIN32
#include "windows_utils.h"
#endif

/* The structure used to store the status. */
TextData *text_data = (TextData *) NULL;

/* The structure used to configure text input. */
TextConfig *text_config = (TextConfig *) NULL;

/**
 * create_text_config:
 *
 * Allocate and initialize a new TextConfig structure.
 *
 * This function sets default values for text rendering, including:
 *   - font family ("monospace")
 *   - left margin (0)
 *   - tab size (80)
 *   - starting X position (0)
 *
 * Returns:
 *   A pointer to the newly allocated and initialized TextConfig structure.
 **/
TextConfig *
create_text_config (void)
{
  TextConfig *text_config = g_malloc ((gsize) sizeof (TextConfig));
  text_config->fontfamily = "monospace";
  text_config->leftmargin = 0;
  text_config->tabsize    = 80;
  text_config->start_x    = 0;
  return text_config;
}

/**
 * destroy_text_properties:
 *
 * Free the memory associated with a CharInfo structure.
 *
 * This function releases the character string, color, font family,
 * optional background color, and the CharInfo structure itself.
 *
 * Parameters:
 *   data - pointer to a CharInfo structure to be freed.
 **/
void
destroy_text_properties (gpointer data)
{
  CharInfo *char_info = (CharInfo *) data;
  g_free (char_info->character);
  g_free (char_info->color);
  g_free (char_info->font_family);
  if (char_info->background_color)
    g_free (char_info->background_color);
  g_free (char_info);
}

/**
 * stop_timer:
 *
 * Stop the timer used for managing the blinking text cursor.
 *
 * If the timer is active (greater than 0), it removes the timer source
 * and resets the timer identifier to -1.
 **/
void
stop_timer (void)
{
  if (text_data->timer > 0)
    {
      g_source_remove (text_data->timer);
      text_data->timer = -1;
    }
}

/*
 * Draws the blinking cursor using the stable, unified font metrics.
 * The rectangle's height perfectly matches the font's ascent/descent.
 */
static gboolean
blink_cursor (gpointer data)
{
  if (text_data && text_data->pos && text_data->cr)
    {
      cairo_t *cr = text_data->cr;
      cairo_save (cr);

      /*
       * The rectangle's Y position is calculated starting from the
       * baseline (pos->y) and going up by the ascent height.
       */
      gdouble top_y  = text_data->pos->y - text_data->font_ascent;
      gdouble height = text_data->max_font_height;

      gdouble width = calculate_visual_thickness (text_data->pen_width,
                                                  (gint) height);
      if (width < 1.0)
        width = 1.0;

      if (text_data->blink_show)
        {
          cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
          cairo_set_source_color_from_string (cr, text_data->color);
          cairo_rectangle (cr, text_data->pos->x, top_y, width, height);
          cairo_fill (cr);
          text_data->blink_show = FALSE;
        }
      else
        {
          cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
          /* Slightly increase the clearing area for safety */
          cairo_rectangle (cr, text_data->pos->x - 1.0, top_y - 1.0,
                           width + 2.0, height + 2.0);
          cairo_fill (cr);
          text_data->blink_show = TRUE;
        }

      cairo_restore (cr);
      gtk_widget_queue_draw (annotation_data->annotation_window);
    }
  return TRUE;
}

/**
 * start_blink_cursor:
 *
 * Start the blinking cursor in the text annotation window.
 *
 * This function sets the blink state to visible, triggers an initial
 * cursor redraw, and starts a timer to toggle the cursor visibility
 * every 750 milliseconds.
 **/
void
start_blink_cursor (void)
{
  /* Start blink cursor every second. */
  text_data->blink_show = TRUE;
  blink_cursor (NULL);
  text_data->timer = g_timeout_add (750, blink_cursor, NULL);
}

/**
 * stop_blink_cursor:
 *
 * Stop the blinking cursor in the text annotation window.
 *
 * This function stops the blink timer, sets the blink state to hidden,
 * and redraws the cursor in its final state.
 **/
void
stop_blink_cursor (void)
{
  stop_timer ();
  text_data->blink_show = FALSE;
  blink_cursor (NULL);
}

/*
 * Calculates and stores the exact font metrics (ascent/descent).
 * This provides a stable "source of truth" for all vertical alignment.
 */
static void
set_cursor_height (GtkWidget *widget)
{
  PangoLayout *layout = gtk_widget_create_pango_layout (widget, "|");
  pango_layout_set_font_description (layout, annotation_data->font);

  PangoContext     *context = pango_layout_get_context (layout);
  PangoFontMetrics *metrics = pango_context_get_metrics (context,
                                                         annotation_data->font,
                                                         NULL);

  /* Calculate and store the fundamental metrics in pixels */
  text_data->font_ascent =
      (gdouble) pango_font_metrics_get_ascent (metrics) / PANGO_SCALE;
  text_data->font_descent =
      (gdouble) pango_font_metrics_get_descent (metrics) / PANGO_SCALE;
  text_data->max_font_height =
    text_data->font_ascent + text_data->font_descent;

  pango_font_metrics_unref (metrics);
  g_object_unref (layout);
}

/**
 * calculate_visual_thickness:
 *
 * Compute an effective visual stroke thickness for drawing tools.
 *
 * This function linearly interpolates the given `pen_width` within a
 * defined minimum and maximum range and scales it relative to the
 * `font_size`. It ensures that the resulting visual thickness is
 * perceptible and proportional to the text size.
 *
 * Parameters:
 *   pen_width - the raw pen width (input) to be normalized
 *   font_size - the current font size to scale the stroke
 *
 * Returns:
 *   A gdouble representing the effective visual thickness for rendering.
 *
 * The mapping ensures:
 *   - A minimum visible thickness (`OUTPUT_MIN`) is always respected.
 *   - Maximum thickness scales with the font size but is tunable via
 *     `FONT_SIZE_DIVISOR`.
 *   - Input pen_width outside the expected range is clamped safely.
 **/
gdouble
calculate_visual_thickness (gdouble pen_width, gint font_size)
{
  /* Set these to your defined min/max thickness values */
  const gdouble INPUT_MIN = 3.0;
  const gdouble INPUT_MAX = 18.0;

  /*
   * The absolute minimum visual thickness you want to see
   * (for MICRO_THICKNESS)
   */
  const gdouble OUTPUT_MIN = 1.5;

  /*
   * This is the main control knob.
   * A SMALLER number makes the maximum stroke THICKER.
   * A LARGER number makes the maximum stroke THINNER.
   * 6.0 is a good starting point for a very noticeable effect.
   */
  const gdouble FONT_SIZE_DIVISOR = 6;

  /* Calculate the maximum allowed thickness for this font size */
  gdouble output_max = (gdouble) font_size / FONT_SIZE_DIVISOR;
  if (output_max < OUTPUT_MIN)
    {
      output_max = OUTPUT_MIN;
    }

  /* Normalize the input pen_width to a 0.0-1.0 range */
  gdouble normalized_value = (pen_width - INPUT_MIN) / (INPUT_MAX - INPUT_MIN);
  /* Clamp the value between 0.0 and 1.0 to be safe */
  if (normalized_value < 0.0)
    normalized_value = 0.0;
  if (normalized_value > 1.0)
    normalized_value = 1.0;

  /*
   * Linearly interpolate (lerp) the normalized value within the output range.
   * This maps the input [3-18] to the output [OUTPUT_MIN - output_max].
   */
  return OUTPUT_MIN + normalized_value * (output_max - OUTPUT_MIN);
}

/*
 * Creates and assigns a "viewfinder" I-beam cursor, based on the final
 * specific requirements. The central stem of the I-beam perfectly matches
 * the font's height, with serifs added externally above and below.
 */
static gboolean
assign_text_cursor_to_window (GtkWidget *window)
{
  gdouble thickness = calculate_visual_thickness (
        text_data->pen_width, (gint) text_data->max_font_height);

  gdouble ascent = text_data->font_ascent;
  gdouble descent = text_data->font_descent;
  gdouble font_height = ascent + descent;

  gdouble serif_width = thickness + 8.0;

  /*
   * The total pixbuf height is the central stem (font_height)
   * plus the height of the two serifs (thickness * 2).
   */
  gint pix_height = (gint) ceil (font_height + (thickness * 2));
  gint pix_width  = (gint) ceil (serif_width);

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         pix_width,
                                                         pix_height);
  cairo_t         *cr      = cairo_create (surface);

  clear_cairo_context (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  GdkRGBA *fg_ptr = rgba_to_gdkcolor (text_data->color);
  if (fg_ptr)
    {
      cairo_set_source_rgba (cr, fg_ptr->red, fg_ptr->green, fg_ptr->blue,
                             fg_ptr->alpha);
      g_free (fg_ptr);
    }
  else
    {
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    }

  /* Draw the three components as separate, filled rectangles */
  gdouble center_x = pix_width / 2.0;

  /* Top serif: a rectangle at the very top of the pixbuf */
  cairo_rectangle (cr, center_x - serif_width / 2.0, 0, serif_width,
                   thickness);

  /* Central stem: a rectangle whose height matches the blinking cursor */
  cairo_rectangle (cr, center_x - thickness / 2.0, thickness, thickness,
                   font_height);

  /* Bottom serif: a rectangle at the very bottom */
  cairo_rectangle (cr, center_x - serif_width / 2.0, thickness + font_height,
                   serif_width, thickness);

  cairo_fill (cr);

  GdkPixbuf *pixbuf =
      gdk_pixbuf_get_from_surface (surface, 0, 0, pix_width, pix_height);

  /*
   * The hotspot must align with the text's baseline, which is located
   * 'ascent' pixels down from the top of the central stem area.
   */
  gdouble baseline_y = thickness + ascent;

  GdkCursor *cursor = gdk_cursor_new_from_pixbuf (
      gdk_window_get_display (gtk_widget_get_window (window)), pixbuf,
      pix_width / 2, (gint) baseline_y);

  gdk_window_set_cursor (gtk_widget_get_window (window), cursor);

  /* Cleanup */
  g_object_unref (cursor);
  g_object_unref (pixbuf);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  return TRUE;
}

/**
 * save_text:
 *
 * Add a save-point for the current text content.
 *
 * This function is called from `stop_text_widget`. It stops the blinking
 * cursor, pushes the current Cairo context to the annotation stack, and
 * frees the list of text characters (`letterlist`) after saving their
 * properties.
 *
 * If no text data or Cairo context is available, the function does nothing.
 **/
void
save_text (void)
{
  if (text_data != NULL)
    {
      stop_blink_cursor ();
      if (text_data->cr)
        {
          if (text_data->letterlist)
            {
              annotate_push_context (text_data->cr);
              g_slist_free_full (text_data->letterlist,
                                 (GDestroyNotify) destroy_text_properties);
              text_data->letterlist = NULL;
            }
        }
    }
}

/*Clear cairo context of text window. */
static void
clear_if_empty (void)
{
  if (! text_data->letterlist)
    {
      if (text_data->cr != NULL)
        {
          g_debug ("cleaning text window\n");
          clear_cairo_context (text_data->cr);
        }
    }
}

/* Initialization routine. Called on text expose. */
void
init_text_widget (GtkWidget *widget)
{
  set_cursor_height (widget);
  assign_text_cursor_to_window (widget);

#ifdef _WIN32
  grab_pointer (text_data->window, TEXT_MOUSE_EVENTS);
#endif

  clear_if_empty ();
}

/* Create text data. */
static void
create_text_data (void)
{
  g_debug ("create_text_data\n");
  if (text_data == NULL)
    {
      g_debug ("creating new text_data and adding defaults\n");
      text_data = g_malloc ((gsize) sizeof (TextData));

      /* set defaults back */
      text_data->cr                   = NULL;
      text_data->pos                  = g_malloc ((gsize) sizeof (Pos));
      text_data->pos->x               = 0.0;
      text_data->pos->y               = 0.0;
      text_data->letterlist           = NULL;
      text_data->virtual_keyboard_pid = (GPid) 0;
      text_data->timer                = -1;
      text_data->blink_show           = TRUE;
      text_data->color                = "FF0000FF";
      text_data->pen_width            = 1;
      text_data->font_ascent          = 0.0;
      text_data->font_descent         = 0.0;
      text_data->max_font_height      = 0.0;
    }
}

/**
 * start_text_widget:
 *
 * Start the text insertion widget for annotations.
 *
 * This function is triggered when the mouse leaves the toolbar. It creates
 * a new `TextData` structure, sets the pen color and thickness, creates a
 * new Cairo context for the text, initializes the text widget, and marks
 * the text editor as visible.
 *
 * Parameters:
 *   widget    - the GTK widget in which the text will be inserted
 *   color     - the color of the text
 *   thickness - the pen thickness for text rendering
 **/
void
start_text_widget (GtkWidget *widget, gchar *color, gint thickness)
{
  g_debug ("start_text_widget (%s)\n", color);
  create_text_data ();
  text_data->color     = color;
  text_data->pen_width = (gdouble) thickness;

  text_data->cr = create_new_context (
      gtk_widget_get_allocated_width (widget),
      gtk_widget_get_allocated_height (widget));

  init_text_widget (widget);
  annotation_data->is_text_editor_visible = TRUE;
}

/**
 * stop_text_widget:
 *
 * Stop the text insertion widget for annotations.
 *
 * This function is triggered when the mouse enters the toolbar again. It
 * hides the text editor, stops the blinking cursor and virtual keyboard,
 * saves the current text as a save-point, destroys the Cairo context,
 * frees allocated position and text data, and resets `text_data` to NULL.
 **/
void
stop_text_widget (void)
{
  annotation_data->is_text_editor_visible = FALSE;
  g_debug ("stop_text_widget\n");
  if (text_data)
    {
      stop_blink_cursor ();
      stop_virtual_keyboard ();

      /* destroys letter list and passes CR to annotation window */
      save_text ();

      if (text_data->cr)
        {
          cairo_destroy (text_data->cr);
          text_data->cr = NULL;
        }

      if (text_data->pos)
        {
          g_free (text_data->pos);
          text_data->pos = NULL;
        }

      g_free (text_data);
      text_data = NULL;
    }
}
