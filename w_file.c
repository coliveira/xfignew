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
#include "f_read.h"
#include "f_util.h"
#include "u_create.h"
#include "u_redraw.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_export.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_icons.h"
#include "w_zoom.h"

#include "e_compound.h"
#include "f_load.h"
#include "f_save.h"
#include "u_free.h"
#include "u_list.h"
#include "w_canvas.h"
#include "w_cmdpanel.h"
#include "w_color.h"
#include "w_cursor.h"

#define	FILE_WID	157	/* width of Current file etc to show preview to the right */
#define FILE_ALT_WID	260	/* width of file alternatives panel */
#define PREVIEW_CANVAS_W 232	/* width of landscape preview canvas (swap W, H for portrait) */
#define PREVIEW_CANVAS_H 180	/* height of landscape preview */

/* file modes for popup panel */

enum file_modes {
	F_SAVEAS,
	F_OPEN,
	F_MERGE
};

/* EXPORTS */

Boolean		file_up = False;	/* whether the file panel is up (or the export panel) */
Widget		file_panel;		/* the file panel itself */
Widget		cfile_text;		/* widget for the current filename */
Widget		file_selfile,		/* selected file widget */
		file_mask,		/* mask widget */
		file_dir,		/* current directory widget */
		file_flist,		/* file list widget */
		file_dlist;		/* dir list widget */
Widget		file_popup;
Boolean		colors_are_swapped = False;
Widget		preview_size, preview_widget, preview_widget_form, preview_name;
Widget		preview_stop, preview_label, dummy_label;
Widget		comments_widget;
Pixmap		preview_land_pixmap, preview_port_pixmap;
Boolean		cancel_preview = False;
Boolean		preview_in_progress = False;
void		load_request(Widget w, XButtonEvent *ev);		/* this is needed by main() */

/* LOCALS */

static void	file_preview_stop(Widget w, XButtonEvent *ev);
static Boolean	file_cancel_request = False;
static Boolean	file_load_request = False;
static Boolean	file_save_request = False;
static Boolean	file_merge_request = False;

/* these are in fig units */

static float	offset_unit_conv[] = { (float)PIX_PER_INCH, (float)PIX_PER_CM, 1.0 };

static char	*load_msg = "The current figure is modified.\nDo you want to discard it and load the new file?";
static char	buf[40];

/* to save image colors when doing a figure preview */

static XColor	save_image_cells[MAX_COLORMAP_SIZE];
static int	save_avail_image_cols;
static Boolean	image_colors_are_saved = False;

static void	popup_file_panel(int mode);
static void	file_panel_cancel(Widget w, XButtonEvent *ev);
static void	do_load(Widget w, XButtonEvent *ev), do_merge(Widget w, XButtonEvent *ev);
static void	merge_request(Widget w, XButtonEvent *ev), cancel_request(Widget w, XButtonEvent *ev), save_request(Widget w, XButtonEvent *ev);
static void	clear_preview(void);

DeclareStaticArgs(15);
static Widget	file_stat_label, file_status, num_obj_label, num_objects;
static Widget	figure_off, cfile_lab;
static Widget	cancel_button, save_button, merge_button;
static Widget	load_button, new_xfig_button;
static Widget	fig_offset_lbl_x, fig_offset_lbl_y;
static Widget	fig_offset_x, fig_offset_y;
static Widget	file_xoff_unit_panel;
static Widget	file_yoff_unit_panel;
static int	xoff_unit_setting, yoff_unit_setting;

static String	file_list_load_translations =
			"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
			<Btn1Up>(2): DoLoad()\n\
			<Key>Return: DoLoad()";
static String	file_name_load_translations =
			"<Key>Return: DoLoad()\n\
			<Key>Escape: CancelFile()";
static String	file_list_saveas_translations =
			"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
			<Btn1Up>(2): SaveRequest()\n\
			<Key>Return: DoSave()\n\
			<Key>Escape: CancelFile()";
static String	file_name_saveas_translations =
			"<Key>Return: DoSave()\n\
			<Key>Escape: CancelFile()";
static String	file_list_merge_translations =
			"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
			<Btn1Up>(2): DoMerge()\n\
			<Key>Return: DoMerge()";
static String	file_name_merge_translations =
			"<Key>Return: DoMerge()\n\
			<Key>Escape: CancelFile()";

static XtActionsRec	file_name_actions[] =
{
    {"DoLoad", (XtActionProc) load_request},
    {"SaveRequest", (XtActionProc) save_request},
    {"DoSave", (XtActionProc) do_save},
    {"DoMerge", (XtActionProc) merge_request},
    {"Load", (XtActionProc) popup_open_panel},
    {"Merge", (XtActionProc) popup_merge_panel},
};
static String	file_translations =
	"<Message>WM_PROTOCOLS: DismissFile()";
static XtActionsRec	file_actions[] =
{
    {"DismissFile", (XtActionProc) cancel_request},
    {"CancelFile", (XtActionProc) cancel_request},
    {"DoLoad", (XtActionProc) load_request},
    {"DoMerge", (XtActionProc) merge_request},
};


int renamefile (char *file);
void create_file_panel (void);

void
file_panel_dismiss(void)
{
    int i;

    if (cur_file_dir[0] != '\0')
	change_directory(cur_file_dir);

    if (image_colors_are_saved) {
	/* restore image colors on canvas */
	avail_image_cols = save_avail_image_cols;
	for (i=0; i<avail_image_cols; i++) {
	    image_cells[i].red   = save_image_cells[i].red;
	    image_cells[i].green = save_image_cells[i].green;
	    image_cells[i].blue  = save_image_cells[i].blue;
	    image_cells[i].flags  = DoRed|DoGreen|DoBlue;
        }
	YStoreColors(tool_cm, image_cells, avail_image_cols);
	image_colors_are_saved = False;
    }
    FirstArg(XtNstring, "\0");
    SetValues(file_selfile);	/* clear Filename string */
    XtPopdown(file_popup);
    file_up = popup_up = False;
    /* in case the colormap was switched while previewing */
    redisplay_canvas();
}

