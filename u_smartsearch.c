/*
 * FIG : Facility for Interactive Generation of figures
 * This part Copyright (c) 1999-2002 by Alexander Durner
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "u_list.h"
#include "w_setup.h"
#include "w_zoom.h"

#include "u_smartsearch.h"

#include "u_geom.h"
#include "u_markers.h"
#include "u_search.h"

/* how close to user-selected location? */
#define TOLERANCE (zoomscale>1?2:(int)(2/zoomscale))

/* `singularities' */
#define SING_TOLERANCE (zoomscale>.5?1:(int)(.5/zoomscale))

/***************************************************************************/

Boolean smart_next_arc_found(int x, int y, int tolerance, int *px, int *py, int shift);
Boolean smart_next_ellipse_found(int x, int y, int tolerance, int *px, int *py, int shift);
Boolean smart_next_line_found(int x, int y, int tolerance, int *px, int *py, int shift);
Boolean smart_next_spline_found(int x, int y, int tolerance, int *px, int *py, int shift);
Boolean smart_next_text_found(int x, int y, int tolerance, int *px, int *py, int shift);
Boolean smart_next_compound_found(int x, int y, int tolerance, int *px, int *py, int shift);

void do_smart_object_search(int x, int y, unsigned int shift);

void smart_show_objecthighlight(void);
void smart_erase_objecthighlight(void);
void smart_toggle_objecthighlight(void);

/* exports: */
F_point  smart_point1, smart_point2;

/* locals: */
static int	(*manipulate) ();
static int	(*handlerproc_left) ();
static int	(*handlerproc_middle) ();
static int	(*handlerproc_right) ();
static int	type;
static long	objectcount;
static long	n;
static int	csr_x, csr_y;

static F_arc   *a;
static F_ellipse *e;
static F_line  *l;
static F_spline *s;
static F_text  *t;
static F_compound *c;

/***************************************************************************/

/* functions: */



void
init_smart_searchproc_left(int (*handlerproc) (/* ??? */))
{
    handlerproc_left = handlerproc;
}

void
init_smart_searchproc_middle(int (*handlerproc) (/* ??? */))
{
    handlerproc_middle = handlerproc;
}

void
init_smart_searchproc_right(int (*handlerproc) (/* ??? */))
{
    handlerproc_right = handlerproc;
}


void
smart_object_search_left(int x, int y, unsigned int shift)
       		         
                          	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_left;
    do_smart_object_search(x, y, shift);
}

void
smart_object_search_middle(int x, int y, unsigned int shift)
       		         
                          	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_middle;
    do_smart_object_search(x, y, shift);
}

void
smart_object_search_right(int x, int y, unsigned int shift)
       		         
                          	/* Shift Key Status from XEvent */
{
    manipulate = handlerproc_right;
    do_smart_object_search(x, y, shift);
}

/***************************************************************************/

static void
set_smart_points(int x1, int y1, int x2, int y2)
{
    smart_point1.x = x1;
    smart_point1.y = y1;
    smart_point2.x = x2;
    smart_point2.y = y2;
}

static void
init_smart_search(void)
{
    if (highlighting)
	smart_erase_objecthighlight();
    else {
	objectcount = 0;
	if (ellipse_in_mask())
	    for (e = objects.ellipses; e != NULL; e = e->next)
		objectcount++;
	if (anyline_in_mask())
	    for (l = objects.lines; l != NULL; l = l->next)
		if (validline_in_mask(l))
		    objectcount++;
	if (anyspline_in_mask())
	    for (s = objects.splines; s != NULL; s = s->next)
		if (validspline_in_mask(s))
		    objectcount++;
	if (anytext_in_mask())
	    for (t = objects.texts; t != NULL; t = t->next)
		if (validtext_in_mask(t))
		    objectcount++;
	if (arc_in_mask())
	    for (a = objects.arcs; a != NULL; a = a->next)
		objectcount++;
	if (compound_in_mask())
	    for (c = objects.compounds; c != NULL; c = c->next)
		objectcount++;
	e = NULL;
	type = O_ELLIPSE;
    }
}

