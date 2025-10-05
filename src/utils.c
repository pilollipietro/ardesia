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

#include <assert.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <math.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "utils.h"
#ifdef _WIN32
#include "windows_utils.h"
#endif
#include "annotation_window.h"
#include "ardesia.h"
#include "bar.h"
#include "workspace.h"

/** The list of the artefacts created in the current session. */
static GSList *artifacts = NULL;

/** Builder for the bar window. */
GtkBuilder *bar_gtk_builder = NULL;

/**
 * intersect:
 * @a: First rectangle.
 * @b: Second rectangle.
 *
 * Returns TRUE if the two rectangles intersect, FALSE otherwise.
 */
gboolean
intersect (GdkRectangle *a, GdkRectangle *b)
{
  return !(a->x + a->width < b->x  ||
           a->x > b->x + b->width  ||
           a->y + a->height < b->y ||
           a->y > b->y + b->height);
}

/**
 * draw_test_square:
 * @context: Cairo context to draw into.
 *
 * Draws a white test square in the Cairo context.
 */
void
draw_test_square (cairo_t *context)
{
  cairo_t *cr = context;
  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width (cr, 10.0);
  cairo_set_source_rgb (cr, 255, 255, 255);
  cairo_rectangle (cr, 10, 10, 100, 100);
  cairo_stroke (cr);
  cairo_restore (cr);
}

/**
 * draw_test_square_with_color:
 * @context: Cairo context to draw into.
 * @r: Red component (0-255).
 * @g: Green component (0-255).
 * @b: Blue component (0-255).
 *
 * Draws a colored test square in the Cairo context.
 */
void
draw_test_square_with_color (cairo_t *context, int r, int g, int b)
{
  cairo_t *cr = context;
  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width (cr, 10.0);
  cairo_set_source_rgb (cr, r, g, b);
  cairo_rectangle (cr, 10, 10, 100, 100);
  cairo_stroke (cr);
  cairo_restore (cr);
}

/**
 * get_project_name:
 *
 * Returns the name of the current project.
 *
 * Returns: pointer to the project name string.
 */
gchar *
get_project_name (void)
{
  return workspace->project_name;
}

/**
 * set_project_name:
 * @name: New project name.
 *
 * Sets the name of the current project.
 */
void
set_project_name (gchar *name)
{
  workspace->project_name = name;
}

/**
 * get_project_dir:
 *
 * Returns the directory of the current project.
 *
 * Returns: pointer to the project directory string.
 */
gchar *
get_project_dir (void)
{
  return workspace->project_dir;
}

/**
 * set_project_dir:
 * @dir: New project directory.
 *
 * Sets the directory of the current project.
 */
void
set_project_dir (gchar *dir)
{
  workspace->project_dir = dir;
}

/**
 * get_iwb_filename:
 *
 * Returns the iwb file of the current project.
 *
 * Returns: pointer to the iwb filename string.
 */
gchar *
get_iwb_filename (void)
{
  return workspace->iwb_filename;
}

/**
 * set_iwb_filename:
 * @file: New iwb file name.
 *
 * Sets the iwb file of the current project.
 */
void
set_iwb_filename (gchar *file)
{
  workspace->iwb_filename = file;
}

/**
 * get_artifacts:
 *
 * Returns the list of paths of the artefacts created in the session.
 *
 * Returns: GSList of artefact paths.
 */
GSList *
get_artifacts (void)
{
  return artifacts;
}

/**
 * add_artifact:
 * @path: Path of the artefact to add.
 *
 * Adds the path of an artefact to the session's artefact list.
 */
void
add_artifact (gchar *path)
{
  gchar *copied_path = g_strdup_printf ("%s", path);
  artifacts          = g_slist_prepend (artifacts, copied_path);
}

/**
 * free_artifacts:
 *
 * Frees the artefact list structure created in the session.
 */
void
free_artifacts (void)
{
  g_slist_foreach (artifacts, (GFunc) g_free, NULL);
}

/**
 * get_bar_widget:
 *
 * Returns the bar window widget.
 *
 * Returns: GtkWidget pointer for the bar.
 */
GtkWidget *
get_bar_widget (void)
{
  return GTK_WIDGET (gtk_builder_get_object (bar_gtk_builder, BAR_WIDGET_NAME));
}

/**
 * get_distance:
 * @x1: X coordinate of the first point.
 * @y1: Y coordinate of the first point.
 * @x2: X coordinate of the second point.
 * @y2: Y coordinate of the second point.
 *
 * Returns the Euclidean distance between two points.
 */
