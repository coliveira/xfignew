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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "f_read.h"
#include "u_create.h"
#include "u_list.h"
#include "u_elastic.h"
#include "u_redraw.h"
#include "u_undo.h"
#include "w_layers.h"
#include "w_setup.h"

#include "u_draw.h"
#include "u_markers.h"

/*************************************/
/****** DELETE object from list ******/
/*************************************/


int point_on_perim (F_point *p, int llx, int lly, int urx, int ury);
int point_on_inside (F_point *p, int llx, int lly, int urx, int ury);

void
list_delete_arc(F_arc **arc_list, F_arc *arc)
{
    F_arc	   *a, *aa;

    if (*arc_list == NULL)
	return;
    if (arc == NULL)
	return;

    if (arc_list == &objects.arcs)
	remove_depth(O_ARC, arc->depth);
    for (a = aa = *arc_list; aa != NULL; a = aa, aa = aa->next) {
	if (aa == arc) {
	    if (aa == *arc_list)
		*arc_list = (*arc_list)->next;
	    else
		a->next = aa->next;
	    break;
	}
    }
    arc->next = NULL;
}

void
list_delete_ellipse(F_ellipse **ellipse_list, F_ellipse *ellipse)
{
    F_ellipse	   *q, *r;

    if (*ellipse_list == NULL)
	return;
    if (ellipse == NULL)
	return;

    if (ellipse_list == &objects.ellipses)
	remove_depth(O_ELLIPSE, ellipse->depth);
    for (q = r = *ellipse_list; r != NULL; q = r, r = r->next) {
	if (r == ellipse) {
	    if (r == *ellipse_list)
		*ellipse_list = (*ellipse_list)->next;
	    else
		q->next = r->next;
	    break;
	}
    }
    ellipse->next = NULL;
}

void
list_delete_line(F_line **line_list, F_line *line)
{
    F_line	   *q, *r;

    if (*line_list == NULL)
	return;
    if (line == NULL)
	return;

    if (line_list == &objects.lines)
	remove_depth(O_POLYLINE, line->depth);
    for (q = r = *line_list; r != NULL; q = r, r = r->next) {
	if (r == line) {
	    if (r == *line_list)
		*line_list = (*line_list)->next;
	    else
		q->next = r->next;
	    break;
	}
    }
    line->next = NULL;
}

void
list_delete_spline(F_spline **spline_list, F_spline *spline)
{
    F_spline	   *q, *r;

    if (*spline_list == NULL)
	return;
    if (spline == NULL)
	return;

    if (spline_list == &objects.splines)
	remove_depth(O_SPLINE, spline->depth);
    for (q = r = *spline_list; r != NULL; q = r, r = r->next) {
	if (r == spline) {
	    if (r == *spline_list)
		*spline_list = (*spline_list)->next;
	    else
		q->next = r->next;
	    break;
	}
    }
    spline->next = NULL;
}

void
list_delete_text(F_text **text_list, F_text *text)
{
    F_text	   *q, *r;

    if (*text_list == NULL)
	return;
    if (text == NULL)
	return;

    if (text_list == &objects.texts)
	remove_depth(O_TEXT, text->depth);
    for (q = r = *text_list; r != NULL; q = r, r = r->next)
	if (r == text) {
	    if (r == *text_list)
		*text_list = text->next;
	    else
		q->next = text->next;
	    break;
	}
    text->next = NULL;
}

void
list_delete_compound(F_compound **list, F_compound *compound)
{
    F_compound	   *c, *cc;

    if (*list == NULL)
	return;
    if (compound == NULL)
	return;

    if (list == &objects.compounds)
	remove_compound_depth(compound);

    for (cc = c = *list; c != NULL; cc = c, c = c->next) {
	if (c == compound) {
	    if (c == *list) 
		*list = (*list)->next;
	    else
		cc->next = c->next;
	    break;
	}
    }
    compound->next = NULL;
}

