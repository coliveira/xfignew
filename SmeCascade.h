/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2003 by Brian V. Smith
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

/* Public definitions for SmeCascade object */

#ifndef _SmeCascade_h
#define _SmeCascade_h

#include <X11/Xmu/Converters.h>

#ifdef XAW3D
#include <X11/Xaw3d/Sme.h>
#else
#include <X11/Xaw/Sme.h>
#endif
#include "SmeBSB.h"

/****************************************************************
 *
 * SmeCascade object
 *
 ****************************************************************/

/* Cascade Menu Entry Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 font                Font               XFontStruct *   XtDefaultFont
 foreground          Foreground         Pixel           XtDefaultForeground
 height		     Height		Dimension	0
 label               Label              String          Name of entry
 leftBitmap          LeftBitmap         Pixmap          None
 leftMargin          HorizontalMargins  Dimension       4
 rightBitmap         RightBitmap        Pixmap          None
 rightMargin         HorizontalMargins  Dimension       4
 sensitive	     Sensitive		Boolean		True
 vertSpace           VertSpace          int             25
 width		     Width		Dimension	0
 x		     Position		Position	0n
 y		     Position		Position	0
 subMenu	     SubMenu		Widget		NULL
 selectCascade	     SelectCascade	Boolean		False
*/

typedef struct _SmeCascadeClassRec    *SmeCascadeObjectClass;
typedef struct _SmeCascadeRec         *SmeCascadeObject;

extern WidgetClass smeCascadeObjectClass;

void popdown_subs(void);

#define	XtNsubMenu	 "subMenu"
#define	XtCSubMenu	 "SubMenu"
#define XtNselectCascade "selectCascade"
#define XtCSelectCascade "SelectCascade"

#endif /* _SmeBSB_h */
