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

#include "ardesia.h"
#include "annotation_window.h"
#include "background_window.h"
#include "bar.h"
#include "bar_callbacks.h"
#include "commandline.h"
#include "project_dialog.h"
#include "text_window.h"
#include "utils.h"

extern TextConfig *text_config;
GtkWidget         *ardesia_bar_window;
GtkWidget         *background_window;
GtkWidget         *annotation_window;
Workspace         *workspace;
CommandLine       *commandline = NULL;

/**
 * is_wayland_session:
 *
 * Detects if Ardesia is running under Wayland by checking XDG_SESSION_TYPE
 * environment variable.
 *
 * On Wayland: compositing is always active in the protocol
 * On X11: requires explicit composite manager
 *
 * Returns: TRUE if running on Wayland, FALSE on X11 or unknown
 */
static gboolean
is_wayland_session (void)
{
  const gchar *xdg_session_type = g_getenv ("XDG_SESSION_TYPE");

  if (xdg_session_type == NULL)
    {
      return FALSE;
    }

  gboolean is_wayland = (g_strcmp0 (xdg_session_type, "wayland") == 0);

  if (is_wayland)
    {
      g_debug ("Display server: Wayland (compositing always available)");
    }

  return is_wayland;
}

/**
 * get_drawable_area:
 *
 * Returns the GdkRectangle representing the drawable area for annotation,
 * text, and background windows, based on the current command line options
 * and workspace settings.
 *
 * The drawable area depends on the mode:
 * - DRAW_ON_MONITOR: returns the rectangle of the selected monitor.
 * - DRAW_ON_FULLDESKTOP: returns a rectangle covering the full desktop.
 * - DRAW_ON_CLIPAREA: returns a rectangle defined by commandline->clipRect,
 *   clamped to the screen boundaries.
 *
 * Returns: a pointer to a GdkRectangle, or NULL if the area cannot be
 *          determined.
 */
GdkRectangle *
get_drawable_area (void)
{
  if (commandline != NULL)
    {
      if (commandline->mode == DRAW_ON_MONITOR)
        {
          GList *monitors;
          monitors               = workspace->monitors;
          gint monitors_len      = g_list_length (monitors);
          gint workspace_monitor = commandline->workspace_monitor;
          g_debug ("Detected %d monitors", monitors_len);
          g_debug ("Selected monitor %d", workspace_monitor);
          if (workspace_monitor < 0 || workspace_monitor >= monitors_len)
            {
              g_warning ("Workspace monitor was given an illegal value, moving "
                         "to monitor 0.\n");
              commandline->workspace_monitor = 0;
            }
          Monitor *monitor = g_list_nth_data (monitors,
                                              commandline->workspace_monitor);
          return monitor->rect;
        }
      else if (commandline->mode == DRAW_ON_FULLDESKTOP)
        {
          GdkDisplay *display      = gdk_display_get_default ();
          GdkMonitor *monitor      = gdk_display_get_primary_monitor (display);
          GdkRectangle geometry;

          if (monitor == NULL)
            monitor = gdk_display_get_monitor (display, 0);

          gdk_monitor_get_geometry (monitor, &geometry);

          commandline->clipRect->x      = geometry.x;
          commandline->clipRect->y      = geometry.y;
          commandline->clipRect->width  = geometry.width;
          commandline->clipRect->height = geometry.height;

          return commandline->clipRect;
        }
      else
        {
          // check clipRect bounds
          GdkDisplay *display = gdk_display_get_default ();
          GdkMonitor *monitor = gdk_display_get_primary_monitor (display);
          GdkRectangle geometry;
          int maxwidth;
          int maxheight;

          if (monitor == NULL)
            monitor = gdk_display_get_monitor (display, 0);

          gdk_monitor_get_geometry (monitor, &geometry);

          maxwidth  = geometry.width;
          maxheight = geometry.height;

          g_debug ("Maximum Size: %d %d\n", maxwidth, maxheight);

          if (commandline->clipRect->x < 0)
            {
              commandline->clipRect->x = 0;
            }
          if (commandline->clipRect->y < 0)
            {
              commandline->clipRect->y = 0;
            }
          if (commandline->clipRect->x > maxwidth)
            {
              commandline->clipRect->x = maxwidth;
            }
          if (commandline->clipRect->x > maxheight)
            {
              commandline->clipRect->y = maxheight;
            }
          return commandline->clipRect;
        }
    }
  return NULL;
}
/**
 * get_toolbar_area:
 *
 * Returns the GdkRectangle representing the area for the toolbar window.
 *
 * In DRAW_ON_MONITOR mode, the rectangle corresponds to the selected
 * tools monitor.
 * In other modes, the drawable area is used as toolbar area.
 *
 * Returns: a pointer to a GdkRectangle, or NULL if the area cannot be
 *          determined.
 */
GdkRectangle *
get_toolbar_area (void)
{
  if (commandline != NULL)
    {
      if (commandline->mode == DRAW_ON_MONITOR)
        {
          Monitor *monitor = g_list_nth_data (workspace->monitors,
                                              commandline->tools_monitor);
          return monitor->rect;
        }
      else
        {
          /*
           * Non-monitor modes: use the drawable area as toolbar area.
           */
          return get_drawable_area ();
        }
    }
  return NULL;
}

#ifndef _WIN32

