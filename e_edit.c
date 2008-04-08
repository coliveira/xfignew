/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Change function implemented by Frank Schmuck (schmuck@svax.cs.cornell.edu)
 * X version by Jon Tombs <jon@uk.ac.oxford.robots>
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
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "e_edit.h"
#include "d_subspline.h"
#include "d_text.h"
#include "f_read.h"
#include "f_util.h"
#include "u_create.h"
#include "u_fonts.h"
#include "u_search.h"
#include "u_list.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_markers.h"
#include "u_undo.h"
#include "w_browse.h"
#include "w_canvas.h"
#include "w_capture.h"
#include "w_drawprim.h"
#include "w_fontbits.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"

#include "d_line.h"
#include "e_placelib.h"
#include "e_scale.h"
#include "f_picobj.h"
#include "u_bound.h"
#include "u_free.h"
#include "u_geom.h"
#include "u_redraw.h"
#include "u_translate.h"
#include "w_color.h"
#include "w_cursor.h"
#include "w_dir.h"

#include <sys/wait.h>  /* waitpid() */

/* EXPORTS */

Widget	pic_name_panel;  	/* global to be visible in w_browse */

/* LOCAL */

#define NUM_IMAGES	16
#define	MAXDISPTS	100	/* maximum number of points to display for line, spline, etc */

static	int what_rotation(int dx, int dy);

#define XY_WIDTH	56	/* width of text widgets for x/y points */

/* define some true/false constants to make proc calls easier to read */
#define GENERICS	True
#define NO_GENERICS	False

#define ARROWS		True
#define NO_ARROWS	False

static Position rootx, rooty;
static void	new_generic_values(void);
static void	new_arrow_values(void);
static void	get_new_line_values(void);
static void	generic_window(char *object_type, char *sub_type, icon_struct *icon, void (*d_proc) (/* ??? */), Boolean generics, Boolean arrows, char *comments);
static void     spline_point_window(int x, int y);
static void	font_image_panel(Pixmap pixmap, char *label, Widget *pi_x);
static Widget	pen_color_selection_panel(void);
static Widget	fill_color_selection_panel(void);
static void	float_label(float x, char *label, Widget *pi_x);
static void	int_label(int x, char *label, Widget *pi_x);
static void	str_panel(char *string, char *name, Widget *pi_x, int width, Boolean size_to_width, Boolean international);
static void	xy_panel(int x, int y, char *label, Widget *pi_x, Widget *pi_y, Boolean make_unit_menu);
static void	f_pair_panel(F_pos *fp, char *label, Widget *pi_x, char *xlabel, Widget *pi_y, char *ylabel, Boolean make_unit_menu);
static void	get_f_pos(F_pos *fp, Widget pi_x, Widget pi_y);
static void	points_panel(struct f_point *p);
static void	get_points(struct f_point *p);
static void	arc_type_select(Widget w, XtPointer new_style, XtPointer call_data);
static void	cap_style_select(Widget w, XtPointer new_type, XtPointer call_data);
static void	join_style_select(Widget w, XtPointer new_type, XtPointer call_data);
static void	line_style_select(Widget w, XtPointer new_style, XtPointer call_data);
static void	for_arrow_type_select(Widget w, XtPointer new_type, XtPointer call_data);
static void	back_arrow_type_select(Widget w, XtPointer new_type, XtPointer call_data);
static void	textjust_select(Widget w, XtPointer new_textjust, XtPointer call_data);
static void	fill_style_select(Widget w, XtPointer new_fillflag, XtPointer call_data);
static void	flip_pic_select(Widget w, XtPointer new_flipflag, XtPointer call_data);
static void	rotation_select(Widget w, XtPointer new_rotation, XtPointer call_data);
static void	hidden_text_select(Widget w, XtPointer new_hidden_text, XtPointer call_data);
static void	rigid_text_select(Widget w, XtPointer new_rigid_text, XtPointer call_data);
static void	special_text_select(Widget w, XtPointer new_special_text, XtPointer call_data);
static void	pen_color_select(Widget w, XtPointer new_color, XtPointer call_data);
static void	fill_color_select(Widget w, XtPointer new_color, XtPointer call_data);
static int	panel_get_dim_value(Widget widg);
static void	panel_clear_value(Widget widg);
static void	get_new_compound_values(void);
static void	panel_set_scaled_int(Widget widg, int intval);
static void	unit_select(Widget w, XtPointer new_unit, XtPointer call_data);
static void	init_convert_array(void);
static void	add_to_convert(Widget widget);
static double	cvt_to_units(int fig_unit);
static void	cvt_to_units_str(int x, char *buf);
static int	cvt_to_fig(double real_unit);

static void	reposition_picture(Widget w);
static void	resize_picture(Widget w);

static void	modify_compound(Widget w);
static void	reposition_top(void);
static void	reposition_bottom(void);
static void	rescale_compound(void);
static void	collapse_depth(Widget panel_local, XtPointer closure, XtPointer call_data);

static void	done_line(void);
static void	done_text(void);
static void	done_arc(void);
static void	done_ellipse(void);
static void	done_spline(void);
static void	done_spline_point(void);
static void	done_compound(void);
static void	done_figure_comments(void);

static void	done_button(Widget panel_local, XtPointer closure, XtPointer call_data), apply_button(Widget panel_local, XtPointer closure, XtPointer call_data), cancel_button(Widget panel_local, XtPointer closure, XtPointer call_data);
static void	toggle_sfactor_type(Widget panel_local, XtPointer _sfactor_index, XtPointer call_data);
static void	change_sfactor_value(Widget panel_local, XtPointer closure, XtPointer _top);
static void	scroll_sfactor_value(Widget panel_local, XtPointer closure, XtPointer _num_pixels);
static void	grab_button(Widget panel_local, XtPointer closure, XtPointer call_data), browse_button(Widget panel_local, XtPointer closure, XtPointer call_data), image_edit_button(Widget panel_local, XtPointer closure, XtPointer call_data);
static void	update_fill_image(Widget w, XtPointer dummy, XtPointer dummy2);

static Widget	popup, form;
static Widget	below, beside;
static Widget	above_arrows;
static Widget	comment_popup;
static Widget	arc_points_form;

static Widget	float_panel(float x, Widget parent, char *label, Widget pbeside, Widget *pi_x, float min, float max, float inc, int prec);
static Widget	int_panel(int x, Widget parent, char *label, Widget beside, Widget *pi_x, int min, int max, int inc);
static Widget	int_panel_callb(int x, Widget parent, char *label, Widget pbeside, Widget *pi_x, int min, int max, int inc, XtCallbackProc callback);
static Widget	reread, shrink, expand, origsize;
static Widget	percent_button, percent, percent_entry;
static Widget	label;
static Widget	thickness_panel;
static Widget	depth_panel;
static Widget	angle_panel;
static Widget	textjust_panel;
static Widget	hidden_text_panel;
static Widget	rigid_text_panel;
static Widget	special_text_panel;
static Widget	fill_intens, fill_intens_panel, fill_intens_label, fill_image;
static Widget	fill_pat, fill_pat_panel, fill_pat_label;
static Widget	flip_pic_panel;
static Widget	rotation_panel;
static Widget	arc_type_panel;
static Widget	cap_style_panel;
static Widget	join_style_panel;
static Widget	line_style_panel;
static Widget	for_arrow_type_panel;
static Widget	back_arrow_type_panel;
static Widget	style_val, style_val_panel, style_val_label;
static Widget	for_arrow_height,for_arrow_width,for_arrow_thick;
static Widget	back_arrow_height,back_arrow_width,back_arrow_thick;
static Widget	for_thick_label,for_height_label,for_width_label;
static Widget	back_thick_label,back_height_label,back_width_label;
static Widget	for_thick,for_height,for_width;
static Widget	back_thick,back_height,back_width;
static Boolean	for_arrow, back_arrow;

static Widget	text_panel;
static Widget	x1_panel, y1_panel;
static Widget	x2_panel, y2_panel;
static Widget	x3_panel, y3_panel;
static Widget	width_panel, height_panel;
static Widget	hw_ratio_panel;
static Widget	orig_hw_panel;
static Widget	font_panel;
static Widget	cur_fontsize_panel;
static Widget	fill_style_button;
static Widget	radius, num_objects;
static Widget	comments_panel;
static Widget	for_aform,back_aform;
static Widget	but1;
static Widget	unit_menu_button;
static Widget	unit_pulldown_menu(Widget below, Widget beside);
static Widget	min_depth_w, max_depth_w;

/* for PIC object type */
static Widget	pic_size, pic_type_box[NUM_PIC_TYPES], pic_colors, transp_color;

static Widget	pen_col_button, pen_color_popup=0;
static Widget	fill_col_button, fill_color_popup=0;

DeclareStaticArgs(20);
static char	buf[64];

static Widget	px_panel[MAXDISPTS];
static Widget	py_panel[MAXDISPTS];

/* for our own fill % and fill pattern pixmap in the popup */

#define FILL_SIZE 40	/* size of indicators */
static	Pixmap	fill_image_pm,fill_image_bm[NUMFILLPATS];
static	Boolean	fill_image_bm_exist = False;	
static	GC	fill_image_gc = 0;
static	Boolean	fill_style_exists = False;
static	Pixel	form_bg = -1;

/* pulldown menu for units of point coords */

static char	*unit_items[] = {NULL,	 /* this is modified in unit_select() */
				 "Fig units"};
static int	points_units=0;		/*  0=Ruler scale, 1=Fig*/
static int	conv_array_idx = 0;	/* to keep track of widgets that need unit conversions */
static Widget	convert_array[MAXDISPTS*2]; /* array for those widgets */

/* For edit all texts inside compound function */

#define MAX_COMPOUND_TEXT_PANELS 2000	/* max number of texts that can be edited in compound */
static	Widget	compound_text_panels[MAX_COMPOUND_TEXT_PANELS];
Boolean	edit_remember_lib_mode = False;		/* Remember that we were in library mode */
Boolean	edit_remember_dimline_mode = False;	/* Remember that we were in dimension line mode */
Boolean	lib_cursor = False;			/* to reset cursor back to lib mode */
Boolean	dim_cursor = False;			/* to reset cursor back to line mode */

/*********************************************************************/
/* NOTE: If you change this you must change pictypes in object.h too */
/*********************************************************************/

static char	*pic_names[] = {
			"--", "EPS/PS", "GIF", 
#ifdef USE_JPEG
			"JPEG",
#endif /* USE_JPEG */
			"PCX", "PNG", "PPM", "TIFF", "XBM",
#ifdef USE_XPM
			"XPM",
#endif /* USE_XPM */
		};

static int	ellipse_flag;
static int	fill_flag;
static int	flip_pic_flag;
static void	(*done_proc) ();
static int	button_result;
static int	textjust;
static Color	pen_color, fill_color;
static int	hidden_text_flag;
static int	special_text_flag;
static int	rigid_text_flag;
static int	new_ps_font, new_latex_font;
static int	new_psflag;
static int	min_compound_depth;
static Boolean	changed, reread_file;
static Boolean  file_changed=False;
static Boolean	actions_added=False;

static void     edit_cancel(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params);
static void     edit_done(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params), edit_apply(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params);
static void	popdown_comments(void);

/*
 * the following pix_table and xxxx_pixmaps entries are guaranteed to be 
 * initialized to 0 by the compiler
 */

static struct {
    icon_struct	   *image;
    Pixmap	    image_pm;
}		pix_table[NUM_IMAGES];

static Pixmap	    capstyle_pixmaps[NUM_JOINSTYLE_TYPES] = {0};
static Pixmap	    joinstyle_pixmaps[NUM_CAPSTYLE_TYPES] = {0};

/* picture flip menu options */
static char    *flip_pic_items[] = {"Normal            ",
				"Flipped about diag"};

/* picture rotation menu options */
static char    *rotation_items[] = {"  0", " 90", "180", "270"};

/**********************************************/
/* Translations and actions for edit cancel   */
/* Make Escape cancel the edit in addition to */
/* destroy window                             */
/**********************************************/

static String   edit_popup_translations =
	"<Key>Escape: CancelEdit()\n\
	<Message>WM_PROTOCOLS: CancelEdit()\n";

static XtActionsRec     edit_actions[] =
{
    {"CancelEdit", (XtActionProc) edit_cancel},
};

/*************************************************/
/* Translations and actions for showing comments */
/*************************************************/

static String   showcomment_popup_translations =
	"<Btn2Up>: PopdownComments()\n\
	<Message>WM_PROTOCOLS: PopdownComments()";

static XtActionsRec     showcomment_actions[] =
{
    {"PopdownComments", (XtActionProc) popdown_comments},
};

/*********************************************/
/* Translations and actions for text widgets */
/*********************************************/

/* don't allow newlines in text until we handle multiple line texts */
/** Add Ctrl-Return to do a quick apply and DONE */
/** Bound ^U to erase line instead of multiply(4) */

static String         edit_text_translations =
	"Ctrl<Key>Return: DoneEdit()\n\
	<Key>Return:	ApplyEdit()\n\
	<Key>Escape:	CancelEdit()\n\
	Ctrl<Key>J:	no-op(RingBell)\n\
	Ctrl<Key>M:	no-op(RingBell)\n\
	Ctrl<Key>X:	EmptyTextKey()\n\
	Ctrl<Key>U:	EmptyTextKey()\n\
	<Key>F18:	PastePanelKey()\n";

/* scroll in comments */

static String	      edit_comment_translations = 
	"<Btn4Down>:	scroll-one-line-up()\n\
	<Btn5Down>:	scroll-one-line-down()\n";

static XtActionsRec     text_actions[] =
{
    {"DoneEdit", (XtActionProc) edit_done},
    {"ApplyEdit", (XtActionProc) edit_apply},
};

static String edit_picture_pos_translations =
	"<Key>Return: RepositionPicture()\n";

static XtActionsRec picture_pos_actions[] =
{
    {"RepositionPicture", (XtActionProc) reposition_picture},
};

static String edit_picture_size_translations =
	"<Key>Return: ResizePicture()\n";

static XtActionsRec picture_size_actions[] =
{
    {"ResizePicture", (XtActionProc) resize_picture},
};

static String edit_compound_translations =
	"<Key>Return: ModifyCompound()\n";

static XtActionsRec compound_actions[] =
{
    {"ModifyCompound", (XtActionProc) modify_compound},
};

#define CANCEL		0
#define DONE		1
#define APPLY		2

/******************************/
/* specific stuff for splines */
/******************************/

#define THUMB_H  0.05
#define STEP_VALUE 0.02
#define SFACTOR_BAR_HEIGHT 200
#define SFACTOR_BAR_WIDTH (SFACTOR_BAR_HEIGHT/10)
#define SFACTOR_SIGN(x) ( (x) < 0 ? 1.0 : -1.0)
#define SFACTOR_TO_PERCENTAGE(x) ((-(x) + 1.0) / 2.0)
#define PERCENTAGE_TO_CONTROL(x) (-(SFACTOR_BAR_HEIGHT/100) * (x) + 1.0)

static void update_sfactor_value(double new_value);
static struct sfactor_def
{
  char label[13];
  double value;
}
  sfactor_type[3] =
{ 
  { "Approximated", S_SPLINE_APPROX  },
  { "Angular",      S_SPLINE_ANGULAR },
  { "Interpolated", S_SPLINE_INTERP  }
};

static void      make_window_spline_point(F_spline *s, int x, int y);

static Widget    sfactor_bar;
static F_sfactor *edited_sfactor, *sub_sfactor;
static F_point   *edited_point;
static F_spline  *sub_new_s;
static int       num_spline_points;

/*************************************/
/* end of specific stuff for splines */
/*************************************/


static struct {
    int		 thickness;
    Color	 pen_color;
    Color	 fill_color;
    int		 depth;
    int		 arc_type;
    int		 cap_style;
    int		 join_style;
    int		 style;
    float	 style_val;
    int		 pen_style;
    int		 fill_style;
    char	*comments;
    F_arrow	 for_arrow;
    F_arrow	 back_arrow;
}	generic_vals;

#define put_generic_vals(x) \
	generic_vals.thickness	= x->thickness; \
	generic_vals.pen_color	= x->pen_color; \
	generic_vals.fill_color	= x->fill_color; \
	generic_vals.depth	= x->depth; \
	generic_vals.style	= x->style; \
	generic_vals.style_val	= x->style_val; \
	generic_vals.pen_style	= x->pen_style; \
	generic_vals.fill_style = x->fill_style

#define get_generic_vals(x) \
	new_generic_values(); \
	x->thickness	= generic_vals.thickness; \
	x->pen_color	= generic_vals.pen_color; \
	x->fill_color	= generic_vals.fill_color; \
	x->depth	= generic_vals.depth; \
	x->style	= generic_vals.style; \
	x->style_val	= generic_vals.style_val; \
	x->pen_style	= generic_vals.pen_style; \
	x->fill_style	= generic_vals.fill_style; \
	x->comments	= generic_vals.comments

#define put_join_style(x) \
	generic_vals.join_style = x->join_style;

#define get_join_style(x) \
	x->join_style = generic_vals.join_style;

#define put_cap_style(x) \
	generic_vals.cap_style = x->cap_style;

#define get_cap_style(x) \
	x->cap_style = generic_vals.cap_style;

#define put_arc_type(x) \
	generic_vals.arc_type = x->type;


void make_window_line (F_line *l);
void make_window_text (F_text *t);
void make_window_ellipse (F_ellipse *e);
void make_window_arc (F_arc *a);
void make_window_spline (F_spline *s);
void make_window_compound (F_compound *c);
void make_window_figure (void);
void check_depth (void);
void text_transl (Widget w);
void check_thick (void);
void reset_edit_cursor (void);
void arc_type_menu (void);
void fill_style_menu (int fill, int fill_flag);
void cap_style_panel_menu (void);
void join_style_panel_menu (void);
void set_image_pm (int val);
void fill_style_sens (Boolean state);
void fill_pat_sens (Boolean state);
void collapse_depths (F_compound *compound);

void get_arc_type(F_arc *arc)
{
    arc->type = generic_vals.arc_type; 
    /* remove any arrowheads from pie-wedge style arc */
    if (arc->type == T_PIE_WEDGE_ARC) {
	if (arc->for_arrow) {
	    free((char *) arc->for_arrow);
	    arc->for_arrow = NULL;
	}
	if (arc->back_arrow) {
	    free((char *) arc->back_arrow);
	    arc->back_arrow = NULL;
	}
    }
}

/* NOTE: This procedure requires that the structure components for f_arc, f_line
	and f_spline are in the same order up to and including the arrows */

void put_generic_arrows(F_line *x)
{
    for_arrow = (x->for_arrow != NULL);
    back_arrow = (x->back_arrow != NULL);
    if (for_arrow) {
	generic_vals.for_arrow.type	= x->for_arrow->type;
	generic_vals.for_arrow.style	= x->for_arrow->style;
	generic_vals.for_arrow.thickness = x->for_arrow->thickness;
	generic_vals.for_arrow.wd	= x->for_arrow->wd;
	generic_vals.for_arrow.ht	= x->for_arrow->ht;
    } else {
	generic_vals.for_arrow.type	= -1;
	generic_vals.for_arrow.style	= -1;
	/* no existing arrow, set dialog to current ind panel settings */
	if (use_abs_arrowvals) {
	    generic_vals.for_arrow.thickness = cur_arrowthick;
	    generic_vals.for_arrow.wd	= cur_arrowwidth;
	    generic_vals.for_arrow.ht	= cur_arrowheight;
	} else {
	    /* use multiple of current line thickness */
	    generic_vals.for_arrow.thickness = cur_arrow_multthick * x->thickness;
	    generic_vals.for_arrow.wd	= cur_arrow_multwidth * x->thickness;
	    generic_vals.for_arrow.ht	= cur_arrow_multheight * x->thickness;
	}
    }
    if (back_arrow) {
	generic_vals.back_arrow.type	= x->back_arrow->type;
	generic_vals.back_arrow.style	= x->back_arrow->style;
	generic_vals.back_arrow.thickness = x->back_arrow->thickness;
	generic_vals.back_arrow.wd	= x->back_arrow->wd;
	generic_vals.back_arrow.ht	= x->back_arrow->ht;
    } else {
	generic_vals.back_arrow.type	= -1;
	generic_vals.back_arrow.style	= -1;
	/* no existing arrow, set dialog to current ind panel settings */
	if (use_abs_arrowvals) {
	    generic_vals.back_arrow.thickness = cur_arrowthick;
	    generic_vals.back_arrow.wd	= cur_arrowwidth;
	    generic_vals.back_arrow.ht	= cur_arrowheight;
	} else {
	    /* use multiple of current line thickness */
	    generic_vals.back_arrow.thickness = cur_arrow_multthick * x->thickness;
	    generic_vals.back_arrow.wd	= cur_arrow_multwidth * x->thickness;
	    generic_vals.back_arrow.ht	= cur_arrow_multheight * x->thickness;
	}
    }
}

/* NOTE: This procedure requires that the structure components for f_arc, f_line
	and f_spline are in the same order up to and including the arrows */

