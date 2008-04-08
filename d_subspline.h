/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
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

#ifndef D_SUBSPLINE_H
#define D_SUBSPLINE_H

extern F_spline  *create_subspline(int *num_pts, F_spline *spline, F_point *point, F_sfactor **sfactor, F_sfactor **sub_sfactor);
extern void       free_subspline(int num_pts, F_spline **spline);
extern void       draw_subspline(int num_pts, F_spline *spline, int op);

#endif /* D_SUBSPLINE_H */
