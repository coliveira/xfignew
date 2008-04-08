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

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "f_read.h"
#include "u_fonts.h"
#include "u_create.h"
#include "version.h"
#include "w_drawprim.h"
#include "w_export.h"
#include "w_file.h"
#include "w_print.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#include "d_spline.h"
#include "e_update.h"
#include "f_picobj.h"
#include "f_readold.h"
#include "f_util.h"
#include "u_bound.h"
#include "u_free.h"
#include "u_scale.h"
#include "u_translate.h"
#include "w_util.h"
#include "w_layers.h"

/* EXPORTS */

int		 defer_update_layers = 0; /* if != 0, update_layers() doesn't update */
int		 line_no, save_line;	/* current input line number */
int		 num_object;		/* current number of objects */
char		*read_file_name;	/* current input file name */

/* LOCAL */

static char	Err_incomp[] = "Incomplete %s object at line %d.";

static void        read_colordef(FILE *fp);
static F_ellipse  *read_ellipseobject(void);
static F_line     *read_lineobject(FILE *fp);
static F_text     *read_textobject(FILE *fp);
static F_spline   *read_splineobject(FILE *fp);
static F_arc      *read_arcobject(FILE *fp);
static F_compound *read_compoundobject(FILE *fp);
static char	  *attach_comments(void);
static void	   count_lines_correctly(FILE *fp);
static int	   read_return(int status);
static Boolean	   contains_picture(F_compound *compound);

#define FILL_CONVERT(f) \
	   ((proto >= 22) ? (f): \
	     (((proto>=20) || (f) == 0 || !TFX) ?  \
		(f-1) : (!TFX? (NUMFILLPATS-1) - ((f)-1)*5: UNFILLED)))


/* max number of comments that can be stored with each object */
#define MAXCOMMENTS	100

/* input buffer length */
#define	BUF_SIZE	1024

char		 buf[BUF_SIZE];		/* input buffer */
char		*comments[MAXCOMMENTS];	/* comments saved for current object */
int		 numcom;		/* current comment index */
Boolean		 com_alloc = False;	/* whether or not the comment array has been init. */
int		 TFX;			/* true for 1.4TFX protocol */
int		 proto;			/* file protocol*10 */
float	         fproto, xfigproto;	/* floating values for protocol of figure
					   file and current protocol */

/* initialize the user color counter - then read figure file. 
   Called from load_file(), merge_file(), preview_figure(), load_lib_obj(),
   and paste(), but NOT from read_figure() (import Fig as picture) */


void merge_colors (F_compound *objects);
void swap_colors (void);
int readfp_fig (FILE *fp, F_compound *obj, Boolean merge, int xoff, int yoff, fig_settings *settings);
int read_line (FILE *fp);
int read_objects (FILE *fp, F_compound *obj, int *res);
void scale_figure (F_compound *obj, float mul, int offset);
void shift_figure (F_compound *obj);
void fix_depth (int *depth);
void check_color (int *color);
void convert_arrow (int *type, float *wd, float *ht);
void fix_angle (float *angle);
void skip_line (FILE *fp);
int backslash_count (char *cp, int start);
int save_comment (FILE *fp);
void renumber_comp (F_compound *compound);
void renumber (int *color);

int
read_figc(char *file_name, F_compound *obj, Boolean merge, Boolean remapimages, int xoff, int yoff, fig_settings *settings)
{
    int i,status;

    n_num_usr_cols = -1;
    for (i=0; i<MAX_USR_COLS; i++)
	n_colorFree[i] = True;

    /* now read the file */
    status = read_fig(file_name, obj, merge, xoff, yoff, settings);

    if (status != 0)
	return status;

    if (merge) {
	/* merge any user colors from the new file just read into the current figure */
	merge_colors(obj);
    } else {
	/* now swap old figure colors with new colors (if any) */
	swap_colors();
	current_memory = -1;
	show_pencolor();
	show_fillcolor();
    }

    /* and remap colors for all pictures */
    /* but only if a picture object is in the new file */
    if (remapimages && contains_picture(obj))
	remap_imagecolors();

    return status;
}

static Boolean
contains_picture(F_compound *compound)
{
    F_line	   *l;
    F_compound	   *c;
    /* traverse the compounds in this compound */
    for (c = compound->compounds; c != NULL; c = c->next) {
	if (contains_picture(c))
	    return True;
    }
    for (l = compound->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE)
	    return True;
    }
    return False;
}

/**********************************************************
Read_fig returns :

           0 : successful read.
  BAD_FORMAT : File is in incorrect format
  EMPTY_FILE : File is empty
  NO_VERSION : No version was found in "#FIG" header
      err_no : if file can not be read for various reasons
		(from /usr/include/sys/errno.h)

The resolution (ppi) is stored in resolution.
The coordinate system is 1 for lower left at 0,0 and
2 for upper left at 0,0, but this value is not used,
because xfig only uses 2 for the coordinate system.

If "merge" is True, the user colors are merged into the color
set, else they replace the current user colors.

Also, if merging, if the file being merged into the main figure
has different units (inches/cm) it is rescaled to match the
main figure.

If update_figs is false, any imported images are read in, otherwise 
they are not.  The latter case is used for the -update command-line
option where the user just wants to read and re-write Fig files
to bring them up-to-date.
**********************************************************/

int
read_fig(char *file_name, F_compound *obj, Boolean merge, int xoff, int yoff, fig_settings *settings)
{
    FILE	   *fp;
    int		    status;

    read_file_name = file_name;
    first_file_msg = True;
    if (uncompress_file(file_name) == False)
	return ENOENT;		/* doesn't exist */
    if ((fp = fopen(file_name, "r")) == NULL)
	return errno;
    else {
	if (!update_figs)
	    put_msg("Reading objects from \"%s\" ...", file_name);
#ifdef I18N
	/* set the numeric locale to C so we get decimal points for numbers */
	setlocale(LC_NUMERIC, "C");
#endif  /* I18N */
	status = readfp_fig(fp, obj, merge, xoff, yoff, settings);
#ifdef I18N
	/* reset to original locale */
	setlocale(LC_NUMERIC, "");
#endif  /* I18N */
	fclose(fp);
	/* so subsequent file_msg() calls don't print wrong file name */
	first_file_msg = False;
	return status;
    }
}

