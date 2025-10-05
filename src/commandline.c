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

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include "ardesia.h"
#include "commandline.h"
#include "config.h"
#include "monitor.h"

static struct option long_options[] = {
  /* These options set a flag. */
  { "help",              no_argument,       0, 'h'  },
  { "decorated",         no_argument,       0, 'd'  },
  { "verbose",           no_argument,       0, 'V'  },
  { "version",           no_argument,       0, 'v'  },
  /*
   * These options don't set a flag.
   * We distinguish them by their indices.
   */
  { "gravity",           required_argument, 0, 'g'  },
  { "leftmargin",        required_argument, 0, 'l'  },
  { "tabsize",           required_argument, 0, 't'  },
  { "tools-monitor",     required_argument, 0, 'm'  },
  { "workspace-monitor", required_argument, 0, 'M'  },
  { "x",                 required_argument, 0, 'x'  },
  { "y",                 required_argument, 0, 'y'  },
  { "width",             required_argument, 0, 1000 },
  { "height",            required_argument, 0, 1001 },
  { "opaque",            no_argument,       0, 'o'  },
  { 0,                   0,                 0, 0    }
};

void
add_defaults_to_commandline (CommandLine *commandline)
{
  /*
   * Crucial safety check: if the provided pointer is NULL, we cannot
   * proceed.
   */
  if (commandline == NULL)
    {
      g_warning ("add_defaults_to_commandline called with a NULL pointer.");
      return;
    }

  commandline->position          = EAST;
  commandline->debug             = FALSE;
  commandline->iwb_filename      = NULL;
  commandline->decorated         = FALSE;
  commandline->text_leftmargin   = 0;
  commandline->text_tabsize      = 80;
  commandline->mode              = DRAW_ON_MONITOR;
  commandline->workspace_monitor = 1;
  commandline->tools_monitor     = 0; /* Set the final value directly */
  commandline->is_opaque         = FALSE;

  /* Safely allocate and initialize the clipRect */
  commandline->clipRect = g_new0 (GdkRectangle, 1);

  if (commandline->clipRect == NULL)
    {
      g_critical ("Failed to allocate memory for clipRect.");
      return;
    }

  commandline->clipRect->x      = 0;
  commandline->clipRect->y      = 0;
  commandline->clipRect->width  = 200;
  commandline->clipRect->height = 200;
}

/**
 * create_command_line:
 *
 * Allocates and initializes a new #CommandLine structure with default values.
 *
 * This function creates a new #CommandLine object and populates it with
 * default settings by calling add_defaults_to_commandline().
 *
 * Returns: (transfer full): A pointer to the newly allocated and
 * initialized #CommandLine struct. The caller is responsible for freeing
 * this memory with destroy_command_line().
 **/
CommandLine *
create_command_line (void)
{
  CommandLine *commandline = g_malloc ((gsize) sizeof (CommandLine));
  add_defaults_to_commandline (commandline);
  return commandline;
}

/**
 * destroy_command_line:
 * @commandline: (transfer full): The #CommandLine object to free.
 *
 * Frees all memory associated with a #CommandLine object.
 *
 * This function first frees any internally allocated members (such as
 * `clipRect`) before freeing the #CommandLine structure itself.
 **/
void
destroy_command_line (CommandLine *commandline)
{
  g_debug ("Destroying command line\n");
  if (commandline->clipRect != NULL)
    {
      g_free (commandline->clipRect);
    }
  g_free (commandline);
}

/* Print the version of the tool and exit. */
static void
print_version (void)
{
  g_printf ("Ardesia %s; the free digital sketchpad\n\n", PACKAGE_VERSION);
  exit (EXIT_SUCCESS);
}

/* Print the command line help. */
static void
print_help (void)
{
  gchar *authors = "Tom McCallum (2018-2019), Pietro Pilolli (2009-2025)";
  g_printf ("Usage: %s [options] [filename]\n\n", PACKAGE_NAME);
  g_printf ("Ardesia the free digital sketchpad\n\n");
  g_printf ("options:\n");
  g_printf ("  --verbose ,\t\t-V\t\tEnable verbose mode to see the logs\n");
  g_printf ("  --decorate,\t\t-d\t\tDecorate the window with the borders\n");
  g_printf ("  --gravity ,\t\t-g\t\tSet the gravity of the bar. Possible "
            "values are:\n");
  g_printf ("  \t\t\t\t\teast [default]\n");
  g_printf ("  \t\t\t\t\twest\n");
  g_printf ("  \t\t\t\t\tnorth\n");
  g_printf ("  \t\t\t\t\tsouth\n");
  g_printf ("  --leftmargin,\t\t-l\t\tSet the left margin in text window to "
            "set after hitting Enter\n");
  g_printf ("  --tabsize,\t\t-t\t\tSet the tabsize in pixel in text window\n");

  g_printf ("  --coverage,\t\t-c\t\tSet whether to fit to "
            "(monitor|area|full)\n");
  g_printf ("  --tools-monitor,\t-m\t\tSet which monitor has the tools window "
            "appear (default: 1)\n");
  g_printf ("  --workspace-monitor,\t-M\t\tSet which monitor the main window "
            "will appear over (default: 1)\n");
  g_printf ("  -x\t\t\t\t\tSet the x position of the main window (default: "
            "0)\n");
  g_printf ("  -y\t\t\t\t\tSet the y position of the main window (default: "
            "0)\n");
  g_printf ("  --width,\t\t\t\tSet the width of the main window (default: "
            "200)\n");
  g_printf ("  --height,\t\t\t\tSet the height of the main window (default: "
            "200)\n");

  g_printf ("  --opaque,\t\t-o\t\tForce the main window to be opaque and not "
            "transparent\n");
  g_printf ("  --help    ,\t\t-h\t\tShows the help screen\n");
  g_printf ("  --version ,\t\t-v\t\tShows version information and exit\n");
  g_printf ("\n");
  g_printf ("filename:\t\t  \t\tThe interactive Whiteboard Common File "
            "(iwb)\n");
  g_printf ("\n");
  g_printf ("%s (C) %s\n", PACKAGE_STRING, authors);
  exit (EXIT_FAILURE);
}

