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

#include <glib.h>
#include <librsvg/rsvg.h>

#include "cursors.h"
#include "utils.h"

/* The image surface that will contain the pen icon. */
static cairo_surface_t *pen_image_surface = (cairo_surface_t *) NULL;

/* The image surface that will contain the highlighter icon. */
static cairo_surface_t *highlighter_image_surface = (cairo_surface_t *) NULL;

/* The image surface that will contain the eraser icon. */
static cairo_surface_t *eraser_image_surface = (cairo_surface_t *) NULL;

/* The image surface that will contain the arrow icon. */
static cairo_surface_t *arrow_image_surface = (cairo_surface_t *) NULL;

/* The image surface that will contain the filler icon. */
static cairo_surface_t *filler_image_surface = (cairo_surface_t *) NULL;

/**
 * cairo_image_surface_create_from_svg:
 * Creates a Cairo image surface from an SVG file.
 *
 * Loads the SVG document from @file, retrieves its intrinsic size,
 * and renders it into a newly allocated ARGB32 Cairo image surface.
 *
 * The returned surface must be destroyed with cairo_surface_destroy()
 * by the caller.
 *
 * @file: Path to the SVG file.
 *
 * Returns: (transfer full): A newly created cairo_image_surface_t,
 *   or %NULL on failure.
 */
static cairo_surface_t *
cairo_image_surface_create_from_svg (const gchar *file)
{
  cairo_surface_t *surface;
  cairo_t         *cr;
  RsvgHandle      *handle;

  handle                 = rsvg_handle_new_from_file (file, NULL);
  RsvgRectangle viewport = { 0.0, 0.0, 0.0, 0.0 };

  rsvg_handle_get_intrinsic_size_in_pixels (handle,
                                            &viewport.width,
                                            &viewport.height);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        round (viewport.width),
                                        round (viewport.height));

  cr = cairo_create (surface);
  rsvg_handle_render_document (handle, cr, &viewport, NULL);
  cairo_destroy (cr);
  g_object_unref (handle);
  return surface;
}

/**
 * svg_replace_color:
 * Replaces all occurrences of a color string inside SVG data.
 *
 * Creates a modified copy of @svg_data where every occurrence of
 * @old_color is replaced with @new_color.
 *
 * The returned string is newly allocated and must be freed with g_free().
 *
 * @svg_data: The original SVG text buffer.
 * @old_color: The color string to be replaced (e.g. "#FF0000").
 * @new_color: The replacement color string.
 *
 * Returns: (transfer full): A newly allocated string containing the
 *   modified SVG data, or %NULL if any parameter is %NULL.
 */
static
gchar *
svg_replace_color (const gchar *svg_data,
                   const gchar *old_color,
                   const gchar *new_color)
{
  if (! svg_data || ! old_color || ! new_color)
    {
      return NULL;
    }

  GString *gstr = g_string_new (svg_data);
  g_string_replace (gstr, old_color, new_color, 0);

  return g_string_free (gstr, FALSE);
}

/**
 * cairo_image_surface_create_from_svg_repl:
 * @file: Path to the SVG file.
 * @old_color: The color string to be replaced (e.g. "#FF0000").
 * @new_color: The replacement color string.
 *
 * Loads an SVG file, replaces all occurrences of @old_color with @new_color,
 * and renders it into a newly allocated Cairo ARGB32 image surface.
 *
 * The returned surface must be destroyed with cairo_surface_destroy()
 * by the caller.
 *
 * Returns: (transfer full): A newly created #cairo_surface_t, or %NULL on
 * failure.
 */
