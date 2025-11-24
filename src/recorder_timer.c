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

#include "recorder_timer.h"
#include <math.h>

/* Display sizing constants */
#define DIGIT_SCALE_FACTOR 2.45
#define SEGMENT_THICKNESS  0.15
#define DIGIT_SPACING      0.3
#define COLON_SPACING      0.5
#define COLON_RADIUS       0.12
#define MAX_HOURS          99

/* Number of digits in display (HH:MM:SS = 6 digits) */
#define NUM_DIGITS 6
#define NUM_COLONS 2

/* LCD color scheme */
typedef struct
{
  gdouble r;
  gdouble g;
  gdouble b;
  gdouble a;
} LCDColor;

static const LCDColor LCD_GHOST_COLOR  = { 0.75, 1.0, 1.0, 0.75 };
static const LCDColor LCD_ACTIVE_COLOR = { 0.3, 0.5, 0.85, 1.0 };

/* Geometry calculations for segments */
typedef struct
{
  gdouble size;
  gdouble seg_thick;
  gdouble seg_len;
} SegmentGeometry;

/* Pre-calculated layout positions */
typedef struct
{
  gdouble digit_x[NUM_DIGITS];
  gdouble colon_x[NUM_COLONS];
} DisplayLayout;

/* Seven-segment encoding for digits 0-9 */
static const int segments_by_digit[10][7] = {
  { 1, 1, 1, 1, 1, 1, 0 }, /* 0 */
  { 0, 1, 1, 0, 0, 0, 0 }, /* 1 */
  { 1, 1, 0, 1, 1, 0, 1 }, /* 2 */
  { 1, 1, 1, 1, 0, 0, 1 }, /* 3 */
  { 0, 1, 1, 0, 0, 1, 1 }, /* 4 */
  { 1, 0, 1, 1, 0, 1, 1 }, /* 5 */
  { 1, 0, 1, 1, 1, 1, 1 }, /* 6 */
  { 1, 1, 1, 0, 0, 0, 0 }, /* 7 */
  { 1, 1, 1, 1, 1, 1, 1 }, /* 8 */
  { 1, 1, 1, 1, 0, 1, 1 }  /* 9 */
};

/**
 * calculate_geometry:
 * @size: Base size for scaling.
 *
 * Pre-calculates segment geometry to avoid repeated calculations.
 *
 * Returns: A #SegmentGeometry structure with computed values.
 */
static SegmentGeometry
calculate_geometry (gdouble size)
{
  SegmentGeometry geom;
  geom.size      = size;
  geom.seg_thick = size * SEGMENT_THICKNESS;
  geom.seg_len   = size;
  return geom;
}

/**
 * calculate_layout:
 * @x: Starting X coordinate.
 * @geom: Segment geometry information.
 * @spacing: Space between digits.
 * @colon_space: Space for colon separator.
 *
 * Pre-calculates all digit and colon positions for the display.
 * This optimization is particularly useful for high-frequency redraws.
 *
 * Returns: A #DisplayLayout structure with all positions.
 */
static DisplayLayout
calculate_layout (gdouble x, const SegmentGeometry *geom,
                  gdouble spacing, gdouble colon_space)
{
  DisplayLayout layout;
  gdouble       cur_x     = x;
  gint          colon_idx = 0;

  for (gint i = 0; i < NUM_DIGITS; i++)
    {
      layout.digit_x[i] = cur_x;
      cur_x += geom->size + (2.0 * geom->seg_thick) + spacing;

      /* Place colons after hours (position 1) and minutes (position 3) */
      if (i == 1 || i == 3)
        {
          layout.colon_x[colon_idx++] = cur_x + colon_space * 0.5;
          cur_x += colon_space + spacing;
        }
    }

  return layout;
}

/**
 * draw_segment:
 * @cr: Cairo context.
 * @x: Left coordinate.
 * @y: Top coordinate.
 * @w: Width.
 * @h: Height.
 *
 * Draw a single rectangular segment of a seven-segment digit.
 */
static void
draw_segment (cairo_t *cr, gdouble x, gdouble y, gdouble w, gdouble h)
{
  cairo_rectangle (cr, x, y, w, h);
  cairo_fill (cr);
}

/**
 * draw_digit_off:
 * @cr: Cairo context.
 * @x: Left coordinate.
 * @y: Top coordinate.
 * @geom: Pre-calculated segment geometry.
 *
 * Draw all seven segments in "off" state (dimmed) to show the digit
 * structure when the display is inactive.
 */
