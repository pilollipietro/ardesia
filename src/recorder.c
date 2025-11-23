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

#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "annotation_window.h"
#include "ardesia.h"
#include "keyboard.h"
#include "recorder.h"
#include "saver.h"
#include "utils.h"

/* pid of the recording process. */
static GPid recorder_pid;

/* is the recorder started */
static gboolean started = FALSE;

/* is the recorder paused */
static gboolean paused = FALSE;

/**
 * call_recorder:
 * @filename: The name of the video file to save.
 * @option:   The recorder command option ("start", "pause", "resume", "stop").
 *
 * Spawns a separate screencast recorder process (VLC) with the specified
 * @filename and @option. Sets up temporary PID and log files in the
 * project directory and checks if the recorder started successfully.
 *
 * Returns: The process ID (PID) of the recorder if started successfully,
 *          or -1 if the recorder failed to start.
 **/
static GPid
call_recorder (gchar *filename, gchar *option)
{
  GPid pid = (GPid) 0;

  gchar *pidfilename = g_strdup_printf ("%s%s%s", get_project_dir (),
                                        G_DIR_SEPARATOR_S,
                                        "ardesia_recorder.pid");

  gchar *logfilename = g_strdup_printf ("%s%s%s", get_project_dir (),
                                        G_DIR_SEPARATOR_S,
                                        "ardesia_recorder.log");

  gchar *quoted_filename = g_strdup_printf ("%s", filename);

  gchar *argv[10]        = { RECORDER_FILE,
                             option,
                             logfilename,
                             "0",
                             "0",
                             "100",
                             "100",
                             quoted_filename,
                             pidfilename,
                             (gchar *) 0 };

  gint       x = 0, y = 0;
  GtkWidget *annotation_window = get_annotation_window ();

  gdk_window_get_root_origin (gtk_widget_get_window (annotation_window),
                              &x, &y);

  argv[3] = g_strdup_printf ("%d", y);
  argv[4] = g_strdup_printf ("%d", x);

  argv[5] = g_strdup_printf (
      "%d", gtk_widget_get_allocated_width (annotation_window));

  argv[6] = g_strdup_printf (
      "%d", gtk_widget_get_allocated_height (annotation_window));

  g_debug ("call_recorder: %s %s %s %s %s %s %s\n",
           logfilename,
           argv[3],
           argv[4],
           argv[5],
           argv[6],
           argv[7],
           argv[8]);

  if (g_spawn_async (NULL /*working_directory*/,
                     argv,
                     NULL /*envp*/,
                     G_SPAWN_SEARCH_PATH,
                     NULL /*child_setup*/,
                     NULL /*user_data*/,
                     &pid /*child_pid*/,
                     NULL /*error*/))
    {
      started = TRUE;
    }
  g_free (logfilename);
  g_free (quoted_filename);
  g_free (argv[3]);
  g_free (argv[4]);
  g_free (argv[5]);
  g_free (argv[6]);

  // wait 1 second and then check to see if ardesia_recorder.pid exists
  // if its does not then we failed to start VLC properly
  gboolean pid_exists = file_exists (pidfilename);
  int      wait       = 0;
  if (! pid_exists)
    {
      while (wait < 3 && ! pid_exists)
        {
#ifdef _WIN32
          sleep (1000);
#else
          sleep (1);
#endif
          pid_exists = file_exists (pidfilename);
          wait++;
        }
    }

  g_free (pidfilename);
  if (! pid_exists)
    {
      pid = -1;
    }
  g_debug ("Recorder PID: %d\n", pid);
  return pid;
}

/**
 * is_recorder_available:
 *
 * Checks if the screencast recorder (VLC) is available on the system.
 *
 * On Windows, it first checks for the VLC installation in the standard
 * Program Files directories. On all platforms, it tries to spawn VLC in
 * dummy mode to see if it can run without errors.
 *
 * Returns: TRUE if the recorder is available, FALSE otherwise.
 **/
