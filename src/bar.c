
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
#include "text_input.h"
#include "utils.h"

/* Timer used to up-rise the window. */
static gint timer = -1;

BarData *bar_data = NULL;

/**
 * calculate_position:
 * @ardesia_bar_window: The window whose position is being calculated.
 * @d_width:            The width of the display or monitor.
 * @d_height:           The height of the display or monitor.
 * @x:                  (out): Return location for the calculated X coordinate.
 * @y:                  (out): Return location for the calculated Y coordinate.
 * @w_width:            The width of the window.
 * @w_height:           The height of the window.
 * @position:           The desired position (e.g., NORTH, SOUTH, EAST, WEST).
 *
 * Calculates the top-left (x, y) coordinates for the toolbar window to
 * center it vertically or horizontally on a display.
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

/**
 * calculate_initial_position:
 * @ardesia_bar_window: The window to be positioned.
 * @x:                  (out): Return location for the final X coordinate.
 * @y:                  (out): Return location for the final Y coordinate.
 * @w_width:            The width of the window.
 * @w_height:           The height of the window.
 * @rect:               The geometry of the target monitor.
 * @position:           The desired logical position (e.g., NORTH, SOUTH).
 *
 * Calculates the initial position for the toolbar window, resizing it if
 * necessary to fit within the target monitor's geometry.
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

/**
 * activate_tool_button:
 * @tool_button_name: The ID of the #GtkToggleToolButton in the GtkBuilder file.
 *
 * Programmatically activates a toggle tool button in the toolbar.
 *
 * This function finds a button by its name in the global `bar_gtk_builder`
 * and sets its state to active. It is used to synchronize the UI with the
 * application's internal state.
 */
void
activate_tool_button (gchar *tool_button_name)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder, tool_button_name);
  GtkToggleToolButton *tool_button = GTK_TOGGLE_TOOL_BUTTON (obj);
  gtk_toggle_tool_button_set_active (tool_button, TRUE);
}

/**
 * init_bar_data:
 *
 * Allocates and initializes a new #BarData structure with default values.
 *
 * Returns: (transfer full): A pointer to the newly allocated and initialized
 * #BarData. The caller is responsible for freeing this memory.
 */
static BarData *
init_bar_data (void)
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
  bar_data->color                       = NULL;
  activate_tool_button ("buttonHighlighter");
  activate_tool_button ("buttonYellow");
  return bar_data;
}

/**
 * update_color_in_bar:
 * @rgba_color: (nullable): An 8-character RGBA hex string (e.g.,"FF0000FF").
 *
 * Activates the corresponding color preset button in the toolbar that
 * matches the given color.
 *
 * This function compares the opaque version of the @rgba_color against
 * a set of predefined color constants (WHITE, RED, etc.). If a match is
 * found, the corresponding radio tool button is activated. If no match
 * is found or if @rgba_color is %NULL, the generic "Color Chooser"
 * button is activated.
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
  gchar *opaque_color = g_strdup_printf ("%.6sFF", rgba_color);
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

/**
 * set_thickness:
 * @thickness: The new thickness value to set.
 *
 * Updates the thickness value in the global `bar_data` struct.
 */
void
set_thickness (gint thickness)
{
  bar_data->thickness = thickness;
}

/**
 * select_thickness:
 * @toolbutton: The #GtkToolButton whose icon will be updated.
 * @thickness:  The string ID of the widget (e.g., a #GtkImage) to use
 * as the new icon.
 *
 * Updates the icon of a tool button to visually represent a selected
 * thickness.
 *
 * This function retrieves a widget, identified by the @thickness string
 * ID, from the global `bar_gtk_builder` and sets it as the new icon for
 * the @toolbutton.
 */
void
select_thickness (GtkToolButton *toolbutton, gchar *thickness)
{
  GObject *thickness_obj = gtk_builder_get_object (bar_gtk_builder, thickness);
  gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (thickness_obj));
}

