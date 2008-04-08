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
#include "u_quartic.h"
#include <math.h>
//#include <complex.h>
#undef I
  
#define ISET_P1 (1 << 0)
#define ISET_P2 (1 << 1)

intersect_state_e intersect_state = INTERSECT_INITIAL;

static INLINE Boolean
is_point_on_segment(double x1, double y1, double xc, double yc, double x2, double y2)
{
  return ((((x1 <= xc) && (xc <= x2)) || ((x1 >= xc) && (xc >= x2))) && 
	  (((y1 <= yc) && (yc <= y2)) || ((y1 >= yc) && (yc >= y2)))) ? True : False;
}

void
insert_isect(isect_cb_s * isect_cb, double ix, double iy, int seg_idx)
{
  if (isect_cb->max_isects <= isect_cb->nr_isects) {
    isect_cb->max_isects += ISECTS_INCR;
    isect_cb->isects = realloc(isect_cb->isects,
			       isect_cb->max_isects * sizeof(isects_s));
  }
  isect_cb->isects[isect_cb->nr_isects].x = (int)lrint(ix);
  isect_cb->isects[isect_cb->nr_isects].y = (int)lrint(iy);
  isect_cb->isects[isect_cb->nr_isects++].seg_idx = seg_idx;
}


/* returns coeffs of the form Ax + By + C = 0 */

static void
fget_line_from_points(double * c,
		      double s1x, double s1y,
		      double s2x, double s2y)
{
  c[0] = s1y - s2y;
  c[1] = s2x - s1x;
  c[2] = (s1x * s2y) - (s1y * s2x);
}

/* returns True if the bounding box defined by corners p1a - p1b	*/
/* overlaps the box defined by p2a - p2b				*/

static Boolean
boxes_overlap(struct f_point * p1a, struct f_point * p1b,
	      struct f_point * p2a, struct f_point * p2b)
{
  Boolean rc;
  int p1_min_x = (p1a->x < p1b->x) ? p1a->x : p1b->x;
  int p1_max_x = (p1a->x > p1b->x) ? p1a->x : p1b->x;
  int p1_min_y = (p1a->y < p1b->y) ? p1a->y : p1b->y;
  int p1_max_y = (p1a->y > p1b->y) ? p1a->y : p1b->y;
  int p2_min_x = (p2a->x < p2b->x) ? p2a->x : p2b->x;
  int p2_max_x = (p2a->x > p2b->x) ? p2a->x : p2b->x;
  int p2_min_y = (p2a->y < p2b->y) ? p2a->y : p2b->y;
  int p2_max_y = (p2a->y > p2b->y) ? p2a->y : p2b->y;
  rc = ((p1_max_x < p2_min_x) || (p1_min_x > p2_max_x) ||
	(p1_max_y < p2_min_y) || (p1_min_y > p2_max_y)) ? False : True;
  return rc;
}

