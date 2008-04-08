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
#include "object.h"
#include "mode.h"
#include "e_edit.h"
#include "u_create.h"
#include "u_print.h"
#include "w_export.h"
#include "w_print.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_util.h"

#include "f_util.h"
#include "u_bound.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_color.h"
#include "w_cursor.h"

/* EXPORTS */

Widget	print_popup;	/* the main print popup */
Widget	print_panel;	/* the form it's in */
Widget	print_orient_panel;
Widget	print_just_panel;
Widget	print_papersize_panel;
Widget	print_multiple_panel;
Widget	print_overlap_panel;
Widget	printer_menu_button;
Widget	print_mag_text;
Widget	print_background_panel;
void	print_update_figure_size(void);
Boolean	print_all_layers=True;
Widget	print_grid_minor_text, print_grid_major_text;
Widget	print_grid_minor_menu_button, print_grid_minor_menu;
Widget	print_grid_major_menu_button, print_grid_major_menu;
Widget	print_grid_unit_label;

/* LOCAL */

DeclareStaticArgs(15);

static Widget	beside, below;

#define MAX_PRINTERS 1000		/* for those systems using lprng :-) */
static char    *printer_names[MAX_PRINTERS];
static int	parse_printcap(char **names);
static int	numprinters;

static void	orient_select(Widget w, XtPointer new_orient, XtPointer garbage);

static void	just_select(Widget w, XtPointer new_just, XtPointer garbage);
static Widget	just_lab;

static void	papersize_select(Widget w, XtPointer new_papersize, XtPointer garbage);
static Widget	papersize_menu;

static void	background_select(Widget w, XtPointer closure, XtPointer call_data);
static Widget	background_menu;

static void	multiple_select(Widget w, XtPointer new_multiple, XtPointer garbage);
static void	overlap_select(Widget w, XtPointer new_overlap, XtPointer garbage);

static void	printer_select(Widget w, XtPointer new_printer, XtPointer garbage);

static Widget	dismiss, print, 
		printer_text, param_text,
		clear_batch, print_batch, 
		num_batch,
		printalltoggle, printactivetoggle;

static Widget	size_lab;

static void	update_figure_size(void);
static void	fit_page(void);

static Position xposn, yposn;
static String   prn_translations =
        "<Message>WM_PROTOCOLS: DismissPrint()\n";
static void     print_panel_dismiss(Widget w, XButtonEvent *ev), do_clear_batch(Widget w);
static void	get_magnif(void);
static void update_mag(Widget widget, XtPointer *item, XtPointer *event);
void		do_print(Widget w), do_print_batch(Widget w);
static XtCallbackProc switch_print_layers(Widget w, XtPointer closure, XtPointer call_data);

/* callback list to keep track of magnification window */

String  print_translations =
        "<Key>Return: UpdateMag()\n\
	Ctrl<Key>J: UpdateMag()\n\
	Ctrl<Key>M: UpdateMag()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()\n";

static XtActionsRec     prn_actions[] =
{
    {"DismissPrint", (XtActionProc) print_panel_dismiss},
    {"Dismiss", (XtActionProc) print_panel_dismiss},
    {"PrintBatch", (XtActionProc) do_print_batch},
    {"ClearBatch", (XtActionProc) do_clear_batch},
    {"Print", (XtActionProc) do_print},
    {"UpdateMag", (XtActionProc) update_mag},
};


void create_print_panel (Widget w);
void update_batch_count (void);

static void
print_panel_dismiss(Widget w, XButtonEvent *ev)
{
    /* first get magnification in case it changed */
    /* the other things like paper size, justification, etc. are already
       updated because they are from menus */
    get_magnif();
    XtPopdown(print_popup);
}

static char	print_msg[] = "PRINT";

