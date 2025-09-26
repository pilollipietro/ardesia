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

#include <gdk/gdk.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "annotation_window.h"
#include "background_config.h"
#include "background_window.h"
#include "bar.h"
#include "bar_callbacks.h"
#include "cairo_functions.h"
#include "color_selector.h"
#include "font_selector.h"
#include "info_dialog.h"
#include "iwb_saver.h"
#include "pdf_saver.h"
#include "preference_dialog.h"
#include "recorder.h"
#include "saver.h"
#include "share_confirmation_dialog.h"
#include "text_window.h"
#include "utils.h"

/* Windows state event: this occurs when the windows state changes. */
G_MODULE_EXPORT gboolean
on_bar_window_state_event (GtkWidget *widget,
		           GdkEventWindowState *event,
			   gpointer func_data)
{
  g_debug ("on bar state event\n");
  BarData *bar_data = (BarData *) func_data;
  GdkWindow *win = gtk_widget_get_window(widget);
  GdkWindowState state = gdk_window_get_state(win);

  /* Track the minimized signals */
  if (state & GDK_WINDOW_STATE_ICONIFIED)
    {
      release_lock (bar_data);
    }

  return TRUE;
}

G_MODULE_EXPORT gboolean
on_bar_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  g_debug ("bar draw event\n");
  return FALSE;
}

void
on_bar_hide_event (GtkWidget *widget, gpointer user_data)
{
  g_debug ("bar hide event\n");
}

/* Configure events occurs.
 * Called at very start and selected defaults
 */
G_MODULE_EXPORT gboolean
on_bar_configure_event (GtkWidget *widget, GdkEvent *event, gpointer func_data)
{
  g_debug ("bar configure event (%d)\n", bar_data->screenshot_pending);
  if (! bar_data->screenshot_pending)
    {
      set_options (bar_data);
    }
  return TRUE;
}

/* Called when push the quit button or the window close action */
G_MODULE_EXPORT gboolean
on_bar_quit (GtkToolButton *toolbutton, gpointer func_data)
{
  g_debug ("on_bar_quit\n");
  BarData *bar_data = (BarData *) func_data;

  stop_recorder ();

  bar_data->grab = FALSE;
  /* Release grab. */
  annotate_release_grab ();

  export_iwb (get_iwb_filename ());
  quit_pdf_saver ();
  // start_share_dialog ();

  // handles removal of background and text data structures
  annotate_quit ();

  /* Quit the gtk engine. */
  gtk_main_quit ();
  return FALSE;
}

/* Called when push the info button. */
G_MODULE_EXPORT gboolean
on_bar_info (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;

  /* Release grab. */
  annotate_release_grab ();

  /* Start the info dialog. */
  start_info_dialog (toolbutton, GTK_WINDOW (get_bar_widget ()));

  bar_data->grab = grab_value;
  start_tool (bar_data);
  return TRUE;
}

/* Called when leave the window. */
G_MODULE_EXPORT gboolean
on_bar_leave_notify_event (GtkWidget *widget,
		           GdkEvent *event,
			   gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  start_tool (bar_data);
  return TRUE;
}

/* Called when enter the window. */
G_MODULE_EXPORT gboolean
on_bar_enter_notify_event (GtkWidget *widget,
		           GdkEvent *event,
			   gpointer func_data)
{
  g_debug ("bar enter notify event\n");
  if (is_text_toggle_tool_button_active ())
    {
      stop_text_widget ();
    }
  return TRUE;
}

/* Push pointer button. */
G_MODULE_EXPORT void
on_bar_pointer_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  release_lock (bar_data);
}

/* Push text button. */
G_MODULE_EXPORT void
on_bar_text_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  replace_status_message (gettext ("Text tool selected"));
}

/* Push mode button. */
G_MODULE_EXPORT void
on_bar_mode_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  take_pen_tool ();
  if (! bar_data->rectifier)
    {
      if (! bar_data->rounder)
        {
          /* Select the rounder mode. */
          GObject *rounder_obj = gtk_builder_get_object (bar_gtk_builder,
                                                         "rounder");

          gtk_tool_button_set_icon_widget (toolbutton,
			                   GTK_WIDGET (rounder_obj));

          bar_data->rounder   = TRUE;
          bar_data->rectifier = FALSE;
          replace_status_message (gettext ("Rounder mode selected"));
        }
      else
        {
          /* Select the rectifier mode. */
          GObject *rectifier_obj = gtk_builder_get_object (bar_gtk_builder,
                                                           "rectifier");

          gtk_tool_button_set_icon_widget (toolbutton,
			                   GTK_WIDGET (rectifier_obj));

          bar_data->rectifier = TRUE;
          bar_data->rounder   = FALSE;
          replace_status_message (gettext ("Polygon mode selected"));
        }
    }
  else
    {
      /* Select the free hand writing mode. */
      GObject *hand_obj = gtk_builder_get_object (bar_gtk_builder, "hand");
      gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (hand_obj));
      bar_data->rectifier = FALSE;
      bar_data->rounder   = FALSE;
      replace_status_message (gettext ("Freehand mode selected"));
    }
}

