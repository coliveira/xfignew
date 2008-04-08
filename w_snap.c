/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2004 by Chris Moller
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
#include "object.h"
#include "mode.h"
#include "w_snap.h"
#include "w_intersect.h"
#include "w_layers.h"
#include "w_setup.h"
#include "w_indpanel.h"
#include "w_util.h"
#include "u_quartic.h"
#include <math.h>
#include <alloca.h>

int snap_gx;
int snap_gy;
Boolean snap_found;
Boolean snap_msg_set;
static Boolean snap_held = False;

snap_mode_e snap_mode = SNAP_MODE_NONE;

void snap_release(Widget w, XtPointer closure, XtPointer call_data);

static void snap_polyline_handler(F_line * l, int x, int y);


/*                   */
/*  utility routines */
/*                   */

/* returns the value of vector [x y] rotated through theta radians */

void
snap_rotate_vector(double * dx, double * dy, double x, double y, double theta)
{
  /* [dx dy] = [x y] [  cos(theta) sin(theta) ] */
  /*                 [ -sin(theta) cos(theta) ] */
  double rx = (x * cos(theta)) - (y * sin(theta));
  double ry = (x * sin(theta)) + (y * cos(theta));
  /* so you can do snap_rotate_vector(&x, ..., x, ...) w/o munging things up */
  *dx = rx;
  *dy = ry;
}


/* returns the perpendicular distance from a point to a line	*/
/* defined by two points 					*/

static double
point_to_line(px, py, l1, l2)
     int px;
     int py;
     struct f_point * l1;
     struct f_point * l2;
{
  double x0 = (double)px;
  double y0 = (double)py;
  double x1 = (double)(l1->x);
  double y1 = (double)(l1->y);
  double x2 = (double)(l2->x);
  double y2 = (double)(l2->y);

  double dn = ((x2 - x1)*(y1 - y0)) - ((x1 - x0)*(y2 - y1));
  double dd = sqrt(pow(x2 - x1, 2.0) + pow(y2 - y1, 2.0));

  return (dd != 0.0) ? dn/dd : HUGE_VAL;
}


/* returns coeffs of the form Ax + By + C = 0 */

void
get_line_from_points(double * c, struct f_point * s1, struct f_point * s2)
{
  double s1x = (double)s1->x;
  double s1y = (double)s1->y;
  double s2x = (double)s2->x;
  double s2y = (double)s2->y;
  c[0] = s1y - s2y;
  c[1] = s2x - s1x;
  c[2] = (s1x * s2y) - (s1y * s2x);
}


/*                                                              */
/*  locate the endpoint in a polyline nearest the event point   */
/*                                                              */

static void
snap_polyline_endpoint_handler(l, x, y)
     F_line  *l;
     int x;
     int y;
{
  struct f_point * point;
  double mind = HUGE_VAL;
  for (point = l->points; point != NULL; point = point->next) {
    double dist = hypot((double)(point->x - x), (double)(point->y - y));
    if (dist < mind) {
      mind = dist;
      snap_gx = point->x;
      snap_gy = point->y;
      snap_found = True;
    }
  }
}

/*                                                              */
/*  locate the unweighted centroid of a polyline		*/
/*                                                              */

void
snap_polyline_focus_handler(F_line * l, int x, int y)
{
  /* polyline (type 1): simple verts			*/
  /* box (type 2):					*/
  /* polygon (type 3)					*/
  /* arcbox (type 4):					*/
  /* picture (type 5):	n+1 verts, first == last	*/

  double sx;
  double sy;
  int n;
  struct f_point * point =  l->points;
  if (point && (l->type != T_POLYLINE)) point = point->next;
  sx = sy = 0.0;
  for (n = 0; point != NULL; point = point->next, n++) {
    sx += (double)(point->x);
    sy += (double)(point->y);
  }
  if (n > 0) {
    snap_gx = (int)lrint(sx/(double)n);
    snap_gy = (int)lrint(sy/(double)n);
    snap_found = True;
  }
}

/*                                                              */
/*  locate the midpoint in a polyline nearest the event point   */
/*                                                              */

static void
snap_polyline_midpoint_handler(l, x, y)
     F_line  *l;
     int x;
     int y;
{
  struct f_point * prev_point = NULL;
  struct f_point * point;
  double mind = HUGE_VAL;
  for (point = l->points; point != NULL; point = point->next) {
    if (NULL != prev_point) {
      double mpx = ((double)(point->x + prev_point->x))/2.0;
      double mpy = ((double)(point->y + prev_point->y))/2.0;
      double dist = hypot(mpx - (double)x, mpy - (double)y);
      if (dist < mind) {
            mind = dist;
            snap_gx = (int)lrint(mpx);
            snap_gy = (int)lrint(mpy);
            snap_found = True;
      }
    }
    prev_point = point;
  }
}

/*                                                              */
/*  locate the polyline segment nearest the event point and     */
/*  compute the location on the extension of that segment such  */
/*  that the segment from the current point to the computed     */
/*  point is normal located polyline segment.                   */
/*                                                              */

