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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "e_rotate.h"
#include "u_draw.h"
#include "u_search.h"
#include "u_create.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"

#include "u_markers.h"
#include "u_redraw.h"
#include "w_cursor.h"

/* EXPORTS */

int		setanchor;
int		setanchor_x;
int		setanchor_y;

static int	flip_axis;
static int	copy;

static void	init_flip(F_line *p, int type, int x, int y, int px, int py);
static void	init_copynflip(F_line *p, int type, int x, int y, int px, int py);
static void	set_unset_anchor(int x, int y);
static void	init_fliparc(F_arc *old_a, int px, int py);
static void	init_flipcompound(F_compound *old_c, int px, int py);
static void	init_flipellipse(F_ellipse *old_e, int px, int py);
static void	init_flipline(F_line *old_l, int px, int py);
static void	init_flipspline(F_spline *old_s, int px, int py);
static void	flip_selected(void);
static void	flip_search(F_line *p, int type, int x, int y, int px, int py);


void flip_arc (F_arc *a, int x, int y, int flip_axis);
void flip_compound (F_compound *c, int x, int y, int flip_axis);
void flip_ellipse (F_ellipse *e, int x, int y, int flip_axis);
void flip_line (F_line *l, int x, int y, int flip_axis);
void flip_spline (F_spline *s, int x, int y, int flip_axis);

void
flip_ud_selected(void)
{
    flip_axis = UD_FLIP;
    /* erase any existing anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
    /* and any center */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    setcenter = 0;
    setanchor = 0;
    flip_selected();
    reset_action_on();
}

void
flip_lr_selected(void)
{
    flip_axis = LR_FLIP;
    /* erase any existing anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
    /* and any center */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    setcenter = 0;
    setanchor = 0;
    flip_selected();
    reset_action_on();
}

static void
flip_selected(void)
{
    set_mousefun("flip", "copy & flip", "set anchor", 
			LOC_OBJ, LOC_OBJ, "set anchor");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_flip);
    init_searchproc_middle(init_copynflip);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = set_unset_anchor;
    set_cursor(pick15_cursor);
}

static void
set_unset_anchor(int x, int y)
{
    if (setanchor) {
      set_mousefun("flip", "copy & flip", "set anchor", 
			LOC_OBJ, LOC_OBJ, "set anchor");
      draw_mousefun_canvas();
      setanchor = 0;
      center_marker(setanchor_x,setanchor_y);
      /* second call to center_mark on same position deletes */
    }
    else {
      set_mousefun("flip", "copy & flip", "unset anchor", 
			LOC_OBJ, LOC_OBJ, "unset anchor");
      draw_mousefun_canvas();
      setanchor = 1;
      setanchor_x = x;
      setanchor_y = y;
      center_marker(setanchor_x,setanchor_y);
    }
      
}

static void
init_flip(F_line *p, int type, int x, int y, int px, int py)
{
    copy = 0;
    if (setanchor) 
	flip_search(p, type, x, y,setanchor_x,setanchor_y );
	/* remember rotation center, e.g for multiple rotation*/
    else
	flip_search(p, type, x, y, px, py);
}

static void
init_copynflip(F_line *p, int type, int x, int y, int px, int py)
{
    copy = 1;
    if (setanchor) 
	flip_search(p, type, x, y,setanchor_x,setanchor_y );
	/* remember rotation center, e.g for multiple rotation*/
    else
	flip_search(p, type, x, y, px, py);
}

static void
flip_search(F_line *p, int type, int x, int y, int px, int py)
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) p;
	init_flipline(cur_l, px, py);
	break;
    case O_ARC:
	cur_a = (F_arc *) p;
	init_fliparc(cur_a, px, py);
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) p;
	init_flipellipse(cur_e, px, py);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	init_flipspline(cur_s, px, py);
	break;
    case O_COMPOUND:
	cur_c = (F_compound *) p;
	init_flipcompound(cur_c, px, py);
	break;
    default:
	return;
    }
}

