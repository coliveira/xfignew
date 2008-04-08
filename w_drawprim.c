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
 * This file provides some drawing primitives which make use of the
 * underlying low-level windowing system operations.
 *
 * The file is divided into routines for:
 *
 * GRAPHICS CONTEXTS (which are used by all the following)
 * FONTS
 * LINES
 * SHADING
 */

/* IMPORTS */

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "paintop.h"
#include "mode.h"
#include "object.h"
#include "u_create.h"
#include "u_fonts.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"

#include "u_create.h"
#include "w_cursor.h"
#include "w_file.h"
#include "w_rottext.h"

/* EXPORTS */

XFontStruct	*bold_font;
XFontStruct	*roman_font;
XFontStruct	*button_font;
XFontStruct	*canvas_font;

/* LOCAL */

static void	clip_line(int x1, int y1, int x2, int y2,  short *x3, short *y3, short *x4, short *y4);
static int	clip_poly(XPoint *inVertices, int npoints, XPoint *outVertices);
static void	intersect(XPoint first, XPoint second, int x1, int y1, int x2, int y2, XPoint *intersectPt);
static Boolean	inside (XPoint testVertex, int x1, int y1, int x2, int y2);
static void	setup_next(int npoints, XPoint *in, XPoint *out);
static Pixel	gc_color[NUMOPS], gc_background[NUMOPS];
static XRectangle clip[1];
static int	parsesize(char *name);
static Boolean	openwinfonts;

#define MAXNAMES 35

static struct {
    char	   *fn;
    int		    s;
}		flist[MAXNAMES];


void rescale_pattern (int patnum);
void zXFillPolygon (Display *d, Window w, GC gc, zXPoint *points, int n, int complex, int coordmode);
void zXDrawLines (Display *d, Window w, GC gc, zXPoint *points, int n, int coordmode);
void scale_pattern (int indx);
int SutherlandHodgmanPolygoClip (XPoint *inVertices, XPoint *outVertices, int inLength, int x1, int y1, int x2, int y2);

void init_font(void)
{
    struct xfont   *newfont, *nf;
    int		    f, count, i, p, ss;
    char	    template[300];
    char	  **fontlist, **fname;

    if (appres.boldFont == NULL || *appres.boldFont == '\0')
	appres.boldFont = BOLD_FONT;
    if (appres.normalFont == NULL || *appres.normalFont == '\0')
	appres.normalFont = NORMAL_FONT;
    if (appres.buttonFont == NULL || *appres.buttonFont == '\0')
	appres.buttonFont = BUTTON_FONT;

    while ((roman_font = XLoadQueryFont(tool_d, appres.normalFont)) == 0) {
	if (strcmp(appres.normalFont,"fixed") == 0) {
	    fprintf(stderr, "Can't load 'fixed' font, something is wrong");
	    fprintf(stderr," with your server - quitting.\n");
	    exit(1);
	}
	file_msg("Can't load font: %s, using 'fixed'\n", appres.normalFont);
	appres.normalFont = "fixed";
    } /* now loop to load "fixed" */
    hidden_text_length = 4 * roman_font->max_bounds.width;
    if ((bold_font = XLoadQueryFont(tool_d, appres.boldFont)) == 0) {
	file_msg("Can't load font: %s, using %s\n",
		appres.boldFont, appres.normalFont);
	bold_font = XLoadQueryFont(tool_d, appres.normalFont);
    }
    if ((button_font = XLoadQueryFont(tool_d, appres.buttonFont)) == 0) {
	file_msg("Can't load font: %s, using %s\n",
		appres.buttonFont, appres.normalFont);
	button_font = XLoadQueryFont(tool_d, appres.normalFont);
    }
    /*
     * Now initialize the font structure for the X fonts corresponding to the
     * Postscript fonts for the canvas.	 OpenWindows can use any LaserWriter
     * fonts at any size, so we don't need to load anything if we are using
     * it.
     */

    /* if the user hasn't disallowed scalable fonts, check that the
       server really has them by checking for font of 0-0 size */
    openwinfonts = False;
    if (appres.scalablefonts) {
	/* first look for OpenWindow style font names (e.g. times-roman) */
	if ((fontlist = XListFonts(tool_d, ps_fontinfo[1].name, 1, &count))!=0) {
		openwinfonts = True;	/* yes, use them */
		for (f=0; f<NUM_FONTS; f++)	/* copy the OpenWindow font names */
		    x_fontinfo[f].Template = ps_fontinfo[f+1].name;
	} else {
	    strcpy(template,x_fontinfo[0].Template);  /* nope, check for font size 0 */
	    strcat(template,"0-0-*-*-*-*-");
	    /* add ISO8859 (if not Symbol font or ZapfDingbats) to font name */
	    if (strstr(template,"ymbol") == NULL &&
		strstr(template,"ingbats") == NULL)
		    strcat(template,"ISO8859-*");
	    else
		strcat(template,"*-*");
	    fontlist = XListFonts(tool_d, template, 1, &count);
	}
	XFreeFontNames(fontlist); 
    }

    /* no scalable fonts - query the server for all the font
       names and sizes and build a list of them */

    if (!appres.scalablefonts) {
	for (f = 0; f < NUM_FONTS; f++) {
	    nf = NULL;
	    strcpy(template,x_fontinfo[f].Template);
	    strcat(template,"*-*-*-*-*-*-");
	    /* add ISO8859 (if not Symbol font or ZapfDingbats) to font name */
	    if (strstr(template,"ymbol") == NULL &&
		strstr(template,"ingbats") == NULL)
		    strcat(template,"ISO8859-*");
	    else
		strcat(template,"*-*");
	    /* don't free the Fontlist because we keep pointers into it */
	    p = 0;
	    if ((fontlist = XListFonts(tool_d, template, MAXNAMES, &count))==0) {
		/* no fonts by that name found, substitute the -normal font name */
		flist[p].fn = appres.normalFont;
		flist[p++].s = 12;	/* just set the size to 12 */
	    } else {
		fname = fontlist; /* go through the list finding point
				   * sizes */
		while (count--) {
		ss = parsesize(*fname);	/* get the point size from
					 * the name */
		flist[p].fn = *fname++;	/* save name of this size
					 * font */
		flist[p++].s = ss;	/* and save size */
		}
	    }
	    for (ss = 4; ss <= 50; ss++) {
		for (i = 0; i < p; i++)
			if (flist[i].s == ss)
			    break;
		if (i < p && flist[i].s == ss) {
			newfont = (struct xfont *) malloc(sizeof(struct xfont));
			if (nf == NULL)
			    x_fontinfo[f].xfontlist = newfont;
			else
			    nf->next = newfont;
			nf = newfont;	/* keep current ptr */
			nf->size = ss;	/* store the size here */
			nf->fname = flist[i].fn;	/* keep actual name */
			nf->fstruct = NULL;
			nf->next = NULL;
		    }
	    } /* next size */
	} /* next font, f */
    } /* !appres.scalablefonts */
}

/* parse the point size of font 'name' */
/* e.g. -adobe-courier-bold-o-normal--10-100-75-75-m-60-ISO8859-1 */

static int
parsesize(char *name)
{
    int		    s;
    char	   *np;

    for (np = name; *(np + 1); np++)
	if (*np == '-' && *(np + 1) == '-')	/* look for the -- */
	    break;
    s = 0;
    if (*(np + 1)) {
	np += 2;		/* point past the -- */
	s = atoi(np);		/* get the point size */
    } else
	fprintf(stderr, "Can't parse '%s'\n", name);
    return s;
}

/*
 * Lookup an X font, "f" corresponding to a Postscript font style that is
 * close in size to "s"
 */

