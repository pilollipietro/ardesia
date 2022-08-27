/*
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli.pietro@gmail.com>
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
 * Functions for handling various (GTK+)-Events.
 */


#include <annotation_window_callbacks.h>
#include <annotation_window.h>
#include <utils.h>
#include <input.h>
#include <background_window.h>
#include <text_window.h>
#include <text_window_callbacks.h>
#include <cairo_functions.h>
#include <bar.h>




/* On configure event. */
G_MODULE_EXPORT gboolean
on_configure       (GtkWidget      *widget,
                    GdkEventExpose *event,
                    gpointer        user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;
  GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (widget));

  g_printf("DEBUG: Annotation window get configure event (%d,%d,%d,%d)\n",
             gtk_widget_get_allocated_width (widget),
             gtk_widget_get_allocated_height (widget),
             state,
             gtk_widget_is_focus (widget)
             );
  if (!data->is_grabbed)
    {
      return FALSE;
    }

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    if ( data->is_background_visible == TRUE ) {
        annotation_window_change(width, height);
        gtk_widget_set_opacity (data->annotation_window, 1.0);
    } else {
        //gtk_widget_set_opacity (data->annotation_window, 0.0);
    }
    if ( data->is_annotation_visible == TRUE ) {
        annotation_window_change(width, height);
    }
    if ( data->is_text_editor_visible == TRUE ) {
        annotation_window_change(width, height);
    }
}

G_MODULE_EXPORT gboolean
on_keypress (GtkWidget *widget,
             GdkEvent  *event,
             gpointer   user_data) {
    AnnotateData* data = (AnnotateData*) user_data;
    GdkEventKey* ev = (GdkEventKey*) event;
    gboolean retval = FALSE;

     g_printf("DEBUG: Annotation on_keypress event (%d, %d)\n",
              ev->type,
              ev->keyval
           );

    if ( data->is_text_editor_visible == TRUE ) {
        retval = on_text_window_key_press_event(widget, event, text_data);

    }

    gtk_widget_queue_draw( widget );
    return retval;
}

G_MODULE_EXPORT gboolean
on_keyrelease (GtkWidget *widget,
            GdkEvent  *event,
            gpointer   user_data) {

                GdkEventKey* ev = (GdkEventKey*) event;
                 g_printf("DEBUG: Annotation on_keyrelease event (%d, %d)\n",
                          ev->type,
                          ev->keyval
                       );
                return FALSE;
}


/** When the window changes z-order or state */
G_MODULE_EXPORT gboolean
on_window_state_event (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data){
    return FALSE;
}


/* On screen changed.
 * Required to make window transparent
 **/
G_MODULE_EXPORT void
on_screen_changed       (GtkWidget  *widget,
                         GdkScreen  *previous_screen,
                         gpointer    user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;
  if (data->debug)
    {
      g_printerr ("DEBUG: Annotation window get screen-changed event\n");
    }

  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (widget));
  GdkVisual *visual = gdk_screen_get_rgba_visual (screen);

  if (visual == NULL)
    {
      visual = gdk_screen_get_system_visual (screen);
    }

  gtk_widget_set_visual (widget, visual);
}


