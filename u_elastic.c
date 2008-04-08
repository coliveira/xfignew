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
#include "u_elastic.h"
#include "u_geom.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_setup.h"
#include "w_zoom.h"
#include "d_arc.h"

#include "u_draw.h"
#include "w_cursor.h"
#include "w_msgpanel.h"

/********************** EXPORTS **************/

int		constrained;
int		work_numsides;
float		cur_angle;
int		x1off, x2off, y1off, y2off;
Cursor		cur_latexcursor;
int		from_x, from_y;
double		cosa, sina;
int		movedpoint_num;
F_point	       *left_point, *right_point;

/**************** LOCAL ***********/

static void	elastic_links(int dx, int dy, float sx, float sy);
static void	angle0_line(int x, int y);
static void	angle45_line(int x, int y);
static void	angle90_line(int x, int y);
static void	angle135_line(int x, int y);

/*************************** BOXES *************************/



void
elastic_box(int x1, int y1, int x2, int y2)
{
    int		    wid, ht;

    set_line_stuff(1, RUBBER_LINE, 0.0, JOIN_MITER, CAP_BUTT,
		INV_PAINT, DEFAULT);
    wid = abs(x2-x1)+1;
    ht = abs(y2-y1)+1;
    zXDrawRectangle(tool_d, canvas_win, gccache[INV_PAINT],min2(x1,x2),min2(y1,y2),wid,ht);
}

void
elastic_fixedbox(void)
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
}

void
elastic_movebox(void)
{
    register int    x1, y1, x2, y2;

    x1 = cur_x + x1off;
    x2 = cur_x + x2off;
    y1 = cur_y + y1off;
    y2 = cur_y + y2off;
    elastic_box(x1, y1, x2, y2);
    elastic_links(cur_x - fix_x, cur_y - fix_y, 1.0, 1.0);
}

void
moving_box(int x, int y)
{
    elastic_movebox();
    adjust_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    length_msg(MSG_DIST);
    elastic_movebox();
}

void
resizing_box(int x, int y)
{
    elastic_fixedbox();
    cur_x = x;
    cur_y = y;
    boxsize_msg(1);
    elastic_fixedbox();
}

void
constrained_resizing_box(int x, int y)
{
    elastic_fixedbox();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    boxsize_msg(1);
    elastic_fixedbox();
}

void
constrained_resizing_scale_box(int x, int y)
{
    elastic_fixedbox();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    elastic_fixedbox();
    boxsize_scale_msg(1);
}

void
scaling_compound(int x, int y)
{
    elastic_scalecompound(cur_c);
    adjust_box_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    elastic_scalecompound(cur_c);
}

void
elastic_scale_curcompound(void)
{
    elastic_scalecompound(cur_c);
}

void
elastic_scalecompound(F_compound *c)
{
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;
    int		    x1, y1, x2, y2;

    newx = cur_x - fix_x;
    newy = cur_y - fix_y;
    newd = sqrt(newx * newx + newy * newy);
    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);
    scalefact = newd / oldd;
    x1 = fix_x + round((c->secorner.x - fix_x) * scalefact);
    y1 = fix_y + round((c->secorner.y - fix_y) * scalefact);
    x2 = fix_x + round((c->nwcorner.x - fix_x) * scalefact);
    y2 = fix_y + round((c->nwcorner.y - fix_y) * scalefact);
    boxsize_scale_msg(2);
    elastic_box(x1, y1, x2, y2);
}

/*************************** LINES *************************/

/* refresh a line segment */

