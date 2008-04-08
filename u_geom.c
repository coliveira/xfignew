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

/*
 * Routines dealing with geometry under the following headings:
 *	COMPUTE NORMAL, CLOSE TO VECTOR, COMPUTE ARC CENTER,
 *	COMPUTE ANGLE, COMPUTE DIRECTION, LATEX LINE ROUTINES.
 */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "u_geom.h"

/*************************** ROTATE VECTOR **********************
In fact, rotate & scale ;-)                                    */


int compute_arccenter (F_pos p1, F_pos p2, F_pos p3, float *x, float *y);
int gcd (int a, int b);

static void
rotate_vector(double *vx, double *vy, double c, double s)
{
    double wx, wy;
    wx = c * (*vx) - s * (*vy);
    wy = s * (*vx) + c * (*vy);
    *vx = wx;
    *vy = wy;
}


/*************************** COMPUTE NORMAL **********************

Input arguments :
	(x1,y1)(x2,y2) : the vector
	direction : direction of the normal vector to (x1,y1)(x2,y2)
Output arguments :
	(*x,*y)(x2,y2) : a normal vector.
Return value : none

******************************************************************/

void compute_normal(float x1, float y1, int x2, int y2, int direction, int *x, int *y)
{
    if (direction) {		/* counter clockwise  */
	*x = round(x2 - (y2 - y1));
	*y = round(y2 - (x1 - x2));
    } else {
	*x = round(x2 + (y2 - y1));
	*y = round(y2 + (x1 - x2));
    }
}

/******************** CLOSE TO VECTOR **************************

Input arguments:
	(x1,y1)(x2,y2) : the vector
	(xp,yp) : the point
	d : tolerance (max. allowable distance from the point to the vector)
	dd : d * d
Output arguments:
	(*px,*py) : a point on the vector which is not far from (xp,yp)
		by more than d. Normally the vector (*px,*py)(xp,yp)
		is normal to vector (x1,y1)(x2,y2) except when (xp,yp)
		is within d from (x1,y1) or (x2,y2), in which cases,
		(*px,*py) = (x1,y1) or (x2,y2) respectively.
Return value :
	0 : No point on the vector is within d from (xp, yp)
	1 : (*px, *py) is such a point.

******************************************************************/

int close_to_vector(int x1, int y1, int x2, int y2, int xp, int yp, int d, float dd, int *px, int *py)
{
    int		    xmin, ymin, xmax, ymax;
    float	    x, y, slope, D2, dx, dy;

    if (abs(xp - x1) <= d && abs(yp - y1) <= d) {
	*px = x1;
	*py = y1;
	return (1);
    }
    if (abs(xp - x2) <= d && abs(yp - y2) <= d) {
	*px = x2;
	*py = y2;
	return (1);
    }
    if (x1 < x2) {
	xmin = x1 - d;
	xmax = x2 + d;
    } else {
	xmin = x2 - d;
	xmax = x1 + d;
    }
    if (xp < xmin || xmax < xp)
	return (0);

    if (y1 < y2) {
	ymin = y1 - d;
	ymax = y2 + d;
    } else {
	ymin = y2 - d;
	ymax = y1 + d;
    }
    if (yp < ymin || ymax < yp)
	return (0);

    if (x2 == x1) {
	x = x1;
	y = yp;
    } else if (y1 == y2) {
	x = xp;
	y = y1;
    } else {
	slope = ((float) (x2 - x1)) / ((float) (y2 - y1));
	y = (slope * (xp - x1 + slope * y1) + yp) / (1 + slope * slope);
	x = ((float) x1) + slope * (y - y1);
    }
    dx = ((float) xp) - x;
    dy = ((float) yp) - y;
    D2 = dx * dx + dy * dy;
    if (D2 < dd) {
	*px = round(x);
	*py = round(y);
	return (1);
    }
    return (0);
}

/********************* COMPUTE ARC RADIUS ******************
Input arguments :
	(x1, y1), (x2, y2), (x3, y3): 3 points on arc
Output arguments :
	(*r) : Radius of the arc
Return value :
	0 : if p1, p2, p3 are co-linear.
	1 : if they are not.
*************************************************************/

