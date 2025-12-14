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
#include "text_input.h"
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
  if (char_info == NULL)
    {
      return;
    }

  g_free (char_info->character);
  g_free (char_info->color);
  g_free (char_info->font_family);
  g_free (char_info->background_color);

  g_free (char_info);
  char_info = NULL;
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

/**
 * blink_cursor:
 * @data: User data passed to the timeout function (unused).
 *
 * Toggles the visibility of the text cursor in the annotation window.
 * The cursor is drawn as a vertical rectangle aligned with the text's
 * baseline and scaled according to the current font metrics and pen width.
 * When visible, it is drawn with the current text color; when hidden, the
 * same area is cleared to maintain a blinking effect.
 *
 * Returns: TRUE to keep the timeout active for continuous blinking.
 **/
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
      GtkWidget *annotation_window = get_annotation_window ();

      gint dirty_x      = (gint) text_data->pos->x - 1;
      gint dirty_y      = (gint) top_y - 1;
      gint dirty_width  = (gint) width + 2;
      gint dirty_height = (gint) height + 2;

      gtk_widget_queue_draw_area (annotation_window,
                                  dirty_x,
                                  dirty_y,
                                  dirty_width,
                                  dirty_height);
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

/**
 * set_cursor_height:
 * @widget: GTK widget used to create a temporary Pango layout.
 *
 * Computes and stores precise font metrics (ascent, descent, and total height)
 * for the current annotation font. These metrics provide a stable reference
 * for vertical alignment of the blinking cursor and text rendering.
 **/
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

/**
 * assign_text_cursor_to_window:
 * @window: The GTK window to which the custom text cursor will be applied.
 *
 * Creates a custom "I-beam" text cursor for the specified window.
 * The central stem of the cursor matches the font height, while
 * small serifs are added at the top and bottom for visibility.
 *
 * The cursor hotspot is aligned with the text baseline to ensure
 * proper placement during text editing.
 *
 * Returns: TRUE on successful cursor assignment, FALSE if the
 *          necessary text data or metrics are uninitialized.
 **/
static gboolean
assign_text_cursor_to_window (GtkWidget *window)
{
  /* Add defensive checks to ensure data is initialized */
  if (! text_data || ! text_data->color || text_data->max_font_height <= 0)
    {
      g_warning ("assign_text_cursor_to_window called with uninitialized "
                 "data.");
      return FALSE;
    }

  gdouble thickness = calculate_visual_thickness (
        text_data->pen_width, (gint) text_data->max_font_height);

  gdouble ascent      = text_data->font_ascent;
  gdouble descent     = text_data->font_descent;
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
  if (text_data == NULL)
    {
      return; /* Nothing to do */
    }

  stop_blink_cursor ();

  /*
   * If a cairo context exists, push its final state to the main canvas.
   * This ensures that the last cursor blink (usually a clear) and any
   * drawn text are permanently rendered.
   */
  if (text_data->cr)
    {
      annotate_push_context (text_data->cr);
    }

  /*
   * Free the letterlist if it exists regardless of the state of the
   * cairo context.
   */
  if (text_data->letterlist)
    {
      g_slist_free_full (text_data->letterlist,
                         (GDestroyNotify) destroy_text_properties);
      text_data->letterlist = NULL;
    }
}

/**
 * clear_if_empty:
 *
 * Clears the Cairo drawing context of the text window if there are
 * no characters currently stored in the letter list.
 *
 * This ensures that the window does not retain stale drawings when
 * the text content is empty.
 **/
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

/**
 * init_text_widget:
 * @widget: The GTK widget representing the text window.
 *
 * Initializes the text widget when it is first exposed. This includes:
 *   - Calculating and storing font metrics for stable cursor height.
 *   - Creating and assigning the I-beam text cursor to the window.
 *   - Grabbing the pointer on Windows for proper mouse event handling.
 *   - Clearing the text window if no characters are present.
 **/
void
init_text_widget (GtkWidget *widget)
{
  set_cursor_height (widget);
  assign_text_cursor_to_window (widget);

#ifdef _WIN32
  GtkWidget *annotation_window = annotation_data->annotation_window;
  grab_pointer (annotation_window, TEXT_MOUSE_EVENTS);
#endif

  clear_if_empty ();
}

