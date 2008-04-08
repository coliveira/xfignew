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
   w_listwidgetP.h -

   FigList - 
   This is an attempt to subclass the listWidgetClass to add
   the functionality of up/down arrows to scroll up/down in the list
*/

/* protect again multiple includes */

#ifndef _FigListP_h
#define _FigListP_h

/* get the superclass private header */
#ifdef XAW3D
#include <X11/Xaw3d/ListP.h>
#else /* XAW3D */
#include <X11/Xaw/ListP.h>
#endif /* XAW3D */

/* our header file */
#include "w_listwidget.h"

/* New fields we need for the class record */

typedef struct {
    int make_compiler_happy;	/* just so it's not empty */
} FigListClassPart;

/* Full class record declaration */

typedef struct _FigListClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    ListClassPart	list_class;
    FigListClassPart	figList_class;
} FigListClassRec;

extern FigListClassRec	figListClassRec;

/* New fields for the FigList widget record */

typedef struct {
    /* resources */

    /* (none) */

    /* private state */

    /* (none) */
    int make_compiler_happy;

} FigListPart;


/* Full instance record declaration */

typedef struct _FigListRec {
    CorePart	core;
    SimplePart	simple;
    ListPart	list;
    FigListPart	figlist;
} FigListRec;

#endif /* _FigListP_h */
