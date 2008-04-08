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
#include "mode.h"
#include "d_arc.h"
#include "e_flip.h"
#include "e_rotate.h"
#include "u_draw.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#include "d_text.h"
#include "u_bound.h"
#include "u_elastic.h"
#include "u_markers.h"
#include "w_cursor.h"
#include "w_rulers.h"

/* EXPORTS */

/* set in redisplay_region if called when preview_in_progress is true */
Boolean	request_redraw = False;

/*
 * Support for rendering based on correct object depth.	 A simple depth based
 * caching scheme; anything more will require major surgery on the object
 * data structures that will percolate throughout program.
 *
 * One ``counts'' structure for each object type at each nesting depth from 0
 * to MAX_DEPTH - 1.  We track both the number of objects per type per depth,
 * as well as the number of objects drawn so far per type per depth to cut
 * down on search loop overhead.
 */

/*
 * The array of ``counts'' structures.	All objects at depth >= MAX_DEPTH are
 * accounted for in the counts[MAX_DEPTH] entry.
 */

struct counts	counts[MAX_DEPTH + 1], saved_counts[MAX_DEPTH + 1];

/*
 * Function to clear the array of object counts with file load or new command.
 */


void redisplay_arcobject (F_arc *arcs, int depth);
void redisplay_compoundobject (F_compound *compounds, int depth);
void redisplay_ellipseobject (F_ellipse *ellipses, int depth);
void redisplay_lineobject (F_line *lines, int depth);
void redisplay_splineobject (F_spline *splines, int depth);
void redisplay_textobject (F_text *texts, int depth);
void redraw_pageborder (void);
void draw_pb (int x, int y, int w, int h);

void
clearallcounts(void)
{
    register struct counts *cp;

    for (cp = &counts[0]; cp <= &counts[MAX_DEPTH]; ++cp) {
	cp->num_arcs = 0;
	cp->num_lines = 0;
	cp->num_ellipses = 0;
	cp->num_splines = 0;
	cp->num_texts = 0;
    }
    clearcounts();
}

/*
 * Clear count of objects drawn at each depth
 */

void
clearcounts(void)
{
    register struct counts *cp;

    for (cp = &counts[0]; cp <= &counts[MAX_DEPTH]; ++cp) {
	cp->cnt_arcs = 0;
	cp->cnt_lines = 0;
	cp->cnt_ellipses = 0;
	cp->cnt_splines = 0;
	cp->cnt_texts = 0;
    }
}

void redisplay_objects(F_compound *active_objects)
{
    int		    depth;
    F_compound	   *objects, *save_objects;
  
    objects = active_objects;
    save_objects = (F_compound *) NULL;

    draw_parent_gray = False;

    if (objects == NULL)
	return;

    /*
     * Opened compound with `keep parent visible'?
     */
    for (; (objects->parent != NULL) && (objects->draw_parent); ) {
	if (!save_objects)
	    save_objects = objects;
	/* put in any changes */
	*((F_compound*)objects->GABPtr) = *objects;            
	/* follow parent to the top */
	objects = objects->parent;
	/* instruct lower level procs to draw gray */
	draw_parent_gray = True;
    }

    clearcounts();

    /* if user wants gray inactive layers, draw them first */
    if (gray_layers || draw_parent_gray) {
	for (depth = max_depth; depth >= min_depth; --depth) {
	    if (!active_layer(depth) || draw_parent_gray) {
		redisplay_arcobject(objects->arcs, depth);
		redisplay_compoundobject(objects->compounds, depth);
		redisplay_ellipseobject(objects->ellipses, depth);
		redisplay_lineobject(objects->lines, depth);
		redisplay_splineobject(objects->splines, depth);
		redisplay_textobject(objects->texts, depth);
	    }
	}
    }

    if (draw_parent_gray) {
	/* now point to just the open compound and (re)draw it */
	clearcounts();
	objects = save_objects;
	draw_parent_gray = False;
    }

    /* now draw the active layers in their normal colors */
    for (depth = max_depth; depth >= min_depth; --depth) {
	if (active_layer(depth)) {
	    redisplay_arcobject(objects->arcs, depth);
	    redisplay_compoundobject(objects->compounds, depth);
	    redisplay_ellipseobject(objects->ellipses, depth);
	    redisplay_lineobject(objects->lines, depth);
	    redisplay_splineobject(objects->splines, depth);
	    redisplay_textobject(objects->texts, depth);
	}
    }

    /*
     * Point markers and compounds, not being ``real objects'', are handled
     * outside the depth loop.
     */

    /* show the markers if they are on */
    toggle_markers_in_compound(active_objects);
    /* mark any center if requested */
    if (setcenter)
	center_marker(setcenter_x, setcenter_y);
    /* and any anchor */
    if (setanchor)
	center_marker(setanchor_x, setanchor_y);
}