gboolean
is_recorder_available (void)
{
#ifdef _WIN32
  // dummy-quiet stops a dos command box from opening
  gchar *argv[5] = { "vlc",           "-I",         "dummy",
                     "--dummy-quiet", "vlc://quit", (gchar *) 0 };
#else
  gchar *argv[5] = { "vlc", "-I", "dummy", "vlc://quit", (gchar *) 0 };
#endif

#ifdef _WIN32
  gchar   *videolan    = "\\VideoLAN\\VLC";
  gchar   *programfile = getenv ("PROGRAMFILES");
  gchar   *file        = g_strdup_printf ("%s\\%s", programfile, videolan);
  gboolean ret         = file_exists (file);

  if (ret)
    {
      return TRUE;
    }

  g_free (programfile);
  g_free (file);
  programfile = getenv ("PROGRAMFILES (X86)");
  file        = g_strdup_printf ("%s\\%s", programfile, videolan);
  ret         = file_exists (file);
  g_free (programfile);
  g_free (file);

  if (ret)
    {
      return TRUE;
    }
#endif

  return g_spawn_async (NULL /*working_directory*/,
                        argv,
                        NULL /*envp*/,
                        G_SPAWN_SEARCH_PATH,
                        NULL /*child_setup*/,
                        NULL /*user_data*/,
                        NULL /*child_pid*/,
                        NULL /*error*/);
}

/**
 * is_started:
 *
 * Checks if the screencast recorder is currently running.
 *
 * Returns: TRUE if the recorder has been started and the process ID is valid,
 *          FALSE otherwise.
 **/
gboolean
is_started (void)
{
  return started && recorder_pid > 0;
}

/**
 * is_paused:
 *
 * Checks if the screencast recorder is currently paused.
 *
 * Returns: TRUE if the recorder is paused, FALSE otherwise.
 **/
gboolean
is_paused (void)
{
  return paused;
}

/**
 * pause_recorder:
 *
 * Pauses the currently running screencast recorder if it has been started.
 * Updates the internal paused flag accordingly.
 **/
void
pause_recorder (void)
{
  if (is_started ())
    {
      recorder_pid = call_recorder (NULL, "pause");
      paused       = TRUE;
    }
}

/**
 * resume_recorder:
 *
 * Resumes the paused screencast recorder if it was previously started.
 * Updates the internal paused flag accordingly.
 **/
void
resume_recorder (void)
{
  if (is_started ())
    {
      recorder_pid = call_recorder (NULL, "resume");
      paused       = FALSE;
    }
}

/**
 * stop_recorder:
 *
 * Stops the currently running screencast recorder.
 * Closes the recorder process and updates the internal started flag.
 **/
void
stop_recorder (void)
{
  if (is_started ())
    {
      // g_spawn_close_pid (recorder_pid);
      recorder_pid = call_recorder (NULL, "stop");
      g_spawn_close_pid (recorder_pid);
      started = FALSE;
    }
}

/**
 * visualize_missing_recorder_program_dialog:
 * @parent: The parent #GtkWindow for modal positioning.
 * @message: The error message to display.
 *
 * Displays a modal GTK error dialog informing the user that the recorder
 * program is missing or failed to start. The function blocks until the user
 * acknowledges the dialog by clicking OK.
 *
 * This function ensures the dialog is modal relative to @parent and
 * automatically destroys the dialog widget after use.
 **/
void
visualize_missing_recorder_program_dialog (GtkWindow *parent, gchar *message)
{
  GtkWidget *miss_dialog = (GtkWidget *) NULL;

  miss_dialog = gtk_message_dialog_new (parent,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        "%s",
                                        message);

  // gtk_window_set_keep_above (GTK_WINDOW (miss_dialog), TRUE);

  gtk_dialog_run (GTK_DIALOG (miss_dialog));

  if (miss_dialog != NULL)
    {
      gtk_widget_destroy (miss_dialog);
      miss_dialog = NULL;
    }
}