void
remove_depth(int type, int depth)
{
    int		    i;

    object_depths[depth]--;
    if (appres.DEBUG)
	fprintf(stderr,"remove depth %d, count=%d\n",depth,object_depths[depth]);
    /* now subtract one from the counter for this object type */
    switch (type) {
	case O_ARC:
	    --counts[depth].num_arcs;
	    if (appres.DEBUG)
		fprintf(stderr,"Arc[%d] count=%d\n",depth,counts[depth].num_lines);
	    break;
	case O_POLYLINE:
	    --counts[depth].num_lines;
	    if (appres.DEBUG)
		fprintf(stderr,"Line[%d] count=%d\n",depth,counts[depth].num_lines);
	    break;
	case O_ELLIPSE:
	    --counts[depth].num_ellipses;
	    if (appres.DEBUG)
		fprintf(stderr,"Ellipse[%d] count=%d\n",depth,counts[depth].num_ellipses);
	    break;
	case O_SPLINE:
	    --counts[depth].num_splines;
	    if (appres.DEBUG)
		fprintf(stderr,"Spline[%d] count=%d\n",depth,counts[depth].num_splines);
	    break;
	case O_TEXT:
	    --counts[depth].num_texts;
	    if (appres.DEBUG)
		fprintf(stderr,"Text[%d] count=%d\n",depth,counts[depth].num_texts);
	    break;
    }
    if (min_depth != -1 && (min_depth < depth && max_depth > depth) &&
	object_depths[depth] != 0)
	    return;
    /* if no objects at this depth, find new min/max */
    for (i=0; i<=MAX_DEPTH; i++)
	if (object_depths[i])
	    break;
    if (i<=MAX_DEPTH) {
	min_depth = i;
	if (appres.DEBUG)
	    fprintf(stderr,"New min = %d\n",min_depth);
	for (i=MAX_DEPTH; i>=0; i--)
	    if (object_depths[i])
		break;
	if (i>=0) {
	    max_depth = i;
	    if (appres.DEBUG)
		fprintf(stderr,"New max = %d\n",max_depth);
	}
    } else {
	min_depth = -1;
	if (appres.DEBUG)
	    fprintf(stderr,"No objects\n");
    }
    /* adjust the layer buttons */
    update_layers();
}

void
remove_compound_depth(F_compound *comp)
{
    F_arc	   *aa;
    F_ellipse	   *ee;
    F_line	   *ll;
    F_spline	   *ss;
    F_text	   *tt;
    F_compound	   *cc;

    if (comp == (F_compound *) 0)
	return;

    /* defer updating of layer buttons until we've added the entire compound */
    defer_update_layers++;
    for (aa=comp->arcs; aa; aa=aa->next)
	remove_depth(O_ARC, aa->depth);
    for (ee=comp->ellipses; ee; ee=ee->next)
	remove_depth(O_ELLIPSE, ee->depth);
    for (ll=comp->lines; ll; ll=ll->next)
	remove_depth(O_POLYLINE, ll->depth);
    for (ss=comp->splines; ss; ss=ss->next)
	remove_depth(O_SPLINE, ss->depth);
    for (tt=comp->texts; tt; tt=tt->next)
	remove_depth(O_TEXT, tt->depth);
    for (cc=comp->compounds; cc; cc=cc->next)
	remove_compound_depth(cc);
    /* decrement the defer flag and update layer buttons if it hits 0 */
    defer_update_layers--;
    update_layers();
}


/********************************/
/****** ADD object to list ******/
/********************************/

void
list_add_arc(F_arc **list, F_arc *a)
{
    F_arc	   *aa;

    a->next = NULL;
    if ((aa = last_arc(*list)) == NULL)
	*list = a;
    else
	aa->next = a;
    if (list == &objects.arcs)
	while (a) {
	    add_depth(O_ARC, a->depth);
	    a = a->next;
	}
}

void
list_add_ellipse(F_ellipse **list, F_ellipse *e)
{
    F_ellipse	   *ee;

    e->next = NULL;
    if ((ee = last_ellipse(*list)) == NULL)
	*list = e;
    else
	ee->next = e;
    if (list == &objects.ellipses)
	while (e) {
	    add_depth(O_ELLIPSE, e->depth);
	    e = e->next;
	}
}

void
list_add_line(F_line **list, F_line *l)
{
    F_line	   *ll;

    l->next = NULL;
    if ((ll = last_line(*list)) == NULL)
	*list = l;
    else
	ll->next = l;
    if (list == &objects.lines)
	while (l) {
	    add_depth(O_POLYLINE, l->depth);
	    l = l->next;
	}
}

