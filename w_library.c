/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1998 by Stephane Mancini
 * Parts Copyright (c) 1999-2002 by Brian V. Smith
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

/* This is where the library popup is created */

#include "fig.h"
#include "figx.h"
#include <stdarg.h>
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "e_placelib.h"
#include "e_placelib.h"
#include "f_read.h"
#include "u_create.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_drawprim.h"		/* for max_char_height */
#include "w_dir.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_library.h"
#include "w_listwidget.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_util.h"
#include "w_setup.h"
#include "w_icons.h"
#include "w_zoom.h"

#include "f_util.h"
#include "u_free.h"
#include "u_list.h"
#include "u_markers.h"
#include "u_translate.h"
#include "w_color.h"
#include "w_cursor.h"
#include "w_mousefun.h"

#ifndef XAW3D1_5E
#include "SmeCascade.h"
#endif /* XAW3D1_5E */

#ifdef HAVE_NO_DIRENT
#include <sys/dir.h>
#else
#include <dirent.h>
#endif /* HAVE_NO_DIRENT */

#define N_LIB_MAX	  100		/* max number of libraries */
#define N_LIB_OBJECT_MAX  400		/* max number of objects in a library */
#define N_LIB_NAME_MAX	  81		/* max length of library name + 1 */
#define N_LIB_LINE_MAX    300		/* one line in the file */

#define MAIN_WIDTH	  435		/* width of main panel */
#define LIB_PREVIEW_SIZE  150		/* size (square) of preview canvas */
#define LIB_FILE_WIDTH	  MAIN_WIDTH-LIB_PREVIEW_SIZE-20 /* size of object list panel */

/* STATICS */
static void	library_cancel(Widget w, XButtonEvent *ev), load_library(Widget w, XtPointer new_library, XtPointer garbage), library_stop(Widget w, XButtonEvent *ev);
static void	create_library_panel(void), libraryStatus(char *format,...);
static void	put_object_sel(Widget w, XButtonEvent *ev), set_cur_obj_name(int obj), erase_pixmap(Pixmap pixmap);
static void	sel_item_icon(Widget w, XButtonEvent *ev);
static void	sel_icon(int object), unsel_icon(int object);
static void	sel_view(Widget w, XtPointer new_view, XtPointer garbage), change_icon_size(Widget w, XtPointer menu_entry, XtPointer garbage);
static void	set_preview_name(int obj), update_preview(void);
static Boolean	preview_libobj(int objnum, Pixmap pixmap, int pixsize, int margin);
static void	copy_icon_to_preview(int object);

DeclareStaticArgs(15);

static Widget	library_popup=0;
static Widget	put_cur_obj;
static Widget	library_form, title;
static Widget	lib_obj_label, library_label, library_menu_button;
#ifndef XAW3D1_5E
static Widget	library_menu;
#endif /* XAW3D1_5E */
static Widget	sel_view_button;
static Widget	object_form;
static Widget	icon_form, icon_viewport, icon_box;
static Widget	list_form, list_viewport, object_list;
static Widget	icon_size_button;
static Widget	library_status;
static Widget	lib_preview_label, preview_lib_widget;
static Widget	comment_label, object_comments;
static Widget	cancel, selobj, stop;
static Widget	lib_buttons[N_LIB_OBJECT_MAX];

static Pixmap	preview_lib_pixmap, cur_obj_preview;
static Pixmap	lib_icons[N_LIB_OBJECT_MAX];
static Pixel	sel_color, unsel_color;
static int	num_library_names;
static int	num_list_items=0;
static int	which_num;
static int	char_ht,char_wd;
static int	prev_icon_size;
static char	*icon_sizes[] = { "40", "60", "80", "100", "120" };
static int	num_icon_sizes = sizeof(icon_sizes)/sizeof(char *);
static Boolean	loading_library = False;	/* lockout flag */
static Boolean	library_stop_request = False;	/* to stop loading library */

static String	library_translations =
			"<Message>WM_PROTOCOLS: DismissLibrary()\n\
			 <Key>Escape: DismissLibrary()\n\
			 <Btn1Up>(2): put_object_sel()\n\
			 <Key>Return: put_object_sel()\n";
static String	object_list_translations =
	"<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	 <Btn1Up>(2): put_object_sel()\n\
	 <Key>Return: put_object_sel()\n";
static String	object_icon_translations =
	"<Btn1Down>,<Btn1Up>: sel_item_icon()\n\
	 <Btn1Up>(2): put_object_sel()\n\
	 <Key>Return: put_object_sel()\n";

static XtActionsRec	library_actions[] =
{
  {"DismissLibrary",	(XtActionProc) library_cancel},
  {"library_cancel",	(XtActionProc) library_cancel},
  {"load_library",	(XtActionProc) load_library},
  {"sel_item_icon",	(XtActionProc) sel_item_icon},
  {"put_object_sel",	(XtActionProc) put_object_sel},
};

struct lib_rec {
  char	  *name;		/* directory name with Fig library files */
  char	  *longname;		/* longname (e.g. "Mechanical_Din / Holes / Through" */
  char	  *path;		/* full path */
  Boolean  figs_at_top;		/* whether or not there are Fig files in toplevel directory */
  int	   nsubs;		/* number of sub-directories, if any */
  struct lib_rec *subdirs[N_LIB_MAX+1];	/* array of lib_recs, one for each subdirectory */
};

static struct lib_rec	*cur_library=NULL;
static char	 	*cur_library_name=NULL;
static char	 	*cur_library_path=NULL, **cur_objects_names;

/* types for view menu */
static char	 *viewtypes[] = {"List View", "Icon View"};

static struct lib_rec *library_rec[N_LIB_MAX + 1];

char		**library_objects_texts=NULL;
F_libobject	**lib_compounds=NULL;

static Position xposn, yposn;

/* comparison function for filename sorting using qsort() */


static Boolean	MakeObjectLibrary(char *library_dir, char **objects_names, F_libobject **libobjects),MakeLibraryFileList(char *dir_name, char **obj_list);
static int	MakeLibrary(void);
static Boolean	ScanLibraryDirectory(Boolean at_top, struct lib_rec **librec, char *path, char *longname, char *name, Boolean *figs_at_top, int *nentries);
static Boolean	PutLibraryEntry(struct lib_rec *librec, char *path, char *lname, char *name);
static Boolean	lib_just_loaded, icons_made;
static Boolean	load_lib_obj(int obj);
static Widget	make_library_menu(Widget parent, char *name, struct lib_rec **librec, int num);


static int
SPComp(char **s1, char **s2)
{
  return (strcasecmp(*s1, *s2));
}

/* comparison function for librec sorting using qsort() */

static int
LRComp(struct lib_rec **r1, struct lib_rec **r2)
{
    struct lib_rec *name1, *name2;
    name1 = *r1;
    name2 = *r2;
    return (strcasecmp(name1->name, name2->name));
}

Boolean	put_select_requested = False;

void put_selected_request(void)
{
    if (preview_in_progress == True) {
	put_select_requested = True;
	/* request to cancel the preview */
	cancel_preview = True;
    } else {
	put_selected();
    }
}

/* request to stop loading library */

static void
library_stop(Widget w, XButtonEvent *ev)
{
     library_stop_request = True;
     XtSetSensitive(stop, False);
}

static void
library_dismiss(void)
{
  XtPopdown(library_popup);
  put_selected_request();
}

