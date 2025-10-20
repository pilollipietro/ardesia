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

#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-outfile-zip.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-utils.h>

#include "annotation_window.h"
#include "background_window.h"
#include "iwb_saver.h"
#include "utils.h"

/* The file pointer to the iwb file. */
static FILE *fp = NULL;

/* Add the xml header. */
static void
add_header (void)
{
  const gchar *becta_ns    = "http://www.becta.org.uk/iwb";
  const gchar *svg_ns      = "http://www.w3.org/2000/svg";
  const gchar *xlink_ns    = "http://www.w3.org/1999/xlink";
  const gchar *iwb_version = "1.0";
  const gchar *iwb_format  = "<iwb xmlns:iwb=\"%s\" xmlns:svg=\"%s\" "
                             "xmlns:xlink=\"%s\" version=\"%s\">\n";

  gchar *line = g_strdup_printf(iwb_format,
                                becta_ns,
                                svg_ns,
                                xlink_ns,
                                iwb_version);

  fputs (line, fp);
  g_free (line);
}

/* Close the iwb xml tag. */
static void
close_iwb (void)
{
  fputs ("</iwb>\n", fp);
}

/* Open the svg tag. */
static void
open_svg (void)
{
  GtkWidget *annotation_window = get_annotation_window ();
  gint       width  = gtk_widget_get_allocated_width (annotation_window);
  gint       height = gtk_widget_get_allocated_height (annotation_window);

  gchar *line = g_strdup_printf (
      "\t<svg:svg width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
      width,
      height,
      width,
      height);

  fputs(line, fp);
  g_free(line);
}

/* Close the svg tag. */
static void
close_svg (void)
{
  fputs("\t</svg:svg>\n", fp);
}

/* Add the savepoint element. */
static void
add_savepoint (gint index)
{
  GtkWidget *annotation_window = get_annotation_window ();
  gint       width  = gtk_widget_get_allocated_width (annotation_window);
  gint       height = gtk_widget_get_allocated_height (annotation_window);
  gchar     *id     = g_strdup_printf ("id%d", index + 1);
  gchar *file = g_strdup_printf ("images/%s_%d_vellum.png",
                                 PACKAGE_NAME,
                                 index);

  gchar *svg_line;

  const gchar *svg_image_format = "\t\t<svg:image id=\"%s\" xlink:href=\"%s\" "
                                  "x=\"0\" y=\"0\" "
                                  "width=\"%d\" height=\"%d\"/>\n";

  svg_line = g_strdup_printf (svg_image_format, id, file, width, height);

  open_svg ();

  if (svg_line)
    {
      fputs (svg_line, fp);
      g_free (svg_line);
    }

  g_free (file);
  file = NULL;
  g_free (id);
  close_svg ();
}

/* Add the background element. */
static void
add_background (gchar *img_dir_path, gchar *background_image)
{
  GtkWidget *annotation_window = get_annotation_window ();
  gint       width  = gtk_widget_get_allocated_width (annotation_window);
  gint       height = gtk_widget_get_allocated_height (annotation_window);
  gchar     *image_destination_path = (gchar *) NULL;

  image_destination_path = g_build_filename (img_dir_path,
                                             "ardesia_0_vellum.png",
                                             (gchar *) 0);

  /* Background image valid and set to image type */
  if ((background_image) && (background_data->type == 2))
    {
      /* if the background is oupdated */
      if (g_strcmp0 (background_image, image_destination_path) != 0)
        {
          /*
           * Copy the file in ardesia_0_vellum.png
           * under image_path overriding it.
           */
          GFile *image_destination = NULL;
          image_destination   = g_file_new_for_path (image_destination_path);
          GFile *image_source = g_file_new_for_path (background_image);

          g_file_copy (image_source,
                       image_destination,
                       G_FILE_COPY_OVERWRITE,
                       NULL,
                       NULL,
                       NULL,
                       NULL);

          g_object_unref (image_source);
          g_object_unref (image_destination);
        }
      add_savepoint (0);
    }
  else
    {
      gchar       *color           = background_data->color;
      /* Initialize the rgba components to transparent. */
      guint        r               = 0;
      guint        g               = 0;
      guint        b               = 0;
      guint        a               = 0;
      const gchar *svg_rect_format = "\t\t<svg:rect id=\"id1\" x=\"0\" y=\"0\""
                                     " width=\"%d\" height=\"%d\""
                                     " fill=\"%s\" fill-opacity=\"%d\"/>\n";

      /* If the background type is color then parse it. */
      if ((color != NULL) && (background_data->type != 0))
        {
          sscanf (color, "%02X%02X%02X%02X", &r, &g, &b, &a);
        }

      gchar *rgb = g_strdup_printf ("rgb(%d,%d,%d)", r, g, b);

      open_svg ();

      gchar *line = g_strdup_printf (svg_rect_format, width, height, rgb, a);
      fputs (line, fp);
      g_free (line);

      close_svg ();
      g_free (rgb);
    }

  g_free (image_destination_path);
}