XFontStruct *
lookfont(int fnum, int size)
{
	XFontStruct    *fontst;
	char		fn[300],back_fn[300];
	char		template[300], *sub;
	Boolean		found;
	struct xfont   *newfont, *nf, *oldnf;

	if (fnum == DEFAULT)
	    fnum = 0;			/* pass back the -normal font font */
	if (size < 0)
	    size = DEF_FONTSIZE;	/* default font size */
	if (size < MIN_X_FONT_SIZE)
	    size = MIN_X_FONT_SIZE;	/* minimum allowable */
	else if (size > MAX_X_FONT_SIZE)
	    size = MAX_X_FONT_SIZE;	/* maximum allowable */

	/* if user asks, adjust for correct font size */
	if (appres.correct_font_size)
	    size = round(size*80.0/72.0);

	/* see if we've already loaded that font size 'size'
	   from the font family 'fnum' */

	found = False;

	/* start with the basic font name (e.g. adobe-times-medium-r-normal-...
		OR times-roman for OpenWindows fonts) */

	nf = x_fontinfo[fnum].xfontlist;
	oldnf = nf;
	if (nf != NULL) {
	    if (nf->size > size && !appres.scalablefonts)
		found = True;
	    else {
		while (nf != NULL) {
		    if (nf->size == size || (!appres.scalablefonts &&
			   (nf->size >= size && oldnf->size <= size))) {
			found = True;
			break;
		    }
		    oldnf = nf;
		    nf = nf->next;
		}
	    }
	}
	if (found) {		/* found exact size (or only larger available) */
	    strcpy(fn,nf->fname);  /* put the name in fn */
	    if (size < nf->size)
		put_msg("Font size %d not found, using larger %d point",size,nf->size);
	} else if (!appres.scalablefonts) {	/* not found, use largest available */
	    nf = oldnf;
	    strcpy(fn,nf->fname);  		/* put the name in fn */
	    if (size > nf->size)
		put_msg("Font size %d not found, using smaller %d point",size,nf->size);
	} else { /* scalablefonts; none yet of that size, alloc one and put it in the list */
	    newfont = (struct xfont *) malloc(sizeof(struct xfont));
	    /* add it on to the end of the list */
	    if (x_fontinfo[fnum].xfontlist == NULL)
	        x_fontinfo[fnum].xfontlist = newfont;
	    else
	        oldnf->next = newfont;
	    nf = newfont;		/* keep current ptr */
	    nf->size = size;		/* store the size here */
	    nf->fstruct = NULL;
	    nf->next = NULL;

	    if (openwinfonts) {
		/* OpenWindows fonts, create font name like times-roman-13 */
		sprintf(fn, "%s-%d", x_fontinfo[fnum].Template, size);
	    } else {
		/* X11 fonts, create a full XLFD font name */
		strcpy(template,x_fontinfo[fnum].Template);
		/* attach pointsize to font name */
		strcat(template,"%d-*-*-*-*-*-");
		/* add ISO8859 (if not Symbol font or ZapfDingbats) to font name */
		if (strstr(template,"ymbol") == NULL &&
		    strstr(template,"ingbats") == NULL)
			strcat(template,"ISO8859-*");
	        else
			strcat(template,"*-*");
		/* use the pixel field instead of points in the fontname so that the
		font scales with screen size */
		sprintf(fn, template, size);
		/* do same process with backup font name in case first doesn't exist */
		strcpy(template,x_backup_fontinfo[fnum].Template);
		strcat(template,"%d-*-*-*-*-*-");
		/* add ISO8859 (if not Symbol font or ZapfDingbats) to font name */
		if (strstr(template,"ymbol") == NULL &&
		    strstr(template,"ingbats") == NULL)
			strcat(template,"ISO8859-*");
	        else
			strcat(template,"*-*");
		sprintf(back_fn, template, size);
	    }
	    /* allocate space for the name and put it in the structure */
	    nf->fname = (char *) new_string(max2(strlen(fn),strlen(back_fn)));
	    strcpy(nf->fname, fn);
	    /* save the backup name too */
	    nf->bname = (char *) new_string(strlen(back_fn));
	    strcpy(nf->bname, back_fn);
	} /* scalable */

	if (nf->fstruct == NULL) {
	    if (appres.DEBUG)
		fprintf(stderr,"Loading font %s\n",fn);
	    /* if we are previewing a figure and the user pressed Cancel,
	       return now with the simple roman font */
	    if (check_cancel())
		return roman_font;
	    set_temp_cursor(wait_cursor);
	    fontst = XLoadQueryFont(tool_d, fn);
	    reset_cursor();
	    if (fontst == NULL) {
		/* doesn't exist, see if substituting "condensed" for "narrow" will match */
		if ((sub=strstr(fn,"-narrow-")) != NULL) {
		    strcpy(template, fn);
		    strcpy(&template[sub-fn],"-condensed-");
		    strcat(template, (sub+8));
		    /* try it */
		    fontst = XLoadQueryFont(tool_d, template);
		} else {
		    /* doesn't exist, try backup font name */
		    fontst = XLoadQueryFont(tool_d, nf->bname);
		    if (fontst) {
			/* use this name instead */
			strcpy(nf->fname, nf->bname);
		    } else {
			/* backup name doesn't exist, see if we can substitute "condensed" for "narrow" */
			if ((sub=strstr(nf->bname,"-narrow-")) != NULL) {
			    /* see if substituting "condensed" for "narrow" will match */
			    strcpy(template, nf->bname);
			    strcpy(&template[sub-nf->bname],"-condensed-");
			    strcat(template, (sub+8));
			    /* try it */
			    fontst = XLoadQueryFont(tool_d, template);
			}
		    }
		}
	    }
	    if (fontst == NULL) {
		if (fontst == NULL) {
		    /* even that font doesn't exist, use a plain one */
		    file_msg("Can't find %s, using %s", fn, appres.normalFont);
		    fontst = XLoadQueryFont(tool_d, appres.normalFont);
		    if (nf->fname)
			free(nf->fname);
		    /* allocate space for the name and put it in the structure */
		    nf->fname = (char *) new_string(strlen(appres.normalFont));
		    strcpy(nf->fname, appres.normalFont);  /* keep actual name */
		}
	    }
	    /* put the structure in the list */
	    nf->fstruct = fontst;
	} /* if (nf->fstruct == NULL) */

	return (nf->fstruct);
}

/* print "string" in window "w" using font specified in fstruct at angle
	"angle" (radians) at (x,y)
   If background is != COLOR_NONE, draw background color ala DrawImageString
*/

void
pw_text(Window w, int x, int y, int op, int depth, XFontStruct *fstruct,
	float angle, char *string, Color color, Color background)
{
    int		xfg, xbg;

    if (fstruct == NULL) {
	fprintf(stderr,"Error, in pw_text, fstruct==NULL\n");
	return;
    }

    /* if this depth is inactive, draw the text in gray */
    /* if depth == MAX_DEPTH+1 then the caller wants the original color no matter what */
    if (draw_parent_gray || (depth < MAX_DEPTH+1 && !active_layer(depth)))
	color = MED_GRAY;

    /* get the X colors */
    xfg = x_color(color);
    xbg = x_color(background);
    if ((xfg != gc_color[op]) ||
	(background != COLOR_NONE && (xbg != gc_background[op]))) {
	    /* don't change the colors for ERASE */
	    if (op == PAINT || op == INV_PAINT) {
		if (op == PAINT)
		    set_x_fg_color(gccache[op], color);
		else
		    XSetForeground(tool_d,gccache[op], xfg ^ x_bg_color.pixel);
		gc_color[op] = xfg;
		if (background != COLOR_NONE) {
		    set_x_bg_color(gccache[op], background);
		    gc_background[op] = xbg;
		}
	    }
    }

    /* check for preview cancel here.  The text call may take some time if
       a large font has to be rotated. */
    if (check_cancel())
	return ;

    if (background != COLOR_NONE) {
	zXRotDrawImageString(tool_d, fstruct, angle, w, gccache[op], x, y, string);
    } else {
	zXRotDrawString(tool_d, fstruct, angle, w, gccache[op], x, y, string);
    }
}

PR_SIZE
textsize(XFontStruct *fstruct, int n, char *s)
{
    PR_SIZE	    ret;
    int		    dir, asc, desc;
    XCharStruct	    overall;

#ifdef I18N
    if (appres.international) {
      extern i18n_text_extents();
      i18n_text_extents(fstruct, s, n, &dir, &asc, &desc, &overall);
      ret.length = ZOOM_FACTOR * overall.width;
      ret.ascent = ZOOM_FACTOR * overall.ascent;
      ret.descent = ZOOM_FACTOR * overall.descent;
      return (ret);
    }
#endif  /* I18N */
    XTextExtents(fstruct, s, n, &dir, &asc, &desc, &overall);
    ret.length = ZOOM_FACTOR * overall.width;
    ret.ascent = ZOOM_FACTOR * overall.ascent;
    ret.descent = ZOOM_FACTOR * overall.descent;
    return (ret);
}

/* LINES */

static int	gc_thickness[NUMOPS],
		gc_line_style[NUMOPS],
		gc_join_style[NUMOPS],
		gc_cap_style[NUMOPS];

GC
makegc(int op, Pixel fg, Pixel bg)
{
    register GC	    ngc;
    XGCValues	    gcv;
    unsigned long   gcmask;

    gcv.font = roman_font->fid;
    gcv.join_style = JoinMiter;
    gcv.cap_style = CapButt;
    gcmask = GCJoinStyle | GCCapStyle | GCFunction | GCForeground |
		GCBackground | GCFont;
    switch (op) {
      case PAINT:
	gcv.foreground = fg;
	gcv.background = bg;
	gcv.function = GXcopy;
	break;
      case ERASE:
	gcv.foreground = bg;
	gcv.background = bg;
	gcv.function = GXcopy;
	break;
      case INV_PAINT:
	gcv.foreground = fg ^ bg;
	gcv.background = bg;
	gcv.function = GXxor;
	break;
    }

    ngc = XCreateGC(tool_d, tool_w, gcmask, &gcv);
    return (ngc);
}

void init_gc(void)
{
    int		    i;
    XColor	    tmp_color;
    XGCValues	    gcv;

    gccache[PAINT] = makegc(PAINT, x_fg_color.pixel, x_bg_color.pixel);
    gccache[ERASE] = makegc(ERASE, x_fg_color.pixel, x_bg_color.pixel);
    gccache[INV_PAINT] = makegc(INV_PAINT, x_fg_color.pixel, x_bg_color.pixel);
    /* parse any grid color spec */
fprintf(stderr,"color = '%s'\n",appres.grid_color);
    XParseColor(tool_d, tool_cm, appres.grid_color, &tmp_color);
    if (XAllocColor(tool_d, tool_cm, &tmp_color)==0) {
	fprintf(stderr,"Can't allocate color for grid \n");
        grid_color = x_fg_color.pixel;
	grid_gc = makegc(PAINT, grid_color, x_bg_color.pixel);
    } else {
        grid_color = tmp_color.pixel;
	grid_gc = makegc(PAINT, grid_color, x_bg_color.pixel);
    }

    for (i = 0; i < NUMOPS; i++) {
	gc_color[i] = -1;
	gc_background[i] = -1;
	gc_thickness[i] = -1;
	gc_line_style[i] = -1;
	gc_join_style[i] = -1;
    }
    /* gc for page border and axis lines */
    border_gc = DefaultGC(tool_d, tool_sn);
    /* set the roman font for the message window */
    XSetFont(tool_d, border_gc, roman_font->fid);

    /* gc for picture pixmap rendering */
    pic_gc = XCreateGC(tool_d, DefaultRootWindow(tool_d), 0, NULL);

    /* make a gc for the command buttons */
    gcv.font = button_font->fid;
    button_gc = XCreateGC(tool_d, DefaultRootWindow(tool_d), GCFont, &gcv);
    /* copy the other components from the page border gc to the button_gc */
    XCopyGC(tool_d, border_gc, ~GCFont, button_gc);

}

