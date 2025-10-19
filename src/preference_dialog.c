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

#include "cairo_functions.h"
#include "background_config.h"
#include "background_window.h"
#include "annotation_window.h"
#include "background_window.h"
#include "keyboard.h"
#include "preference_dialog.h"
#include "utils.h"

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
  permission_denied_dialog =
      gtk_message_dialog_new (parent,
                              GTK_DIALOG_MODAL,
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
 * start_add_background_preference_dialog:
 * @parent: The parent #GtkWindow for the preference dialog.
 *
 * Launches the modal dialog that allows a user to define and add a new
 * background to the application. The user can typically choose to add a
 * new solid color or select an image file from the filesystem.
 */
void
start_add_background_preference_dialog (GtkWindow *parent)
{
  GObject        *preference_obj    = (GObject *) NULL;
  GtkWidget      *preference_dialog = (GtkWidget *) NULL;
  GObject        *img_obj           = (GObject *) NULL;
  GtkFileChooser *chooser           = NULL;
  GtkFileFilter  *filter            = (GtkFileFilter *) NULL;
  GObject        *bg_color_obj      = (GObject *) NULL;

  PreferenceData *preference_data = (PreferenceData *) NULL;

  start_virtual_keyboard ();

  preference_data = g_malloc ((gsize) sizeof (PreferenceData));

  /* Initialize the main window. */
  GtkBuilder *preference_dialog_gtk_builder = gtk_builder_new ();

  preference_data->preference_dialog_gtk_builder =
      preference_dialog_gtk_builder;

  /* Load the gtk builder file created with glade. */
  gtk_builder_add_from_file (preference_dialog_gtk_builder,
                             PREFERENCE_UI_FILE,
                             NULL);

  /* Take the preference object. */
  preference_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                           "preferences");

  preference_dialog = GTK_WIDGET (preference_obj);

  gtk_window_set_transient_for (GTK_WINDOW (preference_dialog), parent);
  gtk_window_set_modal (GTK_WINDOW (preference_dialog), TRUE);
  gtk_window_set_keep_above (GTK_WINDOW (preference_dialog), TRUE);

  img_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                    "imageChooserButton");

  chooser = GTK_FILE_CHOOSER (img_obj);

  gtk_file_chooser_set_current_folder (chooser, BACKGROUNDS_FOLDER);

  /* Put the file filter for the supported formats. */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "PNG");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_chooser_add_filter (chooser, filter);

  preference_data->preview = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (chooser, preference_data->preview);

  bg_color_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                         "backgroundColorButton");

  gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (bg_color_obj), TRUE);

  /* Connect all signals by reflection. */
  gtk_builder_connect_signals (preference_dialog_gtk_builder,
                               (gpointer) preference_data);

  gint background_type = background_data->type;

  if (background_type == 1)
    {
      GObject *color_obj;
      color_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                          "color");

      GtkToggleButton *color_tool_button = GTK_TOGGLE_BUTTON (color_obj);
      gtk_toggle_button_set_active (color_tool_button, TRUE);
    }
  else if (background_type == 2)
    {
      GObject *file_obj = gtk_builder_get_object (preference_dialog_gtk_builder,
                                                  "file");

      GtkToggleButton *image_tool_button = GTK_TOGGLE_BUTTON (file_obj);
      gtk_toggle_button_set_active (image_tool_button, TRUE);
    }

  gchar *rgba = background_data->color;
  if (rgba)
    {
      GdkRGBA *gdkcolor = rgba_to_gdkcolor (rgba);
      gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (bg_color_obj), TRUE);
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (bg_color_obj), gdkcolor);
    }

  gchar *filename = background_data->image;
  if (filename)
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);
    }

  gtk_dialog_run (GTK_DIALOG (preference_dialog));

  if (preference_dialog)
    {
      gtk_widget_destroy (preference_dialog);
      preference_dialog = NULL;
    }

  g_object_unref (preference_dialog_gtk_builder);
  preference_data->preference_dialog_gtk_builder = NULL;
  g_free (preference_data);
  preference_data = NULL;

  stop_virtual_keyboard ();

#ifdef _WIN32
  /*
   * In Windows the parent bar go above the dialog;
   * to avoid this behaviour I have put the parent keep above to false,
   * and now I restore it.
   */
  gtk_window_set_keep_above (GTK_WINDOW (parent), TRUE);
#endif
}

/**
 * on_add_new_background:
 * @widget:    The #GtkWidget that emitted the "clicked" signal.
 * @user_data: User data passed from the signal connection.
 *
 * A callback function triggered when the user clicks the 'Add new background'
 * button. It launches the preference dialog, which allows a new color or
 * image background to be defined and saved.
 *
 * Returns: %TRUE to indicate that the event has been handled.
 */
