/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
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

extern void	init_topruler(Widget tool);
extern void	popup_unit_panel(void);
extern void	erase_rulermark(void);
extern void	set_unit_indicator(Boolean use_userscale);
extern void init_unitbox(Widget tool);
extern void init_sideruler(Widget tool);
extern void redisplay_sideruler (void);
extern void redisplay_topruler (void);
extern void reset_sideruler (void);
extern void reset_topruler (void);
extern void resize_sideruler (void);
extern void resize_topruler (void);
extern void setup_sideruler (void);
extern void reset_rulers (void);
extern void update_rulerpanel ();
extern void setup_rulers(void);
extern void set_rulermark(int x, int y);
extern void erase_siderulermark(void);
extern void erase_toprulermark(void);
extern void set_rulermark(int x, int y);
extern void set_siderulermark(int y);
extern void set_toprulermark(int x);
extern void setup_topruler(void);
