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
#include "e_scale.h"
#include "u_draw.h"
#include "u_search.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_markers.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_modepanel.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"

#include "f_util.h"
#include "u_geom.h"
#include "u_redraw.h"
#include "w_cursor.h"

/* local routine declarations */

static F_point *moved_point;

static void	fix_movedcompoundpoint(int x, int y);
static void	cancel_compound(void);

static Boolean	init_ellipsepointmoving(void);
static void	init_arcpointmoving(void);
static void	init_linepointmoving(void);
static void	init_splinepointmoving(void);
static void	init_compoundpointmoving(void);

static void	relocate_arcpoint(F_arc *arc, int x, int y, int movedpoint_num);
static void	relocate_ellipsepoint(F_ellipse *ellipse, int x, int y, int point_num);
static void	relocate_linepoint(F_line *line, int x, int y, F_point *moved_point, F_point *left_point);
static void	relocate_splinepoint(F_spline *s, int x, int y, F_point *moved_point);

static Boolean	init_move_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q);
static void	init_arb_move_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q);
static void	init_stretch_move_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q);

static void	fix_movedarcpoint(int x, int y);
static void	fix_movedellipsepoint(int x, int y);
static void	fix_movedsplinepoint(int x, int y);
static void	fix_box(int x, int y);
static void	fix_movedlinepoint(int x, int y);

static void	cancel_movedarcpoint(void);
static void	cancel_movedellipsepoint(void);
static void	cancel_movedsplinepoint(void);
static void	cancel_movept_box(void);
static void	cancel_movedlinepoint(void);


void assign_newboxpoint (F_line *b, int x1, int y1, int x2, int y2);

void move_point_selected(void)
{
    set_mousefun("move point", "horiz/vert move", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_arb_move_point);
    init_searchproc_middle(init_stretch_move_point);
    canvas_leftbut_proc = point_search_left;
    canvas_middlebut_proc = point_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    force_anglegeom();
    reset_action_on();
}

static void
init_arb_move_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
    constrained = MOVE_ARB;
    if (!init_move_point(obj, type, x, y, p, q))
	return;
    set_mousefun("new posn", "", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    canvas_middlebut_proc = null_proc;
}

static void
init_stretch_move_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
    constrained = MOVE_HORIZ_VERT;
    if (!init_move_point(obj, type, x, y, p, q))
	return;
    set_mousefun("", "new posn", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    canvas_middlebut_proc = canvas_leftbut_proc;
    canvas_leftbut_proc = null_proc;
}

static Boolean
init_move_point(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
    left_point = p;
    moved_point = q;
    switch (type) {
      case O_POLYLINE:
	cur_l = (F_line *) obj;
	right_point = q->next;
	init_linepointmoving();
	break;
      case O_SPLINE:
	cur_s = (F_spline *) obj;
	right_point = q->next;
	init_splinepointmoving();
	break;
      case O_ELLIPSE:
	force_noanglegeom();
	/* dirty trick - arcpoint_num is stored in p */
	movedpoint_num = (int) p;
	cur_e = (F_ellipse *) obj;
	if (!init_ellipsepointmoving()) /* selected center, ignore */
	    return False;
	break;
      case O_ARC:
	force_noanglegeom();
	/* dirty trick - arcpoint_num is stored in p */
	movedpoint_num = (int) p;
	cur_a = (F_arc *) obj;
	init_arcpointmoving();
	break;
      case O_COMPOUND:
	force_noanglegeom();
	/* dirty trick - posn of corner is stored in p and q */
	cur_x = (int) p;
	cur_y = (int) q;
	cur_c = (F_compound *) obj;
	init_compoundpointmoving();
	break;
    default:
	return False;
    }
    return True;
}

static void
wrapup_movepoint(void)
{
    reset_action_on();
    move_point_selected();
    draw_mousefun_canvas();
}

/*************************  ellipse  *******************************/

