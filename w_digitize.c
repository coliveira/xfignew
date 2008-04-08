/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2002 by Brian V. Smith
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
#include "w_digitize.h"
#include "w_indpanel.h"
#include "w_setup.h"
#include "w_util.h"

#include "w_canvas.h"

/* LOCAL */

/* max value for sequence number in filename */
#define MAX_SEQ	9999

static	Widget	digitize_popup = 0;
static	Widget	digitize_panel;
static	Widget	digitize_file_prefix, digitize_file_seq, digitize_file_suffix;
static	Widget	newfiletoggle, appendtoggle, example_filename;
static	void	digitize_panel_done(void), digitize_panel_cancel(void);
static	Boolean	digitize_append_flag = True;	/* whether to append to same file or make
						   new ones for each digitized line */
static	void	create_digitize_panel(void);
static	void	update_example(Widget w, XtPointer info, XtPointer dum);
static	void    switch_file_mode(Widget w, XtPointer closure, XtPointer call_data);

String  dig_translations =
        "<Message>WM_PROTOCOLS: DismissDigitize()";

static XtActionsRec	digitize_actions[] =
{
    {"DismissDigitize", (XtActionProc) digitize_panel_done},
    {"PopupDigitize", (XtActionProc) popup_digitize_panel},
};

static XtCallbackRec update_callback_rec[] = {
		{(XtCallbackProc) update_example, (XtPointer)NULL},
		{(XtCallbackProc)NULL, (XtPointer)NULL},
	};

static	char	*prefix_save =  (char *) NULL;
static	char	*start_save =   (char *) NULL;
static	char	*suffix_save =  (char *) NULL;
static	char	*example_save = (char *) NULL;
static	Boolean	digitize_append_save;

void
popup_digitize_panel(Widget w)
{
	DeclareArgs(2);
	char	   *tmpstr;

	/* turn off Compose key LED */
	setCompLED(0);

	if (digitize_popup == 0)
	    create_digitize_panel();
	/* save current values */
	prefix_save  = my_strdup(panel_get_value(digitize_file_prefix));
	start_save   = my_strdup(panel_get_value(digitize_file_seq));
	suffix_save  = my_strdup(panel_get_value(digitize_file_suffix));
	digitize_append_save = digitize_append_flag;
	FirstArg(XtNlabel, &tmpstr);
	GetValues(example_filename);
	example_save = my_strdup(tmpstr);
	
	XtPopup(digitize_popup, XtGrabNonexclusive);
}

static void
create_digitize_panel(void)
{
	DeclareArgs(20);
	Widget	    	beside, below, done_but;
	Widget		file_prefix_seq; 
	Position	xposn, yposn;

	XtTranslateCoords(tool, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn+50);
	NextArg(XtNy, yposn+50);
	NextArg(XtNtitle, "Xfig: Export menu");
	NextArg(XtNcolormap, tool_cm);
	digitize_popup = XtCreatePopupShell("digitize_popup",
					  transientShellWidgetClass,
					  tool, Args, ArgCount);
	XtOverrideTranslations(digitize_popup,
			   XtParseTranslationTable(dig_translations));
	XtAppAddActions(tool_app, digitize_actions, XtNumber(digitize_actions));

	digitize_panel = XtCreateManagedWidget("digitize_panel", formWidgetClass,
					     digitize_popup, NULL, ZERO);

	/* Title */

	FirstArg(XtNlabel, "Digitize to file(s)");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("digitize_title", labelWidgetClass,
					digitize_panel, Args, ArgCount);

	/* File prefix */

	FirstArg(XtNlabel, "Filename prefix");
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 8);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("file_prefix_label", labelWidgetClass,
					digitize_panel, Args, ArgCount);

	/* entry for file prefix */

	FirstArg(XtNwidth, 100);
	NextArg(XtNcallback, update_callback_rec);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 8);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	below = digitize_file_prefix = XtCreateManagedWidget("file_prefix_entry",
					    asciiTextWidgetClass, digitize_panel,
					    Args, ArgCount);
        XtOverrideTranslations(digitize_file_prefix,
                           XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "     Starting #");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("starting_num_label", labelWidgetClass,
					digitize_panel, Args, ArgCount);
	
	/* spinner for starting sequence number */

	below = file_prefix_seq = MakeIntSpinnerEntry(digitize_panel, &digitize_file_seq,
				"file_prefix_seq", below, beside, update_example,
				"0", 0, MAX_SEQ, 1, 40);

	FirstArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	SetValues(below);

	/* label for suffix */

	FirstArg(XtNlabel, "Filename suffix");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("file_suffix_label", labelWidgetClass,
					digitize_panel, Args, ArgCount);
	
	/* entry for file suffix */

	FirstArg(XtNstring, "xy");
	NextArg(XtNwidth, 40);
	NextArg(XtNcallback, update_callback_rec);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	below = digitize_file_suffix = XtCreateManagedWidget("file_suffix_entry",
					    asciiTextWidgetClass, digitize_panel,
					    Args, ArgCount);
        XtOverrideTranslations(digitize_file_suffix,
                           XtParseTranslationTable(text_translations));

	FirstArg(XtNlabel, "        Example");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("example_filelabel", labelWidgetClass,
					digitize_panel, Args, ArgCount);

	FirstArg(XtNlabel, "                        ");
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	below = example_filename = XtCreateManagedWidget("example_filename", labelWidgetClass,
					digitize_panel, Args, ArgCount);

	FirstArg(XtNlabel, "  For each line");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("label", labelWidgetClass,
					digitize_panel, Args, ArgCount);

	/* create two radio buttons, "Append to file" and "Create new file" */

	/* radio for append */

	FirstArg(XtNbitmap, (digitize_append_flag? sm_check_pm : sm_null_check_pm));
	NextArg(XtNwidth, sm_check_width+6);
	NextArg(XtNheight, sm_check_height+6);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);	/* make it stay on left side */
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNsensitive, (digitize_append_flag? False : True)); /* make opp. button sens. */
	NextArg(XtNstate, digitize_append_flag);  /* initial state */
	NextArg(XtNradioData, 1);		/* when this is pressed the value is 1 */
	appendtoggle = XtCreateManagedWidget("appendtoggle", toggleWidgetClass,
				digitize_panel, Args, ArgCount);
	XtAddCallback(appendtoggle, XtNcallback, (XtCallbackProc) switch_file_mode,
					(XtPointer) NULL);

	/* label - "Append to file" */

	FirstArg(XtNlabel, "Append to file ");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, appendtoggle);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("appendlabel", labelWidgetClass,
				digitize_panel, Args, ArgCount);

	/* radio for blanking */

	FirstArg(XtNbitmap, (digitize_append_flag? sm_null_check_pm : sm_check_pm));
	NextArg(XtNwidth, sm_check_width+6);
	NextArg(XtNheight, sm_check_height+6);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNsensitive, (digitize_append_flag? True : False)); /* make opp. button sens. */
	NextArg(XtNstate, !digitize_append_flag); /* initial state */
	NextArg(XtNradioData, 2);		/* when this is pressed the value is 2 */
	NextArg(XtNradioGroup, appendtoggle);	/* this is the other radio button in the group */
	newfiletoggle = XtCreateManagedWidget("newfiletoggle", toggleWidgetClass,
				digitize_panel, Args, ArgCount);
	XtAddCallback(newfiletoggle, XtNcallback, (XtCallbackProc) switch_file_mode,
					(XtPointer) NULL);

	/* label - "Create new file" */

	FirstArg(XtNlabel, "Create new file");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, newfiletoggle);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("appendlabel", labelWidgetClass,
				digitize_panel, Args, ArgCount);

	/* Done button */

	FirstArg(XtNlabel, "Done");
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	done_but = XtCreateManagedWidget("done", commandWidgetClass,
					   digitize_panel, Args, ArgCount);
	XtAddCallback(done_but, XtNcallback, (XtCallbackProc) digitize_panel_done,
					(XtPointer) NULL);
	/* Cancel button */

	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNfromHoriz, done_but);
	NextArg(XtNhorizDistance, 20);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("cancel", commandWidgetClass,
					   digitize_panel, Args, ArgCount);
	XtAddCallback(below, XtNcallback, (XtCallbackProc) digitize_panel_cancel,
					(XtPointer) NULL);
}