/**
 * update_thickness_in_bar:
 * @thickness: A string identifier for the thickness icon (e.g., "thin").
 *
 * Updates the main thickness button's icon on the toolbar to reflect
 * the current thickness.
 */
void
update_thickness_in_bar (gchar *thickness)
{
  GObject       *obj = gtk_builder_get_object (bar_gtk_builder, "buttonThick");
  GtkToolButton *tool_button = GTK_TOOL_BUTTON (obj);
  select_thickness (tool_button, thickness);
}

/**
 * set_icon:
 * @toolbutton: The #GtkToolButton to modify.
 * @icon_id:    The ID of the #GtkImage widget in the GtkBuilder file.
 *
 * A helper function that sets the icon of a tool button by finding a
 * GtkImage widget by its ID in the global `bar_gtk_builder`.
 */
void
set_icon (GtkToolButton *toolbutton, gchar *icon_id)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder, icon_id);

  gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (obj));
}

/*
 * Sets the 'rounder' icon on a GtkToolButton.
 */
void
set_rounder_icon (GtkToolButton *toolbutton)
{
  set_icon (toolbutton, "rounder");
}

/**
 * set_rectifier_icon:
 * @toolbutton: The #GtkToolButton whose icon will be set.
 *
 * Convenience function to set the "rectifier" icon on a tool button.
 */
void
set_rectifier_icon (GtkToolButton *toolbutton)
{
  set_icon (toolbutton, "rectifier");
}

/**
 * set_hand_icon:
 * @toolbutton: The #GtkToolButton whose icon will be set.
 *
 * Convenience function to set the "hand" (freehand) icon on a tool button.
 */
void
set_hand_icon (GtkToolButton *toolbutton)
{
  set_icon (toolbutton, "hand");
}

/**
 * setup_bar_mode:
 * @toolbutton: The #GtkToolButton that was clicked, whose icon will be updated.
 * @bar_data: (inout): The #BarData struct containing the current drawing mode
 * state, which will be modified.
 *
 * Cycles through the three main drawing modes: Freehand, Rounder, and
 * Rectifier (Polygon).
 *
 * This function is intended to be a callback for a single button. Each
 * time it is called, it transitions to the next mode in the cycle:
 * Freehand -> Rounder -> Rectifier -> Freehand.
 *
 * It updates the `rectifier` and `rounder` flags in the @bar_data
 * struct and changes the icon of the @toolbutton to reflect the new
 * active mode.
 */
void
setup_bar_mode (GtkToolButton *toolbutton, BarData *bar_data)
{
  if (! bar_data->rectifier)
    {
      if (! bar_data->rounder)
        {
          /* Select the rounder mode. */
          set_rounder_icon (toolbutton);

          bar_data->rounder   = TRUE;
          bar_data->rectifier = FALSE;
          g_debug ("Rounder mode selected");
        }
      else
        {
          /* Select the rectifier mode. */
          set_rectifier_icon (toolbutton);
          bar_data->rectifier = TRUE;
          bar_data->rounder   = FALSE;
          g_debug ("Polygon mode selected");
        }
    }
  else
    {
      /* Select the free hand writing mode. */
      set_hand_icon (toolbutton);
      bar_data->rectifier = FALSE;
      bar_data->rounder   = FALSE;
      g_debug ("Freehand mode selected");
    }
}

/**
 * update_modifiers_in_bar:
 * @bar_data: (in): The #BarData struct containing the current modifier flags.
 *
 * Updates the icon on the "buttonMode" GtkToolButton according to the
 * modifier flags (`rectifier` and `rounder`) in @bar_data. If the
 * `rectifier` flag is set, the button shows the rectifier icon; if the
 * `rounder` flag is set, the button shows the rounder icon.
 */
static void
update_modifiers_in_bar (BarData *bar_data)
{
  GObject       *obj = gtk_builder_get_object (bar_gtk_builder, "buttonMode");
  GtkToolButton *tool_button = GTK_TOOL_BUTTON (obj);
  if (bar_data->rectifier)
    {
      set_rectifier_icon (tool_button);
    }
  if (bar_data->rounder)
    {
      set_rounder_icon (tool_button);
    }
}

