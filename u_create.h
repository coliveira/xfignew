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

#ifndef U_CREATE_H
#define U_CREATE_H

extern F_arc      *create_arc(void);
extern F_ellipse  *create_ellipse(void);
extern F_line     *create_line(void);
extern F_spline   *create_spline(void);
extern F_text     *create_text(void);
extern F_compound *create_compound(void);
extern F_pic      *create_pic(void);
extern F_point    *create_point(void);
extern F_sfactor  *create_sfactor(void);
extern F_compound  *create_dimension_line(F_line *line, Boolean add_to_figure);
extern void	  create_dimline_ticks(F_line *line, F_line **tick1, F_line **tick2);
extern struct _pics * create_picture_entry(void);

extern F_arc      *copy_arc(F_arc *a);
extern F_ellipse  *copy_ellipse(F_ellipse *e);
extern F_line     *copy_line(F_line *l);
extern F_spline   *copy_spline(F_spline *s);
extern F_text     *copy_text(F_text *t);
extern F_compound *copy_compound(F_compound *c);

extern void	  copy_comments(char **source, char **dest);
extern F_point   *copy_points(F_point *orig_pt);
extern F_sfactor *copy_sfactors(F_sfactor *orig_sf);
extern void       reverse_points(F_point *orig_pt);
extern void       reverse_sfactors(F_sfactor *orig_sf);

extern F_arrow	  *forward_arrow(void);
extern F_arrow	  *backward_arrow(void);
extern F_arrow	  *create_arrow(void);
extern F_arrow	  *forward_dim_arrow(void);
extern F_arrow	  *backward_dim_arrow(void);

extern F_arrow	  *new_arrow(int type, int style, float thickness, float wd, float ht);
extern char   	  *new_string(int len);
extern F_linkinfo *new_link(F_line *l, F_point *ep, F_point *pp);

#endif /* U_CREATE_H */
