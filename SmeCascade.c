/*
 * FIG : Facility for Interactive Generation of figures
 * Parts Copyright (c) 1989-2003 by Brian V. Smith
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
 *
 */

/* This subclasses the SmeBSB object to make a cascade menu */

#include <stdio.h>
#include "fig.h"
#include "figx.h"
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>

#include <X11/Xmu/Drawing.h>

#ifdef XAW3D
#include <X11/Xaw3d/XawInit.h>
#else
#include <X11/Xaw/XawInit.h>
#endif /* XAW3D */

#include "SmeCascadeP.h"

#include <X11/ShellP.h>

#define offset(field) XtOffset(SmeCascadeObject, sme_cascade.field)

static XtResource resources[] = {
  {XtNsubMenu,  XtCSubMenu, XtRWidget, sizeof(Widget),
     				offset(subMenu), XtRString, NULL},
  {XtNselectCascade,  XtCSelectCascade, XtRBoolean, sizeof(Boolean),
     				offset(selectCascade), XtRBoolean, (XtPointer) False},
  {"highlighted",  "Highlighted", XtRBoolean, sizeof(Boolean),
     				offset(highlighted), XtRBoolean, (XtPointer) False},
};   
#undef offset

/*
 * Semi Public function definitions. 
 */

static	void ClassInitialize(void);
static	void HighlightCascade(Widget w);
static	void UnhighlightCascade(Widget w);
static	void Notify(Widget w);

/* 
 * Private Function Definitions.
 */

void	popdown_subs(void);
static	void popdown(Widget w);

#define superclass (&smeBSBClassRec)
SmeCascadeClassRec smeCascadeClassRec = {
  {
    /* superclass         */    (WidgetClass) superclass,
    /* class_name         */    "SmeCascade",
    /* size               */    sizeof(SmeCascadeRec),
    /* class_initializer  */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed      */	FALSE,
    /* initialize         */    NULL,
    /* initialize_hook    */	NULL,
    /* realize            */    NULL,
    /* actions            */    NULL,
    /* num_actions        */    ZERO,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE, 
    /* compress_exposure  */    FALSE,
    /* compress_enterleave*/ 	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    NULL,
    /* expose             */    XtInheritExpose,
    /* set_values         */    NULL,
    /* set_values_hook    */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus       */    NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table		  */    NULL,
    /* query_geometry	  */    XtInheritQueryGeometry,
    /* display_accelerator*/    NULL,
    /* extension	  */    NULL
  },{
    /* Simple Menu Entry (Sme) Fields */
      
    /* highlight          */    HighlightCascade,
    /* unhighlight        */    UnhighlightCascade,
    /* notify             */    Notify,
    /* extension	  */    NULL
  }, {

#ifdef XAW3D
    /* ThreeDClass Fields */
    /* shadowdraw	  */    XtInheritXawSme3dShadowDraw
  }, {
#endif /* XAW3D */

    /* SmeBSB Menu entry Fields */  

    /* extension	  */    NULL
  }, {
    /* Cascade Menu entry Fields */  

    /* extension	  */    NULL
  }
};

WidgetClass smeCascadeObjectClass = (WidgetClass) &smeCascadeClassRec;

/* really, really private stuff */

static Widget	popped_up[50];	/* to keep track of which menus are still popped up */
static int	popped;		/* index into popped_up[] */


/************************************************************
 *
 * Public Functions.
 *
 ************************************************************/

/*	Function Name: popdown_subs
 *	Description: pops down any remaining submenus if user releases
 *		     pointer button outside of submenu
 *	Arguments: none.
 *	Returns: none.
 */

void
popdown_subs(void)
{
    int		     i;

    /* pop down all submenus */
    for (i=popped-1; i>=0; i--)
	popdown(popped_up[i]);
    popped = 0;
}

/************************************************************
 *
 * Semi-Public Functions.
 *
 ************************************************************/

/*	Function Name: ClassInitialize
 *	Description: Initializes the SmeBSBObject. 
 *	Arguments: none.
 *	Returns: none.
 */

static void 
ClassInitialize(void)
{
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRWidget, XmuCvtStringToWidget, NULL, 0 );
    popped = 0;
}


/*	Function Name: MoveMenu
 *	Description: Actually moves the menu, may force it to
 *                   to be fully visable if menu_on_screen is TRUE.
 *	Arguments: w - the simple menu widget.
 *                 x, y - the current location of the widget.
 *	Returns: none 
 *	Modified from SimpleMenu.c by Chris D. Peterson
 */

static void
MoveMenu(Widget w, Position x, Position y)
{
    Arg			arglist[2];
    Cardinal		num_args = 0;
    int			width = w->core.width + 2 * w->core.border_width;
    int			height = w->core.height + 2 * w->core.border_width;

    if (x < 0) 
	x = 0;
    else {
	int scr_width = WidthOfScreen(XtScreen(w));
	if (x + width > scr_width)
	    x = scr_width - width;
    }
	
    if (y < 0)
	y = 0;
    else {
	int scr_height = HeightOfScreen(XtScreen(w));
	if (y + height > scr_height)
	    y = scr_height - height;
    }
    
    XtSetArg(arglist[num_args], XtNx, x); num_args++;
    XtSetArg(arglist[num_args], XtNy, y); num_args++;
    XtSetValues(w, arglist, num_args);
}

/************************************************************
 * 
 * Highlight entry and pop up submenu of cascade entry.
 *
 ************************************************************/