int
compute_arcradius(int x1, int y1, int x2, int y2, int x3, int y3, float *r)
{
   F_pos p1, p2, p3;
   float cx, cy, dx, dy;

   p1.x = x1; p1.y = y1;
   p2.x = x2; p2.y = y2;
   p3.x = x3; p3.y = y3;

   if (!compute_arccenter(p1, p2, p3, &cx, &cy))
     return 0;
   dx = x2 - cx;
   dy = y2 - cy;
   if (dx == 0.0 && dy == 0.0) {
     *r = 0.0;
     return 0;
   }
   else {
     *r = sqrt(dx * dx + dy * dy);
     return 1;
   }
}


/********************* COMPUTE ARC CENTER ******************

New routine 12/11/00 from Thomas Henlich - better at finding center

Input arguments :
	p1, p2, p3 : 3 points on the arc
Output arguments :
	(*x,*y) : Center of the arc
Return value :
	0 : if p1, p2, p3 are co-linear.
	1 : if they are not.

*************************************************************/

int
compute_arccenter(F_pos p1, F_pos p2, F_pos p3, float *x, float *y)
{
    double	    a, b, c, d, e, f, g, h, i, j;
    double	    resx, resy;

    if ((p1.x == p3.x && p1.y == p3.y) ||
	(p1.x == p2.x && p1.y == p2.y) ||
	(p2.x == p3.x && p2.y == p3.y)) {
	    return 0;
    }
    a = (double)p1.x - (double)p2.x;
    b = (double)p1.x + (double)p2.x;
    c = (double)p1.y - (double)p2.y;
    d = (double)p1.y + (double)p2.y;
    e = (a*b + c*d)/2.0;

    f = (double)p2.x - (double)p3.x;
    g = (double)p2.x + (double)p3.x;
    h = (double)p2.y - (double)p3.y;
    i = (double)p2.y + (double)p3.y;
    j = (f*g + h*i)/2.0;

    if (a*h - c*f != 0.0)
	resy = (a*j - e*f)/(a*h - c*f);
    else {
	return 0;
    }
    if (a != 0.0)
	resx = (e - resy*c)/a;
    else if (f != 0.0)
	resx = (j - resy*h)/f;
    else {
	return 0;
    }

    *x = (float) resx;
    *y = (float) resy;

    return 1;
}

/********************* CLOSE TO ARC ************************

Input arguments :
	a: arc object
        xp, yp: Point
        d: tolerance
Output arguments :
	(*px,*py) : `near' point on the arc
Return value :
	0 on failure, 1 on success

*************************************************************/

int
close_to_arc(F_arc *a, int xp, int yp, int d, float *px, float *py)
{
   int i, ok;
   double ux, uy, vx, vy, wx, wy, dx, dy, rarc, rp, uang, vang, wang, pang;
   double cphi, sphi;

   ux = a->point[0].x - a->center.x;
   uy = a->point[0].y - a->center.y;
   vx = a->point[1].x - a->center.x;
   vy = a->point[1].y - a->center.y;
   wx = a->point[2].x - a->center.x;
   wy = a->point[2].y - a->center.y;
   if (((ux == 0.0) && (uy == 0.0)) || ((vx == 0.0) && (vy == 0.0)) || 
       ((wx == 0.0) && (wy == 0.0)))
     return 0;
   rarc = sqrt(ux * ux + uy * uy);
   dx = xp - a->center.x;
   dy = yp - a->center.y;
   if ((dx == 0.0) && (dy == 0.0))
     return 0;
   rp = sqrt(dx * dx + dy * dy);
   if (fabs(rarc - rp) > d)
     return 0;
   /* radius ok, check special cases first */
   for (i = 0; i < 3; i++) {
     if (abs(xp - a->point[i].x) <= d && abs(yp - a->point[i].y <= d)) {
       *px = (float)(a->point[i].x);
       *py = (float)(a->point[i].y);
       return 1;
     }
   }

   /* check angles */
   uang = atan2(uy, ux);
   vang = atan2(vy, vx);
   wang = atan2(wy, wx);
   pang = atan2(dy, dx);

   if (uang >= wang) {
     if ((uang >= vang) && (vang >= wang))
       ok = (uang >= pang) && (pang >= wang);
     else {
       wang += 2*M_PI;
       if (uang >= pang)
         pang += 2*M_PI;
       ok = (pang <= wang);
     }
   }
   else {
     if ((uang <= vang) && (vang <= wang))
       ok = (uang <= pang) && (pang <= wang);     
     else {
       wang -= 2*M_PI;
       if (uang <= pang)
         pang -= 2*M_PI;
         ok = (pang >= wang);
     }
   }       
   if (!ok)
     return 0;
   /* generate point */
   cphi = cos(pang);
   sphi = sin(pang);
   dx = rarc;
   dy = 0;
   rotate_vector(&dx, &dy, cphi, sphi);             
   *px = (float)(a->center.x + dx);
   *py = (float)(a->center.y + dy);
   return 1;
}

