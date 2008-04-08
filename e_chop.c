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


/* >>>>>>>>>>>>>>>>>>> fixme -- don't forget undo ! <<<<<<<<<<<<<<<< */

#include <stdlib.h>
#include <alloca.h>
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_modepanel.h"
#include "w_zoom.h"
#include "w_snap.h"
#include "w_intersect.h"

static void select_axe_object();
static void select_log_object();
static void clear_axe_objects();

typedef struct {
  void * object;
  int type;
} axe_objects_s;

static axe_objects_s * axe_objects = NULL;
static int axe_objects_next = 0;
static int axe_objects_max = 0;
#define AXE_OBJECTS_INCR 8

typedef enum {
  PTYPE_NONE,
  PTYPE_START_PLINE,
  PTYPE_START_VERTEX,
  PTYPE_END_VERTEX,
  PTYPE_END_PLINE,
  PTYPE_CUT
} ptype_e;
  
typedef struct {
  int x;
  int y;
  ptype_e ptype;
  double dist;
} s_point_s;
  
typedef struct {
  s_point_s * points;
  int points_next;
  int points_max;
} l_point_s;
#define POINTS_INCR	8  

void
chop_selected(void)
{
    set_mousefun("Select axe object", "Select log object", "Clear axe list", 
			LOC_OBJ, LOC_OBJ, LOC_OBJ);
    draw_mousefun_canvas();
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(select_axe_object);
    init_searchproc_middle(select_log_object);
    /*    init_searchproc_right(init_chop_right); */	/* fixme don't need this now */
    canvas_leftbut_proc = object_search_left;		/* point search for axe */
    canvas_middlebut_proc = object_search_middle;	/* object search for log */
    canvas_rightbut_proc = clear_axe_objects;
    set_cursor(pick9_cursor);
    /* set the markers to show we only allow POLYLINES, ARCS and ELLIPSES */
    /* (the markers are originally set this way from the mode panel, but
       we need to set them again after the previous chop */   /* fixme -- neede for chop ? */
    update_markers(M_POLYLINE | M_ARC | M_ELLIPSE);
    reset_action_on();

    axe_objects_next = 0;
}

static void
select_axe_object(obj, type, x, y, p, q)		/* select axe objects */
     void   * obj;
     int      type;
     int      x, y;
     F_point * p;
     F_point * q;
{
  int i;

  for (i = 0; i < axe_objects_next; i++) {
    if ((axe_objects[i].type == type) &&
	(axe_objects[i].object == obj)) {
      put_msg("Duplicate axe object selected.");
      beep();
      return;
    }
  }
  if (axe_objects_max <= axe_objects_next) {
    axe_objects_max += AXE_OBJECTS_INCR;
    axe_objects = realloc(axe_objects, axe_objects_max * sizeof(axe_objects_s));
  }
  axe_objects[axe_objects_next].type = type;
  axe_objects[axe_objects_next++].object = obj;
  put_msg("Axe object %d selected", axe_objects_next);
}

static void
clear_axe_objects(obj, type, x, y, p, q)		/* clear axe objects */
     void   * obj;
     int      type;
     int      x, y;
     F_point * p;
     F_point * q;
{
  put_msg("Axe object list cleared.");
  axe_objects_next = 0;
}

static int
point_sort_fcn(a, b)
     s_point_s * a;
     s_point_s * b;
{
  return (a->dist == b->dist) ? 0
    : ((a->dist < b->dist) ? -1 : 1);
}

static int
point_sort_reverse_fcn(a, b)
     s_point_s * a;
     s_point_s * b;
{
  return (a->dist == b->dist) ? 0
    : ((a->dist > b->dist) ? -1 : 1);
}


