/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2002 by Brian V. Smith
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
#include "e_edit.h"
#include "f_util.h"
#include "u_print.h"
#include "w_dir.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_export.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_print.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_icons.h"

#include "u_bound.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_color.h"
#include "w_cursor.h"

/* EXPORTS */

char	default_export_file[PATH_MAX];

Widget	export_popup;	/* the main export popup */
Widget	export_panel;	/* the form it's in */
Widget	exp_selfile,	/* output (selected) file widget */
	exp_mask,	/* mask widget */
	exp_dir,	/* current directory widget */
	exp_flist,	/* file list widget */
	exp_dlist;	/* dir list widget */

Boolean	export_up = False;

Widget	top_section, bottom_section, postscript_form, bitmap_form;
Widget  ps_form_label;
Widget	export_orient_panel;
Widget	export_just_panel;
Widget	export_papersize_panel;
Widget	export_multiple_panel;
Widget	export_overlap_panel;
Widget	export_mag_text;
Widget	mag_spinner;
void	export_update_figure_size(void);
Widget	export_transp_panel;
Widget	export_background_panel;

Widget	export_grid_minor_text, export_grid_major_text;
Widget	export_grid_minor_menu_button, export_grid_minor_menu;
Widget	export_grid_major_menu_button, export_grid_major_menu;
Widget	export_grid_unit_label;

/* LOCAL */

Widget	export_grid_label;

/* these are in fig2dev print units (1/72 inch) */

static float    offset_unit_conv[] = { 72.0, 72.0/2.54, 72.0/PIX_PER_INCH };

/* the bounding box of the figure when the export panel was popped up */
static int	ux,uy,lx,ly;

static String	file_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	<Btn1Up>(2): Export()\n\
	<Key>Return: ExportFile()";
static String	file_name_translations =
	"<Key>Return: ExportFile()\n\
	<Key>Escape: CancelExport()";

void		do_export(Widget w);
static XtActionsRec	file_name_actions[] =
{
    {"ExportFile", (XtActionProc) do_export},
};
static void     export_panel_cancel(Widget w, XButtonEvent *ev);

/* callback list to keep track of magnification window */

static void update_mag(Widget widget, Widget *item, int *event);

String  exp_translations =
        "<Message>WM_PROTOCOLS: DismissExport()";

String  export_translations =
        "<Key>Return: UpdateMag()\n\
	Ctrl<Key>J: UpdateMag()\n\
	Ctrl<Key>M: UpdateMag()\n\
	Ctrl<Key>X: EmptyTextKey()\n\
	Ctrl<Key>U: multiply(4)\n\
	<Key>F18: PastePanelKey()";

static XtActionsRec     export_actions[] =
{
    {"DismissExport", (XtActionProc) export_panel_cancel},
    {"CancelExport", (XtActionProc) export_panel_cancel},
    {"Export", (XtActionProc) do_export},
    {"UpdateMag", (XtActionProc) update_mag},
};

static char	*smooth_choices[] = {
		    " No smoothing ",
		    "Some smoothing",
		    "More smoothing"};

static char	named_file[60];

static void	orient_select(Widget w, XtPointer client_data, XtPointer call_data);

static void	lang_select(Widget w, XtPointer new_lang, XtPointer call_data);
static Widget	lang_panel, lang_lab;

static Widget	layer_choice;

static Widget	border_lab, border_text, border_spinner;

static void	background_select(Widget w, XtPointer client_data, XtPointer call_data);
static Widget	background_lab, background_menu;

static void	transp_select(Widget w, XtPointer client_data, XtPointer call_data);
static Widget	transp_lab, transp_menu;

static Widget	quality_lab, quality_text, quality_spinner;

static Widget	smooth_menu_button;
static Widget	smooth_lab;

static Widget	orient_lab;

static void	just_select(Widget w, XtPointer client_data, XtPointer call_data);
static Widget	just_lab;

static void	papersize_select(Widget w, XtPointer client_data, XtPointer call_data);
static Widget	papersize_lab;

static void	multiple_select(Widget w, XtPointer client_data, XtPointer call_data);
static Widget	multiple_lab;
static void	overlap_select(Widget w, XtPointer client_data, XtPointer call_data);

static void	get_magnif(void);
static void	get_quality(void);

static void	update_figure_size(void);
static Widget	fitpage;
static void	fit_page(void);

static Widget	cancel_but, export_but;
static Widget	dfile_lab, dfile_text, nfile_lab;
static Widget	mag_lab;
static Widget	size_lab;
static Widget	exp_off_lab, exp_xoff_lab, exp_yoff_lab;
static Widget	export_offset_x, export_offset_y;
static Widget	exp_xoff_unit_panel;
static Widget	exp_yoff_unit_panel;

static Widget   pcl_cmd_label;
static Widget   pcl_cmd_checkbox;
static Widget   hpgl_font_label;
static Widget   hpgl_font_checkbox;

static int	xoff_unit_setting, yoff_unit_setting;

DeclareStaticArgs(15);


void create_export_panel (Widget w);
void manage_optional (void);
void set_export_mask (int lang);

static void
export_panel_dismiss(void)
{
    /* first get magnification from the widget into appres.magnification
       in case it changed */
    /* the other things like paper size, justification, etc. are already
       updated because they are from menus */
    get_magnif();

    /* get the image quality value too */
    get_quality();

    FirstArg(XtNstring, "\0");
    SetValues(exp_selfile);		/* clear ascii widget string */
    XtPopdown(export_popup);
    export_up = popup_up = False;
}

static void
export_panel_cancel(Widget w, XButtonEvent *ev)
{
    export_panel_dismiss();
}

/* get x/y offsets from panel and convert to 1/72 inch for fig2dev */

void exp_getxyoff(int *ixoff, int *iyoff)
{
    float xoff, yoff;
    *ixoff = *iyoff = 0;
    /* if no file panel yet, use 0, 0 for offsets */
    if (export_offset_x == (Widget) 0 ||
	export_offset_y == (Widget) 0)
	    return;

    sscanf(panel_get_value(export_offset_x),"%f",&xoff);
    *ixoff = round(xoff*offset_unit_conv[xoff_unit_setting]);
    sscanf(panel_get_value(export_offset_y),"%f",&yoff);
    *iyoff = round(yoff*offset_unit_conv[yoff_unit_setting]);
}


static char	export_msg[] = "EXPORT";
static char	exp_msg[] = "The current figure is modified.\nDo you want to save it before exporting?";

