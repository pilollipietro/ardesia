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

#include <ctype.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "annotation_window.h"

#ifdef _WIN32
#include <cairo-win32.h>
#else
#ifdef __APPLE__
#include <cairo-quartz.h>
#else
#include <cairo-xlib.h>
#endif
#endif

/**
 * CAIRO_ARGB32_SET_PIXEL:
 * @d: pointer to the destination buffer
 * @r: red component, not pre-multiplied
 * @g: green component, not pre-multiplied
 * @b: blue component, not pre-multiplied
 * @a: alpha component
 *
 * Sets a single pixel in an Cairo image surface in %CAIRO_FORMAT_ARGB32.
 *
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAIRO_ARGB32_SET_PIXEL (d, r, g, b, a) \
  G_STMT_START                                 \
  {                                            \
    guint tr = (a) * (r) + 0x80;               \
    guint tg = (a) * (g) + 0x80;               \
    guint tb = (a) * (b) + 0x80;               \
    d[0]     = (((tb) >> 8) + (tb)) >> 8;      \
    d[1]     = (((tg) >> 8) + (tg)) >> 8;      \
    d[2]     = (((tr) >> 8) + (tr)) >> 8;      \
    d[3]     = (a);                            \
  }                                            \
  G_STMT_END
#else
#define GIMP_CAIRO_ARGB32_SET_PIXEL (d, r, g, b, a) \
  G_STMT_START                                      \
  {                                                 \
    guint tr = (a) * (r) + 0x80;                    \
    guint tg = (a) * (g) + 0x80;                    \
    guint tb = (a) * (b) + 0x80;                    \
    d[0]     = (a);                                 \
    d[1]     = (((tr) >> 8) + (tr)) >> 8;           \
    d[2]     = (((tg) >> 8) + (tg)) >> 8;           \
    d[3]     = (((tb) >> 8) + (tb)) >> 8;           \
  }                                                 \
  G_STMT_END
#endif

/* Macro to get the rgba value of the pixel at coordinate (x,y).*/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAIRO_ARGB32_GET_PIXEL (s, r, g, b, a) \
  G_STMT_START                                 \
  {                                            \
    const guint tb = (s)[0];                   \
    const guint tg = (s)[1];                   \
    const guint tr = (s)[2];                   \
    const guint ta = (s)[3];                   \
    (r)            = (tr << 8) / (ta + 1);     \
    (g)            = (tg << 8) / (ta + 1);     \
    (b)            = (tb << 8) / (ta + 1);     \
    (a)            = ta;                       \
  }                                            \
  G_STMT_END
#else
#define CAIRO_ARGB32_GET_PIXEL (s, r, g, b, a) \
  G_STMT_START                                 \
  {                                            \
    const guint ta = (s)[0];                   \
    const guint tr = (s)[1];                   \
    const guint tg = (s)[2];                   \
    const guint tb = (s)[3];                   \
    (r)            = (tr << 8) / (ta + 1);     \
    (g)            = (tg << 8) / (ta + 1);     \
    (b)            = (tb << 8) / (ta + 1);     \
    (a)            = ta;                       \
  }                                            \
  G_STMT_END
#endif

#define RGB_TO_UINT (r, g, b) \
  ((((guint) (r)) << 16) | (((guint) (g)) << 8) | ((guint) (b)))
#define RGB_TO_RGBA (x, a)        (((x) << 8) | ((((guint) a) & 0xff)))
#define RGBA_TO_UINT (r, g, b, a) RGB_TO_RGBA (RGB_TO_UINT (r, g, b), a)
#define UINT_RGBA_R (x)           (((guint) (x)) >> 24)
#define UINT_RGBA_G (x)           ((((guint) (x)) >> 16) & 0xff)
#define UINT_RGBA_B (x)           ((((guint) (x)) >> 8) & 0xff)
#define UINT_RGBA_A (x)           (((guint) (x)) & 0xff)

/*
 * Perform a fill operation starting from the specified point (x, y)
 * within a closed path in the provided annotation data.
 *
 * Parameters:
 *   annotation_data - pointer to the annotation data containing the path
 *   x, y            - coordinates of the starting point for the fill
 *
 * This function fills the interior of a closed shape starting at
 * the given point.
 */
void fill (AnnotateData *annotation_data,
           gdouble x,
           gdouble y);
