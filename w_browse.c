/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 Jim Daley (jdaley@cix.compulink.co.uk)
 * Parts Copyright (c) 1995-2002 by Brian V. Smith
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
#include "w_drawprim.h"
#include "w_dir.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"

#include "f_util.h"
#include "w_color.h"
#include "w_cursor.h"

/* EXPORTS */

char		cur_browse_dir[PATH_MAX];	/* current directory for browsing */

Widget		browse_selfile,	/* selected file widget */
		browse_mask,	/* mask widget */
		browse_dir,	/* current directory widget */
		browse_flist,	/* file list wiget */
		browse_dlist;	/* dir list wiget */
Boolean		browse_up;


DeclareStaticArgs(12);
static Widget	cfile_lab;
static Widget	closebtn, apply;
static Widget	browse_parent;
static Position xposn, yposn;
static Widget	browse_panel,
		browse_popup;	/* the popup itself */

static String	file_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	<Btn1Up>(2): ApplyBrowseAndClose()\n\
	<Key>Return: ApplyBrowseAndClose()\n";

static String	file_name_translations =
	"<Key>Return: ApplyBrowseAndClose()\n";
static void	browse_panel_close(Widget w, XButtonEvent *ev);
void		got_browse(Widget w, XButtonEvent *ev);

static XtActionsRec	file_name_actions[] =
{
    {"ApplyBrowse", (XtActionProc) got_browse},
};

static String	file_translations =
	"<Message>WM_PROTOCOLS: DismissBrowse()\n";

static XtActionsRec	file_actions[] =
{
    {"DismissBrowse", (XtActionProc) browse_panel_close},
    {"CloseBrowse", (XtActionProc) browse_panel_close},
    {"ApplyBrowse", (XtActionProc) got_browse},
    {"ApplyBrowseAndClose", (XtActionProc) browse_panel_close},
};

static char browse_filename[PATH_MAX];
static char local_dir[PATH_MAX];

static char file_viewed[PATH_MAX];	/* name of file already previewed */


void create_browse_panel (Widget w);

static void
browse_panel_dismiss(void)
{
    char	   *fval;

    FirstArg(XtNstring, &fval);
    GetValues(browse_selfile);	/* check the ascii widget for a filename */
    if (strcmp(fval, file_viewed) != 0) {
	/* apply any selected filename before closing panel */
	got_browse(0, 0);
    }
    FirstArg(XtNstring, "\0");
    SetValues( browse_selfile );   /* blank out name for next call */
    XtPopdown(browse_popup);
    XtSetSensitive(browse_parent, True);
    browse_up = False;
}

void
got_browse(Widget w, XButtonEvent *ev)
{
    char	   *fval, *dval, *path;

    if (browse_popup) {
	FirstArg(XtNstring, &dval);
	GetValues(browse_dir);
	FirstArg(XtNstring, &fval);
	GetValues(browse_selfile);	/* check the ascii widget for a filename */
	if (emptyname(fval)) {
	    fval = browse_filename;	/* Filename widget empty, use current filename */
	}
	strcpy(file_viewed, fval);	/* indicate name as viewed */

	if (emptyname_msg(fval, "Apply"))
	    return;

	path = new_string( strlen(dval) + 1 + strlen(fval) + 1);
	if ( path ) {
	    strcpy( path,dval );
	    strcat( path, "/");
	    strcat( path, fval );
	    panel_set_value( pic_name_panel, path );
	    free(path);
	}
	push_apply_button();  /* slightly iffy - assumes called from edit picture */
    }
}

static void
browse_panel_close(Widget w, XButtonEvent *ev)
{
    browse_panel_dismiss();
}

void popup_browse_panel(Widget w)
{
    char *fval, *pval;

    set_temp_cursor(wait_cursor);
    XtSetSensitive(w, False);
    browse_parent = w;
    browse_up = True;
    file_viewed[0] = '\0';

    if (!browse_popup) {
	get_directory(local_dir);
    } else {
	strcpy(local_dir, cur_browse_dir);
    }

    /* move to the file directory  - if not the current dir
        and set up the file/directory values
    */
    pval = (char*) panel_get_value( pic_name_panel );
    fval = strrchr(pval, '/');
    if ( !fval ) {	/* no path in name so just use name */
      strcpy(browse_filename, pval);
    } else {		/* set us up in the same path as the file */
      strcpy(local_dir, pval);
      strcpy(browse_filename, fval+1);
      local_dir[strlen(pval) - strlen(fval)] = '\0';
      (void) change_directory(local_dir);
    }

    if (!browse_popup) {
	create_browse_panel(w);
    }

    FirstArg(XtNstring, local_dir);
    SetValues(browse_dir);
    FirstArg(XtNstring, browse_filename);
    SetValues(browse_selfile);	

    XtPopup(browse_popup, XtGrabNonexclusive);
    Rescan(0, 0, 0, 0);

    (void) XSetWMProtocols(tool_d, XtWindow(browse_popup), &wm_delete_window, 1);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    set_cmap(XtWindow(browse_popup));  /* ensure most recent cmap is installed */
    reset_cursor();
}

