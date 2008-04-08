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
    w_menuentryP.h - Private Header file for subclassed BSB Menu Entry object 
    
    This adds the underline resource to underline one character of the label
*/

#ifndef _FigSmeBSBP_h
#define _FigSmeBSBP_h

/***********************************************************************
 *
 * Sme Object Private Data
 *
 ***********************************************************************/

#ifdef XAW3D
#include <X11/Xaw3d/SmeP.h>
#else
#include <X11/Xaw/SmeP.h>
#endif /* XAW3D */

#include "SmeBSBP.h"

/* our header file */
#include "w_menuentry.h"

/************************************************************
 *
 * New fields for the Fig Sme Object class record.
 *
 ************************************************************/

typedef struct {
  int make_compiler_happy;	/* just so it's not empty */
} FigSmeBSBClassPart;

/* Full class record declaration */
typedef struct _FigSmeBSBClassRec {
    RectObjClassPart	rect_class;
    SmeClassPart	sme_class;
#ifdef XAW3D
    SmeThreeDClassPart	sme_threeD_class;
#endif /* XAW3D */
    SmeBSBClassPart	sme_bsb_class;
    FigSmeBSBClassPart	figSme_bsb_class;
} FigSmeBSBClassRec;

extern FigSmeBSBClassRec figSmeBSBClassRec;

/* New fields for the FigSme Object record */
typedef struct {
    /* resources */
    int underline;		/* which letter of the label to underline */

    /* private resources. */
    int make_compiler_happy;	/* just so it's not empty */

} FigSmeBSBPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _FigSmeBSBRec {
    ObjectPart		object;
    RectObjPart		rectangle;
    SmePart		sme;
#ifdef XAW3D
    SmeThreeDPart	sme_threeD;
#endif /* XAW3D */
    SmeBSBPart		sme_bsb;
    FigSmeBSBPart	figSme_bsb;
} FigSmeBSBRec;

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

#endif /* _FigSmeBSBP_h */