/********************* COMPUTE ELLIPSE DISTANCE ***************

Input arguments :
       (dx, dy)   vector from center to test point
       (a, b)     main axes (`radii') of ellipse
Output arguments :
       *dis       distance from center as is
       *r         distance from center as should be
*************************************************************/
static void
compute_ellipse_distance(double dx, double dy, double a, double b, double *dis, double *r)
{
    if (dx == 0.0 && dy == 0.0)
        *dis = 0.0;        /* prevent core dumps */
    else
       *dis = sqrt(dx * dx + dy * dy);    
    if (a * dy == 0.0 && b * dx == 0.0)
        *r = 0.0;		/* prevent core dumps */
    else
        *r = a * b * (*dis) / sqrt(1.0 * b * b * dx * dx + 1.0 * a * a * dy * dy);
}

/********************* CLOSE TO ELLIPSE ************************

Input arguments :
	e: (possibly rotated!) ellipse object
        xp, yp: Point
        d: tolerance
Output arguments :
	(*ex,*ey) : `near' point on the ellipse
        (*vx,*vy) : tangent vector
Return value :
	0 on failure, 1 on success
*************************************************************/
int
close_to_ellipse(F_ellipse *e, int xp, int yp, int d, float *ex, float *ey, float *vx, float *vy)
{
   double a, b, dx, dy, phi, cphi, sphi, qx, qy, dvx, dvy;
   double dis, r;
   int wx, wy;

   dx = xp - e->center.x;
   dy = yp - e->center.y;
   a = e->radiuses.x;
   b = e->radiuses.y;
   phi = e->angle;

   if (phi == 0.0) { /* extra case for speed */
     compute_ellipse_distance(dx, dy, a, b, &dis, &r);
     if (fabs(dis - r) <= d) {
       wx = round(r * dx / dis);
       wy = round(r * dy / dis);
       *ex = (float)(e->center.x + wx);     
       *ey = (float)(e->center.y + wy);     
       if (a == 0.0) { /* degenerated */
         dvx = 0.0;
         dvy = b;
       }
       else if (b == 0.0) { /* degenerated */
         dvx = a;
         dvy = 0.0;
       }
       else {
         dvx = - ((double)wy) * a /b;
         dvy = ((double)wx) * b /a;
       }
       *vx = (float)dvx;
       *vy = (float)dvy;
       return 1;
     }
   }
   else { /* general case: rotation */
     cphi = cos(phi);
     sphi = sin(phi);
     rotate_vector(&dx, &dy, cphi, sphi);
     compute_ellipse_distance(dx, dy, a, b, &dis, &r);
     if (fabs(dis - r) <= d) {
       qx = r * dx / dis;
       qy = r * dy / dis;
       if (a == 0.0) {
         dvx = 0.0; 
         dvy = b;
       }
       else if (b == 0.0) {
         dvx = a; 
         dvy = 0.0;
       }
       else {
         dvx = -qy * a/b;
         dvy = qx * b/a;
       }
       rotate_vector(&qx, &qy, cphi, -sphi);
       rotate_vector(&dvx, &dvy, cphi, -sphi);
       *ex = (float)(e->center.x + qx);
       *ey = (float)(e->center.y + qy);
       *vx = (float)dvx;
       *vy = (float)dvy;
       return 1;
     }
   }
   return 0;
}       


/********************* CLOSE TO POLYLINE ************************

Input arguments :
	p: polyline object
        xp, yp: Point
        d: tolerance
        sd: singular tolerance
Output arguments :
	(*px,*py) : `near' point on the polyline
        (*lx1, *ly1), (*lx2, *ly2): neighbor polyline corners
Return value :
	0 on failure, 1 on success
*************************************************************/

