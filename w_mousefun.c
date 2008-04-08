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
#include "figx.h"
#include "resources.h"
#include <X11/keysym.h>
#include "w_icons.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_mousefun.h"
#include "w_setup.h"
#include "w_util.h"

#define MOUSE_BUT_WID		(int) (MOUSEFUN_WD * 0.045)
#define MOUSE_BUT_HGT		(int) (MOUSEFUN_HT * 0.5)
#define MOUSE_LEFT_SPACE	(int) ((MOUSEFUN_WD - 4 * MOUSE_BUT_WID) / 2)
#define MOUSE_LEFT_CTR		(int) (MOUSE_LEFT_SPACE/2)
#define MOUSE_MID_CTR		(int) (MOUSEFUN_WD / 2)
#define MOUSE_RIGHT_CTR		(int) (MOUSEFUN_WD - MOUSE_LEFT_CTR)
#define MOUSE_RIGHT		      (appres.flipvisualhints? MOUSE_LEFT_CTR: MOUSE_RIGHT_CTR)
#define MOUSE_LEFT		      (appres.flipvisualhints? MOUSE_RIGHT_CTR: MOUSE_LEFT_CTR)
#define MOUSEFUN_MAX		       20

/* Function Prototypes */
static void	reset_mousefun(void);

DeclareStaticArgs(18);
static char	mousefun_l[MOUSEFUN_MAX];
static char	mousefun_m[MOUSEFUN_MAX];
static char	mousefun_r[MOUSEFUN_MAX];
static char	mousefun_sh_l[MOUSEFUN_MAX];
static char	mousefun_sh_m[MOUSEFUN_MAX];
static char	mousefun_sh_r[MOUSEFUN_MAX];

/* labels for the left and right buttons have 18 chars max */
static char	lr_blank[]  = "                  ";
/* give the middle button label the same */
static char	mid_blank[] = "                  ";

static Pixmap	mousefun_pm;
static Pixmap	keybd_pm;

#ifndef XAW3D1_5E
/* popup message over button when mouse enters it */
static void     mouse_balloon_trigger(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch);
static void     mouse_unballoon(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch);
#endif /* XAW3D1_5E */


void mouse_title (void);

void
init_mousefun(Widget tool)
{
    /* start with nominal height and adjust later */
    FirstArg(XtNheight, MSGPANEL_HT);
    NextArg(XtNwidth, MOUSEFUN_WD);
    NextArg(XtNresizable, False);
    NextArg(XtNfromHoriz, name_panel);
    NextArg(XtNhorizDistance, -INTERNAL_BW);
    NextArg(XtNfromVert, NULL);
    NextArg(XtNvertDistance, 0);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNbackgroundPixmap, NULL);
    NextArg(XtNmappedWhenManaged, False);
    NextArg(XtNlabel, "");
    NextArg(XtNleft, XtChainLeft);	/* chain so doesn't resize */
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);

    mousefun = XtCreateManagedWidget("mouse_panel", labelWidgetClass,
				     tool, Args, ArgCount);
#ifdef XAW3D1_5E
    update_mousepanel();
#else
    /* popup when mouse passes over button */
    XtAddEventHandler(mousefun, EnterWindowMask, False,
		      mouse_balloon_trigger, (XtPointer) mousefun);
    XtAddEventHandler(mousefun, LeaveWindowMask, False,
		      mouse_unballoon, (XtPointer) mousefun);
#endif /* XAW3D1_5E */
}

/* widgets are realized and windows exist at this point */

void
setup_mousefun(void)
{
    XDefineCursor(tool_d, XtWindow(mousefun), arrow_cursor);
    /* now that the message panel has the real height it needs (because of
       the font size we can resize the mouse panel */
    MOUSEFUN_HT = MSGPANEL_HT + CMDFORM_HT + INTERNAL_BW;
    XtUnmanageChild(mousefun);
    FirstArg(XtNheight, MOUSEFUN_HT);
    SetValues(mousefun);
    XtManageChild(mousefun);
    reset_mousefun();
    set_mousefun("", "", "", "", "", "");
}

#ifdef XAW3D1_5E
void update_mousepanel()
{
    if (mousefun)
	if (appres.showballoons)
	    XawTipEnable(mousefun,
			 "Shows which mouse buttons\nare active in each mode");
	else
	    XawTipDisable(mousefun);
}
#else
/* come here when the mouse passes over a button in the mouse indicator panel */

