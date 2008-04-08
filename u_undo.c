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

/**************** IMPORTS ****************/

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "e_addpt.h"
#include "e_arrow.h"
#include "e_convert.h"
#include "u_draw.h"
#include "u_elastic.h"
#include "u_list.h"
#include "u_redraw.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_setup.h"

#include "e_deletept.h"
#include "e_scale.h"
#include "f_read.h"
#include "u_bound.h"
#include "u_free.h"
#include "u_markers.h"
#include "u_translate.h"
#include "w_cmdpanel.h"
#include "w_indpanel.h"
#include "w_color.h"

/*************** EXPORTS *****************/

/*
 * Object_tails *usually* points to the last object in each linked list in
 * objects.  The exceptions occur when multiple objects are added to a figure
 * (e.g. file read, break compound, undo delete region).  In these cases,
 * the added objects are appended to the object lists (and saved_objects is
 * set up to point to the new objects) but object_tails is not changed.
 * This speeds up a subsequent undo operation which need only set
 * all the "next" fields of objects pointed to by object_tails to NULL.
 */

F_compound	saved_objects = {0, 0, { 0, 0 }, { 0, 0 }, 
				NULL, NULL, NULL, NULL, NULL, NULL, NULL};
F_compound	object_tails = {0, 0, { 0, 0 }, { 0, 0 }, 
				NULL, NULL, NULL, NULL, NULL, NULL, NULL};
F_arrow		*saved_for_arrow = (F_arrow *) NULL;
F_arrow		*saved_back_arrow = (F_arrow *) NULL;
F_line		*latest_line;		/* for undo_join (line) */
F_spline	*latest_spline;		/* for undo_join (spline) */

int		last_action = F_NULL;

/*************** LOCAL *****************/

static int	last_object;
static F_pos	last_position, new_position;
static int	last_arcpointnum;
static F_point *last_prev_point, *last_selected_point, *last_next_point;
static F_sfactor  *last_selected_sfactor;
static F_linkinfo *last_links;
static F_arrow    *last_for_arrow, *last_back_arrow;
static int	last_linkmode;
static double	last_origin_tension, last_extremity_tension;


void undo_add (void);
void undo_delete (void);
void undo_move (void);
void undo_change (void);
void undo_glue (void);
void undo_break (void);
void undo_load (void);
void undo_scale (void);
void undo_addpoint (void);
void undo_deletepoint (void);
void undo_add_arrowhead (void);
void undo_delete_arrowhead (void);
void undo_convert (void);
void undo_open_close (void);
void undo_join_split (void);
void set_action_object (int action, int object);
void swap_newp_lastp (void);

void
undo(void)
{
    /* turn off Compose key LED */
    setCompLED(0);

    switch (last_action) {
      case F_ADD:
	undo_add();
	break;
      case F_DELETE:
	undo_delete();
	break;
      case F_MOVE:
	undo_move();
	break;
      case F_EDIT:
	undo_change();
	break;
      case F_GLUE:
	undo_glue();
	break;
      case F_BREAK:
	undo_break();
	break;
      case F_LOAD:
	undo_load();
	break;
      case F_SCALE:
	undo_scale();
	break;
      case F_ADD_POINT:
	undo_addpoint();
	break;
      case F_DELETE_POINT:
	undo_deletepoint();
	break;
      case F_ADD_ARROW_HEAD:
	undo_add_arrowhead();
	break;
      case F_DELETE_ARROW_HEAD:
	undo_delete_arrowhead();
	break;
      case F_CONVERT:
	undo_convert();
	break;
      case F_OPEN_CLOSE:
	undo_open_close();
	break;
      case F_JOIN:
      case F_SPLIT:
	undo_join_split();
	break;
    default:
	put_msg("Nothing to UNDO");
	return;
    }
    put_msg("Undo complete");
}

