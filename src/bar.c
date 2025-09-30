
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

#include "bar.h"
#include "annotation_config.h"
#include "annotation_window.h"
#include "background_window.h"
#include "commandline.h"
#include "text_window.h"
#include "utils.h"

/* Timer used to up-rise the window. */
static gint timer = -1;

BarData *bar_data = NULL;

/*
 * Calculate the better position where put the bar.
 */
static void
calculate_position (GtkWidget *ardesia_bar_window,
		    gint d_width,
		    gint d_height,
                    gint *x,
		    gint *y,
		    gint w_width,
		    gint w_height,
		    gint position)
{
  *y = ((d_height - w_height - SPACE_FROM_BORDER) / 2);
  /* Vertical layout. */
  if (position == WEST)
    {
      *x = 0;
    }
  else if (position == EAST)
    {
      *x = d_width - w_width;
    }
  else
    {
      /* Horizontal layout. */
      *x = (d_width - w_width) / 2;
      if (position == NORTH)
        {
          /* Assuming that the bar is in south. */
          *y = 0; // on north SPACE_FROM_BORDER;
        }
      else if (position == SOUTH)
        {
          /* South. assuming bar is on south. */
          *y = d_height - SPACE_FROM_BORDER - w_height;
        }
      else
        {
          /* Invalid position. */
          perror ("Valid positions are NORTH, SOUTH, WEST or EAST\n");
          exit (EXIT_FAILURE);
        }
    }
}

/*
 * Calculate the initial position.
 */
static void
calculate_initial_position (GtkWidget *ardesia_bar_window,
		            gint *x,
			    gint *y,
			    gint w_width,
                            gint w_height,
			    GdkRectangle *rect,
			    gint position)
{
  gint d_width  = rect->width;
  gint d_height = rect->height;
  /* Resize if larger that screen width. */
  if (w_width > d_width)
    {
      w_width = d_width;
      gtk_window_resize (GTK_WINDOW (ardesia_bar_window), w_width, w_height);
    }

  /* Resize if larger that screen height. */
  if (w_height > d_height)
    {
      gint tollerance = 15;
      w_height        = d_height - tollerance;
      gtk_widget_set_size_request (ardesia_bar_window, w_width, w_height);
    }

  calculate_position (ardesia_bar_window, d_width, d_height,
		      x, y, w_width, w_height, position);
}

/* Activate tool button by name. */
void
activate_tool_button (gchar *tool_button_name)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder, tool_button_name);
  GtkToggleToolButton *tool_button = GTK_TOGGLE_TOOL_BUTTON (obj);
  gtk_toggle_tool_button_set_active (tool_button, TRUE);
}

/* Get status bar. */
GtkStatusbar *
get_statusbar ()
{
  GObject *g_object = gtk_builder_get_object (bar_gtk_builder,
                                              gettext ("statusbar"));
  return GTK_STATUSBAR (g_object);
}

/* Replace staus message of status bar. */
void
replace_status_message (gchar *message)
{
  GtkStatusbar *bar = get_statusbar ();
  if (bar != NULL)
    {
      gtk_statusbar_pop (bar, 0);
      gtk_statusbar_push (bar, 0, message);
    }
}

/* Allocate and initialize the bar data structure. */
static BarData *
init_bar_data ()
{
  BarData *bar_data = (BarData *) g_malloc ((gsize) sizeof (BarData));
  bar_data->annotation_is_visible       = TRUE;
  bar_data->grab                        = TRUE;
  bar_data->rectifier                   = FALSE;
  bar_data->rounder                     = FALSE;
  bar_data->screenshot_pending          = FALSE;
  bar_data->screenshot_callback         = NULL;
  bar_data->screenshot_saved_location_x = -1;
  bar_data->screenshot_saved_location_y = -1;
  bar_data->snapshot_surface            = NULL;
  /* default to yellow highlighter */
  activate_tool_button ("buttonHighlighter");
  activate_tool_button ("buttonYellow");
  set_color (bar_data, "FFFF0088");
  return bar_data;
}

