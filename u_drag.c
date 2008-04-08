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
#include "paintop.h"
#include "e_copy.h"
#include "u_drag.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_create.h"
#include "u_undo.h"
#include "mode.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_zoom.h"

#include "u_bound.h"
#include "u_free.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "u_translate.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"

static void array_place_line(int x, int y),     place_line(int x, int y),     place_line_x(int x, int y),     cancel_line(void);
static void array_place_arc(int x, int y),      place_arc(int x, int y),      place_arc_x(int x, int y),      cancel_drag_arc(void);
static void array_place_spline(int x, int y),   place_spline(int x, int y),   place_spline_x(int x, int y),   cancel_spline(void);
static void array_place_ellipse(int x, int y),  place_ellipse(int x, int y),  place_ellipse_x(int x, int y),  cancel_ellipse(void);
static void array_place_text(int x, int y),     place_text(int x, int y),     place_text_x(int x, int y),     cancel_text(void);
static void array_place_compound(int x, int y), place_compound(int x, int y), place_compound_x(int x, int y), cancel_drag_compound(void);

/***************************** ellipse section ************************/



void
init_ellipsedragging(F_ellipse *e, int x, int y)
{
    new_e = e;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    cur_angle = e->angle;
    x1off = (e->center.x - e->radiuses.x) - cur_x;
    x2off = (e->center.x + e->radiuses.x) - cur_x;
    y1off = (e->center.y - e->radiuses.y) - cur_y;
    y2off = (e->center.y + e->radiuses.y) - cur_y;
    canvas_locmove_proc = moving_ellipse;
    canvas_ref_proc = elastic_moveellipse;
    canvas_leftbut_proc = place_ellipse;
    canvas_middlebut_proc = array_place_ellipse;
    canvas_rightbut_proc = cancel_ellipse;
    set_action_on();
    elastic_moveellipse();
}

