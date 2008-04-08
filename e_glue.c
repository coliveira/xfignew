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
#include "d_line.h"
#include "f_read.h"
#include "u_bound.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_layers.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"

#include "d_box.h"
#include "u_markers.h"
#include "w_cursor.h"

static void	create_compoundobject(int x, int y), cancel_tag_region(void),
		init_tag_region(int x, int y), tag_region(int x, int y), tag_object(F_line *p, int type, int x, int y, int px, int py);
static void	get_arc(F_arc **list), sel_arc(int xmin, int ymin, int xmax, int ymax);
static void	get_compound(F_compound **list), sel_compound(int xmin, int ymin, int xmax, int ymax);
static void	get_ellipse(F_ellipse **list), sel_ellipse(int xmin, int ymin, int xmax, int ymax);
static void	get_line(F_line **list), sel_line(int xmin, int ymin, int xmax, int ymax);
static void	get_spline(F_spline **list), sel_spline(int xmin, int ymin, int xmax, int ymax);
static void	get_text(F_text **list), sel_text(int xmin, int ymin, int xmax, int ymax);


void tag_obj_in_region (int xmin, int ymin, int xmax, int ymax);
int compose_compound (F_compound *c);

void
compound_selected(void)
{
    set_mousefun("tag object", "tag region", "compound tagged", 
			LOC_OBJ, "", "");
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(tag_object);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = init_tag_region;
    canvas_rightbut_proc = create_compoundobject;
    set_cursor(pick9_cursor);
    reset_action_on();
}

static void
tag_object(F_line *p, int type, int x, int y, int px, int py)
{
    switch (type) {
    case O_COMPOUND:
        cur_c = (F_compound *) p;
        toggle_compoundhighlight(cur_c);
	cur_c->tagged = 1 - cur_c->tagged;
        break;
    case O_POLYLINE:
        cur_l = (F_line *) p;
        toggle_linehighlight(cur_l);
	cur_l->tagged = 1 - cur_l->tagged;
        break;
    case O_TEXT:
        cur_t = (F_text *) p;
        toggle_texthighlight(cur_t);
	cur_t->tagged = 1 - cur_t->tagged;
        break;
    case O_ELLIPSE:
        cur_e = (F_ellipse *) p;
        toggle_ellipsehighlight(cur_e);
	cur_e->tagged = 1 - cur_e->tagged;
        break;
    case O_ARC:
        cur_a = (F_arc *) p;
        toggle_archighlight(cur_a);
	cur_a->tagged = 1 - cur_a->tagged;
        break;
    case O_SPLINE:
        cur_s = (F_spline *) p;
        toggle_splinehighlight(cur_s);
	cur_s->tagged = 1 - cur_s->tagged;
        break;
    default:
        return;
    }
}

static void
init_tag_region(int x, int y)
{
    init_box_drawing(x, y);
    set_mousefun("", "final corner", "cancel", "", "", "");
    draw_mousefun_canvas();
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = tag_region;
    canvas_rightbut_proc = cancel_tag_region;
}

static void
cancel_tag_region(void)
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_box_lengths();
    compound_selected();
    draw_mousefun_canvas();
}

static void
tag_region(int x, int y)
{
    int		    xmin, ymin, xmax, ymax;

    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_box_lengths();
    xmin = min2(fix_x, x);
    ymin = min2(fix_y, y);
    xmax = max2(fix_x, x);
    ymax = max2(fix_y, y);
    tag_obj_in_region(xmin, ymin, xmax, ymax);
    compound_selected();
    draw_mousefun_canvas();
}

static void
create_compoundobject(int x, int y)
{
    F_compound	   *c;

    if ((c = create_compound()) == NULL)
	return;

    if (compose_compound(c) == 0) {
	free((char *) c);
	compound_selected();
	draw_mousefun_canvas();
	put_msg("Empty compound, ignored");
	return;
    }
    /*
     * Make the bounding box exactly match the dimensions of the compound.
     */
    compound_bound(c, &c->nwcorner.x, &c->nwcorner.y,
		   &c->secorner.x, &c->secorner.y);

    /* if zero width or height in the compound, adjust to next positioning 
       grid point or a few pixels if positioning grid is "ANY" */
    if (c->nwcorner.x == c->secorner.x) {
	if (cur_pointposn != P_ANY) {
	    c->secorner.x += posn_rnd[cur_gridunit][cur_pointposn];
	    c->secorner.x = ceil_coords(c->secorner.x);
	}
    }
    if (c->nwcorner.y == c->secorner.y) {
	if (cur_pointposn != P_ANY) {
	    c->secorner.y += posn_rnd[cur_gridunit][cur_pointposn];
	    c->secorner.y = ceil_coords(c->secorner.y);
	}
    }
    c->next = NULL;
    clean_up();
    set_action(F_GLUE);
    toggle_markers_in_compound(c);
    list_add_compound(&objects.compounds, c);
    mask_toggle_compoundmarker(c);
    set_latestcompound(c);
    set_modifiedflag();
    compound_selected();
    draw_mousefun_canvas();
}

