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

#include "annotation_window.h"
#include "font_config.h"
#include "font_selector.h"

/**
 * on_font_selector_response:
 * @dialog: The #GtkFontChooserDialog that emitted the signal.
 * @response_id: The response ID from the dialog (e.g., %GTK_RESPONSE_OK).
 * @user_data: (unused): User data.
 *
 * Handles the "response" signal from the font chooser dialog.
 *
 * If the user accepts the dialog (%GTK_RESPONSE_OK), this function
 * retrieves the selected font description, updates the global
 * application font, and saves the new choice to the configuration
 * file. The dialog widget is destroyed upon exit, regardless of the
 * response.
 **/
void
on_font_selector_response (GtkDialog *dialog,
                           gint response_id,
                           gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      PangoFontDescription *new_font_desc;
      GtkFontChooser       *chooser;
      chooser       = GTK_FONT_CHOOSER (dialog);
      new_font_desc = gtk_font_chooser_get_font_desc (chooser);

      if (annotation_data->font != NULL)
        {
          pango_font_description_free (annotation_data->font);
        }
      annotation_data->font = new_font_desc;
      /* Save to config file */
      font_config_save (annotation_data->font);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/**
 * on_font_selector_destroy:
 * @window: The #GtkWidget (the font window) that is being destroyed.
 * @user_data: (unused): User data.
 *
 * A callback connected to the "destroy" signal of the font chooser window.
 *
 * Its purpose is to set the global pointer to the font window
 * (`annotation_data->font_window`) to %NULL. This prevents dangling
 * pointers and allows other parts of the code to know that the window is
 * no longer available.
 **/
void
on_font_selector_destroy (GtkWidget *window, gpointer user_data)
{
  annotation_data->font_window = NULL;
}
