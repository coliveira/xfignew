/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

extern double	compute_angle(double dx, double dy);	/* compute the angle between 0 to 2PI  */
extern int close_to_arc (F_arc *a, int xp, int yp, int d, float *px, float *py);
extern int close_to_ellipse (F_ellipse *e, int xp, int yp, int d, float *ex, float *ey, float *vx, float *vy);
extern int close_to_polyline (F_line *l, int xp, int yp, int d, int sd, int *px, int *py, int *lx1, int *ly1, int *lx2, int *ly2);
extern int close_to_spline (F_spline *spline, int xp, int yp, int d, int *px, int *py, int *lx1, int *ly1, int *lx2, int *ly2);
extern int close_to_vector (int x1, int y1, int x2, int y2, int xp, int yp, int d, float dd, int *px, int *py);
extern int compute_3p_angle (F_point *p1, F_point *p2, F_point *p3, double *alpha);
extern int compute_arc_angle (F_arc *a, double *alpha);
extern int compute_arc_area (F_arc *a, float *ap);
extern int compute_arc_length (F_arc *a, float *lp);
extern int compute_arccenter (F_pos p1, F_pos p2, F_pos p3, float *x, float *y);
extern int compute_arcradius (int x1, int y1, int x2, int y2, int x3, int y3, float *r);
extern int compute_direction (F_pos p1, F_pos p2, F_pos p3);
extern int compute_ellipse_area (F_ellipse *e, float *ap);
extern int compute_line_angle (F_line *l, F_point *p, double *alpha);
extern void compute_normal (float x1, float y1, int x2, int y2, int direction, int *x, int *y);
extern int compute_poly_length (F_line *l, float *lp);
extern void latex_endpoint (int x1, int y1, int x2, int y2, int *xout, int *yout, int arrow, int magnet);
extern void compute_poly_area (F_line *l, float *ap);

