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

#include "annotation_config.h"
#include "config_path.h"
#include "user_config.h"
#include <glib.h>
#include <glib/gstdio.h>

/* Section name in config file */
#define ANNOTATION_SECTION "annotation"

/**
 * annotate_paint_type_to_string:
 * @data: The current #AnnotateData structure.
 *
 * Convert the currently selected paint context type into a string
 * suitable for storage in the configuration file.
 *
 * Returns: (not nullable) A string representing the current tool
 *          ("pen", "highlighter", "eraser", "filler", or "pointer").
 **/
static const gchar *
annotate_paint_type_to_string (AnnotateData *data)
{
  AnnotatePaintType ctx_type = data->cur_context->type;
  switch (ctx_type)
    {
    case ANNOTATE_PEN:
      if (is_selected_color_opaque ())
        {
          return "pen";
        }
      else
        {
          return "highlighter";
        }
    case ANNOTATE_ERASER:
      return "eraser";
    case ANNOTATE_FILLER:
      return "filler";
    case ANNOTATE_POINTER:
      return "pointer";
    default:
      return "pen";
    }
}

/**
 * annotate_set_context_from_string:
 * @data: The #AnnotateData structure to update.
 * @paint_type: The string representation of a paint tool.
 *
 * Converts a string from the configuration file into the corresponding
 * #AnnotatePaintContext.
 *
 * Returns: (nullable) The corresponding #AnnotatePaintContext,
 *          or %NULL for "pointer". Defaults to pen if unknown.
 **/
static AnnotatePaintContext *
annotate_set_context_from_string (AnnotateData *data, const gchar *paint_type)
{
  if (paint_type == NULL)
    return data->default_pen;

  if ((g_ascii_strcasecmp (paint_type, "pen") == 0) ||
      (g_ascii_strcasecmp (paint_type, "highlighter") == 0))
    return data->default_pen;
  if (g_ascii_strcasecmp (paint_type, "eraser") == 0)
    return data->default_eraser;
  if (g_ascii_strcasecmp (paint_type, "filler") == 0)
    return data->default_filler;
  if (g_ascii_strcasecmp (paint_type, "pointer") == 0)
    return NULL;

  /* fallback */
  return data->default_pen;
}

/**
 * annotate_thickness_pixel_to_label:
 * @thickness_pixel: The pen thickness in pixels.
 *
 * Converts a numeric pen thickness into a human-readable string label.
 *
 * Returns: (not nullable) A string label:
 *          "micro", "thin", "medium", or "thick".
 **/
gchar *
annotate_thickness_pixel_to_label (gdouble thickness_pixel)
{
  gint thickness_int = (gint) thickness_pixel;
  if (thickness_int == MICRO_THICKNESS)
    {
      return "micro";
    }
  if (thickness_pixel == THIN_THICKNESS)
    {
      return "thin";
    }
  if (thickness_int == MEDIUM_THICKNESS)
    {
      return "medium";
    }
  if (thickness_int == THICK_THICKNESS)
    {
      return "thick";
    }
  return "medium";
}

/**
 * annotate_thickness_label_to_pixel:
 * @thickness_label: The string label representing pen thickness.
 *
 * Converts a human-readable thickness label into a numeric pixel value.
 *
 * Returns: The corresponding thickness in pixels. Defaults to MEDIUM_THICKNESS
 *          if unknown.
 **/
gdouble
annotate_thickness_label_to_pixel (gchar *thickness_label)
{
  if (g_ascii_strcasecmp (thickness_label, "micro") == 0)
    {
      return MICRO_THICKNESS;
    }
  if (g_ascii_strcasecmp (thickness_label, "thin") == 0)
    {
      return THIN_THICKNESS;
    }
  if (g_ascii_strcasecmp (thickness_label, "medium") == 0)
    {
      return MEDIUM_THICKNESS;
    }
  if (g_ascii_strcasecmp (thickness_label, "thick") == 0)
    {
      return THICK_THICKNESS;
    }
  return MEDIUM_THICKNESS;
}

/**
 * annotation_config_save_state:
 * @data: The #AnnotateData structure containing the current annotation state.
 *
 * Saves the current annotation settings to the user configuration file.
 * This includes color, arrow mode, thickness, rectify/roundify options,
 * text tool state, and the currently selected paint tool.
 *
 * Notes:
 * - The user configuration file is ensured to exist before saving.
 * - Thickness and tool type are converted to string representations for
 *   storage.
 **/
