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

/** Widget for text insertion */

#ifndef __TEXT_WINDOW_H
#define __TEXT_WINDOW_H

#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <cairo.h>

#ifdef _WIN32
#include <cairo-win32.h>
#else
#ifdef __APPLE__
#include <cairo-quartz.h>
#else
#include <cairo-xlib.h>
#endif
#endif

#define TEXT_CURSOR_WIDTH 4

#ifdef _WIN32
#define TEXT_MOUSE_EVENTS                                                      \
  (GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | \
   GDK_PROXIMITY_IN | GDK_PROXIMITY_OUT | GDK_MOTION_NOTIFY | GDK_BUTTON_PRESS)
#endif

/*
 * Per character settings so we can extend the application to have
 * multiple characteristics.
 */
typedef struct
{

  gdouble x;

  gdouble y;

  gchar *color;

  gchar *background_color;

  gint pen_width; // width of normal text

  gchar *character;

  gchar *font_family; // e.g serif

  guint font_size;

  guint font_weight;

  gboolean bold;

  gboolean italics;

  gboolean subscript;

  gboolean superscript;

  cairo_text_extents_t extents;

  PangoFontDescription *pango_font_description;

  gint baseline; // to be useful needs to be divided by PANGO_SCALE

  gint text_width;

  gint text_height;

  gdouble visual_thickness;

  PangoRectangle ink_rect;

} CharInfo;

typedef struct
{

  gdouble x;

  gdouble y;

} Pos;

typedef struct
{

  /* Gtkbuilder to build the window. */
  GtkBuilder *text_window_gtk_builder;

  GtkWidget *window;

  GPid virtual_keyboard_pid;

  cairo_t *cr;

  Pos *pos;

  GSList *letterlist;

  gchar *color;

  gint pen_width;

  gdouble max_font_height;

  cairo_text_extents_t extents;

  gint timer;

  gboolean blink_show;

  gdouble font_ascent;
  gdouble font_descent;

} TextData;

/* Option for text config */
typedef struct
{
  gchar *fontfamily;
  gint   leftmargin;
  gint   tabsize;
  gint   start_x; // where first character will go
} TextConfig;

extern TextData   *text_data;
extern TextConfig *text_config;

void destroy_text_properties (gpointer data);

TextConfig *create_text_config (void);

void start_blink_cursor (void);

void stop_blink_cursor (void);

void save_text (void);

/* Start text widget. */
void start_text_widget (GtkWidget *parent, gchar *color, gint thickness);

/* Stop text widget. */
void stop_text_widget (void);

gdouble calculate_visual_thickness (gdouble pen_width, gint font_size);

#endif //__TEXT_WINDOW_H