void get_generic_arrows(F_line *x)
{
	new_arrow_values();
	if (for_arrow) {
	    if (!x->for_arrow)
		x->for_arrow = create_arrow();
	    x->for_arrow->type = generic_vals.for_arrow.type;
	    x->for_arrow->style = generic_vals.for_arrow.style;
	    x->for_arrow->thickness = (float) fabs((double) generic_vals.for_arrow.thickness);
	    x->for_arrow->wd = (float) fabs((double) generic_vals.for_arrow.wd);
	    x->for_arrow->ht = (float) fabs((double) generic_vals.for_arrow.ht);
	} else {
	    if (x->for_arrow)
		free((char *) x->for_arrow);
	    x->for_arrow = (F_arrow *) NULL;
	}
	if (back_arrow) {
	    if (!x->back_arrow)
		x->back_arrow = create_arrow();
	    x->back_arrow->type = generic_vals.back_arrow.type;
	    x->back_arrow->style = generic_vals.back_arrow.style;
	    x->back_arrow->thickness = (float) fabs((double) generic_vals.back_arrow.thickness);
	    x->back_arrow->wd = (float) fabs((double) generic_vals.back_arrow.wd);
	    x->back_arrow->ht = (float) fabs((double) generic_vals.back_arrow.ht);
	} else {
	    if (x->back_arrow)
		free((char *) x->back_arrow);
	    x->back_arrow = (F_arrow *) NULL;
	}
}

void	edit_item(F_line *p, int type, int x, int y);
void	edit_spline_point(F_spline *spline, int type, int x, int y, F_point *previous_point, F_point *the_point);
void	edit_figure_comments(int x, int y, unsigned int shift);

void edit_item_selected(void)
{
    set_mousefun("edit object", "edit Main comment", "edit point", 
			LOC_OBJ, "show comments", LOC_OBJ);
    canvas_kbd_proc = null_proc;
    canvas_locmove_proc = null_proc;
    canvas_ref_proc = null_proc;
    init_searchproc_left(edit_item);
    init_searchproc_right(edit_spline_point);
    canvas_leftbut_proc = object_search_left;
    canvas_middlebut_proc = edit_figure_comments;
    canvas_rightbut_proc = point_search_right;
    set_cursor(pick9_cursor);
    reset_action_on();
}

/* 
   this handles both viewing of any object's comments (shift=1) and 
   editing of the whole figure comments (shift=0)
*/

void	popup_show_comments(F_line *p, int type, int x, int y);

void
edit_figure_comments(int x, int y, unsigned int shift)
       		         
                          	/* Shift Key Status from XEvent */
{
    if (shift) {
	/* locate the object and popup the comments panel */
	init_searchproc_left(popup_show_comments);
	object_search_left(x, y, False);
	/* reset search proc function */
	init_searchproc_left(edit_item);
    } else {
	/* popup an edit window for the whole figure */
	edit_item(&objects, O_FIGURE, 0, 0);
    }
}

void popup_show_comments(F_line *p, int type, int x, int y)
{
	Widget	    form;
	static Boolean  actions_added = False;
	char	   *comments;
	F_arc      *a;
	F_compound *c;
	F_ellipse  *e;
	F_line	   *l;
	F_spline   *s;
	F_text     *t;

	switch (type) {
	    case O_ARC:  
		a = (F_arc *) p;
		comments = a->comments;
		break;
	    case O_COMPOUND:  
		c = (F_compound *) p;
		comments = c->comments;
		break;
	    case O_ELLIPSE:  
		e = (F_ellipse *) p;
		comments = e->comments;
		break;
	    case O_POLYLINE:  
		l = (F_line *) p;
		comments = l->comments;
		break;
	    case O_SPLINE:  
		s = (F_spline *) p;
		comments = s->comments;
		break;
	    case O_TEXT:  
		t = (F_text *) p;
		comments = t->comments;
		break;
	} /* switch */

	/* locate the mouse in screen coords */
	XtTranslateCoords(canvas_sw, ZOOMX(x), ZOOMY(y), &rootx, &rooty);
	/* popup a panel showing the object comments */
	FirstArg(XtNtitle, "Xfig: Object comments");
	NextArg(XtNcolormap, tool_cm);
	NextArg(XtNx, rootx-10);	/* pop it up just under the mouse */
	NextArg(XtNy, rooty-10);
	comment_popup = XtCreatePopupShell("show_comments",
			       overrideShellWidgetClass, tool,
			       Args, ArgCount);
	FirstArg(XtNborderWidth, 1);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	form = XtCreateManagedWidget("comment_form", formWidgetClass, 
				comment_popup, Args, ArgCount);
	FirstArg(XtNlabel, "Comments:");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("comment_label", labelWidgetClass,
				       form, Args, ArgCount);
	/* make label widgets for comment lines */
	if (comments == NULL)
	    comments = "(None)";
	FirstArg(XtNlabel, comments);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 2);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	below = XtCreateManagedWidget("comments", labelWidgetClass, 
				form, Args, ArgCount);
	XtAugmentTranslations(comment_popup,
			XtParseTranslationTable(showcomment_popup_translations));
	if (!actions_added) {
	    XtAppAddActions(tool_app, showcomment_actions, XtNumber(showcomment_actions));
	    actions_added = True;
	}
	XtPopupSpringLoaded(comment_popup);
}

static void
popdown_comments(void)
{
	XtDestroyWidget(comment_popup);
}

void edit_item(F_line *p, int type, int x, int y)
{
    XtWidgetGeometry xtgeom,comp;
    int		    llx, lly, urx, ury, dum;
    Dimension	    w, h;
    Position	    rootlx, rootly, rootux, rootuy;

    /* make some pixmaps if we haven't already */
    if (joinstyle_pixmaps[0] == 0) {
	joinstyle_pixmaps[0] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) joinmiter_ic.bits, joinmiter_ic.width, joinmiter_ic.height); 
	joinstyle_pixmaps[1] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) joinround_ic.bits, joinround_ic.width, joinround_ic.height); 
	joinstyle_pixmaps[2] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) joinbevel_ic.bits, joinbevel_ic.width, joinbevel_ic.height); 
    }
    if (capstyle_pixmaps[0] == 0) {
	capstyle_pixmaps[0] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) capbutt_ic.bits, capbutt_ic.width, capbutt_ic.height); 
	capstyle_pixmaps[1] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) capround_ic.bits, capround_ic.width, capround_ic.height); 
	capstyle_pixmaps[2] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) capproject_ic.bits, capproject_ic.width, capproject_ic.height); 
    }

    changed = False;
    reread_file = False;
    /* make a window based on the object being edited */
    /* also get the bounds of the object to position the popup away from it */
    switch (type) {
      case O_POLYLINE:
	line_bound((F_line *) p, &llx, &lly, &urx, &ury);
	make_window_line((F_line *) p);
	break;
      case O_TEXT:
	text_bound((F_text *) p, &llx, &lly, &urx, &ury,
		&dum,&dum,&dum,&dum,&dum,&dum,&dum,&dum);
	make_window_text((F_text *) p);
	break;
      case O_ELLIPSE:
	ellipse_bound((F_ellipse *) p, &llx, &lly, &urx, &ury);
	make_window_ellipse((F_ellipse *) p);
	break;
      case O_ARC:
	arc_bound((F_arc *) p, &llx, &lly, &urx, &ury);
	make_window_arc((F_arc *) p);
	break;
      case O_SPLINE:
	spline_bound((F_spline *) p, &llx, &lly, &urx, &ury);
	make_window_spline((F_spline *) p);
	break;
      case O_COMPOUND:
	compound_bound((F_compound *) p, &llx, &lly, &urx, &ury);
	/* turn on the point positioning indicator since it is used for editing compound */
	update_indpanel(I_MIN2);
	make_window_compound((F_compound *) p);
	break;
      case O_FIGURE:
	compound_bound((F_compound *) p, &llx, &lly, &urx, &ury);
	make_window_figure();
	break;
    }

    /* try to position the window so it doesn't obscure the object being edited */

    /* first realize the popup widget so we can get its width */
    XtRealizeWidget(popup);

    /* translate object coords to screen coords relative to the canvas */
    llx = ZOOMX(llx);
    urx = ZOOMX(urx);
    lly = ZOOMY(lly);
    ury = ZOOMY(ury);

    /* translate those to absolute screen coords */
    XtTranslateCoords(canvas_sw, llx, lly, &rootlx, &rootly);
    XtTranslateCoords(canvas_sw, urx, ury, &rootux, &rootuy);

    /* size of popup window */
    FirstArg(XtNwidth, &w);
    NextArg(XtNheight, &h);
    GetValues(popup);

    /* try putting it just to the right of the object */
    if (rootux + 10 + w < screen_wd) {
	x = rootux+10;
    } else { 
	/* else put it to the left of the object */
	x = rootlx - 10 - w;
	/* but always on the screen */
	if (x < 0)
	    x = 0;
    }
    y = 40;

    /* only change X position of widget */
    xtgeom.request_mode = CWX|CWY;
    xtgeom.x = x;
    xtgeom.y = y;
    (void) XtMakeGeometryRequest(popup, &xtgeom, &comp);

    /* now pop it up */
    XtPopup(popup, XtGrabNonexclusive);
    popup_up = True;
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();

    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(popup));
    (void) XSetWMProtocols(tool_d, XtWindow(popup), &wm_delete_window, 1);
}

static void
reread_picfile(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    Boolean	    dum;

    if (new_l->pic->pic_cache == 0)
	return;		/* hmm, shouldn't get here if no picture */

    changed = True;
    reread_file = True;	/* force remapping colors */
    list_delete_line(&objects.lines, new_l);
    redisplay_line(new_l);
    /* reread the file */
    read_picobj(new_l->pic, new_l->pic->pic_cache->file, new_l->pen_color, True, &dum);
    /* calculate h/w ratio */
    new_l->pic->hw_ratio = (float) new_l->pic->pic_cache->bit_size.y/new_l->pic->pic_cache->bit_size.x;
    get_new_line_values();
    list_add_line(&objects.lines, new_l);
    redisplay_line(new_l);
}


void edit_spline_point(F_spline *spline, int type, int x, int y, F_point *previous_point, F_point *the_point)
{
    if (type!=O_SPLINE) {
	put_msg("Only spline points can be edited");
	return;
    }

    if (open_spline(spline) && (previous_point==NULL || the_point->next==NULL)) {
	put_msg("Cannot edit end-points");
	return;
    }

    changed = False;
    make_window_spline_point(spline, the_point->x, the_point->y);

    XtPopup(popup, XtGrabNonexclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
  
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(popup));
    (void) XSetWMProtocols(tool_d, XtWindow(popup), &wm_delete_window, 1);
}

static void
expand_pic(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;
    register float  orig_ratio = new_l->pic->hw_ratio;

    p1.x = panel_get_dim_value(x1_panel);
    p1.y = panel_get_dim_value(y1_panel);
    p2.x = panel_get_dim_value(x2_panel);
    p2.y = panel_get_dim_value(y2_panel);
    /* size is upper-lower */
    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    rotation = what_rotation(dx,dy);
    if (dx == 0 || dy == 0 || orig_ratio == 0.0)
	return;
    if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
	(rotation != 0 && rotation != 180 && flip_pic_flag)) {
	ratio = (float) fabs((double) dy / (double) dx);
	if (ratio < orig_ratio)
	    p2.y = p1.y + signof(dy) * (int) (fabs((double) dx) * orig_ratio);
	else
	    p2.x = p1.x + signof(dx) * (int) (fabs((double) dy) / orig_ratio);
    } else {
	ratio = (float) fabs((double) dx / (double) dy);
	if (ratio < orig_ratio)
	    p2.x = p1.x + signof(dx) * (int) (fabs((double) dy) * orig_ratio);
	else
	    p2.y = p1.y + signof(dy) * (int) (fabs((double) dx) / orig_ratio);
    }
    panel_set_scaled_int(x2_panel, p2.x);
    panel_set_scaled_int(y2_panel, p2.y);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
shrink_pic(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;
    register float  orig_ratio = new_l->pic->hw_ratio;

    p1.x = panel_get_dim_value(x1_panel);
    p1.y = panel_get_dim_value(y1_panel);
    p2.x = panel_get_dim_value(x2_panel);
    p2.y = panel_get_dim_value(y2_panel);
    /* size is upper-lower */
    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    rotation = what_rotation(dx,dy);
    if (dx == 0 || dy == 0 || orig_ratio == 0.0)
	return;
    if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
	(rotation != 0 && rotation != 180 && flip_pic_flag)) {
	ratio = (float) fabs((double) dy / (double) dx);
	/* upper coord is lower+size */
	if (ratio > orig_ratio)
	    p2.y = p1.y + signof(dy) * (int) (fabs((double) dx) * orig_ratio);
	else
	    p2.x = p1.x + signof(dx) * (int) (fabs((double) dy) / orig_ratio);
    } else {
	ratio = (float) fabs((double) dx / (double) dy);
	if (ratio > orig_ratio)
	    p2.x = p1.x + signof(dx) * (int) (fabs((double) dy) * orig_ratio);
	else
	    p2.y = p1.y + signof(dy) * (int) (fabs((double) dx) / orig_ratio);
    }
    panel_set_scaled_int(x2_panel, p2.x);
    panel_set_scaled_int(y2_panel, p2.y);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
origsize_pic(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    struct f_point  p1, p2;
    int		    dx, dy;
    register float  orig_ratio = new_l->pic->hw_ratio;

    p1.x = panel_get_dim_value(x1_panel);
    p1.y = panel_get_dim_value(y1_panel);
    p2.x = panel_get_dim_value(x2_panel);
    p2.y = panel_get_dim_value(y2_panel);

    /* size is upper-lower */
    dx = p2.x - p1.x;
    dy = p2.y - p1.y;

    if (dx == 0 || dy == 0 || orig_ratio == 0.0)
	return;

    /* upper coord is lower+size */
    p2.x = p1.x + signof(dx) * new_l->pic->pic_cache->size_x;
    p2.y = p1.y + signof(dy) * new_l->pic->pic_cache->size_y;
    panel_set_scaled_int(x2_panel, p2.x);
    panel_set_scaled_int(y2_panel, p2.y);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
scale_percent_pic(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    struct f_point  p1, p2;
    int		    dx, dy;
    float	    orig_ratio = new_l->pic->hw_ratio;
    float	    pct;

    if (orig_ratio == 0.0)
	return;

    p1.x = panel_get_dim_value(x1_panel);
    p1.y = panel_get_dim_value(y1_panel);
    p2.x = panel_get_dim_value(x2_panel);
    p2.y = panel_get_dim_value(y2_panel);

    /* get the percent scale */
    pct = atof(panel_get_value(percent_entry));
    if (pct <= 0.0) {
	panel_set_value(percent_entry,"0.001");
	pct = 0.001;
    }

    /* find direction of corners - size is upper-lower */
    dx = p2.x - p1.x;
    dy = p2.y - p1.y;

    /* upper coord is lower+size */
    p2.x = p1.x + (signof(dx) * new_l->pic->pic_cache->size_x * pct/100.0);
    p2.y = p1.y + (signof(dy) * new_l->pic->pic_cache->size_y * pct/100.0);
    panel_set_scaled_int(x2_panel, p2.x);
    panel_set_scaled_int(y2_panel, p2.y);
    sprintf(buf, "%1.1f", orig_ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}


/* make a panel for the user to change the comments for the whole figure */

void make_window_figure(void)
{
    generic_window("Whole Figure", "", &figure_ic, done_figure_comments,
		NO_GENERICS, NO_ARROWS, objects.comments);
}

static void
done_figure_comments(void)
{
    char	*s;

    switch (button_result) {
      case DONE:
	/* save old comments */
	saved_objects.comments = objects.comments;
	/* get new comments */
	s = panel_get_value(comments_panel);
	/* allocate space and copy */
	copy_comments(&s, &objects.comments);
	clean_up();
	set_action_object(F_EDIT, O_FIGURE);
	set_modifiedflag();
	break;
      case CANCEL:
	break;
    }
}

void make_window_compound(F_compound *c)
{
    F_text	*t;
    int		 i;
    Widget	 save_form, viewp;
    F_pos	 dimen;		/* need temp storage for width, height panel */

    set_cursor(panel_cursor);
    mask_toggle_compoundmarker(c);
    old_c = copy_compound(c);
    new_c = c;

    generic_window("COMPOUND", "", &glue_ic, done_compound,
				NO_GENERICS, NO_ARROWS, c->comments);

    /* tell the pulldown unit menu which panels to convert */
    init_convert_array();
    f_pair_panel(&c->nwcorner, "Top left corner", 
				&x1_panel, "X =", &y1_panel, "Y =", True);

    /* override translations for Return to reposition top-left of compound */
    XtOverrideTranslations(x1_panel, XtParseTranslationTable(edit_compound_translations));
    XtOverrideTranslations(y1_panel, XtParseTranslationTable(edit_compound_translations));

    f_pair_panel(&c->secorner, "Bottom right corner", 
				&x2_panel, "X =", &y2_panel, "Y =", False);

    /* override translations for Return to reposition bottom-right of compound */
    XtOverrideTranslations(x2_panel, XtParseTranslationTable(edit_compound_translations));
    XtOverrideTranslations(y2_panel, XtParseTranslationTable(edit_compound_translations));

    dimen.x = c->secorner.x - c->nwcorner.x;
    dimen.y = c->secorner.y - c->nwcorner.y;
    f_pair_panel(&dimen, "Dimensions", &width_panel, "Width =", 
					&height_panel, "Height =", False);

    /* override translations for Return to scale compound */
    XtOverrideTranslations(width_panel, XtParseTranslationTable(edit_compound_translations));
    XtOverrideTranslations(height_panel, XtParseTranslationTable(edit_compound_translations));

    FirstArg(XtNlabel, "Depths");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromVert, below);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = XtCreateManagedWidget("min_depth", labelWidgetClass,
				       form, Args, ArgCount);

    /* show min/max depths in compound */
    min_compound_depth = find_smallest_depth(c);
    sprintf(buf,"Minimum: %d", min_compound_depth);
    FirstArg(XtNlabel, buf);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNhorizDistance, 10);
    NextArg(XtNfromVert, below);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    min_depth_w = beside = XtCreateManagedWidget("min_depth", labelWidgetClass,
				       form, Args, ArgCount);
    sprintf(buf,"Maximum: %d", find_largest_depth(c));
    FirstArg(XtNlabel, buf);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, below);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    max_depth_w = below = XtCreateManagedWidget("max_depth", labelWidgetClass,
				       form, Args, ArgCount);

    /* button to collapse depths */
    FirstArg(XtNlabel, "Collapse Depths");
    NextArg(XtNfromVert, below);
    NextArg(XtNhorizDistance, 10);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = XtCreateManagedWidget("collapse_depths", commandWidgetClass,
				       form, Args, ArgCount);
    XtAddCallback(below, XtNcallback, (XtCallbackProc) collapse_depth, (XtPointer) NULL);

    int_label(object_count(c), "Number of objects", &num_objects);

    /* make a form to contain the text objects in the compound */
    if (c->texts) {
	/* save toplevel form so that the following widgets will go in this sub-form */
	save_form = form;
	/* form to frame the text heading with the texts */
	FirstArg(XtNborderWidth, 1);
	NextArg(XtNfromVert, below);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	form = XtCreateManagedWidget("form", formWidgetClass, save_form,
					Args, ArgCount);
	FirstArg(XtNlabel, "Text objects in this compound");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("label", labelWidgetClass, form,
					Args, ArgCount);
	/* first make a viewport in case we need a scrollbar */
	FirstArg(XtNallowVert, True);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, below);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	viewp = XtCreateManagedWidget("textview", viewportWidgetClass, form,
					Args, ArgCount);
	/* form in the viewport */
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	form = XtCreateManagedWidget("form", formWidgetClass, viewp, Args, ArgCount);
	below = NULL;
	for (t=c->texts, i=0; t && i<MAX_COMPOUND_TEXT_PANELS; t=t->next, i++) {
	    str_panel(t->cstring, "Text", &compound_text_panels[i], 220, True, False);
	    /* make the margins small */
	    FirstArg(XtNhorizDistance, 1);
	    SetValues(below);
	    if (i>0) {
		/* scrunch the texts together vertically */
		FirstArg(XtNvertDistance, 1);
		SetValues(below);
	    }
	}
	/* if many texts, set height of viewport */
	if (i > 8) {
	    FirstArg(XtNheight, 320);
	    SetValues(viewp);
	}
	/* restore original form */
	form = save_form;
    }
}

/* come here when user presses return in any compound corner/size entry */

static void
modify_compound(Widget w)
{
    if (w == x1_panel || w == y1_panel)
	reposition_top();
    else if (w == x2_panel || w == y2_panel)
	reposition_bottom();
    else if (w == width_panel || w == height_panel)
	rescale_compound();
}

/* come here when user presses return in picture X/Y position */

