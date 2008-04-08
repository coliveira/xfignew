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
#include "mode.h"
#include "paintop.h"
#include "u_pan.h"
#include "w_drawprim.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_rulers.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_zoom.h"
#include "object.h"

#include "f_util.h"
#include "u_redraw.h"
#include "u_scale.h"
#include "w_canvas.h"
#include "w_color.h"
#include "w_export.h"
#include "w_grid.h"
#include "w_print.h"

/*
 * The following will create rulers the same size as the initial screen size.
 * if the user resizes the xfig window, the rulers won't have numbers there.
 * Should really reset the sizes if the screen resizes.
 */

/* height of ticks for fraction of inch */
#define		INCH_MARK	8
#define		HALF_MARK	6
#define		QUARTER_MARK	4
#define		SIXTEENTH_MARK	3

#define		CM_MARK		8
#define		HALFCM_MARK	6
#define		MM_MARK		4

#define		TRM_WID		16
#define		TRM_HT		8
#define		SRM_WID		8
#define		SRM_HT		16

static int	lasty = -100, lastx = -100;
static int	troffx = -8, troffy = -10;
static int	orig_zoomoff;
static int	last_drag_x, last_drag_y;
static unsigned char	tr_marker_bits[] = {
    0xFE, 0xFF,		/* ***************  */
    0x04, 0x40,		/*  *           *  */
    0x08, 0x20,		/*   *         *  */
    0x10, 0x10,		/*    *       *  */
    0x20, 0x08,		/*     *     *  */
    0x40, 0x04,		/*      *   *  */
    0x80, 0x02,		/*       * *  */
    0x00, 0x01,		/*        *  */
};
icon_struct trm_pr = {TRM_WID, TRM_HT, (char*)tr_marker_bits};

static int	srroffx = 2, srroffy = -7;
static unsigned char	srr_marker_bits[] = {
    0x80,		/*        *  */
    0xC0,		/*       **  */
    0xA0,		/*      * *  */
    0x90,		/*     *  *  */
    0x88,		/*    *   *  */
    0x84,		/*   *    *  */
    0x82,		/*  *     *  */
    0x81,		/* *      *  */
    0x82,		/*  *     *  */
    0x84,		/*   *    *  */
    0x88,		/*    *   *  */
    0x90,		/*     *  *  */
    0xA0,		/*      * *  */
    0xC0,		/*       **  */
    0x80,		/*        *  */
    0x00
};
icon_struct srrm_pr = {SRM_WID, SRM_HT, (char*)srr_marker_bits};

static int	srloffx = -10, srloffy = -7;
static unsigned char	srl_marker_bits[] = {
    0x01,		/* *	      */
    0x03,		/* **	      */
    0x05,		/* * *	      */
    0x09,		/* *  *	      */
    0x11,		/* *   *      */
    0x21,		/* *    *     */
    0x41,		/* *     *    */
    0x81,		/* *      *   */
    0x41,		/* *     *    */
    0x21,		/* *    *     */
    0x11,		/* *   *      */
    0x09,		/* *  *	      */
    0x05,		/* * *	      */
    0x03,		/* **	      */
    0x01,		/* *	      */
    0x00
};
icon_struct srlm_pr = {SRM_WID, SRM_HT, (char*)srl_marker_bits};

static Pixmap	toparrow_pm = 0, sidearrow_pm = 0;
static Pixmap	topruler_pm = 0, sideruler_pm = 0;

DeclareStaticArgs(14);

static void	topruler_selected(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams);
static void	topruler_exposed(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams);
static void	sideruler_selected(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams);
static void	sideruler_exposed(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams);

#ifndef XAW3D1_5E
/* popup message over button when mouse enters it */
static void     unit_balloon_trigger(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch);
static void     unit_unballoon(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch);
#endif /* XAW3D1_5E */

/* turn these into macros so we can use them in
   struct initialization */
#define	HINCH (PPI / 2)
#define	QINCH (PPI / 4)
#define	SINCH (PPI / 16)
#define	TINCH (PPI / 10)
#define	HALFCM (PPCM / 2)
#define	TWOMM (PPCM / 5)
#define	ONEMM (PPCM / 10)

/* Strings and ticks on the rulers */
typedef struct tick_info_struct {
  float min_zoom;
  int skip;
  int length;
} tick_info;

/* These lists must be sorted by increasing min_zoom! */
static tick_info tick_inches[] = {
  { 0.0, PPI, INCH_MARK },
  { 0.1, HINCH, HALF_MARK },
  { 0.2, QINCH, QUARTER_MARK },
  { 0.6, SINCH, SIXTEENTH_MARK },
};
#define Ntick_inches (sizeof(tick_inches)/sizeof(tick_info))

static tick_info tick_tenth_inches[] = {
  { 0.0, PPI, INCH_MARK },
  { 0.1, HINCH, HALF_MARK },
  { 0.2, QINCH, QUARTER_MARK },
  { 0.6, TINCH, SIXTEENTH_MARK },
  { 1.0, TINCH/2, SIXTEENTH_MARK },
  { 4.0, TINCH/4, SIXTEENTH_MARK },
};
#define Ntick_tenth_inches (sizeof(tick_tenth_inches)/sizeof(tick_info))

static tick_info tick_metric[] = {
  { 0.0,  PPCM, CM_MARK },
  { 0.6,  HALFCM, HALFCM_MARK },
  { 1.0,  ONEMM, MM_MARK },
};
#define Ntick_metric (sizeof(tick_metric)/sizeof(tick_info))

typedef struct ruler_skip_struct {
  float	max_zoom;	/* upper bound of range where these settings apply */
  int	skipt;		/* step value for loop  */
  int	skipx;		/* skip between strings is skipx times skipt */
  int	prec;		/* format precision for floats */
} ruler_skip;

/* These lists must be sorted by increasing max_zoom! */

static ruler_skip ruler_skip_inches[] = {
  {  0.01,   12*PPI, 4, 1},
  {  0.0201, 6*PPI, 4, 1},
  {  0.0401, 2*PPI, 6, 1},
  {  0.0501, PPI, 12, 1},
  {  0.0801, PPI, 6, 1},
  {  0.201,  SINCH, 64, 1},
  {  0.301,  SINCH, 32, 1},
  {  2.0,    SINCH, 16, 1},
  {  4.0,    SINCH, 8, 1},
  {  8.0,    SINCH, 4, 2},
  { 20.0,    SINCH, 2, 3},
  /* max_zoom == 0.0 indicates last element */
  {  0.0,    SINCH, 1, 4},  
};