gdouble
get_distance (gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
  gdouble x_delta  = fabs (x2 - x1);
  gdouble y_delta  = fabs (y2 - y1);
  gdouble quad_sum = pow (x_delta, 2) + pow (y_delta, 2);
  return sqrt (quad_sum);
}

/**
 * gdkcolor_to_rgb:
 * @gdkcolor: GdkRGBA pointer.
 *
 * Converts a GdkRGBA color to an RGB string (hex format, e.g. "FF0000").
 *
 * Returns: newly-allocated string representing RGB color.
 */
gchar *
gdkcolor_to_rgb (GdkRGBA *gdkcolor)
{
  gchar *ret_str = g_strdup_printf ("%02X%02X%02X",
                                    (int) gdkcolor->red / 255,
                                    (int) gdkcolor->green / 255,
                                    (int) gdkcolor->blue / 255);

  return ret_str;
}

/**
 * gdkrgba_to_rgba:
 * @gdkcolor: GdkRGBA pointer.
 *
 * Converts a GdkRGBA color to an RGBA string (hex format, e.g. "FF0000FF").
 *
 * Returns: newly-allocated string representing RGBA color.
 */
gchar *
gdkrgba_to_rgba (GdkRGBA *gdkcolor)
{
  gchar *ret_str = g_strdup_printf ("%02X%02X%02X%02X",
                                    (int) (gdkcolor->red * 255),
                                    (int) (gdkcolor->green * 255),
                                    (int) (gdkcolor->blue * 255),
                                    (int) (gdkcolor->alpha * 255));

  return ret_str;
}

/*
 * Take an rgb or a rgba string and return the pointer to the allocated GdkColor
 * neglecting the alpha channel; the gtkColor does not support the rgba color.
 */
GdkRGBA *
rgba_to_gdkcolor (gchar *rgba)
{
  assert (rgba != NULL);
  assert (strlen (rgba) == 8);

  int br = 0, bg = 0, bb = 0, ba = 0;
  sscanf (rgba, "%02X%02X%02X%02X", &br, &bg, &bb, &ba);
  GdkRGBA *gdkcolor = g_malloc ((gsize) sizeof (GdkRGBA));
  gdkcolor->red     = (double) br / 255;
  gdkcolor->green   = (double) bg / 255;
  gdkcolor->blue    = (double) bb / 255;
  gdkcolor->alpha   = (double) ba / 255;
  return gdkcolor;
}

/**
 * clear_cairo_context:
 * @cr: Cairo context to clear.
 *
 * Clears the entire Cairo context to transparent.
 */
void
clear_cairo_context (cairo_t *cr)
{
  if (cr)
    {
      cairo_save (cr); // without this the tool icon disappears
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_restore (cr);
    }
}

/**
 * scale_surface:
 * @surface: Source Cairo surface.
 * @width: Desired width.
 * @height: Desired height.
 *
 * Scales the given surface to the requested width and height.
 *
 * Returns: Newly allocated cairo_surface_t with scaled content.
 */
cairo_surface_t *
scale_surface (cairo_surface_t *surface, gdouble width, gdouble height)
{
  gdouble old_width  = cairo_image_surface_get_width (surface);
  gdouble old_height = cairo_image_surface_get_height (surface);

  cairo_surface_t *new_surface =
      cairo_surface_create_similar (surface,
                                    CAIRO_CONTENT_COLOR_ALPHA,
                                    width,
                                    height);

  cairo_t *cr = cairo_create (new_surface);

  /* Scale *before* setting the source surface (1) */
  cairo_scale (cr, width / old_width, height / old_height);
  cairo_set_source_surface (cr, surface, 0, 0);

  /* To avoid getting the edge pixels blended with 0 alpha, which would
   * occur with the default EXTEND_NONE. Use EXTEND_PAD for 1.2 or newer (2)
   */
  cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REFLECT);

  /* Replace the destination with the source instead of overlaying. */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  /* Do the actual drawing. */
  cairo_paint (cr);

  cairo_destroy (cr);

  return new_surface;
}

/**
 * cairo_set_source_color_from_string:
 * @cr: Cairo context.
 * @color: RGBA string (e.g., "FF0000FF").
 *
 * Sets the source color of the Cairo context using a hex RGBA string.
 */