/*
 * Redisplay a list of arcs.  Only display arcs of the correct depth.
 * For each arc drawn, update the count for the appropriate depth in
 * the counts array.
 */

void redisplay_arcobject(F_arc *arcs, int depth)
{
    F_arc	   *arc;
    struct counts  *cp;
    
    if (arcs == NULL)
	return;
    cp = &counts[min2(depth, MAX_DEPTH)];

    arc = arcs;
    while (arc != NULL && cp->cnt_arcs < cp->num_arcs) {
	if (depth == arc->depth) {
		draw_arc(arc, PAINT);
		++cp->cnt_arcs;
	    }
	arc = arc->next;
    }
}

/*
 * Redisplay a list of ellipses.  Only display ellipses of the correct depth
 * For each ellipse drawn, update the count for the
 * appropriate depth in the counts array.
 */

void redisplay_ellipseobject(F_ellipse *ellipses, int depth)
{
    F_ellipse	   *ep;
    struct counts  *cp;
    
    if (ellipses == NULL)
	return;
    cp = &counts[min2(depth, MAX_DEPTH)];


    ep = ellipses;
    while (ep != NULL && cp->cnt_ellipses < cp->num_ellipses) {
	if (depth == ep->depth) {
		draw_ellipse(ep, PAINT);
		++cp->cnt_ellipses;
	    }
	ep = ep->next;
    }
}

/*
 * Redisplay a list of lines.  Only display lines of the correct depth.
 * For each line drawn, update the count for the appropriate
 * depth in the counts array.
 */

void redisplay_lineobject(F_line *lines, int depth)
{
    F_line	   *lp;
    struct counts  *cp;
    
    if (lines == NULL)
	return;
    cp = &counts[min2(depth, MAX_DEPTH)];


    lp = lines;
    while (lp != NULL && cp->cnt_lines < cp->num_lines) {
	if (depth == lp->depth) {
		draw_line(lp, PAINT);
		++cp->cnt_lines;
	    }
	lp = lp->next;
    }
}

/*
 * Redisplay a list of splines.	 Only display splines of the correct depth
 * For each spline drawn, update the count for the
 * appropriate depth in the counts array.
 */

void redisplay_splineobject(F_spline *splines, int depth)
{
    F_spline	   *spline;
    struct counts  *cp;
    
    if (splines == NULL)
	return;
    cp = &counts[min2(depth, MAX_DEPTH)];

    spline = splines;
    while (spline != NULL && cp->cnt_splines < cp->num_splines) {
	if (depth == spline->depth) {
		draw_spline(spline, PAINT);
		++cp->cnt_splines;
	    }
	spline = spline->next;
    }
}

/*
 * Redisplay a list of texts.  Only display texts of the correct depth.	 For
 * each text drawn, update the count for the appropriate depth in the counts
 * array.
 */

void redisplay_textobject(F_text *texts, int depth)
{
    F_text	   *text;
    struct counts  *cp;
    
    if (texts == NULL)
	return;
    cp = &counts[min2(depth, MAX_DEPTH)];

    text = texts;
    while (text != NULL && cp->cnt_texts < cp->num_texts) {
	if (depth == text->depth) {
	    draw_text(text, PAINT);
	    ++cp->cnt_texts;
	}
	text = text->next;
    }
}

/*
 * Redisplay a list of compounds at a current depth.  Basically just farm the
 * work out to the objects contained in the compound.
 */