/**
 * set_modifiers:
 * @bar_data: (out): The #BarData struct to update.
 * @data:     (in): The #AnnotateData struct to read from.
 *
 * Synchronizes the modifier flags (`rectifier`, `rounder`) from the main
 * #AnnotateData struct to the #BarData struct.
 */
static void
set_modifiers (BarData *bar_data, AnnotateData *data)
{
  if (data->rectify)
    {
      bar_data->rectifier = TRUE;
      bar_data->rounder   = FALSE;
    }
  if (data->roundify)
    {
      bar_data->rounder   = TRUE;
      bar_data->rectifier = FALSE;
    }
}

/**
 * activate_tools:
 * @data: The #AnnotateData struct containing the current tool state.
 *
 * Synchronizes the toolbar's UI by activating the tool buttons that
 * correspond to the state stored in the #AnnotateData struct.
 */
void
activate_tools (AnnotateData *data)
{
  switch (data->cur_context->type)
    {
    case ANNOTATE_ERASER:
      activate_tool_button ("buttonEraser");
      break;
    case ANNOTATE_PEN:
      if (is_selected_color_opaque ())
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
      activate_tool_button ("buttonText");
    }
}

/**
 * update_bar_data_state:
 * @bar_data: (out): The #BarData struct to update.
 * @data:     (in): The #AnnotateData struct containing the source state.
 *
 * Populates the #BarData state from the main #AnnotateData state.
 *
 * This function acts as a synchronization point, typically after loading a
 * configuration, to ensure the toolbar's internal state and UI reflect
 * the application's global state.
 */
static void
update_bar_data_state (BarData *bar_data, AnnotateData *data)
{
  activate_tools (data);

  set_thickness (data->thickness);

  gchar *thickness_label =
      annotate_thickness_pixel_to_label (bar_data->thickness);

  update_thickness_in_bar (thickness_label);

  bar_data->color = g_strdup (data->color);
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
  if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
      return file;
    }

  free (file);

  for (dir = system_dirs; *dir; ++dir)
    {
      file = g_build_filename (*dir, name, NULL);
      if (g_file_test (file, G_FILE_TEST_EXISTS))
        {
          return file;
        }
      free (file);
    }
  return NULL;
}

/**
 * create_bar_window:
 * @commandline: Command line options that influence the bar's layout,
 * decoration, and position.
 * @rect: The geometry of the monitor on which the bar will be placed.
 * @parent: (nullable): The main application window, to be set as the
 * transient parent for the bar window.
 *
 * Creates and initializes the main Ardesia toolbar window.
 *
 * This function handles several setup tasks:
 * - Loads custom user styles from `ardesia/gtk.css` if it exists.
 * - Selects the appropriate GtkBuilder UI file (horizontal or vertical)
 * based on the requested position and screen resolution.
 * - Loads the UI, connects all signal handlers, and synchronizes the UI state.
 * - Calculates the bar's initial on-screen position and moves the
 * window into place.
 *
 * Returns: (transfer none): A pointer to the newly created toolbar #GtkWidget,
 * or %NULL on failure.
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

      GdkDisplay *display = gdk_display_get_default ();
      GdkScreen  *screen  = gdk_display_get_default_screen (display);

      gtk_style_context_add_provider_for_screen (
          screen,
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

  bar_data = init_bar_data ();
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

  return bar_window;
}

/**
 * is_toggle_tool_button_active:
 * @toggle_tool_button_name: The ID of the #GtkToggleToolButton in the UI file.
 *
 * A generic helper function to check if a named toggle tool button is active.
 *
 * Returns: %TRUE if the button is active, %FALSE otherwise.
 */
gboolean
is_toggle_tool_button_active (gchar *toggle_tool_button_name)
{
  GObject *g_object = gtk_builder_get_object (bar_gtk_builder,
                                              toggle_tool_button_name);

  GtkToggleToolButton *toggle_tool_button = GTK_TOGGLE_TOOL_BUTTON (g_object);
  return gtk_toggle_tool_button_get_active (toggle_tool_button);
}

