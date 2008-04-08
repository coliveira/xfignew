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
#include "e_edit.h"
#include "u_create.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"

#include "e_scale.h"
#include "u_free.h"
#include "u_list.h"
#include "w_cursor.h"
#include "w_modepanel.h"
#include "w_mousefun.h"

static char	Err_mem[] = "Running out of memory.";

/****************** ARROWS ****************/



F_arrow *
create_arrow(void)
{
    F_arrow	   *a;

    if ((a = (F_arrow *) malloc(ARROW_SIZE)) == NULL)
	put_msg(Err_mem);
    return a;
}

F_arrow	       *
forward_arrow(void)
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }

    a->type = ARROW_TYPE(cur_arrowtype);
    a->style = ARROW_STYLE(cur_arrowtype);
    if (use_abs_arrowvals) {
	a->thickness = cur_arrowthick;
	a->wd = cur_arrowwidth;
	a->ht = cur_arrowheight;
    } else {
	a->thickness = cur_arrow_multthick*cur_linewidth;
	a->wd = cur_arrow_multwidth*cur_linewidth;
	a->ht = cur_arrow_multheight*cur_linewidth;
    }
    return a;
}

F_arrow	       *
backward_arrow(void)
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }

    a->type = ARROW_TYPE(cur_arrowtype);
    a->style = ARROW_STYLE(cur_arrowtype);
    if (use_abs_arrowvals) {
	a->thickness = cur_arrowthick;
	a->wd = cur_arrowwidth;
	a->ht = cur_arrowheight;
    } else {
	a->thickness = cur_arrow_multthick*cur_linewidth;
	a->wd = cur_arrow_multwidth*cur_linewidth;
	a->ht = cur_arrow_multheight*cur_linewidth;
    }
    return a;
}

F_arrow	       *
new_arrow(int type, int style, float thickness, float wd, float ht)
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }

    /* check arrow type for legality */
    if (type > NUM_ARROW_TYPES/2) { /* type*2+style = NUM_ARROW_TYPES */
	type = style = 0;
    }
    /* only open or filled allowed */
    if (style > 1)
	style = 0;

    /* if thickness is <= 0 or > 10 inches, make reasonable values */
    if (thickness <= 0.0 || thickness > 10.0 * DISPLAY_PIX_PER_INCH)
	thickness = cur_arrowthick;
    /* if width is <= 0 or > 20 inches, make reasonable values */
    if (wd <= 0.0)
	wd = cur_arrowwidth;
    /* if height is < 0 or > 50 inches, make reasonable values */
    if (ht < 0.0 || ht > 50.0 * DISPLAY_PIX_PER_INCH)
	ht = cur_arrowheight;
    a->type = type;
    a->style = style;
    a->thickness = thickness;
    a->wd = wd;
    a->ht = ht;
    return a;
}

/************************ COMMENTS *************************/

void
copy_comments(char **source, char **dest)
{
    if (*source == NULL) {
	*dest = NULL;
	return;
    }
    if ((*dest = (char*) new_string(strlen(*source))) == NULL)
	return;
    strcpy(*dest,*source);
}

/************************ SMART LINKS *************************/

