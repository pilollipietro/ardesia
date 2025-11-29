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

#include "text_window_callbacks.h"
#include "annotation_window.h"
#include "bar.h"
#include "keyboard.h"
#include "text_window.h"
#include "utils.h"
#include <ctype.h>
#include <math.h>

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
 * on_text_window_expose_event:
 * @widget: The GTK widget (text window) being exposed.
 * @cr: Cairo drawing context for rendering.
 * @data: User data passed to the callback (unused).
 *
 * Called when the text window is exposed (needs redraw). This is typically
 * triggered when the window is first shown or uncovered. Double buffering
 * must be enabled for proper rendering.
 *
 * Returns: FALSE to propagate the event further, allowing default GTK
 *          drawing behavior.
 **/
G_MODULE_EXPORT gboolean
on_text_window_expose_event (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  return FALSE;
}

/**
 * on_text_window_button_release:
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
on_text_window_button_release (GtkWidget *win,
                               GdkEventButton *ev,
                               TextData *data)
{
  g_debug ("on_text_window_button_release BEGIN\n");
  /* only button1 allowed */
  if (ev->button != 1)
    {
      return TRUE;
    }

#ifdef _WIN32
  gboolean above = is_above_virtual_keyboard (ev->x_root, ev->y_root);

  if (above)
    {
      /* You have lost the focus; re grab it. */
      grab_pointer (text_data->window, TEXT_MOUSE_EVENTS);
      /* Ignore the data; the event will be passed to the virtual keyboard. */
      return TRUE;
    }
#endif

  if ((text_data) && (text_data->pos))
    {
      g_debug ("on_text_window_button_release: %f %f %f %f\n",
               ev->x,
               ev->y,
               ev->x_root,
               ev->y_root);

      save_text (); // @TODO is this required?

      text_data->pos->x    = ev->x; // x_root
      text_data->pos->y    = ev->y; // y_root
      text_config->start_x = ev->x;

      g_debug ("on_text_window_button_release: text pos: %f %f",
               text_data->pos->x,
               text_data->pos->y);

      stop_virtual_keyboard ();
      start_virtual_keyboard ();

      start_blink_cursor ();
    }
  g_debug ("on_text_window_button_release END\n");
  return TRUE;
}

/**
 * on_text_window_cursor_motion:
 *
 * Handles cursor motion events in the text window.
 *
 * This function stops text input when the cursor moves over the bar window
 * (Windows-specific behavior). It is called on every motion event.
 *
 * Returns: TRUE to indicate that the event has been handled.
 **/
G_MODULE_EXPORT gboolean
on_text_window_cursor_motion (GtkWidget *win,
                              GdkEventMotion *ev,
                              gpointer func_data)
{
#ifdef _WIN32
  if (inside_bar_window (ev->x_root, ev->y_root))
    {
      stop_text_widget ();
    }
#endif
  return TRUE;
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

  gtk_widget_queue_draw_area(annotation_window,
                              dirty_x, dirty_y,
                              dirty_width, dirty_height);
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
      gdouble padding = 1.0;
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
 * on_text_window_key_press_event:
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
G_MODULE_EXPORT gboolean
on_text_window_key_press_event (GtkWidget *widget, GdkEvent *event,
                                gpointer user_data)
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
