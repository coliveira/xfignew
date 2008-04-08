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
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "paintop.h"
#include "object.h"
#include "w_indpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#include "u_redraw.h"

#define null_width 32
#define null_height 32

static char	null_bits[null_width * null_height / 8] = {0};

static Pixmap	null_pm, grid_pm = 0;
static unsigned long bg, fg;



void init_grid(void)
{
    DeclareArgs(2);

    if (null_pm == 0) {
	FirstArg(XtNbackground, &bg);
	NextArg(XtNforeground, &fg);
	GetValues(canvas_sw);

	null_pm = XCreatePixmapFromBitmapData(tool_d, canvas_win,
				(char *) null_bits, null_width, null_height,
				      fg, bg, tool_dpth);
    }
}

/* grid in X11 is simply the background of the canvas */

void setup_grid(void)
{
    double	    spacing;
    double	    x, x0c, y, y0c;
    int		    grid, grid_unit;
    int		    dim;
    static int	    prev_grid = -1;

    DeclareArgs(2);

    grid = cur_gridmode;

    if (grid == GRID_0) {
	FirstArg(XtNbackgroundPixmap, null_pm);
    } else {
	grid_unit = cur_gridunit;
	/* user scale not = 1.0, use tenths of inch if inch scale */
	if (appres.userscale != 1.0 && appres.INCHES)
	    grid_unit = TENTH_UNIT;
	spacing = grid_spacing[grid_unit][grid-1] * zoomscale/appres.userscale;
	/* if zoom is small, use larger grid */
	while (spacing < 5.0 && ++grid <= GRID_4) {
	    spacing = grid_spacing[grid_unit][grid-1] * zoomscale/appres.userscale;
	}
	/* round to three decimal places */
	spacing = (int) (spacing*1000.0) / 1000.0;

	if (spacing <= 4.0) {
	    /* too small at this zoom, no grid */
	    FirstArg(XtNbackgroundPixmap, null_pm);
	    redisplay_canvas();
	} else {
		/* size of the pixmap equal to 1 inch or 2 cm to reset any 
		   error at those boundaries */
		dim = (int) round(appres.INCHES? 
				PIX_PER_INCH*zoomscale/appres.userscale:
				2*PIX_PER_CM*zoomscale/appres.userscale);
		/* HOWEVER, if that is a ridiculous size, limit it to approx screen size */
		if (dim > CANVAS_WD)
		    dim = CANVAS_WD;

		while (fabs((float) dim / spacing - (int) ((float) dim / spacing)) > 0.01) {
		    dim++;
		    if (dim > CANVAS_WD)
			break;
		}

		if (grid_pm)
		    XFreePixmap(tool_d, grid_pm);
		grid_pm = XCreatePixmap(tool_d, canvas_win, dim, dim, tool_dpth);
		/* first fill the pixmap with the background color */
		XSetForeground(tool_d, grid_gc, bg);
		XFillRectangle(tool_d, grid_pm, grid_gc, 0, 0, dim, dim);
		/* now write the grid in the grid color */
		XSetForeground(tool_d, grid_gc, grid_color);
		x0c = -fmod((double) zoomscale * zoomxoff, spacing);
		y0c = -fmod((double) zoomscale * zoomyoff, spacing);
		if (spacing - x0c < 0.5)
			x0c = 0.0;
		if (spacing - y0c < 0.5)
			y0c = 0.0;
		for (x = x0c; x < dim; x += spacing)
		    XDrawLine(tool_d, grid_pm, grid_gc, (int) round(x), 0, (int) round(x), dim);
		for (y = y0c; y < dim; y += spacing)
		    XDrawLine(tool_d, grid_pm, grid_gc, 0, (int) round(y), dim, (int) round(y));

		FirstArg(XtNbackgroundPixmap, grid_pm);
	}
    }
    SetValues(canvas_sw);
    if (prev_grid == GRID_0 && grid == GRID_0)
	redisplay_canvas();
    prev_grid = grid;
}
