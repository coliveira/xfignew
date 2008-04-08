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

#ifndef U_BOUND_H
#define U_BOUND_H

extern int	overlapping(int xmin1, int ymin1, int xmax1, int ymax1, int xmin2, int ymin2, int xmax2, int ymax2);
extern int	floor_coords(int x);
extern int	ceil_coords(int x);
extern void arc_bound (F_arc *arc, int *xmin, int *ymin, int *xmax, int *ymax);
extern void compound_bound (F_compound *compound, int *xmin, int *ymin, int *xmax, int *ymax);
extern void ellipse_bound (F_ellipse *e, int *xmin, int *ymin, int *xmax, int *ymax);
extern void line_bound (F_line *l, int *xmin, int *ymin, int *xmax, int *ymax);
extern void spline_bound (F_spline *s, int *xmin, int *ymin, int *xmax, int *ymax);
extern void text_bound (F_text *t, int *xmin, int *ymin, int *xmax, int *ymax, int *rx1, int *ry1, int *rx2, int *ry2, int *rx3, int *ry3, int *rx4, int *ry4);

#endif /* U_BOUND_H */
