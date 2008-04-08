/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
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
#include "object.h"
#include "mode.h"
#include "paintop.h"
#include "d_text.h"
#include "u_create.h"
#include "u_list.h"
#include "u_draw.h"
#include "u_search.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"

#include "e_scale.h"
#include "f_util.h"
#include "u_bound.h"
#include "u_fonts.h"
#include "u_free.h"
#include "u_redraw.h"
#include "w_cursor.h"
#include "w_util.h"

static void	init_update_object(F_line *p, int type, int x, int y, int px, int py);
static void	init_update_settings(F_line *p, int type, int x, int y, int px, int py);

static Boolean	keep_depth = False;
static int	delta_depth;


#define min(a,b) ((a)<(b)) ? (a) : (b)
#define up_part(lv,rv,mask) \
		if (cur_updatemask & (mask)) \
		    (lv) = (rv)
#define up_depth_part(lv,rv) \
		if (cur_updatemask & I_DEPTH) \
		    (lv) = keep_depth ? \
			(min((lv)+delta_depth,MAX_DEPTH)) : (rv)


void up_dashdot (float styleval, int style, unsigned int mask);
void up_from_arrow (F_arrow *arrow, int thick);
void update_compound (F_compound *compound);
void update_line (F_line *line);
void update_text (F_text *text);
void update_ellipse (F_ellipse *ellipse);
void update_arc (F_arc *arc);
void update_spline (F_spline *spline);
void fix_fillstyle (void *object);
void up_arrow (F_line *object);
void update_lines (F_line *lines);
void update_splines (F_spline *splines);
void update_ellipses (F_ellipse *ellipses);
void update_arcs (F_arc *arcs);
void update_texts (F_text *texts);
void update_compounds (F_compound *compounds);