static void
do_snap_polyline_normal(l, x, y, cur_point_x, cur_point_y)
     F_line  *l;
     int x;
     int y;
     double cur_point_x;
     double cur_point_y;
{
  struct f_point * prev_point = NULL;
  struct f_point * point;
  struct f_point * min_l1;
  struct f_point * min_l2;
  double mind = HUGE_VAL;

  for (point = l->points; point != NULL; point = point->next) {
    if (NULL != prev_point) {
      double dist = fabs(point_to_line(x, y, point, prev_point));
      if (dist < mind) {
        mind = dist;
        min_l1 = point;
        min_l2 = prev_point;
        snap_found = True;
      }
    }
    prev_point = point;
  }
  if (True == snap_found) {
    if (min_l1->x == min_l2->x) {
      snap_gx = min_l1->x;
      snap_gy = cur_point_y;
    }
    else if (min_l1->y == min_l2->y) {
      snap_gx = cur_point_x;
      snap_gy = min_l1->y;
    }
    else {
      double mn = (double)(min_l2->y - min_l1->y);
      double md = (double)(min_l2->x - min_l1->x);
      double m  = mn/md;
      double b  = ((double)(min_l1->y)) - m * ((double)(min_l1->x));
      double n  = -1/m;
      double c  = cur_point_y - n * cur_point_x;
      double rx = (c - b)/(m - n);
      double ry1 = (m * rx) + b;
      double ry2 = (n * rx) + c;
      snap_gx = (int)lrint(rx);
      snap_gy = (int)lrint(ry1);
    }
  }
}

static void
snap_polyline_normal_handler(l, x, y)
     F_line  *l;
     int x;
     int y;
{
  do_snap_polyline_normal(l, x, y, (double)(cur_point->x), (double)(cur_point->y));
}

/*                                                                      */
/*  locate the focus of the given ellipse nearest the event point       */
/*                                                                      */

static void
snap_ellipse_focus_ellipse_handler(e, x, y)
     F_ellipse  *e;
     int x;
     int y;
{
  int idx;
  int idy;
  double dx, dy;
  double dist_1, dist_2;

  double a = pow((double)(e->radiuses.x), 2.0);
  double b = pow((double)(e->radiuses.y), 2.0);
  double c = sqrt(fabs(b - a));

  snap_rotate_vector(&dx, &dy, c, 0.0, -((double)(e->angle)));
  idx = (int)lrint(dx);
  idy = (int)lrint(dy);

  dist_1 = hypot((double)((e->center.x + idx) - x),
                        (double)((e->center.y + idy) - y));
  dist_2 = hypot((double)((e->center.x - idx) - x),
                        (double)((e->center.y - idy) - y));
  if (dist_1 < dist_2) {
    snap_gx = e->center.x + idx;
    snap_gy = e->center.y + idy;
  }
  else {
    snap_gx = e->center.x - idx;
    snap_gy = e->center.y - idy;
  }
  snap_found = True;
}

/*
 * compute the angle between the segment from point [PX, PY] to point (x, y)
 * on the ellipse defined by (a, b) and the normal to that ellipse at that
 * point.
 */

static double
check_alignment(double x,  double y,
		double a,  double b,
		double PX, double PY)
{
  double theta, phi;
  
  /* angle from found point to ref point	*/
  theta = atan2(PY - y, PX - x);
  if (0.0 > theta) theta += M_PI;
  
  /* angle of normal at found point	*/
  phi = atan2(y * pow(a, 2.0), x * pow(b, 2.0));
  if (0.0 > phi) phi += M_PI;

  return(fabs(theta - phi));
}

/*                                                                      */
/*  locate the point on the given ellipse such that the segment drawn   */
/*  to it from from the current point is the normal to the ellipse	*/
/*  closest to the event point                                          */
/*                                                                      */