static void
draw_digit_off (cairo_t *cr, gdouble x, gdouble y, 
                const SegmentGeometry *geom)
{
  gdouble left = x;
  gdouble top  = y;

  /* Segment 0: horizontal bar at top */
  draw_segment (cr, left + geom->seg_thick, top, 
                geom->seg_len, geom->seg_thick);

  /* Segment 1: vertical bar at top-right */
  draw_segment (cr, left + geom->seg_len + geom->seg_thick,
                top + geom->seg_thick, geom->seg_thick, geom->seg_len);

  /* Segment 2: vertical bar at bottom-right */
  draw_segment (cr, left + geom->seg_len + geom->seg_thick,
                top + geom->seg_len + geom->seg_thick * 2.0, 
                geom->seg_thick, geom->seg_len);

  /* Segment 3: horizontal bar at bottom */
  draw_segment (cr, left + geom->seg_thick, 
                top + 2.0 * geom->seg_len + 2.0 * geom->seg_thick,
                geom->seg_len, geom->seg_thick);

  /* Segment 4: vertical bar at bottom-left */
  draw_segment (cr, left, top + geom->seg_len + 2.0 * geom->seg_thick, 
                geom->seg_thick, geom->seg_len);

  /* Segment 5: vertical bar at top-left */
  draw_segment (cr, left, top + geom->seg_thick, 
                geom->seg_thick, geom->seg_len);

  /* Segment 6: horizontal bar at middle */
  draw_segment (cr, left + geom->seg_thick, 
                top + geom->seg_len + geom->seg_thick,
                geom->seg_len, geom->seg_thick);
}

/**
 * draw_colon_off:
 * @cr: Cairo context.
 * @x: Horizontal center.
 * @y: Top coordinate.
 * @geom: Pre-calculated segment geometry.
 *
 * Draw two dimmed dots when display is off.
 */
static void
draw_colon_off (cairo_t *cr, gdouble x, gdouble y, 
                const SegmentGeometry *geom)
{
  gdouble r = geom->size * COLON_RADIUS;
  gdouble dy_top = geom->seg_len * 0.5 + geom->seg_thick;
  gdouble dy_bottom = geom->seg_len * 1.5 + 2.0 * geom->seg_thick;

  cairo_arc (cr, x, y + dy_top, r, 0, 2.0 * M_PI);
  cairo_fill (cr);
  cairo_arc (cr, x, y + dy_bottom, r, 0, 2.0 * M_PI);
  cairo_fill (cr);
}

/**
 * draw_digit:
 * @cr: Cairo context.
 * @digit: Digit to draw (0-9).
 * @x: Left coordinate.
 * @y: Top coordinate.
 * @geom: Pre-calculated segment geometry.
 *
 * Draw a seven-segment representation of the given digit.
 * Only the segments corresponding to the digit value are drawn.
 */
static void
draw_digit (cairo_t *cr, gint digit, gdouble x, gdouble y, 
            const SegmentGeometry *geom)
{
  g_return_if_fail (digit >= 0 && digit <= 9);

  gdouble left = x;
  gdouble top  = y;

  /* Segment 0: horizontal bar at top */
  if (segments_by_digit[digit][0])
    draw_segment (cr, left + geom->seg_thick, top, 
                  geom->seg_len, geom->seg_thick);

  /* Segment 1: vertical bar at top-right */
  if (segments_by_digit[digit][1])
    draw_segment (cr, left + geom->seg_len + geom->seg_thick, 
                  top + geom->seg_thick, geom->seg_thick, geom->seg_len);

  /* Segment 2: vertical bar at bottom-right */
  if (segments_by_digit[digit][2])
    draw_segment (cr, left + geom->seg_len + geom->seg_thick,
                  top + geom->seg_len + geom->seg_thick * 2.0, 
                  geom->seg_thick, geom->seg_len);

  /* Segment 3: horizontal bar at bottom */
  if (segments_by_digit[digit][3])
    draw_segment (cr, left + geom->seg_thick, 
                  top + 2.0 * geom->seg_len + 2.0 * geom->seg_thick,
                  geom->seg_len, geom->seg_thick);

  /* Segment 4: vertical bar at bottom-left */
  if (segments_by_digit[digit][4])
    draw_segment (cr, left, top + geom->seg_len + 2.0 * geom->seg_thick, 
                  geom->seg_thick, geom->seg_len);

  /* Segment 5: vertical bar at top-left */
  if (segments_by_digit[digit][5])
    draw_segment (cr, left, top + geom->seg_thick, 
                  geom->seg_thick, geom->seg_len);

  /* Segment 6: horizontal bar at middle */
  if (segments_by_digit[digit][6])
    draw_segment (cr, left + geom->seg_thick, 
                  top + geom->seg_len + geom->seg_thick,
                  geom->seg_len, geom->seg_thick);
}