static void
create_new_line( int x, int y, F_point ** this_point, F_line ** this_line, F_line * l)
{
  *this_line  = create_line();
  (*this_line)->type 		= l->type;
  (*this_line)->style 		= l->style;
  (*this_line)->thickness 	= l->thickness;
  (*this_line)->pen_color 	= l->pen_color;
  (*this_line)->fill_color 	= l->fill_color;
  (*this_line)->depth 		= l->depth;
  (*this_line)->pen_style 	= l->pen_style;
  (*this_line)->join_style 	= l->join_style;
  (*this_line)->cap_style 	= l->cap_style;
  (*this_line)->fill_style	= l->fill_style;
  (*this_line)->style_val		= l->style_val;
  if (l->for_arrow) {
    (*this_line)->for_arrow = create_arrow();
    memcpy((*this_line)->for_arrow, l->for_arrow, sizeof(F_arrow));
  }
  if (l->back_arrow) {
    (*this_line)->back_arrow = create_arrow();
    memcpy((*this_line)->back_arrow, l->back_arrow, sizeof(F_arrow));
  }
  (*this_point) = create_point();
  (*this_point)->x = x;
  (*this_point)->y = y;
  (*this_line)->points = (*this_point);
}

static Boolean
check_poly(F_line * this_line, struct f_point ** i_point_p, int type)
{
  int nr_pts;
  struct f_point * next_point;
  struct f_point * this_point;
  struct f_point * prev_prev_point;
  struct f_point * prev_point = NULL;

  if (!this_line || !(this_line->points)) return False;

  nr_pts = 0;
  for (this_point = this_line->points; this_point; this_point = next_point) {
    Boolean update_pp;

    update_pp = True;
    next_point = this_point->next;
    nr_pts++;
    if (prev_point) {
      if ((prev_point->x == this_point->x) &&
	  (prev_point->y == this_point->y)) {
	prev_point->next = next_point;
	free(this_point);
	nr_pts--;
	update_pp = False;
      }
    }
    if (True == update_pp) {
      prev_prev_point = prev_point;
      prev_point = this_point;
    }
  }

  if (i_point_p) *i_point_p = prev_prev_point;
  return (nr_pts < ((T_POLYGON == type) ? 3 : 2)) ? False : True;
}

static int
dice_polygons(int nr_segs, l_point_s * top_l_points, F_line * l)
{
  int i, j, k;
  int rc;
  int start_x;
  int start_y;
  int last_x = -1;
  int last_y = -1;
  ptype_e last_ptype = PTYPE_NONE;
  int end_seg;
  int end_point;
  int start_point = -1;
  Boolean completed = False;
  int this_x;
  int this_y;
  int this_ptype;
  F_line  * this_line;
  F_point * this_point;
  s_point_s * points;
  int nr_points = 0;

  
  for (i = 0; i < nr_segs; i++) {		/* analyse points */
    for (j = 0; j < top_l_points[i].points_next; j++) {
      if ((-1 == start_point) && (PTYPE_CUT == top_l_points[i].points[j].ptype)) /* find first cut */
	start_point = nr_points;
      nr_points++;
    }
  }

  if (-1 == start_point) return 0;		/* no cuts, shouldn't be possible ... */

  if (T_PIE_WEDGE_ARC == cur_arctype)
    snap_polyline_focus_handler(l, 0, 0);
	    
  points = alloca(nr_points * sizeof(s_point_s));
  for (k = 0, i = 0; i < nr_segs; i++) {		/* serialise the points */
    for (j = 0; j < top_l_points[i].points_next; j++, k++) {
      points[k] = top_l_points[i].points[j];
    }
  }
  
  rc = 0;
  this_line = NULL;
  for (i = 0; i <= nr_points; i++) {		/* yes, i do mean "<=" */
    j = (start_point + i) % nr_points;
      
    this_x     = points[j].x;
    this_y     = points[j].y;
    this_ptype = points[j].ptype;
      
    switch(this_ptype) {
    case PTYPE_END_PLINE:
    case PTYPE_END_VERTEX:
      /* do nothing -- same as next start vertex */
      break;
    case PTYPE_CUT:
      if (this_line) {				/* will be null on first cut point */
	float area;
	F_point * i_point;
	
	append_point(this_x, this_y, &this_point);	/* current point */
	if (T_PIE_WEDGE_ARC == cur_arctype)
	  append_point(snap_gx, snap_gy, &this_point);	/* centerpoint */
	append_point(start_x, start_y, &this_point);	/* close pgon */
	add_line(this_line);
	area = 0.0;
	if (True == check_poly(this_line, &i_point, T_POLYGON))	/* chk for dup adjacent pts */
	  compute_poly_area(this_line, &area);	/* make sure the pgon is real; */
	if (0.0 >= area) delete_line(this_line);
	else {
#if 0	  /* this creates bizarre but interesting results... */
	  if (T_PIE_WEDGE_ARC == cur_arctype)
	    insert_point(snap_gx, snap_gy, i_point);
#endif	  
	  rc++;
	}
	this_line = NULL;
      }
      if (i < (nr_points -1)) {
	create_new_line(this_x, this_y, &this_point, &this_line, l);
	start_x = last_x = this_x;
	start_y = last_y = this_y;
	last_ptype = this_ptype;
      }
      break;
    case PTYPE_START_VERTEX:
    case PTYPE_START_PLINE:
      append_point(this_x, this_y, &this_point);
      last_x = this_x;
      last_y = this_y;
      last_ptype = this_ptype;
      break;
    }			/* end of switch point type */
  }			/* end of points loop */

  return rc;
}

