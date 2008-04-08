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
#include "main.h"
#include "mode.h"
#include "object.h"
#include "f_util.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_drawprim.h"
#include "w_export.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_print.h"
#include "w_util.h"
#include "w_setup.h"

#include <X11/IntrinsicP.h> /* XtResizeWidget() */

#ifdef I18N
#include "d_text.h"
#endif /* I18N */

/* EXPORTS */

char	 *grid_inch_choices[] = { "None", "1/16", "1/8", "1/4", "1/2", "1", "2", "5", "10" };
int	  num_grid_inch_choices = sizeof(grid_inch_choices) / sizeof(char *);

char	 *grid_tenth_inch_choices[] = { "None", "1/10", "1/5", "1/2", "1", "2", "5", "10" };
int	  num_grid_tenth_inch_choices = sizeof(grid_tenth_inch_choices) / sizeof(char *);

char	 *grid_cm_choices[] = { "None", "1 mm", "2 mm", "5 mm", "10 mm",
					"20 mm", "50 mm", "100 mm" };
int	  num_grid_cm_choices = sizeof(grid_cm_choices) / sizeof(char *);

char	**grid_choices;
int	  n_grid_choices, grid_minor, grid_major;
Pixmap	  check_pm, null_check_pm;
Pixmap	  sm_check_pm, sm_null_check_pm;
Pixmap	  balloons_on_bitmap=(Pixmap) 0;
Pixmap	  balloons_off_bitmap=(Pixmap) 0;
Pixmap	  menu_arrow, menu_cascade_arrow;
Pixmap	  arrow_pixmaps[NUM_ARROW_TYPES+1];
Pixmap	  diamond_pixmap;
Pixmap	  linestyle_pixmaps[NUM_LINESTYLE_TYPES];
Pixmap	  mouse_l=(Pixmap) 0,		/* mouse indicator bitmaps for the balloons */
	  mouse_r=(Pixmap) 0;

/* LOCALS */

DeclareStaticArgs(14);
static void _installscroll(Widget parent, Widget widget);

static Pixmap	spinup_bm=0;	/* pixmaps for spinners */
static Pixmap	spindown_bm=0;
static void	validate_int(Widget w, XtPointer info, XtPointer dum);	/* validation for spinners */
static void	convert_gridstr(Widget widget, float mult);

/* for internal consumption only (use MakeIntSpinnerEntry or MakeFloatSpinnerEntry) */
static Widget	MakeSpinnerEntry(Widget parent, Widget *text, char *name, Widget below, Widget beside, XtCallbackProc callback, char *string, int type, float min, float max, float inc, int width);

/* bitmap for checkmark */
static unsigned char check_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x7e, 0x00, 0x7f,
   0x8c, 0x1f, 0xde, 0x07, 0xfe, 0x03, 0xfc, 0x01, 0xf8, 0x00, 0xf0, 0x00,
   0x70, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00};

/* smaller checkmark */
/* sm_check_width and _height are defined in w_util.c so w_layers.c can use them */
static unsigned char sm_check_bits[] = {
   0x00, 0x00, 0x80, 0x01, 0xc0, 0x01, 0xe0, 0x01, 0x76, 0x00, 0x3e, 0x00,
   0x1c, 0x00, 0x18, 0x00, 0x08, 0x00, 0x00, 0x00};

#define balloons_on_width 16
#define balloons_on_height 15
static unsigned char balloons_on_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0xfe, 0x67, 0xfe, 0x63, 0xfe, 0x71, 0xfe, 0x79,
   0xfe, 0x7c, 0xe2, 0x7c, 0x46, 0x7e, 0x0e, 0x7e, 0x0e, 0x7f, 0x1e, 0x7f,
   0x9e, 0x7f, 0xfe, 0x7f, 0x00, 0x00};

#define balloons_off_width 16
#define balloons_off_height 15
static unsigned char balloons_off_bits[] = {
   0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
   0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
   0x01, 0x80, 0x01, 0x80, 0xff, 0xff};

/* spinner up/down icons */

#define spinup_width 9
#define spinup_height 6
static char spinup_bits[] = {
   0x10, 0x00, 0x38, 0x00, 0x7c, 0x00, 0xfe, 0x00, 0xff, 0x01, 0x00, 0x00};
#define spindown_width 9
#define spindown_height 6
static char spindown_bits[] = {
   0x00, 0x00, 0xff, 0x01, 0xfe, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x10, 0x00};

#define mouse_r_width 19
#define mouse_r_height 10
static unsigned char mouse_r_bits[] = {
   0xff, 0xff, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07, 0x41, 0xf0, 0x07,
   0x41, 0xf0, 0x07, 0xff, 0xff, 0x07};

#define mouse_l_width 19
#define mouse_l_height 10
static unsigned char mouse_l_bits[] = {
   0xff, 0xff, 0x07, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04, 0x7f, 0x10, 0x04,
   0x7f, 0x10, 0x04, 0xff, 0xff, 0x07};

/* arrow for pull-down menus */

#define menu_arrow_width 11
#define menu_arrow_height 13
static unsigned char menu_arrow_bits[] = {
  0xf8,0x00,0xd8,0x00,0xa8,0x00,0xd8,0x00,0xa8,0x00,0xd8,0x00,
  0xaf,0x07,0x55,0x05,0xaa,0x02,0x54,0x01,0xa8,0x00,0x50,0x00,
  0x20,0x00};

/* arrow for cascade menu entries */

#define menu_cascade_arrow_width 10
#define menu_cascade_arrow_height 12
static unsigned char menu_cascade_arrow_bits[] = {
   0x00, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x3e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x01, 0xfe, 0x00, 0x3e, 0x00, 0x0e, 0x00, 0x02, 0x00, 0x00, 0x00};

/* diamond for library menu to indicate toplevel files.
 * For the XAW3D1_5E version, there is a single point on the left edge and the 
 * diamond is moved to the right because the menu's 3D edge obscures it otherwise */

#ifdef XAW3D1_5E
#define diamond_width 12
#define diamond_height 7
static unsigned char diamond_bits[] = {
   0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xe1,
   0x0f, 0xc0, 0x07, 0x80, 0x03, 0x00, 0x01};
#else
#define diamond_width 7
#define diamond_height 7
static unsigned char diamond_bits[] = {
   0x08, 0x1c, 0x3e, 0x7f, 0x3e, 0x1c, 0x08};
#endif /* XAW3D1_5E */

/* popup a confirmation window */

static int	query_result, query_done;
static String   query_translations =
        "<Message>WM_PROTOCOLS: DismissQuery()\n";
static void     accept_cancel(Widget widget, XtPointer closure, XtPointer call_data);
static XtActionsRec     query_actions[] =
{
    {"DismissQuery", (XtActionProc) accept_cancel},
};

/* synchronize so that (e.g.) new cursor appears etc. */



void app_flush(void)
{
	/* this method prevents "ghost" rubberbanding when the user
	   moves the mouse after creating/resizing object */
	XSync(tool_d, False);
}

static void
accept_yes(Widget widget, XtPointer closure, XtPointer call_data)
{
    query_done = 1;
    query_result = RESULT_YES;
}

static void
accept_no(Widget widget, XtPointer closure, XtPointer call_data)
{
    query_done = 1;
    query_result = RESULT_NO;
}

static void
accept_cancel(Widget widget, XtPointer closure, XtPointer call_data)
{
    query_done = 1;
    query_result = RESULT_CANCEL;
}

static void
accept_part(Widget widget, XtPointer closure, XtPointer call_data)
{
    query_done = 1;
    query_result = RESULT_PART;
}

static void
accept_all(Widget widget, XtPointer closure, XtPointer call_data)
{
    query_done = 1;
    query_result = RESULT_ALL;
}