static ruler_skip ruler_skip_tenth_inches[] = {
  {  0.01,   10*PPI,  10, 1},		/* label every   100 inches, tick 100 */
  {  0.0201, 5*PPI,   10, 1},		/* label every    50 inches, tick   5 */
  {  0.0401, 2*PPI,   10, 1},		/* label every    20 inches, tick   2 */
  {  0.0801, PPI,     10, 1},		/* label every    10 inches, tick   1 */
  {  0.201,  PPI,      5, 1},		/* label every     5 inches, tick   1 */
  {  0.401,  TINCH*2, 10, 1},		/* label every     2 inches, tick   1/5 */
  {  2.0,    TINCH,   10, 1},		/* label every     1 inch, tick 1/10 */
  {  5.0,    TINCH,    5, 1},		/* label every   1/2 inch, tick 1/10 */
  { 15.0,    TINCH/2,  2, 1},		/* label every  1/10 inch, tick 1/20 */
  { 50.0,    TINCH/4,  2, 2},  		/* label every  1/20 inch, tick 1/40 */
  /* max_zoom == 0.0 indicates last element */
  {  0.0,    TINCH/4,  1, 3},  		/* label every  1/40 inch, tick 1/40 */
};

static ruler_skip ruler_skip_metric[] = {
  { 0.011,  20*PPCM, 5, 1},
  { 0.041,  10*PPCM, 5, 1},
  { 0.081,  5*PPCM, 4, 1},
  { 0.11,   2*PPCM, 5, 1},
  { 0.31,   PPCM, 5, 1},
  { 0.61,   ONEMM, 20, 1},
  { 2.01,   ONEMM, 10, 1},
  { 8.01,   ONEMM, 5, 1},
  {12.01,   ONEMM, 2, 1},
  /* max_zoom == 0.0 indicates last element */
  { 0.0,    ONEMM, 1, 1},   
};

typedef struct ruler_info_struct {
  int		 ruler_unit;
  ruler_skip	*skips;
  tick_info	*ticks;
  int		 nticks;
} ruler_info;

static ruler_info ruler_inches = {
  PPI, ruler_skip_inches, tick_inches, Ntick_inches };
static ruler_info ruler_tenth_inches = {
  PPI, ruler_skip_tenth_inches, tick_tenth_inches, Ntick_tenth_inches };
static ruler_info ruler_metric = {
  PPCM, ruler_skip_metric, tick_metric, Ntick_metric };

static tick_info* ticks;
static int	  nticks;
static int	  ruler_unit;
static int	  skip;
static int	  skipx;
static char	  precstr[10];

static Boolean	rul_unit_setting;
static Boolean	fig_unit_setting=False, fig_scale_setting=False;
static Widget	rul_unit_panel, fig_unit_panel, fig_scale_panel;
static Widget	user_unit_lab, user_unit_entry, fraction_checkbox;
static Widget	scale_factor_lab, scale_factor_entry;
static void     unit_panel_cancel(Widget w, XButtonEvent *ev), unit_panel_set(Widget w, XButtonEvent *ev);
static Pixel	unit_bg, unit_fg;
static Pixel	scale_bg, scale_fg;

XtActionsRec	unitbox_actions[] =
{
    {"EnterUnitBox", (XtActionProc) draw_mousefun_unitbox},
    {"LeaveUnitBox", (XtActionProc) clear_mousefun},
    {"HomeRulers", (XtActionProc) pan_origin},
    {"PopupUnits", (XtActionProc) popup_unit_panel},
};

String  unit_text_translations =
	"<Key>Return: SetUnits()\n\
	Ctrl<Key>J: no-op()\n\
	Ctrl<Key>M: SetUnits()\n\
	<Key>Escape: QuitUnits() \n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static String	unitbox_translations =
	"<EnterWindow>:EnterUnitBox()\n\
	<LeaveWindow>:LeaveUnitBox()\n\
	<Btn1Down>:HomeRulers()\n\
	<Btn3Down>:PopupUnits()\n";

static String   unit_translations =
        "<Message>WM_PROTOCOLS: QuitUnits()\n\
	<Key>Return:SetUnits()\n";

static XtActionsRec     unit_actions[] =
{
    {"QuitUnits", (XtActionProc) unit_panel_cancel},
    {"SetUnits", (XtActionProc) unit_panel_set},
};



void
redisplay_rulers(void)
{
    redisplay_topruler();
    redisplay_sideruler();
}

void
setup_rulers(void)
{
    setup_topruler();
    setup_sideruler();
}

void
reset_rulers(void)
{
    reset_topruler();
    reset_sideruler();
    redisplay_rulers();
}

void
set_rulermark(int x, int y)
{
    if (appres.tracking) {
	set_siderulermark(y);
	set_toprulermark(x);
    }
}

void
erase_rulermark(void)
{
    if (appres.tracking) {
	erase_siderulermark();
	erase_toprulermark();
    }
}

void
init_unitbox(Widget tool)
{
    char	    buf[20];

    if (appres.userscale != 1.0)
	fig_scale_setting = True;

    if (strlen(cur_fig_units)) {
	fig_unit_setting = True;
	sprintf(buf,"1%s = %.2f%s", appres.INCHES? "in": "cm",
		appres.userscale, cur_fig_units);
    } else {
	strcpy(cur_fig_units, appres.INCHES ? "in" : "cm");
	sprintf(buf,"1:%.2f", appres.userscale);
    }

    FirstArg(XtNlabel, buf);
    NextArg(XtNwidth, UNITBOX_WD);
    NextArg(XtNheight, RULER_WD);
    NextArg(XtNfromHoriz, topruler_sw);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, msg_panel);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    unitbox_sw = XtCreateWidget("unitbox", labelWidgetClass, tool,
				Args, ArgCount);
    XtAppAddActions(tool_app, unitbox_actions, XtNumber(unitbox_actions));
#ifndef XAW3D1_5E
    /* popup when mouse passes over button */
    XtAddEventHandler(unitbox_sw, EnterWindowMask, False,
		      unit_balloon_trigger, (XtPointer) unitbox_sw);
    XtAddEventHandler(unitbox_sw, LeaveWindowMask, False,
		      unit_unballoon, (XtPointer) unitbox_sw);
#endif /* XAW3D1_5E */
    XtOverrideTranslations(unitbox_sw,
			   XtParseTranslationTable(unitbox_translations));

#ifdef XAW3D1_5E
    update_rulerpanel();
