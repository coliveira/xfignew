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
#include "d_arcbox.h"
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_cursor.h"
#include "w_msgpanel.h"
#include "w_mousefun.h"

#include "f_util.h"
#include "u_redraw.h"
} 

/*************************** local procedures *********************/

static void	create_arc_boxobject(int x, int y);
static void	cancel_arc_boxobject(int, int);
static void	init_arc_box_drawing(int x, int y);



void arcbox_drawing_selected()
{
    set_mousefun("corner point", "", "", "", "", "");
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = (FCallBack)init_arc_box_drawing;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)null_proc;
    set_cursor(crosshair_cursor);
    reset_action_on();
}

static void
init_arc_box_drawing(int x, int y)
{
    cur_x = fix_x = x;
    cur_y = fix_y = y;
    set_mousefun("final point", "", "cancel", "", "", "");
    draw_mousefun_canvas();
    canvas_locmove_proc = resizing_box;
    canvas_leftbut_proc = (FCallBack)create_arc_boxobject;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)cancel_arc_boxobject;
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    set_cursor(null_cursor);
    set_action_on();
}

static void
cancel_arc_boxobject(int a, int b)
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    arcbox_drawing_selected();
    draw_mousefun_canvas();
}

static void
create_arc_boxobject(int x, int y)
{
    F_line	   *box;
    F_point	   *point;

    elastic_box(fix_x, fix_y, cur_x, cur_y);
    /* erase last lengths if appres.showlengths is true */
    erase_box_lengths();

    if (fix_x == x || fix_y == y) {
	beep();
	put_msg("Arc box must have area");
	arcbox_drawing_selected();
	draw_mousefun_canvas();
	return;
    }

    if ((point = create_point()) == NULL)
	return;

    point->x = x;
    point->y = y;
    point->next = NULL;

    if ((box = create_line()) == NULL) {
	free((char *) point);
	return;
    }
    box->type = T_ARCBOX;
    box->style = cur_linestyle;
    box->thickness = cur_linewidth;
    box->pen_color = cur_pencolor;
    box->fill_color = cur_fillcolor;
    box->depth = cur_depth;
    box->pen_style = -1;
    box->join_style = cur_joinstyle;
    box->cap_style = cur_capstyle;
    box->fill_style = cur_fillstyle;
    /* multiply	 dash length by line thickness */
    box->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    box->radius = cur_boxradius;/* corner radius */
    box->points = point;
    append_point(x, fix_y, &point);
    append_point(fix_x, fix_y, &point);
    append_point(fix_x, y, &point);
    append_point(x, y, &point);
    add_line(box);
    reset_action_on(); /* this signals redisplay_curobj() not to refresh */
    /* draw it and anything on top of it */
    redisplay_line(box);
    arcbox_drawing_selected();
    draw_mousefun_canvas();
}