/* get x/y offsets from panel */

void file_getxyoff(int *ixoff, int *iyoff)
{
    float xoff, yoff;
    *ixoff = *iyoff = 0;
    /* if no file panel yet, use 0, 0 for offsets */
    if (fig_offset_x == (Widget) 0 ||
	fig_offset_y == (Widget) 0)
	    return;

    sscanf(panel_get_value(fig_offset_x),"%f",&xoff);
    *ixoff = round(xoff*offset_unit_conv[xoff_unit_setting]);
    sscanf(panel_get_value(fig_offset_y),"%f",&yoff);
    *iyoff = round(yoff*offset_unit_conv[yoff_unit_setting]);
}

void
merge_request(Widget w, XButtonEvent *ev)
{
    if (preview_in_progress) {
	file_merge_request = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	/* make sure this is false from any previous previewing */
	cancel_preview = False;
	do_merge(w, ev);
    }
}

static void
do_merge(Widget w, XButtonEvent *ev)
{
    char	    path[PATH_MAX], fname[PATH_MAX];
    char	   *fval, *dval;
    int		    xoff, yoff;

    FirstArg(XtNstring, &fval);
    GetValues(file_selfile);	/* check the ascii widget for a filename */
    strcpy(fname, fval);
    if (emptyname(fname))
	strcpy(fname, cur_filename);	/* "Filename" widget empty, use current filename */

    if (!strchr(fname,'.'))	/* if no suffix, add .fig */
	strcat(fname,".fig");

    if (emptyname_msg(fname, "MERGE"))
	return;

    FirstArg(XtNstring, &dval);
    GetValues(file_dir);

    strcpy(path, dval);
    strcat(path, "/");
    strcat(path, fname);
    file_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */
    /* if the user colors are saved we must swap current colors to save them for the preview */
    if (user_colors_saved) {
	swap_colors();
	/* get back the colors from the figure on the canvas */
	restore_user_colors();
	merge_file(path, xoff, yoff);
    } else {
	merge_file(path, xoff, yoff);
    }
    /* we have merged any image colors from the file with the canvas */
    image_colors_are_saved = False;
    /* dismiss the panel */
    file_panel_dismiss();
}

/*
   Set a flag requesting a load.  This is called by double clicking on a
   filename or by pressing the Open button. 

   If we aren't in the middle of previewing a file, do the actual load.

   Otherwise just set a flag here because we are in the middle of a preview, 
   since the first click on the filename started up preview_figure(). 

   We can't do the load yet because the canvas_win variable is occupied by
   the pixmap for the preview, and we need to change it back to the real canvas
   before we draw the figure we will load.
*/

void
load_request(Widget w, XButtonEvent *ev)
{
    if (preview_in_progress) {
	file_load_request = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	/* make sure this is false from any previous previewing */
	cancel_preview = False;
	do_load(w, ev);
    }
}

void
do_load(Widget w, XButtonEvent *ev)
{
    char	    fname[PATH_MAX];
    char	   *fval, *dval;
    int		    xoff, yoff;

    /* don't load if in the middle of drawing/editing */
    if (check_action_on())
	return;

    /* restore any colors that were saved during a preview */
    restore_user_colors();
    restore_nuser_colors();

    /* first check if the figure was modified before reloading it */
    if (!emptyfigure() && figure_modified) {
	if (file_popup)
	    XtSetSensitive(load_button, False);
	if (popup_query(QUERY_YESCAN, load_msg) == RESULT_CANCEL) {
	    if (file_popup)
		XtSetSensitive(load_button, True);
	    return;
	}
    }
    /* if the user used a keyboard accelerator but the filename
       is empty, popup the file panel to force him/her to enter a name */
    if (emptyname(cur_filename) && !file_up) {
	put_msg("No filename, please enter name");
	beep();
	popup_open_panel();
	return;
    }
    if (file_popup) {
	app_flush();			/* make sure widget is updated (race condition) */
	FirstArg(XtNstring, &dval);
	GetValues(file_dir);
	FirstArg(XtNstring, &fval);
	GetValues(file_selfile);	/* check the ascii widget for a filename */
	if (emptyname(fval))
	    strcpy(fname, cur_filename); /* Filename widget empty, use current filename */
	else
	    strcpy(fname, fval);	/* get name from widget */

	if (!strchr(fname,'.'))		/* if no suffix, add .fig */
	    strcat(fname,".fig");

	if (emptyname_msg(fname, "LOAD"))
	    return;
	if (change_directory(dval) == 0) {
	    file_getxyoff(&xoff,&yoff);	/* get x/y offsets from panel */
	    if (load_file(fname, xoff, yoff) == 0) {
		FirstArg(XtNlabel, fname);
		SetValues(cfile_text);		/* set the current filename */
		update_def_filename();		/* update default export filename */
		XtSetSensitive(load_button, True);
		/* we have just loaded the image colors from the file */
		image_colors_are_saved = False;
		if (file_up)
		    file_panel_dismiss();
	    }
	}
    } else {
	file_getxyoff(&xoff,&yoff);
	(void) load_file(cur_filename, xoff, yoff);
    }
}

