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

#ifndef E_CONVERT_H
#define E_CONVERT_H

extern void     spline_line(F_spline *s);
extern void     line_spline(F_line *l, int type_value);
extern void     toggle_polyline_polygon(F_line *line, F_point *previous_point, F_point *selected_point);
extern void     toggle_open_closed_spline(F_spline *spline, F_point *previous_point, F_point *selected_point);
extern void	box_2_box(F_line *old_l);
extern void convert_selected (void);

#endif /* E_CONVERT_H */