static void
snap_ellipse_normal_ellipse_handler(e, x, y, cur_point_x, cur_point_y)
     F_ellipse  *e;
     int x;
     int y;
     double cur_point_x;
     double cur_point_y;
{

  /*
   * Given point P at [X, Y] and an origin-centered orthogonal
   * ellipse E of semi-axes (a, b), the solutions [x, y] of the
   * following represent the four possible normals from P to E.
   *
   * f(x)  =  c4 x^4  + c3 x^3 + c2 x^2 + c1 x + c0 = 0
   *
   * where
   *
   * c4 =   (a^2 - b^2)^2
   * c3 = - 2a^2 X (a^2 - b^2)
   * c2 =   a^2 [a^2 X^2 + b^2 Y^2 - (a^2 - b^2)^2]
   * c1 =   2a^4 X (a^2 - b^2)
   * c0 = - a^6 X^2
   *
   * y = +- b sqrt[1 - (x/a)^2]
   *
   */

  double * c;
  double * solr;
  double * soli;
  double tf;
  double a, b;
  int nsol;
  int i;
  double dist;
  double mind = HUGE_VAL;
  double ix[2];
  double iy[2];
  double dtheta[2];
  double theta, phi;
  int sel_idx;
  
  /* translate to ellipse origin */
  
  double PX = cur_point_x - (double)(e->center.x);
  double PY = cur_point_y - (double)(e->center.y);
  double X  = (double)(x - e->center.x);
  double Y  = (double)(y - e->center.y);
  
  mind = HUGE_VAL;
      
  /* rotate around ellipse angle so ellipse semi-axes are ortho to space */
  snap_rotate_vector (&PX, &PY, PX, PY, (double)(e->angle));
  snap_rotate_vector (&X,  &Y,  X,  Y,  (double)(e->angle));

  c    = alloca(5 * sizeof(double));
  solr = alloca(4 * sizeof(double));
  soli = alloca(4 * sizeof(double));

  a = (double)(e->radiuses.x);
  b = (double)(e->radiuses.y);

  /* fixme -- do something about cases where a < b (swap the coords around?)*/
  tf = pow(a, 2.0) - pow(b, 2.0);
  
  c[4] = pow(tf, 2.0);
  c[3] = -2.0 * pow(a, 2.0) * PX * tf;
  c[2] = pow(a, 2.0) * (((pow(a, 2.0) * pow(PX, 2.0)) +
			 (pow(b, 2.0) * pow(PY, 2.0))) - c[4]);
  c[1] = 2.0 * pow(a, 4.0) * PX * tf;
  c[0] = -pow(a, 6.0) * pow(PX, 2.0);

  nsol = quartic(c, solr, soli);

  for (i = 0; i < 4; i++) {
    ix[0] = ix[1] = solr[i];
    iy[0] =  b * sqrt(1.0 - pow((ix[0]/a), 2.0));
    iy[1] = -b * sqrt(1.0 - pow((ix[1]/a), 2.0));

    dtheta[0] = check_alignment(ix[0], iy[0], a, b, PX, PY);
    dtheta[1] = check_alignment(ix[1], iy[1], a, b, PX, PY);


    sel_idx = (dtheta[0] < dtheta[1]) ? 0 : 1;

    dist = hypot(iy[sel_idx] - Y, ix[sel_idx] - X);
    
    if (dist < mind) {
      /* rotate back to space axes */
      snap_rotate_vector (&ix[sel_idx], &iy[sel_idx],
			  ix[sel_idx],  iy[sel_idx],
			  -((double)(e->angle)));
    
      /* translate back to space axes */
      ix[sel_idx] += (double)(e->center.x);
      iy[sel_idx] += (double)(e->center.y);
    
      snap_gx = (int)lrint(ix[sel_idx]);
      snap_gy = (int)lrint(iy[sel_idx]);
      snap_found = True;

      mind = dist;
    }
  }
}
      

/*                                                                      */
/*  locate the point on the given circle such that the segment drawn    */
/*  to it from from the current point is the normal to the circle	*/
/*  closest to the event point                                          */
/*                                                                      */

static void
circle_normal_handler(center_x, center_y, radius, x, y, cur_point_x, cur_point_y)
     double center_x;
     double center_y;
     double radius;
     int x;
     int y;
     double cur_point_x;
     double cur_point_y;
{
  double a;
  double txa, tya;
  double txb, tyb;
  double da, db;
  if ((center_y == (double)(cur_point_y)) &&
      (center_x == (double)(cur_point_x))) {
    /* check for cur_point at center.  if so, draw a radius near the */
    /* event point 						   */
    a = atan2(center_y - (double)y, center_x - (double)x);
  }	
  else {
    a = atan2(center_y - (double)(cur_point_y),
	      center_x - (double)(cur_point_x));
  }
  txa = center_x - (radius * cos(a));
  tya = center_y - (radius * sin(a));
  txb = center_x + (radius * cos(a));
  tyb = center_y + (radius * sin(a));
  da = hypot(tya - (double)y, txa - (double)x);
  db = hypot(tyb - (double)y, txb - (double)x);
  if (da < db) {
    snap_gx = (int)lrint(txa);
    snap_gy = (int)lrint(tya);
  }
  else {
    snap_gx = (int)lrint(txb);
    snap_gy = (int)lrint(tyb);
  }
  snap_found = True;
}

static void
snap_ellipse_normal_circle_handler(e, x, y, cur_point_x, cur_point_y)
     F_ellipse  *e;
     int x;
     int y;
     double cur_point_x;
     double cur_point_y;
{
  circle_normal_handler((double)(e->center.x),
			(double)(e->center.y),
			(double)(e->radiuses.x),
			x, y,
			cur_point_x, cur_point_y);
}

/*                                                                      */
/*  locate the point on the given circle such that the segment drawn    */
/*  to it from the current point is tangent to the circle.		*/
/*                                                                      */

