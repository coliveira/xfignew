/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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

#ifndef E_EDIT_H
#define E_EDIT_H

extern void	change_sfactor(int x, int y, unsigned int button);
extern void	Quit(void);
extern void	clear_text_key(Widget w);
extern void	paste_panel_key(Widget w, XKeyEvent *event);
extern Widget	color_selection_panel(char *label, char *wname, char *name, Widget parent, Widget below, Widget beside, Widget *button, Widget *popup, int color, XtCallbackProc callback);
extern void	color_select(Widget w, Color color);
extern void edit_item (F_line *p, int type, int x, int y);
extern void edit_item_selected (void);
extern void push_apply_button (void);

extern Boolean	edit_remember_lib_mode;	     /* Remember that we were in library mode */
extern Boolean	edit_remember_dimline_mode;  /* Remember that we were in dimension line mode */

extern Widget	pic_name_panel;

#endif /* E_EDIT_H */
