/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1992 by James Tough
 * Parts Copyright (c) 1998 by Georg Stemmer
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
#include "d_text.h"
#include "f_util.h"
#include "u_bound.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_fonts.h"
#include "u_geom.h"
#include "u_error.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"
#include "u_redraw.h"
#include "w_cursor.h"

static Boolean add_point(int x, int y);
static void init_point_array(void);
static Boolean add_closepoint(void);

#include "u_draw_spline.c"


/* the spline definition stuff has been moved to u_draw_spline.c which
   is included later in this file */

/************** ARRAY FOR ARROW SHAPES **************/ 

struct _fpnt { 
		double x,y;
	};

struct _arrow_shape {
		int	numpts;		/* number of points in arrowhead */
		int	tipno;		/* which point contains the tip */
		int	numfillpts;	/* number of points to fill */
		Boolean	simplefill;	/* if true, use points array to fill otherwise use fill_points array */
		Boolean	clip;		/* if false, no clip area needed (e.g. for reverse triangle arrowhead) */
		Boolean	half;		/* if true, arrowhead is half-wide and must be shifted to cover the line */
		double	tipmv;		/* acuteness of tip (smaller angle, larger tipmv) */
		struct	_fpnt points[6]; /* points in arrowhead */
		struct	_fpnt fillpoints[6]; /* points to fill if not "simple" */
	};

static struct _arrow_shape arrow_shapes[NUM_ARROW_TYPES] = {
		   /* number of points, index of tip, {datapairs} */
		   /* first point must be upper-left point of tail, then tip */

		   /* type 0 */
		   { 3, 1, 0, True, True, False, 2.15, {{-1,0.5}, {0,0}, {-1,-0.5}}},
		   /* place holder for what would be type 0 filled */
		   { 0 },
		   /* type 1a simple triangle */
		   { 4, 1, 0, True, True, False, 2.1, {{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}},
		   /* type 1b filled simple triangle*/
		   { 4, 1, 0, True, True, False, 2.1, {{-1.0,0.5}, {0,0}, {-1.0,-0.5}, {-1.0,0.5}}},
		   /* type 2a concave spearhead */
		   { 5, 1, 0, True, True, False, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}},
		   /* type 2b filled concave spearhead */
		   { 5, 1, 0, True, True, False, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}}},
		   /* type 3a convex spearhead */
		   { 5, 1, 0, True, True, False, 1.5, {{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}},
		   /* type 3b filled convex spearhead */
		   { 5, 1, 0, True, True, False, 1.5, {{-0.75,0.5},{0,0},{-0.75,-0.5},{-1.0,0},{-0.75,0.5}}},
#ifdef NEWARROWTYPES
		   /* type 4a diamond */
		   { 5, 1, 0, True, True, False, 1.15, {{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}},
		   /* type 4b filled diamond */
		   { 5, 1, 0, True, True, False, 1.15, {{-0.5,0.5},{0,0},{-0.5,-0.5},{-1.0,0},{-0.5,0.5}}},
		   /* type 5a/b circle - handled in code */
		   { 0, 0, 0, True, True, False, 0.0 },
		   { 0, 0, 0, True, True, False, 0.0 },
		   /* type 6a/b half circle - handled in code */
		   { 0, 0, 0, True, True, False, -1.0 },
		   { 0, 0, 0, True, True, False, -1.0 },
		   /* type 7a square */
		   { 5, 1, 0, True, True, False, 0.0, {{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}},
		   /* type 7b filled square */
		   { 5, 1, 0, True, True, False, 0.0, {{-1.0,0.5},{0,0.5},{0,-0.5},{-1.0,-0.5},{-1.0,0.5}}},
		   /* type 8a reverse triangle */
		   { 4, 1, 0, True, False, False, 0.0, {{-1.0,0},{0,0.5},{0,-0.5},{-1.0,0}}},
		   /* type 8b filled reverse triangle */
		   { 4, 1, 0, True, False, False, 0.0, {{-1.0,0},{0,0.5},{0,-0.5},{-1.0,0}}},

		   /* type 9a top-half filled concave spearhead */
		   { 5, 1, 3, False, True, False, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}},
			   			 {{-1.25,-0.5},{0,0},{-1,0}}},
		   /* type 9b bottom-half filled concave spearhead */
		   { 5, 1, 3, False, True, False, 2.6, {{-1.25,0.5},{0,0},{-1.25,-0.5},{-1.0,0},{-1.25,0.5}},
			   			 {{-1.25,0.5},{0,0},{-1,0}}},

		   /* type 10o top-half simple triangle */
		   { 4, 1, 0, True, True, True, 2.5, {{-1.0,0.5}, {0,0}, {-1,0.0}, {-1.0,0.5}}},
		   /* type 10f top-half filled simple triangle*/
		   { 4, 1, 0, True, True, True, 2.5, {{-1.0,0.5}, {0,0}, {-1,0.0}, {-1.0,0.5}}},
		   /* type 11o top-half concave spearhead */
		   { 4, 1, 0, True, True, True, 3.5, {{-1.25,0.5}, {0,0}, {-1,0}, {-1.25,0.5}}},
		   /* type 11f top-half filled concave spearhead */
		   { 4, 1, 0, True, True, True, 3.5, {{-1.25,0.5}, {0,0}, {-1,0}, {-1.25,0.5}}},
		   /* type 12o top-half convex spearhead */
		   { 4, 1, 0, True, True, True, 2.5, {{-0.75,0.5}, {0,0}, {-1,0}, {-0.75,0.5}}},
		   /* type 12f top-half filled convex spearhead */
		   { 4, 1, 0, True, True, True, 2.5, {{-0.75,0.5}, {0,0}, {-1,0}, {-0.75,0.5}}},

		   /* type 13a "wye" */
		   { 3, 0, 0, True, True, False, -1.0, {{0,0.5},{-1.0,0},{0,-0.5}}},
		   /* type 13b bar */
		   { 2, 1, 0, True, True, False, 0.0, {{0,0.5},{0,-0.5}}},
		   /* type 14a two-prong fork */
		   { 4, 0, 0, True, True, False, -1.0, {{0,0.5},{-1.0,0.5},{-1.0,-0.5},{0,-0.5}}},
		   /* type 14b backward two-prong fork */
		   { 4, 1, 0, True, True, False, 0.0, {{-1.0,0.5,},{0,0.5},{0,-0.5},{-1.0,-0.5}}},
#endif /* NEWARROWTYPES */
		};

/************** POLYGON/CURVE DRAWING FACILITIES ****************/

static int	npoints;
static zXPoint *points = NULL;
static int	max_points = 0;
static int	allocstep = 200;
static char     bufx[10];	/* for appres.shownums */

/* these are for the arrowheads */
static zXPoint	    farpts[50],barpts[50];
static zXPoint	    farfillpts[50], barfillpts[50];
static int	    nfpts, nbpts, nffillpts, nbfillpts;

/************* Code begins here *************/


void clip_arrows (F_line *obj, int objtype, int op, int skip);
void draw_arrow (F_line *obj, F_arrow *arrow, zXPoint *points, int npoints, zXPoint *points2, int npoints2, int op);
void debug_depth (int depth, int x, int y);
void newpoint (float xp, float yp);
void draw_arcbox (F_line *line, int op);
void draw_pic_pixmap (F_line *box, int op);
void create_pic_pixmap (F_line *box, int rotation, int width, int height, int flipped);
void clr_mask_bit (int r, int c, int bwidth, unsigned char *mask);
void greek_text (F_text *text, int x1, int y1, int x2, int y2);

static void
init_point_array(void)
{
  npoints = 0;
}

static Boolean
add_point(int x, int y)
{
	if (npoints >= max_points) {
	    int tmp_n;
	    zXPoint *tmp_p;
	    tmp_n = max_points + allocstep;
	    /* too many points, return false */
	    if (tmp_n > MAXNUMPTS) {
		if (appres.DEBUG)
		    fprintf(stderr,"add_point - reached MAXNUMPTS (%d)\n",tmp_n);
	    	return False;
	    }
	    if (max_points == 0) {
		tmp_p = (zXPoint *) malloc(tmp_n * sizeof(zXPoint));
		if (appres.DEBUG)
		    fprintf(stderr,"add_point - alloc %d points\n",tmp_n);
	    } else {
		tmp_p = (zXPoint *) realloc(points, tmp_n * sizeof(zXPoint));
		if (appres.DEBUG)
		    fprintf(stderr,"add_point - realloc %d points\n",tmp_n);
	    }
	    if (tmp_p == NULL) {
		fprintf(stderr,
		      "xfig: insufficient memory to allocate point array\n");
		return False;
	    }
	    points = tmp_p;
	    max_points = tmp_n;
	}
	/* ignore identical points */
	if (npoints > 0 && points[npoints-1].x == x && points[npoints-1].y == 
y)
		    return True;
	points[npoints].x = x;
	points[npoints].y = y;
	npoints++;
	return True;
}

void draw_point_array(Window w, int op, int depth, int line_width, int line_style, float style_val, int join_style, int cap_style, int fill_style, int pen_color, int fill_color)
{
	pw_lines(w, points, npoints, op, depth, line_width, line_style, style_val,
		    join_style, cap_style, fill_style, pen_color, fill_color);
}

/*********************** ARC ***************************/

