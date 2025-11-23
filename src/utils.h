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

#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <glib/gstdio.h>

#include <assert.h>
#include <errno.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define BAR_WIDGET_NAME "ArdesiaBar"

#ifdef _WIN32
#include <cairo-win32.h>
#else
#ifdef __APPLE__
#include <cairo-quartz.h>
#else
#include <cairo-xlib.h>
#endif
#endif

#include "config.h"

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define gettext (char *) (char *)
#endif

/* The gtk builder object of the bar window */
extern GtkBuilder *bar_gtk_builder;

#define PROGRAM_NAME "Ardesia"

/* Color definition in RGB. */
#define BLACK  "000000FF"
#define WHITE  "FFFFFFFF"
#define RED    "FF0000FF"
#define YELLOW "FFFF00FF"
#define GREEN  "00FF00FF"
#define BLUE   "0000FFFF"

/* Struct to store the painted point. */
typedef struct
{
  gdouble x;
  gdouble y;
  gdouble width;
  gdouble pressure;
} AnnotatePoint;

gboolean intersect (GdkRectangle *a, GdkRectangle *b);

GdkPixbuf *take_screenshot_now (void);

void draw_test_square (cairo_t *context);

void draw_test_square_with_color (cairo_t *context, int r, int g, int b);

gchar *get_project_name (void);

void set_project_name (gchar *name);

gchar *get_project_dir (void);

void set_project_dir (gchar *dir);

gchar *get_iwb_filename (void);

void set_iwb_filename (gchar *file);

GSList *get_artifacts (void);

void add_artifact (gchar *path);

void free_artifacts (void);

GtkWidget *get_bar_widget ();

gchar *gdkrgba_to_rgba (GdkRGBA *gdkcolor);

cairo_surface_t *scale_surface (cairo_surface_t *surface,
                                gdouble width,
                                gdouble height);

void cairo_set_source_color_from_string (cairo_t *cr, gchar *color);

void cairo_set_transparent_color (cairo_t *cr);

gdouble get_distance (gdouble x1, gdouble y1, gdouble x2, gdouble y2);

void clear_cairo_context (cairo_t *cr);

gboolean inside_bar_window (gdouble xp, gdouble yp);

GdkRGBA *rgba_to_gdkcolor (gchar *rgb);

gboolean save_pixbuf_on_png_file (GdkPixbuf *pixbuf, const gchar *filename);

void grab_screenshot (void (*screenshot_callback) (GdkPixbuf *));

gchar *get_default_filename (void);

gchar *get_date (void);

gboolean file_exists (gchar *filename);

const gchar *get_home_dir (void);

const gchar *get_desktop_dir (void);

const gchar *get_documents_dir (void);

void rmdir_recursive (gchar *path);

void remove_dir_if_empty (gchar *dir_path);

AnnotatePoint *allocate_point (gdouble x,
                               gdouble y,
                               gdouble width,
                               gdouble pressure);

void send_email (gchar *to,
                 gchar *subject,
                 gchar *body,
                 GSList *attachment_list);

void send_artifacts_with_email (GSList *attachment_list);

void send_trace_with_email (gchar *attachment);

gboolean is_gnome (void);

void xdg_create_desktop_entry (gchar *filename, gchar *type, gchar *name,
                               gchar *icon, gchar *exec);

void xdg_create_link (gchar *src_filename, gchar *dest, gchar *icon);

int g_substrlastpos (const char *str, const char *substr);

gchar *g_substr (const gchar *string, gint start, gint end);

void create_segmentation_fault (void);

void get_surface_size (cairo_surface_t *surface, int *width, int *height);

void get_context_size (cairo_t *cr, int *width, int *height);

void save_cairo_context (cairo_t *cr,
                         gchar *savedir,
                         gchar *category,
                         int index);

#endif // UTILS_H
