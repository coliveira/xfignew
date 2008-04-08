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

#ifndef U_LIST_H
#define U_LIST_H

void		list_delete_arc(F_arc **arc_list, F_arc *arc);
void		list_delete_ellipse(F_ellipse **ellipse_list, F_ellipse *ellipse);
void		list_delete_line(F_line **line_list, F_line *line);
void		list_delete_spline(F_spline **spline_list, F_spline *spline);
void		list_delete_text(F_text **text_list, F_text *text);
void		list_delete_compound(F_compound **list, F_compound *compound);
void		remove_depth(int type, int depth);
void		remove_compound_depth(F_compound *comp);

void		list_add_arc(F_arc **list, F_arc *a);
void		list_add_ellipse(F_ellipse **list, F_ellipse *e);
void		list_add_line(F_line **list, F_line *l);
void		list_add_spline(F_spline **list, F_spline *s);
void		list_add_text(F_text **list, F_text *t);
void		list_add_compound(F_compound **list, F_compound *c);
void		add_depth(int type, int depth);
void		add_compound_depth(F_compound *comp);

F_line	       *last_line(F_line *list);
F_arc	       *last_arc(F_arc *list);
F_ellipse      *last_ellipse(F_ellipse *list);
F_text	       *last_text(F_text *list);
F_spline       *last_spline(F_spline *list);
F_compound     *last_compound(F_compound *list);
F_point	       *last_point(F_point *list);
F_sfactor      *last_sfactor(F_sfactor *list);

Boolean         first_spline_point(int x, int y, double s, F_spline *spline);
Boolean         append_sfactor(double s, F_sfactor *cpoint);
F_point        *search_spline_point(F_spline *spline, int x, int y);
F_point        *search_line_point(F_line *line, int x, int y);
F_sfactor      *search_sfactor(F_spline *spline, F_point *selected_point);
Boolean         insert_point(int x, int y, F_point *point);
int            num_points(F_point *points);

F_line	       *prev_line(F_line *list, F_line *line);
F_arc	       *prev_arc(F_arc *list, F_arc *arc);
F_ellipse      *prev_ellipse(F_ellipse *list, F_ellipse *ellipse);
F_text	       *prev_text(F_text *list, F_text *text);
F_spline       *prev_spline(F_spline *list, F_spline *spline);
F_compound     *prev_compound(F_compound *list, F_compound *compound);
F_point	       *prev_point(F_point *list, F_point *point);

void		delete_line(F_line *old_l);
void		delete_arc(F_arc *old_a);
void		delete_ellipse(F_ellipse *old_e);
void		delete_text(F_text *old_t);
void		delete_spline(F_spline *old_s);
void		delete_compound(F_compound *old_c);

void		add_line(F_line *new_l);
void		add_arc(F_arc *new_a);
void		add_ellipse(F_ellipse *new_e);
void		add_text(F_text *new_t);
void		add_spline(F_spline *new_s);
void		add_compound(F_compound *new_c);

void		change_line(F_line *old_l, F_line *new_l);
void		change_arc(F_arc *old_a, F_arc *new_a);
void		change_ellipse(F_ellipse *old_e, F_ellipse *new_e);
void		change_text(F_text *old_t, F_text *new_t);
void		change_spline(F_spline *old_s, F_spline *new_s);
void		change_compound(F_compound *old_c, F_compound *new_c);

void		get_links(int llx, int lly, int urx, int ury);
void		adjust_links(int mode, F_linkinfo *links, int dx, int dy, int cx, int cy, float sx, float sy, Boolean copying);
extern void append_objects (F_compound *l1, F_compound *l2, F_compound *tails);
extern void cut_objects (F_compound *objects, F_compound *tails);
extern int object_count (F_compound *list);
extern void set_tags (F_compound *list, int tag);
extern void get_interior_links (int llx, int lly, int urx, int ury);
extern void append_point(int x, int y, F_point **point);
extern void remove_arc_depths (F_arc *a);
extern void remove_ellipse_depths (F_ellipse *e);
extern void remove_line_depths (F_line *l);
extern void remove_spline_depths (F_spline *s);
extern void remove_text_depths (F_text *t);
extern void tail(F_compound *ob, F_compound *tails);

#endif /* U_LIST_H */
