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

extern void	splinepoint_adding(F_spline *spline, F_point *left_point, F_point *added_point, F_point *right_point, double sfactor);
extern void	linepoint_adding(F_line *line, F_point *left_point, F_point *added_point);
extern void	find_endpoints(F_point *p, int x, int y, F_point **fp, F_point **sp);
extern void point_adding_selected (void);