static	Widget mouse_balloon_popup = (Widget) 0;

static	XtIntervalId balloon_id = (XtIntervalId) 0;
static	Widget balloon_w;

static void mouse_balloon(void);

static void
mouse_balloon_trigger(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch)
{
	if (!appres.showballoons)
		return;
	balloon_w = widget;
	balloon_id = XtAppAddTimeOut(tool_app, appres.balloon_delay,
			(XtTimerCallbackProc) mouse_balloon, (XtPointer) NULL);
}

static void
mouse_balloon(void)
{
	Position  x, y;
	XtWidgetGeometry xtgeom,comp;
	Dimension wpop;
	Widget box, balloon_label;

	XtTranslateCoords(balloon_w, 0, 0, &x, &y);
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	mouse_balloon_popup = XtCreatePopupShell("mouse_balloon_popup",
					overrideShellWidgetClass, tool, Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNhSpace, 0);
	NextArg(XtNvSpace, 0);
	box = XtCreateManagedWidget("box", boxWidgetClass, mouse_balloon_popup,
					Args, ArgCount);
	FirstArg(XtNborderWidth, 0);
	NextArg(XtNlabel, "Shows which mouse buttons\nare active in each mode");
	balloon_label = XtCreateManagedWidget("label", labelWidgetClass,
				box, Args, ArgCount);

	XtRealizeWidget(mouse_balloon_popup);

	/* get width of popup with label in it */
	FirstArg(XtNwidth, &wpop);
	GetValues(balloon_label);
	/* only change X position of widget */
	xtgeom.request_mode = CWX;
	/* shift popup left */
	xtgeom.x = x-wpop-5;
	(void) XtMakeGeometryRequest(mouse_balloon_popup, &xtgeom, &comp);
	SetValues(balloon_label);
	XtPopup(mouse_balloon_popup,XtGrabNone);
	XtRemoveTimeOut(balloon_id);
	balloon_id = (XtIntervalId) 0;
}

/* come here when the mouse leaves a button in the mouse panel */

static void
mouse_unballoon(Widget widget, XtPointer closure, XEvent *event, Boolean *continue_to_dispatch)
{
    if (balloon_id) 
	XtRemoveTimeOut(balloon_id);
    balloon_id = (XtIntervalId) 0;
    if (mouse_balloon_popup != (Widget) 0) {
	XtDestroyWidget(mouse_balloon_popup);
	mouse_balloon_popup = (Widget) 0;
    }
}
#endif /* XAW3D1_5E */