int
close_to_polyline(F_line *l, int xp, int yp, int d, int sd, int *px, int *py, int *lx1, int *ly1, int *lx2, int *ly2)
{
   F_point *point;
   int x1, y1, x2, y2;
   float tol2;
   tol2 = (float) d*d;

   point = l->points;
   x1 = point->x;
   y1 = point->y;
   if (abs(xp - x1) <= sd && abs(yp - y1) <= sd) {
     *px = *lx1 = *lx2 = x1;
     *py = *ly1 = *ly2 = y1;
     return 1;
   }
   for (point = point->next; point != NULL; point = point->next) {
     x2 = point->x;
     y2 = point->y;
     if (abs(xp - x2) <= sd && abs(yp - y2) <= sd) {
       *px = *lx1 = *lx2 = x2;
       *py = *ly1 = *ly2 = y2;
       return 1;
     }
     if (close_to_vector(x1, y1, x2, y2, xp, yp, d, tol2, px, py)) {
       *lx1 = x1; *ly1 = y1;
       *lx2 = x2; *ly2 = y2;
       return 1;
     }
     x1 = x2;
     y1 = y2;
   }
   return 0;
}
    
/********************* CLOSE TO SPLINE ************************

Input arguments :
	p: spline object
        xp, yp: Point
        d: tolerance
Output arguments :
	(*px,*py) : `near' point on the spline
        (*lx1, *ly1), (*lx2, *ly2): points for tangent
Return value :
	0 on failure, 1 on success
*************************************************************/



/* Take a look at close_to_vector in u_geom.c ... and then tell me:
   Are those people mathematicians? */

static int
close_to_float_vector(float x1, float y1, float x2, float y2, float xp, float yp, float d, float *px, float *py, float *lambda)
{
  /* always stores in (*px,*py) the orthogonal projection of (xp,yp)
     onto the line through (x1,y1) and (x2,y2)
     returns 1 if (x,y) is closer than d to the *segment* between these two,
     otherwise 0 */
    float rx, ry, radius, radius2, nx, ny, distance;
    int ok;

    rx = x2 - x1;
    ry = y2 - y1; /* direction vector of the straight line */
    radius2 = rx * rx + ry * ry;
    if (radius2 > 0.0)
        radius = sqrt(radius2); /* its length */
    else
        return(0); /* singular case */
    nx = -ry/radius;
    ny = rx/radius; /* unit normal vector */
    distance = (xp - x1) * nx + (yp - y1) * ny; /* distance (Hesse) */
    *px = xp - distance * nx;
    *py = yp - distance * ny; /* the projection */
    if (fabs(distance) > d)
      ok = 0;
    else if ((xp-x1) * (xp-x1) + (yp-y1) * (yp-y1) <= d * d) {
      *lambda = 0.0;
      ok = 1; /* close to endpoint */
    }
    else if ((xp-x2) * (xp-x2) + (yp-y2) * (yp-y2) <= d * d) {
      *lambda = 1.0;
      ok = 1; /* close to endpoint */
    }
    else {
      *lambda = ((xp-x1)*rx + (yp-y1)*ry)/radius2; /* the parameter (0=point1, 1=point2) */
      if ((0.0 <= *lambda) && (*lambda <= 1.0))
        ok = 1;
      else
        ok = 0;
    }
    return ok;
}

static int isfirst, prevx, prevy, firstx, firsty;
static int tx, ty, td, px1, py1, px2, py2, foundx, foundy;

/* dirty trick: redefine init_point_array to do nothing and 
   add_point to check distance */

/**********************************/
/* include common spline routines */
/**********************************/

static Boolean	add_point(int x, int y);
static void	init_point_array(void);
static Boolean	add_closepoint(void);

#include "u_draw_spline.c"

static void
init_point_array(void)
{
}

static Boolean
add_point(int x, int y)
{
    float ux, uy, lambda;

    if (isfirst) {
	firstx = x;
	firsty = y;
	isfirst = False;
    } else {
	if (close_to_float_vector((float)prevx, (float)prevy, (float)x, (float)y, 
                              (float)tx, (float)ty, (float)td, &ux, &uy, &lambda)) {
	    foundx = round(ux);
	    foundy = round(uy);
	    px1 = prevx;
	    py1 = prevy;
	    px2 = x;
	    py2 = y;            
	    DONE = True;
	}
    }
    prevx = x;
    prevy = y;

    return True;
}

static Boolean
add_closepoint(void) {
    return add_point(firstx, firsty);
}