static void
library_cancel(Widget w, XButtonEvent *ev)
{
    /* unhighlight the selected item */
    if (appres.icon_view)
	unsel_icon(cur_library_object);
    XawListUnhighlight(object_list);

    /* make the "Select object" button inactive */
    FirstArg(XtNsensitive, False);
    SetValues(selobj);

    /* clear current object */
    cur_library_object = old_library_object;
    set_cur_obj_name(cur_library_object);

    /* if we didn't just load a new library, restore some stuff */
    if (!lib_just_loaded) {
	/* re-highlight the current object */
	if (appres.icon_view)
	    sel_icon(cur_library_object);

	/* restore old label in preview label */
	set_preview_name(cur_library_object);

	/* restore old comments */
#if 0  /* wrong argument to set_comments() ::  cur_library_object is type "int" !!! */
	set_comments(cur_library_object->comments);
#endif

	/* copy current object preview back to main preview pixmap */
	XCopyArea(tool_d, cur_obj_preview, preview_lib_pixmap, gccache[PAINT], 0, 0,
			LIB_PREVIEW_SIZE, LIB_PREVIEW_SIZE, 0, 0);
    }

    /* get rid of the popup */
    XtPopdown(library_popup);

    /* if user was in the library mode when he popped this up AND there is a
       library loaded, continue with old place */
    if (action_on && library_objects_texts[0] && cur_library_object >= 0) {
	put_selected_request();
    } else {
	/* otherwise, cancel all library operations */
	reset_action_on();
	canvas_kbd_proc = null_proc;
	canvas_locmove_proc = null_proc;
	canvas_ref_proc = null_proc;
	canvas_leftbut_proc = null_proc;
	canvas_middlebut_proc = null_proc;
	canvas_rightbut_proc = null_proc;
	set_mousefun("","","","","","");
    }
}

/* user has selected a library from the pulldown menu - load the objects */

static void
load_library(Widget w, XtPointer new_library, XtPointer garbage)
{
    struct lib_rec *librec = (struct lib_rec *) new_library;
    Dimension	    vwidth, vheight;
    Dimension	    vawidth, vaheight;
    Dimension	    lwidth, lheight;
    Dimension	    lawidth, laheight;
    int		    i;

#ifndef XAW3D1_5E
    /* pop down the menu */
    XtPopdown(library_menu);
#endif /* XAW3D1_5E */

    /* if already in the middle of loading a library, return */
    if (loading_library) {
	return;
    }

    /* set lockout flag */
    loading_library = True;

    /* clear stop request flag */
    library_stop_request = False;
    /* make the stop button sensitive */
    XtSetSensitive(stop, True);

    /* no object selected yet */
    old_library_object = cur_library_object = -1;

    /* first make sure markers are off or they'll show up in the icons */
    update_markers(M_NONE);

    /* only set current library if the load is successful */
    if (MakeObjectLibrary(librec->path,
		  library_objects_texts, lib_compounds) == True) {
	/* flag to say we just loaded the library but haven't picked anything yet */
	lib_just_loaded = True;
	/* set new */
	cur_library = librec;
	cur_library_name = librec->name;
	cur_library_path = librec->path;
	/* erase the preview and preview backup */
	erase_pixmap(preview_lib_pixmap);
	erase_pixmap(cur_obj_preview);
	/* force the toolkit to refresh it */
	update_preview();
	/* erase the comment window */
	set_comments("");
	/* erase the preview name */
	set_preview_name(-1);
	/* and the current object name */
	set_cur_obj_name(-1);

	/* put long library name in button */
	FirstArg(XtNlabel, librec->longname);
	SetValues(library_menu_button);

	/* put the objects in the list */
	FirstArg(XtNwidth, &lwidth);
	NextArg(XtNheight, &lheight);
	GetValues(object_list);
	FirstArg(XtNwidth, &vwidth);
	NextArg(XtNheight, &vheight);
	GetValues(list_viewport);

	XawListChange(object_list, library_objects_texts, 0, 0, True);
	FirstArg(XtNwidth, &lawidth);
	NextArg(XtNheight, &laheight);
	GetValues(object_list);
	FirstArg(XtNwidth, &vawidth);
	NextArg(XtNheight, &vaheight);
	GetValues(list_viewport);

	/* make the "Select object" button inactive */
	FirstArg(XtNsensitive, False);
	SetValues(selobj);
    }
    /* clear lockout flag */
    loading_library = False;

    /* install the wheel scrolling for the scrollbar in the list widget */
    InstallScroll(object_list);
    /* and the icon box */
    InstallScroll(icon_box);
    for (i=0; i<num_list_items; i++)
	InstallScrollParent(lib_buttons[i]);
}

/* come here when the user double clicks on object or
   clicks Select Object button */

static void
put_object_sel(Widget w, XButtonEvent *ev)
{ 	
    int	    obj;

    /* if there is no library loaded, ignore select */
    if ((library_objects_texts[0] == 0) || (cur_library_object == -1))
	return;

    /* if user was placing another object, remove it from the list first */
    if (action_on)
	remove_compound_depth(new_c);

    obj = cur_library_object;
     /* create a compound for the chosen object if it hasn't already been created */
    if (load_lib_obj(obj) == False)
	    return;		/* problem loading object */
    old_library_object = cur_library_object;
    /* copy current preview pixmap to current object preview pixmap */
    XCopyArea(tool_d, preview_lib_pixmap, cur_obj_preview, gccache[PAINT], 0, 0,
			LIB_PREVIEW_SIZE, LIB_PREVIEW_SIZE, 0, 0);
    library_dismiss();
}

/* read the library object file and put into a compound */

static Boolean
load_lib_obj(int obj)
{
	fig_settings     settings;
	char		 fname[PATH_MAX];
	char		 save_file_dir[PATH_MAX];
	F_compound	*c, *c2;
	Boolean		 status;

	/* if already loaded, return */
	if (lib_compounds[obj]->compound != 0)
		return True;

	libraryStatus("Loading %s",cur_objects_names[obj]);

	/* go to the library directory in case there are picture files to load */
	strcpy(save_file_dir, cur_file_dir);
	strcpy(cur_file_dir, cur_library_path);
	change_directory(cur_file_dir);

	c = lib_compounds[obj]->compound = create_compound();

	/* read in the object file */
	/* we'll ignore the stuff returned in "settings" */
	sprintf(fname,"%s%s",cur_objects_names[obj],".fig");

	if (read_figc(fname, c, MERGE, REMAP_IMAGES, 0, 0, &settings)==0) {
	    /* if there are no other objects than one compound, don't encapsulate
		it in another compound */
	    if (c->arcs == 0 && c->ellipses == 0 && c->lines == 0 && 
		c->splines == 0 && c->texts == 0) {
		/* but first check that there is a compound */
		if (c->compounds == 0) {
		    /* an error reading a .fig file */
		    file_msg("Empty library file: %s.fig, ignoring",cur_objects_names[obj]);
		    /* delete the compound we just created */
		    delete_compound(lib_compounds[obj]->compound);
		    lib_compounds[obj]->compound = (F_compound *) NULL;
		    /* go back to the file directory */
		    strcpy(cur_file_dir, save_file_dir);
		    change_directory(cur_file_dir);
		    return False;
		}
		/* now check if the compound is the only one */
		if (c->compounds->next == 0) {
		    /* yes, save ptr to embedded compound */
		    c2 = c->compounds;
		    /* move the top comment down after freeing any existing in that compound */
		    if (c2->comments)
			 free((char *) c2->comments);
		    c2->comments = c->comments;
		    /* free the toplevel */
		    free((char *) c);
		    /* make the embedded compound the toplevel */
	            c = c2;
		    lib_compounds[obj]->compound = c;
		}
	    }
	    /* save the upper-left corner before we translate it to 0,0 */
	    lib_compounds[obj]->corner.x = lib_compounds[obj]->compound->nwcorner.x;
	    lib_compounds[obj]->corner.y = lib_compounds[obj]->compound->nwcorner.y;
	    /* now translate it to 0,0 */
	    translate_compound(lib_compounds[obj]->compound,
			 -lib_compounds[obj]->compound->nwcorner.x,
			 -lib_compounds[obj]->compound->nwcorner.y);
	    status = True;
	} else {
		/* an error reading a .fig file */
		file_msg("Error reading %s.fig",cur_objects_names[obj]);
		/* delete the compound we just created */
		delete_compound(lib_compounds[obj]->compound);
		lib_compounds[obj]->compound = (F_compound *) NULL;
		status = False;
	}
	/* go back to the file directory */
	strcpy(cur_file_dir, save_file_dir);
	change_directory(cur_file_dir);
	return status;
}