static void
cancel_ellipse(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_moveellipse();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    if (return_proc == copy_selected) {
	free_ellipse(&new_e);
    } else {
	list_add_ellipse(&objects.ellipses, new_e);
	redisplay_ellipse(new_e);
    }
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

static void
array_place_ellipse(int x, int y)
{
    int		    i, j, delta_x, delta_y, start_x, start_y;
    int		    nx, ny;
    F_ellipse	   *save_ellipse;

    elastic_moveellipse();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();

    tail(&objects, &object_tails);
    save_ellipse = new_e;

    if ((!cur_numxcopies) && (!cur_numycopies)) {
	place_ellipse(x, y);
    }
    else {
	delta_x = cur_x - fix_x;
	delta_y = cur_y - fix_y;
	start_x = cur_x - delta_x;
	start_y = cur_y - delta_y;
	if ((cur_numxcopies < 2) && (cur_numycopies < 2)) {  /* special cases */
	    if (cur_numxcopies > 0) {
		place_ellipse_x(start_x+delta_x, start_y);
		new_e = copy_ellipse(cur_e);
	    }
	    if (cur_numycopies > 0) {
		place_ellipse_x(start_x, start_y+delta_y);
		new_e = copy_ellipse(cur_e);
	    }
	} else {
	    nx = cur_numxcopies;
	    if (nx == 0)
		nx++;
	    ny = cur_numycopies;
	    if (ny == 0)
		ny++;
	    for (i = 0, x = start_x;  i < nx; i++, x+=delta_x) {
		for (j = 0, y = start_y;  j < ny; j++, y+=delta_y) {
		    if (i || j ) {
			place_ellipse_x(x, y);
			new_e = copy_ellipse(cur_e);
		    }
		}
	    }
	}
    }
    /* put all new ellipses in the saved objects structure for undo */
    saved_objects.ellipses = save_ellipse;
    set_action_object(F_ADD, O_ALL_OBJECT);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

static void
place_ellipse(int x, int y)
{
    elastic_moveellipse();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    place_ellipse_x(x, y);
}

static void
place_ellipse_x(int x, int y)
{
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_ellipse(new_e, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_ellipse(new_e);
    } else {
	list_add_ellipse(&objects.ellipses, new_e);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_ELLIPSE);
	set_latestellipse(new_e);
	set_modifiedflag();
    }
    redisplay_ellipse(new_e);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*****************************	arc  section  *******************/

void
init_arcdragging(F_arc *a, int x, int y)
{
    new_a = a;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    canvas_locmove_proc = moving_arc;
    canvas_ref_proc = elastic_movenewarc;
    canvas_leftbut_proc = place_arc;
    canvas_middlebut_proc = array_place_arc;
    canvas_rightbut_proc = cancel_drag_arc;
    set_action_on();
    elastic_movearc(new_a);
}

static void
cancel_drag_arc(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_movearc(new_a);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    if (return_proc == copy_selected) {
	free_arc(&new_a);
    } else {
	list_add_arc(&objects.arcs, new_a);
	redisplay_arc(new_a);
    }
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

static void
array_place_arc(int x, int y)
{
    int		    i, j, delta_x, delta_y, start_x, start_y;
    int		    nx, ny;
    F_arc	   *save_arc;

    elastic_movearc(new_a);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();

    tail(&objects, &object_tails);
    save_arc = new_a;

    if ((!cur_numxcopies) && (!cur_numycopies)) {
	place_arc(x, y);
    } else {
	delta_x = cur_x - fix_x;
	delta_y = cur_y - fix_y;
	start_x = cur_x - delta_x;
	start_y = cur_y - delta_y;
	if ((cur_numxcopies < 2) && (cur_numycopies < 2)) {  /* special cases */
	    if (cur_numxcopies > 0) {
		place_arc_x(start_x+delta_x, start_y);
		new_a = copy_arc(cur_a);
	    }
	    if (cur_numycopies > 0) {
		place_arc_x(start_x, start_y+delta_y);
		new_a = copy_arc(cur_a);
	    }
	} else {
	    nx = cur_numxcopies;
	    if (nx == 0)
		nx++;
	    ny = cur_numycopies;
	    if (ny == 0)
		ny++;
	    for (i = 0, x = start_x;  i < nx; i++, x+=delta_x) {
		for (j = 0, y = start_y;  j < ny; j++, y+=delta_y) {
		    if (i || j ) {
			place_arc_x(x, y);
			new_a = copy_arc(cur_a);
		    }
		}
	    }
	}
    }
    /* put all new arcs in the saved objects structure for undo */
    saved_objects.arcs = save_arc;
    set_action_object(F_ADD, O_ALL_OBJECT);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

static void
place_arc(int x, int y)
{
    elastic_movearc(new_a);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    place_arc_x(x, y);
}

static void
place_arc_x(int x, int y)
{
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_arc(new_a, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_arc(new_a);
    } else {
	list_add_arc(&objects.arcs, new_a);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_ARC);
	set_latestarc(new_a);
	set_modifiedflag();
    }
    redisplay_arc(new_a);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*************************  line  section  **********************/

void
init_linedragging(F_line *l, int x, int y)
{
    int		    xmin, ymin, xmax, ymax;

    new_l = l;
    cur_x = fix_x = x;
    cur_y = fix_y = y;
    canvas_locmove_proc = moving_line;
    canvas_ref_proc = elastic_movenewline;
    canvas_leftbut_proc = place_line;
    canvas_middlebut_proc = array_place_line;
    canvas_rightbut_proc = cancel_line;
    set_action_on();
    if (l->type == T_BOX || l->type == T_ARCBOX || l->type == T_PICTURE) {
	line_bound(l, &xmin, &ymin, &xmax, &ymax);
	get_links(xmin, ymin, xmax, ymax);
    }
    elastic_moveline(new_l->points);
}

static void
cancel_line(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_moveline(new_l->points);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    free_linkinfo(&cur_links);
    if (return_proc == copy_selected) {
	free_line(&new_l);
    } else {
	list_add_line(&objects.lines, new_l);
	redisplay_line(new_l);
    }
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

static void
array_place_line(int x, int y)
{
    int		    i, j, delta_x, delta_y, start_x, start_y;
    int		    nx, ny;
    F_line	   *save_line;

    elastic_moveline(new_l->points);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    tail(&objects, &object_tails);
    save_line = new_l;
    if ((cur_numxcopies==0) && (cur_numycopies==0)) {
	place_line(x, y);
    } else {
	delta_x = cur_x - fix_x;
	delta_y = cur_y - fix_y;
	start_x = cur_x - delta_x;
	start_y = cur_y - delta_y;
	if ((cur_numxcopies < 2) && (cur_numycopies < 2)) {  /* special cases */
	    if (cur_numxcopies > 0) {
		place_line_x(start_x+delta_x, start_y);
		new_l = copy_line(cur_l);
	    }
	    if (cur_numycopies > 0) {
		place_line_x(start_x, start_y+delta_y);
		new_l = copy_line(cur_l);
	    }
	} else {
	    nx = cur_numxcopies;
	    if (nx == 0)
		nx++;
	    ny = cur_numycopies;
	    if (ny == 0)
		ny++;
	    for (i = 0, x = start_x;  i < nx; i++, x+=delta_x) {
		for (j = 0, y = start_y;  j < ny; j++, y+=delta_y) {
		    if (i || j ) {
			place_line_x(x, y);
			new_l = copy_line(cur_l);
		    }
		}
	    }
	}
    }
    /* put all new lines in the saved objects structure for undo */
    saved_objects.lines = save_line;
    set_action_object(F_ADD, O_ALL_OBJECT);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

static void
place_line(int x, int y)
{
    elastic_moveline(new_l->points);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    place_line_x(x, y);
}

static void
place_line_x(int x, int y)
{
    int		    dx, dy;
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    dx = x - fix_x;
    dy = y - fix_y;
    translate_line(new_l, dx, dy);
    clean_up();
    set_latestline(new_l);
    if (return_proc == copy_selected) {
	add_line(new_l);
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, True);
	free_linkinfo(&cur_links);
    } else {
	list_add_line(&objects.lines, new_l);
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, False);
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_lastlinkinfo(cur_linkmode, cur_links);
	cur_links = NULL;
	set_action_object(F_MOVE, O_POLYLINE);
    }
    set_modifiedflag();
    redisplay_line(new_l);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/************************  text section	 **************************/

static PR_SIZE	txsize;

void
init_textdragging(F_text *t, int x, int y)
{
    float	   cw,cw2;
    int		   x1, y1;

    new_t = t;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    x1 = new_t->base_x;
    y1 = new_t->base_y;
    /* adjust fix_x/y so that text will fall on grid if grid is on */
    round_coords(x1,y1);
    fix_x += new_t->base_x - x1;
    fix_y += new_t->base_y - y1;
    x1off = x1-x; /*new_t->base_x - x;*/
    y1off = y1-y; /*new_t->base_y - y;*/
    if (t->type == T_CENTER_JUSTIFIED || t->type == T_RIGHT_JUSTIFIED) {
	txsize = textsize(t->fontstruct, strlen(t->cstring), t->cstring);
	if (t->type == T_CENTER_JUSTIFIED) {
	    cw2 = txsize.length/2.0/display_zoomscale;
	    x1off = round(x1off - cos((double)t->angle)*cw2);
	    y1off = round(y1off + sin((double)t->angle)*cw2);
	} else { /* T_RIGHT_JUSTIFIED */
	    cw = 1.0*txsize.length/display_zoomscale;
	    x1off = round(x1off - cos((double)t->angle)*cw);
	    y1off = round(y1off + sin((double)t->angle)*cw);
	}
    }
    canvas_locmove_proc = moving_text;
    canvas_ref_proc = elastic_movetext;
    canvas_leftbut_proc = place_text;
    canvas_middlebut_proc = array_place_text;
    canvas_rightbut_proc = cancel_text;
    elastic_movetext();
    set_action_on();
}

static void
array_place_text(int x, int y)
{
    int		    i, j, delta_x, delta_y, start_x, start_y;
    int		    nx, ny;
    F_text	   *save_text;

    elastic_movetext();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();

    tail(&objects, &object_tails);
    save_text = new_t;

    if ((!cur_numxcopies) && (!cur_numycopies)) {
	place_text(x, y);
    } else {
	delta_x = cur_x - fix_x;
	delta_y = cur_y - fix_y;
	start_x = cur_x - delta_x;
	start_y = cur_y - delta_y;
	if ((cur_numxcopies < 2) && (cur_numycopies < 2)) {  /* special cases */
	    if (cur_numxcopies > 0) {
		place_text_x(start_x+delta_x, start_y);
		new_t = copy_text(cur_t);
	    }
	    if (cur_numycopies > 0) {
		place_text_x(start_x, start_y+delta_y);
		new_t = copy_text(cur_t);
	    }
	} else {
	    nx = cur_numxcopies;
	    if (nx == 0)
		nx++;
	    ny = cur_numycopies;
	    if (ny == 0)
		ny++;
	    for (i = 0, x = start_x;  i < nx; i++, x+=delta_x) {
		for (j = 0, y = start_y;  j < ny; j++, y+=delta_y) {
		    if (i || j ) {
			place_text_x(x, y);
			new_t = copy_text(cur_t);
		    }
		}
	    }
	}
    }
    /* put all new texts in the saved objects structure for undo */
    saved_objects.texts = save_text;
    set_action_object(F_ADD, O_ALL_OBJECT);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

static void
cancel_text(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_movetext();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    if (return_proc == copy_selected) {
	free_text(&new_t);
    } else {
	list_add_text(&objects.texts, new_t);
	redisplay_text(new_t);
    }
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

static void
place_text(int x, int y)
{
    elastic_movetext();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    place_text_x(x, y);
}

static void
place_text_x(int x, int y)
{
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_text(new_t, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_text(new_t);
    } else {
	list_add_text(&objects.texts, new_t);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_TEXT);
	set_latesttext(new_t);
	set_modifiedflag();
    }
    redisplay_text(new_t);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*************************  spline  section  **********************/

void
init_splinedragging(F_spline *s, int x, int y)
{
    new_s = s;
    cur_x = fix_x = x;
    cur_y = fix_y = y;
    canvas_locmove_proc = moving_spline;
    canvas_ref_proc = elastic_movenewspline;
    canvas_leftbut_proc = place_spline;
    canvas_middlebut_proc = array_place_spline;
    canvas_rightbut_proc = cancel_spline;
    set_action_on();
    elastic_moveline(new_s->points);
}

static void
cancel_spline(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_moveline(new_s->points);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    if (return_proc == copy_selected) {
	free_spline(&new_s);
    } else {
	list_add_spline(&objects.splines, new_s);
	redisplay_spline(new_s);
    }
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

static void
array_place_spline(int x, int y)
{
    int		    i, j, delta_x, delta_y, start_x, start_y;
    int		    nx, ny;
    F_spline	   *save_spline;

    elastic_moveline(new_s->points);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();

    tail(&objects, &object_tails);
    save_spline = new_s;

    if ((!cur_numxcopies) && (!cur_numycopies)) {
	place_spline(x, y);
    } else {
	delta_x = cur_x - fix_x;
	delta_y = cur_y - fix_y;
	start_x = cur_x - delta_x;
	start_y = cur_y - delta_y;
	if ((cur_numxcopies < 2) && (cur_numycopies < 2)) {  /* special cases */
	    if (cur_numxcopies > 0) {
		place_spline_x(start_x+delta_x, start_y);
		new_s = copy_spline(cur_s);
	    }
	    if (cur_numycopies > 0) {
		place_spline_x(start_x, start_y+delta_y);
		new_s = copy_spline(cur_s);
	    }
	} else {
	    nx = cur_numxcopies;
	    if (nx == 0)
		nx++;
	    ny = cur_numycopies;
	    if (ny == 0)
		ny++;
	    for (i = 0, x = start_x;  i < nx; i++, x+=delta_x) {
		for (j = 0, y = start_y;  j < ny; j++, y+=delta_y) {
		    if (i || j ) {
			place_spline_x(x, y);
			new_s = copy_spline(cur_s);
		    }
		}
	    }
	}
    }
    /* put all new splines in the saved objects structure for undo */
    saved_objects.splines = save_spline;
    set_action_object(F_ADD, O_ALL_OBJECT);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

static void
place_spline(int x, int y)
{
    elastic_moveline(new_s->points);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    place_spline_x(x, y);
}

static void
place_spline_x(int x, int y)
{
    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    translate_spline(new_s, x - fix_x, y - fix_y);
    if (return_proc == copy_selected) {
	add_spline(new_s);
    } else {
	list_add_spline(&objects.splines, new_s);
	clean_up();
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_action_object(F_MOVE, O_SPLINE);
	set_latestspline(new_s);
	set_modifiedflag();
    }
    redisplay_spline(new_s);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

/*****************************	Compound section  *******************/

void
init_compounddragging(F_compound *c, int x, int y)
{
    new_c = c;
    fix_x = cur_x = x;
    fix_y = cur_y = y;
    x1off = c->nwcorner.x - x;
    x2off = c->secorner.x - x;
    y1off = c->nwcorner.y - y;
    y2off = c->secorner.y - y;
    canvas_locmove_proc = moving_box;
    canvas_ref_proc = elastic_movebox;
    canvas_leftbut_proc = place_compound;
    canvas_middlebut_proc = array_place_compound;
    canvas_rightbut_proc = cancel_drag_compound;
    set_action_on();
    get_interior_links(c->nwcorner.x, c->nwcorner.y, c->secorner.x, c->secorner.y);
    elastic_movebox();
}

static void
cancel_drag_compound(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_movebox();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    free_linkinfo(&cur_links);
    if (return_proc == copy_selected) {
	free_compound(&new_c);
    } else {
	list_add_compound(&objects.compounds, new_c);
	redisplay_compound(new_c);
    }
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}

static void
array_place_compound(int x, int y)
{
    int		    i, j, delta_x, delta_y, start_x, start_y;
    int		    nx, ny;
    F_compound	   *save_compound;

    elastic_movebox();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();

    tail(&objects, &object_tails);
    save_compound = new_c;

    if ((!cur_numxcopies) && (!cur_numycopies)) {
	place_compound(x, y);
    } else {
	delta_x = cur_x - fix_x;
	delta_y = cur_y - fix_y;
	start_x = cur_x - delta_x;
	start_y = cur_y - delta_y;
	if ((cur_numxcopies < 2) && (cur_numycopies < 2)) {  /* special cases */
	    if (cur_numxcopies > 0) {
		place_compound_x(start_x+delta_x, start_y);
		new_c = copy_compound(cur_c);
	    }
	    if (cur_numycopies > 0) {
		place_compound_x(start_x, start_y+delta_y);
		new_c = copy_compound(cur_c);
	    }
	} else {
	    nx = cur_numxcopies;
	    if (nx == 0)
		nx++;
	    ny = cur_numycopies;
	    if (ny == 0)
		ny++;
	    for (i = 0, x = start_x;  i < nx; i++, x+=delta_x) {
		for (j = 0, y = start_y;  j < ny; j++, y+=delta_y) {
		    if (i || j ) {
			place_compound_x(x, y);
			new_c = copy_compound(cur_c);
		    }
		}
	    }
	}
    }
    /* put all new compounds in the saved objects structure for undo */
    saved_objects.compounds = save_compound;
    set_action_object(F_ADD, O_ALL_OBJECT);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

static void
place_compound(int x, int y)
{

    elastic_movebox();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    place_compound_x(x, y);
}

static void
place_compound_x(int x, int y)
{
    int		    dx, dy;

    canvas_leftbut_proc = null_proc;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    adjust_pos(x, y, fix_x, fix_y, &x, &y);
    dx = x - fix_x;
    dy = y - fix_y;
    translate_compound(new_c, dx, dy);
    clean_up();
    set_latestcompound(new_c);
    if (return_proc == copy_selected) {
	add_compound(new_c);
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, True);
	free_linkinfo(&cur_links);
    } else {
	list_add_compound(&objects.compounds, new_c);
	adjust_links(cur_linkmode, cur_links, dx, dy, 0, 0, 1.0, 1.0, False);
	set_lastposition(fix_x, fix_y);
	set_newposition(x, y);
	set_lastlinkinfo(cur_linkmode, cur_links);
	cur_links = NULL;
	set_action_object(F_MOVE, O_COMPOUND);
    }
    set_modifiedflag();
    redisplay_compound(new_c);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    (*return_proc) ();
    draw_mousefun_canvas();
}
