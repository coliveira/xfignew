#ifndef U_SMARTSEARCH_H
#define U_SMARTSEARCH_H
/*
 * FIG : Facility for Interactive Generation of figures
 * This part Copyright (c) 1999-2002 by A. Durner
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

void		init_smart_searchproc_left(int (*handlerproc) (/* ??? */));
void		init_smart_searchproc_middle(int (*handlerproc) (/* ??? */));
void		init_smart_searchproc_right(int (*handlerproc) (/* ??? */));

void		smart_object_search_left(int x, int y, unsigned int shift);
void		smart_object_search_middle(int x, int y, unsigned int shift);
void		smart_object_search_right(int x, int y, unsigned int shift);
void		smart_null_proc(void);

extern F_point  smart_point1, smart_point2;

#endif /* U_SMARTSEARCH_H */