void
do_export(Widget w)
{
	char	   *fval;
	int	    xoff, yoff;
	F_line     *l;
	char	    transparent[10], backgrnd[10], grid[80];
	char	    msg[200];
	int	    transp;
	int	    border;
	Boolean	    use_transp_backg;

	/* don't export if in the middle of drawing/editing */
	if (check_action_on())
		return;

	/* if the popup has been created, get magnification and jpeg quality
	   here to fix any bad values */
	if (export_popup) {
	    /* get the magnification into appres.magnification */
	    get_magnif();

	    /* get the image quality value too */
	    get_quality();
	}

	if (emptyfigure_msg(export_msg))
		return;

	/* update figure size window */
	export_update_figure_size();

	/* if modified (and non-empty) ask to save first */
	if (!query_save(exp_msg))
	    return;		/* cancel, do not export */

	if (!export_popup) 
		create_export_panel(w);

	/* if there is no default export name (e.g. if user has done "New" and not 
		   entered a name) then make one up */
	if (!default_export_file || default_export_file[0] == '\0') {
	    /* for tiff and jpeg use three-letter suffixes */
	    if (cur_exp_lang==LANG_TIFF)
		sprintf(default_export_file,"NoName.tif");
	    else if (cur_exp_lang==LANG_JPEG)
		sprintf(default_export_file,"NoName.jpg");
	    else if (cur_exp_lang==LANG_PDFTEX)
		/* for pdftex, just use .pdf */
		sprintf(default_export_file,"NoName.pdf");
	    else
		sprintf(default_export_file,"NoName.%s",lang_items[cur_exp_lang]);
	}

	fval = panel_get_value(exp_selfile);
	if (emptyname(fval)) {		/* output filename is empty, use default */
	    fval = default_export_file;
	    warnexist = False;		/* don't warn if this file exists */
	} else if (strcmp(fval,default_export_file) != 0) {
	    warnexist = True;		/* warn if the file exists and is diff. from default */
	}
	
	/* get the magnification into appres.magnification */
	get_magnif();

	/* get the image quality value too */
	get_quality();

	/* get any grid spec */
	get_grid_spec(grid, export_grid_minor_text, export_grid_major_text);

	/* make sure that the export file isn't one of the picture files (e.g. xxx.eps) */
	for (l = objects.lines; l != NULL; l = l->next) {
	    if (l->type == T_PICTURE && l->pic->pic_cache) {       
	       if (strcmp(fval,l->pic->pic_cache->file) == 0) {
		    beep();
		    sprintf(msg, "\"%s\" is an input file.\nExport aborted", fval);
		    (void) popup_query(QUERY_OK, msg);
		    return;
	       }
	     }
	 }         
	
	/* if not absolute path, change directory */
	if (*fval != '/') {
	    if (change_directory(cur_export_dir) != 0)
		return;
	}

	/* make the export button insensitive during the export */
	XtSetSensitive(export_but, False);
	app_flush();

	exp_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */

	use_transp_backg = False;
	/* convert the transparent color to an RGB hex value */
	if (cur_exp_lang == LANG_GIF) {
	    /* use the mapped color for the transparent color */
	    /* the color was mapped from  Fig.color -> X.color -> mapped.color */
	    transp = appres.transparent;
	    /* make background transparent */
	    if (transp == TRANSP_BACKGROUND) {
		use_transp_backg = True;
		sprintf(transparent,"#%02x%02x%02x",
				x_bg_color.red>>8,
				x_bg_color.green>>8,
				x_bg_color.blue>>8);
	    /* make other color transp */
	    } else {
		XColor  col;
		col.pixel = x_color(transp);
		XQueryColor(tool_d, tool_cm, &col);
		sprintf(transparent,"#%02x%02x%02x",
				col.red>>8, col.green>>8, col.blue>>8);
		use_transp_backg = False;
	    }
	}
	/* make a #rrggbb string from the background color */
	make_rgb_string(export_background_color, backgrnd);

	/* get margin width from the panel */
	sscanf(panel_get_value(border_text), "%d", &border);
	/* update appres */
	appres.export_margin = border;

	/* call fig2dev to export the file */
	if (print_to_file(fval, lang_items[cur_exp_lang],
			      appres.magnification, xoff, yoff, backgrnd,
			      (transp == TRANSP_NONE? NULL: transparent),
			      use_transp_backg, print_all_layers,
			      border, appres.smooth_factor, grid, appres.overlap) == 0) {
		FirstArg(XtNlabel, fval);
		SetValues(dfile_text);		/* set the default filename */
		if (strcmp(fval,default_export_file) != 0)
		    strcpy(default_export_file,fval); /* and copy to default */
		export_panel_dismiss();
	}

	XtSetSensitive(export_but, True);
}

static void
orient_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (appres.landscape != (int) client_data) {
	change_orient();
	appres.landscape = (int) client_data;
	/* make sure that paper size is appropriate */
	papersize_select(export_papersize_panel, (XtPointer) appres.papersize, (XtPointer) 0);
    }
}

static void
just_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(export_just_panel);
    /* change print justification if it exists */
    if (print_just_panel)
	SetValues(print_just_panel);
    appres.flushleft = (client_data? True: False);
}

static void
papersize_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    int papersize = (int) client_data;

    FirstArg(XtNlabel, paper_sizes[papersize].fname);
    SetValues(export_papersize_panel);
    /* change print papersize if it exists */
    if (print_papersize_panel)
	SetValues(print_papersize_panel);
    appres.papersize = papersize;
    /* update the red line showing the new page size */
    update_pageborder();
}

static void
multiple_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    int multiple = (int) client_data;

    FirstArg(XtNlabel, multiple_pages[multiple]);
    SetValues(export_multiple_panel);
    /* change print multiple if it exists */
    if (print_multiple_panel)
	SetValues(print_multiple_panel);
    appres.multiple = (multiple? True : False);
    /* if multiple pages, disable justification (must be flush left) */
    if (appres.multiple) {
	XtSetSensitive(just_lab, False);
	XtSetSensitive(export_just_panel, False);
	if (cur_exp_lang == LANG_PS)
	    XtSetSensitive(export_overlap_panel, True);
	if (print_just_panel) {
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(print_just_panel, False);
	    XtSetSensitive(print_overlap_panel, True);
	}
    } else {
	XtSetSensitive(just_lab, True);
	XtSetSensitive(export_just_panel, True);
	XtSetSensitive(export_overlap_panel, False);
	if (print_just_panel) {
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(print_just_panel, True);
	    XtSetSensitive(print_overlap_panel, False);
	}
    }
}

static void
overlap_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    int overlap = (int) client_data;

    FirstArg(XtNlabel, overlap_pages[overlap]);
    SetValues(export_overlap_panel);
    /* change print overlap if it exists */
    if (print_overlap_panel)
	SetValues(print_overlap_panel);
    appres.overlap = (overlap? True : False);
}

