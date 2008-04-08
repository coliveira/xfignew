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

extern "C" {
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "e_addpt.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_modepanel.h"

#include "u_geom.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "u_undo.h"
#include "w_cursor.h"
}

static void	init_point_adding(F_line *p, int type, int x, int y, int px, int py);
static void	fix_linepoint_adding(int x, int y);
static void	fix_splinepoint_adding(int x, int y);
static void	init_linepointadding(int px, int py);
static void	init_splinepointadding(int px, int py);


void
point_adding_selected(void)
{
    set_mousefun("break/add here", "", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = (FCallBack)null_proc;
    init_searchproc_left((IntFunc)init_point_adding);
    canvas_leftbut_proc = (FCallBack)object_search_left;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)null_proc;
    set_cursor(pick9_cursor);
    force_nopositioning();
    force_anglegeom();
    constrained = MOVE_ARB;
    reset_action_on();
}

static void
init_point_adding(F_line *p, int type, int x, int y, int px, int py)
{
    set_action_on();
    set_mousefun("place new point", "", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    set_cursor(null_cursor);
    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) p;
	/* the search routine will ensure that we don't have a box */
	init_linepointadding(px, py);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	init_splinepointadding(px, py);
	break;
    default:
	return;
    }
    force_positioning();
    /* draw in rubber-band line */
    elastic_linelink();

    if (left_point == NULL || right_point == NULL) {
	if (latexline_mode || latexarrow_mode) {
	    canvas_locmove_proc = latex_line;
	    canvas_ref_proc = (FCallBack)elastic_line;
	    return;
	}
	if (mountain_mode || manhattan_mode) {
	    canvas_locmove_proc = constrainedangle_line;
	    canvas_ref_proc = (FCallBack)elastic_line;
	    return;
	}
    } else {
	force_noanglegeom();
    }
    canvas_locmove_proc = reshaping_line;
    canvas_ref_proc = (FCallBack)elastic_linelink;
}

static void
wrapup_pointadding(void)
{
    reset_action_on();
    point_adding_selected();
    draw_mousefun_canvas();
}

static void
cancel_pointadding(void)
{
    canvas_ref_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    elastic_linelink();
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_pointadding();
}

static void
cancel_line_pointadding(int x, int y, int shift)
{
    canvas_ref_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    if (left_point != NULL && right_point != NULL)
	pw_vector(canvas_win, left_point->x, left_point->y,
		  right_point->x, right_point->y, PAINT,
		  cur_l->thickness, cur_l->style, cur_l->style_val,
		  cur_l->pen_color);
    cancel_pointadding();
}

/**************************  spline  *******************************/

static void
init_splinepointadding(int px, int py)
{
    find_endpoints(cur_s->points, px, py, &left_point, &right_point);

    cur_x = fix_x = px;
    cur_y = fix_y = py;
    if (left_point == NULL && closed_spline(cur_s)) {
	/* The added_point is between the 1st and 2nd point. */
	left_point = right_point;
	right_point = right_point->next;
    }
    if (right_point == NULL && closed_spline(cur_s)) {
	/* The added_point is between the last and 1st point. */
	right_point = cur_s->points;
    }
    canvas_leftbut_proc = (FCallBack)fix_splinepoint_adding;
    canvas_rightbut_proc = (FCallBack)cancel_pointadding;
    /* turn off all markers */
    update_markers(0);
}