int
readfp_fig(FILE *fp, F_compound *obj, Boolean merge, int xoff, int yoff, fig_settings *settings)
{
    int		    status;
    int		    i;
    int		    resolution;
    char	    versstring[10];

    defer_update_layers = 1;		/* prevent update_layers() from updating */

    /* initialize settings structure in case we read an older Fig format */
    settings->landscape = appres.landscape;
    settings->flushleft = appres.flushleft;
    settings->units	= appres.INCHES;
    settings->papersize	= appres.papersize;
    settings->magnification = appres.magnification;
    settings->multiple	= appres.multiple;
    settings->transparent = appres.transparent;

    num_object = 0;
    /* reset comment number */
    numcom = 0;
    /* initialize the comment array */
    if (!com_alloc)
	for (i=0; i<MAXCOMMENTS; i++)
	    comments[i] = (char *) NULL;
    com_alloc = True;
    bzero((char*)obj, COMOBJ_SIZE);
    line_no = 1;
    /* read the version header line (e.g. #FIG 3.2) */
    if (fgets(buf, BUF_SIZE, fp) == 0)
	return read_return(EMPTY_FILE);
    if (strncmp(buf, "#FIG", 4) == 0) {		/* versions 1.4/later have #FIG in first line */
	if (strlen(buf) <= 6) {
	    return read_return(NO_VERSION);	/* Short line - say corrupt */
	}
	if ((sscanf((char*)(strchr(buf, ' ') + 1), "%f", &fproto)) == 0)  /* assume 1.4 */
	    proto = 14;
	else
	    proto = (fproto + .01) * 10;	/* protocol version*10 */

	/* if file protocol != current protocol, give message */
	strcpy(versstring, PROTOCOL_VERSION);	/* copy because gcc doesn't allow writing */
	sscanf(versstring,"%f",&xfigproto);	/* to const strings. sscanf does ungetc */
	if (fproto > xfigproto) {
	    file_msg("You must have a NEWER version of Xfig to load this figure (%.1f).",
			fproto);
	    return read_return(BAD_FORMAT);
	}
	/* Protocol 2.2 was only beta test - 3.0 is the release (and is identical) */
	if (proto == 22)
	    proto = 30;

	TFX = False;
	if (strstr(buf, "TFX") != NULL)
	    TFX = True;

	if (proto >= 30) {
	    /* read Portrait/Landscape indicator now */
	    if (read_line(fp) < 0) {
		file_msg("No Portrait/Landscape specification");
		return read_return(BAD_FORMAT);		/* error */
	    }
	    settings->landscape = (strncasecmp(buf,"landscape",9) == 0);

	    /* read Centering indicator now */
	    if (read_line(fp) < 0) {
		file_msg("No Center/Flushleft specification");
		return read_return(BAD_FORMAT);		/* error */
	    }
	    if ((strncasecmp(buf,"center",6) == 0) || 
		(strncasecmp(buf,"flush",5) == 0)) {
		    /* use negative to ensure 1/0 (strcmp may return 3 or 4 for false) */
		    settings->flushleft = !strncasecmp(buf,"flush",5);
		    /* NOW read metric/inches indicator */
		    if (read_line(fp) < 0) {
			file_msg("No Metric/Inches specification");
			return read_return(BAD_FORMAT);		/* error */
		    }
	    }
	    /* set metric/inches mode appropriately */
	    settings->units = (strncasecmp(buf,"metric",5) != 0);

	    /* paper size, magnification, multiple page flag and transparent color
	       (for GIF export) new in 3.2 */
	    if (proto >= 32) {
		/* read paper size now */
		if (read_line(fp) < 0) {
		    file_msg("No Paper size specification");
		    return read_return(BAD_FORMAT);		/* error */
		}
		/* parse the paper size */
		settings->papersize = parse_papersize(buf);

		/* read magnification now */
		if (read_line(fp) < 0) {
		    file_msg("No Magnification specification");
		    return read_return(BAD_FORMAT);		/* error */
		}
		settings->magnification = atoi(buf);

		/* read multiple page flag now */
		if (read_line(fp) < 0) {
		    file_msg("No Multiple page flag specification");
		    return read_return(BAD_FORMAT);		/* error */
		}
		if (strncasecmp(buf,"multiple",8) != 0 &&
		    strncasecmp(buf,"single",6) != 0) {
		    file_msg("No Multiple page flag specification");
		    return read_return(BAD_FORMAT);
		}
		settings->multiple = (strncasecmp(buf,"multiple",8) == 0);

		/* read transparent color now */
		if (read_line(fp) < 0) {
		    file_msg("No Transparent color specification");
		    return read_return(BAD_FORMAT);		/* error */
		}
		settings->transparent = atoi(buf);
	    }
	}
	/* now read the figure itself */
	status = read_objects(fp, obj, &resolution);

    } else {
	file_msg("Seeing if this figure is Fig format 1.3");
	file_msg("If this doesn't work then this is not a Fig file.");
	proto = 13;
	status = read_1_3_objects(fp, buf, obj);
    }
    /* don't go any further if there was an error in reading the figure */
    if (status != 0) {
	return read_return(status);
    }

    n_num_usr_cols++;	/* number of user colors = max index + 1 */
    /*******************************************************************************
	The older versions of xfig (1.3 to 2.1) used values that ended in 4 or 9
	for coordinates on the "grid".  When multiplied by 15 for the 3.0
	resolution these values ended up 14 "new" pixels off the grid.

	For 3.0 files, 1 is added to the coordinates, and in addition, the USER
	is supposed to set the x and y offset in the file panel both to the
	amount necessary to correct the problem.
	For older files 1 is first added to coordinates then they are multiplied by 15.
    ********************************************************************************/

    if (proto == 30) {
       scale_figure(obj,((float)PIX_PER_INCH)/resolution,0);
    } else if (resolution != PIX_PER_INCH) {
       if (proto == 21 && resolution == 76 && !settings->units)
	  scale_figure(obj,((float)PIX_PER_INCH)/80,15); /* for 2.1.8S, HWS */
       else
          scale_figure(obj,((float)PIX_PER_INCH)/resolution,15);
    }

    /* if merging a figure with different units, rescale for mixed units, HWS */
    if (merge && (proto >= 30)) {		
       if (!appres.INCHES && settings->units)
	   read_scale_compound(obj,(2.54*PIX_PER_CM)/((float)PIX_PER_INCH),0);
       if (appres.INCHES && !settings->units)
	   read_scale_compound(obj,((float)PIX_PER_INCH)/(2.54*PIX_PER_CM),0);
    }
    /* if the user wants to scale the figure */
    if (scale_factor != 1.0)
	read_scale_compound(obj, scale_factor, 0);

    /* shift the figure by the amount in the x and y offsets from the file panel */
    translate_compound(obj, xoff, yoff);

    /* get bounding box of whole figure */
    compound_bound(obj,&obj->nwcorner.x,&obj->nwcorner.y,&obj->secorner.x,&obj->secorner.y);

    /* ask the user if the figure should be shifted if there are negative coords */
    if (!update_figs)
	shift_figure(obj);

    /* now update the grid/ruler units */
    if (settings->units) {
	/* inches */
	if (strcasecmp(appres.tgrid_unit, "default") != 0) {
	    /* the user specified a units when he started, try to keep it */
	    if (strcasecmp(appres.tgrid_unit, "tenth") == 0 || 
		strcasecmp(appres.tgrid_unit, "ten") == 0 ||
		strcasecmp(appres.tgrid_unit, "1/10") == 0 ||
		strcasecmp(appres.tgrid_unit, "10") == 0)
		    settings->grid_unit = TENTH_UNIT;
		else
		    settings->grid_unit = FRACT_UNIT;
	} else {
	    settings->grid_unit = FRACT_UNIT;
	}
    } else {
	/* metric, only choice */
	settings->grid_unit = MM_UNIT;
    }

    /* return with status */
    return read_return(status);
}

/* clear defer_update_layers counter, update the layer buttons and return status */

static int
read_return(int status)
{
    defer_update_layers = 0;
    if (!update_figs)
	update_layers();
    return status;
}