/* 
 * Map a color string to the corresponding toolbar button and activate it.
 * If the color does not match any predefined color, the "buttonColor"
 * (custom color) button is activated.
 */
void
update_color_in_bar (const gchar *rgba_color)
{
  if (rgba_color == NULL)
    {
      /* Default to custom color button */
      activate_tool_button ("buttonColor");
      return;
    }
  gchar *opaque_color = g_strdup_printf("%.6sFF", rgba_color);
  if (g_ascii_strcasecmp (opaque_color, WHITE) == 0)
    {
      activate_tool_button ("buttonWhite");
    }
  else if (g_ascii_strcasecmp (opaque_color, RED) == 0)
    {
      activate_tool_button ("buttonRed");
    }
  else if (g_ascii_strcasecmp (opaque_color, YELLOW) == 0)
    {
      activate_tool_button ("buttonYellow");
    }
  else if (g_ascii_strcasecmp (opaque_color, GREEN) == 0)
    {
      activate_tool_button ("buttonGreen");
    }
  else if (g_ascii_strcasecmp (opaque_color, BLUE) == 0)
    {
      activate_tool_button ("buttonBlue");
    }
  else
    {
      /* Not a predefined color: activate the custom color button */
      activate_tool_button ("buttonColor");
    }
  g_free (opaque_color);
}

/* Set thickness in bar data. */
void
set_thickness (gint thickness)
{
  bar_data->thickness = thickness;
}

/* Activate tool button by name. */
void
select_thickness (GtkToolButton *toolbutton, gchar *thickness)
{
  GObject *thickness_obj = gtk_builder_get_object (bar_gtk_builder, thickness);
  gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (thickness_obj));
}

/* Update thickness in bar. */
void
update_thickness_in_bar(gchar *thickness)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder, "buttonThick");
  GtkToolButton *tool_button = GTK_TOOL_BUTTON (obj);
  select_thickness (tool_button, thickness);
}

/*
 * Finds a widget by its ID in the GtkBuilder UI definition and sets it
 * as the icon for a GtkToolButton.
 */
void set_icon(GtkToolButton *toolbutton, gchar *icon_id)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder,
                                         icon_id);
  
  gtk_tool_button_set_icon_widget (toolbutton,
                                   GTK_WIDGET (obj));
}

/*
 * Sets the 'rounder' icon on a GtkToolButton.
 */
void set_rounder_icon(GtkToolButton *toolbutton)
{
  set_icon(toolbutton, "rounder");
}

/*
 * Sets the 'rectifier' icon on a GtkToolButton.
 */
void set_rectifier_icon(GtkToolButton *toolbutton)
{
  set_icon(toolbutton, "rectifier");
}

/*
 * Sets the 'hand' icon on a GtkToolButton.
 */
void set_hand_icon(GtkToolButton *toolbutton)
{
  set_icon(toolbutton, "hand");
}

/* Read from bar data and setup modifier on bar */
void
setup_bar_mode(GtkToolButton *toolbutton, BarData *bar_data)
{
  if (! bar_data->rectifier)
    {
      if (! bar_data->rounder)
        {
          /* Select the rounder mode. */
          set_rounder_icon(toolbutton);
  
          bar_data->rounder   = TRUE;
          bar_data->rectifier = FALSE;
          replace_status_message (gettext ("Rounder mode selected"));
        }
      else                       
        {
          /* Select the rectifier mode. */
          set_rectifier_icon(toolbutton);
          bar_data->rectifier = TRUE;
          bar_data->rounder   = FALSE;
          replace_status_message (gettext ("Polygon mode selected"));
        }
    }
  else
    {
      /* Select the free hand writing mode. */
      set_hand_icon(toolbutton);
      bar_data->rectifier = FALSE;
      bar_data->rounder   = FALSE;
      replace_status_message (gettext ("Freehand mode selected"));
    }

}

/*
 * Sets the rounder or rectifier icon on the buttonMode GtkToolButton
 * depending in bar_data settings.
 */