static void
set_postscript_options(void)
{
	/* enable or disable features available for PostScript */

	/* if multiple pages, enable overlap and disable justification (must be flush left) */
	if (appres.multiple) {
	    XtSetSensitive(export_overlap_panel, True);
	    XtSetSensitive(just_lab, False);
	    XtSetSensitive(export_just_panel, False);
	    if (print_just_panel) {
		XtSetSensitive(just_lab, False);
		XtSetSensitive(print_just_panel, False);
	    }
	} else {
	    XtSetSensitive(export_overlap_panel, False);
	    XtSetSensitive(just_lab, True);
	    XtSetSensitive(export_just_panel, True);
	    if (print_just_panel) {
		XtSetSensitive(just_lab, True);
		XtSetSensitive(print_just_panel, True);
	    }
	}
}

static void
lang_select(Widget w, XtPointer new_lang, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(lang_panel);
    cur_exp_lang = (int) new_lang;

    /* enable or disable features available for PostScript or other languages */
    if (cur_exp_lang == LANG_PS)
	set_postscript_options();

    /* manage optional buttons/menus etc */
    manage_optional();

    update_def_filename();
    FirstArg(XtNlabel, default_export_file);
    SetValues(dfile_text);
    /* set the new wildcard mask based on the current export language */
    set_export_mask(cur_exp_lang);
}

/***
   set the new export mask based on the language passed.
   e.g. make *.jpg for jpeg output
***/

void set_export_mask(int lang)
{
    char	    mask[100];

    /* make wildcard mask based on filename suffix for output language */
    /* for tiff and jpeg use three-letter suffixes */
    if (cur_exp_lang==LANG_TIFF)
	strcpy(mask, "*.tif");
    else if (cur_exp_lang==LANG_JPEG)
	strcpy(mask, "*.jpg");
    else if (cur_exp_lang==LANG_PDFTEX)
	/* for pdftex, just use .pdf */
	strcpy(mask, "*.pdf");
    else if (cur_exp_lang == LANG_EPS_ASCII ||
	     cur_exp_lang == LANG_EPS_MONO_TIFF ||
	     cur_exp_lang == LANG_EPS_COLOR_TIFF)
	/* just .eps for all eps files */
	strcpy(mask, "*.eps");
    else
	sprintf(mask,"*.%s",lang_items[lang]);
    FirstArg(XtNstring, mask);
    SetValues(exp_mask);

    /* rescan current directory */
    Rescan(0, 0, 0, 0);
}

/* user has chosen a smooth factor from the pulldown menu */

static void
smooth_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    int new_smooth = (int) client_data;
    appres.smooth_factor = new_smooth==0? 1: new_smooth*2;
    FirstArg(XtNlabel, smooth_choices[new_smooth]);
    SetValues(smooth_menu_button);
}

/* user selected a background color from the menu */

static void
background_select(Widget w, XtPointer client_data, XtPointer call_data)
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
    SetValues(export_background_panel);
    /* update the print panel too if it exists */
    if (print_background_panel)
	SetValues(print_background_panel);
    export_background_color = (int)client_data;

    XtPopdown(background_menu);
}

/* user selected a transparent color from the menu */

static void
transp_select(Widget w, XtPointer client_data, XtPointer call_data)
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
    SetValues(export_transp_panel);
    appres.transparent = (int)client_data;

    XtPopdown(transp_menu);
}

/* come here when user chooses minor grid interval from menu */

void
export_grid_minor_select(Widget w, XtPointer new_grid_choice, XtPointer call_data)
{
    char *val;
    grid_minor = (int) new_grid_choice;

    /* put selected grid value in the text area */
    /* first get rid of any "mm" following the value */
    val = strtok(my_strdup(grid_choices[grid_minor]), " ");
    FirstArg(XtNstring, val);
    SetValues(export_grid_minor_text);
    free(val);
}

/* come here when user chooses major grid interval from menu */

void
export_grid_major_select(Widget w, XtPointer new_grid_choice, XtPointer call_data)
{
    char *val;
    grid_major = (int) new_grid_choice;

    /* put selected grid value in the text area */
    /* first get rid of any "mm" following the value */
    val = strtok(my_strdup(grid_choices[grid_major]), " ");
    FirstArg(XtNstring, val);
    SetValues(export_grid_major_text);
    free(val);
}


/* enable/disable optional buttons etc depending on export language */