int
popup_query(int query_type, char *message)
{
    Widget	    query_popup, query_form, query_message;
    Widget	    query_yes, query_no, query_cancel;
    Widget	    query_part, query_all;
    int		    xposn, yposn;
    Window	    win;
    XEvent	    event;
    static Boolean  actions_added=False;

    XTranslateCoordinates(tool_d, main_canvas, XDefaultRootWindow(tool_d),
			  150, 200, &xposn, &yposn, &win);
    FirstArg(XtNallowShellResize, True);
    NextArg(XtNx, xposn);
    NextArg(XtNy, yposn);
    NextArg(XtNborderWidth, POPUP_BW);
    NextArg(XtNtitle, "Xfig: Query");
    NextArg(XtNcolormap, tool_cm);
    query_popup = XtCreatePopupShell("query_popup", transientShellWidgetClass,
				     tool, Args, ArgCount);
    XtOverrideTranslations(query_popup,
                       XtParseTranslationTable(query_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, query_actions, XtNumber(query_actions));
	actions_added = True;
    }

    FirstArg(XtNdefaultDistance, 10);
    query_form = XtCreateManagedWidget("query_form", formWidgetClass,
				       query_popup, Args, ArgCount);

    FirstArg(XtNfont, bold_font);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, message);
    query_message = XtCreateManagedWidget("message", labelWidgetClass,
					  query_form, Args, ArgCount);

    FirstArg(XtNheight, 25);
    NextArg(XtNvertDistance, 15);
    NextArg(XtNfromVert, query_message);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNhorizDistance, 55);

    if (query_type == QUERY_ALLPARTCAN) {
	NextArg(XtNlabel, "Save All ");
	query_all = XtCreateManagedWidget("all", commandWidgetClass,
				      query_form, Args, ArgCount);
	XtAddCallback(query_all, XtNcallback, accept_all, (XtPointer) NULL);
	ArgCount = 4;
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNlabel, "Save Part");
	NextArg(XtNfromHoriz, query_all);
	query_part = XtCreateManagedWidget("part", commandWidgetClass,
					 query_form, Args, ArgCount);
	XtAddCallback(query_part, XtNcallback, accept_part, (XtPointer) NULL);
	/* setup for the cancel button */
	ArgCount = 5;
	NextArg(XtNfromHoriz, query_part);

    } else if (query_type == QUERY_OK) {
	/* just OK button */
	NextArg(XtNlabel, " Ok  ");
	query_yes = XtCreateManagedWidget("ok", commandWidgetClass,
				query_form, Args, ArgCount);
	/* just use the accept_yes callback because the caller only gets one result anyway */
	XtAddCallback(query_yes, XtNcallback, accept_yes, (XtPointer) NULL);

    } else {
	/* yes/no or yes/no/cancel */

	NextArg(XtNlabel, " Yes  ");
	query_yes = XtCreateManagedWidget("yes", commandWidgetClass,
				query_form, Args, ArgCount);
	XtAddCallback(query_yes, XtNcallback, accept_yes, (XtPointer) NULL);

	if (query_type == QUERY_YESNO || query_type == QUERY_YESNOCAN) {
	    ArgCount = 4;
	    NextArg(XtNhorizDistance, 25);
	    NextArg(XtNlabel, "  No  ");
	    NextArg(XtNfromHoriz, query_yes);
	    query_no = XtCreateManagedWidget("no", commandWidgetClass,
				query_form, Args, ArgCount);
	    XtAddCallback(query_no, XtNcallback, accept_no, (XtPointer) NULL);

	    /* setup for the cancel button */
	    ArgCount = 5;
	    NextArg(XtNfromHoriz, query_no);
	} else {
	    /* setup for the cancel button */
	    ArgCount = 4;
	    NextArg(XtNhorizDistance, 25);
	    NextArg(XtNfromHoriz, query_yes);
	}
    }

    if (query_type == QUERY_YESCAN || query_type == QUERY_YESNOCAN ||
	query_type == QUERY_ALLPARTCAN) {
	    NextArg(XtNlabel, "Cancel");
	    query_cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
						query_form, Args, ArgCount);
	    XtAddCallback(query_cancel, XtNcallback, accept_cancel, (XtPointer) NULL);
    }

    XtPopup(query_popup, XtGrabExclusive);
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(query_popup));
    (void) XSetWMProtocols(tool_d, XtWindow(query_popup), &wm_delete_window, 1);
    XDefineCursor(tool_d, XtWindow(query_popup), arrow_cursor);

    query_done = 0;
    while (!query_done) {
	/* pass events */
	XNextEvent(tool_d, &event);
	XtDispatchEvent(&event);
    }

    XtPopdown(query_popup);
    XtDestroyWidget(query_popup);

    return (query_result);
}

/*
 * make a pulldown menu with "nent" button entries (labels) that call "callback"
 * when pressed.
 * Additionally, make a dividing line in the menu at the position "divide_line"
 * (use -1 if no line desired)
 */

#include "SmeCascade.h"

#include "d_text.h"
#include "e_placelib.h"
#include "w_rulers.h"

Widget
make_pulldown_menu(char **entries, Cardinal nent, int divide_line, char *divide_message, Widget parent, XtCallbackProc callback)
{
    Widget	    pulldown_menu, entry;
    int		    i;

    pulldown_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, parent,
				  NULL, ZERO);

    for (i = 0; i < nent; i++) {
	if (i == divide_line) {
	    (void) XtCreateManagedWidget(entries[i], smeLineObjectClass, pulldown_menu, 
					NULL, ZERO);
	    FirstArg(XtNlabel, divide_message);
	    (void) XtCreateManagedWidget("menu_divider", smeBSBObjectClass, pulldown_menu, 
					Args, ArgCount);
	    (void) XtCreateManagedWidget(entries[i], smeLineObjectClass, pulldown_menu, 
					NULL, ZERO);
	}
	entry = XtCreateManagedWidget(entries[i], smeBSBObjectClass, pulldown_menu, 
					NULL, ZERO);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }
    return pulldown_menu;
}

static void
CvtStringToFloat(XrmValuePtr args, Cardinal *num_args, XrmValuePtr fromVal, XrmValuePtr toVal)
{
    static float    f;

    if (*num_args != 0)
	XtWarning("String to Float conversion needs no extra arguments");
    if (sscanf((char *) fromVal->addr, "%f", &f) == 1) {
	(*toVal).size = sizeof(float);
	(*toVal).addr = (caddr_t) & f;
    } else
	XtStringConversionWarning((char *) fromVal->addr, "Float");
}

static void
CvtIntToFloat(XrmValuePtr args, Cardinal *num_args, XrmValuePtr fromVal, XrmValuePtr toVal)
{
    static float    f;

    if (*num_args != 0)
	XtWarning("Int to Float conversion needs no extra arguments");
    f = *(int *) fromVal->addr;
    (*toVal).size = sizeof(float);
    (*toVal).addr = (caddr_t) & f;
}

void fix_converters(void)
{
    XtAppAddConverter(tool_app, "String", "Float", CvtStringToFloat, NULL, 0);
    XtAppAddConverter(tool_app, "Int", "Float", CvtIntToFloat, NULL, 0);
}


static void
cancel_color(Widget w, XtPointer widget, XtPointer dum1)
{
    XtPopdown((Widget)widget);
}

Widget
make_color_popup_menu(Widget parent, char *name, XtCallbackProc callback, Boolean include_transp, Boolean include_backg)
{
    Widget	    pop_menu, pop_form, color_box;
    Widget	    viewp, entry, label;
    int		    i;
    char	    buf[30];
    Position	    x_val, y_val;
    Dimension	    height;
    int		    wd, ht;
    Pixel	    bgcolor;
    Boolean	    first;

    /* position selection panel just below button */
    FirstArg(XtNheight, &height);
    GetValues(parent);
    XtTranslateCoords(parent, (Position) 0, (Position) height,
		      &x_val, &y_val);

    /* by using the name "menu", the menuButton that is using this 
     * shell will know to pop it up, since that is the default menu name */
    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val+4);
    pop_menu = XtCreatePopupShell("menu",
			         overrideShellWidgetClass, parent,
				 Args, ArgCount);

    /* outer box containing label, color box, and cancel button */
    FirstArg(XtNorientation, XtorientVertical);
    pop_form = XtCreateManagedWidget("color_menu_form", formWidgetClass,
    				    pop_menu, Args, ArgCount);

    /* get the background color of the form for the "Background" button */
    FirstArg(XtNbackground, &bgcolor);
    GetValues(pop_form);

    FirstArg(XtNlabel, name);
    label = XtCreateManagedWidget("color_menu_label", labelWidgetClass,
    				    pop_form, Args, ArgCount);

    /* put the Background, None and Default outside the box/viewport */
    first = True;
    for (i=(include_transp? TRANSP_BACKGROUND: 
			    include_backg? COLOR_NONE: DEFAULT); i<0; i++) {
	set_color_name(i,buf);
	FirstArg(XtNwidth, COLOR_BUT_WID);
	NextArg(XtNborderWidth, COLOR_BUT_BD_WID);
	if (!first) {
	    NextArg(XtNfromHoriz, entry);
	}
	first = False;
	NextArg(XtNfromVert, label);
	if (i==TRANSP_BACKGROUND) {
	    /* make its background transparent by using color of parent */
	    NextArg(XtNforeground, black_color.pixel);
	    NextArg(XtNbackground, bgcolor);
	    /* and it is a little wider due to the longer name */
	    NextArg(XtNwidth, COLOR_BUT_WID+14);
	} else if (i==TRANSP_NONE) {
	    NextArg(XtNforeground, black_color.pixel);
	    NextArg(XtNbackground, white_color.pixel);
	} else {
	    NextArg(XtNforeground, white_color.pixel);
	    NextArg(XtNbackground, black_color.pixel);
	}
	entry = XtCreateManagedWidget(buf, commandWidgetClass, pop_form, Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }

    /* make a scrollable viewport in case all the buttons don't fit */

    FirstArg(XtNallowVert, True);
    NextArg(XtNallowHoriz, False);
    NextArg(XtNfromVert, entry);
    if (num_usr_cols > 8) {
	/* allow for width of scrollbar */
	wd = 4*(COLOR_BUT_WID+2*COLOR_BUT_BD_WID)+25;
    } else {
	wd = 4*(COLOR_BUT_WID+2*COLOR_BUT_BD_WID);
    }
    if (num_usr_cols == 0) {
	ht = 8*(COLOR_BUT_HT+2*COLOR_BUT_BD_WID+3);
    } else {
	ht = 12*(COLOR_BUT_HT+2*COLOR_BUT_BD_WID+3);
    }
    NextArg(XtNwidth, wd);
    NextArg(XtNheight, ht);
    NextArg(XtNborderWidth, 1);
    viewp = XtCreateManagedWidget("color_viewp", viewportWidgetClass, 
    				  pop_form, Args, ArgCount);

    FirstArg(XtNorientation, XtorientVertical);
    NextArg(XtNhSpace, 0);	/* small space between entries */
    NextArg(XtNvSpace, 0);
    color_box = XtCreateManagedWidget("color_box", boxWidgetClass,
    				    viewp, Args, ArgCount);

    /* now make the buttons in the box */
    for (i = 0; i < NUM_STD_COLS+num_usr_cols; i++) {
	XColor		xcolor;
	Pixel		col;

	/* only those user-defined colors that are defined */
	if (i >= NUM_STD_COLS && colorFree[i-NUM_STD_COLS])
	    continue;
	set_color_name(i,buf);
	FirstArg(XtNwidth, COLOR_BUT_WID);
	NextArg(XtNborderWidth, COLOR_BUT_BD_WID);
	if (all_colors_available) {
	    xcolor.pixel = colors[i];
	    /* get RGB of the color to check intensity */
	    XQueryColor(tool_d, tool_cm, &xcolor);
	    /* make contrasting label */
	    if ((0.3 * xcolor.red + 0.59 * xcolor.green + 0.11 * xcolor.blue) <
			0.55 * (255 << 8))
		col = colors[WHITE];
	    else
		col = colors[BLACK];
	    NextArg(XtNforeground, col);
	    NextArg(XtNbackground, colors[i]);
	}
	entry = XtCreateManagedWidget(buf, commandWidgetClass, color_box,
				      Args, ArgCount);
	XtAddCallback(entry, XtNcallback, callback, (XtPointer) i);
    }

    /* make the cancel button */
    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, viewp);
    entry = XtCreateManagedWidget(buf, commandWidgetClass, pop_form,
				      Args, ArgCount);
    XtAddCallback(entry, XtNcallback, (XtCallbackProc) cancel_color,
    		  (XtPointer) pop_menu);

    return pop_menu;
}