/**
 * draw_colon:
 * @cr: Cairo context.
 * @x: Horizontal center.
 * @y: Top coordinate.
 * @geom: Pre-calculated segment geometry.
 *
 * Draw two dots as separator between digit groups (HH:MM:SS).
 */
static void
draw_colon (cairo_t *cr, gdouble x, gdouble y, const SegmentGeometry *geom)
{
  gdouble r         = geom->size * COLON_RADIUS;
  gdouble dy_top    = geom->seg_len * 0.5 + geom->seg_thick;
  gdouble dy_bottom = geom->seg_len * 1.5 + 2.0 * geom->seg_thick;

  cairo_arc (cr, x, y + dy_top, r, 0, 2.0 * M_PI);
  cairo_fill (cr);
  cairo_arc (cr, x, y + dy_bottom, r, 0, 2.0 * M_PI);
  cairo_fill (cr);
}

/**
 * draw_digit_structure:
 * @cr: Cairo context.
 * @digit_y: Y coordinate for digits.
 * @geom: Pre-calculated segment geometry.
 * @layout: Pre-calculated display layout.
 *
 * Helper function to draw the ghost structure of all 6 digits and 2 colons.
 * This reduces code duplication between display_off and active display modes.
 */
static void
draw_digit_structure (cairo_t *cr, gdouble digit_y,
                      const SegmentGeometry *geom,
                      const DisplayLayout *layout)
{
  for (gint i = 0; i < NUM_DIGITS; i++)
    {
      draw_digit_off (cr, layout->digit_x[i], digit_y, geom);
    }

  for (gint i = 0; i < NUM_COLONS; i++)
    {
      draw_colon_off (cr, layout->colon_x[i], digit_y, geom);
    }
}

/**
 * timer_state_new:
 *
 * Allocates and initializes a new #TimerState structure. The timer is
 * initially set to not running, with zero elapsed time.
 *
 * Returns: (transfer full): A pointer to the newly allocated #TimerState.
 * The caller is responsible for freeing this memory using
 * timer_state_free().
 */
TimerState *
timer_state_new (void)
{
  TimerState *state     = g_new0 (TimerState, 1);
  state->running        = FALSE;
  state->elapsed_before = 0;
  return state;
}

/**
 * timer_state_free:
 * @state: (transfer full): The #TimerState structure to free.
 *
 * Frees a #TimerState structure previously allocated by timer_state_new().
 * If the timer is still running, it will be stopped first to properly
 * update the elapsed time.
 * Does nothing if @state is %NULL.
 */
void
timer_state_free (TimerState *state)
{
  if (! state)
    return;

  /* Stop timer before freeing to ensure elapsed time is properly recorded */
  if (state->running)
    timer_stop (state);

  g_free (state);
}

/**
 * timer_start:
 * @state: (inout): The #TimerState to start.
 *
 * Starts the timer. If the timer is not currently running, it records the
 * current system time (`time(NULL)`) as the `start_time`.
 * If the timer is already running, the function performs a no-op.
 */
void
timer_start (TimerState *state)
{
  g_return_if_fail (state != NULL);

  if (! state->running)
    {
      state->running    = TRUE;
      state->start_time = time (NULL);
    }
}

/**
 * timer_stop:
 * @state: (inout): The #TimerState to stop.
 *
 * Stops the timer. If the timer is running, it calculates the elapsed time
 * since the last `timer_start` call and adds it to `elapsed_before`.
 * The running state is then set to %FALSE.
 */
void
timer_stop (TimerState *state)
{
  g_return_if_fail (state != NULL);

  if (state->running)
    {
      state->elapsed_before += time (NULL) - state->start_time;
      state->running = FALSE;
    }
}

/**
 * timer_reset:
 * @state: (inout): The #TimerState to reset.
 *
 * Resets the timer's internal state, setting the running flag to %FALSE
 * and the accumulated elapsed time (`elapsed_before`) to zero.
 */
