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

/*
 * config_path.c
 *
 * Implementation of small helpers that resolve the config file path.
 * Reading helpers prefer the user file (if present) and fall back to
 * the system file. Writing helpers ensure the user file exists.
 */

#include "config_path.h"
#include <glib.h>

/* Forward-declare the function that creates the user file from system config.
 * The real implementation lives in user_config.c (user_config_ensure_file()).
 * Keep this as an extern to avoid adding an extra header dependency.
 */
extern void user_config_ensure_file (void);

/* System config filename constants */
#ifndef ARDESIA_SYSCONFDIR
#define ARDESIA_SYSCONFDIR "/etc"
#endif

#define SYSTEM_CONFIG_BASENAME "ardesia.conf"
#define USER_CONFIG_BASENAME   "ardesiarc"

/**
 * get_system_config_file:
 *
 * Returns the full path to the system-wide configuration file.
 *
 * Returns: (transfer full): a newly-allocated string containing the path,
 * which must be freed with g_free().
 **/
gchar *
get_system_config_file (void)
{
  return g_build_filename (ARDESIA_SYSCONFDIR, SYSTEM_CONFIG_BASENAME, NULL);
}

/**
 * get_config_file:
 *
 * Returns the configuration file path to be used for reading. If the user
 * configuration file exists, it is preferred; otherwise, the system
 * configuration file is returned. If neither file exists, returns NULL.
 *
 * Returns: (transfer full): a newly-allocated string with the file path
 * or NULL if no configuration file exists.
 **/
gchar *
get_config_file (void)
{
  gchar *user = get_user_config_file ();
  if (g_file_test (user, G_FILE_TEST_IS_REGULAR))
    {
      return user;
    }
  g_free (user);

  gchar *sys = get_system_config_file ();
  if (g_file_test (sys, G_FILE_TEST_IS_REGULAR))
    {
      return sys;
    }
  g_free (sys);

  return NULL;
}

/**
 * get_user_config_file:
 *
 * Returns the full path to the user-specific configuration file;
 * This ensure a user config exists (copy from system if needed).
 *
 * Returns: (transfer full): a newly-allocated string containing the path,
 * which must be freed with g_free().
 **/
gchar *
get_user_config_file (void)
{
  /* Ensure ~/.config/ardesiarc exists (the function will copy system config
   * into the user location if appropriate). */
  user_config_ensure_file ();
  return g_build_filename (g_get_user_config_dir (), USER_CONFIG_BASENAME, NULL);
}
