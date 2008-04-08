/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
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

#ifndef W_MOUSEFUN_H
#define W_MOUSEFUN_H

void		init_mousefun(Widget tool);
void		setup_mousefun(void);
void		set_mousefun(const char *left, const char *middle, const char *right, const char *sh_left, const char *sh_middle, const char *sh_right);
void		draw_mousefun_mode(void);
void		draw_mousefun_ind(void);
void		draw_mousefun_unitbox(void);
void		shift_top_mousfun();
void		draw_mousefun_topruler(Widget w, XEvent *event, String *params, Cardinal *num_params);
void		shift_side_mousfun();
void		draw_mousefun_sideruler(Widget w, XEvent *event, String *params, Cardinal *num_params);
void		draw_mousefun_canvas(void);
void		draw_mousefun(const char *left, const char *middle, const char *right);
void		draw_mousefn2(const char *left, const char *middle, const char *right);
void		clear_mousefun(void);
void		notused_middle(void);
void		clear_middle(void);
void		notused_right(void);
void		clear_right(void);
void		draw_mousefun_kbd(void);
void		clear_mousefun_kbd(void);
void		init_mousefun_actions();
extern String	kbd_translations;
extern void init_kbd_actions (void);
extern void update_mousepanel ();
extern void draw_shift_mousefun_canvas(void);
extern void draw_shift_mousefun_canvas2(const char *tl, const char *tm, const char *tr);

#endif /* W_MOUSEFUN_H */