void
new_xfig_request(Widget w, XButtonEvent *ev)
{
    pid_t pid;
    char *fval, fname[PATH_MAX];
    char **xxargv;
    int i, j;
    extern int xargc;
    extern char **xargv;
    extern char *arg_filename;

    FirstArg(XtNstring, &fval);
    GetValues(file_selfile);	/* check the ascii widget for a filename */
    strcpy(fname, fval);

    if (emptyname(fname)) 
	put_msg("Launching new xfig...");
    else {
	if (!strchr(fname,'.'))		/* if no suffix, add .fig */
	    strcat(fname,".fig");
	put_msg("Launching new xfig for file %s...", fname);
    }

    pid = fork();
    if (pid == -1) {
	file_msg("Couldn't fork the process: %s", strerror(errno));
    } else if (pid == 0) {
	xxargv = (char **)XtMalloc((xargc+2) * sizeof (char *));
	for (i = j = 0; i < xargc; i++) {
	    if (emptyname(arg_filename) || strcmp(xargv[i], arg_filename) != 0)
		xxargv[j++] = xargv[i];
	}
	if (!emptyname(fname)) 
	    xxargv[j++] = fname;
	xxargv[j] = NULL;
	execvp(xxargv[0], xxargv);

	fprintf(stderr, "xfig: couldn't exec %s: %s\n", xxargv[0], strerror(errno));
	exit(1);
    }

    file_panel_dismiss();
}

/* set a request to save.  See notes above for load_request() */

void
save_request(Widget w, XButtonEvent *ev)
{
    /* turn off Compose key LED */
    setCompLED(0);

    if (preview_in_progress) {
	file_save_request = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	/* make sure this is false from any previous previewing */
	cancel_preview = False;
	do_save(w, ev);
    }
}

void
do_save(Widget w, XButtonEvent *ev)
{
    char	   *fval, *dval, fname[PATH_MAX];
    int		    qresult;

    /* don't save if in the middle of drawing/editing */
    if (check_action_on())
	return;

    if (emptyfigure_msg("Save"))
	return;

    /* restore any colors that were saved during a preview */
    restore_user_colors();
    restore_nuser_colors();

    /* if the user is inside any compound objects, ask whether to save all or just this part */
    if ((F_compound *)objects.parent != NULL) {
	qresult = popup_query(QUERY_ALLPARTCAN,
		"You have opened a compound. You may save just\nthe visible part or all of the figure.");
	if (qresult == RESULT_CANCEL)
		return;
	if (qresult == RESULT_ALL)
	    close_all_compounds();
    }

    /* go to the file directory */
    if (change_directory(cur_file_dir) != 0)
	return;

    /* if the user used a keyboard accelerator or right button on File but the filename
       is empty, popup the file panel to force him/her to enter a name */
    if (emptyname(cur_filename) && !file_up) {
	put_msg("No filename, please enter name");
	beep();
	popup_saveas_panel();
	/* wait for user to save file or cancel file popup */
	while (file_up) {
	    if (XtAppPending(tool_app))
		XtAppProcessEvent(tool_app, XtIMAll);
	}
	return;
    }
    if (file_popup) {
	FirstArg(XtNstring, &fval);
	GetValues(file_selfile);	/* check the ascii widget for a filename */
	strcpy(fname, fval);
	if (emptyname(fname)) {
	    strcpy(fname, cur_filename); /* "Filename" widget empty, use current filename */
	    warnexist = False;		/* don't warn if this file exists */
	/* copy the name from the file_name widget to the current filename */
	} else {
	    if (!strchr(fname,'.'))	/* if no suffix, add .fig */
		strcat(fname,".fig");
	    if (strcmp(cur_filename, fname) != 0)
		warnexist = True;	/* warn if this file exists */
	}

	if (emptyname_msg(fname, "Save"))
	    return;

	/* get the directory from the ascii widget */
	FirstArg(XtNstring, &dval);
	GetValues(file_dir);

	if (change_directory(dval) == 0) {
	    if (!ok_to_write(fname, "SAVE"))
		return;
	    XtSetSensitive(save_button, False);
	    (void) renamefile(fname);
	    if (write_file(fname, True) == 0) {
		FirstArg(XtNlabel, fname);
		SetValues(cfile_text);
		if (strcmp(fname, cur_filename) != 0) {
		    update_cur_filename(fname);	/* update cur_filename */
		    update_def_filename();	/* update the default export filename */
		}
		reset_modifiedflag();
		if (file_up)
		    file_panel_dismiss();
	    }
	    XtSetSensitive(save_button, True);
	}
    } else {
	/* see if writable */
	if (!ok_to_write(cur_filename, "SAVE"))
	    return;
	/* not using popup => filename not changed so ok to write existing file */
	warnexist = False;			
	(void) renamefile(cur_filename);
	if (write_file(cur_filename, True) == 0)
	    reset_modifiedflag();
    }
}

/* try to rename current to current.bak */

int renamefile(char *file)
{
    char	    bak_name[PATH_MAX];

    strcpy(bak_name,file);
    strcat(bak_name,".bak");
    if (rename(file,bak_name) < 0)
	return (-1);
    return 0;
}

Boolean
query_save(char *msg)
{
    int		    qresult;
    if (!emptyfigure() && figure_modified && !aborting) {
	if ((qresult = popup_query(QUERY_YESNOCAN, msg)) == RESULT_CANCEL) 
	    return False;
	else if (qresult == RESULT_YES) {
	    save_request((Widget) 0, (XButtonEvent *) 0);
	    /*
	     * if saving was not successful, figure_modified is still true:
	     * do not quit!
	     */
	    if (figure_modified)
		return False;
	}
    }
    /* ok */
    return True;
}

static void
cancel_request(Widget w, XButtonEvent *ev)
{
    if (preview_in_progress) {
	file_cancel_request = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	/* make sure this is false from any previous previewing */
	cancel_preview = False;
	file_panel_cancel(w, ev);
    }
}

