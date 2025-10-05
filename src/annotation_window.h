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

#ifndef ANNOTATE_H
#define ANNOTATE_H

#include <glib.h>

#include <gtk/gtk.h>

#include <cairo.h>

#ifdef _WIN32
#include <cairo-win32.h>
#include <gdkwin32.h>
#include <winuser.h>
#else
#ifdef __APPLE__
#include <cairo-quartz.h>
#else
#include <cairo-xlib.h>
#endif
#endif

#include "ardesia.h"
#include "recordingstudio.h"

#ifdef _WIN32
#define ANNOTATION_UI_FOLDER "..\\share\\ardesia\\ui"
#define ANNOTATION_UI_FILE   ANNOTATION_UI_FOLDER "\\annotation_window.glade"

#else
#define ANNOTATION_UI_FOLDER PACKAGE_DATA_DIR "/ardesia/ui"
#define ANNOTATION_UI_FILE   ANNOTATION_UI_FOLDER "/annotation_window.glade"
#define RECORDINGSTUDIO_UI_FILE \
  ANNOTATION_UI_FOLDER "/recordingstudio_window.glade"
#define CURSOR_UI_FILE ANNOTATION_UI_FOLDER "/cursor_window.glade"
#define PAPER_BACKGROUND_FILE \
  ANNOTATION_UI_FOLDER "/backgrounds/notebook_paper.png"
#define TRANSPARENT_BACKGROUND_FILE \
  ANNOTATION_UI_FOLDER "/icons/desktop_transparent.png"
#endif

#define MICRO_THICKNESS  3
#define THIN_THICKNESS   6
#define MEDIUM_THICKNESS 12
#define THICK_THICKNESS  18

/* Enumeration containing tools. */
typedef enum
{

  ANNOTATE_PEN,

  ANNOTATE_ERASER,

  ANNOTATE_FILLER,
  ANNOTATE_POINTER
} AnnotatePaintType;

/* Paint context. */
typedef struct
{

  /* Context type. */
  AnnotatePaintType type;

} AnnotatePaintContext;

/* Structure to store the save-point. */
typedef struct _AnnotateSavePoint
{

  /* The file name that represents the save-point. */
  gchar *filename;

} AnnotateSavepoint;

typedef struct
{
  /* List of the coordinates of the last line drawn. */
  GSList *coord_list;

  /* The slave device. */
  GdkDevice *lastslave;

  /* The state. */
  guint state;
} AnnotateDeviceData;

// Background selection
#define BACKGROUND_NONE_SELECTED -1
#define BACKGROUND_MODE_NONE     0
#define BACKGROUND_MODE_FILE     1
#define BACKGROUND_MODE_COLOR    2

typedef struct
{
  gint         mode;
  gchar       *filename;
  gchar       *color;
  GtkToolItem *button;
  gint         index;
  gint         size;
} BackgroundButtonData;

/* Annotation data used by the callbacks. */
typedef struct
{
  gboolean    is_background_visible;
  gboolean    is_text_editor_visible;
  gboolean    is_annotation_visible;
  gboolean    is_window_covering_toolbar;
  gboolean    is_opaque;
  /* Gtkbuilder for annotation window. */
  GtkBuilder *annotation_window_gtk_builder;

  /* Directory where store the save-point. */
  gchar *savepoint_dir;

  /* The annotation window. */
  GtkWidget *annotation_window;

  /* the recording studio child window */
  GtkBuilder *recordingstudio_window_gtk_builder;
  GtkWidget  *recordingstudio_window;

  /* Associated data */
  RecordingStudioData *recordingstudio_options;
  cairo_t             *clapperboard_cairo_context;
  gboolean             is_clapperboard_visible;

  // cursor window
  GtkBuilder *cursor_window_gtk_builder;
  GtkWidget  *cursor_window;
  gboolean    is_cursor_visible;
  gint        cursor_timer;
  gint        cursor_step;

  // background window information
  GtkWidget *background_selection_window;
  GtkWidget *background_selection_container;
  GSList    *background_button_data;
  // last background item selected, -1 if not
  gint       background_button_last_selected;

  gfloat highlighter_multiplier;
  gfloat pen_multiplier;
  gfloat eraser_multiplier;

  /* The cairo context attached to the window. */
  cairo_t *annotation_cairo_context;

  /* The back buffer surface used to do the input shape combine region. */
  cairo_surface_t *annotation_backsurface;

  /* Mouse cursor to be used. */
  GdkCursor *cursor;

  /* Mouse invisible cursor. */
  GdkCursor *invisible_cursor;

  /* List of the savepoint. */
  GSList *savepoint_list;

  /*
   * The index of the position in the save-point list
   * of the current picture shown.
   */
  guint current_save_index;

  /* Hashtable that contains device dependant info. */
  GHashTable *devdatatable;

  /* Paint context for the pen. */
  AnnotatePaintContext *default_pen;

  /* Paint context for the eraser. */
  AnnotatePaintContext *default_eraser;

  /* Paint context for the filler. */
  AnnotatePaintContext *default_filler;

  /* Point to the current context. */
  AnnotatePaintContext *cur_context;

  /*
   * This store the old paint type tool;
   * it is used to switch from/to eraser/pen
   * using a tablet pen.
   */
  AnnotatePaintType old_paint_type;

  /* Tool thickness. */
  gdouble thickness;

  /* Is the rectify mode enabled? */
  gboolean rectify;

  /* Is the roundify mode enabled?*/
  gboolean roundify;

  /* Arrow. */
  gboolean arrow;

  /* Text tool. */
  gboolean text_tool;

  /* Is the cursor grabbed. */
  gboolean is_grabbed;

  /* Is the cursor hidden. */
  gboolean is_cursor_hidden;

  /* Pen color. */
  gchar *color;

  GtkWidget            *font_window;
  PangoFontDescription *font;

  /* monitor */
  Monitor *monitor;

  /* paths */
  GList *paths;
} AnnotateData;

extern AnnotateData *annotation_data;

void initialize_annotation_cairo_context (AnnotateData *data);

GtkWidget *create_annotation_window (Workspace *workspace,
                                     CommandLine *commandline);

void position_annotation_window (int x, int y, int width, int height);

void annotate_init (Monitor *monitor);

void annotation_window_change (int width, int height);

GtkWidget *get_annotation_window (void);

void annotate_push_context (cairo_t *cr);

void annotate_coord_dev_list_free (AnnotateDeviceData *devdata);

void annotate_undo (void);

void annotate_redo (void);

void annotate_quit (void);

void annotate_set_color (gchar *color);

void annotate_set_thickness (gdouble thickness);

gdouble annotate_get_thickness (void);

void annotate_set_rectifier (gboolean rectify);

void annotate_set_rounder (gboolean rounder);

void annotate_set_arrow (gboolean arrow);

void annotate_release_grab (void);

void annotate_acquire_grab (void);

void annotate_clear_screen (void);

void annotate_select_pen (void);

void annotate_select_eraser (void);

void annotate_select_filler (void);

void annotate_add_savepoint (void);

gboolean annotation_window_button_press (GdkEventButton *ev,
                                         AnnotateData *data);

gboolean annotation_window_mouse_move (GdkEventMotion *ev,
                                       AnnotateData *data);

gboolean annotation_window_button_release (GdkEventButton *ev,
                                           AnnotateData *data);

void initialize_font ();

#endif
