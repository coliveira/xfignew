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

/* Private definitions for SmeCascade object */

#ifndef _XawSmeCascadeP_h
#define _XawSmeCascadeP_h

/***********************************************************************
 *
 * Sme Cascade Object Private Data
 *
 ***********************************************************************/

#ifdef XAW3D
#include <X11/Xaw3d/SmeThreeDP.h>
#else
#include <X11/Xaw/SmeP.h>
#endif

#include "SmeBSBP.h"
#include "SmeCascade.h"

/************************************************************
 *
 * New fields for the Sme Cascade Object class record.
 *
 ************************************************************/

typedef struct _SmeCascadeClassPart {
  XtPointer extension;
} SmeCascadeClassPart;

/* Full class record declaration */
typedef struct _SmeCascadeClassRec {
    RectObjClassPart       	rect_class;
    SmeClassPart     		sme_class;
#ifdef XAW3D
    SmeThreeDClassPart 		sme_threeD_class;
#endif /* XAW3D */
    SmeBSBClassPart  		sme_bsb_class;
    SmeCascadeClassPart		sme_cascade_class;
} SmeCascadeClassRec;

extern SmeCascadeClassRec smeCascadeClassRec;

/* New fields for the Sme Cascade Object record */
typedef struct {
    /* resources */
    Widget	subMenu;	/* sub-menu that I post */
    Boolean	selectCascade;	/* whether or not to make this cascade selectable (Notify) */
    Boolean	highlighted;	/* holds the state of the entry */
    /* private fields */
} SmeCascadePart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _SmeCascadeRec {
  ObjectPart		object;
  RectObjPart		rectangle;
  SmePart		sme;
#ifdef XAW3D
  SmeThreeDPart		sme_threeD;
#endif /* XAW3D */
  SmeBSBPart   		sme_bsb;
  SmeCascadePart	sme_cascade;
} SmeCascadeRec;

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

#endif /* _XawSmeCascadeP_h */
