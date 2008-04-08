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
#include "d_line.h"
#include "e_edit.h"
#include "e_scale.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_setup.h"

#include "u_free.h"
#include "u_redraw.h"
#include "w_cursor.h"
#include "w_drawprim.h"
#include "w_msgpanel.h"
}
Boolean	freehand_line;
Boolean	dimension_line;


/* LOCAL */

static void	    init_line_drawing(int x, int y, int shift);
static void	    init_line_freehand_drawing(int x, int y);
typedef void (*func_pt)(int, int);

/**********************	 polyline and polygon section  **********************/



void
line_drawing_selected(void)
{
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = (FCallBack)init_line_drawing;
    canvas_middlebut_proc = (FCallBack)init_line_freehand_drawing;
    set_cursor(crosshair_cursor);
    reset_action_on();
    if (cur_mode == F_POLYGON) {
	set_mousefun("first point", "freehand", "", "", "", "");
	min_num_points = 3;
	canvas_rightbut_proc = (FCallBack)null_proc;
    } else {
	set_mousefun("first point", "freehand", "single point", "dimension line", "", "");
	min_num_points = 1;
	num_point = 0;
	fix_x = fix_y = -1;
	canvas_rightbut_proc = (FCallBack)create_lineobject;
    }
}

static void
init_line_freehand_drawing(int x, int y)
{
    freehand_line = True;
    /* not a dimension line */
    dimension_line = False;
    init_trace_drawing(x, y);
}

static void
init_line_drawing(int x, int y, int shift)
{
    freehand_line = False;
    /* if the user pressed shift then make a dimension line */
    dimension_line = shift;
    if (shift) {
	min_num_points = 2;
    }
    canvas_middlebut_proc = (FCallBack)null_proc;
    init_trace_drawing(x, y);
}

void
cancel_line_drawing(int a, int b)
{
    elastic_line();
    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    cur_x = fix_x;
    cur_y = fix_y;
    if (cur_point != first_point)
	elastic_moveline(first_point);	/* erase control vector */
    free_points(first_point);
    first_point = NULL;
    return_proc();
    draw_mousefun_canvas();
}

void
init_trace_drawing(int x, int y)
{
    if ((first_point = create_point()) == NULL)
	return;

    cur_point = first_point;
    set_action_on();
    cur_point->x = fix_x = cur_x = x;
    cur_point->y = fix_y = cur_y = y;
    cur_point->next = NULL;
    canvas_leftbut_proc = (FCallBack)get_intermediatepoint;
    if (freehand_line) {
	canvas_locmove_proc = freehand_get_intermediatepoint;
    } else {
	/* only two points in a dimension line */
	if (dimension_line)
	    canvas_leftbut_proc = (FCallBack)create_lineobject;
	if (latexline_mode || latexarrow_mode) {
	    canvas_locmove_proc = latex_line;
	} else if (manhattan_mode || mountain_mode) {
	    canvas_locmove_proc = constrainedangle_line;
	} else {
	    canvas_locmove_proc = unconstrained_line;
	}
    }
    canvas_middlebut_save = create_lineobject;
    canvas_rightbut_proc = (FCallBack)cancel_line_drawing;
    return_proc = line_drawing_selected;
    num_point = 1;
    set_mousefun("next point", "", "cancel", "del point", "", "");
    if (dimension_line) {
	set_mousefun("final point", "", "cancel", "del point", "", "");
	canvas_middlebut_proc = (FCallBack)null_proc;
    } else if (num_point >= min_num_points - 1) {
	set_mousefun("next point", "final point", "cancel", "del point", "", "");
	canvas_middlebut_proc = (FCallBack)canvas_middlebut_save;
    }
	
    draw_mousefun_canvas();
    set_cursor(null_cursor);
    elastic_line();
}

/* we have this extra proc to call get_intermediatepoint() because we come
   here from a canvas_locmove_proc which doesn't have a shift value (its
   not a keypress event)
*/

void
freehand_get_intermediatepoint(int x, int y)
{
    /* if shift key is pressed user wants to delete points with
       left button press, return now */
    if (shift) {
	unconstrained_line(x,y);
	return;
    }
    get_intermediatepoint(x, y, 0);
}
    