static void
reposition_picture(Widget w)
{
    int x1, y1, x2, y2;
    int width, height, rotation;

    x1 = panel_get_dim_value(x1_panel);
    y1 = panel_get_dim_value(y1_panel);
    x2 = panel_get_dim_value(x2_panel);
    y2 = panel_get_dim_value(y2_panel);
    /* calculate new width, height - keep sign for now to determine rotation */
    width = x2-x1;
    height = y2-y1;
    rotation = 0;
    if (width < 0 && height < 0)
	rotation = 2;	/* 180 degrees */
    else if (width < 0 && height >= 0)
	rotation = 3;	/* 270 degrees */
    else if (height < 0 && width >= 0)
	rotation = 1;	/* 90 degrees */
    FirstArg(XtNlabel, rotation_items[rotation]);
    SetValues(rotation_panel);

    /* absolute value */
    width = abs(width);
    height = abs(height);
    if (width > 50*PIX_PER_INCH) {
	file_msg("Picture too wide (> 50 inches)");
	return;
    }
    if (height > 50*PIX_PER_INCH) {
	file_msg("Picture too tall (> 50 inches)");
	return;
    }
    /* put back */
    panel_set_scaled_int(width_panel, width);
    panel_set_scaled_int(height_panel, height);
    /* resize the picture */
    edit_apply(w, (XButtonEvent *) 0, (String*) 0, (Cardinal*) 0);
}

/* come here when user presses return in picture width or height */

static void
resize_picture(Widget w)
{
    int x1, y1, x2, y2;
    int width, height;

    x1 = panel_get_dim_value(x1_panel);
    y1 = panel_get_dim_value(y1_panel);
    x2 = panel_get_dim_value(x2_panel);
    y2 = panel_get_dim_value(y2_panel);
    width = panel_get_dim_value(width_panel);
    height = panel_get_dim_value(height_panel);
    if (width > 50*PIX_PER_INCH) {
	file_msg("Picture too wide (> 50 inches)");
	return;
    }
    if (height > 50*PIX_PER_INCH) {
	file_msg("Picture too tall (> 50 inches)");
	return;
    }
    if (w == width_panel) {
	/* adjust x to reflect new width */
	if (x1 < x2) {
	    x2 = x1 + width;
	    panel_set_scaled_int(x2_panel, x2);
	} else {
	    x1 = x2 + width;
	    panel_set_scaled_int(x1_panel, x1);
	}
    } else {
	/* adjust y to reflect new height */
	if (y1 < y2) {
	    y2 = y1 + height;
	    panel_set_scaled_int(y2_panel, y2);
	} else {
	    y1 = y2 + height;
	    panel_set_scaled_int(y1_panel, y1);
	}
    }
    /* resize the picture */
    edit_apply(w, (XButtonEvent *) 0, (String*) 0, (Cardinal*) 0);
}

/* reposition the compound using the user's new upper-left values */

static void
reposition_top(void)
{
    int		 nw_x, nw_y, se_x, se_y, dx, dy;

    /* get user's new top-left corner values */
    nw_x = panel_get_dim_value(x1_panel);
    nw_y = panel_get_dim_value(y1_panel);

    /* get shift */
    dx = nw_x - new_c->nwcorner.x;
    dy = nw_y - new_c->nwcorner.y;

    /* adjust lower-right corner */
    se_x = new_c->secorner.x + dx;
    se_y = new_c->secorner.y + dy;

    /* and update those values in panel */
    panel_set_scaled_int(x2_panel, se_x);
    panel_set_scaled_int(y2_panel, se_y);

    /* update the compound on the screen */
    button_result = APPLY;
    done_compound();
}

/* reposition the compound using the user's new lower-right values */

static void
reposition_bottom(void)
{
    int		 nw_x, nw_y, se_x, se_y, dx, dy;

    /* get user's new lower-right corner values */
    se_x = panel_get_dim_value(x2_panel);
    se_y = panel_get_dim_value(y2_panel);

    /* get shift */
    dx = se_x - new_c->secorner.x;
    dy = se_y - new_c->secorner.y;

    /* adjust upper-left corner */
    nw_x = new_c->nwcorner.x + dx;
    nw_y = new_c->nwcorner.y + dy;

    /* and update those values in panel */
    panel_set_scaled_int(x1_panel, nw_x);
    panel_set_scaled_int(y1_panel, nw_y);

    /* update the compound on the screen */
    button_result = APPLY;
    done_compound();
}

/* rescale the compound using the user's new width/height values */
/* it is rescaled relative to the upper-left corner */

static void
rescale_compound(void)
{
    int		 width, height, se_x, se_y;

    /* get user's new width/height values */
    width = panel_get_dim_value(width_panel);
    height = panel_get_dim_value(height_panel);

    /* adjust lower-right corner with new size */
    se_x = new_c->nwcorner.x + width;
    se_y = new_c->nwcorner.y + height;

    /* update the lower-right corner values in panel */
    panel_set_scaled_int(x2_panel, se_x);
    panel_set_scaled_int(y2_panel, se_y);

    /* update the compound on the screen */
    button_result = APPLY;
    done_compound();
}

static void
get_new_compound_values(void)
{
    int		 dx, dy, nw_x, nw_y, se_x, se_y;
    float	 scalex, scaley;
    F_text	*t;
    int		 i;
    PR_SIZE	 size;

    nw_x = panel_get_dim_value(x1_panel);
    nw_y = panel_get_dim_value(y1_panel);
    se_x = panel_get_dim_value(x2_panel);
    se_y = panel_get_dim_value(y2_panel);
    dx = nw_x - new_c->nwcorner.x;
    dy = nw_y - new_c->nwcorner.y;
    if (new_c->nwcorner.x - new_c->secorner.x == 0)
	scalex = 0.0;
    else
	scalex = (float) (nw_x - se_x) /
		(float) (new_c->nwcorner.x - new_c->secorner.x);
    if (new_c->nwcorner.y - new_c->secorner.y == 0)
	scaley = 0.0;
    else
	scaley = (float) (nw_y - se_y) /
		(float) (new_c->nwcorner.y - new_c->secorner.y);

    /* get any comments */
    new_c->comments = my_strdup(panel_get_value(comments_panel));

    /* get any new text object values */
    for (t=new_c->texts,i=0; t ;t=t->next,i++) {
	if (t->cstring)
	    free(t->cstring);
	t->cstring = my_strdup(panel_get_value(compound_text_panels[i]));
	/* calculate new size */
	/* get the fontstruct for zoom = 1 to get the size of the string */
	canvas_font = lookfont(x_fontnum(psfont_text(t), t->font), t->size);
	size = textsize(canvas_font, strlen(t->cstring), t->cstring);
	t->length = size.length;
	t->ascent = size.ascent;
	t->descent = size.descent;
    }

    translate_compound(new_c, dx, dy);
    scale_compound(new_c, scalex, scaley, nw_x, nw_y);
}

static void
done_compound(void)
{
    switch (button_result) {

      case APPLY:
	changed = True;
	list_delete_compound(&objects.compounds, new_c);
	redisplay_compound(new_c);
	get_new_compound_values();
	compound_bound(new_c, &new_c->nwcorner.x, &new_c->nwcorner.y,
		   &new_c->secorner.x, &new_c->secorner.y);
	/* update top-left and bottom-right values in panel from new bounding box */
	panel_set_scaled_int(x1_panel, new_c->nwcorner.x);
	panel_set_scaled_int(y1_panel, new_c->nwcorner.y);
	panel_set_scaled_int(x2_panel, new_c->secorner.x);
	panel_set_scaled_int(y2_panel, new_c->secorner.y);
	list_add_compound(&objects.compounds, new_c);
	redisplay_compound(new_c);
	toggle_compoundmarker(new_c);
	break;

      case DONE:
	get_new_compound_values();
	compound_bound(new_c, &new_c->nwcorner.x, &new_c->nwcorner.y,
		   &new_c->secorner.x, &new_c->secorner.y);
	/* update top-left and bottom-right values in panel from new bounding box */
	panel_set_scaled_int(x1_panel, new_c->nwcorner.x);
	panel_set_scaled_int(y1_panel, new_c->nwcorner.y);
	panel_set_scaled_int(x2_panel, new_c->secorner.x);
	panel_set_scaled_int(y2_panel, new_c->secorner.y);
	redisplay_compounds(new_c, old_c);
	clean_up();
	old_c->next = new_c;
	set_latestcompound(old_c);
	set_action_object(F_EDIT, O_COMPOUND);
	set_modifiedflag();
	remove_compound_depth(old_c);
	add_compound_depth(new_c);
	/* if this was a place-and-edit, continue with library place */
	if (edit_remember_lib_mode) {
	    edit_remember_lib_mode = False;
	    put_selected();
	    /* and set flag to reset cursor back to place lib */
	    lib_cursor = True;
	}
	/* if this was a dimension line edit, continue with line mode */
	if (edit_remember_dimline_mode) {
	    edit_remember_dimline_mode = False;
	    line_drawing_selected();
	    /* and set flag to reset cursor back to place lib */
	    dim_cursor = True;
	}
	break;

      case CANCEL:
	list_delete_compound(&objects.compounds, new_c);
	list_add_compound(&objects.compounds, old_c);
	if (saved_objects.compounds && saved_objects.compounds->next &&
	    saved_objects.compounds->next == new_c)
		saved_objects.compounds->next = old_c;
	else if (saved_objects.compounds == new_c)
		saved_objects.compounds = old_c;
	if (changed)
	    redisplay_compounds(old_c, new_c);
	else
	    toggle_compoundmarker(old_c);
	free_compound(&new_c);
	/* if this was a place-and-edit, continue with library place */
	if (edit_remember_lib_mode) {
	    edit_remember_lib_mode = False;
	    put_selected();
	    /* and set flag to reset cursor back to place lib */
	    lib_cursor = True;
	}
	/* if this was a dimension line edit, continue with line mode */
	if (edit_remember_dimline_mode) {
	    edit_remember_dimline_mode = False;
	    line_drawing_selected();
	    /* and set flag to reset cursor back to place lib */
	    dim_cursor = True;
	}
	break;
    }
}

void make_window_line(F_line *l)
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;
    int		    i, vdist;
    F_pos	    dimen;		/* need temp storage for width, height panel */
    Widget	    type;

    set_cursor(panel_cursor);
    mask_toggle_linemarker(l);
    old_l = copy_line(l);
    new_l = l;

    put_generic_vals(new_l);
    put_cap_style(new_l);
    put_join_style(new_l);
    pen_color = new_l->pen_color;
    fill_color = new_l->fill_color;

    /* tell the pulldown unit menu which panels to convert */
    init_convert_array();

    /* single-point lines don't get arrows - delete any that might already exist */
    if (new_l->points->next == NULL) {
	if (new_l->for_arrow)
		free((char *) new_l->for_arrow);
	if (new_l->back_arrow)
		free((char *) new_l->back_arrow);
	new_l->for_arrow = new_l->back_arrow = (F_arrow *) NULL;
    }
    switch (new_l->type) {
      case T_POLYLINE:
	put_generic_arrows(new_l);
	/* don't make arrow panels if single-point line */
	generic_window("POLYLINE", "Polyline", &line_ic, done_line, 
			GENERICS, (new_l->points->next? ARROWS: NO_ARROWS), l->comments);
	points_panel(new_l->points);
	break;
      case T_POLYGON:
	generic_window("POLYLINE", "Polygon", &polygon_ic, done_line, 
			GENERICS, NO_ARROWS, l->comments);
	points_panel(new_l->points);
	break;
      case T_BOX:
	generic_window("POLYLINE", "Box", &box_ic, done_line, 
			GENERICS, NO_ARROWS, l->comments);
	p1 = *new_l->points;
	p2 = *new_l->points->next->next;
	xy_panel(p1.x, p1.y, "First corner", &x1_panel, &y1_panel, True);
	xy_panel(p2.x, p2.y, "Opposite corner", &x2_panel, &y2_panel, False);
	break;
      case T_ARCBOX:
	generic_window("POLYLINE", "ArcBox", &arc_box_ic, done_line, 
			GENERICS, NO_ARROWS, l->comments);
	p1 = *new_l->points;
	p2 = *new_l->points->next->next;
	(void) int_panel(new_l->radius, form, "Corner radius", (Widget) 0, &radius,
					MIN_BOX_RADIUS, MAX_BOX_RADIUS, 1);
	xy_panel(p1.x, p1.y, "First corner", &x1_panel, &y1_panel, True);
	xy_panel(p2.x, p2.y, "Opposite corner", &x2_panel, &y2_panel, False);
	break;
      case T_PICTURE:
	old_l->type = T_BOX;	/* so colors of old won't be included in new */
	generic_window("POLYLINE", "Picture Object", &picobj_ic, done_line, 
			NO_GENERICS, NO_ARROWS, l->comments);

	below = pen_color_selection_panel();
	/* only the XBM (bitmap) type has a pen color */
	if (new_l->pic != 0 && new_l->pic->pic_cache && new_l->pic->pic_cache->subtype != T_PIC_XBM)
	    XtSetSensitive(pen_col_button, False);

	(void) int_panel(new_l->depth, form, "     Depth", (Widget) 0, &depth_panel,
				MIN_DEPTH, MAX_DEPTH, 1);
	str_panel(new_l->pic->pic_cache? new_l->pic->pic_cache->file: "", "Picture filename", 
				&pic_name_panel, 290, False, False);

	/* make a button to reread the picture file */
	FirstArg(XtNfromVert, beside);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNlabel, "Reread");
	NextArg(XtNsensitive, 
		(new_l->pic->pic_cache && new_l->pic->pic_cache->file[0])); /* only sensitive if there is a filename */
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	reread = below = XtCreateManagedWidget("reread", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddCallback(below, XtNcallback, (XtCallbackProc) reread_picfile, (XtPointer) NULL);

	/* add browse button for image files */
	FirstArg(XtNlabel, "Browse");
	NextArg(XtNfromHoriz, below);
	NextArg(XtNfromVert, beside);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
        but1 = XtCreateManagedWidget("browse", commandWidgetClass, 
			form, Args, ArgCount);
        XtAddCallback(but1, XtNcallback,
		(XtCallbackProc) browse_button, (XtPointer) NULL);

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Type:");
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	type = beside = XtCreateManagedWidget("pic_type_label", labelWidgetClass,
				       form, Args, ArgCount);
	vdist = 4;
	for (i=0; i<NUM_PIC_TYPES; i++) {
	    /* start new row after 6 */
	    if (i == 6) {
		below = pic_type_box[0];
		beside = type;
		vdist = 10;
	    }
	    FirstArg(XtNfromHoriz, beside);
	    NextArg(XtNfromVert, below);
	    NextArg(XtNhorizDistance, 7);
	    NextArg(XtNvertDistance, vdist);
	    NextArg(XtNborderWidth, 1);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    /* make box red indicating this type of picture file */
	    if (new_l->pic != 0 && new_l->pic->pic_cache && new_l->pic->pic_cache->subtype == i+1) {
		NextArg(XtNbackground, colors[RED]);
		NextArg(XtNsensitive, True);
	    } else {
		NextArg(XtNbackground, colors[WHITE]);
		NextArg(XtNsensitive, False);
	    }
	    NextArg(XtNlabel, " ");
	    pic_type_box[i] = XtCreateManagedWidget("pic_type_box", 
					labelWidgetClass, form, Args, ArgCount);
	    FirstArg(XtNfromHoriz, pic_type_box[i]);
	    NextArg(XtNfromVert, below);
	    NextArg(XtNhorizDistance, 0);
	    NextArg(XtNvertDistance, vdist);
	    NextArg(XtNborderWidth, 0);
	    NextArg(XtNlabel, pic_names[i+1]);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    beside = XtCreateManagedWidget("pic_type_name", labelWidgetClass,
				       form, Args, ArgCount);
	}

	below = beside;
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Size:");
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("pic_size_label", labelWidgetClass,
				       form, Args, ArgCount);

	if (new_l->pic != 0 && new_l->pic->pic_cache && new_l->pic->pic_cache->subtype != T_PIC_NONE)
	    sprintf(buf,"%4d x %4d",
		new_l->pic->pic_cache->bit_size.x,new_l->pic->pic_cache->bit_size.y);
	else
	    strcpy(buf,"     --      ");

	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNresizable, False);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, buf);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	pic_size = XtCreateManagedWidget("pic_size", labelWidgetClass,
				       form, Args, ArgCount);

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, pic_size);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNlabel, "Colors:");
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("pic_colors_label", labelWidgetClass,
				       form, Args, ArgCount);
  
	/* although the EPS may have colors, the actual number is not known */
	if (new_l->pic != 0 && new_l->pic->pic_cache && (
	     new_l->pic->pic_cache->subtype == T_PIC_PNG ||
#ifdef USE_XPM
	     new_l->pic->pic_cache->subtype == T_PIC_XPM ||
#endif /* USE_XPM */
	     new_l->pic->pic_cache->subtype == T_PIC_PCX ||
	     new_l->pic->pic_cache->subtype == T_PIC_PPM ||
	     new_l->pic->pic_cache->subtype == T_PIC_TIF ||
#ifdef USE_JPEG
	     new_l->pic->pic_cache->subtype == T_PIC_JPEG ||
#endif /* USE_JPEG */
	     new_l->pic->pic_cache->subtype == T_PIC_GIF))
		sprintf(buf,"%3d",new_l->pic->pic_cache->numcols);
	else
		strcpy(buf,"N/A");

	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNresizable, False);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNlabel, buf);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	pic_colors = XtCreateManagedWidget("pic_colors", labelWidgetClass,
				       form, Args, ArgCount);

	/* transparent color entry for GIF files */
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, pic_colors);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNlabel, "Transp color:");
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("transp_label", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	if (new_l->pic->pic_cache && new_l->pic->pic_cache->subtype == T_PIC_GIF) {
	    if (new_l->pic->pic_cache->transp == TRANSP_NONE)
		sprintf(buf,"None");
	    else
		sprintf(buf,"%4d",(unsigned int) new_l->pic->pic_cache->transp);
	} else
	    sprintf(buf," N/A");
	NextArg(XtNlabel, buf);
	transp_color = XtCreateManagedWidget("transp_color", labelWidgetClass,
				       form, Args, ArgCount);
	below = pic_colors;

	p1 = *new_l->points;
	p2 = *new_l->points->next->next;

	xy_panel(p1.x, p1.y, "First corner", &x1_panel, &y1_panel, True);
	xy_panel(p2.x, p2.y, "Opposite corner", &x2_panel, &y2_panel, False);
	/* call the resize_picture proc when the user changes x or y */
	XtOverrideTranslations(x1_panel, XtParseTranslationTable(edit_picture_pos_translations));
	XtOverrideTranslations(y1_panel, XtParseTranslationTable(edit_picture_pos_translations));
	XtOverrideTranslations(x2_panel, XtParseTranslationTable(edit_picture_pos_translations));
	XtOverrideTranslations(y2_panel, XtParseTranslationTable(edit_picture_pos_translations));

	/* width and height */
	dimen.x = abs(p1.x - p2.x);
	dimen.y = abs(p1.y - p2.y);
	f_pair_panel(&dimen, "Dimensions", &width_panel, "Width =", 
					&height_panel, "Length =", False);
	/* override translations for Return to resize picture */
	XtOverrideTranslations(width_panel, XtParseTranslationTable(edit_picture_size_translations));
	XtOverrideTranslations(height_panel, XtParseTranslationTable(edit_picture_size_translations));

	/* make pulldown flipped menu */
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("     Orientation", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	flip_pic_flag = new_l->pic->flipped;
	flip_pic_panel = XtCreateManagedWidget(
	       flip_pic_items[flip_pic_flag ? 1 : 0], menuButtonWidgetClass,
					       form, Args, ArgCount);
	below = flip_pic_panel;
	make_pulldown_menu(flip_pic_items, XtNumber(flip_pic_items), -1, "",
			       flip_pic_panel, flip_pic_select);

	/* size is upper-lower */
	dx = p2.x - p1.x;
	dy = p2.y - p1.y;
	rotation = 0;
	if (dx < 0 && dy < 0)
	    rotation = 2; /* 180 */
	else if (dx < 0 && dy >= 0)
	    rotation = 3; /* 270 */
	else if (dx >= 0 && dy < 0)
	    rotation = 1; /* 90 */
	if (dx == 0 || dy == 0)
	    ratio = 0.0;
	else if (((rotation == 0 || rotation == 2) && !flip_pic_flag) ||
		 (rotation != 0 && rotation != 2 && flip_pic_flag))
	    ratio = (float) fabs((double) dy / (double) dx);
	else
	    ratio = (float) fabs((double) dx / (double) dy);

	/* make pulldown rotation menu */
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("        Rotation", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	rotation_panel = XtCreateManagedWidget(
	       rotation_items[rotation], menuButtonWidgetClass,
					       form, Args, ArgCount);
	below = rotation_panel;
	make_pulldown_menu(rotation_items, XtNumber(rotation_items), -1, "",
			       rotation_panel, rotation_select);


	float_label(ratio,  "  Curr h/w Ratio", &hw_ratio_panel);
	float_label(new_l->pic->hw_ratio, "  Orig h/w Ratio", &orig_hw_panel);
	below = orig_hw_panel;
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("Change h/w ratio", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNfromHoriz, beside);
	shrink = XtCreateManagedWidget("Shrink to orig", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddEventHandler(shrink, ButtonReleaseMask, False,
			  (XtEventHandler) shrink_pic, (XtPointer) NULL);
	beside = shrink;

	/* TRICK - make sure XtNfromHoriz is last resource above */
	ArgCount--;
	/*********************************************************/

	NextArg(XtNfromHoriz, beside);
	expand = XtCreateManagedWidget("Expand to orig", commandWidgetClass,
				       form, Args, ArgCount);
	XtAddEventHandler(expand, ButtonReleaseMask, False,
			  (XtEventHandler) expand_pic, (XtPointer) NULL);

	below = expand;
	FirstArg(XtNfromVert, below);
	NextArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	origsize = XtCreateManagedWidget("Use original size", commandWidgetClass, 
					form, Args, ArgCount);
	XtAddEventHandler(origsize, ButtonReleaseMask, False,
			  (XtEventHandler) origsize_pic, (XtPointer) NULL);

	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, origsize);
	NextArg(XtNhorizDistance, 15);
	NextArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	percent_button = XtCreateManagedWidget("Scale by %", commandWidgetClass,
					form, Args, ArgCount);
	XtAddEventHandler(percent_button, ButtonReleaseMask, False,
			  (XtEventHandler) scale_percent_pic, (XtPointer) NULL);
	
	percent = MakeFloatSpinnerEntry(form, &percent_entry, "scale_percent", 
				below, percent_button, 
				(XtCallbackProc) 0, "100.0", 0.00001, 1000.0, 1.0, 45);
	XtSetSensitive(percent, new_l->pic->hw_ratio ? True : False);

	break;
    }
}

static void
get_new_line_values(void)
{
    struct f_point  p1, p2, *p;
    char	   *string;
    char	    longname[PATH_MAX];
    int		    dx, dy, rotation;
    float	    ratio;
    int		    i;
    Boolean	    existing;

    switch (new_l->type) {
      case T_POLYLINE:
	get_generic_vals(new_l);
	get_cap_style(new_l);
	get_join_style(new_l);
	get_generic_arrows(new_l);
	get_points(new_l->points);
	return;
      case T_POLYGON:
	get_generic_vals(new_l);
	get_join_style(new_l);
	get_points(new_l->points);
	return;
      case T_ARCBOX:
	new_l->radius = atoi(panel_get_value(radius));
      case T_BOX:
	get_generic_vals(new_l);
	get_join_style(new_l);
	p1.x = panel_get_dim_value(x1_panel);
	p1.y = panel_get_dim_value(y1_panel);
	p2.x = panel_get_dim_value(x2_panel);
	p2.y = panel_get_dim_value(y2_panel);
	break;
      case T_PICTURE:
	check_depth();
	new_l->pen_color = pen_color;
	new_l->fill_color = fill_color;
	new_l->depth = atoi(panel_get_value(depth_panel));
	/* get any comments (this is done in get_generic_vals for other line types) */
	new_l->comments = my_strdup(panel_get_value(comments_panel));
	p1.x = panel_get_dim_value(x1_panel);
	p1.y = panel_get_dim_value(y1_panel);
	p2.x = panel_get_dim_value(x2_panel);
	p2.y = panel_get_dim_value(y2_panel);

	/* size is upper-lower */
	dx = p2.x - p1.x;
	dy = p2.y - p1.y;
	rotation = what_rotation(dx,dy);
	if (dx == 0 || dy == 0)
	    ratio = 0.0;
	else if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
		 (rotation != 0 && rotation != 180 && flip_pic_flag))
	    ratio = (float) fabs((double) dy / (double) dx);
	else
	    ratio = (float) fabs((double) dx / (double) dy);

	new_l->pic->flipped = flip_pic_flag;
	sprintf(buf, "%1.1f", ratio);
	FirstArg(XtNlabel, buf);
	SetValues(hw_ratio_panel);
	string = panel_get_value(pic_name_panel);
	if (string[0] == '\0')
	    string = EMPTY_PIC;
	else if (string[0] == '~') {
	    /* if user typed tilde, parse user path and put in string panel */
	    parseuserpath(string,longname);
	    panel_set_value(pic_name_panel, longname);
	    string = longname;
	} else if (string[0] != '/') {
	    /* make absolute */
	    sprintf(longname, "%s/%s", cur_file_dir, string);
	    panel_set_value(pic_name_panel, longname);
	    string = longname;
	}
	    
	file_changed = False;

	/* if the filename changed, or this is a new picture */
	if (!new_l->pic->pic_cache || strcmp(string, new_l->pic->pic_cache->file)) {
	    reread_file = False;
	    file_changed = True;
	    if (new_l->pic->pic_cache) {
		/* decrease ref count in picture repository for this name and free if 0 */
		free_picture_entry(new_l->pic->pic_cache);
	    }
	    new_l->pic->hw_ratio = 0.0;
	    /* read the new picture file */
	    if (strcmp(string, EMPTY_PIC)) {
		read_picobj(new_l->pic, string, new_l->pen_color, reread_file, &existing);
		/* calculate h/w ratio */
		new_l->pic->hw_ratio = (float) new_l->pic->pic_cache->bit_size.y/new_l->pic->pic_cache->bit_size.x;
	    }
	    /* enable reread button now */
	    XtSetSensitive(reread, True);
	}
	/* update bitmap size */
	if (new_l->pic->pic_cache && new_l->pic->pic_cache->subtype != T_PIC_NONE) {
	    sprintf(buf,"%d x %d",
			new_l->pic->pic_cache->bit_size.x,new_l->pic->pic_cache->bit_size.y);
	    /* only the XBM (bitmap) type has a pen color */
	    if (new_l->pic->pic_cache->subtype == T_PIC_XBM)
		XtSetSensitive(pen_col_button, True);
	    else
		XtSetSensitive(pen_col_button, False);
	} else {
	    strcpy(buf,"--");
	}
	FirstArg(XtNlabel, buf);
	SetValues(pic_size);
	/* stop here if no picture file specified yet */
	if (strcmp(string, EMPTY_PIC) == 0)
	    break;

	/* make box red indicating this type of picture file */
	for (i=0; i<NUM_PIC_TYPES; i++) {
	    if (new_l->pic->pic_cache->subtype == i+1) {
		FirstArg(XtNbackground, colors[RED]);
		NextArg(XtNsensitive, True);
	    } else {
		FirstArg(XtNbackground, colors[WHITE]);
		NextArg(XtNsensitive, False);
	    }
	    SetValues(pic_type_box[i]);
	}

	/* number of colors */
	/* although the FIG and EPS may have colors, the actual number is not known */
	FirstArg(XtNlabel, buf);
	if (
	     new_l->pic->pic_cache->subtype == T_PIC_PCX ||
	     new_l->pic->pic_cache->subtype == T_PIC_PPM ||
	     new_l->pic->pic_cache->subtype == T_PIC_TIF ||
	     new_l->pic->pic_cache->subtype == T_PIC_PNG ||
#ifdef USE_XPM
	     new_l->pic->pic_cache->subtype == T_PIC_XPM ||
#endif /* USE_XPM */
#ifdef USE_JPEG
	     new_l->pic->pic_cache->subtype == T_PIC_JPEG ||
#endif /* USE_JPEG */
	     new_l->pic->pic_cache->subtype == T_PIC_GIF)
		sprintf(buf,"%3d",new_l->pic->pic_cache->numcols);
	else
		strcpy(buf,"N/A");
	SetValues(pic_colors);

	/* now transparent color if GIF file */
	if (new_l->pic->pic_cache->subtype == T_PIC_GIF) {
	    if (new_l->pic->pic_cache->transp == TRANSP_NONE)
		sprintf(buf,"None");
	    else
		sprintf(buf,"%4d",(unsigned int) new_l->pic->pic_cache->transp);
	} else
	    sprintf(buf," N/A");
	SetValues(transp_color);

	/* h/w ratio */
	sprintf(buf, "%1.1f", new_l->pic->hw_ratio);
	FirstArg(XtNlabel, buf);
	SetValues(orig_hw_panel);
	app_flush();

	if (new_l->pic->pic_cache->subtype == T_PIC_XBM)
	     put_msg("Read XBM image of %dx%d pixels OK",
		new_l->pic->pic_cache->bit_size.x, new_l->pic->pic_cache->bit_size.y);
	/* recolor and redraw all pictures if this is a NEW picture */
	if (reread_file || !existing && file_changed && !appres.monochrome &&
	   (new_l->pic->pic_cache->numcols > 0) && (new_l->pic->pic_cache->bitmap != 0)) {
		reread_file = False;
		/* remap all picture colors */
		remap_imagecolors();
		/* make sure current colormap is installed */
		set_cmap(XtWindow(popup));
		/* and redraw all of the pictures already on the canvas */
		redraw_images(&objects);
		put_msg("Read %s image of %dx%d pixels and %d colors OK",
			  new_l->pic->pic_cache->subtype == T_PIC_EPS? "EPS":
			    new_l->pic->pic_cache->subtype == T_PIC_GIF? "GIF":
#ifdef USE_JPEG
			      new_l->pic->pic_cache->subtype == T_PIC_JPEG? "JPEG":
#endif /* USE_JPEG */
				new_l->pic->pic_cache->subtype == T_PIC_PCX? "PCX":
				  new_l->pic->pic_cache->subtype == T_PIC_PPM? "PPM":
				    new_l->pic->pic_cache->subtype == T_PIC_TIF? "TIFF":
				      new_l->pic->pic_cache->subtype == T_PIC_PNG? "PNG":
#ifdef USE_XPM
				        new_l->pic->pic_cache->subtype == T_PIC_XPM? "XPM":
#endif /* USE_XPM */
					"Unknown",
			new_l->pic->pic_cache->bit_size.x, new_l->pic->pic_cache->bit_size.y,
			new_l->pic->pic_cache->numcols);
		app_flush();
	}

	FirstArg(XtNsensitive, new_l->pic->hw_ratio ? True : False);
	SetValues(shrink);
	SetValues(expand);
	SetValues(origsize);
	SetValues(percent_button);
	/* must explicitely set a spinner with XtSetSensitive() */
	XtSetSensitive(percent, new_l->pic->hw_ratio ? True : False);
	break;
    } /* switch */

    p = new_l->points;
    p->x = p1.x;
    p->y = p1.y;
    p = p->next;
    p->x = p2.x;
    p->y = p1.y;
    p = p->next;
    p->x = p2.x;
    p->y = p2.y;
    p = p->next;
    p->x = p1.x;
    p->y = p2.y;
    p = p->next;
    p->x = p1.x;
    p->y = p1.y;
}