/* Push thickness button. */
G_MODULE_EXPORT void
on_bar_thick_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;

  if (bar_data->thickness == MICRO_THICKNESS)
    {
      replace_status_message (gettext ("Brush thickness set to thin"));
      /* Set the thin icon. */
      GObject *thin_obj = gtk_builder_get_object (bar_gtk_builder, "thin");
      gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (thin_obj));
      bar_data->thickness = THIN_THICKNESS;
    }
  else if (bar_data->thickness == THIN_THICKNESS)
    {
      replace_status_message (gettext ("Brush thickness set to medium"));
      /* Set the medium icon. */
      GObject *medium_obj = gtk_builder_get_object (bar_gtk_builder, "medium");
      gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (medium_obj));
      bar_data->thickness = MEDIUM_THICKNESS;
    }
  else if (bar_data->thickness == MEDIUM_THICKNESS)
    {
      replace_status_message (gettext ("Brush thickness set to thick"));
      /* Set the thick icon. */
      GObject *thick_obj = gtk_builder_get_object (bar_gtk_builder, "thick");
      gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (thick_obj));
      bar_data->thickness = THICK_THICKNESS;
    }
  else if (bar_data->thickness == THICK_THICKNESS)
    {
      replace_status_message (gettext ("Brush thickness set to micro"));
      /* Set the micro icon. */
      GObject *micro_obj = gtk_builder_get_object (bar_gtk_builder, "micro");
      gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (micro_obj));
      bar_data->thickness = MICRO_THICKNESS;
    }
}

/* Push arrow button. */
G_MODULE_EXPORT void
on_bar_arrow_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  set_color (bar_data, bar_data->color);
  replace_status_message (gettext ("Arrow tool selected"));
}

/* Push pencil button. */
G_MODULE_EXPORT void
on_bar_pencil_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  set_color (bar_data, bar_data->color);
  replace_status_message (gettext ("Pencil tool selected"));
}

/* Push highlighter button. */
G_MODULE_EXPORT void
on_bar_highlighter_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  set_color (bar_data, bar_data->color);
  replace_status_message (gettext ("Highlighter tool selected"));
}

/* Push filler button. */
G_MODULE_EXPORT void
on_bar_filler_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  annotate_select_filler ();
  replace_status_message (gettext ("Filler tool selected"));
}

/* Push eraser button. */
G_MODULE_EXPORT void
on_bar_eraser_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  lock (bar_data);
  annotate_select_eraser ();
  replace_status_message (gettext ("Eraser tool selected"));
}

/* Push save (screen-shoot) button. */
G_MODULE_EXPORT void
on_bar_screenshot_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;
  /* Release grab. */
  annotate_release_grab ();
  replace_status_message (gettext ("Taking screenshot"));
  gdk_window_set_cursor (gtk_widget_get_window (get_annotation_window ()),
                         (GdkCursor *) NULL);
  start_save_image_dialog ();
  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/* Add page to pdf. */
G_MODULE_EXPORT void
on_bar_add_pdf_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;

  /* Release grab. */
  annotate_release_grab ();

  replace_status_message (gettext ("Exporting as PDF"));
  add_pdf_page (GTK_WINDOW (get_bar_widget ()));
  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/*
 * Hide state event: this occurs when the show/hide widget event happens
 */