void
set_color_name(int color, char *buf)
{
    if (color == TRANSP_NONE)
	sprintf(buf,"None");
    else if (color == TRANSP_BACKGROUND)
	sprintf(buf,"Background");
    else if (color == DEFAULT || (color >= 0 && color < NUM_STD_COLS))
	sprintf(buf, "%s", colorNames[color + 1].name);
    else
	sprintf(buf, "User %d", color);
}

/*
 * Set the color name in the label of widget, set its foreground to 
 * that color, and set its background to a contrasting color
 */

void
set_but_col(Widget widget, Pixel color)
{
	XColor		 xcolor;
	Pixel		 but_col;
	char		 buf[50];

	/* put the color name in the label and the color itself as the background */
	set_color_name(color, buf);
	but_col = x_color(color);
	FirstArg(XtNlabel, buf);
	NextArg(XtNbackground, but_col);  /* set color of button */
	SetValues(widget);

	/* now set foreground to contrasting color */
	xcolor.pixel = but_col;
	XQueryColor(tool_d, tool_cm, &xcolor);
	pick_contrast(xcolor, widget);
}

static void
inc_flt_spinner(Widget widget, XtPointer info, XtPointer dum)
{
    float val;
    char *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;

    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (float) atof(sval);
    val += spins->inc;
    if (val < spins->min)
	val = spins->min;
    if (val > spins->max)
	val = spins->max;
    sprintf(str,"%0.2f",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

static void
dec_flt_spinner(Widget widget, XtPointer info, XtPointer dum)
{
    float val;
    char *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;


    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (float) atof(sval);
    val -= spins->inc;
    if (val < spins->min)
	val = spins->min;
    if (val > spins->max)
	val = spins->max;
    sprintf(str,"%0.2f",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

static void
inc_int_spinner(Widget widget, XtPointer info, XtPointer dum)
{
    int     val;
    char   *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;

    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (int) atof(sval);
    val += (int) spins->inc;
    if (val < (int) spins->min)
	val = (int) spins->min;
    if (val > (int) spins->max)
	val = (int) spins->max;
    sprintf(str,"%d",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

static void
dec_int_spinner(Widget widget, XtPointer info, XtPointer dum)
{
    int     val;
    char   *sval,str[40];
    spin_struct *spins = (spin_struct*) info;
    Widget textwidg = (Widget) spins->widget;

    FirstArg(XtNstring, &sval);
    GetValues(textwidg);
    val = (int) atof(sval);
    val -= (int) spins->inc;
    if (val < (int) spins->min)
	val = (int) spins->min;
    if (val > (int) spins->max)
	val = (int) spins->max;
    sprintf(str,"%d",val);
    FirstArg(XtNstring, str);
    SetValues(textwidg);
    FirstArg(XtNinsertPosition, strlen(str));
    SetValues(textwidg);
}

/***********************************************************************/
/* Code to handle automatic spinning when user holds down mouse button */
/***********************************************************************/

static XtTimerCallbackProc auto_spin(XtPointer client_data, XtIntervalId *id);
static XtIntervalId	   auto_spinid;
static XtEventHandler	   stop_spin_timer(int widget, int data, int event);
static Widget		   cur_spin = (Widget) 0;

XtEventHandler
start_spin_timer(Widget widget, XtPointer data, XEvent event)
{
    auto_spinid = XtAppAddTimeOut(tool_app, appres.spinner_delay,
				(XtTimerCallbackProc) auto_spin, (XtPointer) NULL);
    /* add event to cancel timer when user releases button */
    XtAddEventHandler(widget, ButtonReleaseMask, False,
			  (XtEventHandler) stop_spin_timer, (XtPointer) NULL);
    /* keep track of which one the user is pressing */
    cur_spin = widget;

    return;
}

static XtEventHandler
stop_spin_timer(int widget, int data, int event)
{
    XtRemoveTimeOut(auto_spinid);

    return;
}

static	XtTimerCallbackProc
auto_spin(XtPointer client_data, XtIntervalId *id)
{
    auto_spinid = XtAppAddTimeOut(tool_app, appres.spinner_rate,
				(XtTimerCallbackProc) auto_spin, (XtPointer) NULL);
    /* call the proper spinup/down routine */
    XtCallCallbacks(cur_spin, XtNcallback, 0);

    return;
}

/***************************/
/* make an integer spinner */
/***************************/

Widget
MakeIntSpinnerEntry(Widget parent, Widget *text, char *name, Widget below, Widget beside, XtCallbackProc callback, char *string, int min, int max, int inc, int width)
{
    return MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, I_IVAL, (float) min, (float) max, (float) inc, width);
}

/*********************************/
/* make a floating point spinner */
/*********************************/

Widget
MakeFloatSpinnerEntry(Widget parent, Widget *text, char *name, Widget below, Widget beside, XtCallbackProc callback, char *string, float min, float max, float inc, int width)
{
    return MakeSpinnerEntry(parent, text, name, below, beside, callback,
			string, I_FVAL, min, max, inc, width);
}

/*****************************************************************************************
   Make a "spinner" entry widget - a widget with an asciiTextWidget and
   two spinners, an up and down spinner to increase and decrease the value.
   Call with:	parent	- (Widget)  parent widget
		text	- (Widget*) text widget ID is stored here (RETURN)
		name    - (char*)   name for text widget
		below	- (Widget)  widget below which this one goes
		beside	- (Widget)  widget beside which this one goes
		callback - (XtCallbackProc) callback for the text widget and
						spinners (0 if none) 
						Callback is passed spin_struct which
						has min, max
		string	- (char*)   initial string for text widget
		type	- (int)     entry type (I_FVAL = float, I_IVAL = int)
		min	- (float)   minimum value it is allowed to have
		max	- (float)   maximum value it is allowed to have
		inc	- (float)   increment/decrement value for each press of arrow
		width	- (int)	    width (pixels) of the text widget part

   Return value is outer box widget which may be used for positioning subsequent widgets.
*****************************************************************************************/

static Widget
MakeSpinnerEntry(Widget parent, Widget *text, char *name, Widget below, Widget beside, XtCallbackProc callback, char *string, int type, float min, float max, float inc, int width)
{
    Widget	 inform, outform, spinup, spindown, source;
    spin_struct *spinstruct;
    Pixel	 bgcolor;

    if ((spinstruct = (spin_struct *) malloc(sizeof(spin_struct))) == NULL) {
	file_msg("Can't allocate space for spinner");
	return 0;
    }

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    outform = XtCreateManagedWidget("spinner_form", formWidgetClass, parent, Args, ArgCount);

    /* first the ascii widget to the left of the spinner controls */
    FirstArg(XtNstring, string);
    NextArg(XtNwidth, width);
    NextArg(XtNleftMargin, 4);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNinsertPosition, strlen(string));
    NextArg(XtNhorizDistance, 1);
    NextArg(XtNvertDistance, 1);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, 0);
    *text = XtCreateManagedWidget(name, asciiTextWidgetClass,
                                              outform, Args, ArgCount);
    /* get id of text source widget */
    FirstArg(XtNtextSource, &source);
    GetValues(*text);

    /* install callback to validate data that user types in */
    if (type == I_IVAL)
	XtAddCallback(source, XtNcallback, validate_int, (XtPointer) spinstruct);

    /* add any callback the user wants */
    if (callback) {
	XtAddCallback(source, XtNcallback, callback, (XtPointer) spinstruct);
    }

    XtOverrideTranslations(*text, XtParseTranslationTable(text_translations));

    /* setup the data structure that gets passed to the callback */
    spinstruct->widget = *text;
    spinstruct->min = min;
    spinstruct->max = max;
    spinstruct->inc = inc;

    /* now the spinners */

    /* get the background color of the text widget and make that the
       background of the spinners */
    FirstArg(XtNbackground, &bgcolor);
    GetValues(*text);

    /* give the main frame the same background so the little part under
       the bottom spinner will blend with it */
    FirstArg(XtNbackground, bgcolor);
    SetValues(outform);

    /* make the spinner bitmaps if we haven't already */
    if (spinup_bm == 0 || spindown_bm == 0) {
	spinup_bm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) spinup_bits, spinup_width, spinup_height,
		    x_color(BLACK), bgcolor, tool_dpth);
	spindown_bm = XCreatePixmapFromBitmapData(tool_d, XtWindow(ind_panel),
		    (char *) spindown_bits, spindown_width, spindown_height,
		    x_color(BLACK), bgcolor, tool_dpth);
    }

    /* a form to put them in */

    FirstArg(XtNfromHoriz, *text);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNbackground, bgcolor);
    inform = XtCreateManagedWidget("spinner_frame", formWidgetClass, outform, Args, ArgCount);

    /* then the up spinner */

    FirstArg(XtNbitmap, spinup_bm);
    NextArg(XtNbackground, bgcolor);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtRubber);
    NextArg(XtNdefaultDistance, 0);
    spinup = XtCreateManagedWidget("spinup", commandWidgetClass, inform, Args, ArgCount);
    XtAddCallback(spinup, XtNcallback, 
		(XtCallbackProc) (type==I_IVAL? inc_int_spinner: inc_flt_spinner),
		(XtPointer) spinstruct);
    /* add event to start timer when user presses spinner */
    /* if he keeps pressing, it will count automatically */
    XtAddEventHandler(spinup, ButtonPressMask, False,
			  (XtEventHandler) start_spin_timer, (XtPointer) NULL);
    /* add any user validation callback */
    if (callback) {
	XtAddCallback(spinup, XtNcallback, callback, (XtPointer) spinstruct);
    }

    /* finally the down spinner */

    FirstArg(XtNbitmap, spindown_bm);
    NextArg(XtNbackground, bgcolor);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNinternalWidth, 0);
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNfromVert, spinup);
    NextArg(XtNhorizDistance, 0);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNtop, XtRubber);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNdefaultDistance, 0);
    spindown = XtCreateManagedWidget("spindown", commandWidgetClass, inform, Args, ArgCount);
    XtAddCallback(spindown, XtNcallback, 
		(XtCallbackProc) (type==I_IVAL? dec_int_spinner: dec_flt_spinner),
		(XtPointer) spinstruct);
    /* add event to start timer when user presses spinner */
    /* if he keeps pressing, it will count automatically */
    XtAddEventHandler(spindown, ButtonPressMask, False,
			  (XtEventHandler) start_spin_timer, (XtPointer) NULL);
    /* add any user validation callback */
    if (callback) {
	XtAddCallback(spindown, XtNcallback, callback, (XtPointer) spinstruct);
    }
    return outform;
}

