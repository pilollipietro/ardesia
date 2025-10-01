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

/* Return system-wide config path (caller must g_free()). */
gchar *
get_system_config_file (void)
{
  return g_build_filename (ARDESIA_SYSCONFDIR, SYSTEM_CONFIG_BASENAME, NULL);
}

/* Return the user config path (caller must g_free()). */
gchar *
get_user_config_file (void)
{
  const gchar *user_config_dir = g_get_user_config_dir ();
  return g_build_filename (user_config_dir, USER_CONFIG_BASENAME, NULL);
}

/* For reads: prefer user config if present, otherwise the system file.
 * Returns newly-allocated string or NULL if no file exists.
 */
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

/* For writes: ensure a user config exists (copy from system if needed),
 * then return the user config path (newly allocated).
 */
gchar *
get_user_file (void)
{
  /* Ensure ~/.config/ardesiarc exists (the function will copy system config
   * into the user location if appropriate). */
  user_config_ensure_file ();
  return get_user_config_file ();
}
