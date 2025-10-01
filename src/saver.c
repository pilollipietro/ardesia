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

#include "keyboard.h"
#include "saver.h"
#include "utils.h"

/**
 * show_override_dialog:
 * @parent: (nullable): The parent GtkWindow for this dialog.
 *
 * Displays a modal confirmation dialog asking the user if they want to
 * overwrite an existing file.
 *
 * The dialog blocks until the user makes a choice.
 *
 * Returns: %TRUE if the user clicks "Yes", %FALSE otherwise (if they
 * click "No" or close the dialog).
 **/
gboolean
show_override_dialog (GtkWindow *parent)
{
  GtkWidget *dialog;
  gint       result;

  dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_YES_NO,
                                   "File exists. Overwrite?");

  /* Use g_gettext for internationalization if available */
  /* gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                               "A file with the same name
                                               already exists."); */

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return (result == GTK_RESPONSE_YES);
}

/**
 * show_could_not_write_dialog:
 * @parent_window: (nullable): The parent GtkWindow for this dialog.
 *
 * Displays a modal error dialog with a hardcoded "Permission denied"
 * message. The dialog has a single "OK" button and blocks until the
 * user closes it.
 **/
void
show_could_not_write_dialog (GtkWindow *parent_window)
{
  GtkWidget   *permission_denied_dialog = (GtkWidget *) NULL;
  const gchar *message_text = gettext ("Couldn't open file for writing: "
                                       "Permission denied");

  permission_denied_dialog = gtk_message_dialog_new (parent_window,
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     "%s",
                                                     message_text);

  gtk_window_set_modal (GTK_WINDOW (permission_denied_dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (permission_denied_dialog));
  if (permission_denied_dialog)
    {
      gtk_widget_destroy (permission_denied_dialog);
      permission_denied_dialog = NULL;
    }
}

/**
 * start_save_image_dialog_callback:
 * @buffer: (transfer full): The GdkPixbuf containing the screenshot data.
 * This function takes ownership of the buffer and will unref it upon exit.
 *
 * This function is a callback executed after a screenshot is taken. It opens
 * a "Save As" file chooser dialog to save the image as a PNG file.
 *
 * It creates a scaled-down 128x128 thumbnail of the screenshot for the
 * preview widget to ensure the dialog has a reasonable size. If the chosen
 * file already exists, it calls the custom `show_override_dialog` to ask
 * for overwrite confirmation. Finally, if all conditions are met, it saves
 * the buffer as a PNG file.
 **/
void
start_save_image_dialog_callback (GdkPixbuf *buffer)
{
  GtkWindow *parent = GTK_WINDOW (get_bar_widget ());
  GtkWidget *chooser;
  gchar *filename = NULL;
  gboolean do_save = FALSE;

  chooser = gtk_file_chooser_dialog_new ("Save Screenshot as PNG",
                                         parent,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Save",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

  gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);

  /*
   * Create a scaled-down thumbnail for the preview to prevent the
   * dialog from resizing to the full screenshot dimensions. This is
   * the correct logic from the original code.
   */
  GdkPixbuf *preview_pixbuf = gdk_pixbuf_scale_simple (buffer,
                                                       128 /* width */,
                                                       128 /* height */,
                                                       GDK_INTERP_BILINEAR);

  GtkWidget *preview = gtk_image_new_from_pixbuf (preview_pixbuf);
  g_object_unref (preview_pixbuf); /* The image widget now holds its own ref */
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (chooser), preview);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                       get_project_dir ());
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
                                     "screenshot.png");

  start_virtual_keyboard ();

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      do_save  = TRUE;

      /* Ensure the filename has a .png suffix */
      if (! g_str_has_suffix (filename, ".png"))
        {
          gchar *new_filename = g_strdup_printf ("%s.png", filename);
          g_free (filename);
          filename = new_filename;
        }

      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          /* Correctly check the boolean return value from the custom dialog */
          if (! show_override_dialog (GTK_WINDOW (chooser)))
            {
              /* User chose not to overwrite */
              do_save = FALSE;
            }
        }
    }

  stop_virtual_keyboard ();

  if (do_save && filename)
    {
      if (save_pixbuf_on_png_file (buffer, filename))
        {
          add_artifact (filename);
        }
    }

  /*
   * The chooser owns the preview widget, so we must not destroy it manually.
   * Destroying the chooser is sufficient and correct.
   */
  gtk_widget_destroy (chooser);

  if (filename)
    {
      g_free (filename);
    }

  g_object_unref (buffer);
}

/**
 * start_save_image_dialog:
 *
 * Initiates the screenshot grabbing process.
 *
 * This function calls grab_screenshot() and provides a callback function
 * (start_save_image_dialog_callback) which will be executed upon
 * completion. The callback is responsible for showing the actual save
 * dialog to the user.
 **/
void
start_save_image_dialog (void)
{
  g_debug ("calling start_save_image_dialog\n");
  grab_screenshot (start_save_image_dialog_callback);
}
