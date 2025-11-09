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
#include "background_config.h"
#include "background_window.h"
#include "cairo_functions.h"
#include "keyboard.h"
#include "preference_dialog.h"
#include "preference_dialog_callbacks.h"
#include "utils.h"

const gint BACKGROUND_ICON_SIZE = 128;

/**
 * show_permission_denied_dialog:
 * @parent: the parent #GtkWindow for the dialog.
 *
 * Shows a modal error dialog indicating that access to a file
 * was denied due to insufficient permissions.
 **/
void
show_permission_denied_dialog (GtkWindow *parent)
{
  GtkWidget   *permission_denied_dialog = (GtkWidget *) NULL;
  const gchar *message_text;
  message_text = gettext ("Fail to open the file: Permission denied");
  permission_denied_dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     "%s",
                                                     message_text);

  gtk_window_set_keep_above (GTK_WINDOW (permission_denied_dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (permission_denied_dialog));

  if (permission_denied_dialog != NULL)
    {
      gtk_widget_destroy (permission_denied_dialog);
      permission_denied_dialog = NULL;
    }
}

/**
 * resize_image_to_button:
 * @data: A pointer to the #BackgroundButtonData containing the button
 *        and image.
 * @size: The desired width and height for the image in pixels.
 *
 * Resizes the icon image of a background selection button to the specified
 * square size. This function recreates the icon using Cairo to render either
 * the background color or image onto a new surface, then updates the GtkImage
 * inside the GtkToolButton.
 *
 * Note: The visual update of the icon may not appear immediately unless
 * gtk_widget_show_all() is called on the parent container afterwards.
 */
void
resize_image_to_button (BackgroundButtonData *data, gint size)
{
  gint             x = 0, y = 0, w = size, h = size;
  cairo_t         *cr      = NULL;
  cairo_surface_t *surface = NULL;
  gchar           *output  = NULL;
  GError          *error   = NULL;
  GtkToolButton   *tool_button;
  GtkWidget       *icon_widget;
  GtkImage        *image;

  tool_button = GTK_TOOL_BUTTON (data->button);
  icon_widget = gtk_tool_button_get_icon_widget (tool_button);
  image       = GTK_IMAGE (icon_widget);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  cr      = cairo_create (surface);

  if (data->filename != NULL)
    {
      load_file_onto_context (data->filename, cr);
    }
  else if (data->color != NULL)
    {
      load_color_onto_context (data->color, cr);
    }

  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, x, y, w, h);
  output            = g_strdup_printf ("test_output%d.png", data->index);
  gdk_pixbuf_save (pixbuf, output, "png", &error, NULL);
  g_free (output);

  if (error != NULL)
    {
      g_printerr ("%s\n", error->message);
    }

  if (image == NULL)
    {
      g_debug ("create new image\n");
      image = GTK_IMAGE (gtk_image_new_from_pixbuf (pixbuf));
    }
  else
    {
      g_debug ("setting image\n");
      gtk_image_set_from_pixbuf (image, pixbuf);
    }
  data->size = size;

  gtk_tool_button_set_icon_widget (tool_button, GTK_WIDGET (image));

  g_object_unref (pixbuf);
  cairo_surface_destroy (surface);
  cairo_destroy (cr);
  gtk_widget_show (GTK_WIDGET (image));
}

/**
 * background_selection_on_toggled:
 * @toggle_tool_button: The #GtkToggleToolButton that emitted the signal.
 * @userdata:           A pointer to the #BackgroundButtonData associated with
 * the toggled button.
 *
 * Callback for the "toggled" signal on a background selection button.
 *
 * This function is triggered whenever a background radio button's state
 * changes. It acts only when a button is **activated**, ignoring the
 * deactivation signal.
 *
 * Its purpose is to apply the selected background as a visual preview
 * without permanently saving the choice. It updates the application's
 * internal selection state (`background_button_last_selected`) and queues
 * a redraw of the main canvas to show the new background.
 */
