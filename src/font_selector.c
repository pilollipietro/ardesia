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

#include "font_selector.h"
#include "annotation_window.h"
#include "font_selector_callbacks.h"

/**
 * @brief Creates and shows the font settings dialog.
 * @param parent_window The window to set as the dialog's parent.
 */
void
create_font_selector_window (GtkWindow *parent)
{
  if (annotation_data->font_window == NULL)
    {
      GtkBuilder *builder;
      GtkWidget  *font_dialog;

      gchar *file = FONT_UI_FILE;
      builder     = gtk_builder_new_from_file (file);
      GObject *obj;

      obj         = gtk_builder_get_object (builder, "font_dialog");
      font_dialog = GTK_WIDGET (obj);

      gtk_window_set_transient_for (GTK_WINDOW (font_dialog), parent);
      gtk_window_set_modal (GTK_WINDOW (font_dialog), TRUE);

      if (annotation_data->font != NULL)
        {
          gtk_font_chooser_set_font_desc (GTK_FONT_CHOOSER (font_dialog),
                                          annotation_data->font);
        }

    g_signal_connect (font_dialog,
		      "response",
		      G_CALLBACK (on_font_selector_response),
		      NULL);

    g_signal_connect (font_dialog,
		      "destroy",
		      G_CALLBACK (on_font_selector_destroy),
		      NULL);

      annotation_data->font_window = font_dialog;
    }
}

void
show_font_selector_window ()
{
  gtk_widget_show_all (annotation_data->font_window);
}