static void
file_panel_cancel(Widget w, XButtonEvent *ev)
{
    if (user_colors_saved) {
	restore_user_colors();
	restore_nuser_colors();
	colors_are_swapped = 0;
    }
    if (colors_are_swapped)
	swap_colors();
    file_panel_dismiss();
}

void
popup_saveas_panel(void)
{
	popup_file_panel(F_SAVEAS);
}

void
popup_open_panel(void)
{
	popup_file_panel(F_OPEN);
}

void
popup_merge_panel(void)
{
	popup_file_panel(F_MERGE);
}

static void
popup_file_panel(int mode)
{
	DeclareArgs(5);

	/* turn off Compose key LED */
	setCompLED(0);

	/* already up? */
	if (file_up) {
	    /* yes, just raise to top */
	    XRaiseWindow(tool_d, XtWindow(file_popup));
	    return;
	}

	/* clear flags */
	colors_are_swapped = False;
	image_colors_are_saved = False;

	set_temp_cursor(wait_cursor);
	/* if the export panel is up get rid of it */
	if (export_up) {
	    export_up = False;
	    XtPopdown(export_popup);
	}
	file_up = popup_up = True;

	/* create it if it isn't already created */
	if (!file_popup) {
	    create_file_panel();
	} else {
	    FirstArg(XtNstring, cur_file_dir);
	    SetValues(file_dir);
	    Rescan(0, 0, 0, 0);
	}
	XtSetSensitive(figure_off, mode != F_SAVEAS);
	XtSetSensitive(fig_offset_lbl_x, mode != F_SAVEAS);
	XtSetSensitive(fig_offset_lbl_y, mode != F_SAVEAS);
	XtSetSensitive(fig_offset_x, mode != F_SAVEAS);
	XtSetSensitive(fig_offset_y, mode != F_SAVEAS);
	XtSetSensitive(file_xoff_unit_panel, mode != F_SAVEAS);
	XtSetSensitive(file_yoff_unit_panel, mode != F_SAVEAS);

	/* now add/remove appropriate buttons and set the title in the titlebar */
	if (mode == F_SAVEAS) {
	    XtUnmanageChild(load_button);
	    XtUnmanageChild(merge_button);
	    XtManageChild(save_button);
	    FirstArg(XtNtitle, "Xfig: SaveAs");
	    SetValues(file_popup);
	    /* make <return> in the filename window save the file */
	    XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(file_name_saveas_translations));
	    /* make <return> and a double click in the file list window save the file */
	    XtOverrideTranslations(file_flist,
			   XtParseTranslationTable(file_list_saveas_translations));

	} else if (mode == F_OPEN) {
	    XtUnmanageChild(save_button);
	    XtUnmanageChild(merge_button);
	    XtManageChild(load_button);
	    XtManageChild(new_xfig_button);
	    FirstArg(XtNtitle, "Xfig: Open Figure");
	    SetValues(file_popup);
	    /* make <return> in the filename window load the file */
	    XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(file_name_load_translations));
	    /* make <return> and a double click in the file list window load the file */
	    XtOverrideTranslations(file_flist,
			   XtParseTranslationTable(file_list_load_translations));

	} else { /* must be F_MERGE */
	    XtUnmanageChild(save_button);
	    XtUnmanageChild(load_button);
	    XtManageChild(merge_button);
	    FirstArg(XtNtitle, "Xfig: Merge Figure");
	    SetValues(file_popup);
	    /* make <return> in the filename window merge the file */
	    XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(file_name_merge_translations));
	    /* make <return> and a double click in the file list window merge the file */
	    XtOverrideTranslations(file_flist,
			   XtParseTranslationTable(file_list_merge_translations));

	}

	FirstArg(XtNlabel, (figure_modified ? "Modified    " :
					      "Not modified"));
	SetValues(file_status);
	sprintf(buf, "%d", object_count(&objects));
	FirstArg(XtNlabel, buf);
	SetValues(num_objects);
	XtPopup(file_popup, XtGrabNone);

	/* get rid of the dummy label if we haven't already */
	XtUnmanageChild(dummy_label);
	/* and put in the real preview widget */
	XtManageChild(preview_widget);
	/* insure that the most recent colormap is installed */
	set_cmap(XtWindow(file_popup));
	(void) XSetWMProtocols(tool_d, XtWindow(file_popup), &wm_delete_window, 1);
	reset_cursor();

	/* this will install the wheel accelerators on the scrollbars
	   now that the panel is realized */
	Rescan(0, 0, 0, 0);
}

static void
file_xoff_unit_select(Widget w, XtPointer new_unit, XtPointer garbage)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(file_xoff_unit_panel);
    xoff_unit_setting = (int) new_unit;
}

static void
file_yoff_unit_select(Widget w, XtPointer new_unit, XtPointer garbage)
{
    FirstArg(XtNlabel, XtName(w));
    SetValues(file_yoff_unit_panel);
    yoff_unit_setting = (int) new_unit;
}

