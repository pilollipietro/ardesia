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

#include "background_config.h"
#include "background_window.h"
#include "config_path.h"
#include "user_config.h"

/**
 * background_config_get_color_keys:
 *
 * Retrieve all color keys from the user configuration.
 *
 * This function loads the user configuration key file and returns
 * an array of strings containing all keys under the "colors" group.
 *
 * Parameters:
 *   n_colors - output parameter to store the number of keys returned
 *
 * Returns:
 *   A NULL-terminated array of strings containing the color keys.
 *   The caller is responsible for freeing the array and its elements.
 **/
gchar **
background_config_get_color_keys (gsize *n_colors)
{
  GKeyFile *kf   = user_config_load_keyfile ();
  gchar   **keys = g_key_file_get_keys (kf, "colors", n_colors, NULL);
  g_key_file_unref (kf);
  return keys;
}

/**
 * background_config_get_image_keys:
 *
 * Retrieve all image keys from the user configuration.
 *
 * This function loads the user configuration key file and returns
 * an array of strings containing all keys under the "images" group.
 *
 * Parameters:
 *   n_images - output parameter to store the number of keys returned
 *
 * Returns:
 *   A NULL-terminated array of strings containing the image keys.
 *   The caller is responsible for freeing the array and its elements.
 **/
gchar **
background_config_get_image_keys (gsize *n_images)
{
  GKeyFile *kf   = user_config_load_keyfile ();
  gchar   **keys = g_key_file_get_keys (kf, "images", n_images, NULL);
  g_key_file_unref (kf);
  return keys;
}

/**
 * background_config_get_color:
 *
 * Retrieve the color value associated with a given key from
 * the user configuration.
 *
 * Parameters:
 *   key - the color key to lookup in the "colors" group
 *
 * Returns:
 *   A newly allocated string containing the color value, or NULL if not found.
 *   The caller is responsible for freeing the returned string.
 **/
gchar *
background_config_get_color (const gchar *key)
{
  GKeyFile *kf  = user_config_load_keyfile ();
  gchar    *val = g_key_file_get_string (kf, "colors", key, NULL);
  g_key_file_unref (kf);
  return val;
}

/**
 * background_config_get_image:
 *
 * Retrieve the image path associated with a given key from
 * the user configuration.
 *
 * Parameters:
 *   key - the image key to lookup in the "images" group
 *
 * Returns:
 *   A newly allocated string containing the image path, or NULL if not found.
 *   The caller is responsible for freeing the returned string.
 **/
gchar *
background_config_get_image (const gchar *key)
{
  GKeyFile *kf  = user_config_load_keyfile ();
  gchar    *val = g_key_file_get_string (kf, "images", key, NULL);
  g_key_file_unref (kf);
  return val;
}