int read_objects(FILE *fp, F_compound *obj, int *res)
{
    F_ellipse	   *e, *le = NULL;
    F_line	   *l, *ll = NULL;
    F_text	   *t, *lt = NULL;
    F_spline	   *s, *ls = NULL;
    F_arc	   *a, *la = NULL;
    F_compound	   *c, *lc = NULL;
    int		    object, ppi, coord_sys;

    if (read_line(fp) < 0) {
	file_msg("No Resolution specification; figure is empty");
	return BAD_FORMAT;
    }

    /* read the resolution (ppi) and the coordinate system used (upper-left or lower-left) */
    if (sscanf(buf, "%d%d\n", &ppi, &coord_sys) != 2) {
	file_msg("Figure resolution or coordinate specifier missing in line %d.", line_no);
	return BAD_FORMAT;
    }

    /* attach any comments found thus far to the whole figure */
    obj->comments = attach_comments();

    /* save the resolution for caller */
    *res = ppi;

    while (read_line(fp) > 0) {
	if (sscanf(buf, "%d", &object) != 1) {
	    file_msg("Incorrect format at line %d.", line_no);
	    return (num_object != 0? 0: BAD_FORMAT);	/* ok if any objects have been read */
	}
	switch (object) {
	case O_COLOR_DEF:
	    read_colordef(fp);
	    if (num_object) {
		file_msg("Color definitions must come before other objects (line %d).",
			line_no);
	    }
	    break;
	case O_POLYLINE:
	    if ((l = read_lineobject(fp)) == NULL)
		continue;
	    if (ll)
		ll = (ll->next = l);
	    else
		ll = obj->lines = l;
	    num_object++;
	    break;
	case O_SPLINE:
	    if ((s = read_splineobject(fp)) == NULL)
		continue;
	    if (ls)
		ls = (ls->next = s);
	    else
		ls = obj->splines = s;
	    num_object++;
	    break;
	case O_ELLIPSE:
	    if ((e = read_ellipseobject()) == NULL)
		continue;
	    if (le)
		le = (le->next = e);
	    else
		le = obj->ellipses = e;
	    num_object++;
	    break;
	case O_ARC:
	    if ((a = read_arcobject(fp)) == NULL)
		continue;
	    if (la)
		la = (la->next = a);
	    else
		la = obj->arcs = a;
	    num_object++;
	    break;
	case O_TEXT:
	    if ((t = read_textobject(fp)) == NULL)
		continue;
	    if (lt)
		lt = (lt->next = t);
	    else
		lt = obj->texts = t;
	    num_object++;
	    break;
	case O_COMPOUND:
	    if ((c = read_compoundobject(fp)) == NULL)
		continue;
	    if (lc)
		lc = (lc->next = c);
	    else
		lc = obj->compounds = c;
	    num_object++;
	    break;
	default:
	    file_msg("Incorrect object code at line %d.", line_no);
	    continue;
	} /* switch */
	
    } /* while */

    if (feof(fp))
	return 0;
    else
	return errno;
}				/* read_objects */

int parse_papersize(char *size)
{
    int i,len;
    char *c;

    /* first get rid of trailing newline */
    if (size[strlen(size)-1]=='\n')
	size[strlen(size)-1]='\0';
    /* then truncate at first space or parenthesis "(" in passed size */
    if (((c=strchr(size,' '))!= NULL)||((c=strchr(size,'(')) != NULL)) {
	*c ='\0';
    }
    len = strlen(size);
    /* change ledger (deprecated) to tabloid */
    if (strncasecmp(size,"ledger",len) == 0)
	strcpy(size,"tabloid");
    for (i=0; i<NUMPAPERSIZES; i++) {
	if (strncasecmp(size,paper_sizes[i].sname,len) == 0)
	    break;
    }
    /* return entry 0 for bad papersize */
    if (i >= NUMPAPERSIZES)
	return 0;
    return i;
}

static void
read_colordef(FILE *fp)
{
    int		    c,r,g,b;

    if ((sscanf(buf, "%*d %d #%02x%02x%02x", &c, &r, &g, &b) != 4) ||
		(c < NUM_STD_COLS) || (c >= MAX_USR_COLS+NUM_STD_COLS)) {
	buf[strlen(buf)-1]='\0';	/* remove the newline */
	file_msg("Invalid color definition: %s, setting to black (#00000).",buf);
	r=g=b=0;
	c = NUM_STD_COLS;
    }
    /* make in the range 0...MAX_USR_COLS */
    c -= NUM_STD_COLS;
    n_user_colors[c].red = r*256;
    n_user_colors[c].green = g*256;
    n_user_colors[c].blue = b*256;
    n_colorFree[c] = False;
    /* keep track of highest color number */
    n_num_usr_cols = max2(c, n_num_usr_cols);
}

static F_arc   *
read_arcobject(FILE *fp)
{
    F_arc	   *a;
    int		    n, fa, ba;
    int		    type, style;
    float	    thickness, wd, ht;

    if ((a = create_arc()) == NULL)
	return NULL;

    save_line = line_no;
    a->next = NULL;
    a->for_arrow = a->back_arrow = NULL;
    if (proto >= 30) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%d%d%d%f%f%d%d%d%d%d%d\n",
	       &a->type, &a->style, &a->thickness,
	       &a->pen_color, &a->fill_color, &a->depth,
	       &a->pen_style, &a->fill_style,
	       &a->style_val, &a->cap_style,
	       &a->direction, &fa, &ba,
	       &a->center.x, &a->center.y,
	       &a->point[0].x, &a->point[0].y,
	       &a->point[1].x, &a->point[1].y,
	       &a->point[2].x, &a->point[2].y);
    } else {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d%f%f%d%d%d%d%d%d\n",
	       &a->type, &a->style, &a->thickness,
	       &a->pen_color, &a->depth,
	       &a->pen_style, &a->fill_style,
	       &a->style_val, &a->direction, &fa, &ba,
	       &a->center.x, &a->center.y,
	       &a->point[0].x, &a->point[0].y,
	       &a->point[1].x, &a->point[1].y,
	       &a->point[2].x, &a->point[2].y);
	a->fill_color = a->pen_color;
	a->cap_style = CAP_BUTT;	/* butt line cap */
    }
    a->type--;	/* internally, 0=open arc, 1=pie wedge */
    if (((proto < 22) && (n != 19)) || ((proto >= 30) && (n != 21))) {
	file_msg(Err_incomp, "arc", save_line);
	free((char *) a);
	return NULL;
    }
    a->fill_style = FILL_CONVERT(a->fill_style);
    fix_depth(&a->depth);
    check_color(&a->pen_color);
    check_color(&a->fill_color);
    fix_fillstyle(a);	/* make sure that black/white have legal fill styles */

    a->comments = attach_comments();		/* attach any comments */

    /* forward arrow */
    if (fa) {
	if (read_line(fp) == -1)
	    return a;
	if (sscanf(buf, "%d%d%f%f%f", &type, &style, &thickness, &wd, &ht) != 5) {
	    file_msg(Err_incomp, "arc", save_line);
	    return a;
	}
	/* throw away any arrow heads on pie-wedge arcs */
	if (a->type == T_OPEN_ARC) {
	    /* make sure arrowhead is legal and convert units */
	    convert_arrow(&type, &wd, &ht);
	    a->for_arrow = new_arrow(type, style, thickness, wd, ht);
	}
    }

    /* backward arrow */
    if (ba) {
	if (read_line(fp) == -1)
	    return a;
	if (sscanf(buf, "%d%d%f%f%f", &type, &style, &thickness, &wd, &ht) != 5) {
	    file_msg(Err_incomp, "arc", save_line);
	    return a;
	}
	/* throw away any arrow heads on pie-wedge arcs */
	if (a->type == T_OPEN_ARC) {
	    /* make sure arrowhead is legal and convert units */
	    convert_arrow(&type, &wd, &ht);
	    a->back_arrow = new_arrow(type, style, thickness, wd, ht);
	}
    }
    return a;
}