static void
init_fliparc(F_arc *old_a, int px, int py)
{
    F_arc	   *new_a;

    set_temp_cursor(wait_cursor);
    new_a = copy_arc(old_a);
    flip_arc(new_a, px, py, flip_axis);
    if (copy) {
	add_arc(new_a);
    } else {
	toggle_arcmarker(old_a);
	draw_arc(old_a, ERASE);
	change_arc(old_a, new_a);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_arc(old_a);
    /* and this arc and any other objects on top */
    redisplay_arc(new_a);
    reset_cursor();
}

static void
init_flipcompound(F_compound *old_c, int px, int py)
{
    F_compound	   *new_c;

    set_temp_cursor(wait_cursor);
    new_c = copy_compound(old_c);
    flip_compound(new_c, px, py, flip_axis);
    if (copy) {
	add_compound(new_c);
    } else {
	toggle_compoundmarker(old_c);
	draw_compoundelements(old_c, ERASE);
	change_compound(old_c, new_c);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_compound(old_c);
    /* and this object and any other objects on top */
    redisplay_compound(new_c);
    reset_cursor();
}

static void
init_flipellipse(F_ellipse *old_e, int px, int py)
{
    F_ellipse	   *new_e;

    new_e = copy_ellipse(old_e);
    flip_ellipse(new_e, px, py, flip_axis);
    if (copy) {
	add_ellipse(new_e);
    } else {
	toggle_ellipsemarker(old_e);
	draw_ellipse(old_e, ERASE);
	change_ellipse(old_e, new_e);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_ellipse(old_e);
    /* and this object and any other objects on top */
    redisplay_ellipse(new_e);
}

static void
init_flipline(F_line *old_l, int px, int py)
{
    F_line	   *new_l;

    new_l = copy_line(old_l);
    flip_line(new_l, px, py, flip_axis);
    if (copy) {
	add_line(new_l);
    } else {
	toggle_linemarker(old_l);
	draw_line(old_l, ERASE);
	change_line(old_l, new_l);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_line(old_l);
    /* and this object and any other objects on top */
    redisplay_line(new_l);
}

static void
init_flipspline(F_spline *old_s, int px, int py)
{
    F_spline	   *new_s;

    new_s = copy_spline(old_s);
    flip_spline(new_s, px, py, flip_axis);
    if (copy) {
	add_spline(new_s);
    } else {
	toggle_splinemarker(old_s);
	draw_spline(old_s, ERASE);
	change_spline(old_s, new_s);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_spline(old_s);
    /* and this object and any other objects on top */
    redisplay_spline(new_s);
}

void flip_line(F_line *l, int x, int y, int flip_axis)
{
    F_point	   *p;

    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	for (p = l->points; p != NULL; p = p->next)
	    p->y = y + (y - p->y);
	break;
    case LR_FLIP:		/* y axis  */
	for (p = l->points; p != NULL; p = p->next)
	    p->x = x + (x - p->x);
	break;
    }
    if (l->type == T_PICTURE)
	l->pic->flipped = 1 - l->pic->flipped;
}

void flip_spline(F_spline *s, int x, int y, int flip_axis)
{
    F_point	   *p;

    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	for (p = s->points; p != NULL; p = p->next)
	    p->y = y + (y - p->y);
	break;
    case LR_FLIP:		/* y axis  */
	for (p = s->points; p != NULL; p = p->next)
	    p->x = x + (x - p->x);
	break;
    }
}

void flip_text(F_text *t, int x, int y, int flip_axis)
{
    double	    sina, cosa;

    while (t->angle > M_2PI)
	t->angle -= M_2PI;
    /* flip the angle around 2PI */
    t->angle = M_2PI - t->angle;
    switch (flip_axis) {
      case LR_FLIP:		/* left/right around the y axis  */
	t->base_x = x + (x - t->base_x);
	/* switch justification */
	if (t->type == T_LEFT_JUSTIFIED)
	    t->type = T_RIGHT_JUSTIFIED;
	else if (t->type == T_RIGHT_JUSTIFIED)
	    t->type = T_LEFT_JUSTIFIED;
	break;
      case UD_FLIP:		/* up/down around the x axis  */
	cosa = cos((double)t->angle);
	sina = sin((double)t->angle);
	t->base_x = t->base_x + round((t->ascent - t->descent)*sina);
	t->base_y = y + (y - t->base_y) + round((t->ascent - t->descent)*cosa);
	break;
    }
}

void flip_ellipse(F_ellipse *e, int x, int y, int flip_axis)
{
    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	e->direction ^= 1;
	e->center.y = y + (y - e->center.y);
	e->start.y = y + (y - e->start.y);
	e->end.y = y + (y - e->end.y);
	break;
    case LR_FLIP:		/* y axis  */
	e->direction ^= 1;
	e->center.x = x + (x - e->center.x);
	e->start.x = x + (x - e->start.x);
	e->end.x = x + (x - e->end.x);
	break;
    }
    e->angle = - e->angle;
}

void flip_arc(F_arc *a, int x, int y, int flip_axis)
{
    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	a->direction ^= 1;
	a->center.y = y + (y - a->center.y);
	a->point[0].y = y + (y - a->point[0].y);
	a->point[1].y = y + (y - a->point[1].y);
	a->point[2].y = y + (y - a->point[2].y);
	break;
    case LR_FLIP:		/* y axis  */
	a->direction ^= 1;
	a->center.x = x + (x - a->center.x);
	a->point[0].x = x + (x - a->point[0].x);
	a->point[1].x = x + (x - a->point[1].x);
	a->point[2].x = x + (x - a->point[2].x);
	break;
    }
}

void flip_compound(F_compound *c, int x, int y, int flip_axis)
{
    F_line	   *l;
    F_arc	   *a;
    F_ellipse	   *e;
    F_spline	   *s;
    F_text	   *t;
    F_compound	   *c1;
    int		    p, q;

    switch (flip_axis) {
    case UD_FLIP:		/* x axis  */
	p = y + (y - c->nwcorner.y);
	q = y + (y - c->secorner.y);
	c->nwcorner.y = min2(p, q);
	c->secorner.y = max2(p, q);
	break;
    case LR_FLIP:		/* y axis  */
	p = x + (x - c->nwcorner.x);
	q = x + (x - c->secorner.x);
	c->nwcorner.x = min2(p, q);
	c->secorner.x = max2(p, q);
	break;
    }
    for (l = c->lines; l != NULL; l = l->next)
	flip_line(l, x, y, flip_axis);
    for (a = c->arcs; a != NULL; a = a->next)
	flip_arc(a, x, y, flip_axis);
    for (e = c->ellipses; e != NULL; e = e->next)
	flip_ellipse(e, x, y, flip_axis);
    for (s = c->splines; s != NULL; s = s->next)
	flip_spline(s, x, y, flip_axis);
    for (t = c->texts; t != NULL; t = t->next)
	flip_text(t, x, y, flip_axis);
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next)
	flip_compound(c1, x, y, flip_axis);
}
