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
#include "e_deletept.h"
#include "u_list.h"
#include "u_search.h"
#include "u_draw.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "d_spline.h"

#include "f_util.h"
#include "u_redraw.h"
#include "u_undo.h"
#include "w_cursor.h"

static void	init_delete_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q);



void
delete_point_selected(void)
{
    set_mousefun("delete point", "", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_delete_point);
    canvas_leftbut_proc = point_search_left;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    reset_action_on();
}

static void
init_delete_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
    int		    n;

    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) obj;
	/* the search routine will ensure we don't have a box */
	n = num_points(cur_l->points);
	if (cur_l->type == T_POLYGON) {
	    if (n <= 4) {	/* count first pt twice for closed object */
		put_msg("A polygon cannot have less than 3 points");
		beep();
		return;
	    }
	} else if (n <= 1) {
	    /* alternative would be to remove the dot altogether */
	    put_msg("A dot must have at least 1 point");
	    beep();
	    return;
	}
	linepoint_deleting(cur_l, p, q);
	break;
    case O_SPLINE:
	cur_s = (F_spline *) obj;
	n = num_points(cur_s->points);
	if (closed_spline(cur_s)) {
	    if (n <= CLOSED_SPLINE_MIN_NUM_POINTS) {
		put_msg("A closed spline cannot have less than %d points",
			CLOSED_SPLINE_MIN_NUM_POINTS);
		beep();
		return;
	    }
	} else if (n <= OPEN_SPLINE_MIN_NUM_POINTS) {
		put_msg("A spline cannot have less than %d points",
			OPEN_SPLINE_MIN_NUM_POINTS);
		beep();
		return;
	    }
	splinepoint_deleting(cur_s, p, q);
	break;
    default:
	return;
    }
}

/**************************  spline  *******************************/

void
splinepoint_deleting(F_spline *spline, F_point *previous_point, F_point *selected_point)
{
    F_point	   *next_point;
    F_sfactor      *s_prev_point, *selected_sfactor;

    next_point = selected_point->next;
    set_temp_cursor(wait_cursor);
    clean_up();
    set_last_prevpoint(previous_point);
    /* delete it and redraw underlying objects */
    list_delete_spline(&objects.splines, spline);
    draw_spline(spline, ERASE);
    redisplay_spline(spline);
    if (previous_point == NULL) {
	    spline->points = next_point;
	if (open_spline(spline)) {
	    selected_sfactor = spline->sfactors->next;
	    spline->sfactors->next = selected_sfactor->next;
	} else {
	    selected_sfactor = spline->sfactors;
	    spline->sfactors = spline->sfactors->next;
	}
    } else {
	previous_point->next = next_point;

	if ((next_point == NULL) && (open_spline(spline)))
	  previous_point = prev_point(spline->points, previous_point);

	s_prev_point = search_sfactor(spline,previous_point);
	selected_sfactor = s_prev_point->next;
	s_prev_point->next = s_prev_point->next->next;
    }

    /* put it back in the list and draw the new spline */
    list_add_spline(&objects.splines, spline);
    /* redraw it and anything on top of it */
    redisplay_spline(spline);
    set_action_object(F_DELETE_POINT, O_SPLINE);
    set_latestspline(spline);
    set_last_selectedpoint(selected_point);
    set_last_selectedsfactor(selected_sfactor);
    set_last_nextpoint(next_point);
    set_modifiedflag();
    reset_cursor();
}

/***************************  line  ********************************/

/*
 * In deleting a point selected_point, linepoint_deleting uses prev_point and
 * next_point of the point. The relationship between the three points is:
 * prev_point->selected_point->next_point except when selected_point is the
 * first point in the list, in which case prev_point will be NULL.
 */

void
linepoint_deleting(F_line *line, F_point *prev_point, F_point *selected_point)
{
    F_point	   *p, *next_point;

    next_point = selected_point->next;
    /* delete it and redraw underlying objects */
    list_delete_line(&objects.lines, line);
    redisplay_line(line);
    if (line->type == T_POLYGON) {
	if (prev_point == NULL) {
	    /* The deleted point is the first point */
	    line->points = next_point;
	    for (prev_point = next_point, p = prev_point->next;
		 p->next != NULL;
		 prev_point = p, p = p->next);
	    /*
	     * prev_point now points at next to last point (the last point is
	     * a copy of the first).
	     */
	    p->x = next_point->x;
	    p->y = next_point->y;
	    next_point = p;
	    /*
	     * next_point becomes the last point.  If this operation (point
	     * deletion) is reversed (undo), the selected_point will not be
	     * inserted into it original place, but will be between
	     * prev_point and next_point.
	     */
	} else
	    prev_point->next = next_point;
    } else {			/* polyline */
	if (prev_point == NULL)
	    line->points = next_point;
	else
	    prev_point->next = next_point;
    }
    /* put it back in the list and draw the new line */
    list_add_line(&objects.lines, line);
    /* redraw it and anything on top of it */
    redisplay_line(line);
    clean_up();
    set_modifiedflag();
    set_action_object(F_DELETE_POINT, O_POLYLINE);
    set_latestline(line);
    set_last_prevpoint(prev_point);
    set_last_selectedpoint(selected_point);
    set_last_nextpoint(next_point);
}