static F_compound *
read_compoundobject(FILE *fp)
{
    F_arc	   *a, *la = NULL;
    F_ellipse	   *e, *le = NULL;
    F_line	   *l, *ll = NULL;
    F_spline	   *s, *ls = NULL;
    F_text	   *t, *lt = NULL;
    F_compound	   *com, *c, *lc = NULL;
    int		    n, object;

    if ((com = create_compound()) == NULL)
	return NULL;

    com->arcs = NULL;
    com->ellipses = NULL;
    com->lines = NULL;
    com->splines = NULL;
    com->texts = NULL;
    com->compounds = NULL;
    com->next = NULL;
    com->comments = attach_comments();		/* attach any comments */

    save_line = line_no;
    /* read bounding info for compound */
    n = sscanf(buf, "%*d%d%d%d%d\n", &com->nwcorner.x, &com->nwcorner.y,
	       &com->secorner.x, &com->secorner.y);
    /* if compound spec has no bounds, set to 0 and calculate later */
    if (n <= 0) {
	com->nwcorner.x =com->nwcorner.y = com->secorner.x = com->secorner.y = 0;
    } else if (n != 4) {
	/* otherwise, if there aren't 4 numbers, complain */
	file_msg(Err_incomp, "compound", save_line);
	free((char *) com);
	return NULL;
    }
    while (read_line(fp) > 0) {
	if (sscanf(buf, "%d", &object) != 1) {
	    file_msg(Err_incomp, "compound", save_line);
	    free((char *) com);
	    return NULL;
	}
	switch (object) {
	case O_POLYLINE:
	    if ((l = read_lineobject(fp)) == NULL)
		continue;
	    if (ll)
		ll = (ll->next = l);
	    else
		ll = com->lines = l;
	    break;
	case O_SPLINE:
	    if ((s = read_splineobject(fp)) == NULL)
		continue;
	    if (ls)
		ls = (ls->next = s);
	    else
		ls = com->splines = s;
	    break;
	case O_ELLIPSE:
	    if ((e = read_ellipseobject()) == NULL)
		continue;
	    if (le)
		le = (le->next = e);
	    else
		le = com->ellipses = e;
	    break;
	case O_ARC:
	    if ((a = read_arcobject(fp)) == NULL)
		continue;
	    if (la)
		la = (la->next = a);
	    else
		la = com->arcs = a;
	    break;
	case O_TEXT:
	    if ((t = read_textobject(fp)) == NULL)
		continue;
	    if (lt)
		lt = (lt->next = t);
	    else
		lt = com->texts = t;
	    break;
	case O_COMPOUND:
	    if ((c = read_compoundobject(fp)) == NULL)
		continue;
	    if (lc)
		lc = (lc->next = c);
	    else
		lc = com->compounds = c;
	    break;
	case O_END_COMPOUND:
	    /* if compound def had no bounds or all zeroes, calculate bounds now */
	    if (com->nwcorner.x == 0 && com->nwcorner.x == 0 && 
	        com->nwcorner.x == 0 && com->nwcorner.x == 0)
		    compound_bound(com, &com->nwcorner.x, &com->nwcorner.y,
					&com->secorner.x, &com->secorner.y);
	    return com;
	default:
	    file_msg("Incorrect object code at line %d.", save_line);
	    continue;
	}			/* switch */
    } /* while (read_line(fp) > 0) */

    if (feof(fp)) {
 	compound_bound(com, &com->nwcorner.x, &com->nwcorner.y,
 	           &com->secorner.x, &com->secorner.y);
	return com;
    } else {
	return NULL;
    }
}

static F_ellipse *
read_ellipseobject(void)
{
    F_ellipse	   *e;
    int		    n;

    if ((e = create_ellipse()) == NULL)
	return NULL;

    save_line = line_no;
    e->next = NULL;
    if (proto >= 30) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%f%d%d%d%d%d%d%d%d\n",
	       &e->type, &e->style, &e->thickness,
	       &e->pen_color, &e->fill_color, &e->depth,
	       &e->pen_style, &e->fill_style,
	       &e->style_val, &e->direction, &e->angle,
	       &e->center.x, &e->center.y,
	       &e->radiuses.x, &e->radiuses.y,
	       &e->start.x, &e->start.y,
	       &e->end.x, &e->end.y);
    } else {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%f%d%d%d%d%d%d%d%d\n",
	       &e->type, &e->style, &e->thickness,
	       &e->pen_color, &e->depth, &e->pen_style, &e->fill_style,
	       &e->style_val, &e->direction, &e->angle,
	       &e->center.x, &e->center.y,
	       &e->radiuses.x, &e->radiuses.y,
	       &e->start.x, &e->start.y,
	       &e->end.x, &e->end.y);
	e->fill_color = e->pen_color;
    }
    if (((proto < 22) && (n != 18)) || ((proto >= 30) && (n != 19))) {
	file_msg(Err_incomp, "ellipse", save_line);
	free((char *) e);
	return NULL;
    }
    e->fill_style = FILL_CONVERT(e->fill_style);
    fix_angle(&e->angle);	/* make sure angle is 0 to 2PI */
    fix_depth(&e->depth);
    check_color(&e->pen_color);
    check_color(&e->fill_color);
    fix_fillstyle(e);	/* make sure that black/white have legal fill styles */
    e->comments = attach_comments();		/* attach any comments */
    return e;
}