G_MODULE_EXPORT void
on_bar_showhide_activate (GtkToolButton *toolButton, gpointer func_data)
{
  BarData *bar_data              = (BarData *) func_data;
  gboolean annotation_is_visible = bar_data->annotation_is_visible;

  GtkWidget *window = get_annotation_window ();

  if (annotation_is_visible)
    {
      /** currently annotations are visible so icon is the hidden **/
      if (window != NULL)
        {
          replace_status_message (gettext ("Annotations hidden"));

          /* Save current drawing into a cairo surface */
          if (bar_data->snapshot_surface)
            {
              cairo_surface_destroy (bar_data->snapshot_surface);
              bar_data->snapshot_surface = NULL;
            }

          {
            GdkWindow *gdk_win = gtk_widget_get_window (window);
            gint width  = gdk_window_get_width (gdk_win);
            gint height = gdk_window_get_height (gdk_win);

            bar_data->snapshot_surface =
              cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

            cairo_t *cr = cairo_create (bar_data->snapshot_surface);
            gdk_cairo_set_source_window (cr, gdk_win, 0, 0);
            cairo_paint (cr);
            cairo_destroy (cr);
          }

	  annotate_clear_screen();

          bar_data->annotation_is_visible = FALSE;

          /* Set the stop tool-tip. */
          gtk_tool_item_set_tooltip_text ((GtkToolItem *) toolButton,
                                          gettext ("Show Annotations"));

          /* Put the show icon. */
          GtkImage *icon = get_image_from_builder (gettext ("show"));
          gtk_tool_button_set_icon_widget (toolButton, (GtkWidget *) icon);
        }
    }
  else
    {
      /** currently annotations are hidden so icon is the showing icon **/
      if (window != NULL)
        {
          replace_status_message (gettext ("Annotations visible"));

          /* Restore saved drawing if available */
          if (bar_data->snapshot_surface)
            {
	      cairo_t *annotation_cr;
	      annotation_cr = annotation_data->annotation_cairo_context;

              cairo_set_source_surface (annotation_cr,
                                        bar_data->snapshot_surface,
                                        0, 0);
              cairo_paint (annotation_cr);
	      gtk_widget_queue_draw (annotation_window);
            }

          bar_data->annotation_is_visible = TRUE;

          /* Set the stop tool-tip. */
          gtk_tool_item_set_tooltip_text ((GtkToolItem *) toolButton,
                                          gettext ("Hide Annotations"));

          /* Put the hide icon. */
          GtkImage *icon = get_image_from_builder (gettext ("hide"));
          gtk_tool_button_set_icon_widget (toolButton, (GtkWidget *) icon);

        }
    }
}

/* Push recorder button. */
G_MODULE_EXPORT void
on_bar_recorder_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  GError *error = (GError *) NULL;
  // we want to show the recording studio window at this point
  if (annotation_data->recordingstudio_window == NULL)
    {
      g_debug ("Showing recording menu");
      if (annotation_data->recordingstudio_options == NULL)
        {
          annotation_data->recordingstudio_options =
	    g_malloc ((gsize) sizeof (RecordingStudioData));
        }

      // create new window  /* Initialize the main window. */
      annotation_data->recordingstudio_window_gtk_builder = gtk_builder_new ();

      GtkBuilder *recording_studio_gtk_builder;

      recording_studio_gtk_builder =
        annotation_data->recordingstudio_window_gtk_builder;

      gtk_builder_add_from_file (recording_studio_gtk_builder,
                                 RECORDINGSTUDIO_UI_FILE,
				 &error);

      if (error)
        {
          g_debug ("Failed to load builder file: %s", error->message);
          g_error_free (error);
          return;
        }

      GObject   *recordingstudio_obj;
      GtkWidget *recordingstudio_window;

      recordingstudio_obj =
        gtk_builder_get_object (recording_studio_gtk_builder,
			        "recordingstudio_window");

      recordingstudio_window = GTK_WIDGET (recordingstudio_obj);
      annotation_data->recordingstudio_window = recordingstudio_window;

      gtk_window_set_transient_for (GTK_WINDOW (recordingstudio_window),
		                    GTK_WINDOW (get_bar_widget ()));

      if (annotation_data->recordingstudio_window == NULL)
        {
          g_debug ("Failed to create the recording studio window");
          return;
        }
      gtk_builder_connect_signals (recording_studio_gtk_builder,
                                   (gpointer) annotation_data);

      gtk_widget_show (annotation_data->recordingstudio_window);
    }
  else
    {
      // just show the window again
      gtk_widget_show (annotation_data->recordingstudio_window);
    }
}

gboolean
on_remove_background_button (GtkMenuItem *menuitem, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gint width = gtk_widget_get_allocated_width (widget);
  GObject *obj = G_OBJECT (widget);
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
  cwidth = gtk_widget_get_allocated_width (background_selection_window);
  cheight = gtk_widget_get_allocated_height (background_selection_window);
  
  gtk_window_resize (GTK_WINDOW (annotation_data->background_selection_window),
                     cwidth - width,
                     cheight);
  return TRUE;
}

