/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2004 by Chris Moller
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

#ifndef W_INTERSECT_H
#define W_INTERSECT_H

typedef struct {
  int seg_idx;
  int x;
  int y;
} isects_s;

typedef struct {
  isects_s * isects;
  int nr_isects;
  int max_isects;
} isect_cb_s;


/* MUST BE >= 2 */
#define ISECTS_INCR 8

extern void insert_isect(isect_cb_s * isect_cb, double ix, double iy, int seg_idx);

extern void intersect_ellipse_ellipse_handler(F_ellipse *  e1, F_ellipse *  e2, int x, int y,
					      isect_cb_s * isect_cb );

extern void intersect_ellipse_arc_handler(F_ellipse * e, F_arc * a, int x, int y,
					  isect_cb_s * isect_cb);

extern void intersect_ellipse_polyline_handler(F_ellipse * e, F_line *  l, int x, int y, 
					       isect_cb_s * isect_cb);

extern void intersect_arc_arc_handler(F_arc * a1, F_arc * a2, int x, int y,
				      isect_cb_s * isect_cb);

extern void intersect_polyline_arc_handler(F_line * l,  F_arc *a,  int x, int y,
					   isect_cb_s * isect_cb);

extern void intersect_polyline_polyline_handler(F_line * l1, F_line * l2, int x, int y,
						isect_cb_s * isect_cb);

extern void snap_intersect_handler(void * obj1, int type1, void * obj2, int type2, int x, int y);

extern F_line * build_text_bounding_box(F_text * t);

#endif