#endif /* XAW3D1_5E */
}

static Widget	unit_popup, unit_panel, cancel, set, beside, below, label;

#ifdef XAW3D1_5E
void update_rulerpanel()
{
    char msg[80];

    strcpy(msg, "Pan to 0,0        ");
    if (appres.flipvisualhints)
	sprintf(msg + strlen(msg), "(right button)");
    else
	sprintf(msg + strlen(msg), "(left button)");
    sprintf(msg + strlen(msg), "\nSet Units/Scale   ");
    if (appres.flipvisualhints)
	sprintf(msg + strlen(msg), "(left button)");
    else
	sprintf(msg + strlen(msg), "(right button)");

    if (unitbox_sw)
	if (appres.showballoons)
	    XawTipEnable(unitbox_sw, msg);
	else
	    XawTipDisable(unitbox_sw);
}
#else
/* come here when the mouse passes over the unit box */

static	Widget unit_balloon_popup = (Widget) 0;
static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w, balloon_label;

static void unit_balloon(void);

static void
unit_balloon_trigger(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch)
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) unit_balloon, (XtPointer) NULL);
}

static void
unit_balloon(void)
{
	Position  x, y;

	Widget  box;

	XtTranslateCoords(balloon_w, 0, 0, &x, &y);
	/* if mode panel is on right (units on left) position popup to right of box */
	if (appres.RHS_PANEL) {
	    x += SIDERULER_WD+5;
	}
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	unit_balloon_popup = XtCreatePopupShell("unit_balloon_popup",
					overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, unit_balloon_popup, 
					Args, ArgCount);

	/* now make two label widgets, one for left button and one for right */
	FirstArg(XtNborderWidth, 0);
	if (appres.flipvisualhints) {
	    NextArg(XtNleftBitmap, mouse_r);	/* bitmap of mouse with right button pushed */
	} else {
	    NextArg(XtNleftBitmap, mouse_l);	/* bitmap of mouse with left button pushed */
	}
	NextArg(XtNlabel, "Pan to (0,0)   ");
	balloon_label = XtCreateManagedWidget("l_label", labelWidgetClass,
				    box, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	if (appres.flipvisualhints) {
	    NextArg(XtNleftBitmap, mouse_l);	/* bitmap of mouse with left button pushed */
	} else {
	    NextArg(XtNleftBitmap, mouse_r);	/* bitmap of mouse with right button pushed */
	}
	NextArg(XtNlabel, "Set Units/Scale");
	balloon_label = XtCreateManagedWidget("r_label", labelWidgetClass,
				box, Args, ArgCount);
	XtRealizeWidget(unit_balloon_popup);

	/* if the mode panel is on the left-hand side (units on right)
	   shift popup to the left */
	if (!appres.RHS_PANEL) {
	    XtWidgetGeometry xtgeom,comp;
	    Dimension wpop;

	    /* get width of popup with label in it */
	    FirstArg(XtNwidth, &wpop);
	    GetValues(balloon_label);
	    /* only change X position of widget */
	    xtgeom.request_mode = CWX;
	    /* shift popup left */
	    xtgeom.x = x-wpop-5;
	    (void) XtMakeGeometryRequest(unit_balloon_popup, &xtgeom, &comp);
	    SetValues(balloon_label);
	}
	XtPopup(unit_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves a button in the mode panel */

static void
unit_unballoon(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch)
{
    if (balloon_id) 
	XtRemoveTimeOut(balloon_id);
    balloon_id = (XtIntervalId) 0;
    if (unit_balloon_popup != (Widget) 0) {
	XtDestroyWidget(unit_balloon_popup);
	unit_balloon_popup = (Widget) 0;
    }
}
#endif /* XAW3D1_5E */

/* handle unit/scale settings */

static void
unit_panel_dismiss(void)
{
    XtDestroyWidget(unit_popup);
    XtSetSensitive(unitbox_sw, True);
}

static void
unit_panel_cancel(Widget w, XButtonEvent *ev)
{
    unit_panel_dismiss();
}

static void
unit_panel_set(Widget w, XButtonEvent *ev)
{
    int		    old_rul_unit, pix;
    float	    scale_factor;

    old_rul_unit = appres.INCHES;
    appres.INCHES = rul_unit_setting ? True : False;

    /* user's scale factor must be <= current resolution (1200 or 450) */
    scale_factor=atof(panel_get_value(scale_factor_entry));
    pix = appres.INCHES? PIX_PER_INCH: PIX_PER_CM;
    if (scale_factor > pix) {
	beep();
    	file_msg("Scale factor must be <= %d", pix);
	return;
    }

    init_grid();	/* change point positioning messages if necessary */
    if (fig_scale_setting)
	appres.userscale = (float) atof(panel_get_value(scale_factor_entry));
    else
	appres.userscale = 1.0;

    set_unit_indicator(fig_unit_setting);

    /* reset the rulers with the new units or change from fract<->decimal */
    reset_rulers();
    setup_grid();

    if (old_rul_unit != appres.INCHES) {
	/* if there are objects on the canvas, scale them to compensate for the
	   difference in inches/metric PIX_PER_XX */
	if (!emptyfigure()) { 
	  if (old_rul_unit)
	    read_scale_compound(&objects,(2.54*PPCM)/((float)PPI),0);
	  else
	    read_scale_compound(&objects,((float)PPI)/(2.54*PPCM),0);
	  redisplay_canvas();
	}
    }
    /* PIC_FACTOR is the number of Fig units (1200dpi) per printer's points (1/72 inch) */
    /* it changes with Metric and Imperial */
    PIC_FACTOR  = (appres.INCHES ? (float)PPI : 2.54*PPCM)/72.0;

    /* reset figure size in print and export panels */
    export_update_figure_size();
    print_update_figure_size();
    /* finally, reset any grid specs in export or print panels to make consistent */
    reset_grid_menus();

    unit_panel_dismiss();
}

void
set_unit_indicator(Boolean use_userscale)
{
    char     buf[20];

    if (use_userscale) {
        strncpy(cur_fig_units,
	    panel_get_value(user_unit_entry),
	    sizeof(cur_fig_units)-1);
	sprintf(buf,"1%s = %.2f%s",
		appres.INCHES ? "in" : "cm",
		appres.userscale, cur_fig_units);
	put_msg("Figure scale: %s",buf);
    } else {
	if (appres.INCHES) {
	    /* IP */
	    /* if user doesn't have "i", "in*", "f", "ft", "feet", "mi" or "mile" set units to "in" */
	    if (strcmp(cur_fig_units, "i") != 0 && strncmp(cur_fig_units,"in",2) != 0 &&
		strcmp(cur_fig_units, "f") != 0 && strcmp(cur_fig_units, "ft") != 0 && 
		strcmp(cur_fig_units, "feet") != 0 && strcmp(cur_fig_units, "mi") != 0 &&
		strcmp(cur_fig_units, "mile") != 0)
			strcpy(cur_fig_units, "in");
	    /* otherwise leave it as the user specified it */
	} else {
	    /* Metric */
	    /* if user doesn't have "mm", "cm", "dm", "m", or "km", then set units to "cm" */
	    if (strcmp(cur_fig_units, "mm") != 0 && strcmp(cur_fig_units,"cm") != 0 &&
		strcmp(cur_fig_units, "dm") != 0 && strcmp(cur_fig_units, "m") != 0 && 
		strcmp(cur_fig_units, "km") != 0)
			strcpy(cur_fig_units, "cm");
	    /* otherwise leave it as the user specified it */
	}
	sprintf(buf,"1%s = %.2f%s", appres.INCHES? "in":"cm", appres.userscale, cur_fig_units);
	put_msg("Figure scale = %s", buf);
    }
    FirstArg(XtNlabel, buf);
    SetValues(unitbox_sw);
}

static void
fig_unit_select(Widget w, XtPointer new_unit, XtPointer garbage)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(fig_unit_panel);
    fig_unit_setting = (Boolean) (int) new_unit;
    if (fig_unit_setting) {
	/* set foreground to original so we can se it */
	FirstArg(XtNforeground, unit_fg);
    } else {
	/* set foreground to background to make it invisible */
	FirstArg(XtNforeground, unit_bg);
    }
    SetValues(user_unit_entry);
    XtSetSensitive(user_unit_lab, fig_unit_setting);
    XtSetSensitive(user_unit_entry, fig_unit_setting);
    put_msg(fig_unit_setting ? "user defined figure units"
			     : "figure units = ruler units");
}

static void
fig_scale_select(Widget w, XtPointer new_scale, XtPointer garbage)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(fig_scale_panel);
    fig_scale_setting = (Boolean) (int) new_scale;
    if (fig_scale_setting) {
	/* set foreground to original so we can se it */
	FirstArg(XtNforeground, scale_fg);
    } else {
	/* set foreground to background to make it invisible */
	FirstArg(XtNforeground, scale_bg);
    }
    SetValues(scale_factor_entry);
    XtSetSensitive(scale_factor_lab, fig_scale_setting);
    XtSetSensitive(scale_factor_entry, fig_scale_setting);
    put_msg(fig_scale_setting ? "user defined scale factor"
			     : "figure scale = 1:1");
}

static void
rul_unit_select(Widget w, XtPointer closure, XtPointer garbage)
{
    int		    new_unit = (int) closure;

    /* return if no change */
    if (new_unit == cur_gridunit)
	return;

    FirstArg(XtNlabel, XtName(w));
    SetValues(rul_unit_panel);

    /* 0 = metric, 1/2 = inches */

    /* since user changed to/from metric/imperial, set unit name */
    panel_set_value(user_unit_entry, new_unit? "in": "cm");

    rul_unit_setting = new_unit;
    if (rul_unit_setting) {
	/* inches, check if decimal or fractional */
	if (rul_unit_setting == TENTH_UNIT) {
	    /* unmanage the fraction_checkbox */
	    put_msg("ruler scale : decimal inches");
	    XtUnmanageChild(fraction_checkbox);
	    cur_gridunit = TENTH_UNIT;
	} else {
	    put_msg("ruler scale : fractional inches");
	    XtManageChild(fraction_checkbox);
	    cur_gridunit = FRACT_UNIT;
	}
    } else {
	/* metric */
	put_msg("ruler scale : centimeters");
	XtUnmanageChild(fraction_checkbox);
	cur_gridunit = MM_UNIT;
    }
}

void
popup_unit_panel(void)
{
    Position	    x_val, y_val;
    Dimension	    width, height;
    char	    buf[32];
    static Boolean  actions_added=False;
    int		    x;
    static char    *rul_unit_items[] =  { "Metric (cm)        ", 
					  "Imperial (fraction)",
					  "Imperial (decimal) "};
    static char    *fig_unit_items[] =  { "Ruler units ",
					  "User defined"};
    static char    *fig_scale_items[] = { "Unity       ",
					  "User defined"};

    /* don't popup if in the middle of drawing/editing */
    if (check_action_on())
	return;

    /* turn off Compose key LED */
    setCompLED(0);

    FirstArg(XtNwidth, &width);
    NextArg(XtNheight, &height);
    GetValues(tool);
    /* position the popup 3/5 in from left (1/8 if mode panel on right) 
       and 1/3 down from top */
    if (appres.RHS_PANEL)
	x = (width / 8);
    else
	x = (3 * width / 5);
    XtTranslateCoords(tool, (Position) x, (Position) (height / 8),
		      &x_val, &y_val);

    FirstArg(XtNx, x_val);
    NextArg(XtNy, y_val);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNtitle, "Xfig: Unit menu");
    unit_popup = XtCreatePopupShell("unit_popup",
				    transientShellWidgetClass, tool,
				    Args, ArgCount);
    XtOverrideTranslations(unit_popup,
                       XtParseTranslationTable(unit_translations));
    if (!actions_added) {
        XtAppAddActions(tool_app, unit_actions, XtNumber(unit_actions));
	actions_added = True;
    }

    unit_panel = XtCreateManagedWidget("unit_panel", formWidgetClass, unit_popup, NULL, 0);

    FirstArg(XtNborderWidth, 0);
    label = XtCreateManagedWidget("         Unit/Scale settings          ",
		labelWidgetClass, unit_panel, Args, ArgCount);

    /* make ruler units menu */

    rul_unit_setting = appres.INCHES ? True : False;
    FirstArg(XtNfromVert, label);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget(" Ruler units", labelWidgetClass,
                                   unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, label);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    rul_unit_panel = XtCreateManagedWidget(rul_unit_items[cur_gridunit],
				menuButtonWidgetClass, unit_panel, Args, ArgCount);
    below = rul_unit_panel;
    make_pulldown_menu(rul_unit_items, XtNumber(rul_unit_items), -1, "",
                                     rul_unit_panel, rul_unit_select);

    /* make figure units menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Figure units", labelWidgetClass,
                                   unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    fig_unit_panel = XtCreateManagedWidget(fig_unit_items[(int) fig_unit_setting],
				menuButtonWidgetClass, unit_panel, Args, ArgCount);
    below = fig_unit_panel;
    make_pulldown_menu(fig_unit_items, XtNumber(fig_unit_items), -1, "",
                                     fig_unit_panel, fig_unit_select);

    /* user defined units */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "   Unit name");
    user_unit_lab = XtCreateManagedWidget("user_units",
                                labelWidgetClass, unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNfromHoriz, user_unit_lab);
    NextArg(XtNstring, cur_fig_units);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 50);
    user_unit_entry = XtCreateManagedWidget("unit_entry", asciiTextWidgetClass,
					unit_panel, Args, ArgCount);
    XtOverrideTranslations(user_unit_entry,
		XtParseTranslationTable(unit_text_translations));
    below = user_unit_entry;

    /* fraction checkbox (only manage if we're currently in fractional inches) */

    fraction_checkbox = CreateCheckbutton("Show fractions", "display_fractions", 
			unit_panel, fig_unit_panel, user_unit_entry,
			(appres.INCHES? MANAGE: DONT_MANAGE), SMALL_CHK,
			&display_fractions, 0, 0);

    /* make figure scale menu */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    beside = XtCreateManagedWidget("Figure scale", labelWidgetClass,
                                   unit_panel, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    fig_scale_panel = XtCreateManagedWidget(fig_scale_items[fig_scale_setting],
				menuButtonWidgetClass, unit_panel, Args, ArgCount);
    below = fig_scale_panel;
    make_pulldown_menu(fig_scale_items, XtNumber(fig_scale_items), -1, "",
                                     fig_scale_panel, fig_scale_select);

    /* scale factor widget */

    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "Scale factor");
    scale_factor_lab = XtCreateManagedWidget("scale_factor",
                                labelWidgetClass, unit_panel, Args, ArgCount);

    sprintf(buf, "%1.2f", appres.userscale);
    FirstArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNfromHoriz, scale_factor_lab);
    NextArg(XtNstring, buf);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 50);
    scale_factor_entry = XtCreateManagedWidget("factor_entry", asciiTextWidgetClass,
                                        unit_panel, Args, ArgCount);
    XtOverrideTranslations(scale_factor_entry,
		XtParseTranslationTable(unit_text_translations));
    below = scale_factor_entry;

    /* standard set/cancel buttons */

    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, below);
    NextArg(XtNborderWidth, INTERNAL_BW);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   unit_panel, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, False,
		      (XtEventHandler)unit_panel_cancel, (XtPointer) NULL);

    FirstArg(XtNlabel, " Set  ");
    NextArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, cancel);
    NextArg(XtNborderWidth, INTERNAL_BW);
    set = XtCreateManagedWidget("set", commandWidgetClass,
				unit_panel, Args, ArgCount);
    XtAddEventHandler(set, ButtonReleaseMask, False,
		      (XtEventHandler)unit_panel_set, (XtPointer) NULL);


    /* pop it up */
    XtPopup(unit_popup, XtGrabExclusive);

    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(unit_popup));

    XtSetSensitive(user_unit_lab, fig_unit_setting);
    XtSetSensitive(user_unit_entry, fig_unit_setting);
    XtSetSensitive(scale_factor_lab, fig_scale_setting);
    XtSetSensitive(scale_factor_entry, fig_scale_setting);

    /* save the fore/backgrounds of the unit and scale entries so we
       can change them to make them look sensitive/insensitive */
    FirstArg(XtNforeground, &unit_fg);
    NextArg(XtNbackground, &unit_bg);
    GetValues(user_unit_entry);
    if (!fig_unit_setting) {
	/* set foreground to background to blank out entry */
	FirstArg(XtNforeground, unit_bg);
	SetValues(user_unit_entry);
    }
    FirstArg(XtNforeground, &scale_fg);
    NextArg(XtNbackground, &scale_bg);
    GetValues(scale_factor_entry);
    if (!fig_scale_setting) {
	/* set foreground to background to blank out entry */
	FirstArg(XtNforeground, unit_bg);
	SetValues(scale_factor_entry);
    }

    (void) XSetWMProtocols(tool_d, XtWindow(unit_popup), &wm_delete_window, 1);

    XtInstallAccelerators(unit_panel, cancel);
    XtInstallAccelerators(unit_panel, set);
}