void
list_add_spline(F_spline **list, F_spline *s)
{
    F_spline	   *ss;

    s->next = NULL;
    if ((ss = last_spline(*list)) == NULL)
	*list = s;
    else
	ss->next = s;
    if (list == &objects.splines)
	while (s) {
	    add_depth(O_SPLINE, s->depth);
	    s = s->next;
	}
}

void
list_add_text(F_text **list, F_text *t)
{
    F_text	   *tt;

    t->next = NULL;
    if ((tt = last_text(*list)) == NULL)
	*list = t;
    else
	tt->next = t;
    if (list == &objects.texts)
	while (t) {
	    add_depth(O_TEXT, t->depth);
	    t = t->next;
	}
}

void
list_add_compound(F_compound **list, F_compound *c)
{
    F_compound	   *cc;

    c->next = NULL;
    if ((cc = last_compound(*list)) == NULL)
	*list = c;
    else
	cc->next = c;
    
    if (list == &objects.compounds) {
	while (c) {
	    add_compound_depth(c);
	    c = c->next;
	}
    }
}

/* increment objects_depth[depth] for a new object, and add to the 
   counter for this "type" (line, arc, etc) */

void
add_depth(int type, int depth)
{
    int		    i;

    object_depths[depth]++;

    if (appres.DEBUG)
	fprintf(stderr,"add depth %d, count=%d\n",depth,object_depths[depth]);
    /* add one to the counter for this object type */
    switch (type) {
	case O_ARC:
	    ++counts[depth].num_arcs;
	    if (appres.DEBUG)
		fprintf(stderr,"Arc[%d] count=%d\n",depth,counts[depth].num_arcs);
	    break;
	case O_POLYLINE:
	    ++counts[depth].num_lines;
	    if (appres.DEBUG)
		fprintf(stderr,"Line[%d] count=%d\n",depth,counts[depth].num_lines);
	    break;
	case O_ELLIPSE:
	    ++counts[depth].num_ellipses;
	    if (appres.DEBUG)
		fprintf(stderr,"Ellipse[%d] count=%d\n",depth,counts[depth].num_ellipses);
	    break;
	case O_SPLINE:
	    ++counts[depth].num_splines;
	    if (appres.DEBUG)
		fprintf(stderr,"Spline[%d] count=%d\n",depth,counts[depth].num_splines);
	    break;
	case O_TEXT:
	    ++counts[depth].num_texts;
	    if (appres.DEBUG)
		fprintf(stderr,"Text[%d] count=%d\n",depth,counts[depth].num_texts);
	    break;
    }
    if (object_depths[depth] != 1)
	return;
    /* if exactly one object at this depth, see if this is new min or max */
    for (i=0; i<=MAX_DEPTH; i++)
	if (object_depths[i])
	    break;
    min_depth = i;
    if (appres.DEBUG)
	fprintf(stderr,"New min = %d\n",min_depth);
    for (i=MAX_DEPTH; i>=0; i--)
	if (object_depths[i])
	    break;
    if (i>=0) {
	max_depth = i;
	if (appres.DEBUG)
	    fprintf(stderr,"New max = %d\n",max_depth);
    }
    /* adjust the layer buttons */
    update_layers();
}

void
add_compound_depth(F_compound *comp)
{
    F_arc	   *aa;
    F_ellipse	   *ee;
    F_line	   *ll;
    F_spline	   *ss;
    F_text	   *tt;
    F_compound	   *cc;

    /* defer updating of layer buttons until we've added the entire compound */
    defer_update_layers++;
    for (aa=comp->arcs; aa; aa=aa->next)
	add_depth(O_ARC, aa->depth);
    for (ee=comp->ellipses; ee; ee=ee->next)
	add_depth(O_ELLIPSE, ee->depth);
    for (ll=comp->lines; ll; ll=ll->next)
	add_depth(O_POLYLINE, ll->depth);
    for (ss=comp->splines; ss; ss=ss->next)
	add_depth(O_SPLINE, ss->depth);
    for (tt=comp->texts; tt; tt=tt->next)
	add_depth(O_TEXT, tt->depth);
    for (cc=comp->compounds; cc; cc=cc->next)
	add_compound_depth(cc);
    /* decrement the defer flag and update layer buttons if it hits 0 */
    defer_update_layers--;
    update_layers();
}

/**********************************/
/* routines to delete the objects */
/**********************************/

