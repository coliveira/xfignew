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
#include "u_create.h"
#include "u_draw.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_modepanel.h"
#include "w_zoom.h"

#include "e_addpt.h"
#include "f_util.h"
#include "u_free.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "w_cursor.h"

static void	init_join(F_line *obj, int type, int x, int y, F_point *p, F_point *q);
static void	init_split(F_line *obj, int type, int x, int y, int px, int py);
static void	join_lines(F_line *line, F_point *prev_point, F_point *selected_point);
static void	join_splines(F_spline *spline, F_point *prev_point, F_point *selected_point);
static void	split_line(int px, int py);
static void	split_spline(int px, int py);
static void	cancel_join(void);
static void	join_line2(F_line *obj, int type, int x, int y, F_point *p, F_point *q);
static void	join_spline2(F_line *obj, int type, int x, int y, F_point *p, F_point *q);
static Boolean	connect_line_points(F_line *line1, Boolean first1, F_line *line2, Boolean first2, F_line *new_line);
static Boolean	connect_spline_points(F_spline *spline1, Boolean first1, F_spline *spline2, Boolean first2, F_spline *new_spline);


void draw_join_marker (F_point *point);
void split_polygon (F_line *poly, F_line *line, F_point *point);
void split_cspline (F_spline *cspline, F_spline *spline, F_point *point, F_sfactor *csf);

void
join_split_selected(void)
{
    set_mousefun("Join Lines/Splines", "Split Line/Spline", "", 
			LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_join);
    init_searchproc_middle(init_split);
    canvas_leftbut_proc = point_search_left;		/* point search for join */
    canvas_middlebut_proc = object_search_middle;	/* object search for split */
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    /* set the markers to show we only allow POLYLINES and open SPLINES */
    /* (the markers are originally set this way from the mode panel, but
       we need to set them again after the previous join/split */
    update_markers(M_POLYLINE | M_SPLINE_O | M_SPLINE_C);
    reset_action_on();
}

static void
init_join(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
    switch (type) {
      case O_POLYLINE:
	cur_l = (F_line *) obj;
	join_lines(cur_l, p, q);
	break;
      case O_SPLINE:
	cur_s = (F_spline *) obj;
	join_splines(cur_s, p, q);
	break;
      default:
	return;
    }
}

static void
init_split(F_line *obj, int type, int x, int y, int px, int py)
{
    switch (type) {
      case O_POLYLINE:
	cur_l = (F_line *) obj;
	if (cur_l->type == T_PICTURE) {
	    put_msg("Can't remove points from picture");
	    beep();
	    return;
	}
	split_line(px, py);
	break;
      case O_SPLINE:
	cur_s = (F_spline *) obj;
	split_spline(px, py);
	break;
    }
}

/**************************  join lines or splines  *******************************/

static F_line	*line1;
static F_spline	*spline1;
static F_point	*sel_point;
static Boolean	 first1, first2;

