/*
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2025 <il tuo nome o mail>
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
 */

#ifndef ANNOTATION_CONFIG_H
#define ANNOTATION_CONFIG_H

#include "annotation_window.h" /* for AnnotateData */

/* Save the current annotation state to user config */
void annotation_config_save_state (AnnotateData *data);

/* Load the annotation state from config and apply to AnnotateData */
void annotation_config_load_state (AnnotateData *data);

gchar *annotate_thickness_pixel_to_label (gdouble thickness_pixel);
#endif /* ANNOTATION_CONFIG_H */