void
do_print(Widget w)
{
	char	   *printer_val;
	char	   *param_val;
	char	    cmd[255],cmd2[255];
	char	   *c1;
	char	    backgrnd[10], grid[80];

	/* don't print if in the middle of drawing/editing */
	if (check_action_on())
		return;

	if (emptyfigure_msg(print_msg) && !batch_exists)
		return;

	/* create popup panel if not already there so we have all the
	   resources necessary (e.g. printer name etc.) */
	if (!print_popup) 
		create_print_panel(w);

	/* get the magnification into appres.magnification */
	get_magnif();

	/* update the figure size (magnification * bounding_box) */
	print_update_figure_size();

	printer_val = panel_get_value(printer_text);
	param_val = panel_get_value(param_text);

	/* get grid params and assemble into fig2dev parm */
	get_grid_spec(grid, print_grid_minor_text, print_grid_major_text);

	if (batch_exists) {
	    gen_print_cmd(cmd,batch_file,printer_val,param_val);
	    if (system(cmd) != 0)
		file_msg("Error during PRINT");
	    /* clear the batch file and the count */
	    do_clear_batch(w);
	} else {
	    strcpy(cmd, param_val);
	    /* see if the user wants the filename in the param list (%f) */
	    if (!strstr(cur_filename,"%f")) {	/* don't substitute if the filename has a %f */
		while (c1=strstr(cmd,"%f")) {
		    strcpy(cmd2, c1+2);		/* save tail */
		    strcpy(c1, cur_filename);	/* change %f to filename */
		    strcat(c1, cmd2);		/* append tail */
		}
	    }
	    /* make a #rrggbb string from the background color */
	    make_rgb_string(export_background_color, backgrnd);
	    print_to_printer(printer_val, backgrnd, appres.magnification,
				print_all_layers, grid, cmd);
	}
}

/* calculate the magnification needed to fit the figure to the page size */

static void
fit_page(void)
{
	int	lx,ly,ux,uy;
	float	wd,ht,pwd,pht;
	char	buf[60];

	/* get current size of figure */
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	wd = ux-lx;
	ht = uy-ly;

	/* if there is no figure, return now */
	if (wd == 0 || ht == 0)
	    return;

	/* get paper size minus a half inch margin all around */
	pwd = paper_sizes[appres.papersize].width - PIX_PER_INCH;
	pht = paper_sizes[appres.papersize].height - PIX_PER_INCH;
	/* swap height and width if landscape */
	if (appres.landscape) {
	    ux = pwd;
	    pwd = pht;
	    pht = ux;
	}
	/* make magnification lesser of ratio of:
	   page height / figure height or 
	   page width/figure width
	*/
	if (pwd/wd < pht/ht)
	    appres.magnification = 100.0*pwd/wd;
	else
	    appres.magnification = 100.0*pht/ht;
	/* adjust for difference in real metric vs "xfig metric" */
	if(!appres.INCHES)
	    appres.magnification *= PIX_PER_CM * 2.54/PIX_PER_INCH;

	/* update the magnification widget */
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);

	/* and figure size */
	update_figure_size();
}

/* get the magnification from the widget and make it reasonable if not */

static void
get_magnif(void)
{
	char buf[60];

	appres.magnification = (float) atof(panel_get_value(print_mag_text));
	if (appres.magnification <= 0.0)
	    appres.magnification = 100.0;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
}

/* as the user types in a magnification, update the figure size */

static void
update_mag(Widget widget, XtPointer *item, XtPointer *event)
{
    char	   *buf;

    buf = panel_get_value(print_mag_text);
    appres.magnification = (float) atof(buf);
    update_figure_size();
}

static void
update_figure_size(void)
{
    char buf[60];

    print_update_figure_size();
    /* update the export panel's indicators too */
    if (export_popup) {
	export_update_figure_size();
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(export_mag_text);
    }
}

static int num_batch_figures=0;
static Boolean writing_batch=False;