void
delete_line(F_line *old_l)
{
    list_delete_line(&objects.lines, old_l);
    clean_up();
    set_latestline(old_l);
    set_action_object(F_DELETE, O_POLYLINE);
    set_modifiedflag();
}

void
delete_arc(F_arc *old_a)
{
    list_delete_arc(&objects.arcs, old_a);
    clean_up();
    set_latestarc(old_a);
    set_action_object(F_DELETE, O_ARC);
    set_modifiedflag();
}

void
delete_ellipse(F_ellipse *old_e)
{
    list_delete_ellipse(&objects.ellipses, old_e);
    clean_up();
    set_latestellipse(old_e);
    set_action_object(F_DELETE, O_ELLIPSE);
    set_modifiedflag();
}

void
delete_text(F_text *old_t)
{
    list_delete_text(&objects.texts, old_t);
    clean_up();
    set_latesttext(old_t);
    set_action_object(F_DELETE, O_TEXT);
    set_modifiedflag();
}

void
delete_spline(F_spline *old_s)
{
    list_delete_spline(&objects.splines, old_s);
    clean_up();
    set_latestspline(old_s);
    set_action_object(F_DELETE, O_SPLINE);
    set_modifiedflag();
}

void
delete_compound(F_compound *old_c)
{
    list_delete_compound(&objects.compounds, old_c);
    clean_up();
    set_latestcompound(old_c);
    set_action_object(F_DELETE, O_COMPOUND);
    set_modifiedflag();
}

/*******************************/
/* routines to add the objects */
/*******************************/

void
add_line(F_line *new_l)
{
    list_add_line(&objects.lines, new_l);
    clean_up();
    set_latestline(new_l);
    set_action_object(F_ADD, O_POLYLINE);
    set_modifiedflag();
}

void
add_arc(F_arc *new_a)
{
    list_add_arc(&objects.arcs, new_a);
    clean_up();
    set_latestarc(new_a);
    set_action_object(F_ADD, O_ARC);
    set_modifiedflag();
}

void
add_ellipse(F_ellipse *new_e)
{
    list_add_ellipse(&objects.ellipses, new_e);
    clean_up();
    set_latestellipse(new_e);
    set_action_object(F_ADD, O_ELLIPSE);
    set_modifiedflag();
}

void
add_text(F_text *new_t)
{
    list_add_text(&objects.texts, new_t);
    clean_up();
    set_latesttext(new_t);
    set_action_object(F_ADD, O_TEXT);
    set_modifiedflag();
}

void
add_spline(F_spline *new_s)
{
    list_add_spline(&objects.splines, new_s);
    clean_up();
    set_latestspline(new_s);
    set_action_object(F_ADD, O_SPLINE);
    set_modifiedflag();
}

void
add_compound(F_compound *new_c)
{
    list_add_compound(&objects.compounds, new_c);
    clean_up();
    set_latestcompound(new_c);
    set_action_object(F_ADD, O_COMPOUND);
    set_modifiedflag();
}


void
change_line(F_line *old_l, F_line *new_l)
{
    list_delete_line(&objects.lines, old_l);
    list_add_line(&objects.lines, new_l);
    clean_up();
    old_l->next = new_l;
    set_latestline(old_l);
    set_action_object(F_EDIT, O_POLYLINE);
    set_modifiedflag();
}

void
change_arc(F_arc *old_a, F_arc *new_a)
{
    list_delete_arc(&objects.arcs, old_a);
    list_add_arc(&objects.arcs, new_a);
    clean_up();
    old_a->next = new_a;
    set_latestarc(old_a);
    set_action_object(F_EDIT, O_ARC);
    set_modifiedflag();
}

void
change_ellipse(F_ellipse *old_e, F_ellipse *new_e)
{
    list_delete_ellipse(&objects.ellipses, old_e);
    list_add_ellipse(&objects.ellipses, new_e);
    clean_up();
    old_e->next = new_e;
    set_latestellipse(old_e);
    set_action_object(F_EDIT, O_ELLIPSE);
    set_modifiedflag();
}

void
change_text(F_text *old_t, F_text *new_t)
{
    list_delete_text(&objects.texts, old_t);
    list_add_text(&objects.texts, new_t);
    clean_up();
    old_t->next = new_t;
    set_latesttext(old_t);
    set_action_object(F_EDIT, O_TEXT);
    set_modifiedflag();
}