void create_browse_panel(Widget w)
{
	Widget		   file, beside, below;
	XFontStruct	  *temp_font;
	static Boolean	   actions_added=False;

	XtTranslateCoords(w, (Position) 0, (Position) 0, &xposn, &yposn);

	FirstArg(XtNx, xposn);
	NextArg(XtNy, yposn + 50);
	NextArg(XtNtitle, "Xfig: Browse files for picture import");
	browse_popup = XtCreatePopupShell("xfig_browse_menu",
					transientShellWidgetClass,
					tool, Args, ArgCount);
	XtOverrideTranslations(browse_popup,
			   XtParseTranslationTable(file_translations));

	browse_panel = XtCreateManagedWidget("browse_panel", formWidgetClass,
					   browse_popup, NULL, ZERO);

	FirstArg(XtNlabel, "     Filename");
	NextArg(XtNvertDistance, 15);
	NextArg(XtNborderWidth, 0);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	file = XtCreateManagedWidget("file_label", labelWidgetClass,
				     browse_panel, Args, ArgCount);
	FirstArg(XtNfont, &temp_font);
	GetValues(file);

	FirstArg(XtNwidth, 250);
	NextArg(XtNheight, max_char_height(temp_font) * 2 + 4);
	NextArg(XtNeditType, "edit");
	NextArg(XtNstring, browse_filename);
	NextArg(XtNinsertPosition, strlen(browse_filename));
	NextArg(XtNfromHoriz, file);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNfromVert, cfile_lab);
	NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
	NextArg(XtNtop, XtChainTop);
	NextArg(XtNbottom, XtChainTop);
	browse_selfile = XtCreateManagedWidget("file_name", asciiTextWidgetClass,
					     browse_panel, Args, ArgCount);
	XtOverrideTranslations(browse_selfile,
			   XtParseTranslationTable(text_translations));

	if (!actions_added) {
	    XtAppAddActions(tool_app, file_actions, XtNumber(file_actions));
	    actions_added = True;
	    /* add action to apply file */
	    XtAppAddActions(tool_app, file_name_actions, XtNumber(file_name_actions));
	}

	create_dirinfo(False, browse_panel, browse_selfile, &beside, &below, &browse_mask,
		       &browse_dir, &browse_flist, &browse_dlist, E_FILE_WIDTH, False);

	/* make <return> in the filename window apply the file */
	XtOverrideTranslations(browse_selfile,
			   XtParseTranslationTable(file_name_translations));

	/* make <return> and a double click in the file list window apply the file and close */
	XtAugmentTranslations(browse_flist,
			   XtParseTranslationTable(file_list_translations));
	FirstArg(XtNlabel, " Close ");
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNfromHoriz, beside);
	NextArg(XtNfromVert, below);
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	closebtn = XtCreateManagedWidget("close", commandWidgetClass,
				       browse_panel, Args, ArgCount);
	XtAddEventHandler(closebtn, ButtonReleaseMask, False,
			  (XtEventHandler)browse_panel_close, (XtPointer) NULL);


	FirstArg(XtNlabel,  " Apply ");
	NextArg(XtNborderWidth, INTERNAL_BW);
	NextArg(XtNfromHoriz, closebtn);
	NextArg(XtNfromVert, below);
	NextArg(XtNvertDistance, 15);
	NextArg(XtNhorizDistance, 25);
	NextArg(XtNheight, 25);
	NextArg(XtNtop, XtChainBottom);
	NextArg(XtNbottom, XtChainBottom);
	apply = XtCreateManagedWidget("apply", commandWidgetClass,
				     browse_panel, Args, ArgCount);
	XtAddEventHandler(apply, ButtonReleaseMask, False,
			  (XtEventHandler)got_browse, (XtPointer) NULL);
	XtInstallAccelerators(browse_panel, closebtn);
	XtInstallAccelerators(browse_panel, apply);
}