static void
done_line(void)
{
    int		prev_depth;
    switch (button_result) {
      case APPLY:
	changed = True;
	list_delete_line(&objects.lines, new_l);
	redisplay_line(new_l);
	get_new_line_values();
	list_add_line(&objects.lines, new_l);
	redisplay_line(new_l);
	toggle_linemarker(new_l);
	break;
      case DONE:
	/* save in case user changed depth */
	prev_depth = new_l->depth;
	get_new_line_values();
	if (new_l->pic)
	    new_l->pic->New = False;		/* user has modified it */
	if (prev_depth != new_l->depth) {
	    remove_depth(O_POLYLINE, prev_depth);
	    add_depth(O_POLYLINE, new_l->depth);
	}
	redisplay_lines(new_l, old_l);
	if (new_l->type == T_PICTURE)
	    old_l->type = T_PICTURE;		/* restore type */
	clean_up();
	old_l->next = new_l;
	set_latestline(old_l);
	set_action_object(F_EDIT, O_POLYLINE);
	set_modifiedflag();
	break;
      case CANCEL:
	list_delete_line(&objects.lines, new_l);
	/* if user created it but cancelled, delete it */
	if (new_l->pic && new_l->pic->New) {
	    redisplay_line(old_l);
	    free_line(&new_l);
	    return;
	}
	list_add_line(&objects.lines, old_l);
	if (saved_objects.lines && saved_objects.lines->next &&
	    saved_objects.lines->next == new_l)
		saved_objects.lines->next = old_l;
	else if (saved_objects.lines == new_l)
		saved_objects.lines = old_l;
	if (new_l->type == T_PICTURE) {
	    old_l->type = T_PICTURE;		/* restore type */
	    if (file_changed) {
		remap_imagecolors();		/* and restore colors */
		redraw_images(&objects);	/* and refresh them */
	    }
	}
	if (changed)
	    redisplay_lines(new_l, old_l);
	else
	    toggle_linemarker(old_l);
	free_line(&new_l);
	break;
    }

}

void make_window_text(F_text *t)
{
    static char	   *textjust_items[] = {
    "Left justified ", "Centered       ", "Right justified"};
    static char	   *hidden_text_items[] = {
    "Normal ", "Hidden "};
    static char	   *rigid_text_items[] = {
    "Normal ", "Rigid  "};
    static char	   *special_text_items[] = {
    "Normal ", "Special"};

    set_cursor(panel_cursor);
    toggle_textmarker(t);
    old_t = copy_text(t);
    new_t = t;

    textjust = new_t->type;	/* get current justification */
    hidden_text_flag = hidden_text(new_t) ? 1 : 0;
    new_psflag = psfont_text(new_t) ? 1 : 0;
    rigid_text_flag = rigid_text(new_t) ? 1 : 0;
    special_text_flag = special_text(new_t) ? 1 : 0;
    new_ps_font = cur_ps_font;
    new_latex_font = cur_latex_font;
    generic_vals.pen_color = new_t->color;

    pen_color = new_t->color;
    if (new_psflag)
	new_ps_font = new_t->font;	/* get current font */
    else
	new_latex_font = new_t->font;	/* get current font */
    generic_window("TEXT", "", &text_ic, done_text, NO_GENERICS, NO_ARROWS, t->comments);

    (void) int_panel(new_t->size, form, "      Size", (Widget) 0, &cur_fontsize_panel, 
				MIN_FONT_SIZE, MAX_FONT_SIZE, 1);
    below = pen_color_selection_panel();
    (void) int_panel(new_t->depth, form, "     Depth", (Widget) 0, &depth_panel, 
				MIN_DEPTH, MAX_DEPTH, 1);
    (void) float_panel(180.0 / M_PI * new_t->angle, form, "Angle (degrees)",
	      (Widget) 0, &angle_panel, -360.0, 360.0, 1.0, 2);

    /* make text justification menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("  Justification", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNlabel, textjust_items[textjust]);
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    textjust_panel = XtCreateManagedWidget(
			    "justify", menuButtonWidgetClass,
					   form, Args, ArgCount);
    below = textjust_panel;
    make_pulldown_menu(textjust_items, XtNumber(textjust_items), -1, "",
				    textjust_panel, textjust_select);

    /* make hidden text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("    Hidden Flag", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    hidden_text_panel = XtCreateManagedWidget(
		 hidden_text_items[hidden_text_flag], menuButtonWidgetClass,
					      form, Args, ArgCount);
    below = hidden_text_panel;
    make_pulldown_menu(hidden_text_items,
				       XtNumber(hidden_text_items), -1, "",
				     hidden_text_panel, hidden_text_select);

    /* make rigid text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("     Rigid Flag", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    rigid_text_panel = XtCreateManagedWidget(
		   rigid_text_items[rigid_text_flag], menuButtonWidgetClass,
					     form, Args, ArgCount);
    below = rigid_text_panel;
    make_pulldown_menu(rigid_text_items,
				      XtNumber(rigid_text_items), -1, "",
				      rigid_text_panel, rigid_text_select);

    /* make special text menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("   Special Flag", labelWidgetClass,
				   form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    special_text_panel = XtCreateManagedWidget(
				      special_text_items[special_text_flag],
			       menuButtonWidgetClass, form, Args, ArgCount);
    below = special_text_panel;
    make_pulldown_menu(special_text_items,
					XtNumber(special_text_items), -1, "",
				   special_text_panel, special_text_select);

    /* tell the pulldown unit menu which panels to convert (xy_panel adds them automatically) */
    init_convert_array();
    xy_panel(new_t->base_x, new_t->base_y, "Origin", &x1_panel, &y1_panel, True);

    /* make the popup font menu */
    font_image_panel(new_psflag ? psfont_menu_bitmaps[new_t->font + 1] :
		latexfont_menu_bitmaps[new_t->font], "Font", &font_panel);
#ifdef I18N
    str_panel(new_t->cstring, "Text", &text_panel, 220, True,
	      appres.latin_keyboard || is_i18n_font(new_t->fontstruct));
#else
    str_panel(new_t->cstring, "Text", &text_panel, 220, True, False);
#endif /* I18N */
}

static void
get_new_text_values(void)
{
    PR_SIZE	    size;

    check_depth();
    new_t->type = textjust;
    new_t->flags =
	(rigid_text_flag ? RIGID_TEXT : 0)
	| (special_text_flag ? SPECIAL_TEXT : 0)
	| (hidden_text_flag ? HIDDEN_TEXT : 0)
	| (new_psflag ? PSFONT_TEXT : 0);
    if (psfont_text(new_t))
	new_t->font = new_ps_font;
    else
	new_t->font = new_latex_font;
    new_t->size = atoi(panel_get_value(cur_fontsize_panel));
    if (new_t->size < 1) {
	new_t->size = 1;
	panel_set_int(cur_fontsize_panel, 1);
    }
    new_t->color = pen_color;
    new_t->depth = atoi(panel_get_value(depth_panel));
    new_t->angle = M_PI / 180.0 * atof(panel_get_value(angle_panel));
    fix_angle(&new_t->angle);	/* keep between 0 and 2PI */
    new_t->base_x = panel_get_dim_value(x1_panel);
    new_t->base_y = panel_get_dim_value(y1_panel);
    if (new_t->cstring)
	free(new_t->cstring);
    /* get the text string itself */
    new_t->cstring = my_strdup(panel_get_value(text_panel));
    /* get any comments */
    new_t->comments = my_strdup(panel_get_value(comments_panel));
    /* get the fontstruct for zoom = 1 to get the size of the string */
    canvas_font = lookfont(x_fontnum(psfont_text(new_t), new_t->font), new_t->size);
    size = textsize(canvas_font, strlen(new_t->cstring), new_t->cstring);
    new_t->ascent = size.ascent;
    new_t->descent = size.descent;
    new_t->length = size.length;
    /* now set the fontstruct for this zoom scale */
    reload_text_fstruct(new_t);
}

static void
done_text(void)
{
    int		prev_depth;
    switch (button_result) {
      case APPLY:
	changed = True;
	list_delete_text(&objects.texts, new_t);
	redisplay_text(new_t);
	get_new_text_values();
	list_add_text(&objects.texts, new_t);
	redisplay_text(new_t);
	toggle_textmarker(new_t);
	break;
      case DONE:
	/* save in case user changed depth */
	prev_depth = new_t->depth;
	get_new_text_values();
	if (prev_depth != new_t->depth) {
	    remove_depth(O_TEXT, prev_depth);
	    add_depth(O_TEXT, new_t->depth);
	}
	redisplay_texts(new_t, old_t);
	clean_up();
	old_t->next = new_t;
	set_latesttext(old_t);
	set_action_object(F_EDIT, O_TEXT);
	set_modifiedflag();
	break;
      case CANCEL:
	list_delete_text(&objects.texts, new_t);
	list_add_text(&objects.texts, old_t);
	if (saved_objects.texts && saved_objects.texts->next &&
	    saved_objects.texts->next == new_t)
		saved_objects.texts->next = old_t;
	else if (saved_objects.texts == new_t)
		saved_objects.texts = old_t;
	if (changed)
	    redisplay_texts(new_t, old_t);
	else
	    toggle_textmarker(old_t);
	free_text(&new_t);
	break;
    }
}

void make_window_ellipse(F_ellipse *e)
{
    char	   *s1, *s2;
    icon_struct	   *image;

    set_cursor(panel_cursor);
    toggle_ellipsemarker(e);
    old_e = copy_ellipse(e);
    new_e = e;

    pen_color = new_e->pen_color;
    fill_color = new_e->fill_color;
    switch (new_e->type) {
      case T_ELLIPSE_BY_RAD:
	s1 = "ELLIPSE";
	s2 = "specified by radiii";
	ellipse_flag = 1;
	image = &ellrad_ic;
	break;
      case T_ELLIPSE_BY_DIA:
	s1 = "ELLIPSE";
	s2 = "specified by diameters";
	ellipse_flag = 1;
	image = &elldia_ic;
	break;
      case T_CIRCLE_BY_RAD:
	s1 = "CIRCLE";
	s2 = "specified by radius";
	ellipse_flag = 0;
	image = &ellrad_ic;
	break;
      case T_CIRCLE_BY_DIA:
	s1 = "CIRCLE";
	s2 = "specified by diameter";
	ellipse_flag = 0;
	image = &elldia_ic;
	break;
    }
    put_generic_vals(new_e);
    generic_window(s1, s2, image, done_ellipse, GENERICS, NO_ARROWS, e->comments);
    if (new_e->type == T_ELLIPSE_BY_RAD || new_e->type == T_ELLIPSE_BY_DIA) {
	(void) int_panel(round(180 / M_PI * new_e->angle), form, "Angle (degrees)",
	      (Widget) 0, &angle_panel, -360, 360, 1);
    }

    /* tell the pulldown unit menu which panels to convert */
    init_convert_array();
    if (ellipse_flag) {
	f_pair_panel(&new_e->center, "Center", 
					&x1_panel, "X =", &y1_panel, "Y =", True);
	f_pair_panel(&new_e->radiuses, "Radii", 
					&x2_panel, "X =", &y2_panel, "Y =", False);
    } else {
	f_pair_panel(&new_e->center, "Center", 
					&x1_panel, "X =", &y1_panel, "Y =", True);
	FirstArg(XtNlabel, "Radius");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("radius_label", labelWidgetClass, form, Args, ArgCount);

	cvt_to_units_str(new_e->radiuses.x, buf);
	FirstArg(XtNstring, buf);
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinsertPosition, strlen(buf));
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNwidth, XY_WIDTH);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	x2_panel = XtCreateManagedWidget("radius", asciiTextWidgetClass, form, Args, ArgCount);
	text_transl(x2_panel);
	add_to_convert(x2_panel);
    }
}

