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

BackgroundData *
create_background_data ()
{
  g_debug ("Creating background data object\n");
  BackgroundData *background_data = g_malloc ((gsize) sizeof (BackgroundData));
  background_data->color          = (gchar *) NULL;
  background_data->image          = (gchar *) NULL;
  background_data->cr             = (cairo_t *) NULL;
  background_data->type           = 0;
  return background_data;
}

/* Destroy the background data structure */
void
destroy_background_data ()
{
  if (background_data)
    {

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

/* Clear the background. */
void
clear_background_context ()
{
  g_debug ("clear background window, destroying cairo context\n");
  background_data->type = 0;

  clear_cairo_context (background_data->cr);
  annotation_data->is_background_visible = FALSE;
  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/* Update the background image. */
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

/* Update the background color. */
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

/* Restore background. */
void
restore_background ()
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