int
close_to_spline(F_spline *spline, int xp, int yp, int d, int *px, int *py, int *lx1, int *ly1, int *lx2, int *ly2)
{
    float	precision;

    tx = xp; ty = yp; /* test point */
    td = d;           /* tolerance */
    isfirst = True;
    precision = HIGH_PRECISION;

    if (open_spline(spline)) 
	compute_open_spline(spline, precision);
    else
	compute_closed_spline(spline, precision);
    if (DONE) {
	*px = foundx;
	*py = foundy;
	*lx1 = px1;
	*ly1 = py1;
	*lx2 = px2;
	*ly2 = py2;
    }
    return DONE;
}

/********************* COMPUTE ANGLE ************************

Input arguments :
	(dx,dy) : the vector (0,0)(dx,dy)
Output arguments : none
Return value : the angle of the vector in the range [0, 2PI)

*************************************************************/

double
compute_angle(double dx, double dy)		/* compute the angle between 0 to 2PI  */
          	       
{
    double	alpha;

    if (dx == 0) {
	if (dy > 0)
	    alpha = M_PI_2;
	else
	    alpha = 3 * M_PI_2;
    } else if (dy == 0) {
	if (dx > 0)
	    alpha = 0;
	else
	    alpha = M_PI;
    } else {
	alpha = atan(dy / dx);	/* range = -PI/2 to PI/2 */
	if (dx < 0)
	    alpha += M_PI;
	else if (dy < 0)
	    alpha += M_2PI;
    }
    return (alpha);
}


/********************* COMPUTE DIRECTION ********************

Input arguments :
	p1, p2, p3 : 3 points of an arc with p1 the first and p3 the last.
Output arguments : none
Return value :
	0 : if the arc passes p1, p2 and p3 (in that order) in
		clockwise direction
	1 : if direction is counterclockwise

*************************************************************/

int
compute_direction(F_pos p1, F_pos p2, F_pos p3)
{
    double	diff, dx, dy, alpha, theta;

    dx = p2.x - p1.x;
    dy = p1.y - p2.y;		/* because origin of the screen is on the
				 * upper left corner */

    alpha = compute_angle(dx, dy);

    dx = p3.x - p2.x;
    dy = p2.y - p3.y;
    theta = compute_angle(dx, dy);

    diff = theta - alpha;
    if ((0 < diff && diff < M_PI) || diff < -M_PI) {
	return (1);		/* counterclockwise */
    }
    return (0);			/* clockwise */
}

/********************* COMPUTE ANGLE BY 3 POINTS ************
Input arguments:
        p1, p2, p3 : Points
Output arguments : angle in the range [0, 2PI)
Return value : 0 on failure, 1 on success       
*************************************************************/

int 
compute_3p_angle(F_point *p1, F_point *p2, F_point *p3, double *alpha)
{
    double	 vx, vy, wx, wy;
    double	 dang;

    vx = (double)(p1->x - p2->x);
    vy = (double)(p1->y - p2->y);
    wx = (double)(p3->x - p2->x);
    wy = (double)(p3->y - p2->y);
    if (((vx == 0.0) && (vy == 0.0)) || ((wx == 0.0) && (wy == 0.0))) {
       return(0);
    }
    dang = -atan2(wy, wx) + atan2(vy, vx); /* difference angle */
    if (dang < 0.0)
        dang += 2*M_PI;      
    *alpha = dang;
    return(1);
}    


/********************* COMPUTE LINE ANGLE *******************

Input arguments :
        l : the line object
        p : point
Output arguments : angle in the range [0, 2PI)
Return value : 0 on failure, 1 on success
*************************************************************/

int 
compute_line_angle(F_line *l, F_point *p, double *alpha)
{
   F_point	*q, *vorq, *nachq;

   q = l->points;
   vorq = NULL;
   nachq = l->points->next;

   for (; q != NULL; vorq = q, q = nachq, nachq = (nachq) ? nachq->next : NULL) {
     if ((q->x == p->x) && (q->y == p->y) && (vorq)) {
       if (nachq) {
         if (compute_3p_angle(vorq, q, nachq, alpha))
           return(1);
       }
       else if (l->type == T_POLYGON) {
         if (compute_3p_angle(vorq, q, l->points->next, alpha))
           return(1);
       }
     }
   }
   return(0);
}	

/********************* COMPUTE ARC ANGLE *******************

Input arguments :
        a : the arc
Output arguments : orientated angle
Return value : 0 on failure, 1 on success
*************************************************************/