void
do_smart_object_search(int x, int y, unsigned int shift)
       		         
                          	/* Shift Key Status from XEvent */
{
    int		    px, py;
    Boolean	    found = False;

    init_smart_search();
    for (n = 0; n < objectcount;) {
	switch (type) {
	case O_ELLIPSE:
	    found = smart_next_ellipse_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_POLYLINE:
	    found = smart_next_line_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_SPLINE:
	    found = smart_next_spline_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_TEXT: 
	    found = smart_next_text_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_ARC:
	    found = smart_next_arc_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	case O_COMPOUND: 
	    found = smart_next_compound_found(x, y, TOLERANCE, &px, &py, shift);
	    break;
	}

	if (found)
	    break;

	switch (type) {
	case O_ELLIPSE:
	    type = O_POLYLINE;
	    l = NULL;
	    break;
	case O_POLYLINE:
	    type = O_SPLINE;
	    s = NULL;
	    break;
	case O_SPLINE:
	    type = O_TEXT;
	    t = NULL;
	    break;
	case O_TEXT:
	    type = O_ARC;
	    a = NULL;
	    break;
	case O_ARC:
	    type = O_COMPOUND;
	    c = NULL;
	    break;
	case O_COMPOUND:
	    type = O_ELLIPSE;
	    e = NULL;
	    break;
	}
    }
    if (!found) {		/* nothing found */
        /* dummy values */
        smart_point1.x = smart_point1.y = 0;    
        smart_point2.x = smart_point2.y = 0;    
	csr_x = x;
	csr_y = y;
	type = -1;
	smart_show_objecthighlight();
    } else if (shift) {		/* show selected object */
	smart_show_objecthighlight();
    } else {			/* user selected an object */
	smart_erase_objecthighlight();
	switch (type) {
	case O_ELLIPSE:
	    manipulate(e, type, x, y, px, py);
	    break;
	case O_POLYLINE:
	    manipulate(l, type, x, y, px, py);
	    break;
	case O_SPLINE:
	    manipulate(s, type, x, y, px, py);
	    break;
	case O_TEXT:
	    manipulate(t, type, x, y, px, py);
	    break;
	case O_ARC:
	    manipulate(a, type, x, y, px, py);
	    break;
	case O_COMPOUND:
	    manipulate(c, type, x, y, px, py);
	    break;
	}
    }
}

/***************************************************************************/

Boolean
smart_next_arc_found(int x, int y, int tolerance, int *px, int *py, int shift)
{
   float ax, ay;
   int x1, y1, x2, y2;

   if (!arc_in_mask())
     return 0;
   if (a == NULL)
     a = last_arc(objects.arcs);
   else if (shift)
     a = prev_arc(objects.arcs, a);

   for (; a != NULL; a = prev_arc(objects.arcs, a), n++) {
     if (!close_to_arc(a, x, y, tolerance, &ax, &ay))
       continue;
     /* point found */
     *px = x1 = round(ax);
     *py = y1 = round(ay);
     x2 = x1 + round(ay - a->center.y);
     y2 = y1 - round(ax - a->center.x);
     set_smart_points(x1, y1, x2, y2);
     return 1;
   }
   return 0;
}

Boolean
smart_next_ellipse_found(int x, int y, int tolerance, int *px, int *py, int shift)
{
   float ex, ey, vx, vy;
   int x1, y1, x2, y2;

   if (!ellipse_in_mask())
        return (0);
   if (e == NULL)
	e = last_ellipse(objects.ellipses);
   else if (shift)
	e = prev_ellipse(objects.ellipses, e);
   for (; e != NULL; e = prev_ellipse(objects.ellipses, e), n++) {
      if (!close_to_ellipse(e, x, y, tolerance, &ex, &ey, &vx, &vy))
        continue;
      *px = round(ex);
      *py = round(ey);
      /* handle special case of very small ellipse */
      if (fabs(ex - e->center.x) <= SING_TOLERANCE && 
          fabs(ey - e->center.y) <= SING_TOLERANCE) {
        x1 = x2 = *px;
        y1 = y2 = *py;
      }
      else {
        x1 = *px;
        y1 = *py;
        x2 = x1 + round(vx);
        y2 = y1 + round(vy);
      }
      set_smart_points(x1, y1, x2, y2);
      return 1;
   }
   return 0;
}

Boolean
smart_next_line_found(int x, int y, int tolerance, int *px, int *py, int shift)
{				/* return the pointer to lines object if the
				 * search is successful otherwise return
				 * NULL.  The value returned via (px, py) is
				 * the closest point on the vector to point
				 * (x, y)					 */

    int lx1, ly1, lx2, ly2;

    if (!anyline_in_mask())
	return (0);
    if (l == NULL)
	l = last_line(objects.lines);
    else if (shift)
	l = prev_line(objects.lines, l);

    for (; l != NULL; l = prev_line(objects.lines, l)) {
	if (validline_in_mask(l)) {
	    n++;
            if (close_to_polyline(l, x, y, tolerance, SING_TOLERANCE, px, py,
                                  &lx1, &ly1, &lx2, &ly2)) {
              set_smart_points(lx1, ly1, lx2, ly2);
              return 1;
	    }
	}
    }
    return 0;
}              