void
do_print_batch(Widget w)
{
	FILE	   *infp,*outfp;
	char	    tmp_exp_file[32];
	char	    str[255];
	char	    backgrnd[10], grid[80];

	if (writing_batch || emptyfigure_msg(print_msg))
		return;

	/* set lock so we don't come here while still writing a file */
	/* this could happen if the user presses the button too fast */
	writing_batch = True;

	/* make a temporary name to write the batch stuff to */
	sprintf(batch_file, "%s/%s%06d", TMPDIR, "xfig-batch", getpid());
	/* make a temporary name to write this figure to */
	sprintf(tmp_exp_file, "%s/%s%06d", TMPDIR, "xfig-exp", getpid());
	batch_exists = True;
	if (!print_popup) 
		create_print_panel(w);

	/* get magnification into appres.magnification */
	get_magnif();

	/* update the figure size (magnification * bounding_box) */
	print_update_figure_size();

	/* make a #rrggbb string from the background color */
	make_rgb_string(export_background_color, backgrnd);

	/* get grid params and assemble into fig2dev parm */
	get_grid_spec(grid, print_grid_minor_text, print_grid_major_text);

	print_to_file(tmp_exp_file, "ps", appres.magnification, 0, 0, backgrnd,
				NULL, False, print_all_layers, 0, False, grid, appres.overlap);
	put_msg("Appending to batch file \"%s\" (%s mode) ... done",
		    batch_file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");
	app_flush();		/* make sure message gets displayed */

	/* now append that to the batch file */
	if ((infp = fopen(tmp_exp_file, "rb")) == NULL) {
		file_msg("Error during PRINT - can't open temporary file to read");
		return;
		}
	if ((outfp = fopen(batch_file, "ab")) == NULL) {
		file_msg("Error during PRINT - can't open print file to append");
		return;
		}
	while (fgets(str,255,infp) != NULL)
		(void) fputs(str,outfp);
	fclose(infp);
	fclose(outfp);
	unlink(tmp_exp_file);
	/* count this batch figure */
	num_batch_figures++ ;
	/* and update the label widget */
	update_batch_count();
	/* we're done */
	writing_batch = False;
}

static void
do_clear_batch(Widget w)
{
	unlink(batch_file);
	batch_exists = False;
	num_batch_figures = 0;
	/* update the label widget */
	update_batch_count();
}

/* update the label widget with the current number of figures in the batch file */

void update_batch_count(void)
{
	char	    num[10];

	sprintf(num,"%3d",num_batch_figures);
	FirstArg(XtNlabel,num);
	SetValues(num_batch);
	if (num_batch_figures) {
	    XtSetSensitive(clear_batch, True);
	    FirstArg(XtNlabel, "Print BATCH \nto Printer");
	    SetValues(print);
	} else {
	    XtSetSensitive(clear_batch, False);
	    FirstArg(XtNlabel, "Print FIGURE\nto Printer");
	    SetValues(print);
	}
}

static void
orient_select(Widget w, XtPointer new_orient, XtPointer garbage)
{
    if (appres.landscape != (int) new_orient) {
	change_orient();
	appres.landscape = (int) new_orient;
	/* make sure that paper size is appropriate */
	papersize_select(print_papersize_panel, (XtPointer) appres.papersize, (XtPointer) 0);
    }
}

static void
just_select(Widget w, XtPointer new_just, XtPointer garbage)
{

    FirstArg(XtNlabel, XtName(w));
    SetValues(print_just_panel);
    /* change export justification if it exists */
    if (export_just_panel)
	SetValues(export_just_panel);
    appres.flushleft = (new_just? True: False);
}

static void
papersize_select(Widget w, XtPointer new_papersize, XtPointer garbage)
{
    int papersize = (int) new_papersize;

    FirstArg(XtNlabel, paper_sizes[papersize].fname);
    SetValues(print_papersize_panel);
    /* change export papersize if it exists */
    if (export_papersize_panel)
	SetValues(export_papersize_panel);
    appres.papersize = papersize;
    /* update the red line showing the new page size */
    update_pageborder();
}

static void
multiple_select(Widget w, XtPointer new_multiple, XtPointer garbage)
{
    int multiple = (int) new_multiple;

    FirstArg(XtNlabel, multiple_pages[multiple]);
    SetValues(print_multiple_panel);
    /* change export multiple if it exists */
    if (export_multiple_panel)
	SetValues(export_multiple_panel);
    appres.multiple = (multiple? True : False);
    /* if multiple pages, disable justification (must be flush left) */
    if (appres.multiple) {
	XtSetSensitive(just_lab, False);
	XtSetSensitive(print_just_panel, False);
	XtSetSensitive(print_overlap_panel, True);
	if (export_just_panel) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(export_just_panel, False);
	    if (cur_exp_lang == LANG_PS)
		XtSetSensitive(export_overlap_panel, True);
	}
    } else {
	XtSetSensitive(just_lab, True);
	XtSetSensitive(print_just_panel, True);
	XtSetSensitive(print_overlap_panel, False);
	if (export_just_panel) {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(export_just_panel, True);
	    XtSetSensitive(export_overlap_panel, False);
	}
    }
}

