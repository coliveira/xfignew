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
    w_menuentry.c - subclassed BSB Menu Entry object 
    
    This adds the underline resource to underline one character of the label
*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <X11/Xmu/Drawing.h>

#ifdef XAW3D
#include <X11/Xaw3d/XawInit.h>
#else
#include <X11/Xaw/XawInit.h>
#endif /* XAW3D */

#include "figx.h"
#include "w_menuentryP.h"
#include "w_canvas.h"

#include <stdio.h>
#include <stdlib.h>  /* abs() */

#define offset(field) XtOffsetOf(FigSmeBSBRec, figSme_bsb.field)

static XtResource resources[] = {
  {XtNunderline,  XtCIndex, XtRInt, sizeof(int),
     offset(underline), XtRImmediate, (XtPointer) -1},
};   
#undef offset

/*
 * Semi Public function definitions. 
 */

static void Redisplay(Widget w, XEvent *event, Region region);
static Boolean SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);

#define superclass (&smeBSBClassRec)

FigSmeBSBClassRec figSmeBSBClassRec = {
  {
    /* superclass         */    (WidgetClass) superclass,
    /* class_name         */    "FigSmeBSB",
    /* size               */    sizeof(FigSmeBSBRec),
    /* class_initializer  */	XawInitializeWidgetSet,
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
    /* resize             */    XtInheritResize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
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
    /* SimpleMenuClass Fields */
    /* highlight          */	XtInheritHighlight,
    /* unhighlight        */	XtInheritUnhighlight,
    /* notify             */	XtInheritNotify,		
    /* extension	  */	NULL
  },
#ifdef XAW3D
  {
    /* ThreeDClass Fields */
    /* shadowdraw	  */	XtInheritXawSme3dShadowDraw
  },
#endif /* XAW3D */
  {
    /* BSBClass Fields */  
    /* extension	  */    NULL
  }, {
    /* FigBSBClass Fields */  
    /* dummy field	  */    0
  }
};

WidgetClass figSmeBSBObjectClass = (WidgetClass) &figSmeBSBClassRec;


/*      Function Name: Redisplay
 *      Description: Redisplays underlining (if any) in the menu entry.
 *      Arguments: w - the simple menu widget.
 *                 event - the X event that caused this redisplay.
 *                 region - the region that needs to be repainted. 
 *      Returns: none.
 */

