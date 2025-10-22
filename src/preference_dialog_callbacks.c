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

/**
 * on_image_chooser_button_update_preview:
 * @file_chooser: The #GtkFileChooser widget emitting the signal.
 * @data:         User data, expected to be a #PreferenceData pointer.
 *
 * Callback for the "update-preview" signal of the image chooser button.
 * It generates a 128x128 pixel thumbnail of the currently selected file
 * and displays it in the preview widget associated with the preference data.
 * It also activates or deactivates the preview widget based on whether
 * a valid preview could be generated.
 */
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

 /**
 * on_background_color_button_color_set:
 * @buton: The #GtkButton (specifically a #GtkColorButton) that emitted
 * the signal.
 * @data:  User data, expected to be a #PreferenceData pointer.
 *
 * Callback for the "color-set" signal of the background color button.
 * This function is triggered when the user selects a color. It automatically
 * activates the "background color" radio button within the preference dialog
 * to ensure the UI reflects that a color is the currently chosen
 * background type.
 */
G_MODULE_EXPORT void
on_background_color_button_color_set (GtkButton *button, gpointer data)
{
  GdkRGBA gdkcolor;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &gdkcolor);
  gchar *rgba = gdkrgba_to_rgba (&gdkcolor);

  // Aggiungi al grid principale senza persistere
  add_background_button(g_strdup(rgba),
                        BACKGROUND_MODE_COLOR,
                        NULL,
                        g_strdup(rgba),
                        TRUE);

  background_config_add_color (rgba, rgba);
  g_free (rgba);
}

/**
 * on_image_chooser_button_file_set:
 * @buton: The #GtkButton (specifically a #GtkFileChooserButton) that
 * emitted the signal.
 * @data:  User data, expected to be a #PreferenceData pointer.
 *
 * Callback for the "file-set" signal of the image chooser button.
 * This function is triggered when the user selects a file. It automatically
 * activates the "image" radio button within the preference dialog to ensure
 * the UI reflects that an image file is the currently chosen background type.
 */
G_MODULE_EXPORT void
on_image_chooser_button_file_set (GtkFileChooserButton *button,
                                  gpointer user_data)
{
  gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (button));
  if (! filename)
    return;

  FILE *stream = g_fopen (filename, "r");
  if (! stream)
    {
      GtkWidget *parent_widget = annotation_data->background_selection_window;
      show_permission_denied_dialog (GTK_WINDOW (parent_widget));
      return;
    }

  gchar *name = background_config_filename_to_label (filename);

  add_background_button(g_strdup(name),
                        BACKGROUND_MODE_FILE,
                        g_strdup(filename),
                        NULL,
                        TRUE);

  background_config_add_image (name, filename);
  g_free (name);
  g_free (filename);
  fclose (stream);
}

/**
 * on_remove_background_button:
 * @menuitem:  The #GtkMenuItem that emitted the "activate" signal.
 * @user_data: A pointer to the #GtkWidget (the background button)
 *             to be removed.
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
  GtkToolItem *button_item         = GTK_TOOL_ITEM (user_data);
  const gchar *background_key_name = g_object_get_data (G_OBJECT (button_item),
                                                        "background-label");

  if (! background_key_name)
    {
      g_warning ("Could not find background label to remove.");
      return TRUE;
    }

  background_config_remove_key (background_key_name);

  GSList *iter = annotation_data->background_button_data;
  while (iter)
    {
      BackgroundButtonData *bdata = (BackgroundButtonData *) iter->data;
      GSList               *next  = iter->next;
      if (bdata && bdata->button == button_item)
        {
          annotation_data->background_button_data = g_slist_delete_link (
              annotation_data->background_button_data, iter);
          background_button_data_free (bdata);
          break;
        }
      iter = next;
    }

  /* Remove the widget from the container. */
  GtkWidget *wrapper = gtk_widget_get_parent (GTK_WIDGET (button_item));
  if (wrapper)
    gtk_widget_destroy (wrapper);

  /* Force window resize to recalc grid size. */
  GtkWidget *grid = annotation_data->background_selection_container;
  gtk_widget_queue_resize (grid);

  GtkWidget *window = annotation_data->background_selection_window;
  if (window)
    {
      /*
       * This forces GTK to recalc the window size
       * based on the remaining widgets.
       */
      gtk_window_resize (GTK_WINDOW (window), 1, 1);
      gtk_widget_queue_resize (window);
    }

  return TRUE;
}

/**
 * background_selection_on_button_press:
 * @widget:   The background button widget that received the event.
 * @event:    The GdkEvent associated with the button press.
 * @userdata: User data associated with the widget (ignored here).
 *
 * Handles right-click events on background buttons. If the user right-clicks
 * a background, a context menu is created with a "Remove" option to delete
 * the background. The function looks for the "background-label" either on
 * the widget itself or its child widgets.
 *
 * Returns: TRUE if the event was handled (right-click menu shown), FALSE
 *          otherwise (left-click or other events).
 */
gboolean
background_selection_on_button_press (GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer userdata)
{
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  GdkEventButton *ev = (GdkEventButton *) event;
  if (ev->button != GDK_BUTTON_SECONDARY)
    return FALSE;

  const gchar *background_key_name = g_object_get_data (G_OBJECT (widget),
                                                        "background-label");

  /* If label not found on the widget itself, search in child widgets */
  if (! background_key_name && GTK_IS_CONTAINER (widget))
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (widget));
      for (GList *l = children; l != NULL; l = l->next)
        {
          GtkWidget *child = GTK_WIDGET (l->data);
          if (! child)
            continue;

          const gchar *lbl = g_object_get_data (G_OBJECT (child),
                                                "background-label");
          if (lbl)
            {
              background_key_name = lbl;
              break;
            }
        }
      g_list_free (children);
    }

  if (! background_key_name)
    {
      g_printerr ("no background-label found for widget %p\n", widget);
      return FALSE;
    }

  /* Create the context menu. */
  GtkWidget *menu     = gtk_menu_new ();
  GtkWidget *menuitem = gtk_menu_item_new_with_label (gettext ("Remove"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show_all (menu);

  /* Connect the remove callback, passing the container widget. */
  g_signal_connect (menuitem,
                    "activate",
                    G_CALLBACK (on_remove_background_button),
                    widget);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) ev);

  return TRUE;
}

/**
 * on_background_selection_window_destroy:
 * @object:    The background selection window that was destroyed.
 * @user_data: User data passed to the callback (unused here).
 *
 * Cleans up the global annotation_data references when the background
 * selection window is destroyed. Frees all associated background button
 * data and resets the tracking variables.
 */
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