void manage_optional(void)
{
	Widget below;

	/* if language is GIF, enable the transparent color choices */
	if (cur_exp_lang == LANG_GIF) {
	    XtManageChild(transp_lab);
	    XtManageChild(export_transp_panel);
	} else {
	    XtUnmanageChild(transp_lab);
	    XtUnmanageChild(export_transp_panel);
	}

	/* if the export language is JPEG, manage the image quality choices */
	if (cur_exp_lang == LANG_JPEG) {
	    XtManageChild(quality_lab);
	    XtManageChild(quality_spinner);
	} else {
	    XtUnmanageChild(quality_lab);
	    XtUnmanageChild(quality_spinner);
	}

	/* if language is a bitmap format or PS/EPS/PDF/PSTEX/PSPDF or MAP,
	   manage the border margin stuff */
	if (cur_exp_lang == LANG_PS || cur_exp_lang == LANG_EPS ||
		cur_exp_lang == LANG_EPS_ASCII || 
		cur_exp_lang == LANG_EPS_MONO_TIFF ||
		cur_exp_lang == LANG_EPS_COLOR_TIFF || 
		cur_exp_lang == LANG_PDF || cur_exp_lang == LANG_PSTEX || cur_exp_lang == LANG_PSPDF || 
		cur_exp_lang == LANG_MAP || cur_exp_lang >= FIRST_BITMAP_LANG) {
	    XtManageChild(border_lab);
	    XtManageChild(border_spinner);
	} else {
	    XtUnmanageChild(border_lab);
	    XtUnmanageChild(border_spinner);
	}

	/* if language is a bitmap format, tk or PS/EPS/PDF/PSTEX/PSPDF manage the background color and grid */
	if (cur_exp_lang == LANG_PS || cur_exp_lang == LANG_EPS || cur_exp_lang == LANG_EPS_ASCII || 
		cur_exp_lang == LANG_EPS_MONO_TIFF || cur_exp_lang == LANG_EPS_COLOR_TIFF || 
		cur_exp_lang == LANG_PDF || cur_exp_lang == LANG_PSTEX || cur_exp_lang == LANG_TK ||
		cur_exp_lang == LANG_PSPDF || cur_exp_lang >= FIRST_BITMAP_LANG) {
	    XtManageChild(background_lab);
	    XtManageChild(export_background_panel);
	} else {
	    XtUnmanageChild(background_lab);
	    XtUnmanageChild(export_background_panel);
	}
	/* for all of the above *EXCEPT* tk, manage grid options */
	if (cur_exp_lang == LANG_PS || cur_exp_lang == LANG_EPS || cur_exp_lang == LANG_EPS_ASCII || 
		cur_exp_lang == LANG_EPS_MONO_TIFF || cur_exp_lang == LANG_EPS_COLOR_TIFF || 
		cur_exp_lang == LANG_PDF || cur_exp_lang == LANG_PSTEX || cur_exp_lang == LANG_PSPDF ||
		cur_exp_lang >= FIRST_BITMAP_LANG) {
	    XtManageChild(export_grid_label);
	    XtManageChild(export_grid_minor_menu_button);
	    XtManageChild(export_grid_minor_text);
	    XtManageChild(export_grid_major_menu_button);
	    XtManageChild(export_grid_major_text);
	    XtManageChild(export_grid_unit_label);
	} else {
	    XtUnmanageChild(export_grid_label);
	    XtUnmanageChild(export_grid_minor_menu_button);
	    XtUnmanageChild(export_grid_minor_text);
	    XtUnmanageChild(export_grid_major_menu_button);
	    XtUnmanageChild(export_grid_major_text);
	    XtUnmanageChild(export_grid_unit_label);
	}

	/* now manage either the PostScript form, the Bitmap form or neither */
	if (cur_exp_lang == LANG_PS) {
	    XtUnmanageChild(postscript_form);
	    FirstArg(XtNlabel, "Postscript Options");
	    SetValues(ps_form_label);
	    XtManageChild(multiple_lab);
	    XtManageChild(export_multiple_panel);
	    XtManageChild(export_overlap_panel);
	    FirstArg(XtNfromVert, export_multiple_panel);
	    SetValues(exp_off_lab);
	    SetValues(exp_xoff_lab);
	    SetValues(export_offset_x);
	    SetValues(exp_xoff_unit_panel);
	    SetValues(exp_yoff_lab);
	    SetValues(export_offset_y);
	    SetValues(exp_yoff_unit_panel);
	    XtUnmanageChild(pcl_cmd_label);
	    XtUnmanageChild(pcl_cmd_checkbox);
	    XtUnmanageChild(hpgl_font_label);
	    XtUnmanageChild(hpgl_font_checkbox);

	    XtManageChild(postscript_form);
	    XtUnmanageChild(bitmap_form);
	    below = postscript_form;
	} else if (cur_exp_lang == LANG_IBMGL) {
	    /* Use all of the Postscript form except the multiple page options
	     */
	    XtUnmanageChild(postscript_form);
	    FirstArg(XtNlabel, "HPGL Options");
	    SetValues(ps_form_label);
	    XtUnmanageChild(multiple_lab);
	    XtUnmanageChild(export_multiple_panel);
	    XtUnmanageChild(export_overlap_panel);
	    FirstArg(XtNfromVert, export_orient_panel);
	    SetValues(exp_off_lab);
	    SetValues(exp_xoff_lab);
	    SetValues(export_offset_x);
	    SetValues(exp_xoff_unit_panel);
	    SetValues(exp_yoff_lab);
	    SetValues(export_offset_y);
	    SetValues(exp_yoff_unit_panel);
	    XtManageChild(pcl_cmd_label);
	    XtManageChild(pcl_cmd_checkbox);
	    XtManageChild(hpgl_font_label);
	    XtManageChild(hpgl_font_checkbox);

	    XtManageChild(postscript_form);
	    XtUnmanageChild(bitmap_form);
	    below = postscript_form;
	} else if (cur_exp_lang >= FIRST_BITMAP_LANG) {
	    XtManageChild(bitmap_form);
	    XtUnmanageChild(postscript_form);
	    below = bitmap_form;
	} else {
	    XtUnmanageChild(postscript_form);
	    XtUnmanageChild(bitmap_form);
	    below = top_section;
	}

	/* reposition bottom section */
	FirstArg(XtNfromVert, below);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	SetValues(bottom_section);
	XtManageChild(bottom_section);
}

/* update the figure size window */

void
export_update_figure_size(void)
{
	float	mult;
	char	*unit;
	char	buf[40];

	if (!export_popup)
	    return;
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	sprintf(buf, "Figure size: %.1f%s x %.1f%s",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	FirstArg(XtNlabel, buf);
	SetValues(size_lab);
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
	SetValues(export_mag_text);

	/* and figure size */
	update_figure_size();
}

/* get the magnification from the widget and make it reasonable if not */

static void
get_magnif(void)
{
	char	buf[60];

	appres.magnification = (float) atof(panel_get_value(export_mag_text));
	if (appres.magnification <= 0.0)
	    appres.magnification = 100.0;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(export_mag_text);
}

/* as the user types in a magnification, update the figure size */

static void
update_mag(Widget widget, Widget *item, int *event)
{
    char	   *buf;

    buf = panel_get_value(export_mag_text);
    appres.magnification = (float) atof(buf);
    update_figure_size();
}

static void
update_figure_size(void)
{
    char buf[60];

    export_update_figure_size();
    /* update the print panel's indicators too */
    if (print_popup) {
	print_update_figure_size();
	sprintf(buf,"%.1f",appres.magnification);
	FirstArg(XtNstring, buf);
	SetValues(print_mag_text);
    }
}

/* get the quality from the widget and make it reasonable if not */

static void
get_quality(void)
{
	char	buf[60];

	appres.jpeg_quality = (int) atof(panel_get_value(quality_text));
	if (appres.jpeg_quality <= 0 || appres.jpeg_quality > 100)
	    appres.jpeg_quality = 100;
	/* write it back to the widget in case it had a bad value */
	sprintf(buf,"%d",appres.jpeg_quality);
	FirstArg(XtNstring, buf);
	SetValues(quality_text);
}

/* create (if necessary) and popup the export panel */

void
popup_export_panel(Widget w)
{
	char	buf[60];

	/* turn off Compose key LED */
	setCompLED(0);

	set_temp_cursor(wait_cursor);
	/* if the file panel is up now, get rid of it */
	if (file_up) {
		file_up = False;
		XtPopdown(file_popup);
	}
	/* already up? */
	if (export_up) {
		/* yes, just raise to top */
		XRaiseWindow(tool_d, XtWindow(export_popup));
		return;
	}

	export_up = popup_up = True;

	if (export_popup) {
	    /* the export panel has already been created, but the number of colors
	       may be different than before.  Re-create the transparent color menu */
	    XtDestroyWidget(transp_menu);
	    transp_menu = make_color_popup_menu(export_transp_panel,
	    				"Transparent Color", transp_select, 
					INCL_TRANSP, NO_BACKG);
	    /* and the background color menu */
	    XtDestroyWidget(background_menu);
	    background_menu = make_color_popup_menu(export_background_panel,
	    				"Background Color", background_select, 
					NO_TRANSP, INCL_BACKG);
	    /* now set the color and name in the background button */
	    set_but_col(export_background_panel, export_background_color);
	    /* now set the color and name in the transparent button */
	    set_but_col(export_transp_panel, appres.transparent);

	    /* also the magnification may have been changed in the print popup */
	    sprintf(buf,"%.1f",appres.magnification);
	    FirstArg(XtNstring, buf);
	    SetValues(export_mag_text);
	    /* also the figure size (magnification * bounding_box) */
	    /* get the bounding box of the figure just once */
	    compound_bound(&objects, &lx, &ly, &ux, &uy);
	    export_update_figure_size();
	} else {
	    create_export_panel(w);
	}

	/* set the directory widget to the current export directory */
	FirstArg(XtNstring, cur_export_dir);
	SetValues(exp_dir);

	Rescan(0, 0, 0, 0);

	/* put the default export file name */
	FirstArg(XtNlabel, default_export_file);
	NextArg(XtNwidth, E_FILE_WIDTH);
	SetValues(dfile_text);

	/* manage/unmanage optional buttons etc depending on export language */
	manage_optional();

	/* finally, popup the export panel */
	XtPopup(export_popup, XtGrabNone);

	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(export_popup));
    	XSetWMProtocols(tool_d, XtWindow(export_popup), &wm_delete_window, 1);
	reset_cursor();

	/* this will install the wheel accelerators on the scrollbars
	   now that the panel is realized */
	Rescan(0, 0, 0, 0);
}

