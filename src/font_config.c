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
#include <pango/pango.h>

#include "config_path.h"
#include "font_config.h"
#include "user_config.h"

#define FONT_SECTION    "font"
#define FONT_KEY_FAMILY "family"
#define FONT_KEY_SIZE   "size"
#define FONT_KEY_STYLE  "style"

/* Load font settings from configuration (user first, then system).
 * Returns a newly-allocated PangoFontDescription or NULL if none found.
 */
PangoFontDescription *
font_config_load (void)
{
  GKeyFile *kf = user_config_load_keyfile ();
  gchar *family;
  family = g_key_file_get_string (kf, FONT_SECTION, FONT_KEY_FAMILY, NULL);

  gint   size;
  size = g_key_file_get_integer (kf, FONT_SECTION, FONT_KEY_SIZE, NULL);

  gchar *style = g_key_file_get_string (kf, FONT_SECTION, FONT_KEY_STYLE, NULL);

  PangoFontDescription *desc = NULL;
  if (family != NULL)
    {
      desc = pango_font_description_new ();
      pango_font_description_set_family (desc, family);
      if (size > 0)
        pango_font_description_set_size (desc, size * PANGO_SCALE);

      if (style != NULL)
        {
          if (g_ascii_strcasecmp (style, "bolditalic") == 0)
            {
              pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
              pango_font_description_set_style (desc, PANGO_STYLE_ITALIC);
            }
          else if (g_ascii_strcasecmp (style, "bold") == 0)
            {
              pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
            }
          else if (g_ascii_strcasecmp (style, "italic") == 0)
            {
              pango_font_description_set_style (desc, PANGO_STYLE_ITALIC);
            }
        }
    }

  g_free (family);
  g_free (style);
  g_key_file_unref (kf);
  return desc;
}

/* Save font settings into the user config file.
 * This will ensure the user config exists (copying the system file
 * if necessary) before writing.
 */
void
font_config_save (const PangoFontDescription *font_desc)
{
  if (font_desc == NULL)
    return;

  gchar *user = get_user_file (); /* ensures user file exists */
  if (user == NULL)
    return;

  GKeyFile *kf = g_key_file_new ();
  /* Load existing user file if any; ignore errors and overwrite keys */
  g_key_file_load_from_file (kf, user, G_KEY_FILE_KEEP_COMMENTS, NULL);

  const gchar *family = pango_font_description_get_family (font_desc);
  gint  size = pango_font_description_get_size (font_desc) / PANGO_SCALE;

  PangoStyle  pstyle    = pango_font_description_get_style (font_desc);
  PangoWeight pweight   = pango_font_description_get_weight (font_desc);
  gchar      *style_buf = NULL;
  if (pweight >= PANGO_WEIGHT_BOLD && pstyle == PANGO_STYLE_ITALIC)
    style_buf = g_strdup ("bolditalic");
  else if (pweight >= PANGO_WEIGHT_BOLD)
    style_buf = g_strdup ("bold");
  else if (pstyle == PANGO_STYLE_ITALIC)
    style_buf = g_strdup ("italic");
  else
    style_buf = g_strdup ("normal");

  g_key_file_set_string (kf,
                        FONT_SECTION,
			FONT_KEY_FAMILY,
			family ? family : "Sans");

  g_key_file_set_integer (kf,
                          FONT_SECTION,
			  FONT_KEY_SIZE,
			  size > 0 ? size : 32);

  g_key_file_set_string (kf, FONT_SECTION, FONT_KEY_STYLE, style_buf);

  gsize   len   = 0;
  gchar  *data  = g_key_file_to_data (kf, &len, NULL);
  GError *error = NULL;
  if (! g_file_set_contents (user, data, len, &error))
    {
      g_warning ("Could not write font config to %s: %s", user,
                 error ? error->message : "unknown error");
      if (error)
        g_error_free (error);
    }

  g_free (data);
  g_free (style_buf);
  g_key_file_unref (kf);
  g_free (user);
}