static cairo_surface_t *
cairo_image_surface_create_from_svg_repl (const gchar *file,
                                          const gchar *old_color,
                                          const gchar *new_color)
{
  cairo_surface_t *surface;
  cairo_t         *cr;
  RsvgHandle      *handle;
  gchar           *svg_text = NULL;
  GError          *error    = NULL;

  /* Read svg file. */
  if (! g_file_get_contents (file, &svg_text, NULL, &error))
    {
      g_printerr ("Cannot read file %s: %s\n", file, error->message);
      g_error_free (error);
      return NULL;
    }

  /* Replace color. */
  gchar *new_svg = svg_replace_color (svg_text, old_color, new_color);
  g_free (svg_text);

  /* Read svg from memory. */
  GInputStream *stream;
  stream = g_memory_input_stream_new_from_data (new_svg,
                                                strlen (new_svg),
                                                NULL);

  handle = rsvg_handle_new_from_stream_sync (stream,
                                             NULL,
                                             RSVG_HANDLE_FLAGS_NONE,
                                             NULL,   // cancellable
                                             &error); // GError**

  g_object_unref (stream);
  g_free (new_svg);

  if (! handle)
    {
      g_printerr ("Cannot parse SVG: %s\n", error->message);
      g_error_free (error);
      return NULL;
    }

  /* Get size. */
  RsvgRectangle viewport = {0.0, 0.0, 0.0, 0.0};
  rsvg_handle_get_intrinsic_size_in_pixels (handle,
                                            &viewport.width,
                                            &viewport.height);

  /* Build cairo surface. */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        round (viewport.width),
                                        round (viewport.height));
  cr = cairo_create (surface);
  rsvg_handle_render_document (handle, cr, &viewport, NULL);
  cairo_destroy (cr);
  g_object_unref (handle);

  return surface;
}

/**
 * get_eraser_image_surface:
 * Returns the cached eraser image surface.
 *
 * If the surface was not created yet, it is created from the SVG
 * defined by %ERASER_ICON and then cached for future calls.
 *
 * The returned surface is owned by this module and must not be
 * destroyed by the caller.
 *
 * Returns: (transfer none): The cached eraser #cairo_surface_t.
 */
static cairo_surface_t *
get_eraser_image_surface (void)
{
  if (eraser_image_surface)
    {
      return eraser_image_surface;
    }

  eraser_image_surface = cairo_image_surface_create_from_svg (ERASER_ICON);
  return eraser_image_surface;
}

/**
 * get_highlighter_image_surface:
 * @old_color: The hex color string to be replaced (e.g. "FF0000FF").
 * @new_color: The replacement hex color string (e.g. "00FF00FF").
 *
 * Gets the highlighter image surface with a modified color.
 *
 * If a previous surface exists, it is destroyed and recreated using the
 * %HIGHLIGHTER_ICON SVG, replacing @old_color with @new_color.
 *
 * The returned surface is cached internally and must not be freed by
 * the caller.
 *
 * Returns: (transfer none): The updated highlighter #cairo_surface_t.
 */
static cairo_surface_t *
get_highlighter_image_surface (const gchar *old_color,
                               const gchar *new_color)
{
  if (highlighter_image_surface)
    {
      cairo_surface_destroy (highlighter_image_surface);
    }

  highlighter_image_surface =
    cairo_image_surface_create_from_svg_repl (HIGHLIGHTER_ICON,
                                              old_color,
                                              new_color);

  return highlighter_image_surface;
}

/**
 * get_arrow_image_surface:
 * @old_color: The hex color string to be replaced (e.g. "FF0000FF").
 * @new_color: The replacement hex color string (e.g. "00FF00FF").
 *
 * Gets the arrow image surface with a modified color.
 *
 * If a previous surface exists, it is destroyed and recreated using the
 * %ARROW_ICON SVG, replacing @old_color with @new_color.
 *
 * The returned surface is cached internally and must not be freed by
 * the caller.
 *
 * Returns: (transfer none): The updated arrow #cairo_surface_t.
 */
static cairo_surface_t *
get_arrow_image_surface (const gchar *old_color, char *new_color)
{
  if (arrow_image_surface)
    {
      cairo_surface_destroy (arrow_image_surface);
    }

  arrow_image_surface = cairo_image_surface_create_from_svg_repl (ARROW_ICON,
                                                                  old_color,
                                                                  new_color);
  return arrow_image_surface;
}

/**
 * get_filler_image_surface:
 * @old_color: The hex color string to be replaced (e.g. "FF0000FF").
 * @new_color: The replacement hex color string (e.g. "00FF00FF").
 *
 * Gets the filler image surface with a modified color.
 *
 * If a previous surface exists, it is destroyed and recreated using the
 * %FILLER_ICON SVG, replacing @old_color with @new_color.
 *
 * The returned surface is cached internally and must not be freed by
 * the caller.
 *
 * Returns: (transfer none): The updated filler #cairo_surface_t.
 */
