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

#include <glib.h>
#include "config.h"
#include "font_config.h"
#include "user_config.h"

#define CONFIG_FILE_NAME "ardesia.conf"
#define CONFIG_SECTION   "font"

static gchar *
get_config_file (void)
{
  /* build user config path first */
  return g_build_filename (g_get_user_config_dir (), CONFIG_FILE_NAME, NULL);
}

PangoFontDescription *
font_config_load (void)
{
  GKeyFile *key_file = g_key_file_new ();
  gchar *user_file = get_config_file ();
  gchar *system_file = g_build_filename (ARDESIA_SYSCONFDIR,
                                         CONFIG_FILE_NAME, NULL);

  gboolean loaded = FALSE;

  if (g_file_test (user_file, G_FILE_TEST_EXISTS))
    loaded = g_key_file_load_from_file (key_file, user_file,
                                        G_KEY_FILE_NONE, NULL);
  if (!loaded && g_file_test (system_file, G_FILE_TEST_EXISTS))
    loaded = g_key_file_load_from_file (key_file, system_file,
                                        G_KEY_FILE_NONE, NULL);

  g_free (user_file);
  g_free (system_file);

  if (!loaded)
    {
      g_key_file_unref (key_file);
      return NULL;
    }

  gchar *family = g_key_file_get_string (key_file, CONFIG_SECTION, "family", NULL);
  gint size = g_key_file_get_integer (key_file, CONFIG_SECTION, "size", NULL);
  gchar *style = g_key_file_get_string (key_file, CONFIG_SECTION, "style", NULL);

  /* build font description */
  PangoFontDescription *font = NULL;
  if (family)
    {
      gchar *font_str = g_strdup_printf ("%s %d %s",
                                         family ? family : "Sans",
                                         size > 0 ? size : 32,
                                         style ? style : "");
      font = pango_font_description_from_string (font_str);
      g_free (font_str);
    }

  g_free (family);
  g_free (style);
  g_key_file_unref (key_file);
  return font;
}

void
font_config_save (const PangoFontDescription *font)
{
  /* Ensure user file exists first */
  user_config_ensure_file ();

  gchar *user_file = get_config_file ();
  GKeyFile *key_file = g_key_file_new ();
  g_key_file_load_from_file (key_file, user_file, G_KEY_FILE_NONE, NULL);

  const gchar *family = pango_font_description_get_family (font);
  int size = pango_font_description_get_size (font) / PANGO_SCALE;

  /* style as string is a bit tricky but you can build it */
  gchar *style_str = g_strdup (pango_font_description_to_string (font)); 
  /* optional: parse style separately */

  g_key_file_set_string (key_file, CONFIG_SECTION, "family", family);
  g_key_file_set_integer (key_file, CONFIG_SECTION, "size", size);
  /* if you have style string separately, set it here */

  GError *error = NULL;
  gsize length = 0;
  gchar *data = g_key_file_to_data (key_file, &length, NULL);
  g_file_set_contents (user_file, data, length, &error);
  if (error)
    {
      g_warning ("Cannot save font config: %s", error->message);
      g_clear_error (&error);
    }

  g_free (data);
  g_free (style_str);
  g_free (user_file);
  g_key_file_unref (key_file);
}