static void
get_new_ellipse_values(void)
{
    get_generic_vals(new_e);
    if (new_e->type == T_ELLIPSE_BY_RAD || new_e->type == T_ELLIPSE_BY_DIA) {
	new_e->angle = M_PI / 180 * atoi(panel_get_value(angle_panel));
	fix_angle(&new_e->angle);	/* keep between 0 and 2PI */
    }
    get_f_pos(&new_e->center, x1_panel, y1_panel);
    if (ellipse_flag)
	get_f_pos(&new_e->radiuses, x2_panel, y2_panel);
    else
	new_e->radiuses.x = new_e->radiuses.y =
	    panel_get_dim_value(x2_panel);

    if (new_e->type == T_ELLIPSE_BY_RAD || new_e->type == T_CIRCLE_BY_RAD) {
	new_e->start = new_e->center;
    } else {
	new_e->start.x = new_e->center.x - new_e->radiuses.x;
	new_e->start.y = new_e->center.y;
    }
    new_e->end.x = new_e->center.x + new_e->radiuses.x;
    new_e->end.y = new_e->center.y;
}

static void
done_ellipse(void)
{
    int		prev_depth;
    switch (button_result) {
      case APPLY:
	changed = True;
	list_delete_ellipse(&objects.ellipses, new_e);
	redisplay_ellipse(new_e);
	get_new_ellipse_values();
	list_add_ellipse(&objects.ellipses, new_e);
	redisplay_ellipse(new_e);
	toggle_ellipsemarker(new_e);
	break;
      case DONE:
	/* save in case user changed depth */
	prev_depth = new_e->depth;
	get_new_ellipse_values();
	if (prev_depth != new_e->depth) {
	    remove_depth(O_ELLIPSE, prev_depth);
	    add_depth(O_ELLIPSE, new_e->depth);
	}
	redisplay_ellipses(new_e, old_e);
	clean_up();
	old_e->next = new_e;
	set_latestellipse(old_e);
	set_action_object(F_EDIT, O_ELLIPSE);
	set_modifiedflag();
	break;
      case CANCEL:
	list_delete_ellipse(&objects.ellipses, new_e);
	list_add_ellipse(&objects.ellipses, old_e);
	if (saved_objects.ellipses && saved_objects.ellipses->next &&
	    saved_objects.ellipses->next == new_e)
		saved_objects.ellipses->next = old_e;
	else if (saved_objects.ellipses == new_e)
		saved_objects.ellipses = old_e;
	if (changed)
	    redisplay_ellipses(new_e, old_e);
	else
	    toggle_ellipsemarker(old_e);
	free_ellipse(&new_e);
	break;
    }

}

Pixel arrow_panel_bg;

void make_window_arc(F_arc *a)
{
    Widget	    save_form, save_below;

    set_cursor(panel_cursor);
    toggle_arcmarker(a);
    old_a = copy_arc(a);
    new_a = a;

    pen_color = new_a->pen_color;
    fill_color = new_a->fill_color;
    put_generic_vals(new_a);
    put_generic_arrows((F_line *) new_a);
    put_arc_type(new_a);
    put_cap_style(new_a);

    /* tell the pulldown unit menu which panels to convert */
    init_convert_array();

    /* create main window */
    generic_window("ARC", "Specified by 3 points", &arc_ic, done_arc, 
			GENERICS, ARROWS, a->comments);
    save_form = form;
    /* put the points panel in a sub-form */
    FirstArg(XtNborderWidth, 0);
    if (new_a->type == T_PIE_WEDGE_ARC) {
	NextArg(XtNfromVert, above_arrows);	/* put it just below the stuff before arrows */
    } else {
	NextArg(XtNfromVert, below);		/* after the arrow forms */
    }
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    form = arc_points_form = XtCreateManagedWidget("form", formWidgetClass, 
				save_form, Args, ArgCount);
    save_below = below;
    below = (Widget) NULL;
    f_pair_panel(&new_a->point[0], "First point", 
				&x1_panel, "X =", &y1_panel, "Y =", True);
    f_pair_panel(&new_a->point[1], "Second point", 
				&x2_panel, "X =", &y2_panel, "Y =", False);
    f_pair_panel(&new_a->point[2], "Third point", 
				&x3_panel, "X =", &y3_panel, "Y =", False);
    
    form = save_form;
    below = save_below;
    /* if this is a pie-wedge arc, unmanage the arrow stuff until user changes
       it to an open type */
    if (new_a->type == T_PIE_WEDGE_ARC) {
	XtRealizeWidget(popup);
	XtUnmanageChild(form);
	XtUnmanageChild(for_aform);
	XtUnmanageChild(back_aform);
	XtManageChild(form);
    }
}

static void
get_new_arc_values(void)
{
    F_pos	    p0, p1, p2;
    float	    cx, cy;

    get_generic_vals(new_a);
    get_generic_arrows((F_line *) new_a);
    get_cap_style(new_a);
    get_arc_type(new_a);
    get_f_pos(&p0, x1_panel, y1_panel);
    get_f_pos(&p1, x2_panel, y2_panel);
    get_f_pos(&p2, x3_panel, y3_panel);
    if (compute_arccenter(p0, p1, p2, &cx, &cy)) {
	new_a->point[0] = p0;
	new_a->point[1] = p1;
	new_a->point[2] = p2;
	new_a->center.x = cx;
	new_a->center.y = cy;
	new_a->direction = compute_direction(p0, p1, p2);
    } else
	put_msg("Invalid ARC points!");
}

static void
done_arc(void)
{
    int		prev_depth;
    switch (button_result) {
      case APPLY:
	changed = True;
	list_delete_arc(&objects.arcs, new_a);
	redisplay_arc(new_a);
	get_new_arc_values();
	list_add_arc(&objects.arcs, new_a);
	redisplay_arc(new_a);
	toggle_arcmarker(new_a);
	break;
      case DONE:
	/* save in case user changed depth */
	prev_depth = new_a->depth;
	get_new_arc_values();
	if (prev_depth != new_a->depth) {
	    remove_depth(O_ARC, prev_depth);
	    add_depth(O_ARC, new_a->depth);
	}
	redisplay_arcs(new_a, old_a);
	clean_up();
	old_a->next = new_a;
	set_latestarc(old_a);
	set_action_object(F_EDIT, O_ARC);
	set_modifiedflag();
	break;
      case CANCEL:
	list_delete_arc(&objects.arcs, new_a);
	list_add_arc(&objects.arcs, old_a);
	if (saved_objects.arcs && saved_objects.arcs->next &&
	    saved_objects.arcs->next == new_a)
		saved_objects.arcs->next = old_a;
	else if (saved_objects.arcs == new_a)
		saved_objects.arcs = old_a;
	if (changed)
	    redisplay_arcs(new_a, old_a);
	else
	    toggle_arcmarker(old_a);
	free_arc(&new_a);
	break;
    }

}

void make_window_spline(F_spline *s)
{
    set_cursor(panel_cursor);
    toggle_splinemarker(s);
    old_s = copy_spline(s);
    new_s = s;

    pen_color = new_s->pen_color;
    fill_color = new_s->fill_color;
    put_generic_vals(new_s);
    put_generic_arrows((F_line *) new_s);
    put_cap_style(new_s);
    switch (new_s->type) {
      case T_OPEN_APPROX:
	generic_window("SPLINE", "Open approximated spline", &spl_ic,
		       done_spline, GENERICS, ARROWS, s->comments);
	points_panel(new_s->points);
	break;
      case T_CLOSED_APPROX:
	generic_window("SPLINE", "Closed approximated spline", &c_spl_ic,
		       done_spline, GENERICS, NO_ARROWS, s->comments); /* no arrowheads */
	points_panel(new_s->points);
	break;
      case T_OPEN_INTERP:
	generic_window("SPLINE", "Open interpolated spline", &intspl_ic,
		       done_spline, GENERICS, ARROWS, s->comments);
	points_panel(new_s->points);
	break;
      case T_CLOSED_INTERP:
	generic_window("SPLINE", "Closed interpolated spline", &c_intspl_ic,
		       done_spline, GENERICS, NO_ARROWS, s->comments); /* no arrowheads */
	points_panel(new_s->points);
	break;
      case T_OPEN_XSPLINE:
	generic_window("SPLINE", "X-Spline open", &xspl_ic,
		       done_spline, GENERICS, ARROWS, s->comments);
	points_panel(new_s->points);
	break;
      case T_CLOSED_XSPLINE:
	generic_window("SPLINE", "X-Spline closed", &c_xspl_ic,
		       done_spline, GENERICS, ARROWS, s->comments);
	points_panel(new_s->points);
	break;
    }
}


static void
done_spline(void)
{
    int		prev_depth;
    switch (button_result) {
      case APPLY:
	changed = True;
	list_delete_spline(&objects.splines, new_s);
	redisplay_spline(new_s);
	get_generic_vals(new_s);
	get_generic_arrows((F_line *) new_s);
	get_cap_style(new_s);
	get_points(new_s->points);
	list_add_spline(&objects.splines, new_s);
	redisplay_spline(new_s);
	toggle_splinemarker(new_s);
	break;
      case DONE:
	/* save in case user changed depth */
	prev_depth = new_s->depth;
	get_generic_vals(new_s);
	get_generic_arrows((F_line *) new_s);
	get_cap_style(new_s);
	get_points(new_s->points);
	if (prev_depth != new_s->depth) {
	    remove_depth(O_SPLINE, prev_depth);
	    add_depth(O_SPLINE, new_s->depth);
	}
	redisplay_splines(new_s, old_s);
	clean_up();
	old_s->next = new_s;
	set_latestspline(old_s);
	set_action_object(F_EDIT, O_SPLINE);
	set_modifiedflag();
	break;
      case CANCEL:
	list_delete_spline(&objects.splines, new_s);
	list_add_spline(&objects.splines, old_s);
	if (saved_objects.splines && saved_objects.splines->next &&
	    saved_objects.splines->next == new_s)
		saved_objects.splines->next = old_s;
	else if (saved_objects.splines == new_s)
		saved_objects.splines = old_s;
	if (changed)
	    redisplay_splines(new_s, old_s);
	else
	    toggle_splinemarker(old_s);
	free_spline(&new_s);
	break;
    }
}


static void
make_window_spline_point(F_spline *s, int x, int y)
{
    set_cursor(panel_cursor);
    new_s = copy_spline(s);
    if (new_s == NULL) 
      return;
    new_s->next = s;

    edited_point   = search_spline_point(new_s, x, y);

    sub_new_s = create_subspline(&num_spline_points, new_s, edited_point,
				 &edited_sfactor, &sub_sfactor);
    if (sub_new_s == NULL)
      return;

    /* make solid, black unfilled spline of thickness 1 */
    s->thickness		= 1;
    s->pen_color		= BLACK;
    s->fill_style		= UNFILLED;
    s->style			= SOLID_LINE;
    sub_new_s->thickness	= 1;
    sub_new_s->pen_color	= BLACK;
    sub_new_s->fill_style	= UNFILLED;
    sub_new_s->style		= SOLID_LINE;
    redisplay_spline(s);

    spline_point_window(x, y);
}

static void
done_spline_point(void)
{
    old_s = new_s->next;
    /* restore original attributes */
    old_s->thickness = new_s->thickness;
    old_s->pen_color = new_s->pen_color;
    old_s->fill_style = new_s->fill_style;
    old_s->style = new_s->style;
    edited_sfactor->s = sub_sfactor->s;
    free_subspline(num_spline_points, &sub_new_s);

    switch (button_result) {
      case DONE:	
	new_s->next = NULL;
	change_spline(old_s, new_s);
	redisplay_spline(new_s);
	break;
      case CANCEL:
	if (changed) 
	    draw_spline(new_s, ERASE);
	redisplay_spline(old_s);
	new_s->next = NULL;
	free_spline(&new_s);
	break;
    }
    return;
}


