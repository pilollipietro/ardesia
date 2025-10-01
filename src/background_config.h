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

#ifndef __BACKGROUND_CONFIG_H__
#define __BACKGROUND_CONFIG_H__

#include <glib.h>

typedef enum
{
  BACKGROUND_RESTORED_NONE,
  BACKGROUND_RESTORED_COLOR,
  BACKGROUND_RESTORED_IMAGE
} BackgroundRestoredType;

typedef struct
{
  BackgroundRestoredType type;
  gchar                 *value; /* color string or filename */
} BackgroundRestored;

gchar **background_config_get_color_keys (gsize *n_colors);

gchar **background_config_get_image_keys (gsize *n_images);

gchar *background_config_get_color (const gchar *key);

gchar *background_config_get_image (const gchar *key);

void background_config_add_color (const gchar *name, const gchar *rgba);

void background_config_add_image (const gchar *name, const gchar *path);

void background_config_remove_key (const gchar *key_name);

void background_config_set_current_background (const gchar *background);

gchar *background_config_get_current_background (void);

gchar *background_config_filename_to_label (const gchar *filename);

BackgroundRestored *background_config_restore_last_background (void);

void background_restored_free (BackgroundRestored *br);

#endif /* __BACKGROUND_CONFIG_H__ */