void draw_arc(F_arc *a, int op)
{
    double	    rx, ry, rcx, rcy;
    int		    cx, cy, scx, scy;
    int		    radius;
    int		    xmin, ymin, xmax, ymax;
    int		    i;

    arc_bound(a, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    rx = a->point[0].x - a->center.x;
    ry = a->center.y - a->point[0].y;
    radius = round(sqrt(rx * rx + ry * ry));

    /* need both int and double center points */
    cx = rcx = a->center.x;
    cy = rcy = a->center.y;

    /* show point numbers if requested */
    if (appres.shownums && active_layer(a->depth)) {
	/* we may have to enlarge the clipping area to include the center point of the arc */
	scx = ZOOMX(cx);
	scy = ZOOMY(cy);
	if (scx < clip_xmin+10 || scx > clip_xmax-10 ||
	    scy < clip_ymin+10 || scy > clip_ymax-10)
		set_clip_window(min2(scx-10,clip_xmin), min2(scy-10,clip_ymin), 
				max2(scx+10,clip_xmax), max2(scy+10,clip_ymax));
	sprintf(bufx,"c");
	pw_text(canvas_win, cx, round(cy-3.0/zoomscale), 
		op, a->depth, roman_font, 0.0, bufx, RED, COLOR_NONE);
	pw_point(canvas_win, cx, cy, op, a->depth, 4, RED, CAP_ROUND);
	for (i=1; i<=3; i++) {
	    /* label the point number above the point */
	    sprintf(bufx,"%d",i);
	    pw_text(canvas_win, a->point[i-1].x, round(a->point[i-1].y-3.0/zoomscale), 
		op, a->depth, roman_font, 0.0, bufx, RED, COLOR_NONE);
	    pw_point(canvas_win, a->point[i-1].x, a->point[i-1].y, op, a->depth, 4, RED, CAP_ROUND);
	}
	/* restore original clip window */
	set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);
    }
    /* fill points array but don't display the points yet */

    curve(canvas_win, a->depth,
    	  round(a->point[0].x - rcx),
	  round(rcy - a->point[0].y),
	  round(a->point[2].x - rcx),
	  round(rcy - a->point[2].y),
	  DONT_DRAW_POINTS, (a->type == T_PIE_WEDGE_ARC? DRAW_CENTER: DONT_DRAW_CENTER),
	  a->direction, radius, radius,
	  round(rcx), round(rcy), op,
	  a->thickness, a->style, a->style_val, a->fill_style,
	  a->pen_color, a->fill_color, a->cap_style);

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(a,O_ARC,op,0);

    /* draw the arc itself */
    draw_point_array(canvas_win, op, a->depth, a->thickness,
		     a->style, a->style_val, JOIN_BEVEL,
		     a->cap_style, a->fill_style,
		     a->pen_color, a->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    /* draw the arrowheads, if any */
    if (a->type != T_PIE_WEDGE_ARC) {
      if (a->for_arrow) {
	    draw_arrow(a, a->for_arrow, farpts, nfpts, farfillpts, nffillpts, op);
      }
      if (a->back_arrow) {
	    draw_arrow(a, a->back_arrow, barpts, nbpts, barfillpts, nbfillpts, op);
      }
    }
    /* write the depth on the object */
    debug_depth(a->depth,a->point[0].x,a->point[0].y);
}

/*********************** ELLIPSE ***************************/

void draw_ellipse(F_ellipse *e, int op)
{
    int		    a, b, xmin, ymin, xmax, ymax;

    ellipse_bound(e, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    if (e->angle != 0.0) {
	angle_ellipse(e->center.x, e->center.y, e->radiuses.x, e->radiuses.y,
		e->angle, op, e->depth, e->thickness, e->style,
		e->style_val, e->fill_style, e->pen_color, e->fill_color);
    /* it is much faster to use curve() for dashed and dotted lines that to
       use the server's sloooow algorithms for that */
    } else if (op != ERASE && (e->style == DOTTED_LINE || e->style == DASH_LINE)) {
	a = e->radiuses.x;
	b = e->radiuses.y;
	curve(canvas_win, e->depth, a, 0, a, 0, DRAW_POINTS, DONT_DRAW_CENTER, e->direction,
		(b * b), (a * a),
		e->center.x, e->center.y, op,
		e->thickness, e->style, e->style_val, e->fill_style,
		e->pen_color, e->fill_color, CAP_ROUND);
    /* however, for solid lines the server is muuuch faster even for thick lines */
    } else {
	xmin = e->center.x - e->radiuses.x;
	ymin = e->center.y - e->radiuses.y;
	xmax = e->center.x + e->radiuses.x;
	ymax = e->center.y + e->radiuses.y;
	pw_curve(canvas_win, xmin, ymin, xmax, ymax, op, e->depth,
		 e->thickness, e->style, e->style_val, e->fill_style,
		 e->pen_color, e->fill_color, CAP_ROUND);
    }
    /* write the depth on the object */
    debug_depth(e->depth,e->center.x,e->center.y);
}

static Boolean
add_closepoint(void)
{
  return add_point(points[0].x,points[0].y);
}

/*
 *  An Ellipse Generator.
 *  Written by James Tough   7th May 92
 *
 *  The following routines displays a filled ellipse on the screen from the
 *    semi-minor axis 'a', semi-major axis 'b' and angle of rotation
 *    'phi'.
 *
 *  It works along these principles .....
 *
 *        The standard ellipse equation is
 *
 *             x*x     y*y
 *             ---  +  ---
 *             a*a     b*b
 *
 *
 *        Rotation of a point (x,y) is well known through the use of
 *
 *            x' = x*COS(phi) - y*SIN(phi)
 *            y' = y*COS(phi) + y*COS(phi)
 *
 *        Taking these to together, this gives the equation for a rotated
 *      ellipse centered around the origin.
 *
 *           [x*COS(phi) - y*SIN(phi)]^2   [x*SIN(phi) + y*COS(phi)]^2
 *           --------------------------- + ---------------------------
 *                      a*a                           b*b
 *
 *        NOTE -  some of the above equation can be precomputed, eg,
 *
 *              i = COS(phi)/a        and        j = SIN(phi)/b
 *
 *        NOTE -  y is constant for each line so,
 *
 *              m = -yk*SIN(phi)/a    and     n = yk*COS(phi)/b
 *              where yk stands for the kth line ( y subscript k)
 *
 *        Where yk=y, we get a quadratic,
 *
 *              (i*x + m)^2 + (j*x + n)^2 = 1
 *
 *        Thus for any particular line, y, there is two corresponding x
 *      values. These are the roots of the quadratic. To get the roots,
 *      the above equation can be rearranged using the standard method,
 *
 *          -(i*m + j*n) +- sqrt[i^2 + j^2 - (i*n -j*m)^2]
 *      x = ----------------------------------------------
 *                           i^2 + j^2
 *
 *        NOTE -  again much of this equation can be precomputed.
 *
 *           c1 = i^2 + j^2
 *           c2 = [COS(phi)*SIN(phi)*(a-b)]
 *           c3 = [b*b*(COS(phi)^2) + a*a*(SIN(phi)^2)]
 *           c4 = a*b/c3
 *
 *      x = c2*y +- c4*sqrt(c3 - y*y),    where +- must be evaluated once
 *                                      for plus, and once for minus.
 *
 *        We also need to know how large the ellipse is. This condition
 *      arises when the sqrt of the above equation evaluates to zero.
 *      Thus the height of the ellipse is give by
 *
 *              sqrt[ b*b*(COS(phi)^2) + a*a*(SIN(phi)^2) ]
 *
 *       which just happens to be equal to sqrt(c3).
 *
 *         It is now possible to create a routine that will scan convert
 *       the ellipse on the screen.
 *
 *        NOTE -  c2 is the gradient of the new ellipse axis.
 *                c4 is the new semi-minor axis, 'a'.
 *           sqr(c3) is the new semi-major axis, 'b'.
 *
 *         These values could be used in a 4WS or 8WS ellipse generator
 *       that does not work on rotation, to give the feel of a rotated
 *       ellipse. These ellipses are not very accurate and give visable
 *       bumps along the edge of the ellipse. However, these routines
 *       are very quick, and give a good approximation to a rotated ellipse.
 *
 *       NOTES on the code given.
 *
 *           All the routines take there parameters as ( x, y, a, b, phi ),
 *           where x,y are the center of the ellipse ( relative to the
 *           origin ), a and b are the vertical and horizontal axis, and
 *           phi is the angle of rotation in RADIANS.
 *
 *           The 'moveto(x,y)' command moves the screen cursor to the
 *               (x,y) point.
 *           The 'lineto(x,y)' command draws a line from the cursor to
 *               the point (x,y).
 *
 */


/*
 *  QuickEllipse, uses the same method as Ellipse, but uses incremental
 *    methods to reduce the amount of work that has to be done inside
 *    the main loop. The speed increase is very noticeable.
 *
 *  Written by James Tough
 *  7th May 1992
 *
 */

static int	x[MAXNUMPTS/4][4],y[MAXNUMPTS/4][4];
static int	nump[4];
static int	totpts,i,j;
static int	order[4]={0,1,3,2};

void angle_ellipse(int center_x, int center_y, int radius_x, int radius_y, float angle, int op, int depth, int thickness, int style, float style_val, int fill_style, int pen_color, int fill_color)
{
	float	xcen, ycen, a, b;

	double	c1, c2, c3, c4, c5, c6, v1, cphi, sphi, cphisqr, sphisqr;
	double	xleft, xright, d, asqr, bsqr;
	int	ymax, yy=0;
	int	k,m,dir;
	float	savezoom;
	int	savexoff, saveyoff;

	if (radius_x == 0 || radius_y == 0)
		return;

	/* adjust for zoomscale so we iterate over zoomed pixels */
	xcen = ZOOMX(center_x);
	ycen = ZOOMY(center_y);
	a = radius_x*zoomscale;
	b = radius_y*zoomscale;
	savezoom = zoomscale;
	savexoff = zoomxoff;
	saveyoff = zoomyoff;
	zoomscale = 1.0;
	zoomxoff = zoomyoff = 0;

	cphi = cos((double)angle);
	sphi = sin((double)angle);
	cphisqr = cphi*cphi;
	sphisqr = sphi*sphi;
	asqr = a*a;
	bsqr = b*b;
	
	c1 = (cphisqr/asqr)+(sphisqr/bsqr);
	c2 = ((cphi*sphi/asqr)-(cphi*sphi/bsqr))/c1;
	c3 = (bsqr*cphisqr) + (asqr*sphisqr);
	ymax = sqrt(c3);
	c4 = a*b/c3;
	c5 = 0;
	v1 = c4*c4;
	c6 = 2*v1;
	c3 = c3*v1-v1;
	totpts = 0;
	for (i=0; i<=3; i++)
		nump[i]=0;
	i=0; j=0;
	/* odd first points */
	if (ymax % 2) {
		d = sqrt(c3);
		newpoint(xcen-d,ycen);
		newpoint(xcen+d,ycen);
		c5 = c2;
		yy=1;
	}
	while (c3>=0) {
		d = sqrt(c3);
		xleft = c5-d;
		xright = c5+d;
		newpoint(xcen+xleft,ycen+yy);
		newpoint(xcen+xright,ycen+yy);
		newpoint(xcen-xright,ycen-yy);
		newpoint(xcen-xleft,ycen-yy);
		c5+=c2;
		v1+=c6;
		c3-=v1;
		yy=yy+1;
	}
	dir=0;
	totpts++;	/* add another point to join with first */
	init_point_array();
	/* now go down the 1st column, up the 2nd, down the 4th
	   and up the 3rd to get the points in the correct order */
	for (k=0; k<=3; k++) {
	    if (dir==0)
		for (m=0; m<nump[k]; m++) {
		    if (!add_point(x[m][order[k]],y[m][order[k]]))
			break;
		}
	    else
		for (m=nump[k]-1; m>=0; m--) {
		    if (!add_point(x[m][order[k]],y[m][order[k]]))
			break;
		}
	    dir = 1-dir;
	} /* next k */
	/* add another point to join with first */
	if (!add_point(points[0].x,points[0].y))
		too_many_points();
	draw_point_array(canvas_win, op, depth, thickness, style, style_val,
		 JOIN_BEVEL, CAP_ROUND, fill_style, pen_color, fill_color);

	zoomscale = savezoom;
	zoomxoff = savexoff;
	zoomyoff = saveyoff;
	return;
}

/* store the points across (row-wise in) the matrix */

void newpoint(float xp, float yp)
{
    if (totpts >= MAXNUMPTS/4) {
	if (totpts == MAXNUMPTS/4) {
	    put_msg("Too many points to fully display rotated ellipse. %d points max",
		MAXNUMPTS);
	    totpts++;
	}
	return;
    }
    x[i][j]=round(xp);
    y[i][j]=round(yp);
    nump[j]++;
    totpts++;
    if (++j > 3) {
	j=0;
	i++;
    }
}


/*********************** LINE ***************************/

void draw_line(F_line *line, int op)
{
    F_point	   *point;
    int		    i, x, y;
    int		    xmin, ymin, xmax, ymax;
    char	   *string;
    F_point	   *p0, *p1, *p2;
    PR_SIZE	    txt;

    line_bound(line, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    /* is it an arcbox? */
    if (line->type == T_ARCBOX) {
	draw_arcbox(line, op);
	return;
    }
    /* is it a picture object or a Fig figure? */
    if (line->type == T_PICTURE) {
	if (line->pic->pic_cache) {
	    if ((line->pic->pic_cache->bitmap != (Pixmap) NULL) && active_layer(line->depth)) {
		/* only draw the picture if there is a pixmap AND this layer is active */
		draw_pic_pixmap(line, op);
		return;
	    } else if (line->pic->pic_cache->bitmap != NULL) { 
		/* if there is a pixmap but the layer is not active, draw it as a filled box */
		line->type = T_BOX;
		line->fill_style = NUMSHADEPATS-1;	 /* fill it */
		draw_line(line, op);
		line->type = T_PICTURE;
		return;
	    }
	}

	/* either there is no pixmap or this layer is grayed out,
	   label empty pic bounding box */
	if (!line->pic->pic_cache || line->pic->pic_cache->file[0] == '\0')
	    string = EMPTY_PIC;
	else {
	    string = xf_basename(line->pic->pic_cache->file);
	}
	p0 = line->points;
	p1 = p0->next;
	p2 = p1->next;
	xmin = min3(p0->x, p1->x, p2->x);
	ymin = min3(p0->y, p1->y, p2->y);
	xmax = max3(p0->x, p1->x, p2->x);
	ymax = max3(p0->y, p1->y, p2->y);
	canvas_font = lookfont(0, 12);	/* get a size 12 font */
	txt = textsize(canvas_font, strlen(string), string);
	/* if the box is large enough, put the filename in the four corners */
	if (xmax - xmin > 2.5*txt.length) {
	    int u,d,w,marg;
	    u = txt.ascent;
	    d = txt.descent;
	    w = txt.length;
	    marg = 6 * ZOOM_FACTOR;	/* margin space around text */

	    pw_text(canvas_win, xmin+marg, ymin+u+marg, op, line->depth,
			canvas_font, 0.0, string, DEFAULT, GREEN);
	    pw_text(canvas_win, xmax-txt.length-marg, ymin+u+marg, op, line->depth,
			canvas_font, 0.0, string, DEFAULT, GREEN);
	    /* do bottom two corners if tall enough */
	    if (ymax - ymin > 3*(u+d)) {
		pw_text(canvas_win, xmin+marg, ymax-d-marg, op, line->depth,
			canvas_font, 0.0, string, DEFAULT, GREEN);
		pw_text(canvas_win, xmax-txt.length-marg, ymax-d-marg, op, line->depth,
			canvas_font, 0.0, string, DEFAULT, GREEN);
	    }
	} else {
	    /* only room for one label - center it */
	    x = (xmin + xmax) / 2 - txt.length/display_zoomscale / 2;
	    y = (ymin + ymax) / 2;
	    pw_text(canvas_win, x, y, op, line->depth, canvas_font, 0.0, string, DEFAULT, GREEN);
	}
    }
    /* get first point and coordinates */
    point = line->points;
    x = point->x;
    y = point->y;

    /* is it a single point? */
    if (line->points->next == NULL) {
	/* draw but don't fill */
	pw_point(canvas_win, x, y, op, line->depth,
			line->thickness, line->pen_color, line->cap_style);
	/* label the point number above the point */
	if (appres.shownums && active_layer(line->depth)) {
	    pw_text(canvas_win, x, round(y-3.0/zoomscale), PAINT, line->depth, 
			roman_font, 0.0, "0", RED, COLOR_NONE);
	}
	return;
    }

    /* accumulate the points in an array - start with 50 */
    init_point_array();

    i=0;
    for (point = line->points; point != NULL; point = point->next) {
	x = point->x;
	y = point->y;
	/* label the point number above the point */
	if (appres.shownums && active_layer(line->depth)) {
	    /* if BOX or POLYGON, don't label last point (which is same as first) */
	    if (((line->type == T_BOX || line->type == T_POLYGON) && point->next != NULL) ||
		(line->type != T_BOX && line->type != T_POLYGON)) {
		sprintf(bufx,"%d",i++);
		pw_text(canvas_win, x, round(y-3.0/zoomscale), PAINT, line->depth,
			roman_font, 0.0, bufx, RED, COLOR_NONE);
	    }
	}
	if (!add_point(x, y)) {
	    too_many_points();
	    break;
	}
    }

    /* setup clipping so that spline doesn't protrude beyond arrowhead */
    /* also create the arrowheads */
    clip_arrows(line,O_POLYLINE,op,0);

    draw_point_array(canvas_win, op, line->depth, line->thickness, 
		     line->style, line->style_val, line->join_style,
		     line->cap_style, line->fill_style,
		     line->pen_color, line->fill_color);

    /* restore clipping */
    set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

    /* draw the arrowheads, if any */
    if (line->for_arrow)
	draw_arrow(line, line->for_arrow, farpts, nfpts, farfillpts, nffillpts, op);
    if (line->back_arrow)
	draw_arrow(line, line->back_arrow, barpts, nbpts, barfillpts, nbfillpts, op);
    /* write the depth on the object */
    debug_depth(line->depth,line->points->x,line->points->y);
}

void draw_arcbox(F_line *line, int op)
{
    F_point	   *point;
    int		    xmin, xmax, ymin, ymax;
    int		    i;

    point = line->points;
    xmin = xmax = point->x;
    ymin = ymax = point->y;
    i = 0;
    while (point->next) {	/* find lower left (upper-left on screen) */
	/* and upper right (lower right on screen) */
	point = point->next;
	if (point->x < xmin)
	    xmin = point->x;
	else if (point->x > xmax)
	    xmax = point->x;
	if (point->y < ymin)
	    ymin = point->y;
	else if (point->y > ymax)
	    ymax = point->y;
	/* label the point number above the point */
	if (appres.shownums && active_layer(line->depth)) {
	    sprintf(bufx,"%d",i++);
	    pw_text(canvas_win, point->x, round(point->y-3.0/zoomscale), PAINT, line->depth,
			roman_font, 0.0, bufx, RED, COLOR_NONE);
	}
    }
    pw_arcbox(canvas_win, xmin, ymin, xmax, ymax, round(line->radius*ZOOM_FACTOR),
	      op, line->depth, line->thickness, line->style, line->style_val, line->fill_style,
	      line->pen_color, line->fill_color);
    /* write the depth on the object */
    debug_depth(line->depth,xmin,ymin);
}

static Boolean
subset(int xmin1, int ymin1, int xmax1, int ymax1, int xmin2, int ymin2, int xmax2, int ymax2)
{
    return (xmin2 <= xmin1) && (xmax1 <= xmax2) && 
           (ymin2 <= ymin1) && (ymax1 <= ymax2);
}

void draw_pic_pixmap(F_line *box, int op)
{
    int		    xmin, ymin;
    int		    xmax, ymax;
    int		    width, height, rotation;
    F_pos	    origin;
    F_pos	    opposite;
    Pixmap          clipmask;
    XGCValues	    gcv;

    origin.x = ZOOMX(box->points->x);
    origin.y = ZOOMY(box->points->y);
    opposite.x = ZOOMX(box->points->next->next->x);
    opposite.y = ZOOMY(box->points->next->next->y);

    xmin = min2(origin.x, opposite.x);
    ymin = min2(origin.y, opposite.y);
    xmax = max2(origin.x, opposite.x);
    ymax = max2(origin.y, opposite.y);

    if (op == ERASE) {
	clear_region(xmin, ymin, xmax, ymax);
	return;
    }
    /* width is upper-lower+1 */
    width = abs(origin.x - opposite.x) + 1;
    height = abs(origin.y - opposite.y) + 1;
    rotation = 0;
    if (origin.x > opposite.x && origin.y > opposite.y)
	rotation = 180;
    if (origin.x > opposite.x && origin.y <= opposite.y)
	rotation = 270;
    if (origin.x <= opposite.x && origin.y > opposite.y)
	rotation = 90;

    /* if something has changed regenerate the pixmap */
    if (box->pic->pixmap == 0 ||
	box->pic->color != box->pen_color ||
	box->pic->pix_rotation != rotation ||
	abs(box->pic->pix_width - width) > 1 ||		/* rounding makes diff of 1 bit */
	abs(box->pic->pix_height - height) > 1 ||
	box->pic->pix_flipped != box->pic->flipped)
	    create_pic_pixmap(box, rotation, width, height, box->pic->flipped);

    if (box->pic->mask) {
      /* mask is in rectangle (xmin,ymin)...(xmax,ymax)
         clip to rectangle (clip_xmin,clip_ymin)...(clip_xmax,clip_ymax) */
      if (subset(xmin, ymin, xmax, ymax, clip_xmin, clip_ymin, clip_xmax, clip_ymax)) {        
	gcv.clip_mask = box->pic->mask;
	gcv.clip_x_origin = xmin;
	gcv.clip_y_origin = ymin;
        clipmask = (Pixmap)0; /* don't need extra clipmask now */
      }
      else {
        /* compute intersection of the two rectangles */        
        GC depth_one_gc;
        int xxmin, yymin, xxmax, yymax, ww, hh;
        xxmin = max2(xmin, clip_xmin);
        xxmax = min2(xmax, clip_xmax);
        yymin = max2(ymin, clip_ymin);
        yymax = min2(ymax, clip_ymax);
        ww = xxmax - xxmin + 1;
        hh = yymax - yymin + 1;
	/* 
        The caller should have caught the case that pic and clip rectangle 
        don't overlap. So we may assume ww > 0, hh > 0.
	*/
        clipmask = XCreatePixmap(tool_d, canvas_win, ww, hh, 1);
	depth_one_gc = XCreateGC(tool_d, clipmask, (unsigned long) 0, 0);
	XSetForeground(tool_d, depth_one_gc, 0);
        XCopyArea(tool_d, box->pic->mask, clipmask, depth_one_gc,
                  xxmin - xmin, yymin - ymin, /* origin source */
                  ww, hh,                     /* width & height */
                  0, 0);                      /* origin destination */
        XFreeGC(tool_d, depth_one_gc);
	gcv.clip_mask = clipmask;
	gcv.clip_x_origin = xxmin;
	gcv.clip_y_origin = yymin; 
      }
      XChangeGC(tool_d, gccache[op], GCClipMask|GCClipXOrigin|GCClipYOrigin, &gcv);
    }
    XCopyArea(tool_d, box->pic->pixmap, canvas_win, gccache[op],
	      0, 0, width, height, xmin, ymin);
    if (box->pic->mask) {
	gcv.clip_mask = 0;
	XChangeGC(tool_d, gccache[op], GCClipMask, &gcv);
      if (clipmask)
        XFreePixmap(tool_d, clipmask);
      /* restore clipping */
      set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);
    }
    XFlush(tool_d);
}

/*
 * The input to this routine is the bitmap read from the source
 * image file. That input bitmap has an arbitrary number of rows
 * and columns. This routine re-samples the input bitmap creating
 * an output bitmap of dimensions width-by-height. This output
 * bitmap is made into a Pixmap for display purposes.
 */

#define	ALLOC_PIC_ERR "Can't alloc memory for image: %s"

void create_pic_pixmap(F_line *box, int rotation, int width, int height, int flipped)
{
    int		    cwidth, cheight;
    int		    i,j,k;
    int		    bwidth;
    unsigned char  *data, *tdata, *mask;
    int		    nbytes;
    int		    bbytes;
    int		    ibit, jbit, jnb;
    int		    wbit;
    int		    fg, bg;
    XImage	   *image;
    Boolean	    type1,hswap,vswap;

    /* this could take a while */
    set_temp_cursor(wait_cursor);
    if (box->pic->pixmap != 0)
	XFreePixmap(tool_d, box->pic->pixmap);
    if (box->pic->mask != 0)
	XFreePixmap(tool_d, box->pic->mask);

    if (appres.DEBUG)
	fprintf(stderr,"Scaling pic pixmap to %dx%d pixels\n",width,height);

    cwidth = box->pic->pic_cache->bit_size.x;	/* current width, height */
    cheight = box->pic->pic_cache->bit_size.y;

    box->pic->color = box->pen_color;
    box->pic->pix_rotation = rotation;
    box->pic->pix_width = width;
    box->pic->pix_height = height;
    box->pic->pix_flipped = flipped;
    box->pic->pixmap = (Pixmap) 0;
    box->pic->mask = (Pixmap) 0;

    mask = (unsigned char *) 0;

    /* create a new bitmap at the specified size (requires interpolation) */

    /* MONOCHROME display OR XBM */
    if (box->pic->pic_cache->numcols == 0) {
	    nbytes = (width + 7) / 8;
	    bbytes = (cwidth + 7) / 8;
	    if ((data = (unsigned char *) malloc(nbytes * height)) == NULL) {
		file_msg(ALLOC_PIC_ERR, box->pic->pic_cache->file);
		return;
	    }
	    if ((tdata = (unsigned char *) malloc(nbytes)) == NULL) {
		file_msg(ALLOC_PIC_ERR, box->pic->pic_cache->file);
		free(data);
		return;
	    }
	    bzero((char*)data, nbytes * height);	/* clear memory */
	    if ((!flipped && (rotation == 0 || rotation == 180)) ||
		(flipped && !(rotation == 0 || rotation == 180))) {
		for (j = 0; j < height; j++) {
		    /* check if user pressed cancel button */
		    if (check_cancel())
			break;
		    jbit = cheight * j / height * bbytes;
		    for (i = 0; i < width; i++) {
			ibit = cwidth * i / width;
			wbit = (unsigned char) *(box->pic->pic_cache->bitmap + jbit + ibit / 8);
			if (wbit & (1 << (7 - (ibit & 7))))
			    *(data + j * nbytes + i / 8) += (1 << (i & 7));
		    }
		}
	    } else {
		for (j = 0; j < height; j++) {
		    /* check if user pressed cancel button */
		    if (check_cancel())
			break;
		    ibit = cwidth * j / height;
		    for (i = 0; i < width; i++) {
			jbit = cheight * i / width * bbytes;
			wbit = (unsigned char) *(box->pic->pic_cache->bitmap + jbit + ibit / 8);
			if (wbit & (1 << (7 - (ibit & 7))))
			    *(data + (height - j - 1) * nbytes + i / 8) += (1 << (i & 7));
		    }
		}
	    }

	    /* horizontal swap */
	    if (rotation == 180 || rotation == 270)
		for (j = 0; j < height; j++) {
		    /* check if user pressed cancel button */
		    if (check_cancel())
			break;
		    jnb = j*nbytes;
		    bzero((char*)tdata, nbytes);
		    for (i = 0; i < width; i++)
			if (*(data + jnb + (width - i - 1) / 8) & (1 << ((width - i - 1) & 7)))
			    *(tdata + i / 8) += (1 << (i & 7));
		    bcopy(tdata, data + j * nbytes, nbytes);
		}

	    /* vertical swap */
	    if ((!flipped && (rotation == 180 || rotation == 270)) ||
		(flipped && !(rotation == 180 || rotation == 270))) {
		for (j = 0; j < (height + 1) / 2; j++) {
		    jnb = j*nbytes;
		    bcopy(data + jnb, tdata, nbytes);
		    bcopy(data + (height - j - 1) * nbytes, data + jnb, nbytes);
		    bcopy(tdata, data + (height - j - 1) * nbytes, nbytes);
		}
	    }

	    if (box->pic->pic_cache->subtype == T_PIC_XBM) {
		fg = x_color(box->pen_color);		/* xbm, use object pen color */
		bg = x_bg_color.pixel;
	    } else if (box->pic->pic_cache->subtype == T_PIC_EPS) {
		fg = black_color.pixel;			/* pbm from gs is inverted */
		bg = white_color.pixel;
	    } else {
		fg = white_color.pixel;			/* gif, xpm after map_to_mono */
		bg = black_color.pixel;
	    }
		
	    box->pic->pixmap = XCreatePixmapFromBitmapData(tool_d, canvas_win,
					(char *)data, width, height, fg,bg, tool_dpth);
	    free(data);
	    free(tdata);

      /* EPS, PCX, XPM, GIF, PNG or JPEG on *COLOR* display */
      /* It is important to note that the Cmap pixels are unsigned long. */
      /* Therefore all manipulation of the image data should be as unsigned long. */
      /* bpl = bytes per line */

      } else {
	    unsigned char	*pixel, *cpixel, *dst, *src, tmp;
	    int			 bpl, cbpp, cbpl;
	    unsigned int	*Lpixel;
	    unsigned short	*Spixel;
	    unsigned char	*Cpixel;
	    struct Cmap		*cmap = box->pic->pic_cache->cmap;
	    Boolean	    	 endian;
	    unsigned char	 byte;

	    /* figure what endian machine this is (big=True or little=False) */
	    endian = True;
	    bpl = 1;
	    Cpixel = (char *) &bpl;
	    /* look at first byte of integer */
	    if (Cpixel[0] == 1)
		endian = False;

	    cbpp = 1;
	    cbpl = cwidth * cbpp;
	    bpl = width * image_bpp;
	    if ((data = (unsigned char *) malloc(bpl * height)) == NULL) {
		file_msg(ALLOC_PIC_ERR,box->pic->pic_cache->file);
		return;
	    }
	    /* allocate mask for any transparency information */
	    if (box->pic->pic_cache->subtype == T_PIC_GIF && 
	        box->pic->pic_cache->transp != TRANSP_NONE) {
		    if ((mask = (unsigned char *) malloc((width+7)/8 * height)) == NULL) {
			file_msg(ALLOC_PIC_ERR,box->pic->pic_cache->file);
			free(data);
			return;
		    }
		    /* set all bits in mask */
		    for (i = (width+7)/8 * height - 1; i >= 0; i--)
			*(mask+i)=  (unsigned char) 255;
	    }
	    bwidth = (width+7)/8;
	    bzero((char*)data, bpl * height);

	    type1 = False;
	    hswap = False;
	    vswap = False;

	    if ((!flipped && (rotation == 0 || rotation == 180)) ||
		(flipped && !(rotation == 0 || rotation == 180)))
			type1 = True;
	    /* horizontal swap */
	    if (rotation == 180 || rotation == 270)
		hswap = True;
	    /* vertical swap */
	    if ((!flipped && (rotation == 90 || rotation == 180)) ||
		( flipped && (rotation == 90 || rotation == 180)))
			vswap = True;

	    for( j=0; j<height; j++ ) {
		  /* check if user pressed cancel button */
		  if (check_cancel())
			break;

		if (type1) {
			src = box->pic->pic_cache->bitmap + (j * cheight / height * cbpl);
			dst = data + (j * bpl);
		} else {
			src = box->pic->pic_cache->bitmap + (j * cbpl / height);
			dst = data + (j * bpl);
		}

		pixel = dst;
		for( i=0; i<width; i++ ) {
		    if (type1) {
			    cpixel = src + (i * cbpl / width );
		    } else {
			    cpixel = src + (i * cheight / width * cbpl);
		    }
		    /* if this pixel is the transparent color then clear the mask pixel */
		    if (box->pic->pic_cache->transp != TRANSP_NONE && 
			(*cpixel==(unsigned char) box->pic->pic_cache->transp)) {
			  if (type1) {
			    if (hswap) {
				if (vswap)
				    clr_mask_bit(height-j-1,width-i-1,bwidth,mask);
				else
				    clr_mask_bit(j,width-i-1,bwidth,mask);
			    } else {
				if (vswap)
				    clr_mask_bit(height-j-1,i,bwidth,mask);
				else
				    clr_mask_bit(j,i,bwidth,mask);
			    }
			  } else {
			    if (!vswap) {
				if (hswap)
				    clr_mask_bit(j,width-i-1,bwidth,mask);
				else
				    clr_mask_bit(j,i,bwidth,mask);
			    } else {
				if (hswap)
				    clr_mask_bit(height-j-1,width-i-1,bwidth,mask);
				else
				    clr_mask_bit(height-j-1,i,bwidth,mask);
			    }
			  } /* type */
		    }
		    if (image_bpp == 4) {
			Lpixel = (unsigned int *) pixel;
			*Lpixel = (unsigned int) cmap[*cpixel].pixel;
			/* swap the 4 bytes on big-endian machines */
			if (endian) {
			    Cpixel = (char *) Lpixel;
			    byte = Cpixel[0]; Cpixel[0] = Cpixel[3]; Cpixel[3] = byte;
			    byte = Cpixel[1]; Cpixel[1] = Cpixel[2]; Cpixel[2] = byte;
			}
		    } else if (image_bpp == 3) {
			unsigned char *p;
			p = (unsigned char *)&(cmap[*cpixel].pixel);
			/* note which endian */
			if (endian) {
			    Cpixel = (unsigned char *) pixel+2;
			    *Cpixel-- = *p++;
			    *Cpixel-- = *p++;
			    *Cpixel = *p;
			} else {
			    Cpixel = (unsigned char *) pixel;
			    *Cpixel++ = *p++;
			    *Cpixel++ = *p++;
			    *Cpixel = *p;
			}
		    } else if (image_bpp == 2) {
			Spixel = (unsigned short *) pixel;
			*Spixel = (unsigned short)cmap[*cpixel].pixel;
			/* swap the 2 bytes on big-endian machines */
			if (endian) {
			    Cpixel = (char *) Lpixel;
			    byte = Cpixel[0]; Cpixel[0] = Cpixel[1]; Cpixel[1] = byte;
			}
		    } else {
			Cpixel = (unsigned char *) pixel;
			*Cpixel = (unsigned char)cmap[*cpixel].pixel;
		    }
		    /* next pixel position */
		    pixel += image_bpp;
		}
	    }

	    /* horizontal swap */
	    if (hswap) {
		for( j=0; j<height; j++ ) {
		  /* check if user pressed cancel button */
		  if (check_cancel())
			break;
		  dst = data + (j * bpl);
		  src = dst + ((width - 1) * image_bpp);
		  for( i=0; i<width/2; i++, src -= 2*image_bpp ) {
		    for( k=0; k<image_bpp; k++, dst++, src++ ) {
		      tmp = *dst;
		      *dst = *src;
		      *src = tmp;
		    }
		  }
		}
	    }

	    /* vertical swap */
	    if (vswap) {
		for( i=0; i<width; i++ ) {
		  dst = data + (i * image_bpp);
		  src = dst + ((height - 1) * bpl);
		  for( j=0; j<height/2; j++, dst += (width-1)*image_bpp, src -= (width+1)*image_bpp ) {
		    for( k=0; k<image_bpp; k++, dst++, src++ ) {
		      tmp = *dst;
		      *dst = *src;
		      *src = tmp;
		    }
		  }
		}
	    }

	    image = XCreateImage(tool_d, tool_v, tool_dpth,
				ZPixmap, 0, (char *)data, width, height, 8, 0);
	    box->pic->pixmap = XCreatePixmap(tool_d, canvas_win,
				width, height, tool_dpth);
	    if (image->byte_order == MSBFirst) {
		    image->byte_order = LSBFirst;
		    _XInitImageFuncPtrs(image);
	    }
	    if (image->bitmap_bit_order == MSBFirst) {
		    image->bitmap_bit_order = LSBFirst;
		    _XInitImageFuncPtrs(image);
	    }
	    XPutImage(tool_d, box->pic->pixmap, pic_gc, image, 0, 0, 0, 0, width, height);
	    XDestroyImage(image);
	    /* make the clipmask to do the GIF transparency */
	    if (mask) {
		box->pic->mask = XCreateBitmapFromData(tool_d, tool_w, (char*) mask,
						width, height);
		free(mask);
	    }
    }
    reset_cursor();
}

/* clear bit at row "r", column "c" in array "mask" of width "width" */

static unsigned char bits[8] = { 1,2,4,8,16,32,64,128 };

void clr_mask_bit(int r, int c, int bwidth, unsigned char *mask)
{
    int		    byte;
    unsigned char   bit;

    byte = r*bwidth + c/8;
    bit  = c % 8;
    mask[byte] &= ~bits[bit];
}

/*********************** TEXT ***************************/

static char    *hidden_text_string = "<<>>";

void draw_text(F_text *text, int op)
{
    PR_SIZE	    size;
    int		    x,y;
    int		    xmin, ymin, xmax, ymax;
    int		    x1,y1, x2,y2, x3,y3, x4,y4;
    double	    cost, sint;

    if (text->zoom != zoomscale || text->fontstruct == (XFontStruct*) 0)
	reload_text_fstruct(text);
    text_bound(text, &xmin, &ymin, &xmax, &ymax,
	       &x1,&y1, &x2,&y2, &x3,&y3, &x4,&y4);

    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    /* outline the text bounds in red if debug resource is set */
    if (appres.DEBUG) {
	pw_vector(canvas_win, x1, y1, x2, y2, op, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, x2, y2, x3, y3, op, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, x3, y3, x4, y4, op, 1, RUBBER_LINE, 0.0, RED);
	pw_vector(canvas_win, x4, y4, x1, y1, op, 1, RUBBER_LINE, 0.0, RED);
    }

    x = text->base_x;
    y = text->base_y;
    cost = cos(text->angle);
    sint = sin(text->angle);
    if (text->type == T_CENTER_JUSTIFIED || text->type == T_RIGHT_JUSTIFIED) {
	size = textsize(text->fontstruct, strlen(text->cstring),
			    text->cstring);
	size.length = size.length/display_zoomscale;
	if (text->type == T_CENTER_JUSTIFIED) {
	    x = round(x-cost*size.length/2);
	    y = round(y+sint*size.length/2);
	} else {	/* T_RIGHT_JUSTIFIED */
	    x = round(x-cost*size.length);
	    y = round(y+sint*size.length);
	}
    }
    if (hidden_text(text)) {
	pw_text(canvas_win, x, y, op, text->depth, lookfont(0,12),
		text->angle, hidden_text_string, DEFAULT, COLOR_NONE);
    } else {
	/* if size is less than the displayable size, Greek it by drawing a DARK gray line,
	   UNLESS the depth is inactive in which case draw it in MED_GRAY */
	if (text->size*display_zoomscale < MIN_X_FONT_SIZE) {
	    x1 = (x1+x4)/2;
	    x2 = (x2+x3)/2;
	    y1 = (y1+y4)/2;
	    y2 = (y2+y3)/2;
	    greek_text(text, x1, y1, x2, y2);
	} else {
	    /* Otherwise, draw the text normally */
	    pw_text(canvas_win, x, y, op, text->depth, text->fontstruct,
		text->angle, text->cstring, text->color, COLOR_NONE);
	}
    }

    /* write the depth on the object */
    debug_depth(text->depth,x,y);
}

/*
 * Draw text as "Greeked", from (x1, y1) to (x2, y2), meaning as a dashed gray line.
 * This is done when the text would be too small to read anyway.
 */

void greek_text(F_text *text, int x1, int y1, int x2, int y2)
{
    int		 color;
    int		 lensofar, lenword, lenspace;
    int		 xs, ys, xe, ye;
    float	 dx, dy;
    char	 *cp;

    if (text->depth < MAX_DEPTH+1 && !active_layer(text->depth))
	color = MED_GRAY;
    else
	color = DARK_GRAY;

    cp = text->cstring;

    /* get unit dx, dy */
    dx = (float)(x2-x1)/strlen(cp);
    dy = (float)(y2-y1)/strlen(cp);

    lensofar = 0;
    while (*cp) {
	lenword = 0;
	/* count chars in this word */
	while (*cp) {
	    if (*cp == ' ')
		break;
	    lenword++;
	    cp++;
	}
	/* now count how many spaces follow it */
	lenspace = 0;
	while (*cp) {
	    if (*cp != ' ')
		break;
	    lenspace++;
	    cp++;
	}
	xs = x1 + dx*lensofar;
	ys = y1 + dy*lensofar;
	xe = xs + dx*lenword;
	ye = ys + dy*lenword;
	pw_vector(canvas_win, xs, ys, xe, ye, PAINT, 1, RUBBER_LINE, 0.0, color);
	/* add length of this word and space(s) to lensofar */
	lensofar = lensofar + lenword + lenspace;
    }
}

/*********************** COMPOUND ***************************/

void
draw_compoundelements(F_compound *c, int op)
{
    F_line	   *l;
    F_spline	   *s;
    F_ellipse	   *e;
    F_text	   *t;
    F_arc	   *a;
    F_compound	   *c1;

    if (!overlapping(ZOOMX(c->nwcorner.x), ZOOMY(c->nwcorner.y),
		     ZOOMX(c->secorner.x), ZOOMY(c->secorner.y),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    for (l = c->lines; l != NULL; l = l->next) {
	if (active_layer(l->depth))
	    draw_line(l, op);
    }
    for (s = c->splines; s != NULL; s = s->next) {
	if (active_layer(s->depth))
	    draw_spline(s, op);
    }
    for (a = c->arcs; a != NULL; a = a->next) {
	if (active_layer(a->depth))
	    draw_arc(a, op);
    }
    for (e = c->ellipses; e != NULL; e = e->next) {
	if (active_layer(e->depth))
	    draw_ellipse(e, op);
    }
    for (t = c->texts; t != NULL; t = t->next) {
	if (active_layer(t->depth))
	    draw_text(t, op);
    }
    for (c1 = c->compounds; c1 != NULL; c1 = c1->next) {
	if (any_active_in_compound(c1))
	    draw_compoundelements(c1, op);
    }
}

/*************************** ARROWS *****************************

 compute_arcarrow_angle - Computes a point on a line which is a chord
	to the arc specified by center (x1,y1) and endpoint (x2,y2),
	where the chord intersects the arc arrow->ht from the endpoint.

 May give strange values if the arrow.ht is larger than about 1/4 of
 the circumference of a circle on which the arc lies.

****************************************************************/

void compute_arcarrow_angle(float x1, float y1, int x2, int y2, int direction, F_arrow *arrow, int *x, int *y)
{
    double	r, alpha, beta, dy, dx;
    double	lpt,h;

    dy=y2-y1;
    dx=x2-x1;
    r=sqrt(dx*dx+dy*dy);

    h = (double) arrow->ht*ZOOM_FACTOR;
    /* lpt is the amount the arrowhead extends beyond the end of the line */
    lpt = arrow->thickness/2.0/(arrow->wd/h/2.0);
    /* add this to the length */
    h += lpt;

    /* radius too small for this method, use normal method */
    if (h > 2.0*r) {
	compute_normal(x1,y1,x2,y2,direction,x,y);
	return;
    }

    beta=atan2(dy,dx);
    if (direction) {
	alpha=2*asin(h/2.0/r);
    } else {
	alpha=-2*asin(h/2.0/r);
    }

    *x=round(x1+r*cos(beta+alpha));
    *y=round(y1+r*sin(beta+alpha));
}

/* temporary error handler - see call to XSetRegion in clip_arrows below */

int tempXErrorHandler (Display *display, XErrorEvent *event)
{
	return 0;
}


/****************************************************************

 clip_arrows - calculate a clipping region which is the current 
	clipping area minus the polygons at the arrowheads.

 This will prevent the object (line, spline etc.) from protruding
 on either side of the arrowhead Also calculate the arrowheads
 themselves and put the outline polygons in farpts[nfpts] for forward
 arrow and barpts[nbpts] for backward arrow, and the fill areas in
 farfillpts[nffillpts] for forward arrow and barfillpts[nbfillpts]
 for backward arrow.

 The points[] array must already have the points for the object
 being drawn (spline, line etc), and npoints, the number of points.

 "skip" points are skipped from each end of the points[] array (for splines)
****************************************************************/

void clip_arrows(F_line *obj, int objtype, int op, int skip)
{
    Region	    mainregion, newregion;
    Region	    region;
    XPoint	    xpts[50];
    int		    x, y;
    zXPoint	    clippts[50];
    int		    i, j, n, nclippts;


    if (obj->for_arrow || obj->back_arrow) {
	/* start with current clipping area - maybe we won't have to draw anything */
	xpts[0].x = clip_xmin;
	xpts[0].y = clip_ymin;
	xpts[1].x = clip_xmax;
	xpts[1].y = clip_ymin;
	xpts[2].x = clip_xmax;
	xpts[2].y = clip_ymax;
	xpts[3].x = clip_xmin;
	xpts[3].y = clip_ymax;
	mainregion = XPolygonRegion(xpts, 4, WindingRule);
    }

    if (skip > npoints-2)
	skip = 0;

    /* get points for any forward arrowhead */
    if (obj->for_arrow) {
	x = points[npoints-skip-2].x;
	y = points[npoints-skip-2].y;
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y, a->point[2].x,
				a->point[2].y, a->direction,
				a->for_arrow, &x, &y);
	}
	calc_arrow(x, y, points[npoints-1].x, points[npoints-1].y, obj->thickness,
		   obj->for_arrow, farpts, &nfpts, farfillpts, &nffillpts, clippts, &nclippts);
	if (nclippts) {
		/* set clipping in scaled space */
		for (i=0; i < nclippts; i++) {
		    xpts[i].x = ZOOMX(clippts[i].x);
		    xpts[i].y = ZOOMY(clippts[i].y);
		}
		n = i;
		/* draw the clipping area for debugging */
		if (appres.DEBUG) {
		  for (i=0; i<n; i++) {
		    if (i==n-1)
			j=0;
		    else
			j=i+1;
		    pw_vector(canvas_win,xpts[i].x,xpts[i].y,xpts[j].x,xpts[j].y,op,1,
			PANEL_LINE,0.0,RED);
		  }
		}
		region = XPolygonRegion(xpts, n, WindingRule);
		newregion = XCreateRegion();
		XSubtractRegion(mainregion, region, newregion);
		XDestroyRegion(region);
		XDestroyRegion(mainregion);
		mainregion=newregion;
	}
    }
	
    /* get points for any backward arrowhead */
    if (obj->back_arrow) {
	x = points[skip+1].x;
	y = points[skip+1].y;
	if (objtype == O_ARC) {
	    F_arc  *a = (F_arc *) obj;
	    compute_arcarrow_angle(a->center.x, a->center.y, a->point[0].x,
			       a->point[0].y, a->direction ^ 1,
			       a->back_arrow, &x, &y);
	}
	calc_arrow(x, y, points[0].x, points[0].y, obj->thickness,
		    obj->back_arrow, barpts, &nbpts, barfillpts, &nbfillpts, clippts, &nclippts);
	if (nclippts) {
		/* set clipping in scaled space */
		for (i=0; i < nclippts; i++) {
		    xpts[i].x = ZOOMX(clippts[i].x);
		    xpts[i].y = ZOOMY(clippts[i].y);
		}
		n = i;
		/* draw the clipping area for debugging */
		if (appres.DEBUG) {
		  int j;
		  for (i=0; i<n; i++) {
		    if (i==n-1)
			j=0;
		    else
			j=i+1;
		    pw_vector(canvas_win,xpts[i].x,xpts[i].y,xpts[j].x,xpts[j].y,op,1,
			PANEL_LINE,0.0,RED);
		  }
		}
		region = XPolygonRegion(xpts, n, WindingRule);
		newregion = XCreateRegion();
		XSubtractRegion(mainregion, region, newregion);
		XDestroyRegion(region);
		XDestroyRegion(mainregion);
		mainregion=newregion;
	}
    }
    /* now set the clipping region for the subsequent drawing of the object */
    if (obj->for_arrow || obj->back_arrow) {
	/* install a temporary error handler to ignore any BadMatch error
	   from the buggy R5 Xlib XSetRegion() */
	XSetErrorHandler (tempXErrorHandler);
	XSetRegion(tool_d, gccache[op], mainregion);
	/* restore original error handler */
	if (!appres.DEBUG)
	    XSetErrorHandler(X_error_handler);
	XDestroyRegion(mainregion);
    }
}

/****************************************************************

 calc_arrow - calculate arrowhead points heading from (x1, y1) to (x2, y2)

		        |\
		        |  \
		        |    \
(x1,y1) +---------------|      \+ (x2, y2)
		        |      /
		        |    /
		        |  /
		        |/ 

 Fills points[] array with npoints arrowhead *outline* coordinates and
 fillpoints[] array with nfillpoints points for the part to be filled *IF*
 it is a special arrowhead that has a different fill area than the outline.

 Otherwise, the points[] array is also used to fill the arrowhead in draw_arrow()
 The linethick param is the thickness of the *main line/spline/arc*,
 not the arrowhead.

 The clippts[] array is filled with the clip area so that the line won't
 protrude through the arrowhead.

****************************************************************/

#define ROTX(x,y)  (x)*cosa + (y)*sina + xa
#define ROTY(x,y) -(x)*sina + (y)*cosa + ya

#define ROTX2(x,y)  (x)*cosa + (y)*sina + x2
#define ROTY2(x,y) -(x)*sina + (y)*cosa + y2

#define ROTXC(x,y)  (x)*cosa + (y)*sina + fix_x
#define ROTYC(x,y) -(x)*sina + (y)*cosa + fix_y

void calc_arrow(int x1, int y1, int x2, int y2, int linethick, F_arrow *arrow, zXPoint *points, int *npoints, zXPoint *fillpoints, int *nfillpoints, zXPoint *clippts, int *nclippts)
{
    double	    x, y, xb, yb, dx, dy, l, sina, cosa;
    double	    mx, my;
    double	    ddx, ddy, lpt, tipmv;
    double	    alpha;
    double	    miny, maxy;
    int		    xa, ya, xs, ys;
    double	    wd  = (double) arrow->wd*ZOOM_FACTOR;
    double	    len = (double) arrow->ht*ZOOM_FACTOR;
    double	    th  = arrow->thickness*ZOOM_FACTOR;
    double	    radius;
    double	    angle, init_angle, rads;
    double	    fix_x, fix_y;
    int		    type, style, indx, tip;
    int		    i, np;
    int		    offset, halfthick;

    /* to enlarge the clip area in case the line is thick */
    halfthick = linethick * ZOOM_FACTOR/2 + 1;

    /* types = 0...10 */
    type = arrow->type;
    /* style = 0 (unfilled) or 1 (filled) */
    style = arrow->style;
    /* index into shape array */
    indx = 2*type + style;

    *npoints = *nfillpoints = 0;
    *nclippts = 0;
    dx = x2 - x1;
    dy = y1 - y2;
    /* return now if arrowhead width or length is 0 or line has zero length */
    if (wd == 0 || len == 0 || (dx==0 && dy==0))
	return;

    /* lpt is the amount the arrowhead extends beyond the end of the
       line because of the sharp point (miter join) */
    tipmv = arrow_shapes[indx].tipmv;
    lpt = 0.0;
    if (tipmv > 0.0)
	lpt = th / (2.0 * sin(atan(wd / (tipmv * len))));
    else if (tipmv == 0.0)
	lpt = th / 2.0;	/* types which have blunt end */
			/* (Don't adjust those with tipmv < 0) */

    /* alpha is the angle the line is relative to horizontal */
    alpha = atan2(dy,-dx);

    /* ddx, ddy is amount to move end of line back so that arrowhead point
       ends where line used to */
    ddx = lpt * cos(alpha);
    ddy = lpt * sin(alpha);

    /* move endpoint of line back */
    mx = x2 + ddx;
    my = y2 + ddy;

    l = sqrt(dx * dx + dy * dy);
    sina = dy / l;
    cosa = dx / l;
    xb = mx * cosa - my * sina;
    yb = mx * sina + my * cosa;

    /* (xa,ya) is the rotated endpoint (used in ROTX and ROTY macros) */
    xa =  xb * cosa + yb * sina + 0.5;
    ya = -xb * sina + yb * cosa + 0.5;

    miny =  100000.0;
    maxy = -100000.0;

    if (type == 5 || type == 6) {
	/*
	 * CIRCLE and HALF-CIRCLE arrowheads
	 *
	 * We approximate circles with (40+zoom)/4 points
	 */

	/* use original dx, dy to get starting angle */
	init_angle = compute_angle(dx, dy);

	/* (xs,ys) is a point the length of the arrowhead BACK from
	   the end of the shaft */
	/* for the half circle, use 0.0 */
	xs =  (xb-(type==5? len: 0.0)) * cosa + yb * sina + 0.5;
	ys = -(xb-(type==5? len: 0.0)) * sina + yb * cosa + 0.5;

	/* calc new (dx, dy) from moved endpoint to (xs, ys) */
	dx = mx - xs;
	dy = my - ys;
	/* radius */
	radius = len/2.0;
	fix_x = xs + (dx / (double) 2.0);
	fix_y = ys + (dy / (double) 2.0);
	/* choose number of points for circle - 40+zoom/4 points */
	np = round(display_zoomscale/4.0) + 40;

	if (type == 5) {
	    /* full circle */
	    init_angle = 5.0*M_PI_2 - init_angle;
	    rads = M_2PI;
	} else {
	    /* half circle */
	    init_angle = 3.0*M_PI_2 - init_angle;
	    rads = M_PI;
	}

	/* draw the half or full circle */
	for (i = 0; i < np; i++) {
	    angle = init_angle - (rads * (double) i / (double) (np-1));
	    x = fix_x + round(radius * cos(angle));
	    points[*npoints].x = x;
	    y = fix_y + round(radius * sin(angle));
	    points[*npoints].y = y;
	    (*npoints)++;
	}

	/* set clipping to a box at least as large as the line thickness
	   or diameter of the circle, whichever is larger */
	/* 4 points in clip box */
	miny = min2(-halfthick, -radius-th/2.0);
	maxy = max2( halfthick,  radius+th/2.0);

	i=0;
	/* start at new endpoint of line */
	clippts[i].x = ROTXC(0,            -radius-th/2.0);
	clippts[i].y = ROTYC(0,            -radius-th/2.0);
	i++;
	clippts[i].x = ROTXC(0,             miny);
	clippts[i].y = ROTYC(0,             miny);
	i++;
	clippts[i].x = ROTXC(radius+th/2.0, miny);
	clippts[i].y = ROTYC(radius+th/2.0, miny);
	i++;
	clippts[i].x = ROTXC(radius+th/2.0, maxy);
	clippts[i].y = ROTYC(radius+th/2.0, maxy);
	i++;
	clippts[i].x = ROTXC(0,             maxy);
	clippts[i].y = ROTYC(0,             maxy);
	i++;
	*nclippts = i;

    } else {
	/*
	 * ALL OTHER HEADS
	 */

	*npoints = arrow_shapes[indx].numpts;
	/* we'll shift the half arrowheads down by the difference of the main line thickness 
	   and the arrowhead thickness to make it flush with the main line */
	if (arrow_shapes[indx].half)
	    offset = ZOOM_FACTOR * (linethick - arrow->thickness)/2;
	else
	    offset = 0;

	/* fill the points array with the outline */
	for (i=0; i<*npoints; i++) {
	    x = arrow_shapes[indx].points[i].x * len;
	    y = arrow_shapes[indx].points[i].y * wd - offset;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    points[i].x = ROTX(x,y);
	    points[i].y = ROTY(x,y);
	}

	/* and the fill points array if there are fill points different from the outline */
	*nfillpoints = arrow_shapes[indx].numfillpts;
	for (i=0; i<*nfillpoints; i++) {
	    x = arrow_shapes[indx].fillpoints[i].x * len;
	    y = arrow_shapes[indx].fillpoints[i].y * wd - offset;
	    miny = min2(y, miny);
	    maxy = max2(y, maxy);
	    fillpoints[i].x = ROTX(x,y);
	    fillpoints[i].y = ROTY(x,y);
	}

	/* to include thick lines in clip area */
	miny = min2(miny, -halfthick);
	maxy = max2(maxy, halfthick);

	/* set clipping to the first three points of the arrowhead and
	   the (enlarged) box surrounding it */
	*nclippts = 0;
	if (arrow_shapes[indx].clip) {
		for (i=0; i < 3; i++) {
		    x = arrow_shapes[indx].points[i].x * len;
		    y = arrow_shapes[indx].points[i].y * wd - offset;
		    clippts[i].x = ROTX(x,y);
		    clippts[i].y = ROTY(x,y);
		}

		/* locate the tip of the head */
		tip = arrow_shapes[indx].tipno;

		/* now make the box around it at least as large as the line thickness */
		/* start with last x, lower y */

		clippts[i].x = ROTX(x,miny);
		clippts[i].y = ROTY(x,miny);
		i++;
		/* x tip, same y (note different offset in ROTX/Y2 rotation) */
		clippts[i].x = ROTX2(arrow_shapes[indx].points[tip].x*len + ZOOM_FACTOR, miny);
		clippts[i].y = ROTY2(arrow_shapes[indx].points[tip].x*len + ZOOM_FACTOR, miny);
		i++;
		/* x tip, upper y (note different offset in ROTX/Y2 rotation) */
		clippts[i].x = ROTX2(arrow_shapes[indx].points[tip].x*len + ZOOM_FACTOR, maxy);
		clippts[i].y = ROTY2(arrow_shapes[indx].points[tip].x*len + ZOOM_FACTOR, maxy);
		i++;
		/* first x of arrowhead, upper y */
		clippts[i].x = ROTX(arrow_shapes[indx].points[0].x*len, maxy);
		clippts[i].y = ROTY(arrow_shapes[indx].points[0].x*len, maxy);
		i++;
	}
	/* set the number of points in the clip or bounds */
	*nclippts = i;
    }
}

/* draw the arrowhead resulting from the call to calc_arrow() */
/* points[npoints] contains the outline and points2[npoints2] the points to be filled */

void draw_arrow(F_line *obj, F_arrow *arrow, zXPoint *points, int npoints, zXPoint *points2, int npoints2, int op)
{
    int		    fill;

    if (obj->thickness == 0)
	return;
    if (arrow->type == 0 || arrow->type >= 13)
	fill = UNFILLED;			/* old arrow head or new unfilled types */
    else if (arrow->style == 0)
	fill = NUMTINTPATS+NUMSHADEPATS-1;	/* "hollow", fill with white */
    else
	fill = NUMSHADEPATS-1;			/* "solid", fill with solid color */
    if (npoints2==0)
	/* no special fill, use outline points to fill too */
	pw_lines(canvas_win, points, npoints, op, obj->depth, round(arrow->thickness),
		SOLID_LINE, 0.0, JOIN_MITER, CAP_BUTT,
		fill, obj->pen_color, obj->pen_color);
    else {
	/* fill whole (outline) with white to obscure the line inside */
	pw_lines(canvas_win, points, npoints, op, obj->depth, 0,
		SOLID_LINE, 0.0, JOIN_MITER, CAP_BUTT,
		NUMTINTPATS+NUMSHADEPATS-1, obj->pen_color, obj->pen_color);
	/* draw outline */
	pw_lines(canvas_win, points, npoints, op, obj->depth, round(arrow->thickness),
		SOLID_LINE, 0.0, JOIN_MITER, CAP_BUTT,
		UNFILLED, obj->pen_color, obj->pen_color);
	/* fill special part with pen color */
	pw_lines(canvas_win, points2, npoints2, op, obj->depth, 0,
		SOLID_LINE, 0.0, JOIN_MITER, CAP_BUTT,
		NUMSHADEPATS-1, obj->pen_color, obj->pen_color);
    }
}

/********************* CURVES FOR ARCS AND ELLIPSES ***************

 This routine plot two dimensional curve defined by a second degree
 polynomial of the form : 2    2 f(x, y) = ax + by + g = 0

 (x0,y0) is the starting point as well as ending point of the curve. The curve
 will translate with the offset xoff and yoff.

 This algorithm is derived from the eight point algorithm in : "An Improved
 Algorithm for the generation of Nonparametric Curves" by Bernard W.
 Jordan, William J. Lennon and Barry D. Holm, IEEE Transaction on Computers
 Vol C-22, No. 12 December 1973.

 This routine is only called for ellipses when the angle is 0 and the line type
 is not solid.  For angles of 0 with solid lines, pw_curve() is called.
 For all other angles angle_ellipse() is called.

 Will fill the curve if fill_style is != UNFILLED (-1)
 Call with draw_points = True to display the points using draw_point_array
	Otherwise global points array is filled with npoints values but
	not displayed.
 Call with draw_center = True and center_x, center_y set to draw endpoints
	to center point (xoff,yoff) (arc type 2, i.e. pie wedge)

****************************************************************/

void
curve(Window window, int depth, int xstart, int ystart, int xend, int yend, 
	Boolean draw_points, Boolean draw_center, int direction,
	int a, int b, int xoff, int yoff, int op, int thick,
	int style, float style_val, int fill_style, 
	Color pen_color, Color fill_color, int cap_style)
{
    register int    x, y;
    register double deltax, deltay, dfx, dfy;
    double	    dfxx, dfyy;
    double	    falpha, fx, fy, fxy, absfx, absfy, absfxy;
    int		    margin, test_succeed, inc, dec;
    float	    zoom;

    /* if this depth is inactive, draw the curve in gray */
    if (depth < MAX_DEPTH+1 && !active_layer(depth)) {
	pen_color = MED_GRAY;
	fill_color = LT_GRAY;
    }

    zoom = 1.0;
    /* if drawing on canvas (not in indicator button) adjust values by zoomscale */
    if (style != PANEL_LINE) {
	zoom = zoomscale;
	xstart = round(xstart * zoom);
	ystart = round(ystart * zoom);
	xend = round(xend * zoom);
	yend = round(yend * zoom);
	a = round(a * zoom);
	b = round(b * zoom);
	xoff = round(xoff * zoom);
	yoff = round(yoff * zoom);
    }

    init_point_array();

    /* this must be AFTER init_point_array() */
    if (a == 0 || b == 0)
	return;

    x = xstart;
    y = ystart;
    dfx = 2 * (double) a * (double) xstart;
    dfy = 2 * (double) b * (double) ystart;
    dfxx = 2 * (double) a;
    dfyy = 2 * (double) b;

    falpha = 0;
    if (direction) {
	inc = 1;
	dec = -1;
    } else {
	inc = -1;
	dec = 1;
    }
    if (xstart == xend && ystart == yend) {
	test_succeed = margin = 2;
    } else {
	test_succeed = margin = 3;
    }

    if (!add_point(round((xoff + x)/zoom), round((yoff - y)/zoom))) {
	return;
    } else {
      while (test_succeed) {
	deltax = (dfy < 0) ? inc : dec;
	deltay = (dfx < 0) ? dec : inc;
	fx = falpha + dfx * deltax + a;
	fy = falpha + dfy * deltay + b;
	fxy = fx + fy - falpha;
	absfx = fabs(fx);
	absfy = fabs(fy);
	absfxy = fabs(fxy);

	if ((absfxy <= absfx) && (absfxy <= absfy))
	    falpha = fxy;
	else if (absfy <= absfx) {
	    deltax = 0;
	    falpha = fy;
	} else {
	    deltay = 0;
	    falpha = fx;
	}
	x += deltax;
	y += deltay;
	dfx += (dfxx * deltax);
	dfy += (dfyy * deltay);

	if (!add_point(round((xoff + x)/zoom), round((yoff - y)/zoom))) {
	    break;
	}

	if ((abs(x - xend) < margin && abs(y - yend) < margin) &&
	    (x != xend || y != yend))
		test_succeed--;
      }

    }

    if (xstart == xend && ystart == yend)	/* end points should touch */
	if (!add_point(round((xoff + xstart)/zoom),
			round((yoff - ystart)/zoom)))
		too_many_points();

    /* if this is arc type 2 then connect end points to center */
    if (draw_center) {
	if (!add_point(round(xoff/zoom),round(yoff/zoom)))
		too_many_points();
	if (!add_point(round((xoff + xstart)/zoom),round((yoff - ystart)/zoom)))
		too_many_points();
    }
	
    if (draw_points) {
	draw_point_array(window, op, depth, thick, style, style_val, JOIN_BEVEL,
			cap_style, fill_style, pen_color, fill_color);
    }
}

/* redraw all the picture objects */

void redraw_images(F_compound *obj)
{
    F_line	   *l;
    F_compound	   *c;

    for (c = obj->compounds; c != NULL; c = c->next) {
	redraw_images(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE && l->pic->pic_cache && l->pic->pic_cache->numcols > 0)
	    redisplay_line(l);
    }
}

void too_many_points(void)
{
    put_msg("Too many points, recompile with MAXNUMPTS > %d in w_drawprim.h", MAXNUMPTS);
}

void debug_depth(int depth, int x, int y)
{
    char	str[10];
    PR_SIZE	size;

    if (appres.DEBUG) {
	sprintf(str,"%d",depth);
	size = textsize(roman_font, strlen(str), str);
	pw_text(canvas_win, x-size.length-round(3.0/zoomscale), round(y-3.0/zoomscale),
		PAINT, depth, roman_font, 0.0, str, RED, COLOR_NONE);
    }
}

/*********************** SPLINE ***************************/

/**********************************/
/* include common spline routines */
/**********************************/

void
draw_spline(F_spline *spline, int op)
{
    Boolean         success;
    int		    xmin, ymin, xmax, ymax;
    int		    i;
    F_point	   *p;
    float           precision;

    spline_bound(spline, &xmin, &ymin, &xmax, &ymax);
    if (!overlapping(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax),
		     clip_xmin, clip_ymin, clip_xmax, clip_ymax))
	return;

    precision = (display_zoomscale < ZOOM_PRECISION) ? LOW_PRECISION 
                                                     : HIGH_PRECISION;

    if (appres.shownums && active_layer(spline->depth)) {
	for (i=0, p=spline->points; p; p=p->next) {
	    /* label the point number above the point */
	    sprintf(bufx,"%d",i++);
	    pw_text(canvas_win, p->x, round(p->y-3.0/zoomscale), PAINT, spline->depth,
		roman_font, 0.0, bufx, RED, COLOR_NONE);
	}
    }
    if (open_spline(spline))
	success = compute_open_spline(spline, precision);
    else
	success = compute_closed_spline(spline, precision);
    if (success) {
	/* setup clipping so that spline doesn't protrude beyond arrowhead */
	/* also create the arrowheads */
	clip_arrows(spline,O_SPLINE,op,4);

	draw_point_array(canvas_win, op, spline->depth, spline->thickness,
		       spline->style, spline->style_val,
		       JOIN_MITER, spline->cap_style,
		       spline->fill_style, spline->pen_color,
		       spline->fill_color);
	/* restore clipping */
	set_clip_window(clip_xmin, clip_ymin, clip_xmax, clip_ymax);

	if (spline->for_arrow)	/* forward arrow  */
	    draw_arrow(spline, spline->for_arrow, farpts, nfpts, farfillpts, nffillpts, op);
	if (spline->back_arrow)	/* backward arrow  */
	    draw_arrow(spline, spline->back_arrow, barpts, nbpts, barfillpts, nbfillpts, op);
	/* write the depth on the object */
	debug_depth(spline->depth,spline->points->x,spline->points->y);
    }
}

void
quick_draw_spline(F_spline *spline, int operator)
{
  int        k;
  float     step;
  F_point   *p0, *p1, *p2, *p3;
  F_sfactor *s0, *s1, *s2, *s3;
  
  init_point_array();

  INIT_CONTROL_POINTS(spline, p0, s0, p1, s1, p2, s2, p3, s3);
 
  for (k=0 ; p3!=NULL ; k++) {
      SPLINE_SEGMENT_LOOP(k, p0, p1, p2, p3, s1->s, s2->s, LOW_PRECISION);
      NEXT_CONTROL_POINTS(p0, s0, p1, s1, p2, s2, p3, s3);
  }
  draw_point_array(canvas_win, operator, spline->depth, spline->thickness,
		   spline->style, spline->style_val,
		   JOIN_MITER, spline->cap_style,
		   spline->fill_style, spline->pen_color, spline->fill_color);
}