void
cairo_set_source_color_from_string (cairo_t *cr, gchar *color)
{
  if (cr)
    {
      guint r, g, b, a;
      sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);

      cairo_set_source_rgba (cr, 1.0 * r / 255,
                             1.0 * g / 255,
                             1.0 * b / 255,
                             1.0 * a / 255);
    }
}

/**
 * save_pixbuf_on_png_file:
 * @pixbuf: GdkPixbuf to save.
 * @filename: Destination filename.
 *
 * Saves the GdkPixbuf content into a PNG file.
 *
 * Returns: TRUE on success.
 */
gboolean
save_pixbuf_on_png_file (GdkPixbuf *pixbuf, const gchar *filename)
{
  gint width  = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         width,
                                                         height);

  cairo_t *cr = cairo_create (surface);
  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);

  /* Write to the png surface. */
  cairo_surface_write_to_png (surface, filename);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return TRUE;
}

/**
 * is_bar_window_over_annotation_window:
 *
 * Checks if the bar window overlaps the annotation window.
 *
 * Returns: TRUE if overlapping, FALSE otherwise.
 */
gboolean
is_bar_window_over_annotation_window (void)
{
  GtkWidget *bar = get_bar_widget ();
  gint       x, y, width, height;

  gtk_window_get_position (GTK_WINDOW (bar), &x, &y);
  gtk_window_get_size (GTK_WINDOW (bar), &width, &height);

  GdkRectangle *rB = g_new (GdkRectangle, 1);
  rB->x            = x;
  rB->y            = y;
  rB->width        = width;
  rB->height       = height;

  GtkWidget *widget    = annotation_data->annotation_window;
  gint       ann_width = 0, ann_height = 0, ann_x = 0, ann_y = 0;
  gtk_window_get_position (GTK_WINDOW (widget), &ann_x, &ann_y);
  gtk_window_get_size (GTK_WINDOW (widget), &ann_width, &ann_height);

  GdkRectangle *rA = g_new (GdkRectangle, 1);
  rA->x            = ann_x;
  rA->y            = ann_y;
  rA->width        = ann_width;
  rA->height       = ann_height;

  gboolean result = intersect (rA, rB);
  g_free (rA);
  g_free (rB);
  return result;
}

/**
 * take_screenshot_now:
 *
 * Takes a screenshot of the annotation window, temporarily hiding the bar.
 *
 * Returns: GdkPixbuf with the screenshot.
 */
GdkPixbuf *
take_screenshot_now (void)
{
  GtkWidget *widget = annotation_data->annotation_window;

  GtkWidget *bar_widget = GTK_WIDGET (get_bar_widget ());
  gdouble    opacity    = gtk_widget_get_opacity (bar_widget);
  gtk_widget_set_opacity (bar_widget, 0.0);

  /*
   * This loop is the only way to force a synchronous wait for the UI
   * to update before proceeding in a synchronous function.
   */
  while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }

  gint ann_width = 0, ann_height = 0, ann_x = 0, ann_y = 0;
  gtk_window_get_position (GTK_WINDOW (widget), &ann_x, &ann_y);
  gtk_window_get_size (GTK_WINDOW (widget), &ann_width, &ann_height);

  GdkWindow *root_window = gdk_get_default_root_window ();

  GdkPixbuf *snapshot = gdk_pixbuf_get_from_window (root_window,
                                                    ann_x,
                                                    ann_y,
                                                    ann_width,
                                                    ann_height);
  gtk_widget_set_opacity (bar_widget, opacity);
  return snapshot;
}

/**
 * grab_screenshot:
 * @screenshot_callback: Callback function to handle the captured screenshot.
 *
 * Grabs a screenshot and passes it to the callback.
 */
void
grab_screenshot (void (*screenshot_callback) (GdkPixbuf *))
{
  g_debug ("grab screenshot\n");
  GdkPixbuf *buffer = take_screenshot_now ();
  screenshot_callback (buffer);
}

/**
 * inside_bar_window:
 * @xp: X coordinate.
 * @yp: Y coordinate.
 *
 * Returns TRUE if the point is inside the bar window.
 */
gboolean
inside_bar_window (gdouble xp, gdouble yp)
{
  gint       x          = 0;
  gint       y          = 0;
  gint       width      = 0;
  gint       height     = 0;
  gdouble    xd         = 0;
  gdouble    yd         = 0;
  GtkWindow *bar_window = GTK_WINDOW (get_bar_widget ());
  gtk_window_get_position (bar_window, &x, &y);
  xd = (gdouble) x;
  yd = (gdouble) y;

  gtk_window_get_size (bar_window, &width, &height);

  if ((yp >= yd) && (yp < yd + height))
    {

      if ((xp >= xd) && (xp < xd + width))
        {
          return TRUE;
        }
    }

  return FALSE;
}