static int
dice_polylines(int nr_segs, l_point_s * top_l_points, F_line * l)
{
  int i, j;
  int rc;
  Boolean completed = False;
  int last_x = -1;
  int last_y = -1;
  ptype_e last_ptype = PTYPE_NONE;

  rc = 0;
  for (i = 0; (False == completed) && (i < nr_segs); i++) {		/* analyse points */
    for (j = 0; (False == completed) && (j < top_l_points[i].points_next); j++) {
      F_line  * this_line;
      F_point * this_point;
      
      int this_x     = top_l_points[i].points[j].x;
      int this_y     = top_l_points[i].points[j].y;
      int this_ptype = top_l_points[i].points[j].ptype;
      
      switch(this_ptype) {
      case PTYPE_END_VERTEX:
	/* do nothing -- same as next start vertex */
	break;
      case PTYPE_CUT:
	append_point(this_x, this_y, &this_point);
	add_line(this_line);
	if (True == check_poly(this_line, NULL, T_POLYLINE)) rc++;	/* chk for dup adjacent pts */
	else delete_line(this_line);

	create_new_line(this_x, this_y, &this_point, &this_line, l);
	last_x = this_x;
	last_y = this_y;
	last_ptype = this_ptype;
	break;
      case PTYPE_END_PLINE:
	append_point(this_x, this_y, &this_point);
	add_line(this_line);
	if (True == check_poly(this_line, NULL, T_POLYLINE)) rc++;	/* chk for dup adjacent pts */
	else delete_line(this_line);
	completed = True;		/* catches anomalous case of cut following end of pline */
	break;				
      case PTYPE_START_VERTEX:
	append_point(this_x, this_y, &this_point);
	last_x = this_x;
	last_y = this_y;
	last_ptype = this_ptype;
	break;
      case PTYPE_START_PLINE:
	create_new_line(this_x, this_y, &this_point, &this_line, l);
	last_x = this_x;
	last_y = this_y;
	last_ptype = this_ptype;
	break;
      }		/* end of switch point type */
    }			/* end of points loop with seg */
  }			/* end of segs loop */

  return rc;
}

