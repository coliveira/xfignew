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
#include "object.h"
#include "mode.h"
#include "paintop.h"
#include "u_markers.h"
#include "w_drawprim.h"
#include "w_layers.h"
#include "w_zoom.h"

#define set_marker(win,x,y,w,h) \
	XDrawRectangle(tool_d,(win),gccache[INV_PAINT], \
	     ZOOMX(x)-((w-1)/2),ZOOMY(y)-((w-1)/2),(w),(h))

#define CHANGED_MASK(msk) \
    ((oldmask & msk) != (newmask & msk))



void center_marker(int x, int y)
{

    pw_vector(canvas_win, x, y - (int)(CENTER_MARK/zoomscale),
	      x, y + (int)(CENTER_MARK/zoomscale), INV_PAINT, 1,
	      RUBBER_LINE, 0.0, DEFAULT);
    pw_vector(canvas_win, x - (int)(CENTER_MARK/zoomscale),
	      y, x + (int)(CENTER_MARK/zoomscale), y, INV_PAINT, 1,
	      RUBBER_LINE, 0.0, DEFAULT);

}

void toggle_csrhighlight(int x, int y)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win, x - 2, y - 2, MARK_SIZ, MARK_SIZ);
    set_marker(canvas_win, x - 1, y - 1, MARK_SIZ-2, MARK_SIZ-2);
}

int ellipse_in_mask(void)
{
    return (cur_objmask & M_ELLIPSE);
}

int arc_in_mask(void)
{
    return (cur_objmask & M_ARC);
}

int compound_in_mask(void)
{
    return (cur_objmask & M_COMPOUND);
}

int anytext_in_mask(void)
{
    return (cur_objmask & M_TEXT);
}

int validtext_in_mask(F_text *t)
{
    return ((hidden_text(t) && (cur_objmask & M_TEXT_HIDDEN)) ||
	    ((!hidden_text(t)) && (cur_objmask & M_TEXT_NORMAL)));
}

int anyline_in_mask(void)
{
    return (cur_objmask & M_POLYLINE);
}

int validline_in_mask(F_line *l)
{
    return ((((l->type == T_BOX) || (l->type == T_ARCBOX)) && (cur_objmask & M_POLYLINE_BOX)) ||
	    ((l->type == T_PICTURE) && (cur_objmask & M_POLYLINE_BOX)) ||
	    ((l->type == T_POLYLINE) && (cur_objmask & M_POLYLINE_LINE)) ||
	    ((l->type == T_POLYGON) && (cur_objmask & M_POLYLINE_POLYGON)));
}

int anyspline_in_mask(void)
{
    return (cur_objmask & M_SPLINE);
}

int validspline_in_mask(F_spline *s)
{
    return (((s->type == T_OPEN_INTERP) && (cur_objmask & M_SPLINE_O_INTERP)) ||
	    ((s->type == T_OPEN_APPROX) && (cur_objmask & M_SPLINE_O_APPROX))   ||
	    ((s->type == T_OPEN_XSPLINE) && (cur_objmask & M_SPLINE_O_XSPLINE)) ||
	    ((s->type == T_CLOSED_INTERP) && (cur_objmask & M_SPLINE_C_INTERP)) ||
	    ((s->type == T_CLOSED_APPROX) && (cur_objmask & M_SPLINE_C_APPROX)) ||
	    ((s->type == T_CLOSED_XSPLINE) && (cur_objmask & M_SPLINE_C_XSPLINE)));
}

void mask_toggle_ellipsemarker(F_ellipse *e)
{
    if (ellipse_in_mask())
	toggle_ellipsemarker(e);
}

void mask_toggle_arcmarker(F_arc *a)
{
    if (arc_in_mask())
	toggle_arcmarker(a);
}

void mask_toggle_compoundmarker(F_compound *c)
{
    if (compound_in_mask())
	toggle_compoundmarker(c);
}

void mask_toggle_textmarker(F_text *t)
{
    if (validtext_in_mask(t))
	toggle_textmarker(t);
}

void mask_toggle_linemarker(F_line *l)
{
    if (validline_in_mask(l))
	toggle_linemarker(l);
}

void mask_toggle_splinemarker(F_spline *s)
{
    if (validspline_in_mask(s))
	toggle_splinemarker(s);
}

void toggle_markers(void)
{
    toggle_markers_in_compound(&objects);
}