void
change_spline(F_spline *old_s, F_spline *new_s)
{
    list_delete_spline(&objects.splines, old_s);
    list_add_spline(&objects.splines, new_s);
    clean_up();
    old_s->next = new_s;
    set_latestspline(old_s);
    set_action_object(F_EDIT, O_SPLINE);
    set_modifiedflag();
}

void
change_compound(F_compound *old_c, F_compound *new_c)
{
    list_delete_compound(&objects.compounds, old_c);
    list_add_compound(&objects.compounds, new_c);
    clean_up();
    old_c->next = new_c;
    set_latestcompound(old_c);
    set_action_object(F_EDIT, O_COMPOUND);
    set_modifiedflag();
}

/* find the tails of all the object lists */

void tail(F_compound *ob, F_compound *tails)
{
    F_arc	   *a;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;
    F_text	   *t;

    if (NULL != (a = ob->arcs))
	for (; a->next != NULL; a = a->next)
		;
    if (NULL != (c = ob->compounds))
	for (; c->next != NULL; c = c->next)
		;
    if (NULL != (e = ob->ellipses))
	for (; e->next != NULL; e = e->next)
		;
    if (NULL != (l = ob->lines))
	for (; l->next != NULL; l = l->next)
		;
    if (NULL != (s = ob->splines))
	for (; s->next != NULL; s = s->next)
		;
    if (NULL != (t = ob->texts))
	for (; t->next != NULL; t = t->next)
		;

    tails->arcs = a;
    tails->compounds = c;
    tails->ellipses = e;
    tails->lines = l;
    tails->splines = s;
    tails->texts = t;
}

/*
 * Make pointers in tails point to the last element of each list of l1 and
 * Append the lists in l2 after those in l1. The tails pointers must be
 * defined prior to calling append.
 */

void append_objects(F_compound *l1, F_compound *l2, F_compound *tails)
{
    /* don't forget to account for the depths */
    add_compound_depth(l2);

    if (tails->arcs)
	tails->arcs->next = l2->arcs;
    else
	l1->arcs = l2->arcs;
    if (tails->compounds)
	tails->compounds->next = l2->compounds;
    else
	l1->compounds = l2->compounds;
    if (tails->ellipses)
	tails->ellipses->next = l2->ellipses;
    else
	l1->ellipses = l2->ellipses;
    if (tails->lines)
	tails->lines->next = l2->lines;
    else
	l1->lines = l2->lines;
    if (tails->splines)
	tails->splines->next = l2->splines;
    else
	l1->splines = l2->splines;
    if (tails->texts)
	tails->texts->next = l2->texts;
    else
	l1->texts = l2->texts;
}

/* Cut is the dual of append. */

void cut_objects(F_compound *objects, F_compound *tails)
{
    if (tails->arcs) {
	remove_arc_depths(tails->arcs->next);
	tails->arcs->next = NULL;
    } else if (objects->arcs) {
	remove_arc_depths(objects->arcs);
	objects->arcs = NULL;
    }
    if (tails->compounds) {
	remove_compound_depth(tails->compounds->next);
	tails->compounds->next = NULL;
    } else if (objects->compounds) {
	remove_compound_depth(objects->compounds);
	objects->compounds = NULL;
    }
    if (tails->ellipses) {
	remove_ellipse_depths(tails->ellipses->next);
	tails->ellipses->next = NULL;
    } else if (objects->ellipses) {
	remove_ellipse_depths(objects->ellipses);
	objects->ellipses = NULL;
    }
    if (tails->lines) {
	remove_line_depths(tails->lines->next);
	tails->lines->next = NULL;
    } else if (objects->lines) {
	remove_line_depths(objects->lines);
	objects->lines = NULL;
    }
    if (tails->splines) {
	remove_spline_depths(tails->splines->next);
	tails->splines->next = NULL;
    } else if (objects->splines) {
	remove_spline_depths(objects->splines);
	objects->splines = NULL;
    }
    if (tails->texts) {
	remove_text_depths(tails->texts->next);
	tails->texts->next = NULL;
    } else if (objects->texts) {
	remove_text_depths(objects->texts);
	objects->texts = NULL;
    }
}

void remove_arc_depths(F_arc *a)
{
    for ( ; a; a= a->next)
	remove_depth(O_ARC, a->depth);
}