/**
 * get_date:
 *
 * Returns the current date in a printable format.
 *
 * Returns: Newly allocated string containing the date.
 */
gchar *
get_date (void)
{
  struct tm *t;
  time_t     now;
  gchar     *date = "";

  time (&now);
  t = localtime (&now);

  char buffer[1024];
  strftime (buffer, sizeof (buffer), "%Y-%m-%d_%H%M%S", t);

  date = g_strdup_printf ("%s", buffer);

  return date;
}

/**
 * file_exists:
 * @filename: File path to check.
 *
 * Returns TRUE if the file exists.
 */
gboolean
file_exists (gchar *filename)
{
  return g_file_test (filename, G_FILE_TEST_EXISTS);
}

/**
 * get_default_filename:
 *
 * Returns a default file name based on the project name and current date.
 *
 * Returns: Newly allocated string with the filename.
 */
gchar *
get_default_filename (void)
{
  gchar *date     = get_date ();
  gchar *filename = g_strdup_printf ("%s_%s", workspace->project_name, date);
  g_free (date);
  return filename;
}

/**
 * get_home_dir:
 *
 * Returns the home directory.
 *
 * Returns: Path to the home directory.
 */
const gchar *
get_home_dir (void)
{
  const char *homedir = g_getenv ("HOME");
  if (! homedir)
    {
      homedir = g_get_home_dir ();
    }
  return homedir;
}

/**
 * get_desktop_dir:
 *
 * Returns the desktop directory.
 *
 * Returns: Path to the desktop directory.
 */
const gchar *
get_desktop_dir (void)
{
  return g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
}

/**
 * get_documents_dir:
 *
 * Returns the documents directory, or home if unavailable.
 *
 * Returns: Path to the documents directory.
 */
const gchar *
get_documents_dir (void)
{
  const gchar *documents_dir;
  documents_dir = g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);
  if (documents_dir == NULL)
    {
      documents_dir = get_home_dir ();
    }
  return documents_dir;
}

/**
 * rmdir_recursive:
 * @path: Directory path to remove recursively.
 *
 * Deletes a directory and all its contents.
 */
void
rmdir_recursive (gchar *path)
{
  GDir        *cur_dir;
  const gchar *dir_file;

  cur_dir = g_dir_open (path, 0, NULL);

  if (cur_dir)
    {
      while ((dir_file = g_dir_read_name (cur_dir)))
        {
          gchar *fpath = g_build_filename (path, dir_file, NULL);

          if (fpath)
            {
              if (g_file_test (fpath, G_FILE_TEST_IS_DIR))
                {
                  rmdir_recursive (fpath);
                }
              else
                {
                  g_unlink (fpath);
                }
              g_free (fpath);
            }
        }

      g_dir_close (cur_dir);
    }

  g_rmdir (path);
}

/**
 * remove_dir_if_empty:
 * @dir_path: Directory path.
 *
 * Removes the directory if it is empty.
 */
void
remove_dir_if_empty (gchar *dir_path)
{
  GDir        *dir   = NULL;
  GError      *error = NULL;
  const gchar *name;

  /* Safely try to open the directory */
  dir = g_dir_open (dir_path, 0, &error);
  if (error)
    {
      g_debug ("Could not open directory '%s': %s", dir_path, error->message);
      g_error_free (error);
      return;
    }
  if (! dir)
    {
      return; /* Directory does not exist or other silent error */
    }

  /* Check for any content other than "." and ".." */
  gboolean is_empty = TRUE;
  while ((name = g_dir_read_name (dir)))
    {
      if (g_strcmp0 (name, ".") != 0 && g_strcmp0 (name, "..") != 0)
        {
          /* Found a real file or subdirectory, so it's not empty */
          is_empty = FALSE;
          break; /* No need to check further */
        }
    }

  g_dir_close (dir);

  if (is_empty)
    {
      g_debug ("Directory '%s' is empty, removing it.", dir_path);
      rmdir_recursive (dir_path);
    }
}

/**
 * allocate_point:
 * @x: X coordinate.
 * @y: Y coordinate.
 * @width: Stroke width.
 * @pressure: Pressure value.
 *
 * Allocates a new AnnotatePoint structure.
 *
 * Returns: Newly allocated AnnotatePoint.
 */