static void
new_generic_values(void)
{
    int		    fill;
    char	   *val;

    generic_vals.thickness = atoi(panel_get_value(thickness_panel));
    generic_vals.pen_color = pen_color;
    generic_vals.fill_color = fill_color;
    check_thick();
    check_depth();
    generic_vals.depth = atoi(panel_get_value(depth_panel));
    /* get the comments */
    generic_vals.comments = my_strdup(panel_get_value(comments_panel));
    /* include dash length in panel, too */
    generic_vals.style_val = (float) atof(panel_get_value(style_val_panel));
    if (generic_vals.style == DASH_LINE || generic_vals.style == DOTTED_LINE ||
        generic_vals.style == DASH_DOT_LINE ||
        generic_vals.style == DASH_2_DOTS_LINE ||
        generic_vals.style == DASH_3_DOTS_LINE )
        if (generic_vals.style_val <= 0.0) {
	    generic_vals.style_val = ((generic_vals.style == DASH_LINE ||
                generic_vals.style == DASH_DOT_LINE ||
                generic_vals.style == DASH_2_DOTS_LINE ||
                generic_vals.style == DASH_3_DOTS_LINE)?
					     cur_dashlength: cur_dotgap)*
					(generic_vals.thickness + 1) / 2;
	    panel_set_float(style_val_panel, generic_vals.style_val, "%1.1f");
	}
	
    if (fill_flag==1) {		/* fill color */
	val = panel_get_value(fill_intens_panel);
	if (*val >= ' ' && *val <= '9') {
	    /* if fill value > 200%, set to 100%.  Also if > 100% and fill color is
	       black or white set to 100% */
	    if ((fill = atoi(val)) > 200 ||
		(fill_color == BLACK || fill_color == DEFAULT || fill_color == WHITE) &&
			fill > 100)
		fill = 100;
	    generic_vals.fill_style = fill / (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	} else {
	    generic_vals.fill_style = 0;
	}
	fill = generic_vals.fill_style * (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	panel_set_int(fill_intens_panel, fill);
    } else if (fill_flag==2) {	/* fill pattern */
	val = panel_get_value(fill_pat_panel);
	if (*val >= ' ' && *val <= '9') {
	    if ((fill = atoi(val)) >= NUMPATTERNS)
		fill = NUMPATTERNS-1;
	    if (fill < 0)
		fill = 0;
	    generic_vals.fill_style = fill+NUMSHADEPATS+NUMTINTPATS;
	} else {
	    fill = 0;
	    generic_vals.fill_style = NUMSHADEPATS+NUMTINTPATS;
	}
	panel_set_int(fill_pat_panel, fill);
    } else {
	generic_vals.fill_style = -1;		/* no fill */
    }
}

static void
new_arrow_values(void)
{
	if (for_arrow) {
	    generic_vals.for_arrow.thickness =
				(float) fabs(atof(panel_get_value(for_arrow_thick)));
	    generic_vals.for_arrow.wd =
				(float) fabs(atof(panel_get_value(for_arrow_width)));
	    generic_vals.for_arrow.ht =
				(float) fabs(atof(panel_get_value(for_arrow_height)));
	}
	if (back_arrow) {
	    generic_vals.back_arrow.thickness =
				(float) fabs(atof(panel_get_value(back_arrow_thick)));
	    generic_vals.back_arrow.wd =
				(float) fabs(atof(panel_get_value(back_arrow_width)));
	    generic_vals.back_arrow.ht =
				(float) fabs(atof(panel_get_value(back_arrow_height)));
	}
}

static void
done_button(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    button_result = DONE;
    done_proc();
    reset_edit_cursor();
    Quit();
}

static void
apply_button(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    button_result = DONE;
    button_result = APPLY;
    /* make sure current colormap is installed */
    set_cmap(XtWindow(popup));
    done_proc();
}

static void
cancel_button(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    button_result = CANCEL;
    done_proc();
    reset_edit_cursor();
    Quit();
}

static void
edit_done(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    done_button(w, NULL, NULL);
}

static void
edit_apply(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    apply_button(w, NULL, NULL);
}

static void
edit_cancel(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
    cancel_button(w, NULL, NULL);
}

void reset_edit_cursor(void)
{
    if (lib_cursor)
	set_cursor(null_cursor);
    else if (dim_cursor)
	set_cursor(arrow_cursor);
    else
	set_cursor(pick9_cursor);
    lib_cursor = False;
    dim_cursor = False;
}

static void
generic_window(char *object_type, char *sub_type, icon_struct *icon, void (*d_proc) (/* ??? */), Boolean generics, Boolean arrows, char *comments)
{
    Dimension	    label_height, image_height;
    int		    button_distance;
    int		    i, fill;
    Widget	    image, cancel;
    Pixmap	    image_pm;
    XFontStruct	   *temp_font;

    FirstArg(XtNtitle, "Xfig: Edit panel");
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNallowShellResize, True);
    popup = XtCreatePopupShell("edit_panel",
			       transientShellWidgetClass, tool,
			       Args, ArgCount);
    XtAugmentTranslations(popup,
			XtParseTranslationTable(edit_popup_translations));
    init_kbd_actions();
    if (!actions_added) {
        XtAppAddActions(tool_app, edit_actions, XtNumber(edit_actions));
	XtAppAddActions(tool_app, picture_pos_actions, XtNumber(picture_pos_actions));
	XtAppAddActions(tool_app, picture_size_actions, XtNumber(picture_size_actions));
	XtAppAddActions(tool_app, text_actions, XtNumber(text_actions));
	XtAppAddActions(tool_app, compound_actions, XtNumber(compound_actions));
	actions_added = True;
    }

    form = XtCreateManagedWidget("form", formWidgetClass, popup, NULL, 0);

    done_proc = d_proc;

    /***************** LABEL and IMAGE of the object *****************/

    FirstArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    image = XtCreateManagedWidget("image", labelWidgetClass, form,
				  Args, ArgCount);

    /* put in the image */
    /* search to see if that pixmap has already been created */
    image_pm = 0;
    for (i = 0; i < NUM_IMAGES; i++) {
	if (pix_table[i].image == 0)
	    break;
	if (pix_table[i].image == icon) {
	    image_pm = pix_table[i].image_pm;
	    break;
	}
    }

    /* no need for colon (:) if no sub-type */
    if (strlen(sub_type) == 0)
	sprintf(buf, "%s", object_type);
    else
	sprintf(buf, "%s: %s", object_type, sub_type);
    FirstArg(XtNfromHoriz, image);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    label = XtCreateManagedWidget(buf, labelWidgetClass, form, Args, ArgCount);

    /* doesn't already exist, create a pixmap from the data (ala panel.c) */
    /* OpenWindows bug doesn't handle a 1-plane bitmap on a n-plane display */
    /* so we use CreatePixmap.... */
    if (image_pm == 0) {
	Pixel	    fg, bg;
	/* get the foreground/background of the widget */
	FirstArg(XtNforeground, &fg);
	NextArg(XtNbackground, &bg);
	GetValues(image);

	image_pm = XCreatePixmapFromBitmapData(tool_d, canvas_win,
				     icon->bits, icon->width, icon->height,
				     fg, bg, tool_dpth);
	pix_table[i].image_pm = image_pm;
	pix_table[i].image = icon;
    }
    FirstArg(XtNbitmap, image_pm);
    SetValues(image);

    /* get height of label widget and distance between widgets */
    FirstArg(XtNheight, &label_height);
    NextArg(XtNvertDistance, &button_distance);
    GetValues(label);
    /* do the same for the image widget */
    FirstArg(XtNheight, &image_height);
    GetValues(image);

    /***************** BUTTONS *****************/

    FirstArg(XtNlabel, " Done ");
    NextArg(XtNfromHoriz, image);
    NextArg(XtNfromVert, label);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    but1 = XtCreateManagedWidget("done", commandWidgetClass, form, Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc) done_button, (XtPointer) NULL);

    /* editing whole figure comments doesn't need an apply button, just done and cancel */

    if (strcmp("Whole Figure", object_type)) {
	FirstArg(XtNlabel, "Apply ");
	NextArg(XtNfromHoriz, but1);
	NextArg(XtNfromVert, label);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	but1 = XtCreateManagedWidget("apply", commandWidgetClass, form, Args, ArgCount);
	XtAddCallback(but1, XtNcallback, (XtCallbackProc) apply_button, (XtPointer) NULL);
    }

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromHoriz, but1);
    NextArg(XtNfromVert, label);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    cancel = but1 = XtCreateManagedWidget("cancel", commandWidgetClass, form, Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc) cancel_button, (XtPointer) NULL);
    below = but1;

    /* add "Screen Capture" and "Edit Image" buttons if picture object */
    if (!strcmp(sub_type,"Picture Object")) {
	FirstArg(XtNlabel,"Screen Capture");
	NextArg(XtNfromHoriz, but1);
	NextArg(XtNfromVert, label);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	but1 = XtCreateManagedWidget("screen_capture",
	    commandWidgetClass, form, Args, ArgCount);
	XtAddCallback(but1, XtNcallback,
	    (XtCallbackProc) grab_button, (XtPointer) NULL);

	if ( cur_image_editor != NULL && *cur_image_editor != (char) NULL) {
	    FirstArg(XtNlabel,"Edit Image");
	    NextArg(XtNfromHoriz, but1);
	    NextArg(XtNfromVert, label);
	    NextArg(XtNtop, XtChainTop);
	    NextArg(XtNbottom, XtChainTop);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    but1 = XtCreateManagedWidget("edit_image",
		commandWidgetClass, form, Args, ArgCount);
	    XtAddCallback(but1, XtNcallback,
		(XtCallbackProc) image_edit_button, (XtPointer) NULL);
	}
    }

    /***************** COMMENTS *****************/

    /* label for Comments */
    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = XtCreateManagedWidget("Comments", labelWidgetClass,
				       form, Args, ArgCount);
    /* get the font of above label widget */
    FirstArg(XtNfont, &temp_font);
    GetValues(below);

    /* make text widget for any comment lines */
    FirstArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 2);
    NextArg(XtNstring, comments);
    NextArg(XtNinsertPosition, 0);
    NextArg(XtNeditType, XawtextEdit);
    if (!strcmp(sub_type,"Picture Object")) {
	NextArg(XtNwidth, 415);		/* as wide as picture filename widget below */
    } else {
	NextArg(XtNwidth, 300);
    }
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    /* allow enough height for 3 lines + scrollbar */
    NextArg(XtNheight, max_char_height(temp_font) * 3 + 20);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNscrollVertical, XawtextScrollWhenNeeded);

    comments_panel = below = XtCreateManagedWidget("comments", asciiTextWidgetClass, 
				form, Args, ArgCount);
    XtOverrideTranslations(comments_panel,
		    XtParseTranslationTable(edit_comment_translations));

    /***************** COMMON PARAMETERS *****************/

    if (generics) {
	Widget lbelow, lbeside;

	/***************** ARC TYPE MENU *****************/

	if (!strcmp(object_type,"ARC"))
		arc_type_menu();

	/***************** LINE WIDTH *****************/

	lbelow = below;
	(void) int_panel(generic_vals.thickness, form, "Width", 
				(Widget) 0, &thickness_panel, 0, 1000, 1);

	lbeside = below;
	below = lbelow;
	/***************** OBJECT DEPTH *****************/
	(void) int_panel(generic_vals.depth,     form, "Depth", 
				lbeside, &depth_panel, MIN_DEPTH, MAX_DEPTH, 1);

	/***************** COLOR MENUS *****************/

	below = pen_color_selection_panel();
	below = fill_color_selection_panel();

	if (generic_vals.fill_style == -1) {
	    fill = -1;
	    fill_flag = 0;
	} else if (generic_vals.fill_style < NUMSHADEPATS+NUMTINTPATS) {
	    fill = generic_vals.fill_style * (200 / (NUMSHADEPATS+NUMTINTPATS - 1));
	    fill_flag = 1;
	} else {	/* fill pattern */
	    fill = generic_vals.fill_style - NUMSHADEPATS - NUMTINTPATS;
	    fill_flag = 2;
	}

	/***************** FILL STYLE MENU *****************/

	fill_style_menu(fill, fill_flag);
	below = fill_image;	/* line style goes below fill pattern image if
				   there is no cap style and no join style */

	/***************** LINE STYLE MENU *****************/

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("Line style", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNbitmap, linestyle_pixmaps[generic_vals.style]);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	line_style_panel = XtCreateManagedWidget("line_style", menuButtonWidgetClass,
					    form, Args, ArgCount);
	make_pulldown_menu_images(linestyle_choices, NUM_LINESTYLE_TYPES,
			linestyle_pixmaps, NULL, line_style_panel, line_style_select);

	/* new field for style_val */
	style_val = float_panel(generic_vals.style_val, form, "Dash length/\nDot gap",
		    line_style_panel, &style_val_panel, 0.0, 1000.0, 1.0, 1);
	/* save pointer to dash/dot gap label panel */
	style_val_label = beside;
	FirstArg(XtNhorizDistance, 20);
	SetValues(style_val_label);
	if (generic_vals.style == SOLID_LINE) {
	    XtSetSensitive(style_val,False);
	    XtSetSensitive(style_val_label,False);
	    /* and clear any value from the dash length panel */
	    panel_clear_value(style_val_panel);
	}
	below = line_style_panel;

	/***************** CAP STYLE MENU *****************/

	/* if a polyline, arc or open spline make a panel for cap style */

	lbelow = below;
	beside = (Widget) 0;
	if (!strcmp(object_type,"ARC") || 
	    (!strcmp(object_type,"SPLINE") && strstr(sub_type,"Open")) ||
	    (!strcmp(object_type,"POLYLINE") && !strcmp(sub_type,"Polyline"))) {
		cap_style_panel_menu();
		below = beside = cap_style_panel;
	}

	/***************** JOIN STYLE MENU *****************/

	if (!strcmp(sub_type,"Polyline") || !strcmp(sub_type,"Polygon") ||
	    !strcmp(sub_type,"Box")) {
		below = lbelow;
		join_style_panel_menu();
		below = join_style_panel;
	}

	/* save widget that is just above the arrow panels for the arc window usage */
	above_arrows = below;

	/***************** ARROW panels *****************/

	if (arrows) {
	    int		type;

	    /****** FIRST THE FORWARD ARROW ******/

	    FirstArg(XtNfromVert, above_arrows);
	    NextArg(XtNvertDistance, 2);
	    NextArg(XtNhorizDistance, 20);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    for_aform = XtCreateManagedWidget("arrow_form", formWidgetClass,
					  form, Args, ArgCount);

	    FirstArg(XtNborderWidth, 0);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    beside = XtCreateManagedWidget("Forward\nArrow", labelWidgetClass,
				       for_aform, Args, ArgCount);
	    /* make pulldown arrow style menu */
	    if (generic_vals.for_arrow.type == -1) {
		type = -1;
	    } else {
		type = generic_vals.for_arrow.type*2 - 1 + generic_vals.for_arrow.style;
		if (type < 0)
		    type = 0;
	    }

	    FirstArg(XtNfromHoriz, beside);
	    NextArg(XtNlabel, "");
	    NextArg(XtNinternalWidth, 0);
	    NextArg(XtNinternalHeight, 0);
	    NextArg(XtNbitmap, arrow_pixmaps[type+1]);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    for_arrow_type_panel = XtCreateManagedWidget("Arrowtype",
					    menuButtonWidgetClass,
					    for_aform, Args, ArgCount);
	    below = for_arrow_type_panel;

	    /* make pulldown menu with arrow type images */
	    make_pulldown_menu_images(dim_arrowtype_choices,
			NUM_ARROW_TYPES, arrow_pixmaps, NULL,
			for_arrow_type_panel, for_arrow_type_select);

	    for_thick = float_panel(generic_vals.for_arrow.thickness, for_aform,
			"Thick ", (Widget) 0, &for_arrow_thick, 0.0, 10000.0, 1.0, 1);
	    for_thick_label = beside;
	    for_width = float_panel(generic_vals.for_arrow.wd, for_aform,
			"Width ", (Widget) 0, &for_arrow_width, 0.0, 10000.0, 1.0, 1);
	    for_width_label = beside;
	    for_height = float_panel(generic_vals.for_arrow.ht, for_aform,
			"Length", (Widget) 0, &for_arrow_height, 0.0, 10000.0, 1.0, 1);
	    for_height_label = beside;

	    /****** NOW THE BACK ARROW ******/

	    FirstArg(XtNfromVert, above_arrows);
	    NextArg(XtNvertDistance, 2);
	    NextArg(XtNfromHoriz, for_aform);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    back_aform = XtCreateManagedWidget("arrow_form", formWidgetClass,
					  form, Args, ArgCount);
	    FirstArg(XtNborderWidth, 0);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    beside = XtCreateManagedWidget("Backward\nArrow", labelWidgetClass,
				       back_aform, Args, ArgCount);

	    /* make pulldown arrow style menu */
	    if (generic_vals.back_arrow.type == -1) {
		type = -1;
	    } else {
		type = generic_vals.back_arrow.type*2 - 1 + generic_vals.back_arrow.style;
		if (type < 0)
		    type = 0;
	    }
	    FirstArg(XtNfromHoriz, beside);
	    NextArg(XtNlabel, "");
	    NextArg(XtNinternalWidth, 0);
	    NextArg(XtNinternalHeight, 0);
	    NextArg(XtNbitmap, arrow_pixmaps[type+1]);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    back_arrow_type_panel = XtCreateManagedWidget("Arrowtype",
					    menuButtonWidgetClass,
					    back_aform, Args, ArgCount);
	    below = back_arrow_type_panel;

	    /* make pulldown menu with arrow type images */
	    make_pulldown_menu_images(dim_arrowtype_choices,
			NUM_ARROW_TYPES, arrow_pixmaps, NULL,
			back_arrow_type_panel, back_arrow_type_select);

	    back_thick = float_panel(generic_vals.back_arrow.thickness, back_aform,
			"Thick ", (Widget) 0, &back_arrow_thick, 0.0, 10000.0, 1.0, 1);
	    back_thick_label = beside;
	    back_width = float_panel(generic_vals.back_arrow.wd, back_aform,
			"Width ", (Widget) 0, &back_arrow_width, 0.0, 10000.0, 1.0, 1);
	    back_width_label = beside;
	    back_height = float_panel(generic_vals.back_arrow.ht, back_aform,
			"Length", (Widget) 0, &back_arrow_height, 0.0, 10000.0, 1.0, 1);
	    back_height_label = beside;
	    below = for_aform;	/* for the widget that follows us in the panel */
	}
    }
    XtInstallAccelerators(form, cancel);
}


static void
spline_point_window(int x, int y)
{
    Widget          but_spline[3];
    Dimension	    label_height, label_width;
    int		    i, dist;

    static char use_item[]="Edit the behavior\nof the control point";
    
    /* locate the mouse in screen coords */
    XtTranslateCoords(canvas_sw, ZOOMX(x), ZOOMY(y), &rootx, &rooty);

    /* position the window to the right of the mouse */
    FirstArg(XtNx, rootx+30);
    NextArg(XtNy, rooty);
    NextArg(XtNtitle, "Edit spline point");
    NextArg(XtNcolormap, tool_cm);
    popup = XtCreatePopupShell("edit_spline_point_panel",
			       transientShellWidgetClass, tool,
			       Args, ArgCount);
    XtAugmentTranslations(popup,
			  XtParseTranslationTable(edit_popup_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, edit_actions, XtNumber(edit_actions));
        XtAppAddActions(tool_app, text_actions, XtNumber(text_actions));
	actions_added = True;
    }

    form = XtCreateManagedWidget("form", formWidgetClass, popup, NULL, 0);

    done_proc = done_spline_point;

    FirstArg(XtNwidth, SFACTOR_BAR_WIDTH);
    NextArg(XtNheight, SFACTOR_BAR_HEIGHT);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    sfactor_bar= XtCreateManagedWidget("sfactor_bar", scrollbarWidgetClass,
				       form, Args, ArgCount);
    XtAddCallback(sfactor_bar, XtNjumpProc,
		  (XtCallbackProc) change_sfactor_value, (XtPointer)NULL);
    XtAddCallback(sfactor_bar, XtNscrollProc,
		  (XtCallbackProc) scroll_sfactor_value, (XtPointer)NULL);

    XawScrollbarSetThumb(sfactor_bar,
			 SFACTOR_TO_PERCENTAGE(sub_sfactor->s), THUMB_H);

    FirstArg(XtNfromHoriz, sfactor_bar);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    label= XtCreateManagedWidget(use_item, labelWidgetClass, form,
			      Args, ArgCount);

    /* get height and width of label widget and distance between widgets */
    FirstArg(XtNheight, &label_height);
    NextArg(XtNwidth, &label_width);
    NextArg(XtNvertDistance, &dist);
    GetValues(label);


    FirstArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, sfactor_bar);
    NextArg(XtNvertDistance, dist);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    but1 = XtCreateManagedWidget("Done", commandWidgetClass, form, Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc) done_button, (XtPointer) NULL);

    below = but1;
    FirstArg(XtNfromHoriz, but1);
    NextArg(XtNfromVert, label);
    NextArg(XtNvertDistance, dist);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    but1 = XtCreateManagedWidget("Cancel", commandWidgetClass, form,
				 Args, ArgCount);
    XtAddCallback(but1, XtNcallback, (XtCallbackProc) cancel_button,
		  (XtPointer) NULL);

    /* fromVert and vertDistance  must be first, for direct access to the Args array in the loop */
    FirstArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 7 * dist); /* it must be second, same reason */
    NextArg(XtNfromHoriz, sfactor_bar);
    NextArg(XtNwidth, label_width);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    for (i=0; i<3; i++)
      {
	below = but_spline[i] = XtCreateManagedWidget(sfactor_type[i].label,
				   commandWidgetClass, form, Args, ArgCount);
	XtAddCallback(but_spline[i], XtNcallback,
		      (XtCallbackProc) toggle_sfactor_type, (XtPointer) i);
	XtSetArg(Args[0], XtNfromVert, below);        /* here are the direct */
	XtSetArg(Args[1], XtNvertDistance, 3 * dist); /* accesses to Args    */
      }
}


static void
toggle_sfactor_type(Widget panel_local, XtPointer _sfactor_index, XtPointer call_data)
{
  int             sfactor_index = (int) _sfactor_index;

  update_sfactor_value(sfactor_type[sfactor_index].value);
  XawScrollbarSetThumb(sfactor_bar,
		       SFACTOR_TO_PERCENTAGE(sub_sfactor->s), THUMB_H);
}


static void
change_sfactor_value(Widget panel_local, XtPointer closure, XtPointer _top)
{
  float 	   *top = (float *) _top;

  update_sfactor_value(PERCENTAGE_TO_CONTROL(*top));
  XawScrollbarSetThumb(panel_local, *top, THUMB_H);

}

static void
scroll_sfactor_value(Widget panel_local, XtPointer closure, XtPointer _num_pixels)
{
  int 		   *num_pixels = (int *) _num_pixels;

  update_sfactor_value(sub_sfactor->s + 
		       (STEP_VALUE * SFACTOR_SIGN((int) num_pixels)));
  XawScrollbarSetThumb(panel_local, SFACTOR_TO_PERCENTAGE(sub_sfactor->s),
		       THUMB_H);
}

static void
update_sfactor_value(double new_value)
{
  if (new_value < S_SPLINE_INTERP || new_value > S_SPLINE_APPROX)
    return;

  if (new_value == S_SPLINE_ANGULAR)
      new_value = 1.0E-5;

  toggle_pointmarker(edited_point->x, edited_point->y);
  draw_subspline(num_spline_points, sub_new_s, ERASE);

  sub_sfactor->s = new_value;

  if ((approx_spline(new_s) && new_value != S_SPLINE_APPROX)
      || (int_spline(new_s) && new_value != S_SPLINE_INTERP))
    new_s->type = (open_spline(new_s)) ? T_OPEN_XSPLINE : T_CLOSED_XSPLINE;

  changed = True;
  draw_subspline(num_spline_points, sub_new_s, PAINT);
  toggle_pointmarker(edited_point->x, edited_point->y);
}

/* come here when the user changes either the fill or pen color */
/* update the fill intensity/pattern images */

void recolor_fill_image(void)
{
    if (fill_style_exists) {
	update_fill_image((Widget) 0, (XtPointer) 0, (XtPointer) 0);
    }
}

/* update the image of the fill intensity or pattern when user presses up/down in spinner */

static void
update_fill_image(Widget w, XtPointer dummy, XtPointer dummy2)
{
    char	  *sval;
    int		   val;
    Pixel	   bg;

    /* get the background color of the main form if we haven't already done that */
    if (XtIsRealized(form)) {
	FirstArg(XtNbackground, &form_bg);
	GetValues(form);
    }

    /* get current value from the fill intensity spinner */

    sval = panel_get_value(fill_intens_panel);
    /* if there is a number there, then intensity is used */
    if (sval[0]!=' ') {
	val = atoi(sval);
	/* check for min/max */
	if ((fill_color == BLACK || fill_color == WHITE)? (val > 100): (val > 200) || val < 0) {
	    if (val < 0)
		val = 0;
	    else {
		if (fill_color == BLACK || fill_color == WHITE)
		    val = 100;
		else
		    val = 200;
	    }
	    panel_set_int(fill_intens_panel,val);
	}

	/* get current colors into the gc */
	XSetForeground(tool_d,fill_image_gc,x_color(fill_color));
	/* shade (use BLACK) or tint (use WHITE)? */
	bg = (val <= 100? BLACK: WHITE);
	XSetBackground(tool_d,fill_image_gc,x_color(bg));

	/* convert intensity to shade/tint index */
	val = val / (200 / (NUMSHADEPATS+NUMTINTPATS-1));

	/* update the pixmap */
	set_image_pm(val);
    } else {
	/* get current value from the fill pattern spinner */
	sval = panel_get_value(fill_pat_panel);
	/* if there is a number there, then pattern is used */
	if (sval[0]==' ') {
	    FirstArg(XtNbitmap, None);
	    NextArg(XtNbackground, form_bg);
	    SetValues(fill_image);
	} else {
	    val = atoi(sval);
	    /* check for min/max */
	    if (val >= NUMPATTERNS || val < 0) {
		if (val < 0)
		    val = 0;
		else
		    val = NUMPATTERNS-1;
	       panel_set_int(fill_pat_panel,val);
	    }
	    val += NUMSHADEPATS+NUMTINTPATS;

	    /* get current colors into the gc */
	    XSetForeground(tool_d,fill_image_gc,x_color(pen_color));
	    XSetBackground(tool_d,fill_image_gc,x_color(fill_color));

	    /* update the pixmap */
	    set_image_pm(val);
	}
    }
}

/* update the pixmap in the proper widget */

void set_image_pm(int val)
{
	/* set stipple from the one-bit bitmap */
	XSetStipple(tool_d, fill_image_gc, fill_image_bm[val]);
	/* stipple the bitmap into the pixmap with the desired color */
	XFillRectangle(tool_d, fill_image_pm, fill_image_gc, 0, 0, FILL_SIZE, FILL_SIZE);
	/* put the pixmap in the widget background */
	FirstArg(XtNbitmap, None);
	SetValues(fill_image);
	FirstArg(XtNbitmap, fill_image_pm);
	SetValues(fill_image);
}

/* make fill style menu and associated stuff */
/* fill = fill value, fill_flag = 0 means no fill, 1=intensity, 2=pattern */

void fill_style_menu(int fill, int fill_flag)
{
	static	char *fill_style_items[] = {
			"No fill", "Filled ", "Pattern"};
	int	i,j;
	Pixel	fg,bg;

	fg = x_color(pen_color);
	bg = x_color(fill_color);
	if (!fill_image_bm_exist) {
	    fill_image_bm_exist = True;
	    fill_image_gc = makegc(PAINT, fg, bg);
	    fill_image_pm = XCreatePixmap(tool_d, tool_w, FILL_SIZE, FILL_SIZE, tool_dpth);
	    XSetFillStyle(tool_d, fill_image_gc, FillOpaqueStippled);
	    /* make the one-bit deep bitmaps and blank pixmaps */
	    for (i = 0; i < NUMSHADEPATS; i++) {
		fill_image_bm[i] = XCreateBitmapFromData(tool_d, tool_w, 
					(char*) shade_images[i], SHADE_IM_SIZE, SHADE_IM_SIZE);
	    }
	    for (i = NUMSHADEPATS; i < NUMSHADEPATS+NUMTINTPATS; i++) {
		j = NUMSHADEPATS+NUMTINTPATS-i-1;	/* reverse the patterns */
		fill_image_bm[i] = XCreateBitmapFromData(tool_d, tool_w, 
					(char*) shade_images[j], SHADE_IM_SIZE, SHADE_IM_SIZE);
	    }
	    for (i = NUMSHADEPATS+NUMTINTPATS; i < NUMFILLPATS; i++) {
		j = i-(NUMSHADEPATS+NUMTINTPATS);
		fill_image_bm[i] = XCreateBitmapFromData(tool_d, tool_w,
				   pattern_images[j].cdata,
				   pattern_images[j].cwidth,
				   pattern_images[j].cheight);
	    }
	}
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("Fill style", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fill_style_button = XtCreateManagedWidget(fill_style_items[fill_flag],
				menuButtonWidgetClass, form, Args, ArgCount);
	below = fill_style_button;
	make_pulldown_menu(fill_style_items, XtNumber(fill_style_items), -1, "",
			       fill_style_button, fill_style_select);

	fill_intens = int_panel_callb(fill, form, "Fill intensity %",
				(Widget) 0, &fill_intens_panel, 0, 200, 5, update_fill_image);
	fill_intens_label = beside;	/* save pointer to fill label */

	fill_pat = int_panel_callb(fill, form, "Fill pattern    ",
				(Widget) 0, &fill_pat_panel, 0, NUMPATTERNS-1, 1, update_fill_image);

	fill_pat_label = beside;	/* save pointer to fill label */

	/* make fill intensity spinner widget sensitive or not */
	XtSetSensitive(fill_intens,(fill_flag==1));
	FirstArg(XtNhorizDistance, 20);
	NextArg(XtNsensitive, (fill_flag==1));
	SetValues(fill_intens_label); /* and label (and position it) */
	/* if fill is not a fill %, blank it out */
	if (fill_flag != 1)
	    panel_clear_value(fill_intens_panel);

	/* make label widget to the right of fill intensity and pattern to show chosen */

	FirstArg(XtNlabel,"");
	NextArg(XtNwidth, FILL_SIZE);
	NextArg(XtNheight, FILL_SIZE);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNfromHoriz, fill_pat);
	NextArg(XtNfromVert, fill_pat);
	NextArg(XtNvertDistance, -FILL_SIZE-2);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fill_image = XtCreateManagedWidget("fill_image", labelWidgetClass,
				form, Args, ArgCount);
	/* put the pixmap in the widget background */
	if (fill_flag == 1 || fill_flag == 2) {
	    update_fill_image((Widget) 0, (XtPointer) 0, (XtPointer) 0);
	}

	/* make fill pattern panel insensitive if not a fill pattern */
	XtSetSensitive(fill_pat,(fill_flag==2));
	FirstArg(XtNhorizDistance, 20);
	NextArg(XtNsensitive, (fill_flag==2));
	SetValues(fill_pat_label);

	/* and blank value if not a pattern */
	if (fill_flag != 2)
	    panel_clear_value(fill_pat_panel);
	/* say we exist */
	fill_style_exists = True;
}