int 
compute_arc_angle(F_arc *a, double *alpha)
{
   double	 ux, uy, vx, vy, wx, wy;
   double	 uang, vang, wang, dang;

   ux = a->point[0].x - a->center.x;
   uy = a->point[0].y - a->center.y;
   vx = a->point[1].x - a->center.x;
   vy = a->point[1].y - a->center.y;
   wx = a->point[2].x - a->center.x;
   wy = a->point[2].y - a->center.y;
   if (((ux == 0.0) && (uy == 0.0)) || ((vx == 0.0) && (vy == 0.0)) || 
       ((wx == 0.0) && (wy == 0.0))) {
       return(0);
   }
   uang = atan2(uy, ux);
   vang = atan2(vy, vx);
   wang = atan2(wy, wx);
 
   if (uang >= wang) {
      if ((uang >= vang) && (vang >= wang))
          dang = uang - wang;
      else
          dang = -2*M_PI + uang - wang;
   }
   else {
      if ((uang <= vang) && (vang <= wang))
          dang = uang - wang;
      else
          dang = 2*M_PI + uang - wang;
   }
   *alpha = dang;
   return(1);
}   

/********************* COMPUTE POLY LENGTH *******************

Input arguments :
        l : the polyline
Output arguments : 
        *lp : computed length
Return value : 0 on failure, 1 on success
*************************************************************/
int
compute_poly_length(F_line *l, float *lp)
{
   double	 length;
   F_point	*q, *prevq;
   double	 dx, dy, dl;

   length = 0.0;
   prevq = NULL;

   for (q = l->points; q != NULL; prevq = q, q = q->next) {
     if (prevq) {
       dx = (double)(q->x - prevq->x);
       dy = (double)(q->y - prevq->y);
       if ((dx == 0.0) && (dy == 0.0))
         dl = 0.0;
       else
         dl = sqrt(dx * dx + dy * dy);
       length += dl;
     }
   }
   *lp = (float)length;
   return 1;
}    

/********************* COMPUTE ARC LENGTH *******************

Input arguments :
        a : the arc
Output arguments : 
        *lp : computed length
Return value : 0 on failure, 1 on success
*************************************************************/
int
compute_arc_length(F_arc *a, float *lp)
{
   double	 alpha, ux, uy, arcradius;

   if (!compute_arc_angle(a, &alpha))
     return 0;
   ux = a->point[0].x - a->center.x;
   uy = a->point[0].y - a->center.y;
   if ((ux == 0.0) && (uy == 0.0)) 
     arcradius = 0.0;
   else
     arcradius = sqrt(ux*ux + uy*uy);
   *lp = (float)(arcradius * fabs(alpha));
   return 1;
}

/********************* COMPUTE POLYGON AREA *******************

Input arguments :
        l : the polygon
Output arguments : 
        *lp : computed area
Return value : 0 on failure, 1 on success
*************************************************************/

void
compute_poly_area(F_line *l, float *ap)
{
   float	 area;
   F_point	*q, *prevq;

   area = 0.0;
   prevq = NULL;
   for (q = l->points; q != NULL; prevq = q, q = q->next) {
     if (prevq) 
       area += (float)-prevq->x * (float)q->y + (float)prevq->y * (float)q->x;
   }
   area /= 2.0;
   *ap = area;
}

/********************* COMPUTE ARC AREA *******************

Input arguments :
        a : the arc
Output arguments : 
        *lp : computed area
Return value : 0 on failure, 1 on success
*************************************************************/

int
compute_arc_area(F_arc *a, float *ap)
{
   double	 area, tri_area, alpha, ux, uy, arcradius;
   
   if (!compute_arc_angle(a, &alpha))
	return 0;
   ux = a->point[0].x - a->center.x;
   uy = a->point[0].y - a->center.y;
   if ((ux == 0.0) && (uy == 0.0)) 
	arcradius = 0.0;
   else
	arcradius = sqrt(ux*ux + uy*uy);   
   /* sector (`pie-shape') 
      A = (pi * r * r) * (alpha / 2pi) */
   area = 0.5 * alpha * arcradius * arcradius;
   /* open arc: subtract triangular area */
   if (a->type == T_OPEN_ARC) {
	tri_area = 0.5 * arcradius * arcradius * fabs(sin(alpha));
	if (area > 0)
	    area -= tri_area;
	else
	    area += tri_area;
   }
   *ap = (float)area;
   return 1;
}

/********************* COMPUTE ELLIPSE AREA *******************

Input arguments :
        e : the ellipse
Output arguments : 
        *lp : computed area
Return value : 0 on failure, 1 on success
*************************************************************/

