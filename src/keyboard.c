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

#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "keyboard.h"
#include "utils.h"

/**
 * start_virtual_keyboard:
 *
 * Starts the virtual keyboard on the system. On Linux, this sets the
 * Florence keyboard to be always visible. On failure, logs a warning.
 **/
void
start_virtual_keyboard (void)
{
  int result = -1;
  result     = system ("gsettings set org.florence.behaviour auto-hide false");
  if (result != 0)
    {
      g_warning ("Fail to show virtual keyboard: Florence and gsettings "
                 "packages are required\n");
    }
}

/**
 * stop_virtual_keyboard:
 *
 * Stops the virtual keyboard. On Linux, this sets the Florence keyboard
 * to auto-hide. On Windows, attempts to close the Florence window.
 * Logs a warning if the operation fails.
 **/
void
stop_virtual_keyboard (void)
{
#ifdef _WIN32
  if (virtual_keyboard_pid > 0)
    {
      /* @TODO replace this with the cross platform g_pid_terminate
       * when it will available.
       */
      HWND hwnd = FindWindow (VIRTUALKEYBOARD_WINDOW_NAME, NULL);
      SendMessage (hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
      g_spawn_close_pid (virtual_keyboard_pid);
      virtual_keyboard_pid = (GPid) 0;
    }
#else
  int result = -1;
  result     = system ("gsettings set org.florence.behaviour auto-hide true");
  if (result != 0)
    {
      g_warning ("Fail to hide virtual keyboard: Florence and gsettings "
                 "packages are required\n");
    }
#endif
}