void undo_join_split(void)
{
    F_line	    swp_l;
    F_spline	    swp_s;
    if (last_object == O_POLYLINE) {
	new_l = saved_objects.lines;		/* the original */
	old_l = latest_line;			/* the changed object */
	/* swap old with new */
	bcopy((char*)old_l, (char*)&swp_l, sizeof(F_line));
	bcopy((char*)new_l, (char*)old_l, sizeof(F_line));
	bcopy((char*)&swp_l, (char*)new_l, sizeof(F_line));
	/* this assumes that the object are at the end of the objects list */
	/* correct the depth counts if necessary */
	if (!new_l->next && old_l->next) add_depth(O_POLYLINE, old_l->depth);
	else if (new_l->next && !old_l->next) remove_depth(O_POLYLINE, old_l->depth);
	set_action_object(F_JOIN, O_POLYLINE);
	redisplay_lines(new_l, old_l);
    } else {
	new_s = saved_objects.splines;		/* the original */
	old_s = latest_spline;			/* the changed object */
	/* swap old with new */
	bcopy((char*)old_s, (char*)&swp_s, sizeof(F_spline));
	bcopy((char*)new_s, (char*)old_s, sizeof(F_spline));
	bcopy((char*)&swp_s, (char*)new_s, sizeof(F_spline));
	/* this assumes that the object are at the end of the objects list */
	/* correct the depth counts if necessary */
	if (!new_s->next && old_s->next) add_depth(O_SPLINE, old_s->depth);
	else if (new_s->next && !old_s->next) remove_depth(O_SPLINE, old_s->depth);
	set_action_object(F_JOIN, O_SPLINE);
	redisplay_splines(new_s, old_s);
    }
}

void undo_addpoint(void)
{
    if (last_object == O_POLYLINE)
	linepoint_deleting(saved_objects.lines, last_prev_point,
			   last_selected_point);
    else
	splinepoint_deleting(saved_objects.splines, last_prev_point,
			     last_selected_point);
}

void undo_deletepoint(void)
{
    last_action = F_NULL;	/* to avoid doing a clean-up during adding */

    if (last_object == O_POLYLINE) {
	linepoint_adding(saved_objects.lines, last_prev_point,
			 last_selected_point);
	/* turn back on all relevant markers */
	update_markers(new_objmask);

    } else {	/* last_object is a spline */
	splinepoint_adding(saved_objects.splines, last_prev_point,
			 last_selected_point, last_next_point,
			 last_selected_sfactor->s);
    }

    last_next_point = NULL;
}

void undo_break(void)
{
    cut_objects(&objects, &object_tails);
    /* remove the depths from this compound because they'll be added in right after */
    remove_compound_depth(saved_objects.compounds);
    list_add_compound(&objects.compounds, saved_objects.compounds);
    last_action = F_GLUE;
    toggle_markers_in_compound(saved_objects.compounds);
    mask_toggle_compoundmarker(saved_objects.compounds);
}

void undo_glue(void)
{
    list_delete_compound(&objects.compounds, saved_objects.compounds);
    tail(&objects, &object_tails);
    append_objects(&objects, saved_objects.compounds, &object_tails);
    /* add the depths from this compound because they weren't added by the append_objects() */
    add_compound_depth(saved_objects.compounds);
    last_action = F_BREAK;
    mask_toggle_compoundmarker(saved_objects.compounds);
    toggle_markers_in_compound(saved_objects.compounds);
    if (cur_mode != F_GLUE && cur_mode != F_BREAK)
	set_tags(saved_objects.compounds, 0);
}

void undo_convert(void)
{
    switch (last_object) {
      case O_POLYLINE:
	if (saved_objects.lines->type == T_BOX ||
	    saved_objects.lines->type == T_ARCBOX)
		box_2_box(latest_line);
	else
		spline_line(saved_objects.splines);
	break;
      case O_SPLINE:
	line_spline(saved_objects.lines, saved_objects.splines->type);
	break;
    }
}