static void
reset_mousefun(void)
{
    /* get the foreground and background from the mousefun widget */
    /* and create a gc with those values */
    mouse_button_gc = XCreateGC(tool_d, XtWindow(mousefun), (unsigned long) 0, NULL);
    FirstArg(XtNforeground, &mouse_but_fg);
    NextArg(XtNbackground, &mouse_but_bg);
    GetValues(mousefun);
    XSetBackground(tool_d, mouse_button_gc, mouse_but_bg);
    XSetForeground(tool_d, mouse_button_gc, mouse_but_fg);
    XSetFont(tool_d, mouse_button_gc, button_font->fid);

    /* also create gc with fore=background for blanking areas */
    mouse_blank_gc = XCreateGC(tool_d, XtWindow(mousefun), (unsigned long) 0, NULL);
    XSetBackground(tool_d, mouse_blank_gc, mouse_but_bg);
    XSetForeground(tool_d, mouse_blank_gc, mouse_but_bg);

    mousefun_pm = XCreatePixmap(tool_d, XtWindow(mousefun),
		    MOUSEFUN_WD, MOUSEFUN_HT, tool_dpth);

    XFillRectangle(tool_d, mousefun_pm, mouse_blank_gc, 0, 0,
		   MOUSEFUN_WD, MOUSEFUN_HT);

    /* draw the left button */
    XDrawRectangle(tool_d, mousefun_pm, mouse_button_gc, MOUSE_LEFT_SPACE,
		   (int) (MOUSEFUN_HT * 0.45), MOUSE_BUT_WID, MOUSE_BUT_HGT);
    /* draw a small line horizontally to the left of the left button */
    XDrawLine(tool_d, mousefun_pm, mouse_button_gc, 
		   MOUSE_LEFT_SPACE, (int) (MOUSEFUN_HT * 0.45 + MOUSE_BUT_HGT/2),
		   MOUSE_LEFT_SPACE-5, (int) (MOUSEFUN_HT * 0.45 + MOUSE_BUT_HGT/2));

    /* draw the middle button */
    XDrawRectangle(tool_d, mousefun_pm, mouse_button_gc,
		   (int) (MOUSE_LEFT_SPACE + 1.5 * MOUSE_BUT_WID),
		   (int) (MOUSEFUN_HT * 0.45), MOUSE_BUT_WID, MOUSE_BUT_HGT);
    /* draw a small line vertically above the middle button */
    XDrawLine(tool_d, mousefun_pm, mouse_button_gc, 
		   (int) (MOUSE_LEFT_SPACE + 1.5 * MOUSE_BUT_WID + 0.5*MOUSE_BUT_WID),
		   (int) (MOUSEFUN_HT * 0.45), 
		   (int) (MOUSE_LEFT_SPACE + 1.5 * MOUSE_BUT_WID + 0.5*MOUSE_BUT_WID),
		   (int) (MOUSEFUN_HT * 0.45 - 5)); 

    /* draw the right button */
    XDrawRectangle(tool_d, mousefun_pm, mouse_button_gc,
		   (int) (MOUSE_LEFT_SPACE + 3 * MOUSE_BUT_WID),
		   (int) (MOUSEFUN_HT * 0.45), MOUSE_BUT_WID, MOUSE_BUT_HGT);
    /* draw a small line horizontally to the right of the right button */
    XDrawLine(tool_d, mousefun_pm, mouse_button_gc, 
		   (int) (MOUSE_LEFT_SPACE + 4 * MOUSE_BUT_WID),
		   (int) (MOUSEFUN_HT * 0.45 + MOUSE_BUT_HGT/2),
		   (int) (MOUSE_LEFT_SPACE + 4 * MOUSE_BUT_WID)+5,
		   (int) (MOUSEFUN_HT * 0.45 + MOUSE_BUT_HGT/2));

    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
    mouse_title();
    FirstArg(XtNmappedWhenManaged, True);
    SetValues(mousefun);
}

static char *title = "Mouse Buttons";