void toggle_markers_in_compound(F_compound *cmpnd)
{
    F_ellipse	   *e;
    F_arc	   *a;
    F_line	   *l;
    F_spline	   *s;
    F_text	   *t;
    F_compound	   *c;
    register int    mask;

    mask = cur_objmask;
    if (mask & M_ELLIPSE)
	for (e = cmpnd->ellipses; e != NULL; e = e->next)
	    if (active_layer(e->depth))
		toggle_ellipsemarker(e);
    if (mask & M_TEXT)
	for (t = cmpnd->texts; t != NULL; t = t->next) {
	    if (active_layer(t->depth) &&
	        (((hidden_text(t) && (mask & M_TEXT_HIDDEN)) ||
		((!hidden_text(t)) && (mask & M_TEXT_NORMAL)))))
		    toggle_textmarker(t);
	}
    if (mask & M_ARC)
	for (a = cmpnd->arcs; a != NULL; a = a->next)
	    if (active_layer(a->depth))
		toggle_arcmarker(a);
    if (mask & M_POLYLINE)
	for (l = cmpnd->lines; l != NULL; l = l->next) {
	    if (active_layer(l->depth) &&
	        ((((l->type == T_BOX) ||
		  (l->type == T_ARCBOX)) && (mask & M_POLYLINE_BOX)) ||
		((l->type == T_PICTURE) && (mask & M_POLYLINE_BOX)) ||
		((l->type == T_POLYLINE) && (mask & M_POLYLINE_LINE)) ||
		((l->type == T_POLYGON) && (mask & M_POLYLINE_POLYGON))))
		toggle_linemarker(l);
	}
    if (mask & M_SPLINE)
	for (s = cmpnd->splines; s != NULL; s = s->next) {
	    if (active_layer(s->depth) &&
	       (((s->type == T_OPEN_INTERP) && (mask & M_SPLINE_O_INTERP)) ||
		((s->type == T_OPEN_APPROX) && (mask & M_SPLINE_O_APPROX))   ||
		((s->type == T_OPEN_XSPLINE) && (mask & M_SPLINE_O_XSPLINE)) ||
	       ((s->type == T_CLOSED_INTERP) && (mask & M_SPLINE_C_INTERP)) ||
		((s->type == T_CLOSED_APPROX) && (mask & M_SPLINE_C_APPROX)) ||
		((s->type == T_CLOSED_XSPLINE) && (mask & M_SPLINE_C_XSPLINE))))
		toggle_splinemarker(s);
	}
    if (mask & M_COMPOUND)
	for (c = cmpnd->compounds; c != NULL; c = c->next)
	    if (any_active_in_compound(c))
		toggle_compoundmarker(c);
}

void update_markers(int mask)
{
    F_ellipse	   *e;
    F_arc	   *a;
    F_line	   *l;
    F_spline	   *s;
    F_text	   *t;
    F_compound	   *c;
    register int    oldmask, newmask;

    oldmask = cur_objmask;
    newmask = mask;
    if (CHANGED_MASK(M_ELLIPSE))
	for (e = objects.ellipses; e != NULL; e = e->next)
	    if (active_layer(e->depth))
		toggle_ellipsemarker(e);
    if (CHANGED_MASK(M_TEXT_NORMAL) || CHANGED_MASK(M_TEXT_HIDDEN))
	for (t = objects.texts; t != NULL; t = t->next) {
	    if (active_layer(t->depth) && 
		((hidden_text(t) && CHANGED_MASK(M_TEXT_HIDDEN))   ||
		((!hidden_text(t)) && CHANGED_MASK(M_TEXT_NORMAL))))
		    toggle_textmarker(t);
	}
    if (CHANGED_MASK(M_ARC))
	for (a = objects.arcs; a != NULL; a = a->next)
	    if (active_layer(a->depth))
		toggle_arcmarker(a);
    if (CHANGED_MASK(M_POLYLINE_LINE) ||
	CHANGED_MASK(M_POLYLINE_POLYGON) ||
	CHANGED_MASK(M_POLYLINE_BOX))
	for (l = objects.lines; l != NULL; l = l->next) {
	    if (active_layer(l->depth) &&
		((((l->type == T_BOX || l->type == T_ARCBOX || 
		   l->type == T_PICTURE)) && CHANGED_MASK(M_POLYLINE_BOX)) ||
		((l->type == T_POLYLINE) && CHANGED_MASK(M_POLYLINE_LINE)) ||
		((l->type == T_POLYGON) && CHANGED_MASK(M_POLYLINE_POLYGON))))
		    toggle_linemarker(l);
	}
    if (CHANGED_MASK(M_SPLINE_O_APPROX) || CHANGED_MASK(M_SPLINE_C_APPROX) ||
	CHANGED_MASK(M_SPLINE_O_INTERP) || CHANGED_MASK(M_SPLINE_C_INTERP) ||
	CHANGED_MASK(M_SPLINE_O_XSPLINE) || CHANGED_MASK(M_SPLINE_C_XSPLINE))
	for (s = objects.splines; s != NULL; s = s->next) {
	    if (active_layer(s->depth) &&
		(((s->type == T_OPEN_INTERP) && CHANGED_MASK(M_SPLINE_O_INTERP)) ||
		((s->type == T_OPEN_APPROX) && CHANGED_MASK(M_SPLINE_O_APPROX)) ||
		((s->type == T_OPEN_XSPLINE) && CHANGED_MASK(M_SPLINE_O_XSPLINE)) ||
		((s->type == T_CLOSED_INTERP) && CHANGED_MASK(M_SPLINE_C_INTERP)) ||
		((s->type == T_CLOSED_APPROX) && CHANGED_MASK(M_SPLINE_C_APPROX)) ||
		((s->type == T_CLOSED_XSPLINE) && CHANGED_MASK(M_SPLINE_C_XSPLINE))))
		    toggle_splinemarker(s);
	  }
    if (CHANGED_MASK(M_COMPOUND))
	for (c = objects.compounds; c != NULL; c = c->next)
	    if (any_active_in_compound(c))
		toggle_compoundmarker(c);
    cur_objmask = newmask;
}

