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
 */

/********************** DECLARATIONS ********************/

/* IMPORTS */

extern "C" {
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "d_arc.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_cursor.h"
#include "w_drawprim.h"
#include "w_msgpanel.h"
#include "w_mousefun.h"
#include "u_geom.h"

#include "f_util.h"
#include "u_draw.h"
#include "u_free.h"
#include "u_markers.h"
#include "u_redraw.h"
}
/* EXPORT */

F_pos		point[3];
F_pos		center_point;
Boolean		center_marked;

/* LOCAL */

static void	create_arcobject(int lx, int ly);
static void	get_arcpoint(int x, int y);
static void	init_arc_drawing(int x, int y);
static void	cancel_arc(int, int);
static void	init_arc_c_drawing(int x, int y);
static void	resizing_arc(int x, int y);

static F_arc	*tmparc = 0;
static F_pos	tpoint[3];

static Boolean save_shownums;


void arc_drawing_selected(void)
{
    center_marked = FALSE;
    set_mousefun("first point", "center point", "", "", "", "");
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc   = (FCallBack)init_arc_drawing;
    canvas_middlebut_proc = (FCallBack)init_arc_c_drawing;
    canvas_rightbut_proc  = (FCallBack)null_proc;
    set_cursor(crosshair_cursor);
    reset_action_on();
}

static void
init_arc_drawing(int x, int y)
{
    int		i;
    if (center_marked) {
	/* erase the circle */
	elastic_cbr();
	/* save and turn off showing vertex numbers */
	save_shownums   = appres.shownums;
	appres.shownums = False;
	set_mousefun("direction", "", "cancel", "", "", "");
	tmparc = create_arc();
	tmparc->thickness = 1;
	tmparc->center.x = center_point.x;
	tmparc->center.y = center_point.y;
	/* initialize points in elastic arc */
	for (i=0; i<3; i++) {
	    tpoint[i].x = x;
	    tpoint[i].y = y;
	}
	/* start the arc */
	elastic_arc();
	canvas_locmove_proc = resizing_arc;
    } else {
	set_mousefun("mid point", "", "cancel", "", "", "");
    }
    draw_mousefun_canvas();
    canvas_rightbut_proc = (FCallBack)cancel_arc;
    point[0].x = fix_x = cur_x = x;
    point[0].y = fix_y = cur_y = y;
    num_point = 1;
    if (!center_marked) {
	canvas_locmove_proc = unconstrained_line;
	elastic_line();
    }
    canvas_leftbut_proc = (FCallBack)get_arcpoint;
    canvas_middlebut_proc = (FCallBack)null_proc;
    set_cursor(null_cursor);
    set_action_on();
}

static void
init_arc_c_drawing(int x, int y)
{
    set_mousefun("first point", "", "cancel", "", "", "");
    draw_mousefun_canvas();
    canvas_locmove_proc = resizing_cbr;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)cancel_arc;
    center_point.x = fix_x = cur_x = x;
    center_point.y = fix_y = cur_y = y;
    center_marked = TRUE;
    center_marker(center_point.x, center_point.y);
    set_action_on();
    num_point = 0;
}

static void
cancel_arc(int a, int b)
{
    if (center_marked) {
	/* erase circle */
	if (num_point == 0)
	    elastic_cbr();
	else
	    elastic_arc();
	/* free the temporary arc */
	if (tmparc) {
	    free_arc(&tmparc);
	    tmparc = (F_arc *) 0;
	}
	center_marked = FALSE;
	center_marker(center_point.x, center_point.y);  /* clear center marker */
	/* restore shownums */
	appres.shownums = save_shownums;
    } else {
	/* erase any length info if appres.showlengths is true */
	erase_lengths();
	elastic_line();
	if (num_point == 2) {
	    /* erase initial part of line */
	    cur_x = point[0].x;
	    cur_y = point[0].y;
	    elastic_line();
	}
    }
    arc_drawing_selected();
    draw_mousefun_canvas();
}

static void
get_arcpoint(int x, int y)
{
    if (x == fix_x && y == fix_y)
    	return;

    if (num_point == 1) {
	if (center_marked) {
  	    set_mousefun("final angle", "", "cancel", "", "", "");
	} else {
	    set_mousefun("final point", "", "cancel", "", "", "");
	}
	draw_mousefun_canvas();
	canvas_leftbut_proc = (FCallBack)create_arcobject;
    }
    if (!center_marked)
	elastic_line();
    cur_x = x;
    cur_y = y;
    if (!center_marked)
	elastic_line();
    point[num_point].x = fix_x = x;
    point[num_point++].y = fix_y = y;
    if (!center_marked)
	elastic_line();
}


void SetArcValues(F_arc *arc, float xx, float yy)
{
    arc->center.x = xx;
    arc->center.y = yy;
    arc->point[0].x = point[0].x;
    arc->point[0].y = point[0].y;
    arc->point[1].x = point[1].x;
    arc->point[1].y = point[1].y;
    arc->point[2].x = point[2].x;
    arc->point[2].y = point[2].y;
    arc->next = NULL;
}