void
timer_reset (TimerState *state)
{
  g_return_if_fail (state != NULL);

  state->running        = FALSE;
  state->elapsed_before = 0;
}

/**
 * timer_get_elapsed:
 * @state: (transfer none): The #TimerState to query.
 *
 * Computes the total time elapsed since the timer was first started (or
 * last reset), including time accumulated during running sessions and
 * time recorded in `elapsed_before`.
 *
 * Returns: The total elapsed time in seconds.
 */
time_t
timer_get_elapsed (const TimerState *state)
{
  g_return_val_if_fail (state != NULL, 0);

  time_t total = state->elapsed_before;
  if (state->running)
    total += time (NULL) - state->start_time;
  return total;
}

/**
 * timer_draw_overlay:
 * @cr: Cairo context used for drawing.
 * @state: (transfer none): The #TimerState containing time information.
 * @x: Starting X coordinate for the drawing.
 * @y: Starting Y coordinate for the drawing.
 * @height: Total vertical space allocated for the timer display.
 *
 * Renders the seven-segment timer display onto the Cairo context @cr.
 *
 * The function:
 * - Calculates the current time (HH:MM:SS) based on @state, capping at 99:59:59
 * - Dynamically determines segment size based on the available @height
 * - Draws either the current time or a dimmed "off" display (88:88:88)
 * - Uses a two-pass rendering: ghost structure first, then active segments
 *
 * The display will show "88:88:88" in dimmed colors when the timer is not
 * running and has no elapsed time. Otherwise, it shows the actual time with
 * bright segments over dimmed background segments (LCD effect).
 */
void
timer_draw_overlay (cairo_t *cr, const TimerState *state,
                    gdouble x, gdouble y, gdouble height)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (state != NULL);
  g_return_if_fail (height > 0);

  cairo_save (cr);

  /* Compute elapsed time, capping at maximum displayable value */
  time_t total_elapsed = timer_get_elapsed (state);

  /* Check if display should be "off" (not running and no elapsed time) */
  gboolean display_off = (! state->running && total_elapsed == 0);

  /* Set ghost color for background structure */
  cairo_set_source_rgba (cr,
                         LCD_GHOST_COLOR.r,
                         LCD_GHOST_COLOR.g,
                         LCD_GHOST_COLOR.b,
                         LCD_GHOST_COLOR.a);

  /* Pre-calculate all geometry and layout positions */
  gdouble         size        = height / DIGIT_SCALE_FACTOR;
  SegmentGeometry geom        = calculate_geometry (size);
  gdouble         spacing     = size * DIGIT_SPACING;
  gdouble         colon_space = size * COLON_SPACING;
  DisplayLayout   layout = calculate_layout (x, &geom, spacing, colon_space);

  /* Vertical centering */
  gdouble digit_y = y + (height - (2.0 * size + 3.0 * geom.seg_thick)) / 2.0;

  if (display_off)
    {
      /* Draw all segments dimmed for "88:88:88" effect when inactive */
      draw_digit_structure (cr, digit_y, &geom, &layout);
    }
  else
    {
      /* First pass: draw ghost LCD structure for inactive segments */
      draw_digit_structure (cr, digit_y, &geom, &layout);

      /* Second pass: draw active segments with bright color */
      cairo_set_source_rgba (cr,
                            LCD_ACTIVE_COLOR.r,
                            LCD_ACTIVE_COLOR.g,
                            LCD_ACTIVE_COLOR.b,
                            LCD_ACTIVE_COLOR.a);

      /* Calculate time components, limiting hours to displayable range */
      gint hours   = (total_elapsed / 3600) % (MAX_HOURS + 1);
      gint minutes = (total_elapsed % 3600) / 60;
      gint seconds = total_elapsed % 60;

      /* Extract individual digits for display */
      gint vals[NUM_DIGITS] = {
        (hours / 10) % 10,
        hours % 10,
        minutes / 10,
        minutes % 10,
        seconds / 10,
        seconds % 10
      };

      /* Draw all six digits using pre-calculated positions */
      for (gint i = 0; i < NUM_DIGITS; i++)
        {
          draw_digit (cr, vals[i], layout.digit_x[i], digit_y, &geom);
        }

      /* Draw colons between digit groups */
      for (gint i = 0; i < NUM_COLONS; i++)
        {
          draw_colon (cr, layout.colon_x[i], digit_y, &geom);
        }
    }

  cairo_restore (cr);
}