static void
update_modifiers_in_bar (BarData *bar_data)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder, "buttonMode");
  GtkToolButton *tool_button = GTK_TOOL_BUTTON (obj);
  if (bar_data->rectifier)
    {
      set_rectifier_icon(tool_button);
    }
  if (bar_data->rounder)
    {
      set_rounder_icon(tool_button);
    }
}

/*
 * Sets AnnotateData modifier settings compliant with bar_data settings.
 */
static void
set_modifiers (BarData *bar_data, AnnotateData *data)
{
  if (data->rectify)
    {
      bar_data->rectifier = TRUE;
    }
  if (data->roundify)
    {
      bar_data->rounder = TRUE;
    }
}

/* Read annotate data and and activate tools. */
void
activate_tools(AnnotateData *data)
{
  switch (data->cur_context->type)
    {
      case ANNOTATE_ERASER:
        activate_tool_button ("buttonEraser");
        break;
      case ANNOTATE_PEN:
        if (data->is_opaque)
	  {
            activate_tool_button ("buttonPencil");
          }
        else
	  {
            activate_tool_button ("buttonHighlighter");
          }
        break;
      case ANNOTATE_FILLER:
        activate_tool_button ("buttonFiller");
        break;
      case ANNOTATE_POINTER:
        activate_tool_button ("buttonPointer");
        break;
      default:
        break;
    }

  if (data->arrow)
    {
      activate_tool_button ("buttonArrow");
    }

  if (data->text_tool)
    {
      activate_tool_button("buttonText");
    }
}

/* Update bar data from the state of AnnotateData comimg from config file. */
static void
update_bar_data_state (BarData *bar_data, AnnotateData *data)
{
  activate_tools(data);

  set_thickness (data->thickness);
  gchar *thickness_label = annotate_thickness_pixel_to_label(bar_data->thickness);
  update_thickness_in_bar (thickness_label);
  
  bar_data->color = data->color;
  update_color_in_bar (bar_data->color);
  
  set_modifiers (bar_data, data);
  update_modifiers_in_bar (bar_data);
}

/**
 * get_xdg_config_file:
 * @name: The name of the configuration file (e.g., "app-name/settings.conf").
 *
 * Finds a configuration file by searching directories specified by the
 * XDG Base Directory Specification.
 *
 * It prioritizes the user's personal configuration directory
 * (from g_get_user_config_dir()) before searching the system-wide
 * directories (from g_get_system_config_dirs()). The function returns
 * the full path to the first matching file found.
 *
 * Returns: (transfer full): A newly allocated string containing the file path,
 * or %NULL if the file was not found in any of the standard
 * locations. The caller is responsible for freeing the returned
 * string with g_free().
 */
gchar *
get_xdg_config_file (const char *name)
{
  const gchar        *user_dir = g_get_user_config_dir ();
  const gchar *const *system_dirs;
  const gchar *const *dir;
  gchar              *file;

  system_dirs = g_get_system_config_dirs ();
  file        = g_build_filename (user_dir, name, NULL);
  if (g_file_test (file, G_FILE_TEST_EXISTS) == TRUE)
    {
      return file;
    }

  free (file);

  for (dir = system_dirs; *dir; ++dir)
    {
      file = g_build_filename (*dir, name, NULL);
      if (g_file_test (file, G_FILE_TEST_EXISTS) == TRUE)
        {
          return file;
        }
      free (file);
    }
  return NULL;
}

/*
 * Create the ardesia bar window.
 * @rect    the monitor rectangle to place toolbar on
 */