static void
exp_xoff_unit_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(exp_xoff_unit_panel);
    xoff_unit_setting = (int) client_data;
}

static void
exp_yoff_unit_select(Widget w, XtPointer client_data, XtPointer call_data)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(exp_yoff_unit_panel);
    yoff_unit_setting = (int) client_data;
}

/* when the user checks PCL command option. */
static XtCallbackProc
toggle_hpgl_pcl_switch(Widget w, XtPointer closure, XtPointer call_data)
{
    Boolean	    state;

    /* check state of the toggle and set/remove checkmark */
    FirstArg(XtNstate, &state);
    GetValues(w);
    
    if (state ) {
	FirstArg(XtNbitmap, sm_check_pm);
    } else {
	FirstArg(XtNbitmap, sm_null_check_pm);
    }
    SetValues(w);

    /* set global state */
    print_hpgl_pcl_switch = state;

    return;
}

static XtCallbackProc
toggle_hpgl_font(Widget w, XtPointer closure, XtPointer call_data)
{
    Boolean	    state;

    /* check state of the toggle and set/remove checkmark */
    FirstArg(XtNstate, &state);
    GetValues(w);
    
    if (state ) {
	FirstArg(XtNbitmap, sm_check_pm);
    } else {
	FirstArg(XtNbitmap, sm_null_check_pm);
    }
    SetValues(w);

    /* set global state */
    hpgl_specified_font = state;

    return;
}