static F_line  *
read_lineobject(FILE *fp)
{
    F_line	   *l;
    F_point	   *p, *q;
    int		    n, x, y, fa, ba, npts, cnpts;
    int		    type, style, radius_flag;
    float	    thickness, wd, ht;
    int		    ox, oy;
    char	    picfile[PATH_MAX];
    Boolean	    dum;

    if ((l = create_line()) == NULL)
	return NULL;

    save_line = line_no;
    l->points = NULL;
    l->for_arrow = l->back_arrow = NULL;
    l->next = NULL;

    sscanf(buf, "%*d%d", &l->type);

    /* 2.0 has radius parm only for arc-box objects */
    /* 2.1 or later has radius parm for all line objects */
    /* 3.0(experimental 2.2) or later additionally has number of points parm for
	all line objects and fill color separate from border color */
    radius_flag = ((proto >= 21) || (l->type == T_ARCBOX && proto == 20));
    if (proto >= 30) {
	n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%d%d%d%d%d",
		   &l->type, &l->style, &l->thickness, &l->pen_color, &l->fill_color,
		   &l->depth, &l->pen_style, &l->fill_style, &l->style_val,
		   &l->join_style, &l->cap_style, &l->radius, &fa, &ba, &npts);
    } else {	/* v2.1 and earlier */
	if (radius_flag) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d%d",
		   &l->type, &l->style, &l->thickness, &l->pen_color, &l->depth,
	      &l->pen_style, &l->fill_style, &l->style_val, &l->radius, &fa, &ba);
	} else { /* old format uses pen for radius of arc-box * corners */
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d",
		   &l->type, &l->style, &l->thickness, &l->pen_color,
	           &l->depth, &l->pen_style, &l->fill_style, &l->style_val, &fa, &ba);
	    if (l->type == T_ARCBOX) {
		l->radius = l->pen_style;
		l->pen_style = -1;
	    } else
		l->radius = DEFAULT;
	}
	l->fill_color = l->pen_color;
	l->join_style = JOIN_MITER;	/* miter joint */
	l->cap_style = CAP_BUTT;	/* butt line cap */
    }
    if ((!radius_flag && n != 10) ||
	(radius_flag && ((proto == 21 && n != 11) ||
			((proto >= 30) && n != 15)))) {
	    file_msg(Err_incomp, "line", save_line);
	    free((char *) l);
	    return NULL;
    }
    l->fill_style = FILL_CONVERT(l->fill_style);
    fix_depth(&l->depth);
    check_color(&l->pen_color);
    check_color(&l->fill_color);
    fix_fillstyle(l);	/* make sure that black/white have legal fill styles */
    /* forward arrow */
    if (fa) {
	if (read_line(fp) == -1)
	    return NULL;
	if (sscanf(buf, "%d%d%f%f%f", &type, &style, &thickness, &wd, &ht) != 5) {
	    file_msg(Err_incomp, "line", save_line);
	    return NULL;
	}
	/* make sure arrowhead is legal and convert units */
	convert_arrow(&type, &wd, &ht);
	l->for_arrow = new_arrow(type, style, thickness, wd, ht);
    }
    /* backward arrow */
    if (ba) {
	if (read_line(fp) == -1)
	    return NULL;
	if (sscanf(buf, "%d%d%f%f%f", &type, &style, &thickness, &wd, &ht) != 5) {
	    file_msg(Err_incomp, "line", save_line);
	    return NULL;
	}
	/* make sure arrowhead is legal and convert units */
	convert_arrow(&type, &wd, &ht);
	l->back_arrow = new_arrow(type, style, thickness, wd, ht);
    }
    if (l->type == T_PICTURE) {
	char s1[PATH_MAX];

	if (read_line(fp) == -1) {
	    free((char *) l);
	    return NULL;
	}
	if ((l->pic = create_pic()) == NULL) {
	    free((char *) l);
	    return NULL;
	}
	if (sscanf(buf, "%d %[^\n]", &l->pic->flipped, s1) != 2) {
	    file_msg(Err_incomp, "Picture Object", save_line);
	    free((char *) l);
	    return NULL;
	}

	/* if path is relative convert it to absolute path */
	if (s1[0] != '/')
	    sprintf(picfile, "%s/%s", cur_file_dir, s1);
	else
	    strcpy(picfile, s1);

	if (!update_figs) {
	    /* only read in the image if update_figs is False */
	    read_picobj(l->pic, picfile, l->pen_color, False, &dum);
	} else {
	    /* otherwise just make a pseudo entry with the filename */
	    l->pic->pic_cache = create_picture_entry();
	    l->pic->pic_cache->file = strdup(picfile);
	}
	/* we've read in a pic object - merge_file uses this info to decide
	   whether or not to remap any picture colors in first figure */
	pic_obj_read = True;
    } else
	l->pic = NULL;

    if ((p = create_point()) == NULL) {
	free((char *) l);
	return NULL;
    }

    l->points = p;
    p->next = NULL;

    /* read first point */
    line_no++;
    if (fscanf(fp, "%d%d", &p->x, &p->y) != 2) {
	file_msg(Err_incomp, "line", save_line);
	free_linestorage(l);
	return NULL;
    }
    ox = p->x;
    oy = p->y;
    /* read subsequent points */
    if (proto < 22)
	npts = 1000000;	/* loop until we find 9999 9999 for previous fig files */
    cnpts = 1;		/* keep track of actual number of points read */
    for (--npts; npts > 0; npts--) {
	count_lines_correctly(fp);
	if (fscanf(fp, "%d%d", &x, &y) != 2) {
	    file_msg(Err_incomp, "line", save_line);
	    free_linestorage(l);
	    return NULL;
	}
	if (proto < 22 && x == 9999)
	    break;
	/* ignore identical consecutive points */
	if (ox == x && oy == y)
	    continue;
	ox = x;
	oy = y;
	if ((q = create_point()) == NULL) {
	    free_linestorage(l);
	    return NULL;
	}
	q->x = x;
	q->y = y;
	q->next = NULL;
	p->next = q;
	p = q;
	cnpts++;
    }
    /* also, if it has fewer than 5 points and is a box, picture, or arcbox, 
       or if it has fewer than 3 points and it is a polygon remove it */
    if ((cnpts < 5 && (l->type == T_BOX || l->type == T_ARCBOX || l->type == T_PICTURE)) ||
	(cnpts < 3 && l->type == T_POLYGON)) {
	    if (l->type == T_POLYGON) {
		file_msg("Deleting polygon containing fewer than 3 points at line %d",
			save_line);
	    } else {
		file_msg("Deleting zero-size %s at line %d",
			l->type==T_BOX? "box" : l->type==T_ARCBOX? "arcbox" : "picture",
			save_line);
	    }
	    free_linestorage(l);
	    return NULL;
    }
    /* if the line has only one point, delete any arrowheads it might have now */
    if (l->points->next == NULL) {
	if (l->for_arrow) {
	    free((char *) l->for_arrow);
	    l->for_arrow = (F_arrow *) NULL;
	}
	if (l->back_arrow) {
	    free((char *) l->back_arrow);
	    l->back_arrow = (F_arrow *) NULL;
	}
    }
    l->comments = attach_comments();		/* attach any comments */
    /* skip to the next line */
    skip_line(fp);
    return l;
}

static F_spline *
read_splineobject(FILE *fp)
{
    F_spline	   *s;
    F_point	   *p, *q;
    F_sfactor	   *cp, *cq;
    int		    c, n, x, y, fa, ba, npts, numpts;
    int		    type, style;
    float	    thickness, wd, ht;
    double	    s_param;
    float	    lx, ly, rx, ry;

    if ((s = create_spline()) == NULL)
	return NULL;

    save_line = line_no;
    s->points = NULL;
    s->sfactors = NULL;
    s->for_arrow = s->back_arrow = NULL;
    s->next = NULL;

    /* 3.0(experimental 2.2) or later has number of points parm for all spline
	objects and fill color separate from border color */
    if (proto >= 30) {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%d%f%d%d%d%d",
		    &s->type, &s->style, &s->thickness, &s->pen_color, &s->fill_color,
		    &s->depth, &s->pen_style, &s->fill_style, &s->style_val,
		    &s->cap_style, &fa, &ba, &npts);
    } else {
	    n = sscanf(buf, "%*d%d%d%d%d%d%d%d%f%d%d",
		    &s->type, &s->style, &s->thickness, &s->pen_color,
		    &s->depth, &s->pen_style, &s->fill_style, &s->style_val, &fa, &ba);
	    s->fill_color = s->pen_color;
	    s->cap_style = CAP_BUTT;	/* butt line cap */
    }
    if (((proto < 22) && (n != 10)) || ((proto >= 30) && n != 13)) {
	file_msg(Err_incomp, "spline", save_line);
	free((char *) s);
	return NULL;
    }
    s->fill_style = FILL_CONVERT(s->fill_style);
    fix_depth(&s->depth);
    check_color(&s->pen_color);
    check_color(&s->fill_color);
    fix_fillstyle(s);	/* make sure that black/white have legal fill styles */
    /* forward arrow */
    if (fa) {
	if (read_line(fp) == -1)
	    return NULL;
	if (sscanf(buf, "%d%d%f%f%f", &type, &style, &thickness, &wd, &ht) != 5) {
	    file_msg(Err_incomp, "spline", save_line);
	    return NULL;
	}
	/* make sure arrowhead is legal and convert units */
	convert_arrow(&type, &wd, &ht);
	s->for_arrow = new_arrow(type, style, thickness, wd, ht);
    }
    /* backward arrow */
    if (ba) {
	if (read_line(fp) == -1)
	    return NULL;
	if (sscanf(buf, "%d%d%f%f%f", &type, &style, &thickness, &wd, &ht) != 5) {
	    file_msg(Err_incomp, "spline", save_line);
	    return NULL;
	}
	/* make sure arrowhead is legal and convert units */
	convert_arrow(&type, &wd, &ht);
	s->back_arrow = new_arrow(type, style, thickness, wd, ht);
    }

    /* read first point */
    line_no++;
    if ((n = fscanf(fp, "%d%d", &x, &y)) != 2) {
	file_msg(Err_incomp, "spline", save_line);
	free_splinestorage(s);
	return NULL;
    };
    if ((p = create_point()) == NULL) {
	free_splinestorage(s);
	return NULL;
    }
    s->points = p;
    p->x = x;
    p->y = y;
    c = 1;
    /* read subsequent points */
    if (proto < 22)
	npts = 1000000;	/* loop until we find 9999 9999 for previous fig files */
    numpts = 1;
    for (--npts; npts > 0; npts--) {
	count_lines_correctly(fp);
	if (fscanf(fp, "%d%d", &x, &y) != 2) {
	    file_msg(Err_incomp, "spline", save_line);
	    p->next = NULL;
	    free_splinestorage(s);
	    return NULL;
	};
	if (proto < 22 && x == 9999)
	    break;
	if ((q = create_point()) == NULL) {
	    free_splinestorage(s);
	    return NULL;
	}
	q->x = x;
	q->y = y;
	p->next = q;
	p = q;
	c++;
	numpts++;
    }
    p->next = NULL;

    if (proto <= 31) {		/* to read files from version 3.1 and older */
	if int_spline(s) {
	                        /* 2 control points per point given by user in
			           version 3.1 and older : don't read them */
          while (c--) {
            count_lines_correctly(fp);
            if (fscanf(fp, "%f%f%f%f", &lx, &ly, &rx, &ry) != 4) {
              file_msg(Err_incomp, "spline", save_line);
	      free_splinestorage(s);
              return NULL;
            }
          }
        }
	if (closed_spline(s)) {
	    F_point *ptr   = s->points; 
	    s->points = s->points->next;
	    free (ptr);
	}
	if (! make_sfactors(s)) {
	    free_splinestorage(s);
	    return NULL;
	}
	return s;
    }

    /* Read sfactors - the s parameter for splines */
    
    count_lines_correctly(fp);
    if ((n = fscanf(fp, "%lf", &s_param)) != 1) {
	file_msg(Err_incomp, "spline", save_line);
	free_splinestorage(s);
	return NULL;
    };
    if ((cp = create_sfactor()) == NULL) {
	free_splinestorage(s);
	return NULL;
    }
    s->sfactors = cp;
    cp->s = s_param;
    while (--c) {
	count_lines_correctly(fp);
	if (fscanf(fp, "%lf", &s_param) != 1) {
	    file_msg(Err_incomp, "spline", save_line);
	    cp->next = NULL;
	    free_splinestorage(s);
	    return NULL;
	};
	if ((cq = create_sfactor()) == NULL) {
	    cp->next = NULL;
	    free_splinestorage(s);
	    return NULL;
	}
	cq->s=s_param;
	cp->next = cq;
	cp = cq;
    }
    if (closed_spline(s) && numpts < 3) {
	file_msg("Closed splines must have 3 or more points, removing spline at line %d", save_line);
	free_splinestorage(s);
	return NULL;
    } else if (numpts < 2) {
	file_msg("Open splines must have 2 or more points, removing spline at line %d", save_line);
	free_splinestorage(s);
	return NULL;
    }
    cp->next = NULL;
    s->comments = attach_comments();		/* attach any comments */

    /* skip to the end of the line */
    skip_line(fp);
    return s;
}