static void
do_circle_ellipse_intersect(r, X, Y, e, x, y, arc, isect_cb)
     double r, X, Y;
     F_ellipse * e;
     int x, y;
     F_arc * arc;
     isect_cb_s * isect_cb;
{
  double K, L, M, N;
  double hix, hiy;
  
  double * poly;
  double * solr;
  double * soli;
  double mind = HUGE_VAL;
  int nsol, i;

  double a = (double)(e->radiuses.x);
  double b = (double)(e->radiuses.y);
  
  /* translate so center of ellipse is at origin */
  double EX = (double)(x - e->center.x);
  double EY = (double)(y - e->center.y);
  
  /* rotate around ellipse angle so ellipse semi-axes are ortho to space */
  snap_rotate_vector (&EX, &EY, EX, EY, (double)(e->angle));
  snap_rotate_vector (&X,  &Y,  X,  Y,  (double)(e->angle));

  poly = alloca(5 * sizeof(double));
  solr = alloca(4 * sizeof(double));
  soli = alloca(4 * sizeof(double));

  K = 1.0 - pow(b/a, 2.0);
  L = 2.0 * X;
  M = (pow(X, 2.0) + pow(Y, 2.0) + pow(b, 2.0)) - pow(r, 2.0);
  N = (2.0 * Y * b)/a;

  poly[4] = pow(K, 2.0);
  poly[3] = -2.0 * K * L;
  poly[2] = (2.0 * K * M) + pow(L, 2.0) + pow(N, 2.0);
  poly[1] = -2.0 * L * M;
  poly[0] = pow(M, 2.0) - pow(N * a, 2.0);

  nsol = quartic(poly, solr, soli);

  for (i = 0; i < nsol; i++) {
    if (1.0 > fabs(soli[i])) {	/* ignore complex roots */
      double dd;
      double ix, cy, ey;
      int ix_idx, cy_idx, ey_idx;
      
      for (ix_idx = 0; ix_idx < 2; ix_idx++) {			/* for +/- ix vals... */
	double rxc, rxe, dt, de;
	
	ix = (ix_idx & 1) ? -solr[i] : solr[i];
	rxc = pow(r, 2.0) - pow(ix - X, 2.0);
	rxe = pow(a, 2.0) - pow(ix, 2.0);
	if ((0.0 <= rxc) && (0.0 <= rxe)) {
	  for (cy_idx = 0; cy_idx < 2; cy_idx++) {		/* and +/- circle slns */
	    cy = Y + ((cy_idx & 1) ? -sqrt(rxc) : sqrt(rxc));
	    for (ey_idx = 0; ey_idx < 2; ey_idx++) {		/* and +/- ellipse slns */
	      Boolean point_ok;
	    
	      ey = (b/a) * ((ey_idx & 1) ? -sqrt(rxe) : sqrt(rxe));

	      if (arc) {
		double ttx, tty;
		int ittx, itty;
		snap_rotate_vector (&ttx,  &tty,  ix,  cy,  -(double)(e->angle));
		ittx = ((int)lrint(ttx)) + e->center.x;
		itty = ((int)lrint(tty)) + e->center.y;
		point_ok = is_point_on_arc(arc, ittx, itty);
	      }
	      else point_ok = True;
	      
	      if (True == point_ok) {
		dt = fabs(cy - ey);
		if (dt < 1.0) {
		  if (isect_cb) {
		    double isx, isy;
		    snap_rotate_vector (&isx,  &isy,  ix,  cy,  -(double)(e->angle));
		    isx += (double)(e->center.x);
		    isy += (double)(e->center.y);
		    insert_isect(isect_cb, isx, isy, -1);
		  }
		  dd = hypot(cy - EY, ix - EX);
		  if (dd < mind) {
		    mind = dd;
		    hix = ix;
		    hiy = cy;
		    snap_found = True;
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }

  if (True == snap_found) {
    snap_rotate_vector (&hix,  &hiy,  hix,  hiy,  -(double)(e->angle));
    snap_gx = ((int)lrint(hix)) + e->center.x;
    snap_gy = ((int)lrint(hiy)) + e->center.y;
  }
}
     
static void
circle_ellipse_intersect(c, e, x, y, isect_cb)
     F_ellipse * c;
     F_ellipse * e;
     int x, y;
     isect_cb_s * isect_cb;
{
  double r = (double)(c->radiuses.x);
  double X  = (double)(c->center.x - e->center.x);
  double Y  = (double)(c->center.y - e->center.y);

  do_circle_ellipse_intersect(r, X, Y, e, x, y, isect_cb);
}

static void
do_circle_circle(PX, PY, X2, Y2, R1, R2, OX, OY, arc, arc2, isect_cb)
     double PX;				/* event point	*/
     double PY;
     double X2;				/* translated arc center */
     double Y2;
     double R1;				/* circle radius */
     double R2;
     double OX;				/* circle -> arc displacement */
     double OY;
     F_arc * arc;
     F_arc * arc2;
     isect_cb_s * isect_cb;
{
  double dc = hypot(Y2, X2);

  if ((0.0 == dc) && (R1 == R2)) {
    put_msg("Selected circles/arcs are congruent.");
    beep();
    snap_msg_set = True;
  }
  else if (dc < (R1 + R2)) {			/* real intersection */
    double K = (pow(R1, 2.0) - pow(R2, 2.0)) + pow(dc, 2.0);
    double L = 2.0 * Y2;
    double M = 2.0 * X2;
    double A = pow(L, 2.0) + pow(M, 2.0);
    double B = -2.0 * K * L;
    double C = pow(K, 2.0) - (pow(M, 2.0) * pow(R1, 2.0));
    double rx = pow(B, 2.0) - (4.0 * A * C);
	  
    if (0.0 <= rx) {			/* should albays be true...*/
      double ix, iy;
      double hix, hiy;
      double mind = HUGE_VAL;
      int iy_idx, ix_idx, ixa_idx;

      rx = sqrt(rx)/(2.0 * A);

      for (iy_idx = 0; iy_idx < 2; iy_idx++) {
	double tix, tixa;
	iy = (-B/(2.0 * A)) + ((iy_idx & 1) ? -rx : rx);
	tix = sqrt(pow(R1, 2.0) - pow(iy, 2.0));	/* sln of circle or arc1 */
	tixa = sqrt(pow(R2, 2.0) - pow(iy - Y2, 2.0));
	for (ixa_idx = 0; ixa_idx < 2; ixa_idx++) {
	  double ixa = X2 + ((ixa_idx & 1) ? -tixa : tixa);

	  for (ix_idx = 0; ix_idx < 2; ix_idx++) {
	    double dd;
	    Boolean point_ok;
	  
	    ix = (ix_idx & 1) ? -tix : tix;
	    if (fabs(ix - ixa) < 1.0) {
	      if (arc) {
		int ittx, itty;
	  
		ittx = (int)lrint(ix + OX);
		itty = (int)lrint(iy + OY);
		point_ok = is_point_on_arc(arc, ittx, itty);

		if (arc2 && (True == point_ok))
		  point_ok = is_point_on_arc(arc2, ittx, itty);
	    
	      }
	      else point_ok = True;
	  
	      if (True == point_ok) {
		if (isect_cb) insert_isect(isect_cb, ix + OX, iy + OY, -1);
		dd = hypot(iy - PY, ix - PX);
		if (dd < mind) {
		  mind = dd;
		  hix = ix;
		  hiy = iy;
		}
	      }
	    }
	  }
	}
      }
      
      snap_gx = (int)lrint(hix + OX);
      snap_gy = (int)lrint(hiy + OY);
      snap_found = True;
    }
  }
  else if (dc == (R1 + R2)) {		/* single point tangency */
    double theta = atan2(Y2, X2);
    double ix = R1 * cos(theta);
    double iy = R1 * sin(theta);
    
    if (isect_cb) insert_isect(isect_cb, ix, iy, -1);
    snap_gx = (int)lrint(ix);
    snap_gy = (int)lrint(iy);
    snap_found = True;
  }
  else {
    /* no intersection */
  }
}

static void
circle_circle_intersect(e1, e2, x, y, isect_cb)
     F_ellipse * e1;
     F_ellipse * e2;
     int x, y;
     isect_cb_s * isect_cb;
{
  double PX = (double)(x - e1->center.x);
  double PY = (double)(y - e1->center.y);
  double X2 = (double)(e2->center.x - e1->center.x);
  double Y2 = (double)(e2->center.y - e1->center.y);
  double R1 = (double)(e1->radiuses.x);
  double R2 = (double)(e2->radiuses.x);
  double OX = (double)(e1->center.x);
  double OY = (double)(e1->center.y);
  
  do_circle_circle(PX, PY, X2, Y2, R1, R2, OX, OY, NULL, NULL, isect_cb);
}



static void
non_ortho_ellipse_ellipse_intersect(e1, e2, x, y, isect_cb)
     F_ellipse * e1;
     F_ellipse * e2;
     int x, y;
     isect_cb_s * isect_cb;
{						/* ellipse-ellipse -- non-orthogonal */
  
  double A, B, C, D;
  double SIN, COS;
  double KV, KW;
  double E, F, G, H, J, K, L, M, N, P, Q, R, S, T, U, V, W;
  double  Z0,  Z1,  Z2,  Z3,  Z4, Z5, Z6, Z7, Z8, Z9;
  double Z10, Z11, Z12, Z13, Z14;
  double * poly;
  double * solr;
  double * soli;
  double ix, iye1, iye2;
  double PX = (double)(x - e1->center.x);
  double PY = (double)(y - e1->center.y);
  double a  = (double)(e1->radiuses.x);
  double b  = (double)(e1->radiuses.y);
  double c  = (double)(e2->radiuses.x);
  double d  = (double)(e2->radiuses.y);
  double X  = (double)(e2->center.x - e1->center.x);
  double Y  = (double)(e2->center.y - e1->center.y);
  double mind = HUGE_VAL;
  double dist;
  double hix, hiy;
  int nsol, i, ix_idx, iye1_idx, iye2_idx;
  
  snap_rotate_vector (&PX, &PY, PX, PY, (double)(e1->angle));
  snap_rotate_vector (&X,  &Y,  X,  Y,  (double)(e1->angle));
  
  poly = alloca(5 * sizeof(double));
  solr = alloca(4 * sizeof(double));
  soli = alloca(4 * sizeof(double));
  
  /*
   * x^2 / a^2   +   y^2 / b^2  =  1			Eq  1,  origin ctrd, ortho, ellipse 1
   *
   * A = a^2
   * B = b^2
   *
   */
  
  A = pow(a, 2.0);
  B = pow(b, 2.0);

  /*
   * x^2 / A  +  y^2 / B  = 1				Eq  2, from Eq 1
   *
   *   B x^2  +  A y^2    =  AB				Eq  3, from Eq 2
   *
   *             A y^2    =  AB  -  B x^2		Eq  4, from Eq 3
   *
   *               y^2    =   B  -  (B/A) x^2		Eq  5, from Eq 4
   *
   *               y      = sqrt(B  -  (B/A) x^2)	Eq  6, from Eq 5
   *
   */
  
  /*
   * v^2 / c^2   +   w^2 / d^2  =  1			Eq  7,  [X Y] origin, non-ortho, ellipse 2
   *
   * C = c^2
   * D = d^2
   *
   */
  
  C = pow(c, 2.0);
  D = pow(d, 2.0);
  
  /*	
   * v^2 / C  +  w^2 / D  =  1				Eq  8, from Eq 7
   *
   *  D v^2   +  C w^2    =  CD				Eq  9, from Eq 8
   *
   * [v w] = ([x y] - [X Y]) rot theta			[v w] is a rotation of [x y] around
   *							the center of ellipse 2
   *
   * v = (x - X) cos T + (y - Y) sin T			Eq 10
   * w = (x - X) sin T - (y - Y) cos T			Eq 11
   *
   * COS = cos T
   * SIN = sin T
   *
   */

  COS = cos((double)(e1->angle - e2->angle));
  SIN = sin((double)(e1->angle - e2->angle));

  /*
   * v = x COS - X COS  +  y SIN - Y SIN		Eq 12, from Eq 10
   * w = x SIN - X SIN  -  y COS + Y COS		Eq 13, from Eq 11
   *
   * v = (x COS + y SIN)  -   (X COS + Y SIN)		Eq 14, from Eq 12
   * w = (x SIN - y COS)  -   (X SIN - Y COS)		Eq 15, from Eq 13
   *
   * KV = X COS + Y SIN
   * KW = X SIN - Y COS
   *
   */
  
  KV = (X * COS) + (Y * SIN);
  KW = (X * SIN) - (Y * COS);

  
  /*
   * v = (x COS + y SIN) -  KV				Eq 16, from Eq 14
   * w = (x SIN - y COS) -  KW				Eq 17, from Eq 15
   *
   *
   *   D (x COS + y SIN -  KV)^2   +  C  (x SIN - y COS -  KW)^2  =  CD		Eq 18, from Eq  9
   *
   *   D ( COS^2 x^2 + COS SIN xy - COS KV x					Eq 19, from Eq 18
   *                 + COS SIN xy             + SIN^2 y^2 - SIN KV y
   *                              - COS KV x              - SIN KV y    + KV^2)
   * + C ( SIN^2 x^2 - COS SIN xy - SIN KW x
   *                 - COS SIN xy             + COS^2 y^2 + COS KW y
   *                              - SIN KW x              + COS KW y    + KW^2)  = CD
   *
   *   + (   D COS^2    +    C SIN^2   ) x^2					Eq 21, from Eq 20
   *   - ( 2 D COS KV   +  2 C SIN KW  )  x
   *   + ( 2 D COS SIN  -  2 C COS SIN ) xy
   *   - ( 2 D SIN KV   -  2 C COS KW  )  y
   *   + (   D SIN^2    +    C COS^2   ) y^2
   *   + (   D KV^2     +    C KW^2  - CD )  = 0
   *
   */
  
  E =  (      D * pow(COS, 2.0)    +        C * pow(SIN, 2.0) );
  F = -(2.0 * D * COS * KV         +  2.0 * C * SIN * KW      );
  G =  (2.0 * D * COS * SIN        -  2.0 * C * COS * SIN     );
  H = -(2.0 * D * SIN * KV         -  2.0 * C * COS * KW      );
  K =  (      D * pow(SIN, 2.0)    +        C * pow(COS, 2.0) );
  L =  (    ( D * pow(KV, 2.0)     +        C * pow(KW, 2.0) ) - C * D);

   
  /*
   *
   *   E x^2  +  F x  +  G xy  +  H y  +  K y^2  +  L  = 0			Eq 22, from Eq 21
   *
   *
   *     E x^2
   *  +  F x
   *  +  K (B - (B/A) x^2)
   *  +  L
   *             =
   *  -  G x sqrt(B - (B/A) x^2)				Eq 23, from Eq 22
   *  -  H sqrt(B - (B/A) x^2)
   *
   *
   *
   *     E x^2
   *  +  F x
   *  +  K B - K(B/A) x^2
   *  +  L
   *             =
   *  -  (G x + H) sqrt(B - (B/A) x^2)				Eq 23, from Eq 22
   *
   *
   *
   *     (E - KB/A) x^2
   *  +  F x
   *  +  (KB +  L)
   *             =
   *  -  (G x + H) sqrt(B - (B/A) x^2)				Eq 23, from Eq 22
   *
   *
   */

  M = E - (K * B / A);
  N = (K * B) + L;

  /*
   *    M x^2  +  F x  +  N  =  -(G x + H) sqrt(B - (B/A) x^2)			Eq 27, from Eq 26
   *
   *   (M x^2  +  F x  +  N)^2  =  (G x + H)^2 (B - (B/A) x^2)			Eq 28, from Eq 27
   *
   *         M^2 x^4    +     FM x^3         +    MN x^2			Eq 29, from Eq 28
   *                    +     FM x^3         +   F^2 x^2  +   FN x          
   *                                         +    MN x^2  +   FN x  + N^2
   *		   = (G^2 x^2  + 2GH x  + H^2)  (B - (B/A) x^2)
   *
   *         M^2 x^4  +      2FM x^3  +  (2MN + F^2) x^2  +  2FN x  + N^2	Eq 30, from Eq 29
   *                                        -  B G^2 x^2  - 2BGH x  - B H^2
   * + (B/A) G^2 x^4  + 2(B/A)GH x^3  +    (B/A) H^2 x^2  =  0
   *
   */

  poly[4] = pow(M, 2.0)  +  (B/A) * pow(G, 2.0);
  poly[3] = 2.0 * F * M  +  2.0 * (B/A) * G * H;
  poly[2] = (2.0 * M * N  +  pow(F, 2.0)  +  (B/A) * pow(H, 2.0)) - B * pow(G, 2.0);
  poly[1] = 2.0 * F * N  -  2.0 * B * G * H;
  poly[0] = pow(N, 2.0)  -  B * pow(H, 2.0);

  nsol = quartic(poly, solr, soli);
  
  for (i = 0; i < nsol; i++) {
    if (1.0 > fabs(soli[i])) {
      for (ix_idx = 0; ix_idx < 2; ix_idx++) {
	double rx;
	ix = (ix_idx & 1) ? -solr[i] : solr[i];
	rx = A - pow(ix, 2.0);						/* for origin ellipse 1 */
	if (0.0 <= rx) {
	  for (iye1_idx = 0; iye1_idx < 2; iye1_idx++) {
	    double v, w, res;
	    iye1 = (b/a) * ((iye1_idx & 1) ? -sqrt(rx) : sqrt(rx));		/* e1 */
	    
	    v = (ix - X) * COS + (iye1 - Y) * SIN;
	    w = (ix - X) * SIN - (iye1 - Y) * COS;
	    res = (pow(v, 2.0) / C)  +  (pow(w, 2.0) / D);

	    if (fabs(1.0 - res) < 0.0001) {
	      if (isect_cb) {
		double isx, isy;
		snap_rotate_vector (&isx,  &isy,  ix,  iye1,  -(double)(e1->angle));
		isx += (double)(e1->center.x);
		isy += (double)(e1->center.y);
		insert_isect(isect_cb, isx, isy, -1);
	      }
	      dist = hypot(iye1 - PY, ix - PX);
	      if (dist < mind) {
		mind = dist;
		hix = ix;
		hiy = iye1;
		snap_found = True;
	      }
	    }
	  }
	}
      }
    }
  }
  if (True == snap_found) {
    snap_rotate_vector (&hix, &hiy, hix, hiy, -(double)(e1->angle));
    snap_gx = (int)lrint(hix) + e1->center.x;
    snap_gy = (int)lrint(hiy) + e1->center.y;
  }
}


static void
ortho_ellipse_ellipse_intersect(e1, e2, x, y, isect_cb)
     F_ellipse * e1;
     F_ellipse * e2;
     int x, y;
     isect_cb_s * isect_cb;
{						/* ellipse-ellipse -- orthogonal */
  /*
   * x^2 / a^2   +   y^2 / b2  =  1					Eq  1,  origin ctrd, eclipse 1
   *
   * (x - X)^2 / c^2  +  (y - Y)^2 / d^2  =  1				Eq  2.  [X Y] ctrd eclipse 2
   *
   * A = a^2
   * B = b^2
   * C = c^2
   * D = d^2
   *
   * x^2 / A  +  y^2 / B  = 1						Eq  3, from Eq 1
   *	
   * (x - X)^2 / C  +  (y - Y)^2 / D  =  1				Eq  4, from Eq 2
   *
   * D(x - X)^2 / C  +  (y - Y)^2  =  D					Eq  4a, from Eq 4
   *
   *                    (y - Y)^2  =  D - D(x - X)^2 / C			Eq  4b, from Eq 4a
   *
   *                    (y - Y)^2  =  D(1 - (x - X)^2 / C)		Eq  4c, from Eq 4b
   *
   *		      (y - Y)^2  =  (D/C)(C - (x - X)^2)		Eq  4d, from Eq 4c
   *
   *		      (y - Y)    =  (d/c)sqrt(C - (x - X)^2)		Eq  4e, from Eq 4d
   *
   *		      y  =  Y  + (d/c)sqrt(C - (x - X)^2)		Eq  4f, from Eq 4e
   * 
   * B x^2  +  A y^2  =  AB						Eq  5, from Eq 3
   * 
   *           A y^2  =  AB  -  B x^2					Eq  6, from Eq 5
   *
   *             y^2  =  B   -  (B/A) x^2	 =  B(A - x^2)/A		Eq  7, from Eq 6
   *
   *	       y    =  (b/a)sqrt(A - x^2)				Eq  8, from Eq 7b
   *
   * D(x - X)^2  +  C(y - Y)^2  = CD					Eq  9, from Eq 4
   *
   * D(x^2 - 2Xx + X^2)  +  C(y^2 - 2Yy + Y^2)  =  CD			Eq 10, from Eq 9
   *
   * Dx^2 - 2DXx + DX^2  +  Cy^2 - 2CYy + CY^2  =  CD			Eq 11, from Eq 10
   *
   *	plug Eq 7a and Eq 8 into Eq 11
   *
   * Dx^2 - 2DXx + DX^2  +  C(B - (B/A) x^2) - 2CY((b/a)sqrt(A - x^2)) + CY^2  =  CD	Eq 12, from Eq 11
   *
   * Dx^2 - 2DXx + DX^2  +  CB - C(B/A) x^2 - 2CY((b/a)sqrt(A - x^2)) + CY^2  =  CD	Eq 13, from Eq 12
   *
   * (D - CB/A)x^2 - 2DXx + (DX^2 + CY^2 +  CB - CD)  =   2CY((b/a)sqrt(A - x^2))	Eq 14, from Eq 13
   *
   * E = D - CB/A
   * F = 2DX
   * G = DX^2 + CY^2 +  CB - CD
   * H = 2CY((b/a)
   *
   * Ex^2 - Fx + G  =   H sqrt(A - x^2)						Eq 15, from Eq 14
   *
   * (Ex^2 - Fx + G)^2  =   H^2 (A - x^2)						Eq 16, from Eq 15
   *
   * E^2 x^4  -  EF x^3  +   EG x^2
   *          -  EF x^3  +  F^2 x^2  -  FG x
   *                     +   EG x^2  -  FG x  +  G^2  = H^2 A  -  H^2 x^2		Eq 17, from Eq 16
   *
   * E^2 x^4 - 2EF x^3 + (2EG + F^2) x^2 - 2FG x + G^2 = H^2 A - H^2 x^2		Eq 18, from Eq 17
   *
   * E^2 x^4 - 2EF x^3 + (2EG + F^2 + H^2) x^2 - 2FG x + (G^2 - H^2 A) = 0	Eq 19, from Eq 18
   *
   * J = E^2
   * K = 2EF
   * L = 2EG + F^2 + H^2
   * M = 2FG
   * N = G^2 - H^2 A
   *
   * J x^4 - K x^3 + L x^2 - M x + N = 0					Eq 20, from Eq 19
   *
   */
  
  double A, B, C, D;
  double E, F, G, H, J, K, L, M, N;
  double * poly;
  double * solr;
  double * soli;
  double ix, iye1, iye2;
  double PX = (double)(x - e1->center.x);
  double PY = (double)(y - e1->center.y);
  double a = (double)(e1->radiuses.x);
  double b = (double)(e1->radiuses.y);
  double c = (double)(e2->radiuses.x);
  double d = (double)(e2->radiuses.y);
  double X = (double)(e2->center.x - e1->center.x);
  double Y = (double)(e2->center.y - e1->center.y);
  double mind = HUGE_VAL;
  double dist;
  double hix, hiy;
  int nsol, i, ix_idx, iye1_idx, iye2_idx;
  
  snap_rotate_vector (&PX, &PY, PX, PY, (double)(e1->angle));
  snap_rotate_vector (&X,  &Y,  X,  Y,  (double)(e1->angle));
  
  poly = alloca(5 * sizeof(double));
  solr = alloca(4 * sizeof(double));
  soli = alloca(4 * sizeof(double));
  
  A = pow(a, 2.0);
  B = pow(b, 2.0);
  C = pow(c, 2.0);
  D = pow(d, 2.0);
  
  E = D - (C * B)/A;
  F = 2.0 * D * X;
  G = ((D * pow(X, 2.0)) + (C * pow(Y, 2.0)) +  (C * B)) - (C * D);
  H = 2.0 * C * Y * (b/a);
  
  J = pow(E, 2.0);
  K = 2.0 * E * F;
  L = (2.0 * E * G) + pow(F, 2.0) + pow(H, 2.0);
  M = 2.0 * F * G;
  N = pow(G, 2.0) - (pow(H, 2.0) * A);

  poly[4] =  J;
  poly[3] = -K;
  poly[2] =  L;
  poly[1] = -M;
  poly[0] =  N;

  nsol = quartic(poly, solr, soli);
  
  for (i = 0; i < nsol; i++) {
    if (1.0 > fabs(soli[i])) {
      for (ix_idx = 0; ix_idx < 2; ix_idx++) {
	double rx, ex;
	ix = (ix_idx & 1) ? -solr[i] : solr[i];
	rx = A - pow(ix, 2.0);						/* for origin ellipse 1 */
	if (0.0 <= rx) {
	  for (iye1_idx = 0; iye1_idx < 2; iye1_idx++) {
	    iye1 = (b/a) * ((iye1_idx & 1) ? -sqrt(rx) : sqrt(rx));		/* e1 */
	    
	    /* y  =  Y  + (d/c)sqrt(C - (x - X)^2) */
	    
	    ex = C - pow(ix - X, 2.0);					/* for [X Y] ellipse 2 */
	    if (0.0 <= ex) {
	      for (iye2_idx = 0; iye2_idx < 2; iye2_idx++) {
		iye2 = Y + ((d/c) * ((iye2_idx & 1) ? -sqrt(ex) : sqrt(ex)));
		if (fabs(iye1 - iye2) < 1.0) {
		  if (isect_cb) {
		    double isx, isy;
		    snap_rotate_vector (&isx,  &isy,  ix,  iye1,  -(double)(e1->angle));
		    isx += (double)(e1->center.x);
		    isy += (double)(e1->center.y);
		    insert_isect(isect_cb, isx, isy, -1);
		  }
		  dist = hypot(iye1 - PY, ix - PX);
		  if (dist < mind) {
		    mind = dist;
		    hix = ix;
		    hiy = iye1;
		    snap_found = True;
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
  if (True == snap_found) {
    snap_rotate_vector (&hix, &hiy, hix, hiy, -(double)(e1->angle));
    snap_gx = (int)lrint(hix) + e1->center.x;
    snap_gy = (int)lrint(hiy) + e1->center.y;
  }
}

void
intersect_ellipse_ellipse_handler(F_ellipse *  e1, F_ellipse *  e2, int x, int y, isect_cb_s * isect_cb )
{
  switch(e1->type) {
  case T_ELLIPSE_BY_RAD:
  case T_ELLIPSE_BY_DIA:
    switch(e2->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      if (e1->angle == e2->angle) ortho_ellipse_ellipse_intersect(e1, e2, x, y, isect_cb);
      else non_ortho_ellipse_ellipse_intersect(e1, e2, x, y, isect_cb);
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      circle_ellipse_intersect(e2, e1, x, y, isect_cb);
      break;
    }
    break;
  case T_CIRCLE_BY_RAD:
  case T_CIRCLE_BY_DIA:
    switch(e2->type) {
    case T_ELLIPSE_BY_RAD:
    case T_ELLIPSE_BY_DIA:
      circle_ellipse_intersect(e1, e2, x, y, isect_cb);
      break;
    case T_CIRCLE_BY_RAD:
    case T_CIRCLE_BY_DIA:
      circle_circle_intersect(e1, e2, x, y, isect_cb);
      break;
    }
    break;
  }
}

static void
do_intersect_ellipse_polyline(ecx, ecy, ea, eb, theta, l, x, y, arc, isect_cb)
     double ecx;
     double ecy;
     double ea;
     double eb;
     double theta;
     F_line * l;
     int x, y;
     F_arc * arc;
     isect_cb_s * isect_cb;
{
  /* if arc is non-null, ecx, ecy, ea, and eb arc from arc.*/
  /* if isect_cb is non-null, return the intersects */
  struct f_point * p;
  struct f_point * p_start;
  double dist;
  double mind;
  double c[3];
  double ix1, iy1;
  double ix2, iy2;
  int seg_idx;

  /* fixme -- handle ellipse v. circle */
  
  mind = HUGE_VAL;
  p_start = NULL;
  for (seg_idx = -1, p = l->points; p != NULL; seg_idx++, p = p->next) {
    if (p_start) {
      double rx;
      /* translate to ellipse origin */
      double psx = (double)(p_start->x) - ecx;
      double psy = (double)(p_start->y) - ecy;
      double pex = (double)(p->x) - ecx;
      double pey = (double)(p->y) - ecy;
      
      /* rotate around ellipse angle so ellipse semi-axes are ortho to space */
      snap_rotate_vector (&psx, &psy, psx, psy, theta);
      snap_rotate_vector (&pex, &pey, pex, pey, theta);
      
      if  (1) {					/* fixme find a quick screening test */
	int iset = 0;
	double M = pow(eb, 2.0);		
	double N = pow(ea, 2.0);
	fget_line_from_points(c, psx, psy, pex, pey);

	if ((0.0 == c[0]) && (0.0 != c[1]) && (0.0 != M)) {	/* a == 0; horzontal */
	  iy1 = iy2 = -c[2]/c[1];
	  rx = N - ((N/M) * pow(iy1, 2.0));
	  if (0.0 <= rx) {			/* tangent or normal two-point intersection */
	    ix1 =  sqrt(rx);
	    ix2 = -sqrt(rx);
	    iset = ISET_P1 | ISET_P2;
	  }
	  else if (-1.0 < rx) {			/* near miss sometimes caused by roundoff */
	    ix1 = ix2 = 0.0;
	    iset = ISET_P1;
	  }
	}
	else if ((0.0 != c[0]) && (0.0 == c[1]) && (0.0 != N)) {	/* b == 0; vertical */
	  ix1 = ix2 = -c[2]/c[0];
	  rx = M - ((M/N) * pow(ix1, 2.0));
	  if (0.0 <= rx) {			/* tangent or normal two-point intersection */
	    iy1 =  sqrt(rx);
	    iy2 = -sqrt(rx);
	    iset = ISET_P1 | ISET_P2;
	  }
	  else if (-1.0 < rx) {			/* near miss sometimes caused by roundoff */
	    iy1 = iy2 = 0.0;
	    iset = ISET_P1;
	  }
	}
	else if ((0.0 != c[0]) && (0.0 != c[1])) {
	  /* simplify things... */
	  /* normalise line coeffs... */
	  double K = -c[1]/c[0];
	  double L = -c[2]/c[0];
	  
	  double P = M * N;
	  
	  /* plug line into ellipse... */
	  double W = (M * pow(K, 2.0)) + N;
	  double X = 2.0 * K * L * M;
	  double Y = (M * pow(L, 2.0)) - P;
	  if (0.0 != W) {
	    rx = pow(X, 2.0) - (4.0 * W * Y);
	    if (0.0 <= rx) {
	      iy1 = ((-X) + sqrt(rx))/(2.0 * W);
	      ix1 = (K * iy1) + L;
	      iy2 = ((-X) - sqrt(rx))/(2.0 * W);
	      ix2 = (K * iy2) + L;
	      iset = ISET_P1 | ISET_P2;
	    }
	    else if (fabs(rx) < 10.0) {	/* fixme -- (too?) small arbitrary tolerance */
	      /* due to roundoff errors, the intersection of tangents to ellipses and
	       * circles may be slightly complex.  this code catches such near misses.
	       */
#if 0
	      double complex ix1c;
	      double complex iy1c;
	      double complex ix2c;
	      double complex iy2c;
	      double x1mag, y1mag;
	      double x2mag, y2mag;

	      iy1c = ((-X) + (sqrt(-rx) *  _Complex_I))/(2.0 * W);
	      ix1c = (K * iy1c) + L;
	      
	      iy2c = ((-X) - (sqrt(-rx) *  _Complex_I))/(2.0 * W);
	      ix2c = (K * iy2c) + L;

	      ix1 = creal(ix1c);
	      iy1 = creal(iy1c);
	      ix2 = creal(ix2c);
	      iy2 = creal(iy2c);

	      x1mag = hypot(ix1, cimag(ix1c));
	      y1mag = hypot(iy1, cimag(iy1c));

	      x2mag = hypot(ix2, cimag(ix2c));
	      y2mag = hypot(iy2, cimag(iy2c));
	      
	      if ((1.0 > fabs(ix1) - x1mag) &&
		  (1.0 > fabs(iy1) - y1mag) &&
		  (1.0 > fabs(ix2) - x2mag) &&
		  (1.0 > fabs(iy2) - y2mag)) iset = ISET_P1 | ISET_P2;
#endif
	    }
	  }
	}

	if (0 != iset) {
	  Boolean point_ok;
	
	  if ((iset & ISET_P1) && (!is_point_on_segment(psx, psy, ix1, iy1, pex, pey)))
	    iset &= ~ISET_P1;
	  if ((iset & ISET_P2) && (!is_point_on_segment(psx, psy, ix2, iy2, pex, pey)))
	    iset &= ~ISET_P2;
	  
	  /* rotate back to screen space */
	  snap_rotate_vector (&ix1, &iy1, ix1, iy1, -theta);
	  snap_rotate_vector (&ix2, &iy2, ix2, iy2, -theta);
	      
	  /* translate back to screen space */
	  ix1 +=  ecx;
	  iy1 +=  ecy;
	  ix2 +=  ecx;
	  iy2 +=  ecy;

	  if (iset & ISET_P1) {
	    point_ok = (NULL == arc)
	      ? True
	      : is_point_on_arc(arc, (int)lrint(ix1), (int)lrint(iy1));
	    if (True == point_ok) {
	      if (isect_cb) insert_isect(isect_cb, ix1, iy1, seg_idx);
	      dist = hypot(iy1 - (double)y, ix1 - (double)x);
	      if (dist < mind) {
		mind = dist;
		snap_gx = (int)lrint(ix1);
		snap_gy = (int)lrint(iy1);
		snap_found = True;
	      }
	    }
	  }
	  
	  if (iset & ISET_P2) {
	    point_ok = (NULL == arc)
	      ? True
	      :	 is_point_on_arc(arc, (int)lrint(ix2), (int)lrint(iy2));
	    if (True == point_ok) {
	      if (isect_cb) insert_isect(isect_cb, ix2, iy2, seg_idx);
	      dist = hypot(iy2 - (double)y, ix2 - (double)x);
	      if (dist < mind) {
		mind = dist;
		snap_gx = (int)lrint(ix2);
		snap_gy = (int)lrint(iy2);
		snap_found = True;
	      }
	    }
	  }
	  
	  if (True == snap_found) {
	    /* check if we're near a polyline segment endpoint.  if so, snap to it. */
	    double de1 = hypot((double)(p_start->y - snap_gy),
			       (double)(p_start->x - snap_gx));
	    double de2 = hypot((double)(p->y - snap_gy),
				 (double)(p->x - snap_gx));
	    if (5.0 > de1) {			/* arbitarary tolerance... */
	      snap_gx = p_start->x;
	      snap_gy = p_start->y;
	    }
	    else if (5.0 > de2) {		/* arbitarary tolerance... */
	      snap_gx = p->x;
	      snap_gy = p->y;
	    }
	    snap_found = (NULL == arc) ? True : is_point_on_arc(arc, snap_gx, snap_gy);
	  }
	}
      }
    }
    p_start = p;
  }
  if (!isect_cb) {
    if (False == snap_found) {
      put_msg("Selected ellipse and polyline do not intersect.");
      beep();
      snap_msg_set = True;
    }
  }
}

void
intersect_ellipse_polyline_handler(F_ellipse * e, F_line *  l, int x, int y, isect_cb_s * isect_cb)
{
  double ecx = (double)(e->center.x);
  double ecy = (double)(e->center.y);
  double ea  = (double)(e->radiuses.x);
  double eb  = (double)(e->radiuses.y);
  double theta = (double)(e->angle);

  do_intersect_ellipse_polyline(ecx, ecy, ea, eb, theta, l, x, y, NULL, isect_cb);
}

static void
intersect_ellipse_spline_handler(obj1, obj2, x, y)
     void * obj1;
     void * obj2;
     int x, y;
{
  put_msg("Ellipse-spline intersections not yet implemented");
  beep();
  snap_msg_set = True;
}

void
delete_text_bounding_box(F_line * l)
{
  if (l) {
    if (l->points) free(l->points);
    free(l);
  }
}

F_line *
build_text_bounding_box(F_text * t) 
{
  /*
   * do everything wrt to a (rotated) square bounding box around the text.
   * the lower corners are at the descent, the upper at the ascent.
   *
   */
  struct f_point * points;
  int i;
  F_line * f_line_p = malloc(sizeof(F_line));
  f_line_p->type = T_BOX;
  f_line_p->points = points = malloc(5 * sizeof(struct f_point));

  points[0].x = 0;		points[0].y =   t->descent;
  points[1].x = t->length;	points[1].y =   t->descent;
  points[2].x = t->length;	points[2].y = -(t->ascent);
  points[3].x = 0;		points[3].y = -(t->ascent);
  points[4] = points[0];

  for (i = 0; i < 5; i++) {
    double tx, ty;
    switch(t->type) {
    case T_LEFT_JUSTIFIED:
      /* do nothing */
      break;
    case T_CENTER_JUSTIFIED:
      points[i].x -= (t->length)/2;
      break;
    case T_RIGHT_JUSTIFIED:
      points[i].x -= t->length;
      break;
    }
    tx = (double)points[i].x;
    ty = (double)points[i].y;
    snap_rotate_vector(&tx, &ty, tx, ty, -(double)(t->angle));
    points[i].x = t->base_x + (int)lrint(tx);
    points[i].y = t->base_y + (int)lrint(ty);
    points[i].next = (i < 4) ? &points[i+1] : NULL;
  }
  return f_line_p;
}

static void
intersect_ellipse_text_handler(e, t, x, y)
     F_ellipse * e;
     F_text * t;
     int x, y;
{
  F_line * f_line_p = build_text_bounding_box(t);
  intersect_ellipse_polyline_handler(e, f_line_p, x, y, NULL);
  delete_text_bounding_box(f_line_p);
}

void
intersect_ellipse_arc_handler(F_ellipse * e, F_arc * a, int x, int y, isect_cb_s * isect_cb)
{
  switch(e->type) {
  case T_ELLIPSE_BY_RAD:
  case T_ELLIPSE_BY_DIA:
    {
      double r = hypot((double)(a->center.y) - (double)(a->point[1].y),
		       (double)(a->center.x) - (double)(a->point[1].x));
      double X  = (double)(a->center.x) - (double)(e->center.x);
      double Y  = (double)(a->center.y) - (double)(e->center.y);
 
      do_circle_ellipse_intersect(r, X, Y, e, x, y, a, isect_cb);
    }
    break;
  case T_CIRCLE_BY_RAD:
  case T_CIRCLE_BY_DIA:
    {
      double PX = (double)(x - e->center.x);
      double PY = (double)(y - e->center.y);
      double X2 = (double)(a->center.x) - (double)(e->center.x);
      double Y2 = (double)(a->center.y) - (double)(e->center.y);
      double R1 = (double)(e->radiuses.x);
      double R2 =  hypot((double)(a->center.y) - (double)(a->point[1].y),
			 (double)(a->center.x) - (double)(a->point[1].x));
      double OX = (double)(e->center.x);
      double OY = (double)(e->center.y);
  
      do_circle_circle(PX, PY, X2, Y2, R1, R2, OX, OY, a, isect_cb);
    }
  }
}

void
intersect_polyline_polyline_handler(F_line * l1, F_line * l2, int x, int y, isect_cb_s * isect_cb)
{
  struct f_point * p1;
  struct f_point * p2;
  struct f_point * p1_start;
  struct f_point * p2_start;
  double c1[3];
  double c2[3];
#define lA c1[0]
#define lB c1[1]
#define lC c1[2]
#define lD c2[0]
#define lE c2[1]
#define lF c2[2]
  double ci[2][2];
#define ciA ci[0][0]  
#define ciB ci[0][1]  
#define ciC ci[1][0]  
#define ciD ci[1][1]
  double det;
  double ix, iy;
  double dist;
  double mind;
  int seg_idx;

  mind = HUGE_VAL;
  p1_start = NULL;
  for (p1 = l1->points; p1 != NULL; p1 = p1->next) {
    if (p1_start) {
      get_line_from_points(c1, p1_start, p1);
      p2_start = NULL;
      for (seg_idx = -1, p2 = l2->points; p2 != NULL; seg_idx++, p2 = p2->next) {
	if (p2_start) {
	  if (True == boxes_overlap(p1_start, p1, p2_start, p2)) {
	    get_line_from_points(c2, p2_start, p2);
	    det = (lB * lD) - (lA * lE);
	    ciA = lE/det;
	    ciB = -lD/det;
	    ciC = -lB/det;
	    ciD = lA/det;
	    ix = (lC * ciA) + (lF * ciC);
	    iy = (lC * ciB) + (lF * ciD);
	    snap_found = True;
	    if (isect_cb) {
	      if ((True == is_point_on_segment((double)(p1_start->x), (double)(p1_start->y),
					       ix, iy,
					       (double)(p1->x), (double)(p1->y))) && 
		  (True == is_point_on_segment((double)(p2_start->x), (double)(p2_start->y),
					       ix, iy,
					       (double)(p2->x), (double)(p2->y))))
		insert_isect(isect_cb, ix, iy, seg_idx);
	    }
	    else {
	      dist = hypot(iy - (double)y, ix - (double)x);
	      if (dist < mind) {
		mind = dist;
		snap_gx = (int)lrint(ix);
		snap_gy = (int)lrint(iy);
	      }
	    }
	  }
	}
	p2_start = p2;
      }
    }
    p1_start = p1;
  }
  if (!isect_cb) {
    if (False == snap_found) {
      put_msg("Selected polylines do not intersect.");
      beep();
      snap_msg_set = True;
    }
  }
}

static void
intersect_polyline_spline_handler(obj1, obj2, x, y)
     void * obj1;
     void * obj2;
     int x, y;
{
  put_msg("Polyline-spline intersections not yet implemented");
  beep();
  snap_msg_set = True;
}

static void
intersect_polyline_text_handler(l, t, x, y)
     F_line * l;
     F_text * t;
     int x, y;
{
  F_line * f_line_p = build_text_bounding_box(t);
  intersect_polyline_polyline_handler(l, f_line_p, x, y, NULL);
  delete_text_bounding_box(f_line_p);
}

void
intersect_polyline_arc_handler(F_line * l,  F_arc *a,  int x, int y, isect_cb_s * isect_cb)
{
  double ecx = (double)(a->center.x);
  double ecy = (double)(a->center.y);
  double ea  =  hypot((double)(a->center.y) - (double)(a->point[1].y),
		      (double)(a->center.x) - (double)(a->point[1].x));
  double theta = 0.0;

  do_intersect_ellipse_polyline(ecx, ecy, ea, ea, theta, l, x, y, a, isect_cb);
}

static void
intersect_spline_spline_handler(obj1, obj2, x, y)
     void * obj1;
     void * obj2;
     int x, y;
{
  put_msg("Spline-spline intersections not yet implemented");
  beep();
  snap_msg_set = True;
}

static void
intersect_spline_text_handler(s, t, x, y)
     F_spline * s;
     F_text * t;
     int x, y;
{
  put_msg("Spline-text intersections not yet implemented");
  beep();
  snap_msg_set = True;
}

static void
intersect_spline_arc_handler(obj1, obj2, x, y)
     void * obj1;
     void * obj2;
     int x, y;
{
  put_msg("Spline-arc intersections not yet implemented");
  beep();
  snap_msg_set = True;
}

static void
intersect_text_text_handler(t1, t2, x, y)
     F_text * t1;
     F_text * t2;
     int x, y;
{
  F_line * f1_line_p = build_text_bounding_box(t1);
  F_line * f2_line_p = build_text_bounding_box(t2);
  intersect_polyline_polyline_handler(f1_line_p, f2_line_p, x, y, NULL);
  delete_text_bounding_box(f1_line_p);
  delete_text_bounding_box(f2_line_p);
}

static void
intersect_text_arc_handler(t, a, x, y)
     F_text * t;
     F_arc * a;
     int x, y;
{
  F_line * f_line_p = build_text_bounding_box(t);
  intersect_polyline_arc_handler(f_line_p, a, x, y, NULL);
  delete_text_bounding_box(f_line_p);
}

void
intersect_arc_arc_handler(F_arc * a1, F_arc * a2, int x, int y, isect_cb_s * isect_cb)
{
  /* translate to center of a1 */
  double PX = (double)(x) - (double)(a1->center.x);	/* translated event point */
  double PY = (double)(y) - (double)(a1->center.y);
  double X2 = (double)(a2->center.x - a1->center.x);	/* translated center of a2 */
  double Y2 = (double)(a2->center.y - a1->center.y);
  double R1 = hypot((double)(a1->center.y) - (double)(a1->point[1].y),
		    (double)(a1->center.x) - (double)(a1->point[1].x));
  double R2 = hypot((double)(a2->center.y) - (double)(a2->point[1].y),
		    (double)(a2->center.x) - (double)(a2->point[1].x));
  double OX = (double)(a1->center.x);
  double OY = (double)(a1->center.y);
  
  do_circle_circle(PX, PY, X2, Y2, R1, R2, OX, OY, a1, a2, isect_cb);
}

static void
intersect_ellipse_handler(obj1, obj2, type2, x, y)
     void * obj1;
     void * obj2;
     int type2;
     int x, y;
{
  switch (type2) {
  case O_ELLIPSE:
    intersect_ellipse_ellipse_handler(obj1, obj2, x, y, NULL);
    break;
  case O_POLYLINE:
    intersect_ellipse_polyline_handler(obj1, obj2, x, y, NULL);
    break;
  case O_SPLINE:
    intersect_ellipse_spline_handler(obj1, obj2, x, y);
    break;
  case O_TEXT:
    intersect_ellipse_text_handler(obj1, obj2, x, y);
    break;
  case O_ARC:
    intersect_ellipse_arc_handler(obj1, obj2, x, y, NULL);
    break;
  }
}

static void
intersect_polyline_handler(obj1, obj2, type2, x, y)
     void * obj1;
     void * obj2;
     int type2;
     int x, y;
{
  switch (type2) {
  case O_ELLIPSE:
    intersect_ellipse_polyline_handler(obj2, obj1, x, y, NULL);
    break;
  case O_POLYLINE:
    intersect_polyline_polyline_handler(obj1, obj2, x, y, NULL);
    break;
  case O_SPLINE:
    intersect_polyline_spline_handler(obj1, obj2, x, y);
    break;
  case O_TEXT:
    intersect_polyline_text_handler(obj1, obj2, x, y);
    break;
  case O_ARC:
    intersect_polyline_arc_handler(obj1, obj2, x, y, NULL);
    break;
  }
}

static void
intersect_spline_handler(obj1, obj2, type2, x, y)
     void * obj1;
     void * obj2;
     int type2;
     int x, y;
{
  switch (type2) {
  case O_ELLIPSE:
    intersect_ellipse_spline_handler(obj2, obj1, x, y);
    break;
  case O_POLYLINE:
    intersect_polyline_spline_handler(obj2, obj2, x, y);
    break;
  case O_SPLINE:
    intersect_spline_spline_handler(obj1, obj2, x, y);
    break;
  case O_TEXT:
    intersect_spline_text_handler(obj1, obj2, x, y);
    break;
  case O_ARC:
    intersect_spline_arc_handler(obj1, obj2, x, y);
    break;
  }
}

static void
intersect_text_handler(obj1, obj2, type2, x, y)
     void * obj1;
     void * obj2;
     int type2;
     int x, y;
{
  switch (type2) {
  case O_ELLIPSE:
    intersect_ellipse_text_handler(obj2, obj1, x, y);
    break;
  case O_POLYLINE:
    intersect_polyline_text_handler(obj2, obj2, x, y);
    break;
  case O_SPLINE:
    intersect_spline_text_handler(obj2, obj1, x, y);
    break;
  case O_TEXT:
    intersect_text_text_handler(obj1, obj2, x, y);
    break;
  case O_ARC:
    intersect_text_arc_handler(obj1, obj2, x, y);
    break;
  }
}

static void
intersect_arc_handler(obj1, obj2, type2, x, y)
     void * obj1;
     void * obj2;
     int type2;
     int x, y;
{
  switch (type2) {
  case O_ELLIPSE:
    intersect_ellipse_arc_handler(obj2, obj1, x, y, NULL);
    break;
  case O_POLYLINE:
    intersect_polyline_arc_handler(obj2, obj1, x, y, NULL);
    break;
  case O_SPLINE:
    intersect_spline_arc_handler(obj2, obj1, x, y);
    break;
  case O_TEXT:
    intersect_text_arc_handler(obj2, obj1, x, y);
    break;
  case O_ARC:
    intersect_arc_arc_handler(obj1, obj2, x, y, NULL);
    break;
  }
}

void
snap_intersect_handler(obj1, type1, obj2, type2, x, y)
     void * obj1;
     int type1;
     void * obj2;
     int type2;
     int x, y;
{
  switch (type1) {
  case O_ELLIPSE:
    intersect_ellipse_handler(obj1, obj2, type2, x, y);
    break;
  case O_POLYLINE:
    intersect_polyline_handler(obj1, obj2, type2, x, y);
    break;
  case O_SPLINE:
    intersect_spline_handler(obj1, obj2, type2, x, y);
    break;
  case O_TEXT:
    intersect_text_handler(obj1, obj2, type2, x, y);
    break;
  case O_ARC:
    intersect_arc_handler(obj1, obj2, type2, x, y);
    break;
  }
  if ((False == snap_found) && (False == snap_msg_set)) {
    put_msg("No intersection found.");
    beep();
    snap_msg_set = True;
  }
}


