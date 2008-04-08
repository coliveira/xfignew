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

#ifndef W_DRAWPRIM_H
#define W_DRAWPRIM_H

#include "w_zoom.h"

/* our version of XPoint */

typedef struct { int x,y; } zXPoint ;

/* function prototypes */

extern void pw_text(Window w, int x, int y, int op, int depth, XFontStruct *fstruct,
	float angle, char *string, Color color, Color background);
extern void pw_vector(Window w, int x1, int y1, int x2, int y2, int op,
	  int line_width, int line_style, float style_val, Color color);
extern void pw_curve(Window w, int xstart, int ystart, int xend, int yend,
	 int op, int depth, int linewidth, int style, float style_val, int fill_style,
	 Color pen_color, Color fill_color, int cap_style);
extern void pw_point(Window w, int x, int y, int op, int depth, int line_width,
	 Color color, int cap_style);
extern void pw_arcbox(Window w, int xmin, int ymin, int xmax, int ymax, int radius,
	  int op, int depth, int line_width, int line_style,
	  float style_val, int fill_style, Color pen_color, Color fill_color);
extern void pw_lines(Window w, zXPoint *points, int npoints, int op, int depth,
	 int line_width, int line_style, float style_val,
	 int join_style, int cap_style, int fill_style,
	 Color pen_color, Color fill_color);
extern void init_font(void);
extern void init_fill_gc (void);
extern void init_fill_pm (void);
extern void reset_clip_window (void);
extern void set_clip_window (int xmin, int ymin, int xmax, int ymax);
extern void set_fill_gc (int fill_style, int op, int pencolor, int fillcolor, int xorg, int yorg);
extern void set_line_stuff (int width, int style, float style_val, int join_style, int cap_style, int op, int color);
extern int x_color (int col);
extern void init_gc(void);

/* convert Fig units to pixels at current zoom */

#define ZOOMX(x) (int) round(zoomscale*((x)-zoomxoff))
#define ZOOMY(y) (int) round(zoomscale*((y)-zoomyoff))

/* convert pixels to Fig units at current zoom */
#define BACKX(x) round(x/zoomscale+zoomxoff)
#define BACKY(y) round(y/zoomscale+zoomyoff)

#define zXDrawArc(disp,win,gc,x,y,d1,d2,a1,a2)\
    XDrawArc(disp,win,gc,ZOOMX(x),ZOOMY(y), \
	     (short)round(zoomscale*(d1)),(short)round(zoomscale*(d2)),\
	     a1,a2)

#define zXFillArc(disp,win,gc,x,y,d1,d2,a1,a2)\
    XFillArc(disp,win,gc,ZOOMX(x),ZOOMY(y), \
	     (short)round(zoomscale*(d1)),(short)round(zoomscale*(d2)),\
	     a1,a2)
#define zXDrawLine(disp,win,gc,x1,y1,x2,y2)\
    XDrawLine(disp,win,gc,ZOOMX(x1),ZOOMY(y1), \
	      ZOOMX(x2),ZOOMY(y2))

#define zXRotDrawString(disp,font,ang,win,gc,x,y,s)\
    XRotDrawString(disp,font,ang,win,gc,ZOOMX(x),ZOOMY(y),s)

#define zXRotDrawImageString(disp,font,ang,win,gc,x,y,s)\
    XRotDrawImageString(disp,font,ang,win,gc,ZOOMX(x),ZOOMY(y),s)

#define zXFillRectangle(disp,win,gc,x,y,w,h)\
    XFillRectangle(disp,win,gc,ZOOMX(x),ZOOMY(y),\
		(short)round(zoomscale*(w)),(short)round(zoomscale*(h)))

#define zXDrawRectangle(disp,win,gc,x,y,w,h)\
    XDrawRectangle(disp,win,gc,ZOOMX(x),ZOOMY(y),\
		(short)round(zoomscale*(w)),(short)round(zoomscale*(h)))


extern pr_size      textsize(XFontStruct *fstruct, int n, char *s);
extern XFontStruct *bold_font;
extern XFontStruct *roman_font;
extern XFontStruct *button_font;
extern XFontStruct *canvas_font;
extern XFontStruct *lookfont(int fnum, int size);
extern GC	    makegc(int op, Pixel fg, Pixel bg);

/* patterns like bricks, etc */
typedef struct _patrn_strct {
	  int	 owidth,oheight;	/* original width/height */
	  char	*odata;			/* original bytes */
	  int	 cwidth,cheight;	/* current width/height */
	  char	*cdata;			/* bytes at current zoom */
    } patrn_strct;

#define SHADE_IM_SIZE	32		/* fixed by literal patterns in w_drawprim.c */
extern patrn_strct	pattern_images[NUMPATTERNS];
extern unsigned char	shade_images[NUMSHADEPATS][128];

/* Maximum number of points for polygons etc */
/* This may be overridden by adding -DMAXNUMPTS=xxxx in the Imakefile/Makefile */
#ifndef MAXNUMPTS
#define		MAXNUMPTS	25000
#endif /* MAXNUMPTS */

#define		NORMAL_FONT	"fixed"
#define		BOLD_FONT	"8x13bold"
#define		BUTTON_FONT	"6x13"

#define		max_char_height(font) \
		((font)->max_bounds.ascent + (font)->max_bounds.descent)

#define		char_width(font) ((font)->max_bounds.width)

#define		char_advance(font,char) \
		    (((font)->per_char)?\
		    ((font)->per_char[(char)-(font)->min_char_or_byte2].width):\
		    ((font)->max_bounds.width))

#define set_x_fg_color(gc,col) XSetForeground(tool_d,gc, x_color(col))
#define set_x_bg_color(gc,col) XSetBackground(tool_d,gc, x_color(col))

#endif /* W_DRAWPRIM_H */