void
elastic_line(void)
{
    pw_vector(canvas_win, fix_x, fix_y, cur_x, cur_y,
	      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    /* erase endpoint because next seg will redraw it */
    pw_vector(canvas_win, cur_x, cur_y, cur_x, cur_y,
	      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
}

/* this is only called by  get_intermediatepoint() for drawing lines and by 
   the canvas_locmove_proc for drawing arcs */

void
unconstrained_line(int x, int y)
{
    elastic_line();
    cur_x = x;
    cur_y = y;
    length_msg(MSG_PNTS_LENGTH);
    elastic_line();
}

void
latex_line(int x, int y)
{
    Cursor c;

    elastic_line();
    latex_endpoint(fix_x, fix_y, x, y, &cur_x, &cur_y, latexarrow_mode,
		   (cur_pointposn == P_ANY) ? 1 : posn_rnd[cur_gridunit][cur_pointposn]);
    length_msg(MSG_LENGTH);
    elastic_line();
    c = (x == cur_x && y == cur_y) ? null_cursor : crosshair_cursor;
    if (c != cur_cursor) {
	set_temp_cursor(c);
	cur_cursor = c;
    }
}

void
constrainedangle_line(int x, int y)
{
    double	    angle, dx, dy;

    if (x == cur_x && y == cur_y)
	return;

    dx = x - fix_x;
    dy = fix_y - y;
    /* only move if the pointer has moved at least 2 pixels */
    if (sqrt(dx * dx + dy * dy) < 2.0)
	return;
    if (dx == 0)
	angle = -90.0;
    else
	angle = 180.0 * atan(dy / dx) / M_PI;

    elastic_line();
    if (manhattan_mode) {
	if (mountain_mode) {
	    if (angle < -67.5)
		angle90_line(x, y);
	    else if (angle < -22.5)
		angle135_line(x, y);
	    else if (angle < 22.5)
		angle0_line(x, y);
	    else if (angle < 67.5)
		angle45_line(x, y);
	    else
		angle90_line(x, y);
	} else {
	    if (angle < -45.0)
		angle90_line(x, y);
	    else if (angle < 45.0)
		angle0_line(x, y);
	    else
		angle90_line(x, y);
	}
    } else {
	if (angle < 0.0)
	    angle135_line(x, y);
	else
	    angle45_line(x, y);
    }
    length_msg(MSG_LENGTH);
    elastic_line();
}

static void
angle0_line(int x, int y)
{
    cur_x = x;
    cur_y = fix_y;
}

static void
angle90_line(int x, int y)
{
    cur_y = y;
    cur_x = fix_x;
}

static void
angle45_line(int x, int y)
{
    if (abs(x - fix_x) < abs(y - fix_y)) {
	cur_x = fix_x - y + fix_y;
	cur_y = y;
    } else {
	cur_y = fix_y + fix_x - x;
	cur_x = x;
    }
}

static void
angle135_line(int x, int y)
{
    if (abs(x - fix_x) < abs(y - fix_y)) {
	cur_x = fix_x + y - fix_y;
	cur_y = y;
    } else {
	cur_y = fix_y + x - fix_x;
	cur_x = x;
    }
}

void
reshaping_line(int x, int y)
{
    elastic_linelink();
    adjust_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    /* one or two lines moving with the move point? */
    if (left_point != NULL && right_point != NULL) {
	length_msg2(left_point->x,left_point->y,
		    right_point->x,right_point->y,cur_x,cur_y);
    } else if (left_point != NULL) {
	altlength_msg(MSG_LENGTH,left_point->x,left_point->y);
    } else if (right_point != NULL) {
	altlength_msg(MSG_LENGTH,right_point->x,right_point->y);
    }
    elastic_linelink();
}

void
elastic_linelink(void)
{
    if (left_point != NULL) {
	pw_vector(canvas_win, left_point->x, left_point->y,
	       cur_x, cur_y, INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    }
    if (right_point != NULL) {
	pw_vector(canvas_win, right_point->x, right_point->y,
	       cur_x, cur_y, INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    }
}

void
moving_line(int x, int y)
{
    elastic_moveline(new_l->points);
    adjust_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    length_msg(MSG_DIST);
    elastic_moveline(new_l->points);
}

void
elastic_movenewline(void)
{
    elastic_moveline(new_l->points);
}

void
elastic_moveline(F_point *pts)
{
    F_point	   *p;
    int		    dx, dy, x, y, xx, yy;

    p = pts;
    if (p->next == NULL) {	/* dot */
	pw_vector(canvas_win, cur_x, cur_y, cur_x, cur_y,
		  INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    } else {
	dx = cur_x - fix_x;
	dy = cur_y - fix_y;
	x = p->x + dx;
	y = p->y + dy;
	for (p = p->next; p != NULL; x = xx, y = yy, p = p->next) {
	    xx = p->x + dx;
	    yy = p->y + dy;
	    pw_vector(canvas_win, x, y, xx, yy, INV_PAINT, 1,
		      RUBBER_LINE, 0.0, DEFAULT);
	    /* erase endpoint of last segment because next will redraw it */
	    if (p->next)
		pw_vector(canvas_win, xx, yy, xx, yy, INV_PAINT, 1,
		      RUBBER_LINE, 0.0, DEFAULT);
	}
	elastic_links(dx, dy, 1.0, 1.0);
    }
}

static void
elastic_links(int dx, int dy, float sx, float sy)
{
    F_linkinfo	   *k;

    if (cur_linkmode == SMART_OFF)
	return;

    for (k = cur_links; k != NULL; k = k->next)
	if (k->prevpt == NULL) {/* dot */
	    pw_vector(canvas_win, k->endpt->x, k->endpt->y,
		      k->endpt->x, k->endpt->y,
		      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	} else {
	    if (cur_linkmode == SMART_MOVE)
		pw_vector(canvas_win, k->endpt->x + dx, k->endpt->y + dy,
			  k->prevpt->x, k->prevpt->y,
			  INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	    else if (cur_linkmode == SMART_SLIDE) {
		if (k->endpt->x == k->prevpt->x) {
		    if (!k->two_pts)
			pw_vector(canvas_win, k->prevpt->x,
			      k->prevpt->y, k->prevpt->x + dx, k->prevpt->y,
			     INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
		    pw_vector(canvas_win, k->endpt->x + dx,
			  k->endpt->y + dy, k->prevpt->x + dx, k->prevpt->y,
			      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
		} else {
		    if (!k->two_pts)
			pw_vector(canvas_win, k->prevpt->x,
			      k->prevpt->y, k->prevpt->x, k->prevpt->y + dy,
			     INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
		    pw_vector(canvas_win, k->endpt->x + dx,
			  k->endpt->y + dy, k->prevpt->x, k->prevpt->y + dy,
			      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
		}
	    }
	}
}

void
scaling_line(int x, int y)
{
    elastic_scalepts(cur_l->points);
    adjust_box_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    if (cur_l->type == T_BOX || cur_l->type == T_ARCBOX || cur_l->type == T_PICTURE)
	boxsize_msg(2);
    elastic_scalepts(cur_l->points);
}

void
elastic_scale_curline(int x, int y)
{
    elastic_scalepts(cur_l->points);
}

void
scaling_spline(int x, int y)
{
    elastic_scalepts(cur_s->points);
    adjust_box_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    elastic_scalepts(cur_s->points);
}

void
elastic_scale_curspline(void)
{
    elastic_scalepts(cur_s->points);
}

void
elastic_scalepts(F_point *pts)
{
    F_point	   *p;
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;
    int		    ox, oy, xx, yy;

    p = pts;
    newx = cur_x - fix_x;
    newy = cur_y - fix_y;
    newd = sqrt(newx * newx + newy * newy);

    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);

    scalefact = newd / oldd;
    ox = fix_x + round((p->x - fix_x) * scalefact);
    oy = fix_y + round((p->y - fix_y) * scalefact);
    for (p = p->next; p != NULL; ox = xx, oy = yy, p = p->next) {
	xx = fix_x + round((p->x - fix_x) * scalefact);
	yy = fix_y + round((p->y - fix_y) * scalefact);
	pw_vector(canvas_win, ox, oy, xx, yy, INV_PAINT, 1,
		  RUBBER_LINE, 0.0, DEFAULT);
    }
}

void
elastic_poly(int x1, int y1, int x2, int y2, int numsides)
{
    register float  angle;
    register int    nx, ny, i; 
    double	    dx, dy;
    double	    init_angle, mag;
    int		    ox, oy;

    dx = x2 - x1;
    dy = y2 - y1;
    mag = sqrt(dx * dx + dy * dy);
    init_angle = compute_angle(dx, dy);
    ox = x2;
    oy = y2;

    /* now append numsides points */
    for (i = 1; i < numsides; i++) {
	angle = (float)(init_angle - M_2PI * (double) i / (double) numsides);
	if (angle < 0)
	    angle += M_2PI;
	nx = x1 + round(mag * cos((double) angle));
	ny = y1 + round(mag * sin((double) angle));
	pw_vector(canvas_win, nx, ny, ox, oy, INV_PAINT, 1,
		  RUBBER_LINE, 0.0, DEFAULT);
	ox = nx;
	oy = ny;
    }
    pw_vector(canvas_win, ox, oy, x2, y2, INV_PAINT, 1,
	      RUBBER_LINE, 0.0, DEFAULT);
}

void
resizing_poly(int x, int y)
{
    elastic_poly(fix_x, fix_y, cur_x, cur_y, work_numsides);
    cur_x = x;
    cur_y = y;
    work_numsides = cur_numsides;
    length_msg(MSG_LENGTH);
    elastic_poly(fix_x, fix_y, cur_x, cur_y, work_numsides);
}

/*********************** ELLIPSES *************************/

void
elastic_ebr(void)
{
    register int    x1, y1, x2, y2;
    int		    rx, ry;

    rx = cur_x - fix_x;
    ry = cur_y - fix_y;
    if (cur_angle != 0.0) {
	angle_ellipse(fix_x, fix_y, rx, ry, cur_angle, INV_PAINT, MAX_DEPTH+1, 1, 
	     RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT);
    } else {
	x1 = fix_x + rx;
	x2 = fix_x - rx;
	y1 = fix_y + ry;
	y2 = fix_y - ry;
	pw_curve(canvas_win, x1, y1, x2, y2, INV_PAINT, MAX_DEPTH+1, 1,
	     RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);
    }
}

void
resizing_ebr(int x, int y)
{
    elastic_ebr();
    cur_x = x;
    cur_y = y;
    length_msg(MSG_RADIUS2);
    elastic_ebr();
}

void
constrained_resizing_ebr(int x, int y)
{
    elastic_ebr();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    length_msg(MSG_RADIUS2);
    elastic_ebr();
}

void
elastic_ebd(void)
{
    int	    centx,centy;

    centx = (fix_x+cur_x)/2;
    centy = (fix_y+cur_y)/2;
    length_msg(MSG_DIAM);
    if (cur_angle != 0.0) {
	angle_ellipse(centx, centy, abs(cur_x-fix_x)/2, 
		  abs(cur_y-fix_y)/2, cur_angle,
		  INV_PAINT, MAX_DEPTH+1, 1, RUBBER_LINE, 0.0, UNFILLED,
		  DEFAULT, DEFAULT);
    } else {
	pw_curve(canvas_win, fix_x, fix_y, cur_x, cur_y, INV_PAINT, MAX_DEPTH+1, 1,
			RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);
    }
}

void
resizing_ebd(int x, int y)
{
    elastic_ebd();
    cur_x = x;
    cur_y = y;
    length_msg(MSG_DIAM);
    elastic_ebd();
}

void
constrained_resizing_ebd(int x, int y)
{
    elastic_ebd();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    length_msg(MSG_DIAM);
    elastic_ebd();
}

void
elastic_cbr(void)
{
    register int    radius, x1, y1, x2, y2;
    double	    rx, ry;

    rx = cur_x - fix_x;
    ry = cur_y - fix_y;
    radius = round(sqrt(rx * rx + ry * ry));
    x1 = fix_x + radius;
    x2 = fix_x - radius;
    y1 = fix_y + radius;
    y2 = fix_y - radius;
    pw_curve(canvas_win, x1, y1, x2, y2, INV_PAINT, MAX_DEPTH+1, 1,
	     RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);
}

void
resizing_cbr(int x, int y)
{
    elastic_cbr();
    cur_x = x;
    cur_y = y;
    length_msg(MSG_RADIUS);
    elastic_cbr();
}

void
elastic_cbd(void)
{
    register int    x1, y1, x2, y2;
    int		    radius;
    double	    rx, ry;

    rx = (cur_x - fix_x) / 2;
    ry = (cur_y - fix_y) / 2;
    radius = round(sqrt(rx * rx + ry * ry));
    x1 = fix_x + rx + radius;
    x2 = fix_x + rx - radius;
    y1 = fix_y + ry + radius;
    y2 = fix_y + ry - radius;
    pw_curve(canvas_win, x1, y1, x2, y2, INV_PAINT, MAX_DEPTH+1, 1,
	     RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);
}

void
resizing_cbd(int x, int y)
{
    elastic_cbd();
    cur_x = x;
    cur_y = y;
    length_msg(MSG_DIAM);
    elastic_cbd();
}

void
constrained_resizing_cbd(int x, int y)
{
    elastic_cbd();
    adjust_box_pos(x, y, from_x, from_y, &cur_x, &cur_y);
    length_msg(MSG_DIAM);
    elastic_cbd();
}

void
elastic_moveellipse(void)
{
    register int    x1, y1, x2, y2;

    x1 = cur_x + x1off;
    x2 = cur_x + x2off;
    y1 = cur_y + y1off;
    y2 = cur_y + y2off;
    if (cur_angle != 0.0) {
	angle_ellipse((x1+x2)/2, (y1+y2)/2, abs(x1-x2)/2, abs(y1-y2)/2, cur_angle,
		  INV_PAINT, MAX_DEPTH+1, 1, RUBBER_LINE, 0.0, UNFILLED,
		  DEFAULT, DEFAULT);
    } else {
	pw_curve(canvas_win, x1, y1, x2, y2, INV_PAINT, MAX_DEPTH+1, 1,
	     RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);
    }
}

void
moving_ellipse(int x, int y)
{
    elastic_moveellipse();
    adjust_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    length_msg(MSG_DIST);
    elastic_moveellipse();
}

void
elastic_scaleellipse(F_ellipse *e)
{
    register int    x1, y1, x2, y2;
    int		    rx, ry;
    double	    newx, newy, oldx, oldy;
    float	    newd, oldd, scalefact;

    newx = cur_x - fix_x;
    newy = cur_y - fix_y;
    newd = sqrt(newx * newx + newy * newy);

    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);

    scalefact = newd / oldd;

    rx = round(e->radiuses.x * scalefact);
    ry = round(e->radiuses.y * scalefact);
    if (cur_angle != 0.0) {
	angle_ellipse(e->center.x, e->center.y, rx, ry, cur_angle,
		  INV_PAINT, MAX_DEPTH+1, 1, RUBBER_LINE, 0.0, UNFILLED,
		  DEFAULT, DEFAULT);
    } else {
	x1 = fix_x + rx;
	x2 = fix_x - rx;
	y1 = fix_y + ry;
	y2 = fix_y - ry;
	pw_curve(canvas_win, x1, y1, x2, y2, INV_PAINT, MAX_DEPTH+1, 1,
	     RUBBER_LINE, 0.0, UNFILLED, DEFAULT, DEFAULT, CAP_BUTT);
    }
}

void
scaling_ellipse(int x, int y)
{
    elastic_scaleellipse(cur_e);
    adjust_box_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    if (cur_e->type == T_ELLIPSE_BY_RAD)
	length_msg(MSG_RADIUS2);
    else if (cur_e->type == T_CIRCLE_BY_RAD)
	length_msg(MSG_RADIUS);
    else
	length_msg(MSG_DIAM);
    elastic_scaleellipse(cur_e);
}

void
elastic_scale_curellipse(void)
{
    elastic_scaleellipse(cur_e);
}

/*************************** ARCS *************************/

void
arc_point(int x, int y, int numpoint)
{
    elastic_line();
    cur_x = x;
    cur_y = y;
    altlength_msg(MSG_RADIUS_ANGLE, center_point.x, center_point.y);
    elastic_line();
}

void
reshaping_arc(int x, int y)
{
    elastic_arclink();
    adjust_pos(x, y, cur_a->point[movedpoint_num].x,
	       cur_a->point[movedpoint_num].y, &cur_x, &cur_y);
    if (movedpoint_num == 1) {
	/* middle point */
        arc_msg(cur_a->point[0].x, cur_a->point[0].y,
		 cur_a->point[2].x, cur_a->point[2].y,
		 cur_x, cur_y);
    } else {
	/* end point */
	altlength_msg(MSG_LENGTH,cur_a->point[1].x,cur_a->point[1].y);
    }
    elastic_arclink();
}

void
elastic_arclink(void)
{
    switch (movedpoint_num) {
    case 0:
	pw_vector(canvas_win, cur_x, cur_y,
		  cur_a->point[1].x, cur_a->point[1].y,
		  INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	break;
    case 1:
	pw_vector(canvas_win, cur_a->point[0].x, cur_a->point[0].y,
		  cur_x, cur_y, INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	pw_vector(canvas_win, cur_a->point[2].x, cur_a->point[2].y,
		  cur_x, cur_y, INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	break;
    default:
	pw_vector(canvas_win, cur_a->point[1].x, cur_a->point[1].y,
		  cur_x, cur_y, INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    }
}

void
moving_arc(int x, int y)
{
    elastic_movearc(new_a);
    adjust_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    length_msg(MSG_DIST);
    elastic_movearc(new_a);
}

void
elastic_movenewarc(void)
{
    elastic_movearc(new_a);
}

void
elastic_movearc(F_arc *a)
{
    int		    dx, dy;

    dx = cur_x - fix_x;
    dy = cur_y - fix_y;
    pw_vector(canvas_win, a->point[0].x + dx, a->point[0].y + dy,
	      a->point[1].x + dx, a->point[1].y + dy,
	      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    pw_vector(canvas_win, a->point[1].x + dx, a->point[1].y + dy,
	      a->point[2].x + dx, a->point[2].y + dy,
	      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
}

void
scaling_arc(int x, int y)
{
    elastic_scalearc(cur_a);
    adjust_box_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    elastic_scalearc(cur_a);
}

void
elastic_scale_curarc(void)
{ 
    elastic_scalearc(cur_a);
}

void
elastic_scalearc(F_arc *a)
{
    double	    newx, newy, oldx, oldy;
    double	    newd, oldd, scalefact;
    F_pos	    p0, p1, p2;

    newx = cur_x - fix_x;
    newy = cur_y - fix_y;
    newd = sqrt(newx * newx + newy * newy);

    oldx = from_x - fix_x;
    oldy = from_y - fix_y;
    oldd = sqrt(oldx * oldx + oldy * oldy);

    scalefact = newd / oldd;

    p0 = a->point[0];
    p1 = a->point[1];
    p2 = a->point[2];
    p0.x = fix_x + round((p0.x - fix_x) * scalefact);
    p0.y = fix_y + round((p0.y - fix_y) * scalefact);
    p1.x = fix_x + round((p1.x - fix_x) * scalefact);
    p1.y = fix_y + round((p1.y - fix_y) * scalefact);
    p2.x = fix_x + round((p2.x - fix_x) * scalefact);
    p2.y = fix_y + round((p2.y - fix_y) * scalefact);

    length_msg2(p0.x, p0.y, p2.x, p2.y, p1.x, p1.y);
    pw_vector(canvas_win, p0.x, p0.y, p1.x, p1.y,
	      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
    pw_vector(canvas_win, p1.x, p1.y, p2.x, p2.y,
	      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
}

/*************************** TEXT *************************/

void
moving_text(int x, int y)
{
    elastic_movetext();
    adjust_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    length_msg(MSG_DIST);
    elastic_movetext();
}

/* use x1off, y1off so that the beginning of the text isn't
   shifted under the cursor */

void
elastic_movetext(void)
{
    pw_text(canvas_win, cur_x + x1off, cur_y + y1off, INV_PAINT, MAX_DEPTH+1,
	    new_t->fontstruct, new_t->angle, 
	    new_t->cstring, new_t->color, COLOR_NONE);
}


/*************************** SPLINES *************************/

void
moving_spline(int x, int y)
{
    elastic_moveline(new_s->points);
    adjust_pos(x, y, fix_x, fix_y, &cur_x, &cur_y);
    length_msg(MSG_DIST);
    elastic_moveline(new_s->points);
}

void
elastic_movenewspline(void)
{
    elastic_moveline(new_s->points);
}

/*********** AUXILIARY FUNCTIONS FOR CONSTRAINED MOVES ******************/

void
adjust_box_pos(int curs_x, int curs_y, int orig_x, int orig_y, int *ret_x, int *ret_y)
{
    int		    xx, sgn_csr2fix_x, yy, sgn_csr2fix_y;
    double	    mag_csr2fix_x, mag_csr2fix_y;

    switch (constrained) {
    case MOVE_ARB:
	*ret_x = curs_x;
	*ret_y = curs_y;
	break;
    case BOX_HSTRETCH:
	*ret_x = curs_x;
	*ret_y = orig_y;
	break;
    case BOX_VSTRETCH:
	*ret_x = orig_x;
	*ret_y = curs_y;
	break;
    default:
	/* calculate where scaled and stretched box corners would be */
	xx = curs_x - fix_x;
	sgn_csr2fix_x = signof(xx);
	mag_csr2fix_x = (double) abs(xx);

	yy = curs_y - fix_y;
	sgn_csr2fix_y = signof(yy);
	mag_csr2fix_y = (double) abs(yy);

	if (mag_csr2fix_x * sina > mag_csr2fix_y * cosa) {	/* above diagonal */
	    *ret_x = curs_x;
	    if (constrained == BOX_SCALE) {
		if (cosa == 0.0) {
		    *ret_y = fix_y + sgn_csr2fix_y * (int) (mag_csr2fix_x);
		} else {
		    *ret_y = fix_y + sgn_csr2fix_y * (int) (mag_csr2fix_x * sina / cosa);
		}
	    } else {
		    *ret_y = fix_y + sgn_csr2fix_y * abs(fix_y - orig_y);
		 }
	} else {
	    *ret_y = curs_y;
	    if (constrained == BOX_SCALE) {
		if (sina == 0.0) {
		    *ret_x = fix_x + sgn_csr2fix_x * (int) (mag_csr2fix_y);
		} else {
		    *ret_x = fix_x + sgn_csr2fix_x * (int) (mag_csr2fix_y * cosa / sina);
		}
	    } else {
		*ret_x = fix_x + sgn_csr2fix_x * abs(fix_x - orig_x);
	    }
	}
    } /* switch */
}

void
adjust_pos(int curs_x, int curs_y, int orig_x, int orig_y, int *ret_x, int *ret_y)
{
    if (constrained) {
	if (abs(orig_x - curs_x) > abs(orig_y - curs_y)) {
	    *ret_x = curs_x;
	    *ret_y = orig_y;
	} else {
	    *ret_x = orig_x;
	    *ret_y = curs_y;
	}
    } else {
	*ret_x = curs_x;
	*ret_y = curs_y;
    }
}