void
background_selection_on_toggled (GtkToggleToolButton *toggle_tool_button,
		                 gpointer userdata)
{
  BackgroundButtonData *button_data = (BackgroundButtonData *) userdata;
  annotation_data->background_button_last_selected = button_data->index;

  GSList *node;
  BackgroundButtonData *data;
  node = g_slist_nth (annotation_data->background_button_data,
		      button_data->index);
  data = (BackgroundButtonData *) node->data;

  if (data->mode == BACKGROUND_MODE_COLOR)
    {
      update_background_color (data->color);
    }
  else if (data->mode == BACKGROUND_MODE_FILE)
    {
      update_background_image (data->filename);
    }
  else
    {
      clear_background_context ();

      annotation_data->background_button_last_selected =
	BACKGROUND_NONE_SELECTED;

    }
}

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

gboolean
on_add_new_background (GtkWidget *widget, gpointer user_data)
{
  start_preference_dialog (GTK_WINDOW (widget));
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
  GtkToolButton *tool_button;
  GtkWidget     *icon_widget;
  GtkImage      *image;
  
  tool_button = GTK_TOOL_BUTTON (data->button);
  icon_widget = gtk_tool_button_get_icon_widget (tool_button);
  image = GTK_IMAGE (icon_widget);

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

  cairo_surface_destroy (surface);
  cairo_destroy (cr);
}


gboolean
on_background_selection_window_configure_event (GtkWidget *widget,
                                                GdkEvent *event,
                                                gpointer user_data)
{
  if (widget == annotation_data->background_selection_window &&
      event->type == GDK_CONFIGURE)
    {
      gint elements = g_slist_length (annotation_data->background_button_data);
      GtkWidget *window = GTK_WIDGET(annotation_data->background_selection_window);
      gint h = gtk_widget_get_allocated_height(window);
      gint m = (int) log2 ((double) h);
      m      = (int) pow (2.0, m);

      for (gint ii = 0; ii < elements; ii++)
        {
          GSList *node;
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

void
add_background_button (gchar *label, gint mode, gchar *filename, gchar *color)
{
  GtkToolItem *button = NULL;
  
  if (annotation_data->background_button_data == NULL)
    {
      button = gtk_radio_tool_button_new (NULL);
    }
  else
    {
      GSList *last_node;
      BackgroundButtonData *data;
      last_node = g_slist_last (annotation_data->background_button_data);
      data = (BackgroundButtonData *) last_node->data;

      GtkRadioToolButton *radio_button;
      radio_button = GTK_RADIO_TOOL_BUTTON (data->button);
      button = gtk_radio_tool_button_new_from_widget (radio_button);
    }
  g_object_set_data_full (G_OBJECT (button),
                        "background-label",      /* key */
                        g_strdup (label),        /* value */
                        g_free);                 /* destroy notify */
  if (label == NULL)
    {
      label = "No Label";
    }
  if (g_slist_length (annotation_data->background_button_data) == 0)
    {
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (button),
                                         TRUE);
    }
  else
    {
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (button),
                                         FALSE);
    }

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
  data->index = g_slist_length (annotation_data->background_button_data);
  data->size  = 32;
  data->button               = button;
  
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

  if (g_slist_length (annotation_data->background_button_data) > 3)
    {
      gtk_widget_show_all (annotation_data->background_selection_window);
    }
}

void
on_background_selection_window_destroy (GtkWidget *object, gpointer user_data)
{
  annotation_data->background_selection_window    = NULL;
  annotation_data->background_selection_container = NULL;
  g_slist_free (annotation_data->background_button_data);
  annotation_data->background_button_data          = NULL;
  annotation_data->background_button_last_selected = BACKGROUND_NONE_SELECTED;
}

void
on_background_selection_size_allocate (GtkWidget *widget,
		                       GdkRectangle *allocation,
                                       gpointer user_data)
{
  // g_printf("size allocate %d %d\n", allocation->width, allocation->height);
}

/* Load colors and images from config file. */
static void
load_backgrounds_from_config ()
{
  gsize n_colors = 0;
  gchar **color_keys = background_config_get_color_keys (&n_colors);

  if (color_keys)
    {
      for (gsize i = 0; i < n_colors; i++)
        {
          gchar *hex = background_config_get_color (color_keys[i]);
          if (hex)
            {
              g_debug ("Load background color %s in preference", hex);
              add_background_button (color_keys[i],
                                     BACKGROUND_MODE_COLOR,
                                     NULL,
                                     g_strdup(hex));
            }
        }
      g_strfreev (color_keys);
    }


  gsize n_images = 0;
  gchar **image_keys = background_config_get_image_keys (&n_images);
  
  if (image_keys)
    {
      for (gsize i = 0; i < n_images; i++)
        {
          gchar *path = background_config_get_image (image_keys[i]);
          if (path)
            {
              g_debug ("Load background image %s in preference", path);
              add_background_button (image_keys[i],
                                     BACKGROUND_MODE_FILE,
                                     g_strdup (path),
                                     NULL);
              g_free(path);
            }
        }
      g_strfreev (image_keys);
    }
}

void create_bar_preference_window (GtkWindow *parent)
{
  GtkWidget   *window = NULL;
  GtkToolItem *button = NULL;
  GtkBox      *box    = NULL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), gettext ("Backgrounds"));
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 128);

  box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (box));

  annotation_data->background_selection_window    = window;
  annotation_data->background_selection_container = GTK_WIDGET (box);

  add_background_button ("transparent",
                         BACKGROUND_MODE_NONE,
                         TRANSPARENT_BACKGROUND_FILE,
                         NULL);

  load_backgrounds_from_config ();

  GtkWidget *add_image =
    gtk_image_new_from_icon_name ("list-add",
                                  GTK_ICON_SIZE_LARGE_TOOLBAR);

  button = gtk_tool_button_new (add_image, NULL);

  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (button),
                                  gettext ("Add background"));

  gtk_box_pack_start (box, GTK_WIDGET (button), TRUE, TRUE, 0);

  /* 
   * Store a pointer to Add on the container so new backgrounds
   * can be inserted right before it.
   */
  g_object_set_data (G_OBJECT (box),
		     "background_add_button",
		     GTK_WIDGET (button));

  gtk_window_set_transient_for (GTK_WINDOW (window), parent);

  g_signal_connect (button,
		    "clicked",
		    (GCallback) on_add_new_background,
		    window);

  g_signal_connect (window,
                    "destroy",
                    (GCallback) on_background_selection_window_destroy,
                    NULL);

  g_signal_connect (window,
                    "configure-event",
                    (GCallback) on_background_selection_window_configure_event,
                    NULL);

  g_signal_connect (window,
                    "size-allocate",
                    (GCallback) on_background_selection_size_allocate,
                    NULL);

  gtk_widget_show_all (window);
}