/* validate the integer spinner as user types */

void
validate_int(Widget w, XtPointer info, XtPointer dum)
{
    DeclareArgs(4);
    spin_struct *spins = (spin_struct*) info;
    char	buf[200];
    int		val, i, pos;

    /* save cursor position */
    FirstArg(XtNinsertPosition, &pos);
    GetValues(spins->widget);

    buf[sizeof(buf)-1]='\0';
    strncpy(buf,panel_get_value(spins->widget),sizeof(buf));
    for (i=0; i<strlen(buf); )
	/* delete any non-digits (including leading "-" when min >= 0 */
	if ((spins->min >= 0.0 && buf[i] == '-') || ((buf[i] < '0' || buf[i] > '9') && buf[i] != '-') || 
			(i != 0 && buf[i] == '-')) {
	    strcpy(&buf[i],&buf[i+1]);
	    /* adjust cursor for char we just removed */
	    pos--;
	} else {
	    i++;
	}

    if (strlen(buf) > 0 && !(strlen(buf)==1 && buf[0] == '-')) {
	val = atoi(buf);
	/* only check max.  If min is, say 3 and user wants to type 10, the 1 is too small */
	if (val > (int) spins->max)
	    val = (int) spins->max;
	sprintf(buf,"%d", val);
    }
    panel_set_value(spins->widget, buf);
    /* put cursor back */
    if (pos < strlen(buf)) {
	FirstArg(XtNinsertPosition, pos+1);
	SetValues(spins->widget);
    }
}

/* handle the wheelmouse wheel */

void
spinner_up_down(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params)
{
  w = XtParent(w);
  if (params[0][0] == '+') w = XtNameToWidget(w, "*spinup");
  else w = XtNameToWidget(w, "*spindown");
  if (w == None) {
    fprintf(stderr, "spinner_up_down() is called for wrong widget\n");
  } else {
    ev->window = XtWindow(w);
    XSendEvent(ev->display, ev->window, TRUE, ButtonPressMask, (XEvent *)ev);
  }
}


/* if the file message window is up add it to the grab */
/* in this way a user may dismiss the file_msg panel if another panel is up */

void file_msg_add_grab(void)
{
    if (file_msg_popup)
	XtAddGrab(file_msg_popup, False, False);
}

/* get the pointer relative to the window it is in */

void
get_pointer_win_xy(int *xposn, int *yposn)
{ 
    Window	   win;
    int		   cx, cy;

    get_pointer_root_xy(&cx, &cy);
    XTranslateCoordinates(tool_d, XDefaultRootWindow(tool_d), main_canvas,
			  cx, cy, xposn, yposn, &win);
}

/* get the pointer relative to the root */

void
get_pointer_root_xy(int *xposn, int *yposn)
{ 
    Window	   rw, cw;
    int		   cx, cy;
    unsigned int   mask;

    XQueryPointer(tool_d, XtWindow(tool),
	&rw, &cw, xposn, yposn, &cx, &cy, &mask);
}

/* give error message if action_on is true.  This means we are trying
   to switch modes in the middle of some drawing/editing operation */

Boolean
check_action_on(void)
{
    /* zooming is ok */
    if (action_on && cur_mode != F_ZOOM) {
	if (cur_mode == F_TEXT)
	    finish_text_input(0,0,0);/* finish up any text input */
	else {
	    if (cur_mode == F_PLACE_LIB_OBJ)
		cancel_place_lib_obj(0, 0, 0);	      
	    else {
		put_msg("Finish (or cancel) the current operation before changing modes");
		beep();
		return True;
	    }
	}
    }
    return False;
}

/* process any (single) outstanding Xt event to allow things like button pushes */

void process_pending(void)
{
    while (XtAppPending(tool_app))
	XtAppProcessEvent(tool_app, XtIMAll);
    app_flush();
}

Boolean	user_colors_saved = False;
XColor	saved_user_colors[MAX_USR_COLS];
Boolean	saved_userFree[MAX_USR_COLS];
int	saved_user_num;

Boolean	nuser_colors_saved = False;
XColor	saved_nuser_colors[MAX_USR_COLS];
Boolean	saved_nuserFree[MAX_USR_COLS];
int	saved_nuser_num;

/* save user colors into temp vars */

void save_user_colors(void)
{
    int		i;

    if (appres.DEBUG)
	fprintf(stderr,"** Saving user colors. Before: user_colors_saved = %d\n",
		user_colors_saved);

    user_colors_saved = True;

    /* first save the current colors because del_color_cell destroys them */
    for (i=0; i<num_usr_cols; i++)
	saved_user_colors[i] = user_colors[i];
    /* and save Free entries */
    for (i=0; i<num_usr_cols; i++)
	saved_userFree[i] = colorFree[i];
    /* now free any previously defined user colors */
    for (i=0; i<num_usr_cols; i++) {
	    del_color_cell(i);		/* remove widget and colormap entry */
    }
    saved_user_num = num_usr_cols;
}

/* save n_user colors into temp vars */

void save_nuser_colors(void)
{
    int		i;

    if (appres.DEBUG)
	fprintf(stderr,"** Saving n_user colors. Before: nuser_colors_saved = %d\n",
		user_colors_saved);

    nuser_colors_saved = True;

    /* first save the current colors because del_color_cell destroys them */
    for (i=0; i<n_num_usr_cols; i++)
	saved_nuser_colors[i] = n_user_colors[i];
    /* and save Free entries */
    for (i=0; i<n_num_usr_cols; i++)
	saved_nuserFree[i] = n_colorFree[i];
    saved_nuser_num = n_num_usr_cols;
}

/* restore user colors from temp vars */

