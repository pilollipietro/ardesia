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
#include <config.h>
#endif

#include "annotation_window.h"
#include "background_config.h"
#include "background_window.h"
#include "cairo_functions.h"
#include "utils.h"

BackgroundData *background_data;

/**
 * destroy_background_data_preview:
 *
 * Frees the resources used by the background preview.
 *
 * This function safely destroys the Cairo context (`preview_cr`)
 * associated with the temporary background preview, if it exists,
 * and sets the pointer to %NULL. It should be called to clean up
 * the preview state.
 */
void
destroy_background_data_preview (void)
{
    if (background_data->preview_cr)
    {
        cairo_destroy (background_data->preview_cr);
        background_data->preview_cr = NULL;
    }
}

/**
 * create_preview_background:
 *
 * Creates a new Cairo context for rendering a background preview.
 *
 * This function initializes a temporary drawing surface whose size matches
 * the main annotation window. It first destroys any existing preview
 * context, then creates a new one (`preview_cr`) ready for drawing.
 * This allows a background to be rendered as a preview without
 * modifying the main canvas directly.
 */
void
create_preview_background (void)
{
    destroy_background_data_preview ();
    cairo_surface_t *preview_surface = NULL;
    GtkAllocation allocation;
    gtk_widget_get_allocation (annotation_data->annotation_window, &allocation);

    preview_surface = cairo_image_surface_create (
        CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);

    background_data->preview_cr = cairo_create (preview_surface);
    cairo_surface_destroy (preview_surface);
}

/**
 * create_background_data:
 *
 * Allocates and initializes a new #BackgroundData structure.
 *
 * All fields within the structure are set to their default zero or
 * %NULL values.
 *
 * Returns: (transfer full): A pointer to the newly allocated #BackgroundData
 * struct. The caller is responsible for freeing this memory with
 * g_free().
 **/
BackgroundData *
create_background_data (void)
{
  g_debug ("Creating background data object\n");
  BackgroundData *background_data = g_malloc ((gsize) sizeof (BackgroundData));
  background_data->color          = (gchar *) NULL;
  background_data->image          = (gchar *) NULL;
  background_data->cr             = (cairo_t *) NULL;
  background_data->preview_cr             = (cairo_t *) NULL;
  background_data->type           = 0;
  return background_data;
}

/**
 * destroy_background_data:
 *
 * Frees all memory associated with the global #BackgroundData object.
 *
 * This function safely checks for %NULL pointers before freeing them. It
 * destroys the Cairo context, frees the color string, and then frees the
 * #BackgroundData structure itself. Finally, it sets the global
 * `background_data` pointer to %NULL to prevent dangling pointer issues.
 **/
void
destroy_background_data (void)
{
  if (background_data)
    {
      destroy_background_data_preview ();
      if (background_data->cr)
        {
          cairo_destroy (background_data->cr);
          background_data->cr = (cairo_t *) NULL;
        }     
      if (background_data->color)
        {
          g_free (background_data->color);
          background_data->color = (gchar *) NULL;
        }
      g_free (background_data);
      background_data = (BackgroundData *) NULL;
    }
}

/**
 * clear_background_context:
 *
 * Clears the visual content of the background, effectively making it
 * transparent.
 *
 * This function resets the background type, clears the associated Cairo
 * context, and updates the application state to hide the background layer.
 * It then queues a redraw of the main annotation window to apply the
 * changes visually.
 **/
void
clear_background_context (void)
{
  g_debug ("clear background window, destroying cairo context\n");
  background_data->type = 0;

  clear_cairo_context (background_data->cr);
  annotation_data->is_background_visible = FALSE;
  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/**
 * update_background_image:
 * @name: The filename of the image to set as the background.
 *
 * Sets a new background image.
 *
 * This function updates the background state to 'image' mode, stores the
 * provided @name pointer, and calls a helper to render the image onto the
 * background's Cairo context. It then queues a redraw of the main
 * window. Note: This function stores the @name pointer directly; the caller
 * must ensure the string remains valid for the lifetime of the object.
 **/
void
update_background_image (gchar *name)
{
  g_debug ("Updating background image: %s\n", name);
  background_data->type  = 2;
  background_data->image = name;
  load_file_onto_context (background_data->image, background_data->cr);
  annotation_data->is_background_visible = TRUE;
  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/**
 * update_background_color:
 * @rgba: An RGBA color string (e.g., "FF0000FF").
 *
 * Sets a new solid color as the background.
 *
 * This function updates the background state to 'color' mode and renders
 * the color onto the background's Cairo context. It allocates and stores
 * a new copy of the @rgba string, then queues a redraw of the main window.
 **/
void
update_background_color (gchar *rgba)
{
  g_debug ("Updating background color\n");
  background_data->type  = 1;
  background_data->color = g_strdup_printf ("%s", rgba);
  load_color_onto_context (background_data->color, background_data->cr);
  annotation_data->is_background_visible = TRUE;
  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/**
 * restore_background:
 *
 * Restores the last used background from the user's configuration.
 *
 * This function retrieves the last saved background setting (either a color
 * or an image) and calls the appropriate update function to apply it to
 * the screen.
 **/
void
restore_background (void)
{
  BackgroundRestored *br = background_config_restore_last_background ();
  if (br)
    {
      switch (br->type)
        {
        case BACKGROUND_RESTORED_COLOR:
          update_background_color (br->value);
          break;
        case BACKGROUND_RESTORED_IMAGE:
          update_background_image (br->value);
          break;
        default:
          break;
        }
      background_restored_free (br);
    }
}
