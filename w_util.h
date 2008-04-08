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

#ifndef W_UTIL_H
#define W_UTIL_H

#include "w_indpanel.h"

/* constant values used for popup_query */

#define QUERY_YESCAN	0
#define QUERY_YESNO	1
#define QUERY_YESNOCAN	2
#define QUERY_ALLPARTCAN 3
#define QUERY_OK	4

#define RESULT_CANCEL	-1
#define RESULT_NO	0
#define RESULT_YES	1
#define RESULT_ALL	2
#define RESULT_PART	3

/* some consts used by both w_indpanel.c, w_export.c and w_print.c */

#define I_CHOICE	0
#define I_IVAL		1
#define I_FVAL		2

/* width/height of the color buttons */

#define COLOR_BUT_WID	82
#define COLOR_BUT_HT	18
#define	COLOR_BUT_BD_WID 1	/* border width */

/* true/false consts for make_color_popup_menu */

#define INCL_TRANSP	True	/* include transparent color button */
#define NO_TRANSP	False	/* don't ... */

#define INCL_BACKG	True	/* include background color button */
#define NO_BACKG	False	/* don't ... */

#define	MANAGE		True	/* manage checkbox after creating */
#define	DONT_MANAGE	False	/* don't ... */

#define	LARGE_CHK	True	/* use large checkmark */
#define	SMALL_CHK	False	/* use small checkmark (don't use large) */

/* number of arrow types */

#ifdef NEWARROWTYPES
#define NUM_ARROW_TYPES		30
#else
#define NUM_ARROW_TYPES		8
#endif /* NEWARROWTYPES */

/* EXPORTS */

extern	char	*grid_inch_choices[], *grid_tenth_inch_choices[];
extern	char	*grid_cm_choices[];
extern	int	num_grid_inch_choices, num_grid_tenth_inch_choices, num_grid_cm_choices;
extern	char	**grid_choices;
extern	int	n_grid_choices, grid_minor, grid_major;
extern	Widget	make_grid_options(Widget parent, Widget put_below, Widget put_beside, char *minor_grid_value, char *major_grid_value, Widget *grid_minor_menu_button, Widget *grid_major_menu_button, Widget *grid_minor_menu, Widget *grid_major_menu, Widget *print_grid_minor_text, Widget *print_grid_major_text, Widget *grid_unit_label, void (*grid_major_select) (/* ??? */), void (*grid_minor_select) (/* ??? */));
extern	void	reset_grid_menus(void);

extern	Boolean	check_action_on(void);
extern	void	check_for_resize(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams);
extern	void	check_colors(void);

extern	Widget	make_pulldown_menu(char **entries, Cardinal nent, int divide_line, char *divide_message, Widget parent, XtCallbackProc callback);
extern	Widget	make_color_popup_menu(Widget parent, char *name, XtCallbackProc callback, Boolean include_transp, Boolean include_backg);
extern	void	set_color_name(int color, char *buf);
extern	void	set_but_col(Widget widget, Pixel color);
extern	Widget	MakeIntSpinnerEntry(Widget parent, Widget *text, char *name, Widget below, Widget beside, XtCallbackProc callback, char *string, int min, int max, int inc, int width);
extern	Widget	MakeFloatSpinnerEntry(Widget parent, Widget *text, char *name, Widget below, Widget beside, XtCallbackProc callback, char *string, float min, float max, float inc, int width);
extern	Widget	CreateCheckbutton(char *label, char *widget_name, Widget parent, Widget below, Widget beside, Boolean manage, Boolean large, Boolean *value, XtCallbackProc user_callback, Widget *togwidg);
extern	XtCallbackProc toggle_checkbutton(Widget w, XtPointer data, XtPointer garbage);
extern	Pixmap	mouse_l, mouse_r;
extern	Pixmap	check_pm, null_check_pm;
extern	Pixmap	sm_check_pm, sm_null_check_pm;
/* put these here so w_layers.c can get to them too */
#define check_width 16
#define check_height 16
#define sm_check_width 10
#define sm_check_height 10
extern	Pixmap	balloons_on_bitmap;
extern	Pixmap	balloons_off_bitmap;
extern	Pixmap	menu_arrow, menu_cascade_arrow;
extern	Pixmap	arrow_pixmaps[NUM_ARROW_TYPES+1];
extern	Pixmap	diamond_pixmap;
extern	Pixmap	linestyle_pixmaps[NUM_LINESTYLE_TYPES];
extern	char    *panel_get_value(Widget widg);
extern	void	panel_set_value(Widget widg, char *val);
extern	void	panel_set_int(Widget widg, int intval), panel_set_float(Widget widg, float floatval, char *format);
extern	void	update_wm_title(char *name);
extern	void	get_pointer_win_xy(int *xposn, int *yposn);
extern	void	get_pointer_root_xy(int *xposn, int *yposn);
extern	void	spinner_up_down(Widget w, XButtonEvent *ev, String *params, Cardinal *num_params);
extern	void	clear_splash(void);
extern	void	InstallScroll(Widget widget);
extern	void	InstallScrollParent(Widget widget);
extern  void fix_converters(void);

extern	Boolean	user_colors_saved;
extern	Boolean	nuser_colors_saved;

/*
 * Author:	Doyle C. Davidson
 *		Intergraph Corporation
 *		One Madison Industrial Park
 *		Huntsville, Al.	 35894-0001
 *
 * Modification history:
 *		11 May 91 - added SetValues and GetValues - Paul King
 *
 * My macros for using XtSetArg easily:
 * Usage:
 *
 *	blah()
 *	{
 *	DeclareArgs(2);
 *		...
 *		FirstArg(XmNx, 100);
 *		NextArg(XmNy, 80);
 *		button = XmCreatePushButton(parent, name, Args, ArgCount);
 *	}
 */

#include <assert.h>

#define ArgCount	_ArgCount
#define Args		_ArgList
#define ArgCountMax	_ArgCountMax

#define DeclareArgs(n)	Arg Args[n]; int ArgCountMax = n; int ArgCount

#define DeclareStaticArgs(n)  static Arg Args[n]; static int ArgCountMax = n; static int ArgCount

#define FirstArg(name, val) \
	{ XtSetArg(Args[0], (name), (val)); ArgCount=1;}
#define NextArg(name, val) \
	{ assert(ArgCount < ArgCountMax); \
	  XtSetArg(Args[ArgCount], (name), (val)); ArgCount++;}
#define GetValues(n)	XtGetValues(n, Args, ArgCount)
#define SetValues(n)	XtSetValues(n, Args, ArgCount)

/* data structure passed to SpinnerEntry callback */

typedef struct {
	Widget	widget;		/* text widget inside spinner */
	float	min, max;	/* min, max values allowed */
	float	inc;		/* how much to inc/dec spinner with each click */
} spin_struct;

extern void app_flush (void);
extern void file_msg_add_grab (void);
extern void process_pending (void);
extern void resize_all (int width, int height);
extern void restore_nuser_colors (void);
extern void restore_user_colors (void);
extern void save_nuser_colors (void);
extern void save_user_colors (void);
extern int popup_query(int query_type, char *message);
extern void create_bitmaps(void);
extern void splash_screen(void);
extern int xallncol(char *name, XColor *color, XColor *exact);

#endif /* W_UTIL_H */