void create_export_panel(Widget w)
{
	Widget	    	 beside, below;
	Widget		 entry, papersize_menu;
	XFontStruct	*temp_font;
	char		 buf[50];
	char		*unit;
	float		 mult;
	int		 i;
	Position	 xposn, yposn;

	xoff_unit_setting = yoff_unit_setting = (int) appres.INCHES? 0: 1;

	XtTranslateCoords(tool, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn+50);
	NextArg(XtNy, yposn+50);
	NextArg(XtNtitle, "Xfig: Export menu");
	NextArg(XtNcolormap, tool_cm);
	NextArg(XtNallowShellResize, True);
	export_popup = XtCreatePopupShell("export_popup",
					  transientShellWidgetClass,
					  tool, Args, ArgCount);
	XtOverrideTranslations(export_popup,
			   XtParseTranslationTable(exp_translations));
	XtAppAddActions(tool_app, export_actions, XtNumber(export_actions));

	export_panel = XtCreateManagedWidget("export_panel", formWidgetClass,
					     export_popup, NULL, ZERO);

	/* The export language is first */

	FirstArg(XtNlabel, "     Language");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	lang_lab = XtCreateManagedWidget("lang_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNlabel, lang_texts[cur_exp_lang]);
	NextArg(XtNfromHoriz, lang_lab);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	lang_panel = XtCreateManagedWidget("language",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	/* make a dividing line for the Bitmap items */
	make_pulldown_menu(lang_texts, XtNumber(lang_texts), 
				    FIRST_BITMAP_LANG, "Bitmap Formats",
				    lang_panel, lang_select);

	/* Magnification */

	FirstArg(XtNlabel, "Magnification %");
	NextArg(XtNfromVert, lang_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	mag_lab = XtCreateManagedWidget("mag_label", labelWidgetClass,
					export_panel, Args, ArgCount);

	/* make a spinner entry for the mag */
	/* note: this was called "magnification" */
	sprintf(buf, "%.1f", appres.magnification);
	/* we want to track typing here to update figure size label */
	mag_spinner = MakeFloatSpinnerEntry(export_panel, &export_mag_text, "magnification",
			(Widget) NULL, mag_lab, update_mag, buf, 0.0, 10000.0, 1.0, 45);
	FirstArg(XtNfromVert, lang_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	SetValues(mag_spinner);

	XtOverrideTranslations(export_mag_text,
			   XtParseTranslationTable(export_translations));

	/* Figure Size to the right of the magnification window */

	mult = appres.INCHES? PIX_PER_INCH : PIX_PER_CM;
	unit = appres.INCHES? "in": "cm";
	/* get the size of the figure */
	compound_bound(&objects, &lx, &ly, &ux, &uy);
	sprintf(buf, "Figure size: %.1f%s x %.1f%s",
		(float)(ux-lx)/mult*appres.magnification/100.0,unit,
		(float)(uy-ly)/mult*appres.magnification/100.0,unit);
	/* fill out label with blanks or when it becomes longer later it won't show it all */
	if ((i = strlen(buf)) < 39)
	   strncat(buf, "                                        ",39-i);

	FirstArg(XtNlabel, buf);
	NextArg(XtNfromVert, lang_panel);
	NextArg(XtNfromHoriz, mag_spinner);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	size_lab = XtCreateManagedWidget("size_label", labelWidgetClass,
					export_panel, Args, ArgCount);

	/* make radiobuttons to all user to export all or only active layers */

	layer_choice = make_layer_choice("Export all layers ", "Export only active",
				export_panel, mag_spinner, NULL, 2, 0);

	/* the border margin and background color will appear depending on the export language */

	FirstArg(XtNlabel, "Border Margin");
	NextArg(XtNfromVert, layer_choice);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	border_lab = XtCreateManagedWidget("border_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	/* spinner entry for border margin */
	sprintf(buf,"%d",appres.export_margin);

	border_spinner = MakeIntSpinnerEntry(export_panel, &border_text, "border_text",
			layer_choice, border_lab, (XtCallbackProc) 0, buf, 0, 1000, 1, 35);
	FirstArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	SetValues(border_spinner);

	/* background color option for all bitmap export and PS/EPS */

	FirstArg(XtNlabel, "Background");
	NextArg(XtNfromVert, layer_choice);
	NextArg(XtNfromHoriz, border_spinner);
	NextArg(XtNhorizDistance, 8);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	background_lab = XtCreateManagedWidget("background_label", labelWidgetClass,
					 export_panel, Args, ArgCount);

	FirstArg(XtNfromVert, layer_choice);
	NextArg(XtNfromHoriz, background_lab);
	NextArg(XtNresize, False);
	NextArg(XtNwidth, 80);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	export_background_panel = XtCreateManagedWidget("background",
					   menuButtonWidgetClass,
					   export_panel, Args, ArgCount);
	/* now set the color and name in the button */
	set_but_col(export_background_panel, export_background_color);

	/* make color menu */
	background_menu = make_color_popup_menu(export_background_panel, 
				    "Background Color", background_select, 
				    NO_TRANSP, INCL_BACKG);

	/* grid options */
	FirstArg(XtNlabel, "Grid");
	NextArg(XtNfromVert, border_lab);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	export_grid_label = beside = XtCreateManagedWidget("grid_label", labelWidgetClass,
					    export_panel, Args, ArgCount);
	top_section = below = make_grid_options(export_panel, border_lab, beside, minor_grid, major_grid,
				&export_grid_minor_menu_button, &export_grid_major_menu_button,
				&export_grid_minor_menu, &export_grid_major_menu,
				&export_grid_minor_text, &export_grid_major_text,
				&export_grid_unit_label,
				export_grid_major_select, export_grid_minor_select);

	/*************************************************************/
	/* Put the Bitmap export options in their own frame.         */
	/* Don't manage it unless the export language is bitmap type */
	/*************************************************************/

	FirstArg(XtNborderWidth, 1);
	NextArg(XtNfromVert, top_section);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	bitmap_form = XtCreateWidget("bitmap_form", formWidgetClass,
					     export_panel, Args, ArgCount);
	/* now a label */
	FirstArg(XtNlabel, "Bitmap Options");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = XtCreateManagedWidget("bitmap_label", labelWidgetClass,
					bitmap_form, Args, ArgCount);

	/* Label for smoothing options */
	FirstArg(XtNlabel, "Smoothing");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	smooth_lab = XtCreateManagedWidget("smooth_label", labelWidgetClass,
					bitmap_form, Args, ArgCount);

	/* make a pulldown menu for smooth (No smoothing=1, Some=2, More=3, Most=4) */
	FirstArg(XtNlabel, smooth_choices[appres.smooth_factor==1? 0: appres.smooth_factor/2]);
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, smooth_lab);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	smooth_menu_button = XtCreateManagedWidget("smooth_factor",
					   menuButtonWidgetClass,
					   bitmap_form, Args, ArgCount);
	make_pulldown_menu(smooth_choices, XtNumber(smooth_choices), 
				    -1, "", smooth_menu_button, smooth_select);

	/* transparent color option for GIF export */

	FirstArg(XtNlabel, "GIF Transparent color");
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, smooth_menu_button);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	transp_lab = XtCreateManagedWidget("transp_label", labelWidgetClass,
					 bitmap_form, Args, ArgCount);

	FirstArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, transp_lab);
	NextArg(XtNresize, False);
	NextArg(XtNwidth, 80);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	export_transp_panel = XtCreateManagedWidget("transparent",
					   menuButtonWidgetClass,
					   bitmap_form, Args, ArgCount);
	/* now set the color and name in the button */
	set_but_col(export_transp_panel, appres.transparent);

	transp_menu = make_color_popup_menu(export_transp_panel, 
					"Transparent Color", transp_select, 
					INCL_TRANSP, NO_BACKG);

	/* image quality option for JPEG export */

	/* first label */
	FirstArg(XtNlabel, "JPEG Image quality (%)");
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, smooth_menu_button);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	quality_lab = XtCreateManagedWidget("quality_label", labelWidgetClass,
					 bitmap_form, Args, ArgCount);

	/* spinner entry for quality */
	sprintf(buf,"%d",appres.jpeg_quality);
	quality_spinner = MakeIntSpinnerEntry(bitmap_form, &quality_text, "quality_text",
			below, quality_lab, (XtCallbackProc) 0, buf, 0, 100, 1, 30);
	FirstArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	SetValues(quality_spinner);

	/***** End of Bitmap Option form *****/

	/**************************************************************/
	/* Put the PostScript/HPGL options in their own frame (form). */
	/* Don't manage it unless the export language is PS/HPGL type */
	/**************************************************************/

	FirstArg(XtNborderWidth, 1);
	NextArg(XtNfromVert, top_section);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	postscript_form = XtCreateWidget("postscript_form", formWidgetClass,
					     export_panel, Args, ArgCount);
	/* now a label */
	FirstArg(XtNlabel, "PostScript Options");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	below = ps_form_label = XtCreateManagedWidget("postscript_label", labelWidgetClass,
					postscript_form, Args, ArgCount);

	/* paper size */

	FirstArg(XtNlabel, " Paper Size");
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	papersize_lab = XtCreateManagedWidget("papersize_label", labelWidgetClass,
					 postscript_form, Args, ArgCount);

	FirstArg(XtNlabel, paper_sizes[appres.papersize].fname);
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, papersize_lab);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNresizable, True);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	export_papersize_panel = XtCreateManagedWidget("papersize",
					   menuButtonWidgetClass,
					   postscript_form, Args, ArgCount);

	/* make the menu items */
	papersize_menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, 
				    export_papersize_panel, NULL, ZERO);
	for (i = 0; i < XtNumber(paper_sizes); i++) {
	    entry = XtCreateManagedWidget(paper_sizes[i].fname, smeBSBObjectClass, 
					papersize_menu, NULL, ZERO);
	    XtAddCallback(entry, XtNcallback, papersize_select, (XtPointer) i);
	}

	/* Fit Page */

	FirstArg(XtNlabel, "Fit to Page");
	NextArg(XtNfromVert, below);
	NextArg(XtNfromHoriz, export_papersize_panel);
	NextArg(XtNhorizDistance, 9);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fitpage = XtCreateManagedWidget("fitpage", commandWidgetClass,
				       postscript_form, Args, ArgCount);
	XtAddEventHandler(fitpage, ButtonReleaseMask, False,
			  (XtEventHandler)fit_page, (XtPointer) NULL);

	/* Landscape/Portrait Orientation */

	FirstArg(XtNlabel, "Orientation");
	NextArg(XtNfromVert, export_papersize_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	orient_lab = XtCreateManagedWidget("orient_label", labelWidgetClass,
					 postscript_form, Args, ArgCount);

	FirstArg(XtNlabel, orient_items[appres.landscape]);
	NextArg(XtNfromVert, export_papersize_panel);
	NextArg(XtNfromHoriz, orient_lab);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	export_orient_panel = XtCreateManagedWidget("orientation",
					     menuButtonWidgetClass,
					     postscript_form, Args, ArgCount);
	make_pulldown_menu(orient_items, XtNumber(orient_items), -1, "",
				      export_orient_panel, orient_select);
	/* Justification */

	FirstArg(XtNlabel, "Justification");
	NextArg(XtNfromVert, export_papersize_panel);
	NextArg(XtNfromHoriz, export_orient_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	just_lab = XtCreateManagedWidget("just_label", labelWidgetClass,
					 postscript_form, Args, ArgCount);

	FirstArg(XtNlabel, just_items[appres.flushleft? 1 : 0]);
	NextArg(XtNfromVert, export_papersize_panel);
	NextArg(XtNfromHoriz, just_lab);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	export_just_panel = XtCreateManagedWidget("justify",
					   menuButtonWidgetClass,
					   postscript_form, Args, ArgCount);
	make_pulldown_menu(just_items, XtNumber(just_items), -1, "",
				    export_just_panel, just_select);

	/* multiple/single page */

	FirstArg(XtNlabel, "      Pages");
	NextArg(XtNfromVert, export_orient_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	multiple_lab = XtCreateManagedWidget("multiple_label", labelWidgetClass,
					 postscript_form, Args, ArgCount);

	FirstArg(XtNlabel, multiple_pages[appres.multiple? 1:0]);
	NextArg(XtNfromVert, export_orient_panel);
	NextArg(XtNfromHoriz, multiple_lab);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	export_multiple_panel = XtCreateManagedWidget("multiple_pages",
					   menuButtonWidgetClass,
					   postscript_form, Args, ArgCount);
	make_pulldown_menu(multiple_pages, XtNumber(multiple_pages), 
					-1, "", export_multiple_panel, multiple_select);

	/* overlap/no-overlap for multiple page selection */

	FirstArg(XtNlabel, overlap_pages[appres.overlap? 1:0]);
	NextArg(XtNfromVert, export_orient_panel);
	NextArg(XtNfromHoriz, export_multiple_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	export_overlap_panel = XtCreateManagedWidget("overlap_pages",
					   menuButtonWidgetClass,
					   postscript_form, Args, ArgCount);
	make_pulldown_menu(overlap_pages, XtNumber(overlap_pages), 
					-1, "", export_overlap_panel, overlap_select);

	/* X/Y offset choices */

	FirstArg(XtNlabel, "     Offset");
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	exp_off_lab = XtCreateManagedWidget("export_offset_label", labelWidgetClass,
				     postscript_form, Args, ArgCount);
	FirstArg(XtNlabel, "X");
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNfromHoriz, exp_off_lab);
	NextArg(XtNhorizDistance, 5);
	NextArg(XtNborderWidth, 1);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	exp_xoff_lab = XtCreateManagedWidget("export_offset_lbl_x", labelWidgetClass,
				     postscript_form, Args, ArgCount);
	FirstArg(XtNwidth, 50);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0.0");
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNfromHoriz, exp_xoff_lab);
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	export_offset_x = XtCreateManagedWidget("export_offset_x", asciiTextWidgetClass,
					     postscript_form, Args, ArgCount);
	FirstArg(XtNfromHoriz, export_offset_x);
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	exp_xoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, postscript_form, Args, ArgCount);
	make_pulldown_menu(offset_unit_items, XtNumber(offset_unit_items), 
				-1, "", exp_xoff_unit_panel, exp_xoff_unit_select);

	FirstArg(XtNlabel, "Y");
	NextArg(XtNfromHoriz, exp_xoff_unit_panel);
	NextArg(XtNhorizDistance, 10);
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNborderWidth, 1);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	exp_yoff_lab = XtCreateManagedWidget("export_offset_lbl_y", labelWidgetClass,
				     postscript_form, Args, ArgCount);
	FirstArg(XtNwidth, 50);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0.0");
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNfromHoriz, exp_yoff_lab);
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	export_offset_y = XtCreateManagedWidget("export_offset_y", asciiTextWidgetClass,
					     postscript_form, Args, ArgCount);
	FirstArg(XtNfromHoriz, export_offset_y);
	NextArg(XtNfromVert, export_multiple_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	exp_yoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, postscript_form, Args, ArgCount);
	make_pulldown_menu(offset_unit_items, XtNumber(offset_unit_items), 
				-1, "", exp_yoff_unit_panel, exp_yoff_unit_select);


	/* (HPGL) PCL output option */
	FirstArg(XtNlabel, "Issue PCL Command to use HPGL");  /* Label */
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	pcl_cmd_label = XtCreateManagedWidget("pcl_cmd_label",
					      labelWidgetClass,
					      postscript_form, Args, ArgCount);

	/* (HPGL) checkbox for including PCL switch */

	FirstArg(XtNbitmap, (print_hpgl_pcl_switch
			     ? sm_check_pm : sm_null_check_pm));
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNfromHoriz, pcl_cmd_label); 
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNlabel, "  ");
	NextArg(XtNstate, print_hpgl_pcl_switch);	/* initial state */
	pcl_cmd_checkbox = XtCreateManagedWidget("pcl_cmd_checkbox",
						 toggleWidgetClass,
						 postscript_form,
						 Args, ArgCount);
	XtAddCallback(pcl_cmd_checkbox, XtNcallback,
		      (XtCallbackProc)toggle_hpgl_pcl_switch,(XtPointer) NULL);

	/* (HPGL) Use 'SD' font command */
	FirstArg(XtNlabel, "Use Specified Font");  /* Label */
	NextArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNfromHoriz, pcl_cmd_checkbox);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	hpgl_font_label = XtCreateManagedWidget("hpgl_font_label",
					      labelWidgetClass,
					      postscript_form, Args, ArgCount);

	/* (HPGL)checkbox for including PCL switch */

	FirstArg(XtNbitmap, (hpgl_specified_font
			     ? sm_check_pm : sm_null_check_pm));
	NextArg(XtNfromVert, exp_off_lab);
	NextArg(XtNfromHoriz, hpgl_font_label); 
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNinternalWidth, 1);
	NextArg(XtNinternalHeight, 1);
	NextArg(XtNlabel, "  ");
	NextArg(XtNstate, print_hpgl_pcl_switch);	/* initial state */
	hpgl_font_checkbox = XtCreateManagedWidget("hpgl_font_checkbox",
						   toggleWidgetClass,
						   postscript_form,
						   Args, ArgCount);
	XtAddCallback(hpgl_font_checkbox, XtNcallback,
		      (XtCallbackProc)toggle_hpgl_font, (XtPointer) NULL);


	/***** End of PostScript/HPGL Option form *****/

	/* now manage either the PostScript form, the Bitmap form or neither */
	if (cur_exp_lang == LANG_PS) {
		XtManageChild(postscript_form);
		below = postscript_form;
	} else if (cur_exp_lang >= FIRST_BITMAP_LANG) {
		XtManageChild(bitmap_form);
		below = bitmap_form;
	} else {
		below = top_section;
	}


	/* now make a lower section form so we can manage/unmanage it when
	 * the middle section (options) changes */

	FirstArg(XtNborderWidth, 0);
	NextArg(XtNfromVert, below);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	bottom_section = XtCreateWidget("bottom_section",
					formWidgetClass,
					     export_panel, Args, ArgCount);
	/* next the default file name */

	FirstArg(XtNlabel, " Default File");
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	dfile_lab = XtCreateManagedWidget("def_file_label", labelWidgetClass,
					  bottom_section, Args, ArgCount);

	FirstArg(XtNlabel, default_export_file);
	NextArg(XtNfromHoriz, dfile_lab);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	dfile_text = XtCreateManagedWidget("def_file_name", labelWidgetClass,
					   bottom_section, Args, ArgCount);

	FirstArg(XtNlabel, "  Output File");
	NextArg(XtNfromVert, dfile_text);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	nfile_lab = XtCreateManagedWidget("out_file_name", labelWidgetClass,
					  bottom_section, Args, ArgCount);

	FirstArg(XtNfont, &temp_font);
	GetValues(nfile_lab);

	FirstArg(XtNwidth, E_FILE_WIDTH);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNfromHoriz, nfile_lab);
	NextArg(XtNfromVert, dfile_text);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNstring, named_file);
	NextArg(XtNinsertPosition, strlen(named_file));
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	exp_selfile = XtCreateManagedWidget("file", asciiTextWidgetClass,
					    bottom_section, Args, ArgCount);
	XtOverrideTranslations(exp_selfile,
			   XtParseTranslationTable(export_translations));

	/* add action to export file for following translation */
	XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));

	/* make <return> in the filename window export the file */
	XtOverrideTranslations(exp_selfile,
			   XtParseTranslationTable(file_name_translations));

	/* create the directory widgets */
	change_directory(cur_export_dir);
	create_dirinfo(False, bottom_section, exp_selfile, &beside, &below,
		       &exp_mask, &exp_dir, &exp_flist, &exp_dlist, E_FILE_WIDTH, False);
	/* make <return> or double click in the file list window export the file */
	XtOverrideTranslations(exp_flist,
			   XtParseTranslationTable(file_list_translations));

	/* cancel button */
	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	cancel_but = XtCreateManagedWidget("cancel", commandWidgetClass,
					   bottom_section, Args, ArgCount);
	XtAddEventHandler(cancel_but, ButtonReleaseMask, False,
			  (XtEventHandler) export_panel_cancel, (XtPointer) NULL);

	FirstArg(XtNlabel, "Export");
	NextArg(XtNfromHoriz, cancel_but);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	export_but = XtCreateManagedWidget("export", commandWidgetClass,
					   bottom_section, Args, ArgCount);
	XtAddEventHandler(export_but, ButtonReleaseMask, False,
			  (XtEventHandler)do_export, (XtPointer) NULL);

	/* install accelerators for cancel, and export in the main panel */
	XtInstallAccelerators(export_panel, cancel_but);
	XtInstallAccelerators(export_panel, export_but);

	update_def_filename();
	/* set the initial wildcard mask based on the current export language */
	set_export_mask(cur_exp_lang);
}