/* come here when user clicks on library object name */

static void
NewObjectSel(Widget w, XtPointer closure, XtPointer call_data)
{
    int		    new_obj;
    XawListReturnStruct *ret_struct = (XawListReturnStruct *) call_data;

    new_obj = ret_struct->list_index;
    if (icons_made) {
	/* unhighlight the current view icon */
	unsel_icon(cur_library_object);
	/* highlight the new one */
	sel_icon(new_obj);
    }
    /* make current */
    cur_library_object = new_obj;
    /* put name in name field */
    set_cur_obj_name(cur_library_object);

    /* change preview label */
    FirstArg(XtNlabel,library_objects_texts[cur_library_object]);
    SetValues(lib_obj_label);

    /* we have picked an object */
    lib_just_loaded = False;

    /* show a preview of the figure in the preview canvas */
    if (preview_libobj(cur_library_object, preview_lib_pixmap,
			LIB_PREVIEW_SIZE, 20) == False)
	return;

    /* change label in preview label */
    FirstArg(XtNlabel,library_objects_texts[cur_library_object]);
    SetValues(lib_obj_label);

    /* make the "Select object" button active now */
    FirstArg(XtNsensitive, True);
    SetValues(selobj);
}

void
popup_library_panel(void)
{
    set_temp_cursor(wait_cursor);
    if (!library_popup)
	create_library_panel();

    old_library_object = cur_library_object;
    XtPopup(library_popup, XtGrabNonexclusive);

    /* raise the window of the (form of the) view we want */
    if (appres.icon_view) {
	XRaiseWindow(tool_d, XtWindow(icon_form));
    } else {
	XRaiseWindow(tool_d, XtWindow(list_form));
    }

    /* get background color of box to "uncolor" borders of object icons */
    FirstArg(XtNbackground, &unsel_color);
    GetValues(icon_box);
    /* use black for "selected" border color */
    sel_color = x_color(BLACK);

    /* if the file message window is up add it to the grab */
    file_msg_add_grab();

    /* now put in the current (first) library name */
    /* if no libraries, indicate so */
    if (num_library_names == 0) {
	FirstArg(XtNlabel, "No libraries");
    } else if (cur_library_name == NULL ) {
	FirstArg(XtNlabel, "None Loaded");
    } else {
	FirstArg(XtNlabel, cur_library->longname);
    }
    SetValues(library_menu_button);

    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(library_popup));
    (void) XSetWMProtocols(tool_d, XtWindow(library_popup), &wm_delete_window, 1);

    reset_cursor();
}

