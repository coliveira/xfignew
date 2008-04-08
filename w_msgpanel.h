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

extern void	init_msg(Widget tool);
extern void	put_msg(const char *format, ...);
extern void	file_msg(char *format, ...);
extern void	boxsize_msg(int fact);
extern void	length_msg(int type);
extern void	altlength_msg(int type, int fx, int fy);
extern void	length_msg2(int x1, int y1, int x2, int y2, int x3, int y3);
extern void	popup_file_msg(void);
extern void	make_dimension_string(float length, char *str, Boolean square);

extern Boolean	popup_up;
extern Boolean	first_file_msg;
extern Boolean	file_msg_is_popped;
extern Widget	file_msg_popup;
extern Boolean	first_lenmsg;

extern void boxsize_scale_msg (int fact);
extern void erase_box_lengths (void);
extern void erase_lengths (void);
extern void arc_msg (int x1, int y1, int x2, int y2, int x3, int y3);
extern void areameas_msg (char *msgtext, float area, float totarea, int flag);
extern void lenmeas_msg (char *msgtext, float len, float totlen);
extern void setup_msg(void);