/* Push fonts button. */
G_MODULE_EXPORT void
on_bar_fonts_clicked (GtkToolButton *toolbutton, gpointer func_data)
{
  create_font_selector_window (GTK_WINDOW (get_bar_widget ()));
  show_font_selector_window ();
}

/* Push preference button. */
G_MODULE_EXPORT void
on_bar_preferences_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  if (annotation_data->background_selection_window != NULL)
    {
      gtk_widget_show_all (annotation_data->background_selection_window);
    }
  else
    {
      create_bar_preference_window (GTK_WINDOW (get_bar_widget ()));
    }
}

/* Push undo button. */
G_MODULE_EXPORT void
on_bar_undo_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  annotate_undo ();
}

/* Push redo button. */
G_MODULE_EXPORT void
on_bar_redo_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  annotate_redo ();
}

/* Push clear button. */
G_MODULE_EXPORT void
on_bar_clear_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  replace_status_message (gettext ("Screen has been cleared"));
  annotate_clear_screen ();
  annotate_add_savepoint ();
}

/* Push color selector button. */
G_MODULE_EXPORT void
on_bar_color_activate (GtkToggleToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data   = (BarData *) func_data;
  gboolean grab_value = bar_data->grab;
  gchar   *new_color  = "";

  if (! gtk_toggle_tool_button_get_active (toolbutton))
    {
      return;
    }

  /* Release grab. */
  annotate_release_grab ();

  bar_data->grab = FALSE;
  gdk_window_set_cursor (gtk_widget_get_window (get_annotation_window ()),
                         (GdkCursor *) NULL);
  new_color = start_color_selector_dialog (GTK_TOOL_BUTTON (toolbutton),
                                           GTK_WINDOW (get_bar_widget ()),
                                           bar_data->color);

  if (new_color) // if it is a valid color
    {
      set_color (bar_data, new_color);
      g_free (new_color);
    }

  bar_data->grab = grab_value;
  start_tool (bar_data);
}

/* Push blue color button. */
G_MODULE_EXPORT void
on_bar_blue_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, BLUE);
}

/* Push red color button. */
G_MODULE_EXPORT void
on_bar_red_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, RED);
}

/* Push green color button. */
G_MODULE_EXPORT void
on_bar_green_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, GREEN);
}

/* Push yellow color button. */
G_MODULE_EXPORT void
on_bar_yellow_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, YELLOW);
}

/* Push white color button. */
G_MODULE_EXPORT void
on_bar_white_activate (GtkToolButton *toolbutton, gpointer func_data)
{
  BarData *bar_data = (BarData *) func_data;
  set_color (bar_data, WHITE);
}