static cairo_surface_t *
get_filler_image_surface (const gchar *old_color,
                          const gchar *new_color)
{
  if (filler_image_surface)
    {
      cairo_surface_destroy (filler_image_surface);
    }

  filler_image_surface = cairo_image_surface_create_from_svg_repl (FILLER_ICON,
                                                                   old_color,
                                                                   new_color);
  return filler_image_surface;
}

/**
 * get_pen_image_surface:
 * @old_color: The hex color string to be replaced (e.g. "FF0000FF").
 * @new_color: The replacement hex color string (e.g. "00FF00FF").
 *
 * Gets the pen image surface with a modified color.
 *
 * If a previous pen surface already exists, it is destroyed and recreated
 * from the %PENCIL_ICON SVG, applying a color replacement from @old_color
 * to @new_color.
 *
 * The returned surface is cached internally and must not be freed by
 * the caller.
 *
 * Returns: (transfer none): The updated pen #cairo_surface_t.
 */
static cairo_surface_t *
get_pen_image_surface (const gchar *old_color,
                       const gchar *new_color)
{
  if (pen_image_surface)
    {
      cairo_surface_destroy (pen_image_surface);
    }

  pen_image_surface = cairo_image_surface_create_from_svg_repl (PENCIL_ICON,
                                                                old_color,
                                                                new_color);

  return pen_image_surface;
}

/**
 * destroy_eraser_image_surface:
 *
 * Destroys the cached eraser image surface, if it exists.
 */
static void
destroy_eraser_image_surface (void)
{
  if (eraser_image_surface)
    {
      cairo_surface_destroy (eraser_image_surface);
    }
}

/**
 * destroy_highlighter_image_surface:
 *
 * Destroys the cached highlighter image surface, if it exists.
 */
static void
destroy_highlighter_image_surface (void)
{
  if (highlighter_image_surface)
    {
      cairo_surface_destroy (highlighter_image_surface);
    }
}

/**
 * destroy_pen_image_surface:
 *
 * Destroys the cached pen image surface, if it exists.
 */

static void
destroy_pen_image_surface (void)
{
  if (pen_image_surface)
    {
      cairo_surface_destroy (pen_image_surface);
    }
}

/**
 * destroy_filler_image_surface:
 *
 * Destroys the cached filler image surface, if it exists.
 */
static void
destroy_filler_image_surface (void)
{
  if (filler_image_surface)
    {
      cairo_surface_destroy (filler_image_surface);
    }
}

/**
 * gdk_pixbuf_swap_blue_with_red:
 * @pixbuf: (inout): a pointer to a #GdkPixbuf whose pixels will be modified.
 *
 * Swaps the blue and red channels of all pixels in @pixbuf.
 *
 * The pixbuf is modified in place.
 */
static void
gdk_pixbuf_swap_blue_with_red (GdkPixbuf **pixbuf)
{
  gint n_channels = gdk_pixbuf_get_n_channels (*pixbuf);

  gint    pixbuf_width  = gdk_pixbuf_get_width (*pixbuf);
  gint    pixbuf_height = gdk_pixbuf_get_height (*pixbuf);
  gint    rowstride     = gdk_pixbuf_get_rowstride (*pixbuf);
  guchar *pixels        = gdk_pixbuf_get_pixels (*pixbuf);

  gint x = 0;

  for (x = 0; x < pixbuf_height; x++)
    {
      gint y = 0;

      for (y = 0; y < pixbuf_width; y++)
        {
          guchar *p  = pixels + y * rowstride + x * n_channels;
          /* swap the pixel red value with the blue */
          guchar  p0 = p[0];
          p[0]       = p[2];
          p[2]       = p0;
        }
    }
}

/**
 * get_eraser_pixbuf:
 * @thickness: Thickness of the eraser circle.
 * @pixbuf: (out): Location to store the newly created pixbuf.
 * @circle_width: Width of the circle outline.
 *
 * Generates a #GdkPixbuf representing the eraser cursor, including
 * a circle with the given thickness and the eraser icon. The pixbuf
 * is modified in-place to swap red and blue channels as needed.
 */