static void
create_arcobject(int lx, int ly)
{
    F_arc	   *arc;
    int		    x, y;
    float	    xx, yy;

    /* erase last line segment */
    if (!center_marked)
	elastic_line();
    else /* restore shownums */
	appres.shownums = save_shownums;
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    point[num_point].x = lx;
    point[num_point++].y = ly;
    x = point[0].x;
    y = point[0].y;
    if (center_marked) {
	double theta, r;

	/* erase the center marker */
	center_marker(center_point.x, center_point.y);

	/* free the temporary arc */
	free_arc(&tmparc);

	/* recompute the second and last points to put them on the arc */
	r = sqrt((double)(point[0].x - center_point.x) * (point[0].x - center_point.x)
	       + (double)(point[0].y - center_point.y) * (point[0].y - center_point.y));

	theta = compute_angle((double)(point[1].x - center_point.x),
			    (double)(point[1].y - center_point.y));
	point[1].x = center_point.x + r * cos(theta);
	point[1].y = center_point.y + r * sin(theta);
      
	theta = compute_angle((double)(point[2].x - center_point.x),
			    (double)(point[2].y - center_point.y));
	point[2].x = center_point.x + r * cos(theta);
	point[2].y = center_point.y + r * sin(theta);
    } else {
	/* erase previous line segment */
	pw_vector(canvas_win, x, y, point[1].x, point[1].y, INV_PAINT,
		  1, RUBBER_LINE, 0.0, DEFAULT);
    }

    if (!compute_arccenter(point[0], point[1], point[2], &xx, &yy)) {
	put_msg("Invalid ARC geometry");
	beep();
	arc_drawing_selected();
	draw_mousefun_canvas();
	return;
    }
    if ((arc = create_arc()) == NULL) {
	arc_drawing_selected();
	draw_mousefun_canvas();
	return;
    }
    arc->type = cur_arctype;
    arc->style = cur_linestyle;
    arc->thickness = cur_linewidth;
    /* scale dash length according to linethickness */
    arc->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    arc->pen_style = -1;
    arc->fill_style = cur_fillstyle;
    arc->pen_color = cur_pencolor;
    arc->fill_color = cur_fillcolor;
    arc->cap_style = cur_capstyle;
    arc->depth = cur_depth;
    arc->direction = compute_direction(point[0], point[1], point[2]);
    /* only allow arrowheads for open arc */
    if (arc->type == T_PIE_WEDGE_ARC) {
	arc->for_arrow = NULL;
	arc->back_arrow = NULL;
    } else {
	if (autoforwardarrow_mode)
	    arc->for_arrow = forward_arrow();
	else
	    arc->for_arrow = NULL;
	if (autobackwardarrow_mode)
	    arc->back_arrow = backward_arrow();
	else
	    arc->back_arrow = NULL;
    }
    SetArcValues(arc, xx, yy);
    add_arc(arc);
    reset_action_on(); /* this signals redisplay_curobj() not to refresh */
    /* draw it and anything on top of it */
    redisplay_arc(arc);
    arc_drawing_selected();
    draw_mousefun_canvas();
}

static void
resizing_arc(int x, int y)
{
    F_point	p3;
    double	alpha;

    elastic_arc();
    cur_x = x;
    cur_y = y;
    p3.x = x;
    p3.y = y;
    if (num_point == 0)
	length_msg(MSG_RADIUS);
    else {
	compute_3p_angle((F_point*)&tpoint[0], (F_point*)&center_point, &p3, &alpha);
	put_msg("1st angle = %.2f degrees", (float) alpha*180.0/M_PI);
    }
    elastic_arc();
}

void
elastic_arc(void)
{
	register int i;
	register double	r, theta1, theta2;

	tpoint[2].x = cur_x;
	tpoint[2].y = cur_y;
	/* if user hasn't entered second point yet, make it average of endpoints */
	if (num_point <= 1) {
	    tpoint[1].x = (tpoint[0].x + cur_x)/2;
	    tpoint[1].y = (tpoint[0].y + cur_y)/2;
	}

	if (tpoint[0].x == tpoint[2].x && tpoint[0].y == tpoint[2].y)
		return;

	r = sqrt((double)(tpoint[0].x - center_point.x) * (tpoint[0].x - center_point.x)
	       + (double)(tpoint[0].y - center_point.y) * (tpoint[0].y - center_point.y));

	theta1 = compute_angle((double)(tpoint[1].x - center_point.x),
			    (double)(tpoint[1].y - center_point.y));
	tpoint[1].x = center_point.x + r * cos(theta1);
	tpoint[1].y = center_point.y + r * sin(theta1);
      
	theta2 = compute_angle((double)(tpoint[2].x - center_point.x),
			    (double)(tpoint[2].y - center_point.y));
	tpoint[2].x = center_point.x + r * cos(theta2);
	tpoint[2].y = center_point.y + r * sin(theta2);

	for (i=0; i<3; i++)
	    tmparc->point[i] = tpoint[i];
	tmparc->direction = compute_direction(tpoint[0], tpoint[1], tpoint[2]);

	draw_arc(tmparc, INV_PAINT);
}