void create_file_panel(void)
{
	Widget		 file, beside, below, butbelow;
	XFontStruct	*temp_font;
	static Boolean   actions_added=False;
	Widget		 preview_form;
	Position	 xposn, yposn;

	xoff_unit_setting = yoff_unit_setting = (int) appres.INCHES? 0: 1;

	XtTranslateCoords(tool, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn+50);
	NextArg(XtNy, yposn+50);
	NextArg(XtNtitle, "Xfig: File menu");
	NextArg(XtNcolormap, tool_cm);
	file_popup = XtCreatePopupShell("file_popup",
					transientShellWidgetClass,
					tool, Args, ArgCount);
	XtOverrideTranslations(file_popup,
			   XtParseTranslationTable(file_translations));

	file_panel = XtCreateManagedWidget("file_panel", formWidgetClass,
					   file_popup, NULL, ZERO);

	FirstArg(XtNlabel, "  File Status");
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	file_stat_label = XtCreateManagedWidget("file_status_label", labelWidgetClass,
					    file_panel, Args, ArgCount);
	/* start with long label so length is enough - it will be reset by calling proc */
	FirstArg(XtNlabel, "Not modified ");
	NextArg(XtNfromHoriz, file_stat_label);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	file_status = XtCreateManagedWidget("file_status", labelWidgetClass,
					    file_panel, Args, ArgCount);

	FirstArg(XtNlabel, " # of Objects");
	NextArg(XtNfromVert, file_status);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	num_obj_label = XtCreateManagedWidget("num_objects_label", labelWidgetClass,
					  file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 50);
	NextArg(XtNfromVert, file_status);
	NextArg(XtNfromHoriz, num_obj_label);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNresize, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	num_objects = XtCreateManagedWidget("num_objects", labelWidgetClass,
					    file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "Figure Offset");
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	figure_off = XtCreateManagedWidget("fig_offset_label", labelWidgetClass,
				     file_panel, Args, ArgCount);
	FirstArg(XtNlabel, "X");
	NextArg(XtNfromHoriz, figure_off);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fig_offset_lbl_x = XtCreateManagedWidget("fig_offset_lbl_x", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 40);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNfromHoriz, fig_offset_lbl_x);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fig_offset_x = XtCreateManagedWidget("fig_offset_x", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, fig_offset_x);
	NextArg(XtNfromVert, num_objects);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	file_xoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, file_panel, Args, ArgCount);
	make_pulldown_menu(offset_unit_items, XtNumber(offset_unit_items), 
				   -1, "", file_xoff_unit_panel, file_xoff_unit_select);

	/* put the Y offset below the X */

	FirstArg(XtNlabel, "Y");
	NextArg(XtNfromHoriz, figure_off);
	NextArg(XtNfromVert, file_xoff_unit_panel);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fig_offset_lbl_y = XtCreateManagedWidget("fig_offset_lbl_y", labelWidgetClass,
				     file_panel, Args, ArgCount);

	FirstArg(XtNwidth, 40);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, "0");
	NextArg(XtNinsertPosition, 0);
	NextArg(XtNfromHoriz, fig_offset_lbl_y);
	NextArg(XtNfromVert, file_xoff_unit_panel);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	fig_offset_y = XtCreateManagedWidget("fig_offset_y", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	FirstArg(XtNfromHoriz, fig_offset_y);
	NextArg(XtNfromVert, file_xoff_unit_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
	file_yoff_unit_panel = XtCreateManagedWidget(offset_unit_items[appres.INCHES? 0: 1],
				menuButtonWidgetClass, file_panel, Args, ArgCount);
	make_pulldown_menu(offset_unit_items, XtNumber(offset_unit_items), 
				   -1, "", file_yoff_unit_panel, file_yoff_unit_select);

	FirstArg(XtNlabel, " Current File");
	NextArg(XtNfromVert, file_yoff_unit_panel);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	cfile_lab = XtCreateManagedWidget("cur_file_label", labelWidgetClass,
					  file_panel, Args, ArgCount);

	FirstArg(XtNlabel, cur_filename);
	NextArg(XtNfromVert, file_yoff_unit_panel);
	NextArg(XtNfromHoriz, cfile_lab);
	NextArg(XtNjustify, XtJustifyLeft);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNwidth, FILE_WID);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	cfile_text = XtCreateManagedWidget("cur_file_name", labelWidgetClass,
					   file_panel, Args, ArgCount);

	FirstArg(XtNlabel, "     Filename");
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	file = XtCreateManagedWidget("file_label", labelWidgetClass,
				     file_panel, Args, ArgCount);
	FirstArg(XtNfont, &temp_font);
	GetValues(file);

	/* make the filename slot narrow so we can show the preview to the right */
	FirstArg(XtNwidth, FILE_WID);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNeditType, XawtextEdit);
	NextArg(XtNstring, cur_filename);
	NextArg(XtNinsertPosition, strlen(cur_filename));
	NextArg(XtNfromHoriz, file);
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainRight);
	file_selfile = XtCreateManagedWidget("file_name", asciiTextWidgetClass,
					     file_panel, Args, ArgCount);
	XtOverrideTranslations(file_selfile,
			   XtParseTranslationTable(text_translations));

	if (!actions_added) {
	    XtAppAddActions(tool_app, file_actions, XtNumber(file_actions));
	    actions_added = True;
	    /* add action to load file */
	    XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));
	}

	/* make the directory list etc */
	change_directory(cur_file_dir);
	create_dirinfo(True, file_panel, file_selfile, &beside, &butbelow, &file_mask,
		       &file_dir, &file_flist, &file_dlist, FILE_ALT_WID, True);

	/*****************************************************/
	/* now make a preview canvas on the whole right side */
	/*****************************************************/

	/* first a form just to the right of the figure offset menu button */
	FirstArg(XtNfromHoriz, file_yoff_unit_panel);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainRight);
	NextArg(XtNright, XtChainRight);
	preview_form = XtCreateManagedWidget("preview_form", formWidgetClass,
				file_panel, Args, ArgCount);

	/* a title */
	FirstArg(XtNlabel, "Preview   ");
	NextArg(XtNborderWidth, 0);
	preview_label = XtCreateManagedWidget("preview_label", labelWidgetClass,
					    preview_form, Args, ArgCount);

	/* make a label for the figure size */
	FirstArg(XtNlabel, " ");
	NextArg(XtNfromHoriz, preview_label);
	NextArg(XtNborderWidth, 0);
	preview_size = XtCreateManagedWidget("preview_size", labelWidgetClass,
					    preview_form, Args, ArgCount);

	/* and one for the figure name in the preview */
	FirstArg(XtNlabel, " ");
	NextArg(XtNfromVert, preview_label);
	NextArg(XtNborderWidth, 0);
	preview_name = XtCreateManagedWidget("preview_name", labelWidgetClass,
					    preview_form, Args, ArgCount);

	/* now a label widget for the preview canvas */

	/* first create a pixmap for its background, into which we'll draw the figure */
	/* actually, make one for landscape and one for portrait */
	preview_land_pixmap = XCreatePixmap(tool_d, canvas_win, 
			PREVIEW_CANVAS_W, PREVIEW_CANVAS_H, tool_dpth);
	preview_port_pixmap = XCreatePixmap(tool_d, canvas_win, 
			PREVIEW_CANVAS_H, PREVIEW_CANVAS_W, tool_dpth);

	/* make a form for the preview widget so we can center it */

	FirstArg(XtNwidth, PREVIEW_CANVAS_W+8);
	NextArg(XtNheight, PREVIEW_CANVAS_W+8);
	NextArg(XtNfromVert, preview_name);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	preview_widget_form = XtCreateManagedWidget("preview_widget_form",
				formWidgetClass, preview_form, Args, ArgCount);

	/* make a dummy label to fill the space until we put up the preview */
	FirstArg(XtNlabel, "");
	NextArg(XtNwidth, PREVIEW_CANVAS_W);
	NextArg(XtNheight, PREVIEW_CANVAS_W);
	dummy_label = XtCreateManagedWidget("dummy_label", labelWidgetClass,
					  preview_widget_form, Args, ArgCount);

	/* start the widget preview in portrait */
	/* don't manage it yet until the panel is popped up */

	FirstArg(XtNlabel, "");
	NextArg(XtNbackgroundPixmap, preview_port_pixmap);
	NextArg(XtNhorizDistance, (PREVIEW_CANVAS_W-PREVIEW_CANVAS_H)/2+4); /* center */
	NextArg(XtNvertDistance, 4);
	NextArg(XtNwidth, PREVIEW_CANVAS_H);
	NextArg(XtNheight, PREVIEW_CANVAS_W);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	preview_widget = XtCreateWidget("preview_widget", labelWidgetClass,
					  preview_widget_form, Args, ArgCount);
	/* erase the preview pixmaps */
	clear_preview();

	/* label for the figure comments */

	FirstArg(XtNlabel, "Figure comments:");
	NextArg(XtNfromVert, preview_widget_form);
	below = XtCreateManagedWidget("comments_label", labelWidgetClass,
					  preview_form, Args, ArgCount);
	/* window for the figure comments */

	FirstArg(XtNwidth, PREVIEW_CANVAS_W+8);
	NextArg(XtNleftMargin, 4);
	NextArg(XtNheight, 50);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 1);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNscrollVertical, XawtextScrollWhenNeeded);
	NextArg(XtNdisplayCaret, False);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	comments_widget = XtCreateManagedWidget("comments", asciiTextWidgetClass,
					     preview_form, Args, ArgCount);

	/* make cancel preview button */
	FirstArg(XtNlabel, "Stop Preview");
	NextArg(XtNsensitive, False);
	NextArg(XtNfromVert, comments_widget);
	preview_stop = XtCreateManagedWidget("preview_stop", commandWidgetClass,
				       preview_form, Args, ArgCount);
	XtAddEventHandler(preview_stop, ButtonReleaseMask, False,
			  (XtEventHandler)file_preview_stop, (XtPointer) NULL);

	/* end of preview form */

	/* now make buttons along the bottom */

	FirstArg(XtNlabel, "Cancel");
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNfromVert, butbelow);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	beside = cancel_button = XtCreateManagedWidget("cancel", commandWidgetClass,
				       file_panel, Args, ArgCount);
	XtAddEventHandler(cancel_button, ButtonReleaseMask, False,
			  (XtEventHandler)cancel_request, (XtPointer) NULL);

	/* now create Save, Open and Merge buttons but don't manage them - that's
	   done in popup_saveas_panel, popup_open_panel, and popup_merge_panel */

	FirstArg(XtNlabel, " Save ");
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, butbelow);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	save_button = XtCreateWidget("save", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(save_button, ButtonReleaseMask, False,
			  (XtEventHandler) do_save, (XtPointer) NULL);

	FirstArg(XtNlabel, " Open ");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, butbelow);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	load_button = XtCreateWidget("load", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(load_button, ButtonReleaseMask, False,
			  (XtEventHandler)load_request, (XtPointer) NULL);

	FirstArg(XtNlabel, "Merge ");
	NextArg(XtNhorizDistance, 20);
	NextArg(XtNfromVert, butbelow);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNheight, 25);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	merge_button = XtCreateWidget("merge", commandWidgetClass,
				      file_panel, Args, ArgCount);
	XtAddEventHandler(merge_button, ButtonReleaseMask, False,
			  (XtEventHandler)merge_request, (XtPointer) NULL);

	FirstArg(XtNlabel, "New xfig...");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, butbelow);
	NextArg(XtNfromHoriz, merge_button);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	NextArg(XtNleft, XtChainLeft);
	NextArg(XtNright, XtChainLeft);
	new_xfig_button = XtCreateWidget("new_xfig", commandWidgetClass,
				     file_panel, Args, ArgCount);
	XtAddEventHandler(new_xfig_button, ButtonReleaseMask, False,
			  (XtEventHandler)new_xfig_request, (XtPointer) NULL);

	XtInstallAccelerators(file_panel, cancel_button);
	XtInstallAccelerators(file_panel, save_button);
	XtInstallAccelerators(file_panel, load_button);
	XtInstallAccelerators(file_panel, new_xfig_button);
	XtInstallAccelerators(file_panel, merge_button);

}