/************************* TOPRULER ************************/

XtActionsRec	topruler_actions[] =
{
    {"EventTopRuler", (XtActionProc) topruler_selected},
    {"ExposeTopRuler", (XtActionProc) topruler_exposed},
    {"EnterTopRuler", (XtActionProc) draw_mousefun_topruler},
    {"LeaveTopRuler", (XtActionProc) clear_mousefun},
};

static String	topruler_translations =
"Any<BtnDown>:EventTopRuler()\n\
    Any<BtnUp>:EventTopRuler()\n\
    <Btn2Motion>:EventTopRuler()\n\
    Meta <Btn3Motion>:EventTopRuler()\n\
    <EnterWindow>:EnterTopRuler()\n\
    <LeaveWindow>:LeaveTopRuler()\n\
    <KeyPress>:EnterTopRuler()\n\
    <KeyRelease>:EnterTopRuler()\n\
    <Expose>:ExposeTopRuler()\n";

static void
topruler_selected(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams)
{
    XButtonEvent   *be = (XButtonEvent *) event;

    /* see if wheel mouse */
    switch (be->button) {
	/* yes, map button 4 to 1 and 5 to 3 */
	case Button4:
		be->button = Button1;
		break;
	case Button5:
		be->button = Button3;
		break;
    }
    switch (event->type) {
      case ButtonPress:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	  case Button1:
	    XDefineCursor(tool_d, topruler_win, l_arrow_cursor);
	    break;
	  case Button2:
	    XDefineCursor(tool_d, topruler_win, bull_cursor);
	    orig_zoomoff = zoomxoff;
	    last_drag_x = event->x;
	    break;
	  case Button3:
	    XDefineCursor(tool_d, topruler_win, r_arrow_cursor);
	    break;
	}
	break;
      case ButtonRelease:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	  case Button1:
	    pan_left(event->state&ShiftMask);
	    break;
	  case Button2:
	    if (orig_zoomoff != zoomxoff)
		setup_grid();
	    break;
	  case Button3:
	    pan_right(event->state&ShiftMask);
	    break;
	}
	XDefineCursor(tool_d, topruler_win, lr_arrow_cursor);
	break;
      case MotionNotify:
	if (event->x != last_drag_x) {
	    zoomxoff -= ((event->x - last_drag_x)/zoomscale*
				(event->state&ShiftMask?5.0:1.0));
	    if (!appres.allownegcoords && (zoomxoff < 0))
		zoomxoff = 0;
	    reset_topruler();
	    redisplay_topruler();
	}
	last_drag_x = event->x;
	break;
    }
}