static void
get_eraser_pixbuf (gdouble thickness,
                   GdkPixbuf **pixbuf,
                   gdouble circle_width)
{
  cairo_t         *eraser_cr     = (cairo_t *) NULL;
  cairo_surface_t *image_surface = get_eraser_image_surface ();
  gint             icon_width, icon_height;
  gint             cursor_width, cursor_height;

  icon_width  = cairo_image_surface_get_width (image_surface);
  icon_height = cairo_image_surface_get_height (image_surface);

  cursor_width  = (gint) icon_width + thickness + circle_width;
  cursor_height = (gint) icon_height + thickness + circle_width;

  cairo_surface_t *surface = (cairo_surface_t *) NULL;

  *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                            TRUE,
                            8,
                            cursor_width,
                            cursor_height);

  guchar *pixels = gdk_pixbuf_get_pixels (*pixbuf);

  surface =
      cairo_image_surface_create_for_data (pixels,
                                           CAIRO_FORMAT_RGB24,
                                           gdk_pixbuf_get_width (*pixbuf),
                                           gdk_pixbuf_get_height (*pixbuf),
                                           gdk_pixbuf_get_rowstride (*pixbuf));

  eraser_cr = cairo_create (surface);

  clear_cairo_context (eraser_cr);

  cairo_set_line_width (eraser_cr, circle_width);

  /* Add a circle with the desired width. */
  cairo_set_source_rgba (eraser_cr, 0, 0, 1, 1);

  cairo_arc (eraser_cr, thickness / 2 + circle_width,
             cursor_height - thickness / 2 - circle_width,
             thickness / 2, 0, 2 * M_PI);

  cairo_stroke (eraser_cr);

  cairo_set_source_surface (eraser_cr, image_surface, thickness * 3 / 4, 0);
  cairo_paint (eraser_cr);
  cairo_stroke (eraser_cr);

  cairo_surface_destroy (surface);
  cairo_destroy (eraser_cr);

  /* The pixbuf created by cairo has the r and b color inverted. */
  gdk_pixbuf_swap_blue_with_red (pixbuf);
}

/**
 * get_filler_pixbuf:
 * @pixbuf: (out): Location to store the newly created pixbuf.
 * @color: Hex string representing the desired color.
 *
 * Generates a #GdkPixbuf representing the filler (paint bucket) cursor
 * with the specified color. The pixbuf is modified in-place to swap
 * red and blue channels as needed.
 */
static void
get_filler_pixbuf (GdkPixbuf **pixbuf,
                   gchar *color)
{
  cairo_surface_t *image_surface = (cairo_surface_t *) NULL;
  cairo_surface_t *surface       = (cairo_surface_t *) NULL;
  cairo_t         *filler_cr     = (cairo_t *) NULL;

  gint image_width;
  gint image_height;

  gchar *rgb    = g_strndup (color, 6);
  image_surface = get_filler_image_surface ("ff0000", rgb);
  g_free (rgb);

  image_width  = cairo_image_surface_get_width (image_surface);
  image_height = cairo_image_surface_get_height (image_surface);

  *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                            TRUE,
                            8,
                            image_width,
                            image_height);

  guchar *pixels = gdk_pixbuf_get_pixels (*pixbuf);

  surface =
      cairo_image_surface_create_for_data (pixels,
                                           CAIRO_FORMAT_RGB24,
                                           gdk_pixbuf_get_width (*pixbuf),
                                           gdk_pixbuf_get_height (*pixbuf),
                                           gdk_pixbuf_get_rowstride (*pixbuf));

  filler_cr = cairo_create (surface);

  clear_cairo_context (filler_cr);

  cairo_set_operator (filler_cr, CAIRO_OPERATOR_SOURCE);

  cairo_set_source_surface (filler_cr, image_surface, 0, 0);

  cairo_paint (filler_cr);
  cairo_stroke (filler_cr);

  cairo_surface_destroy (surface);
  cairo_destroy (filler_cr);

  /* The pixbuf created by cairo has the r and b color inverted. */
  gdk_pixbuf_swap_blue_with_red (pixbuf);
}