gboolean
on_add_new_background (GtkWidget *widget, gpointer user_data)
{
  start_add_background_preference_dialog (GTK_WINDOW (widget));
  return TRUE;
}

/**
 * Effects will not show until after another gtk_widget_show_all call
 * @param data [description]
 * @param size [description]
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

  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (data->button),
                                   GTK_WIDGET (image));

  g_object_unref (pixbuf);
  cairo_surface_destroy (surface);
  cairo_destroy (cr);
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
  if (!gtk_toggle_tool_button_get_active (toggle_tool_button))
    {
      return;
    }
  BackgroundButtonData *data = (BackgroundButtonData *) userdata;
    
  if (background_data->preview_cr==NULL)
    {
      create_preview_background();
    }
  if (data->mode == BACKGROUND_MODE_COLOR)
    {
      load_color_onto_context (data->color, background_data->preview_cr);
    }
  else if (data->mode == BACKGROUND_MODE_FILE)
    {
      load_file_onto_context (data->filename, background_data->preview_cr);
    }
    
  gtk_widget_queue_draw (annotation_data->annotation_window);
}

/**
 * on_remove_background_button:
 * @menuitem:  The #GtkMenuItem that emitted the "activate" signal.
 * @user_data: A pointer to the #GtkWidget (the background button) to be removed.
 *
 * Callback for the context menu action to remove a user-added background.
 *
 * This function is triggered when the "Remove" option is selected from the
 * right-click menu of a background button. It retrieves the unique label
 * of the background from the widget's data, removes the corresponding
 * entry from the configuration file, and then destroys the button widget
 * itself to remove it from the selection dialog.
 *
 * Returns: %TRUE to indicate the signal was handled.
 */
gboolean
on_remove_background_button (GtkMenuItem *menuitem, gpointer user_data)
{
  GtkWidget   *widget = GTK_WIDGET (user_data);
  gint         width  = gtk_widget_get_allocated_width (widget);
  GObject     *obj    = G_OBJECT (widget);
  const gchar *background_key_name;
  background_key_name = g_object_get_data (obj, "background-label");

  if (background_key_name)
    {
      background_config_remove_key (background_key_name);
    }

  gtk_widget_destroy (widget);
  GtkWidget *background_selection_window;
  gint       cwidth;
  gint       cheight;
  background_selection_window = annotation_data->background_selection_window;
  cwidth  = gtk_widget_get_allocated_width (background_selection_window);
  cheight = gtk_widget_get_allocated_height (background_selection_window);

  gtk_window_resize (GTK_WINDOW (annotation_data->background_selection_window),
                     cwidth - width,
                     cheight);
  return TRUE;
}

/**
 * background_selection_on_button_press:
 * @widget:    The #GtkWidget (the background button) that received the event.
 * @event:     The #GdkEventButton containing details about the press.
 * @userdata:  A pointer to the #BackgroundButtonData for the pressed button.
 *
 * Callback for the "button-press-event" signal on a background button,
 * used to handle right-clicks.
 *
 * This function checks if the user has right-clicked on a user-added
 * background. If so, it creates and displays a popup context menu with a
 * "Remove" option, allowing the user to delete that background.
 *
 * Returns: %TRUE if a context menu was shown (event handled), otherwise
 * %FALSE to allow the default event handling (e.g., the 'toggled'
 * signal for a left-click) to proceed.
 */