static void
circle_tangent_handler(center_x, center_y, r, x, y)
     double center_x;
     double center_y;
     double r;
     int x;
     int y;
{
  double p = hypot(center_y - (double)(cur_point->y),
		   center_x - (double)(cur_point->x));
  /* make sure we're outside the circle -- can't draw a tangent from inside */
  if (p > r) {
    double ix, iy;
    double txa, tya;
    double txb, tyb;
    double da, db;
    double a = atan2((double)(cur_point->y) - center_y,
		     (double)(cur_point->x) - center_x);
    double b = acos(r/p);
    snap_rotate_vector(&ix, &iy, r, 0.0, a);
    snap_rotate_vector(&txa, &tya, ix, iy, b);
    snap_rotate_vector(&txb, &tyb, ix, iy, -b);
    txa += center_x;
    tya += center_y;
    txb += center_x;
    tyb += center_y;
    da = hypot(tya - (double)y, txa - (double)x);
    db = hypot(tyb - (double)y, txb - (double)x);
    if (da < db) {
      snap_gx = (int)lrint(txa);
      snap_gy = (int)lrint(tya);
    }
    else {
      snap_gx = (int)lrint(txb);
      snap_gy = (int)lrint(tyb);
    }
    snap_found = True;
  }
  else {
    put_msg("No tangent can be drawn from the current point.");
    beep();
    snap_msg_set = True;
  }
}

static void
snap_ellipse_tangent_circle_handler(e, x, y)
     F_ellipse  *e;
     int x;
     int y;
{
  circle_tangent_handler((double)(e->center.x), (double)(e->center.y),
			 (double)(e->radiuses.x), x, y);
}

/*                                                                      */
/*  locate the point on the given ellipse such that the segment drawn   */
/*  to it from the current point is tangent to the circle.		*/
/*                                                                      */

static void
snap_ellipse_tangent_ellipse_handler(e, x, y)
     F_ellipse  *e;
     int x;
     int y;
{

  /*
   * [X Y] = initial point
   * [x y]   = tangent point
   *
   * me = -[b^2]x / [a^2]y
   *
   * mv = (Y - y) / (X - x)
   *
   * mv = me
   *
   * (Y - y) / (X - x) = -[b^2]x / [a^2]x					(Eq 1)
   *
   *	A = a^2
   *	B = b^2
   *
   * (Y - y) / (X - x) = -Bx / Ay						(Eq 1)
   *
   * [YA]y - Ay^2 = -[XB]x + Bx^2						(Eq 2, from Eq 1)
   *	
   * (x^2 / [a^2]) + (y^2 / [b^2]) = 1						(Eq 3)
   *	
   * (x^2 / A) + (y^2 / B) = 1
   *	
   * B x^2 + A y^2 = [AB]							(Eq 4, from Eq 3)
   *
   * -------------------------------------------------------
   *
   * B x^2 = [AB] - Ay^2 = A(B - y^2)						(Eq 5, from Eq 4)
   *
   * x = sqrt(A/B) sqrt(B - y^2) = (a/b)sqrt(B - y^2)				(Eq 6, from Eq 5)
   *
   * --------------------------------------------------------
   *
   * [YA]y - Ay^2 = -[XB]([a/b] sqrt([B - y^2)) + AB - Ay^2
   *
   * [YA]y - AB = -[Xab]sqrt(B - y^2)				(Eq 9, from Eq 8)
   *
   * [Ya]y - [a b^2] = -[Xb]sqrt(B - y^2)					(Eq 10, from Eq 9)
   *
   * 	K = Ya
   *	L = a b^2
   *	M = Xb
   *
   * Ky - L = -Msqrt(B - y^2)				
   *
   * (Ky - L)^2 = M^2 (B - y^2)	
   *
   * [K^2]y^2 - [2KL]y + [L^2] = [M^2 B] - [M^2] y^2
   *
   * [K^2 + M^2]y^2 - [2KL]y + [L^2 - M^2 B] = 0
   *
   * 	P = K^2 + M^2
   *	Q = -2KL
   *	R = L^2 - M^2 B
   *
   * P y^2 + Q y + R = 0
   *
   */

  double A, B;
  double K, L, M;
  double P, Q, R;
  
  double a = (double)(e->radiuses.x);
  double b = (double)(e->radiuses.y);

  /* translate to ellipse origin */

  double PX = (double)(cur_point->x - e->center.x);
  double PY = (double)(cur_point->y - e->center.y);
  double X  = (double)(x - e->center.x);
  double Y  = (double)(y - e->center.y);
    
  /* rotate around ellipse angle so ellipse semi-axes are ortho to space */
  snap_rotate_vector (&PX, &PY, PX, PY, (double)(e->angle));
  snap_rotate_vector (&X,  &Y,  X,  Y,  (double)(e->angle));

  A = pow(a, 2.0);
  B = pow(b, 2.0);

  K = PY * a;
  L = a * pow(b, 2.0);
  M = PX * b;

  P = pow(K, 2.0) + pow(M, 2.0);
  Q = -2.0 * K * L;
  R = pow(L, 2.0) - (pow(M, 2.0) * B);

  {
    double rx = pow(Q, 2.0) - (4.0 * P * R);

    if (0.0 <= rx) {
      double xx, yy;
      double tx[4], ty[2];
      double dist;
      double mind = HUGE_VAL;
      int xi, yi, px, py;
      
      ty[0] = ((-Q) + sqrt(rx))/(2.0 * P);
      ty[1] = ((-Q) - sqrt(rx))/(2.0 * P);
      tx[0] = (a/b) * sqrt(pow(b, 2.0) - pow(ty[0], 2.0));
      tx[1] = (a/b) * sqrt(pow(b, 2.0) - pow(ty[1], 2.0));
      tx[2] = -tx[0];
      tx[3] = -tx[1];

      for (xi = 0; xi < 4; xi++) {
	for (yi = 0; yi < 2; yi++) {
	  dist = hypot(Y - ty[yi], X - tx[xi]);
	  if (dist < mind) {
	    mind = dist;
	    px = xi;
	    py = yi;
	  }
	}
      }
      snap_rotate_vector (&xx,  &yy,  tx[px],  ty[py],  -(double)(e->angle));
      xx += (double)(e->center.x);
      yy += (double)(e->center.y);
      snap_gx = (int)lrint(xx);
      snap_gy = (int)lrint(yy);
      snap_found = True;
    }
    else {
      put_msg("No tangent can be drawn from the current point.");
      beep();
      snap_msg_set = True;
    }
  }
}

