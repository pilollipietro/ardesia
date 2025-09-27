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
#include "background_config.h"
#include "background_window.h"
#include "bar_callbacks.h"
#include "preference_dialog.h"
#include "user_config.h"
#include "utils.h"

/* Update the preview image. */
G_MODULE_EXPORT void
on_image_chooser_button_update_preview (GtkFileChooser *file_chooser,
		                        gpointer data)
{
  PreferenceData *preference_data = (PreferenceData *) data;
  gchar          *filename;
  GdkPixbuf      *pixbuf;
  gboolean        have_preview;

  filename = gtk_file_chooser_get_preview_filename (file_chooser);

  if (filename)
    {
      pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
      have_preview = (pixbuf != NULL);
      g_free (filename);
      filename = NULL;

      gtk_image_set_from_pixbuf (GTK_IMAGE (preference_data->preview), pixbuf);

      if (pixbuf)
        {
          g_object_unref (pixbuf);
          pixbuf = NULL;
        }

      gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
    }
}

/* Shot when the selected folder change in the file browser. */
G_MODULE_EXPORT void
on_image_chooser_button_file_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData *) data;

  GtkBuilder *preference_dialog_gtk_builder =
    preference_data->preference_dialog_gtk_builder;

  GObject *file_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
		                              "file");

  GtkToggleButton *image_tool_button = GTK_TOGGLE_BUTTON (file_obj);
  gtk_toggle_button_set_active (image_tool_button, TRUE);
}

/* Shot when is pushed the background color button. */
G_MODULE_EXPORT void
on_background_color_button_color_set (GtkButton *buton, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData *) data;
  
  GtkBuilder *preference_dialog_gtk_builder =
    preference_data->preference_dialog_gtk_builder;

  GObject *color_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
		                               "color");

  GtkToggleButton *color_tool_button = GTK_TOGGLE_BUTTON (color_obj);
  gtk_toggle_button_set_active (color_tool_button, TRUE);
}

/* Shot when the ok button in preference dialog is pushed. */
G_MODULE_EXPORT void
on_preference_ok_button_clicked (GtkButton *buton, gpointer data)
{
  PreferenceData *preference_data = (PreferenceData *) data;
  gchar          *rgba            = NULL;

  GtkBuilder *preference_dialog_gtk_builder =
    preference_data->preference_dialog_gtk_builder;
    
  /* Ensure that the user config file exists before we modify it */
  user_config_ensure_file ();

  GObject *color_tool_obj;
  color_tool_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                           "color");

  GtkToggleButton *color_tool_button = GTK_TOGGLE_BUTTON (color_tool_obj);

  if (gtk_toggle_button_get_active (color_tool_button))
    {
      /* background color */
      GObject *bg_color_obj;
      bg_color_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                             "backgroundColorButton");

      GdkRGBA *gdkcolor = g_malloc ((gsize) sizeof (GdkRGBA));
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (bg_color_obj), gdkcolor);
      rgba = gdkrgba_to_rgba (gdkcolor);

      add_background_button (rgba, BACKGROUND_MODE_COLOR, NULL, rgba);

      /* Add chosen color to user configuration */
      background_config_add_color (rgba, rgba);

      g_free (gdkcolor);
    }
  else
    {
      GObject *file_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                                  "file");
      GtkToggleButton *image_tool_button = GTK_TOGGLE_BUTTON (file_obj);
      if (gtk_toggle_button_get_active (image_tool_button))
        {
          /* background png from file */
          GObject *image_obj = NULL;
          image_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                              "imageChooserButton");
          GtkFileChooserButton *image_chooser_button = NULL;
          image_chooser_button = GTK_FILE_CHOOSER_BUTTON (image_obj);
          gchar *filename      = NULL;

          GtkFileChooser *chooser;
          chooser = GTK_FILE_CHOOSER (image_chooser_button);

          filename = gtk_file_chooser_get_filename (chooser);
          if (filename)
            {
              FILE *stream = g_fopen (filename, "r");
              if (stream == NULL)
                {
                  GtkWindow *preference_window = (GtkWindow *) NULL;

                  GObject   *preference_obj    =
	            gtk_builder_get_object (preference_dialog_gtk_builder,
                                            "preferences");

                  preference_window = GTK_WINDOW (preference_obj);
                  show_permission_denied_dialog (preference_window);
                }
              else
                {
                  /*
                   * Cut out filename (without extension)
                   * from absolute file path.
                   */
                  int start = g_substrlastpos (filename, G_DIR_SEPARATOR_S) + 1;
                  if (start < 0)
                    {
                      start = 0;
                    }
                  int end = g_substrlastpos (filename, ".");
                  if (end < start)
                    {
                      end = strlen (filename);
                    }
                  gchar *name = g_substr (filename, start, end);
                  add_background_button (name,
				         BACKGROUND_MODE_FILE,
					 filename,
					 NULL);
                  /* Save chosen image to user configuration */
                  background_config_add_image (name, filename);
                  fclose (stream);
                }
            }
        }
    }
}

/* Shot when is pushed the cancel button. */
G_MODULE_EXPORT void
on_preference_cancel_button_clicked (GtkButton *buton, gpointer data)
{
  /* do nothing */
}