static F_text  *
read_textobject(FILE *fp)
{
    F_text	   *t;
    int		    l,n,len;
    int		    ignore = 0;
    char	    s[BUF_SIZE], s_temp[BUF_SIZE], junk[2];
    float	    tx_size;
    float	    length, height;
    Boolean	    more;
    PR_SIZE	    tx_dim;

    if ((t = create_text()) == NULL)
	return NULL;

    save_line = line_no;
    t->next = NULL;
    /*
     * The text object is terminated by a CONTROL-A, so we read everything up
     * to the CONTROL-A and then read that character. If we do not find the
     * CONTROL-A on this line then this must be a multi-line text object and
     * we will have to read more.
     *
     * We read text size, height and length as floats because TransFig uses
     * floats for these, but they are rounded to ints internally to xfig.
     */
    /* read the leading blanks for the string, but delete the first one later */

    /*
     * NOTE: The height, length and descent will be recalculated from the
     *	     actual font structure in read_scale_text().
     */

    if (proto >= 30) {	/* order of parms is more like other objects now;
			   string is now terminated with the literal '\001',
			   and 8-bit characters are represented as \xxx */
	n = sscanf(buf, "%*d%d%d%d%d%d%f%f%d%f%f%d%d%[^\n]",
		&t->type, &t->color, &t->depth, &t->pen_style,
		&t->font, &tx_size, &t->angle, &t->flags,
		&height, &length, &t->base_x, &t->base_y, s);
    } else {
	n = sscanf(buf, "%*d%d%d%f%d%d%d%f%d%f%f%d%d%[^\1]%[\1]",
		&t->type, &t->font, &tx_size, &t->pen_style,
		&t->color, &t->depth, &t->angle, &t->flags,
		&height, &length, &t->base_x, &t->base_y, s, junk);
    }
    /* remove newline */
    buf[strlen(buf)-1] = '\0';
    if (buf[strlen(buf)-1] == '\r')
	buf[strlen(buf)-1] = '\0';
    /* remove any trailing carriage returns (^M, possibly from a PC) */
    if (s[strlen(s)-1] == '\r')
	s[strlen(s)-1] = '\0';
    /* use these for now, but recalculate later in read_scale_text if not update_figs */
    t->ascent = round(height);
    t->descent = 0;
    t->length = round(length);

    if (n < 11) {
	file_msg(Err_incomp, "text", save_line);
	free((char *) t);
	return NULL;
    }

    /* now round size to int */

    /* change DEFAULT (-1) or 0 size to default size */
    if ((int) tx_size == DEFAULT || (int) tx_size == 0)
	t->size = DEF_FONTSIZE;
    else
	t->size = round(tx_size);

    /* set some limits */
    if (t->size < MIN_FONT_SIZE)
	t->size = MIN_FONT_SIZE;
    else if (t->size > MAX_FONT_SIZE)
	t->size = MAX_FONT_SIZE;

    /* make sure angle is 0 to 2PI */
    fix_angle(&t->angle);

    /* convert all pre-2.1 NON-TFX text flags (used to be font_style) to PostScript
       and all pre-2.1 TFX flags to PostScript + Special */
    if (proto <= 20) {
	t->flags = PSFONT_TEXT;
	if (TFX)
		t->flags |= SPECIAL_TEXT;
    }

    /* check for valid font number */
    if (t->font >= MAXFONT(t)) {
	file_msg("Invalid text font (%d) at line %d, setting to DEFAULT.",
		t->font, save_line);
	t->font = DEFAULT;
    }

    /* get the UNZOOMED font struct */
    if (!update_figs)
	t->fontstruct = lookfont(x_fontnum(psfont_text(t), t->font), t->size);

    fix_depth(&t->depth);
    check_color(&t->color);
    more = False;
    if (proto < 22 && n == 13)
	more = True;		/* in older xfig there is more if ^A wasn't found yet */
    else if (proto >= 30) { 	/* in 3.0(2.2) there is more if \001 wasn't found */
	len = strlen(s);
	if ((strcmp(&s[len-4],"\\001") == 0) &&	/* if we find '\000' */
	    !(backslash_count(s, len-5) % 2)) { /* and not '\\000' */
		more = False;			/* then there are no more lines */
		s[len-4]='\0';			/* and get rid of the '\001' */
	} else {
		more = True;
		s[len++]='\n';			/* put back the end of line char */
		s[len] = '\0';			/* and terminate it */
	}
    }
    if (more) {
	/* Read in the subsequent lines of the text object if there is more than one. */
	do {
	    line_no++;		/* As is done in read_line */
	    if (fgets(buf, BUF_SIZE, fp) == NULL)
		break;
	    /* remove newline */
	    buf[strlen(buf)-1] = '\0';
	    if (buf[strlen(buf)-1] == '\r')
		buf[strlen(buf)-1] = '\0';
	    if (proto < 22) {
		n = sscanf(buf, "%[^\1]%[\1]", s_temp, junk);
	    } else {
		strcpy(s_temp,buf);
		len = strlen(s_temp);
		if ((strncmp(&s_temp[len-4],"\\001",4) == 0) &&
		    !(backslash_count(s, len-5) % 2)) {
			n=0;			/* found the '\001', set n to stop */
			s_temp[len-4]='\0';	/* and get rid of the '\001' */
		} else {
			n=1;			/* keep going (more lines) */
		}
	    }
	    /* Safety check */
	    if (strlen(s) + 1 + strlen(s_temp) + 1 > BUF_SIZE) {
		/* Too many characters.	 Ignore the rest. */
		if (!ignore)
		    file_msg("Truncating TEXT object to %d chars in line %d.",
				BUF_SIZE, save_line);
		ignore = 1;
	    }
	    if (!ignore)
		strcat(s, s_temp);
	} while (n == 1);
    }
    if (proto >= 30) {
	/* now convert any \xxx to ascii characters */
	if (strchr(s,'\\')) {
		int num;
		len = strlen(s);
		for (l=0,n=0; l < len; l++) {
		    if (s[l]=='\\') {
			/* a backslash, see if a digit follows */
			if (l < len && isdigit(s[l+1])) {
			    /* yes, allow exactly 3 digits following the \ for the octal value */
			    if (sscanf(&s[l+1],"%3o",&num)!=1) {
				file_msg("Error in parsing text string on line.", save_line);
				free((char *) t);
				return NULL;
			    }
			    buf[n++]= (unsigned char) num;	/* put char in */
			    l += 3;			/* skip over digits */
			} else {
			    buf[n++] = s[++l];		/* some other escaped character */
			}
		    } else {
			buf[n++] = s[l];		/* ordinary character */
		    }
		}
		buf[n]='\0';		/* terminate */
		strcpy(s,buf);		/* copy back to s */
	}
    }

    if (t->type > T_RIGHT_JUSTIFIED) {
	file_msg("Invalid text justification at line %d, setting to LEFT.", save_line);
	t->type = T_LEFT_JUSTIFIED;
    }

    if (strlen(s) <= 1) {
	s[0]=' ';s[1]=0;
    }
    /* skip first blank from input file by starting at s[1] */
    if ((t->cstring = new_string(strlen(&s[1]))) == NULL) {
	free((char *) t);
	return NULL;
    }
    /* copy string to text object */
    (void) strcpy(t->cstring, &s[1]);

    if (!update_figs) {
	/* now calculate the actual length and height of the string in fig units */
	tx_dim = textsize(t->fontstruct, strlen(t->cstring), t->cstring);
	t->length = round(tx_dim.length);
	t->ascent = round(tx_dim.ascent);
	t->descent = round(tx_dim.descent);
	/* now get the zoomed font struct */
	t->zoom = zoomscale;
	if (display_zoomscale != 1.0)
	    t->fontstruct = lookfont(x_fontnum(psfont_text(t), t->font),
				round(t->size*display_zoomscale));
    }

    t->comments = attach_comments();		/* attach any comments */
    return t;
}

