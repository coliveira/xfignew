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
#include "e_convert.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "d_spline.h"

#include "f_util.h"
#include "u_free.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "w_cursor.h"

static void	init_convert_line_spline(F_line *p, int type, int x, int y, int px, int py);
static void	init_convert_open_closed(F_line *obj, int type, int x, int y, F_point *p, F_point *q);



void convert_selected(void)
{
    set_mousefun("spline<->line", "", "open<->closed", LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_convert_line_spline);
    init_searchproc_right(init_convert_open_closed);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = null_proc;
    canvas_rightbut_proc = point_search_right;
    set_cursor(pick15_cursor);
    reset_action_on();
}

static void
init_convert_open_closed(F_line *obj, int type, int x, int y, F_point *p, F_point *q)
{
    switch (type) {
    case O_POLYLINE:
      cur_l = (F_line *) obj;
      toggle_polyline_polygon(cur_l, p, q);
	break;
    case O_SPLINE:
      cur_s = (F_spline *) obj;
      toggle_open_closed_spline(cur_s, p, q);
	break;
    default:
	return;
    }
}

static void
init_convert_line_spline(F_line *p, int type, int x, int y, int px, int py)
{
    static int flag = 0;

    switch (type) {
    case O_POLYLINE:
	cur_l = (F_line *) p;
	/* the search routine will ensure that we don't have a box */
	if (cur_l->type == T_POLYLINE || cur_l->type == T_POLYGON) {
	    line_spline(cur_l, cur_l->type == T_POLYGON ?
		    flag ? T_CLOSED_APPROX : T_CLOSED_INTERP :
		    flag ? T_OPEN_APPROX : T_OPEN_INTERP);
	} else if (cur_l->type == T_ARCBOX || cur_l->type == T_BOX) {
		box_2_box(cur_l);
	}
	break;
    case O_SPLINE:
	cur_s = (F_spline *) p;
	flag = (cur_s->type==T_OPEN_INTERP) || (cur_s->type==T_CLOSED_INTERP);
	spline_line(cur_s);
	break;
    default:
	return;
    }
}

/* handle conversion of box to arc_box and arc_box to box */

void
box_2_box(F_line *old_l)
{
    F_line	   *new_l;

    new_l = copy_line(old_l);
    switch (old_l->type) {
      case T_BOX:
	new_l->type = T_ARCBOX;
	if (new_l->radius == DEFAULT || new_l->radius == 0)
	    new_l->radius = cur_boxradius;
	break;
      case T_ARCBOX:
	new_l->type = T_BOX;
	break;
    }
    list_delete_line(&objects.lines, old_l);
    list_add_line(&objects.lines, new_l);
    clean_up();
    old_l->next = new_l;
    set_latestline(old_l);
    set_action_object(F_CONVERT, O_POLYLINE);
    set_modifiedflag();
    /* save pointer to this line for undo */
    latest_line = new_l;
    redisplay_line(new_l);
    return;
}

void
line_spline(F_line *l, int type_value)
{
    F_spline	   *s;

    if (num_points(l->points) < CLOSED_SPLINE_MIN_NUM_POINTS) {
	put_msg("Not enough points for a spline");
	beep();
	return;
    }

    if ((s = create_spline()) == NULL)
        return;
    s->type = type_value;

    if (l->type == T_POLYGON)
	s->points = copy_points(l->points->next);
    else
	s->points = copy_points(l->points);

    s->style = l->style;
    s->thickness = l->thickness;
    s->pen_color = l->pen_color;
    s->fill_color = l->fill_color;
    s->depth = l->depth;
    s->style_val = l->style_val;
    s->cap_style = l->cap_style;
    s->pen_style = l->pen_style;
    s->fill_style = l->fill_style;
    s->sfactors = NULL;
    s->next = NULL;

    if (l->for_arrow) {
	s->for_arrow = create_arrow();
	s->for_arrow->type = l->for_arrow->type;
	s->for_arrow->style = l->for_arrow->style;
	s->for_arrow->thickness = l->for_arrow->thickness;
	s->for_arrow->wd = l->for_arrow->wd;
	s->for_arrow->ht = l->for_arrow->ht;
    } else {
	s->for_arrow = NULL;
    }
    if (l->back_arrow) {
	s->back_arrow = create_arrow();
	s->back_arrow->type = l->back_arrow->type;
	s->back_arrow->style = l->back_arrow->style;
	s->back_arrow->thickness = l->back_arrow->thickness;
	s->back_arrow->wd = l->back_arrow->wd;
	s->back_arrow->ht = l->back_arrow->ht;
    } else {
	s->back_arrow = NULL;
    }
    /* A spline must have an s parameter for each point */
    if (!make_sfactors(s)) {
	free_spline(&s);
	return;
    }

    /* Get rid of the line and draw the new spline */
    delete_line(l);
    /* now put back the new spline */
    mask_toggle_splinemarker(s);
    list_add_spline(&objects.splines, s);
    redisplay_spline(s);
    set_action_object(F_CONVERT, O_POLYLINE);
    set_latestspline(s);
    set_modifiedflag();
}