void undo_add_arrowhead(void)
{
    switch (last_object) {
      case O_POLYLINE:
	delete_linearrow(saved_objects.lines,
			 last_prev_point, last_selected_point);
	break;
      case O_SPLINE:
	delete_splinearrow(saved_objects.splines,
			   last_prev_point, last_selected_point);
	break;
      case O_ARC:
	delete_arcarrow(saved_objects.arcs, last_arcpointnum);
	break;
      default:
	return;
    }
    last_action = F_DELETE_ARROW_HEAD;
}

void undo_delete_arrowhead(void)
{
    switch (last_object) {
      case O_POLYLINE:
	if (saved_for_arrow)
	    saved_objects.lines->for_arrow = saved_for_arrow;
	if (saved_back_arrow)
	    saved_objects.lines->back_arrow = saved_back_arrow;
	redisplay_line(saved_objects.lines);
	break;
      case O_SPLINE:
	if (saved_for_arrow)
	    saved_objects.splines->for_arrow = saved_for_arrow;
	if (saved_back_arrow)
	    saved_objects.splines->back_arrow = saved_back_arrow;
	redisplay_spline(saved_objects.splines);
	break;
      case O_ARC:
	if (saved_for_arrow)
	    saved_objects.arcs->for_arrow = saved_for_arrow;
	if (saved_back_arrow)
	    saved_objects.arcs->back_arrow = saved_back_arrow;
	redisplay_arc(saved_objects.arcs);
	break;
      default:
	return;
    }
    last_action = F_ADD_ARROW_HEAD;
}

/*
 * saved_objects.xxxx contains a pointer to the original object,
 * saved_objects.xxxx->next points to the changed object.
 */