void restore_user_colors(void)
{
    int		i,num;

    if (!user_colors_saved)
	return;

    if (appres.DEBUG)
	fprintf(stderr,"** Restoring user colors. Before: user_colors_saved = %d\n",
		user_colors_saved);

    user_colors_saved = False;

    /* first free any previously defined user colors */
    for (i=0; i<num_usr_cols; i++) {
	    del_color_cell(i);		/* remove widget and colormap entry */
    }

    num_usr_cols = saved_user_num;

    /* now restore the orig user colors */
    for (i=0; i<num_usr_cols; i++)
	 user_colors[i] = saved_user_colors[i];
    /* and Free entries */
    for (i=0; i<num_usr_cols; i++)
	colorFree[i] = saved_userFree[i];

    /* now try to allocate those colors */
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

/* Restore user colors from temp vars into n_user_...  */

void restore_nuser_colors(void)
{
    int		i;

    if (!nuser_colors_saved)
	return;

    if (appres.DEBUG)
	fprintf(stderr,"** Restoring user colors into n_...\n");

    nuser_colors_saved = False;

    n_num_usr_cols = saved_nuser_num;

    /* now restore the orig user colors */
    for (i=0; i<n_num_usr_cols; i++)
	 n_user_colors[i] = saved_nuser_colors[i];
    /* and Free entries */
    for (i=0; i<n_num_usr_cols; i++)
	n_colorFree[i] = saved_nuserFree[i];
}

/* create some global bitmaps like menu arrows, checkmarks, etc. */

void create_bitmaps(void)
{
	int	       i;

	/* make a down-arrow for any pull-down menu buttons */
	menu_arrow = XCreateBitmapFromData(tool_d, tool_w, 
			(char *) menu_arrow_bits, menu_arrow_width, menu_arrow_height);
	/* make a right-arrow for any cascade menu entries */
	menu_cascade_arrow = XCreateBitmapFromData(tool_d, tool_w, 
			(char *) menu_cascade_arrow_bits, 
			menu_cascade_arrow_width, menu_cascade_arrow_height);
	/* make pixmap for red checkmark */
	check_pm = XCreatePixmapFromBitmapData(tool_d, tool_w,
		    (char *) check_bits, check_width, check_height,
		    colors[RED], colors[WHITE], tool_dpth);
	/* and make one same size but all white */
	null_check_pm = XCreatePixmapFromBitmapData(tool_d, tool_w,
		    (char *) check_bits, check_width, check_height,
		    colors[WHITE], colors[WHITE], tool_dpth);
	/* and one for a smaller checkmark */
	sm_check_pm = XCreatePixmapFromBitmapData(tool_d, tool_w,
		    (char *) sm_check_bits, sm_check_width, sm_check_height,
		    colors[RED], colors[WHITE], tool_dpth);
	/* and make one same size but all white */
	sm_null_check_pm = XCreatePixmapFromBitmapData(tool_d, tool_w,
		    (char *) sm_check_bits, sm_check_width, sm_check_height,
		    colors[WHITE], colors[WHITE], tool_dpth);
	/* create two bitmaps to show on/off state */
	balloons_on_bitmap = XCreateBitmapFromData(tool_d, tool_w, 
				 (char *) balloons_on_bits,
				 balloons_on_width, balloons_on_height);
	balloons_off_bitmap = XCreateBitmapFromData(tool_d, tool_w, 
				 (char *) balloons_off_bits,
				 balloons_off_width, balloons_off_height);
	/* create the 1-plane bitmaps of the arrow images */
	/* these will go in the "left bitmap" part of the menu */
	/* they are used in e_edit.c and w_indpanel.c */
	for (i =- 1; i < NUM_ARROW_TYPES-1; i++) {
	    arrow_pixmaps[i+1] = XCreateBitmapFromData(tool_d,canvas_win,
				(i==-1? (char*) no_arrow_bits: (char*) arrowtype_choices[i].icon->bits),
				32, 32); 
	}
	diamond_pixmap = XCreateBitmapFromData(tool_d, canvas_win, 
			(char*) diamond_bits, diamond_width, diamond_height); 
	if (linestyle_pixmaps[0] == 0) {
	    for (i=0; i<NUM_LINESTYLE_TYPES; i++)
		linestyle_pixmaps[i] = XCreateBitmapFromData(tool_d,canvas_win,
			(char*) linestyle_choices[i].icon->bits, 
			linestyle_choices[i].icon->width, linestyle_choices[i].icon->height); 
	}

	/* create the bitmaps that look like mouse buttons pressed */
	mouse_l = XCreateBitmapFromData(tool_d, tool_w, 
		(char *) mouse_l_bits, mouse_l_width, mouse_l_height);
	mouse_r = XCreateBitmapFromData(tool_d, tool_w, 
		(char *) mouse_r_bits, mouse_r_width, mouse_r_height);
}

/* put a string into an ASCII text widget */

void
panel_set_value(Widget widg, char *val)
{
    FirstArg(XtNstring, val);
    SetValues(widg);
    /* this must be done separately from setting the string */
    FirstArg(XtNinsertPosition, strlen(val));
    SetValues(widg);
}

/* get a string from an ASCII text widget */

char *
panel_get_value(Widget widg)
{
    char	   *val;

    FirstArg(XtNstring, &val);
    GetValues(widg);
    return val;
}

/* put an int into an ASCII text widget */

void
panel_set_int(Widget widg, int intval)
{
    char	    buf[80];
    sprintf(buf, "%d", intval);
    panel_set_value(widg, buf);
}

/* put a float into an ASCII text widget */

void
panel_set_float(Widget widg, float floatval, char *format)
{
    char	    buf[80];
    sprintf(buf, format, floatval);
    panel_set_value(widg, buf);
}

/* create a checkbutton with a labelled area to the right */

Widget
CreateCheckbutton(char *label, char *widget_name, Widget parent, Widget below, Widget beside, Boolean manage, Boolean large, Boolean *value, XtCallbackProc user_callback, Widget *togwidg)
{
	DeclareArgs(20);
	Widget   form, toggle, labelw;
	unsigned int check_ht, udum;
	int	 idum;
	Window	 root;

	FirstArg(XtNdefaultDistance, 1);
	NextArg(XtNresizable, True);
	if (below != NULL)
	    NextArg(XtNfromVert, below);
	if (beside != NULL)
	    NextArg(XtNfromHoriz, beside);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNborderWidth, 0);
	form = XtCreateWidget(widget_name, formWidgetClass, parent, Args, ArgCount);

	FirstArg(XtNradioData, 1);
	if (*value) {
	    if (large) {
		NextArg(XtNbitmap, check_pm);
	    } else {
		NextArg(XtNbitmap, sm_check_pm);
	    }
	} else {
	    if (large) {
		NextArg(XtNbitmap, null_check_pm);
	    } else {
		NextArg(XtNbitmap, sm_null_check_pm);
	    }
	}
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	toggle = XtCreateManagedWidget("toggle", toggleWidgetClass,
					form, Args, ArgCount);
	/* user wants widget ID */
	if (togwidg)
	    *togwidg = toggle;
	XtAddCallback(toggle, XtNcallback, (XtCallbackProc) toggle_checkbutton, 
				(XtPointer) value);
	if (user_callback)
	    XtAddCallback(toggle, XtNcallback, (XtCallbackProc) user_callback,
			(XtPointer) value);

	/* get the height of the checkmark pixmap */
	(void) XGetGeometry(tool_d, (large?check_pm:sm_check_pm), &root, 
			&idum, &idum, &udum, &check_ht, &udum, &udum);

	FirstArg(XtNlabel, label);
	NextArg(XtNheight, check_ht+4);	/* make label as tall as the check mark */
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, toggle);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	/* for small checkmarks, leave less room around label */
	if (!large) {
	    NextArg(XtNinternalWidth, 2);
	    NextArg(XtNinternalHeight, 1);
	}
	labelw = XtCreateManagedWidget("label", labelWidgetClass,
					form, Args, ArgCount);
	if (manage)
	    XtManageChild(form);
	return form;
}

XtCallbackProc
toggle_checkbutton(Widget w, XtPointer data, XtPointer garbage)
{
    DeclareArgs(5);
    Pixmap	   pm;
    Boolean	  *what = (Boolean *) data;
    Boolean	   large;

    *what = !*what;
    /* first find out what size pixmap we are using */
    FirstArg(XtNbitmap, &pm);
    GetValues(w);

    large = False;
    if (pm == check_pm || pm == null_check_pm)
	large = True;

    if (*what) {
	if (large) {
	    FirstArg(XtNbitmap, check_pm);
	} else {
	    FirstArg(XtNbitmap, sm_check_pm);
	}
	/* make button depressed */
	NextArg(XtNstate, False);
    } else {
	if (large) {
	    FirstArg(XtNbitmap, null_check_pm);
	} else {
	    FirstArg(XtNbitmap, sm_null_check_pm);
	}
	/* make button raised */
	NextArg(XtNstate, True);
    }
    SetValues(w);

    return;
}

/* assemble main window title bar with xfig title and (base) file name */

void
update_wm_title(char *name)
{
    char  wm_title[200];
    DeclareArgs(2);

    if (strlen(name)==0) sprintf(wm_title, "%s - No file", tool_name);
    else sprintf(wm_title, "Xfig - %s", xf_basename(name));
    FirstArg(XtNtitle, wm_title);
    SetValues(tool);
}

void
check_for_resize(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams)
{
    int		    dx, dy;
    XConfigureEvent *xc = (XConfigureEvent *) event;

    if (xc->width == TOOL_WD && xc->height == TOOL_HT)
	return;		/* no size change */
    dx = xc->width - TOOL_WD;
    dy = xc->height - TOOL_HT;
    TOOL_WD = xc->width;
    TOOL_HT = xc->height;
    resize_all(CANVAS_WD + dx, CANVAS_HT + dy);
#ifdef I18N
    if (xim_ic != NULL)
      xim_set_ic_geometry(xim_ic, CANVAS_WD, CANVAS_HT);
#endif /* I18N */
}

/* resize whole shebang given new canvas size (width,height) */