static void
create_library_panel(void)
{
    Widget	 status_label;
    int		 i;
    XFontStruct	*temp_font;
    static Boolean actions_added=False;
    Widget	 beside;
    char		 buf[20];

    app_flush();

    /* make sure user doesn't request an icon size too small or large */
    if (appres.library_icon_size < atoi(icon_sizes[0]))
	appres.library_icon_size = atoi(icon_sizes[0]);
    if (appres.library_icon_size > atoi(icon_sizes[num_icon_sizes-1]))
	appres.library_icon_size = atoi(icon_sizes[num_icon_sizes-1]);

    prev_icon_size = appres.library_icon_size;

    library_objects_texts = (char **) malloc(N_LIB_OBJECT_MAX*sizeof(char *));
     for (i=0; i < N_LIB_OBJECT_MAX; i++)
       library_objects_texts[i]=NULL;

    lib_compounds = (F_libobject **) malloc(N_LIB_OBJECT_MAX*sizeof(F_libobject *));
    for (i=0;i<N_LIB_OBJECT_MAX;i++) {
      lib_compounds[i] = (F_libobject *) malloc(sizeof(F_libobject));
      lib_compounds[i]->compound=NULL;
    }

    /* get all the library directory names into library_rec[] */
    /* return value is number of directories at the top level only */
    num_library_names = MakeLibrary();

    XtTranslateCoords(tool, (Position) 200, (Position) 50, &xposn, &yposn);

    FirstArg(XtNx, xposn);
    NextArg(XtNy, yposn + 50);
    NextArg(XtNtitle, "Select an object or library");
    NextArg(XtNcolormap, tool_cm);
    library_popup = XtCreatePopupShell("library_menu",
				     transientShellWidgetClass,
				     tool, Args, ArgCount);

    library_form = XtCreateManagedWidget("library_form", formWidgetClass,
					library_popup, NULL, ZERO);
    XtAugmentTranslations(library_popup,
			 XtParseTranslationTable(library_translations));
	
    FirstArg(XtNlabel, "Load a Library");
    NextArg(XtNwidth, MAIN_WIDTH-4);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    title=XtCreateManagedWidget("library_title", labelWidgetClass,
			      library_form, Args, ArgCount);

    /* label for Library */
    FirstArg(XtNlabel, "Library:");
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNfromVert, title);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    library_label = XtCreateManagedWidget("library_label", labelWidgetClass,
				      library_form, Args, ArgCount);

    /* pulldown menu for Library */

    if (num_library_names == 0) {
	/* if no libraries, make menu button insensitive */
	FirstArg(XtNlabel, "No libraries");
	NextArg(XtNsensitive, False);
    } else {
	/* make long label to fill out panel */
	FirstArg(XtNlabel, "000000000000000000000000000000000000000000000000");
    }
    NextArg(XtNfromHoriz, library_label);
    NextArg(XtNfromVert, title);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    library_menu_button = XtCreateManagedWidget("library_menu_button",
					    menuButtonWidgetClass,
					    library_form, Args, ArgCount);
    /* make the menu and attach it to the button */
    (void) make_library_menu(library_menu_button, "menu",
    					library_rec, num_library_names);

    if (!actions_added) {
	XtAppAddActions(tool_app, library_actions, XtNumber(library_actions));
	actions_added = True;
    }		

    /* Status indicator label */
    FirstArg(XtNlabel, " Status:");
    NextArg(XtNfromVert, library_menu_button);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    status_label = XtCreateManagedWidget("status_label", labelWidgetClass,
				library_form, Args, ArgCount);

    /* text widget showing status */

    FirstArg(XtNwidth, MAIN_WIDTH-77);
    NextArg(XtNstring, "None loaded");
    NextArg(XtNeditType, XawtextRead);	/* read-only */
    NextArg(XtNdisplayCaret, FALSE);	/* don't want to see the ^ cursor */
    NextArg(XtNheight, 30);		/* make room for scrollbar if necessary */
    NextArg(XtNfromHoriz, status_label);
    NextArg(XtNhorizDistance, 5);
    NextArg(XtNfromVert, library_menu_button);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    library_status = XtCreateManagedWidget("library_status", asciiTextWidgetClass,
					     library_form, Args, ArgCount);

    /* button to stop loading library */

    FirstArg(XtNlabel,"  Stop  ");
    NextArg(XtNsensitive, False);	/* disabled to start */
    NextArg(XtNfromVert, status_label);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    stop = XtCreateManagedWidget("stop", commandWidgetClass,
				 library_form, Args, ArgCount);
    XtAddEventHandler(stop, ButtonReleaseMask, False,
		    (XtEventHandler) library_stop, (XtPointer) NULL);

    FirstArg(XtNlabel,"Selected object:");
    NextArg(XtNresize, False);
    NextArg(XtNfromVert, stop);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("cur_lib_object_label", labelWidgetClass,
			      library_form, Args, ArgCount);

    FirstArg(XtNlabel, " ");
    NextArg(XtNresize, False);
    NextArg(XtNwidth, MAIN_WIDTH-130);
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, stop);
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    put_cur_obj = XtCreateManagedWidget("cur_lib_object", labelWidgetClass,
			      library_form, Args, ArgCount);

    /* make a label and pulldown to choose list or icon view */
    FirstArg(XtNlabel, "View:");
    NextArg(XtNfromVert, put_cur_obj);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("view_label", labelWidgetClass,
			      library_form, Args, ArgCount);


    FirstArg(XtNlabel, appres.icon_view? "Icon View":"List View");
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, put_cur_obj);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    sel_view_button = XtCreateManagedWidget("view_menu_button",
					    menuButtonWidgetClass,
					    library_form, Args, ArgCount);

    /* make the view menu and attach it to the menu button */

    make_pulldown_menu(viewtypes, 2, -1, "",
				 sel_view_button, sel_view);

    /* label and pulldown to choose size of icons */

    FirstArg(XtNlabel, "Icon size:");
    NextArg(XtNfromHoriz, sel_view_button);
    NextArg(XtNhorizDistance, 10);
    NextArg(XtNfromVert, put_cur_obj);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    beside = XtCreateManagedWidget("icon_size_label", labelWidgetClass,
			      library_form, Args, ArgCount);

    sprintf(buf, "%4d", appres.library_icon_size);
    FirstArg(XtNlabel, buf);
    NextArg(XtNsensitive, appres.icon_view? True: False); /* sensitive only if icon view */
    NextArg(XtNfromHoriz, beside);
    NextArg(XtNfromVert, put_cur_obj);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    NextArg(XtNleftBitmap, menu_arrow);	/* use menu arrow for pull-down */
    icon_size_button = XtCreateManagedWidget("icon_size_menu_button",
					    menuButtonWidgetClass,
					    library_form, Args, ArgCount);

    /* make the view menu and attach it to the menu button */

    make_pulldown_menu(icon_sizes, num_icon_sizes, -1, "",
				 icon_size_button, change_icon_size);

    /* get height of font so we can make a form an integral number of lines tall */
    FirstArg(XtNfont, &temp_font);
    GetValues(put_cur_obj);
    char_ht = max_char_height(temp_font) + 2;
    char_wd = char_width(temp_font) + 2;

    /* make that form to hold either the list stuff or icon view stuff */

    /* because of the problem of sizing them if we manage/unmanage the
       two views, we have both managed at all times and just raise the
       window of the form we want (icon_form or list_form) */

    FirstArg(XtNfromVert,sel_view_button);
    NextArg(XtNheight, 17*char_ht+8);
    NextArg(XtNwidth, MAIN_WIDTH);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    object_form = XtCreateManagedWidget("object_form", formWidgetClass,
					  library_form, Args, ArgCount);

    /**** ICON VIEW STUFF ****/

    /* make a form to hold the viewport which holds the box */

    FirstArg(XtNheight, 17*char_ht+8);
    NextArg(XtNwidth, MAIN_WIDTH);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    icon_form = XtCreateManagedWidget("icon_form", formWidgetClass,
					  object_form, Args, ArgCount);

    /* make a viewport in which to put the box widget which holds the icons */

    FirstArg(XtNheight, 17*char_ht+8);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNallowHoriz, False);
    NextArg(XtNallowVert, True);
    icon_viewport = XtCreateManagedWidget("icon_vport", viewportWidgetClass,
					  icon_form, Args, ArgCount);
					
    /* make a box widget to hold the object icons */

    FirstArg(XtNorientation, XtorientVertical);
    NextArg(XtNhSpace, 2);
    NextArg(XtNvSpace, 2);
    NextArg(XtNwidth, MAIN_WIDTH-12);
    NextArg(XtNheight, 17*char_ht+4);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    icon_box = XtCreateManagedWidget("icon_box", boxWidgetClass,
					icon_viewport, Args, ArgCount);

    /**** LIST VIEW STUFF ****/

    /* make a form to hold the viewport and preview stuff */

    FirstArg(XtNheight, 17*char_ht+4);
    NextArg(XtNwidth, MAIN_WIDTH);
    NextArg(XtNdefaultDistance, 2);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    list_form = XtCreateManagedWidget("list_form", formWidgetClass,
					  object_form, Args, ArgCount);

    /* make a viewport to hold the list */

    FirstArg(XtNheight, 17*char_ht+4);
    NextArg(XtNwidth, LIB_FILE_WIDTH);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNforceBars, True);
    NextArg(XtNallowHoriz, False);
    NextArg(XtNallowVert, True);
    list_viewport = XtCreateManagedWidget("list_vport", viewportWidgetClass,
					list_form, Args, ArgCount);

    /* make a list widget in the viewport with entries going down
       the column then right */

    FirstArg(XtNverticalList,True);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNwidth, LIB_FILE_WIDTH);
    NextArg(XtNheight, 17*char_ht);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    object_list = XtCreateManagedWidget("object_list", figListWidgetClass,
					list_viewport, Args, ArgCount);
    /* install the empty list */
    XawListChange(object_list, library_objects_texts, 0, 0, True);

    XtAddCallback(object_list, XtNcallback, NewObjectSel, (XtPointer) NULL);

    XtAugmentTranslations(object_list,
			XtParseTranslationTable(object_list_translations));

    /* make a label for the preview canvas */

    FirstArg(XtNlabel, "Object preview");
    NextArg(XtNfromHoriz, list_viewport);
    NextArg(XtNvertDistance, 40);
    NextArg(XtNwidth, LIB_PREVIEW_SIZE+2);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    lib_preview_label = XtCreateManagedWidget("library_preview_label", labelWidgetClass,
					  list_form, Args, ArgCount);

    /* and a label for the object name */

    FirstArg(XtNlabel," ");
    NextArg(XtNfromHoriz, list_viewport);
    NextArg(XtNfromVert,lib_preview_label);
    NextArg(XtNwidth, LIB_PREVIEW_SIZE+2);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    lib_obj_label = XtCreateManagedWidget("library_object_label", labelWidgetClass,
					  list_form, Args, ArgCount);

    /* first create a pixmap for its background, into which we'll draw the figure */

    preview_lib_pixmap = XCreatePixmap(tool_d, canvas_win,
			LIB_PREVIEW_SIZE, LIB_PREVIEW_SIZE, tool_dpth);

    /* create a second one as a copy in case the user cancels the popup */
    cur_obj_preview = XCreatePixmap(tool_d, canvas_win,
			LIB_PREVIEW_SIZE, LIB_PREVIEW_SIZE, tool_dpth);
    /* erase them */
    erase_pixmap(preview_lib_pixmap);
    erase_pixmap(cur_obj_preview);

    /* now make a preview canvas to the right of the object list */
    FirstArg(XtNfromHoriz, list_viewport);
    NextArg(XtNfromVert, lib_obj_label);
    NextArg(XtNlabel, "");
    NextArg(XtNborderWidth, 1);
    NextArg(XtNbackgroundPixmap, preview_lib_pixmap);
    NextArg(XtNwidth, LIB_PREVIEW_SIZE);
    NextArg(XtNheight, LIB_PREVIEW_SIZE);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainRight);
    NextArg(XtNright, XtChainRight);
    preview_lib_widget = XtCreateManagedWidget("library_preview_widget", labelWidgetClass,
					  list_form, Args, ArgCount);

    /**** END OF LIST VIEW STUFF ****/

    /* now a place for the library object comments */
    FirstArg(XtNlabel, "Object comments:");
    NextArg(XtNwidth, MAIN_WIDTH-6);
    NextArg(XtNfromVert, object_form);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    comment_label = XtCreateManagedWidget("comment_label", labelWidgetClass,
					  library_form, Args, ArgCount);
    FirstArg(XtNfromVert, comment_label);
    NextArg(XtNvertDistance, 1);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    NextArg(XtNwidth, MAIN_WIDTH-6);
    NextArg(XtNheight, 50);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNscrollVertical, XawtextScrollWhenNeeded);
    object_comments = XtCreateManagedWidget("object_comments", asciiTextWidgetClass,
					  library_form, Args, ArgCount);

    FirstArg(XtNlabel, "   Cancel    ");
    NextArg(XtNfromVert, object_comments);
    NextArg(XtNvertDistance, 10);
    NextArg(XtNheight, 25);
    NextArg(XtNresize, False);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				 library_form, Args, ArgCount);
    XtAddEventHandler(cancel, ButtonReleaseMask, False,
		    (XtEventHandler) library_cancel, (XtPointer) NULL);

    FirstArg(XtNlabel, "Select object");
    NextArg(XtNsensitive, False);			/* no library loaded yet */
    NextArg(XtNfromVert, object_comments);
    NextArg(XtNvertDistance, 10);
    NextArg(XtNfromHoriz, cancel);
    NextArg(XtNhorizDistance, 15);
    NextArg(XtNheight, 25);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    selobj = XtCreateManagedWidget("select", commandWidgetClass,
				 library_form, Args, ArgCount);
    XtAddEventHandler(selobj, ButtonReleaseMask, False,
		    (XtEventHandler) put_object_sel, (XtPointer) NULL);
    XtInstallAccelerators(library_form, cancel);
}