void
annotation_config_save_state (AnnotateData *data)
{
  if (! data)
    return;

  gchar *userfile = get_user_config_file ();
  if (userfile == NULL)
    return;

  GKeyFile *kf = g_key_file_new ();
  g_key_file_load_from_file (kf, userfile, G_KEY_FILE_NONE, NULL);

  /* Save basic fields. Adjust keys as needed */
  g_key_file_set_string (kf, ANNOTATION_SECTION, "color", data->color);
  g_key_file_set_boolean (kf, ANNOTATION_SECTION, "arrow", data->arrow);

  const gchar *thickness_label;
  thickness_label = annotate_thickness_pixel_to_label (data->thickness);
  g_key_file_set_string (kf, ANNOTATION_SECTION, "thickness", thickness_label);

  g_key_file_set_boolean (kf, ANNOTATION_SECTION, "rectify", data->rectify);
  g_key_file_set_boolean (kf, ANNOTATION_SECTION, "roundify", data->roundify);

  g_key_file_set_boolean (kf, ANNOTATION_SECTION, "text_tool", data->text_tool);

  /* Save tool as a string key (pen, eraser, filler, pointer). */
  const gchar *ctx_str = annotate_paint_type_to_string (data);

  g_key_file_set_string (kf, ANNOTATION_SECTION, "tool", ctx_str);

  gsize  length;
  gchar *content = g_key_file_to_data (kf, &length, NULL);
  g_file_set_contents (userfile, content, length, NULL);

  g_free (content);
  g_free (userfile);
  g_key_file_unref (kf);
}

/**
 * config_load_boolean:
 * @key_file: The #GKeyFile to read from.
 * @key: The key of the boolean value to load.
 * @target_variable: (out) Pointer to a gboolean variable to update.
 *
 * Safely loads a boolean value from a key file. If the key exists and
 * is valid, updates @target_variable. Otherwise leaves it unchanged.
 **/
static void
config_load_boolean (GKeyFile     *key_file,
                      const gchar  *key,
                      gboolean     *target_variable)
{
  GError  *error = NULL;
  gboolean value;

  value = g_key_file_get_boolean (key_file, ANNOTATION_SECTION, key, &error);
  if (! error)
    {
      *target_variable = value;
    }
  g_clear_error (&error);
}

/**
 * annotation_config_load_state:
 * @annotation_data: The #AnnotateData structure to populate with
 *                   loaded settings.
 *
 * Loads annotation settings from the configuration file.
 * This includes color, arrow mode, pen thickness, tool selection,
 * rectify/roundify flags, and text tool state.
 *
 * Notes:
 * - Only keys present in the configuration file are applied;
 *   missing keys leave the current values unchanged.
 * - Rectify and roundify are mutually exclusive; if both are set to TRUE,
 *   roundify is forced to FALSE.
 * - Uses #config_load_boolean helper to safely load boolean values.
 **/
void
annotation_config_load_state (AnnotateData *annotation_data)
{
  g_return_if_fail (annotation_data != NULL);

  gchar *cfgfile = get_config_file ();
  if (! cfgfile)
    {
      return;
    }

  GKeyFile *kf    = g_key_file_new ();
  GError   *error = NULL;

  if (! g_key_file_load_from_file (kf, cfgfile, G_KEY_FILE_NONE, &error))
    {
      g_warning ("Failed to load config file '%s': %s",
                 cfgfile,
                 error ? error->message : "unknown error");
      g_clear_error (&error);
      g_key_file_unref (kf);
      g_free (cfgfile);
      return;
    }

  gchar *color = g_key_file_get_string (kf, ANNOTATION_SECTION, "color", NULL);
  if (color)
    {
      annotate_set_color (color);
      g_free (color);
    }

  gchar *thickness = g_key_file_get_string (kf,
                                            ANNOTATION_SECTION,
                                            "thickness",
                                            NULL);
  if (thickness)
    {
      annotation_data->thickness =
          annotate_thickness_label_to_pixel (thickness);

      g_free (thickness);
    }

  gchar *tool_str = g_key_file_get_string (kf,
                                           ANNOTATION_SECTION,
                                           "tool",
                                           NULL);
  if (tool_str)
    {
      annotation_data->cur_context =
          annotate_set_context_from_string (annotation_data, tool_str);

      g_free (tool_str);
    }

  /* Load all boolean values safely using the helper function */
  config_load_boolean (kf, "arrow", &annotation_data->arrow);
  config_load_boolean (kf, "rectify", &annotation_data->rectify);
  config_load_boolean (kf, "roundify", &annotation_data->roundify);
  config_load_boolean (kf, "text_tool", &annotation_data->text_tool);

  /* Ensure mutual exclusion for rectify/roundify */
  if (annotation_data->rectify && annotation_data->roundify)
    {
      g_warning ("Both rectify and roundify set; forcing roundify to FALSE");
      annotation_data->roundify = FALSE;
    }

  g_key_file_unref (kf);
  g_free (cfgfile);
}