void resize_all(int width, int height)
{
    Dimension	b, b2, b3, w, h, h2;
    int		min_sw_per_row;
    DeclareArgs(10);

    setup_sizes(width, height);

    FirstArg(XtNheight, &h);
    NextArg(XtNborderWidth, &b);
    GetValues(mode_panel);
    FirstArg(XtNheight, &h2);
    NextArg(XtNborderWidth, &b2);
    GetValues(topruler_sw);
    FirstArg(XtNborderWidth, &b3);
    GetValues(canvas_sw);
    h = h+2*b;
    h2 = CANVAS_HT+2*b3+h2+2*b2 + 2;
    /* if mode panel is taller than the canvas + topruler height, increase 
       but_per_row and decrease canvas width */
    /* but don't change it if user explicitely used -but_per_row */
    if (appres.but_per_row == 0) {
	if (h > h2) {
	    min_sw_per_row = (MODE_SW_HT + INTERNAL_BW*2) * (NUM_MODE_SW+2) / h2 + 1;
	    if (min_sw_per_row != SW_PER_ROW) 
		width -= (min_sw_per_row - SW_PER_ROW)*(MODE_SW_WD+2*INTERNAL_BW);
	    SW_PER_ROW = max2(min_sw_per_row, SW_PER_ROW);
	    setup_sizes(width, height);
	    XtUnmanageChild(mode_panel);
	    FirstArg(XtNwidth, MODEPANEL_WD);
	    NextArg(XtNresizable, True);
	    SetValues(mode_panel);
	    /* now resize width of labels "Drawing" and "Editing" */
	    FirstArg(XtNwidth, MODE_SW_WD * SW_PER_ROW + INTERNAL_BW * (SW_PER_ROW - 1));
	    SetValues(d_label);
	    SetValues(e_label);
	    FirstArg(XtNwidth, MODEPANEL_WD);
	    NextArg(XtNresizable, False);
	    SetValues(mode_panel);
	    XtManageChild(mode_panel);
	} else {
	    /* check if we can decrease number of buttons per row (user made tool taller) */
	    /* do this by checking the height of the "Drawing" label */
	    FirstArg(XtNheight, &h);
	    GetValues(d_label);
	    min_sw_per_row = (MODE_SW_HT + INTERNAL_BW*2) * (NUM_MODE_SW+2) / h2 + 1;
	    if (min_sw_per_row < SW_PER_ROW) {
		width -= (min_sw_per_row - SW_PER_ROW)*(MODE_SW_WD+2*INTERNAL_BW);
		SW_PER_ROW = min_sw_per_row;
		setup_sizes(width, height);
		XtUnmanageChild(mode_panel);
		FirstArg(XtNwidth, MODEPANEL_WD);
		NextArg(XtNresizable, True);
		SetValues(mode_panel);
		/* now resize width of labels "Drawing" and "Editing" */
		FirstArg(XtNwidth, MODE_SW_WD * SW_PER_ROW + INTERNAL_BW * (SW_PER_ROW - 1));
		SetValues(d_label);
		SetValues(e_label);
		FirstArg(XtNwidth, MODEPANEL_WD);
		NextArg(XtNresizable, False);
		SetValues(mode_panel);
		XtManageChild(mode_panel);
	    }
	} /* if (h > h2) */
    } /* appres.but_per_row == 0 */

    XawFormDoLayout(tool_form, False);
    ignore_exp_cnt++;		/* canvas is resized twice - redraw only once */

    /* first redo the top panels */
    FirstArg(XtNborderWidth, &b);
    GetValues(name_panel);
    XtResizeWidget(name_panel, NAMEPANEL_WD, CMD_BUT_HT, b);
    GetValues(msg_panel);
    XtUnmanageChild(mousefun);	/* unmanage the mouse function... */
    XtUnmanageChild(layer_form);/* and the layer form */
    XtResizeWidget(msg_panel, MSGPANEL_WD, MSGPANEL_HT, b);
    XtManageChild(mousefun);	/* so that they shift with msg_panel */

    /* now redo the center area */
    XtUnmanageChild(mode_panel);
    FirstArg(XtNheight, (MODEPANEL_SPACE + 1) / 2);
    SetValues(d_label);
    FirstArg(XtNheight, (MODEPANEL_SPACE) / 2);
    SetValues(e_label);
    XtManageChild(mode_panel);	/* so that it adjusts properly */

    FirstArg(XtNborderWidth, &b);
    GetValues(canvas_sw);
    XtResizeWidget(canvas_sw, CANVAS_WD, CANVAS_HT, b);
    GetValues(topruler_sw);
    XtResizeWidget(topruler_sw, TOPRULER_WD, TOPRULER_HT, b);
    resize_topruler();
    GetValues(sideruler_sw);
    XtResizeWidget(sideruler_sw, SIDERULER_WD, SIDERULER_HT, b);
    resize_sideruler();
    XtUnmanageChild(sideruler_sw);
    XtManageChild(sideruler_sw);/* so that it shifts with canvas */
    XtUnmanageChild(unitbox_sw);
    XtManageChild(unitbox_sw);	/* so that it shifts with canvas */

    /* and the bottom */
    XtUnmanageChild(ind_panel);
    FirstArg(XtNborderWidth, &b);
    NextArg(XtNheight, &h);
    GetValues(ind_panel);
    w = INDPANEL_WD;
    /* account for update control */
    if (XtIsManaged(upd_ctrl))
	w -= UPD_CTRL_WD-2*INTERNAL_BW;
    XtResizeWidget(ind_panel, w, h, b);
    XtManageChild(ind_panel);
    XtUnmanageChild(ind_box);
    XtManageChild(ind_box);
    XtManageChild(layer_form);

    XawFormDoLayout(tool_form, True);
}

void
check_colors(void)
{
    int		    i;
    XColor	    dum,color;

    /* no need to allocate black and white specially */
    colors[BLACK] = black_color.pixel;
    colors[WHITE] = white_color.pixel;
    /* fill the colors array with black (except for white) */
    for (i=0; i<NUM_STD_COLS; i++)
	if (i != BLACK && i != WHITE)
		colors[i] = colors[BLACK];

    /* initialize user color cells */
    for (i=0; i<MAX_USR_COLS; i++) {
	    colorFree[i] = True;
	    n_colorFree[i] = True;
	    num_usr_cols = 0;
    }

    /* if monochrome resource is set, do not even check for colors */
    if (!all_colors_available || appres.monochrome) {
	return;
    }

    for (i=0; i<NUM_STD_COLS; i++) {
	/* try to allocate another named color */
	/* first try by #xxxxx form if exists, then by name from rgb.txt file */
	if (!xallncol(colorNames[i+1].rgb,&color,&dum)) {
	     /* can't allocate it, switch colormaps try again */
	     if (!switch_colormap() || 
	        (!xallncol(colorNames[i+1].rgb,&color,&dum))) {
		    fprintf(stderr, "Not enough colormap entries available for basic colors\n");
		    fprintf(stderr, "using monochrome mode.\n");
		    all_colors_available = False;
		    return;
	    }
	}
	/* put the colorcell number in the color array */
	colors[i] = color.pixel;
    }

    /* get two grays for insensitive spinners */
    if (tool_cells == 2 || appres.monochrome) {
	/* use black to gray out an insensitive spinner */
	dark_gray_color = colors[BLACK];
	med_gray_color = colors[BLACK];
	lt_gray_color = colors[BLACK];
	/* color of page border */
	pageborder_color = colors[BLACK];
    } else {
	XColor x_color;
	/* get a dark gray for making certain spinners insensitive */
	XParseColor(tool_d, tool_cm, "gray65", &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    dark_gray_color = x_color.pixel;
	} else {
	    dark_gray_color = colors[WHITE];
	}
	/* get a medium one too */
	XParseColor(tool_d, tool_cm, "gray80", &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    med_gray_color = x_color.pixel;
	} else {
	    med_gray_color = colors[WHITE];
	}
	/* get a lighter one too */
	XParseColor(tool_d, tool_cm, "gray90", &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    lt_gray_color = x_color.pixel;
	} else {
	    lt_gray_color = colors[WHITE];
	}

	/* get page border color */
	XParseColor(tool_d, tool_cm, appres.pageborder, &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    pageborder_color = x_color.pixel;
	} else {
	    pageborder_color = colors[BLACK];
	}
	/* get axis lines color */
	XParseColor(tool_d, tool_cm, appres.axislines, &x_color);
	if (XAllocColor(tool_d, tool_cm, &x_color)) {
	    axis_lines_color = x_color.pixel;
	} else {
	    axis_lines_color = colors[BLACK];
	}
    }

}

/* useful when using ups */
void XSyncOn(void)
{
	XSynchronize(tool_d, True);
	XFlush(tool_d);
}

void XSyncOff(void)
{
	XSynchronize(tool_d, False);
	XFlush(tool_d);
}

/* 
 * This will parse the hexadecimal form of the named colors in the standard color
 * names.  Some servers can't parse the hex form for XAllocNamedColor()
 */

int
xallncol(char *name, XColor *color, XColor *exact)
{
    unsigned	short r,g,b;
    char	nam[30];

    if (*name != '#')
	return XAllocNamedColor(tool_d,tool_cm,name,color,exact);

    /* gcc doesn't allow writing on constant strings without the -fwritable_strings
       option, and apparently some versions of sscanf need to write a char back */
    strcpy(nam,name);
    if (sscanf(nam,"#%2hx%2hx%2hx",&r,&g,&b) != 3 || nam[7] != '\0') {
	fprintf(stderr,
	  "Malformed color specification %s in resources.c must be 6 hex digits",nam);
	exit(1);
    }

    color->red   = r<<8;
    color->green = g<<8;
    color->blue  = b<<8;
    color->flags = DoRed|DoGreen|DoBlue;
    *exact = *color;
    return XAllocColor(tool_d,tool_cm,color);
}