/* make a popup arc type menu */
void arc_type_menu(void)
{
	static char   *arc_type_items[] = {
			 "Open     ", "Pie Wedge"};

	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("  Arc type", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	arc_type_panel = XtCreateManagedWidget(
			arc_type_items[generic_vals.arc_type],
			menuButtonWidgetClass,
			form, Args, ArgCount);
	below = arc_type_panel;
	make_pulldown_menu(arc_type_items, XtNumber(arc_type_items), -1, "",
			       arc_type_panel, arc_type_select);
}

void join_style_panel_menu(void)
{
	FirstArg(XtNfromVert, below);
	NextArg(XtNhorizDistance, 10);	/* space it a bit from the cap style */
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("Join style", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNbitmap, joinstyle_choices[generic_vals.join_style].pixmap);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	join_style_panel = XtCreateManagedWidget("join_style", 
				menuButtonWidgetClass, form, Args, ArgCount);
	make_pulldown_menu_images(joinstyle_choices, NUM_JOINSTYLE_TYPES,
			    joinstyle_pixmaps, NULL,
			    join_style_panel, join_style_select);
}

/* make a popup cap style menu */
void cap_style_panel_menu(void)
{
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("Cap style", labelWidgetClass,
				       form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNbitmap, capstyle_choices[generic_vals.cap_style].pixmap);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	cap_style_panel = XtCreateManagedWidget("cap_style_image", menuButtonWidgetClass,
					form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, cap_style_panel);
	NextArg(XtNborderWidth, 0);
	make_pulldown_menu_images(capstyle_choices, NUM_CAPSTYLE_TYPES, 
			    capstyle_pixmaps, NULL, cap_style_panel, cap_style_select);
}

/* make a button panel with the image 'pixmap' in it */
/* for the font selection */

void		f_menu_popup(void);

static XtCallbackRec f_sel_callback[] =
{
    {f_menu_popup, NULL},
    {NULL, NULL},
};

void set_font_image(Widget widget)
{
    FirstArg(XtNbitmap, new_psflag ?
	     psfont_menu_bitmaps[new_ps_font + 1] :
	     latexfont_menu_bitmaps[new_latex_font]);
    SetValues(widget);
}

static void
font_image_panel(Pixmap pixmap, char *label, Widget *pi_x)
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 2);
    NextArg(XtNbitmap, pixmap);
    NextArg(XtNcallback, f_sel_callback);
    NextArg(XtNwidth, MAX_FONTIMAGE_WIDTH);
    NextArg(XtNinternalWidth, 2);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    *pi_x = XtCreateManagedWidget(label, commandWidgetClass, form, Args, ArgCount);
    below = *pi_x;
}

/* come here when user presses font image button */

void
f_menu_popup(void)
{
    fontpane_popup(&new_ps_font, &new_latex_font, &new_psflag,
		   set_font_image, font_panel);
}

static Widget
pen_color_selection_panel(void)
{
    color_selection_panel(" Pen color","border_colors", "Pen color", form, below, beside,
   			&pen_col_button, &pen_color_popup, 
			generic_vals.pen_color, pen_color_select);
    return pen_col_button;
}

static Widget
fill_color_selection_panel(void)
{
    color_selection_panel("Fill color","fill_colors", "Fill color", form, below, beside,
			&fill_col_button, &fill_color_popup,
			generic_vals.fill_color, fill_color_select);
    return fill_col_button;
}

Widget
color_selection_panel(char *label, char *wname, char *name, Widget parent, Widget below, Widget beside, Widget *button, Widget *popup, int color, XtCallbackProc callback)
{

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass,
				   parent, Args, ArgCount);
    set_color_name(color,buf);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNwidth, COLOR_BUT_WID);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = *button = XtCreateManagedWidget(wname,
		     menuButtonWidgetClass, parent, Args, ArgCount);
    /*
     * cheat a little - set the initial fore/background colors by calling the
     * callback
     */
    /* also set the label */
    (callback)(below, (XtPointer) color, NULL);
    *popup = make_color_popup_menu(below, name, callback, NO_TRANSP, NO_BACKG);

    return *button;
}

static Widget
int_panel(int x, Widget parent, char *label, Widget beside, Widget *pi_x, int min, int max, int inc)
{
    return int_panel_callb(x, parent, label, beside, pi_x, min, max, inc, (XtCallbackProc) 0);
}

static Widget
int_panel_callb(int x, Widget parent, char *label, Widget pbeside, Widget *pi_x, int min, int max, int inc, XtCallbackProc callback)
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, pbeside);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, parent, Args, ArgCount);

    sprintf(buf, "%d", x);
    below = MakeIntSpinnerEntry(parent, pi_x, label, below, beside, 
		callback, buf, min, max, inc, 45);
    /* we want the spinner to be chained to the bottom (it's below the comments) */
    FirstArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    SetValues(below);
    text_transl(*pi_x);
    return below;
}

static Widget
float_panel(float x, Widget parent, char *label, Widget pbeside, Widget *pi_x, float min, float max, float inc, int prec)
{
    char	    fmt[10];

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, pbeside);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, parent,
				   Args, ArgCount);
    /* first make format string from precision */
    sprintf(fmt, "%%.%df", prec);
    /* now format the value */
    sprintf(buf, fmt, x);
    below = MakeFloatSpinnerEntry(parent, pi_x, label, below, beside, 
		(XtCallbackProc) 0, buf, min, max, inc, 50);
    /* we want the spinner to be chained to the bottom (it's below the comments) */
    FirstArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    SetValues(below);
    text_transl(*pi_x);
    return below;
}

static void
float_label(float x, char *label, Widget *pi_x)
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form,
				   Args, ArgCount);
    sprintf(buf, "%1.1f", x);
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNwidth, 40);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    *pi_x = XtCreateManagedWidget(label, labelWidgetClass, form,
				  Args, ArgCount);
    below = *pi_x;
}

static void
int_label(int x, char *label, Widget *pi_x)
{
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form,
				   Args, ArgCount);
    sprintf(buf, "%d", x);
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNwidth, 40);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    *pi_x = XtCreateManagedWidget(label, labelWidgetClass, form,
				  Args, ArgCount);
    below = *pi_x;
}

static void
str_panel(char *string, char *name, Widget *pi_x, int width, Boolean size_to_width, Boolean international)
{
    int		    nlines, i;
    Dimension	    pwidth;
    XFontStruct	   *temp_font;
    char	   *labelname, *textname;


    /* does the user want a label beside the text widget? */
    if (name && strlen(name) > 0) {
	/* yes, make it and manage it */
	/* make the labels of the widgets xxx_label for the label part and xxx_text for
	   the asciiwidget part */
	labelname = (char *) new_string(strlen(name)+7);
	textname = (char *) new_string(strlen(name)+6);
	strcpy(labelname,name);
	strcat(labelname,"_label");
	strcpy(textname,name);
	strcat(textname,"_text");
	FirstArg(XtNfromVert, below);
	NextArg(XtNlabel, name);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget(labelname, labelWidgetClass, form, Args, ArgCount);
	/* get the font and width of above label widget */
	FirstArg(XtNfont, &temp_font);
	NextArg(XtNwidth, &pwidth);
	GetValues(beside);
    } else {
	/* no, make a widget to get the font, but don't manage it */
	beside = (Widget) 0;
	pwidth = 0;
	beside = XtCreateWidget("dummy", labelWidgetClass, form, Args, ArgCount);
	textname = "text_text";
	/* get the font of above label widget */
	FirstArg(XtNfont, &temp_font);
	GetValues(beside);
	beside = 0;
    }
    /* make panel as wide as image pane above less the label widget's width */
    /* but at least "width" pixels wide */
    if (size_to_width)
	width = max2(PS_FONTPANE_WD - pwidth + 2, width);

    /* count number of lines in this text string */
    nlines = 1;			/* number of lines in string */
    for (i = 0; i < strlen(string); i++) {
	if (string[i] == '\n') {
	    nlines++;
	}
    }
    if (nlines > 4)	/* limit to displaying 4 lines and show scrollbars */
	nlines = 4;
    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, string);
    NextArg(XtNinsertPosition, strlen(string));
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, width);
    /* allow enough height for scrollbar */
    NextArg(XtNheight, max_char_height(temp_font) * nlines + 20);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNscrollVertical, XawtextScrollWhenNeeded);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
#ifdef I18N
    if (!appres.international || !international)
      NextArg(XtNinternational, False);
#endif /* I18N */
    *pi_x = XtCreateManagedWidget(textname, asciiTextWidgetClass, form, Args, ArgCount);

    /* make CR do nothing for now */
    text_transl(*pi_x);

    /* read personal key configuration */
    XtOverrideTranslations(*pi_x, XtParseTranslationTable(local_translations));

    below = *pi_x;

    if (name && strlen(name) > 0) {
	free((char *) textname);
	free((char *) labelname);
    }
}

/* call with make_unit_menu = True to make pulldown unit menu beside first label */

static void
xy_panel(int x, int y, char *label, Widget *pi_x, Widget *pi_y, Boolean make_unit_menu)
{
    Widget	    save_below = below;

    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    /* pulldown menu for units */
    if (make_unit_menu)
	(void) unit_pulldown_menu(save_below, below);

    FirstArg(XtNfromVert, below);
    NextArg(XtNhorizDistance, 20);
    NextArg(XtNlabel, "X =");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    cvt_to_units_str(x, buf);
    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, XY_WIDTH);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    *pi_x = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_x);
    add_to_convert(*pi_x);

    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, "Y =");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromHoriz, *pi_x);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    cvt_to_units_str(y, buf);
    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, XY_WIDTH);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    *pi_y = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_y);
    add_to_convert(*pi_y);

    below = *pi_x;
}

/* make an X= Y= pair of labels and text widgets with optional pulldown unit menu */
/* Label for X and Y are variables */

static void
f_pair_panel(F_pos *fp, char *label, Widget *pi_x, char *xlabel, Widget *pi_y, char *ylabel, Boolean make_unit_menu)
{
    Widget	    save_below = below;

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    below = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    /* pulldown menu for units */
    if (make_unit_menu)
	(void) unit_pulldown_menu(save_below, below);

    FirstArg(XtNfromVert, below);
    NextArg(XtNhorizDistance, 20);
    NextArg(XtNlabel, xlabel);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    /* convert first number to desired units */
    cvt_to_units_str(fp->x, buf);

    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, XY_WIDTH);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    *pi_x = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_x);
    /* add this widget to the list that get conversions */
    add_to_convert(*pi_x);

    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, ylabel);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNfromHoriz, *pi_x);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget(label, labelWidgetClass, form, Args, ArgCount);

    /* convert second number to desired units */
    cvt_to_units_str(fp->y, buf);

    FirstArg(XtNfromVert, below);
    NextArg(XtNstring, buf);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNinsertPosition, strlen(buf));
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, XY_WIDTH);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = *pi_y = XtCreateManagedWidget(label, asciiTextWidgetClass, form, Args, ArgCount);
    text_transl(*pi_y);
    /* add this widget to the list that get conversions */
    add_to_convert(*pi_y);

    below = *pi_x;
}

static void
get_f_pos(F_pos *fp, Widget pi_x, Widget pi_y)
{
    fp->x = panel_get_dim_value(pi_x);
    fp->y = panel_get_dim_value(pi_y);
}

/* this makes a scrollable panel in which the x/y points for the
	Fig object are displayed */

static void
points_panel(struct f_point *p)
{
    struct f_point *pts;
    char	    buf[32];
    int		    npts, j;
    Widget	    viewp,formw,beside,npoints;

    /* label */
    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("Points", labelWidgetClass, form,
				  Args, ArgCount);
    /* number of points */
    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, "");
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    npoints = XtCreateManagedWidget("num_points", labelWidgetClass, form,
				  Args, ArgCount);
    /* pulldown menu for units */
    below = unit_pulldown_menu(below, npoints);

    FirstArg(XtNallowVert, True);
    NextArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 2);
    NextArg(XtNhorizDistance, 20);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    pts = p;
    for (npts = 0; pts != NULL; npts++)
	pts = pts->next;
    /* limit size of points panel and scroll if more than 140 pixels */
    if (npts > 6)
	    NextArg(XtNheight, 140);
    viewp = XtCreateManagedWidget("pointspanel", viewportWidgetClass, form, Args, ArgCount);
    formw = XtCreateManagedWidget("pointsform", formWidgetClass, viewp, NULL, 0);
    below = (Widget) 0;
    /* initialize which panels to convert */
    init_convert_array();
    for (j = 0; j < npts; j++) {
	/* limit number of points displayed to prevent system failure :-) */
	if (j >= MAXDISPTS) {
	    FirstArg(XtNfromVert, below);
	    NextArg(XtNtop, XtChainBottom);
	    NextArg(XtNbottom, XtChainBottom);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    XtCreateManagedWidget("Too many points to display  ", labelWidgetClass,
						formw, Args, ArgCount);
	    break;
	}
	FirstArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	sprintf(buf, "X%d =", j);
	beside = XtCreateManagedWidget(buf, labelWidgetClass, formw,
				       Args, ArgCount);
	cvt_to_units_str(p->x, buf);
	FirstArg(XtNfromVert, below);
	NextArg(XtNstring, buf);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinsertPosition, strlen(buf));
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNwidth, XY_WIDTH);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	px_panel[j] = XtCreateManagedWidget("xy", asciiTextWidgetClass,
					    formw, Args, ArgCount);
	text_transl(px_panel[j]);
	/* this panel is also converted by units */
	add_to_convert(px_panel[j]);

	sprintf(buf, "Y%d =", j);
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, px_panel[j]);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget(buf, labelWidgetClass,
				       formw, Args, ArgCount);

	cvt_to_units_str(p->y, buf);
	FirstArg(XtNfromVert, below);
	NextArg(XtNstring, buf);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNinsertPosition, strlen(buf));
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNwidth, XY_WIDTH);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);

	py_panel[j] = XtCreateManagedWidget("xy", asciiTextWidgetClass,
					    formw, Args, ArgCount);
	text_transl(py_panel[j]);
	/* this panel is also converted by units */
	add_to_convert(py_panel[j]);

	below = px_panel[j];

	p = p->next;
    }
    /* now put the (actual) number of points in a label */
    sprintf(buf, "%d", npts);
    FirstArg(XtNlabel, buf);
    SetValues(npoints);
}

static void
get_points(struct f_point *p)
{
    struct f_point *q;
    int		    i;

    for (q = p, i = 0; q != NULL; i++) {
	if (i >= MAXDISPTS)
	    break;
	q->x = panel_get_dim_value(px_panel[i]);
	q->y = panel_get_dim_value(py_panel[i]);
	q = q->next;
    }
}

/* make a unit pulldown menubutton and menu */

static Widget
unit_pulldown_menu(Widget below, Widget beside)
{
	/* put the current user units in unit_items[0] */
	unit_items[0] = cur_fig_units;
	
	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 15);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("Units:",labelWidgetClass, form, Args, ArgCount);
	FirstArg(XtNfromVert, below);
	NextArg(XtNwidth, 95);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 2);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	unit_menu_button = XtCreateManagedWidget(unit_items[points_units],
					menuButtonWidgetClass, form, Args, ArgCount);
	/* pulldown menu for units */
	make_pulldown_menu(unit_items, XtNumber(unit_items), -1, "",
		       unit_menu_button, unit_select);
	return unit_menu_button;
}
static void
init_convert_array(void)
{
    conv_array_idx = 0;
}

static void
add_to_convert(Widget widget)
{
    convert_array[conv_array_idx++] = widget;
}
    
/* unit conversion for displaying coordinates */

static double
cvt_to_units(int fig_unit)
{
    if (points_units == 1)
	return (double) fig_unit;
    return (double) fig_unit /
		(double) (appres.INCHES? PIX_PER_INCH: PIX_PER_CM)*appres.userscale;
}

/* unit conversion that writes ASCII to buf */

static void
cvt_to_units_str(int x, char *buf)
{
    double	    dval;

    if (points_units) {
	sprintf(buf, "%d", x);
    } else {
	dval = cvt_to_units(x);
	sprintf(buf, "%.4lf", dval);
    }
}

static int
cvt_to_fig(double real_unit)
{
    if (points_units)
	return round(real_unit);
    return round(real_unit/appres.userscale * (appres.INCHES? PIX_PER_INCH: PIX_PER_CM));
}

static void
unit_select(Widget w, XtPointer new_unit, XtPointer call_data)
{
    int		    i, new_points_units;
    char	    buf[30];
    int		    ival;
    double	    val;

    new_points_units = (int) new_unit;
    if (points_units == new_points_units)
	return;

    /* change label in menu button */
    FirstArg(XtNlabel, unit_items[new_points_units]);
    SetValues(unit_menu_button);

    /* now go through the array of text widgets and convert their values */
    points_units = 0;
    for (i=0; i<conv_array_idx; i++) {
	/* get the value from the next widget */
	val = atof(panel_get_value(convert_array[i]));
	if (new_points_units) {
	    ival = cvt_to_fig(val);
	    sprintf(buf, "%d", ival);
	} else {
	    cvt_to_units_str(round(val), buf);
	}
	/* put the value back */
	panel_set_value(convert_array[i], buf);
    }
    points_units = new_points_units;
}

void
Quit(void)
{
    /* turn off the point positioning indicator now */
    update_indpanel(0);
    popup_up = False;
    XtDestroyWidget(popup);
    fill_style_exists = False;
    if (pen_color_popup) {
	XtDestroyWidget(pen_color_popup);
	pen_color_popup = 0;
    }
    if (fill_color_popup) {
	XtDestroyWidget(fill_color_popup);
	fill_color_popup = 0;
    }
}

static int
panel_get_dim_value(Widget widg)
{
    char	*str = panel_get_value(widg);

    if (strchr(str, 'i'))
	return (atof(str) * PIX_PER_INCH);
    else if (strchr(str, 'c'))
	return (atof(str) * PIX_PER_CM);
    else
	return cvt_to_fig((atof(str)));
}

/* put an integer value into an ASCII text widget after converting
   it from Fig to (double) user units */

static void
panel_set_scaled_int(Widget widg, int intval)
{
    char	    buf[80];
    cvt_to_units_str(intval, buf);
    panel_set_value(widg, buf);
}

static void
panel_clear_value(Widget widg)
{
    FirstArg(XtNstring, " ");
    NextArg(XtNinsertPosition, 0);
    SetValues(widg);
}

static void
arc_type_select(Widget w, XtPointer new_style, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(arc_type_panel);

    generic_vals.arc_type = (int) new_style;
    /* if now a pie-wedge type, make the arrow panels insensitive */
    if (generic_vals.arc_type == T_PIE_WEDGE_ARC) {
	/* unmanage arrow forms */
	XtUnmanageChild(for_aform);
	XtUnmanageChild(back_aform);
	/* unmanage and re-manage points form so it moves up */
	XtUnmanageChild(arc_points_form);
	/* position it just under the stuff above the arrow forms */
	FirstArg(XtNfromVert, above_arrows);
	SetValues(arc_points_form);
	XtManageChild(arc_points_form);
    } else {
	/* re-manage arrow forms */
	XtManageChild(for_aform);
	XtManageChild(back_aform);
	/* unmanage and re-manage points form so it moves down */
	XtUnmanageChild(arc_points_form);
	/* position it just under the arrow forms */
	FirstArg(XtNfromVert, for_aform);
	SetValues(arc_points_form);
	XtManageChild(arc_points_form);
    }
    /* allow the form to resize */
    XtUnmanageChild(form);
    XtManageChild(form);
}