/* akm 28/2/95 - count consecutive backslashes backwards */
int
backslash_count(char *cp, int start)
{
  int i, count = 0;

  for(i=start; i>=0; i--) {
    if (cp[i] == '\\')
	count++;
    else
	break;
  }
  return count;
}

/* attach comments together */ 

static char *
attach_comments(void)
{
    int		    i,len;
    char	   *comp;

    if (appres.DEBUG && (numcom > 0))
	fprintf(stderr,"Comments:\n");
    /* add up length of all comment lines */
    len = 0;
    for (i=0; i<numcom; i++)
	len += strlen(comments[i])+1;
    if ((comp = new_string(len)) == NULL) {
	numcom = 0;
	return NULL;
    }
    /* now make them into one string */
    comp[0] = '\0';
    for (i=0; i<numcom; i++) {
	if (appres.DEBUG)
	    fprintf(stderr,"%2d: %s\n",i,comments[i]);
	strcat(comp, comments[i]);
	/* don't put trailing newline for last comment line */
	if (i<numcom-1)
	    strcat(comp,"\n");
    }
    /* reset comment number */
    numcom = 0;
    return comp;
}

int read_line(FILE *fp)
{
    while (1) {
	if (NULL == fgets(buf, BUF_SIZE, fp)) {
	    return -1;
	}
	line_no++;
	if (*buf == '#') {		/* save any comments */
	    if (save_comment(fp) < 0)
		return -1;
	} else if (*buf != '\n')	/* Skip empty lines */
	    return 1;
    }
}

/* save a comment line to be stored with the *subsequent* object */

int save_comment(FILE *fp)
{
    int		    i;

    i=strlen(buf);
    /* see if we've allocated space for this comment */
    if (comments[numcom])
	free(comments[numcom]);
    if ((comments[numcom] = (char*) new_string(i+1)) == NULL)
	    return -1;
    /* remove any newline */
    if (buf[i-1] == '\n')
	buf[i-1] = '\0';
    i=1; 
    if (buf[1] == ' ')	/* remove one leading blank from the comment, if there is one */
	i=2;
    if (numcom < MAXCOMMENTS)
	strcpy(comments[numcom++], &buf[i]);
    return 1;
}

/* skip to the end of the current line */

void skip_line(FILE *fp)
{
    while (fgetc(fp) != '\n') {
	if (feof(fp))
	    return;
    }
}

/* make sure angle is 0 to 2PI */

void fix_angle(float *angle)
{
    while (*angle < 0.0)
	*angle += M_2PI;
    while (*angle >= M_2PI)
	*angle -= M_2PI;
}

void fix_depth(int *depth)
{
    if (*depth>MAX_DEPTH) {
	    *depth=MAX_DEPTH;
	    file_msg("Depth > Maximum allowed (%d), setting to %d in line %d.",
			MAX_DEPTH, save_line, MAX_DEPTH);
	}
	else if (*depth<0 || proto<21) {
	    *depth=0;
	    if (proto>=21)
		file_msg("Depth < 0, setting to 0 in line %d.", save_line);
	}
}

char shift_msg[] = "The figure has objects which have negative coordinates,\ndo you wish to shift it back on the page?";

void shift_figure(F_compound *obj)
{
    F_ellipse	   *e;
    F_arc	   *a;
    F_line	   *l;
    F_spline	   *s;
    F_compound	   *c;
    F_text	   *t;
    int		    lowx,lowy,dx,dy;
    int		    rnd;

    /* if user is allowing negative coords, return */
    if (appres.allownegcoords)
	return;

    lowx = obj->nwcorner.x;
    lowy = obj->nwcorner.y;

    /* check if any part of the figure has negative coords */
    if (lowx >= 0 && lowy >= 0)
	return;				/* no, ok */

    /* ask the user */
    if (!preview_in_progress && (popup_query(QUERY_YESNO, shift_msg)==RESULT_NO))
	return;

    /* shift the whole figure to keep it "on the page" */
    dx = dy = 0;
    rnd = posn_rnd[cur_gridunit][cur_pointposn];
    if (lowx < 0) {
	dx = -lowx+rnd;	/* and round up to small grid */
    }
    if (lowy < 0) {
	dy = -lowy+rnd;
    }
    if (!preview_in_progress)
	file_msg("Shifting entire figure %d units right and %d units down to keep on page.",
		    dx,dy);
    for (e = obj->ellipses; e != NULL; e = e->next)
	translate_ellipse(e, dx, dy);
    for (a = obj->arcs; a != NULL; a = a->next)
	translate_arc(a, dx, dy);
    for (l = obj->lines; l != NULL; l = l->next)
	translate_line(l, dx, dy);
    for (s = obj->splines; s != NULL; s = s->next)
	translate_spline(s, dx, dy);
    for (c = obj->compounds; c != NULL; c = c->next)
	translate_compound(c, dx, dy);
    for (t = obj->texts; t != NULL; t = t->next)
	translate_text(t, dx, dy);
}

void scale_figure(F_compound *obj, float mul, int offset)
{
    /* scale the whole figure for new pixels per inch */
    if (mul != 1.0)
	put_msg("Scaling figure by a factor of %.1f for new %d pixel per inch resolution.",
		mul,PIX_PER_INCH);
    read_scale_ellipses(obj->ellipses, mul, offset);
    read_scale_arcs(obj->arcs, mul, offset);
    read_scale_lines(obj->lines, mul, offset);
    read_scale_splines(obj->splines, mul, offset);
    read_scale_compounds(obj->compounds, mul, offset);
    read_scale_texts(obj->texts, mul, offset);

}

/* check if user color <color> is defined */

