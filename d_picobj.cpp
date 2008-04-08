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
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"

#include "d_box.h"
#include "e_edit.h"
#include "u_redraw.h"
#include "w_cursor.h"
void picobj_drawing_selected(void);
}
	
/*************************** local declarations *********************/

static void	init_picobj_drawing(int x, int y);
static void	create_picobj(int x, int y);
static void 	cancel_picobj(int,int);



void picobj_drawing_selected(void)
{
    set_mousefun("corner point", "", "", "", "", "");
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = (FCallBack)init_picobj_drawing;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_rightbut_proc = (FCallBack)null_proc;
    set_cursor(crosshair_cursor);
    reset_action_on();
}

static void
init_picobj_drawing(int x, int y)
{
    init_box_drawing(x, y);
    canvas_leftbut_proc = (FCallBack)create_picobj;
    canvas_rightbut_proc = (FCallBack)cancel_picobj;
}

static void
cancel_picobj(int a,int b)
{
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    picobj_drawing_selected();
    draw_mousefun_canvas();
}

static void
create_picobj(int x, int y)
{
    F_line	   *box;
    F_point	   *point;

    /* erase last lengths if appres.showlengths is true */
    erase_lengths();
    elastic_box(fix_x, fix_y, cur_x, cur_y);
    canvas_locmove_proc = null_proc;

    if ((point = create_point()) == NULL)
	return;

    point->x = fix_x;
    point->y = fix_y;
    point->next = NULL;

    if ((box = create_line()) == NULL) {
	free((char *) point);
	return;
    }
    box->type = T_PICTURE;
    box->style = SOLID_LINE;
    box->thickness = 1;
    box->pen_color = cur_pencolor;
    box->fill_color = DEFAULT;
    box->depth = cur_depth;
    box->pen_style = -1;
    box->join_style = 0;	/* not used */
    box->cap_style = 0;		/* not used */
    box->fill_style = UNFILLED;
    box->style_val = 0;

    if ((box->pic = create_pic()) == NULL) {
	free((char *) point);
	free((char *) box);
	return;
    }
    box->pic->New = True;		/* set new flag to delete if it user cancels edit operation */
    box->pic->pic_cache = NULL;
    box->pic->flipped = 0;
    box->pic->hw_ratio = 0.0;
    box->pic->pixmap = 0;
    box->pic->pix_width = 0;
    box->pic->pix_height = 0;
    box->pic->pix_rotation = 0;
    box->pic->pix_flipped = 0;
    box->points = point;
    append_point(fix_x, y, &point);
    append_point(x, y, &point);
    append_point(x, fix_y, &point);
    append_point(fix_x, fix_y, &point);
    add_line(box);
    /* draw it and anything on top of it */
    redisplay_line(box);
    put_msg("Please enter name of Picture Object file in EDIT window");
    edit_item(box, O_POLYLINE, 0, 0);
    picobj_drawing_selected();
    draw_mousefun_canvas();
}