void undo_change(void)
{
    char	   *swp_comm;
    F_compound	    swp_c;
    F_line	    swp_l;
    F_spline	    swp_s;
    F_ellipse	    swp_e;
    F_arc	    swp_a;
    F_text	    swp_t;

    last_action = F_NULL;	/* to avoid a clean-up during "unchange" */
    switch (last_object) {
      case O_POLYLINE:
	new_l = saved_objects.lines;		/* the original */
	old_l = saved_objects.lines->next;	/* the changed object */
	/* account for depths */
	remove_depth(O_POLYLINE, old_l->depth);
	add_depth(O_POLYLINE, new_l->depth);
	/* swap old with new */
	bcopy((char*)old_l, (char*)&swp_l, sizeof(F_line));
	bcopy((char*)new_l, (char*)old_l, sizeof(F_line));
	bcopy((char*)&swp_l, (char*)new_l, sizeof(F_line));
	/* but keep the next pointers unchanged */
	swp_l.next = old_l->next;
	old_l->next = new_l->next;
	new_l->next = swp_l.next;
	set_action_object(F_EDIT, O_POLYLINE);
	redisplay_lines(new_l, old_l);
	break;
      case O_ELLIPSE:
	new_e = saved_objects.ellipses;
	old_e = saved_objects.ellipses->next;
	/* account for depths */
	remove_depth(O_ELLIPSE, old_e->depth);
	add_depth(O_ELLIPSE, new_e->depth);
	/* swap old with new */
	bcopy((char*)old_e, (char*)&swp_e, sizeof(F_ellipse));
	bcopy((char*)new_e, (char*)old_e, sizeof(F_ellipse));
	bcopy((char*)&swp_e, (char*)new_e, sizeof(F_ellipse));
	/* but keep the next pointers unchanged */
	swp_e.next = old_e->next;
	old_e->next = new_e->next;
	new_e->next = swp_e.next;
	set_action_object(F_EDIT, O_ELLIPSE);
	redisplay_ellipses(new_e, old_e);
	break;
      case O_TEXT:
	new_t = saved_objects.texts;
	old_t = saved_objects.texts->next;
	/* account for depths */
	remove_depth(O_TEXT, old_t->depth);
	add_depth(O_TEXT, new_t->depth);
	/* swap old with new */
	bcopy((char*)old_t, (char*)&swp_t, sizeof(F_text));
	bcopy((char*)new_t, (char*)old_t, sizeof(F_text));
	bcopy((char*)&swp_t, (char*)new_t, sizeof(F_text));
	/* but keep the next pointers unchanged */
	swp_t.next = old_t->next;
	old_t->next = new_t->next;
	new_t->next = swp_t.next;
	set_action_object(F_EDIT, O_TEXT);
	redisplay_texts(new_t, old_t);
	break;
      case O_SPLINE:
	new_s = saved_objects.splines;
	old_s = saved_objects.splines->next;
	/* account for depths */
	remove_depth(O_SPLINE, old_s->depth);
	add_depth(O_SPLINE, new_s->depth);
	/* swap old with new */
	bcopy((char*)old_s, (char*)&swp_s, sizeof(F_spline));
	bcopy((char*)new_s, (char*)old_s, sizeof(F_spline));
	bcopy((char*)&swp_s, (char*)new_s, sizeof(F_spline));
	/* but keep the next pointers unchanged */
	swp_s.next = old_s->next;
	old_s->next = new_s->next;
	new_s->next = swp_s.next;
	set_action_object(F_EDIT, O_SPLINE);
	redisplay_splines(new_s, old_s);
	break;
      case O_ARC:
	new_a = saved_objects.arcs;
	old_a = saved_objects.arcs->next;
	/* account for depths */
	remove_depth(O_ARC, old_a->depth);
	add_depth(O_ARC, new_a->depth);
	/* swap old with new */
	bcopy((char*)old_a, (char*)&swp_a, sizeof(F_arc));
	bcopy((char*)new_a, (char*)old_a, sizeof(F_arc));
	bcopy((char*)&swp_a, (char*)new_a, sizeof(F_arc));
	/* but keep the next pointers unchanged */
	swp_a.next = old_a->next;
	old_a->next = new_a->next;
	new_a->next = swp_a.next;
	set_action_object(F_EDIT, O_ARC);
	redisplay_arcs(new_a, old_a);
	break;
      case O_COMPOUND:
	new_c = saved_objects.compounds;
	old_c = saved_objects.compounds->next;
	/* account for depths */
	remove_compound_depth(old_c);
	add_compound_depth(new_c);
	/* swap old with new */
	bcopy((char*)old_c, (char*)&swp_c, sizeof(F_compound));
	bcopy((char*)new_c, (char*)old_c, sizeof(F_compound));
	bcopy((char*)&swp_c, (char*)new_c, sizeof(F_compound));
	/* but keep the next pointers unchanged */
	swp_c.next = old_c->next;
	old_c->next = new_c->next;
	new_c->next = swp_c.next;
	set_action_object(F_EDIT, O_COMPOUND);
	redisplay_compounds(new_c, old_c);
	break;
      case O_FIGURE:
	/* swap saved figure comments with current */
	swp_comm = objects.comments;
	objects.comments = saved_objects.comments;
	saved_objects.comments = swp_comm;
	set_action_object(F_EDIT, O_FIGURE);
	break;
      case O_ALL_OBJECT:
	swp_c = objects;
	objects = saved_objects;
	saved_objects = swp_c;
	new_c = &objects;
	old_c = &saved_objects;
	/* account for depths */
	remove_compound_depth(old_c);
	add_compound_depth(new_c);
	set_action_object(F_EDIT, O_ALL_OBJECT);
	set_modifiedflag();
	redisplay_zoomed_region(0, 0, BACKX(CANVAS_WD), BACKY(CANVAS_HT));
	break;
    }
}

/*
 * When a single object is created, it is appended to the appropriate list
 * in objects.	It is also placed in the appropriate list in saved_objects.
 *
 * When a number of objects are created (usually by reading them in from
 * a file or undoing a remove-all action), they are appended to the lists in
 * objects and also saved in saved_objects.  The pointers in object_tails
 * will be set to point to the last members of the lists in objects prior to
 * the appending.
 *
 * Note: The read operation will set the pointers in object_tails while the
 * remove-all operation will zero pointers in objects.
 */