int
compute_ellipse_area(F_ellipse *e, float *ap)
{
    *ap = (float)(M_PI * e->radiuses.x * e->radiuses.y);
    return 1;
}


/*********************** LATEX LINE ROUTINES ***************************/

/*
 * compute greatest common divisor, assuming 0 < a <= b
 */

int
pgcd(int a, int b)
{
    b = b % a;
    return (b) ? gcd(b, a) : a;
}

/*
 * compute greatest common divisor
 */

int
gcd(int a, int b)
{
    if (a < 0)
	a = -a;
    if (b < 0)
	b = -b;
    return (a <= b) ? pgcd(a, b) : pgcd(b, a);
}

/*
 * compute least common multiple
 */

int
lcm(int a, int b)
{
    return abs(a * b) / gcd(a, b);
}


double		rad2deg = 57.295779513082320877;

struct angle_table {
    int		    x, y;
    double	    angle;
};

struct angle_table line_angles[25] =
{{0, 1, 90.0},
{1, 0, 0.0},
{1, 1, 45.0},
{1, 2, 63.434948822922010648},
{1, 3, 71.565051177077989351},
{1, 4, 75.963756532073521417},
{1, 5, 78.690067525979786913},
{1, 6, 80.537677791974382609},
{2, 1, 26.565051177077989351},
{2, 3, 56.309932474020213086},
{2, 5, 68.198590513648188229},
{3, 1, 18.434948822922010648},
{3, 2, 33.690067525979786913},
{3, 4, 53.130102354155978703},
{3, 5, 59.036243467926478582},
{4, 1, 14.036243467926478588},
{4, 3, 36.869897645844021297},
{4, 5, 51.340191745909909396},
{5, 1, 11.309932474020213086},
{5, 2, 21.801409486351811770},
{5, 3, 30.963756532073521417},
{5, 4, 38.659808254090090604},
{5, 6, 50.194428907734805993},
{6, 1, 9.4623222080256173906},
{6, 5, 39.805571092265194006}
};

struct angle_table arrow_angles[13] =
{{0, 1, 90.0},
{1, 0, 0.0},
{1, 1, 45.0},
{1, 2, 63.434948822922010648},
{1, 3, 71.565051177077989351},
{1, 4, 75.963756532073521417},
{2, 1, 26.565051177077989351},
{2, 3, 56.309932474020213086},
{3, 1, 18.434948822922010648},
{3, 2, 33.690067525979786913},
{3, 4, 53.130102354155978703},
{4, 1, 14.036243467926478588},
{4, 3, 36.869897645844021297},
};

void get_slope(int dx, int dy, int *sxp, int *syp, int arrow)
{
    double	    angle;
    int		    i, s, max;
    double	    d, d1;
    struct angle_table *st;

    if (dx == 0) {
	*sxp = 0;
	*syp = signof(dy);
	return;
    }
    angle = atan((double) abs(dy) / (double) abs(dx)) * rad2deg;
    if (arrow) {
	st = arrow_angles;
	max = 13;
    } else {
	st = line_angles;
	max = 25;
    }
    s = 0;
    d = 9.9e9;
    for (i = 0; i < max; i++) {
	d1 = fabs(angle - st[i].angle);
	if (d1 < d) {
	    s = i;
	    d = d1;
	}
    }
    *sxp = st[s].x;
    if (dx < 0)
	*sxp = -*sxp;
    *syp = st[s].y;
    if (dy < 0)
	*syp = -*syp;
}

void latex_endpoint(int x1, int y1, int x2, int y2, int *xout, int *yout, int arrow, int magnet)
{
    int		    dx, dy, sx, sy, ds, dsx, dsy;

    dx = x2 - x1;
    dy = y2 - y1;
    get_slope(dx, dy, &sx, &sy, arrow);
    if (abs(sx) >= abs(sy)) {
	ds = lcm(sx, magnet * gcd(sx, magnet));
	dsx = (2 * abs(dx) / ds + 1) / 2;
	dsx = (dx >= 0) ? dsx * ds : -dsx * ds;
	*xout = x1 + dsx;
	*yout = y1 + dsx * sy / sx;
    } else {
	ds = lcm(sy, magnet * gcd(sy, magnet));
	dsy = (2 * abs(dy) / ds + 1) / 2;
	dsy = (dy >= 0) ? dsy * ds : -dsy * ds;
	*yout = y1 + dsy;
	*xout = x1 + dsy * sx / sy;
    }
}
