
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
get_statusbar (void)
{
  GObject *g_object = gtk_builder_get_object (bar_gtk_builder,
                                              gettext ("statusbar"));
  return GTK_STATUSBAR (g_object);
}

/**
 * replace_status_message:
 * @message: the message string to display in the status bar
 *
 * Replaces the current message on the application's status bar with
 * the provided message. If no status bar exists, the function does nothing.
 *
 * This function first removes the top message from the status bar stack
 * and then pushes the new message.
 **/
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

/* Set thickness in bar data. */
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
 **/
void
select_thickness (GtkToolButton *toolbutton, gchar *thickness)
{
  GObject *thickness_obj = gtk_builder_get_object (bar_gtk_builder, thickness);
  gtk_tool_button_set_icon_widget (toolbutton, GTK_WIDGET (thickness_obj));
}

/* Update thickness in bar. */
void
update_thickness_in_bar (gchar *thickness)
{
  GObject       *obj = gtk_builder_get_object (bar_gtk_builder, "buttonThick");
  GtkToolButton *tool_button = GTK_TOOL_BUTTON (obj);
  select_thickness (tool_button, thickness);
}

/*
 * Finds a widget by its ID in the GtkBuilder UI definition and sets it
 * as the icon for a GtkToolButton.
 */
void
set_icon (GtkToolButton *toolbutton, gchar *icon_id)
{
  GObject *obj = gtk_builder_get_object (bar_gtk_builder,
                                         icon_id);

  gtk_tool_button_set_icon_widget (toolbutton,
                                   GTK_WIDGET (obj));
}

/*
 * Sets the 'rounder' icon on a GtkToolButton.
 */
void
set_rounder_icon (GtkToolButton *toolbutton)
{
  set_icon (toolbutton, "rounder");
}

/*
 * Sets the 'rectifier' icon on a GtkToolButton.
 */
void
set_rectifier_icon (GtkToolButton *toolbutton)
{
  set_icon (toolbutton, "rectifier");
}

/*
 * Sets the 'hand' icon on a GtkToolButton.
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
 **/
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
          replace_status_message (gettext ("Rounder mode selected"));
        }
      else
        {
          /* Select the rectifier mode. */
          set_rectifier_icon (toolbutton);
          bar_data->rectifier = TRUE;
          bar_data->rounder   = FALSE;
          replace_status_message (gettext ("Polygon mode selected"));
        }
    }
  else
    {
      /* Select the free hand writing mode. */
      set_hand_icon (toolbutton);
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
activate_tools (AnnotateData *data)
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
      activate_tool_button ("buttonText");
    }
}

