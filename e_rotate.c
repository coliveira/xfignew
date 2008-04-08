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
#include "e_flip.h"
#include "u_draw.h"
#include "u_geom.h"
#include "u_search.h"
#include "u_create.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"

#include "d_text.h"
#include "u_bound.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "w_cursor.h"

/* EXPORTS  */

int		setcenter;
int		setcenter_x;
int		setcenter_y;
int		rotn_dirn;
float		act_rotnangle;

/* LOCAL */

static int	copy;

static void	init_rotate(F_line *p, int type, int x, int y, int px, int py);
static void	set_unset_center(int x, int y);
static void	init_copynrotate(F_line *p, int type, int x, int y, int px, int py);
static void	rotate_selected(void);
static void	rotate_search(F_line *p, int type, int x, int y, int px, int py);
static void	init_rotateline(F_line *l, int px, int py);
static void	init_rotatetext(F_text *t, int px, int py);


void init_rotatearc (F_arc *a, int px, int py);
void init_rotateellipse (F_ellipse *e, int px, int py);
void init_rotatespline (F_spline *s, int px, int py);
void init_rotatecompound (F_compound *c, int px, int py);
void rotate_line (F_line *l, int x, int y);
void rotate_text (F_text *t, int x, int y);
void rotate_ellipse (F_ellipse *e, int x, int y);
void rotate_arc (F_arc *a, int x, int y);
void rotate_spline (F_spline *s, int x, int y);
int valid_rot_angle (F_compound *c);
void rotate_compound (F_compound *c, int x, int y);
void rotate_point (F_point *p, int x, int y);
void rotate_xy (int *orig_x, int *orig_y, int x, int y);

void
rotate_cw_selected(void)
{
    rotn_dirn = 1;
    /* erase any existing center */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    /* and any anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
    setcenter = 0;
    setanchor = 0;
    rotate_selected();
}

void
rotate_ccw_selected(void)
{
    rotn_dirn = -1;
    /* erase any existing center */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    /* and any anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
    setcenter = 0;
    setanchor = 0;
    rotate_selected();
}

static void
rotate_selected(void)
{
    set_mousefun("rotate object", "copy & rotate", "set center", 
			LOC_OBJ, LOC_OBJ, "set center");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_rotate);
    init_searchproc_middle(init_copynrotate);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = set_unset_center;
    set_cursor(pick15_cursor);
    reset_action_on();
}

static void
set_unset_center(int x, int y)
{
    if (setcenter) {
      set_mousefun("rotate object", "copy & rotate", "set center", 
			LOC_OBJ, LOC_OBJ, "set center");
      draw_mousefun_canvas();
      setcenter = 0;
      center_marker(setcenter_x,setcenter_y);
      /* second call to center_mark on same position deletes */
    }
    else {
      set_mousefun("rotate object", "copy & rotate", "unset center", 
			LOC_OBJ, LOC_OBJ, "unset center");
      draw_mousefun_canvas();
      setcenter = 1;
      setcenter_x = x;
      setcenter_y = y;
      center_marker(setcenter_x,setcenter_y);
    }
      
}

static void
init_rotate(F_line *p, int type, int x, int y, int px, int py)
{
    copy = 0;
    act_rotnangle = cur_rotnangle;
    if (setcenter) 
        rotate_search(p, type, x, y,setcenter_x,setcenter_y );
      /* remember rotation center, e.g for multiple rotation*/
    else
        rotate_search(p, type, x, y, px, py);
}

static void
init_copynrotate(F_line *p, int type, int x, int y, int px, int py)
{
    int		    i;

    copy = 1;
    act_rotnangle = cur_rotnangle;
    for ( i = 1; i <= cur_numcopies; act_rotnangle += cur_rotnangle, i++) {
	if (setcenter) 
	    rotate_search(p, type, x, y,setcenter_x,setcenter_y );
	/* remember rotation center */
	else
            rotate_search(p, type, x, y, px, py);
    }
}