/*                                                                      */
/*  locate the endpoint of the semi-axis of the the given ellipse	*/
/*  nearest the event point.						*/
/*                                                                      */

static void
snap_ellipse_endpoint_handler(e, x, y)
     F_ellipse  *e;
     int x;
     int y;
{
    double dist;
  int i;
  double tx,ty;
  double mind = HUGE_VAL;

  for (i = 0; i < 4; i++) {
    double rx,ry;
    double dist;
    double xx = (double)((i & 1) ? 0 : ((i & 2) ? e->radiuses.x : -(e->radiuses.x)));
    double xy = (double)((i & 1) ? ((i & 2) ? e->radiuses.y : -(e->radiuses.y)) : 0);
    
    snap_rotate_vector(&rx, &ry, xx, xy, -((double)e->angle));
    
    rx += (double)(e->center.x);
    ry += (double)(e->center.y);
    
    dist = hypot(rx - (double)x, ry - (double)y);
    if (dist < mind) {
      mind = dist;
      tx = rx;
      ty = ry;
    }
  }
  snap_gx = (int)lrint(tx);
  snap_gy = (int)lrint(ty);
  snap_found = True;
}

/*                                      */
/*  handle polyline interactions        */
/*                                      */

static void
snap_polyline_handler(l, x, y)
     F_line  *l;
     int x;
     int y;
{
  switch (snap_mode) {
  case SNAP_MODE_ENDPOINT:
    snap_polyline_endpoint_handler(l, x, y);
    break;
  case SNAP_MODE_MIDPOINT:
    snap_polyline_midpoint_handler(l, x, y);
    break;
  case SNAP_MODE_TANGENT:
    put_msg("Polylines have no tangents.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_NORMAL:
    snap_polyline_normal_handler(l, x, y);
    break;
  case SNAP_MODE_FOCUS:
    snap_polyline_focus_handler(l, x, y);
    break;
  case SNAP_MODE_DIAMETER:
    put_msg("Polylines do not have diameters.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_NEAREST:
    do_snap_polyline_normal(l, x, y, (double)x, (double)y);
    break;
  }
}

/*                                      */
/*  handle spline interactions        */
/*                                      */

static void
snap_spline_handler(s, x, y)
     F_spline  *s;
     int x;
     int y;
{
  switch (snap_mode) {
  case SNAP_MODE_ENDPOINT:
    put_msg("Spline endpoints not yet implemented.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_MIDPOINT:
    put_msg("Spline midpoints not yet implemented.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_TANGENT:
    put_msg("Spline tangents not yet implemented.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_NORMAL:
    put_msg("Spline normals not yet implemented.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_FOCUS:
    {
      F_line * f_line_p = alloca(sizeof(F_line));
      f_line_p->type = T_POLYLINE;
      f_line_p->points = s->points;
      snap_polyline_handler(f_line_p, x, y);
      break;
    }
  case SNAP_MODE_DIAMETER:
    switch(s->type) {
    case T_OPEN_APPROX:
    case T_OPEN_INTERP:
    case T_OPEN_XSPLINE:
      put_msg("Polylines do not have diameters.");
      beep();
      snap_msg_set = True;
      break;
    case T_CLOSED_APPROX:
    case T_CLOSED_INTERP:
    case T_CLOSED_XSPLINE:
      put_msg("Spline diameters not yet implemented.");
      beep();
      snap_msg_set = True;
      break;
    }
    break;
  case SNAP_MODE_NEAREST:
    put_msg("Spline snap nearest not yet implemented.\n");
    beep();
    snap_msg_set = True;
    break;
  }
}

/*                                      */
/*  handle text interactions        */
/*                                      */

static void
snap_text_handler(t, x, y)
     F_text  *t;
     int x;
     int y;
{
  F_line * f_line_p = build_text_bounding_box(t); 
  snap_polyline_handler(f_line_p, x, y);
  delete_text_bounding_box(f_line_p);
}


Boolean
is_point_on_arc(a, x, y)
     F_arc * a;
     int x;
     int y;
{
  /*
   * check if the found point is on the arc and not on the
   * "missing" extension of the arc.  check that the sign of the
   * distance from the p[0]-p[2] chord to center point p[1] matches
   * the sign of the point-chord distance
   *
   * dist = ((x2 - x0)(y0 - y)  -  (y2 - y0)(x0 - x))/mag((x2 - x0), (y2 - y0))
   *
   * skip normalisation for this test.
   *
   */
  
  double d1, dt;
  double dx = (double)(a->point[2].x - a->point[0].x);
  double dy = (double)(a->point[2].y - a->point[0].y);

  d1 = (dx * (double)(a->point[0].y - a->point[1].y))
    -  (dy * (double)(a->point[0].x - a->point[1].x));
  
  dt = (dx * (double)(a->point[0].y - y))
    -  (dy * (double)(a->point[0].x - x));
  
  return (signbit(d1) == signbit(dt)) ? True : False;
}

/*                                      */
/*  handle arc interactions  */
/*                                      */

static void
snap_arc_handler(a, x, y)
     F_arc  *a;
     int x;
     int y;
{
  switch (snap_mode) {
  case SNAP_MODE_ENDPOINT:
    {
      double dist;
      double mind = HUGE_VAL;
      int i, p;

      for (i = 0; i < 3; i++) {
	dist = hypot((double)(a->point[i].y - y), (double)(a->point[i].x - x));
	if (dist < mind) {
	  mind = dist;
	  p = i;
	}
      }
      snap_gx = a->point[p].x;
      snap_gy = a->point[p].y;
      snap_found = True;
    }
    break;
  case SNAP_MODE_MIDPOINT:
    {
      int i, sx, sy;

      sx = sy = 0;
      for (i = 0; i < 3; i++) {
	sx += a->point[i].x;
	sy += a->point[i].y;
      }
      snap_gx = (int)lrint(((double)sx)/3.0);
      snap_gy = (int)lrint(((double)sy)/3.0);
      snap_found = True;
    }
    break;
  case SNAP_MODE_TANGENT:
    {
      double radius = hypot((double)(a->center.y) - (double)(a->point[1].y),
			    (double)(a->center.x) - (double)(a->point[1].x));
      circle_tangent_handler((double)(a->center.x), (double)(a->center.y),
			     radius, x, y);
      if (True == snap_found) {
	if (False == is_point_on_arc(a, snap_gx, snap_gy)) {
	  snap_found = False;
	  put_msg("The tangent point found is not on the arc.");
	  beep();
	  snap_msg_set = True;
	}
      }
    }
    break;
  case SNAP_MODE_NORMAL:
    {
      double radius = hypot(a->center.y - (double)(a->point[1].y),
			    a->center.x - (double)(a->point[1].x));
      circle_normal_handler((double)(a->center.x),
			    (double)(a->center.y),
			    radius,
			    x, y, (double)(cur_point->x), (double)(cur_point->y));
      if (True == snap_found) {
	if (False == is_point_on_arc(a, snap_gx, snap_gy)) {
	  snap_found = False;
	  put_msg("The normal point found is not on the arc.");
	  beep();
	  snap_msg_set = True;
	}
      }
    }
    break;
  case SNAP_MODE_FOCUS:
    snap_gx = (int)lrint((double)(a->center.x));
    snap_gy = (int)lrint((double)(a->center.y));
    snap_found = True;
    break;
  case SNAP_MODE_DIAMETER:
    put_msg("Arcs do not have diameters.");
    beep();
    snap_msg_set = True;
    break;
  case SNAP_MODE_NEAREST:
    {
      double radius = hypot(a->center.y - (double)(a->point[1].y),
			    a->center.x - (double)(a->point[1].x));
      circle_normal_handler((double)(a->center.x),
			    (double)(a->center.y),
			    radius,
			    x, y, (double)x, (double)y);
      if (True == snap_found) {
	if (False == is_point_on_arc(a, snap_gx, snap_gy)) {
	  snap_found = False;
	  put_msg("The closest point found is not on the arc.");
	  beep();
	  snap_msg_set = True;
	}
      }
    }
    break;
  }
}

/*                                      */
/*  handle ellipse/circle interactions  */
/*                                      */

static void
snap_ellipse_handler(e, x, y)
     F_ellipse  *e;
     int x;
     int y;
{
  switch (snap_mode) {
  case SNAP_MODE_ENDPOINT:
    switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      snap_ellipse_endpoint_handler(e, x, y);
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      put_msg("Circles have no endpoints.");
      beep();
      snap_msg_set = True;
      break;
    }
    break;
  case SNAP_MODE_MIDPOINT:
    snap_gx = e->center.x;
    snap_gy = e->center.y;
    snap_found = True;
    break;
  case SNAP_MODE_TANGENT:
    switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      snap_ellipse_tangent_ellipse_handler(e, x, y);
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      snap_ellipse_tangent_circle_handler(e, x, y);
      break;
    }
    break;
  case SNAP_MODE_NORMAL:
    switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      snap_ellipse_normal_ellipse_handler(e, x, y, (double)(cur_point->x), (double)(cur_point->y));
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      snap_ellipse_normal_circle_handler(e, x, y, (double)(cur_point->x), (double)(cur_point->y));
      break;
    }
    break;
  case SNAP_MODE_FOCUS:
    switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      snap_ellipse_focus_ellipse_handler(e, x, y);
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      snap_gx = e->center.x;
      snap_gy = e->center.y;
      snap_found = True;
      break;
    }
    break;
  case SNAP_MODE_DIAMETER:
    {
      int dx = cur_point->x - e->center.x;
      int dy = cur_point->y - e->center.y;
      snap_gx = e->center.x - dx;
      snap_gy = e->center.y - dy;
      snap_found = True;
    }
    break;
  case SNAP_MODE_NEAREST:
    switch (e->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      snap_ellipse_normal_ellipse_handler(e, x, y, (double)x, (double)y);
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      snap_ellipse_normal_circle_handler(e, x, y, (double)x, (double)y);
      break;
    }
    break;
  }
}

static void
snap_handler(p, type, x, y, px, py)
    void * p;
    int type;
    int x, y;
    int px, py;
{
  F_arc      *a;
  F_line     *l;
  F_spline   *s;
  F_text     *t;
  static void * intersect_object_1;
  static int intersect_type_1;

  if (snap_mode == SNAP_MODE_INTERSECT) {
    switch(intersect_state) {
    case INTERSECT_INITIAL:
      intersect_object_1 = p;
      intersect_type_1 = type;
      intersect_state = INTERSECT_FIRST_FOUND;
      XtVaSetValues(snap_indicator_label, XtNlabel, "I'sect 2 " , NULL);
      put_msg("Select second object.");
      snap_msg_set = True;
      break;
    case INTERSECT_FIRST_FOUND:
      snap_intersect_handler(intersect_object_1, intersect_type_1, p, type, x, y);
      intersect_state = INTERSECT_INITIAL;
      XtVaSetValues(snap_indicator_label, XtNlabel, "Intersect" , NULL);
      break;
    }
  }
  else {
    switch (type) {
    case O_ELLIPSE:
      snap_ellipse_handler(p, x, y);
      break;
    case O_POLYLINE:
      snap_polyline_handler(p, x, y);
      break;
    case O_SPLINE:
      snap_spline_handler(p, x, y);
      break;
    case O_TEXT:
      snap_text_handler(p, x, y);
      break;
    case O_ARC:
      snap_arc_handler(p, x, y);
      break;
    }
  }
}

/*              */
/*  public i/f  */
/*              */

Boolean
snap_process(px, py, state)
     int * px;
     int * py;
     unsigned int state;
{
  int hold_objmask = cur_objmask;

  snap_found = False;
  snap_msg_set = False;
  cur_objmask = M_OBJECT;
  init_searchproc_right(snap_handler);
  object_search_right(*px, *py, state);
  cur_objmask = hold_objmask;

  if (True == snap_found) {
    *px = snap_gx;
    *py = snap_gy;
    if (False == snap_held) {
      snap_mode = SNAP_MODE_NONE;
      XtVaSetValues(snap_indicator_label, XtNlabel, "None     " , NULL);
    }
  }
  else {
    if (False == snap_msg_set) {
      put_msg("No point found.");
      beep();
    }
  }

  return snap_found;
}

void
snap_hold(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  snap_held = True;
}

void
snap_release(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  snap_held = False;
  snap_mode = SNAP_MODE_NONE;
  XtVaSetValues(snap_indicator_label, XtNlabel, "None     " , NULL);
}

void
snap_endpoint(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* snap to:									*/
  /*		polyline (incl box and polygon) vertices		-- done	*/
  /*		ellipse (but not circle) semi-axis endpoints		-- done	*/
  /*		spline endpoints					-- todo	*/
  /*		text bounding box vertices (?)				-- done	*/
  /*		arc endpoints						-- done	*/
  
  XtVaSetValues(snap_indicator_label, XtNlabel, "Endpoint" , NULL);
  snap_mode = SNAP_MODE_ENDPOINT;
}

void
snap_midpoint(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* snap to:									*/
  /*		polyline (incl box and polygon) segment midpoints	-- done	*/
  /*		ellipse and circle centerpoints				-- done	*/
  /*		spline midpoints					-- todo	*/
  /*		text bounding box segment midpoints (?)			-- done	*/
  /*		arc midpoints						-- done	*/
  
  XtVaSetValues(snap_indicator_label, XtNlabel, "Midpoint" , NULL);
  snap_mode = SNAP_MODE_MIDPOINT;
}

void
snap_nearest(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* snap to:									*/
  /*		nearest object							*/
  
  XtVaSetValues(snap_indicator_label, XtNlabel, "Nearest" , NULL);
  snap_mode = SNAP_MODE_NEAREST;
}

void
snap_focus(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* snap to:									*/
  /*		closed polyline (box and polygon) centroids		-- done	*/
  /*		open polyline centroids (?)				-- done	*/
  /*		ellipse foci and circle centerpoints			-- done	*/
  /*		closed spline centroids					-- todo	*/
  /*		open spline centroids (?)				-- ????	*/
  /*		text bounding box centroids (?)				-- done	*/
  /*		arc centroids (or origin ?)				-- done	*/
  
  XtVaSetValues(snap_indicator_label, XtNlabel, "Focus" , NULL);
  snap_mode = SNAP_MODE_FOCUS;
}

void
snap_diameter(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* snap to:									*/
  /*		closed polyline (box and polygon) point opp centroids	-- todo	*/
  /*		open polyline  (?)					-- null	*/
  /*		ellipse foci and circle diameters			-- todo	*/
  /*		closed spline opp centroids				-- todo	*/
  /*		open spline centroids (?)				-- null	*/
  /*		text bounding box opp centroids (?)			-- todo	*/
  /*		arc centroids (or origin ?)				-- null	*/
  
  XtVaSetValues(snap_indicator_label, XtNlabel, "Focus" , NULL);
  snap_mode = SNAP_MODE_DIAMETER;
}

void
snap_intersect(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* snap to the intersection of the next two objects selected.			*/

  XtVaSetValues(snap_indicator_label, XtNlabel, "Intersect" , NULL);
  snap_mode = SNAP_MODE_INTERSECT;
  intersect_state = INTERSECT_INITIAL;
}

void
snap_normal(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* for current polyline, box, or polygon, snap to a point on the target 	*/
  /* such that the segment defined by that point and the existing current	*/
  /* point is normal to:							*/
  /*		polyline (incl box and polygon) segments		-- done	*/
  /*		ellipses and circles 					-- done	*/
  /*		spline (?)						-- ????	*/
  /*		text bounding box segments (?)				-- ????	*/
  /*		arcs 							-- done	*/
  
  if ((F_POLYLINE != cur_mode) &&
      (F_BOX      != cur_mode) &&
      (F_POLYGON  != cur_mode)) {
    put_msg("Normals can only be computed for polylines, boxes, and polygons.");
    beep();
    snap_msg_set = True;
  }
  else if (!cur_point) {
    put_msg("No prior point from which to create a normal.");
    beep();
    snap_msg_set = True;
  }
  else {
    XtVaSetValues(snap_indicator_label, XtNlabel, "Normal" , NULL);
    snap_mode = SNAP_MODE_NORMAL;
  }
}

void
snap_tangent(w, closure, call_data)
    Widget    w;
    XtPointer closure;
    XtPointer call_data;
{
  
  /* for current polyline, box, or polygon, snap to a point on the target 	*/
  /* such that the segment defined by that point and the existing current	*/
  /* point is tangent to:							*/
  /*		polyline (?)						-- ????	*/
  /*		ellipses and circles 					-- done	*/
  /*		spline (?)						-- ????	*/
  /*		text bounding box segments (?)				-- ????	*/
  /*		arcs 							-- done	*/
  
  if ((F_POLYLINE != cur_mode) &&
      (F_BOX      != cur_mode) &&
      (F_POLYGON  != cur_mode)) {
    put_msg("Tangents can only be computed for polylines.");
    beep();
    snap_msg_set = True;
  }
  else if (!cur_point) {
    put_msg("No prior point from which to create a tangent.");
    beep();
    snap_msg_set = True;
  }
  else {
    XtVaSetValues(snap_indicator_label, XtNlabel, "Tangent" , NULL);
    snap_mode = SNAP_MODE_TANGENT;
  }
}

Widget snap_indicator_panel;
Widget snap_indicator_label;


void
init_snap_panel(parent)
    Widget	 parent;
{
  Widget dlabel;
  DeclareArgs(10);
  

  /* MOUSEFUN_HT and ind_panel height aren't known yet */
  LAYER_HT = TOPRULER_HT + CANVAS_HT;

  /* main form to hold all the layer stuff */

  FirstArg(XtNfromHoriz, sideruler_sw);
  NextArg(XtNdefaultDistance, 1);
  NextArg(XtNwidth, LAYER_WD);
  NextArg(XtNheight, LAYER_HT);
  NextArg(XtNleft, XtChainRight);
  NextArg(XtNright, XtChainRight);
  snap_indicator_panel = XtCreateWidget("snap_indicator_form",
					formWidgetClass, parent, Args, ArgCount);

  /* snap title label */
  FirstArg(XtNlabel, "Snap Mode");
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainRight);
  dlabel = XtCreateManagedWidget("snap_dlabel", labelWidgetClass, snap_indicator_panel, 
				 Args, ArgCount);

  /* snap mode indicator */
  FirstArg(XtNlabel, "None     ");
  NextArg(XtNfromVert, dlabel);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainRight);
  snap_indicator_label = XtCreateManagedWidget("snap_label", labelWidgetClass,
					       snap_indicator_panel, Args, ArgCount);

  /* fixme -- add event handler */

  XtManageChild(snap_indicator_panel);
}
