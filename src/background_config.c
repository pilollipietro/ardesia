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
 
#include "background_config.h"
#include <gio/gio.h>
#include <glib/gstdio.h>

#ifndef ARDESIA_SYSCONFDIR
#define ARDESIA_SYSCONFDIR "/etc"
#endif

/* Copy system config to user config if needed */
void
background_config_ensure_user_file (void)
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
          if (!g_file_copy (source,
                            dest,
                            G_FILE_COPY_OVERWRITE,
                            NULL, NULL, NULL, &error))
            {
              g_warning ("Unable to copy system config %s to %s: %s",
                         sys_conf_path,
                         user_conf_path,
                         error ? error->message : "unknown error");
              if (error) g_error_free (error);
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

/* Internal: load a GKeyFile either from user or system config */
static GKeyFile *
background_config_load_keyfile (void)
{
  GKeyFile *kf = g_key_file_new ();

  gchar *usrfile = g_build_filename (g_get_user_config_dir (),
                                     "ardesiarc",
                                     NULL);

  if (g_file_test (usrfile, G_FILE_TEST_IS_REGULAR))
    {
      g_key_file_load_from_file (kf, usrfile, G_KEY_FILE_NONE, NULL);
    }
  else
    {
      gchar *sysfile = g_build_filename (ARDESIA_SYSCONFDIR,
                                         "ardesia.conf",
                                         NULL);
      g_key_file_load_from_file (kf, sysfile, G_KEY_FILE_NONE, NULL);
      g_free (sysfile);
    }

  g_free (usrfile);
  return kf;
}

gchar **
background_config_get_color_keys (gsize *n_colors)
{
  GKeyFile *kf = background_config_load_keyfile ();
  gchar **keys = g_key_file_get_keys (kf, "colors", n_colors, NULL);
  g_key_file_unref (kf);
  return keys;
}

gchar **
background_config_get_image_keys (gsize *n_images)
{
  GKeyFile *kf = background_config_load_keyfile ();
  gchar **keys = g_key_file_get_keys (kf, "images", n_images, NULL);
  g_key_file_unref (kf);
  return keys;
}

gchar *
background_config_get_color (const gchar *key)
{
  GKeyFile *kf = background_config_load_keyfile ();
  gchar *val = g_key_file_get_string (kf, "colors", key, NULL);
  g_key_file_unref (kf);
  return val;
}

gchar *
background_config_get_image (const gchar *key)
{
  GKeyFile *kf = background_config_load_keyfile ();
  gchar *val = g_key_file_get_string (kf, "images", key, NULL);
  g_key_file_unref (kf);
  return val;
}

gboolean
background_config_save (GKeyFile *kf)
{
  background_config_ensure_user_file ();

  gchar *usrfile = g_build_filename (g_get_user_config_dir (),
                                     "ardesiarc",
                                     NULL);

  gsize len = 0;
  gchar *data = g_key_file_to_data (kf, &len, NULL);
  gboolean ok = g_file_set_contents (usrfile, data, len, NULL);

  g_free (data);
  g_free (usrfile);

  return ok;
}

/* Add color entry to the user configuration file */
void
background_config_add_color (const gchar *name, const gchar *rgba)
{
  GKeyFile *kf = background_config_load_keyfile ();
  g_key_file_set_string (kf, "colors", name, rgba);
  background_config_save (kf);
  g_key_file_unref (kf);
}

/* Add an image entry to the user configuration file */
void
background_config_add_image (const gchar *name, const gchar *path)
{
  GKeyFile *kf = background_config_load_keyfile ();
  g_key_file_set_string (kf, "images", name, path);
  background_config_save (kf);
  g_key_file_unref (kf);
}

/* Remove an entry from both color and image sections in user config file */
void
background_config_remove_key (const gchar *key_name)
{
  GKeyFile *kf = background_config_load_keyfile ();
  
  if (g_key_file_has_key (kf, "colors", key_name, NULL))
    {
      g_key_file_remove_key (kf, "colors", key_name, NULL);
    }
  else if (g_key_file_has_key (kf, "images", key_name, NULL))
    {
      g_key_file_remove_key (kf, "images", key_name, NULL);
    }
  
  background_config_save (kf);
  g_key_file_unref (kf);
}