static void
fix_splinepoint_adding(int x, int y)
{
    F_point	   *p;

    /* if this point is coincident with the point being added to, return */
    if (((left_point == NULL) && 
	   (cur_x == cur_s->points[0].x) && (cur_y == cur_s->points[0].y)) ||
	   ((left_point != NULL) && 
	   (left_point->x == cur_x) && (left_point->y == cur_y))) {
	return;
    }

    (*canvas_locmove_proc) (x, y);
    if ((p = create_point()) == NULL) {
	wrapup_pointadding();
	return;
    }
    p->x = cur_x;
    p->y = cur_y;
    elastic_linelink();
    wrapup_pointadding();
    splinepoint_adding(cur_s, left_point, p, right_point,
		   approx_spline(cur_s) ? S_SPLINE_APPROX : S_SPLINE_INTERP);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

/*
 * Added_point is always inserted between left_point and
 * right_point, except in two cases. (1) left_point is NULL, the added_point
 * will be prepended to the list of points. (2) right_point is NULL, the 
 * added_point will be appended to the end of the list.
 */

void
splinepoint_adding(F_spline *spline, F_point *left_point, F_point *added_point, F_point *right_point, double sfactor)
{
    F_sfactor	   *c, *prev_sfactor;


    if ((c = create_sfactor()) == NULL)
	    return;
    set_temp_cursor(wait_cursor);
    /* delete it and redraw underlying objects */
    list_delete_spline(&objects.splines, spline);
    redisplay_spline(spline);
    if (left_point == NULL) {
	added_point->next = spline->points;
	spline->points = added_point;
	if (open_spline(spline)) {
	  c->s = S_SPLINE_ANGULAR;
	  spline->sfactors->s = sfactor;
    }
	else
	  c->s = sfactor;

	c->next = spline->sfactors;
	spline->sfactors = c;
      }
    else {
      prev_sfactor = search_sfactor(spline, left_point);
      if (open_spline(spline) && (right_point == NULL))
	{
	  c->s = S_SPLINE_ANGULAR;
	  prev_sfactor->s = sfactor;
	}
      else
	c->s = sfactor;
     
      c->next = prev_sfactor->next;
      prev_sfactor->next = c;
      added_point->next = left_point->next; /*right_point;*/
      left_point->next = added_point;
    }
    /* put it back in the list and draw the new spline */
    list_add_spline(&objects.splines, spline);
    /* redraw it and anything on top of it */
    redisplay_spline(spline);
    clean_up();
    set_modifiedflag();
    set_last_prevpoint(left_point);
    set_last_selectedpoint(added_point);
    set_action_object(F_ADD_POINT, O_SPLINE);
    set_latestspline(spline);
    reset_cursor();
}

/***************************  line  ********************************/

static void
init_linepointadding(int px, int py)
{
    find_endpoints(cur_l->points, px, py, &left_point, &right_point);

    /* set cur_x etc at new point coords */
    cur_x = fix_x = px;
    cur_y = fix_y = py;
    if (left_point == NULL && cur_l->type == T_POLYGON) {
	left_point = right_point;
	right_point = right_point->next;
    }
    /* erase line segment where new point is */
    if (left_point != NULL && right_point != NULL)
	pw_vector(canvas_win, left_point->x, left_point->y,
		  right_point->x, right_point->y, ERASE,
		  cur_l->thickness, cur_l->style, cur_l->style_val,
		  cur_l->pen_color);

    canvas_leftbut_proc = (FCallBack)fix_linepoint_adding;
    canvas_rightbut_proc = cancel_line_pointadding;
    /* turn off all markers */
    update_markers(0);
}

static void
fix_linepoint_adding(int x, int y)
{
    F_point	   *p;

    /* if this point is coincident with the point being added to, return */
    if (((left_point == NULL) && 
	   (cur_x == cur_l->points[0].x) && (cur_y == cur_l->points[0].y)) ||
	   ((left_point != NULL) && 
	   (left_point->x == cur_x) && (left_point->y == cur_y))) {
	return;
    }

    (*canvas_locmove_proc) (x, y);
    if ((p = create_point()) == NULL) {
	wrapup_pointadding();
	return;
    }
    p->x = cur_x;
    p->y = cur_y;
    elastic_linelink();
    wrapup_pointadding();
    linepoint_adding(cur_l, left_point, p);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
}

void
linepoint_adding(F_line *line, F_point *left_point, F_point *added_point)
{
    /* turn off all markers */
    update_markers(0);
    /* delete it and redraw underlying objects */
    list_delete_line(&objects.lines, line);
    redisplay_line(line);
    if (left_point == NULL) {
	added_point->next = line->points;
	line->points = added_point;
    } else {
	added_point->next = left_point->next;
	left_point->next = added_point;
    }
    /* put it back in the list and draw the new line */
    list_add_line(&objects.lines, line);
    /* redraw it and anything on top of it */
    redisplay_line(line);
    clean_up();
    set_action_object(F_ADD_POINT, O_POLYLINE);
    set_latestline(line);
    set_last_prevpoint(left_point);
    set_last_selectedpoint(added_point);
    set_modifiedflag();
}

/*******************************************************************/

/*
 * If (x,y) is close to a point q, fp points to q and sp points to q->next
 * (right).  However if q is the first point, fp contains NULL and sp points
 * to q.
 */

void
find_endpoints(F_point *p, int x, int y, F_point **fp, F_point **sp)
{
    int		    d;
    F_point	   *a = NULL, *b = p;

    if (x == b->x && y == b->y) {
	*fp = a;
	*sp = b;
	return;
    }
    for (a = p, b = p->next; b != NULL; a = b, b = b->next) {
	if (x == b->x && y == b->y) {
	    *fp = b;
	    *sp = b->next;
	    return;
	}
	if (close_to_vector(a->x, a->y, b->x, b->y, x, y, 1, 1.0, &d, &d)) {
	    *fp = a;
	    *sp = b;
	    return;
	}
    }
    *fp = a;
    *sp = b;
}