void
cut_out_window_holes(cairo_t* cr)
{
    if ( cr == NULL ) return;

    gint x=0, y=0, width=0, height=0;
    GdkRectangle* rB =NULL;
    gint ann_width=0, ann_height=0, ann_x=0, ann_y=0;
    gtk_window_get_position (GTK_WINDOW (annotation_data->annotation_window), &ann_x, &ann_y);
    gtk_window_get_size (GTK_WINDOW (annotation_data->annotation_window), &ann_width, &ann_height);
    // enclosing path
    //cairo_new_sub_path(cr);
    cairo_new_path(cr);
    //cairo_rectangle( cr, ann_x, ann_y, ann_width, ann_height );

    GdkRectangle* rA = g_new( GdkRectangle, 1 );
    rA->x = ann_x;
    rA->y = ann_y;
    rA->width = ann_width;
    rA->height = ann_height;

    GtkWidget *bar= get_bar_widget();

    if ( bar != NULL ) {
        gtk_window_get_position (GTK_WINDOW (bar), &x, &y);
        gtk_window_get_size (GTK_WINDOW (bar), &width, &height);

        rB = g_new( GdkRectangle, 1 );
        rB->x = x;
        rB->y = y;
        rB->width = width;
        rB->height = height;

        if ( intersect( rA, rB ) == TRUE ) {
          cairo_new_sub_path(cr);
          cairo_rectangle( cr, x+1-ann_x, y+1-ann_y, width-1, height-1 );
          g_printf("making hole for bar - intersect was true\n");
          g_printf("%d %d %d %d\n", x-ann_x, y-ann_y, width, height );
        }
        g_free( rB );
        rB = NULL;
    }

  // we also want to drill down other windows if they too are visible
  if ( annotation_data->background_selection_window != NULL ) {
      gtk_window_get_position (GTK_WINDOW (annotation_data->background_selection_window), &x, &y);
      gtk_window_get_size (GTK_WINDOW (annotation_data->background_selection_window), &width, &height);
      rB = g_new( GdkRectangle, 1 );
      rB->x = x;
      rB->y = y;
      rB->width = width;
      rB->height = height;
      if ( intersect( rA, rB ) == TRUE ) {
          g_printf("making hole for background selection window - intersect was true\n");
          g_printf("%d %d %d %d\n", x-ann_x, y-ann_y, width, height );
          cairo_new_sub_path(cr);
          cairo_rectangle( cr, x-ann_x, y-ann_y, width, height );
      }
      g_free( rB );
      rB = NULL;
  }

  if ( annotation_data->font_window != NULL ) {
      gtk_window_get_position (GTK_WINDOW (annotation_data->font_window), &x, &y);
      gtk_window_get_size (GTK_WINDOW (annotation_data->font_window), &width, &height);
      rB = g_new( GdkRectangle, 1 );
      rB->x = x;
      rB->y = y;
      rB->width = width;
      rB->height = height;
      if ( intersect( rA, rB ) == TRUE ) {
          g_printf("making hole for font selection window - intersect was true\n");
          g_printf("%d %d %d %d\n", x-ann_x, y-ann_y, width, height );
          cairo_new_sub_path(cr);
          cairo_rectangle( cr, x-ann_x, y-ann_y, width, height );
      }
      g_free( rB );
      rB = NULL;
  }

  // draw in reverse
  cairo_new_sub_path(cr);
  cairo_move_to(cr, ann_width, 0);
  cairo_line_to(cr, 0, 0);
  cairo_line_to(cr, 0, ann_width);
  cairo_line_to(cr, ann_width, ann_height);
  cairo_close_path(cr);

  if ( rA != NULL ) {
      g_free( rA );
  }
  if ( rB != NULL ) {
      g_free( rB );
  }
}

/* Expose event: this occurs when the window is shown.
 */
