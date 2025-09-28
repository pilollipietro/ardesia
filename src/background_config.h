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

typedef enum {
  BACKGROUND_RESTORED_NONE,
  BACKGROUND_RESTORED_COLOR,
  BACKGROUND_RESTORED_IMAGE
} BackgroundRestoredType;

typedef struct {
  BackgroundRestoredType type;
  gchar *value; /* color string or filename */
} BackgroundRestored;

/*
 * Load all color keys from config file.
 * Returns an array of key names (NULL terminated).
 * Caller must free with g_strfreev ().
 */
gchar **background_config_get_color_keys (gsize *n_colors);

/*
 * Load all image keys from config file.
 * Returns an array of key names (NULL terminated).
 * Caller must free with g_strfreev ().
 */
gchar **background_config_get_image_keys (gsize *n_images);

/* Get the value for a color key */
gchar *background_config_get_color (const gchar *key);

/* Get the value for an image key */
gchar *background_config_get_image (const gchar *key);

/*
 * Save the provided GKeyFile (with colors/images already set)
 * to the user config file.
 */
gboolean background_config_save (GKeyFile *kf);

/* Add color entry to the user configuration file */
void background_config_add_color (const gchar *name, const gchar *rgba);

/* Add an image entry to the user configuration file */
void background_config_add_image (const gchar *name, const gchar *path);

/* Remove an entry from both color and image sections in user config file */
void background_config_remove_key (const gchar *key_name);

/**
 * Save the currently selected background (color name or image path)
 * into the user configuration.
 */
void background_config_set_current_background (const gchar *background);

/**
 * Retrieve the currently selected background from config.
 * Caller must g_free() the returned string.
 */
gchar *background_config_get_current_background (void);

/* Utility to derive the label from a filename (without path and extension) */
gchar * background_config_filename_to_label (const gchar *filename);

/*
 * Restore last background used.
 */
BackgroundRestored *background_config_restore_last_background (void);

void background_restored_free (BackgroundRestored *br);

#endif /* __BACKGROUND_CONFIG_H__ */