void
background_selection_on_toggled (GtkToggleToolButton *toggle_tool_button,
                                 gpointer userdata)
{
  if (! gtk_toggle_tool_button_get_active (toggle_tool_button))
    {
      return;
    }
  BackgroundButtonData *data = (BackgroundButtonData *) userdata;

  if (background_data->preview_cr == NULL)
    {
      create_preview_background ();
    }

  if (data->mode == BACKGROUND_MODE_COLOR)
    {
      load_color_onto_context (data->color, background_data->preview_cr);
    }
  else if (data->mode == BACKGROUND_MODE_FILE)
    {
      load_file_onto_context (data->filename, background_data->preview_cr);
    }
  else if (data->mode == BACKGROUND_MODE_NONE)
    {
      load_color_onto_context ("00000000", background_data->preview_cr);
    }

  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/**
 * background_button_data_free:
 * @data: The #BackgroundButtonData to free, passed as a #gpointer.
 *
 * A #GDestroyNotify callback function for freeing a #BackgroundButtonData
 * struct and its internally allocated strings.
 *
 * This function frees the `filename` and `color` strings within the
 * struct. It does **not** free the `button` member, as the widget's
 * lifecycle is managed by its parent container. Finally, it frees the
 * #BackgroundButtonData struct itself.
 */
void
background_button_data_free (gpointer data)
{
  BackgroundButtonData *background_data = (BackgroundButtonData *) data;

  if (background_data == NULL)
    {
      return;
    }

  if (background_data->filename != NULL)
    {
      g_free (background_data->filename);
      background_data->filename = NULL;
    }

  if (background_data->color != NULL)
    {
      g_free (background_data->color);
      background_data->color = NULL;
    }

  // NOTE: Do NOT free bdata->button. It's a GtkWidget and its
  // lifecycle is managed by its parent container.

  g_free (background_data);
  background_data = NULL;
}

/**
 * add_background_button:
 * @label:   the text label for the button
 * @mode:    background mode (color or image)
 * @filename: path to the image file (if mode is image)
 * @color:   color value (if mode is color)
 * @active:  %TRUE to set this button as the initially active one
 *
 * Creates a new radio tool button representing a background (color or image)
 * and appends it to the annotation background selection container. If there
 * is already at least one button, this button is linked to the previous ones
 * as a radio button group. The button is inserted before the "Add" button
 * if one exists, keeping the "Add" button at the end.
 *
 * Additional button metadata (mode, filename, color, index, size) is stored
 * in a BackgroundButtonData structure attached to the button.
 *
 * Signal handlers are connected for toggled and button_press_event events.
 * If more than 3 buttons exist, the background selection window is shown.
 **/
void
add_background_button (gchar *label,
                       gint mode,
                       gchar *filename,
                       gchar *color,
                       gboolean active)
{
  GtkToolItem          *button_item;
  BackgroundButtonData *data;
  GtkWidget            *button;
  GtkWidget            *grid;
  GtkWidget            *wrapper;
  gint                  n_buttons, row, col;

  grid = annotation_data->background_selection_container;

  /* Create a new radio tool button (group with previous if exists). */
  if (annotation_data->background_button_data == NULL)
    button_item = gtk_radio_tool_button_new (NULL);
  else
    {
      GSList *last_node = NULL;
      last_node = g_slist_last (annotation_data->background_button_data);

      BackgroundButtonData *last_data = NULL;
      last_data = (BackgroundButtonData *) last_node->data;

      button_item = gtk_radio_tool_button_new_from_widget (
          GTK_RADIO_TOOL_BUTTON (last_data->button));
    }

  /* Store the label. */
  g_object_set_data_full (G_OBJECT (button_item),
                          "background-label",
                          label,
                          g_free);

  /* Allocate and populate button data. */
  data           = g_new0 (BackgroundButtonData, 1);
  data->mode     = mode;
  data->filename = filename;
  data->color    = color;
  data->index    = g_slist_length (annotation_data->background_button_data);
  data->size     = BACKGROUND_ICON_SIZE;
  data->button   = button_item;

  /* Create GtkImage for icon. */
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (button_item),
                                   gtk_image_new ());

  resize_image_to_button (data, data->size);

  button = GTK_WIDGET (button_item);
  gtk_widget_set_size_request (button, data->size, data->size);

  /* Wrap in EventBox per Grid compat. */
  wrapper = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (wrapper), button);

  /* Compute row/column. */
  n_buttons = g_slist_length (annotation_data->background_button_data);
  col       = n_buttons % 6;
  row       = n_buttons / 6;

  gtk_grid_attach (GTK_GRID (grid), wrapper, col, row, 1, 1);

  /* Track the button data. */
  annotation_data->background_button_data =
      g_slist_append (annotation_data->background_button_data,
                      data);

  /* Connect signals. */
  g_signal_connect (button_item,
                    "toggled",
                    G_CALLBACK (background_selection_on_toggled),
                    data);

  g_signal_connect (button,
                    "button_press_event",
                    G_CALLBACK (background_selection_on_button_press),
                    data);

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (button_item),
                                     active);

  gtk_widget_show_all (wrapper);
  gtk_widget_queue_resize (annotation_data->background_selection_window);
}