/**
 * get_image_from_builder:
 * @image_name: The string ID of the #GtkImage widget in the UI
 * definition file.
 *
 * Retrieves a pointer to a #GtkImage widget from the global
 * `bar_gtk_builder`.
 *
 * Returns: (transfer none) (nullable): A pointer to the #GtkImage
 * widget, or %NULL if an object with the given ID is not found. The
 * returned widget is owned by the builder and must not be unreferenced.
 */
GtkImage *
get_image_from_builder (gchar *image_name)
{
  GObject  *g_object = gtk_builder_get_object (bar_gtk_builder, image_name);
  GtkImage *image    = GTK_IMAGE (g_object);
  return image;
}

/**
 * is_text_toggle_tool_button_active:
 *
 * Checks if the text tool toggle button is currently active.
 *
 * Returns: %TRUE if the text tool button is active, %FALSE otherwise.
 */
gboolean
is_text_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonText");
}

/**
 * is_highlighter_toggle_tool_button_active:
 *
 * Checks if the highlighter tool toggle button is currently active.
 *
 * Returns: %TRUE if the highlighter tool button is active, %FALSE otherwise.
 */
gboolean
is_highlighter_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonHighlighter");
}

/**
 * is_filler_toggle_tool_button_active:
 *
 * Checks if the filler tool toggle button is currently active.
 *
 * Returns: %TRUE if the filler tool button is active, %FALSE otherwise.
 */
gboolean
is_filler_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonFiller");
}

/**
 * is_eraser_toggle_tool_button_active:
 *
 * Checks if the eraser tool toggle button is currently active.
 *
 * Returns: %TRUE if the eraser tool button is active, %FALSE otherwise.
 **/
gboolean
is_eraser_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonEraser");
}

/**
 * is_pen_toggle_tool_button_active:
 *
 * Checks if the pen tool toggle button is currently active.
 *
 * Returns: %TRUE if the pen tool button is active, %FALSE otherwise.
 */
gboolean
is_pen_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonPencil");
}

/**
 * is_pointer_toggle_tool_button_active:
 *
 * Checks if the pointer tool toggle button is currently active.
 *
 * Returns: %TRUE if the pointer tool button is active, %FALSE otherwise.
 */
gboolean
is_pointer_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonPointer");
}

/**
 * is_arrow_toggle_tool_button_active:
 *
 * Checks if the arrow tool toggle button is currently active.
 *
 * Returns: %TRUE if the arrow tool button is active, %FALSE otherwise.
 */
gboolean
is_arrow_toggle_tool_button_active (void)
{
  return is_toggle_tool_button_active ("buttonArrow");
}

/**
 * add_alpha:
 * @bar_data: (inout): The #BarData struct containing the color string
 *                     to modify.
 *
 * Modifies the alpha component of the color string in @bar_data based on
 * the currently active tool.
 *
 * This function assumes the color is an 8-character hexadecimal RGBA string.
 * It sets the alpha to a semi-opaque value for the highlighter and a fully
 * opaque value for the pen.
 */
void
add_alpha (BarData *bar_data)
{
  if (is_highlighter_toggle_tool_button_active ())
    {
      memcpy (&bar_data->color[6], SEMI_OPAQUE_ALPHA, 2);
    }
  else if (is_pen_toggle_tool_button_active ())
    {
      memcpy (&bar_data->color[6], OPAQUE_ALPHA, 2);
    }
}

/**
 * take_pen_tool:
 *
 * Programmatically activates a drawing tool (pen or highlighter) on the
 * toolbar.
 *
 * This function is a helper to ensure a drawing tool is selected,
 * deactivating other tools like the eraser or pointer. It has special
 * logic to select the highlighter if the current color is semi-transparent,
 * otherwise it defaults to the pen.
 */