static int
chop_polyline(F_line * l, int x, int y)
{
  int i, j;
  isect_cb_s isect_cb;
  struct f_point * p;
  int nr_verts;
  int nr_segs;
  Boolean runnable;
  int rc;

  l_point_s * top_l_points;

  typedef enum {
    STATE_PLINE_IDLE,
    STATE_PLINE_RUNNING
  } state_pline_e;

  if ((T_POLYLINE != l->type) && (T_POLYGON != l->type)) {
    put_msg("Only unconstrained polylines anf polygons may be chopped.");
    beep();
    return -1;
  }
  
  rc = 0;
  
  isect_cb.nr_isects = 0;
  isect_cb.max_isects = 0;
  isect_cb.isects = NULL;

  /* chopping polyline l by various things */
  
  runnable = True;
  for (i = 0; i < axe_objects_next; i++) {
    switch (axe_objects[i].type) {
    case O_POLYLINE:
      /* check for congruent axe/log */
      if (l == axe_objects[i].object) {
	put_msg("An axe cannot chop itself.");
	beep();
	runnable = False;
	break;
      }
      else intersect_polyline_polyline_handler((F_line *)(axe_objects[i].object), l,
					       x, y, &isect_cb);
      break;
    case O_ARC:
      intersect_polyline_arc_handler(l, (F_arc *)(axe_objects[i].object),  x, y, &isect_cb);
      break;
    case O_ELLIPSE:
      intersect_ellipse_polyline_handler((F_ellipse *)(axe_objects[i].object), l, x, y, &isect_cb);
      break;
    }
  }

  if ((True == runnable) && (0 < isect_cb.nr_isects)) {
    if ((T_POLYGON == l->type) && (2 > isect_cb.nr_isects)) {
      put_msg("Closed figures require two or more intersects.");
      beep();
      return -1;
    }
    for (p = l->points, nr_verts = 0; p != NULL; p = p->next, nr_verts++); /* just counting */
    nr_segs = nr_verts - 1;

    top_l_points = alloca(nr_segs * sizeof(l_point_s));

    {
      struct f_point * pp;
      int p_idx;
    
      for (p_idx = -1, pp = NULL, p = l->points;
	   p != NULL;
	   p = p->next, p_idx++) {
	if (pp) {
	  top_l_points[p_idx].points_next = 2;
	  top_l_points[p_idx].points_max = POINTS_INCR;
	  top_l_points[p_idx].points = malloc(POINTS_INCR * sizeof(s_point_s));
	  top_l_points[p_idx].points[0].x = pp->x;
	  top_l_points[p_idx].points[0].y = pp->y;
	  top_l_points[p_idx].points[0].dist = 0.0;
	  top_l_points[p_idx].points[0].ptype
	    = (p_idx == 0) ? PTYPE_START_PLINE : PTYPE_START_VERTEX;
	  top_l_points[p_idx].points[1].x = p->x;
	  top_l_points[p_idx].points[1].y = p->y;
	  top_l_points[p_idx].points[1].dist = hypot((double)(p->y - pp->y),
						     (double)(p->x - pp->x));
	  top_l_points[p_idx].points[1].ptype
	    = (NULL == p->next) ? PTYPE_END_PLINE : PTYPE_END_VERTEX;
	}
	pp = p;
      }
    }
    
    for (j = 0; j < isect_cb.nr_isects; j++) {		/* insert isect points */
      int next_p;
      
      int t_idx = isect_cb.isects[j].seg_idx;
      
      next_p = top_l_points[t_idx].points_next;
      if (next_p >= top_l_points[t_idx].points_max) {
	top_l_points[t_idx].points_max += POINTS_INCR;
	top_l_points[t_idx].points = realloc(top_l_points[t_idx].points,
					     top_l_points[t_idx].points_max * sizeof(s_point_s));
      }
      top_l_points[t_idx].points[next_p].x =  isect_cb.isects[j].x;
      top_l_points[t_idx].points[next_p].y =  isect_cb.isects[j].y;
      top_l_points[t_idx].points[next_p].dist
	= hypot((double)(isect_cb.isects[j].y - top_l_points[t_idx].points[0].y),
		(double)(isect_cb.isects[j].x - top_l_points[t_idx].points[0].x));
      top_l_points[t_idx].points[next_p].ptype = PTYPE_CUT;
      top_l_points[t_idx].points_next += 1;
    }
    
    for (i = 0; i < nr_segs; i++)		/* sort points by dist along seg */
      qsort(top_l_points[i].points, top_l_points[i].points_next,
	    sizeof(s_point_s), point_sort_fcn);

    rc = (T_POLYLINE == l->type)
      ? dice_polylines(nr_segs, top_l_points, l)
      : dice_polygons(nr_segs, top_l_points, l);

    if (0 < rc) {
      delete_line(l);
      redisplay_canvas();
    }
  
    if (isect_cb.isects) free(isect_cb.isects);

    for (i = 0; i < nr_segs; i++) {
      if (top_l_points[i].points) free(top_l_points[i].points);
    }
  }		/* end of check for 0 isects */

  return rc;
}