F_linkinfo     *
new_link(F_line *l, F_point *ep, F_point *pp)
{
    F_linkinfo	   *k;

    if ((k = (F_linkinfo *) malloc(LINKINFO_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    k->line = l;
    k->endpt = ep;
    k->prevpt = pp;
    k->next = NULL;
    return k;
}

/************************ POINTS *************************/

F_point	       *
create_point(void)
{
    F_point	   *p;

    if ((p = (F_point *) malloc(POINT_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    p->x = 0;
    p->y = 0;
    p->next = NULL;
    return p;
}

F_sfactor      *
create_sfactor(void)
{
    F_sfactor	   *cp;

    if ((cp = (F_sfactor *) malloc(CONTROL_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    cp->next = NULL;
    return cp;
}

F_point	       *
copy_points(F_point *orig_pt)
{
    F_point	   *new_pt, *prev_pt, *first_pt;

    if ((new_pt = create_point()) == NULL)
	return NULL;

    first_pt = new_pt;
    *new_pt = *orig_pt;
    new_pt->next = NULL;
    prev_pt = new_pt;
    for (orig_pt = orig_pt->next; orig_pt != NULL; orig_pt = orig_pt->next) {
	if ((new_pt = create_point()) == NULL) {
	    free_points(first_pt);
	    return NULL;
	}
	prev_pt->next = new_pt;
	*new_pt = *orig_pt;
	new_pt->next = NULL;
	prev_pt = new_pt;
    }
    return first_pt;
}

F_sfactor *
copy_sfactors(F_sfactor *orig_sf)
{
    F_sfactor	 *new_sf, *prev_sf, *first_sf;

    if ((new_sf = create_sfactor()) == NULL)
	return NULL;

    first_sf = new_sf;
    *new_sf = *orig_sf;
    new_sf->next = NULL;
    prev_sf = new_sf;
    for (orig_sf = orig_sf->next; orig_sf != NULL; orig_sf = orig_sf->next) {
	if ((new_sf = create_sfactor()) == NULL) {
	    free_sfactors(first_sf);
	    return NULL;
	}
	prev_sf->next = new_sf;
	*new_sf = *orig_sf;
	new_sf->next = NULL;
	prev_sf = new_sf;
    }
    return first_sf;
}

/* reverse points in list */

void
reverse_points(F_point *orig_pt)
{
    F_point	   *cur_pt;
    int		    npts,i;
    F_point	   *tmp_pts;

    /* count how many points are in the list */
    cur_pt = orig_pt;
    for (npts=0; cur_pt; cur_pt=cur_pt->next)
	npts++;
    /* make a temporary stack (array) */
    tmp_pts = (F_point *) malloc(npts*sizeof(F_point));
    cur_pt = orig_pt;
    /* and put them on in reverse order */
    for (i=npts-1; i>=0; i--) {
	tmp_pts[i].x = cur_pt->x;
	tmp_pts[i].y = cur_pt->y;
	cur_pt = cur_pt->next;
    }
    /* now reverse them */
    cur_pt = orig_pt;
    for (i=0; i<npts; i++) {
	cur_pt->x = tmp_pts[i].x;
	cur_pt->y = tmp_pts[i].y;
	cur_pt = cur_pt->next;
    }
    /* free the temp array */
    free(tmp_pts);
}

/* reverse sfactors in list */

void
reverse_sfactors(F_sfactor *orig_sf)
{
    F_sfactor	   *cur_sf;
    int		    nsf,i;
    F_sfactor	   *tmp_sf;

    /* count how many sfactors are in the list */
    cur_sf = orig_sf;
    for (nsf=0; cur_sf; cur_sf=cur_sf->next)
	nsf++;
    /* make a temporary stack (array) */
    tmp_sf = (F_sfactor *) malloc(nsf*sizeof(F_sfactor));
    cur_sf = orig_sf;
    /* and put them on in reverse order */
    for (i=nsf-1; i>=0; i--) {
	tmp_sf[i].s = cur_sf->s;
	cur_sf = cur_sf->next;
    }
    /* now reverse them */
    cur_sf = orig_sf;
    for (i=0; i<nsf; i++) {
	cur_sf->s = tmp_sf[i].s;
	cur_sf = cur_sf->next;
    }
    /* free the temp array */
    free(tmp_sf);
}

/************************ ARCS *************************/

F_arc	       *
create_arc(void)
{
    F_arc	   *a;

    if ((a = (F_arc *) malloc(ARCOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    a->tagged = 0;
    a->next = NULL;
    a->type = 0;
    a->for_arrow = NULL;
    a->back_arrow = NULL;
    a->comments = NULL;
    a->depth = 0;
    a->thickness = 0;
    a->pen_color = BLACK;
    a->fill_color = DEFAULT;
    a->fill_style = UNFILLED;
    a->pen_style = -1;
    a->style = SOLID_LINE;
    a->style_val = 0.0;
    a->cap_style = CAP_BUTT;
    a->direction = 0;
    a->angle = 0.0;
    return a;
}

F_arc	       *
copy_arc(F_arc *a)
{
    F_arc	   *arc;
    F_arrow	   *arrow;

    if ((arc = create_arc()) == NULL)
	return NULL;

    /* copy static items first */
    *arc = *a;
    arc->next = NULL;

    /* do comments next */
    copy_comments(&a->comments, &arc->comments);

    if (a->for_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) arc);
	    return NULL;
	}
	arc->for_arrow = arrow;
	*arrow = *a->for_arrow;
    }
    if (a->back_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) arc);
	    return NULL;
	}
	arc->back_arrow = arrow;
	*arrow = *a->back_arrow;
    }
    return arc;
}

/************************ ELLIPSES *************************/

F_ellipse      *
create_ellipse(void)
{
    F_ellipse	   *e;

    if ((e = (F_ellipse *) malloc(ELLOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    e->tagged = 0;
    e->next = NULL;
    e->comments = NULL;
    return e;
}

F_ellipse      *
copy_ellipse(F_ellipse *e)
{
    F_ellipse	   *ellipse;

    if ((ellipse = create_ellipse()) == NULL)
	return NULL;

    /* copy static items first */
    *ellipse = *e;
    ellipse->next = NULL;

    /* do comments next */
    copy_comments(&e->comments, &ellipse->comments);

    return ellipse;
}

/************************ LINES *************************/

F_line	       *
create_line(void)
{
    F_line	   *l;

    if ((l = (F_line *) malloc(LINOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    l->tagged = 0;
    l->next = NULL;
    l->pic = NULL;
    l->for_arrow = NULL;
    l->back_arrow = NULL;
    l->points = NULL;
    l->radius = DEFAULT;
    l->comments = NULL;
    return l;
}

F_pic	       *
create_pic(void)
{
    F_pic	   *pic;

    if ((pic = (F_pic *) malloc(PIC_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    pic->mask = (Pixmap) 0;
    pic->New = False;
    pic->pic_cache = NULL;
    return pic;
}

/* create a new picture entry for the repository */

struct _pics *
create_picture_entry(void)
{
    struct _pics *picture;

    picture = malloc(sizeof(struct _pics));

    picture->file = picture->realname = (unsigned char *) NULL;
    picture->bitmap = (unsigned char *) NULL;
    picture->transp = TRANSP_NONE;
    picture->numcols = 0;
    picture->refcount = 0;
    picture->prev = picture->next = NULL;
    if (appres.DEBUG)
	fprintf(stderr,"create picture entry %x\n",(int) picture);
    return picture;
}

F_line	       *
copy_line(F_line *l)
{
    F_line	   *line;
    F_arrow	   *arrow;
    int		    width, height;
    GC		    one_bit_gc;

    if ((line = create_line()) == NULL)
	return NULL;

    /* copy static items first */
    *line = *l;
    line->next = NULL;

    /* do comments next */
    copy_comments(&l->comments, &line->comments);

    if (l->for_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) line);
	    return NULL;
	}
	line->for_arrow = arrow;
	*arrow = *l->for_arrow;
    }
    if (l->back_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) line);
	    return NULL;
	}
	line->back_arrow = arrow;
	*arrow = *l->back_arrow;
    }
    line->points = copy_points(l->points);
    if (NULL == line->points) {
	put_msg(Err_mem);
	free_linestorage(line);
	return NULL;
    }
    /* copy picture information */
    if (l->pic) {
	if ((line->pic = create_pic()) == NULL) {
	    free((char *) line);
	    return NULL;
	}
	/* copy all the numbers and the pointer to the picture repository (pic->pic_cache) */
	bcopy(l->pic, line->pic, PIC_SIZE);
	/* increase reference count for this picture */
	if (line->pic->pic_cache)
	    line->pic->pic_cache->refcount++;

	width = l->pic->pix_width;
	height = l->pic->pix_height;
	/* copy pixmap */
	if (l->pic->pixmap != 0) {
	    line->pic->pixmap = XCreatePixmap(tool_d, tool_w,
				width, height, tool_dpth);
	    XCopyArea(tool_d, l->pic->pixmap, line->pic->pixmap, gccache[PAINT], 
				0, 0, width, height, 0, 0);
	}
	/* and copy any mask (GIF transparency) */
	if (l->pic->mask != 0) {
            line->pic->mask = XCreatePixmap(tool_d, tool_w, width, height, 1);
	    /* need a 1-bit deep GC to copy it */
	    one_bit_gc = XCreateGC(tool_d, line->pic->mask, (unsigned long) 0, 0);
	    XSetForeground(tool_d, one_bit_gc, 0);
	    XCopyArea(tool_d, l->pic->mask, line->pic->mask, one_bit_gc, 
				0, 0, width, height, 0, 0);
	}
    }
    return line;
}

/************************ SPLINES *************************/

F_spline       *
create_spline(void)
{
    F_spline	   *s;

    if ((s = (F_spline *) malloc(SPLOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    s->tagged = 0;
    s->next = NULL;
    s->comments = NULL;
    return s;
}

F_spline       *
copy_spline(F_spline *s)
{
    F_spline	   *spline;
    F_arrow	   *arrow;

    if ((spline = create_spline()) == NULL)
	return NULL;

    /* copy static items first */
    *spline = *s;
    spline->next = NULL;

    /* do comments next */
    copy_comments(&s->comments, &spline->comments);

    if (s->for_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) spline);
	    return NULL;
	}
	spline->for_arrow = arrow;
	*arrow = *s->for_arrow;
    }
    if (s->back_arrow) {
	if ((arrow = create_arrow()) == NULL) {
	    free((char *) spline);
	    return NULL;
	}
	spline->back_arrow = arrow;
	*arrow = *s->back_arrow;
    }
    spline->points = copy_points(s->points);
    if (NULL == spline->points) {
	put_msg(Err_mem);
	free_splinestorage(spline);
	return NULL;
    }

    if (s->sfactors == NULL)
	return spline;
    spline->sfactors = copy_sfactors(s->sfactors);
    if (NULL == spline->sfactors) {
	put_msg(Err_mem);
	free_splinestorage(spline);
	return NULL;
    }

    return spline;
}

/************************ TEXTS *************************/

F_text	       *
create_text(void)
{
    F_text	   *t;

    if ((t = (F_text *) malloc(TEXOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    t->tagged = 0;
    t->fontstruct = 0;
    t->comments = NULL;
    t->cstring = NULL;
    t->next = NULL;
    return t;
}

/* allocate len+1 characters in a new string */

char	       *
new_string(int len)
{
    char	   *c;

    if ((c = (char *) calloc((unsigned) len + 1, sizeof(char))) == NULL)
	put_msg(Err_mem);
    return c;
}

F_text	       *
copy_text(F_text *t)
{
    F_text	   *text;

    if ((text = create_text()) == NULL)
	return NULL;

    /* copy static items first */
    *text = *t;
    text->next = NULL;

    /* do comments next */
    copy_comments(&t->comments, &text->comments);

    if ((text->cstring = new_string(strlen(t->cstring))) == NULL) {
	free((char *) text);
	return NULL;
    }
    strcpy(text->cstring, t->cstring);
    return text;
}

/************************ COMPOUNDS *************************/

F_compound     *
create_compound(void)
{
    F_compound	   *c;

    if ((c = (F_compound *) malloc(COMOBJ_SIZE)) == NULL) {
	put_msg(Err_mem);
	return NULL;
    }
    c->nwcorner.x = 0;
    c->nwcorner.y = 0;
    c->secorner.x = 0;
    c->secorner.y = 0;
    c->distrib = 0;
    c->tagged = 0;
    c->arcs = NULL;
    c->compounds = NULL;
    c->ellipses = NULL;
    c->lines = NULL;
    c->splines = NULL;
    c->texts = NULL;
    c->comments = NULL;
    c->parent = NULL;
    c->GABPtr = NULL;
    c->next = NULL;

    return c;
}

F_compound     *
copy_compound(F_compound *c)
{
    F_ellipse	   *e, *ee;
    F_arc	   *a, *aa;
    F_line	   *l, *ll;
    F_spline	   *s, *ss;
    F_text	   *t, *tt;
    F_compound	   *cc, *ccc, *compound;

    if ((compound = create_compound()) == NULL)
	return NULL;

    compound->nwcorner = c->nwcorner;
    compound->secorner = c->secorner;
    compound->arcs = NULL;
    compound->ellipses = NULL;
    compound->lines = NULL;
    compound->splines = NULL;
    compound->texts = NULL;
    compound->compounds = NULL;
    compound->next = NULL;

    /* do comments first */
    copy_comments(&c->comments, &compound->comments);

    for (e = c->ellipses; e != NULL; e = e->next) {
	if (NULL == (ee = copy_ellipse(e))) {
	    put_msg(Err_mem);
	    return NULL;
	}
	list_add_ellipse(&compound->ellipses, ee);
    }
    for (a = c->arcs; a != NULL; a = a->next) {
	if (NULL == (aa = copy_arc(a))) {
	    put_msg(Err_mem);
	    return NULL;
	}
	list_add_arc(&compound->arcs, aa);
    }
    for (l = c->lines; l != NULL; l = l->next) {
	if (NULL == (ll = copy_line(l))) {
	    put_msg(Err_mem);
	    return NULL;
	}
	list_add_line(&compound->lines, ll);
    }
    for (s = c->splines; s != NULL; s = s->next) {
	if (NULL == (ss = copy_spline(s))) {
	    put_msg(Err_mem);
	    return NULL;
	}
	list_add_spline(&compound->splines, ss);
    }
    for (t = c->texts; t != NULL; t = t->next) {
	if (NULL == (tt = copy_text(t))) {
	    put_msg(Err_mem);
	    return NULL;
	}
	list_add_text(&compound->texts, tt);
    }
    for (cc = c->compounds; cc != NULL; cc = cc->next) {
	if (NULL == (ccc = copy_compound(cc))) {
	    put_msg(Err_mem);
	    return NULL;
	}
	list_add_compound(&compound->compounds, ccc);
    }
    return compound;
}

/********************** DIMENSION LINES **********************/

/* Make a dimension line given an ordinary line

   It consists of:
   1. the line drawn by the user,
   2. "tick" lines at each endpoint if cur_dimline_ticks == True,
   3. the length in a box overlaid in the middle of the line 
   
   all rolled into a compound object
    
   Call with add_to_figure = True to add to main figure objects
*/

F_compound*
create_dimension_line(F_line *line, Boolean add_to_figure)
{
    F_compound	   *comp;
    F_line	   *box, *tick1, *tick2;
    F_text	   *text;
    F_point	   *pnt;

    /* make a new compound */
    comp = create_compound();
    /* if fixed text, put in an unchanging comment for the compound */
    /* (rescale_dimension_line will create the comment if the text not fixed (=length)) */
    if (cur_dimline_fixed) {
	comp->comments = my_strdup("Dimension line: User-defined text");
    } else {
	comp->comments = my_strdup("Dimension line:");
    }

    /* need two objects *on top of* the basic line */
    if (line->depth < 2) {
	line->depth = 2;
    }

    /* put the main line in the compound */
    comp->lines = line;

    /* put a comment in the main line */
    line->comments = my_strdup("main dimension line");
    line->thickness = cur_dimline_thick;
    line->fill_style = UNFILLED;
    line->style = cur_dimline_style;
    if (line->style == DOTTED_LINE)
	line->style_val = cur_dotgap * (cur_dimline_thick + 1) / 2;
    else
	line->style_val = cur_dashlength * (cur_dimline_thick + 1) / 2;
    line->pen_color = cur_dimline_color;
    if (cur_dimline_leftarrow != -1) {
	line->back_arrow = backward_dim_arrow();
    }
    if (cur_dimline_rightarrow != -1) {
	line->for_arrow = forward_dim_arrow();
    }

    /***************************************/
    /* make the text object for the length */
    /***************************************/

    text = create_text();
    text->depth  = line->depth-2;
    text->cstring = (char *) NULL;	/* the string will be put in later */
    text->color = cur_dimline_textcolor;
    text->font = cur_dimline_font;
    text->size = cur_dimline_fontsize;
    text->flags = cur_dimline_psflag? PSFONT_TEXT: 0;
    text->pen_style = -1;
    text->type = T_CENTER_JUSTIFIED;

    /* put it in the compound */
    comp->texts = text;

    /*****************************/
    /* make the box for the text */
    /*****************************/

    box = create_line();
    box->comments = my_strdup("text box");
    box->depth = line->depth-1;
    box->type = T_POLYGON;
    box->style = SOLID_LINE;
    box->style_val = 0.0;
    box->thickness = cur_dimline_boxthick;
    box->pen_color = cur_pencolor;
    box->fill_color = cur_dimline_boxcolor;
    box->fill_style = NUMSHADEPATS-1;	 /* full saturation color */
    box->pen_style = -1;
    box->join_style = cur_joinstyle;
    box->cap_style = CAP_BUTT;

    /* make 5 points for the box */
    pnt = create_point();
    box->points = pnt;
    pnt->next = create_point();
    pnt = pnt->next;
    pnt->next = create_point();
    pnt = pnt->next;
    pnt->next = create_point();
    pnt = pnt->next;
    pnt->next = create_point();

    /* add this to the lines in the compound */
    comp->lines->next = box;

    /********************************************************************/
    /* make the two ticks at the endpoints if cur_dimline_ticks == True */
    /********************************************************************/

    if (cur_dimline_ticks) {
	create_dimline_ticks(line, &tick1, &tick2);

	/* add this to the lines in the compound */
	box->next = tick1;

	/* add this to the lines in the compound */
	tick1->next = tick2;
    } else 
	box->next = (F_line *) NULL;

    /* if user wants fixed text, add an empty string and a comment to the text part. */
    if (cur_dimline_fixed) {
	text->comments = my_strdup("fixed text");
	/* if not adding to figure, this must be the dimension line setting panel */
	if (!add_to_figure)
	    text->cstring = my_strdup("user text");
	else
	    text->cstring = my_strdup("");
    }

    /* add it to the figure */
    if (add_to_figure) {
	add_compound(comp);
	/* if fixed text, popup editor so user can edit text */
	if (cur_dimline_fixed) {
	    clear_mousefun();
	    set_mousefun("","","", "", "", "");
	    turn_off_current();
	    set_cursor(arrow_cursor);
	    /* make the shapes (ticks, etc) */
	    rescale_dimension_line(comp, 1.0, 1.0, 0, 0);
	    /* tells the editor to switch back to line mode when done */
	    edit_remember_dimline_mode = True;
	    /* now let the user change the text */
	    edit_item(comp, O_COMPOUND, 0, 0);
	}
    }
    /* calculate angles, box size etc */
    /* if user just edited it (both cur_dimline_fixed and add_to_figure = True) then
       it has already been scaled */
    if (!cur_dimline_fixed || !add_to_figure) {
	rescale_dimension_line(comp, 1.0, 1.0, 0, 0);
    }

    /* return it to the caller */
    return comp;
}

/*
 * make the two ticks for the endpoints
 * the actual values of the ticks' points are computed in rescale_dimension_line()
 */

void
create_dimline_ticks(F_line *line, F_line **tick1, F_line **tick2)
{
	F_point	   *pnt;
	F_line	   *tick;

	/* first tick */

	tick = create_line();
	/* copy the attributes from the main line */
	*tick = *line;
	/* set thickness */
	tick->thickness = cur_dimline_tickthick;
	/* make solid */
	tick->style = SOLID_LINE;
	tick->comments = my_strdup("tick");
	/* zero the arrows and next pointer */
	tick->for_arrow = tick->back_arrow = (F_arrow *) NULL;
	tick->next = (F_line *) NULL;
	pnt = create_point();
	tick->points = pnt;
	pnt->next = create_point();
	*tick1 = tick;

	/* now the other tick */

	tick = create_line();
	/* copy the attributes from the main line */
	*tick = *line;
	/* set thickness */
	tick->thickness = cur_dimline_tickthick;
	/* make solid */
	tick->style = SOLID_LINE;
	tick->comments = my_strdup("tick");
	/* zero the arrows and next pointer */
	tick->for_arrow = tick->back_arrow = (F_arrow *) NULL;
	tick->next = (F_line *) NULL;
	pnt = create_point();
	tick->points = pnt;
	pnt->next = create_point();
	*tick2 = tick;
}

F_arrow*
backward_dim_arrow(void)
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL) {
	put_msg("Running out of memory");
	return NULL;
    }

    a->type = ARROW_TYPE(cur_dimline_leftarrow);
    a->style = ARROW_STYLE(cur_dimline_leftarrow);
    a->thickness = cur_dimline_thick;
    if (a->thickness == 0.0)
	    a->thickness = 1.0;
    a->wd = cur_dimline_arrowwidth*cur_dimline_thick;
    a->ht = cur_dimline_arrowlength*cur_dimline_thick;
    if (a->wd == 0.0)
	    a->wd = 1.0;
    if (a->ht == 0.0)
	    a->ht = 1.0;
    return a;
}

F_arrow*
forward_dim_arrow(void)
{
    F_arrow	   *a;

    if ((a = create_arrow()) == NULL) {
	put_msg("Running out of memory");
	return NULL;
    }

    a->type = ARROW_TYPE(cur_dimline_rightarrow);
    a->style = ARROW_STYLE(cur_dimline_rightarrow);
    a->thickness = cur_dimline_thick;
    if (a->thickness == 0.0)
	    a->thickness = 1.0;
    a->wd = cur_dimline_arrowwidth*cur_dimline_thick;
    a->ht = cur_dimline_arrowlength*cur_dimline_thick;
    if (a->wd == 0.0)
	    a->wd = 1.0;
    if (a->ht == 0.0)
	    a->ht = 1.0;
    return a;
}