void undo_add(void)
{
    int		    xmin, ymin, xmax, ymax;
    char	    ctemp[PATH_MAX];

    switch (last_object) {
      case O_POLYLINE:
	list_delete_line(&objects.lines, saved_objects.lines);
	redisplay_line(saved_objects.lines);
	break;
      case O_ELLIPSE:
	list_delete_ellipse(&objects.ellipses, saved_objects.ellipses);
	redisplay_ellipse(saved_objects.ellipses);
	break;
      case O_TEXT:
	list_delete_text(&objects.texts, saved_objects.texts);
	redisplay_text(saved_objects.texts);
	break;
      case O_SPLINE:
	list_delete_spline(&objects.splines, saved_objects.splines);
	redisplay_spline(saved_objects.splines);
	break;
      case O_ARC:
	list_delete_arc(&objects.arcs, saved_objects.arcs);
	redisplay_arc(saved_objects.arcs);
	break;
      case O_COMPOUND:
	list_delete_compound(&objects.compounds, saved_objects.compounds);
	redisplay_compound(saved_objects.compounds);
	break;
      case O_ALL_OBJECT:
	cut_objects(&objects, &object_tails);
	compound_bound(&saved_objects, &xmin, &ymin, &xmax, &ymax);
	redisplay_zoomed_region(xmin, ymin, xmax, ymax);
	/* restore filename if necessary (from undo of "New" command) */
	if (save_filename[0] != '\0') {
	    strcpy(ctemp, cur_filename);
	    update_cur_filename(save_filename);
	    strcpy(save_filename, ctemp);
	}
	break;
    }
    last_action = F_DELETE;
}

void undo_delete(void)
{
    int		    xmin, ymin, xmax, ymax;
    char	    ctemp[PATH_MAX];

    switch (last_object) {
      case O_POLYLINE:
	list_add_line(&objects.lines, saved_objects.lines);
	redisplay_line(saved_objects.lines);
	break;
      case O_ELLIPSE:
	list_add_ellipse(&objects.ellipses, saved_objects.ellipses);
	redisplay_ellipse(saved_objects.ellipses);
	break;
      case O_TEXT:
	list_add_text(&objects.texts, saved_objects.texts);
	redisplay_text(saved_objects.texts);
	break;
      case O_SPLINE:
	list_add_spline(&objects.splines, saved_objects.splines);
	redisplay_spline(saved_objects.splines);
	break;
      case O_ARC:
	list_add_arc(&objects.arcs, saved_objects.arcs);
	redisplay_arc(saved_objects.arcs);
	break;
      case O_COMPOUND:
	list_add_compound(&objects.compounds, saved_objects.compounds);
	redisplay_compound(saved_objects.compounds);
	break;
      case O_ALL_OBJECT:
	saved_objects.next = NULL;
	compound_bound(&saved_objects, &xmin, &ymin, &xmax, &ymax);
	tail(&objects, &object_tails);
	append_objects(&objects, &saved_objects, &object_tails);
	redisplay_zoomed_region(xmin, ymin, xmax, ymax);
	/* restore filename if necessary (from "New" command) */
	if (save_filename[0] != '\0') {
	    strcpy(ctemp, cur_filename);
	    update_cur_filename(save_filename);
	    strcpy(save_filename, ctemp);
	}
    }
    last_action = F_ADD;
}