/**
 * persist_current_selection:
 * @window: The parent #GtkWidget (the dialog) to mark as committed.
 *
 * Persists the currently selected background to the user's configuration file.
 *
 * This function retrieves the index of the currently active background from
 * the application's state. It then finds the corresponding background data
 * and calls the `background_config_set_current_background()` API to save
 * the choice permanently.
 *
 * Finally, it sets a "background-committed" flag on the @window, which
 * signals to the destroy handler that the changes were confirmed and
 * should not be reverted.
 */
static void
persist_current_selection (GtkWidget *window)
{
  GSList *node = annotation_data->background_button_data;
  for (; node != NULL; node = node->next)
    {
      BackgroundButtonData *bdata  = (BackgroundButtonData *) node->data;
      gboolean              active = gtk_toggle_tool_button_get_active (
          GTK_TOGGLE_TOOL_BUTTON (bdata->button));
      if (! active)
        continue;

      /* Prefer the stored label for persistence when available. */
      const gchar *label = g_object_get_data (G_OBJECT (bdata->button),
                                              "background-label");

      if (label != NULL)
        {
          background_config_set_current_background (label);
        }
      else if (bdata->mode == BACKGROUND_MODE_FILE && bdata->filename)
        {
          gchar *name = background_config_filename_to_label (bdata->filename);
          background_config_set_current_background (name);
          g_free (name);
        }
      else if (bdata->mode == BACKGROUND_MODE_NONE)
        {
          background_config_set_current_background ("transparent");
        }

      /* Mark window as committed so destroy handler won't revert. */
      g_object_set_data (G_OBJECT (window),
                         "background-committed",
                         GINT_TO_POINTER (1));

      return;
    }
}

/**
 * background_dialog_response:
 * @dialog:  the GtkDialog that emitted the response
 * @response: the GtkResponseType (OK, CANCEL, etc.)
 * @user_data: unused
 *
 * Handles responses from the background selection dialog.
 * - OK: persist the chosen background
 * - CANCEL or others: restore the original background
 */
static void
background_dialog_response (GtkDialog *dialog,
                            gint response,
                            gpointer user_data)
{
  GtkWidget *window = GTK_WIDGET (dialog);

  if (response == GTK_RESPONSE_OK)
    {
      destroy_background_data_preview ();
      persist_current_selection (window);
    }
  else
    {
      destroy_background_data_preview ();
      gtk_widget_queue_draw (annotation_window);
    }
  gtk_widget_destroy (GTK_WIDGET (window));
}

/**
 * load_backgrounds_from_config:
 *
 * Populates the background selection UI with all user-defined backgrounds
 * found in the configuration file.
 *
 * This function queries the configuration file for all entries under the
 * "colors" and "images" groups. For each entry found, it calls
 * `add_background_button()` to create and add a corresponding radio tool
 * button to the selection dialog.
 */