void erase_toprulermark(void)
{
    XClearArea(tool_d, topruler_win, ZOOMX(lastx) + troffx,
	       TOPRULER_HT + troffy, trm_pr.width,
	       trm_pr.height, False);
}

void set_toprulermark(int x)
{
    XClearArea(tool_d, topruler_win, ZOOMX(lastx) + troffx,
	       TOPRULER_HT + troffy, trm_pr.width,
	       trm_pr.height, False);
    XCopyArea(tool_d, toparrow_pm, topruler_win, tr_xor_gc,
	      0, 0, trm_pr.width, trm_pr.height,
	      ZOOMX(x) + troffx, TOPRULER_HT + troffy);
    lastx = x;
}

static void
topruler_exposed(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams)
{
    if (((XExposeEvent *) event)->count > 0)
	return;
    redisplay_topruler();
}

void redisplay_topruler(void)
{
    XClearWindow(tool_d, topruler_win);
}

void
init_topruler(Widget tool)
{
    FirstArg(XtNwidth, TOPRULER_WD);
    NextArg(XtNheight, TOPRULER_HT);
    NextArg(XtNlabel, "");
    NextArg(XtNfromHoriz, mode_panel);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, msg_panel);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    topruler_sw = XtCreateWidget("topruler", labelWidgetClass, tool,
				 Args, ArgCount);

    XtAppAddActions(tool_app, topruler_actions, XtNumber(topruler_actions));
    XtOverrideTranslations(topruler_sw,
			   XtParseTranslationTable(topruler_translations));
}