/**
 * create_text_data:
 *
 * Allocates and initializes the global TextData structure if it has not
 * been created yet. Sets up default values for:
 *   - Drawing context pointer
 *   - Cursor position
 *   - Character list
 *   - Virtual keyboard PID
 *   - Blinking cursor timer and visibility
 *   - Default color and pen width
 *   - Font metrics (ascent, descent, max height)
 *
 * This function ensures the text subsystem has a valid, ready-to-use
 * data structure before any text operations.
 **/
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

  if (text_data->cr != NULL)
    {
      cairo_destroy (text_data->cr);
      text_data->cr = NULL;
    }

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

/**
 * make_new_character:
 *
 * Allocates and initializes a new CharInfo structure for a character.
 *
 * Returns: A pointer to the new CharInfo object, or NULL if allocation fails.
 **/
static CharInfo *
make_new_character (void)
{
  CharInfo *char_info = g_new0 (CharInfo, 1);
  if (char_info == NULL)
    {
      g_error ("failed to create new character object\n");
      return NULL;
    }
  return char_info;
}

/**
 * invalidate_character_area:
 * @char_info: (transfer none): The #CharInfo structure containing
 *             the character's
 * position, Pango metrics, and visual properties.
 *
 * Calculates the screen area occupied by a single character and requests a
 * redraw of that specific area in the annotation window.
 *
 * The function:
 * - Computes the character's visual bounds using Pango's `ink_rect` and the
 * character's origin.
 * - Expands the bounds by the stroke thickness (`visual_thickness`) plus a
 * padding to ensure the entire stroke and its anti-aliasing artifacts are
 * included in the redrawn area.
 * - Calls `gtk_widget_queue_draw_area` to efficiently update only the minimal
 * area affected by the character.
 */
static void
invalidate_character_area (CharInfo *char_info)
{
  if (! char_info)
    return;

  GtkWidget *annotation_window = annotation_data->annotation_window;
  if (! annotation_window)
    return;

  gdouble stroke_expand = ceil (char_info->visual_thickness / 2.0) + 1.0;

  gdouble origin_x = char_info->x;
  gdouble origin_y = char_info->y -
                     (gdouble) char_info->baseline / PANGO_SCALE;

  PangoRectangle ink_rect = char_info->ink_rect;
  gint           dirty_x = (gint) floor (origin_x + ink_rect.x - stroke_expand);
  gint           dirty_y = (gint) floor (origin_y + ink_rect.y - stroke_expand);
  gint dirty_width       = (gint) ceil (ink_rect.width + stroke_expand * 2.0);
  gint dirty_height      = (gint) ceil (ink_rect.height + stroke_expand * 2.0);

  gtk_widget_queue_draw_area (annotation_window,
                              dirty_x, dirty_y,
                              dirty_width, dirty_height);
}

/**
 * draw_layout_with_thickness:
 * @cr: Cairo drawing context.
 * @layout: Pango layout containing the text glyphs.
 * @char_info: Character information including color, pen width, and font.
 *
 * Draws a PangoLayout with custom stroke thickness and fills it,
 * compensating for glyph offsets to align text correctly to the baseline.
 **/
static void
draw_layout_with_thickness (cairo_t *cr,
                            PangoLayout *layout,
                            CharInfo *char_info)
{
  GdkRGBA *color = rgba_to_gdkcolor (char_info->color);
  if (! color)
    {
      /* Fallback to solid black if color conversion fails */
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    }
  else
    {
      cairo_set_source_rgba (cr, color->red, color->green, color->blue,
                             color->alpha);
      g_free (color);
    }

  /*
   * Some glyphs (like '|') are drawn by Pango with a vertical offset within
   * their layout box. We get this offset (ink_rect.y) and translate the
   * canvas upwards to compensate, ensuring all glyphs align correctly to our
   * stable baseline.
   */
  PangoRectangle ink_rect;
  pango_layout_get_pixel_extents (layout, &ink_rect, NULL);
  cairo_save (cr);
  cairo_translate (cr, 0, ink_rect.y);

  /* Convert the text glyphs into a vector path. */
  pango_cairo_layout_path (cr, layout);

  /* Set the outline thickness from the character's properties. */
  gdouble line_width = calculate_visual_thickness (char_info->pen_width,
                                                   char_info->font_size);
  // cairo_set_line_width (cr, char_info->pen_width);
  cairo_set_line_width (cr, line_width);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  /*
   * Draw the outline (stroke) to "thicken" the letter.
   * We use _preserve to keep the path for the fill operation.
   */
  cairo_stroke_preserve (cr);

  /* Fill the inside of the letter */
  cairo_fill (cr);

  cairo_restore (cr); /* Reverts the translation */
}