static void
HighlightCascade(Widget w)
{
    SmeCascadeObject	entry = (SmeCascadeObject) w;
    int			destX, destY, i;
    Window		childRtrn;
    ShellWidget		shellparent;
    Widget		parent, submenu;

    parent = XtParent(w);
    shellparent = (ShellWidget) parent;
    submenu = entry->sme_cascade.subMenu;

    /* if there are any submenus popped up and they are not connected to us, pop them down */
    if (popped) {
	/* if this is a cascade */
	if (submenu) {
	    if (parent != popped_up[popped-1]) {
	      for (i=0; i<popped; i++)
		if (submenu == popped_up[i])
		    break;
	      if (i>=popped) {
		for (i=popped-1; i>=0; i--) {
		    if (popped_up[i] == parent)
			break;
		    popdown(popped_up[i]);
		}
		popped = i+1;
	      }
	    }
	} else {
	    /* an ordinary BSB */
	    if (parent != popped_up[popped-1]) {
		for (i=popped-1; i>=0; i--) {
		    if (popped_up[i] == parent)
			break;
		    popdown(popped_up[i]);
		}
		popped = i+1;
	    }
	}
    }

    /* if this is an ordinary BSB entry */
    if (submenu == NULL) {
	/* highlight the entry */
	if (!entry->sme_cascade.highlighted) {
	    (*superclass->sme_class.highlight) (w);
	    entry->sme_cascade.highlighted = True;
	}
    } else {
	/* else, a cascade entry */

	/* if the user has asked for this cascade to be selectable (Notify) */
	if (entry->sme_cascade.selectCascade && !entry->sme_cascade.highlighted) {
	    (*superclass->sme_class.highlight) (w);
	    entry->sme_cascade.highlighted = True;
	}
	if (shellparent->shell.popped_up) {
	    XtRealizeWidget(submenu);
	    XTranslateCoordinates(XtDisplayOfObject(w),
			    XtWindowOfObject(w),
			    RootWindowOfScreen(XtScreenOfObject(w)),
			    entry->rectangle.width - 12,
			    entry->rectangle.y+2,
			    &destX,&destY,&childRtrn);
            MoveMenu(submenu,destX,destY);
            XtPopup(submenu,XtGrabNonexclusive);
	    /* put this one on the stack if it isn't already there */
	    for (i=0; i<popped; i++) {
		if (popped_up[i] == submenu)
		    break;
	    }
	    if (i >= popped) {
		popped_up[popped] = submenu;
		popped++;
	    }
	} 
    }
}

/************************************************************
 * 
 * Check if the pointer is actually leaving the menu or is in a submenu.
 *
 ************************************************************/

static void
UnhighlightCascade(Widget w)
{
    Dimension	     height, width;
    Window	     root, child;
    int		     root_x, root_y, px, py;
    Position	     x, y;
    unsigned int     mask;
    SmeCascadeObject entry = (SmeCascadeObject) w;
    Widget	     submenu;
    
    submenu = entry->sme_cascade.subMenu;
    if (submenu == NULL) {
	if (entry->sme_cascade.highlighted) {
	    (*superclass->sme_class.unhighlight) (w);
	    entry->sme_cascade.highlighted = False;
	}
	return;
    }
	
    XtVaGetValues(w,
		XtNx, &x,
		XtNy, &y,
		XtNheight, &height,
		XtNwidth, &width,
		NULL);
    XQueryPointer(XtDisplayOfObject(w), 
			XtWindowOfObject(w),
			&root, &child, &root_x, &root_y, &px, &py, &mask);
    x = px-(int)x;
    y = py-(int)y;

    /* see if the user moved the pointer into the submenu */
    if ((x >= width-12) && (x < width) && (y >= 2) && (y < height)) {
	/* yes, if this is a selectable entry, unhighlight it */
	if (entry->sme_cascade.selectCascade) {
	    (*superclass->sme_class.unhighlight) (w);
	    entry->sme_cascade.highlighted = False;
	}
	return;		/* don't pop it down in either case */
    }

    /* if the user has asked for this cascade to be selectable (Notify) */
    if (entry->sme_cascade.selectCascade && entry->sme_cascade.highlighted) {
        (*superclass->sme_class.unhighlight) (w);
    }
    entry->sme_cascade.highlighted = False;
    if (submenu != NULL) {
	if (popped > 0)
	    popped--;
	popdown(submenu);
    }
}

static void
Notify(Widget w)
{
    Window	     root, child;
    int		     root_x, root_y, px, py;
    unsigned int     mask;

    XQueryPointer(XtDisplayOfObject(w), 
		  XtWindowOfObject(w),
		  &root, &child, &root_x, &root_y, &px, &py, &mask);

    /* call superclass notify method */

    if (px < w->core.width - 12) {
	(*superclass->sme_class.notify) (w);
	popdown_subs();
    }
}

/************************************************************
 *
 * Private Functions.
 *
 ************************************************************/

extern void	_XtDefaultWarningMsg();

static void
null_sme_proc(void)
{
}

static void
popdown(Widget w)
{
    /* nullify the warning handler because we may have an unmatched grab */
    XtAppSetWarningMsgHandler(XtWidgetToApplicationContext(w), null_sme_proc);
    XtPopdown(w);
    /* restore the warning handler */
    XtAppSetWarningMsgHandler(XtWidgetToApplicationContext(w), _XtDefaultWarningMsg);
}

