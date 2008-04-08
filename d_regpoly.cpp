/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
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
#include "u_create.h"
#include "u_elastic.h"
#include "u_geom.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"

#include "u_redraw.h"
#include "w_cursor.h"
#include "w_msgpanel.h"
void regpoly_drawing_selected(void);

}
/*************************** local declarations *********************/

static void	init_regpoly_drawing(int x, int y);
static void	create_regpoly(int x, int y);
static void	cancel_regpoly(int,int);



void
regpoly_drawing_selected(void)
{
    set_mousefun("center point", "", "", "", "", "");
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = (FCallBack)init_regpoly_drawing;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)null_proc;
    set_cursor(crosshair_cursor);
    reset_action_on();
}

static void
init_regpoly_drawing(int x, int y)
{
    cur_x = fix_x = x;
    cur_y = fix_y = y;
    work_numsides = cur_numsides;
    set_mousefun("final point", "", "cancel", "", "", "");
    draw_mousefun_canvas();
    canvas_locmove_proc = resizing_poly;
    canvas_leftbut_proc = (FCallBack)create_regpoly;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)cancel_regpoly;
    elastic_poly(fix_x, fix_y, cur_x, cur_y, work_numsides);
    set_cursor(null_cursor);
    set_action_on();
}

static void
cancel_regpoly(int a ,int b)
{
    elastic_poly(fix_x, fix_y, cur_x, cur_y, work_numsides);
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    regpoly_drawing_selected();
    draw_mousefun_canvas();
}

static void
create_regpoly(int x, int y)
{
    register float  angle;
    register int    nx, ny, i;
    double	    dx, dy;
    double	    init_angle, mag;
    F_line	   *poly;
    F_point	   *point;

    elastic_poly(fix_x, fix_y, cur_x, cur_y, work_numsides);
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    if (fix_x == x && fix_y == y)
	return;			/* 0 size */

    if ((point = create_point()) == NULL)
	return;

    point->x = x;
    point->y = y;
    point->next = NULL;

    if ((poly = create_line()) == NULL) {
	free((char *) point);
	return;
    }
    poly->type = T_POLYGON;
    poly->style = cur_linestyle;
    poly->thickness = cur_linewidth;
    poly->pen_color = cur_pencolor;
    poly->fill_color = cur_fillcolor;
    poly->depth = cur_depth;
    poly->pen_style = -1;
    poly->join_style = cur_joinstyle;
    poly->cap_style = cur_capstyle;
    poly->fill_style = cur_fillstyle;
    /* scale dash length by line thickness */
    poly->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    poly->radius = 0;
    poly->points = point;

    dx = x - fix_x;
    dy = y - fix_y;
    mag = sqrt(dx * dx + dy * dy);
    init_angle = compute_angle(dx, dy);

    /* now append cur_numsides points */
    for (i = 1; i < cur_numsides; i++) {
	angle = init_angle - M_2PI * (double) i / (double) cur_numsides;
	if (angle < 0)
	    angle += M_2PI;
	nx = fix_x + round(mag * cos(angle));
	ny = fix_y + round(mag * sin(angle));
	append_point(nx, ny, &point);
    }
    append_point(x, y, &point);

    add_line(poly);
    /* draw it and anything on top of it */
    redisplay_line(poly);
    regpoly_drawing_selected();
    draw_mousefun_canvas();
}