/* create the gc's for fill style (PAINT and ERASE) */
/* the fill_pm[] must already be created */

void init_fill_gc(void)
{
    XGCValues	    gcv;
    int		    i;
    unsigned long   mask;

    gcv.fill_style = FillOpaqueStippled;
    gcv.arc_mode = ArcPieSlice; /* fill mode for arcs */
    gcv.fill_rule = EvenOddRule /* WindingRule */ ;
    for (i = 0; i < NUMFILLPATS; i++) {
	/* all the bits are recolored in set_fill_gc() */
	fill_gc[i] = makegc(PAINT, x_fg_color.pixel, x_color(BLACK));
	mask = GCFillStyle | GCFillRule | GCArcMode;
	if (fill_pm[i]) {
	    gcv.stipple = fill_pm[i];
	    mask |= GCStipple;
	}
	XChangeGC(tool_d, fill_gc[i], mask, &gcv);
    }
}

/* SHADING */

/* grey images for fill patterns (32x32) */
unsigned char shade_images[NUMSHADEPATS][128] = {
 {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
 {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,
 0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x08,
 0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,
 0x88,0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x88,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x08,0x08,0x08,0x08},
 {0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x44,0x00,0x00,0x00,
 0x00,0x01,0x11,0x01,0x11,0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x44,0x00,0x00,
 0x00,0x00,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x44,0x00,
 0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x44,
 0x00,0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x44,0x44,0x44,
 0x44,0x00,0x00,0x00,0x00,0x01,0x11,0x01,0x11,0x00,0x00,0x00,0x00,0x44,0x44,
 0x44,0x44,0x00,0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x44,
 0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
 0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00},
 {0x00,0x00,0x00,0x00,0x11,0x51,0x11,0x51,0x00,0x00,0x00,0x00,0x44,0x44,0x44,
 0x44,0x00,0x00,0x00,0x00,0x15,0x15,0x15,0x15,0x00,0x00,0x00,0x00,0x44,0x44,
 0x44,0x44,0x00,0x00,0x00,0x00,0x51,0x11,0x51,0x11,0x00,0x00,0x00,0x00,0x44,
 0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x15,0x15,0x15,0x15,0x00,0x00,0x00,0x00,
 0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x11,0x51,0x11,0x51,0x00,0x00,0x00,
 0x00,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x15,0x15,0x15,0x15,0x00,0x00,
 0x00,0x00,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x51,0x11,0x51,0x11,0x00,
 0x00,0x00,0x00,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00,0x15,0x15,0x15,0x15,
 0x00,0x00,0x00,0x00,0x44,0x44,0x44,0x44},
 {0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x00,0x00,0x8a,0x88,0x8a,
 0x88,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x00,0x00,0x88,0x88,
 0x88,0x88,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x00,0x00,0x8a,
 0x8a,0x8a,0x8a,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x00,0x00,
 0x88,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x00,
 0x00,0x8a,0x88,0x8a,0x88,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,0x00,
 0x00,0x00,0x88,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,0x00,
 0x00,0x00,0x00,0x8a,0x8a,0x8a,0x8a,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,
 0x00,0x00,0x00,0x00,0x88,0x88,0x88,0x88},
 {0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,
 0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,
 0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,
 0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,
 0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,
 0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,
 0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,
 0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,
 0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00},
 {0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x88,0x88,0x88,
 0x88,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x80,0x80,
 0x80,0x80,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x88,
 0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,
 0x88,0x80,0x88,0x80,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,0x55,
 0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,0x55,
 0x55,0x55,0x80,0x80,0x80,0x80,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,0x55,
 0x55,0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x00,0x00,0x00,0x00,
 0x55,0x55,0x55,0x55,0x88,0x80,0x88,0x80},
 {0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x80,0x80,0x80,0x80,0x55,0x55,0x55,
 0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x88,0x08,0x88,0x08,0x55,0x55,
 0x55,0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x80,0x80,0x80,0x80,0x55,
 0x55,0x55,0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x08,0x08,0x08,0x08,
 0x55,0x55,0x55,0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x80,0x80,0x80,
 0x80,0x55,0x55,0x55,0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x88,0x08,
 0x88,0x08,0x55,0x55,0x55,0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,0x80,
 0x80,0x80,0x80,0x55,0x55,0x55,0x55,0x22,0x22,0x22,0x22,0x55,0x55,0x55,0x55,
 0x08,0x08,0x08,0x08,0x55,0x55,0x55,0x55},
 {0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x22,0xa2,0x22,0xa2,0x55,0x55,0x55,
 0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x2a,0x2a,0x2a,0x2a,0x55,0x55,
 0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0xa2,0x22,0xa2,0x22,0x55,
 0x55,0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x2a,0x2a,0x2a,0x2a,
 0x55,0x55,0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x22,0xa2,0x22,
 0xa2,0x55,0x55,0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0x2a,0x2a,
 0x2a,0x2a,0x55,0x55,0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,0xa2,
 0x22,0xa2,0x22,0x55,0x55,0x55,0x55,0x88,0x88,0x88,0x88,0x55,0x55,0x55,0x55,
 0x2a,0x2a,0x2a,0x2a,0x55,0x55,0x55,0x55},
 {0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x54,0x54,0x54,
 0x54,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x44,0x44,
 0x44,0x44,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x44,
 0x54,0x44,0x54,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,
 0x44,0x44,0x44,0x44,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,
 0xaa,0x54,0x54,0x54,0x54,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,
 0xaa,0xaa,0x44,0x44,0x44,0x44,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,
 0xaa,0xaa,0xaa,0x44,0x54,0x44,0x54,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,
 0xaa,0xaa,0xaa,0xaa,0x44,0x44,0x44,0x44},
 {0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,
 0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,
 0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,
 0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,
 0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,
 0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,
 0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,
 0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,
 0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa},
 {0xdd,0xdd,0xdd,0xdd,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,
 0xaa,0xd5,0xd5,0xd5,0xd5,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,0xaa,
 0xaa,0xaa,0xdd,0xdd,0xdd,0xdd,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,0xaa,
 0xaa,0xaa,0xaa,0xdd,0xd5,0xdd,0xd5,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,0x55,
 0xaa,0xaa,0xaa,0xaa,0xdd,0xdd,0xdd,0xdd,0xaa,0xaa,0xaa,0xaa,0x55,0x55,0x55,
 0x55,0xaa,0xaa,0xaa,0xaa,0xd5,0xd5,0xd5,0xd5,0xaa,0xaa,0xaa,0xaa,0x55,0x55,
 0x55,0x55,0xaa,0xaa,0xaa,0xaa,0xdd,0xdd,0xdd,0xdd,0xaa,0xaa,0xaa,0xaa,0x55,
 0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa,0xdd,0xd5,0xdd,0xd5,0xaa,0xaa,0xaa,0xaa,
 0x55,0x55,0x55,0x55,0xaa,0xaa,0xaa,0xaa},
 {0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xd5,0xd5,0xd5,0xd5,0xaa,0xaa,0xaa,
 0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xdd,0x5d,0xdd,0x5d,0xaa,0xaa,
 0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xd5,0xd5,0xd5,0xd5,0xaa,
 0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0x5d,0xdd,0x5d,0xdd,
 0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xd5,0xd5,0xd5,
 0xd5,0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xdd,0x5d,
 0xdd,0x5d,0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xd5,
 0xd5,0xd5,0xd5,0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,
 0x5d,0xdd,0x5d,0xdd,0xaa,0xaa,0xaa,0xaa},
 {0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,0x55,0x55,0x55,0xfe,0xfe,0xfe,
 0xfe,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,0x55,0x55,0x55,0xee,0xef,
 0xee,0xef,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,0x55,0x55,0x55,0xfe,
 0xfe,0xfe,0xfe,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,0x55,0x55,0x55,
 0xef,0xef,0xef,0xef,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,0x55,0x55,
 0x55,0xfe,0xfe,0xfe,0xfe,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,0x55,
 0x55,0x55,0xee,0xef,0xee,0xef,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,0x55,
 0x55,0x55,0x55,0xfe,0xfe,0xfe,0xfe,0x55,0x55,0x55,0x55,0xbb,0xbb,0xbb,0xbb,
 0x55,0x55,0x55,0x55,0xef,0xef,0xef,0xef},
 {0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,0xaa,0xaa,
 0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x77,0x7f,0x77,0x7f,0xaa,0xaa,
 0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,0x77,0xaa,
 0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x7f,0x7f,0x7f,0x7f,
 0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x77,0x77,0x77,
 0x77,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x77,0x7f,
 0x77,0x7f,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0x77,
 0x77,0x77,0x77,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,
 0x7f,0x7f,0x7f,0x7f,0xaa,0xaa,0xaa,0xaa},
 {0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,
 0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,
 0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,
 0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,
 0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,
 0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,
 0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,
 0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa,
 0xff,0xff,0xff,0xff,0xaa,0xaa,0xaa,0xaa},
 {0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0xdd,0xdd,0xdd,0xdd,0xff,0xff,0xff,
 0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0x5d,0xdd,0x5d,0xdd,0xff,0xff,
 0xff,0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0xdd,0xdd,0xdd,0xdd,0xff,
 0xff,0xff,0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0x5d,0x5d,0x5d,0x5d,
 0xff,0xff,0xff,0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0xdd,0xdd,0xdd,
 0xdd,0xff,0xff,0xff,0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0x5d,0xdd,
 0x5d,0xdd,0xff,0xff,0xff,0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,0xdd,
 0xdd,0xdd,0xdd,0xff,0xff,0xff,0xff,0x55,0x55,0x55,0x55,0xff,0xff,0xff,0xff,
 0x5d,0x5d,0x5d,0x5d,0xff,0xff,0xff,0xff},
 {0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xbb,0xba,0xba,0xba,0xff,0xff,0xff,
 0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xab,0xbb,0xab,0xbb,0xff,0xff,
 0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xbb,0xba,0xba,0xba,0xff,
 0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xbb,0xab,0xbb,0xab,
 0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xbb,0xba,0xba,
 0xba,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xab,0xbb,
 0xab,0xbb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xbb,
 0xba,0xba,0xba,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,
 0xbb,0xab,0xbb,0xab,0xff,0xff,0xff,0xff},
 {0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xfb,0xfb,0xfb,
 0xfb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xbf,0xbb,
 0xbf,0xbb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,0xfb,
 0xfb,0xfb,0xfb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,0xff,
 0xbf,0xbf,0xbf,0xbf,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,0xff,
 0xff,0xfb,0xfb,0xfb,0xfb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,0xff,
 0xff,0xff,0xbf,0xbb,0xbf,0xbb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,0xff,
 0xff,0xff,0xff,0xfb,0xfb,0xfb,0xfb,0xff,0xff,0xff,0xff,0xee,0xee,0xee,0xee,
 0xff,0xff,0xff,0xff,0xbf,0xbf,0xbf,0xbf},
 {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xbb,0xbb,0xbb,
 0xbb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfb,0xfb,
 0xfb,0xfb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xbb,
 0xbb,0xbb,0xbb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xfb,0xfb,0xfb,0xfb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xbb,0xbb,0xbb,0xbb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xfb,0xfb,0xfb,0xfb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xbb,0xbb,0xbb,0xbb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xfb,0xfb,0xfb,0xfb},
 {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
};

/* initial data for patterns */

/* 30 degrees left diagonal */
#define left30_width 8
#define left30_height 4
static unsigned char left30_bits[] = {
   0x03, 0x0c, 0x30, 0xc0};
/* 30 degrees right diagonal */
#define right30_width 8
#define right30_height 4
static unsigned char right30_bits[] = {
   0xc0, 0x30, 0x0c, 0x03};
/* 30 degrees crosshatch */
#define crosshatch30_width 8
#define crosshatch30_height 4
static unsigned char crosshatch30_bits[] = {
   0x81, 0x66, 0x18, 0x66};
/* 45 degrees left diagonal */
#define left45_width 8
#define left45_height 8
static unsigned char left45_bits[] = {
   0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
/* 45 degrees right diagonal */
#define right45_width 8
#define right45_height 8
static unsigned char right45_bits[] = {
   0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
/* 45 degrees crosshatch */
#define crosshatch45_width 8
#define crosshatch45_height 8
static unsigned char crosshatch45_bits[] = {
   0x11, 0x0a, 0x04, 0x0a, 0x11, 0xa0, 0x40, 0xa0};
/* horizontal bricks */
#define bricks_width 16
#define bricks_height 16
static unsigned char bricks_bits[] = {
   0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
   0x00, 0x80, 0xff, 0xff, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xff, 0xff};
/* vertical bricks */
#define vert_bricks_width 16
#define vert_bricks_height 16
static unsigned char vert_bricks_bits[] = {
   0xff, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x80, 0x80, 0x80, 0x80, 0xff, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
/* horizontal lines */
#define horizontal_width 8
#define horizontal_height 4
static unsigned char horizontal_bits[] = {
   0xff, 0x00, 0x00, 0x00};
/* vertical lines */
#define vertical_width 8
#define vertical_height 8
static unsigned char vertical_bits[] = {
   0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88};
/* crosshatch */
#define crosshatch_width 8
#define crosshatch_height 4
static unsigned char crosshatch_bits[] = {
   0xff, 0x88, 0x88, 0x88};
/* left-pointing shingles */
#define leftshingle_width 24
#define leftshingle_height 24
static unsigned char leftshingle_bits[] = {
   0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40,
   0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0xff, 0xff, 0xff,
   0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00,
   0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0x00, 0x00, 0xff, 0xff, 0xff,
   0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00,
   0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0x00, 0xff, 0xff, 0xff};
/* right-pointing shingles */
#define rightshingle_width 24
#define rightshingle_height 24
static unsigned char rightshingle_bits[] = {
   0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20,
   0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff,
   0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00,
   0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff, 0xff,
   0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00,
   0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff};
/* vertical left-pointing shingles */
#define vert_leftshingle_width 24
#define vert_leftshingle_height 24
static unsigned char vert_leftshingle_bits[] = {
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x80, 0x81, 0x80, 0x80, 0x86, 0x80, 0x80, 0x98, 0x80, 0x80, 0xe0,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x81, 0x80, 0x80, 0x86, 0x80, 0x80, 0x98, 0x80, 0x80, 0xe0, 0x80,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x81, 0x80, 0x80, 0x86, 0x80, 0x80, 0x98, 0x80, 0x80, 0xe0, 0x80, 0x80};
/* vertical right-pointing shingles */
#define vert_rightshingle_width 24
#define vert_rightshingle_height 24
static unsigned char vert_rightshingle_bits[] = {
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0xc0, 0x80, 0x80, 0xb0, 0x80, 0x80, 0x8c, 0x80, 0x80, 0x83, 0x80,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0x80, 0x80, 0xc0, 0x80, 0x80, 0xb0, 0x80, 0x80, 0x8c, 0x80, 0x80, 0x83,
   0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
   0xc0, 0x80, 0x80, 0xb0, 0x80, 0x80, 0x8c, 0x80, 0x80, 0x83, 0x80, 0x80};
/* fish scales */
#define fishscales_width 16
#define fishscales_height 8
static unsigned char fishscales_bits[] = {
   0x40, 0x02, 0x30, 0x0c, 0x0e, 0x70, 0x01, 0x80, 0x02, 0x40, 0x0c, 0x30,
   0x70, 0x0e, 0x80, 0x01};
/* small fish scales */
#define small_fishscales_width 8
#define small_fishscales_height 8
static unsigned char small_fishscales_bits[] = {
   0x01, 0x01, 0x82, 0x6c, 0x10, 0x10, 0x28, 0xc6};
/* circles */
#define circles_width 16
#define circles_height 16
static unsigned char circles_bits[] = {
   0xe0, 0x0f, 0x18, 0x30, 0x04, 0x40, 0x02, 0x80, 0x02, 0x80, 0x01, 0x00,
   0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
   0x02, 0x80, 0x02, 0x80, 0x04, 0x40, 0x18, 0x30};
/* hexagons */
#define hexagons_width 30
#define hexagons_height 18
static unsigned char hexagons_bits[] = {
   0x08, 0x80, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00,
   0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
   0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0xf8, 0x3f,
   0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02, 0x00, 0x02, 0x00,
   0x02, 0x00, 0x02, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x01, 0x00,
   0x08, 0x80, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00};
/* octagons */
#define octagons_width 16
#define octagons_height 16
static unsigned char octagons_bits[] = {
   0xe0, 0x0f, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01, 0x00,
   0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
   0x02, 0x80, 0x04, 0x40, 0x08, 0x20, 0x10, 0x10};
/* horizontal sawtooth */
#define horiz_saw_width 16
#define horiz_saw_height 8
static unsigned char horiz_saw_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x14, 0x14, 0x22, 0x22,
   0x41, 0x41, 0x80, 0x80};
/* vertical sawtooth */
#define vert_saw_width 8
#define vert_saw_height 16
static unsigned char vert_saw_bits[] = {
   0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x10,
   0x08, 0x04, 0x02, 0x01};

/* patterns like bricks, etc */
patrn_strct pattern_images[NUMPATTERNS] = {
    {left30_width,            left30_height,            (char*)left30_bits},
    {right30_width,           right30_height,           (char*)right30_bits},
    {crosshatch30_width,      crosshatch30_height,      (char*)crosshatch30_bits},
    {left45_width,            left45_height,            (char*)left45_bits},
    {right45_width,           right45_height,           (char*)right45_bits},
    {crosshatch45_width,      crosshatch45_height,      (char*)crosshatch45_bits},
    {bricks_width,            bricks_height,            (char*)bricks_bits},
    {vert_bricks_width,       vert_bricks_height,       (char*)vert_bricks_bits},
    {horizontal_width,        horizontal_height,        (char*)horizontal_bits},
    {vertical_width,          vertical_height,          (char*)vertical_bits},
    {crosshatch_width,        crosshatch_height,        (char*)crosshatch_bits},
    {leftshingle_width,       leftshingle_height,       (char*)leftshingle_bits},
    {rightshingle_width,      rightshingle_height,      (char*)rightshingle_bits},
    {vert_leftshingle_width,  vert_leftshingle_height,  (char*)vert_leftshingle_bits},
    {vert_rightshingle_width, vert_rightshingle_height, (char*)vert_rightshingle_bits},
    {fishscales_width,        fishscales_height,        (char*)fishscales_bits},
    {small_fishscales_width,  small_fishscales_height,  (char*)small_fishscales_bits},
    {circles_width,           circles_height,           (char*)circles_bits},
    {hexagons_width,          hexagons_height,          (char*)hexagons_bits},
    {octagons_width,          octagons_height,          (char*)octagons_bits},
    {horiz_saw_width,         horiz_saw_height,         (char*)horiz_saw_bits},
    {vert_saw_width,          vert_saw_height,          (char*)vert_saw_bits}
};

/* generate the fill pixmaps */

void init_fill_pm(void)
{
    int		    i,j;

    for (i = 0; i <= NUMFILLPATS; i++) {
	fillstyle_choices[i].value = i;
	fillstyle_choices[i].icon = &none_ic;
    }

    /**********************************************************************************/
    /* NOTE:  All fillstyle_choices pixmaps will be recolored in recolor_fillstyles() */
    /**********************************************************************************/

    /* use same colors for "NONE" indicator for black and color */
    fillstyle_choices[0].pixmap = XCreatePixmapFromBitmapData(tool_d,
			tool_w, none_ic.bits, none_ic.width,
			none_ic.height, x_fg_color.pixel, x_bg_color.pixel,
			tool_dpth);

    /* Shade patterns go from full black to full saturation of the color */
    for (i = 0; i < NUMSHADEPATS; i++) {
	fill_pm[i] = XCreateBitmapFromData(tool_d, tool_w,
				   (char*)shade_images[i], SHADE_IM_SIZE, SHADE_IM_SIZE);
	/* create fill style pixmaps for indicator button */
	/* The actual colors of fg/bg will be reset in recolor_fillstyles */
	fillstyle_choices[i + 1].pixmap = XCreatePixmapFromBitmapData(tool_d,
		 tool_w, (char*)shade_images[i], SHADE_IM_SIZE, SHADE_IM_SIZE,
		 x_fg_color.pixel,x_bg_color.pixel,tool_dpth);
    }
    /* Tint patterns go from full saturation of the color to full white */
    /* Note that there are no fillstyle_choices for tints for black */
    for (i = NUMSHADEPATS; i < NUMSHADEPATS+NUMTINTPATS; i++) {
	j = NUMSHADEPATS+NUMTINTPATS-i-1;	/* reverse the patterns */
	fill_pm[i] = XCreateBitmapFromData(tool_d, tool_w,
				   (char*)shade_images[j], SHADE_IM_SIZE, SHADE_IM_SIZE);
	/* create fill style pixmaps for indicator button */
	/* The actual colors of fg/bg will be reset in recolor_fillstyles */
	fillstyle_choices[i + 1].pixmap = XCreatePixmapFromBitmapData(tool_d,
		 tool_w, (char*)shade_images[j], SHADE_IM_SIZE, SHADE_IM_SIZE,
		 x_fg_color.pixel,x_bg_color.pixel,tool_dpth);
    }
    /* Now do the remaining patterns (bricks, shingles, etc) */
    for (i = NUMSHADEPATS+NUMTINTPATS; i < NUMFILLPATS; i++) {
	/* create pattern at this zoom */
	rescale_pattern(i);
	j = i-(NUMSHADEPATS+NUMTINTPATS);
	/* save these patterns at zoom = 1 for the fill button panel */
	fill_but_pm[j] = fill_pm[i];
	fill_but_pm_zoom[j] = fill_pm_zoom[i];
	/* to force new pixmaps for canvas */
	fill_pm[i] = (Pixmap) 0;
	/* and create another one */
	rescale_pattern(i);
	/* create fill style pixmaps for indicator button */
	/* The actual colors of fg/bg will be reset in recolor_fillstyles */
	fillstyle_choices[i + 1].pixmap = XCreatePixmapFromBitmapData(tool_d,
		 tool_w, pattern_images[j].odata,
		 pattern_images[j].owidth, pattern_images[j].oheight,
		 x_fg_color.pixel,x_bg_color.pixel,tool_dpth);
    }
}

void
pw_vector(Window w, int x1, int y1, int x2, int y2, int op,
	  int line_width, int line_style, float style_val, Color color)
{
    if (line_width == 0)
	return;
    set_line_stuff(line_width, line_style, style_val, JOIN_MITER, CAP_BUTT, op, color);
    if (line_style == PANEL_LINE)
	XDrawLine(tool_d, w, gccache[op], x1, y1, x2, y2);
    else
	zXDrawLine(tool_d, w, gccache[op], x1, y1, x2, y2);
}

void
pw_curve(Window w, int xstart, int ystart, int xend, int yend,
	 int op, int depth, int linewidth, int style, float style_val, int fill_style,
	 Color pen_color, Color fill_color, int cap_style)
{
    int		    xmin, ymin;
    unsigned int    wd, ht;

    /* if this depth is inactive, draw the curve and any fill in gray */
    /* if depth == MAX_DEPTH+1 then the caller wants the original color no matter what */
    if (draw_parent_gray || (depth < MAX_DEPTH+1 && !active_layer(depth))) {
	pen_color = MED_GRAY;
	fill_color = LT_GRAY;
    }

    xmin = min2(xstart, xend);
    ymin = min2(ystart, yend);
    wd = (unsigned int) abs(xstart - xend);
    ht = (unsigned int) abs(ystart - yend);

    /* if it's a fill pat we know about */
    if (fill_style >= 0 && fill_style < NUMFILLPATS) {
	set_fill_gc(fill_style, op, pen_color, fill_color, xstart, ystart);
	zXFillArc(tool_d, w, fillgc, xmin, ymin, wd, ht, 0, 360 * 64);
    }
    if (linewidth == 0)
	return;
    if (op == ERASE) {
	/* kludge - to speed things up we erase with thick solid lines */
	set_line_stuff(linewidth + 3, SOLID_LINE, 0.0, JOIN_MITER,
			cap_style, op, pen_color);
	zXDrawArc(tool_d, w, gccache[op], xmin, ymin, wd, ht, 0, 360 * 64);
    } else {
	set_line_stuff(linewidth, style, style_val, JOIN_MITER,
			cap_style, op, pen_color);
	zXDrawArc(tool_d, w, gccache[op], xmin, ymin, wd, ht, 0, 360 * 64);
    }
}

/* a point object - actually draw a line from (x-line_width/2,y) to (x+linewidth/2,y)
	so that we get some thickness */

void
pw_point(Window w, int x, int y, int op, int depth, int line_width,
	 Color color, int cap_style)
{
    int		    hf_wid;

    /* if this depth is inactive, draw the point in gray */
    if (draw_parent_gray || !active_layer(depth))
	color = MED_GRAY;

    /* pw_point doesn't use line_style or fill_style but needs color */
    set_line_stuff(line_width, SOLID_LINE, 0.0, JOIN_MITER, cap_style,
		op, color);
    if (cap_style > 0)
	hf_wid = 0;
    else
	hf_wid = (int)(ZOOM_FACTOR*line_width/2);
    /* add one to the right if the line_width is odd */
    zXDrawLine(tool_d, w, gccache[op], x-hf_wid, y, x+hf_wid+(line_width%2), y);
}

void
pw_arcbox(Window w, int xmin, int ymin, int xmax, int ymax, int radius,
	  int op, int depth, int line_width, int line_style,
	  float style_val, int fill_style, Color pen_color, Color fill_color)
{
    GC		    gc;
    int		    diam = 2 * radius;

    /* if this depth is inactive, draw the arcbox in gray */
    if (draw_parent_gray || (depth < MAX_DEPTH+1 && !active_layer(depth))) {
	pen_color = MED_GRAY;
	fill_color = LT_GRAY;
    }

    /* if it's a fill pat we know about */
    if (fill_style >= 0 && fill_style < NUMFILLPATS) {
	set_fill_gc(fill_style, op, pen_color, fill_color, xmin, ymin);
	/* upper left */
	zXFillArc(tool_d, w, fillgc, xmin, ymin, diam, diam, 90 * 64, 90 * 64);
	/* lower left */
	zXFillArc(tool_d, w, fillgc, xmin, ymax - diam, diam, diam, 180 * 64, 90 * 64);
	/* lower right */
	zXFillArc(tool_d, w, fillgc, xmax - diam, ymax - diam, diam, diam, 270 * 64, 90 * 64);
	/* upper right */
	zXFillArc(tool_d, w, fillgc, xmax - diam, ymin, diam, diam, 0 * 64, 90 * 64);
	/* fill strip on left side between upper and lower arcs */
	if (ymax - ymin - diam > 0)
	    zXFillRectangle(tool_d, w, fillgc, xmin, ymin + radius, radius,
			    ymax - ymin - diam + 1);
	/* fill middle section */
	if (xmax - xmin - diam > 0)
	    zXFillRectangle(tool_d, w, fillgc, xmin + radius, ymin,
			    xmax - xmin - diam + 1, ymax - ymin + 1);
	/* fill strip on right side between upper and lower arcs */
	if (ymax - ymin - diam > 0)
	    zXFillRectangle(tool_d, w, fillgc, xmax - radius, ymin + radius,
			    radius, ymax - ymin - diam + 1);
    }
    if (line_width == 0)
	return;

    set_line_stuff(line_width, line_style, style_val, JOIN_MITER, CAP_BUTT,
		op, pen_color);
    gc = gccache[op];
    /* now draw the edges and arc corners */
    zXDrawArc(tool_d, w, gc, xmin, ymin, diam, diam, 90 * 64, 90 * 64);
    zXDrawLine(tool_d, w, gc, xmin, ymin + radius, xmin, ymax - radius + 1);
    zXDrawArc(tool_d, w, gc, xmin, ymax - diam, diam, diam, 180 * 64, 90 * 64);
    zXDrawLine(tool_d, w, gc, xmin + radius, ymax, xmax - radius + 1, ymax);
    zXDrawArc(tool_d, w, gc, xmax - diam, ymax - diam, diam, diam, 270 * 64, 90 * 64);
    zXDrawLine(tool_d, w, gc, xmax, ymax - radius, xmax, ymin + radius - 1);
    zXDrawArc(tool_d, w, gc, xmax - diam, ymin, diam, diam, 0 * 64, 90 * 64);
    zXDrawLine(tool_d, w, gc, xmax - radius, ymin, xmin + radius - 1, ymin);
}

void
pw_lines(Window w, zXPoint *points, int npoints, int op, int depth,
	 int line_width, int line_style, float style_val,
	 int join_style, int cap_style, int fill_style,
	 Color pen_color, Color fill_color)
{
    register int i;
    register XPoint *p;

    /* if this depth is inactive, draw the line in gray */
    if (draw_parent_gray || (depth < MAX_DEPTH+1 && !active_layer(depth))) {
	pen_color = MED_GRAY;
	fill_color = LT_GRAY;
    }

    /* if the line has only one point or it has two points and those points are
       coincident AND we are drawing a DOTTED line, this kills Xsun and hangs
       other servers.
       We will just call pw_point since it is only a point anyway */

    if ((npoints == 1) ||
	(npoints == 2 && points[0].x == points[1].x && points[0].y == points[1].y)) {
	    pw_point(w, points[0].x, points[0].y, op, depth, line_width, pen_color, cap_style);
	    return;
    }
	
    if (line_style == PANEL_LINE) {
	/* must use XPoint, not our zXPoint */
	p = (XPoint *) malloc(npoints * sizeof(XPoint));
	for (i=0; i<npoints; i++) {
	    p[i].x = (short) points[i].x;
	    p[i].y = (short) points[i].y;
	}
    }

    /* if it's a fill pat we know about, find upper-left corner for pattern origin */
    if (fill_style >= 0 && fill_style < NUMFILLPATS) {
	int xmin=100000, ymin=100000, i;
	if (fill_style >= NUMTINTPATS+NUMSHADEPATS) {
	    for (i=0; i<npoints; i++) {
		xmin = min2(xmin,points[i].x);
		ymin = min2(ymin,points[i].y);
	    }
	}
	set_fill_gc(fill_style, op, pen_color, fill_color, xmin, ymin);
	if (line_style == PANEL_LINE) {
	    XFillPolygon(tool_d, w, fillgc, p, npoints,
			 Complex, CoordModeOrigin);
	} else {
	    zXFillPolygon(tool_d, w, fillgc, points, npoints,
			  Complex, CoordModeOrigin);
	}
    }
    if (line_width == 0)
	return;
    set_line_stuff(line_width, line_style, style_val, join_style, cap_style,
			op, pen_color);
    if (line_style == PANEL_LINE) {
	XDrawLines(tool_d, w, gccache[op], p, npoints, CoordModeOrigin);
	free((char *) p);
    } else {
	zXDrawLines(tool_d, w, gccache[op], points, npoints, CoordModeOrigin);
    }
}

void set_clip_window(int xmin, int ymin, int xmax, int ymax)
{
    clip_xmin = clip[0].x = xmin;
    clip_ymin = clip[0].y = ymin;
    clip_xmax = xmax;
    clip_ymax = ymax;
    clip_width = clip[0].width = xmax - xmin + 1;
    clip_height = clip[0].height = ymax - ymin + 1;
    XSetClipRectangles(tool_d, border_gc, 0, 0, clip, 1, YXBanded);
    XSetClipRectangles(tool_d, gccache[PAINT], 0, 0, clip, 1, YXBanded);
    XSetClipRectangles(tool_d, gccache[INV_PAINT], 0, 0, clip, 1, YXBanded);
    XSetClipRectangles(tool_d, gccache[ERASE], 0, 0, clip, 1, YXBanded);
}

void set_zoomed_clip_window(int xmin, int ymin, int xmax, int ymax)
{
    set_clip_window(ZOOMX(xmin), ZOOMY(ymin), ZOOMX(xmax), ZOOMY(ymax));
}

void reset_clip_window(void)
{
    set_clip_window(0, 0, CANVAS_WD, CANVAS_HT);
}

void set_fill_gc(int fill_style, int op, int pencolor, int fillcolor, int xorg, int yorg)
{
    Color	    fg, bg;

    /* see if we need to create this fill style if it is a pattern.
       This might have happened if there was a change of zoom. */

    if ((fill_style >= NUMSHADEPATS+NUMTINTPATS) &&
	((fill_pm[fill_style] == 0) || (fill_pm_zoom[fill_style] != display_zoomscale)))
	    rescale_pattern(fill_style);
    fillgc = fill_gc[fill_style];
    if (op != ERASE) {
	/* if a pattern, color the lines in the pen color and the field in fill color */
	if (fill_style >= NUMSHADEPATS+NUMTINTPATS) {
	    fg = x_color(pencolor);
	    bg = x_color(fillcolor);
	} else {
	    if (fillcolor == BLACK) {
		fg = x_color(BLACK);
		bg = x_color(WHITE);
	    } else if (fillcolor == DEFAULT) {
		fg = x_fg_color.pixel;
		bg = x_bg_color.pixel;
	    } else {
		fg = x_color(fillcolor);
		bg = (fill_style < NUMSHADEPATS? x_color(BLACK): x_color(WHITE));
	    }
	}
    } else {
	fg = x_bg_color.pixel;   /* un-fill */
	bg = x_bg_color.pixel;
    }
    XSetForeground(tool_d,fillgc,fg);
    XSetBackground(tool_d,fillgc,bg);
    /* set stipple from the fill_pm array */
    XSetStipple(tool_d, fillgc, fill_pm[fill_style]);
    /* set origin of pattern relative to object itself */
    XSetTSOrigin(tool_d, fillgc, ZOOMX(xorg), ZOOMY(yorg));
    XSetClipRectangles(tool_d, fillgc, 0, 0, clip, 1, YXBanded);
}


static unsigned char dash_list[16][2] = {{255, 255}, {255, 255},
					{255, 255}, {255, 255},
					{255, 255}, {255, 255},
					{255, 255}, {255, 255},
					{255, 255}, {255, 255},
					{255, 255}, {255, 255},
					{255, 255}, {255, 255},
					{255, 255}, {255, 255}};

static int join_styles[3] = { JoinMiter, JoinRound, JoinBevel };
static int cap_styles[3] = { CapButt, CapRound, CapProjecting };

static int ndash_dot = 4;
static float dash_dot[4] = { 1., 0.5, 0., 0.5 };
static int ndash_2dots = 6;
static float dash_2dots[6] = { 1., 0.45, 0., 0.333, 0., 0.45 };
static int ndash_3dots = 8;
static float dash_3dots[8] = { 1., 0.4, 0., 0.3, 0., 0.3, 0., 0.4 };


void set_line_stuff(int width, int style, float style_val, int join_style, int cap_style, int op, int color)
{
    XGCValues	    gcv;
    unsigned long   mask;

    switch (style) {
      case RUBBER_LINE:
	width = 0;
	break;
      case PANEL_LINE:
	break;
      default:
	width = round(display_zoomscale * width);
	break;
    }

    /* user zero-width lines for speed with SOLID lines */
    /* can't do this for dashed lines because server isn't */
    /* required to draw dashes for zero-width lines */
    if (width == 1 && style == SOLID_LINE)
	width = 0;
    /* conversely, if the width is calculated to 0 and this is a dashed line, make width 1 */
    if (width == 0 && style != SOLID_LINE)
	width = 1;

    /* see if all gc stuff is already correct */

    if (width == gc_thickness[op] && style == gc_line_style[op] &&
	join_style == gc_join_style[op] &&
	cap_style == gc_cap_style[op] &&
	(x_color(color) == gc_color[op]) &&
	((style != DASH_LINE && style != DOTTED_LINE &&
          style != DASH_DOT_LINE && style != DASH_2_DOTS_LINE &&
          style != DASH_3_DOTS_LINE) ||
	 dash_list[op][1] == (unsigned char) round(style_val * display_zoomscale)))
	    return;			/* no need to change anything */

    gcv.line_width = width;
    mask = GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle;
    if (op == PAINT) {
	gcv.foreground = x_color(color);
	mask |= GCForeground;
    } else if (op == INV_PAINT) {
	gcv.foreground = x_color(color) ^ x_bg_color.pixel;
	mask |= GCForeground;
    }
    gcv.join_style = join_styles[join_style];
    gcv.cap_style = cap_styles[cap_style];
    gcv.line_style = (style == DASH_LINE || style == DOTTED_LINE ||
             style == DASH_DOT_LINE || style == DASH_2_DOTS_LINE ||
             style == DASH_3_DOTS_LINE) ?
	LineOnOffDash : LineSolid;

    XChangeGC(tool_d, gccache[op], mask, &gcv);
    if (style_val > 0.0) {	/* style_val of 0.0 causes problems */
	if (style == DASH_LINE || style == DOTTED_LINE) {
	    /* length of ON/OFF pixels */
	    if (style_val * display_zoomscale > 255.0)
		dash_list[op][0] = dash_list[op][1] = (char) 255;	/* too large for X! */
	    else
	        dash_list[op][0] = dash_list[op][1] = 
				(char) round(style_val * display_zoomscale);
	    /* length of ON pixels for dotted */
	    if (style == DOTTED_LINE)
		dash_list[op][0] = (char)display_zoomscale;

	    if (dash_list[op][0]==0)		/* take care for rounding to zero ! */
		dash_list[op][0]=1;
	    if (dash_list[op][1]==0)		/* take care for rounding to zero ! */
		dash_list[op][1]=1;
	    XSetDashes(tool_d, gccache[op], 0, (char *) dash_list[op], 2);
	} else if (style == DASH_DOT_LINE || style == DASH_2_DOTS_LINE ||
		  style == DASH_3_DOTS_LINE) {
            int il, nd;
            float *fl;
            if (style == DASH_2_DOTS_LINE) {
		fl=dash_2dots;
		nd=ndash_2dots;
	    } else if (style == DASH_3_DOTS_LINE) {
		fl=dash_3dots;
		nd=ndash_3dots;
	    } else {
		fl=dash_dot;
		nd=ndash_dot;
	    }
	    for (il =0; il<nd; il ++) {
                if (fl[il] != 0.) {
		    if (fl[il] * style_val * display_zoomscale > 255.0)
			dash_list[op][il] = (char) 255;	/* too large for X! */
	    	    else
	        	dash_list[op][il] = (char) round(fl[il] * style_val * 
					display_zoomscale);
		} else {
		    dash_list[op][il] = (char)display_zoomscale;
		}
		if (dash_list[op][il]==0)	/* take care for rounding to zero ! */
			dash_list[op][il]=1;
	    }
	    XSetDashes(tool_d, gccache[op], 0, (char *) dash_list[op], nd);
	}
    }
    gc_thickness[op] = width;
    gc_line_style[op] = style;
    gc_join_style[op] = join_style;
    gc_cap_style[op] = cap_style;
    gc_color[op] = x_color(color);
}

int
x_color(int col)
{
	int	pix;
	if (!all_colors_available) {
		pix = colors[BLACK];
	} else if (col == LT_GRAY) {
		pix = lt_gray_color;
	} else if (col == DARK_GRAY) {
		pix = dark_gray_color;
	} else if (col == MED_GRAY) {
		pix = med_gray_color;
	} else if (col == TRANSP_BACKGROUND) {
		pix = med_gray_color;
	} else if (col == COLOR_NONE) {
		pix = colors[WHITE];
	} else if (col==WHITE) {
		pix = colors[WHITE];
	} else if (col==BLACK) {
		pix = colors[BLACK];
	} else if (col==DEFAULT) {
		pix = x_fg_color.pixel;
	} else if (col==CANVAS_BG) {
		pix = x_bg_color.pixel;
	} else {
	   if (col < 0)
		col = BLACK;
	   if (col >= NUM_STD_COLS+num_usr_cols)
	       pix = x_fg_color.pixel;
	   else
	       pix = colors[col];
	}
	return pix;
}

/* resize the fill patterns for the current display_zoomscale */
/* also generate new Pixmaps in fill_pm[] */

void rescale_pattern(int patnum)
{
	int		j;
	XGCValues	gcv;

	/* this make a few seconds (depending on the machine) */
	set_temp_cursor(wait_cursor);
	j = patnum-(NUMSHADEPATS+NUMTINTPATS);
	/* first rescale the data */
	scale_pattern(j);
 	/* free any old pixmaps before creating new ones */
	if (fill_pm[patnum]) {
		XFreePixmap(tool_d,fill_pm[patnum]);
	}
	fill_pm[patnum] = XCreateBitmapFromData(tool_d, tool_w,
				   pattern_images[j].cdata,
				   pattern_images[j].cwidth,
				   pattern_images[j].cheight);
	/* set the zoom value so we know what zoom it was generated for */
	fill_pm_zoom[patnum] = display_zoomscale;
	/* now update the gc to use the new pixmaps */
	if (fill_gc[patnum]) {
	    gcv.stipple = fill_pm[patnum];
	    XChangeGC(tool_d, fill_gc[patnum], GCStipple, &gcv);
	}
	reset_cursor();
}

void scale_pattern(int indx)
{
    int	    i;
    int	    j;
    char   *odata;
    char   *ndata;
    int	    nbytes;
    int	    obytes;
    int	    ibit;
    int	    jbit;
    int	    wbit;
    int	    width, height;
    int	    nwidth, nheight;

    width = pattern_images[indx].owidth;
    height = pattern_images[indx].oheight;

    nwidth = display_zoomscale * width;
    nheight = display_zoomscale * height;

    /* if already correct size just return */
    if (nwidth ==pattern_images[indx].cwidth &&
	nheight==pattern_images[indx].cheight)
		return;

    /* prevent 0-size bitmaps */
    if (nwidth == 0)
	nwidth = 1;
    if (nheight == 0)
	nheight = 1;

    obytes = (width + 7) / 8;
    nbytes = (nwidth + 7) / 8;

    odata = pattern_images[indx].odata;
    ndata = pattern_images[indx].cdata;
    /* if was already scaled before free that data */
    if (ndata)
	    free(ndata);
    /* allocate new space for zoomed bytes */
    pattern_images[indx].cdata = ndata = (char *) malloc(nbytes * nheight);
    bzero(ndata, nbytes * nheight);	/* clear memory */

    /* create a new bitmap at the specified size (requires interpolation) */
    if (nwidth >= width) {		/* new is larger, loop over its matrix */
	for (j = 0; j < nheight; j++) {
	    jbit = height * j / nheight * obytes;
	    for (i = 0; i < nwidth; i++) {
		ibit = width * i / nwidth;	/* xy bit position from original bitmap */
		wbit = *(odata + jbit + ibit / 8);
		if (wbit & (1 << (ibit & 7)))
		    *(ndata + j * nbytes + i / 8) |= (1 << (i & 7));
	    }
	}
    } else {	/* new is smaller, loop over orig matrix so we don't lose bits */
	for (j = 0; j < height; j++) {
	    jbit = nheight * j / height * nbytes;
	    for (i = 0; i < width; i++) {
		ibit = nwidth * i / width;	/* xy bit position from new bitmap */
		wbit = *(odata + j * obytes + i / 8);
		if (wbit & (1 << (i & 7)))
		    *(ndata + jbit + ibit / 8) |= (1 << (ibit & 7));
	    }
	}
    }
    pattern_images[indx].cwidth = nwidth;
    pattern_images[indx].cheight = nheight;
}

/* storage for conversion of data points to screen coords (zXDrawLines and zXFillPolygon) */

static XPoint	*_pp_ = (XPoint *) NULL;	/* data pointer itself */
static int	 _npp_ = 0;			/* number of points currently allocated */
static Boolean	 _noalloc_ = False;		/* signals previous failed alloc */
static Boolean	 chkalloc(int n);
static void	 convert_sh(zXPoint *p, int n);

void zXDrawLines(Display *d, Window w, GC gc, zXPoint *points, int n, int coordmode)
{
#ifdef CLIP_LINE
    XPoint	*outp;
#endif /* CLIP_LINE */

    /* make sure we have allocated data */
    if (!chkalloc(n)) {
	return;
    }
    /* now convert each point to short into _pp_ */
    convert_sh(points, n);
#ifdef CLIP_LINE
    outp = (XPoint *) malloc(2*n*sizeof(XPoint));
    n = clip_poly(_pp_, n, outp);
    XDrawLines(d, w, gc, outp, n, coordmode);
#else
    XDrawLines(d, w, gc, _pp_, n, coordmode);
#endif /* CLIP_LINE */
}

void zXFillPolygon(Display *d, Window w, GC gc, zXPoint *points, int n, int complex, int coordmode)
{
    XPoint	*outp;

    /* make sure we have allocated data for _pp_ */
    if (!chkalloc(n)) {
	return;
    }
    /* now convert each point to short into _pp_ */
    convert_sh(points, n);
    outp = (XPoint *) malloc(2*n*sizeof(XPoint));
    n = clip_poly(_pp_, n, outp);
    XFillPolygon(d, w, gc, outp, n, complex, coordmode);
    free(outp);
}

/* convert each point to short */

static void
convert_sh(zXPoint *p, int n)
{
    int 	 i;

    for (i=0; i<n; i++) {
	_pp_[i].x = ZOOMX(p[i].x);
	_pp_[i].y = ZOOMY(p[i].y);
    }
}

static Boolean
chkalloc(int n)
{
    int		 i;
    XPoint	*tpp;

    /* see if we need to allocate some (more) memory */
    if (n > _npp_) {
	/* if previous allocation failed, return now */
	if (_noalloc_)
	    return False;
	/* get either what we need +50 points or 500, whichever is larger */
	i = max2(n+50, 500);	
	if (_npp_ == 0) {
	    if ((tpp = (XPoint *) malloc(i * sizeof(XPoint))) == 0) {
		fprintf(stderr,"\007Can't alloc memory for %d point array, exiting\n",i);
		exit(1);
	    }
	} else {
	    if ((tpp = (XPoint *) realloc(_pp_, i * sizeof(XPoint))) == 0) {
		file_msg("Can't alloc memory for %d point array",i);
		_noalloc_ = True;
		return False;
	    }
	}
	/* everything ok, set global pointer and count */
	_pp_ = tpp;
	_npp_ = i;
    }
    return True;
}

/*
 * clip_poly - This procedure performs the Sutherland-Hodgman polygon clipping
 * on the inVertices array, putting the resultant points into the outVertices array,
 * and returning the number of points as the return value.
 * We are clipping to the canvas area.
 */

static int
clip_poly(XPoint *inVertices, int npoints, XPoint *outVertices)
{
	/* clip to left edge */
        npoints = SutherlandHodgmanPolygoClip (inVertices, outVertices, npoints, 
					-100, CANVAS_HT*2, -100, -100);
        setup_next(npoints, inVertices, outVertices);
	/* now to bottom edge */
        npoints = SutherlandHodgmanPolygoClip (inVertices,outVertices, npoints, 
					-100, CANVAS_HT*2, CANVAS_WD*2, CANVAS_HT*2);
        setup_next(npoints, inVertices, outVertices);
	/* right edge */
        npoints = SutherlandHodgmanPolygoClip (inVertices,outVertices, npoints, 
					CANVAS_WD*2, -100, CANVAS_WD*2, CANVAS_HT*2);
        setup_next(npoints, inVertices, outVertices);
	/* top edge */
        npoints = SutherlandHodgmanPolygoClip (inVertices,outVertices, npoints, 
					CANVAS_WD*2, -100, -100, -100);
        setup_next(npoints, inVertices, outVertices);
        return npoints;
}

/*
 * The "SutherlandHodgmanPolygoClip" function is a critical function of the
 * polygon clipping. It uses the Sutherland_Hodgman algorithm to implement
 * a step in clipping a polygon to a clipping window. 
 */

int
SutherlandHodgmanPolygoClip (
    XPoint	*inVertices,		/* Input vertex array */
    XPoint	*outVertices,		/* Output vertex array */
    int		 inLength,		/* Number of entries in inVertices */
    int		 x1, int y1, int x2, int y2)	/* Edge of clip polygon */
{
	XPoint s,p;	/*Start, end point of current polygon edge*/ 
	XPoint i;	/*Intersection point with a clip boundary*/
	int j;		/*Vertex loop counter*/
	int outpts;	/* number of points in output array */

        outpts = 0;
        s.x = inVertices[inLength-1].x; /*Start with the last vertex in inVertices*/
        s.y = inVertices[inLength-1].y;
        for (j=0; j < inLength; j++) {
            p.x = inVertices[j].x; /*Now s and p correspond to the vertices*/
            p.y = inVertices[j].y;
            if (inside(p,x1, y1, x2, y2)) {      /*Cases 1 and 4*/
                 if (inside(s, x1, y1, x2, y2)) {
			outVertices[outpts].x = p.x;
			outVertices[outpts].y = p.y;
			outpts++;
                } else {                            /*Case 4*/
                        intersect(s, p, x1, y1, x2, y2, &i);
			outVertices[outpts].x = i.x;
			outVertices[outpts].y = i.y;
			outpts++;
			outVertices[outpts].x = p.x;
			outVertices[outpts].y = p.y;
			outpts++;
                }
            } else {                  /*Cases 2 and 3*/
                if (inside(s, x1, y1, x2, y2))  /*Cases 2*/ {
                        intersect(s, p, x1, y1, x2, y2, &i);
			outVertices[outpts].x = i.x;
			outVertices[outpts].y = i.y;
			outpts++;
                }
           }                          /*No action for case 3*/
           s.x = p.x;	/*Advance to next pair of vertices*/
           s.y = p.y;
	}
	return outpts;
}      /*SutherlandHodgmanPolygonClip*/

/* 
 * The "Inside" function returns TRUE if the vertex tested is on the inside
 * of the clipping boundary. "Inside" is defined as "to the left of
 * clipping boundary when one looks from the first vertex to the second
 * vertex of the clipping boundary". The code for this function is:
 */

static Boolean
inside (XPoint testVertex, int x1, int y1, int x2, int y2)
{
    if (x2 > x1)              /*bottom edge*/
	if (testVertex.y <= y1) 
	    return True;
    if (x2 < x1)              /*top edge*/
	if (testVertex.y >= y1) 
	    return True;
    if (y2 > y1)              /*right edge*/
	if (testVertex.x <= x2) 
	    return True;
    if (y2 < y1)              /*left edge*/
	if (testVertex.x >= x2) 
	    return True;
    /* outside */
    return False;
}

/*
 * The "intersect" function calculates the intersection of the polygon edge
 * (vertex s to p) with the clipping boundary. 
 */

static void
intersect(XPoint first, XPoint second, int x1, int y1, int x2, int y2,
                XPoint *intersectPt)
{
  if (y1 == y2) {    /*horizontal*/
     intersectPt->y=y1;
     intersectPt->x=first.x +(y1-first.y)*
                    (second.x-first.x)/(second.y-first.y);   /*Vertical*/
   } else {
           intersectPt->x=x1;
           intersectPt->y=first.y +(x1-first.x)*
                    (second.y-first.y)/(second.x-first.x);
   }
}

/*
 * setup_next copies the input to the output
 */

static void
setup_next(int npoints, XPoint *in, XPoint *out)
{
	int i;
	for (i=0; i<npoints; i++) {
		in[i].x = out[i].x;
		in[i].y = out[i].y;
	}
}


/*
 * clip_line - This procedure clips a line to the current clip boundaries and
 * returns the new coordinates in x3, y3 and x4, y4.
 * If the line lies completely outside of the clip boundary, the result is False,
 * otherwise True.
 * This procedure uses the well known Cohen-Sutherland line clipping
 * algorithm to clip each coordinate.
 */

int compoutcode(int x, int y);

/* bitfields for output codes */
#define codetop    1
#define codebottom 2
#define coderight  4
#define codeleft   8

void
clip_line(int x1, int y1, int x2, int y2,  short *x3, short *y3, short *x4, short *y4)
{
  int outcode0;         /* the code of the first endpoint  */
  int outcode1;         /* the code of the second endpoint */
  int outcodeout;
  int x, y;

  outcode0 = compoutcode(x1, y1);		/* compute the original codes   */
  outcode1 = compoutcode(x2, y2);

  /* while not trivially accepted */
  while (outcode0 != 0 || outcode1 != 0)  {
    if ((outcode0 & outcode1) != 0) { 		/* trivial reject */
	/* set line to a single point at the limits of the screen */
	if ((outcode0|outcode1) & codebottom) *y3 = *y4 = CANVAS_HT;
    	if ((outcode0|outcode1) & codetop)    *y3 = *y4 = 0;
    	if ((outcode0|outcode1) & codeleft)   *x3 = *x4 = 0;
    	if ((outcode0|outcode1) & coderight)  *x3 = *x4 = CANVAS_WD;
	return;
    } else {
	/* failed both tests, so calculate the line segment to clip */
	if (outcode0 > 0 )
	    outcodeout = outcode0;	/* clip the first point */
	else
	    outcodeout = outcode1;	/* clip the last point  */

	if ((outcodeout & codebottom) == codebottom) {
	    /* clip the line to the bottom of the viewport     */
	    y = CANVAS_HT;
	    x = x1+(double)(x2-x1)*(double)(y-y1) / (y2 - y1);
	}
	else if ((outcodeout & codetop) == codetop ) {
	    /* clip the line to the top of the viewport        */
	    y = 0;
	    x = x1+(double)(x2-x1)*(double)(y-y1) / (y2 - y1);
	}
	else if ((outcodeout & coderight) == coderight ) {
	    /* clip the line to the right edge of the viewport */
	    x = CANVAS_WD;
	    y = y1+(double)(y2-y1)*(double)(x-x1) / (x2-x1);
	}
	else if ((outcodeout & codeleft) == codeleft ) {
	    /* clip the line to the left edge of the viewport  */
	    x = 0;
	    y = y1+(double)(y2-y1)*(double)(x-x1) / (x2-x1);
	};

	if (outcodeout == outcode0) {		/* modify the first coordinate   */
	    x1 = x; y1 = y;			/* update temporary variables    */  
	    outcode0 = compoutcode(x1, y1);	/* recalculate the outcode       */
	}
	else {
	/* modify the second coordinate  */
	    x2 = x; y2 = y;			/* update temporary variables    */  
	    outcode1 = compoutcode(x2, y2);	/* recalculate the outcode       */
	}
    }
  }

  /* coordinates for the new line! */
  *x3 = (short) x1;
  *y3 = (short) y1;
  *x4 = (short) x2;
  *y4 = (short) y2;

  return;
}

/* return codes for different cases */

int
compoutcode(int x, int y)
{
  int code = 0;

  if      (y > CANVAS_HT) code = codebottom;
  else if (y < 0) code = codetop;

  if      (x > CANVAS_WD) code = code+coderight;
  else if (x < 0) code = code+codeleft;
  return code;
}

