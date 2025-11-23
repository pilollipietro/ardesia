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

#include "workspace.h"
#include "monitor.h"
#include "utils.h"

/**
 * create_workspace:
 *
 * Creates a new #Workspace structure and initializes its fields
 * with default values.
 *
 * Returns: (transfer full): a new #Workspace structure
 **/
Workspace *
create_workspace (void)
{
  g_debug ("Creating workspace\n");
  Workspace *workspace = g_malloc ((gsize) sizeof (Workspace));
  workspace->monitors  = NULL;
  set_defaults_for_workspace (workspace);
  return workspace;
}

/**
 * set_defaults_for_workspace:
 * @workspace: a #Workspace
 *
 * Sets default values for the given #Workspace, including
 * project name, date, and monitor list.
 **/
void
set_defaults_for_workspace (Workspace *workspace)
{
  g_debug ("Setting workspace defaults\n");
  if (workspace->monitors == NULL)
    {
      workspace->monitors = create_monitor_list ();
    }
  workspace->date = get_date ();

  /* Show the project name wizard. */
  workspace->project_name = g_strdup_printf ("ardesia_project_%s",
                                             workspace->date);

  workspace->workspace_dir = NULL;
  workspace->project_dir   = NULL;
  workspace->iwb_filename  = NULL;
}

/**
 * debug_workspace:
 * @workspace: a #Workspace
 *
 * Prints debug information about the given #Workspace to the
 * debug output.
 **/
void
debug_workspace (Workspace *workspace)
{
  g_debug ("Project Name: %s\n", workspace->project_name);
  g_debug ("Project Directory: %s\n", workspace->project_dir);
  g_debug ("Workspace Directory: %s\n", workspace->workspace_dir);
  g_debug ("iwb filename: %s\n", workspace->iwb_filename);
  g_debug ("Date: %s\n", workspace->date);
  debug_monitor_list (workspace->monitors);
}

/**
 * destroy_workspace:
 * @workspace: a #Workspace
 *
 * Frees all memory and resources associated with the #Workspace.
 **/
void
destroy_workspace (Workspace *workspace)
{
  g_debug ("Destroying workspace\n");
  if (workspace->project_dir != NULL)
    {
      remove_dir_if_empty (workspace->project_dir);
    }
  g_free (workspace->date);
  g_free (workspace->project_name);
  g_free (workspace->project_dir);
  g_free (workspace->workspace_dir);
  g_free (workspace->iwb_filename);
  destroy_monitor_list (workspace->monitors);
  g_free (workspace);
}

/**
 * Create_workspace_shortcut:
 * @workspace: a #Workspace structure containing at least the workspace_dir.
 *
 * Creates a shortcut (link) to the given workspace directory on the user’s
 * desktop. The shortcut filename will be “<PACKAGE_NAME>_workspace”.
 *
 * On Windows it calls windows_create_link () using a stock folder icon,
 * on Unix-like systems it calls xdg_create_link () with the
 * “folder-documents” icon.
 *
 * Example:
 *   create_workspace_shortcut (my_workspace);
 */
static void
create_workspace_shortcut (Workspace *workspace)
{
  gchar *workspace_dir = workspace->workspace_dir;
  g_debug ("Creating workspace shortcut\n");
  gchar *desktop_entry_filename = g_strdup_printf ("%s%s%s_workspace",
                                                   get_desktop_dir (),
                                                   G_DIR_SEPARATOR_S,
                                                   PACKAGE_NAME);

#ifdef _WIN32
  windows_create_link (workspace_dir,
                       desktop_entry_filename,
                       "%SystemRoot%\\system32\\imageres.dll",
                       123);

#else
  xdg_create_link (workspace_dir, desktop_entry_filename, "folder-documents");
#endif
  g_free (desktop_entry_filename);
}

