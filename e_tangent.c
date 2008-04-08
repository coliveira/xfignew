/*
 * FIG : Facility for Interactive Generation of figures
 * This part Copyright (c) 1999-2002 Alexander Durner
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
#include "u_create.h"
#include "u_elastic.h"
#include "u_search.h"
#include "u_smartsearch.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_setup.h"

#include "f_util.h"
#include "u_draw.h"
#include "u_list.h"
#include "u_markers.h"
#include "w_cursor.h"
#include "w_msgpanel.h"

#define ZERO_TOLERANCE 2.0


static void	init_tangent_adding(char *p, int type, int x, int y, int px, int py);
static void	init_normal_adding(char *p, int type, int x, int y, int px, int py);
static void	tangent_or_normal(int x, int y, int flag);
static void	tangent_normal_line(int x, int y, float vx, float vy);



void tangent_selected(void)
{
    set_mousefun("add tangent", "add normal", "", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = smart_null_proc;
    canvas_ref_proc = smart_null_proc;
    init_smart_searchproc_left(init_tangent_adding);
    init_smart_searchproc_middle(init_normal_adding);
    canvas_leftbut_proc = smart_object_search_left;
    canvas_middlebut_proc = smart_object_search_middle;
    canvas_rightbut_proc = smart_null_proc;
    set_cursor(pick9_cursor);
    /*    force_nopositioning(); */
    reset_action_on();
}

/*  smart_point1, smart_point2 are two points of the tangent */

static void
init_tangent_adding(char *p, int type, int x, int y, int px, int py)
{
    tangent_or_normal(px, py, 0);
}

static void
init_normal_adding(char *p, int type, int x, int y, int px, int py)
{
    tangent_or_normal(px, py, 1);
}

static void
tangent_or_normal(int x, int y, int flag)
{
    float dx, dy, length, sx, sy, tanlen;

    dx = (float)(smart_point2.x - smart_point1.x);
    dy = (float)(smart_point2.y - smart_point1.y);
    if (dx == 0.0 && dy == 0.0)
        length = 0.0;
    else
        length = sqrt(dx * dx + dy * dy);
    if (length < ZERO_TOLERANCE) {
        put_msg("%s", "singularity, can't draw tangent/normal");
	beep();
        return;
    }
    tanlen = cur_tangnormlen * (appres.INCHES? PIX_PER_INCH: PIX_PER_CM) / 2.0;
    sx = dx * tanlen / length;
    sy = dy * tanlen / length;
    if (flag) {
       tangent_normal_line(x, y, -sy, sx);
       put_msg("%s", "added normal");
    }
    else {
       tangent_normal_line(x, y, sx, sy);
       put_msg("%s", "added tangent");
    }
}

static void
tangent_normal_line(int x, int y, float vx, float vy)
{
    int dx, dy, xl, yl, xr, yr;
    F_line	   *line;
 
    dx = round(vx);
    dy = round(vy);   

    xl = x - dx;
    yl = y - dy;
    xr = x + dx;
    yr = y + dy;

    if ((first_point = create_point()) == NULL) 
        return;
    cur_point = first_point;
    first_point->x = xl;
    first_point->y = yl;
    first_point->next = NULL;
    append_point(x, y, &cur_point);
    append_point(xr, yr, &cur_point);
    
    if ((line = create_line()) == NULL)
        return; /* an error occured */
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
		/* polyline; draw any arrows */
    if (autoforwardarrow_mode)
	line->for_arrow = forward_arrow();
    /* arrow will be drawn in draw_line below */
    if (autobackwardarrow_mode)
	line->back_arrow = backward_arrow();
    /* arrow will be drawn in draw_line below */
    draw_line(line, PAINT);	/* draw final */
    add_line(line);
    toggle_linemarker(line);
}