static void
rotate_search(F_line *p, int type, int x, int y, int px, int py)
{
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) p;
	init_rotateline(cur_l, px, py);
	break;
    case O_ARC:
	cur_a = (F_arc *) p;
	init_rotatearc(cur_a, px, py);
	break;
    case O_ELLIPSE:
	cur_e = (F_ellipse *) p;
	init_rotateellipse(cur_e, px, py);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	init_rotatespline(cur_s, px, py);
	break;
    case O_TEXT:
	cur_t = (F_text *) p;
	init_rotatetext(cur_t, px, py);
	break;
    case O_COMPOUND:
	cur_c = (F_compound *) p;
	init_rotatecompound(cur_c, px, py);
	break;
    default:
	return;
    }
}

static void
init_rotateline(F_line *l, int px, int py)
{
    F_line	   *line;

    set_temp_cursor(wait_cursor);
    line = copy_line(l);
    rotate_line(line, px, py);
    if (copy) {
	add_line(line);
    } else {
	toggle_linemarker(l);
	draw_line(l, ERASE);
	change_line(l, line);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_line(l);
    /* and this line and any other objects on top */
    redisplay_line(line);
    reset_cursor();
}

static void
init_rotatetext(F_text *t, int px, int py)
{
    F_text	   *text;

    set_temp_cursor(wait_cursor);
    text = copy_text(t);
    rotate_text(text, px, py);
    if (copy) {
	add_text(text);
    } else {
	toggle_textmarker(t);
	draw_text(t, ERASE);
	change_text(t, text);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_text(t);
    /* and this text and any other objects on top */
    redisplay_text(text);
    reset_cursor();
}

void init_rotateellipse(F_ellipse *e, int px, int py)
{
    F_ellipse	   *ellipse;

    set_temp_cursor(wait_cursor);
    ellipse = copy_ellipse(e);
    rotate_ellipse(ellipse, px, py);
    if (copy) {
	add_ellipse(ellipse);
    } else {
	toggle_ellipsemarker(e);
	draw_ellipse(e, ERASE);
	change_ellipse(e, ellipse);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_ellipse(e);
    /* and this ellipse and any other objects on top */
    redisplay_ellipse(ellipse);
    reset_cursor();
}

void init_rotatearc(F_arc *a, int px, int py)
{
    F_arc	   *arc;

    set_temp_cursor(wait_cursor);
    arc = copy_arc(a);
    rotate_arc(arc, px, py);
    if (copy) {
	add_arc(arc);
    } else {
	toggle_arcmarker(a);
	draw_arc(a, ERASE);
	change_arc(a, arc);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_arc(a);
    /* and this arc and any other objects on top */
    redisplay_arc(arc);
    reset_cursor();
}

void init_rotatespline(F_spline *s, int px, int py)
{
    F_spline	   *spline;

    set_temp_cursor(wait_cursor);
    spline = copy_spline(s);
    rotate_spline(spline, px, py);
    if (copy) {
	add_spline(spline);
    } else {
	toggle_splinemarker(s);
	draw_spline(s, ERASE);
	change_spline(s, spline);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_spline(s);
    /* and this spline and any other objects on top */
    redisplay_spline(spline);
    reset_cursor();
}

void init_rotatecompound(F_compound *c, int px, int py)
{
    F_compound	   *compound;

    if (!valid_rot_angle(c)) {
	put_msg("Invalid rotation angle for this compound object");
	return;
    }
    set_temp_cursor(wait_cursor);
    compound = copy_compound(c);
    rotate_compound(compound, px, py);
    if (copy) {
	add_compound(compound);
    } else {
	toggle_compoundmarker(c);
	draw_compoundelements(c, ERASE);
	change_compound(c, compound);
    }
    /* redisplay objects under this object before it was rotated */
    redisplay_compound(c);
    /* and this compound and any other objects on top */
    redisplay_compound(compound);
    reset_cursor();
}

void rotate_line(F_line *l, int x, int y)
{
    F_point	   *p;
    int		    dx;

    /* for speed we treat 90 degrees as a special case */
    if (act_rotnangle == 90.0) {
	for (p = l->points; p != NULL; p = p->next) {
	    dx = p->x - x;
	    p->x = x + rotn_dirn * (y - p->y);
	    p->y = y + rotn_dirn * dx;
	}
    } else {
	for (p = l->points; p != NULL; p = p->next)
	    rotate_point(p, x, y);
    }

}

void rotate_figure(F_compound *f, int x, int y)
{
  float old_rotn_dirn, old_act_rotnangle;

  old_rotn_dirn = rotn_dirn;
  old_act_rotnangle = act_rotnangle;
  rotn_dirn = -1;
  act_rotnangle = 90.0;
  rotate_compound(f,x,y);
  rotn_dirn = old_rotn_dirn;
  act_rotnangle = old_act_rotnangle;
}

void rotate_spline(F_spline *s, int x, int y)
{
    F_point	   *p;
    int		    dx;

    /* for speed we treat 90 degrees as a special case */
    if (act_rotnangle == 90.0) {
	for (p = s->points; p != NULL; p = p->next) {
	    dx = p->x - x;
	    p->x = x + rotn_dirn * (y - p->y);
	    p->y = y + rotn_dirn * dx;
	}
    } else {
	for (p = s->points; p != NULL; p = p->next)
	    rotate_point(p, x, y);
    }
}

void rotate_text(F_text *t, int x, int y)
{
    int		    dx;

    if (act_rotnangle == 90.0) { /* treat 90 degs as special case for speed */
	dx = t->base_x - x;
	t->base_x = x + rotn_dirn * (y - t->base_y);
	t->base_y = y + rotn_dirn * dx;
    } else {
	rotate_xy(&t->base_x, &t->base_y, x, y);
    }
    t->angle -= (float) (rotn_dirn * act_rotnangle * M_PI / 180.0);
    if (t->angle < 0.0)
	t->angle += M_2PI;
    else if (t->angle >= M_2PI - 0.001)
	t->angle -= M_2PI;
    reload_text_fstruct(t);
}

void rotate_ellipse(F_ellipse *e, int x, int y)
{
    int		    dxc,dxs,dxe;

    if (act_rotnangle == 90.0) { /* treat 90 degs as special case for speed */
	dxc = e->center.x - x;
	dxs = e->start.x - x;
	dxe = e->end.x - x;
	e->center.x = x + rotn_dirn * (y - e->center.y);
	e->center.y = y + rotn_dirn * dxc;
	e->start.x = x + rotn_dirn * (y - e->start.y);
	e->start.y = y + rotn_dirn * dxs;
	e->end.x = x + rotn_dirn * (y - e->end.y);
	e->end.y = y + rotn_dirn * dxe;
    } else {
	rotate_point((F_point *)&e->center, x, y);
	rotate_point((F_point *)&e->start, x, y);
	rotate_point((F_point *)&e->end, x, y);
    }
    e->angle -= (float) (rotn_dirn * act_rotnangle * M_PI / 180.0);
    if (e->angle < 0.0)
	e->angle += M_2PI;
    else if (e->angle >= M_2PI - 0.001)
	e->angle -= M_2PI;
}

void rotate_arc(F_arc *a, int x, int y)
{
    int		    dx;
    F_pos	    p[3];
    float	    xx, yy;

    /* for speed we treat 90 degrees as a special case */
    if (act_rotnangle == 90.0) {
	dx = a->center.x - x;
	a->center.x = x + rotn_dirn * (y - a->center.y);
	a->center.y = y + rotn_dirn * dx;
	dx = a->point[0].x - x;
	a->point[0].x = x + rotn_dirn * (y - a->point[0].y);
	a->point[0].y = y + rotn_dirn * dx;
	dx = a->point[1].x - x;
	a->point[1].x = x + rotn_dirn * (y - a->point[1].y);
	a->point[1].y = y + rotn_dirn * dx;
	dx = a->point[2].x - x;
	a->point[2].x = x + rotn_dirn * (y - a->point[2].y);
	a->point[2].y = y + rotn_dirn * dx;
    } else {
	p[0] = a->point[0];
	p[1] = a->point[1];
	p[2] = a->point[2];
	rotate_point((F_point *)&p[0], x, y);
	rotate_point((F_point *)&p[1], x, y);
	rotate_point((F_point *)&p[2], x, y);
	if (compute_arccenter(p[0], p[1], p[2], &xx, &yy)) {
	    a->point[0].x = p[0].x;
	    a->point[0].y = p[0].y;
	    a->point[1].x = p[1].x;
	    a->point[1].y = p[1].y;
	    a->point[2].x = p[2].x;
	    a->point[2].y = p[2].y;
	    a->center.x = xx;
	    a->center.y = yy;
	    a->direction = compute_direction(p[0], p[1], p[2]);
	}
    }
}

/* checks to see if the objects within c can be rotated by act_rotnangle */

int valid_rot_angle(F_compound *c)
{
    F_line         *l;
    F_compound     *c1;

    if (fabs(act_rotnangle) == 90.0 || fabs(act_rotnangle == 180.0))
	return 1; /* always valid */
    for (l = c->lines; l != NULL; l = l->next)
	if (l->type == T_ARCBOX || l->type == T_BOX)
	    return 0;
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next)
	if (!valid_rot_angle(c1))
	    return 0;
    return 1;
}

void rotate_compound(F_compound *c, int x, int y)
{
    F_line	   *l;
    F_arc	   *a;
    F_ellipse	   *e;
    F_spline	   *s;
    F_text	   *t;
    F_compound	   *c1;

    for (l = c->lines; l != NULL; l = l->next)
	rotate_line(l, x, y);
    for (a = c->arcs; a != NULL; a = a->next)
	rotate_arc(a, x, y);
    for (e = c->ellipses; e != NULL; e = e->next)
	rotate_ellipse(e, x, y);
    for (s = c->splines; s != NULL; s = s->next)
	rotate_spline(s, x, y);
    for (t = c->texts; t != NULL; t = t->next)
	rotate_text(t, x, y);
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next)
	rotate_compound(c1, x, y);

    /*
     * Make the bounding box exactly match the dimensions of the compound.
     */
    compound_bound(c, &c->nwcorner.x, &c->nwcorner.y,
		   &c->secorner.x, &c->secorner.y);
}

void rotate_point(F_point *p, int x, int y)
{
    /* rotate point p about coordinate (x, y) */
    double	    dx, dy;
    double	    cosa, sina, mag, theta;

    dx = p->x - x;
    dy = y - p->y;
    if (dx == 0 && dy == 0)
	return;

    theta = compute_angle(dx, dy);
    theta -= (double) (rotn_dirn * act_rotnangle * M_PI / 180.0);
    if (theta < 0.0)
	theta += M_2PI;
    else if (theta >= M_2PI - 0.001)
	theta -= M_2PI;
    mag = sqrt(dx * dx + dy * dy);
    cosa = mag * cos(theta);
    sina = mag * sin(theta);
    p->x = round(x + cosa);
    p->y = round(y - sina);
}

void rotate_xy(int *orig_x, int *orig_y, int x, int y)
{
    /* rotate coord (orig_x, orig_y) about coordinate (x, y) */
    double	    dx, dy;
    double	    cosa, sina, mag, theta;

    dx = *orig_x - x;
    dy = y - *orig_y;
    if (dx == 0 && dy == 0)
	return;

    theta = compute_angle(dx, dy);
    theta -= (double) (rotn_dirn * act_rotnangle * M_PI / 180.0);
    if (theta < 0.0)
	theta += M_2PI;
    else if (theta >= M_2PI - 0.001)
	theta -= M_2PI;
    mag = sqrt(dx * dx + dy * dy);
    cosa = mag * cos(theta);
    sina = mag * sin(theta);
    *orig_x = round(x + cosa);
    *orig_y = round(y - sina);
}