void toggle_ellipsemarker(F_ellipse *e)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win, e->start.x - 2, e->start.y - 2, MARK_SIZ, MARK_SIZ);
    set_marker(canvas_win, e->end.x - 2, e->end.y - 2, MARK_SIZ, MARK_SIZ);
    if (e->tagged)
	toggle_ellipsehighlight(e);
}

void toggle_ellipsehighlight(F_ellipse *e)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win, e->start.x, e->start.y, 1, 1);
    set_marker(canvas_win, e->start.x - 1, e->start.y - 1, SM_MARK, SM_MARK);
    set_marker(canvas_win, e->end.x, e->end.y, 1, 1);
    set_marker(canvas_win, e->end.x - 1, e->end.y - 1, SM_MARK, SM_MARK);
}

void toggle_arcmarker(F_arc *a)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win,a->point[0].x-2,a->point[0].y-2,MARK_SIZ,MARK_SIZ);
    set_marker(canvas_win,a->point[1].x-2,a->point[1].y-2,MARK_SIZ,MARK_SIZ);
    set_marker(canvas_win,a->point[2].x-2,a->point[2].y-2,MARK_SIZ,MARK_SIZ);
    if (a->tagged)
	toggle_archighlight(a);
}

void toggle_archighlight(F_arc *a)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win, a->point[0].x, a->point[0].y, 1, 1);
    set_marker(canvas_win, a->point[0].x-1, a->point[0].y-1, SM_MARK, SM_MARK);
    set_marker(canvas_win, a->point[1].x, a->point[1].y, 1, 1);
    set_marker(canvas_win, a->point[1].x-1, a->point[1].y-1, SM_MARK, SM_MARK);
    set_marker(canvas_win, a->point[2].x, a->point[2].y, 1, 1);
    set_marker(canvas_win, a->point[2].x-1, a->point[2].y-1, SM_MARK, SM_MARK);
}

void toggle_textmarker(F_text *t)
{
    int		    dx, dy;

    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    /* adjust for text angle */
    dy = (int) ((double) t->ascent * cos(t->angle));
    dx = (int) ((double) t->ascent * sin(t->angle));
    set_marker(canvas_win,t->base_x-dx-2,t->base_y-dy-2,MARK_SIZ,MARK_SIZ);
    /* only draw second marker if not on top of first (e.g. string with only
       spaces has no height) */
    if (dx != 0 || dy != 0)
	set_marker(canvas_win,t->base_x-2,t->base_y-2,MARK_SIZ,MARK_SIZ);
    if (t->tagged)
	toggle_texthighlight(t);
}

void toggle_texthighlight(F_text *t)
{
    int		    dx, dy;

    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    /* adjust for text angle */
    dy = (int) ((double) t->ascent * cos(t->angle));
    dx = (int) ((double) t->ascent * sin(t->angle));
    set_marker(canvas_win, t->base_x-dx, t->base_y-dy, 1, 1);
    set_marker(canvas_win, t->base_x-dx-1, t->base_y-dy-1, SM_MARK, SM_MARK);
    set_marker(canvas_win, t->base_x, t->base_y, 1, 1);
    set_marker(canvas_win, t->base_x-1, t->base_y-1, SM_MARK, SM_MARK);
}