void
take_pen_tool (void)
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
      gchar *alpha_str   = annotation_data->color + 6;
      gint   alpha_value = 0;
      sscanf (alpha_str, "%02X", &alpha_value);

      if (alpha_value <= atoi (SEMI_OPAQUE_ALPHA))
        {
          pencil_obj = gtk_builder_get_object (bar_gtk_builder,
                                               "buttonHighlighter");

          pencil_tool_button = GTK_TOGGLE_TOOL_BUTTON (pencil_obj);
        }

      GObject *filler_obj = gtk_builder_get_object (bar_gtk_builder,
                                                    "buttonFiller");

      GtkToggleToolButton *filler_tool_button;
      filler_tool_button = GTK_TOGGLE_TOOL_BUTTON (filler_obj);

      gtk_toggle_tool_button_set_active (filler_tool_button, FALSE);
      gtk_toggle_tool_button_set_active (pencil_tool_button, TRUE);
    }
}

/**
 * release_lock:
 * @bar_data: (inout): The #BarData struct containing the grab state.
 *
 * Releases the pointer grab from the annotation window, allowing mouse
 * events to pass through to the desktop and other applications.
 *
 * This function is called to exit the drawing mode. It updates the `grab`
 * flag and triggers a redraw. It includes platform-specific workarounds
 * to achieve input transparency.
 */
void
release_lock (BarData *bar_data)
{
  g_debug ("releasing lock (grab: %d)\n", bar_data->grab);
  if (bar_data->grab)
    {
      /* Lock enabled. */
      bar_data->grab = FALSE;
      annotate_release_grab ();

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
    }
}

/**
 * lock:
 * @bar_data: (inout): The #BarData struct containing the grab state.
 *
 * Prepares the application to enter a pointer-grabbed ("locked") state
 * by updating internal state flags.
 *
 * Note that this function only updates the `grab` flag and does not
 * perform the actual GDK pointer grab itself.
 */
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

/**
 * set_color:
 * @bar_data:       The #BarData struct to update.
 * @selected_color: (transfer none): The new color string (e.g., "FF0000FF").
 *
 * Sets a new color, updating both the toolbar state and the global
 * annotation state.
 *
 * This function is called when a user selects a new color and acts as a
 * high-level coordinator. It ensures a drawing tool is active, updates
 * the color in #BarData (by making a copy), synchronizes the global
 * `annotation_data` color, and adjusts the alpha component.
 *
 * Memory Ownership: This function makes internal copies of the provided
 * color string. The caller retains full ownership of the original
 * @selected_color pointer and is responsible for its memory management.
 */
void
set_color (BarData *bar_data, gchar *selected_color)
{
  if (selected_color == NULL)
    {
      g_warning ("Attempting to set NULL color");
      return;
    }

  take_pen_tool ();
  lock (bar_data);
  if (bar_data->color != NULL)
    {
      g_free (bar_data->color);
      bar_data->color = NULL;
    }
  bar_data->color       = g_strdup (selected_color);
  gchar *annotate_color = g_strdup (selected_color);
  annotate_set_color (selected_color);
  g_free (annotate_color);
  add_alpha (bar_data);
}

/**
 * set_options:
 * @bar_data: The #BarData struct containing the current tool options.
 *
 * Synchronizes the state of the annotation engine with the current
 * options selected in the toolbar.
 *
 * This function reads settings (thickness, rectifier, etc.) from the
 * @bar_data struct and applies them to the main annotation module. It also
 * selects the appropriate drawing tool (pen or eraser) based on which
 * toolbar button is currently active.
 */
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
      gchar *color = g_strdup (bar_data->color);
      annotate_set_color (color);
      g_free (color);
      annotate_select_pen ();
    }
  else if (is_eraser_toggle_tool_button_active ())
    {
      annotate_select_eraser ();
    }
}

/**
 * start_tool:
 * @bar_data: The #BarData struct containing the current application state.
 *
 * Activates the currently selected tool in the annotation engine.
 *
 * This function is called to apply the currently selected tool settings from
 * the toolbar to the main annotation engine. If the text tool is active, it

 * initializes the text editing widget; otherwise, it applies the current
 * drawing options by calling set_options().
 */
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
