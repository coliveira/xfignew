/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Henning Spruth
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

#include <X11/keysym.h>
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "u_create.h"
#include "u_elastic.h"
#include "u_pan.h"
#include "w_canvas.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#include "w_cursor.h"

/* global for w_canvas.c */

Boolean	zoom_in_progress = False;

static void	do_zoom(int x, int y), cancel_zoom(void);
static void	init_zoombox_drawing(int x, int y);

static void	(*save_kbd_proc) ();
static void	(*save_locmove_proc) ();
static void	(*save_ref_proc) ();
static void	(*save_leftbut_proc) ();
static void	(*save_middlebut_proc) ();
static void	(*save_rightbut_proc) ();

float		display_zoomscale;	/* both zoomscales initialized in main() */
float		zoomscale;
int		zoomxoff = 0;
int		zoomyoff = 0;
Boolean		integral_zoom = False;	/* integral zoom flag for area zoom (mouse) */

/* used for private box drawing functions */
static int	my_fix_x, my_fix_y;
static int	my_cur_x, my_cur_y;



void
zoom_selected(int x, int y, unsigned int button)
{
    /* if trying to zoom while drawing an object, don't allow it */
    if ((button != Button2) && check_action_on()) /* panning is ok */
	return;
    /* don't allow zooming while previewing */
    if (preview_in_progress)
	return;

    if (!zoom_in_progress) {
	switch (button) {
	case Button1:
	    set_temp_cursor(magnify_cursor);
	    init_zoombox_drawing(x, y);
	    break;
	case Button2:
	    pan_origin();
	    break;
	case Button3:
	    unzoom();
	    break;
	case Button4:
	    wheel_dec_zoom();
	    break;
	case Button5:
	    wheel_inc_zoom();
	    break;
	}
    } else if (button == Button1) {
	reset_cursor();
	do_zoom(x, y);
    }
}

void
unzoom(void)
{
    display_zoomscale = 1.0;
    show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
}

static void
my_box(int x, int y)
{
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    my_cur_x = x;
    my_cur_y = y;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
}

static void
elastic_mybox(void)
{
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
}

static void
init_zoombox_drawing(int x, int y)
{
    if (check_action_on())
	return;
    save_kbd_proc = canvas_kbd_proc;
    save_locmove_proc = canvas_locmove_proc;
    save_ref_proc = canvas_ref_proc;
    save_leftbut_proc = canvas_leftbut_proc;
    save_middlebut_proc = canvas_middlebut_proc;
    save_rightbut_proc = canvas_rightbut_proc;
    save_kbd_proc = canvas_kbd_proc;

    my_cur_x = my_fix_x = x;
    my_cur_y = my_fix_y = y;

    canvas_locmove_proc = my_box;
    canvas_ref_proc = elastic_mybox;
    canvas_leftbut_proc = do_zoom;
    canvas_middlebut_proc = canvas_rightbut_proc = null_proc;
    canvas_rightbut_proc = cancel_zoom;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    set_action_on();
    cur_mode = F_ZOOM;
    zoom_in_progress = True;
}

static void
do_zoom(int x, int y)
{
    int		    dimx, dimy;
    float	    scalex, scaley;

    /* don't allow zooming while previewing */
    if (preview_in_progress)
	return;
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    zoomxoff = my_fix_x < x ? my_fix_x : x;
    zoomyoff = my_fix_y < y ? my_fix_y : y;
    dimx = abs(x - my_fix_x);
    dimy = abs(y - my_fix_y);
    if (!appres.allownegcoords) {
	if (zoomxoff < 0)
	    zoomxoff = 0;
	if (zoomyoff < 0)
	    zoomyoff = 0;
    }
    if (dimx && dimy) {
	scalex = ZOOM_FACTOR * CANVAS_WD / (float) dimx;
	scaley = ZOOM_FACTOR * CANVAS_HT / (float) dimy;

	display_zoomscale = (scalex > scaley ? scaley : scalex);
	if (display_zoomscale <= 1.0)	/* keep to 0.1 increments */
	    display_zoomscale = (int)((display_zoomscale+0.09)*10.0)/10.0 - 0.1;

	/* round if integral zoom is on (indicator panel in zoom popup) */
	if (integral_zoom && display_zoomscale > 1.0)
	    display_zoomscale = round(display_zoomscale);
	show_zoom(&ind_switches[ZOOM_SWITCH_INDEX]);
    }
    /* restore state */
    canvas_kbd_proc = save_kbd_proc;
    canvas_locmove_proc = save_locmove_proc;
    canvas_ref_proc = save_ref_proc;
    canvas_leftbut_proc = save_leftbut_proc;
    canvas_middlebut_proc = save_middlebut_proc;
    canvas_rightbut_proc = save_rightbut_proc;
    canvas_kbd_proc = save_kbd_proc;
    reset_action_on();
    zoom_in_progress = False;
}

static void
cancel_zoom(void)
{
    elastic_box(my_fix_x, my_fix_y, my_cur_x, my_cur_y);
    /* restore state */
    canvas_kbd_proc = save_kbd_proc;
    canvas_locmove_proc = save_locmove_proc;
    canvas_ref_proc = save_ref_proc;
    canvas_leftbut_proc = save_leftbut_proc;
    canvas_middlebut_proc = save_middlebut_proc;
    canvas_rightbut_proc = save_rightbut_proc;
    canvas_kbd_proc = save_kbd_proc;
    reset_cursor();
    reset_action_on();
    zoom_in_progress = False;
}