/**
 * parse_options:
 * @commandline: (out): A #CommandLine struct to be populated with the
 * parsed options.
 * @argc: The argument count, as passed to main().
 * @argv: The argument vector, as passed to main().
 *
 * Parses the application's command-line arguments using `getopt_long`.
 *
 * This function iterates through the provided @argv, processing both short
 * and long options (defined in the `long_options` array), and populates
 * the @commandline structure accordingly. It handles informational flags
 * like --help and --version, and validates arguments where necessary. Any
 * remaining non-option argument is treated as an input IWB filename.
 **/
void
parse_options (CommandLine *commandline, gint argc, char *argv[])
{

  /* Getopt_long stores the option index here. */
  while (1)
    {
      gint c;

      gint option_index = 0;
      c = getopt_long (argc,
                       argv,
                       "hdvVg:f:l:t:w:c:m:M:x:y:o",
                       long_options,
                       &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        {
          break;
        }

      switch (c)
        {
        case 'h':
          print_help ();
          break;
        case 'v':
          print_version ();
          break;
        case 'd':
          commandline->decorated = TRUE;
          break;
        case 'V':
          commandline->debug = TRUE;
          break;
        case 'g':
          if (g_strcmp0 (optarg, "east") == 0)
            {
              commandline->position = EAST;
            }
          else if (g_strcmp0 (optarg, "west") == 0)
            {
              commandline->position = WEST;
            }
          else if (g_strcmp0 (optarg, "north") == 0)
            {
              commandline->position = NORTH;
            }
          else if (g_strcmp0 (optarg, "south") == 0)
            {
              commandline->position = SOUTH;
            }
          else
            {
              print_help ();
            }
          break;
        case 'l':
          commandline->text_leftmargin = atoi (optarg);
          break;
        case 't':
          commandline->text_tabsize = atoi (optarg);
          break;
        case 'c':
          if (g_strcmp0 (optarg, "monitor") == 0)
            {
              commandline->mode = DRAW_ON_MONITOR;
            }
          else if (g_strcmp0 (optarg, "area") == 0)
            {
              commandline->mode = DRAW_ON_CLIPAREA;
            }
          else if (g_strcmp0 (optarg, "full") == 0)
            {
              commandline->mode = DRAW_ON_FULLDESKTOP;
            }
          else
            {
              print_help ();
            }
          break;

        case 'm':
          commandline->tools_monitor = atoi (optarg);
          break;
        case 'M':
          commandline->workspace_monitor = atoi (optarg);
          break;
        case 'x':
          commandline->clipRect->x = atoi (optarg);
          break;
        case 'y':
          commandline->clipRect->y = atoi (optarg);
          break;
        case 1000:
          commandline->clipRect->width = atoi (optarg);
          break;
        case 1001:
          commandline->clipRect->height = atoi (optarg);
          break;
        case 'o':
          commandline->is_opaque = TRUE;
          break;
        default:
          g_debug ("Invalid argument given: %c\n", c);
          print_help ();
          break;
        }
    }

  if (optind < argc)
    {
      commandline->iwb_filename = argv[optind];
    }
}

/**
 * debug_commandline:
 * @commandline: The #CommandLine struct whose contents will be printed.
 *
 * A debugging utility function that prints the fields of a #CommandLine
 * struct to the debug log.
 *
 * It uses g_debug() to output the values of the main options, including
 * drawing mode, monitor selection, clip rectangle geometry, and the
 * opacity flag. The output is only visible if GLib debugging messages
 * are enabled for the application.
 **/
void
debug_commandline (CommandLine *commandline)
{
  g_debug ("Coverage: %s\n",
            (commandline->mode == DRAW_ON_MONITOR ? "Monitor" : "Area"));

  g_debug ("Tools Monitor: %d\n", commandline->tools_monitor);
  g_debug ("Workspace Monitor: %d\n", commandline->workspace_monitor);

  g_debug ("Rectangle: %d %d %d %d\n",
           commandline->clipRect->x,
           commandline->clipRect->y,
           commandline->clipRect->width,
           commandline->clipRect->height);

  g_debug ("Is Opaque: %d\n", commandline->is_opaque);
}