static void
join_lines(F_line *line, F_point *prev_point, F_point *selected_point)
{
	/* can only join lines (not boxes or polygons) */
	if (line->type != T_POLYLINE)
	    return;
	/* only continue if the user has selected an end point */
	if (prev_point != NULL && selected_point->next != NULL)
		return;
	init_searchproc_left(join_line2);
	canvas_leftbut_proc = point_search_left;
	canvas_middlebut_proc = null_proc;
	canvas_rightbut_proc = cancel_join;
	line1 = line;
	sel_point = selected_point;
	/* set flag if this is the first point of the line */
	first1 = (prev_point == NULL);
	set_mousefun("Choose next Line", "", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
	draw_mousefun_canvas();
	draw_join_marker(sel_point);
	/* only want lines now */
	update_markers(M_POLYLINE_LINE);
}

static void
join_splines(F_spline *spline, F_point *prev_point, F_point *selected_point)
{
	/* can only join open splines */
	if (spline->type & 1)
	    return;

	/* only continue if the user has selected an end point */
	if (prev_point != NULL && prev_point != spline->points && 
	    selected_point->next != NULL)
		return;
	init_searchproc_left(join_spline2);
	canvas_leftbut_proc = point_search_left;
	canvas_middlebut_proc = null_proc;
	canvas_rightbut_proc = cancel_join;
	/* save pointer to first line and its points */
	spline1 = spline;
	sel_point = selected_point;
	/* set flag if this is the first point of the spline */
	first1 = (selected_point == spline->points);
	set_mousefun("Choose next Spline", "", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
	draw_mousefun_canvas();
	draw_join_marker(sel_point);
	/* only want open splines now */
	update_markers(M_SPLINE_O);
}

void draw_join_marker(F_point *point)
{
	int x=ZOOMX(point->x), y=ZOOMY(point->y);
	pw_vector(canvas_win, x-10, y-10, x-10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x-10, y+10, x+10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y+10, x+10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y-10, x-10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
}

void erase_join_marker(F_point *point)
{
    int x=ZOOMX(point->x), y=ZOOMY(point->y);
    if (point) {
	pw_vector(canvas_win, x-10, y-10, x-10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x-10, y+10, x+10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y+10, x+10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y-10, x-10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
    }
    point = NULL;
}


static void
cancel_join(void)
{
    join_split_selected();
    /* erase any marker */
    erase_join_marker(sel_point);
    draw_mousefun_canvas();
}

static void
join_line2(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
	F_line	   *line;
	F_point	   *lastp;

	if (type != O_POLYLINE)
		return;
	/* only continue if the user has selected an end point */
	if (p != NULL && q->next != NULL)
		return;
	line = (F_line *) obj;
	first2 = (p == NULL);
	/* if user clicked on same point twice return */
	if (line == line1 && first1 == first2)
	    return;
	new_l = copy_line(line1);
	/* if user clicked on both endpoints of a line, make it a POLYGON */
	if (line == line1 && first1 != first2) {
	    new_l->type = T_POLYGON;
	    lastp = last_point(new_l->points);
	    append_point(new_l->points->x, new_l->points->y, &lastp);
	} else if (!connect_line_points(line1, first1, line, first2, new_l)) {
		free(new_l);
		return;
	}
	clean_up();
	/* erase marker */
	erase_join_marker(sel_point);
	/* delete old ones from the main list */
	if (line != line1)
	    list_delete_line(&objects.lines, line1);
	list_delete_line(&objects.lines, line);
	/* put them in the saved list */
	if (line != line1)
	    list_add_line(&saved_objects.lines, line1);
	list_add_line(&saved_objects.lines, line);
	/* and add the new one in */
	list_add_line(&objects.lines, new_l);
	set_action_object(F_JOIN, O_POLYLINE);
	/* save pointer to this line for undo */
	latest_line = new_l;
	redisplay_line(new_l);
	/* start over */
	join_split_selected();
}

static void
join_spline2(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
	F_spline   *spline;

	if (type != O_SPLINE)
		return;
	/* only continue if the user has selected an end point */
	if (p != NULL && q->next != NULL)
		return;
	spline = (F_spline *) obj;
	first2 = (q == obj->points);
	new_s = copy_spline(spline1);
	/* if user clicked on both endpoints of a spline, make it a closed SPLINE */
	if (spline == spline1) {
	    /* to make a closed spline from an open one, add one to the type */
	    /* no need to duplicate the first point at the end */
	    new_s->type++;
	    /* set sfactors of old endpoints to sfactor of second point in spline */
	    new_s->sfactors->s = new_s->sfactors->next->s;
	    last_sfactor(new_s->sfactors)->s = new_s->sfactors->s;
	} else if (!connect_spline_points(spline1, first1, spline, first2, new_s)) {
		free(new_s);
		return;
	}
	clean_up();
	/* erase marker */
	erase_join_marker(sel_point);
	/* delete them from the main list */
	if (spline != spline1)
	    list_delete_spline(&objects.splines, spline1);
	list_delete_spline(&objects.splines, spline);
	/* put them in the saved list */
	if (spline != spline1)
	    list_add_spline(&saved_objects.splines, spline1);
	list_add_spline(&saved_objects.splines, spline);
	/* and add the new one in */
	list_add_spline(&objects.splines, new_s);
	set_action_object(F_JOIN, O_SPLINE);
	/* save pointer to this spline for undo */
	latest_spline = new_s;
	redisplay_spline(new_s);
	/* start over */
	join_split_selected();
}

/* Connect the points from line1 and line2 in the order determined
   by first1 and first2, which signify whether user clicked on first point
   of respective line.  
   new_line must already have a copy of line1.
*/

static Boolean
connect_line_points(F_line *line1, Boolean first1, F_line *line2, Boolean first2, F_line *new_line)
{
	F_point    *p1,*p2;

	if (first1 && first2) {
	    /* user clicked on both first points */
	    /* reverse order of points in first line */
	    reverse_points(new_line->points);
	    /* then attach points of second line */
	    p1 = last_point(new_line->points);
	    if ((p1->next = copy_points(line2->points)) == NULL)
		return False;
	} else if (first1 == False && first2) {
	    /* user clicked on endpoint of line 1 and first point of line 2 */
	    /* find the end point of the new points, which is a copy of the first line */
	    p1 = last_point(new_line->points);
	    if ((p1->next = copy_points(line2->points)) == NULL)
		return False;
	} else if (first1 && first2 == False) {
	    /* user clicked on first point of line 1 and endpoint of line 2 */
	    /* reverse order of points in first line */
	    reverse_points(new_line->points);
	    /* make copy of points in second line */
	    if ((p2 = copy_points(line2->points)) == NULL)
		return False;
	    /* reverse them */
	    reverse_points(p2);
	    /* and attach to end of first */
	    p1 = last_point(new_line->points);
	    p1->next = p2;
	} else {
	    /* user clicked on both end points */
	    /* make copy of points in second line */
	    if ((p2 = copy_points(line2->points)) == NULL)
		return False;
	    /* reverse them */
	    reverse_points(p2);
	    /* and attach to end of first */
	    p1 = last_point(new_line->points);
	    p1->next = p2;
	}
	return True;
}

/* Connect the points from spline1 and spline2 in the order determined
   by first1 and first2, which signify whether user clicked on first point
   of respective spline.  
   new_spline must already have a copy of spline1.
*/

static Boolean
connect_spline_points(F_spline *spline1, Boolean first1, F_spline *spline2, Boolean first2, F_spline *new_spline)
{
	F_point    *p1, *p2;
	F_sfactor  *sf1, *sf2;

	if (first1 && first2) {
	    /* user clicked on both first points */
	    /* reverse order of points in first spline */
	    reverse_points(new_spline->points);
	    /* and the spline factors */
	    reverse_sfactors(new_spline->sfactors);
	    /* then attach points of second spline */
	    p1 = last_point(new_spline->points);
	    if ((p1->next = copy_points(spline2->points)) == NULL)
		return False;
	    /* don't forget sfactors */
	    sf1 = last_sfactor(new_spline->sfactors);
	    if ((sf1->next = copy_sfactors(spline2->sfactors)) == NULL)
		return False;
	} else if (first1 == False && first2) {
	    /* user clicked on endpoint of spline 1 and first point of spline 2 */
	    /* find the end point of the new points, which is a copy of the first spline */
	    p1 = last_point(new_spline->points);
	    if ((p1->next = copy_points(spline2->points)) == NULL)
		return False;
	    /* don't forget sfactors */
	    sf1 = last_sfactor(new_spline->sfactors);
	    if ((sf1->next = copy_sfactors(spline2->sfactors)) == NULL)
		return False;
	} else if (first1 && first2 == False) {
	    /* user clicked on first point of spline 1 and endpoint of spline 2 */
	    /* reverse order of points in first spline */
	    reverse_points(new_spline->points);
	    /* and the spline factors */
	    reverse_sfactors(new_spline->sfactors);
	    /* make copy of points in second spline */
	    if ((p2 = copy_points(spline2->points)) == NULL)
		return False;
	    /* and sfactors */
	    if ((sf2 = copy_sfactors(spline2->sfactors)) == NULL)
		return False;
	    /* reverse them */
	    reverse_points(p2);
	    /* and the spline factors */
	    reverse_sfactors(sf2);
	    /* and attach to end of first */
	    p1 = last_point(new_spline->points);
	    p1->next = p2;
	    /* and sfactors */
	    sf1 = last_sfactor(new_spline->sfactors);
	    sf1->next = sf2;
	} else {
	    /* user clicked on both end points */
	    /* make copy of points in second spline */
	    if ((p2 = copy_points(spline2->points)) == NULL)
		return False;
	    /* and sfactors */
	    if ((sf2 = copy_sfactors(spline2->sfactors)) == NULL)
		return False;
	    /* reverse them */
	    reverse_points(p2);
	    /* and the spline factors */
	    reverse_sfactors(sf2);
	    /* and attach to end of first */
	    p1 = last_point(new_spline->points);
	    p1->next = p2;
	    /* and sfactors */
	    sf1 = last_sfactor(new_spline->sfactors);
	    sf1->next = sf2;
	}
	return True;
}

/**************************  split line or spline  *******************************/

static void
split_line(int px, int py)
{
    F_point	   *p;
    F_line	   *new_l1, *new_l2, *save_l;
    F_point	   *left_point, *right_point;

    find_endpoints(cur_l->points, px, py, &left_point, &right_point);

    if (cur_l->points->next == NULL)
	return;				/* A single point line - that would be tough to split */

    if (cur_l->type != T_POLYGON && cur_l->type != T_BOX && cur_l->type != T_ARCBOX) {
	if (left_point == NULL) 	/* selected_point is the first point */
	    return;
	else if (right_point == NULL)	/* last point */
	    return;
    }

    /* copy original */
    new_l1 = copy_line(cur_l);

    /* keep copy of original */
    save_l = copy_line(cur_l);
    /* remove original line from the objects */
    delete_line(cur_l);

    if (cur_l->type == T_POLYGON || cur_l->type == T_BOX || cur_l->type == T_ARCBOX) {
	/* change polygon or box to polyline */

	/* search original for point */
	if (left_point == NULL) {
	    /* if first point, start at second */
	    p = cur_l->points->next;
	} else if (right_point == NULL) {
	    /* if last point, start at first */
	    p = cur_l->points;
	} else {
	    /* somewhere in the middle */
	    p = search_line_point(cur_l, left_point->x, left_point->y);
	    p = p->next;
	}
	split_polygon(cur_l, new_l1, p);
	clean_up();
	list_add_line(&objects.lines,new_l1);
    } else {
	/* split one polyline into two */

	/* find the breakpoint in the line */
	p = search_line_point(new_l1, left_point->x, left_point->y);
	new_l2 = copy_line(cur_l);
	free_points(new_l2->points);
	/* attach right side to new line 2 */
	new_l2->points = p->next;
	/* and unlink that from new line 1 */
	p->next = NULL;
	clean_up();
	list_add_line(&objects.lines,new_l1);
	list_add_line(&objects.lines,new_l2);
    }
    set_modifiedflag();
    /* save pointer to this(these) line(s) for undo */
    latest_line = new_l1;
    /* put the original line in the saved lines list for undo */
    saved_objects.lines = save_l;
    set_action_object(F_SPLIT, O_POLYLINE);
    /* refresh area where original line was.  This is the bounding area of both new lines */
    redisplay_line(save_l);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    join_split_selected();
}

/* turn polygon or box "poly" into polyline "line" by breaking at point "point" */

void split_polygon(F_line *poly, F_line *line, F_point *point)
{
    F_point	   *splitp, *p, *q;

    /* we'll just copy the points from poly (the original) into line starting
       at point, until we hit the end of the points, then copy the points from
       the beginning of poly till we hit the original split point */
    p = line->points;
    splitp = point;
    /* don't copy the last point because it is just a duplicate of the first */
    while (point->next) {
	p->x = point->x;
	p->y = point->y;
	q = p;			/* save pointer to current point for removing later */
	p = p->next;
	point = point->next;
    }
    /* now do points from beginning of poly */
    point = poly->points;
    while (point != splitp) {
	p->x = point->x;
	p->y = point->y;
	q = p;			/* save pointer to current point for removing later */
	p = p->next;
	point = point->next;
    }
    /* free last point from new line because we don't need it in a POLYLINE */
    free_points(p);
    /* and disconnect it from the list */
    q->next = NULL;
    /* last, make it a POLYLINE */
    line->type = T_POLYLINE;
}


static void
split_spline(int px, int py)
{
    F_spline	   *new_spl1, *new_spl2, *save_spl;
    F_point	   *p, *cp, *left_point, *right_point;
    F_sfactor	   *sf, *csf;

    find_endpoints(cur_s->points, px, py, &left_point, &right_point);

    if ((cur_s->type & 1) == 0) {
	/* open spline - don't allow splitting into a spline with only one point */
	if (left_point == NULL || left_point == cur_s->points || /* first point */
	   (right_point == NULL || right_point->next == NULL)) { /* last, OR only one point to the left */
	    put_msg("Cutting there would result in a spline with only one point");
	    beep();
	    return;		
	}
    }

    /* make copy of original */
    new_spl1 = copy_spline(cur_s);

    /* find the breakpoint in the spline */
    p = new_spl1->points;
    sf = new_spl1->sfactors;
    cp = cur_s->points;
    csf = cur_s->sfactors;
    /* first point, set pointers to second point */
    if (left_point == NULL && right_point != NULL) {
	p = new_spl1->points->next;
	sf = new_spl1->sfactors->next;
	cp = cur_s->points->next;
	csf = cur_s->sfactors->next;
    /* if not the last point, search */
    } else if (right_point != NULL) {
	/* locate the sfactors (can't use search_sfactor() because it looks at the
	   pointer values, which won't match because we're looking in the copy */
	/* this will locate the break point itself too */
	while (p->x != left_point->x || p->y != left_point->y) {
	    p = p->next;
	    sf = sf->next;
	    cp = cp->next;
	    csf = csf->next;	/* sfactors are in same order in both copies */
	}
	if ((cur_s->type & 1) == 1) {
	    /* and advance to next if closed spline */
	    p = p->next;
	    sf = sf->next;
	    cp = cp->next;
	    csf = csf->next;
	}
    }

    /* keep copy of original */
    save_spl = copy_spline(cur_s);
    /* remove original spline from the objects */
    delete_spline(cur_s);

    if ((cur_s->type & 1) == 1) {
	/* turn closed spline into open */
	split_cspline(cur_s, new_spl1, cp, csf);
	clean_up();
	list_add_spline(&objects.splines,new_spl1);
    } else {
	/* make two splines from one - make another copy */
	new_spl2 = copy_spline(cur_s);
	free_points(new_spl2->points);
	free_sfactors(new_spl2->sfactors);
	/* attach right side to new spline 2 */
	new_spl2->points = p->next;
	new_spl2->sfactors = sf->next;
	/* make new endpoint have sfactor 0.0 (sharp) */
	new_spl2->sfactors->s = 0.0;
	/* and unlink that from new spline 1 */
	p->next = NULL;
	sf->next = NULL;
	/* make this new endpoint have sfactor 0.0 (sharp) */
	sf->s = 0.0;
	clean_up();
	list_add_spline(&objects.splines,new_spl1);
	list_add_spline(&objects.splines,new_spl2);
    }
    set_modifiedflag();
    /* save pointer to these splines for undo */
    latest_spline = new_spl1;
    /* put the original spline in the saved splines list for undo */
    saved_objects.splines = save_spl;
    set_action_object(F_SPLIT, O_SPLINE);
    /* refresh area where original spline was.  This is the bounding area of both new splines */
    redisplay_spline(save_spl);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    join_split_selected();
}

/* turn closed spline "cspline" open spline "spline" by breaking at point "point" */

void split_cspline(F_spline *cspline, F_spline *spline, F_point *point, F_sfactor *csf)
{
    F_point	   *splitp, *p;
    F_sfactor	   *sf;

    /* we'll just copy the points (and sfactors) from cspline (the original) into
	spline starting at point, until we hit the end of the points, then copy the
	points from the beginning of cspline till we hit the original split point */

    /* save split point */
    splitp = point;

    sf = spline->sfactors;
    p = spline->points;
    while (point) {
	p->x = point->x;
	p->y = point->y;
	sf->s = csf->s;
	if (point == splitp)	 
	    /* set sfactor on first point to 0.0 */
	    sf->s = 0.0;
	if (point->next == NULL && splitp == cspline->points)
	    /* set sfactor on last point to 0.0 if next loop is not done */
	    sf->s = 0.0;
	p = p->next;
	point = point->next;
	sf = sf->next;
	csf = csf->next;
    }
    /* now do points from beginning of cspline */
    point = cspline->points;
    csf = cspline->sfactors;
    while (point != splitp) {
	p->x = point->x;
	p->y = point->y;
	sf->s = csf->s;
	if (point->next == splitp)	/* set sfactor on endpoint to 0.0 */
	    sf->s = 0.0;
	p = p->next;
	point = point->next;
	sf = sf->next;
	csf = csf->next;
    }
    /* last, make it a OPEN spline (closed are odd, open are even) */
    spline->type--;
}
