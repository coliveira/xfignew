/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2004 by Chris Moller
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

#ifndef W_KEYBOARD_H
#define W_KEYBOARD_H

extern void popup_keyboard_panel(Widget widget,
				 XButtonEvent * event,
				 String * params,
				 Cardinal * num_params);
extern void popdown_keyboard_panel();

extern Boolean keyboard_input_available;

extern int keyboard_state;
extern int keyboard_x;
extern int keyboard_y;

#endif