/* Update bar data from the state of AnnotateData comimg from config file. */
static void
update_bar_data_state (BarData *bar_data, AnnotateData *data)
{
  activate_tools (data);

  set_thickness (data->thickness);

  gchar *thickness_label =
      annotate_thickness_pixel_to_label (bar_data->thickness);

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
 * based on the requested position and screen resolution, forcing a
 * horizontal layout on low-resolution screens.
 * - Loads the UI, retrieves the main window widget, and connects all
 * signal handlers.
 * - Calculates the bar's initial on-screen position and moves the
 * window into place.
 *
 * Returns: (transfer none): A pointer to the newly created toolbar #GtkWidget,
 * or %NULL on failure.
 **/
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

      gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
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

void
set_statusbar_label (gchar *message)
{
  GObject *g_object = gtk_builder_get_object (
      bar_gtk_builder, gettext ("labelCurrentSelection"));
  GtkLabel *label = GTK_LABEL (g_object);
  gtk_label_set_label (label, gettext (message));
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
 **/
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
 * This is a convenience wrapper around the generic
 * is_toggle_tool_button_active() function, which it calls with the
 * hardcoded button ID "buttonText".
 *
 * Returns: %TRUE if the text tool button is active, %FALSE otherwise.
 **/
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
 * This is a convenience wrapper around the generic
 * is_toggle_tool_button_active() function, which it calls with the
 * hardcoded button ID "buttonHighlighter".
 *
 * Returns: %TRUE if the highlighter tool button is active, %FALSE otherwise.
 **/
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
 * This is a convenience wrapper that calls is_toggle_tool_button_active()
 * with the hardcoded button ID "buttonFiller".
 *
 * Returns: %TRUE if the filler tool button is active, %FALSE otherwise.
 **/
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
 * This is a convenience wrapper that calls is_toggle_tool_button_active()
 * with the hardcoded button ID "buttonEraser".
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
 * This is a convenience wrapper that calls is_toggle_tool_button_active()
 * with the hardcoded button ID "buttonPencil".
 *
 * Returns: %TRUE if the pen tool button is active, %FALSE otherwise.
 **/
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
 * This is a convenience wrapper that calls is_toggle_tool_button_active()
 * with the hardcoded button ID "buttonPointer".
 *
 * Returns: %TRUE if the pointer tool button is active, %FALSE otherwise.
 **/
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
 * This is a convenience wrapper that calls is_toggle_tool_button_active()
 * with the hardcoded button ID "buttonArrow".
 *
 * Returns: %TRUE if the arrow tool button is active, %FALSE otherwise.
 **/
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
 * It sets the alpha to a semi-opaque value for the highlighter tool and a
 * fully opaque value for the pen tool. Other tools are not affected.
 **/
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

/**
 * take_pen_tool:
 *
 * Programmatically activates the pen or highlighter tool on the toolbar.
 *
 * This function is typically called to switch back to a drawing tool
 * after an action with another tool (like the filler) is complete. It
 * deactivates other currently active tools like the eraser, pointer, or
 * filler.
 *
 * It includes special logic for the filler tool: if the current color
 * is semi-transparent, this function will activate the highlighter tool
 * instead of the standard pen tool. Otherwise, it activates the pen tool.
 **/
void
take_pen_tool (void)
{
  GObject             *pencil_obj = gtk_builder_get_object (bar_gtk_builder,
                                                            "buttonPencil");
  GtkToggleToolButton *pencil_tool_button = GTK_TOGGLE_TOOL_BUTTON (pencil_obj);

  if (is_eraser_toggle_tool_button_active ())
    {
      GObject            *eraser_obj = gtk_builder_get_object (bar_gtk_builder,
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

/**
 * release_lock:
 * @bar_data: (inout): The #BarData struct containing the grab state.
 *
 * Releases the pointer grab from the annotation window, allowing mouse
 * events to pass through to the desktop and other applications.
 *
 * This function is called to exit the drawing mode. It updates the `grab`
 * flag in the @bar_data struct and triggers a redraw of the annotation window.
 * On Windows, it includes a workaround to achieve input transparency by
 * setting the annotation window's opacity to 0.
 **/
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

      gtk_widget_queue_draw (annotation_data->annotation_window);
    }
}

/**
 * lock:
 * @bar_data: (inout): The #BarData struct containing the grab state.
 *
 * Prepares the application to enter a pointer-grabbed ("locked") state.
 *
 * If the pointer is not already grabbed, this function sets the `grab`
 * flag to %TRUE and cancels any pending timers. It is intended to be
 * called before a pointer grab is initiated elsewhere.
 *
 * Note that this function only updates internal state flags and does not
 * perform the actual GDK pointer grab itself.
 *
 * On Windows, it includes a workaround to ensure the background window
 * can receive mouse input by setting its opacity to a small, non-zero
 * value.
 **/
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
 * @bar_data: (inout): The #BarData struct to update with the new color.
 * @selected_color: The new color string (e.g., "FF0000").
 *
 * Sets a new color for the drawing tools.
 *
 * This function is called when the user selects a new color. It ensures a
 * drawing tool is active, updates the color in the #BarData struct, and
 * then adjusts the color's alpha channel based on whether the pen or
 * highlighter is active.
 **/
void
set_color (BarData *bar_data, gchar *selected_color)
{
  take_pen_tool ();
  lock (bar_data);
  bar_data->color = g_strdup_printf ("%s", selected_color);
  annotate_set_color (bar_data->color);
  add_alpha (bar_data);
}

/**
 * set_options:
 * @bar_data: The #BarData struct containing the current tool options.
 *
 * Synchronizes the state of the annotation engine with the current
 * options selected in the toolbar.
 *
 * This function reads various settings (e.g., thickness, rectifier,
 * rounder, arrow mode) from the @bar_data struct and applies them to
 * the annotation module. It also selects the appropriate drawing tool
 * (pen or eraser) in the annotation engine based on which toolbar
 * button is currently active.
 **/
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

/**
 * start_tool:
 * @bar_data: The #BarData struct containing the current application state.
 *
 * Activates the currently selected tool in the annotation engine.
 *
 * This function is called when a tool is selected while a pointer grab is
 * already active. It determines which tool is currently toggled in the
 * toolbar.
 *
 * If the text tool is active, it initializes and starts the text editing
 * widget. For any other drawing tool (pen, eraser, etc.), it ensures the
 * text widget is stopped and then applies the current settings (color,
 * thickness, etc.) to the main annotation engine by calling set_options().
 **/
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

/**
 * end_clapperboard_countdown:
 * @user_data: (unused): Data passed from the timeout source.
 *
 * This function is a timeout callback executed after the clapperboard
 * countdown has finished.
 *
 * It hides the clapperboard overlay, releases the pointer grab, and then
 * re-activates the previously selected tool to return the application to
 * its prior state. It triggers a redraw of the annotation window to
 * reflect these changes.
 *
 * Returns: %G_SOURCE_REMOVE to ensure the timeout is not called again.
 **/
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

/**
 * begin_clapperboard_countdown:
 *
 * Starts a timer that triggers the end of the clapperboard sequence.
 *
 * It schedules the end_clapperboard_countdown() function to be called
 * after the timeout specified by %BAR_TO_TOP_TIMEOUT has elapsed.
 **/
void
begin_clapperboard_countdown (void)
{
  timer = g_timeout_add (BAR_TO_TOP_TIMEOUT, end_clapperboad_countdown, NULL);
}
