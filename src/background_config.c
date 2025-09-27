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
#include "config_path.h"
#include <gio/gio.h>
#include <glib/gstdio.h>

/* Internal: load a GKeyFile either from user or system config */
static GKeyFile *
background_config_load_keyfile (void)
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

gchar **
background_config_get_color_keys (gsize *n_colors)
{
  GKeyFile *kf   = background_config_load_keyfile ();
  gchar   **keys = g_key_file_get_keys (kf, "colors", n_colors, NULL);
  g_key_file_unref (kf);
  return keys;
}

gchar **
background_config_get_image_keys (gsize *n_images)
{
  GKeyFile *kf   = background_config_load_keyfile ();
  gchar   **keys = g_key_file_get_keys (kf, "images", n_images, NULL);
  g_key_file_unref (kf);
  return keys;
}

gchar *
background_config_get_color (const gchar *key)
{
  GKeyFile *kf  = background_config_load_keyfile ();
  gchar    *val = g_key_file_get_string (kf, "colors", key, NULL);
  g_key_file_unref (kf);
  return val;
}

gchar *
background_config_get_image (const gchar *key)
{
  GKeyFile *kf  = background_config_load_keyfile ();
  gchar    *val = g_key_file_get_string (kf, "images", key, NULL);
  g_key_file_unref (kf);
  return val;
}

gboolean
background_config_save (GKeyFile *kf)
{
  /*
   * For writes ensure the user file exists first (this will copy system
   * config to the user's location if necessary), then write the content.
   */
  gchar *usrfile = get_user_file ();
  if (usrfile == NULL)
    {
      g_key_file_unref (kf);
      return FALSE;
    }

  gsize    len  = 0;
  gchar   *data = g_key_file_to_data (kf, &len, NULL);
  GError  *error = NULL;
  gboolean ok   = g_file_set_contents (usrfile, data, len, &error);
  if (!ok)
    {
      g_warning ("Could not write config %s: %s", usrfile,
                 error ? error->message : "unknown error");
      if (error)
        g_error_free (error);
    }

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