Widget
make_grid_options(Widget parent, Widget put_below, Widget put_beside, char *minor_grid_value, char *major_grid_value, Widget *grid_minor_menu_button, Widget *grid_major_menu_button, Widget *grid_minor_menu, Widget *grid_major_menu, Widget *print_grid_minor_text, Widget *print_grid_major_text, Widget *grid_unit_label, void (*grid_major_select) (/* ??? */), void (*grid_minor_select) (/* ??? */))
{
	Widget	below, beside;

	if (appres.INCHES) {
	    if (cur_gridunit == FRACT_UNIT) {
		grid_choices = grid_inch_choices;
		n_grid_choices = sizeof(grid_inch_choices) / sizeof(char *);
	    } else {
		grid_choices = grid_tenth_inch_choices;
		n_grid_choices = sizeof(grid_tenth_inch_choices) / sizeof(char *);
	    }
	} else {
	    grid_choices = grid_cm_choices;
	    n_grid_choices = sizeof(grid_cm_choices) / sizeof(char *);
	}

	FirstArg(XtNlabel, "Minor");
	NextArg(XtNfromVert, put_below);
	NextArg(XtNfromHoriz, put_beside);
	NextArg(XtNhorizDistance, 4);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = *grid_minor_menu_button = XtCreateManagedWidget("minor_grid_menu_button",
						menuButtonWidgetClass,
						parent, Args, ArgCount);
	*grid_minor_menu = make_pulldown_menu(grid_choices, n_grid_choices, -1, NULL,
				*grid_minor_menu_button, grid_minor_select);

	/* text widget for user to type in minor grid spacing */
	FirstArg(XtNstring, minor_grid_value);
	NextArg(XtNwidth, 50);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNfromVert, put_below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 1);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	*print_grid_minor_text = XtCreateManagedWidget("minor_grid_text",
						asciiTextWidgetClass,
						parent, Args, ArgCount);
        XtOverrideTranslations(*print_grid_minor_text,
                           XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "Major");
	NextArg(XtNfromVert, put_below);
	NextArg(XtNfromHoriz, *print_grid_minor_text);
	NextArg(XtNhorizDistance, 8);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = *grid_major_menu_button = XtCreateManagedWidget("major_grid_menu_button",
						menuButtonWidgetClass,
						parent, Args, ArgCount);
	*grid_major_menu = make_pulldown_menu(grid_choices, n_grid_choices, -1, NULL,
				*grid_major_menu_button, grid_major_select);

	/* text widget for user to type in major grid spacing */

	FirstArg(XtNstring, major_grid_value);
	NextArg(XtNwidth, 50);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNfromVert, put_below);
	NextArg(XtNfromHoriz, below);
	NextArg(XtNhorizDistance, 1);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	*print_grid_major_text = XtCreateManagedWidget("major_grid_text",
						asciiTextWidgetClass,
						parent, Args, ArgCount);
        XtOverrideTranslations(*print_grid_major_text,
                           XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, appres.INCHES? "inches" : "mm");
	NextArg(XtNwidth, 60);
	NextArg(XtNfromVert, put_below);
	NextArg(XtNfromHoriz, *print_grid_major_text);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	*grid_unit_label = XtCreateManagedWidget("grid_unit_label", labelWidgetClass,
					    parent, Args, ArgCount);
	
	return *grid_minor_menu_button;
}

/* force the the grid choice menu to be consistent with current IP/metric setting */
/* do this in both the print and export panels */

void
reset_grid_menus(void)
{
	float	convert;

	if (appres.INCHES) {
	    convert = 1.0;
	    /* if was metric and is now inches, convert grid values */
	    if (old_gridunit == MM_UNIT)
		convert = 0.03937;
	    /* if fraction<->decimal find nearest equiv */
	    if (print_panel) {
		/* convert major to metric */
		convert_gridstr(print_grid_major_text, convert);
		/* convert minor to metric */
		convert_gridstr(print_grid_minor_text, convert);
	    }
	    if (export_panel) {
		/* convert major to metric */
		convert_gridstr(export_grid_major_text, convert);
		/* convert minor to metric */
		convert_gridstr(export_grid_minor_text, convert);
	    }
	    if (cur_gridunit == FRACT_UNIT) {
		grid_choices = grid_inch_choices;
		n_grid_choices = num_grid_inch_choices;
	    } else {
		grid_choices = grid_tenth_inch_choices;
		n_grid_choices = num_grid_tenth_inch_choices;
	    }
	    FirstArg(XtNlabel, "inches");
	    if (print_panel)
		SetValues(print_grid_unit_label);
	    if (export_panel)
		SetValues(export_grid_unit_label);
	} else {
	    grid_choices = grid_cm_choices;
	    n_grid_choices = num_grid_cm_choices;
	    FirstArg(XtNlabel, "mm");
	    if (print_panel)
		SetValues(print_grid_unit_label);
	    if (export_panel)
		SetValues(export_grid_unit_label);
	    /* if there are any fractions in the values, change them to 10mm */
	    if (print_panel) {
		/* convert major to metric */
		convert_gridstr(print_grid_major_text, 25.4);
		/* convert minor to metric */
		convert_gridstr(print_grid_minor_text, 25.4);
	    }
	    if (export_panel) {
		/* convert major to metric */
		convert_gridstr(export_grid_major_text, 25.4);
		/* convert minor to metric */
		convert_gridstr(export_grid_minor_text, 25.4);
	    }
	}
	if (old_gridunit != cur_gridunit) {
	    if (print_panel) {
		XtDestroyWidget(print_grid_minor_menu);
		XtDestroyWidget(print_grid_major_menu);
		print_grid_minor_menu = make_pulldown_menu(grid_choices, n_grid_choices, -1, NULL,
					print_grid_minor_menu_button, print_grid_minor_select);
		print_grid_major_menu = make_pulldown_menu(grid_choices, n_grid_choices, -1, NULL,
					print_grid_major_menu_button, print_grid_major_select);
	    }
	    if (export_panel) {
		XtDestroyWidget(export_grid_minor_menu);
		XtDestroyWidget(export_grid_major_menu);
		export_grid_minor_menu = make_pulldown_menu(grid_choices,n_grid_choices,-1, NULL,
					export_grid_minor_menu_button,export_grid_minor_select);
		export_grid_major_menu = make_pulldown_menu(grid_choices,n_grid_choices,-1, NULL,
					export_grid_major_menu_button, export_grid_major_select);
	    }
	}
	old_gridunit = cur_gridunit;
}

static void
convert_gridstr(Widget widget, float mult)
{
	double	 value, numer, denom, diff;
	char	*sval, fraction[20];
	double	 fracts[] = { 2, 4, 8, 16, 32 };
	double	 tol[]    = { 0.05, 0.1, 0.2, 0.3, 0.6};
#define NUM_FRACTS sizeof(fracts)/sizeof(double)
	int	 i;

	FirstArg(XtNstring, &sval);
	GetValues(widget);
	/* don't convert anything if "none" */
	if (strcasecmp(sval, "none") == 0)
	    return;
	if (sscanf(sval,"%lf/%lf", &numer, &denom) == 2) {
	    value = numer/denom*mult;
	} else {
	    /* not fraction, just convert to metric */
	    value = numer * mult;
	}
	/* if user wants fractions, give him fractions */
	if (cur_gridunit == FRACT_UNIT) {
	    for (i=0; i<NUM_FRACTS; i++) {
		numer = round(value*fracts[i]);
		diff = fabs(value*fracts[i] - numer);
		if (diff < tol[i])
		    break;
	    }
	    if (i < NUM_FRACTS) {
		sprintf(fraction, "%d/%d", (int) numer, (int) fracts[i]);
		panel_set_value(widget, fraction);
		return;
	    }
	} else if (cur_gridunit == MM_UNIT) {
	    /* for metric, round to nearest integer mm */
	    value = round(value);
	}
	panel_set_float(widget, value, "%.3lf");
}

/************************/
/*     Splash Screen    */
/************************/

#define SPLASH_LOGO_XOFFSET	 25	/* offset from the corner of the splash to the logo */
#define SPLASH_LOGO_YOFFSET	 35
#define SPLASH_LOGO_XTEXT	325	/* offset from the corner to version text */
#define SPLASH_LOGO_YTEXT	 30
#define	STEPSIZE		  4	/* stepsize in pixel columns */

#define	COLSTEPS		   20	/* number of steps in fading splash */
#define DRAW_PAUSE		    1	/* pause between each scan column when drawing the verison number */
#define FADE_PAUSE		50000	/* pause between each shade change in fade */

/****************************************************************************
 * Do a splash screen (actually on the canvas itself)
 * We draw the xfig logo followed by "3.X.X", STEPSIZE pixels at a time.
 * Then we fade the letters to the background color and remove the icon.
 ****************************************************************************/