/* Add the background reference. */
static void
add_background_reference (void)
{
  gchar *line = g_strdup_printf ("\t<iwb:element ref=\"id1\" "
                                 "background=\"true\"/>\n");

  fputs (line, fp);
  g_free (line);
}

/* Add the savepoint elements. */
static void
add_savepoints (gint savepoint_number)
{
  /* For each i call add_savepoint. */
  gint i = 1;
  for (i = 1; i <= savepoint_number; i++)
    {
      add_savepoint (i);
    }
}

/* Add the savepoint reference. */
static void
add_savepoint_reference (gint index)
{
  gchar *id = g_strdup_printf ("id%d", index + 1);

  gchar *line = g_strdup_printf (
      "\t<iwb:element ref=\"%s\" locked=\"true\"/>\n", id);

  fputs (line, fp);
  g_free (line);

  g_free (id);
}

/* Add the savepoint references. */
static void
add_savepoint_references (gint savepoint_number)
{
  /* For each i call add_savepoint_reference. */
  gint i = 1;
  for (i = 1; i <= savepoint_number; i++)
    {
      add_savepoint_reference (i);
    }
}

/* Create the iwb xml content file. */
static void
create_xml_content (gchar *content_filename,
                    gchar *img_dir_path,
                    gchar *background_image)
{
  int   savepoint_number = -1;
  GDir *img_dir;

  fp = fopen (content_filename, "w");

  add_header ();
  add_background (img_dir_path, background_image);

  if (! background_image)
    {
      savepoint_number = 0;
    }

  img_dir = g_dir_open (img_dir_path, 0, NULL);

  while (g_dir_read_name (img_dir))
    {
      savepoint_number++;
    }

  g_dir_close (img_dir);

  add_savepoints (savepoint_number);
  add_background_reference ();
  add_savepoint_references (savepoint_number);
  close_iwb ();
  fclose (fp);
}

/* Add the filename under path to the gst_outfile. */
static void
add_file_to_gst_outfile (GsfOutfile *out_file,
                         gchar *path,
                         const gchar *file_name)
{
  GError    *err       = (GError *) NULL;
  gchar     *file_path = g_build_filename (path, file_name, NULL);
  GsfInput  *input     = GSF_INPUT (gsf_input_stdio_new (file_path, &err));
  GsfOutput *child     = gsf_outfile_new_child (out_file, file_name, FALSE);

  gsf_input_copy (input, child);
  gsf_output_close (child);
  g_object_unref (child);
  g_object_unref (input);

  g_free (file_path);
}

/* Add all the files in the folder under the working_dir to the gst_outfile. */
static void
add_folder_to_gst_outfile (GsfOutfile *gst_outfile,
                           gchar *working_dir,
                           gchar *folder)
{
  GsfOutfile *gst_dir = NULL;
  gst_dir     = GSF_OUTFILE (gsf_outfile_new_child (gst_outfile, folder, TRUE));
  gchar *path = g_build_filename (working_dir, folder, NULL);
  GDir  *dir  = g_dir_open (path, 0, NULL);

  if (dir)
    {
      const gchar *file = (const gchar *) NULL;

      while ((file = g_dir_read_name (dir)))
        {
          add_file_to_gst_outfile (gst_dir, path, file);
        }

      g_dir_close (dir);
    }

  gsf_output_close ((GsfOutput *) gst_dir);
  g_object_unref (gst_dir);
  g_free (path);
}

