/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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
#include "d_spline.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_draw.h"
#include "w_canvas.h"
#include "w_mousefun.h"

#include "f_util.h"
#include "u_free.h"
#include "u_redraw.h"
#include "w_cursor.h"
#include "w_msgpanel.h"
}

/*************************** local declarations *********************/

static void	init_spline_drawing(int x, int y);
static void	create_splineobject(int x, int y);
static void	init_spline_freehand_drawing(int x, int y);
static void	init_spline_drawing2(int x, int y);



void
spline_drawing_selected(void)
{
    set_mousefun("first point", "freehand", "", "", "", "");
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_leftbut_proc = (FCallBack)init_spline_drawing;
    canvas_middlebut_proc = (FCallBack)init_spline_freehand_drawing;
    canvas_rightbut_proc = (FCallBack)null_proc;
    set_cursor(crosshair_cursor);
    reset_action_on();
}

static void
init_spline_freehand_drawing(int x, int y)
{
    freehand_line = True;
    init_spline_drawing2(x, y);
}

static void
init_spline_drawing(int x, int y)
{
    freehand_line = False;
    init_spline_drawing2(x, y);
}

static void
init_spline_drawing2(int x, int y)
{
    if ((cur_mode == F_APPROX_SPLINE) || (cur_mode == F_INTERP_SPLINE)) {
	min_num_points = OPEN_SPLINE_MIN_NUM_POINTS;
	init_trace_drawing(x, y);
	canvas_middlebut_proc = (FCallBack)create_splineobject;
    } else {
	min_num_points = CLOSED_SPLINE_MIN_NUM_POINTS;
	init_trace_drawing(x, y);     
	canvas_middlebut_save = create_splineobject;
	canvas_middlebut_proc = (FCallBack)null_proc;
    }

    return_proc = spline_drawing_selected;
}


static void
create_splineobject(int x, int y)
{
    F_spline	   *spline;

    if (x != fix_x || y != fix_y) 
	get_intermediatepoint(x, y, 0);
    if (num_point < min_num_points) {	/* too few points */
	put_msg("Not enough points for spline");
	beep();
	return;
    }

    elastic_line();
    /* erase any length info if appres.showlengths is true */
    erase_lengths();
    if ((spline = create_spline()) == NULL) {
	if (num_point == 1) {
	    free((char *) cur_point);
	    cur_point = NULL;
	}
	free((char *) first_point);
	first_point = NULL;
	return;
    }


    spline->style = cur_linestyle;
    spline->thickness = cur_linewidth;
    spline->style_val = cur_styleval * (cur_linewidth + 1) / 2;
    spline->pen_color = cur_pencolor;
    spline->fill_color = cur_fillcolor;
    spline->cap_style = cur_capstyle;
    spline->depth = cur_depth;
    spline->pen_style = -1;
    spline->fill_style = cur_fillstyle;
    /*
     * The current fill style is saved in all spline objects (but support for
     * filling may not be available in all fig2dev languages).
     */
    spline->points = first_point;
    spline->sfactors = NULL;
    spline->next = NULL;
    /* initialize for no arrows - changed below if necessary */
    spline->for_arrow = NULL;
    spline->back_arrow = NULL;

    cur_x = cur_y = fix_x = fix_y = 0;	/* used in elastic_moveline */
    elastic_moveline(spline->points);	/* erase control vector */
    if (cur_mode == F_CLOSED_APPROX_SPLINE) {
	spline->type = T_CLOSED_APPROX;
    } else if (cur_mode == F_CLOSED_INTERP_SPLINE) {			
	spline->type = T_CLOSED_INTERP;
    } else {				/* It must be F_SPLINE */
	if (autoforwardarrow_mode)
	    spline->for_arrow = forward_arrow();
	if (autobackwardarrow_mode)
	    spline->back_arrow = backward_arrow();
	spline->type = (cur_mode == F_APPROX_SPLINE) ? T_OPEN_APPROX : T_OPEN_INTERP;
    }
    if (!make_sfactors(spline)) {
	free_spline(&spline);
    } else {
	add_spline(spline);
    }
    reset_action_on(); /* this signals redisplay_curobj() not to refresh */

    /* draw it and anything on top of it */
    redisplay_spline(spline);
    spline_drawing_selected();
    draw_mousefun_canvas();
}

int
make_sfactors(F_spline *spl)
{
  F_point   *p = spl->points;
  F_sfactor *sp, *cur_sp;
  int       type_s = approx_spline(spl) ? S_SPLINE_APPROX : S_SPLINE_INTERP;
  
  spl->sfactors = NULL;
  if ((sp = create_sfactor()) == NULL)
    return 0;
  spl->sfactors = cur_sp = sp;
  sp->s = closed_spline(spl) ? type_s : S_SPLINE_ANGULAR;
  p = p->next;
  
  for(; p!=NULL ; p=p->next)
    {
      if ((sp = create_sfactor()) == NULL)
	return 0;

      cur_sp->next = sp;
      sp->s = type_s;
      cur_sp = cur_sp->next;
    }
  
  cur_sp->s = spl->sfactors->s;
  cur_sp->next = NULL;
  return 1;
}