void toggle_all_compoundmarkers(void)
{
    F_compound	   *c;
    for (c=objects.compounds; c!=NULL ; c=c->next)
	toggle_compoundmarker(c);
}

void toggle_compoundmarker(F_compound *c)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win,c->nwcorner.x-2,c->nwcorner.y-2,MARK_SIZ,MARK_SIZ);
    set_marker(canvas_win,c->secorner.x-2,c->secorner.y-2,MARK_SIZ,MARK_SIZ);
    set_marker(canvas_win,c->nwcorner.x-2,c->secorner.y-2,MARK_SIZ,MARK_SIZ);
    set_marker(canvas_win,c->secorner.x-2,c->nwcorner.y-2,MARK_SIZ,MARK_SIZ);
    if (c->tagged)
	toggle_compoundhighlight(c);
}

void toggle_compoundhighlight(F_compound *c)
{
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    set_marker(canvas_win, c->nwcorner.x, c->nwcorner.y, 1, 1);
    set_marker(canvas_win, c->nwcorner.x-1, c->nwcorner.y-1, SM_MARK, SM_MARK);
    set_marker(canvas_win, c->secorner.x, c->secorner.y, 1, 1);
    set_marker(canvas_win, c->secorner.x-1, c->secorner.y-1, SM_MARK, SM_MARK);
    set_marker(canvas_win, c->nwcorner.x, c->secorner.y, 1, 1);
    set_marker(canvas_win, c->nwcorner.x-1, c->secorner.y-1, SM_MARK, SM_MARK);
    set_marker(canvas_win, c->secorner.x, c->nwcorner.y, 1, 1);
    set_marker(canvas_win, c->secorner.x-1, c->nwcorner.y-1, SM_MARK, SM_MARK);
}

void toggle_linemarker(F_line *l)
{
    F_point	   *p;
    int		    fx, fy, x, y;

    x = y = INT_MIN;
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    p = l->points;
    fx = p->x;
    fy = p->y;
    for (p = p->next; p != NULL; p = p->next) {
	x = p->x;
	y = p->y;
	set_marker(canvas_win, x - 2, y - 2, MARK_SIZ, MARK_SIZ);
    }
    if (x != fx || y != fy || l->points->next == NULL) {
	set_marker(canvas_win, fx - 2, fy - 2, MARK_SIZ, MARK_SIZ);
    }
    if (l->tagged)
	toggle_linehighlight(l);
}

void toggle_linehighlight(F_line *l)
{
    F_point	   *p;
    int		    fx, fy, x, y;

    x = y = INT_MIN;
    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    p = l->points;
    fx = p->x;
    fy = p->y;
    for (p = p->next; p != NULL; p = p->next) {
	x = p->x;
	y = p->y;
	set_marker(canvas_win, x, y, 1, 1);
	set_marker(canvas_win, x - 1, y - 1, SM_MARK, SM_MARK);
    }
    if (x != fx || y != fy) {
	set_marker(canvas_win, fx, fy, 1, 1);
	set_marker(canvas_win, fx - 1, fy - 1, SM_MARK, SM_MARK);
    }
}

void toggle_splinemarker(F_spline *s)
{
    F_point	   *p;
    int		    fx, fy, x, y;

    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    p = s->points;
    fx = p->x;
    fy = p->y;
    for (p = p->next; p != NULL; p = p->next) {
	x = p->x;
	y = p->y;
	set_marker(canvas_win, x - 2, y - 2, MARK_SIZ, MARK_SIZ);
    }
    if (x != fx || y != fy) {
	set_marker(canvas_win, fx - 2, fy - 2, MARK_SIZ, MARK_SIZ);
    }
    if (s->tagged)
	toggle_splinehighlight(s);
}

void toggle_splinehighlight(F_spline *s)
{
    F_point	   *p;
    int		    fx, fy, x, y;

    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT, INV_PAINT, DEFAULT);
    p = s->points;
    fx = p->x;
    fy = p->y;
    for (p = p->next; p != NULL; p = p->next) {
	x = p->x;
	y = p->y;
	set_marker(canvas_win, x, y, 1, 1);
	set_marker(canvas_win, x - 1, y - 1, SM_MARK, SM_MARK);
    }
    if (x != fx || y != fy) {
	set_marker(canvas_win, fx, fy, 1, 1);
	set_marker(canvas_win, fx - 1, fy - 1, SM_MARK, SM_MARK);
    }
}

void toggle_pointmarker(int x, int y)
{
  set_marker(canvas_win, x - MARK_SIZ/2, y - MARK_SIZ/2, MARK_SIZ, MARK_SIZ);
}