#ifndef XAW3D1_5E
#endif /* XAW3D1_5E */

static Widget
make_library_menu(Widget parent, char *name, struct lib_rec **librec, int num)
{
    Widget	     menu, entry;
#ifndef XAW3D1_5E
    Widget	     submenu;
    char	     submenu_name[200];
#endif /* XAW3D1_5E */
    char	     menu_name[200];
    int		     i;

    menu = XtCreatePopupShell(name, simpleMenuWidgetClass, 
				parent, NULL, ZERO);
#ifndef XAW3D1_5E
    /* if this is the toplevel menu, add a callback to popdown any submenus if
       the user releases the pointer button outside a submenu */
    if (XtIsSubclass(parent, menuButtonWidgetClass)) {
	/* also keep handle for load_library() to make sure it's popped down */
	library_menu = menu;
	XtAddCallback(menu, XtNpopdownCallback, (XtCallbackProc) popdown_subs, (XtPointer) NULL);
    }
#endif /* XAW3D1_5E */
    
    for (i = 0; i < num; i++) {
#ifndef XAW3D1_5E
	if (librec[i]->figs_at_top && librec[i]->nsubs) {
	    sprintf(menu_name, "%s ", librec[i]->name);
	} else {
	    sprintf(menu_name, " %s ", librec[i]->name);	/* to align with ones with diamonds */
	}
#else
	sprintf(menu_name, " %s", librec[i]->name);
	sprintf(submenu_name, "%sMenu", librec[i]->name);
#endif /* XAW3D1_5E */
	if (librec[i]->nsubs) {
#ifndef XAW3D1_5E
	    /* Make the submenu */
	    submenu = make_library_menu(menu, menu_name, librec[i]->subdirs,
  					librec[i]->nsubs);
	    /* and inform Xaw about it */
  	    FirstArg(XtNsubMenu, submenu);
	    NextArg(XtNrightMargin, 9);
#else
	    FirstArg(XtNmenuName, XtNewString(submenu_name));
#endif /* XAW3D1_5E */
	    NextArg(XtNrightBitmap, menu_cascade_arrow);
	    if (librec[i]->figs_at_top) {
#ifndef XAW3D1_5E
		NextArg(XtNleftMargin, 10);		/* room to the left of the diamond */
		NextArg(XtNselectCascade, True);
#endif /* XAW3D1_5E */
		NextArg(XtNleftBitmap, diamond_pixmap);
	    }
#ifndef XAW3D1_5E
	    entry = XtCreateManagedWidget(menu_name, smeCascadeObjectClass,
					  menu, Args, ArgCount);
#else
	    entry = XtCreateManagedWidget(menu_name, smeBSBObjectClass,
					  menu, Args, ArgCount);
#endif /* XAW3D1_5E */
	    if (librec[i]->figs_at_top)
		XtAddCallback(entry, XtNcallback, load_library,
			      (XtPointer) librec[i]);
#ifdef XAW3D1_5E
	    (void) make_library_menu(menu, submenu_name, librec[i]->subdirs,
				     librec[i]->nsubs);
#endif /* XAW3D1_5E */
	} else {
#ifndef XAW3D1_5E
	    entry = XtCreateManagedWidget(menu_name, smeCascadeObjectClass,
					  menu, NULL, ZERO);
#else
	    entry = XtCreateManagedWidget(menu_name, smeBSBObjectClass,
					  menu, NULL, ZERO);
#endif /* XAW3D1_5E */
	    XtAddCallback(entry, XtNcallback, load_library,
			  (XtPointer) librec[i]);
	  }
    }
    return menu;
}

/* user has chosen view from pulldown menu */