AnnotatePoint *
allocate_point (gdouble x, gdouble y, gdouble width, gdouble pressure)
{
  AnnotatePoint *point = g_malloc ((gsize) sizeof (AnnotatePoint));
  point->x             = x;
  point->y             = y;
  point->width         = width;
  point->pressure      = pressure;
  return point;
}

/**
 * send_email:
 * @to: Recipient email address.
 * @subject: Email subject.
 * @body: Email body text.
 * @attachment_list: List of file paths to attach.
 *
 * Sends an email with optional attachments. Uses platform-specific
 * implementation: windows_send_email on Windows, xdg-email on Linux.
 */
void
send_email (gchar *to,
            gchar *subject,
            gchar *body,
            GSList *attachment_list)
{
#ifdef _WIN32
  windows_send_email (to, subject, body, attachment_list);
#else

  guint attach_lenght = g_slist_length (attachment_list);
  guint i             = 0;

  gchar *mailer        = "xdg-email";
  gchar *subject_param = "--subject";
  gchar *body_param    = "--body";
  gchar *attach_param  = "--attach";

  gchar *args = g_strdup_printf ("%s %s %s %s '%s'",
                                 mailer,
                                 subject_param,
                                 subject,
                                 body_param,
                                 body);

  for (i = 0; i < attach_lenght; i++)
    {
      gchar *attachment = (gchar *) g_slist_nth_data (attachment_list, i);
      gchar *attachment_str = g_strdup_printf ("%s '%s'",
                                               attach_param,
                                               attachment);
      gchar *new_args = g_strdup_printf ("%s %s", args, attachment_str);
      g_free (args);
      args = new_args;
      g_free (attachment_str);
    }

  gchar *new_args = g_strdup_printf ("%s %s&", args, to);
  g_free (args);

  if (system (new_args) < 0)
    {
      g_warning ("Problem running command: %s", new_args);
    }

  g_free (new_args);

#endif
}

/**
 * send_artifacts_with_email:
 * @attachment_list: List of artifact file paths.
 *
 * Sends created artifacts to the Ardesia developer group via email.
 */
void
send_artifacts_with_email (GSList *attachment_list)
{
  gchar *to      = "ardesia-developer@googlegroups.com";
  gchar *subject = "ardesia-contribution";
  gchar *body = g_strdup_printf ("%s,\n%s,%s.",
                                 "Dear ardesia developer group",
                                 "I want share my work created with Ardesia "
                                 "with you",
                                 "please for details see the attachment");

  send_email (to, subject, body, attachment_list);
  g_free (body);
}

/**
 * send_trace_with_email:
 * @attachment: Path to stack trace file.
 *
 * Sends an application error report to the Ardesia developer group.
 */
void
send_trace_with_email (gchar *attachment)
{
  GSList *attachment_list = NULL;
  gchar  *to              = "ardesia-developer@googlegroups.com";
  gchar  *subject         = "ardesia-bug-report";

  gchar *body = g_strdup_printf ("%s,\n%s,%s.",
                                 "Dear ardesia developer group",
                                 "An application error occurred",
                                 "please for details see the attachment with "
                                 "the stack trace");

  attachment_list = g_slist_prepend (attachment_list, attachment);
  send_email (to, subject, body, attachment_list);
  g_free (body);
}

/**
 * is_gnome:
 *
 * Checks if the current desktop environment is GNOME.
 *
 * Returns: TRUE if GNOME, FALSE otherwise.
 */
gboolean
is_gnome (void)
{
#ifdef _WIN32
  return FALSE;
#endif

  gchar *current_desktop = getenv ("XDG_CURRENT_DESKTOP");
  if (current_desktop)
    {
      if (strcmp (current_desktop, "GNOME") != 0)
        {
          return FALSE;
        }
    }
  return TRUE;
}

/**
 * xdg_create_desktop_entry:
 * @filename: Path to create the .desktop file.
 * @type: Entry type, e.g., "Application".
 * @name: Display name.
 * @icon: Icon file path.
 * @exec: Executable command.
 *
 * Creates a .desktop entry with the specified parameters.
 */