void tag_obj_in_region(int xmin, int ymin, int xmax, int ymax)
{
    sel_ellipse(xmin, ymin, xmax, ymax);
    sel_line(xmin, ymin, xmax, ymax);
    sel_spline(xmin, ymin, xmax, ymax);
    sel_text(xmin, ymin, xmax, ymax);
    sel_arc(xmin, ymin, xmax, ymax);
    sel_compound(xmin, ymin, xmax, ymax);
}


int compose_compound(F_compound *c)
{
    c->ellipses = NULL;
    c->lines = NULL;
    c->texts = NULL;
    c->splines = NULL;
    c->arcs = NULL;
    c->comments = NULL;
    c->compounds = NULL;
    /* defer updating of layer buttons until we've composed the entire compound */
    defer_update_layers = True;
    get_ellipse(&c->ellipses);
    get_line(&c->lines);
    get_spline(&c->splines);
    get_text(&c->texts);
    get_arc(&c->arcs);
    get_compound(&c->compounds);
    /* now update the layer buttons */
    defer_update_layers = False;
    update_layers();
    if (c->ellipses != NULL)
	return (1);
    if (c->splines != NULL)
	return (1);
    if (c->lines != NULL)
	return (1);
    if (c->texts != NULL)
	return (1);
    if (c->arcs != NULL)
	return (1);
    if (c->compounds != NULL)
	return (1);
    return (0);
}

static void
sel_ellipse(int xmin, int ymin, int xmax, int ymax)
{
    F_ellipse	   *e;

    for (e = objects.ellipses; e != NULL; e = e->next) {
	if (!active_layer(e->depth))
	    continue;
	if (xmin > e->center.x - e->radiuses.x)
	    continue;
	if (xmax < e->center.x + e->radiuses.x)
	    continue;
	if (ymin > e->center.y - e->radiuses.y)
	    continue;
	if (ymax < e->center.y + e->radiuses.y)
	    continue;
	e->tagged = 1 - e->tagged;
	toggle_ellipsehighlight(e);
    }
}

static void
get_ellipse(F_ellipse **list)
{
    F_ellipse	   *e, *ee, *ellipse;

    for (e = objects.ellipses; e != NULL;) {
	if (!e->tagged) {
	    ee = e;
	    e = e->next;
	    continue;
	}
	remove_depth(O_ELLIPSE, e->depth);
	if (*list == NULL)
	    *list = e;
	else
	    ellipse->next = e;
	ellipse = e;
	if (e == objects.ellipses)
	    e = objects.ellipses = objects.ellipses->next;
	else {
	    e = ee->next = e->next;
	}
	ellipse->next = NULL;
    }
}

static void
sel_arc(int xmin, int ymin, int xmax, int ymax)
{
    F_arc	   *a;
    int		    urx, ury, llx, lly;

    for (a = objects.arcs; a != NULL; a = a->next) {
	if (!active_layer(a->depth))
	    continue;
	arc_bound(a, &llx, &lly, &urx, &ury);
	if (xmin > llx)
	    continue;
	if (xmax < urx)
	    continue;
	if (ymin > lly)
	    continue;
	if (ymax < ury)
	    continue;
	a->tagged = 1 - a->tagged;
	toggle_archighlight(a);
    }
}

static void
get_arc(F_arc **list)
{
    F_arc	   *a, *arc, *aa;

    for (a = objects.arcs; a != NULL;) {
	if (!a->tagged) {
	    aa = a;
	    a = a->next;
	    continue;
	}
	remove_depth(O_ARC, a->depth);
	if (*list == NULL)
	    *list = a;
	else
	    arc->next = a;
	arc = a;
	if (a == objects.arcs)
	    a = objects.arcs = objects.arcs->next;
	else
	    a = aa->next = a->next;
	arc->next = NULL;
    }
}