static		Boolean
init_ellipsepointmoving(void)
{
    double	    dx, dy, l;

    if (constrained &&
	(cur_e->type == T_CIRCLE_BY_DIA || cur_e->type == T_CIRCLE_BY_RAD)) {
	put_msg("Constrained move not supported for CIRCLES");
	return False;		/* abort - constrained move for circle not allowed */
    }
    if (movedpoint_num == 0) {
	if (cur_e->type == T_ELLIPSE_BY_RAD ||
	    cur_e->type == T_CIRCLE_BY_RAD) {
	    put_msg("Cannot move CENTER point");
	    return False;	/* abort - center point is selected */
	}
	cur_x = cur_e->start.x;
	cur_y = cur_e->start.y;
	fix_x = cur_e->end.x;
	fix_y = cur_e->end.y;
    } else {
	cur_x = cur_e->end.x;
	cur_y = cur_e->end.y;
	fix_x = cur_e->start.x;
	fix_y = cur_e->start.y;
    }
    if (constrained) {
	dx = cur_x - fix_x;
	dy = cur_y - fix_y;
	l = sqrt(dx * dx + dy * dy);
	cosa = fabs(dx / l);
	sina = fabs(dy / l);
    }
    cur_angle = cur_e->angle;
    set_action_on();
    /* turn off all markers */
    update_markers(0);
    switch (cur_e->type) {
      case T_ELLIPSE_BY_RAD:
	canvas_locmove_proc = constrained_resizing_ebr;
	canvas_ref_proc = elastic_ebr;
	break;
      case T_CIRCLE_BY_RAD:
	canvas_locmove_proc = resizing_cbr;
	canvas_ref_proc = elastic_cbr;
	break;
      case T_ELLIPSE_BY_DIA:
	canvas_locmove_proc = constrained_resizing_ebd;
	canvas_ref_proc = elastic_ebd;
	break;
      case T_CIRCLE_BY_DIA:
	canvas_locmove_proc = resizing_cbd;
	canvas_ref_proc = elastic_cbd;
	break;
    }
    /* show current radius(ii) */
    (canvas_locmove_proc)(cur_x, cur_y);
    from_x = cur_x;
    from_y = cur_y;
    set_cursor(crosshair_cursor);
    canvas_leftbut_proc = fix_movedellipsepoint;
    canvas_rightbut_proc = cancel_movedellipsepoint;
    return True;
}

