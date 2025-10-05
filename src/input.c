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

#include "input.h"

/* Add input device. */
static void
add_input_mode_device (AnnotateData *data, GdkDevice *device, GdkInputMode mode)
{
  if (! data->devdatatable)
    {
      data->devdatatable = g_hash_table_new (NULL, NULL);
    }

  AnnotateDeviceData *devdata = (AnnotateDeviceData *) NULL;
  devdata                     = g_malloc0 ((gsize) sizeof (AnnotateDeviceData));
  devdata->coord_list         = (GSList *) NULL;
  g_hash_table_insert (data->devdatatable, device, devdata);

  if (! gdk_device_set_mode (device, mode))
    {
      g_warning ("Unable to set the device %s to the %d mode\n",
                 gdk_device_get_name (device), mode);
    }

  g_debug ("Enabled Device in mode %s. Device: %p: \"%s\" (Type: %d)\n",
           mode == GDK_MODE_SCREEN ? "SCREEN" : "WINDOW", device,
           gdk_device_get_name (device), gdk_device_get_source (device));
}

/* Set-up input device list. */
static void
setup_input_device_list (AnnotateData *data, GList *devices)
{
  remove_input_devices (data);
  g_list_foreach (devices, (GFunc) add_input_device, data);
}

/* Select the preferred input mode depending on axis. */
static GdkInputMode
select_input_device_mode (GdkDevice *device)
{
  if (gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD &&
      gdk_device_get_n_axes (device) >= 2)
    {
      /* Choose screen mode. */
      g_debug ("Selecting GDK_MODE_SCREEN (%d)\n", GDK_MODE_SCREEN);
      return GDK_MODE_SCREEN;
    }
  else
    {
      /* Choose window mode. */
      g_debug ("Selecting GDK_MODE_WINDOW (%d)\n", GDK_MODE_WINDOW);
      return GDK_MODE_WINDOW;
    }
}

/**
 * remove_input_devices:
 * @data: (inout): The #AnnotateData struct containing the device hash table.
 *
 * Removes all input devices currently tracked in the device hash table.
 *
 * This function iterates through all keys (devices) in the `devdatatable`
 * and calls remove_input_device() for each one. After the iteration is
 * complete, it sets the `devdatatable` pointer in the @data struct to
 * %NULL.
 **/
void
remove_input_devices (AnnotateData *data)
{
  if (data->devdatatable)
    {
      GList *list = (GList *) NULL;
      list        = g_hash_table_get_keys (data->devdatatable);
      g_list_foreach (list, (GFunc) remove_input_device, data);
      g_list_free (list);
      data->devdatatable = (GHashTable *) NULL;
    }
}

int deviceIndex = 0;
void
print_device_info (GdkDevice *device)
{
  g_debug ("Device %d: Name : %s\n", deviceIndex, gdk_device_get_name (device));
  if (gdk_device_get_device_type (device) != GDK_DEVICE_TYPE_MASTER)
    {
      g_debug ("Device %d: Vendor ID : %s\n",
               deviceIndex,
               gdk_device_get_vendor_id (device));
      g_debug ("Device %d: Product ID : %s\n",
               deviceIndex,
               gdk_device_get_product_id (device));
    }
  if (gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD)
    {
      g_debug ("Device %d: Number of Axes : %d\n",
               deviceIndex,
               gdk_device_get_n_axes (device));
    }
  g_debug ("Device %d: Source : %d\n",
           deviceIndex,
           gdk_device_get_source (device));
  switch (gdk_device_get_source (device))
    {
    case 0:
      g_debug ("Device %d: Source Type : %s\n",
               deviceIndex,
               "Mouse");
      break;
    case 4:
      g_debug ("Device %d: Source Type : %s\n",
               deviceIndex,
               "Keyboard");
      break;
    default:
      g_debug ("Device %d: Source Type : %s\n",
               deviceIndex,
               "Unknown");
      break;
    }

  deviceIndex++;
}

/**
 * setup_input_devices:
 * @data: The main #AnnotateData application context.
 *
 * Gathers all available pointing devices and initializes them for use with
 * the application.
 *
 * This function is the main entry point for device setup. It retrieves the
 * master pointer and all associated slave devices from the default #GdkSeat,
 * combines them into a single list, and then passes this list to
 * setup_input_device_list() for further processing.
 **/
void
setup_input_devices (AnnotateData *data)
{
  GList   *devices = NULL;
  GdkSeat *seat    = gdk_display_get_default_seat (gdk_display_get_default ());

  GdkDevice *master = gdk_seat_get_pointer (seat);
  devices           = g_list_append (devices, master);
  GList *slavers = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_ALL_POINTING);
  devices        = g_list_concat (devices, slavers);
  g_assert (g_list_length (devices) > 0);
  // write out the devices
  deviceIndex = 0;
  g_list_foreach (devices, (GFunc) print_device_info, NULL);

  setup_input_device_list (data, devices);
  g_list_free (devices);
}