static int
chop_arc(F_arc * a, int x, int y)
{
  int i, j;
  isect_cb_s isect_cb;
  Boolean runnable;
  int rc;
  s_point_s * s_points = NULL;
  F_arc * this_arc = copy_arc(a);

  rc = 0;
  
  isect_cb.nr_isects = 0;
  isect_cb.max_isects = 0;
  isect_cb.isects = NULL;;
  
  /* 								seg_idx irrelevant */
  insert_isect(&isect_cb, (double)(a->point[0].x), (double)(a->point[0].y), -1);
  insert_isect(&isect_cb, (double)(a->point[2].x), (double)(a->point[2].y), -1);

  /* chopping arc a by various things */
  
  runnable = True;
  for (i = 0; i < axe_objects_next; i++) {
    switch (axe_objects[i].type) {
    case O_POLYLINE:
      intersect_polyline_arc_handler((F_line *)(axe_objects[i].object), a, x, y, &isect_cb);
      break;
    case O_ARC:
      /* check for congruent axe/log */
      if (a == axe_objects[i].object) {
	put_msg("An axe cannot chop itself.");
	beep();
	runnable = False;
	break;
      }
      else intersect_arc_arc_handler(a, (F_arc *)(axe_objects[i].object),  x, y, &isect_cb);
      break;
    case O_ELLIPSE:
      intersect_ellipse_arc_handler((F_ellipse *)(axe_objects[i].object), a, x, y, &isect_cb);
      break;
    }
  }

  if ((True == runnable) && (2 < isect_cb.nr_isects)) {	/* first two set are eps of original */

    double vsumx, vsumy;
    int sp = (1 == a->direction) ? 2 : 0;
    double dsx = (double)(a->point[sp].x) - (double)(a->center.x);
    double dsy = (double)(a->point[sp].y) - (double)(a->center.y);
    double rx = hypot((double)(a->point[1].y) - (double)(a->center.y),
		      (double)(a->point[1].x) - (double)(a->center.x));
    
    /*
     * dist = ((x2 - x0)(y0 - y)  -  (y2 - y0)(x0 - x))/mag((x2 - x0), (y2 - y0))
     *
     * x0, y0 = center point
     * x2, y2 = start point
     * dsx, dsy = start vector = (x2- x0),(y2 - y0)
     * dx, dy = t-vector
     * 
     * dist = dsx * dy  -  dsy * dx		(magnitude irrelevant)
     *
     */

#define lp_distance(dx, dy)      ((dsx * (dy))  -  (dsy * (dx))) 

    s_points = malloc(isect_cb.nr_isects * sizeof(s_point_s));
    for (i = 0; i < isect_cb.nr_isects; i++) {
      double dist, C;
      double dx = (double)(isect_cb.isects[i].x) - (double)(a->center.x);
      double dy = (double)(isect_cb.isects[i].y) - (double)(a->center.y);
      double c = hypot((double)(isect_cb.isects[i].y - a->point[sp].y),
		       (double)(isect_cb.isects[i].x - a->point[sp].x));
      
      if (0.0 > c) c = 0.0;
      else if (c > (2.0 * rx)) c = 2.0 * rx;

      /*
       * cos C = (a^2 + b^2 - c^2)/2ab
       *
       * a = b = r
       *
       * cos C = (2 r^2  -  c^2)/2  r^2
       *
       * cos C = 1 - c^2 / (2 r^2)
       *
       */
      
      C = acos(1 - (pow(c, 2.0) / (2.0 * pow(rx, 2.0))));

      dist = lp_distance(dx, dy);

      if (dist < 0.0) C = (2.0 * M_PI) - C;

      s_points[i].x = isect_cb.isects[i].x;
      s_points[i].y = isect_cb.isects[i].y;

      s_points[i].dist = C;
    }

    
    qsort(s_points, isect_cb.nr_isects, sizeof(s_point_s),
	  (1 == a->direction) ? point_sort_reverse_fcn : point_sort_fcn);
    
    for (i = 1; i < isect_cb.nr_isects; i++) {

      if ((rx * fabs(sin(s_points[i - 1].dist - s_points[i].dist))) > 5.0) {
	double vmag, vpha;
	
	vsumx = ((double)(s_points[i - 1].x + s_points[i].x)) - (2.0 * (double)(a->center.x));
	vsumy = ((double)(s_points[i - 1].y + s_points[i].y)) - (2.0 * (double)(a->center.y));
	vmag = hypot(vsumy, vsumx);
	vpha = atan2(vsumy, vsumx);
	vsumx *= rx/vmag;
	vsumy *= rx/vmag;

	if (fabs(s_points[i - 1].dist -  s_points[i].dist) > M_PI) {
	  vsumx *= -1.0;
	  vsumy *= -1.0;
	}
#if 0				/* probably don't need this */
	else if (fabs(s_points[i - 1].dist -  s_points[i].dist) < 0.01) {
	  snap_rotate_vector(&vsumx, &vsumy, vsumx, vsumy, M_PI - .001);
	}
#endif	

	this_arc->point[0].x = s_points[i - 1].x;
	this_arc->point[0].y = s_points[i - 1].y;
      
	this_arc->point[1].x = (int)lrint((double)(a->center.x) + vsumx);
	this_arc->point[1].y = (int)lrint((double)(a->center.y) + vsumy);

	this_arc->point[2].x = s_points[i].x;
	this_arc->point[2].y = s_points[i].y;

	add_arc(this_arc);
	rc++;
      }
    }
    
    delete_arc(a);
    redisplay_canvas();
  }			/* end of runnable */
  
  if (s_points) free(s_points);
  if (isect_cb.isects) free(isect_cb.isects);
  return rc;
}