/**
 * get_pen_pixbuf:
 * @pixbuf: (out): Location to store the newly created pixbuf.
 * @color: Hex string representing the desired color.
 * @thickness: Thickness of the pen circle.
 * @arrow: %TRUE if the pen should have an arrow shape.
 * @circle_width: Width of the circle outline.
 *
 * Generates a #GdkPixbuf representing the pen cursor, which may include
 * a circle of the given thickness and either a pencil, highlighter, or
 * arrow icon depending on @color and @arrow. The pixbuf is modified
 * in-place to swap red and blue channels as needed.
 */
static void
get_pen_pixbuf (GdkPixbuf **pixbuf,
                gchar *color,
                gdouble thickness,
                gdouble arrow,
                gdouble circle_width)
{
  cairo_t         *pen_cr        = (cairo_t *) NULL;
  cairo_surface_t *surface       = (cairo_surface_t *) NULL;
  cairo_surface_t *image_surface = (cairo_surface_t *) NULL;
  gint             icon_width;
  gint             icon_height;
  gint             cursor_width;
  gint             cursor_height;

  gchar *rgb = g_strndup (color, 6);
  if (arrow)
    { /* load the arrow icon. */
      image_surface = get_arrow_image_surface ("ff0000", rgb);
    }
  else
    {
      gchar *alpha = NULL;
      /* Take the opacity. */
      if (strlen (color) == 8)
        {
          alpha = g_substr (color, 6, 8);
        }
      else
        {
          alpha = g_strdup ("FF");
        }

      if (g_strcmp0 (alpha, "FF") == 0)
        {
          /* load the pencil icon. */
          image_surface = get_pen_image_surface ("ffff00", rgb);
        }
      else
        {
          /* load the highlighter icon. */
          image_surface = get_highlighter_image_surface ("ffff00", rgb);
        }
      if (alpha != NULL)
        {
          g_free (alpha);
        }
    }
  g_free (rgb);

  icon_width  = cairo_image_surface_get_width (image_surface);
  icon_height = cairo_image_surface_get_height (image_surface);

  cursor_width  = (gint) icon_width + thickness / 2 + circle_width;
  cursor_height = (gint) icon_height + thickness / 2 + circle_width;
  *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                            TRUE, 8,
                            cursor_width,
                            cursor_height);

  guchar *pixels = gdk_pixbuf_get_pixels (*pixbuf);
  surface =
      cairo_image_surface_create_for_data (pixels,
                                           CAIRO_FORMAT_RGB24,
                                           gdk_pixbuf_get_width (*pixbuf),
                                           gdk_pixbuf_get_height (*pixbuf),
                                           gdk_pixbuf_get_rowstride (*pixbuf));

  pen_cr = cairo_create (surface);

  clear_cairo_context (pen_cr);

  cairo_set_line_width (pen_cr, circle_width);

  /* Add a circle that respect the width and the selected color. */
  cairo_set_source_color_from_string (pen_cr, color);

  cairo_arc (pen_cr, thickness / 2 + circle_width,
             cursor_height - thickness / 2 - circle_width,
             thickness / 2, 0, 2 * M_PI);

  cairo_stroke (pen_cr);

  cairo_set_source_surface (pen_cr, image_surface, thickness / 2, 0);
  cairo_paint (pen_cr);
  cairo_stroke (pen_cr);

  cairo_surface_destroy (surface);
  cairo_destroy (pen_cr);

  /* The pixbuf created by cairo has the r and b color inverted. */
  gdk_pixbuf_swap_blue_with_red (pixbuf);
}

/**
 * destroy_cached_image_surfaces:
 *
 * Destroys all cached image surfaces used for cursors (eraser, pen,
 * highlighter, filler) to release allocated memory.
 */
static void
destroy_cached_image_surfaces (void)
{
  destroy_eraser_image_surface ();
  destroy_pen_image_surface ();
  destroy_highlighter_image_surface ();
  destroy_filler_image_surface ();
}