/**
 * start_save_video_dialog:
 * @toolbutton: The recorder tool button from the Ardesia toolbar.
 * @parent: The parent #GtkWindow for modal dialog positioning.
 *
 * Opens a file chooser dialog asking the user where to save a screencast video.
 * This function ensures that the filename does not overwrite existing files
 * unless the user confirms.
 * The .ogv extension is automatically added if missing.
 *
 * The function also handles starting and stopping the virtual keyboard
 * as needed and interacts with the user to confirm file overwrites or
 * handle errors.
 *
 * Returns: %TRUE if the recorder was successfully started, %FALSE otherwise.
 *
 * Notes:
 * - Ensures the filename is unique by appending a number if the target file
 *   already exists.
 * - Automatically appends the ".ogv" extension if not present in the filename.
 * - Calls start_virtual_keyboard() before showing the dialog and
 *   stop_virtual_keyboard() afterward.
 * - Calls call_recorder() to start the recording; if it fails, displays an
 *   error dialog.
 **/
gboolean
start_save_video_dialog (GtkButton *toolbutton, GtkWindow *parent)
{
  gboolean status = FALSE;

  gchar *filename = g_strdup_printf ("%s", get_project_name ());

  GtkWidget *chooser;
  chooser = gtk_file_chooser_dialog_new (gettext ("Save video as ogv"),
                                         parent,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "Save _As",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

  gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
  gtk_window_set_keep_above (GTK_WINDOW (chooser), TRUE);

  gtk_window_set_title (GTK_WINDOW (chooser), gettext ("Choose a file"));

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                       get_project_dir ());

  /*
   * Test if ogv file already exists - if it does then add a number to the
   * end - continue until new file can be made
   */
  gchar *supported_extension = ".ogv";
  gchar *filename_copy       = (gchar *) NULL;
  gchar *filename_fullpath   = (gchar *) NULL;
  filename_copy     = g_strdup_printf ("%s%s", filename, supported_extension);
  filename_fullpath = g_strdup_printf ("%s%s%s", get_project_dir (),
                                       G_DIR_SEPARATOR_S, filename_copy);
  gint counter      = 1;
  while (access (filename_fullpath, F_OK) != -1)
    {
      /* File exists */
      filename_copy = g_strdup_printf ("%s_%d%s",
                                       filename,
                                       counter,
                                       supported_extension);

      filename_fullpath = g_strdup_printf ("%s%s%s",
                                           get_project_dir (),
                                           G_DIR_SEPARATOR_S,
                                           filename_copy);

      counter++;
    }
  filename = g_strdup_printf ("%s", filename_copy);

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), filename);

  start_virtual_keyboard ();

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      g_free (filename);
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      filename_copy = g_strdup_printf ("%s", filename);

      if (! g_str_has_suffix (filename, supported_extension))
        {
          g_free (filename_copy);
          filename_copy = g_strdup_printf ("%s%s",
                                           filename,
                                           supported_extension);
        }

      g_free (filename);
      filename = filename_copy;

      if (file_exists (filename))
        {
          gint result = show_override_dialog (GTK_WINDOW (chooser));
          if (result == GTK_RESPONSE_NO)
            {
              g_free (filename);
              filename = NULL;
              gtk_widget_destroy (chooser);
              chooser = NULL;
              return status;
            }
        }
      else
        {
          FILE *stream = g_fopen (filename, "w");
          if (stream == NULL)
            {
              show_could_not_write_dialog (GTK_WINDOW (chooser));
            }
          else
            {
              fclose (stream);
            }
        }
    }

  stop_virtual_keyboard ();

  if (chooser)
    {
      gtk_widget_destroy (chooser);
      chooser = NULL;
    }
  recorder_pid = call_recorder (filename, "start");
  status       = (recorder_pid > 0);
  if (! status)
    {
      visualize_missing_recorder_program_dialog (
          parent,
          "VLC failed to start properly, check installation and logs.");
    }

  g_free (filename);
  filename = NULL;

  return status;
}