gboolean
background_selection_on_button_press (GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer userdata)
{
  GdkEventButton *event_button;
  if (event->type == GDK_BUTTON_PRESS)
    {
      event_button                      = (GdkEventButton *) event;
      BackgroundButtonData *button_data = (BackgroundButtonData *) userdata;
      if (button_data->index > 3)
        {
          if (event_button->button == GDK_BUTTON_SECONDARY)
            {
              // create a popup for delete
              GtkWidget *menu, *menuitem;
              menu     = gtk_menu_new ();
              menuitem = gtk_menu_item_new_with_label (gettext ("Remove"));
              gtk_menu_attach (GTK_MENU (menu), menuitem, 0, 1, 0, 1);

              g_signal_connect (menuitem,
                                "activate",
                                (GCallback) on_remove_background_button,
                                widget);

              gtk_widget_show_all (menu);
              gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
              return TRUE;
            }
        }
    }
  return FALSE;
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
  GtkToolItem *button = NULL;

  if (annotation_data->background_button_data == NULL)
    {
      button = gtk_radio_tool_button_new (NULL);
    }
    else
    {
      GSList               *last_node;
      BackgroundButtonData *data;
      last_node = g_slist_last (annotation_data->background_button_data);
      data      = (BackgroundButtonData *) last_node->data;

      GtkRadioToolButton *radio_button;
      radio_button = GTK_RADIO_TOOL_BUTTON (data->button);
      button       = gtk_radio_tool_button_new_from_widget (radio_button);
    }
  g_object_set_data_full (G_OBJECT (button),
                          "background-label",      /* key */
                          label,                   /* value */
                          g_free);                 /* destroy notify */
                 
  GtkWidget *background_selection_container;
  background_selection_container =
    annotation_data->background_selection_container;

  gtk_box_pack_start (GTK_BOX (background_selection_container),
                      GTK_WIDGET (button),
                      TRUE,
                      TRUE,
                      0);

  /*
   * If an 'Add' button was stored on the container,
   * move the new button right before it so the Add button stays last.
   * This preserves all the existing logic that runs before/after
   * packing the button.
   */
  GtkWidget *add_btn =
      g_object_get_data (G_OBJECT (background_selection_container),
                         "background_add_button");

  if (add_btn != NULL)
    {
      GList *children = gtk_container_get_children (
        GTK_CONTAINER (background_selection_container));

      gint pos = g_list_index (children, add_btn);
      if (pos >= 0)
        {
          gtk_box_reorder_child (GTK_BOX (background_selection_container),
                                 GTK_WIDGET (button),
                                 pos);
        }
      g_list_free (children);
    }

  BackgroundButtonData *data = g_new (BackgroundButtonData, 1);
  data->mode                 = mode;
  data->filename             = filename;
  data->color                = color;
  data->index  = g_slist_length (annotation_data->background_button_data);
  data->size   = 32;
  data->button = button;

  annotation_data->background_button_data =
    g_slist_append (annotation_data->background_button_data,
                    data);

  // used added button
  g_signal_connect (button,
                    "toggled",
                    (GCallback) background_selection_on_toggled,
                    data);

  g_signal_connect (button,
                    "button_press_event",
                    (GCallback) background_selection_on_button_press,
                    data);
                    
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (button),
                                     active);
  
  if (g_slist_length (annotation_data->background_button_data) > 3)
    {
      gtk_widget_show_all (annotation_data->background_selection_window);
    }
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
static void
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
      BackgroundButtonData *bdata = (BackgroundButtonData *) node->data;
      gboolean active = gtk_toggle_tool_button_get_active (
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
      g_object_set_data (G_OBJECT (window), "background-committed",
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
                            gint       response,
                            gpointer   user_data)
{
  GtkWidget *window = GTK_WIDGET (dialog);
  /* Free preview */
  destroy_background_data_preview ();

  if (response == GTK_RESPONSE_OK)
	{
      persist_current_selection (window);
	}
  else
	{
	  gtk_widget_queue_draw (annotation_window);
	}
  gtk_widget_destroy (GTK_WIDGET (window));
}

void
on_background_selection_window_destroy (GtkWidget *object, gpointer user_data)
{
  annotation_data->background_selection_window    = NULL;
  annotation_data->background_selection_container = NULL;

  g_slist_free_full (annotation_data->background_button_data,
                     background_button_data_free);

  annotation_data->background_button_data          = NULL;
  annotation_data->background_button_last_selected = BACKGROUND_NONE_SELECTED;
}

void
on_background_selection_size_allocate (GtkWidget *widget,
                                       GdkRectangle *allocation,
                                       gpointer user_data)
{
  // g_printf ("size allocate %d %d\n", allocation->width, allocation->height);
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
          gboolean is_active = (current_bg_label && g_strcmp0 (color_keys[i], current_bg_label) == 0);
                                
          g_debug ("Load background color %s in preference (active: %d)",
                   hex, is_active);
                   
          if (g_strcmp0 (color_keys[i], "transparent") == 0)
            {
              add_background_button (g_strdup ("transparent"),
                                     BACKGROUND_MODE_NONE,
                                     g_strdup (TRANSPARENT_BACKGROUND_FILE),
                                     NULL,
                                     is_active);
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
          gchar *path = background_config_get_image (image_keys[i]);
          gboolean is_active = (current_bg_label &&
                                g_strcmp0 (image_keys[i], current_bg_label) == 0);
          if (path)
            {
              g_debug ("Load background image %s in preference (active: %d)", path, is_active);
              add_background_button (g_strdup(image_keys[i]),
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
 * on_background_selection_window_configure_event:
 * @widget:    The #GtkWidget (the dialog window) that emitted the signal.
 * @event:     The #GdkEventConfigure containing the new window geometry.
 * @user_data: User data (unused).
 *
 * Callback for the "configure-event" signal, used to dynamically resize
 * the background thumbnail icons.
 *
 * This function is triggered whenever the background selection window is
 * resized. It calculates a new optimal size for the button icons based on
 * the window's new height, rounding it to a power of two. It then
 * iterates through all background buttons and calls `resize_image_to_button()`
 * for any icon whose size does not match the new calculated size. This
 * ensures the thumbnails scale cleanly as the user resizes the window.
 *
 * Returns: %FALSE to allow the default GTK handler to process the event.
 */
gboolean
on_background_selection_window_configure_event (GtkWidget *widget,
                                                GdkEvent *event,
                                                gpointer user_data)
{
  if (widget == annotation_data->background_selection_window &&
      event->type == GDK_CONFIGURE)
    {
      gint elements = g_slist_length (annotation_data->background_button_data);
      GtkWidget *window;
      window = GTK_WIDGET (annotation_data->background_selection_window);
      gint h = gtk_widget_get_allocated_height (window);
      gint m = (int) log2 ((double) h);
      m      = (int) pow (2.0, m);

      for (gint ii = 0; ii < elements; ii++)
        {
          GSList               *node;
          BackgroundButtonData *data;
          node = g_slist_nth (annotation_data->background_button_data, ii);
          data = (BackgroundButtonData *) node->data;
          if (data->size != m)
            {
              g_debug ("redrawing image (%d)\n", m);
              resize_image_to_button (data, m);
            }
        }
      // without this the button icons remain blank and the window does not
      // resize to fit added buttons
      gtk_widget_show_all (annotation_data->background_selection_window);
    }
  return FALSE;
}

/**
 * create_bar_preference_window:
 * @parent: The transient parent #GtkWindow for the dialog.
 *
 * Creates and displays the background selection dialog.
 *
 * If an instance of the dialog already exists, this function simply brings
 * it to the front instead of creating a new one. Otherwise, it creates a
 * new modal #GtkDialog containing a #GtkFlowBox populated with all available
 * background choices.
 *
 * After populating the dialog, it calls `activate_initial_background()` to
 * ensure the displayed selection matches the user's saved preference. Signal
 * handlers are connected to manage the OK (persist choice) and Cancel
 * (revert changes) actions.
 */
void
create_bar_preference_window (GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkBox *box;
	GtkToolItem *add_btn;
	BackgroundRestored *br;

	dialog = gtk_dialog_new_with_buttons (gettext ("Backgrounds"),
	                                      parent,
	                                      GTK_DIALOG_MODAL |
	                                          GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      gettext ("_Cancel"),
	                                      GTK_RESPONSE_CANCEL,
	                                      gettext ("_OK"),
	                                      GTK_RESPONSE_OK,
	                                      NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 320);

	annotation_data->background_selection_window = GTK_WIDGET (dialog);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
	gtk_container_add (GTK_CONTAINER (content_area), GTK_WIDGET (box));
	annotation_data->background_selection_container = GTK_WIDGET (box);

	/* Load backgrounds from configuration */
	load_backgrounds_from_config ();

	/* Store original background to restore on cancel */
	br = background_config_restore_last_background ();
	g_object_set_data_full (G_OBJECT (dialog),
	                        "background-original-br",
	                        br,
	                        (GDestroyNotify) background_restored_free);

	g_object_set_data (G_OBJECT (dialog), "background-committed",
	                   GINT_TO_POINTER (0));

	/* Add "Add background" toolbutton */
	add_btn = GTK_TOOL_ITEM (gtk_tool_button_new (
	    gtk_image_new_from_icon_name ("list-add",
	                                  GTK_ICON_SIZE_LARGE_TOOLBAR),
	    NULL));
	gtk_tool_item_set_tooltip_text (add_btn, gettext ("Add background"));
	g_object_set_data (G_OBJECT (box), "background_add_button",
	                   GTK_WIDGET (add_btn));
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (add_btn), TRUE, TRUE, 0);
	g_signal_connect (add_btn, "clicked",
	                  (GCallback) on_add_new_background,
	                  dialog);

	g_signal_connect (dialog,
	                  "configure-event",
	                  (GCallback) on_background_selection_window_configure_event,
	                  NULL);

	g_signal_connect (dialog,
	                  "size-allocate",
	                  (GCallback) on_background_selection_size_allocate,
	                  NULL);

	g_signal_connect (dialog,
	                  "response",
	                  G_CALLBACK (background_dialog_response),
	                  NULL);

	g_signal_connect (dialog,
	                  "destroy",
	                  (GCallback) on_background_selection_window_destroy,
	                  NULL);

	gtk_widget_show_all (dialog);
}