gboolean
background_config_save (GKeyFile *kf)
{
  gchar *usrfile = get_user_config_file ();
  if (usrfile == NULL)
    {
      g_key_file_unref (kf);
      return FALSE;
    }

  gsize    len   = 0;
  gchar   *data  = g_key_file_to_data (kf, &len, NULL);
  GError  *error = NULL;
  gboolean ok    = g_file_set_contents (usrfile, data, len, &error);
  if (! ok)
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

/**
 * background_config_add_color:
 *
 * Add a color entry to the user configuration file.
 *
 * Parameters:
 *   name - the name/key of the color
 *   rgba - the color value in RGBA format
 **/
void
background_config_add_color (const gchar *name, const gchar *rgba)
{
  GKeyFile *kf = user_config_load_keyfile ();
  g_key_file_set_string (kf, "colors", name, rgba);
  background_config_save (kf);
  g_key_file_unref (kf);
}

/**
 * background_config_filename_to_label:
 *
 * Derive a label from a filename by removing the path and file extension.
 *
 * Parameters:
 *   filename - the input filename
 *
 * Returns:
 *   A newly allocated string containing the label (basename without extension).
 *   The caller is responsible for freeing the returned string.
 **/
gchar *
background_config_filename_to_label (const gchar *filename)
{
  /* Extract basename first */
  gchar *basename = g_path_get_basename (filename);

  /* Find last dot position (if any) */
  const gchar *dot = strrchr (basename, '.');

  gchar *label;
  if (dot != NULL && dot > basename)
    {
      /* Copy up to char before the dot */
      gsize len = (gsize) (dot - basename);
      label     = g_strndup (basename, len);
    }
  else
    {
      /* No dot, duplicate whole basename */
      label = g_strdup (basename);
    }

  g_free (basename);
  return label;
}

/**
 * background_config_add_image:
 *
 * Add an image entry to the user configuration file.
 *
 * Parameters:
 *   name - the name/key of the image
 *   path - the path to the image file
 **/
void
background_config_add_image (const gchar *name, const gchar *path)
{
  GKeyFile *kf = user_config_load_keyfile ();
  g_key_file_set_string (kf, "images", name, path);
  background_config_save (kf);
  g_key_file_unref (kf);
}

/**
 * background_config_remove_key:
 *
 * Remove an entry from both the "colors" and "images" sections
 * in the user configuration file.
 *
 * Parameters:
 *   key_name - the key to remove
 **/
void
background_config_remove_key (const gchar *key_name)
{
  GKeyFile *kf = user_config_load_keyfile ();

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

/**
 * background_config_set_current_background:
 *
 * Persist the currently selected background name/path
 * in the user configuration.
 *
 * Parameters:
 *   background - the name or path of the current background
 **/
void
background_config_set_current_background (const gchar *background)
{
  /* Make sure user config file exists */
  user_config_ensure_file ();

  /* Load the keyfile from user or system */
  GKeyFile *kf = user_config_load_keyfile ();

  /* Store in [background] section under key current_background */
  g_key_file_set_string (kf, "background", "current_background", background);

  /* Save back to user config */
  background_config_save (kf);

  g_key_file_unref (kf);
}

/**
 * background_config_get_current_background:
 *
 * Retrieve the currently selected background from the user
 * configuration.
 *
 * The returned string is a symbolic identifier for the background.
 * It can be one of:
 *
 *  - a predefined background key, e.g. "notebook_paper"
 *  - a symbolic color name, e.g. "white"
 *  - an RGBA hex color string (8 hex digits, alpha last), e.g.
 *    "FF88FF88" (R G B A, each two hex digits)
 *
 * Returns:
 *   A newly allocated NUL-terminated gchar * containing the symbolic
 *   background value. The caller is responsible for freeing the
 *   returned string with g_free (). On error (no setting available)
 *   the function returns NULL.
 */
gchar *
background_config_get_current_background (void)
{
  GKeyFile *kf = user_config_load_keyfile ();

  gchar *val =
      g_key_file_get_string (kf, "background", "current_background", NULL);

  g_key_file_unref (kf);
  return val; /* caller frees */
}

/**
 * background_config_restore_last_background:
 *
 * Restore the last background used by checking the configuration.
 *
 * Returns:
 *   A newly allocated BackgroundRestored struct describing
 *   the last used background.
 *   Returns NULL if no background is found.
 *   The caller is responsible for freeing the struct using
 *   background_restored_free().
 **/
BackgroundRestored *
background_config_restore_last_background (void)
{
  gchar *last_bg = background_config_get_current_background ();
  if (! last_bg)
    return NULL;

  GKeyFile           *kf = user_config_load_keyfile ();
  BackgroundRestored *br = g_new0 (BackgroundRestored, 1);
  if (last_bg == NULL)
    {
      br->type = BACKGROUND_RESTORED_NONE;
      br->value = NULL;
      return br;
    }

  /* check colors section */
  if (g_key_file_has_key (kf, "colors", last_bg, NULL))
    {
      br->type  = BACKGROUND_RESTORED_COLOR;
      br->value = g_key_file_get_string (kf, "colors", last_bg, NULL);
    }
  else if (g_key_file_has_key (kf, "images", last_bg, NULL))
    {
      br->type  = BACKGROUND_RESTORED_IMAGE;
      br->value = g_key_file_get_string (kf, "images", last_bg, NULL);
    }
  else
    {
      br->type  = BACKGROUND_RESTORED_COLOR;
      br->value = g_strdup (last_bg);
    }

  g_free (last_bg);
  g_key_file_unref (kf);
  return br;
}

/**
 * background_restored_free:
 *
 * Free the memory allocated for a BackgroundRestored structure.
 *
 * Parameters:
 *   br - the BackgroundRestored structure to free
 **/
void
background_restored_free (BackgroundRestored *br)
{
  if (! br)
    return;
  g_free (br->value);
  g_free (br);
}
