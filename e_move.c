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
#include "u_drag.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_mousefun.h"

#include "mode.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "w_cursor.h"

static void	init_move(F_line *p, int type, int x, int y, int px, int py), init_arb_move(F_line *p, int type, int x, int y, int px, int py), init_constrained_move(F_line *p, int type, int x, int y, int px, int py);



void
move_selected(void)
{
    set_mousefun("move object", "horiz/vert move", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    init_searchproc_left(init_arb_move);
    init_searchproc_middle(init_constrained_move);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    return_proc = move_selected;
    set_cursor(pick9_cursor);
    reset_action_on();
}

static void
init_arb_move(F_line *p, int type, int x, int y, int px, int py)
{
    constrained = MOVE_ARB;
    init_move(p, type, x, y, px, py);
    canvas_middlebut_proc = null_proc;
    set_mousefun("place object", "", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
}

static void
init_constrained_move(F_line *p, int type, int x, int y, int px, int py)
{
    constrained = MOVE_HORIZ_VERT;
    init_move(p, type, x, y, px, py);
    canvas_middlebut_proc = canvas_leftbut_proc;
    canvas_leftbut_proc = null_proc;
    set_mousefun("", "place object", "cancel", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
}

static void
init_move(F_line *p, int type, int x, int y, int px, int py)
{
    /* turn off all markers */
    update_markers(0);
    switch (type) {
    case O_COMPOUND:
	set_cursor(wait_cursor);
	cur_c = (F_compound *) p;
	list_delete_compound(&objects.compounds, cur_c);
	redisplay_compound(cur_c);
	set_cursor(null_cursor);
	init_compounddragging(cur_c, px, py);
	break;
    case O_POLYLINE:
	set_cursor(wait_cursor);
	cur_l = (F_line *) p;
	list_delete_line(&objects.lines, cur_l);
	redisplay_line(cur_l);
	set_cursor(null_cursor);
	init_linedragging(cur_l, px, py);
	break;
    case O_TEXT:
	set_cursor(wait_cursor);
	cur_t = (F_text *) p;
	list_delete_text(&objects.texts, cur_t);
	redisplay_text(cur_t);
	set_cursor(null_cursor);
	init_textdragging(cur_t, x, y);
	break;
    case O_ELLIPSE:
	set_cursor(wait_cursor);
	cur_e = (F_ellipse *) p;
	list_delete_ellipse(&objects.ellipses, cur_e);
	redisplay_ellipse(cur_e);
	set_cursor(null_cursor);
	init_ellipsedragging(cur_e, px, py);
	break;
    case O_ARC:
	set_cursor(wait_cursor);
	cur_a = (F_arc *) p;
	list_delete_arc(&objects.arcs, cur_a);
	redisplay_arc(cur_a);
	set_cursor(null_cursor);
	init_arcdragging(cur_a, px, py);
	break;
    case O_SPLINE:
	set_cursor(wait_cursor);
	cur_s = (F_spline *) p;
	list_delete_spline(&objects.splines, cur_s);
	redisplay_spline(cur_s);
	set_cursor(null_cursor);
	init_splinedragging(cur_s, px, py);
	break;
    default:
	return;
    }
}
