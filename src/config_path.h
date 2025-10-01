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
 * config_path.h
 *
 * Helpers to obtain configuration file paths.
 * Returned strings are newly allocated and must be freed by the caller.
 */
#ifndef CONFIG_PATH_H
#define CONFIG_PATH_H

#include <glib.h>

/* Return system config file path (e.g. /etc/ardesia.conf). */
gchar *get_system_config_file (void);

/* Return user config file path (e.g. $XDG_CONFIG_HOME/ardesiarc). */
gchar *get_user_config_file (void);

/*
 * Return config path for reading:
 * user path if it exists, otherwise the system
 */
gchar *get_config_file (void);

/* Return config path for writing: ensure user file exists then return it */
gchar *get_user_file (void);

#endif /* CONFIG_PATH_H */