static int
chop_ellipse(F_ellipse * e, int x, int y)
{
  int i, j;
  isect_cb_s isect_cb;
  Boolean runnable;
  int rc;
  s_point_s * s_points = NULL;

  rc = 0;
  
  isect_cb.nr_isects = 0;
  isect_cb.max_isects = 0;
  isect_cb.isects = NULL;;
  
  /* chopping ellipse e by various things */
  
  runnable = True;
  for (i = 0; i < axe_objects_next; i++) {
    switch (axe_objects[i].type) {
    case O_POLYLINE:
      intersect_ellipse_polyline_handler(e, (F_line *)(axe_objects[i].object), x, y, &isect_cb);
      break;
    case O_ARC:
      intersect_ellipse_arc_handler(e, (F_arc *)(axe_objects[i].object),  x, y, &isect_cb);
      break;
    case O_ELLIPSE:
      /* check for congruent axe/log */
      if (e == axe_objects[i].object) {
	put_msg("An axe cannot chop itself.");
	beep();
	runnable = False;
	break;
      }
      else intersect_ellipse_ellipse_handler((F_ellipse *)(axe_objects[i].object), e, x, y, &isect_cb);
      break;
    }
  }

  if (True == runnable) {
    if (2 > isect_cb.nr_isects) {
      put_msg("Closed figures require two or more intersects.");
      beep();
      return -1;
    }

    s_points = malloc((isect_cb.nr_isects + 1) * sizeof(s_point_s));
    for (i = 0; i < isect_cb.nr_isects; i++) {
      s_points[i].x = isect_cb.isects[i].x;
      s_points[i].y = isect_cb.isects[i].y;
      s_points[i].dist = atan2((double)( isect_cb.isects[i].y) - (double)(e->center.y),
			       (double)( isect_cb.isects[i].x) - (double)(e->center.x));
    }
    qsort(s_points, isect_cb.nr_isects, sizeof(s_point_s), point_sort_fcn);
    s_points[isect_cb.nr_isects].x    = s_points[0].x;
    s_points[isect_cb.nr_isects].y    = s_points[0].y;
    s_points[isect_cb.nr_isects].dist = (2.0 * M_PI) + s_points[0].dist;
    
    switch(e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      put_msg("Elliptical arcs not (yet) supported.");
      rc = -1;
      beep();
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      for (i = 0; i < isect_cb.nr_isects; i++) {
	double vmag, vpha;
	double vsumx, vsumy;
	double rx = (double)(e->radiuses.x);
	
	int sp = i;
	int ep = i + 1;
	F_arc * arc = create_arc();

	arc->type	= T_PIE_WEDGE_ARC;
	arc->depth	= e->depth;
	arc->thickness	= e->thickness;
	arc->pen_color	= e->pen_color;
	arc->fill_color	= e->fill_color;
	arc->fill_style	= e->fill_style;
	arc->pen_style	= e->pen_style;
	arc->style	= e->style;   
	arc->style_val	= e->style_val;
	arc->direction	= 0;
	arc->center.x	= e->center.x;
	arc->center.y	= e->center.y;
	
	vsumx = ((double)(s_points[sp].x + s_points[ep].x)) - (2.0 * (double)(arc->center.x));
	vsumy = ((double)(s_points[sp].y + s_points[ep].y)) - (2.0 * (double)(arc->center.y));
	vmag = hypot(vsumy, vsumx);
	vpha = atan2(vsumy, vsumx);
	vsumx *= rx/vmag;
	vsumy *= rx/vmag;

	if (fabs(s_points[sp].dist -  s_points[ep].dist) > M_PI) {
	  vsumx *= -1.0;	
	  vsumy *= -1.0;
	}
	else if (fabs(s_points[sp].dist -  s_points[ep].dist) < 0.01) {
	  snap_rotate_vector(&vsumx, &vsumy, vsumx, vsumy, M_PI - .001);
	}
	
	arc->point[0].x = s_points[sp].x;
	arc->point[0].y = s_points[sp].y;
      
	arc->point[1].x = (int)lrint((double)(arc->center.x) + vsumx);
	arc->point[1].y = (int)lrint((double)(arc->center.y) + vsumy);

	arc->point[2].x = s_points[ep].x;
	arc->point[2].y = s_points[ep].y;

	add_arc(arc);
	rc++;

	
      }
      
      delete_ellipse(e);
      redisplay_canvas();
      break;
    }						/* switch type */
  }						/* if runnable */
  
  if (s_points) free(s_points);
  if (isect_cb.isects) free(isect_cb.isects);
  return rc;
}