void remove_ellipse_depths(F_ellipse *e)
{
    for ( ; e; e = e->next)
	remove_depth(O_ELLIPSE, e->depth);
}

void remove_line_depths(F_line *l)
{
    for ( ; l; l = l->next)
	remove_depth(O_POLYLINE, l->depth);
}

void remove_spline_depths(F_spline *s)
{
    for ( ; s; s = s->next)
	remove_depth(O_SPLINE, s->depth);
}

void remove_text_depths(F_text *t)
{
    for ( ; t; t = t->next)
	remove_depth(O_TEXT, t->depth);
}

void append_point(int x, int y, F_point **point)    /** used in d_arcbox **/
       		         
           	          
{
    F_point	   *p;

    if ((p = create_point()) == NULL)
	return;

    p->x = x;
    p->y = y;
    p->next = NULL;
    (*point)->next = p;
    *point = p;
}

Boolean
insert_point(int x, int y, F_point *point)
{
    F_point	  *p;

    if ((p = create_point()) == NULL)
	return False;

    p->x = x;
    p->y = y;
    p->next = (point)->next;
    (point)->next = p;
    return True;
}

Boolean
append_sfactor(double s, F_sfactor *cpoint)
{
  F_sfactor *newpoint;

  if ((newpoint = create_sfactor()) == NULL)
    return False;
  newpoint->s = s;
  newpoint->next = cpoint->next;
  cpoint->next = newpoint;
  return True;
}


Boolean
first_spline_point(int x, int y, double s, F_spline *spline)
{
  F_point   *newpoint;
  F_sfactor *cpoint;
  
  if ((newpoint = create_point()) == NULL)
    return False;
  if ((cpoint = create_sfactor()) == NULL)
    return False;

  newpoint->x = x;
  newpoint->y = y;
  newpoint->next = spline->points;
  spline->points = newpoint;
  cpoint->s = s;
  cpoint->next = spline->sfactors;
  spline->sfactors = cpoint;
  return True;
}

int num_points(F_point *points)
{
    int		    n;
    F_point	   *p;

    for (p = points, n = 0; p != NULL; p = p->next, n++)
	    ;
    return (n);
}

F_text	       *
last_text(F_text *list)
{
    F_text	   *tt;

    if (list == NULL)
	return NULL;

    for (tt = list; tt->next != NULL; tt = tt->next)
	    ;
    return tt;
}

F_line	       *
last_line(F_line *list)
{
    F_line	   *ll;

    if (list == NULL)
	return NULL;

    for (ll = list; ll->next != NULL; ll = ll->next)
	    ;
    return ll;
}

F_spline       *
last_spline(F_spline *list)
{
    F_spline	   *ss;

    if (list == NULL)
	return NULL;

    for (ss = list; ss->next != NULL; ss = ss->next)
	    ;
    return ss;
}

F_arc	       *
last_arc(F_arc *list)
{
    F_arc	   *tt;

    if (list == NULL)
	return NULL;

    for (tt = list; tt->next != NULL; tt = tt->next)
	    ;
    return tt;
}

F_ellipse      *
last_ellipse(F_ellipse *list)
{
    F_ellipse	   *tt;

    if (list == NULL)
	return NULL;

    for (tt = list; tt->next != NULL; tt = tt->next)
	    ;
    return tt;
}

F_compound     *
last_compound(F_compound *list)
{
    F_compound	   *tt;

    if (list == NULL)
	return NULL;

    for (tt = list; tt->next != NULL; tt = tt->next)
	    ;
    return tt;
}

F_point	       *
last_point(F_point *list)
{
    F_point	   *tt;

    if (list == NULL)
	return NULL;

    for (tt = list; tt->next != NULL; tt = tt->next)
	    ;
    return tt;
}

F_sfactor       *
last_sfactor(F_sfactor *list)
{
    F_sfactor	   *tt;

    if (list == NULL)
	return NULL;

    for (tt = list; tt->next != NULL; tt = tt->next)
	    ;
    return tt;
}

F_point        *
search_line_point(F_line *line, int x, int y)
{
  F_point *point;

  for (point = line->points ; 
       point != NULL && (point->x != x || point->y != y); point = point->next);
  return point;
}

F_point        *
search_spline_point(F_spline *spline, int x, int y)
{
  F_point *point;

  for (point = spline->points ; 
       point != NULL && (point->x != x || point->y != y); point = point->next);
  return point;
}