void redisplay_compoundobject(F_compound *compounds, int depth)
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next) {
	redisplay_arcobject(c->arcs, depth);
	redisplay_compoundobject(c->compounds, depth);
	redisplay_ellipseobject(c->ellipses, depth);
	redisplay_lineobject(c->lines, depth);
	redisplay_splineobject(c->splines, depth);
	redisplay_textobject(c->texts, depth);
    }
}

/*
 * Redisplay the entire drawing.
 */
void
redisplay_canvas(void)
{
    /* turn off Compose key LED */
    setCompLED(0);

    redisplay_region(0, 0, CANVAS_WD, CANVAS_HT);
    reset_rulers();
}

/* redisplay the object currently being created by the user (if any) */

void redisplay_curobj(void)
{
    F_point	*p;
    int		 rx,ry,i;

    /* no object currently being created to refresh, return */
    if (!action_on)
	return;

    /* find which type of object we need to refresh */

    /* creating an object */
    /* placing a library object is slightly different */
    if (cur_mode < FIRST_EDIT_MODE) {
      switch (cur_mode) {
	case F_PLACE_LIB_OBJ:
	   break;	/* don't do anything */
	case F_PICOBJ:
	case F_ARCBOX:
	case F_BOX:
	   elastic_box(fix_x, fix_y, cur_x, cur_y);
	   break;
	case F_CIRCLE_BY_RAD:
	   elastic_cbr();
	   break;
	case F_CIRCLE_BY_DIA:
	   elastic_cbd();
	   break;
	case F_ELLIPSE_BY_RAD:
	   elastic_ebr();
	   break;
	case F_ELLIPSE_BY_DIA:
	   elastic_ebd();
	   break;
	case F_POLYLINE:
	case F_POLYGON:
	case F_APPROX_SPLINE:
	case F_CLOSED_APPROX_SPLINE:
	case F_INTERP_SPLINE:
	case F_CLOSED_INTERP_SPLINE:
	    for (p = first_point; p != NULL; p = p->next) {
		if (p->next) {
		    rx = p->next->x;
		    ry = p->next->y;
		} else {
		    /* no more points in the list, draw to current now */
		    rx = cur_x;
		    ry = cur_y;
		}
		pw_vector(canvas_win, p->x, p->y, rx, ry,
		      PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
		/* erase endpoint because next seg will redraw it */
		pw_vector(canvas_win, rx, ry, rx, ry,
		      INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
	    }
	    break;
	case F_CIRCULAR_ARC:
	    if (center_marked) {
	        center_marker(center_point.x, center_point.y);
		/* redraw either circle or partial arc */
		if (num_point)
		    elastic_arc();
		else
		    elastic_cbr();
	    } else {
		for (i=0; i<num_point; i++) {
		    if (i < num_point-1) {
			rx = point[i+1].x;
			ry = point[i+1].y;
		    } else {
			rx = cur_x;
			ry = cur_y;
		    }
		    pw_vector(canvas_win, point[i].x, point[i].y, rx, ry,
				INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
		}
	    }
	    break;
	case F_TEXT:
	    /* if the user is editing an existing string, erase the original
	       because redisplay_objects just re-drew it */
	    if (cur_t)
		draw_text(cur_t, INV_PAINT);
	    /* now refresh the temporary edit string */
	    draw_char_string();
	    break;
      }
    } else {
	/* editing an object, just refresh it as is */
	(*canvas_ref_proc)(cur_x, cur_y, 0);
    }
}

void redisplay_region(int xmin, int ymin, int xmax, int ymax)
{
    /* if we're generating a preview, don't redisplay the canvas 
       but set request flag so preview will call us with full canvas 
       after it is done generating the preview */

    if (preview_in_progress) {
	request_redraw = True;
	return;
    }

    set_temp_cursor(wait_cursor);
    /* kludge so that markers are redrawn */
    xmin -= 10;
    ymin -= 10;
    xmax += 10;
    ymax += 10;
    set_clip_window(xmin, ymin, xmax, ymax);
    clear_canvas();
    redisplay_objects(&objects);
    redisplay_curobj();
    reset_clip_window();
    reset_cursor();
}

/* update page border with new page size */

void update_pageborder(void)
{
    if (appres.show_pageborder) {
	clear_canvas();
	redisplay_canvas();		
    }
}

/* make a light blue line showing the right and bottom edge of the current page size */
/* also make light blue lines vertically and horizontally through 0,0 if
   showaxislines is true */

void redisplay_pageborder(void)
{

    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);
    /* first the axis lines */
    if (appres.showaxislines) {
	/* set the color */
	XSetForeground(tool_d, border_gc, axis_lines_color);
	XDrawLine(tool_d, canvas_win, border_gc, round(-zoomxoff*zoomscale), 0, 
					  round(-zoomxoff*zoomscale), CANVAS_HT);
	XDrawLine(tool_d, canvas_win, border_gc, 0, round(-zoomyoff*zoomscale), CANVAS_WD, 
					  round(-zoomyoff*zoomscale));
    }
    /* now the page border if user wants it */
    if (appres.show_pageborder) 
	redraw_pageborder();
}

void redraw_pageborder(void)
{
    int		   pwd, pht, ux;
    int		   x, y;
    char	   pname[80];

    /* set the color */
    XSetForeground(tool_d, border_gc, pageborder_color);

    pwd = paper_sizes[appres.papersize].width;
    pht = paper_sizes[appres.papersize].height;
    if (!appres.INCHES) {
	pwd = (int)(pwd*2.54*PIX_PER_CM/PIX_PER_INCH);
	pht = (int)(pht*2.54*PIX_PER_CM/PIX_PER_INCH);
    }
    /* swap height and width if landscape */
    if (appres.landscape) {
	ux = pwd;
	pwd = pht;
	pht = ux;
    }
    /* if multiple page mode, draw all page borders that are visible */
    if (appres.multiple) {
	int	wd, ht;

	wd = CANVAS_WD/zoomscale;
	ht = CANVAS_HT/zoomscale;

	/* draw the page borders in the positive direction */
	for (y=0; y < ht+zoomyoff; y += pht) {
	    for (x=0; x < wd+zoomxoff; x += pwd) {
		draw_pb(x, y, pwd, pht);
	    }
	}
	/* now draw the page borders in the -X, -Y direction */
	for (y=0; y > zoomyoff; y -= pht) {
	    for (x=0; x > zoomxoff; x -= pwd) {
		draw_pb(x, y, -pwd, -pht);
	    }
	}
	/* now draw the page borders in the -X, +Y direction */
	for (y=0; y < ht+zoomyoff; y += pht) {
	    for (x=0; x > zoomxoff; x -= pwd) {
		draw_pb(x, y, -pwd, pht);
	    }
	}
	/* now draw the page borders in the +X, -Y direction */
	for (y=0; y > zoomyoff; y -= pht) {
	    for (x=0; x < wd+zoomxoff; x += pwd) {
		draw_pb(x, y, pwd, -pht);
	    }
	}
    } else {
	zXDrawLine(tool_d, canvas_win, border_gc, 0,   0,   pwd, 0);
	zXDrawLine(tool_d, canvas_win, border_gc, pwd, 0,   pwd, pht);
	zXDrawLine(tool_d, canvas_win, border_gc, pwd, pht, 0,   pht);
	zXDrawLine(tool_d, canvas_win, border_gc, 0,   pht, 0,   0);
    }
    /* Now put the paper size in the lower-right corner just outside the page border.
       We want the full name except for the size in parenthesis */
    strcpy(pname,paper_sizes[appres.papersize].fname);
    /* truncate after first part of name */
    *(strchr(pname, '('))= '\0';
    XDrawString(tool_d,canvas_win,border_gc,ZOOMX(pwd)+3,ZOOMY(pht),pname,strlen(pname));
}

void draw_pb(int x, int y, int w, int h)
{
	zXDrawLine(tool_d, canvas_win, border_gc, x,   y,   x+w, y);
	zXDrawLine(tool_d, canvas_win, border_gc, x+w, y,   x+w, y+h);
	zXDrawLine(tool_d, canvas_win, border_gc, x+w, y+h, x,   y+h);
	zXDrawLine(tool_d, canvas_win, border_gc, x,   y+h, x,   y);
}

void redisplay_zoomed_region(int xmin, int ymin, int xmax, int ymax)
{
    redisplay_region(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax));
}

void redisplay_ellipse(F_ellipse *e)
{
    int		    xmin, ymin, xmax, ymax;

    ellipse_bound(e, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

void redisplay_ellipses(F_ellipse *e1, F_ellipse *e2)
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    ellipse_bound(e1, &xmin1, &ymin1, &xmax1, &ymax1);
    ellipse_bound(e2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

void redisplay_arc(F_arc *a)
{
    int		    xmin, ymin, xmax, ymax;
    int		    cx, cy;

    arc_bound(a, &xmin, &ymin, &xmax, &ymax);
    /* if vertices (and center point) are shown, make sure to include them in the clip area */
    if (appres.shownums) {
	/* first adjust for height/width of labels (don't really care which way they are) */
	xmin -= 175;
	xmax += 175;
	ymin -= 175;
	/* (the numbers are above the vertices so we don't need to move ymax) */

	/* now adjust for center point if it is outside the arc */
	cx = a->center.x;
	cy = a->center.y;
	if (cx < xmin+80)
	    xmin = cx-80;
	if (cx > xmax-150)
	    xmax = cx+150;
	if (cy < ymin+150)
	    ymin = cy-150;
	if (cy > ymax-80)
	    ymax = cy;
    }
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

void redisplay_arcs(F_arc *a1, F_arc *a2)
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    arc_bound(a1, &xmin1, &ymin1, &xmax1, &ymax1);
    arc_bound(a2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

void redisplay_spline(F_spline *s)
{
    int		    xmin, ymin, xmax, ymax;

    spline_bound(s, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

void redisplay_splines(F_spline *s1, F_spline *s2)
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    spline_bound(s1, &xmin1, &ymin1, &xmax1, &ymax1);
    spline_bound(s2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

void redisplay_line(F_line *l)
{
    int		    xmin, ymin, xmax, ymax;

    line_bound(l, &xmin, &ymin, &xmax, &ymax);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

void redisplay_lines(F_line *l1, F_line *l2)
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    line_bound(l1, &xmin1, &ymin1, &xmax1, &ymax1);
    line_bound(l2, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2);
}

void redisplay_compound(F_compound *c)
{
    redisplay_zoomed_region(c->nwcorner.x, c->nwcorner.y,
			    c->secorner.x, c->secorner.y);
}

void redisplay_compounds(F_compound *c1, F_compound *c2)
{
    redisplay_regions(c1->nwcorner.x, c1->nwcorner.y,
		      c1->secorner.x, c1->secorner.y,
		      c2->nwcorner.x, c2->nwcorner.y,
		      c2->secorner.x, c2->secorner.y);
}

void redisplay_text(F_text *t)
{
    int		    xmin, ymin, xmax, ymax;
    int		    dum;

    text_bound(t, &xmin, &ymin, &xmax, &ymax,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    redisplay_zoomed_region(xmin, ymin, xmax, ymax);
}

void redisplay_texts(F_text *t1, F_text *t2)
{
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;
    int		    dum;

    text_bound(t1, &xmin1, &ymin1, &xmax1, &ymax1,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    text_bound(t2, &xmin2, &ymin2, &xmax2, &ymax2,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1,
		      xmin2, ymin2, xmax2, ymax2);
}

void redisplay_regions(int xmin1, int ymin1, int xmax1, int ymax1, int xmin2, int ymin2, int xmax2, int ymax2)
{
    if (xmin1 == xmin2 && ymin1 == ymin2 && xmax1 == xmax2 && ymax1 == ymax2) {
	redisplay_zoomed_region(xmin1, ymin1, xmax1, ymax1);
	return;
    }
    /* below is easier than sending clip rectangle array to X */
    if (overlapping(xmin1, ymin1, xmax1, ymax1, xmin2, ymin2, xmax2, ymax2)) {
	redisplay_zoomed_region(min2(xmin1, xmin2), min2(ymin1, ymin2),
				max2(xmax1, xmax2), max2(ymax1, ymax2));
    } else {
	redisplay_zoomed_region(xmin1, ymin1, xmax1, ymax1);
	redisplay_zoomed_region(xmin2, ymin2, xmax2, ymax2);
    }
}