Boolean
smart_next_spline_found(int x, int y, int tolerance, int *px, int *py, int shift)
/* We call `close_to_spline' which uses HIGH_PRECISION.
   Think about it. 
   */
       		                              
       		          
{
    int lx1, ly1, lx2, ly2;

    if (!anyspline_in_mask())
	return (0);
    if (s == NULL)
	s = last_spline(objects.splines);
    else if (shift)
	s = prev_spline(objects.splines, s);

    for (; s != NULL; s = prev_spline(objects.splines, s)) {
	if (validspline_in_mask(s)) {
	    n++;
            if (close_to_spline(s, x, y, tolerance, px, py, 
				&lx1, &ly1, &lx2, &ly2)) {
              set_smart_points(lx1, ly1, lx2, ly2);
              return 1;
	    }
	}
    }
    return 0;
}

/* actually, the following are not very smart */

Boolean
smart_next_text_found(int x, int y, int tolerance, int *px, int *py, int shift)
{
    int		    dum, tlength;

    if (!anytext_in_mask())
	return (0);
    if (t == NULL)
	t = last_text(objects.texts);
    else if (shift)
	t = prev_text(objects.texts, t);

    for (; t != NULL; t = prev_text(objects.texts, t))
	if (validtext_in_mask(t)) {
	    n++;
	    if (in_text_bound(t, x, y, &dum, False)) {
		*px = x;
		*py = y;
                tlength = text_length(t);
                set_smart_points(t->base_x, t->base_y,
                                 t->base_x + round(tlength * cos((double)t->angle)),
                                 t->base_y + round(tlength * sin((double)t->angle)));
		return 1;
	    }
	}
    return 0;
}

Boolean
smart_next_compound_found(int x, int y, int tolerance, int *px, int *py, int shift)
{
    float	    tol2;

    if (!compound_in_mask())
	return (0);
    if (c == NULL)
	c = last_compound(objects.compounds);
    else if (shift)
	c = prev_compound(objects.compounds, c);

    tol2 = tolerance * tolerance;

    for (; c != NULL; c = prev_compound(objects.compounds, c), n++) {
	if (close_to_vector(c->nwcorner.x, c->nwcorner.y, c->nwcorner.x,
			    c->secorner.y, x, y, tolerance, tol2, px, py)) {
            set_smart_points(c->nwcorner.x, c->nwcorner.y, c->nwcorner.x, c->secorner.y);
            return 1;
	}
	else if (close_to_vector(c->secorner.x, c->secorner.y, c->nwcorner.x,
				 c->secorner.y, x, y, tolerance, tol2, px, py)) {
            set_smart_points(c->secorner.x, c->secorner.y, c->nwcorner.x, c->secorner.y);
	    return 1;
	}
	else if (close_to_vector(c->secorner.x, c->secorner.y, c->secorner.x,
				 c->nwcorner.y, x, y, tolerance, tol2, px, py)) {
            set_smart_points(c->secorner.x, c->secorner.y, c->secorner.x, c->nwcorner.y);
            return 1;
	}
	else if (close_to_vector(c->nwcorner.x, c->nwcorner.y, c->secorner.x,
				 c->nwcorner.y, x, y, tolerance, tol2, px, py)) {
            set_smart_points(c->nwcorner.x, c->nwcorner.y, c->secorner.x, c->nwcorner.y);
            return 1;
	}
    }
    return 0;
}

void smart_show_objecthighlight(void)
{   
    if (highlighting)
	return;
    highlighting = 1;
    smart_toggle_objecthighlight();
}

void smart_erase_objecthighlight(void)
{
    if (!highlighting)
	return;
    highlighting = 0;
    smart_toggle_objecthighlight();
    if (type == -1) {
	e = NULL;
	type = O_ELLIPSE;
    }
}

void smart_toggle_objecthighlight(void)
{
    switch (type) {
    case O_ELLIPSE:
	toggle_ellipsehighlight(e);
	break;
    case O_POLYLINE:
	toggle_linehighlight(l);
	break;
    case O_SPLINE:
	toggle_splinehighlight(s);
	break;
    case O_TEXT:
	toggle_texthighlight(t);
	break;
    case O_ARC:
	toggle_archighlight(a);
	break;
    case O_COMPOUND:
	toggle_compoundhighlight(c);
	break;
    default:
	toggle_csrhighlight(csr_x, csr_y);
    }
}

void
smart_null_proc(void)
{
    /* almost does nothing */
    if (highlighting)
	smart_erase_objecthighlight();
}