static void
sel_line(int xmin, int ymin, int xmax, int ymax)
{
    F_line	   *l;
    F_point	   *p;
    int		    inbound;

    for (l = objects.lines; l != NULL; l = l->next) {
	if (!active_layer(l->depth))
	    continue;
	for (inbound = 1, p = l->points; p != NULL && inbound;
	     p = p->next) {
	    inbound = 0;
	    if (xmin > p->x)
		continue;
	    if (xmax < p->x)
		continue;
	    if (ymin > p->y)
		continue;
	    if (ymax < p->y)
		continue;
	    inbound = 1;
	}
	if (!inbound)
	    continue;
	l->tagged = 1 - l->tagged;
	toggle_linehighlight(l);
    }
}

static void
get_line(F_line **list)
{
    F_line	   *line, *l, *ll;

    for (l = objects.lines; l != NULL;) {
	if (!l->tagged) {
	    ll = l;
	    l = l->next;
	    continue;
	}
	remove_depth(O_POLYLINE, l->depth);
	if (*list == NULL)
	    *list = l;
	else
	    line->next = l;
	line = l;
	if (l == objects.lines)
	    l = objects.lines = objects.lines->next;
	else
	    l = ll->next = l->next;
	line->next = NULL;
    }
}

static void
sel_spline(int xmin, int ymin, int xmax, int ymax)
{
    F_spline	   *s;
    int		    urx, ury, llx, lly;

    for (s = objects.splines; s != NULL; s = s->next) {
	if (!active_layer(s->depth))
	    continue;
	spline_bound(s, &llx, &lly, &urx, &ury);
	if (xmin > llx)
	    continue;
	if (xmax < urx)
	    continue;
	if (ymin > lly)
	    continue;
	if (ymax < ury)
	    continue;
	s->tagged = 1 - s->tagged;
	toggle_splinehighlight(s);
    }
}

static void
get_spline(F_spline **list)
{
    F_spline	   *spline, *s, *ss;

    for (s = objects.splines; s != NULL;) {
	if (!s->tagged) {
	    ss = s;
	    s = s->next;
	    continue;
	}
	remove_depth(O_SPLINE, s->depth);
	if (*list == NULL)
	    *list = s;
	else
	    spline->next = s;
	spline = s;
	if (s == objects.splines)
	    s = objects.splines = objects.splines->next;
	else
	    s = ss->next = s->next;
	spline->next = NULL;
    }
}

static void
sel_text(int xmin, int ymin, int xmax, int ymax)
{
    F_text	   *t;
    int		    txmin, txmax, tymin, tymax;
    int		    dum;

    for (t = objects.texts; t != NULL; t = t->next) {
	if (!active_layer(t->depth))
	    continue;
	text_bound(t, &txmin, &tymin, &txmax, &tymax,
			&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	if (xmin > txmin || xmax < txmax ||
	    ymin > tymin || ymax < tymax)
		continue;
	t->tagged = 1 - t->tagged;
	toggle_texthighlight(t);
    }
}

static void
get_text(F_text **list)
{
    F_text	   *text, *t, *tt;

    for (t = objects.texts; t != NULL;) {
	if (!t->tagged) {
	    tt = t;
	    t = t->next;
	    continue;
	}
	remove_depth(O_TEXT, t->depth);
	if (*list == NULL)
	    *list = t;
	else
	    text->next = t;
	text = t;
	if (t == objects.texts)
	    t = objects.texts = objects.texts->next;
	else
	    t = tt->next = t->next;
	text->next = NULL;
    }
}

static void
sel_compound(int xmin, int ymin, int xmax, int ymax)
{
    F_compound	   *c;

    for (c = objects.compounds; c != NULL; c = c->next) {
	if (!any_active_in_compound(c))
	    continue;
	if (xmin > c->nwcorner.x)
	    continue;
	if (xmax < c->secorner.x)
	    continue;
	if (ymin > c->nwcorner.y)
	    continue;
	if (ymax < c->secorner.y)
	    continue;
	c->tagged = 1 - c->tagged;
	toggle_compoundhighlight(c);
    }
}

static void
get_compound(F_compound **list)
{
    F_compound	   *compd, *c, *cc;

    for (c = objects.compounds; c != NULL;) {
	if (!c->tagged) {
	    cc = c;
	    c = c->next;
	    continue;
	}
	remove_compound_depth(c);
	if (*list == NULL)
	    *list = c;
	else
	    compd->next = c;
	compd = c;
	if (c == objects.compounds)
	    c = objects.compounds = objects.compounds->next;
	else
	    c = cc->next = c->next;
	compd->next = NULL;
    }
}