F_sfactor      *
search_sfactor(F_spline *spline, F_point *selected_point)
{
  F_sfactor *c_point = spline->sfactors;
  F_point *cursor;

  for (cursor = spline->points ; cursor != selected_point ; 
       cursor = cursor->next)
    c_point = c_point->next;
  return c_point;
}


F_arc	       *
prev_arc(F_arc *list, F_arc *arc)
{
    F_arc	   *csr;

    if (list == arc)
	return NULL;

    for (csr = list; csr->next != arc; csr = csr->next)
	    ;
    return csr;
}

F_compound     *
prev_compound(F_compound *list, F_compound *compound)
{
    F_compound	   *csr;

    if (list == compound)
	return NULL;

    for (csr = list; csr->next != compound; csr = csr->next)
	    ;
    return csr;
}

F_ellipse      *
prev_ellipse(F_ellipse *list, F_ellipse *ellipse)
{
    F_ellipse	   *csr;

    if (list == ellipse)
	return NULL;

    for (csr = list; csr->next != ellipse; csr = csr->next)
	    ;
    return csr;
}

F_line	       *
prev_line(F_line *list, F_line *line)
{
    F_line	   *csr;

    if (list == line)
	return NULL;

    for (csr = list; csr->next != line; csr = csr->next)
	    ;
    return csr;
}

F_spline       *
prev_spline(F_spline *list, F_spline *spline)
{
    F_spline	   *csr;

    if (list == spline)
	return NULL;

    for (csr = list; csr->next != spline; csr = csr->next)
	    ;
    return csr;
}

F_text	       *
prev_text(F_text *list, F_text *text)
{
    F_text	   *csr;

    if (list == text)
	return NULL;

    for (csr = list; csr->next != text; csr = csr->next)
	    ;
    return csr;
}

F_point	       *
prev_point(F_point *list, F_point *point)
{
    F_point	   *csr;

    if (list == point)
	return NULL;

    for (csr = list; csr->next != point; csr = csr->next)
	    ;
    return csr;
}

int
object_count(F_compound *list)
{
    register int    cnt;
    F_arc	   *a;
    F_text	   *t;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;

    cnt = 0;
    for (a = list->arcs; a != NULL; a = a->next, cnt++)
	    ;
    for (t = list->texts; t != NULL; t = t->next, cnt++)
	    ;
    for (c = list->compounds; c != NULL; c = c->next, cnt++)
	    ;
    for (e = list->ellipses; e != NULL; e = e->next, cnt++)
	    ;
    for (l = list->lines; l != NULL; l = l->next, cnt++)
	    ;
    for (s = list->splines; s != NULL; s = s->next, cnt++)
	    ;
    return (cnt);
}

void set_tags(F_compound *list, int tag)
{
    F_arc	   *a;
    F_text	   *t;
    F_compound	   *c;
    F_ellipse	   *e;
    F_line	   *l;
    F_spline	   *s;

    for (a = list->arcs; a != NULL; a = a->next) {
	mask_toggle_arcmarker(a);
	a->tagged = tag;
	mask_toggle_arcmarker(a);
    }
    for (t = list->texts; t != NULL; t = t->next) {
	mask_toggle_textmarker(t);
	t->tagged = tag;
	mask_toggle_textmarker(t);
    }
    for (c = list->compounds; c != NULL; c = c->next) {
	mask_toggle_compoundmarker(c);
	c->tagged = tag;
	mask_toggle_compoundmarker(c);
    }
    for (e = list->ellipses; e != NULL; e = e->next) {
	mask_toggle_ellipsemarker(e);
	e->tagged = tag;
	mask_toggle_ellipsemarker(e);
    }
    for (l = list->lines; l != NULL; l = l->next) {
	mask_toggle_linemarker(l);
	l->tagged = tag;
	mask_toggle_linemarker(l);
    }
    for (s = list->splines; s != NULL; s = s->next) {
	mask_toggle_splinemarker(s);
	s->tagged = tag;
	mask_toggle_splinemarker(s);
    }
}