static void
sel_view(Widget w, XtPointer new_view, XtPointer garbage)
{
    if (appres.icon_view == (int) new_view)
	return;

    appres.icon_view = (int) new_view;

    /* change the label in the menu button */
    FirstArg(XtNlabel, viewtypes[(int) appres.icon_view]);
    SetValues(sel_view_button);

    if (appres.icon_view) {
	/* switch to icon view */

	/* make icon size button sensitive */
	XtSetSensitive(icon_size_button, True);
	/* raise the icon stuff */
	XRaiseWindow(tool_d, XtWindow(icon_form));

	/* if the icons haven't been made for this library yet, do it now */
	if (!icons_made && cur_library_name != NULL) {
	    int save_obj;
	    /* save current object number because load_library clears it */
	    save_obj = cur_library_object;
	    load_library(w, (XtPointer) cur_library, garbage);
	    cur_library_object = save_obj;
	    /* highlight the selected icon */
	    sel_icon(cur_library_object);
	}
    } else {
	/* switch to list view */

	/* make icon size button sensitive */
	XtSetSensitive(icon_size_button, False);

	/* make sure preview is current */
	if (cur_library_object != -1)
	    (void) preview_libobj(cur_library_object, preview_lib_pixmap,
			LIB_PREVIEW_SIZE, 20);

	/* raise the list stuff */
	XRaiseWindow(tool_d, XtWindow(list_form));
    }
}

/* user has changed the icon size */

static void
change_icon_size(Widget w, XtPointer menu_entry, XtPointer garbage)
{
    char	   *new_size = (char*) icon_sizes[(int) menu_entry];
    int		    i;

    /* update the menu label with the new size */
    FirstArg(XtNlabel, new_size);
    SetValues(icon_size_button);
    /* save the new size */
    appres.library_icon_size = atoi(new_size);
    /* remake pixmaps */
    if (cur_library_name != NULL)
	(void) MakeObjectLibrary(cur_library_path,
		  library_objects_texts, lib_compounds);
    /* reinstall the wheel scrolling for the scrollbar in the icon box widget */
    InstallScroll(icon_box);
    /* and reinstall scr lling accelerator on them */
    for (i=0; i<num_list_items; i++)
	InstallScrollParent(lib_buttons[i]);
}

/* put the name of obj into the preview label */
/* if obj = -1 the name will be erased */

static void
set_preview_name(int obj)
{
    FirstArg(XtNlabel,(obj<0)? "": library_objects_texts[obj]);
    NextArg(XtNwidth, LIB_PREVIEW_SIZE+2);
    SetValues(lib_obj_label);
}

static void
set_cur_obj_name(int obj)
{
    FirstArg(XtNlabel,(obj<0)? "": library_objects_texts[obj]);
    SetValues(put_cur_obj);
}

static void
erase_pixmap(Pixmap pixmap)
{
    XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0,
			LIB_PREVIEW_SIZE, LIB_PREVIEW_SIZE);
}

static Boolean
PutLibraryEntry(struct lib_rec *librec, char *path, char *lname, char *name)
{
    int i;

    /* strip any trailing whitespace */
    for (i=strlen(name)-1; i>0; i--) {
	if ((name[i] != ' ') && (name[i] != '\t'))
	    break;
    }
    if ((i>0) && (i<strlen(name)-1))
	name[i+1] = '\0';

    librec->longname = (char *) new_string(strlen(lname));
    strcpy(librec->longname, lname);

    librec->name = (char *) new_string(strlen(name));
    strcpy(librec->name, name);

    librec->path = (char *) new_string(strlen(path));
    strcpy(librec->path, path);

    librec->nsubs = 0;
    librec->subdirs[0] = (struct lib_rec *) NULL;

    /* all OK */
    return True;
}

/* scan a library directory and its sub-directories */
/* Inputs:
	librec	- pointer to lib_rec record to put entry
	path	- absolute path of library entry
	longname - name with parents prepended (e.g. Electrical / Physical)
	name	- name of library
   Outputs:
	figs_at_top - whether or not there are Fig files in toplevel
	nentries - number of directories found (including subdirectories)
*/

static Boolean
ScanLibraryDirectory(Boolean at_top, struct lib_rec **librec, char *path, char *longname, char *name, Boolean *figs_at_top, int *nentries)
{
    DIR		*dirp;
    DIRSTRUCT	*dp;
    struct lib_rec *lp;
    struct stat	 st;
    char	 path2[PATH_MAX], lname[N_LIB_NAME_MAX];
    int		 recnum, i;

    dirp = opendir(path);
    if (dirp == NULL) {
	file_msg("Can't open directory: %s", path);
	*nentries = 0;
	return False;
    }

    recnum = 0;

    /* read the directory to see if there are any .fig files at this level */
    *figs_at_top = False;
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	if (strstr(dp->d_name,".fig") != NULL && !IsDirectory(path, dp->d_name)) {
	    /* is a file with .fig in the name */
	    *figs_at_top = True;
	    /* if we're at the toplevel scan, put this directory first */
	    if (at_top) {
		/* toplevel has .fig files, make a library_rec entry for it */
		librec[0] = (struct lib_rec *) malloc(sizeof(struct lib_rec));
		if (librec[0] == 0)
		    return False;
		PutLibraryEntry(librec[0], path, "(Toplevel Directory)", "(Toplevel Directory)");
		recnum++;
	    }
	    break;
	}
    }

    /* now read the directory for the subdirectories */
    rewinddir(dirp);
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

	if (strlen(path) + strlen(dp->d_name) + 2 > PATH_MAX-1) {
            file_msg("Library path too long: %s/%s", path, dp->d_name);
	    continue;
	}
	sprintf(path2, "%s/%s", path, dp->d_name);
	/* check if directory or file */
	if (stat(path2, &st) == 0) {
	    if (S_ISDIR(st.st_mode)) {
		/* directory, scan any sub-directories recursively */
		if (dp->d_name[0] != '.') {
		    if (strlen(longname) == 0) {
			/* check length of name */
			if (strlen(dp->d_name) > N_LIB_NAME_MAX)
			    continue;
			strcpy(lname, dp->d_name);
		    } else {
			/* check length of resulting name */
			if (strlen(longname)+strlen(dp->d_name)+3 > N_LIB_NAME_MAX)
			    continue;
			sprintf(lname, "%s / %s",longname, dp->d_name);
		    }
		    /* allocate an entry for this subdir name */
		    librec[recnum] = (struct lib_rec *) malloc(sizeof(struct lib_rec));
		    if (librec[recnum] == 0) {
			/* close the directory */
			closedir(dirp);
			/* free allocations */
			for (i=0; i<recnum-1; i++)
			    free(librec[i]);
			return False;
		    }
		    lp = librec[recnum];
		    PutLibraryEntry(lp, path2, lname, dp->d_name);
		    recnum++;
		    /* and recurse */
		    if (!ScanLibraryDirectory(False, lp->subdirs, path2, lname, dp->d_name,
				&lp->figs_at_top, &lp->nsubs)) {
			/* close the directory */
			closedir(dirp);
			/* free allocations */
			for (i=0; i<recnum; i++)
			    free(librec[i]);
			return False;
		    }
		    /* if there are no .fig files in this directory and there
		       are no subdirs, remove this entry */
		    if (!lp->figs_at_top && lp->nsubs == 0) {
			recnum--;
			/* free the space */
			free(librec[recnum]);
		    }
		}
	    } 
	}
    }
    closedir(dirp);
    if (recnum > 0) {
	/* sort them since the order of files in directories is not necessarily alphabetical */
	qsort(&librec[0], recnum, sizeof(struct lib_rec *), (int (*)())*LRComp);
    }
    /* all OK */
    *nentries = recnum;
    return True;
}

/*
 * Read the Fig library directory/subdirectory names into library_rec[] structure.
 * Return the number of directories at the top level.
 */