static void
digitize_panel_done(void)
{
    XtPopdown(digitize_popup);
    free(prefix_save);
    free(start_save);
    free(suffix_save);
    free(example_save);
}

static void
digitize_panel_cancel(void)
{
    DeclareArgs(2);

    /* restore original values */
    panel_set_value(digitize_file_prefix, prefix_save);
    panel_set_value(digitize_file_seq, start_save);
    panel_set_value(digitize_file_suffix, suffix_save);
    FirstArg(XtNlabel, example_save);
    SetValues(example_filename);
    if (digitize_append_flag != digitize_append_save) {
	digitize_append_flag = digitize_append_save;
	/* and set radio buttons to flag */
	if (digitize_append_flag) {
	    FirstArg(XtNstate, digitize_append_flag);
	    SetValues(appendtoggle);
	    switch_file_mode(appendtoggle, 0, 0);
	} else {
	    FirstArg(XtNstate, !digitize_append_flag);
	    SetValues(newfiletoggle);
	    switch_file_mode(newfiletoggle, 0, 0);
	}
    }
    /* free the strings and pop down the dialog */
    digitize_panel_done();
}

/* this is called each time user changes prefix, starting number, or suffix */

static void
update_example(Widget w, XtPointer info, XtPointer dum)
{
    DeclareArgs(2);
    char	*prefix, *suffix;
    char	 example[100];
    int		 seq;

    prefix = panel_get_value(digitize_file_prefix);
    seq    = atoi(panel_get_value(digitize_file_seq));
    suffix = panel_get_value(digitize_file_suffix);
	
    /* make the sequence have 4 digits */
    sprintf(example,"%s_%04d.%s", prefix, seq, suffix);
    FirstArg(XtNlabel, example);
    SetValues(example_filename);
}

/* when user toggles between append and newfile digitize mode */

static void
switch_file_mode(Widget w, XtPointer closure, XtPointer call_data)
{
    DeclareArgs(5);
    Boolean	    state;
    int		    which;

    /* check state of the toggle and set/remove checkmark */
    FirstArg(XtNstate, &state);
    GetValues(w);
    
    if (state ) {
	FirstArg(XtNbitmap, sm_check_pm);
    } else {
	FirstArg(XtNbitmap, sm_null_check_pm);
    }
    SetValues(w);

    /* set the sensitivity of the toggle button to the opposite of its state
       so that the user must press the other one now */
    XtSetSensitive(w, !state);
    /* and make the *other* button the opposite state */
    if (w == appendtoggle) {
	XtSetSensitive(newfiletoggle, state);
    } else {
	XtSetSensitive(appendtoggle, state);
    }
    /* which button */
    which = (int) XawToggleGetCurrent(w);
    if (which == 0)		/* no buttons on, in transition so return now */
	return;
    if (which == 2)		/* "newfile" button, invert state */
	state = !state;

    /* set global state */
    digitize_append_flag = state;

    return;
}