static void
select_log_object(obj, type, x, y, p, q)		/* select log objects */
     void   * obj;
     int      type;
     int      x, y;
     F_point * p;
     F_point * q;
{
  int i;
  Boolean rc;

  switch(type) {
  case O_POLYLINE:
    rc = chop_polyline(obj, x, y);
    switch(rc) {
    case -1:
      /* do nothing -- especially don't overwrite the previous msg */
      break;
    case 0:
      put_msg("Polyline does not intersect with any selected axe elements.");
      break;
    default:
      put_msg("Polyline chopped into %d pieces.", rc);
      break;
    }
    break;
  case O_ARC:
    rc = chop_arc(obj, x, y);
    switch(rc) {
    case -1:
      /* do nothing -- especially don't overwrite the previous msg */
      break;
    case 0:
      put_msg("Arc does not intersect with any selected axe elements.");
      break;
    default:
      put_msg("Arc chopped into %d pieces.", rc);
      break;
    }
    break;
  case O_ELLIPSE:
    rc = chop_ellipse(obj, x, y);
    switch(rc) {
    case -1:
      /* do nothing -- especially don't overwrite the previous msg */
      break;
    case 0:
      put_msg("Ellipse does not intersect with any selected axe elements.");
      break;
    default:
      put_msg("Ellipse chopped into %d pieces.", rc);
      break;
    }
    break;
  }
}

#if 0
static void
init_chop_right(obj, type, x, y, px, py)
    F_line	   *obj;
    int		    type;
    int		    x, y;
    int		    px, py;
{
}
#endif

#if 0				/* fixme -- may find a use for this stuff */
draw_join_marker(point)
    F_point	   *point;
{
	int x=ZOOMX(point->x), y=ZOOMY(point->y);
	pw_vector(canvas_win, x-10, y-10, x-10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x-10, y+10, x+10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y+10, x+10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y-10, x-10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
}

erase_join_marker(point)
    F_point	   *point;
{
    int x=ZOOMX(point->x), y=ZOOMY(point->y);
    if (point) {
	pw_vector(canvas_win, x-10, y-10, x-10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x-10, y+10, x+10, y+10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y+10, x+10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
	pw_vector(canvas_win, x+10, y-10, x-10, y-10, INV_PAINT,1,PANEL_LINE,0.0,MAGENTA);
    }
    point = NULL;
}
#endif
