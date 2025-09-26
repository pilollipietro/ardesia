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

#include "annotation_window.h"
#include "bezier_spline.h"
#include "utils.h"

/* 
 * Smooth a polyline with cubic Bézier curves.
 *
 * Given a list of AnnotatePoint (x,y,width,pressure), compute for each
 * segment (Xi, Xi+1) the two control points Pi and Qi that ensure C¹
 * continuity of the whole curve. The resulting GSList contains the control
 * points and end points to draw the smoothed curve.
 */
GSList *
spline (GSList *list)
{
    GSList *ret    = NULL;
    guint   i;
    guint   length = g_slist_length (list);

    if (length < 2)
        return NULL;

    /* Extract input coordinates from the GSList */
    gdouble mx[length][2]; /* input points */
    gdouble width    = 12.0;
    gdouble pressure = 1.0;

    for (i = 0; i < length; i++) {
        AnnotatePoint *point = (AnnotatePoint *) g_slist_nth_data (list, i);
        mx[i][0] = point->x;
        mx[i][1] = point->y;
        if (i == 0) {
            width    = point->width;
            pressure = point->pressure;
        }
    }

    /**************************************************************************
     * Build the linear system for the Bézier control points.
     *
     * For each segment (Xi, Xi+1) we want control points Pi and Qi such that
     * the composite Bézier curve is C¹–continuous. This leads to a block-
     * matrix system A·x = b of the form:
     *
     *     |    1              1           |   |P0  |      |  2*X1|
     *     | 1  2             -2 -1        |   |P1  |      |     0|
     *     |       1              1        |   |P2  |      |  2*X2|
     *     |    1  2             -2 -1     | * |Pn-1|   =  |     0|
     *     |          1              1     |   |Q0  |      |2*Xn-1|
     *     |       1  2             -2 -1  |   |Q1  |      |     0|
     *     | 1                             |   |Q2  |      |    X0|
     *     \                             1 /   \Qn-1/      \    Xn/
     *
     * where:
     *   – Xi are the input points,
     *   – Pi, Qi are the unknown control points,
     *   – the right-hand side b contains the coordinates of the Xi.
     *
     * Solving A·x = b separately for x and y gives the control points.
     **************************************************************************/

    /* Allocate matrix and RHS vectors */
    guint dim = 2 * (length - 1);
    gsl_matrix      *m  = gsl_matrix_calloc (dim, dim);
    gsl_vector      *bx = gsl_vector_calloc (dim);
    gsl_vector      *by = gsl_vector_calloc (dim);

    /* Fill the coefficient matrix A */
    guint eq = 0;
    for (i = 0; i < length - 2; i++) {
        /* Pi+1 + Qi = 2 Xi+1 */
        gsl_matrix_set (m, eq, i + 1, 1.0);
        gsl_matrix_set (m, eq, (length - 1) + i, 1.0);
        eq++;

        /* Pi + 2Pi+1 - Qi+1 - 2Qi = 0 */
        gsl_matrix_set (m, eq, i, 1.0);
        gsl_matrix_set (m, eq, i + 1, 2.0);
        gsl_matrix_set (m, eq, (length - 1) + i + 1, -1.0);
        gsl_matrix_set (m, eq, (length - 1) + i, -2.0);
        eq++;
    }

    /* Boundary conditions: P0 = X0, Qn-1 = Xn */
    gsl_matrix_set (m, eq++, 0, 1.0);
    gsl_matrix_set (m, eq++, dim - 1, 1.0);

    /* Fill RHS vectors b */
    for (i = 0; i < length - 2; i++) {
        gsl_vector_set (bx, 2 * i, 2.0 * mx[i + 1][0]);
        gsl_vector_set (by, 2 * i, 2.0 * mx[i + 1][1]);
    }
    gsl_vector_set (bx, dim - 2, mx[0][0]);
    gsl_vector_set (bx, dim - 1, mx[length - 1][0]);
    gsl_vector_set (by, dim - 2, mx[0][1]);
    gsl_vector_set (by, dim - 1, mx[length - 1][1]);

    /* Solve the system for x and y separately */
    gsl_permutation *perm = gsl_permutation_alloc (dim);
    int s;
    gsl_linalg_LU_decomp (m, perm, &s);

    gsl_vector *solx = gsl_vector_calloc (dim);
    gsl_linalg_LU_solve (m, perm, bx, solx);

    gsl_vector *soly = gsl_vector_calloc (dim);
    gsl_linalg_LU_solve (m, perm, by, soly);

    /* Free the linear system objects */
    gsl_matrix_free (m);
    gsl_vector_free (bx);
    gsl_vector_free (by);
    gsl_permutation_free (perm);

    /* Generate smoothed points directly from solx/soly */
    for (i = 0; i < length - 1; i++) {
        gdouble px = gsl_vector_get (solx, i);
        gdouble qx = gsl_vector_get (solx, i + (length - 1));
        gdouble py = gsl_vector_get (soly, i);
        gdouble qy = gsl_vector_get (soly, i + (length - 1));

        AnnotatePoint *first_point  = allocate_point (px,
			                              py,
						      width,
						      pressure);

        AnnotatePoint *second_point = allocate_point (qx,
			                              qy,
						      width,
						      pressure);

        AnnotatePoint *third_point  = allocate_point (mx[i + 1][0],
			                              mx[i + 1][1],
                                                      width,
						      pressure);

        ret = g_slist_prepend (ret, first_point);
        ret = g_slist_prepend (ret, second_point);
        ret = g_slist_prepend (ret, third_point);
    }

    gsl_vector_free (solx);
    gsl_vector_free (soly);

    ret = g_slist_reverse (ret);
    return ret;
}
