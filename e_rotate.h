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

extern int	setcenter;
extern int	setcenter_x;
extern int	setcenter_y;
extern int	rotn_dirn;
extern float	act_rotnangle;

extern int rotate_compound (F_compound *c, int x, int y);
extern int rotate_line (F_line *l, int x, int y);
extern void rotate_ccw_selected (void);
extern void rotate_cw_selected (void);