static int
MakeLibrary(void)
{
    FILE	*file;
    struct stat	 st;
    char	*path;
    char	 *c, s[N_LIB_LINE_MAX], name[N_LIB_NAME_MAX],path2[N_LIB_NAME_MAX];
    int		 num, numlibs;
    Boolean	 dum;

    path = appres.library_dir;

    if (stat(path, &st) != 0) {       /* no such file */
	file_msg("Can't find %s, no libraries available", path);
	return 0;
    } else if (S_ISDIR(st.st_mode)) {
	/* if it is directory, scan the sub-directories and search libraries */
	(void) ScanLibraryDirectory(True, &library_rec, path, "", "", &dum, &numlibs);
	return numlibs;
    } else {
	/* if it is a file, it must contain list of libraries */
	if ((file = fopen(path, "r")) == NULL) {
            file_msg("Can't open %s, no libraries available", path);
            return 0;
	}
	numlibs = 0;
	while (fgets(s, N_LIB_LINE_MAX, file) != NULL) {
	    if (s[0] != '#') {
		if (sscanf(s, "%s %[^\n]", path2, name) == 1) {
		    if (strrchr(path2, '/') != NULL)
			strcpy(name, strrchr(path2, '/') + 1);
		    else {
			/* use the last dir in the path for the name */
			if (c=strrchr(path2,'/'))
			    strcpy(name,c);
			else
			    strcpy(name, path2);
		    }
		}
		/* allocate an entry for the Library name */
		library_rec[numlibs] = (struct lib_rec *) malloc(sizeof(struct lib_rec));
		if (library_rec[numlibs] == 0) {
		    fclose(file);
		    return numlibs;
		}
		PutLibraryEntry(library_rec[numlibs], path2, name, name);
		/* and attach its subdirectories */
		(void) ScanLibraryDirectory(True, library_rec[numlibs]->subdirs, path2, name, "",
					&library_rec[numlibs]->figs_at_top, &num);
		library_rec[numlibs]->nsubs = num;
		numlibs++;
	    }
        }
	fclose(file);
	return numlibs;
    }
}

/*
 * Given a library path and object names, populate the library panel with
 * icons of the library objects.
 */

static Boolean
MakeObjectLibrary(char *library_dir, char **objects_names, F_libobject **libobjects)
{
    int		itm, j;
    int		num_old_items;
    Boolean	flag, status;

    flag = True;
    /* we don't yet have the new icons */
    icons_made = False;

    itm = 0;
    num_old_items = num_list_items;
    if (appres.icon_view) {
	/* unmanage the box so we don't see each button dissappear */
	XtUnmanageChild(icon_box);
	process_pending();
	/* unmanage old library buttons */
	for (itm=0; itm<num_old_items; itm++) {
	    if (lib_buttons[itm]) {
		XtUnmanageChild(lib_buttons[itm]);
	    }
	    /* unhighlight any that might be highlighted */
	    unsel_icon(itm);
	}
	/* make sure they're unmanaged before we delete any pixmaps */
	process_pending();
        if (appres.library_icon_size != prev_icon_size) {
	    /* destroy any old buttons because the icon size has changed */
	    for (j=0; j<num_old_items; j++) {
		if (lib_buttons[j] != 0) {
		    XtDestroyWidget(lib_buttons[j]);
		    XFreePixmap(tool_d, lib_icons[j]);
		    lib_buttons[j] = (Widget) 0;
		    lib_icons[j] = (Pixmap) 0;
		}
	    }
	}
	/* update icon size */
	prev_icon_size = appres.library_icon_size;
	XtManageChild(icon_box);
    }

    /* now get the list of files in the selected library */

    if (MakeLibraryFileList(library_dir,objects_names)==True) {
        /* save current library name */
        cur_library_path = library_dir;
        cur_objects_names = objects_names;
	if (appres.icon_view) {
	    /* disable library and icon size menu buttons so user can't
	       change them while we're building pixmaps */
	    FirstArg(XtNsensitive, False);
	    SetValues(library_menu_button);
	    SetValues(icon_size_button);
	}
        itm = 0;
        while ((objects_names[itm]!=NULL) && (flag==True)) {
	    /* free any previous compound objects */
	    if (libobjects[itm]->compound != NULL) {
		free_compound(&libobjects[itm]->compound);
	    }
	    libobjects[itm]->compound = (F_compound *) 0;
	    /* make a new pixmap if one doesn't exist */
	    if (!lib_icons[itm])
		lib_icons[itm] = XCreatePixmap(tool_d, canvas_win,
					appres.library_icon_size, appres.library_icon_size, 
					tool_dpth);
	    /* preview the object into this pixmap */
	    status = preview_libobj(itm, lib_icons[itm], appres.library_icon_size, 4);
	    /* finally, make the "button" */
	    if (!lib_buttons[itm]) {
		FirstArg(XtNborderWidth, 1);
		NextArg(XtNborderColor, unsel_color); /* border color same as box bg */
		NextArg(XtNbitmap, status? lib_icons[itm]: (Pixmap) 0);
		NextArg(XtNinternalHeight, 0);
		NextArg(XtNinternalWidth, 0);
		lib_buttons[itm]=XtCreateManagedWidget(objects_names[itm], labelWidgetClass,
					      icon_box, Args, ArgCount);
		/* translations for user to click on label as a button */
		/* but only if the object was read successfully */
		if (status)
		    XtAugmentTranslations(lib_buttons[itm],
				XtParseTranslationTable(object_icon_translations));
	    } else {
		/* button exists from previous load, set the pixmap and manage it */
		FirstArg(XtNbitmap, status? lib_icons[itm]: (Pixmap) 0);
		SetValues(lib_buttons[itm]);
		if (status)
		    XtAugmentTranslations(lib_buttons[itm],
				XtParseTranslationTable(object_icon_translations));
		XtManageChild(lib_buttons[itm]);
	    }
	    /* let the user see it right away */
	    process_pending();
	    itm++;
	    /* if user has requested to stop loading */
	    if (library_stop_request) {
		/* make stop button insensitive */
	        XtSetSensitive(stop, False);
		break;
	    }
        }
	if (appres.icon_view) {
	    /* now we have the icons (used in sel_view) */
	    icons_made = True;
	}
	/* destroy any old buttons not being used and their pixmaps */
	for (j=itm; j<num_old_items; j++) {
	    if (lib_buttons[j] == 0)
		break;
	    XtDestroyWidget(lib_buttons[j]);
	    XFreePixmap(tool_d, lib_icons[j]);
	    lib_buttons[j] = (Widget) 0;
	    lib_icons[j] = (Pixmap) 0;
	}
	/* re-enable menu buttons (we don't check for appres.icon_view because
	   the user may have switched to list view while we were building the pixmaps */
	FirstArg(XtNsensitive, True);
	SetValues(library_menu_button);
	SetValues(icon_size_button);

	if (library_stop_request)
            libraryStatus("aborted - %d objects loaded out of %d in library",itm,num_list_items);
	else
            libraryStatus("%d library objects in library",num_list_items);
    } else {
	flag = False;
    }
    if (itm==0)
	flag = False;

    return flag;
}

/* get the list of files in the library dir_name and put in obj_list[] */

