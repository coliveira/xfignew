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

/*
    w_menuentry.h - Public Header file for subclassed BSB Menu Entry object 
    
    This adds the underline resource to underline one character of the label
*/

#ifndef _FigSmeBSB_h
#define _FigSmeBSB_h

#include <X11/Xmu/Converters.h>

#ifdef XAW3D
#include <X11/Xaw3d/Sme.h>
#else
#include <X11/Xaw/Sme.h>
#endif

#include "SmeBSB.h"

/****************************************************************
 *
 * FigSmeBSB object
 *
 ****************************************************************/

/* FigBSB Menu Entry Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 underline	     Index		int		-1

*/

typedef struct _FigSmeBSBClassRec    *FigSmeBSBObjectClass;
typedef struct _FigSmeBSBRec         *FigSmeBSBObject;

extern WidgetClass figSmeBSBObjectClass;

#define XtNunderline   "underline"

#endif /* Fig_SmeBSB_h */