void mouse_title(void)
{
    /* put a title in the window */
    XDrawImageString(tool_d, mousefun_pm, mouse_button_gc,
		     4, button_font->ascent+4, title, strlen(title));
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
resize_mousefun(void)
{
    XFreePixmap(tool_d, mousefun_pm);
    reset_mousefun();
}

void
set_mousefun(const char *left, const char *middle, const char *right, 
		const char *sh_left, const char *sh_middle, const char *sh_right)
{
    strcpy(mousefun_l, left);
    strcpy(mousefun_m, middle);
    strcpy(mousefun_r, right);
    strcpy(mousefun_sh_l, sh_left);
    strcpy(mousefun_sh_m, sh_middle);
    strcpy(mousefun_sh_r, sh_right);
}

#define KBD_POS_X 210
#define KBD_POS_Y 1

void
draw_mousefun_kbd(void)
{
    if (keybd_pm == 0) {
	keybd_pm = XCreatePixmapFromBitmapData(tool_d, canvas_win, 
				kbd_ic.bits, kbd_ic.width, kbd_ic.height, 
				mouse_but_fg, mouse_but_bg, tool_dpth);
    }
    /* copy the keyboard image pixmap into the mouse function pixmap */
    XCopyArea(tool_d, keybd_pm, mousefun_pm, mouse_button_gc,
	      0, 0, kbd_ic.width, kbd_ic.height,
	      MOUSEFUN_WD-10-kbd_ic.width, KBD_POS_Y);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
clear_mousefun_kbd(void)
{
    XFillRectangle(tool_d, mousefun_pm, mouse_blank_gc,
              MOUSEFUN_WD-10-kbd_ic.width, KBD_POS_Y, 
	      kbd_ic.width, kbd_ic.height);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
draw_mousefun_mode(void)
{
    draw_mousefun("Change Mode", "", "");
}

void
draw_mousefun_ind(void)
{
    draw_mousefun("Menu", "Dec/Prev", "Inc/Next");
}

void
draw_mousefun_unitbox(void)
{
    draw_mousefun("Pan to Origin", "", "Set Units/Scale");
}

void
draw_mousefun_topruler(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (event->type == KeyPress) {
	KeySym	    key;
	XKeyEvent  *xkey = (XKeyEvent *)event;
	key = XLookupKeysym(xkey, 0);
	if (key == XK_Shift_L || key == XK_Shift_R)
	    draw_mousefun("Pan Left x5", "Drag x5", "Pan Right x5");
    } else
	draw_mousefun("Pan Left", "Drag", "Pan Right");
}

void
draw_mousefun_sideruler(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (event->type == KeyPress) {
	KeySym	    key;
	XKeyEvent *xkey = (XKeyEvent *)event;
	key = XLookupKeysym(xkey, 0);
	if (key == XK_Shift_L || key == XK_Shift_R)
	    draw_mousefun("Pan Up x5", "Drag x5", "Pan Down x5");
    } else
	draw_mousefun("Pan Up", "Drag", "Pan Down");
}

void
draw_shift_mousefun_canvas(void)
{
    draw_mousefun(mousefun_sh_l, mousefun_sh_m, mousefun_sh_r);
}

void
draw_shift_mousefun_canvas2(const char *tl, const char *tm, const char *tr)
{
    if (!tl)
      tl = mousefun_sh_l;
    if (!tm)
      tm = mousefun_sh_m;
    if (!tr)
      tr = mousefun_sh_r;
    draw_mousefun(tl, tm, tr);
}

void
draw_mousefun_canvas(void)
{
    draw_mousefun(mousefun_l, mousefun_m, mousefun_r);
}

static void
draw_mousefun_msg(char *s, int xctr, int ypos)
{
    int		    width;

    width = XTextWidth(button_font, s, strlen(s));
    XDrawImageString(tool_d, mousefun_pm, mouse_button_gc,
		     xctr - (int) (width / 2), ypos, s, strlen(s));
}

static char	mousefun_cur_l[MOUSEFUN_MAX];
static char	mousefun_cur_m[MOUSEFUN_MAX];
static char	mousefun_cur_r[MOUSEFUN_MAX];

void
draw_mousefun(const char *left, const char *middle, const char *right)
{
  if (strcmp(left, mousefun_cur_l)
      || strcmp(middle, mousefun_cur_m)
      || strcmp(right, mousefun_cur_r)) {
    clear_mousefun();
    draw_mousefn2(left, middle, right);
  }
}

#define MOUSE_LR_Y 32
#define MOUSE_MID_Y 10
void
draw_mousefn2(const char *left, const char *middle, const char *right)
{
  if (strcmp(left, mousefun_cur_l)
      || strcmp(middle, mousefun_cur_m)
      || strcmp(right, mousefun_cur_r)) {
    strcpy(mousefun_cur_l, left);
    strcpy(mousefun_cur_m, middle);
    strcpy(mousefun_cur_r, right);
    draw_mousefun_msg(left, MOUSE_LEFT, MOUSE_LR_Y);
    draw_mousefun_msg(middle, MOUSE_MID_CTR, MOUSE_MID_Y);
    draw_mousefun_msg(right, MOUSE_RIGHT, MOUSE_LR_Y);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
  }
}

void
notused_middle(void)
{
    draw_mousefun_msg("Not Used", MOUSE_MID_CTR, MOUSE_MID_Y);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
clear_middle(void)
{
    draw_mousefun_msg(mid_blank, MOUSE_MID_CTR, MOUSE_MID_Y);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
notused_right(void)
{
    draw_mousefun_msg("Not Used", MOUSE_RIGHT, MOUSE_LR_Y);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
clear_right(void)
{
    draw_mousefun_msg(mid_blank, MOUSE_RIGHT, MOUSE_LR_Y);
    FirstArg(XtNbackgroundPixmap, 0);
    SetValues(mousefun);
    FirstArg(XtNbackgroundPixmap, mousefun_pm);
    SetValues(mousefun);
}

void
clear_mousefun(void)
{
    draw_mousefn2(lr_blank, mid_blank, lr_blank);
    /* redraw the title in case the blanks overwrite it */
    mouse_title();
}

static XtActionsRec	kbd_actions[] =
{
    {"DrawMousefunKbd", (XtActionProc) draw_mousefun_kbd},
    {"ClearMousefunKbd", (XtActionProc) clear_mousefun_kbd},
};

String          kbd_translations =
	"<EnterNotify>: DrawMousefunKbd()\n\
	<LeaveNotify>: ClearMousefunKbd()\n";

static Boolean	actions_added=False;

void
init_kbd_actions(void)
{
    if (!actions_added) {
	actions_added = True;
	XtAppAddActions(tool_app, kbd_actions, XtNumber(kbd_actions));
    }
}