static Boolean
MakeLibraryFileList(char *dir_name, char **obj_list)
{
  DIR		  *dirp;
  DIRSTRUCT	  *dp;
  char		  *c;
  int		   i,numobj;

  dirp = opendir(dir_name);
  if (dirp == NULL) {
    libraryStatus("Can't open %s",dir_name);
    return False;	
  }

  for (i=0;i<N_LIB_OBJECT_MAX;i++)
    if (library_objects_texts[i]==NULL)
      library_objects_texts[i] = malloc(N_LIB_NAME_MAX*sizeof(char));
  library_objects_texts[N_LIB_OBJECT_MAX-1]=NULL;
  numobj=0;
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    /* if user has requested to stop loading */
    if (library_stop_request) {
	/* make stop button insensitive */
	XtSetSensitive(stop, False);
	break;
    }
    if ((numobj+1<N_LIB_OBJECT_MAX) && (dp->d_name[0] != '.') &&
	((c=strstr(dp->d_name,".fig")) != NULL) &&
	(strstr(dp->d_name,".fig.bak") == NULL)) {
	    if (!IsDirectory(dir_name, dp->d_name)) {
		*c='\0';
		strcpy(obj_list[numobj],dp->d_name);
		numobj++;
	    }
    }
  }
  free(obj_list[numobj]);
  obj_list[numobj]=NULL;
  /* save global number of list items for up/down arrow callbacks */
  num_list_items = numobj;
  /* signals up/down arrows to start at 0 if user doesn't press mouse in list first */
  which_num = -1;
  qsort(obj_list,numobj,sizeof(char*),(int (*)())*SPComp);
  closedir(dirp);
  return True;
}

/* update the status indicator with the string */

static char statstr[100];

static void
libraryStatus(char *format,...)
{
    va_list ap;

    va_start(ap, format);
    vsprintf(statstr, format, ap );
    va_end(ap);
    FirstArg(XtNstring, statstr);
    SetValues(library_status);
    app_flush();
}

static Boolean
preview_libobj(int objnum, Pixmap pixmap, int pixsize, int margin)
{
    int		 xmin, xmax, ymin, ymax;
    float	 width, height, size;
    int		 save_zoomxoff, save_zoomyoff;
    float	 save_zoomscale;
    Boolean	save_shownums;
    F_compound	*compound;
    Boolean	 status;

    /* if already previewing file, return */
    if (preview_in_progress == True)
	return False;

    /* say we are in progress */
    preview_in_progress = True;

    /* first, save current zoom settings */
    save_zoomscale	= display_zoomscale;
    save_zoomxoff	= zoomxoff;
    save_zoomyoff	= zoomyoff;

    /* save and turn off showing vertex numbers */
    save_shownums	= appres.shownums;
    appres.shownums	= False;

    /* save active layer array and set all to True */
    save_active_layers();
    reset_layers();
    save_depths();
    save_counts_and_clear();

    /* now switch the drawing canvas to our pixmap */
    canvas_win = (Window) pixmap;

    /* make wait cursor */
    XDefineCursor(tool_d, XtWindow(library_form), wait_cursor);
    app_flush();

    if ((status=load_lib_obj(objnum)) == True) {
	compound = lib_compounds[objnum]->compound;
	add_compound_depth(compound);	/* count objects at each depth */
	/* put any comments in the comment window */
	set_comments(compound->comments);

	xmin = compound->nwcorner.x;
	ymin = compound->nwcorner.y;
	xmax = compound->secorner.x;
	ymax = compound->secorner.y;
	width = xmax - xmin;
	height = ymax - ymin;
	size = max2(width,height)/ZOOM_FACTOR;

	/* scale to fit the preview canvas */
	display_zoomscale = (float) (pixsize-margin) / size;
	/* lets not get too magnified */
	if (display_zoomscale > 2.0)
	    display_zoomscale = 2.0;
	zoomscale = display_zoomscale/ZOOM_FACTOR;
	/* center the figure in the canvas */
	zoomxoff = -(pixsize/zoomscale - width)/2.0 + xmin;
	zoomyoff = -(pixsize/zoomscale - height)/2.0 + ymin;

	/* clear pixmap */
	XFillRectangle(tool_d, pixmap, gccache[ERASE], 0, 0,
				pixsize, pixsize);

	/* draw the object into the pixmap */
	redisplay_objects(compound);

	/* fool the toolkit into drawing the new pixmap */
	update_preview();
    }

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

    /* reset cursor */
    XUndefineCursor(tool_d, XtWindow(library_form));
    app_flush();

    /* say we are finished */
    preview_in_progress = False;

    /* if user requested a canvas redraw while preview was being generated
       do that now for full canvas */
    if (request_redraw) {
	redisplay_region(0, 0, CANVAS_WD, CANVAS_HT);
	request_redraw = False;
    }
    if (put_select_requested) {
	put_select_requested = False;
	cancel_preview = False;
	put_selected();
    }
    return status;
}

/* fool the toolkit into drawing the new pixmap */

static void
update_preview(void)
{
    FirstArg(XtNbackgroundPixmap, (Pixmap)0);
    SetValues(preview_lib_widget);
    FirstArg(XtNbackgroundPixmap, preview_lib_pixmap);
    SetValues(preview_lib_widget);
}

void set_comments(char *comments)
{
	FirstArg(XtNstring, comments);
	SetValues(object_comments);
}

/* come here when user clicks on an object icon in *icon view* */
/* this is also used to keep track of the arrow key movements */

static	char	   *which_name;
static	int	    old_item = -1;

static void
sel_item_icon(Widget w, XButtonEvent *ev)
{ 	
    int		    i;
    F_compound	   *compound;

    /* get structure having current entry */
    which_name = XtName(w);
    which_num = -1;
    /* we have picked an object */
    lib_just_loaded = False;
    for (i=0; i<num_list_items; i++) {
	if (strcmp(which_name, XtName(lib_buttons[i]))==0) {
	    /* uncolor the border of the *previously selected* icon */
	    unsel_icon(cur_library_object);
	    cur_library_object = which_num = old_item = i;
	    /* color the border of the icon black to show it is selected */
	    sel_icon(cur_library_object);
	    /* make the "Select object" button active now */
	    FirstArg(XtNsensitive, True);
	    SetValues(selobj);
	    /* get the object compound */
	    compound = lib_compounds[which_num]->compound;
	    /* put its comments in the comment window */
	    set_comments(compound->comments);
	    /* copy the icon to the preview in case user switches to list view */
	    copy_icon_to_preview(which_num);
	    /* and change label in preview label */
	    FirstArg(XtNlabel,library_objects_texts[which_num]);
	    NextArg(XtNwidth, LIB_PREVIEW_SIZE+2);
	    SetValues(lib_obj_label);
	    /* finally, highlight the new one in the list */
	    XawListHighlight(object_list, which_num);
	    break;
	}
    }
}

static void
unsel_icon(int object)
{
    if (object != -1 && lib_buttons[object]) {
	FirstArg(XtNborderColor, unsel_color);
	SetValues(lib_buttons[object]);
    }
}

static void
sel_icon(int object)
{
    /* color the border of the icon black to show it is selected */
    if (object != -1 && lib_buttons[object]) {
	FirstArg(XtNborderColor, sel_color);
	SetValues(lib_buttons[object]);
	/* set the name in the name field */
	set_cur_obj_name(object);
    }
}

/* copy pixmap from lib_icons[object] into the preview in case
   the user switches to list view */

static void
copy_icon_to_preview(int object)
{
    XCopyArea(tool_d, lib_icons[object], preview_lib_pixmap,
		gccache[PAINT], 0, 0,
		LIB_PREVIEW_SIZE, LIB_PREVIEW_SIZE, 0, 0);
    update_preview();
}