void
get_links(int llx, int lly, int urx, int ury)
{
    F_line	   *l;
    F_point	   *a;
    F_linkinfo	   *j, *k;

    j = NULL;
    for (l = objects.lines; l != NULL; l = l->next)
	if (l->type == T_POLYLINE) {
	    a = l->points;
	    if (point_on_perim(a, llx, lly, urx, ury)) {
		if ((k = new_link(l, a, a->next)) == NULL)
		    return;
		if (j == NULL)
		    cur_links = k;
		else
		    j->next = k;
		j = k;
		if (k->prevpt != NULL)
		    k->two_pts = (k->prevpt->next == NULL);
		continue;
	    }
	    if (a->next == NULL)/* single point, no need to check further */
		continue;
	    a = last_point(l->points);
	    if (point_on_perim(a, llx, lly, urx, ury)) {
		if ((k = new_link(l, a, prev_point(l->points, a))) == NULL)
		    return;
		if (j == NULL)
		    cur_links = k;
		else
		    j->next = k;
		j = k;
		if (k->prevpt != NULL)
		    k->two_pts = (prev_point(l->points, k->prevpt) == NULL);
		continue;
	    }
	}
}

static int LINK_TOL = 3 * PIX_PER_INCH / DISPLAY_PIX_PER_INCH;

int
point_on_perim(F_point *p, int llx, int lly, int urx, int ury)
{
    return ((abs(p->x - llx) <= LINK_TOL && p->y >= lly - LINK_TOL
	     && p->y <= ury + LINK_TOL) ||
	    (abs(p->x - urx) <= LINK_TOL && p->y >= lly - LINK_TOL
	     && p->y <= ury + LINK_TOL) ||
	    (abs(p->y - lly) <= LINK_TOL && p->x >= llx - LINK_TOL
	     && p->x <= urx + LINK_TOL) ||
	    (abs(p->y - ury) <= LINK_TOL && p->x >= llx - LINK_TOL
	     && p->x <= urx + LINK_TOL));
}
/*
** get_interior_links and point_on_inside, added by Ian Brown Feb 1995.
** This is to allow links within compound objects.
*/

void
get_interior_links(int llx, int lly, int urx, int ury)
{
    F_line	   *l;
    F_point	   *a;
    F_linkinfo	   *j, *k;

    j = NULL;
    for (l = objects.lines; l != NULL; l = l->next)
	if (l->type == T_POLYLINE) {
	    a = l->points;
	    if (point_on_inside(a, llx, lly, urx, ury)) {
		if ((k = new_link(l, a, a->next)) == NULL)
		    return;
		if (j == NULL)
		    cur_links = k;
		else
		    j->next = k;
		j = k;
		if (k->prevpt != NULL)
		    k->two_pts = (k->prevpt->next == NULL);
		continue;
	    }
	    if (a->next == NULL)/* single point, no need to check further */
		continue;
	    a = last_point(l->points);
	    if (point_on_inside(a, llx, lly, urx, ury)) {
		if ((k = new_link(l, a, prev_point(l->points, a))) == NULL)
		    return;
		if (j == NULL)
		    cur_links = k;
		else
		    j->next = k;
		j = k;
		if (k->prevpt != NULL)
		    k->two_pts = (prev_point(l->points, k->prevpt) == NULL);
		continue;
	    }
	}
}

int
point_on_inside(F_point *p, int llx, int lly, int urx, int ury)
{
    return ((p->x >= llx) && (p->x <= urx) &&
	    (p->y >= lly) && (p->y <= ury));
	    
}

void
adjust_links(int mode, F_linkinfo *links, int dx, int dy, int cx, int cy, float sx, float sy, Boolean copying)
       		         
              	          
       		           		/* delta */
       		           		/* center of scale - NOT USED YET */
         	           		/* scale factor - NOT USED YET */
           	            
{
    F_linkinfo	   *k;
    F_line	   *l;

    if (mode != SMART_OFF)
	for (k = links; k != NULL; k = k->next) {
	    if (copying) {
		l = copy_line(k->line);
		list_add_line(&objects.lines, l);
	    } else {
		mask_toggle_linemarker(k->line);
		draw_line(k->line, ERASE);
	    }
	    if (mode == SMART_SLIDE && k->prevpt != NULL) {
		if (k->endpt->x == k->prevpt->x)
		    k->prevpt->x += dx;
		else
		    k->prevpt->y += dy;
	    }
	    k->endpt->x += dx;
	    k->endpt->y += dy;
	    draw_line(k->line, PAINT);
	    mask_toggle_linemarker(k->line);
	}
}
