/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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

#ifndef U_DRAW_H
#define U_DRAW_H

#include "w_drawprim.h"

#define DRAW_POINTS		True
#define DONT_DRAW_POINTS	False
#define DRAW_CENTER		True
#define DONT_DRAW_CENTER	False

/*
 * declarations of routines for drawing objects
 */
/* compounds */

void	draw_compoundelements(F_compound *c, int op);

/* splines */

void	draw_spline(F_spline *spline, int op);
void	quick_draw_spline(F_spline *spline, int op);

/* curve routine needed by arc() and show_boxradius() */

void	curve(Window window, int depth, int xstart, int ystart, int xend, int yend, 
		Boolean draw_points, Boolean draw_center, int direction,
		int a, int b, int xoff, int yoff, int op, int thick,
		int style, float style_val, int fill_style, 
		Color pen_color, Color fill_color, int cap_style);
 
extern void angle_ellipse (int center_x, int center_y, int radius_x, int radius_y, float angle, int op, int depth, int thickness, int style, float style_val, int fill_style, int pen_color, int fill_color);
extern void calc_arrow (int x1, int y1, int x2, int y2, int linethick, F_arrow *arrow, zXPoint *points, int *npoints, zXPoint *fillpoints, int *nfillpoints, zXPoint *clippts, int *nclippts);
extern void compute_arcarrow_angle (float x1, float y1, int x2, int y2, int direction, F_arrow *arrow, int *x, int *y);
extern void draw_arc (F_arc *a, int op);
extern void draw_ellipse (F_ellipse *e, int op);
extern void draw_line (F_line *line, int op);
extern void draw_text (F_text *text, int op);
extern void redraw_images (F_compound *obj);
extern void too_many_points (void);

#endif /* U_DRAW_H */