/**
 * create_default_project_dir:
 * @workspace: a #Workspace structure containing workspace_dir and project_name.
 *
 * Ensures that the default project directory exists under the workspace’s
 * base directory.
 *
 * It builds the full path from workspace_dir + project_name, updates
 * workspace->project_dir accordingly (freeing any previous value),
 * and creates the directory on disk with mode 0700 if it does not exist.
 *
 * Example:
 *   create_default_project_dir (my_workspace);
 */
static void
create_default_project_dir (Workspace *workspace)
{
  g_debug ("Creating project directory\n");
  if (workspace->project_dir != NULL)
    {
      g_free (workspace->project_dir);
      workspace->project_dir = NULL;
    }

  workspace->project_dir = g_build_filename (workspace->workspace_dir,
                                             workspace->project_name,
                                             (gchar *) 0);

  if (! file_exists (workspace->project_dir))
    {
      if (g_mkdir_with_parents (workspace->project_dir, 0700) == -1)
        {
          g_warning ("Unable to create folder %s\n", workspace->project_dir);
        }
    }
}

/**
 * Configure_workspace:
 * @workspace: a #Workspace to configure (its workspace_dir will be set).
 *
 * Configures the workspace by assigning its default directory path.
 *
 * Any existing workspace_dir string is freed. Then the function builds
 * a new path consisting of the user’s Documents directory plus the
 * application’s package name (e.g. “~/Documents/<PACKAGE_NAME>”)
 * and stores it in workspace->workspace_dir.
 *
 * Example:
 *   configure_workspace (my_workspace);
 */
void
configure_workspace (Workspace *workspace)
{
  g_debug ("Configuring workspace\n");
  if (workspace->workspace_dir != NULL)
    {
      g_free (workspace->workspace_dir);
      workspace->workspace_dir = NULL;
    }
  const gchar *documents_dir = get_documents_dir ();

  /* The workspace directory is in the documents ardesia folder. */
  workspace->workspace_dir = g_build_filename (documents_dir,
                                               PACKAGE_NAME,
                                               (gchar *) 0);
}

/**
 * change_workspace_to:
 * @workspace: a #Workspace
 * @filename: a file path
 *
 * Changes the current workspace to the given file, updating
 * project name, project directory, and IWB filename.
 **/
void
change_workspace_to (Workspace *workspace, gchar *filename)
{

  gint init_pos = -1;
  gint end_pos  = -1;

  if (g_path_is_absolute (filename))
    {
      filename = g_strdup (filename);
    }
  else
    {
      gchar *dir = g_get_current_dir ();
      filename   = g_build_filename (dir, filename, (gchar *) 0);
      g_free (dir);
    }

  if (! file_exists (filename))
    {
      g_error ("No such file %s\n", filename);
      exit (EXIT_FAILURE);
    }

  if (workspace->iwb_filename != NULL)
    {
      g_free (workspace->iwb_filename);
      workspace->iwb_filename = NULL;
    }
  workspace->iwb_filename = filename;

  init_pos = g_substrlastpos (filename, G_DIR_SEPARATOR_S);
  end_pos  = g_substrlastpos (filename, ".");

  if (workspace->project_name != NULL)
    {
      g_free (workspace->project_name);
      workspace->project_name = NULL;
    }
  workspace->project_name = g_substr (filename, init_pos + 1, end_pos - 1);
  if (workspace->project_dir != NULL)
    {
      g_free (workspace->project_dir);
      workspace->project_dir = NULL;
    }
  workspace->project_dir = g_substr (filename, 0, init_pos - 1);
}

/**
 * build_workspace_filesystem:
 * @workspace: a #Workspace
 *
 * Configures the workspace, creates the desktop shortcut, and
 * ensures the default project directory exists.
 **/
void
build_workspace_filesystem (Workspace *workspace)
{
  configure_workspace (workspace);
  create_workspace_shortcut (workspace);
  create_default_project_dir (workspace);
  debug_workspace (workspace);
}