/* check if user pressed cancel button */
Boolean
check_cancel(void)
{
    if (preview_in_progress) {
	process_pending();
	return cancel_preview;
    }
    return False;
	
}

static void
clear_preview(void)
{
    FirstArg(XtNlabel, "                                 ");
    SetValues(preview_name);

    FirstArg(XtNlabel, "                     ");
    SetValues(preview_size);

    /* clear both port and land pixmap */
    XSetForeground(tool_d, gccache[ERASE], x_bg_color.pixel);
    XFillRectangle(tool_d, preview_land_pixmap, gccache[ERASE], 0, 0, 
		   PREVIEW_CANVAS_W, PREVIEW_CANVAS_H);
    XFillRectangle(tool_d, preview_port_pixmap, gccache[ERASE], 0, 0, 
		   PREVIEW_CANVAS_H, PREVIEW_CANVAS_W);

    FirstArg(XtNbackgroundPixmap, (Pixmap)0);
    SetValues(preview_widget);
    /* put port pixmap in widget */
    FirstArg(XtNbackgroundPixmap, preview_port_pixmap);
    SetValues(preview_widget);
}

static void
file_preview_stop(Widget w, XButtonEvent *ev)
{
    /* set the cancel flag */
    cancel_preview = True;
    /* make the cancel button insensitive */
    XtSetSensitive(preview_stop, False);
    process_pending();
}