static void
load_backgrounds_from_config (void)
{
  gchar *current_bg_label = background_config_get_current_background ();

  gsize   n_colors   = 0;
  gchar **color_keys = background_config_get_color_keys (&n_colors);

  if (color_keys)
    {
      for (gsize i = 0; i < n_colors; i++)
        {
          gchar *hex = background_config_get_color (color_keys[i]);
          if (! hex)
            {
              continue;
            }
          gboolean is_active = FALSE;
          if (current_bg_label &&
              g_strcmp0 (color_keys[i], current_bg_label) == 0)
            {
              is_active = TRUE;
            }

          g_debug ("Load background color %s in preference (active: %d)",
                   hex,
                   is_active);

          if (g_strcmp0 (color_keys[i], "transparent") == 0)
            {
              add_background_button (g_strdup ("transparent"),
                                     BACKGROUND_MODE_NONE,
                                     g_strdup (TRANSPARENT_BACKGROUND_FILE),
                                     NULL, is_active);
            }
          else
            {
              add_background_button (g_strdup (color_keys[i]),
                                     BACKGROUND_MODE_COLOR,
                                     NULL,
                                     g_strdup (hex),
                                     is_active);
              g_free (hex);
            }
        }
      g_strfreev (color_keys);
    }

  gsize   n_images   = 0;
  gchar **image_keys = background_config_get_image_keys (&n_images);

  if (image_keys)
    {
      for (gsize i = 0; i < n_images; i++)
        {
          gchar   *path      = background_config_get_image (image_keys[i]);
          gboolean is_active = FALSE;
          if (current_bg_label &&
              g_strcmp0 (image_keys[i], current_bg_label) == 0)
            {
              is_active = TRUE;
            }

          if (path)
            {
              g_debug ("Load background image %s in preference (active: %d)",
                       path,
                       is_active);

              add_background_button (g_strdup (image_keys[i]),
                                     BACKGROUND_MODE_FILE,
                                     g_strdup (path),
                                     NULL,
                                     is_active);
              g_free (path);
            }
        }
      g_strfreev (image_keys);
    }
}

/**
 * create_bar_preference_window:
 * @parent: The transient parent #GtkWindow for the dialog.
 *
 * Creates and displays the background selection dialog.
 *
 * After populating the dialog, it calls `activate_initial_background()` to
 * ensure the displayed selection matches the user's saved preference. Signal
 * handlers are connected to manage the OK (persist choice) and Cancel
 * (revert changes) actions.
 */
void
create_bar_preference_window (GtkWindow *parent)
{
  GtkWidget          *dialog;
  GtkWidget          *grid;
  BackgroundRestored *br;
  GtkBuilder         *builder;

  /* Load dialog from Glade */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, PREFERENCE_UI_FILE, NULL);

  /* Get the main dialog widget */
  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "preferences"));
  annotation_data->background_selection_window = GTK_WIDGET (dialog);

  /* Get the grid from Glade */
  grid = GTK_WIDGET (gtk_builder_get_object (builder, "background_grid"));
  annotation_data->background_selection_container = grid;

  /* Store original background to restore on cancel. */
  br = background_config_restore_last_background ();
  g_object_set_data_full (G_OBJECT (dialog),
                          "background-original-br",
                          br,
                          (GDestroyNotify) background_restored_free);

  g_object_set_data (G_OBJECT (dialog),
                     "background-committed",
                     GINT_TO_POINTER (0));

  /* Add PNG filter to image chooser */
  GtkFileChooser *chooser;
  chooser = GTK_FILE_CHOOSER (
      gtk_builder_get_object (builder, "imageChooserButton"));

  PreferenceData *preference_data = (PreferenceData *) NULL;
  preference_data                 = g_malloc ((gsize) sizeof (PreferenceData));
  preference_data->preference_dialog_gtk_builder = builder;

  if (chooser)
    {
      gtk_file_chooser_set_current_folder (chooser, BACKGROUNDS_FOLDER);
      GtkFileFilter *filter = gtk_file_filter_new ();
      gtk_file_filter_set_name (filter, "PNG Images");
      gtk_file_filter_add_pattern (filter, "*.png");
      gtk_file_filter_add_mime_type (filter, "image/png");
      gtk_file_chooser_add_filter (chooser, filter);
      preference_data->preview = gtk_image_new ();
      gtk_file_chooser_set_preview_widget (chooser, preference_data->preview);
    }

  /* Load all backgrounds from configuration. */
  load_backgrounds_from_config ();

  /* Connect all signals by reflection. */
  gtk_builder_connect_signals (builder, (gpointer) preference_data);

  g_signal_connect (dialog,
                    "destroy",
                    G_CALLBACK (on_background_selection_window_destroy),
                    NULL);

  g_signal_connect (dialog,
                    "response",
                    G_CALLBACK (background_dialog_response),
                    NULL);

  /* Show everything. */
  gtk_widget_show_all (dialog);
}