G_MODULE_EXPORT gboolean
on_expose          (GtkWidget *widget,
                    cairo_t   *cr,
                    gpointer   user_data)
{
    AnnotateData *annotation_data = (AnnotateData *) user_data;

    g_print("DEBUG: Annotation window get draw event (grab: %d)\n", bar_data->grab);
    gboolean use_paint = TRUE;
    clear_cairo_context( cr ); // blank the current window for repainting

    // make sure windows are just underneath for this to work
    if ( bar_data->grab == FALSE && annotation_data->is_background_visible == TRUE  ) {
        // @TODO this causes flicker so only want to do it if the windows have moved
        //          or pointer has just been selected
        //       add a new flag to annotation_window to mark request for redraw
        // @TODO fix issue that window decorations are not picked up by allocated size
        //          -- this may require a call to the x-server if available
        // @TODO fix issue that image not updated as window is dragged across screen
        //          -- this needs to know what event is called for window drag, may be mousemove

        gtk_window_present (GTK_WINDOW (get_bar_widget ()));
        if ( annotation_data->background_selection_window != NULL ) {
            g_printf("presenting background selection window\n");
            gtk_window_present (GTK_WINDOW (annotation_data->background_selection_window));
        }
        if ( annotation_data->font_window != NULL ) {
            g_printf("presenting font window\n");
            gtk_window_present (GTK_WINDOW (annotation_data->font_window));
        }

        // if we are not grabbed then make transparent the region that is over the windows
        // these would have been cut out for clicks earlier
        // we want to set the clip first
        cut_out_window_holes(cr);
        use_paint = FALSE;
    } else {
        gint ann_width=0, ann_height=0, ann_x=0, ann_y=0;
        gtk_window_get_position (GTK_WINDOW (annotation_data->annotation_window), &ann_x, &ann_y);
        gtk_window_get_size (GTK_WINDOW (annotation_data->annotation_window), &ann_width, &ann_height);
        cairo_rectangle( cr, ann_x, ann_y, ann_width, ann_height );
        use_paint = TRUE;
    }

    if ( annotation_data->is_background_visible == TRUE ) {
        // draw background layer on context cr
        if ( background_data->cr ) {
            draw_cairo_context( cr, background_data->cr,use_paint  );
        }
    }

    if ( annotation_data->is_annotation_visible == TRUE ) {
        g_printf("316: annotation_window_callbacks\n");
        // draw annotation layer on context cr
        initialize_annotation_cairo_context (annotation_data);
        draw_cairo_context( cr, annotation_data->annotation_cairo_context,use_paint  );
    }

    if ( annotation_data->is_text_editor_visible == TRUE ) {
        if ( text_data->cr ) {
            // draw the text editor layer
            draw_cairo_context( cr, text_data->cr,use_paint  );
        }
    }

    // draw clapperboard on top of everything else
    if ( annotation_data->is_clapperboard_visible == TRUE ) {
        if ( annotation_data->clapperboard_cairo_context ) {
            draw_cairo_context( cr, annotation_data->clapperboard_cairo_context,use_paint  );
        }
    }

  return TRUE;
}


/*
 * Event-Handlers to perform the drawing.
 */


/* This is called when the button is pushed. */
G_MODULE_EXPORT gboolean
on_button_press    (GtkWidget      *win,
                    GdkEventButton *ev,
                    gpointer        user_data)
{
  g_printf("annotation_window:: on_button_press\n");
  AnnotateData *data = (AnnotateData *) user_data;
  gboolean retval = FALSE;

  if ( data->is_annotation_visible == TRUE &&
        data->is_text_editor_visible == FALSE ) {
      retval = annotation_window_button_press( ev, data );
  }

  return retval;
}


/* This shots when the pointer is moving. */
G_MODULE_EXPORT gboolean
on_motion_notify   (GtkWidget       *win,
                    GdkEventMotion  *ev,
                    gpointer         user_data)
{

  AnnotateData *data = (AnnotateData *) user_data;
  gboolean retval = FALSE;
  if ( data->is_annotation_visible == TRUE &&
       data->is_text_editor_visible == FALSE ) {
      retval = annotation_window_mouse_move( ev, data );
  }
  return retval;
}


/* This shots when the button is released. */
G_MODULE_EXPORT gboolean
on_button_release  (GtkWidget       *win,
                    GdkEventButton  *ev,
                    gpointer         user_data)
{
  g_printf("annotation_window::on_button_release\n");
  AnnotateData *data = (AnnotateData *) user_data;
  gboolean retval = FALSE;
  if ( data->is_text_editor_visible == TRUE ) {
      retval = on_text_window_button_release( win, ev, text_data );
  } else if ( data->is_annotation_visible == TRUE ) {
      retval = annotation_window_button_release( ev, data );
  }
  return retval;
}


/* On device added. */
void on_device_removed  (GdkDeviceManager  *device_manager,
                         GdkDevice         *device,
                         gpointer           user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if(data->debug)
    {
      g_printerr ("DEBUG: device '%s' removed\n", gdk_device_get_name(device));
    }

  remove_input_device (device, data);
}


/* On device removed. */
void on_device_added    (GdkDeviceManager  *device_manager,
                         GdkDevice         *device,
                         gpointer           user_data)
{
  AnnotateData *data = (AnnotateData *) user_data;

  if(data->debug)
    {
      g_printerr ("DEBUG: device '%s' added\n", gdk_device_get_name (device));
    }

  add_input_device (device, data);
}