/* ARGSUSED */
static void
Redisplay(Widget w, XEvent *event, Region region)
{
    GC gc;
    FigSmeBSBObject entry = (FigSmeBSBObject) w;
    int	font_ascent, font_descent, y_loc;

    int	fontset_ascent, fontset_descent;
#if (XtVersion >= 11006)
    XFontSetExtents *ext = XExtentsOfFontSet(entry->sme_bsb.fontset);
#endif /* XtVersion R6 */

    Dimension s;
#ifdef XAW3D
    s = entry->sme_threeD.shadow_width;
#else
    s = 0;	/* no shadow width for non-3d widget set */
#endif /* XAW3D */

    /* call the superclass expose method first (draw the label) */
    (*superclass->rect_class.expose) (w, event, region);

#if (XtVersion >= 11006)
    if ( entry->sme.international == True ) {
        fontset_ascent = abs(ext->max_ink_extent.y);
        fontset_descent = ext->max_ink_extent.height - fontset_ascent;
    } else { /*else, compute size from font like R5*/
        font_ascent = entry->sme_bsb.font->max_bounds.ascent;
        font_descent = entry->sme_bsb.font->max_bounds.descent;
    }
#else
    font_ascent = entry->sme_bsb.font->max_bounds.ascent;
    font_descent = entry->sme_bsb.font->max_bounds.descent;
#endif /* XtVersion R6 */

    y_loc = entry->rectangle.y;

    if (XtIsSensitive(w) && XtIsSensitive( XtParent(w) ) ) {
	if ( w == XawSimpleMenuGetActiveEntry(XtParent(w)) ) {
	    gc = entry->sme_bsb.rev_gc;
	} else {
	    gc = entry->sme_bsb.norm_gc;
	}
    } else {
	gc = entry->sme_bsb.norm_gray_gc;
    }
    
    if (entry->sme_bsb.label != NULL) {
	int x_loc = entry->sme_bsb.left_margin;
	int len = strlen(entry->sme_bsb.label);
	char * label = entry->sme_bsb.label;

	switch(entry->sme_bsb.justify) {
	    int width, t_width;

	case XtJustifyCenter:
#if (XtVersion >= 11006)
            if ( entry->sme.international == True ) {
	        t_width = XmbTextEscapement(entry->sme_bsb.fontset,label,len);
                width = entry->rectangle.width - (entry->sme_bsb.left_margin +
					      entry->sme_bsb.right_margin);
            }
            else {
	        t_width = XTextWidth(entry->sme_bsb.font, label, len);
	        width = entry->rectangle.width - (entry->sme_bsb.left_margin +
					      entry->sme_bsb.right_margin);
            }
#else
	    t_width = XTextWidth(entry->sme_bsb.font, label, len);
	    width = entry->rectangle.width - (entry->sme_bsb.left_margin +
					      entry->sme_bsb.right_margin);
#endif /* XtVersion R6 */
	    x_loc += (width - t_width)/2;
	    break;
	case XtJustifyRight:
#if (XtVersion >= 11006)
            if ( entry->sme.international == True ) {
                t_width = XmbTextEscapement(entry->sme_bsb.fontset,label,len);
                x_loc = entry->rectangle.width - ( entry->sme_bsb.right_margin
						 + t_width );
            }
            else {
	        t_width = XTextWidth(entry->sme_bsb.font, label, len);
	        x_loc = entry->rectangle.width - ( entry->sme_bsb.right_margin
						 + t_width );
            }
#else
	    t_width = XTextWidth(entry->sme_bsb.font, label, len);
	    x_loc = entry->rectangle.width - ( entry->sme_bsb.right_margin
						 + t_width );
#endif /* XtVersion R6 */
	    break;
	case XtJustifyLeft:
	default:
	    break;
	}


	/* this will center the text in the gadget top-to-bottom */

#if (XtVersion >= 11006)
        if ( entry->sme.international==True ) {
            y_loc += ((int)entry->rectangle.height - 
		  (fontset_ascent + fontset_descent)) / 2 + fontset_ascent;
        } else {
            y_loc += ((int)entry->rectangle.height - 
		  (font_ascent + font_descent)) / 2 + font_ascent;
        }
#else
        y_loc += ((int)entry->rectangle.height - 
		  (font_ascent + font_descent)) / 2 + font_ascent;
#endif /* XtVersion R6 */

	/* do the underlining here */
	if ( entry->figSme_bsb.underline >= 0 ) {
	    int ul, ul_x1_loc, ul_wid;
	    ul = entry->figSme_bsb.underline;
	    ul_x1_loc = x_loc + s;
	    if ( ul != 0 )
		ul_x1_loc += XTextWidth(entry->sme_bsb.font, label, ul);
	    ul_wid = XTextWidth(entry->sme_bsb.font, &label[ul], 1) - 2;
	    XDrawLine(XtDisplayOfObject(w), XtWindowOfObject(w), gc, 
				ul_x1_loc, y_loc+1, ul_x1_loc+ul_wid, y_loc+1);
	}
    }
}


/*      Function Name: SetValues
 *      Description: Relayout the menu when one of the resources is changed.
 *      Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *      Returns: none
 */

/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    FigSmeBSBObject entry = (FigSmeBSBObject) new;
    FigSmeBSBObject old_entry = (FigSmeBSBObject) current;
    Boolean ret_val = FALSE;

    if (entry->figSme_bsb.underline != old_entry->figSme_bsb.underline) {
	ret_val = TRUE;
    }

    return(ret_val);
}
