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
#include "u_search.h"
#include "u_list.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"

#include "u_markers.h"
#include "w_cursor.h"

static void	init_break(F_line *p, int type, int x, int y, int px, int py, int loc_tag);
static void	init_break_only(F_line *p, int type, int x, int y, int px, int py);
static void	init_break_tag(F_line *p, int type, int x, int y, int px, int py);



void
break_selected(void)
{
    set_mousefun("break compound", "break and tag", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_break_only);
    init_searchproc_middle(init_break_tag);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick15_cursor);
    reset_action_on();
}

static void
init_break_only(F_line *p, int type, int x, int y, int px, int py)
{
    init_break(p, type, x, y, px, py, 0);
}

static void
init_break_tag(F_line *p, int type, int x, int y, int px, int py)
{
    init_break(p, type, x, y, px, py, 1);
}

static void
init_break(F_line *p, int type, int x, int y, int px, int py, int loc_tag)
{
    if (type != O_COMPOUND)
	return;

    cur_c = (F_compound *) p;
    mask_toggle_compoundmarker(cur_c);
    clean_up();
    list_delete_compound(&objects.compounds, cur_c);
    tail(&objects, &object_tails);
    append_objects(&objects, cur_c, &object_tails);
    toggle_markers_in_compound(cur_c);
    set_tags(cur_c, loc_tag);
    set_action(F_BREAK);
    set_latestcompound(cur_c);
    set_modifiedflag();
}