void setup_topruler(void)
{
    unsigned long   bg, fg;
    XGCValues	    gcv;
    unsigned long   gcmask;
    XFontStruct	   *font;

    topruler_win = XtWindow(topruler_sw);
    gcmask = GCFunction | GCForeground | GCBackground | GCFont;

    /* set up the GCs */
    FirstArg(XtNbackground, &bg);
    NextArg(XtNforeground, &fg);
    NextArg(XtNfont, &font);
    GetValues(topruler_sw);

    gcv.font = font->fid;
    gcv.foreground = bg;
    gcv.background = bg;
    gcv.function = GXcopy;
    tr_erase_gc = XCreateGC(tool_d, topruler_win, gcmask, &gcv);

    gcv.foreground = fg;
    tr_gc = XCreateGC(tool_d, topruler_win, gcmask, &gcv);
    /*
     * The arrows will be XORed into the rulers. We want the foreground color
     * in the arrow to result in the foreground or background color in the
     * display. so if the source pixel is fg^bg, it produces fg when XOR'ed
     * with bg, and bg when XOR'ed with bg. If the source pixel is zero, it
     * produces fg when XOR'ed with fg, and bg when XOR'ed with bg.
     */

    /* make pixmaps for top ruler arrow */
    toparrow_pm = XCreatePixmapFromBitmapData(tool_d, topruler_win, 
				trm_pr.bits, trm_pr.width, trm_pr.height, 
				fg^bg, (unsigned long) 0, tool_dpth);

    /* now make the real xor gc */
    gcv.background = bg;
    gcv.function = GXxor;
    tr_xor_gc = XCreateGC(tool_d, topruler_win, gcmask, &gcv);

    XDefineCursor(tool_d, topruler_win, lr_arrow_cursor);

    topruler_pm = XCreatePixmap(tool_d, topruler_win,
				TOPRULER_WD, TOPRULER_HT, tool_dpth);

    reset_topruler();
}

void resize_topruler(void)
{
    XFreePixmap(tool_d, topruler_pm);
    topruler_pm = XCreatePixmap(tool_d, topruler_win,
				TOPRULER_WD, TOPRULER_HT, tool_dpth);
    reset_topruler();
}

/***************************************************************************/

static double factor;

static void
get_skip_prec(void)
{
	register ruler_skip	*rs;
	ruler_info		*ri;

	/* find closest power of 10 to user's scale */
	if (appres.userscale == 1.0) {
		factor = 1.0;
	} else {
		factor = log10((double)appres.userscale);
		if (factor < 0.0)
			factor -= 1.0;
		else
			factor += 1.0;
		factor = (double) (int) factor;
		factor = pow(10.0, factor);
	}

	switch (cur_gridunit) {
		case FRACT_UNIT:  if (appres.userscale == 1.0)
					  ri = &ruler_inches;
				  else
					  ri = &ruler_tenth_inches;
				  break;
		case TENTH_UNIT:  ri = &ruler_tenth_inches;
				  break;
		case MM_UNIT:	  ri = &ruler_metric;
				  break;
	}
			
	ruler_unit = ri->ruler_unit;
	ticks = ri->ticks;
	nticks = ri->nticks;

	for (rs = ri->skips; ; rs++) {
		/* look for values for current zoom */
		if ((display_zoomscale/appres.userscale <= rs->max_zoom) ||
			(rs->max_zoom == 0.0)) {
		    skip = rs->skipt/appres.userscale;
		    skipx = skip * rs->skipx;
		    sprintf(precstr, "%%.%df", rs->prec);
		    break;
		}
	}
}

/* Note: For reset_top/sideruler to work properly, the value of skip should be
 * such that (skip/ruler_unit) is an integer or (ruler_unit/skip) is an integer.
 */

