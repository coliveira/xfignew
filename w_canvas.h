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

#ifndef W_CANVAS_H
#define W_CANVAS_H

/************** DECLARE EXPORTS ***************/

extern void	init_canvas(Widget tool);
extern void	add_canvas_actions(void);
extern void	(*canvas_kbd_proc) (int, int, int);
extern void	(*canvas_locmove_proc) (int, int);
extern void	(*canvas_ref_proc) (int, int, int);
extern void	(*canvas_leftbut_proc) (int, int, int);
extern void	(*canvas_middlebut_proc) (int, int, int);
extern void	(*canvas_middlebut_save) (int, int);
extern void	(*canvas_rightbut_proc) (int, int, int);
extern void	(*return_proc) ();
extern void	null_proc(int, int);
extern void	toggle_show_balloons(void);
extern void	toggle_show_lengths(void);
extern void	toggle_show_vertexnums(void);
extern void	toggle_show_borders(void);

extern void		canvas_selected(Widget tool, XButtonEvent *event, String *params, Cardinal *nparams);
extern void	paste_primary_selection(void);
extern void setup_canvas(void);
extern void clear_region(int xmin, int ymin, int xmax, int ymax);
extern void clear_canvas(void);

extern int	clip_xmin, clip_ymin, clip_xmax, clip_ymax;
extern int	clip_width, clip_height;
extern int	cur_x, cur_y;
extern int	fix_x, fix_y;
extern int	ignore_exp_cnt;
extern int	last_x, last_y;	/* last position of mouse */
extern int	shift;		/* global state of shift key */
extern int	pointer_click;	/* for counting multiple clicks */

/* for Sun keyboard, define COMP_LED 2 */
extern void	setCompLED(int on);

extern String	local_translations;

#define LOC_OBJ	"Locate Object"

/* macro which rounds coordinates depending on point positioning mode */
#define		round_coords(x, y) \
  if (cur_pointposn != P_ANY) \
    if (!anypointposn) { \
	int _txx; \
	if (x<0) \
	    x = ((_txx = abs(x)%posn_rnd[cur_gridunit][cur_pointposn]) < posn_hlf[cur_gridunit][cur_pointposn]) \
		? x + _txx : x - posn_rnd[cur_gridunit][cur_pointposn] + _txx; \
	else \
	    x = ((_txx = x%posn_rnd[cur_gridunit][cur_pointposn]) < posn_hlf[cur_gridunit][cur_pointposn]) \
		? x - _txx : x + posn_rnd[cur_gridunit][cur_pointposn] - _txx; \
	if (y<0) \
	    y = ((_txx = abs(y)%posn_rnd[cur_gridunit][cur_pointposn]) < posn_hlf[cur_gridunit][cur_pointposn]) \
		? y + _txx : y - posn_rnd[cur_gridunit][cur_pointposn] + _txx; \
	else \
	    y = ((_txx = y%posn_rnd[cur_gridunit][cur_pointposn]) < posn_hlf[cur_gridunit][cur_pointposn]) \
		? y - _txx : y + posn_rnd[cur_gridunit][cur_pointposn] - _txx; \
    }

#endif /* W_CANVAS_H */