void splash_screen(void)
{
	GC		splash_gc;
	XColor  	col, colbg;
	Boolean 	fade;
	int		red_step, green_step, blue_step;
	Pixmap		letters_pm, spl_bckgnd, dum;
	int		splash_x, splash_y;
	int		i, x, y, width;
	unsigned long	plane_mask;
	Boolean		use_bitmap = False;
	XGCValues	gcv;

#ifdef USE_XPM
	XpmAttributes	spl_bckgnd_attr;
#endif /* USE_XPM */

	/* center the splash on the canvas */
	splash_x = (CANVAS_WD - spl_bckgnd_ic.width)/2;
	splash_y = (CANVAS_HT - spl_bckgnd_ic.height)/2;

	/* read the background for the splash screen */
#ifdef USE_XPM
	if (all_colors_available) {
	    /* use the color XPM bitmap */
	    spl_bckgnd_attr.valuemask = XpmReturnPixels;
	    spl_bckgnd_attr.colormap = tool_cm;
	    if (XpmCreatePixmapFromData(tool_d, tool_w,
				     spl_bckgnd_xpm, &spl_bckgnd,
				     &dum, &spl_bckgnd_attr) == XpmSuccess)
	        /* use color pixmap */
		use_bitmap = False;
	    else
		/* use mono */
		use_bitmap = True;
	}
#else /* no XPM */
	use_bitmap = True;
#endif /* USE_XPM */

	if (!all_colors_available || use_bitmap) {
	    /* use the xbm bitmap */
	    spl_bckgnd = XCreateBitmapFromData(tool_d, tool_w,
				(char *) spl_bckgnd_ic.bits,
				spl_bckgnd_ic.width, spl_bckgnd_ic.height);
	    /* one-bit deep pixmap */
	    use_bitmap = True;
	}

	/* if we have a color visual, fade the letters from dark color up to the background */

	/* we'll start the text at sort of a bluish-purple */
	col.flags = DoRed|DoGreen|DoBlue;
	col.red   = 0x66<<8;
	col.green = 0x55<<8;
	col.blue  = 0xaa<<8;

	/* with a lighter background (must match color in the background pixmap) */
	colbg.flags = DoRed|DoGreen|DoBlue;
	colbg.red   = 0xa7<<8;
	colbg.green = 0xa7<<8;
	colbg.blue  = 0xd6<<8;

	fade = False;
	if (all_colors_available) {
	    if (tool_vclass == GrayScale || tool_vclass == PseudoColor) {
		/* allocate a color for the background of the text pixmap */
		if (XAllocColorCells(tool_d, tool_cm, 0, &plane_mask, 0,
					&colbg.pixel, 1)==0) {
		    colbg = x_bg_color;
		} else {
		    XStoreColor(tool_d, tool_cm, &colbg);
		}
		/* allocate a colorcell that we can change to fade the text image */
		if (XAllocColorCells(tool_d, tool_cm, 0, &plane_mask, 0,
					&col.pixel, 1)==0) {
		    /* can't get a color, no fading */
		    fade = False;
		} else {
		    XStoreColor(tool_d, tool_cm, &col);
		    fade = True;
		}
	    } else if (tool_vclass == TrueColor) {
		/* for TrueColor, just allocate two colors for the text part */
		XAllocColor(tool_d, tool_cm, &col);
		XAllocColor(tool_d, tool_cm, &colbg);
	    }
	} else {
	    /* monochrome or no colors availble, draw text in fg, bg */
	    col = x_fg_color;
	    colbg = x_bg_color;
	}
	/* make our own gc */
	splash_gc = makegc(PAINT, col.pixel, colbg.pixel);

	XSetForeground(tool_d, splash_gc, col.pixel);
	/* find the step size to go from starting color to the background color */
	red_step   = (x_bg_color.red-col.red)/COLSTEPS;
	green_step = (x_bg_color.green-col.green)/COLSTEPS;
	blue_step  = (x_bg_color.blue-col.blue)/COLSTEPS;

	/* write the background on the canvas */
	if (use_bitmap) {
	    /* this is the monochrome background */
	    XCopyPlane(tool_d, spl_bckgnd, canvas_win, gccache[PAINT],
			0, 0, spl_bckgnd_ic.width, spl_bckgnd_ic.height,
			splash_x, splash_y, 1);
	} else {
	    /* this is the color background */
	    XCopyArea(tool_d, spl_bckgnd, canvas_win, splash_gc,
			0, 0, spl_bckgnd_ic.width, spl_bckgnd_ic.height,
			splash_x, splash_y);
	}

	app_flush();

	/* make the 1-plane bitmap for the version letters "3.X.X" */
	letters_pm = XCreateBitmapFromData(tool_d, tool_w,
				(char *) letters_ic.bits,
				letters_ic.width, letters_ic.height);

	/* now write the letters STEPSIZE pixel columns at a time */
	x = splash_x + SPLASH_LOGO_XTEXT;
	y = splash_y + SPLASH_LOGO_YTEXT;
	width = STEPSIZE;

	/* clip to letters to their shape so we don't write on the background */
	gcv.clip_mask = letters_pm;
	gcv.clip_x_origin = x;
	gcv.clip_y_origin = y;
	XChangeGC(tool_d, splash_gc, GCClipMask | GCClipXOrigin | GCClipYOrigin, &gcv);

	/* write bit-by-bit */
	for (i=0; i<letters_ic.width; i+=width) {
	    if (i+width > letters_ic.width)
		width = letters_ic.width - i;
	    XCopyPlane(tool_d, letters_pm, canvas_win, splash_gc,
		    i, 0, width, letters_ic.height, x+i, y, 1);
	    app_flush();
	    /* even though we're pausing only 1 microsecond (!), it seems to be enough,
	     * probably because of the system call. */
	    usleep(DRAW_PAUSE);
	}

	/* now ramp it up to the background color to fade it */
	if (fade) {
	    for (i=0; i<COLSTEPS-1; i++) {
		XStoreColor(tool_d, tool_cm, &col);
		/* change the color in the colormap */
		XSetForeground(tool_d, splash_gc, col.pixel);
		app_flush();
		usleep(FADE_PAUSE);
		col.red   += red_step;
		col.green += green_step;
		col.blue  += blue_step;
	    }
	} else {
	    /* for fading the text in TrueColor, re-write the pixmap in a changing color */
	    for (i=0; i<COLSTEPS-1; i++) {
		XAllocColor(tool_d, tool_cm, &col);
		gcv.foreground = col.pixel;
		XChangeGC(tool_d, splash_gc, GCForeground, &gcv);
		XCopyPlane(tool_d, letters_pm, canvas_win, splash_gc,
		    0, 0, letters_ic.width, letters_ic.height, x, y, 1);
		app_flush();
		usleep(FADE_PAUSE);
		col.red   += red_step;
		col.green += green_step;
		col.blue  += blue_step;
	    }
	}

	/* free up the pixmaps */
	XFreePixmap(tool_d, letters_pm);
	XFreePixmap(tool_d, spl_bckgnd);

	/* and the GC */
	XFreeGC(tool_d, splash_gc);

	/* free up the color(s) we allocated */
	XFreeColors(tool_d, tool_cm, &colbg.pixel, 1, 0);
	if (fade)
	    XFreeColors(tool_d, tool_cm, &col.pixel, 1, 0);

	/* and the colors in the xpm image */
#ifdef USE_XPM
	if (!use_bitmap)
	    XFreeColors(tool_d, tool_cm,
		spl_bckgnd_attr.pixels, spl_bckgnd_attr.npixels, 0);
#endif /* USE_XPM */
	    
	/* finally, set flag saying splash is on the screen so it will be cleared later */
	splash_onscreen = True;
}

/* clear the splash graphic if it hasn't already been cleared */
/* clear_canvas() zeroes splash_onscreen */

void
clear_splash(void)
{
    if (splash_onscreen)
	clear_canvas();
}

/*
 * Install wheel mouse scrolling for the scrollbar of the parent of the passed widget
 *
 *      viewport
 *        / \
 *   widget  scrollbar
 */

static char scroll_accel[] =
	"<Btn4Down>:	StartScroll(Backward)\n\
	<Btn5Down>:	StartScroll(Forward)\n\
	<BtnUp>:	NotifyScroll(FullLength) EndScroll()\n";

static XtAccelerators scroll_acceltable = 0;

void
InstallScroll(Widget widget)
{
	_installscroll(XtParent(widget), widget);
}

/* 
 * Install wheel mouse scrolling for the scrollbar of the parent of the parent of the passed widget
 *
 * We look to the parent of the parent for the scrollbar because we have: 
 *      viewport
 *        / \
 *     box   scrollbar
 *      |
 *    widget
 */

void
InstallScrollParent(Widget widget)
{
	_installscroll(XtParent(XtParent(widget)), widget);
}

static void
_installscroll(Widget parent, Widget widget)
{
	Widget		scroll;
	XtActionList	action_list;
	int		num_actions, i;
	Boolean		found_scroll_action;

	/* only parse the acceleration table once */
	if (scroll_acceltable == 0)
	    scroll_acceltable = XtParseAcceleratorTable(scroll_accel);

	/* install the wheel scrolling for the scrollbar of the parent onto the widget */
	scroll = XtNameToWidget(parent, "vertical");
	if (scroll) {
	    /* first see if the scrollbar supports the StartScroll action (when Xaw
	     * is compiled with ARROW_SCROLLBAR, it does not have this action */
	    found_scroll_action = False;
	    XtGetActionList(scrollbarWidgetClass, &action_list, &num_actions);
	    for (i=0; i<num_actions; i++)
		if (strcasecmp(action_list[i].string, "startscroll") == 0) {
		    found_scroll_action = True;
		    break;
		}
	    if (found_scroll_action) {
		XtOverrideTranslations(scroll, scroll_acceltable);
		FirstArg(XtNaccelerators, scroll_acceltable);
		SetValues(scroll);
		XtInstallAccelerators(widget, scroll);
	    }
	}
}