void
xdg_create_desktop_entry (gchar *filename,
                          gchar *type,
                          gchar *name,
                          gchar *icon,
                          gchar *exec)
{
  FILE *fp = fopen (filename, "w");
  if (fp)
    {
      fprintf (fp, "[Desktop Entry]\n");
      fprintf (fp, "Type=%s\n", type);
      fprintf (fp, "Name=%s\n", name);
      fprintf (fp, "Icon=%s\n", icon);
      fprintf (fp, "Exec=%s", exec);
      fclose (fp);
      chmod (filename, 0751);
    }
}

/**
 * xdg_create_link:
 * @src: Source file path.
 * @dest: Destination name without extension.
 * @icon: Icon path.
 *
 * Creates a desktop link (.desktop file) pointing to @src.
 */
void
xdg_create_link (gchar *src, gchar *dest, gchar *icon)
{
  gchar *link_extension = "desktop";
  gchar *link_filename  = g_strdup_printf ("%s.%s", dest, link_extension);

  if (! g_file_test (link_filename, G_FILE_TEST_EXISTS))
    {
      gchar *exec = g_strdup_printf ("xdg-open %s\n", src);
      xdg_create_desktop_entry (link_filename,
                                "Application",
                                PACKAGE_NAME,
                                icon,
                                exec);
      g_free (exec);
    }

  g_free (link_filename);
}

/**
 * g_substrlastpos:
 * @str: Input string.
 * @substr: Substring to search for.
 *
 * Finds the last occurrence of @substr in @str.
 *
 * Returns: Index of the last occurrence, or -1 if not found.
 */
gint
g_substrlastpos (const char *str, const char *substr)
{
  gint len = (gint) strlen (str);
  gint i   = 0;

  for (i = len - 1; i >= 0; --i)
    {

      if (str[i] == *substr)
        {
          return i;
        }
    }
  return -1;
}

/**
 * g_substr:
 * @string: Input string.
 * @start: Start index.
 * @end: End index (inclusive).
 *
 * Returns a newly allocated substring from start to end.
 *
 * Returns: Newly allocated string.
 */
gchar *
g_substr (const gchar *string, gint start, gint end)
{
  gint  number_of_char = (end - start + 1);
  gsize size           = (gsize) sizeof (gchar) * number_of_char;
  return g_strndup (&string[start], size);
}

/**
 * create_segmentation_fault:
 *
 * Function that intentionally causes a segmentation fault.
 * Useful for testing the segmentation fault handler.
 */
void
create_segmentation_fault (void)
{
  int *f = NULL;
  *f     = 0;
}

void
get_surface_size (cairo_surface_t *surface, int *width, int *height)
{
  cairo_t *cr;
  double   x1, x2, y1, y2;

  cr = cairo_create (surface);
  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);
  cairo_destroy (cr);

  *width  = x2 - x1;
  *height = y2 - y1;
}

void
get_context_size (cairo_t *cr, int *width, int *height)
{
  double x1, x2, y1, y2;

  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);

  *width  = x2 - x1;
  *height = y2 - y1;
}

/**
 * save_cairo_context:
 * @cr: Cairo drawing context.
 * @savedir: Directory to save the PNG.
 * @category: Logical category name for the file.
 * @index: Index number.
 *
 * Saves a snapshot of the Cairo context to a PNG file:
 * <PACKAGE_NAME>_<category>_<index>_vellum.png
 */
void
save_cairo_context (cairo_t *cr, gchar *savedir, gchar *category, int index)
{
  gchar *filename = g_strdup_printf ("%s%s%s_%s_%d_vellum.png",
                                     savedir,
                                     G_DIR_SEPARATOR_S,
                                     PACKAGE_NAME,
                                     category,
                                     index);

  int w;
  int h;
  get_context_size (cr, &w, &h);

  /*
   * Load a surface with the data->annotation_cairo_context
   * content and write the file.
   */
  cairo_surface_t *saved_surface;
  saved_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                              w,
                                              h);

  cairo_surface_t *source_surface = cairo_get_target (cr);
  cairo_t         *dest_cr        = cairo_create (saved_surface);
  cairo_set_source_surface (dest_cr, source_surface, 0, 0);
  cairo_paint (dest_cr);
  /* Postcondition: the saved_surface now contains the save-point image. */

  /*
   *  Will be create a file in the save-point folder
   *  with format PACKAGE_NAME_1.png.
   */
  cairo_surface_write_to_png (saved_surface, filename);
  cairo_surface_destroy (saved_surface);
  cairo_destroy (dest_cr);
  g_debug ("Saving cairo context image to %s\n", filename);
  g_free (filename);
}
