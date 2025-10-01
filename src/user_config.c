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

#include <gio/gio.h>
#include <glib/gstdio.h>

#include "config.h"
#include "config_path.h"
#include "user_config.h"

/* Copy system config to user config if needed */
void
user_config_ensure_file (void)
{
  gchar *user_conf_path = g_build_filename (g_get_user_config_dir (),
                                            "ardesiarc",
                                            NULL);

  if (!g_file_test (user_conf_path, G_FILE_TEST_IS_REGULAR))
    {
      g_mkdir_with_parents (g_get_user_config_dir (), 0700);

      gchar *sys_conf_path = g_build_filename (ARDESIA_SYSCONFDIR,
                                               "ardesia.conf",
                                               NULL);

      if (g_file_test (sys_conf_path, G_FILE_TEST_IS_REGULAR))
        {
          GFile *source = g_file_new_for_path (sys_conf_path);
          GFile *dest   = g_file_new_for_path (user_conf_path);

          GError *error = NULL;
          if (! g_file_copy (source,
                             dest,
                             G_FILE_COPY_OVERWRITE,
                             NULL,
                             NULL,
                             NULL,
                             &error))
            {
              g_warning ("Unable to copy system config %s to %s: %s",
                         sys_conf_path,
                         user_conf_path,
                         error ? error->message : "unknown error");
              if (error)
                {
                  g_error_free (error);
                }
            }

          g_object_unref (source);
          g_object_unref (dest);
        }
      else
        {
          g_warning ("System configuration file %s not found", sys_conf_path);
        }

      g_free (sys_conf_path);
    }

  g_free (user_conf_path);
}

/* Load a GKeyFile either from user or system config. */
GKeyFile *
user_config_load_keyfile (void)
{
  GKeyFile *kf = g_key_file_new ();

  /*
   * Use helper to get the correct config file for reading.
   * get_config_file() returns the user config path if it exists,
   * otherwise the system config path, or NULL if none exists.
   */
  gchar *cfg = get_config_file ();
  if (cfg != NULL)
    {
      /* load_from_file will tolerate missing groups; errors are ignored */
      g_key_file_load_from_file (kf, cfg, G_KEY_FILE_NONE, NULL);
      g_free (cfg);
    }

  return kf;
}
