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

void		init_trace_drawing(int x, int y);
void		create_lineobject(int x, int y);
void		get_intermediatepoint(int x, int y, int shift);
void		freehand_get_intermediatepoint(int x, int y);

extern Boolean	freehand_line;
extern void line_drawing_selected (void);
extern void line_drawing_selected (void);