static void
cap_style_select(Widget w, XtPointer new_type, XtPointer call_data)
{
    choice_info	    *choice;

    choice = (choice_info *) new_type;
    FirstArg(XtNbitmap, choice->pixmap);
    SetValues(cap_style_panel);

    generic_vals.cap_style = choice->value;
}

static void
join_style_select(Widget w, XtPointer new_type, XtPointer call_data)
{
    choice_info	    *choice;

    choice = (choice_info *) new_type;
    FirstArg(XtNbitmap, choice->pixmap);
    SetValues(join_style_panel);

    generic_vals.join_style = choice->value;
}

static void
for_arrow_type_select(Widget w, XtPointer new_type, XtPointer call_data)
{
    int	    	    choice;

    choice = (int) ((choice_info *) new_type)->value;
    FirstArg(XtNbitmap, arrow_pixmaps[choice+1]);
    SetValues(for_arrow_type_panel);
    for_arrow = (choice != -1);

    if (for_arrow) {
	generic_vals.for_arrow.type = ARROW_TYPE(choice);
	generic_vals.for_arrow.style = ARROW_STYLE(choice);
    }
}

static void
back_arrow_type_select(Widget w, XtPointer new_type, XtPointer call_data)
{
    int	    	    choice;

    choice = (int) ((choice_info *) new_type)->value;
    FirstArg(XtNbitmap, arrow_pixmaps[choice+1]);
    SetValues(back_arrow_type_panel);
    back_arrow = (choice != -1);

    if (back_arrow) {
	generic_vals.back_arrow.type = ARROW_TYPE(choice);
	generic_vals.back_arrow.style = ARROW_STYLE(choice);
    }
}

static void
line_style_select(Widget w, XtPointer new_style, XtPointer call_data)
{
    Boolean	    state;
    choice_info	    *choice;

    choice = (choice_info *) new_style;
    FirstArg(XtNbitmap, choice->pixmap);
    SetValues(line_style_panel);

    generic_vals.style = choice->value;

    switch (generic_vals.style) {
      case SOLID_LINE:
	panel_clear_value(style_val_panel);
	state = False;
	break;
      case DASH_LINE:
	/*
	 * if style_val contains no useful value, set it to the default
	 * dashlength, scaled by the line thickness
	 */
	if (generic_vals.style_val <= 0.0)
	    generic_vals.style_val = cur_dashlength * (generic_vals.thickness + 1) / 2;
	panel_set_float(style_val_panel, generic_vals.style_val, "%1.1f");
	state = True;
	break;
      default:		/* DOTTED, DASH-DOTTED, and the others */
	if (generic_vals.style_val <= 0.0)
	    generic_vals.style_val = cur_dotgap * (generic_vals.thickness + 1) / 2;
	panel_set_float(style_val_panel, generic_vals.style_val, "%1.1f");
	state = True;
	break;
    }
    /* make both the label and value panels sensitive or insensitive */
    XtSetSensitive(style_val,state);
    XtSetSensitive(style_val_label,state);
}

static void
pen_color_select(Widget w, XtPointer new_color, XtPointer call_data)
{
    pen_color = (Color) new_color;
    color_select(pen_col_button, pen_color);
    if (pen_color_popup) {
	XtPopdown(pen_color_popup);
    }
}

static void
fill_color_select(Widget w, XtPointer new_color, XtPointer call_data)
{
    fill_color = (Color) new_color;
    color_select(fill_col_button, fill_color);
    if (fill_color_popup) {
	XtPopdown(fill_color_popup);
    }
}

void
color_select(Widget w, Color color)
{
    XFontStruct	   *f;

    /* update colors for fill image gc */
    recolor_fill_image();

    FirstArg(XtNlabel, XtName(w));
    SetValues(w);
    set_color_name(color,buf);
    FirstArg(XtNfont, &f);
    GetValues(w);
    FirstArg(XtNlabel, buf);
    /* don't know why, but we *MUST* set the size again here or it
       will stay the width that it first had when created */
    NextArg(XtNwidth, COLOR_BUT_WID);

    if (all_colors_available) { /* set color if possible */
	XColor		xcolor;
	Pixel		col;

	/* background in the color selected */
	col = (color < 0 || color >= NUM_STD_COLS+num_usr_cols) ?
			x_fg_color.pixel : colors[color];
	NextArg(XtNbackground, col);
	xcolor.pixel = col;
	/* get RGB of the color to check intensity */
	XQueryColor(tool_d, tool_cm, &xcolor);
	/* set the foreground in a contrasting color (white or black) */
	if ((0.3 * xcolor.red + 0.59 * xcolor.green + 0.11 * xcolor.blue) <
			0.55 * (255 << 8))
	    col = colors[WHITE];
	else
	    col = colors[BLACK];
	NextArg(XtNforeground, col);
    }
    SetValues(w);
}

static void
hidden_text_select(Widget w, XtPointer new_hidden_text, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(hidden_text_panel);
    hidden_text_flag = (int) new_hidden_text;
}

static void
rigid_text_select(Widget w, XtPointer new_rigid_text, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(rigid_text_panel);
    rigid_text_flag = (int) new_rigid_text;
}

static void
special_text_select(Widget w, XtPointer new_special_text, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(special_text_panel);
    special_text_flag = (int) new_special_text;
}

static void
textjust_select(Widget w, XtPointer new_textjust, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(textjust_panel);
    textjust = (int) new_textjust;
}

static void
flip_pic_select(Widget w, XtPointer new_flipflag, XtPointer call_data)
{
    struct f_point  p1, p2;
    int		    dx, dy, rotation;
    float	    ratio;

    FirstArg(XtNlabel, XtName(w));
    SetValues(flip_pic_panel);
    flip_pic_flag = (int) new_flipflag;
    p1.x = panel_get_dim_value(x1_panel);
    p1.y = panel_get_dim_value(y1_panel);
    p2.x = panel_get_dim_value(x2_panel);
    p2.y = panel_get_dim_value(y2_panel);

    /* size is upper-lower */
    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    rotation = what_rotation(dx,dy);

    if (dx == 0 || dy == 0)
	ratio = 0.0;
    else if (((rotation == 0 || rotation == 180) && !flip_pic_flag) ||
	     (rotation != 0 && rotation != 180 && flip_pic_flag))
	ratio = (float) fabs((double) dy / (double) dx);
    else
	ratio = (float) fabs((double) dx / (double) dy);
    sprintf(buf, "%1.1f", ratio);
    FirstArg(XtNlabel, buf);
    SetValues(hw_ratio_panel);
}

static void
rotation_select(Widget w, XtPointer new_rotation, XtPointer call_data)
{
    struct f_point  p1, p2;
    int		    dx, dy, cur_rotation, rotation;
    int		    x1, y1, x2, y2;

    FirstArg(XtNlabel, XtName(w));
    SetValues(rotation_panel);
    /* get new rotation (0 = 0 degrees, 1 = 90, 2 = 180, 3 = 270) */
    rotation = (int) new_rotation;

    /* get the two opposite corners */
    p1.x = panel_get_dim_value(x1_panel);
    p1.y = panel_get_dim_value(y1_panel);
    p2.x = panel_get_dim_value(x2_panel);
    p2.y = panel_get_dim_value(y2_panel);

    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    /* the current rotation based on the current corners */
    cur_rotation = what_rotation(dx,dy);

    /* adjust p2 relative to p1 to get desired rotation */
    x1 = p1.x;
    y1 = p1.y;
    x2 = p2.x;
    y2 = p2.y;
    switch (cur_rotation) {
	case 0:
	    switch (rotation) {
		case 0:	/* 0 degrees */
			break;
		case 1:	/* 90 degrees */
			x2 = x1 + dy;
			y2 = y1 - dx;
			break;
		case 2:	/* 180 degrees */
			x2 = x1 - dx;
			y2 = y1 - dy;
			break;
		case 3:	/* 270 degrees */
			x2 = x1 - dy;
			y2 = y1 + dx;
			break;
	    }
	    break;
	case 90:
	    switch (rotation) {
		case 0:	/* 0 degrees */
			x2 = x1 - dy;
			y2 = y1 + dx;
			break;
		case 1:	/* 90 degrees */
			break;
		case 2:	/* 180 degrees */
			x2 = x1 + dy;
			y2 = y1 - dx;
			break;
		case 3:	/* 270 degrees */
			x2 = x1 - dx;
			y2 = y1 - dy;
			break;
	    }
	    break;
	case 180:
	    switch (rotation) {
		case 0:	/* 0 degrees */
			x2 = x1 - dx;
			y2 = y1 - dy;
			break;
		case 1:	/* 90 degrees */
			x2 = x1 - dy;
			y2 = y1 + dx;
			break;
		case 2:	/* 180 degrees */
			break;
		case 3:	/* 270 degrees */
			x2 = x1 + dy;
			y2 = y1 - dx;
			break;
	    }
	    break;
	case 270:
	    switch (rotation) {
		case 0:	/* 0 degrees */
			x2 = x1 + dy;
			y2 = y1 - dx;
			break;
		case 1:	/* 90 degrees */
			x2 = x1 - dx;
			y2 = y1 - dy;
			break;
		case 2:	/* 180 degrees */
			x2 = x1 - dy;
			y2 = y1 + dx;
			break;
		case 3:	/* 270 degrees */
			break;
	    }
	    break;
    }
    /* put them back in the panel */
    panel_set_scaled_int(x2_panel, x2);
    panel_set_scaled_int(y2_panel, y2);
    /* finally, update width and height */
    panel_set_scaled_int(width_panel, abs(x2-x1));
    panel_set_scaled_int(height_panel, abs(y2-y1));
}

static void
fill_style_select(Widget w, XtPointer new_fillflag, XtPointer call_data)
{
    int		    fill;
    char	   *sval;

    FirstArg(XtNlabel, XtName(w));
    SetValues(fill_style_button);
    fill_flag = (int) new_fillflag;

    if (fill_flag == 0) { 
      /* no fill; blank out fill density value and pattern */
	panel_clear_value(fill_intens_panel);
	panel_clear_value(fill_pat_panel);
	/* make fill% panel insensitive */
	fill_style_sens(False);
	/* and pattern */
	fill_pat_sens(False);
    } else if (fill_flag == 1) { 
      /* filled with color or gray */
	sval = panel_get_value(fill_intens_panel);
	/* if there is already a number there, use it */
	if (sval[0]!=' ') {
	    fill = atoi(sval);
	} else {
	    /* else use 100% */
	    fill = 100;
	}
	if (fill < 0)
	    fill = 100;
	if (fill > 200)
	    fill = 100;
	panel_set_int(fill_intens_panel, fill);
	/* clear value in pattern panel */
	panel_clear_value(fill_pat_panel);
	/* make fill% panel sensitive */
	fill_style_sens(True);
	/* make fill pattern panel insensitive */
	fill_pat_sens(False);
    } else {
      /* filled with pattern */
	sval = panel_get_value(fill_pat_panel);
	/* if there is already a number there, use it */
	if (sval[0]!=' ') {
	    fill = atoi(sval);
	} else {
	    /* else use first pattern */
	    fill = 0;
	}
	panel_set_int(fill_pat_panel, fill);
	/* clear value in fill % panel */
	panel_clear_value(fill_intens_panel);
	/* make fill pattern panel sensitive */
	fill_pat_sens(True);
	/* make fill% panel insensitive */
	fill_style_sens(False);
    }
}

void fill_style_sens(Boolean state)
{
	XtSetSensitive(fill_intens,state);
	XtSetSensitive(fill_intens_label,state);
	update_fill_image((Widget) 0, (XtPointer) 0, (XtPointer) 0);
}

void fill_pat_sens(Boolean state)
{
	XtSetSensitive(fill_pat,state);
	XtSetSensitive(fill_pat_label,state);
	update_fill_image((Widget) 0, (XtPointer) 0, (XtPointer) 0);
}

void
clear_text_key(Widget w)
{
	panel_set_value(w, "");
}

static void get_clipboard(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer buf, long unsigned int *length, int *format);

void
paste_panel_key(Widget w, XKeyEvent *event)
{
	Time event_time;

        event_time = event->time;
        XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, get_clipboard, w, event_time);
}

static void
get_clipboard(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer buf, long unsigned int *length, int *format)
{
	char *c, *p;
	int i;
	char s[256];

	strcpy (s, panel_get_value(client_data));
	p = strchr(s, '\0');
	c = buf;
	for (i=0; i<*length; i++) {
		if (*c=='\0' || *c=='\n' || *c=='\r' || strlen(s)>=sizeof(s)-1)
			break;
		*p = *c;
		p++;
		*p = '\0';
		c++;
	}
	XtFree(buf);
	panel_set_value(client_data, s);
}

void text_transl(Widget w)
{
	/* make CR finish edit */
	XtOverrideTranslations(w, XtParseTranslationTable(edit_text_translations));

	/* enable mousefun kbd display */
	XtAugmentTranslations(w, XtParseTranslationTable(kbd_translations));
}


void
change_sfactor(int x, int y, unsigned int button)
{
  F_spline *spl, *spline;
  F_point  *prev, *the_point;
  F_point   p1, p2;
  F_sfactor *associated_sfactor;

  prev = &p1;
  the_point = &p2;
  spl = get_spline_point(x, y, &prev, &the_point);

  if (spl == NULL) {
    put_msg("Only spline points can be edited");
    return;
  }

  if (open_spline(spl) && ((prev == NULL) || (the_point->next == NULL))) {
    put_msg("Cannot edit end-points");
    return;
  }
  toggle_pointmarker(the_point->x,  the_point->y);

  spline = copy_spline(spl);
  if (spline == NULL)
    return;
  associated_sfactor = search_sfactor(spline,
		      search_spline_point(spline, the_point->x, the_point->y));

  change_spline(spl, spline);
  draw_spline(spline, ERASE);  

  switch (button) {
    case Button1:
      associated_sfactor->s += 2*STEP_VALUE;
      if (associated_sfactor->s > S_SPLINE_APPROX)
	associated_sfactor->s = S_SPLINE_APPROX;
      break;
    case Button2:
      associated_sfactor->s = (round(associated_sfactor->s)) +
	(S_SPLINE_APPROX - S_SPLINE_ANGULAR);
      if (associated_sfactor->s > S_SPLINE_APPROX)
	associated_sfactor->s = S_SPLINE_INTERP;
      break;
    case Button3:
      associated_sfactor->s -= 2*STEP_VALUE;
      if (associated_sfactor->s < S_SPLINE_INTERP)
	associated_sfactor->s = S_SPLINE_INTERP;
      break;
    }

  spline->type = open_spline(spline) ? T_OPEN_XSPLINE : T_CLOSED_XSPLINE;
  draw_spline(spline, PAINT);  
  toggle_pointmarker(the_point->x, the_point->y);
}

void check_depth(void)
{
    int depth;
    depth = atoi(panel_get_value(depth_panel));
    if (depth >= 0 && depth <= MAX_DEPTH)
	return;
    if (depth < MIN_DEPTH)
	depth = MIN_DEPTH;
    else if (depth > MAX_DEPTH)
	depth = MAX_DEPTH;
    panel_set_int(depth_panel, depth);
}

void check_thick(void)
{
    int thick;
    thick = atoi(panel_get_value(thickness_panel));
    if (thick >= 0 && thick <= MAX_LINE_WIDTH)
	return;
    if (thick < 0)
	thick = 0;
    else if (thick > MAX_LINE_WIDTH)
	thick = MAX_LINE_WIDTH;
    panel_set_int(thickness_panel, thick);
}

/*
 these functions  push_apply_button, grab_button,
                       popup_browse_panel & image_edit_button
      implement gif screen capture facility
   note push_apply_button also called from w_browse.c
*/

void
push_apply_button(void)
{
    /* get rid of anything that ought to be done - X wise */
    app_flush();

    button_result = APPLY;
    done_proc();
}


static          void
grab_button(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    time_t	    tim;
    char	    tmpfile[PATH_MAX],tmpname[PATH_MAX];
    char	   *p;

    if (!canHandleCapture(tool_d)) {
      put_msg("Can't capture screen");
      beep();
      return;
    }

    /*  build up a temporary file name from the user login name and the current time */
    tim = time( (time_t*)0);
    /* get figure name without path */
    if (*cur_filename == '\0')
	strcpy(tmpname, "NoName");	/* no name, use "NoName" */
    else 
	strcpy(tmpname,xf_basename(cur_filename));
    /* chop off any .suffix */
    if (p=strrchr(tmpname,'.'))
	*p='\0';
	
    sprintf(tmpfile,"%s_%ld.png",tmpname,tim);

    /* capture the screen area into our tmpfile */

    if (captureImage(popup, tmpfile) == True) {
      panel_set_value(pic_name_panel, tmpfile);
      push_apply_button();
    }
}

/* 
  Edit button has been pushed - invoke an editor on the current file 
*/

static          void
image_edit_button(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    int		argc;
    char	*argv[20];
    pid_t	pid;
    char	cmd[PATH_MAX];
    char	s[PATH_MAX];
    struct stat	original_stat;
    int		err;
    char	*cp;

    /* get the filename for the picture object */
    strcpy(s,panel_get_value(pic_name_panel));
    if (*s == '\0')	/* no name, return */
	return;

    /* uncompress the image file if it is compressed (new name returns in s) */
    if (uncompress_file(s) == False) /* failed! no point in continuing */
        return;

    /* store name back in case any .gz or .Z was removed after uncompressing */
    panel_set_value(pic_name_panel,s);

    button_result = APPLY;

    strcpy(cmd, cur_image_editor);
    argc = 0;
    /* get first word from string as the program */
    argv[argc++] = strtok(cmd," \t");
    /* if there is more than one word, separate args */
    while ((cp = strtok((char*) NULL," \t")) && argc < sizeof(argv)/sizeof(argv[0]) - 2)
      argv[argc++] = cp;
    argv[argc++] = s;  /* put the filename last */
    argv[argc] = NULL;	/* terminate the list */

    /* get the file status (to compare file modification time later) */
    if (stat( s, &original_stat ) != 0 ) /* stat failed! no point in continuing */
        return;

    pid = fork();
    if ( pid == 0 ) {
	err = execvp(argv[0], argv);
	/* should only come here if an error in the execlp */
	fprintf(stderr,"Error in exec'ing image editor (%s): %s\n",
			argv[0], strerror(errno));
        exit(-1);
    }

    if ( pid > 0 ) { /* wait for the lad to finish */
        int status;
        struct stat new_stat;

        (void) waitpid( pid, &status, 0 );

        /* if file modification time has changed, set the changed flag -
           causes a reread of the file as it thinks the name has changed  */
        stat(s, &new_stat );
        if ( original_stat.st_mtime != new_stat.st_mtime ) {
	    new_l->pic->pic_cache->bitmap = NULL;
	    push_apply_button();
	}

    } else {
        fprintf(stderr,"Unable to fork to exec image editor (%s): %s\n",
		argv[0], strerror(errno));
    }
}


static          void
browse_button(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    popup_browse_panel( form );
}

/* collapse the depths in new_c to min_compound_depth and update the max depth value */

static void
collapse_depth(Widget panel_local, XtPointer closure, XtPointer call_data)
{
    collapse_depths(new_c);
    sprintf(buf,"Maximum: %d", min_compound_depth);
    FirstArg(XtNlabel, buf);
    SetValues(max_depth_w);
}

/* need this to recurse through compounds inside new_c */

void collapse_depths(F_compound *compound)
{
	F_line	 *l;
	F_spline	 *s;
	F_ellipse	 *e;
	F_arc	 *a;
	F_text	 *t;
	F_compound *c;

	for (l = compound->lines; l != NULL; l = l->next) {
	    remove_depth(O_POLYLINE, l->depth);
	    l->depth = min_compound_depth;
	    add_depth(O_POLYLINE, l->depth);
	}
	for (s = compound->splines; s != NULL; s = s->next) {
	    remove_depth(O_SPLINE, s->depth);
	    s->depth = min_compound_depth;
	    add_depth(O_SPLINE, s->depth);
	}
	for (e = compound->ellipses; e != NULL; e = e->next) {
	    remove_depth(O_ELLIPSE, e->depth);
	    e->depth = min_compound_depth;
	    add_depth(O_ELLIPSE, e->depth);
	}
	for (a = compound->arcs; a != NULL; a = a->next) {
	    remove_depth(O_ARC, a->depth);
	    a->depth = min_compound_depth;
	    add_depth(O_ARC, a->depth);
	}
	for (t = compound->texts; t != NULL; t = t->next) {
	    remove_depth(O_TEXT, t->depth);
	    t->depth = min_compound_depth;
	    add_depth(O_TEXT, t->depth);
	}
	for (c = compound->compounds; c != NULL; c = c->next) {
	    collapse_depths(c);
	}
}

/* return rotation value based on the signs of dx, dy which come from the
 * two corners of the picture (dx = p2.x - p1.x, dy = p2.y - p1.y)
 */

static int
what_rotation(int dx, int dy)
{
    int rotation;

    rotation = 0;
    if (dx >= 0 && dy < 0)
	rotation = 90;
    else if (dx < 0 && dy < 0)
	rotation = 180;
    else if (dx < 0 && dy >= 0)
	rotation = 270;
    return rotation;
}
