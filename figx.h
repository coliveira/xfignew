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

#ifndef FIGX_H
#define FIGX_H

#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#ifdef XAW3D
#include <X11/Xaw3d/Command.h>
#include <X11/Xaw3d/Label.h>
#include <X11/Xaw3d/Dialog.h>
#include <X11/Xaw3d/Box.h>
#include <X11/Xaw3d/Form.h>
#include <X11/Xaw3d/Cardinals.h>
#include <X11/Xaw3d/Text.h>
#include <X11/Xaw3d/AsciiText.h>
#include <X11/Xaw3d/MenuButton.h>
#include <X11/Xaw3d/Scrollbar.h>
#include <X11/Xaw3d/SimpleMenu.h>
#include <X11/Xaw3d/Sme.h>
#include <X11/Xaw3d/SmeLine.h>
#ifdef XAW3D1_5E
#include <X11/Xaw3d/Tip.h>
#endif /* XAW3D1_5E */
#include <X11/Xaw3d/Toggle.h>
#include <X11/Xaw3d/Paned.h>
#include <X11/Xaw3d/Viewport.h>
#include <X11/Xaw3d/List.h>
#include <X11/Xaw3d/SmeBSB.h>
#else /* XAW3D */
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Sme.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>
#include "SmeBSB.h"
#endif /* XAW3D */

#endif /* FIGX_H */