/* Create the iwb file. */
static void
create_iwb (gchar *zip_filename,
            gchar *working_dir,
            gchar *images_folder,
            gchar *content_filename)
{
  GError     *err         = (GError *) NULL;
  GsfOutfile *gst_outfile = (GsfOutfile *) NULL;
  GsfOutput  *gst_output  = (GsfOutput *) NULL;

  gsf_init ();

  gst_output = gsf_output_stdio_new (zip_filename, &err);
  if (gst_output == NULL)
    {
      g_warning ("Error saving iwb: %s\n", err->message);
      g_error_free (err);
      return;
    }

  gst_outfile = gsf_outfile_zip_new (gst_output, &err);
  if (gst_outfile == NULL)
    {
      g_warning ("Error in gsf_outfile_zip_new: %s\n", err->message);
      g_error_free (err);
      return;
    }

  g_object_unref (G_OBJECT (gst_output));

  add_folder_to_gst_outfile (gst_outfile, working_dir, images_folder);

  add_file_to_gst_outfile (gst_outfile, working_dir, content_filename);

  gsf_output_close ((GsfOutput *) gst_outfile);
  g_object_unref (G_OBJECT (gst_outfile));

  gsf_shutdown ();
}

/**
 * export_iwb:
 * @iwb_location: (nullable): The full path where the .iwb file should be
 * saved. If %NULL, a default path is generated in the project directory.
 *
 * Exports the current annotation session to a .iwb (Interactive Whiteboard)
 * file format.
 *
 * The export is only performed if there is content to save, meaning at
 * least one annotation savepoint exists or a background image is set.
 * The function gathers all session assets from a temporary directory,
 * generates a `content.xml` manifest, and packages them into the .iwb
 * archive (which is a zip file).
 *
 * If @iwb_location is provided, any existing file at that path is
 * overwritten.
 **/
void
export_iwb (gchar *iwb_location)
{
  const gchar *tmpdir           = g_get_tmp_dir ();
  gchar       *images           = "images";
  gchar       *background_image = background_data->image;
  gchar       *project_name     = get_project_name ();
  gchar *ardesia_tmp_dir = g_build_filename (tmpdir, PACKAGE_NAME, (gchar *) 0);

  gchar *project_tmp_dir = g_build_filename (ardesia_tmp_dir,
                                             project_name,
                                             (gchar *) 0);

  gchar *img_dir_path = g_build_filename (project_tmp_dir, images, (gchar *) 0);

  gchar *first_savepoint_file = g_strdup_printf ("%s%s%s_2_vellum.png",
                                                 img_dir_path,
                                                 G_DIR_SEPARATOR_S,
                                                 PACKAGE_NAME);

  /* if exist the file I continue to save */
  if ((file_exists (first_savepoint_file)) || (background_image))
    {
      gchar *iwb_file         = (gchar *) NULL;
      gchar *content_filename = "content.xml";

      gchar *content_filepath = g_build_filename (project_tmp_dir,
                                                  content_filename,
                                                  (gchar *) 0);

      /* If the iwb location is null means that it is a new project. */
      if (iwb_location == NULL)
        {
          /* It will be putted in the project dir. */
          gchar *extension = "iwb";

          gchar *iwb_name = g_strdup_printf ("%s.%s",
                                             get_project_name (),
                                             extension);

          /* The zip file is the iwb file located in the ardesia workspace. */
          iwb_file = g_build_filename (get_project_dir (),
                                       iwb_name,
                                       (gchar *) 0);
          g_free (iwb_name);
        }
      else
        {
          g_remove (iwb_location);
          iwb_file = g_strdup_printf ("%s", iwb_location);
        }

      g_remove (content_filepath);

      create_xml_content (content_filepath, img_dir_path, background_image);

      create_iwb (iwb_file, project_tmp_dir, "images", content_filename);

      /* Add to the list of the artefacts created in the session. */
      add_artifact (iwb_file);
      g_free (iwb_file);
      g_free (content_filepath);
    }

  g_free (first_savepoint_file);
  g_free (ardesia_tmp_dir);
  g_free (project_tmp_dir);
  g_free (img_dir_path);
}