/**
 * add_input_device:
 * @device: The #GdkDevice to potentially add.
 * @data: The main #AnnotateData application context.
 *
 * Adds a single input device to the application's tracking system.
 *
 * This function filters devices to ensure only pointing devices (i.e., not
 * keyboards, and having at least two axes) are added. For a valid
 * device, it determines its mode and calls a helper function to complete
 * the registration.
 **/
void
add_input_device (GdkDevice *device, AnnotateData *data)
{
  /* only enable devices with 2 or more axes and exclude keyboards */
  if ((gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD) &&
      (gdk_device_get_n_axes (device) >= 2))
    {
      add_input_mode_device (data, device, select_input_device_mode (device));
    }
}

/**
 * remove_input_device:
 * @device: The #GdkDevice to remove.
 * @data: (inout): The #AnnotateData struct containing the device hash table.
 *
 * Removes a single input device from the application's tracking system.
 *
 * This function looks up the device in the `devdatatable`, frees any
 * coordinate data associated with it, and then removes the device's
 * entry from the hash table.
 **/
void
remove_input_device (GdkDevice *device, AnnotateData *data)
{
  if (data && data->devdatatable)
    {
      AnnotateDeviceData *devdata = g_hash_table_lookup (data->devdatatable,
                                                         device);
      annotate_coord_dev_list_free (devdata);
      g_hash_table_remove (data->devdatatable, device);
    }
}

/**
 * grab_pointer:
 * @widget: The #GtkWidget whose window will own the grab.
 * @eventmask: (unused): The event mask for the grab. This parameter is
 * currently ignored.
 *
 * Safely acquires a global pointer grab for the application, directing
 * all pointer events to the window associated with @widget.
 *
 * Before attempting a new grab, it first calls ungrab_pointer() to
 * release any pre-existing grabs. It uses an X11 error trap and
 * checks the #GdkGrabStatus return value to handle potential failures
 * gracefully, printing any errors to standard error.
 **/
void
grab_pointer (GtkWidget *widget, GdkEventMask eventmask)
{
  GdkGrabStatus result;
  GdkSeat      *device_manager = (GdkSeat *) NULL;
  GdkDisplay   *display        = (GdkDisplay *) NULL;
  display = gdk_display_get_default ();

  ungrab_pointer ();
  device_manager = gdk_display_get_default_seat (display);

  gdk_x11_display_error_trap_push (display);

  result = gdk_seat_grab (device_manager,
                          gtk_widget_get_window (widget),
                          GDK_SEAT_CAPABILITY_ALL_POINTING,
                          TRUE,
                          NULL,
                          NULL,
                          NULL,
                          NULL);

  gdk_display_flush (display);
  if (gdk_x11_display_error_trap_pop (display))
    {
      g_printerr ("Grab pointer error\n");
    }

  switch (result)
    {
    case GDK_GRAB_SUCCESS:
      break;
    case GDK_GRAB_ALREADY_GRABBED:
      g_printerr ("Grab Pointer failed: AlreadyGrabbed\n");
      break;
    case GDK_GRAB_INVALID_TIME:
      g_printerr ("Grab Pointer failed: GrabInvalidTime\n");
      break;
    case GDK_GRAB_NOT_VIEWABLE:
      g_printerr ("Grab Pointer failed: GrabNotViewable\n");
      break;
    case GDK_GRAB_FROZEN:
      g_printerr ("Grab Pointer failed: GrabFrozen\n");
      break;
    default:
      g_printerr ("Grab Pointer failed: Unknown error\n");
    }
}

/**
 * ungrab_pointer:
 * Safely releases any active pointer grab held by the application on the
 * default seat.
 *
 * This function uses an X11 error trap to gracefully handle potential
 * failures during the ungrab operation (e.g., if no grab was active
 * or the device is no longer available). If an error occurs, a
 * message is printed to standard error.
 **/
void
ungrab_pointer (void)
{
  GdkSeat *seat = (GdkSeat *) NULL;

  GdkDisplay   *display        = (GdkDisplay *) NULL;
  display = gdk_display_get_default ();
  seat    = gdk_display_get_default_seat (display);

  gdk_x11_display_error_trap_push (display);

  gdk_seat_ungrab (seat);
  gdk_display_flush (display);
  if (gdk_x11_display_error_trap_pop (display))
    {
      /*
       * This probably means the device table is outdated,
       * e.g. this device doesn't exist anymore.
       */
      g_printerr ("Ungrab pointer device error\n");
    }
}