void reset_topruler(void)
{
    register int    i,k;
    register tick_info* tk;
    register Pixmap p = topruler_pm;
    char	    number[6];
    int		    X0,len;
    int		    tickmod, tickskip;

    /* top ruler, adjustments for digits are kludges based on 6x13 char */
    XFillRectangle(tool_d, p, tr_erase_gc, 0, 0, TOPRULER_WD, TOPRULER_HT);

    /* set the number of pixels to skip between labels and precision for float */
    get_skip_prec();

    X0 = BACKX(0);
    X0 -= (X0 % skip);
    tickmod = (int) round(ruler_unit/appres.userscale);
    if (tickmod == 0)
	tickmod = 1;

    /* see how big a label is to adjust spacing, if necessary */
    sprintf(number, "%d%s", (X0+(int)((TOPRULER_WD/zoomscale)))/tickmod, cur_fig_units);
    len = XTextWidth(roman_font, number, strlen(number));
    while (skipx < (len + 5)/zoomscale) {
	skip *= 2;
	skipx *= 2;
    }
    /* recalculate new starting point with new skip value */
    X0 = BACKX(0);
    X0 -= (X0 % skip);

    for (i = X0; i <= X0+(TOPRULER_WD/zoomscale); i += skip) {
      /* string */
      if (i % skipx == 0) {
        if ((i/10) % tickmod == 0)
          sprintf(number, "%d%s", i/tickmod, cur_fig_units);
	else if (i % tickmod == 0)
          sprintf(number, "%d", i/tickmod);
        else
          sprintf(number, precstr, (float)(1.0 * i / tickmod));
	/* get length of string to position it */
	len = XTextWidth(roman_font, number, strlen(number));
        /* we center on the number only, letting the minus sign hang out */
	if (number[0] == '-')
	    len += XTextWidth(roman_font, "-", 1);
	/* centered on inch/cm mark */
	XDrawString(tool_d, p, tr_gc, ZOOMX(i) - len/2,
		TOPRULER_HT - INCH_MARK - 5, number, strlen(number));
      }
      /* ticks */
      for (k = 0, tk = ticks; k < nticks; k++, tk++) {
	tickskip = tk->skip;
	while ((int)(tickskip/appres.userscale) == 0)
	    tickskip *= 2;
        if (display_zoomscale >= tk->min_zoom) {
          if (i % (int)(tickskip/appres.userscale) == 0)
            XDrawLine(tool_d, p, tr_gc,
                      ZOOMX(i), TOPRULER_HT -1, ZOOMX(i), TOPRULER_HT - tk->length -1);
	}
        else
          break;
      }
    }
    /* change the pixmap ID to fool the intrinsics to actually set the pixmap */
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(topruler_sw);
    FirstArg(XtNbackgroundPixmap, p);
    SetValues(topruler_sw);
}

/************************* SIDERULER ************************/

XtActionsRec	sideruler_actions[] =
{
    {"EventSideRuler", (XtActionProc) sideruler_selected},
    {"ExposeSideRuler", (XtActionProc) sideruler_exposed},
    {"EnterSideRuler", (XtActionProc) draw_mousefun_sideruler},
    {"LeaveSideRuler", (XtActionProc) clear_mousefun},
};

static String	sideruler_translations =
"Any<BtnDown>:EventSideRuler()\n\
    Any<BtnUp>:EventSideRuler()\n\
    <Btn2Motion>:EventSideRuler()\n\
    Meta <Btn3Motion>:EventSideRuler()\n\
    <EnterWindow>:EnterSideRuler()\n\
    <LeaveWindow>:LeaveSideRuler()\n\
    <KeyPress>:EnterSideRuler()\n\
    <KeyRelease>:EnterSideRuler()\n\
    <Expose>:ExposeSideRuler()\n";

static void
sideruler_selected(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams)
{
    XButtonEvent   *be = (XButtonEvent *) event;

    /* see if wheel mouse */
    switch (be->button) {
	/* yes, map button 4 to 1 and 5 to 3 */
	case Button4:
		be->button = Button1;
		break;
	case Button5:
		be->button = Button3;
		break;
    }
    switch (event->type) {
    case ButtonPress:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	case Button1:
	    XDefineCursor(tool_d, sideruler_win, u_arrow_cursor);
	    break;
	case Button2:
	    XDefineCursor(tool_d, sideruler_win, bull_cursor);
	    orig_zoomoff = zoomyoff;
	    last_drag_y = event->y;
	    break;
	case Button3:
	    XDefineCursor(tool_d, sideruler_win, d_arrow_cursor);
	    break;
	}
	break;
    case ButtonRelease:
	if (be->button == Button3 && be->state & Mod1Mask) {
	    be->button = Button2;
	}
	switch (be->button) {
	case Button1:
	    pan_up(event->state&ShiftMask);
	    break;
	case Button2:
	    if (orig_zoomoff != zoomyoff)
		setup_grid();
	    break;
	case Button3:
	    pan_down(event->state&ShiftMask);
	    break;
	}
	XDefineCursor(tool_d, sideruler_win, ud_arrow_cursor);
	break;
    case MotionNotify:
	if (event->y != last_drag_y) {
	    zoomyoff -= ((event->y - last_drag_y)/zoomscale*
				(event->state&ShiftMask?5.0:1.0));
	    if (!appres.allownegcoords && (zoomyoff < 0))
		zoomyoff = 0;
	    reset_sideruler();
	    redisplay_sideruler();
	}
	last_drag_y = event->y;
	break;
    }
}

static void
sideruler_exposed(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams)
{
    if (((XExposeEvent *) event)->count > 0)
	return;
    redisplay_sideruler();
}

void
init_sideruler(Widget tool)
{
    FirstArg(XtNwidth, SIDERULER_WD);
    NextArg(XtNheight, SIDERULER_HT);
    NextArg(XtNlabel, "");
    NextArg(XtNfromHoriz, canvas_sw);
    NextArg(XtNfromVert, topruler_sw);
    NextArg(XtNvertDistance, -INTERNAL_BW);
    NextArg(XtNresizable, False);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNborderWidth, INTERNAL_BW);

    sideruler_sw = XtCreateWidget("sideruler", labelWidgetClass, tool,
				  Args, ArgCount);

    XtAppAddActions(tool_app, sideruler_actions, XtNumber(sideruler_actions));
    XtOverrideTranslations(sideruler_sw,
			   XtParseTranslationTable(sideruler_translations));
}

void redisplay_sideruler(void)
{
    XClearWindow(tool_d, sideruler_win);
}

