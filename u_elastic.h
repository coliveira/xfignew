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

#ifndef U_ELASTIC_H
#define U_ELASTIC_H

#define		MOVE_ARB	0
#define		MOVE_HORIZ_VERT 1
#define		BOX_SCALE	2
#define		BOX_HSTRETCH	3
#define		BOX_VSTRETCH	4

#define		MSG_RADIUS	0
#define		MSG_RADIUS2	1
#define		MSG_DIAM	2
#define		MSG_LENGTH	3
#define		MSG_DIST	4
#define		MSG_PNTS_LENGTH	5
#define		MSG_DIAM_ANGLE	6
#define		MSG_RADIUS_ANGLE 7

extern int	constrained;
extern float	cur_angle;
extern int	work_numsides;
extern int	x1off, x2off, y1off, y2off;
extern Cursor	cur_latexcursor;
extern int	from_x, from_y;
extern double	cosa, sina;
extern int	movedpoint_num;
extern F_point *left_point, *right_point;

extern void	elastic_box(int x1, int y1, int x2, int y2);
extern void	elastic_fixedbox(void);
extern void	elastic_movebox(void);
extern void	resizing_box(int x, int y);
extern void	elastic_box_constrained();
extern void	constrained_resizing_box(int x, int y);
extern void	constrained_resizing_scale_box(int x, int y);
extern void	moving_box(int x, int y);

extern void	elastic_poly(int x1, int y1, int x2, int y2, int numsides);
extern void	resizing_poly(int x, int y);
extern void	scaling_compound(int x, int y);
extern void	elastic_scalecompound(F_compound *c);
extern void	elastic_scale_curcompound(void);

extern void	resizing_cbr(int x, int y), elastic_cbr(void), resizing_cbd(int x, int y), elastic_cbd(void);
extern void	resizing_ebr(int x, int y), elastic_ebr(void), resizing_ebd(int x, int y), elastic_ebd(void);
extern void	constrained_resizing_ebr(int x, int y), constrained_resizing_ebd(int x, int y);
extern void	constrained_resizing_cbd(int x, int y);
extern void	elastic_moveellipse(void);
extern void	moving_ellipse(int x, int y);
extern void	elastic_scaleellipse(F_ellipse *e);
extern void	scaling_ellipse(int x, int y);
extern void	elastic_scale_curellipse(void);

extern void	unconstrained_line(int x, int y);
extern void	latex_line(int x, int y);
extern void	constrainedangle_line(int x, int y);
extern void	elastic_moveline(F_point *pts);
extern void	elastic_movenewline(void);
extern void	elastic_line(void);
extern void	elastic_dimension_line();
extern void	moving_line(int x, int y);
extern void	reshaping_line(int x, int y);
extern void	reshaping_latexline();
extern void	elastic_linelink(void);
extern void	elastic_scalepts(F_point *pts);
extern void	scaling_line(int x, int y);
extern void	elastic_scale_curline(int x, int y);

extern void	arc_point(int x, int y, int numpoint);
extern void	moving_arc(int x, int y);
extern void	elastic_movearc(F_arc *a);
extern void	elastic_movenewarc(void);
extern void	reshaping_arc(int x, int y);
extern void	elastic_arclink(void);
extern void	scaling_arc(int x, int y);
extern void	elastic_scalearc(F_arc *a);
extern void	elastic_scale_curarc(void);

extern void	moving_text(int x, int y);
extern void	draw_movingtext();
extern void	elastic_movetext(void);

extern void	moving_spline(int x, int y);
extern void	elastic_movenewspline(void);
extern void	scaling_spline(int x, int y);
extern void	elastic_scale_curspline(void);

extern void	adjust_box_pos(int curs_x, int curs_y, int orig_x, int orig_y, int *ret_x, int *ret_y);
extern void	adjust_pos(int curs_x, int curs_y, int orig_x, int orig_y, int *ret_x, int *ret_y);

#endif /* U_ELASTIC_H */
