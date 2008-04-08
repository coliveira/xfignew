/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 2000-2002 by Brian V. Smith
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

/* 
   FigList - 

   This is an attempt to subclass the listWidgetClass to add
   the functionality of up/down arrows to scroll up/down in the list
*/

#include <stdio.h>
#include <ctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Drawing.h>
#ifdef XAW3D
#include <X11/Xaw3d/XawInit.h>
#else /* XAW3D */
#include <X11/Xaw/XawInit.h>
#endif /* XAW3D */

#include "w_listwidgetP.h"

/* new translations for scrolling up/down in the list */

static void SelectHome(Widget w, XEvent *event, String *params, Cardinal *num_params), SelectEnd(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void SelectNext(Widget w, XEvent *event, String *params, Cardinal *num_params), SelectPrev(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void SelectLeft(Widget w, XEvent *event, String *params, Cardinal *num_params), SelectRight(Widget w, XEvent *event, String *params, Cardinal *num_params);

static char defaultTranslations[] =
	"<Btn1Down>:	Set()\n\
	<Btn1Up>:	Notify()\n\
	<Key>Home:	SelectHome()\n\
	<Key>End:	SelectEnd()\n\
	<Key>Down:	SelectNext()\n\
	<Key>Up:	SelectPrev()\n\
	<Key>Left:	SelectLeft()\n\
	<Key>Right:	SelectRight()";

static XtActionsRec	actions[] = {
	{"SelectHome",	SelectHome},
	{"SelectEnd",	SelectEnd},
	{"SelectNext",	SelectNext},
	{"SelectPrev",	SelectPrev},
	{"SelectLeft",	SelectLeft},
	{"SelectRight",	SelectRight},
};

/* Private Data */

#define superclass		(&listClassRec)
FigListClassRec figListClassRec = {
	{ /* core class part */
    /* superclass	  	*/  (WidgetClass) superclass,
    /* class_name	  	*/  "FigList",
    /* widget_size	  	*/  sizeof(FigListRec),
    /* class_initialize   	*/  XawInitializeWidgetSet,
    /* class_part_initialize	*/  NULL,
    /* class_inited       	*/  FALSE,
    /* initialize	  	*/  NULL,
    /* initialize_hook		*/  NULL,
    /* realize		  	*/  XtInheritRealize,
    /* actions		  	*/  actions,
    /* num_actions	  	*/  XtNumber(actions),
    /* resources	  	*/  NULL,
    /* num_resources	  	*/  0,
    /* xrm_class	  	*/  NULLQUARK,
    /* compress_motion	  	*/  TRUE,
    /* compress_exposure  	*/  FALSE,
    /* compress_enterleave	*/  TRUE,
    /* visible_interest	  	*/  FALSE,
    /* destroy		  	*/  NULL,
    /* resize		  	*/  XtInheritResize,
    /* expose		  	*/  XtInheritExpose,
    /* set_values	  	*/  NULL,
    /* set_values_hook		*/  NULL,
    /* set_values_almost	*/  XtInheritSetValuesAlmost,
    /* get_values_hook		*/  NULL,
    /* accept_focus	 	*/  NULL,
    /* intrinsics version	*/  XtVersion,
    /* callback_private   	*/  NULL,
    /* tm_table		   	*/  defaultTranslations,
    /* query_geometry		*/  XtInheritQueryGeometry,
  },
/* Simple class fields initialization */
	{
	/* change_sensitive	*/  XtInheritChangeSensitive
	},

/* FigList class part */
	{
	0,		/* dummy field */
	}
};

/* Declaration of methods */

/* (none) */

WidgetClass figListWidgetClass = (WidgetClass) &figListClassRec;

/**********************/
/*                    */
/* Private Procedures */
/*                    */
/**********************/

/* highlight the first (home) entry */


void FigListFinishUp (Widget w, int item);

static void
SelectHome(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    /* highlight and call any callbacks */
    FigListFinishUp(w, 0);
}

/* highlight the last (end) entry */

static void
SelectEnd(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    
    FigListWidget  lw = (FigListWidget) w;

    /* highlight and call any callbacks */
    FigListFinishUp(w, lw->list.nitems-1);
}

/* highlight the next entry */

static void
SelectNext(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    
    FigListWidget  lw = (FigListWidget) w;
    int		   item;

    item = lw->list.is_highlighted;
    if (item == NO_HIGHLIGHT || item >= lw->list.nitems-1)
	return;

    /* increment item */
    if (lw->list.vertical_cols || lw->list.ncols == 1)
	item++;
    else
	item += lw->list.ncols;	/* horizontal org, jump col amount */

    /* now highlight and call any callbacks */
    FigListFinishUp(w, item);
}

/* highlight the previous entry */

static void
SelectPrev(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    FigListWidget	lw = (FigListWidget) w;
    int		   item;

    item = lw->list.is_highlighted;
    if (item == NO_HIGHLIGHT || item <= 0)
	return;

    /* decrement item */
    if (lw->list.vertical_cols || lw->list.ncols == 1)
	item--;
    else
	item -= lw->list.ncols;	/* horizontal org, jump col amount */

    /* now highlight and call any callbacks */
    FigListFinishUp(w, item);
}

/* move left (if there is more than 1 column) and highlight the entry */

static void
SelectLeft(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    FigListWidget	lw = (FigListWidget) w;
    int		   item;

    item = lw->list.is_highlighted;
    if (item == NO_HIGHLIGHT || item <= 0)
	return;

    /* decrement item */
    if (!lw->list.vertical_cols || lw->list.nrows == 1)
	item--;
    else
	item-= lw->list.nrows;	/* vertical org, jump row amount */

    /* now highlight and call any callbacks */
    FigListFinishUp(w, item);
}

/* move right (if there is more than 1 column) and highlight the entry */

static void
SelectRight(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    FigListWidget	lw = (FigListWidget) w;
    int		   item;

    item = lw->list.is_highlighted;
    if (item == NO_HIGHLIGHT || item >= lw->list.nitems)
	return;

    /* increment item */
    if (!lw->list.vertical_cols || lw->list.nrows == 1)
	item++;
    else
	item+= lw->list.nrows;	/* vertical org, jump row amount */

    /* now highlight and call any callbacks */
    FigListFinishUp(w, item);
}

/* call any callbacks and highlight the new item */

void FigListFinishUp(Widget w, int item)
{
    FigListWidget lw = (FigListWidget) w;
    int		  item_len;
    XawListReturnStruct ret_value;

    /* check result of inc/dec */
    if (item >= lw->list.nitems || item < 0)
	return;

    item_len = strlen(lw->list.list[item]);

    if ( lw->list.paste )	/* if XtNpasteBuffer is true then paste it. */
        XStoreBytes(XtDisplay(w), lw->list.list[item], item_len);

    /* call any callbacks */
    ret_value.string = lw->list.list[item];
    ret_value.list_index = item;
    
    XtCallCallbacks( w, XtNcallback, (XtPointer) &ret_value);

    /* highlight new item */
    XawListHighlight(w, item);
}