void
get_intermediatepoint(int x, int y, int shift)
{
    /* in freehand mode call unconstrained_line explicitely to move the mouse */
    if (freehand_line) {
	unconstrained_line(x,y);
	/* pointer must move by at least freehand_resolution in any direction */
	if (abs(fix_x-cur_x) < appres.freehand_resolution &&
	   (abs(fix_y-cur_y) < appres.freehand_resolution))
		return;
    } else {
	/* otherwise call the (possibly) constrained movement procedure */
	(*canvas_locmove_proc) (x, y);
    }

    /* don't allow coincident consecutive points */
    if (fix_x == cur_x && fix_y == cur_y)
	return;

    num_point++;
    fix_x = cur_x;
    fix_y = cur_y;
    elastic_line();
    if (cur_cursor != null_cursor) {
	set_cursor(null_cursor);
    }
    if (shift && num_point > 2) {
	F_point	*p;

	num_point -= 2;
	p = prev_point(first_point, cur_point);
	p->next = NULL;
	/* erase the newest segment */
	pw_vector(canvas_win, fix_x, fix_y, cur_point->x, cur_point->y,
		  INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	/* and segment drawn before */
	pw_vector(canvas_win, p->x, p->y, cur_point->x, cur_point->y,
		  INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	/* and draw new elastic segment */
	pw_vector(canvas_win, fix_x, fix_y, p->x, p->y,
		  PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	fix_x = p->x;
	fix_y = p->y;
	free_points(cur_point);
	cur_point = p;
    } else {
	append_point(fix_x, fix_y, &cur_point);
    }
    if (num_point == min_num_points - 1) {
	if (freehand_line)
	    set_mousefun("", "final point", "cancel", "del point", "", "");
	else
	    set_mousefun("next point", "final point", "cancel", "del point", "", "");
	draw_mousefun_canvas();
	canvas_middlebut_proc = (FCallBack)canvas_middlebut_save;
    }
}

/* come here upon pressing middle button (last point of lineobject) */
/* or the second point of a dimension line */

void
create_lineobject(int x, int y)
{
    F_line	   *line;
    F_compound	   *comp;
    int		    dot;

    if (num_point == 0) {
	if ((first_point = create_point()) == NULL) {
	    line_drawing_selected();
	    draw_mousefun_canvas();
	    return;
	}
	cur_point = first_point;
	first_point->x = fix_x = cur_x = x;
	first_point->y = fix_y = cur_y = y;
	first_point->next = NULL;
	num_point++;
    } else if (x != fix_x || y != fix_y) {
	get_intermediatepoint(x, y, 0);
    }
    /* dimension line must have 2 different points */
    if (dimension_line && first_point->x == x && first_point->y == y)
	return;

    dot = (num_point == 1);
    elastic_line();
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    if ((line = create_line()) == NULL) {
	line_drawing_selected();
	draw_mousefun_canvas();
	return;
    }
    line->type = T_POLYLINE;
    line->style = cur_linestyle;
    line->thickness = cur_linewidth;
    line->pen_color = cur_pencolor;
    line->fill_color = cur_fillcolor;
    line->depth = cur_depth;
    line->pen_style = -1;
    line->join_style = cur_joinstyle;
    line->cap_style = cur_capstyle;
    line->fill_style = cur_fillstyle;
    line->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    line->points = first_point;
    if (!dot) {
	if (cur_mode == F_POLYGON) {	/* close off polygon */
	    line->type = T_POLYGON;
	    num_point++;
	    append_point(first_point->x, first_point->y, &cur_point);
	    elastic_line();
	    fix_x = first_point->x;
	    fix_y = first_point->y;
	    elastic_line();	/* fix last elastic line */
	} else {		/* polyline; draw any arrows */
	    if (autoforwardarrow_mode && !dimension_line)
		line->for_arrow = forward_arrow();
	    /* arrow will be drawn in draw_line below */
	    if (autobackwardarrow_mode && !dimension_line)
		line->back_arrow = backward_arrow();
	    /* arrow will be drawn in draw_line below */
	}
	cur_x = fix_x;
	cur_y = fix_y;
	elastic_moveline(first_point);	/* erase temporary outline */
    }
    if (dimension_line) {
	comp = create_dimension_line(line, True);
	reset_action_on(); /* this signals redisplay_curobj() not to refresh */
	/* draw it and anything on top of it */
	redisplay_compound(comp);
    } else {
	add_line(line);
	reset_action_on(); /* this signals redisplay_curobj() not to refresh */
	/* draw it and anything on top of it */
	redisplay_line(line);
    }
    line_drawing_selected();
    if (!edit_remember_dimline_mode)
	draw_mousefun_canvas();
}