void undo_move(void)
{
    int		    dx, dy;
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;
    int		    dum;

    dx = last_position.x - new_position.x;
    dy = last_position.y - new_position.y;
    switch (last_object) {
      case O_POLYLINE:
	line_bound(saved_objects.lines, &xmin1, &ymin1, &xmax1, &ymax1);
	translate_line(saved_objects.lines, dx, dy);
	line_bound(saved_objects.lines, &xmin2, &ymin2, &xmax2, &ymax2);
	adjust_links(last_linkmode, last_links, dx, dy, 0, 0, 1.0, 1.0, False);
	redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
	break;
      case O_ELLIPSE:
	ellipse_bound(saved_objects.ellipses, &xmin1, &ymin1, &xmax1, &ymax1);
	translate_ellipse(saved_objects.ellipses, dx, dy);
	ellipse_bound(saved_objects.ellipses, &xmin2, &ymin2, &xmax2, &ymax2);
	redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
	break;
      case O_TEXT:
	text_bound(saved_objects.texts, &xmin1, &ymin1, &xmax1, &ymax1,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	translate_text(saved_objects.texts, dx, dy);
	text_bound(saved_objects.texts, &xmin2, &ymin2, &xmax2, &ymax2,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
	break;
      case O_SPLINE:
	spline_bound(saved_objects.splines, &xmin1, &ymin1, &xmax1, &ymax1);
	translate_spline(saved_objects.splines, dx, dy);
	spline_bound(saved_objects.splines, &xmin2, &ymin2, &xmax2, &ymax2);
	redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
	break;
      case O_ARC:
	arc_bound(saved_objects.arcs, &xmin1, &ymin1, &xmax1, &ymax1);
	translate_arc(saved_objects.arcs, dx, dy);
	arc_bound(saved_objects.arcs, &xmin2, &ymin2, &xmax2, &ymax2);
	redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
	break;
      case O_COMPOUND:
	compound_bound(saved_objects.compounds, &xmin1, &ymin1, &xmax1, &ymax1);
	translate_compound(saved_objects.compounds, dx, dy);
	compound_bound(saved_objects.compounds, &xmin2, &ymin2, &xmax2, &ymax2);
	adjust_links(last_linkmode, last_links, dx, dy, 0, 0, 1.0, 1.0, False);
	redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
	break;
    }
    swap_newp_lastp();
}

void undo_load(void)
{
    F_compound	    temp;
    char	    ctemp[PATH_MAX];

    /* swap objects in current figure/figure we're restoring */
    temp = objects;
    objects = saved_objects;
    saved_objects = temp;
    /* swap filenames */
    strcpy(ctemp, cur_filename);
    update_cur_filename(save_filename);
    strcpy(save_filename, ctemp);
    /* restore colors for the figure we are restoring */
    swap_colors();
    colors_are_swapped = False;
    /* in case current figure doesn't have the colors shown in the fill/pen colors */
    current_memory = -1;
    show_pencolor();
    show_fillcolor();
    /* redisply that figure */
    redisplay_canvas();
    last_action = F_LOAD;
}

void undo_scale(void)
{
    float	    scalex, scaley;
    int		    xmin1, ymin1, xmax1, ymax1;
    int		    xmin2, ymin2, xmax2, ymax2;

    compound_bound(saved_objects.compounds, &xmin1, &ymin1, &xmax1, &ymax1);
    scalex = ((float) (last_position.x - fix_x)) / (new_position.x - fix_x);
    scaley = ((float) (last_position.y - fix_y)) / (new_position.y - fix_y);
    scale_compound(saved_objects.compounds, scalex, scaley, fix_x, fix_y);
    compound_bound(saved_objects.compounds, &xmin2, &ymin2, &xmax2, &ymax2);
    redisplay_regions(xmin1, ymin1, xmax1, ymax1,
			  xmin2, ymin2, xmax2, ymax2);
    swap_newp_lastp();
}

void undo_open_close(void)
{
  switch (last_object) {
  case O_POLYLINE:
    if (saved_objects.lines->type == T_POLYGON) {
	saved_objects.lines->for_arrow = last_for_arrow;
	saved_objects.lines->back_arrow = last_back_arrow;
	last_for_arrow = last_back_arrow = NULL;
    }
    toggle_polyline_polygon(saved_objects.lines, last_prev_point,
			    last_selected_point);   
    break;
  case O_SPLINE:
    if (saved_objects.splines->type == T_OPEN_XSPLINE) {
	F_sfactor *c_tmp;

	draw_spline(saved_objects.splines, ERASE);
	saved_objects.splines->sfactors->s = last_origin_tension;
	for (c_tmp=saved_objects.splines->sfactors ; c_tmp->next != NULL ;
	    c_tmp=c_tmp->next)
		;
	c_tmp->s = last_extremity_tension;
	saved_objects.splines->type = T_CLOSED_XSPLINE;
	draw_spline(saved_objects.splines, PAINT);
    } else {
	if (closed_spline(saved_objects.splines)) {
	    saved_objects.splines->for_arrow = last_for_arrow;
	    saved_objects.splines->back_arrow = last_back_arrow;
	    last_for_arrow = last_back_arrow = NULL;
	  }
	toggle_open_closed_spline(saved_objects.splines, last_prev_point,
				  last_selected_point);
    }
    break;
  }   
}

void swap_newp_lastp(void)
{
    int		    t;		/* swap new_position and last_position	*/

    t = new_position.x;
    new_position.x = last_position.x;
    last_position.x = t;
    t = new_position.y;
    new_position.y = last_position.y;
    last_position.y = t;
}

/*
 * Clean_up should be called before committing a user's request. Clean_up
 * will attempt to free all the allocated memories which resulted from
 * delete/remove action.  It will set the last_action to F_NULL.  Thus this
 * routine should be before set_action_object() and set_last_arrows(),
 * if they are to be called in the same routine.
 */

void clean_up(void)
{
    if (last_action == F_EDIT) {
	switch (last_object) {
	  case O_ARC:
	    saved_objects.arcs->next = NULL;
	    free_arc(&saved_objects.arcs);
	    break;
	  case O_COMPOUND:
	    saved_objects.compounds->next = NULL;
	    free_compound(&saved_objects.compounds);
	    break;
	  case O_ELLIPSE:
	    saved_objects.ellipses->next = NULL;
	    free_ellipse(&saved_objects.ellipses);
	    break;
	  case O_POLYLINE:
	    saved_objects.lines->next = NULL;
	    free_line(&saved_objects.lines);
	    break;
	  case O_SPLINE:
	    saved_objects.splines->next = NULL;
	    free_spline(&saved_objects.splines);
	    break;
	  case O_TEXT:
	    saved_objects.texts->next = NULL;
	    free_text(&saved_objects.texts);
	    break;
	  case O_FIGURE:
	    free((char *) saved_objects.comments);
	    break;
	}
    } else if (last_action==F_DELETE || last_action==F_JOIN || last_action==F_SPLIT) {
	switch (last_object) {
	  case O_ARC:
	    free_arc(&saved_objects.arcs);
	    break;
	  case O_COMPOUND:
	    free_compound(&saved_objects.compounds);
	    break;
	  case O_ELLIPSE:
	    free_ellipse(&saved_objects.ellipses);
	    break;
	  case O_POLYLINE:
	    free_line(&saved_objects.lines);
	    break;
	  case O_SPLINE:
	    free_spline(&saved_objects.splines);
	    break;
	  case O_TEXT:
	    free_text(&saved_objects.texts);
	    break;
	  case O_ALL_OBJECT:
	    free_arc(&saved_objects.arcs);
	    free_compound(&saved_objects.compounds);
	    free_ellipse(&saved_objects.ellipses);
	    free_line(&saved_objects.lines);
	    free_spline(&saved_objects.splines);
	    free_text(&saved_objects.texts);
	    break;
	}
    } else if (last_action == F_DELETE_POINT || last_action == F_ADD_POINT) {
	if (last_action == F_DELETE_POINT) {
/**************************************************
	    free((char *) last_selected_point);
	    free((char *) last_selected_sfactor);
**************************************************/
	    last_next_point = NULL;
	}
	last_prev_point = NULL;
	last_selected_point = NULL;
	saved_objects.arcs = NULL;
	saved_objects.compounds = NULL;
	saved_objects.ellipses = NULL;
	saved_objects.lines = NULL;
	saved_objects.splines = NULL;
	saved_objects.texts = NULL;
    } else if (last_action == F_LOAD) {
	free_arc(&saved_objects.arcs);
	free_compound(&saved_objects.compounds);
	free_ellipse(&saved_objects.ellipses);
	free_line(&saved_objects.lines);
	free_spline(&saved_objects.splines);
	free_text(&saved_objects.texts);
    } else if (last_action == F_GLUE) {
	saved_objects.compounds = NULL;
    } else if (last_action == F_BREAK) {
	free((char *) saved_objects.compounds);
	saved_objects.compounds = NULL;
    } else if (last_action == F_ADD || last_action == F_MOVE) {
	saved_objects.arcs = NULL;
	saved_objects.compounds = NULL;
	saved_objects.ellipses = NULL;
	saved_objects.lines = NULL;
	saved_objects.splines = NULL;
	saved_objects.texts = NULL;
	free_linkinfo(&last_links);
    } else if (last_action == F_CONVERT) {
	if (last_object == O_POLYLINE)
	    saved_objects.splines = NULL;
	else
	    saved_objects.lines = NULL;
    } else if (last_action == F_OPEN_CLOSE) {
        saved_objects.splines = NULL;
        saved_objects.lines = NULL;
	free((char *) last_for_arrow);
	free((char *) last_back_arrow);
    } else if (last_action == F_ADD_ARROW_HEAD ||
	       last_action == F_DELETE_ARROW_HEAD) {
	saved_objects.splines = NULL;
	saved_objects.lines = NULL;
	saved_objects.arcs = NULL;
	last_prev_point = NULL;
	last_selected_point = NULL;
    }
    last_action = F_NULL;
}

void set_latestarc(F_arc *arc)
{
    saved_objects.arcs = arc;
}

void set_latestobjects(F_compound *objects)
{
    saved_objects = *objects;
}

void set_latestcompound(F_compound *compound)
{
    saved_objects.compounds = compound;
}

void set_latestellipse(F_ellipse *ellipse)
{
    saved_objects.ellipses = ellipse;
}

void set_latestline(F_line *line)
{
    saved_objects.lines = line;
}

void set_latestspline(F_spline *spline)
{
    saved_objects.splines = spline;
}

void set_latesttext(F_text *text)
{
    saved_objects.texts = text;
}

void set_last_prevpoint(F_point *prev_point)
{
    last_prev_point = prev_point;
}

void set_last_selectedpoint(F_point *selected_point)
{
    last_selected_point = selected_point;
}

void set_last_selectedsfactor(F_sfactor *selected_sfactor)
{
  last_selected_sfactor = selected_sfactor;
}

void set_last_nextpoint(F_point *next_point)
{
    last_next_point = next_point;
}

void set_last_arcpointnum(int num)
{
    last_arcpointnum = num;
}

void set_lastposition(int x, int y)
{
    last_position.x = x;
    last_position.y = y;
}

void set_newposition(int x, int y)
{
    new_position.x = x;
    new_position.y = y;
}

void set_action(int action)
{
    last_action = action;
}

void set_action_object(int action, int object)
{
    last_action = action;
    last_object = object;
}

void set_lastlinkinfo(int mode, F_linkinfo *links)
{
    last_linkmode = mode;
    last_links = links;
}

void set_last_tension(double origin, double extremity)
{
  last_origin_tension = origin;
  last_extremity_tension = extremity;
}

void set_last_arrows(F_arrow *forward, F_arrow *backward)
{
      last_for_arrow = forward;
      last_back_arrow = backward;
}