GtkWidget *
create_bar_window (CommandLine *commandline,
		   GdkRectangle *rect,
		   GtkWidget *parent)
{
  GtkWidget *bar_window = (GtkWidget *) NULL;
  bar_data              = (BarData *) NULL;
  GError *error         = (GError *) NULL;
  gchar  *file          = UI_FILE;
  gint    x             = 0;
  gint    y             = 0;
  gint    width         = 0;
  gint    height        = 0;

  /* Set up style for ardesia */
  gchar *gtkcss_file = get_xdg_config_file ("ardesia/gtk.css");
  if (gtkcss_file)
    {
      GtkCssProvider *css = gtk_css_provider_new ();
      gtk_css_provider_load_from_path (css, gtkcss_file, NULL);
      g_free (gtkcss_file);
      gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                 GTK_STYLE_PROVIDER (css),
                                                 GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

  bar_gtk_builder = gtk_builder_new ();

  if (commandline->position > 2)
    {
      /* North or south. */
      file = UI_HOR_FILE;
    }
  else
    {
      /* East or west. */
      int height_in_application_pixels = rect->height;
      if (height_in_application_pixels < 720)
        {
          /*
           * The bar is too long and then I use an horizontal layout;
           * this is done to have the full bar for net book and screen
           * with low vertical resolution.
           */
          file                  = UI_HOR_FILE;
          commandline->position = NORTH;
        }
    }

  /*
   * Load the bar_gtk_builder file with the definition
   * of the ardesia bar gui.
   */
  g_debug ("Bar ui file: %s\n", file);
  gtk_builder_add_from_file (bar_gtk_builder, file, &error);
  if (error)
    {
      g_printerr ("Failed to load builder file: %s", error->message);
      g_error_free (error);
      g_object_unref (bar_gtk_builder);
      bar_gtk_builder = NULL;
      return bar_window;
    }

  bar_data         = init_bar_data ();
  update_bar_data_state (bar_data, annotation_data);

  GObject *bar_obj = gtk_builder_get_object (bar_gtk_builder, BAR_WIDGET_NAME);

  bar_window = GTK_WIDGET (bar_obj);
  gtk_widget_set_name (bar_window, BAR_WIDGET_NAME);
  gtk_window_set_transient_for (GTK_WINDOW (bar_window), GTK_WINDOW (parent));

  /* Connect all the callback from bar_gtk_builder xml file. */
  gtk_builder_connect_signals (bar_gtk_builder, (gpointer) bar_data);

  if (commandline->decorated)
    {
      gtk_window_set_decorated (GTK_WINDOW (bar_window), TRUE);
    }

  gtk_window_get_size (GTK_WINDOW (bar_window), &width, &height);

  /* x and y will be the bar left corner coordinates. */
  calculate_initial_position (bar_window, &x, &y, width, height, rect,
                              commandline->position);

  /*
   * The position is calculated respect the top left corner
   * and then I set the north west gravity.
   */
  gtk_window_set_gravity (GTK_WINDOW (bar_window), GDK_GRAVITY_NORTH_WEST);

  /* Move the window in the desired position. */
  gtk_window_move (GTK_WINDOW (bar_window), rect->x + x, rect->y + y);
  
  start_tool(bar_data);
  return bar_window;
}

void
set_statusbar_label (gchar *message)
{
  GObject *g_object = gtk_builder_get_object (
      bar_gtk_builder, gettext ("labelCurrentSelection"));
  GtkLabel *label = GTK_LABEL (g_object);
  gtk_label_set_label (label, gettext (message));
}

/*
 * Try to up-rise the window;
 * this is used for the window manager
 * that does not support the stay above directive.
 */
gboolean
bar_to_top (gpointer data)
{
  if (! gtk_widget_get_visible (GTK_WIDGET (data)))
    {
      gtk_window_present (GTK_WINDOW (data));
      gtk_widget_grab_focus (data);
      gdk_window_lower (gtk_widget_get_window (GTK_WIDGET (data)));
    }
  return TRUE;
}

/* Is the toggle tool button specified with name is active? */
gboolean
is_toggle_tool_button_active (gchar *toggle_tool_button_name)
{
  GObject *g_object = gtk_builder_get_object (bar_gtk_builder,
		                              toggle_tool_button_name);
  GtkToggleToolButton *toggle_tool_button = GTK_TOGGLE_TOOL_BUTTON (g_object);
  return gtk_toggle_tool_button_get_active (toggle_tool_button);
}

/* Get GtkImage object from builder
 */
GtkImage *
get_image_from_builder (gchar *image_name)
{
  GObject  *g_object = gtk_builder_get_object (bar_gtk_builder, image_name);
  GtkImage *image    = GTK_IMAGE (g_object);
  return image;
}

gboolean
is_text_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonText");
}