void check_color(int *color)
{
    if (*color < NUM_STD_COLS)
	return;
    if (!n_colorFree[*color-NUM_STD_COLS])
	return;
    file_msg("Cannot locate user color %d, using default color for line %d.",
		*color,line_no);
    *color = DEFAULT;
    return;
}

/* swap new colors (n_...) with current for file load or undo load */

void swap_colors(void)
{
    int		i,num;
    Boolean	saveFree[MAX_USR_COLS];

    if (appres.DEBUG)
	fprintf(stderr,"Swapping colors. Before: colors_are_swapped = %d\n",colors_are_swapped);

    colors_are_swapped = True;

    /* first save the current colors because del_color_cell destroys them */
    for (i=0; i<num_usr_cols; i++)
	save_colors[i] = user_colors[i];
    /* and save Free entries */
    for (i=0; i<num_usr_cols; i++)
	saveFree[i] = colorFree[i];
    /* now free any previously defined user colors */
    for (i=0; i<num_usr_cols; i++) {
	    del_color_cell(i);		/* remove widget and colormap entry */
    }
    /* now swap old colors with new */
    for (i=0; i<n_num_usr_cols; i++)
	user_colors[i] = n_user_colors[i];
    for (i=0; i<num_usr_cols; i++)
	n_user_colors[i] = save_colors[i];
    /* and swap Free entries */
    for (i=0; i<n_num_usr_cols; i++)
	colorFree[i] = n_colorFree[i];
    for (i=0; i<num_usr_cols; i++)
	n_colorFree[i] = saveFree[i];

    num = num_usr_cols;
    num_usr_cols = n_num_usr_cols;
    n_num_usr_cols = num;

    /* now try to allocate the new colors */
    if (num_usr_cols > 0) {
	num = num_usr_cols;
	num_usr_cols = 0;
	/* fill the colormap and the color memories */
	for (i=0; i<num; i++) {
	    if (colorFree[i]) {
		colorUsed[i] = False;
	    } else {
		/* and add a widget and colormap entry */
		if (add_color_cell(USE_EXISTING_COLOR, i, user_colors[i].red/256,
			user_colors[i].green/256,
			user_colors[i].blue/256) == -1) {
			    file_msg("Can't allocate more than %d user colors, not enough colormap entries",
					num_usr_cols);
			    return;
			}
	        colorUsed[i] = True;
	    }
	}
    }
}

/* Merge any user colors from the new file just merged into the current figure.
   Look through the new color definitions n_... and see if any color numbers
   conflict with the current color defs. If so, renumber those to new, free
   color numbers.
   This is called when doing a "merge read" or a "paste" function. */

static int    renum[MAX_USR_COLS];

void merge_colors(F_compound *objects)
{
    Boolean	    found_exist;
    int		    i,j,newval;
    int		    x_colorFree[MAX_USR_COLS];

    if (n_num_usr_cols == 0)
	return;

    newval = -1;

    /* look for the first free color number */
    for (i=0; i<MAX_USR_COLS; i++)
	if (colorFree[i])
	    break;

    if (i<MAX_USR_COLS)
	newval = i;

    /* we need a tri-state value for n_colorFree[] so make local copy */
    for (i=0; i<MAX_USR_COLS; i++)
	x_colorFree[i] = (int) n_colorFree[i];

    for (i=0; i<n_num_usr_cols; i++) {
	renum[i] = -1;
	if (x_colorFree[i] == 0) {
		x_colorFree[i] = 1;	/* we're not using this number anymore */
		/* see if it is identical to an existing color */
		found_exist = False;
		for (j=0; j<num_usr_cols; j++)
		    /* compare only the upper 8-bits because the server may change the lower */
		    if (colorUsed[j] && 
			 (user_colors[j].red>>8 == n_user_colors[i].red>>8) &&
			 (user_colors[j].green>>8 == n_user_colors[i].green>>8) &&
			 (user_colors[j].blue>>8 == n_user_colors[i].blue>>8)) {
			    renum[i] = j;	/* yes, use it */
			    found_exist=True;
			    break;		/* skip to next */
		    }
		if (!found_exist) {
		    if (newval == -1) {
			renum[i] = 0;		/* out of user colors, use 0 */
		    } else {
			renum[i] = newval;	/* assign it a new color number */
			/* find the next free color number */
			while (newval < MAX_USR_COLS) {
			    newval++;
			    if (colorFree[newval])
				break;
			}
			if (newval >= MAX_USR_COLS)
			    newval = -1;
		    }
		    x_colorFree[renum[i]] = -1;	/* we are using this number now */
		    n_user_colors[renum[i]] = n_user_colors[i];	/* copy rgb values */
		}
	}
    }

    /* renumber them now */
    n_num_usr_cols = max2(newval,num_usr_cols);	/* new upper limit on color number */
    renumber_comp(objects);

    /* now create colorcells for the new colors */
    for (i=0; i<n_num_usr_cols; i++) {
	if (x_colorFree[i] != 1) {
	    user_colors[i] = n_user_colors[i];
	    /* and add a widget and colormap entry */
	    if (add_color_cell(USE_EXISTING_COLOR, i,
		user_colors[i].red/256,
		user_colors[i].green/256,
		user_colors[i].blue/256) == -1) {
		    file_msg("Can't allocate more than %d user colors, not enough colormap entries",
				n_num_usr_cols);
		    return;
	    }
	    colorFree[i] = False;
	    colorUsed[i] = True;
	}
    }
    num_usr_cols = n_num_usr_cols;
}

void renumber_comp(F_compound *compound)
{
	F_arc	   *a;
	F_text	   *t;
	F_compound *c;
	F_ellipse  *e;
	F_line	   *l;
	F_spline   *s;

	/* first renumber colors in the compounds */
	for (c = compound->compounds; c != NULL; c = c->next)
	    renumber_comp(c);

	/* now the primitives */
	for (a = compound->arcs; a != NULL; a = a->next) {
	    renumber(&a->fill_color);
	    renumber(&a->pen_color);
	}
	for (t = compound->texts; t != NULL; t = t->next) {
	    renumber(&t->color);
	}
	for (e = compound->ellipses; e != NULL; e = e->next) {
	    renumber(&e->fill_color);
	    renumber(&e->pen_color);
	}
	for (l = compound->lines; l != NULL; l = l->next) {
	    renumber(&l->fill_color);
	    renumber(&l->pen_color);
	}
	for (s = compound->splines; s != NULL; s = s->next) {
	    renumber(&s->fill_color);
	    renumber(&s->pen_color);
	}
}

void renumber(int *color)
{
    if (*color < NUM_STD_COLS)
	return;
    if (renum[*color-NUM_STD_COLS] != -1)
	*color = renum[*color-NUM_STD_COLS]+NUM_STD_COLS;
}

/* this function is to count line numbers correctly while reading
 * input files.
 * It skips all tabs and spaces and increments the global
 * variable line_no if a newline was found.
 * If any other character is read, it is put back to the input
 * stream and the function returns.
 * It should be called from within the point reading loops
 * in the read_{line,spline}object functions, where the point
 * coordinates may be given in an arbitrary number of lines.
 * Added by Andreas_Bagge@maush2.han.de (A.Bagge), 14.12.94
 */

static void
count_lines_correctly(FILE *fp)
{
    int cc;
    do{
	cc=getc(fp);
	if (cc=='\n') {
	   line_no++;
	   cc=getc(fp);
	}
    } while (cc==' '||cc=='\t');
    ungetc(cc,fp);
}

/* make sure arrow style value is legal and convert arrow width and height to
 * same units as thickness in V4.0 and later we will save the values in these units */

void convert_arrow(int *type, float *wd, float *ht)
{
    if (*type >= NUM_ARROW_TYPES/2)
	*type = 0;
    if (proto < 40) {
	*wd /= ZOOM_FACTOR;
	*ht /= ZOOM_FACTOR;
    }
}