static void
overlap_select(Widget w, XtPointer new_overlap, XtPointer garbage)
{
    int overlap = (int) new_overlap;

    FirstArg(XtNlabel, overlap_pages[overlap]);
    SetValues(print_overlap_panel);
    /* change export overlap if it exists */
    if (export_overlap_panel)
	SetValues(export_overlap_panel);
    appres.overlap = (overlap? True : False);
}

/* user selected a background color from the menu */

static void
background_select(Widget w, XtPointer closure, XtPointer call_data)
{
    Pixel	    bgcolor, fgcolor;

    /* get the colors from the color button just pressed */
    FirstArg(XtNbackground, &bgcolor);
    NextArg(XtNforeground, &fgcolor);
    GetValues(w);

    /* get the colorname from the color button and put it and the colors 
       in the menu button */
    FirstArg(XtNlabel, XtName(w));
    NextArg(XtNbackground, bgcolor);
    NextArg(XtNforeground, fgcolor);
    SetValues(print_background_panel);
    /* update the export panel too if it exists */
    if (export_background_panel)
	SetValues(export_background_panel);
    export_background_color = (int)closure;

    XtPopdown(background_menu);
}

/* come here when user chooses minor grid interval from menu */

void
print_grid_minor_select(Widget w, XtPointer new_grid_choice, XtPointer garbage)
{
    char *val;
    grid_minor = (int) new_grid_choice;

    /* put selected grid value in the text area */
    /* first get rid of any "mm" following the value */
    val = strtok(my_strdup(grid_choices[grid_minor]), " ");
    FirstArg(XtNstring, val);
    SetValues(print_grid_minor_text);
    free(val);
}

/* come here when user chooses major grid interval from menu */

void
print_grid_major_select(Widget w, XtPointer new_grid_choice, XtPointer garbage)
{
    char *val;
    grid_major = (int) new_grid_choice;

    /* put selected grid value in the text area */
    /* first get rid of any "mm" following the value */
    val = strtok(my_strdup(grid_choices[grid_major]), " ");
    FirstArg(XtNstring, val);
    SetValues(print_grid_major_text);
    free(val);
}

static void
printer_select(Widget w, XtPointer new_printer, XtPointer garbage)
{
    int printer = (int) new_printer;

    /* put selected printer in the menu button */
    FirstArg(XtNlabel, printer_names[printer]);
    SetValues(printer_menu_button);
    /* and in the printer text widget */
    FirstArg(XtNstring, printer_names[printer]);
    SetValues(printer_text);
}

/* update the figure size window */

void
print_update_figure_size(void)
{
	float	mult;
	char	*unit;
	char	buf[40];
	int	ux,uy,lx,ly;

	if (!print_popup)
	    return;
	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	sprintf(buf, "Fig Size: %.1f%s x %.1f%s",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	SetValues(size_lab);
}

void
popup_print_panel(Widget w)
{
    char	    buf[30];

    /* turn off Compose key LED */
    setCompLED(0);

    set_temp_cursor(wait_cursor);
    if (print_popup) {
	/* the print popup already exists, but the magnification may have been
	   changed in the export popup */
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
	/* also the figure size (magnification * bounding_box) */
	print_update_figure_size();
	/* now set the color and name in the background button */
	set_but_col(print_background_panel, export_background_color);
	/* and the background color menu */
	XtDestroyWidget(background_menu);
	background_menu = make_color_popup_menu(print_background_panel,
	    				"Background Color", background_select, 
					NO_TRANSP, INCL_BACKG);
    } else {
	create_print_panel(w);
    }
    XtPopup(print_popup, XtGrabNone);
    /* now that the popup is realized, put in the name of the first printer */
    if (printer_names[0] != NULL) {
	FirstArg(XtNlabel, printer_names[0]);
	SetValues(printer_menu_button);
    }
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(print_popup));
    (void) XSetWMProtocols(tool_d, XtWindow(print_popup), &wm_delete_window, 1);
    reset_cursor();

}

/* make the popup print panel */