/* Is the highlighter toggle tool button active? */

gboolean
is_highlighter_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonHighlighter");
}

/* Is the filler toggle tool button active? */
gboolean
is_filler_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonFiller");
}

/* Is the eraser toggle tool button active? */
gboolean
is_eraser_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonEraser");
}

/* Is the eraser toggle tool button active? */
gboolean
is_pen_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonPencil");
}

/* Is the pointer toggle tool button active? */
gboolean
is_pointer_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonPointer");
}

/* Is the pointer toggle tool button active? */
gboolean
is_arrow_toggle_tool_button_active ()
{
  return is_toggle_tool_button_active ("buttonArrow");
}

/* Add alpha channel to build the RGBA string. */
void
add_alpha (BarData *bar_data)
{
  assert (strlen (bar_data->color) == 8);
  if (is_highlighter_toggle_tool_button_active ())
    {
      memcpy (&bar_data->color[6], SEMI_OPAQUE_ALPHA, 2);
    }
  else if (is_pen_toggle_tool_button_active ())
    {
      memcpy (&bar_data->color[6], OPAQUE_ALPHA, 2);
    }
}

/* Select the pen tool. */
void
take_pen_tool ()
{
  GObject             *pencil_obj = gtk_builder_get_object (bar_gtk_builder,
                                                            "buttonPencil");
  GtkToggleToolButton *pencil_tool_button = GTK_TOGGLE_TOOL_BUTTON (pencil_obj);

  if (is_eraser_toggle_tool_button_active ())
    {
      GObject *eraser_obj = gtk_builder_get_object (bar_gtk_builder,
                                                    "buttonEraser");
      GtkToggleToolButton *eraser_tool_button = NULL;
      eraser_tool_button = GTK_TOGGLE_TOOL_BUTTON (eraser_obj);
      gtk_toggle_tool_button_set_active (eraser_tool_button, FALSE);
      gtk_toggle_tool_button_set_active (pencil_tool_button, TRUE);
    }

  if (is_pointer_toggle_tool_button_active ())
    {
      GObject *pointer_obj = gtk_builder_get_object (bar_gtk_builder,
                                                     "buttonPointer");
      GtkToggleToolButton *pointer_tool_button = NULL;
      pointer_tool_button = GTK_TOGGLE_TOOL_BUTTON (pointer_obj);
      gtk_toggle_tool_button_set_active (pointer_tool_button, FALSE);
      gtk_toggle_tool_button_set_active (pencil_tool_button, TRUE);
    }

  if (is_filler_toggle_tool_button_active ())
    {
      if (strcmp (annotation_data->color + 6, "FF") != 0)
        {
          pencil_obj         = gtk_builder_get_object (bar_gtk_builder,
                                                       "buttonHighlighter");
          pencil_tool_button = GTK_TOGGLE_TOOL_BUTTON (pencil_obj);
        }
      GObject            *filler_obj = gtk_builder_get_object (bar_gtk_builder,
                                                               "buttonFiller");
      GtkToggleToolButton *filler_tool_button = NULL;
      filler_tool_button = GTK_TOGGLE_TOOL_BUTTON (filler_obj);
      gtk_toggle_tool_button_set_active (filler_tool_button, FALSE);
      gtk_toggle_tool_button_set_active (pencil_tool_button, TRUE);
    }
}