/**
 * run_missing_composite_manager_dialog:
 *
 * Displays a GTK modal dialog informing the user that a composite manager
 * must be enabled to run Ardesia. Exits the program after the dialog is closed.
 */
static void
run_missing_composite_manager_dialog (void)
{
  GtkWidget *msg_dialog;
  msg_dialog = gtk_message_dialog_new (NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       gettext ("In order to run Ardesia you "
                                                "need to enable a composite "
                                                "manager"));

  gtk_dialog_run (GTK_DIALOG (msg_dialog));

  if (msg_dialog != NULL)
    {
      gtk_widget_destroy (msg_dialog);
      msg_dialog = NULL;
    }

  exit (EXIT_FAILURE);
}

/**
 * check_composite_manager:
 *
 * Checks if a composite manager is active on the current screen.
 *
 * On Wayland: compositing is always available, check is skipped
 * On X11: validates that a composite manager is enabled
 *
 * If compositing is not available on X11, displays error dialog and exits.
 */
static void
check_composite_manager (void)
{
  /* On Wayland, compositing is always part of the protocol */
  if (is_wayland_session ())
    {
      g_debug ("Wayland detected");
      return;
    }

  /* On X11, check if composite manager is running */
  GdkDisplay *display = gdk_display_get_default ();

  if (display == NULL)
    {
      g_critical ("Cannot get default display");
      return;
    }

  GdkScreen *screen = gdk_display_get_default_screen (display);

  if (screen == NULL)
    {
      g_critical ("Cannot get default screen");
      return;
    }

  gboolean composite = gdk_screen_is_composited (screen);

  if (! composite)
    {
      g_warning ("No composite manager detected on X11 session");
      run_missing_composite_manager_dialog ();
    }
  else
    {
      g_debug ("Composite manager is active");
    }
}

#endif

/**
 * enable_localization_support:
 *
 * Initializes localization support using gettext if ENABLE_NLS is defined.
 * Sets locale and text domain for translations.
 */
static void
enable_localization_support (void)
{
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (GETTEXT_PACKAGE);
#endif
}

/**
 * build_annotation_window:
 *
 * Creates and shows the annotation window. Positions it based on
 * the drawable area calculated from the workspace and command line options.
 *
 * If the window cannot be created, quits the program.
 */
void
build_annotation_window (void)
{
  annotation_window = create_annotation_window (workspace, commandline);
  if (annotation_window == NULL)
    {
      annotate_quit ();
      g_free (commandline);
      exit (EXIT_FAILURE);
    }

  GdkRectangle *rect = get_drawable_area ();
  position_annotation_window (rect->x, rect->y, rect->width, rect->height);
  gtk_widget_show (annotation_window);
}

/**
 * build_toolbar_window:
 *
 * Creates and shows the toolbar window (Ardesia bar). Positions it based on
 * the toolbar area calculated from the workspace and command line options.
 *
 * The toolbar window is set to stay above other windows.
 * If the window cannot be created, quits the program.
 */
void
build_toolbar_window (void)
{
  GdkRectangle *rect = get_toolbar_area ();
  ardesia_bar_window = create_bar_window (commandline, rect, annotation_window);

  if (ardesia_bar_window == NULL)
    {
      annotate_quit ();
      g_free (commandline);
      exit (EXIT_FAILURE);
    }

  gtk_window_set_keep_above (GTK_WINDOW (ardesia_bar_window), TRUE);
  gtk_widget_show (ardesia_bar_window);
}

/**
 * main:
 * @argc: argument count
 * @argv: argument vector
 *
 * The main entry point for Ardesia. Initializes GTK, handles command line
 * arguments, sets up the workspace, creates windows, initializes fonts,
 * and enters the GTK main loop.
 *
 * Returns: 0 on normal exit, 1 on fatal error.
 */
int
main (int argc, char *argv[])
{
  // make global variables NULL
  commandline        = (CommandLine *) NULL;
  ardesia_bar_window = (GtkWidget *) NULL;
  background_window  = (GtkWidget *) NULL;
  annotation_window  = (GtkWidget *) NULL;
  workspace          = (Workspace *) NULL;

  /* Enable the localization support with gettext. */
  enable_localization_support ();

  // start GTK
  gtk_init (&argc, &argv);


  // handle command line
  commandline = create_command_line ();
  if (commandline == NULL)
    {
      g_critical ("Fatal: Could not allocate memory for command line options.");
      return 1; /* O EXIT_FAILURE */
    }

  parse_options (commandline, argc, argv);
  if (commandline->debug)
    {
      g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);
      debug_commandline (commandline);
    }

#ifndef _WIN32
  check_composite_manager ();
#endif

  /* Initialize new text configuration options. */
  text_config             = create_text_config ();
  text_config->leftmargin = commandline->text_leftmargin;
  text_config->tabsize    = commandline->text_tabsize;

  /* Handle workspace. */
  workspace = create_workspace ();
  if (commandline->iwb_filename)
    {
      change_workspace_to (workspace, commandline->iwb_filename);
    }
  build_workspace_filesystem (workspace);

  /* Create windows, one for the drawing and one for the toolbar. */
  build_annotation_window ();
  build_toolbar_window ();

  g_debug ("Project started in %s", workspace->project_dir);

  initialize_font ();
  gtk_main ();

  destroy_workspace (workspace);
  destroy_command_line (commandline);

  return 0;
}