void setup_sideruler(void)
{
    unsigned long   bg, fg;
    XGCValues	    gcv;
    unsigned long   gcmask;
    XFontStruct	   *font;

    sideruler_win = XtWindow(sideruler_sw);
    gcmask = GCFunction | GCForeground | GCBackground | GCFont;

    /* set up the GCs */
    FirstArg(XtNbackground, &bg);
    NextArg(XtNforeground, &fg);
    NextArg(XtNfont, &font);
    GetValues(sideruler_sw);

    gcv.font = font->fid;
    gcv.foreground = bg;
    gcv.background = bg;
    gcv.function = GXcopy;
    sr_erase_gc = XCreateGC(tool_d, sideruler_win, gcmask, &gcv);

    gcv.foreground = fg;
    sr_gc = XCreateGC(tool_d, sideruler_win, gcmask, &gcv);
    /*
     * The arrows will be XORed into the rulers. We want the foreground color
     * in the arrow to result in the foreground or background color in the
     * display. so if the source pixel is fg^bg, it produces fg when XOR'ed
     * with bg, and bg when XOR'ed with bg. If the source pixel is zero, it
     * produces fg when XOR'ed with fg, and bg when XOR'ed with bg.
     */

    /* make pixmaps for side ruler arrow */
    if (appres.RHS_PANEL) {
	sidearrow_pm = XCreatePixmapFromBitmapData(tool_d, sideruler_win, 
				srlm_pr.bits, srlm_pr.width, srlm_pr.height, 
				fg^bg, (unsigned long) 0, tool_dpth);
    } else {
	sidearrow_pm = XCreatePixmapFromBitmapData(tool_d, sideruler_win, 
				srrm_pr.bits, srrm_pr.width, srrm_pr.height, 
				fg^bg, (unsigned long) 0, tool_dpth);
    }

    /* now make the real xor gc */
    gcv.background = bg;
    gcv.function = GXxor;
    sr_xor_gc = XCreateGC(tool_d, sideruler_win, gcmask, &gcv);

    XDefineCursor(tool_d, sideruler_win, ud_arrow_cursor);

    sideruler_pm = XCreatePixmap(tool_d, sideruler_win,
				 SIDERULER_WD, SIDERULER_HT, tool_dpth);

    reset_sideruler();
}

void resize_sideruler(void)
{
    XFreePixmap(tool_d, sideruler_pm);
    sideruler_pm = XCreatePixmap(tool_d, sideruler_win,
				 SIDERULER_WD, SIDERULER_HT, tool_dpth);
    reset_sideruler();
}

void reset_sideruler(void)
{
    register int    i,k;
    register tick_info* tk;
    register Pixmap p = sideruler_pm;
    char	    number[6],len;
    int		    Y0;
    int		    tickmod, tickskip;

    /* side ruler, adjustments for digits are kludges based on 6x13 char */
    XFillRectangle(tool_d, p, sr_erase_gc, 0, 0, SIDERULER_WD,
		   (int) (SIDERULER_HT));

    /* set the number of pixels to skip between labels and precision for float */
    get_skip_prec();

    Y0 = BACKY(0);
    Y0 -= (Y0 % skip);
    /* appres.RHS_PANEL: right-hand or left-hand panel */
    tickmod = (int) round(ruler_unit/appres.userscale);
    if (tickmod == 0)
	tickmod = 1;

    /* make sure the labels aren't too close together vertically */
    while (skipx < 20/zoomscale) {
	skip *= 2;
	skipx *= 2;
    }
    /* recalculate new starting point with new skip value */
    Y0 = BACKY(0);
    Y0 -= (Y0 % skip);

    for (i = Y0; i <= Y0+round(SIDERULER_HT/zoomscale); i += skip) {
      /* string */
      if (i % skipx == 0) {
        if ((i/10) % tickmod == 0)
          sprintf(number, "%d%s", i/tickmod, cur_fig_units);
	else if (i % tickmod == 0)
          sprintf(number, "%d", i/tickmod);
        else
          sprintf(number, precstr, (float)(1.0 * i / tickmod));
	/* get length of string to position it */
	len = XTextWidth(roman_font, number, strlen(number));
	/* vertically centered on inch/cm mark */
        if (appres.RHS_PANEL)
	    XDrawString(tool_d, p, sr_gc, SIDERULER_WD - INCH_MARK - len - 4,
			ZOOMY(i) + 3, number, strlen(number));
        else
	    XDrawString(tool_d, p, sr_gc, INCH_MARK + 4, ZOOMY(i) + 3,
                        number, strlen(number));
      }
      /* ticks */
      for (k = 0, tk = ticks; k < nticks; k++, tk++) {
	tickskip = tk->skip;
	while ((int)(tickskip/appres.userscale) == 0)
	    tickskip *= 2;
        if (display_zoomscale >= tk->min_zoom) {
          if (i % (int)(tickskip/appres.userscale) == 0) {
            if (appres.RHS_PANEL)
              XDrawLine(tool_d, p, sr_gc,
                        SIDERULER_WD - tk->length, ZOOMY(i), SIDERULER_WD, ZOOMY(i));
            else
              XDrawLine(tool_d, p, sr_gc,
                        0, ZOOMY(i), tk->length -1, ZOOMY(i));
	  }
	}
        else
          break;
      }
    }
    /* change the pixmap ID to fool the intrinsics to actually set the pixmap */
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(sideruler_sw);
    FirstArg(XtNbackgroundPixmap, p);
    SetValues(sideruler_sw);
}

void erase_siderulermark(void)
{
    if (appres.RHS_PANEL)
	XClearArea(tool_d, sideruler_win,
		   SIDERULER_WD + srloffx, ZOOMY(lasty) + srloffy,
		   srlm_pr.width, srlm_pr.height, False);
    else
	XClearArea(tool_d, sideruler_win,
		   srroffx, ZOOMY(lasty) + srroffy,
		   srlm_pr.width, srlm_pr.height, False);
}

void set_siderulermark(int y)
{
    if (appres.RHS_PANEL) {
	/*
	 * Because the ruler uses a background pixmap, we can win here by
	 * using XClearArea to erase the old thing.
	 */
	XClearArea(tool_d, sideruler_win,
		   SIDERULER_WD + srloffx, ZOOMY(lasty) + srloffy,
		   srlm_pr.width, srlm_pr.height, False);
	XCopyArea(tool_d, sidearrow_pm, sideruler_win,
		  sr_xor_gc, 0, 0, srlm_pr.width,
		  srlm_pr.height, SIDERULER_WD + srloffx, ZOOMY(y) + srloffy);
    } else {
	/*
	 * Because the ruler uses a background pixmap, we can win here by
	 * using XClearArea to erase the old thing.
	 */
	XClearArea(tool_d, sideruler_win,
		   srroffx, ZOOMY(lasty) + srroffy,
		   srlm_pr.width, srlm_pr.height, False);
	XCopyArea(tool_d, sidearrow_pm, sideruler_win,
		  sr_xor_gc, 0, 0, srrm_pr.width,
		  srrm_pr.height, srroffx, ZOOMY(y) + srroffy);
    }
    lasty = y;
}