/* Release to lock the mouse */
void
release_lock (BarData *bar_data)
{
  g_debug ("releasing lock (grab: %d)\n", bar_data->grab);
  if (bar_data->grab)
    {
      /* Lock enabled. */
      bar_data->grab = FALSE;
      annotate_release_grab ();

      /* Try to up-rise the window. */
      timer = g_timeout_add (BAR_TO_TOP_TIMEOUT,
		             bar_to_top,
			     get_annotation_window ());
#ifdef _WIN32 // WIN32
      if (gtk_window_get_opacity (GTK_WINDOW (get_annotation_window ())) != 0)
        {
          /*
           * @HACK This allow the mouse input go below the window putting
           * the opacity to 0; when will be found a better way to make
           * the window transparent to the the pointer input we might
           * remove the previous hack.
           * @TODO Transparent window to the pointer input in a better way.
           */
          gtk_window_set_opacity (GTK_WINDOW (get_annotation_window ()), 0);
        }
#endif

      gtk_widget_queue_draw (annotation_data->annotation_window);
    }
}

/* Lock the mouse. */
void
lock (BarData *bar_data)
{
  if (! bar_data->grab)
    {
      /* Unlock */
      bar_data->grab = TRUE;

      /* delete the old timer */
      if (timer != -1)
        {
          g_source_remove (timer);
          timer = -1;
        }

#ifdef _WIN32 // WIN32

      /*
       * @HACK Deny the mouse input to go below the window putting
       * the opacity greater than 0.
       * @TODO remove the opacity hack when will be solved the next todo.
       */
      if (gtk_window_get_opacity (GTK_WINDOW (get_background_window ())) == 0)
        {
          gtk_window_set_opacity (GTK_WINDOW (get_background_window ()),
			          BACKGROUND_OPACITY);
        }
#endif
    }
}

/* Set color; this is called each time that the user want change color. */
void
set_color (BarData *bar_data, gchar *selected_color)
{
  take_pen_tool ();
  lock (bar_data);
  bar_data->color = g_strdup_printf ("%s", selected_color);
  annotate_set_color (bar_data->color);
  add_alpha (bar_data);
}

/* Pass the options to the annotation window. */
void
set_options (BarData *bar_data)
{

  annotate_set_rectifier (bar_data->rectifier);

  annotate_set_rounder (bar_data->rounder);

  annotate_set_arrow (is_arrow_toggle_tool_button_active ());

  annotate_set_thickness (bar_data->thickness);

  if (is_pen_toggle_tool_button_active ()         ||
      is_highlighter_toggle_tool_button_active () ||
      is_arrow_toggle_tool_button_active ())
    {
      annotate_set_color (bar_data->color);
      annotate_select_pen ();
    }
  else if (is_eraser_toggle_tool_button_active ())
    {
      annotate_select_eraser ();
    }
}

/* Start to paint with the selected tool. */
void
start_tool (BarData *bar_data)
{
  if (bar_data->grab)
    {
      /* release the old cursor */
      annotate_release_grab ();
      /* grab the pointer again so that button release will respond */
      annotate_acquire_grab ();

      if (is_text_toggle_tool_button_active ())
        {
          /* Text button then start the text widget. */
          start_text_widget (annotation_data->annotation_window,
                             bar_data->color,
			     bar_data->thickness);
        }
      else
        {
          g_debug ("start_tool (non-text tool selected)\n");
          /*
           * This call is required as the leave event for the bar occurs
           * when we enter the toolbar object
           */
          stop_text_widget ();
          /* Is an other tool for paint or erase. */
          set_options (bar_data);
        }
    }
}

gboolean
end_clapperboad_countdown (gpointer user_data)
{
  g_debug ("END on_clapperboard_click");
  gboolean grab_value = bar_data->grab;
  bar_data->grab      = FALSE;
  annotate_release_grab ();

  /*
   * Ideally we want to go back to our background
   * settings that we had before.
   */
  annotation_data->is_clapperboard_visible = FALSE;

  /* Make the screen black and then go back to what it was before. */
  bar_data->grab = grab_value;
  start_tool (bar_data);
  gtk_widget_queue_draw (annotation_data->annotation_window);
  return FALSE;
}

void
begin_clapperboard_countdown ()
{
  timer = g_timeout_add (BAR_TO_TOP_TIMEOUT, end_clapperboad_countdown, NULL);
}