/* update the default export filename using the Fig file name */

void update_def_filename(void)
{
    int		    i;

    /* copy Fig filename to export filename without path */
    strcpy(default_export_file, xf_basename(cur_filename));
    if (default_export_file[0] != '\0') {
	i = strlen(default_export_file);
	if (i >= 4 && strcmp(&default_export_file[i - 4], ".fig") == 0)
	    default_export_file[i - 4] = '\0';

	/* for tiff and jpeg use three-letter suffixes */
	if (cur_exp_lang == LANG_TIFF)
	    strcat(default_export_file, ".tif");
	else if (cur_exp_lang == LANG_JPEG)
	    strcat(default_export_file, ".jpg");
	else if (cur_exp_lang == LANG_EPS_ASCII)
	    /* just use .eps for suffix (not eps_ascii) */
	    strcat(default_export_file, ".eps");
	else if (cur_exp_lang == LANG_EPS_MONO_TIFF)
	    /* and add _mtif to show that it has monochrome tiff */
	    strcat(default_export_file, "_mtiff.eps");
	else if (cur_exp_lang == LANG_EPS_COLOR_TIFF)
	    /* and add _ctif to show that it has color tiff */
	    strcat(default_export_file, "_ctiff.eps");
	else if (cur_exp_lang==LANG_PDFTEX)
	    /* for pdftex, just use .pdf */
	    strcat(default_export_file, ".pdf");
	else if (cur_exp_lang==LANG_PSPDF)
	    /* for pspdf, start with just .eps */
	    strcat(default_export_file, ".eps");
	else {
	    strcat(default_export_file, ".");
	    strcat(default_export_file, lang_items[cur_exp_lang]);
	}
    }
    /* remove trailing blanks */
    for (i = strlen(default_export_file) - 1; i >= 0; i--)
	if (default_export_file[i] == ' ')
	    default_export_file[i] = '\0';
	else
	    i = 0;
}