/**
 * cursors_main:
 *
 * Initializes the cursors module.
 *
 * This function is intended to be the main entry point for setting up
 * cursor-related variables and resources. It currently serves as a
 * placeholder, as specific cursor data is loaded on demand at runtime.
 **/
void
cursors_main (void)
{
  // The data will be loaded on runtime.
}

/**
 * set_pen_cursor:
 * @cursor: (out): A location to store the newly created pen cursor.
 * @thickness: The thickness of the pen, used for the cursor's size.
 * @color: A string representing the color of the cursor.
 * @arrow: %TRUE if the cursor should have an arrow shape, %FALSE otherwise.
 *
 * Creates a new custom cursor that visually represents a pen with a given
 * thickness, color, and optional arrow shape.
 *
 * This function internally generates a #GdkPixbuf for the cursor's
 * appearance, calculates the hotspot to be at the pen's tip, and creates
 * the final #GdkCursor. The object returned in @cursor is newly allocated
 * and the caller is responsible for freeing it with g_object_unref().
 **/
void
set_pen_cursor (GdkCursor **cursor,
                gdouble thickness,
                gchar *color,
                gboolean arrow)
{
  GdkPixbuf *pixbuf       = (GdkPixbuf *) NULL;
  gdouble    circle_width = 2.0;

  get_pen_pixbuf (&pixbuf, color, thickness, arrow, circle_width);

  gdouble hotspot_x = thickness / 2 + circle_width;

  gdouble hotspot_y =
      gdk_pixbuf_get_height (pixbuf) - thickness / 2 - circle_width;

  *cursor = gdk_cursor_new_from_pixbuf (gdk_display_get_default (),
                                        pixbuf,
                                        hotspot_x,
                                        hotspot_y);

  g_object_unref (pixbuf);
}

/**
 * set_eraser_cursor:
 * @cursor: (out): A location to store the newly created eraser cursor.
 * @size:   The size of the eraser, used to determine the cursor's
 * dimensions and hotspot.
 *
 * Creates a new custom cursor that visually represents an eraser of a
 * given size.
 *
 * This function generates a #GdkPixbuf for the cursor's appearance,
 * calculates the hotspot to be at the eraser's center, and creates
 * the final #GdkCursor. The object returned in @cursor is newly
 * allocated and must be freed by the caller using g_object_unref().
 **/
void
set_eraser_cursor (GdkCursor **cursor, gint size)
{
  GdkPixbuf *pixbuf       = (GdkPixbuf *) NULL;
  gdouble    circle_width = 2.0;

  get_eraser_pixbuf (size, &pixbuf, circle_width);

  gdouble hotspot_x = size / 2 + circle_width;
  gdouble hotspot_y = gdk_pixbuf_get_height (pixbuf) - size / 2 - circle_width;

  *cursor = gdk_cursor_new_from_pixbuf (gdk_display_get_default (),
                                        pixbuf,
                                        hotspot_x,
                                        hotspot_y);

  g_object_unref (pixbuf);
}

/**
 * set_filler_cursor:
 * @cursor: (out): A location to store the newly created filler cursor.
 * @color:  A string representing the color of the filler tool.
 *
 * Creates a new custom cursor that visually represents the filler
 * (paint bucket) tool with a given color.
 *
 * This function generates a #GdkPixbuf for the cursor's appearance and
 * sets the hotspot to the bottom-right corner of the image. The cursor
 * returned in the @cursor out-parameter is newly allocated and must be
 * freed by the caller using g_object_unref().
 **/
void
set_filler_cursor (GdkCursor **cursor, gchar *color)
{
  GdkPixbuf *pixbuf = (GdkPixbuf *) NULL;
  get_filler_pixbuf (&pixbuf, color);

  *cursor = gdk_cursor_new_from_pixbuf (gdk_display_get_default (),
                                        pixbuf,
                                        0,
                                        gdk_pixbuf_get_height (pixbuf) - 1);
  g_object_unref (pixbuf);
}

/**
 * cursors_main_quit:
 *
 * Finalizes and cleans up the cursors module.
 *
 * This function should be called during application shutdown to release
 * resources allocated by the module, such as cached image surfaces used
 * for custom cursors.
 **/
void
cursors_main_quit (void)
{
  destroy_cached_image_surfaces ();
}