/**
 * draw_character:
 * @cr: Cairo drawing context.
 * @char_info: Character to draw and associated properties.
 *
 * Renders a single character with proper thickness and spacing. Handles
 * word spacing for spaces, computes bounding boxes, and queues redraw
 * areas for the annotation window.
 **/
static void
draw_character (cairo_t *cr, CharInfo *char_info)
{
  if (cr)
    {
      cairo_save (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

      PangoLayout *layout = pango_cairo_create_layout (cr);
      pango_layout_set_font_description (layout,
                                         char_info->pango_font_description);
      pango_layout_set_text (layout, char_info->character, -1);
      pango_cairo_update_layout (cr, layout);

      char_info->baseline = pango_layout_get_baseline (layout);
      cairo_move_to (
          cr,
          char_info->x,
          char_info->y -
          (gdouble) char_info->baseline / PANGO_SCALE);

      draw_layout_with_thickness (cr, layout, char_info);

      PangoRectangle logical_rect;
      PangoRectangle ink_rect;
      pango_layout_get_pixel_extents (layout, &ink_rect, &logical_rect);
      char_info->ink_rect = ink_rect;

      gdouble visual_thickness = calculate_visual_thickness (
          char_info->pen_width, char_info->font_size);

      char_info->visual_thickness = visual_thickness;

      if (g_strcmp0 (char_info->character, " ") == 0)
        {
          /*
           * Word spacing; add the natural space width PLUS a multiplier
           * of the visual thickness.
           * This multiplier is the "tuning knob".
           * A value of 2.0 creates significant and clear word separation.
           */
          const gdouble word_spacing_multiplier = 2.0; /* <-- TUNING KNOB */
          char_info->text_width                 =
              logical_rect.width +
              (gint) ceil (visual_thickness * word_spacing_multiplier);
        }
      else
        {
          /*
           * For a normal character, we only need to compensate for its own
           * right-side bleed, so we add half the thickness.
           */
          char_info->text_width =
              logical_rect.width + (gint) ceil (visual_thickness / 2.0);
        }

      char_info->text_height = ink_rect.height;

      cairo_surface_flush (cairo_get_target (cr));
      cairo_restore (cr);
      g_object_unref (layout);

      invalidate_character_area (char_info);
    }
}

/**
 * is_delete_char:
 * @ch: Key value to test.
 *
 * Determines if the key represents a delete/backspace.
 *
 * Returns: TRUE if the key is Backspace or Delete, FALSE otherwise.
 **/
static gboolean
is_delete_char (int ch)
{
  return (ch == GDK_KEY_BackSpace) || (ch == GDK_KEY_Delete);
}

/**
 * is_tab_char:
 * @ch: Key value to test.
 *
 * Determines if the key represents a tab.
 *
 * Returns: TRUE if the key is Tab, FALSE otherwise.
 **/
static gboolean
is_tab_char (int ch)
{
  return ch == GDK_KEY_Tab;
}

/**
 * is_return_char:
 * @ch: Key value to test.
 *
 * Determines if the key represents a return/enter key.
 *
 * Returns: TRUE if the key is Return, ISO_Enter, or KP_Enter, FALSE otherwise.
 **/
static gboolean
is_return_char (int ch)
{
  return (ch == GDK_KEY_Return) ||
         (ch == GDK_KEY_ISO_Enter) ||
         (ch == GDK_KEY_KP_Enter);
}

/**
 * assign_text_properties:
 * @char_info: Character structure to populate.
 *
 * Assigns properties to a CharInfo object based on the current text
 * data and annotation settings (position, font, color, pen width, etc.).
 **/
static void
assign_text_properties (CharInfo *char_info)
{
  char_info->x                = text_data->pos->x;
  char_info->y                = text_data->pos->y;
  char_info->pen_width        = text_data->pen_width;
  char_info->color            = g_strdup (text_data->color);
  char_info->italics          = CAIRO_FONT_SLANT_NORMAL;
  char_info->font_weight      = CAIRO_FONT_WEIGHT_NORMAL;
  char_info->background_color = NULL;

  if (annotation_data->font == NULL)
    {
      char_info->pango_font_description = NULL;
      char_info->font_family            = g_strdup (text_config->fontfamily);
      char_info->font_size              = 32;
    }
  else
    {
      char_info->pango_font_description = annotation_data->font;

      char_info->font_family = g_strdup_printf (
          "%s", pango_font_description_get_family (annotation_data->font));

      char_info->font_size =
          pango_font_description_get_size (annotation_data->font) / PANGO_SCALE;
    }
}

/**
 * delete_character:
 *
 * Deletes the last character in the text, clearing its stroked
 * bounding box and updating cursor position. Frees memory associated
 * with the deleted character.
 **/
static void
delete_character (void)
{
  if (! text_data->cr || ! text_data->letterlist)
    return;

  CharInfo *char_info = (CharInfo *) text_data->letterlist->data;
  if (! char_info)
    return;

  if (g_strcmp0 (char_info->character, "\n") != 0)
    {
      cairo_save (text_data->cr);
      cairo_set_operator (text_data->cr, CAIRO_OPERATOR_CLEAR);

      /* Get the base "ink" rectangle from Pango. */
      PangoRectangle ink_rect = char_info->ink_rect;

      /* Get the visual thickness that was used for drawing. */
      gdouble visual_thickness = char_info->visual_thickness;

      /* Determine the top-left origin of the layout on the canvas. */
      gdouble origin_x = char_info->x;
      gdouble origin_y =
          char_info->y - (gdouble) char_info->baseline / PANGO_SCALE;

      /*
       * Calculate the final clearing area. This is the ink rectangle's
       * position, expanded on all sides by half the stroke thickness,
       * plus a small safety padding for anti-aliasing.
       */
      gdouble padding = 2.0;
      gdouble rect_x =
          origin_x + ink_rect.x - (visual_thickness / 2.0) - padding;
      gdouble rect_y =
          origin_y + ink_rect.y - (visual_thickness / 2.0) - padding;
      gdouble rect_width =
          (gdouble) ink_rect.width + visual_thickness + (padding * 2);
      gdouble rect_height =
          (gdouble) ink_rect.height + visual_thickness + (padding * 2);

      /* Clear only this exact, calculated rectangle. */
      cairo_rectangle (text_data->cr, rect_x, rect_y, rect_width, rect_height);
      cairo_fill (text_data->cr);
      cairo_restore (text_data->cr);

      invalidate_character_area (char_info);
    }

  /* Reset cursor position to where the deleted character was */
  text_data->pos->x = char_info->x;
  text_data->pos->y = char_info->y;

  /* Free the character's data */
  destroy_text_properties (char_info);
  text_data->letterlist = g_slist_remove (text_data->letterlist, char_info);
}

/**
 * handle_delete_char:
 *
 * Helper function to delete the last character.
 **/
static void
handle_delete_char (void)
{
  delete_character (); // undo the last character inserted
}

/**
 * handle_return_char:
 *
 * Handles inserting a newline character and updating cursor position.
 **/
static void
handle_return_char (void)
{
  /* select the x indentation */
  CharInfo *char_info  = make_new_character ();
  char_info->character = g_strdup ("\n");
  assign_text_properties (char_info);
  text_data->letterlist = g_slist_prepend (text_data->letterlist, char_info);

  /* Move down and to underneath where user started this bit of text. */
  text_data->pos->x = text_config->start_x + text_config->leftmargin;
  /* Use the consistent max_font_height for stable line spacing */
  text_data->pos->y += text_data->max_font_height + 5; /* 5px line spacing */
}

/**
 * handle_tab_char:
 *
 * Handles inserting a tab character and moving the cursor by the tab size.
 **/
static void
handle_tab_char (void)
{
  /* Simple Tab-Implementation */
  CharInfo *char_info  = make_new_character ();
  char_info->character = g_strdup ("\t");
  assign_text_properties (char_info);
  text_data->letterlist = g_slist_prepend (text_data->letterlist, char_info);

  // move cursor along by tab size
  text_data->pos->x += text_config->tabsize;
}

/**
 * handle_printable_char:
 * @ch: The printable character to insert.
 *
 * Handles inserting a printable character, drawing it on the canvas,
 * and advancing the cursor appropriately.
 **/
static void
handle_printable_char (char ch)
{
  CharInfo *char_info  = make_new_character ();
  char_info->character = g_strdup_printf ("%c", ch);
  assign_text_properties (char_info);
  text_data->letterlist = g_slist_prepend (text_data->letterlist, char_info);

  draw_character (text_data->cr, char_info);

  /* Move cursor to the x step using the correct advance width */
  text_data->pos->x += char_info->text_width;
}

/**
 * text_input_key_press:
 *
 * Handle key press events in the text annotation window.
 *
 * This function processes user input when typing in the text window. It stops
 * the blinking cursor temporarily, interprets the key event, and performs
 * the corresponding action:
 *   - Deletes a character if a delete key is pressed.
 *   - Inserts a line break if the return key is pressed or the text reaches
 *     the window width.
 *   - Inserts a tab character if the tab key is pressed.
 *   - Inserts printable characters otherwise.
 *
 * After handling the key, the blinking cursor is restarted.
 *
 * Returns TRUE to indicate that the event has been handled.
 **/
gboolean
text_input_key_press (GdkEvent *event)
{
  GdkEventKey *keyEvent = (GdkEventKey *) event;
  if (event->type != GDK_KEY_PRESS)
    {
      return TRUE;
    }

  stop_blink_cursor ();

  GtkWidget *annotation_window = get_annotation_window ();
  int width = gtk_widget_get_allocated_width (GTK_WIDGET (annotation_window));

  if (is_delete_char (keyEvent->keyval))
    {
      handle_delete_char ();
    }
  else if ((text_data->pos->x + 20 >= width) || /* Use a small buffer */
           is_return_char (keyEvent->keyval))
    {
      handle_return_char ();
    }
  else if (is_tab_char (keyEvent->keyval))
    {
      handle_tab_char ();
    }
  else if (isprint (keyEvent->keyval))
    {
      handle_printable_char (keyEvent->keyval);
    }

  start_blink_cursor ();
  return TRUE;
}

#ifdef _WIN32

/**
 * is_above_virtual_keyboard:
 * @x: X coordinate in screen space.
 * @y: Y coordinate in screen space.
 *
 * Determines if the given (x, y) position is currently above the
 * virtual keyboard window (Windows-specific).
 *
 * Returns: TRUE if the point is over the virtual keyboard, FALSE otherwise.
 **/
static gboolean
is_above_virtual_keyboard (gint x, gint y)
{
  RECT rect;
  HWND hwnd = FindWindow (VIRTUALKEYBOARD_WINDOW_NAME, NULL);
  if (! hwnd)
    {
      return FALSE;
    }
  if (! GetWindowRect (hwnd, &rect))
    {
      return FALSE;
    }
  if ((rect.left < x) &&
      (x < rect.right) &&
      (rect.top < y) &&
      (y < rect.bottom))
    {
      return TRUE;
    }
  return FALSE;
}

#endif

/**
 * text_input_button_release:
 *
 * Handle a button release event in the text annotation window.
 *
 * This function updates the text cursor position when the user releases
 * the mouse button (only button 1 is considered). It may save the current
 * text, updates internal cursor coordinates, presents the annotation
 * window and related UI panels, and manages the virtual keyboard and
 * blinking cursor.
 *
 * On Windows, if the release occurs above the virtual keyboard, it
 * re-grabs the pointer and ignores the event for the text input.
 *
 * Returns TRUE to indicate that the event has been handled.
 **/
gboolean
text_input_button_release (GdkEventButton *ev)
{
  g_debug ("text_input_button_release BEGIN\n");
  /* only button1 allowed */
  if (ev->button != 1)
    {
      return TRUE;
    }
#ifdef _WIN32
  gboolean above = is_above_virtual_keyboard (ev->x_root, ev->y_root);

  if (above)
    {
      GtkWidget *annotation_window = annotation_data->annotation_window;
      /* You have lost the focus; re grab it. */
      grab_pointer (annotation_window, TEXT_MOUSE_EVENTS);
      /* Ignore the data; the event will be passed to the virtual keyboard. */
      return TRUE;
    }
#endif

  if ((text_data) && (text_data->pos))
    {
      g_debug ("text_input_button_release: %f %f %f %f\n",
               ev->x,
               ev->y,
               ev->x_root,
               ev->y_root);

      text_data->pos->x    = ev->x; // x_root
      text_data->pos->y    = ev->y; // y_root
      text_config->start_x = ev->x;

      g_debug ("text_input_button_release: text pos: %f %f",
               text_data->pos->x,
               text_data->pos->y);

      stop_virtual_keyboard ();
      start_virtual_keyboard ();

      start_blink_cursor ();
    }
  g_debug ("text_input_button_release END\n");
  return TRUE;
}

/**
 * destroy_text_config:
 * Frees a TextConfig structure.
 *
 * If @cfg is NULL, the function does nothing.
 *
 * @cfg: The TextConfig structure to free.
 */
void
destroy_text_config (TextConfig *cfg)
{
  if (cfg == NULL)
    return;
  g_free (cfg);
}
