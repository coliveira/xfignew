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

#ifndef U_SEARCH_H
#define U_SEARCH_H

Boolean		in_text_bound(F_text *t, int x, int y, int *posn, Boolean extra);

void		init_searchproc_left(int (*handlerproc) (/* ??? */));
void		init_searchproc_middle(int (*handlerproc) (/* ??? */));
void		init_searchproc_right(int (*handlerproc) (/* ??? */));

void		point_search_left(int x, int y, unsigned int shift);
void		point_search_middle(int x, int y, unsigned int shift);
void		point_search_right(int x, int y, unsigned int shift);

void		object_search_left(int x, int y, unsigned int shift);
void		object_search_middle(int x, int y, unsigned int shift);
void		object_search_right(int x, int y, unsigned int shift);

void		erase_objecthighlight(void);

F_text	       *text_search(int x, int y, int *posn);
F_compound     *compound_search(int x, int y, int tolerance, int *px, int *py);
F_compound     *compound_point_search(int x, int y, int tol, int *cx, int *cy, int *fx, int *fy);
F_spline       *get_spline_point(int x, int y, F_point **p, F_point **q);


#endif /* U_SEARCH_H */