static void
cancel_movedellipsepoint(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    /* erase elastic version */
    switch (cur_e->type) {
      case T_ELLIPSE_BY_RAD:
	elastic_ebr();
	break;
      case T_CIRCLE_BY_RAD:
	elastic_cbr();
	break;
      case T_ELLIPSE_BY_DIA:
	elastic_ebd();
	break;
      case T_CIRCLE_BY_DIA:
	elastic_cbd();
	break;
    }
    /* redraw original ellipse */
    redisplay_ellipse(cur_e);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
fix_movedellipsepoint(int x, int y)
{
    switch (cur_e->type) {
      case T_ELLIPSE_BY_RAD:
	elastic_ebr();
	break;
      case T_CIRCLE_BY_RAD:
	elastic_cbr();
	break;
      case T_ELLIPSE_BY_DIA:
	elastic_ebd();
	break;
      case T_CIRCLE_BY_DIA:
	elastic_cbd();
	break;
    }
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    new_e = copy_ellipse(cur_e);
    relocate_ellipsepoint(new_e, cur_x, cur_y, movedpoint_num);
    change_ellipse(cur_e, new_e);
    /* redraw anything under the old ellipse */
    redisplay_ellipse(cur_e);
    /* and the new one */
    redisplay_ellipse(new_e);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
relocate_ellipsepoint(F_ellipse *ellipse, int x, int y, int point_num)
{
    double	    dx, dy;

    set_temp_cursor(wait_cursor);
    if (point_num == 0) {	/* starting point is selected  */
	fix_x = ellipse->end.x;
	fix_y = ellipse->end.y;
	/* don't allow coincident start/end points */
	if (ellipse->end.x == x && ellipse->end.y == y)
	    return;
	ellipse->start.x = x;
	ellipse->start.y = y;
    } else {
	fix_x = ellipse->start.x;
	fix_y = ellipse->start.y;
	/* don't allow coincident start/end points */
	if (ellipse->start.x == x && ellipse->start.y == y)
	    return;
	ellipse->end.x = x;
	ellipse->end.y = y;
    }
    cur_angle = ellipse->angle;
    switch (ellipse->type) {
      case T_ELLIPSE_BY_RAD:
	ellipse->radiuses.x = abs(x - fix_x);
	ellipse->radiuses.y = abs(y - fix_y);
	break;
      case T_CIRCLE_BY_RAD:
	dx = fix_x - x;
	dy = fix_y - y;
	ellipse->radiuses.x = round(sqrt(dx * dx + dy * dy));
	ellipse->radiuses.y = ellipse->radiuses.x;
	break;
      case T_ELLIPSE_BY_DIA:
	ellipse->center.x = (fix_x + x) / 2;
	ellipse->center.y = (fix_y + y) / 2;
	ellipse->radiuses.x = abs(ellipse->center.x - fix_x);
	ellipse->radiuses.y = abs(ellipse->center.y - fix_y);
	break;
      case T_CIRCLE_BY_DIA:
	dx = ellipse->center.x = round((fix_x + x) / 2);
	dy = ellipse->center.y = round((fix_y + y) / 2);
	dx -= x;
	dy -= y;
	ellipse->radiuses.x = round(sqrt(dx * dx + dy * dy));
	ellipse->radiuses.y = ellipse->radiuses.x;
	break;
    }
    reset_cursor();
}

/***************************  arc  *********************************/

static void
init_arcpointmoving(void)
{
    set_action_on();
    /* turn off all markers */
    update_markers(0);
    cur_x = cur_a->point[movedpoint_num].x;
    cur_y = cur_a->point[movedpoint_num].y;
    set_cursor(crosshair_cursor);
    canvas_locmove_proc = reshaping_arc;
    canvas_ref_proc = elastic_arclink;
    canvas_leftbut_proc = fix_movedarcpoint;
    canvas_rightbut_proc = cancel_movedarcpoint;
    elastic_arclink();
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
}

static void
cancel_movedarcpoint(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_arclink();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
fix_movedarcpoint(int x, int y)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_arclink();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    adjust_pos(x, y, cur_a->point[movedpoint_num].x,
	       cur_a->point[movedpoint_num].y, &x, &y);
    new_a = copy_arc(cur_a);
    relocate_arcpoint(new_a, x, y, movedpoint_num);
    change_arc(cur_a, new_a);
    /* redraw anything under the old arc */
    redisplay_arc(cur_a);
    /* and the new one */
    redisplay_arc(new_a);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
relocate_arcpoint(F_arc *arc, int x, int y, int movedpoint_num)
{
    float	    xx, yy;
    F_pos	    p[3];

    p[0] = arc->point[0];
    p[1] = arc->point[1];
    p[2] = arc->point[2];
    p[movedpoint_num].x = x;
    p[movedpoint_num].y = y;
    if (compute_arccenter(p[0], p[1], p[2], &xx, &yy)) {

	arc->point[movedpoint_num].x = x;
	arc->point[movedpoint_num].y = y;
	arc->center.x = xx;
	arc->center.y = yy;
	arc->direction = compute_direction(p[0], p[1], p[2]);
    }
}

/**************************  spline  *******************************/

static void
init_splinepointmoving(void)
{
    set_action_on();
    /* turn off all markers */
    update_markers(0);
    from_x = cur_x = moved_point->x;
    from_y = cur_y = moved_point->y;
    set_cursor(crosshair_cursor);
    if (open_spline(cur_s)) {
	if (left_point == NULL || right_point == NULL) {
            if (left_point != NULL) {
                fix_x = left_point->x;
                fix_y = left_point->y;
            } else if (right_point != NULL) {
                fix_x = right_point->x;
                fix_y = right_point->y;
            }
	    if (latexline_mode || latexarrow_mode) {
                canvas_locmove_proc = latex_line;
		canvas_ref_proc = elastic_line;
                cur_latexcursor = crosshair_cursor;
            } else if (mountain_mode || manhattan_mode) {
                canvas_locmove_proc = constrainedangle_line;
		canvas_ref_proc = elastic_line;
            } else {
                /* freehand line */
                canvas_locmove_proc = reshaping_line;
		canvas_ref_proc = elastic_linelink;
            }
	} else {
            /* linelink, always freehand */
	    force_noanglegeom();
	    canvas_locmove_proc = reshaping_line;
	    canvas_ref_proc = elastic_linelink;
	}
    } else {
	/* must be closed spline */
	force_noanglegeom();
	canvas_locmove_proc = reshaping_line;
	canvas_ref_proc = elastic_linelink;
	if (left_point == NULL) {
	    for (left_point = right_point;
		 left_point->next != NULL;
		 left_point = left_point->next);
	}
	if (right_point == NULL) {
	   right_point = cur_s->points;  /* take the first */
    }
    }
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
    elastic_linelink();
    canvas_leftbut_proc = fix_movedsplinepoint;
    canvas_rightbut_proc = cancel_movedsplinepoint;
}

static void
cancel_movedsplinepoint(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_linelink();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
fix_movedsplinepoint(int x, int y)
{
    (*canvas_locmove_proc) (x, y);
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_linelink();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    old_s = copy_spline(cur_s);
    clean_up();
    set_latestspline(old_s);
    set_action_object(F_EDIT, O_SPLINE);
    old_s->next = cur_s;
    relocate_splinepoint(cur_s, cur_x, cur_y, moved_point);
    /* redraw anything under the old spline */
    redisplay_spline(old_s);
    /* and the new one */
    redisplay_spline(cur_s);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
relocate_splinepoint(F_spline *s, int x, int y, F_point *moved_point)
{
    moved_point->x = x;
    moved_point->y = y;
    set_modifiedflag();
}

/***************************  compound	********************************/

static void
init_compoundpointmoving(void)
{
    double	    dx, dy, l;
    F_line	   *line, *dum;

    set_action_on();
    if (cur_x == cur_c->nwcorner.x)
	fix_x = cur_c->secorner.x;
    else
	fix_x = cur_c->nwcorner.x;
    if (cur_y == cur_c->nwcorner.y)
	fix_y = cur_c->secorner.y;
    else
	fix_y = cur_c->nwcorner.y;
    from_x = cur_x;
    from_y = cur_y;
    /* turn off all markers */
    update_markers(0);
    set_cursor(crosshair_cursor);
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* is this a dimension line? */
    if (dimline_components(cur_c, &line, &dum, &dum, &dum) && line) {
	/* yes, constrain the resizing in the direction of the line */
	if (line->points->x == line->points->next->x)
	    /* vertical line */
	    constrained = BOX_VSTRETCH;
	else
	    /* horizontal line */
	    constrained = BOX_HSTRETCH;
    }
    if (constrained) {
	dx = cur_x - fix_x;
	dy = cur_y - fix_y;
	l = sqrt(dx * dx + dy * dy);
	cosa = fabs(dx / l);
	sina = fabs(dy / l);
    }
    canvas_locmove_proc = constrained_resizing_scale_box;
    canvas_ref_proc = elastic_fixedbox;
    canvas_leftbut_proc = fix_movedcompoundpoint;
    canvas_rightbut_proc = cancel_compound;
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
}

static void
cancel_compound(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
fix_movedcompoundpoint(int x, int y)
{
    float	    scalex, scaley;

    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);

    /* make a copy of the original and save as unchanged object */
    old_c = copy_compound(cur_c);
    clean_up();
    old_c->next = cur_c;
    set_latestcompound(old_c);
    set_action_object(F_EDIT, O_COMPOUND);

    scalex = ((float) (cur_x - fix_x)) / (from_x - fix_x);
    scaley = ((float) (cur_y - fix_y)) / (from_y - fix_y);
    /* scale the compound */
    scale_compound(cur_c, scalex, scaley, fix_x, fix_y);

    /* redraw anything under the old compound */
    redisplay_compound(old_c);
    /* and the new compound */
    redisplay_compound(cur_c);
    /* turn back on all relevant markers */
    update_markers(new_objmask);

    set_lastposition(from_x, from_y);
    set_newposition(cur_x, cur_y);
    set_modifiedflag();
    wrapup_movepoint();
}

/***************************  line  ********************************/

static void
init_linepointmoving(void)
{
    F_point	   *p;

    set_action_on();
    /* turn off all markers */
    update_markers(0);
    from_x = cur_x = moved_point->x;
    from_y = cur_y = moved_point->y;
    set_cursor(crosshair_cursor);
    switch (cur_l->type) {
      case T_POLYGON:
	if (left_point == NULL)
	    for (left_point = right_point, p = left_point->next;
		 p->next != NULL;
		 left_point = p, p = p->next);
        force_noanglegeom();
	canvas_locmove_proc = reshaping_line;
	canvas_ref_proc = elastic_linelink;
	break;

      case T_BOX:
      case T_ARCBOX:
      case T_PICTURE:
	if (right_point->next == NULL) {	/* point 4 */
	    fix_x = cur_l->points->next->x;
	    fix_y = cur_l->points->next->y;
	} else {
	    fix_x = right_point->next->x;
	    fix_y = right_point->next->y;
	}
	if (constrained) {
	    double		dx, dy, l;

	    dx = cur_x - fix_x;
	    dy = cur_y - fix_y;
	    l = sqrt(dx * dx + dy * dy);
	    cosa = fabs(dx / l);
	    sina = fabs(dy / l);
	}

        force_noanglegeom();
	if (cur_l->thickness != 1)
	    elastic_box(fix_x, fix_y, cur_x, cur_y);
	canvas_locmove_proc = constrained_resizing_box;
	canvas_ref_proc = elastic_fixedbox;
	canvas_leftbut_proc = fix_box;
	canvas_rightbut_proc = cancel_movept_box;
	/* show current length(s) */
	(canvas_locmove_proc)(cur_x, cur_y);
	return;

      case T_POLYLINE:
	if (left_point == NULL || right_point == NULL) {
	    if (left_point != NULL) {
		fix_x = left_point->x;
		fix_y = left_point->y;
	    } else if (right_point != NULL) {
		fix_x = right_point->x;
		fix_y = right_point->y;
	    }
            if (latexline_mode || latexarrow_mode) {
                canvas_locmove_proc = latex_line;
		canvas_ref_proc = elastic_line;
		cur_latexcursor = crosshair_cursor;
            } else if (mountain_mode || manhattan_mode) {
                canvas_locmove_proc = constrainedangle_line;
		canvas_ref_proc = elastic_line;
            } else {
		/* unconstrained or freehand line */
		canvas_locmove_proc = reshaping_line;
		canvas_ref_proc = elastic_linelink;
	    }
	} else {
	    /* linelink, always freehand */
            force_noanglegeom();
	    canvas_locmove_proc = reshaping_line;
	    canvas_ref_proc = elastic_linelink;
	}
	break;
    }
    canvas_leftbut_proc = fix_movedlinepoint;
    canvas_rightbut_proc = cancel_movedlinepoint;
    /* show current length(s) */
    (canvas_locmove_proc)(cur_x, cur_y);
}

static void
cancel_movept_box(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    /* erase the elastic box */
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_box_lengths();
    /* redraw original box */
    redisplay_line(cur_l);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
fix_box(int x, int y)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_box_lengths();
    adjust_box_pos(x, y, from_x, from_y, &x, &y);
    new_l = copy_line(cur_l);
    if (new_l->type == T_PICTURE) {
	if (signof(fix_x - from_x) != signof(fix_x - x))
	    new_l->pic->flipped = 1 - new_l->pic->flipped;
	if (signof(fix_y - from_y) != signof(fix_y - y))
	    new_l->pic->flipped = 1 - new_l->pic->flipped;
    }
    assign_newboxpoint(new_l, fix_x, fix_y, x, y);
    change_line(cur_l, new_l);
    /* redraw anything under the old line */
    redisplay_line(cur_l);
    /* and the new line */
    redisplay_line(new_l);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

void assign_newboxpoint(F_line *b, int x1, int y1, int x2, int y2)
{
    F_point	   *p;

    p = b->points;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
    p = p->next;
    if (p->x != x1)
	p->x = x2;
    if (p->y != y1)
	p->y = y2;
}

static void
cancel_movedlinepoint(void)
{
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    /* erase the elastic line */
    elastic_linelink();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    /* redraw original line */
    redisplay_line(cur_l);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
fix_movedlinepoint(int x, int y)
{
    (*canvas_locmove_proc) (x, y);
    canvas_ref_proc = canvas_locmove_proc = null_proc;
    elastic_linelink();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    if (cur_latexcursor != crosshair_cursor)
	set_cursor(crosshair_cursor);
    /* make a copy of the original and save as unchanged object */
    old_l = copy_line(cur_l);
    clean_up();
    set_latestline(old_l);
    set_action_object(F_EDIT, O_POLYLINE);
    old_l->next = cur_l;
    /* now change the original to become the new object */
    relocate_linepoint(cur_l, cur_x, cur_y, moved_point, left_point);
    /* redraw anything under the old line */
    redisplay_line(old_l);
    /* and the new line */
    redisplay_line(cur_l);
    /* turn back on all relevant markers */
    update_markers(new_objmask);
    wrapup_movepoint();
}

static void
relocate_linepoint(F_line *line, int x, int y, F_point *moved_point, F_point *left_point)
{
    if (line->type == T_POLYGON)
	if (line->points == moved_point) {
	    left_point->next->x = x;
	    left_point->next->y = y;
	}
    moved_point->x = x;
    moved_point->y = y;
    set_modifiedflag();
}