void
spline_line(F_spline *s)
{
    F_line	   *l;
    F_point        *tmppoint;

    /* Now we turn s into a line */
    if ((l = create_line()) == NULL)
	return;

    if (open_spline(s)) {
	l->type = T_POLYLINE;
	l->points = s->points;
    } else {
	l->type = T_POLYGON;
	if ((l->points = create_point())==NULL)
	    return;
	tmppoint = last_point(s->points);
	l->points->x = tmppoint->x;
	l->points->y = tmppoint->y;
	l->points->next = copy_points(s->points);
    }
    l->style = s->style;
    l->thickness = s->thickness;
    l->pen_color = s->pen_color;
    l->fill_color = s->fill_color;
    l->depth = s->depth;
    l->style_val = s->style_val;
    l->cap_style = s->cap_style;
    l->join_style = cur_joinstyle;
    l->pen_style = s->pen_style;
    l->radius = DEFAULT;
    l->fill_style = s->fill_style;
    if (s->for_arrow) {
	l->for_arrow = create_arrow();
	l->for_arrow->type = s->for_arrow->type;
	l->for_arrow->style = s->for_arrow->style;
	l->for_arrow->thickness = s->for_arrow->thickness;
	l->for_arrow->wd = s->for_arrow->wd;
	l->for_arrow->ht = s->for_arrow->ht;
    } else {
	l->for_arrow = NULL;
    }
    if (s->back_arrow) {
	l->back_arrow = create_arrow();
	l->back_arrow->type = s->back_arrow->type;
	l->back_arrow->style = s->back_arrow->style;
	l->back_arrow->thickness = s->back_arrow->thickness;
	l->back_arrow->wd = s->back_arrow->wd;
	l->back_arrow->ht = s->back_arrow->ht;
    } else {
	l->back_arrow = NULL;
    }

    /* now we have finished creating the line, we can get rid of the spline */
    delete_spline(s);

    /* and put in the new line */
    mask_toggle_linemarker(l);
    list_add_line(&objects.lines, l);
    redisplay_line(l);
    set_action_object(F_CONVERT, O_SPLINE);
    set_latestline(l);
    set_modifiedflag();
    return;
}

void
toggle_polyline_polygon(F_line *line, F_point *previous_point, F_point *selected_point)
{
  F_point *point, *last_pt;
  
  last_pt = last_point(line->points);

  if (line->type == T_POLYLINE)
    {

      if (line->points->next == NULL || line->points->next->next == NULL) {
	  put_msg("Not enough points for a polygon");
	  beep();
	  return;  /* less than 3 points - don't close the polyline */
      }
      if ((point = create_point()) == NULL)
	  return;
      
      point->x = last_pt->x;
      point->y = last_pt->y;
      point->next = line->points;
      line->points = point;
      
      line->type = T_POLYGON;
      clean_up();
      set_last_arrows(line->for_arrow, line->back_arrow);
      line->back_arrow = line->for_arrow = NULL;
    }
  else if (line->type == T_POLYGON)
    {
      point = line->points;
      line->points = point->next;           /* unchain the first point */
      free((char *) point);
            
      if ((line->points != selected_point) && (previous_point != NULL))
	{
	  last_pt->next = line->points;     /* let selected point become */
	  previous_point->next = NULL;      /* first point */
	  line->points = selected_point;
	}
      line->type = T_POLYLINE;
      clean_up();
    }
  redisplay_line(line);
  set_action_object(F_OPEN_CLOSE, O_POLYLINE);
  set_last_selectedpoint(line->points);
  set_last_prevpoint(NULL);
  set_latestline(line);
  set_modifiedflag();
}


void
toggle_open_closed_spline(F_spline *spline, F_point *previous_point, F_point *selected_point)
{
  F_point *last_pt;
  F_sfactor *last_sfactor, *previous_sfactor, *selected_sfactor;

  if (spline->points->next == NULL || spline->points->next->next == NULL) {
      put_msg("Not enough points for a spline");
      beep();
      return;  /* less than 3 points - don't close the spline */
  }

  last_pt = last_point(spline->points);
  last_sfactor = search_sfactor(spline, last_pt);

  if (previous_point == NULL)
    {
      previous_sfactor = NULL;
      selected_sfactor = spline->sfactors;
    }
  else
    {
      previous_sfactor = search_sfactor(spline, previous_point);
      selected_sfactor = previous_sfactor->next;
      set_last_tension(selected_sfactor->s, previous_sfactor->s);
    }

  draw_spline(spline, ERASE);

  if (closed_spline(spline))
    {      
      if (spline->points != selected_point)
	{
	  last_pt->next = spline->points;
	  last_sfactor->next = spline->sfactors;
	  previous_point->next = NULL;
	  previous_sfactor->next = NULL;
	  previous_sfactor->s = S_SPLINE_ANGULAR;
	  spline->points = selected_point;
	  spline->sfactors = selected_sfactor;
	}
      else
	{
	  last_sfactor->s = S_SPLINE_ANGULAR;
	}
      spline->sfactors->s = S_SPLINE_ANGULAR;
      spline->type = (x_spline(spline)) ? T_OPEN_XSPLINE :
	(int_spline(spline)) ? T_OPEN_INTERP : T_OPEN_APPROX;
      clean_up();
    }
  else
    {
      int type_tmp;
      double s_tmp;

      if(int_spline(spline))
	{
	  s_tmp = S_SPLINE_INTERP;
	  type_tmp = T_CLOSED_INTERP;
	}
      else
	if (x_spline(spline))
	  {
	      s_tmp = S_SPLINE_INTERP;
	      type_tmp = T_CLOSED_XSPLINE;
	    }
	else
	  {
	    s_tmp = S_SPLINE_APPROX;
	    type_tmp = T_CLOSED_APPROX;
	  }
      spline->sfactors->s = last_sfactor->s = s_tmp;
      spline->type = type_tmp;
      clean_up();
      set_last_arrows(spline->for_arrow, spline->back_arrow);
      spline->back_arrow = spline->for_arrow = NULL;
    }
  draw_spline(spline, PAINT);
  set_action_object(F_OPEN_CLOSE, O_SPLINE);
  set_last_selectedpoint(spline->points);
  set_last_prevpoint(NULL);
  set_latestspline(spline);
  set_modifiedflag();
}