void
update_selected(void)
{
    set_mousefun("update object", "update settings", "",
			LOC_OBJ, LOC_OBJ, LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(init_update_object);
    init_searchproc_middle(init_update_settings);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = object_search_middle;
    canvas_rightbut_proc = null_proc;
    set_cursor(pick9_cursor);
    /* manage on the update buttons */
    manage_update_buts();
    reset_action_on();
}

static int
get_arrow_mode(F_line *object)
{
    if (!object->for_arrow && !object->back_arrow)
	return L_NOARROWS;
    else if (object->for_arrow && !object->back_arrow)
	return L_FARROWS;
    else if (!object->for_arrow && object->back_arrow)
	return L_BARROWS;
    else
	return L_FBARROWS;
}
    
static int
get_arrow_type(F_line *object)
{
    int		    type;

    /* have to pick one or the other */
    if (object->for_arrow)
	type = object->for_arrow->type*2 - 1 + object->for_arrow->style;
    else if (object->back_arrow)
	type = object->back_arrow->type*2 - 1 + object->back_arrow->style;
    else
	type = 0;
    if (type < 0)
	type = 0;
    return type;
}
    
/* update the indicator buttons FROM the selected object */

static void
init_update_settings(F_line *p, int type, int x, int y, int px, int py)
{
    int		    old_psfont_flag, new_psfont_flag;
    F_line	   *dline, *dtick1, *dtick2, *dbox;
    F_text	   *dtext;

    switch (type) {
      case O_COMPOUND:
	cur_c = (F_compound *) p;

	/* if this is a dimension line, update the dimline settings from it */
	if (dimline_components(cur_c, &dline, &dtick1, &dtick2, &dbox)) {
	    if (dline) {
		cur_dimline_thick = dline->thickness;
		cur_dimline_style = dline->style;
		cur_dimline_color = dline->pen_color;
		if (dline->back_arrow) {
		    cur_dimline_leftarrow = dline->back_arrow->type*2 - 1 +
				dline->back_arrow->style;
		    cur_dimline_arrowlength = dline->back_arrow->ht;
		    cur_dimline_arrowwidth = dline->back_arrow->wd;
		} else {
		    cur_dimline_leftarrow = 0;
		}
		if (dline->for_arrow) {
		    cur_dimline_rightarrow = dline->for_arrow->type*2 - 1 +
				dline->for_arrow->style;
		    cur_dimline_arrowlength = dline->for_arrow->ht;
		    cur_dimline_arrowwidth = dline->for_arrow->wd;
		} else {
		    cur_dimline_rightarrow = 0;
		}
	        cur_dimline_ticks = (dtick1 || dtick2);
		if (dtick1)
		    cur_dimline_tickthick = dtick1->thickness;
		else
		    cur_dimline_tickthick = dtick2->thickness;
	    }
	    if (dbox) {
		cur_dimline_boxthick = dbox->thickness;
		cur_dimline_boxcolor = dbox->fill_color;
	    }
	    if (dtext = cur_c->texts) {
		cur_dimline_textcolor = dtext->color;
		cur_dimline_font = dtext->font;
		cur_dimline_fontsize = dtext->size;
		cur_dimline_psflag = (dtext->flags & PSFONT_TEXT)? 1:0;
		cur_dimline_fixed = (dtext->comments && 
				     (strncasecmp(dtext->comments,"fixed text", 10)==0));
	    }
	}

	/* only update the depth setting from the smallest depth in the compound */
	if (cur_updatemask & I_DEPTH) {
	    cur_depth = find_smallest_depth(cur_c);
	}
	break;
      case O_POLYLINE:
	cur_l = (F_line *) p;
	if (cur_l->type != T_PICTURE) {
		up_part(cur_linewidth, cur_l->thickness, I_LINEWIDTH);
		up_part(cur_fillstyle, cur_l->fill_style, I_FILLSTYLE);
		up_part(cur_pencolor, cur_l->pen_color, I_PEN_COLOR);
		up_part(cur_fillcolor, cur_l->fill_color, I_FILL_COLOR);
		up_part(cur_linestyle, cur_l->style, I_LINESTYLE);
		up_part(cur_joinstyle, cur_l->join_style, I_JOINSTYLE);
		up_part(cur_capstyle, cur_l->cap_style, I_CAPSTYLE);
		up_part(cur_styleval, cur_l->style_val*2./(cur_l->thickness+1.), I_LINESTYLE);
		/* update the dash length/dot gap value for cur_styleval */
		up_dashdot(cur_styleval, cur_l->style, I_LINESTYLE);
		up_part(cur_arrowmode, get_arrow_mode(cur_l), I_ARROWMODE);
		up_part(cur_arrowtype, get_arrow_type(cur_l), I_ARROWTYPE);
		if (cur_l->back_arrow)
			up_from_arrow(cur_l->back_arrow,cur_l->thickness);
		if (cur_l->for_arrow)
			up_from_arrow(cur_l->for_arrow,cur_l->thickness);
	} else if (cur_l->pic->pic_cache && cur_l->pic->pic_cache->subtype == T_PIC_XBM) {
		/* only XBM pictures have color */
		up_part(cur_pencolor, cur_l->pen_color, I_PEN_COLOR);
	}
	up_depth_part(cur_depth, cur_l->depth);
	if (cur_l->type == T_ARCBOX)
	    up_part(cur_boxradius, cur_l->radius, I_BOXRADIUS);
	break;
      case O_TEXT:
	cur_t = (F_text *) p;
	up_part(cur_textjust, cur_t->type, I_TEXTJUST);
	up_part(cur_pencolor, cur_t->color, I_PEN_COLOR);
	up_depth_part(cur_depth, cur_t->depth);
	up_part(cur_elltextangle, cur_t->angle/M_PI*180.0, I_ELLTEXTANGLE);
	old_psfont_flag = (cur_textflags & PSFONT_TEXT);
	new_psfont_flag = (cur_t->flags & PSFONT_TEXT);
	up_part(cur_textflags, cur_t->flags & ~PSFONT_TEXT, I_TEXTFLAGS);
	/* for the LaTeX/PostScript flag use I_FONT instead of I_TEXTFLAGS
	   so the font type will change if necessary */
	if (cur_updatemask & I_FONT)
	    cur_textflags |= new_psfont_flag;
	else
	    cur_textflags |= old_psfont_flag;
	if (using_ps) {	/* must use {} because macro has 'if' */
	    up_part(cur_ps_font, cur_t->font, I_FONT);
	} else {	/* must use {} because macro has 'if' */
	    up_part(cur_latex_font, cur_t->font, I_FONT);
	}
	up_part(cur_fontsize, cur_t->size, I_FONTSIZE);
	break;
      case O_ELLIPSE:
	cur_e = (F_ellipse *) p;
	up_part(cur_linewidth, cur_e->thickness, I_LINEWIDTH);
	up_part(cur_elltextangle, cur_e->angle/M_PI*180.0, I_ELLTEXTANGLE);
	up_part(cur_fillstyle, cur_e->fill_style, I_FILLSTYLE);
	up_part(cur_pencolor, cur_e->pen_color, I_PEN_COLOR);
	up_part(cur_fillcolor, cur_e->fill_color, I_FILL_COLOR);
	up_part(cur_linestyle, cur_e->style, I_LINESTYLE);
	up_part(cur_styleval, (cur_e->style_val*2.)/(cur_e->thickness+1.), I_LINESTYLE);
	/* update the dash length/dot gap value for cur_styleval */
	up_dashdot(cur_styleval, cur_e->style, I_LINESTYLE);
	up_depth_part(cur_depth, cur_e->depth);
	break;
      case O_ARC:
	cur_a = (F_arc *) p;
	up_part(cur_linewidth, cur_a->thickness, I_LINEWIDTH);
	up_part(cur_fillstyle, cur_a->fill_style, I_FILLSTYLE);
	up_part(cur_pencolor, cur_a->pen_color, I_PEN_COLOR);
	up_part(cur_fillcolor, cur_a->fill_color, I_FILL_COLOR);
	up_part(cur_arctype, cur_a->type, I_ARCTYPE);
	up_part(cur_linestyle, cur_a->style, I_LINESTYLE);
	up_part(cur_styleval, (cur_a->style_val*2.)/(cur_a->thickness+1.), I_LINESTYLE);
	/* update the dash length/dot gap value for cur_styleval */
	up_dashdot(cur_styleval, cur_a->style, I_LINESTYLE);
	up_part(cur_capstyle, cur_a->cap_style, I_CAPSTYLE);
	up_depth_part(cur_depth, cur_a->depth);
	up_part(cur_arrowmode, get_arrow_mode((F_line *)cur_a), I_ARROWMODE);
	up_part(cur_arrowtype, get_arrow_type((F_line *)cur_a), I_ARROWTYPE);
	if (cur_a->back_arrow)
		up_from_arrow(cur_a->back_arrow,cur_a->thickness);
	if (cur_a->for_arrow)
		up_from_arrow(cur_a->for_arrow,cur_a->thickness);
	break;
      case O_SPLINE:
	cur_s = (F_spline *) p;
	up_part(cur_linewidth, cur_s->thickness, I_LINEWIDTH);
	up_part(cur_fillstyle, cur_s->fill_style, I_FILLSTYLE);
	up_part(cur_pencolor, cur_s->pen_color, I_PEN_COLOR);
	up_part(cur_fillcolor, cur_s->fill_color, I_FILL_COLOR);
	up_part(cur_linestyle, cur_s->style, I_LINESTYLE);
	up_part(cur_styleval, (cur_s->style_val*2.)/(cur_s->thickness+1.), I_LINESTYLE);
	/* update the dash length/dot gap value for cur_styleval */
	up_dashdot(cur_styleval, cur_s->style, I_LINESTYLE);
	if (cur_s->type == T_OPEN_APPROX || cur_s->type == T_OPEN_INTERP)
	    up_part(cur_capstyle, cur_s->cap_style, I_CAPSTYLE);
	up_depth_part(cur_depth, cur_s->depth);
	up_part(cur_arrowmode, get_arrow_mode((F_line *)cur_s), I_ARROWMODE);
	up_part(cur_arrowtype, get_arrow_type((F_line *)cur_s), I_ARROWTYPE);
	if (cur_s->back_arrow)
		up_from_arrow(cur_s->back_arrow,cur_s->thickness);
	if (cur_s->for_arrow)
		up_from_arrow(cur_s->for_arrow,cur_s->thickness);
	break;
      default:
	return;
    }
    update_current_settings();
    if (type == O_COMPOUND) {
	put_msg("Note: Only the (smallest) depth is retrieved from a compound object");
	beep();
    } else {
	put_msg("Settings UPDATED");
    }
}

void up_from_arrow(F_arrow *arrow, int thick)
{
	up_part(cur_arrowthick, arrow->thickness,I_ARROWSIZE);
	up_part(cur_arrowwidth, arrow->wd,I_ARROWSIZE);
	up_part(cur_arrowheight,arrow->ht,I_ARROWSIZE);
	up_part(cur_arrow_multthick, arrow->thickness,I_ARROWSIZE);
	up_part(cur_arrow_multwidth, arrow->wd,I_ARROWSIZE);
	up_part(cur_arrow_multheight,arrow->ht,I_ARROWSIZE);
	if (cur_updatemask & I_ARROWSIZE) {
	    /* now get multiple to line width of object */
	    cur_arrow_multthick /= thick;
	    cur_arrow_multwidth /= thick;
	    cur_arrow_multheight /= thick;
	}
}

/* update the selected object FROM the indicator buttons */

static void
init_update_object(F_line *p, int type, int x, int y, int px, int py)
{
    int		    largest;
    Boolean	    dontupdate;

    dontupdate = False;

    switch (type) {
      case O_COMPOUND:
	set_temp_cursor(wait_cursor);
	cur_c = (F_compound *) p;
	new_c = copy_compound(cur_c);

	/* keep the depths of the objects inside the compound the same
	   relative to each other, setting the depth of the *most shallow*
	   to the desired depth */
	keep_depth = True;
	largest = find_largest_depth(cur_c);
	/* find delta */
	delta_depth = cur_depth - find_smallest_depth(cur_c);
	/* if renumbering would make depths too large don't allow it */
	if ((delta_depth + largest > MAX_DEPTH) && (cur_updatemask & I_DEPTH)) {
	    if (popup_query(QUERY_YESNO,
		"Some depths would exceed maximum - those objects\nwill be set to maximum depth. Update anyway?") != RESULT_YES) {
	    delta_depth = 0;
	    dontupdate = True;
	}
	}
	update_compound(new_c);
	keep_depth = False;

	change_compound(cur_c, new_c);
	/* redraw anything near the old comound */
	redisplay_compound(cur_c);
	/* draw the new compound */
	redisplay_compound(new_c);
	break;
      case O_POLYLINE:
	set_temp_cursor(wait_cursor);
	cur_l = (F_line *) p;
	new_l = copy_line(cur_l);
	update_line(new_l);
	change_line(cur_l, new_l);
	/* redraw anything near the old line */
	redisplay_line(cur_l);
	/* draw the new line */
	redisplay_line(new_l);
	break;
      case O_TEXT:
	set_temp_cursor(wait_cursor);
	cur_t = (F_text *) p;
	new_t = copy_text(cur_t);
	update_text(new_t);
	change_text(cur_t, new_t);
	/* redraw anything near the old text */
	redisplay_text(cur_t);
	/* draw the new text */
	redisplay_text(new_t);
	break;
      case O_ELLIPSE:
	set_temp_cursor(wait_cursor);
	cur_e = (F_ellipse *) p;
	new_e = copy_ellipse(cur_e);
	update_ellipse(new_e);
	change_ellipse(cur_e, new_e);
	/* redraw anything near the old ellipse */
	redisplay_ellipse(cur_e);
	/* draw the new ellipse */
	redisplay_ellipse(new_e);
	break;
      case O_ARC:
	set_temp_cursor(wait_cursor);
	cur_a = (F_arc *) p;
	new_a = copy_arc(cur_a);
	update_arc(new_a);
	change_arc(cur_a, new_a);
	/* redraw anything near the old arc */
	redisplay_arc(cur_a);
	/* draw the new arc */
	redisplay_arc(new_a);
	break;
      case O_SPLINE:
	set_temp_cursor(wait_cursor);
	cur_s = (F_spline *) p;
	new_s = copy_spline(cur_s);
	update_spline(new_s);
	change_spline(cur_s, new_s);
	/* redraw anything near the old spline */
	redisplay_spline(cur_s);
	/* draw the new spline */
	redisplay_spline(new_s);
	break;
    default:
	return;
    }
    reset_cursor();
    if (!dontupdate)
	put_msg("Object(s) UPDATED");
}

void update_ellipse(F_ellipse *ellipse)
{
    draw_ellipse(ellipse, ERASE);
    up_part(ellipse->thickness, cur_linewidth, I_LINEWIDTH);
    up_part(ellipse->angle, cur_elltextangle*M_PI/180.0, I_ELLTEXTANGLE);
    up_part(ellipse->style, cur_linestyle, I_LINESTYLE);
    up_part(ellipse->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
	    I_LINESTYLE);
    up_part(ellipse->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(ellipse->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(ellipse->fill_color, cur_fillcolor, I_FILL_COLOR);
    up_depth_part(ellipse->depth, cur_depth);
    fix_fillstyle(ellipse);	/* make sure it has legal fill style if color changed */
    /* updated object will be redisplayed by init_update_xxx() */
}

void update_arc(F_arc *arc)
{
    draw_arc(arc, ERASE);
    up_part(arc->thickness, cur_linewidth, I_LINEWIDTH);
    up_part(arc->style, cur_linestyle, I_LINESTYLE);
    up_part(arc->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
	    I_LINESTYLE);
    up_part(arc->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(arc->type, cur_arctype, I_ARCTYPE);
    up_part(arc->cap_style, cur_capstyle, I_CAPSTYLE);
    up_part(arc->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(arc->fill_color, cur_fillcolor, I_FILL_COLOR);
    up_depth_part(arc->depth, cur_depth);
    /* check new type - if pie-wedge and there are any arrows, delete them */
    if (arc->type == T_PIE_WEDGE_ARC) {
	if (arc->for_arrow) {
	    free((char *) arc->for_arrow);
	    arc->for_arrow = NULL;
	}
	if (arc->back_arrow) {
	    free((char *) arc->back_arrow);
	    arc->back_arrow = NULL;
	}
    } else {
	/* if not wedge type, update any arrow settings */
	up_arrow((F_line *)arc);
    }
    fix_fillstyle(arc);	/* make sure it has legal fill style if color changed */
    /* updated object will be redisplayed by init_update_xxx() */
}

void update_line(F_line *line)
{
    draw_line(line, ERASE);
	up_part(line->thickness, cur_linewidth, I_LINEWIDTH);
    if (line->type != T_PICTURE) {
	up_part(line->style, cur_linestyle, I_LINESTYLE);
	up_part(line->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
		I_LINESTYLE);
	up_part(line->join_style, cur_joinstyle, I_JOINSTYLE);
	up_part(line->cap_style, cur_capstyle, I_CAPSTYLE);
	up_part(line->pen_color, cur_pencolor, I_PEN_COLOR);
	up_part(line->fill_color, cur_fillcolor, I_FILL_COLOR);
	up_part(line->radius, cur_boxradius, I_BOXRADIUS);
	up_part(line->fill_style, cur_fillstyle, I_FILLSTYLE);
    } else if (line->pic->pic_cache && line->pic->pic_cache->subtype == T_PIC_XBM) {
	/* only XBM pictures have color */
	up_part(line->pen_color, cur_pencolor, I_PEN_COLOR);
    }
    up_depth_part(line->depth, cur_depth);
    /* only POLYLINES with more than one point may have arrow heads */
    if (line->type == T_POLYLINE && line->points->next != NULL)
	up_arrow(line);
    fix_fillstyle(line);	/* make sure it has legal fill style if color changed */
    /* updated object will be redisplayed by init_update_xxx() */
}

void update_text(F_text *text)
{
    PR_SIZE	    size;
    int		old_psfont_flag, new_psfont_flag;

    draw_text(text, ERASE);
    up_part(text->type, cur_textjust, I_TEXTJUST);
    up_part(text->font, using_ps ? cur_ps_font : cur_latex_font, I_FONT);
    old_psfont_flag = (text->flags & PSFONT_TEXT);
    new_psfont_flag = (cur_textflags & PSFONT_TEXT);
    /* turn off any postscript flags */
    text->flags &= ~PSFONT_TEXT;
    /* set text flags to current settings */
    up_part(text->flags, cur_textflags, I_TEXTFLAGS);
    /* for the LaTeX/PostScript flag use I_FONT instead of I_TEXTFLAGS
	so the font type will change if necessary */
    /* and update with new font type */
    if (cur_updatemask & I_FONT)
	text->flags |= new_psfont_flag;
    else
	text->flags |= old_psfont_flag;
    up_part(text->size, cur_fontsize, I_FONTSIZE);
    up_part(text->angle, cur_elltextangle*M_PI/180.0, I_ELLTEXTANGLE);
    up_part(text->color, cur_pencolor, I_PEN_COLOR);
    up_depth_part(text->depth, cur_depth);
    size = textsize(lookfont(x_fontnum(psfont_text(text), text->font),
			text->size), strlen(text->cstring), text->cstring);
    text->ascent = size.ascent;
    text->descent = size.descent;
    text->length = size.length;
    reload_text_fstruct(text);	/* make sure fontstruct is current */
    /* updated object will be redisplayed by init_update_xxx() */
}

void update_spline(F_spline *spline)
{
    draw_spline(spline, ERASE);
    up_part(spline->thickness, cur_linewidth, I_LINEWIDTH);
    up_part(spline->style, cur_linestyle, I_LINESTYLE);
    up_part(spline->style_val, cur_styleval * (cur_linewidth + 1) / 2, 
	    I_LINESTYLE);
    if (spline->type == T_OPEN_APPROX || spline->type == T_OPEN_INTERP)
	up_part(spline->cap_style, cur_capstyle, I_CAPSTYLE);
    up_part(spline->fill_style, cur_fillstyle, I_FILLSTYLE);
    up_part(spline->pen_color, cur_pencolor, I_PEN_COLOR);
    up_part(spline->fill_color, cur_fillcolor, I_FILL_COLOR);
    up_depth_part(spline->depth, cur_depth);
    if (open_spline(spline))
	up_arrow((F_line *)spline);
    fix_fillstyle(spline);	/* make sure it has legal fill style if color changed */
    /* updated object will be redisplayed by init_update_xxx() */
}

/* check that the fill style is legal for the color in the object */
/* WARNING: this procedure assumes that splines, lines, arcs and ellipses
	    all have the same structure up to the fill_style and color */

void fix_fillstyle(void *obj)
{
    F_line *object = obj;
    if (object->fill_color == BLACK || object->fill_color == DEFAULT ||
	object->fill_color == DEFAULT) {
	    if (object->fill_style >= NUMSHADEPATS && 
		object->fill_style < NUMSHADEPATS+NUMTINTPATS)
			object->fill_style = UNFILLED;
    }
    /* a little sanity check */
    if (object->fill_style < DEFAULT)
	object->fill_style = DEFAULT;
    if (object->fill_style >= NUMFILLPATS)
	object->fill_style = NUMFILLPATS;
}

void up_arrow(F_line *object)
{
    if (object->for_arrow) {
	up_part(object->for_arrow->type, ARROW_TYPE(cur_arrowtype), I_ARROWTYPE);
	up_part(object->for_arrow->style, ARROW_STYLE(cur_arrowtype), I_ARROWTYPE);
	if (use_abs_arrowvals) {
	    up_part(object->for_arrow->thickness,cur_arrowthick,I_ARROWSIZE);
	    up_part(object->for_arrow->wd,cur_arrowwidth,I_ARROWSIZE);
	    up_part(object->for_arrow->ht,cur_arrowheight,I_ARROWSIZE);
	} else {
	    up_part(object->for_arrow->thickness,
				cur_arrow_multthick*object->thickness, I_ARROWSIZE);
	    up_part(object->for_arrow->wd,
	   			cur_arrow_multwidth*object->thickness, I_ARROWSIZE);
	    up_part(object->for_arrow->ht,
				cur_arrow_multheight*object->thickness, I_ARROWSIZE);
	}
    }
    if (object->back_arrow) {
	up_part(object->back_arrow->type, ARROW_TYPE(cur_arrowtype), I_ARROWTYPE);
	up_part(object->back_arrow->style, ARROW_STYLE(cur_arrowtype), I_ARROWTYPE);
	if (use_abs_arrowvals) {
	    up_part(object->back_arrow->thickness,cur_arrowthick,I_ARROWSIZE);
	    up_part(object->back_arrow->wd,cur_arrowwidth,I_ARROWSIZE);
	    up_part(object->back_arrow->ht,cur_arrowheight,I_ARROWSIZE);
	} else {
	    up_part(object->back_arrow->thickness,
				cur_arrow_multthick*object->thickness,I_ARROWSIZE);
	    up_part(object->back_arrow->wd,
				cur_arrow_multwidth*object->thickness,I_ARROWSIZE);
	    up_part(object->back_arrow->ht,
				cur_arrow_multheight*object->thickness, I_ARROWSIZE);
	}
    }

    if (! (cur_updatemask & I_ARROWMODE))
	return;

    if (autoforwardarrow_mode) {
	if (object->for_arrow) {
	    if (use_abs_arrowvals) {
		object->for_arrow->thickness = cur_arrowthick;
		object->for_arrow->wd = cur_arrowwidth;
		object->for_arrow->ht = cur_arrowheight;
	    } else {
		object->for_arrow->thickness = cur_arrow_multthick*cur_linewidth;
		object->for_arrow->wd = cur_arrow_multwidth*cur_linewidth;
		object->for_arrow->ht = cur_arrow_multheight*cur_linewidth;
	    }
	} else	/* no arrowhead at all yet, create a new one */
	    up_part(object->for_arrow, forward_arrow(), I_ARROWMODE);
    } else {	/* delete arrowhead if one exists */
	if (object->for_arrow) {
	    free((char *) object->for_arrow);
	    object->for_arrow = NULL;
	}
    }
    if (autobackwardarrow_mode) {
	if (object->back_arrow) {
	    if (use_abs_arrowvals) {
		object->back_arrow->thickness = cur_arrowthick;
		object->back_arrow->wd = cur_arrowwidth;
		object->back_arrow->ht = cur_arrowheight;
	    } else {
		object->back_arrow->thickness = cur_arrow_multthick*cur_linewidth;
		object->back_arrow->wd = cur_arrow_multwidth*cur_linewidth;
		object->back_arrow->ht = cur_arrow_multheight*cur_linewidth;
	    }
	} else {	/* no arrowhead at all yet, create a new one */
	    up_part(object->back_arrow, backward_arrow(), I_ARROWMODE);
	}
    } else {	/* delete arrowhead if one exists */
	if (object->back_arrow) {
	    free((char *) object->back_arrow);
	    object->back_arrow = NULL;
	}
    }
}

void update_compound(F_compound *compound)
{
    F_line	   *dline, *dtick1, *dtick2, *dbox;
    F_text	   *dtext;

    /* if this is a dimension line, update its settings from the dimline settings */
    if (dimline_components(compound, &dline, &dtick1, &dtick2, &dbox)) {
	if (dline) {
	    dline->thickness = cur_dimline_thick;
	    dline->style = cur_dimline_style;
	    dline->pen_color = cur_dimline_color;

	    /* free old left arrow */
	    if (dline->back_arrow) {
		free((char *) dline->back_arrow);
		dline->back_arrow = NULL;
	    }
	    /* create new one if setting says so */
	    if (cur_dimline_leftarrow != -1)
		dline->back_arrow = backward_dim_arrow();

	    /* free old right arrow */
	    if (dline->for_arrow) {
		free((char *) dline->for_arrow);
		dline->for_arrow = NULL;
	    }
	    /* create new one if setting says so */
	    if (cur_dimline_rightarrow != -1)
		dline->for_arrow = forward_dim_arrow();

	    /* update text box */
	    if (dbox) {
		/* attach the polygon after the main line */
		dline->next = dbox;
		dbox->thickness = cur_dimline_boxthick;
		dbox->fill_color = cur_dimline_boxcolor;
	    }
	} /* if (dline) */

	/* free any old ticks */
	if (dtick1)
	    free_linestorage(dtick1);
	if (dtick2)
	    free_linestorage(dtick2);

	/* create new ones if user wants */
	if (cur_dimline_ticks) {
	    create_dimline_ticks(dline, &dtick1, &dtick2);
	    /* attach it to the previous object in the compound */
	    if (dbox)
		dbox->next = dtick1;
	    else
		dline->next = dtick1;
	    dtick1->next = dtick2;
	} else {
	    /* no ticks, terminate list of lines after box */
	    if (dbox)
		dbox->next = (F_line *) NULL;
	}
       /* now put the new line list into the compound */
       if (dline) 
	   compound->lines = dline;
       else
	   compound->lines = dbox;

	/* finally, the text */
	if (dtext = compound->texts) {
	    dtext->color = cur_dimline_textcolor;
	    dtext->font = cur_dimline_font;
	    dtext->size = cur_dimline_fontsize;
	    dtext->flags = cur_dimline_psflag? PSFONT_TEXT: 0;
	    /* free any comments in the text */
	    if (dtext->comments)
		free((char *) dtext->comments);
	    if (cur_dimline_fixed)
		dtext->comments = my_strdup("fixed text");
	}
	/* now update the depths of the components */
	up_depth_part(dline->depth, cur_depth);
	if (dbox)
	    up_depth_part(dbox->depth,  cur_depth);
	if (dtick1)
	    up_depth_part(dtick1->depth, cur_depth);
	if (dtick2)
	    up_depth_part(dtick2->depth, cur_depth);
	if (dtext)
	    up_depth_part(dtext->depth, cur_depth);

	/* finally, rescale if necessary */
	rescale_dimension_line(compound, 1.0, 1.0, 0, 0);

	/* end of dimension line update */

    } else {
	/* ordinary compound */
	update_lines(compound->lines);
	update_splines(compound->splines);
	update_ellipses(compound->ellipses);
	update_arcs(compound->arcs);
	update_texts(compound->texts);
	update_compounds(compound->compounds);
    }
    compound_bound(compound, &compound->nwcorner.x, &compound->nwcorner.y,
		   &compound->secorner.x, &compound->secorner.y);
}

void update_arcs(F_arc *arcs)
{
    F_arc	   *a;

    for (a = arcs; a != NULL; a = a->next)
	update_arc(a);
}

void update_compounds(F_compound *compounds)
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next)
	update_compound(c);
}

void update_ellipses(F_ellipse *ellipses)
{
    F_ellipse	   *e;

    for (e = ellipses; e != NULL; e = e->next)
	update_ellipse(e);
}

void update_lines(F_line *lines)
{
    F_line	   *l;

    for (l = lines; l != NULL; l = l->next)
	update_line(l);
}

void update_splines(F_spline *splines)
{
    F_spline	   *s;

    for (s = splines; s != NULL; s = s->next)
	update_spline(s);
}

void update_texts(F_text *texts)
{
    F_text	   *t;

    for (t = texts; t != NULL; t = t->next)
	update_text(t);
}

void up_dashdot(float styleval, int style, unsigned int mask)
{
	if ((style == DASH_LINE) ||
	    (style == DASH_DOT_LINE) || 
	    (style == DASH_2_DOTS_LINE) || 
	    (style == DASH_3_DOTS_LINE)) {
		cur_dashlength = styleval;
	} else if (style == DOTTED_LINE) {
		cur_dotgap = styleval;
	}
}