void create_print_panel(Widget w)
{
	Widget	    image;
	Widget	    entry,mag_spinner, below, fitpage;
	Pixmap	    p;
	unsigned    long fg, bg;
	char	   *printer_val;
	char	    buf[50];
	char	   *unit;
	int	    ux,uy,lx,ly;
	int	    i,len,maxl;
	float	    mult;

	XtTranslateCoords(tool, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn+50);
	NextArg(XtNy, yposn+50);
	NextArg(XtNtitle, "Xfig: Print menu");
	NextArg(XtNcolormap, tool_cm);
	print_popup = XtCreatePopupShell("print_popup",
					 transientShellWidgetClass,
					 tool, Args, ArgCount);
        XtOverrideTranslations(print_popup,
                           XtParseTranslationTable(prn_translations));
        XtAppAddActions(tool_app, prn_actions, XtNumber(prn_actions));

	print_panel = XtCreateManagedWidget("print_panel", formWidgetClass,
					    print_popup, NULL, ZERO);

	/* start with the picture of the printer */

	FirstArg(XtNlabel, "   ");
	NextArg(XtNwidth, printer_ic.width);
	NextArg(XtNheight, printer_ic.height);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	image = XtCreateManagedWidget("printer_image", labelWidgetClass,
				      print_panel, Args, ArgCount);
	FirstArg(XtNforeground, &fg);
	NextArg(XtNbackground, &bg);
	GetValues(image);
	p = XCreatePixmapFromBitmapData(tool_d, XtWindow(canvas_sw),
		      printer_ic.bits, printer_ic.width, printer_ic.height,
		      fg, bg, tool_dpth);
	FirstArg(XtNbitmap, p);
	SetValues(image);

	FirstArg(XtNlabel, "Print to PostScript Printer");
	NextArg(XtNfromHoriz, image);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	(void) XtCreateManagedWidget("print_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	FirstArg(XtNlabel, " Magnification %");
	NextArg(XtNfromVert, image);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("mag_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	/* make a spinner entry for the mag */
	/* note: this was called "magnification" */
	sprintf(buf, "%.1f", appres.magnification);
	mag_spinner = MakeFloatSpinnerEntry(print_panel, &print_mag_text, "magnification",
				image, beside, update_mag, buf, 0.0, 10000.0, 1.0, 45);

	/* we want to track typing here to update figure size label */

	XtOverrideTranslations(print_mag_text,
			       XtParseTranslationTable(print_translations));

	/* Fit Page to the right of the magnification window */

	FirstArg(XtNlabel, "Fit to Page");
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, mag_spinner);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fitpage = XtCreateManagedWidget("fitpage", commandWidgetClass,
				       print_panel, Args, ArgCount);
	XtAddEventHandler(fitpage, ButtonReleaseMask, False,
			  (XtEventHandler)fit_page, (XtPointer) NULL);

	/* Figure Size to the right of the fit page window */

	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	/* get the size of the figure */
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	sprintf(buf, "Fig Size: %.1f%s x %.1f%s      ",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	NextArg(XtNfromVert, image);
	NextArg(XtNfromHoriz, fitpage);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	size_lab = XtCreateManagedWidget("size_label", labelWidgetClass,
					print_panel, Args, ArgCount);

	/* paper size */

	FirstArg(XtNlabel, "      Paper Size");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, fitpage);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("papersize_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, paper_sizes[appres.papersize].fname);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, fitpage);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	NextArg(XtNrightBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_papersize_panel = XtCreateManagedWidget("papersize",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	papersize_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, 
				    print_papersize_panel, NULL, ZERO);

	/* make the menu items */
	for (i = 0; i < XtNumber(paper_sizes); i++) {
	    entry = XtCreateManagedWidget(paper_sizes[i].fname, smeBSBObjectClass, 
					papersize_menu, NULL, ZERO);
	    XtAddCallback(entry, XtNcallback, papersize_select, (XtPointer) i);
	}

	/* Orientation */

	FirstArg(XtNlabel, "     Orientation");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, print_papersize_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("orient_label", labelWidgetClass,
					   print_panel, Args, ArgCount);

	FirstArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, print_papersize_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_orient_panel = XtCreateManagedWidget(orient_items[appres.landscape],
					     menuButtonWidgetClass,
					     print_panel, Args, ArgCount);
	make_pulldown_menu(orient_items, XtNumber(orient_items), -1, "",
				      print_orient_panel, orient_select);

	FirstArg(XtNlabel, "Justification");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, print_orient_panel);
	NextArg(XtNfromVert, print_papersize_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNfromVert, print_papersize_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	make_pulldown_menu(just_items, XtNumber(just_items), -1, "",
				    print_just_panel, just_select);

	/* multiple/single page */

	FirstArg(XtNlabel, "           Pages");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("multiple_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNlabel, multiple_pages[appres.multiple? 1:0]);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_multiple_panel = XtCreateManagedWidget("multiple_pages",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	make_pulldown_menu(multiple_pages, XtNumber(multiple_pages), -1, "",
				    print_multiple_panel, multiple_select);

	FirstArg(XtNlabel, overlap_pages[appres.overlap? 1:0]);
	NextArg(XtNfromHoriz, print_multiple_panel);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_overlap_panel = XtCreateManagedWidget("overlap_pages",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	make_pulldown_menu(overlap_pages, XtNumber(overlap_pages), -1, "",
				    print_overlap_panel, overlap_select);

	/* background color */

	FirstArg(XtNlabel, "Background");
	NextArg(XtNfromHoriz, print_overlap_panel);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("background_label", labelWidgetClass,
					 print_panel, Args, ArgCount);

	FirstArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, print_just_panel);
	NextArg(XtNresize, False);
	NextArg(XtNwidth, 80);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_background_panel = XtCreateManagedWidget("background",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);

	/* now set the color and name in the background button */
	set_but_col(print_background_panel, export_background_color);

	/* make color menu */
	background_menu = make_color_popup_menu(print_background_panel, 
					"Background Color", background_select,
					NO_TRANSP, INCL_BACKG);
	/* grid options */
	FirstArg(XtNlabel, "            Grid");
	NextArg(XtNfromVert, print_background_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("grid_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	below = make_grid_options(print_panel, print_background_panel, beside, minor_grid, major_grid,
				&print_grid_minor_menu_button, &print_grid_major_menu_button,
				&print_grid_minor_menu, &print_grid_major_menu,
				&print_grid_minor_text, &print_grid_major_text,
				&print_grid_unit_label,
				print_grid_major_select, print_grid_minor_select);

	/* printer name */

	FirstArg(XtNlabel, "         Printer");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("printer_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * printer in a resource, e.g.:	 *printer*string: at6
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	printer_text = XtCreateManagedWidget("printer", asciiTextWidgetClass,
					     print_panel, Args, ArgCount);

	XtOverrideTranslations(printer_text,
			       XtParseTranslationTable(print_translations));

	/* put the printer name in the label if resource isn't set */
	FirstArg(XtNstring, &printer_val);
	GetValues(printer_text);
	/* no printer name specified in resources, get PRINTER environment
	   var and put it into the widget */
	if (emptyname(printer_val)) {
		printer_val=getenv("PRINTER");
		if (printer_val == NULL) {
			printer_val = "";
		} else {
			FirstArg(XtNstring, printer_val);
			SetValues(printer_text);
		}
	}
	/* parse /etc/printcap for printernames for the pull-down menu */
	numprinters = parse_printcap(printer_names);
	/* find longest name */
	maxl = 0;
	for (i=0; i<numprinters; i++) {
	    len=strlen(printer_names[i]);
	    if (len > maxl) {
		maxl = len;
	    }
	}
	/* make string of blanks the length of the longest printer name */
	buf[0] = '\0';
	for (i=0; i<maxl; i++)
	    strcat(buf," ");
	if (numprinters > 0) {
	    FirstArg(XtNlabel, buf);
	    NextArg(XtNfromHoriz, printer_text);
	    NextArg(XtNfromVert, below);
	    NextArg(XtNborderWidth, INTERNAL_BW);
	    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	    NextArg(XtNtop, XtChainTop);
	    NextArg(XtNbottom, XtChainTop);
	    NextArg(XtNleft, XtChainLeft);
	    NextArg(XtNright, XtChainLeft);
	    printer_menu_button = XtCreateManagedWidget("printer_names",
					   menuButtonWidgetClass,
					   print_panel, Args, ArgCount);
	    make_pulldown_menu(printer_names, numprinters, -1, "",
				    printer_menu_button, printer_select);
	}

	FirstArg(XtNlabel, "Print Job Params");
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("job_params_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	/*
	 * don't SetValue the XtNstring so the user may specify the default
	 * job parameters in a resource, e.g.:	 *param*string: -K2
	 */

	FirstArg(XtNwidth, 100);
	NextArg(XtNfromVert, printer_text);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	param_text = XtCreateManagedWidget("job_params", asciiTextWidgetClass,
					     print_panel, Args, ArgCount);

	XtOverrideTranslations(param_text,
			       XtParseTranslationTable(print_translations));

	FirstArg(XtNlabel, "Figures in batch");
	NextArg(XtNfromVert, param_text);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = XtCreateManagedWidget("num_batch_label", labelWidgetClass,
					    print_panel, Args, ArgCount);
	FirstArg(XtNwidth, 30);
	NextArg(XtNlabel, "  0");
	NextArg(XtNfromVert, param_text);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	num_batch = XtCreateManagedWidget("num_batch", labelWidgetClass,
					     print_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Dismiss");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	dismiss = XtCreateManagedWidget("dismiss", commandWidgetClass,
				       print_panel, Args, ArgCount);
	XtAddEventHandler(dismiss, ButtonReleaseMask, False,
			  (XtEventHandler)print_panel_dismiss, (XtPointer) NULL);

	/* radio for printing all layers */

	beside = make_layer_choice("Print all layers ", "Print only active",
				print_panel, num_batch, dismiss, 6, 6);

	/* print buttons */

	FirstArg(XtNlabel, "Print FIGURE\nto Printer");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNresize, False);	/* must not allow resize because the label changes */
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print = XtCreateManagedWidget("print", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(print, ButtonReleaseMask, False,
			  (XtEventHandler)do_print, (XtPointer) NULL);

	FirstArg(XtNlabel, "Print FIGURE\nto Batch");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, print);
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	print_batch = XtCreateManagedWidget("print_batch", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(print_batch, ButtonReleaseMask, False,
			  (XtEventHandler)do_print_batch, (XtPointer) NULL);

	FirstArg(XtNlabel, "Clear\nBatch");
	NextArg(XtNfromVert, num_batch);
	NextArg(XtNfromHoriz, print_batch);
	NextArg(XtNheight, 35);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 10);
	NextArg(XtNhorizDistance, 6);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	clear_batch = XtCreateManagedWidget("clear_batch", commandWidgetClass,
				      print_panel, Args, ArgCount);
	XtAddEventHandler(clear_batch, ButtonReleaseMask, False,
			  (XtEventHandler)do_clear_batch, (XtPointer) NULL);

	/* install accelerators for the following functions */
	XtInstallAccelerators(print_panel, dismiss);
	XtInstallAccelerators(print_panel, print_batch);
	XtInstallAccelerators(print_panel, clear_batch);
	XtInstallAccelerators(print_panel, print);
	update_batch_count();

	/* if multiple pages is on, desensitive justification panels */
	if (appres.multiple) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(print_just_panel, False);
	    if (export_just_panel) {
	        XtSetSensitive(just_lab, False);
	        XtSetSensitive(export_just_panel, False);
	    }
	} else {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(print_just_panel, True);
	    if (export_just_panel) {
	        XtSetSensitive(just_lab, True);
	        XtSetSensitive(export_just_panel, True);
	    }
	}
}

/* when user toggles between printing all or only active layers */

static XtCallbackProc
switch_print_layers(Widget w, XtPointer closure, XtPointer call_data)
{
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
    if (w == printalltoggle) {
	XtSetSensitive(printactivetoggle, state);
    } else {
	XtSetSensitive(printalltoggle, state);
    }
    /* which button */
    which = (int) XawToggleGetCurrent(w);
    if (which == 0)		/* no buttons on, in transition so return now */
	return;
    if (which == 2)		/* "blank" button, invert state */
	state = !state;

    /* set global state */
    print_all_layers = state;

    return;
}

/* if the users's sytem doesn't have an /etc/printcap file, this will return 0 */

static int
parse_printcap(char **names)
{
    FILE   *printcap;
    char    str[300];
    int     i,j,k,len;
    int     printers;
    Boolean comment;
    Boolean dudprinter;

    if ((printcap=fopen(PRINTCAP,"r"))==NULL)
	return 0;
    printers = 0;
    while (!feof(printcap)) {
	if (fgets(str, sizeof(str), printcap) == NULL)
	    break;
	len = strlen(str);
	comment = False;
	/* get rid of newline */
	str[--len] = '\0';
	/* check for comments */
	for (i=0; i<len; i++) {
	    if (str[i] == '#') {
		comment = True;
		break;
	    }
	    if (str[i] != ' ' && str[i] != '\t')
		break;
	}
	/* skip comment */
	if (comment)
	    continue;
	/* skip blank line */
	if (i==len)
	    continue;
	/* get printer name */
	for (j=i; j<len; j++) {
	    if (str[j] == '|' || str[j] == ':' || str[j] == ' ')
		break;
	}
	str[j] = '\0';
        /* Check for empty printer name or duplicate name */
        dudprinter = True;
        for (k=0; k<j; k++) {
            if(str[k] !=' ' && str[k] != '\t')
               dudprinter = False;
        }
        if(printers > 0) {
            for (k=0; k<printers; k++) {
                if(strncmp(names[k],&str[i],j-i+1) == 0)
                    dudprinter = True;
            }
        }
        if (dudprinter == True)
            continue;
	if (printers >= MAX_PRINTERS) {
	    file_msg("Maximum number of printers (%d) exceeded in %s",MAX_PRINTERS,PRINTCAP);
	    break;
	}
	if ((names[printers] = new_string(j-i)) == NULL) {
	    file_msg("Out of memory while getting printer names");
	    fclose(printcap);
	    break;
	}
	strncpy(names[printers],&str[i],j-i+1);
	printers++;
	for (j=len-1; j>0; j--) {
	    if (str[j] == ' ' || str[j] == '\t')
		continue;
	    /* found the next entry, break */
	    if (str[j] != '\\')
		break;
	    /* this line has \ at the end, read the next line and check it */
	    if (fgets(str, sizeof(str), printcap) == NULL)
		break;
	    /* set length to ignore newline */
	    len = strlen(str)-1;
	    /* force loop to start over */
	    j=len;
	}
    }
    fclose(printcap);
    return printers;
}

Widget
make_layer_choice(char *label_all, char *label_active, Widget parent, Widget below, Widget beside, int hdist, int vdist)
{
	Widget	 form;

	FirstArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, vdist);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, hdist);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	form = XtCreateManagedWidget("layer_choice_form", formWidgetClass,
				parent, Args, ArgCount);

	FirstArg(XtNbitmap, (print_all_layers? sm_check_pm : sm_null_check_pm));
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNlabel, "  ");
	NextArg(XtNsensitive, (print_all_layers? False : True)); /* make opposite button sens */
	NextArg(XtNstate, print_all_layers); 	/* initial state */
	NextArg(XtNradioData, 1);		/* when this is pressed the value is 1 */
	printalltoggle = XtCreateManagedWidget("printalltoggle", toggleWidgetClass,
				form, Args, ArgCount);
	XtAddCallback(printalltoggle, XtNcallback, (XtCallbackProc) switch_print_layers,
					(XtPointer) NULL);

	/* label - " XXXX all layers" */

	FirstArg(XtNlabel, label_all);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromHoriz, printalltoggle);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("print_all_layers", labelWidgetClass,
				form, Args, ArgCount);

	/* radio for printing only active layers */

	FirstArg(XtNbitmap, (print_all_layers? sm_null_check_pm : sm_check_pm));
	NextArg(XtNfromVert, printalltoggle);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNlabel, "  ");
	NextArg(XtNsensitive, (print_all_layers? True : False)); /* make opposite button sens */
	NextArg(XtNstate, !print_all_layers);	/* initial state */
	NextArg(XtNradioData, 2);		/* when this is pressed the value is 2 */
	NextArg(XtNradioGroup, printalltoggle);	/* this is the other radio button in the group */
	printactivetoggle = XtCreateManagedWidget("printactivetoggle", toggleWidgetClass,
				form, Args, ArgCount);
	XtAddCallback(printactivetoggle, XtNcallback, (XtCallbackProc) switch_print_layers,
					(XtPointer) NULL);

	/* label - "XXXX only active" */

	FirstArg(XtNlabel, label_active);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, printalltoggle);
	NextArg(XtNfromHoriz, printactivetoggle);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);	/* make it stay on left side */
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("print_active_layers", labelWidgetClass,
				form, Args, ArgCount);

	return form;
}
