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
#include "f_util.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_util.h"

#include "w_canvas.h"

char	*browsecommand;
Boolean  check_docfile(char *name);
char	 filename[PATH_MAX];

void	help_ok(Widget w, XtPointer closure, XtPointer call_data);

static String	about_translations =
	"<Message>WM_PROTOCOLS: DismissAbout()\n";
static XtActionsRec	about_actions[] =
{
    {"DismissAbout", (XtActionProc) help_ok},
};


void launch_viewer (char *filename, char *message, char *viewer);

void
launch_refman(Widget w, XtPointer closure, XtPointer call_data)
{
	/* first check if at least the index file is installed */
	sprintf(filename, "%s/html/index.html", XFIGDOCDIR);
#ifdef I18N
	if (appres.international && getenv("LANG")) {
	  /* check localized file ($XFIGDOCDIR/html/$LANG/index.html) first */
	  sprintf(filename, "%s/html/%s/index.html", XFIGDOCDIR, getenv("LANG"));
	  if (!check_docfile(filename))
	    sprintf(filename, "%s/html/index.html", XFIGDOCDIR);
	}
#endif /* I18N */
	if (!check_docfile(filename))
		return;
	launch_viewer(filename, "Launching Web browser for html pages", cur_browser);
}

void
launch_refpdf_en(Widget w, XtPointer closure, XtPointer call_data)
{
	sprintf(filename,"%s/xfig_ref_en.pdf",XFIGDOCDIR);
	launch_viewer(filename,"Launching PDF viewer for Xfig reference", cur_pdfviewer);
}

void
launch_refpdf_jp(Widget w, XtPointer closure, XtPointer call_data)
{
	sprintf(filename,"%s/xfig_ref_jp.pdf",XFIGDOCDIR);
	launch_viewer(filename,"Launching PDF viewer for Xfig reference", cur_pdfviewer);
}


void
launch_howto(Widget w, XtPointer closure, XtPointer call_data)
{
	sprintf(filename,"%s/xfig-howto.pdf",XFIGDOCDIR);
	launch_viewer(filename,"Launching PDF viewer for How-to Tutorial", cur_pdfviewer);
}

void
launch_man(Widget w, XtPointer closure, XtPointer call_data)
{
	sprintf(filename,"%s/xfig_man.html",XFIGDOCDIR);
	launch_viewer(filename,"Launching Web browser for man pages", cur_browser);
}

void launch_viewer(char *filename, char *message, char *viewer)
{
	/* turn off Compose key LED */
	setCompLED(0);

	/* first check if the file is installed */
	if (!check_docfile(filename))
	    return;
	/* now replace the %f in the browser command with the filename and add the "&" */
	browsecommand = build_command(viewer, filename);
	put_msg(message);
	system(browsecommand);
	free(browsecommand);
}

Boolean
check_docfile(char *name)
{
	struct	stat file_status;
	if (stat(name, &file_status) != 0) {	/* something wrong */
	    if (errno == ENOENT) {
		file_msg("%s is not installed, please install package xfig-doc.",name);
	    } else {
		file_msg("System error: %s on file %s",strerror(errno),name);
	    }
	    beep();
	    return False;
	}
	return True;
}

static Widget    help_popup = (Widget) 0;

void
help_ok(Widget w, XtPointer closure, XtPointer call_data)
{
	XtPopdown(help_popup);
}

void 
launch_about(Widget w, XtPointer closure, XtPointer call_data)
{
    DeclareArgs(10);
    Widget    form, icon, ok;
    Position  x, y;
    char	  info[400];

    /* turn off Compose key LED */
    setCompLED(0);

    /* don't make more than one */
    if (!help_popup) {

	/* get the position of the help button */
	XtTranslateCoords(w, (Position) 0, (Position) 0, &x, &y);
	FirstArg(XtNx, x);
	NextArg(XtNy, y);
	help_popup = XtCreatePopupShell("About Xfig",transientShellWidgetClass,
				tool, Args, ArgCount);
	XtOverrideTranslations(help_popup,
			   XtParseTranslationTable(about_translations));
	XtAppAddActions(tool_app, about_actions, XtNumber(about_actions));

	FirstArg(XtNborderWidth, 0);
	form = XtCreateManagedWidget("help_form", formWidgetClass, help_popup,
				Args, ArgCount);
	/* put the xfig icon in a label and another label saying which version this is */
	FirstArg(XtNbitmap, fig_icon);
	NextArg(XtNinternalHeight, 0);
	NextArg(XtNinternalWidth, 0);
	NextArg(XtNborderWidth, 0);
	icon = XtCreateManagedWidget("xfig_icon", labelWidgetClass, form, Args, ArgCount);

	/* make up some information */
	strcpy(info,xfig_version);
	strcat(info,"\n  Copyright \251 1985-1988 by Supoj Sutanthavibul");
	strcat(info,"\n  Parts Copyright \251 1989-2002 by Brian V. Smith (BVSmith@lbl.gov)");
	strcat(info,"\n  Parts Copyright \251 1991 by Paul King");
	strcat(info,"\n  See source files and man pages for other copyrights");

	FirstArg(XtNlabel, info);
	NextArg(XtNfromHoriz, icon);
	NextArg(XtNhorizDistance, 20);
	NextArg(XtNborderWidth, 0);
	XtCreateManagedWidget("xfig_icon", labelWidgetClass, form, Args, ArgCount);

	FirstArg(XtNlabel, " Ok ");
	NextArg(XtNwidth, 50);
	NextArg(XtNheight, 30);
	NextArg(XtNfromVert, icon);
	NextArg(XtNvertDistance, 20);
	ok = XtCreateManagedWidget("help_ok", commandWidgetClass, form, Args, ArgCount);
	XtAddCallback(ok, XtNcallback, help_ok, (XtPointer) NULL);
    }
    XtPopup(help_popup,XtGrabNone);
    (void) XSetWMProtocols(tool_d, XtWindow(help_popup), &wm_delete_window, 1);
}