void preview_figure(char *filename, Widget parent, Widget canvas, Widget size_widget, Pixmap port_pixmap, Pixmap land_pixmap)
{
    fig_settings    settings;
    int		save_objmask;
    int		save_zoomxoff, save_zoomyoff;
    float	save_zoomscale;
    Boolean	save_shownums;
    F_compound	*figure;
    int		xmin, xmax, ymin, ymax;
    float	width, height, size;
    int		pixwidth, pixheight;
    int		i, status;
    char	figsize[50];
    Pixel	pb;

    /* alloc a compound object - we must do it this way so free_compound()
       works properly */
    figure = create_compound();

    /* if already previewing file, return */
    if (preview_in_progress == True)
	return;

    /* clear the cancel flag */
    cancel_preview = False;

    /* and any request to load a file */
    file_load_request = False;
    /* and merge */
    file_merge_request = False;
    /* and save */
    file_save_request = False;
    /* and cancel */
    file_cancel_request = False;

    /* say we are in progress */
    preview_in_progress = True;

    /* save active layer array and set all to True */
    save_active_layers();
    save_depths();
    save_counts_and_clear();
    reset_layers();

    /* save user colors in case preview changes them */
    if (!user_colors_saved) {
	save_user_colors();
	save_nuser_colors();
    }

    /* make the cancel button sensitive */
    XtSetSensitive(preview_stop, True);
    /* change label to read "Previewing" */
    FirstArg(XtNlabel, "Previewing");
    SetValues(preview_label);
    /* get current background color of label */
    FirstArg(XtNbackground, &pb);
    GetValues(preview_label);
    /* set it to red while previewing */
    FirstArg(XtNbackground, x_color(YELLOW));
    SetValues(preview_label);

    /* give filename a chance to appear in the Filename field */
    process_pending();

    /* first, save current zoom settings */
    save_zoomscale	= display_zoomscale;
    save_zoomxoff	= zoomxoff;
    save_zoomyoff	= zoomyoff;

    /* save and turn off showing vertex numbers */
    save_shownums	= appres.shownums;
    appres.shownums	= False;

    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(parent));

    /* make wait cursor */
    XDefineCursor(tool_d, XtWindow(parent), wait_cursor);
    /* define it for the file list panel too */
    XDefineCursor(tool_d, XtWindow(file_flist), wait_cursor);
    /* but use a "hand" cursor for the Preview Stop button */
    XDefineCursor(tool_d, XtWindow(preview_stop), pick9_cursor);
    process_pending();
     
    /* if we haven't already saved colors of the images on the canvas */
    if (!image_colors_are_saved) {
	image_colors_are_saved = True;
	/* save current canvas image colors */
	for (i=0; i<avail_image_cols; i++) {
	    save_image_cells[i].red   = image_cells[i].red;
	    save_image_cells[i].green = image_cells[i].green;
	    save_image_cells[i].blue  = image_cells[i].blue;
	}
	save_avail_image_cols = avail_image_cols;
    }

    /* unmanage the preview widget in case we need to change its shape (port<->land) */
    XtUnmanageChild(canvas);
    /* fool the toolkit into drawing the new pixmap */
    FirstArg(XtNbackgroundPixmap, (Pixmap)0);
    SetValues(canvas);

    /* read the figure into the local F_compound "figure" */

    /* we'll ignore the stuff returned in "settings" */
    if ((status = read_figc(filename,figure, DONT_MERGE, REMAP_IMAGES, 0,0,&settings)) != 0) {
	switch (status) {
	   case -1: file_msg("Bad format");
		   break;
	   case -2: file_msg("File is empty");
		   break;
	   default: file_msg("Error reading %s: %s",filename,strerror(errno));
	}
	/* clear pixmap of any figure so user knows something went wrong */
	clear_preview();
    } else {
	/*
	 *  successful read, get the size 
	*/

	add_compound_depth(figure);	/* count objects at each depth */

	/* update the preview name */
	FirstArg(XtNlabel, filename);
	SetValues(preview_name);

	/* put in any comments */
	if (figure->comments) {
	    FirstArg(XtNstring, figure->comments);
	} else {
	    FirstArg(XtNstring, "");
	}
	SetValues(comments_widget);

	/* get bounds of figure */
	xmin = figure->nwcorner.x;
	xmax = figure->secorner.x;
	ymin = figure->nwcorner.y;
	ymax = figure->secorner.y;
	width = xmax - xmin;
	height = ymax - ymin;
	size = max2(width,height) / ZOOM_FACTOR;
	/* calculate size in current units */
	if (width < PIX_PER_INCH || height < PIX_PER_INCH) {
	    /* for small figures, show more decimal places */
	    if (appres.INCHES) {
		sprintf(figsize, "%.2f x %.2f in",width/PIX_PER_INCH,height/PIX_PER_INCH);
	    } else {
		sprintf(figsize, "%.1f x %.1f cm",width/PIX_PER_CM,height/PIX_PER_CM);
	    }
	} else {
	    if (appres.INCHES) {
		sprintf(figsize, "%.1f x %.1f in",width/PIX_PER_INCH,height/PIX_PER_INCH);
	    } else {
		sprintf(figsize, "%.0f x %.0f cm",width/PIX_PER_CM,height/PIX_PER_CM);
	    }
	}

	if (size_widget) {
	    FirstArg(XtNlabel, figsize);
	    SetValues(preview_size);
    	}

	/* now switch the drawing canvas to our preview window and set width/height */
	if (settings.landscape) {
	    canvas_win = (Window) land_pixmap;
	    pixwidth = PREVIEW_CANVAS_W;
	    pixheight = PREVIEW_CANVAS_H;
	} else {
	    canvas_win = (Window) port_pixmap;
	    pixwidth = PREVIEW_CANVAS_H;
	    pixheight = PREVIEW_CANVAS_W;
	}

	/* scale to fit the preview canvas */
	display_zoomscale = 
			min2((float)(pixwidth-8)/width, (float)(pixheight-8)/height) *
				ZOOM_FACTOR;
	/* limit magnification to 5.0 */
	if (display_zoomscale > 5.0)
		display_zoomscale = 5.0;
	zoomscale = display_zoomscale/ZOOM_FACTOR;

	/* center the figure in the canvas */
	zoomxoff = -(pixwidth/zoomscale - width) / 2.0 + xmin;
	zoomyoff = -(pixheight/zoomscale - height) / 2.0 + ymin;

	/* insure that the most recent colormap is installed */
	/* we may have switched to a private colormap when previewing this figure */
	set_cmap(XtWindow(parent));

	/* no markers */
	save_objmask = cur_objmask;
	cur_objmask = 0;

	/* clear the pixmap with the canvas background color */
	XSetForeground(tool_d, gccache[ERASE], x_bg_color.pixel);
	XFillRectangle(tool_d, canvas_win, gccache[ERASE], 0, 0, pixwidth, pixheight);

	/* if he pressed "cancel preview" while reading file, skip displaying figure */
	if (! check_cancel()) {
	    /* draw the preview into the pixmap */
	    redisplay_objects(figure);
	}

	/* restore marker flag */
	cur_objmask = save_objmask;

	/* put the pixmap back in and set the width and height and center it */
	FirstArg(XtNbackgroundPixmap, canvas_win);
	NextArg(XtNwidth, pixwidth);
	NextArg(XtNheight, pixheight);
	if (settings.landscape) {
	    NextArg(XtNvertDistance, (PREVIEW_CANVAS_W-PREVIEW_CANVAS_H)/2+4);
	    NextArg(XtNhorizDistance, 4);
	} else {
	    NextArg(XtNvertDistance, 4);
	    NextArg(XtNhorizDistance, (PREVIEW_CANVAS_W-PREVIEW_CANVAS_H)/2+4);
	}
	SetValues(canvas);
	XtManageChild(canvas);
    }

    /* free the figure compound */
    free_compound(&figure);

    /* switch canvas back */
    canvas_win = main_canvas;

    /* restore active layer array */
    restore_active_layers();
    /* and depth and counts */
    restore_depths();
    restore_counts();

    /* now restore settings for main canvas/figure */
    display_zoomscale	= save_zoomscale;
    zoomxoff		= save_zoomxoff;
    zoomyoff		= save_zoomyoff;
    zoomscale		= display_zoomscale/ZOOM_FACTOR;

    /* restore shownums */
    appres.shownums	= save_shownums;

    /* reset cursors */
    XUndefineCursor(tool_d, XtWindow(parent));
    XUndefineCursor(tool_d, XtWindow(file_flist));
    XUndefineCursor(tool_d, XtWindow(preview_stop));

    /* reset the cancel button and flag */
    cancel_preview = False;

    /* make the cancel button insensitive */
    XtSetSensitive(preview_stop, False);

    /* change label to read "Preview" */
    FirstArg(XtNlabel, "Preview");
    /* and change background to original color */
    NextArg(XtNbackground, pb);
    SetValues(preview_label);

    process_pending();
    /* check if user had double clicked on filename which means he wanted
       to load the file, not just preview it */

    /* say we are finished */
    preview_in_progress = False;

    if (file_cancel_request) {
	cancel_preview = False;
	file_panel_cancel((Widget) 0, (XButtonEvent *) 0);
    }
    if (file_load_request) {
	cancel_preview = False;
	do_load((Widget) 0, (XButtonEvent *) 0);
	/* if redisplay_region was called don't bother because
	   we're loading a file over it */
	request_redraw = False;		
    }
    if (file_merge_request) {
	cancel_preview = False;
	do_merge((Widget) 0, (XButtonEvent *) 0);
    }
    if (file_save_request) {
	cancel_preview = False;
	do_save((Widget) 0, (XButtonEvent *) 0);
    }
    /* if user requested a canvas redraw while preview was being generated
       do that now for full canvas */
    if (request_redraw) {
	redisplay_region(0, 0, CANVAS_WD, CANVAS_HT);
	request_redraw = False;
    }
}
