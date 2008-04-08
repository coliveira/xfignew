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

#ifndef U_UNDO_H
#define U_UNDO_H

/*******************  DECLARE EXPORTS  ********************/

extern F_compound	 saved_objects;
extern F_compound	 object_tails;
extern F_arrow		*saved_for_arrow;
extern F_arrow		*saved_back_arrow;
extern F_line		*latest_line;		/* for undo_join (line) */
extern F_spline		*latest_spline;		/* for undo_join (spline) */
extern void		 undo(void);
extern void clean_up (void);
extern void set_action (int action);
extern void set_action_object (int action, int object);
extern void set_last_arcpointnum (int num);
extern void set_last_arrows (F_arrow *forward, F_arrow *backward);
extern void set_last_nextpoint (F_point *next_point);
extern void set_last_prevpoint (F_point *prev_point);
extern void set_last_selectedpoint (F_point *selected_point);
extern void set_last_selectedsfactor (F_sfactor *selected_sfactor);
extern void set_last_tension (double origin, double extremity);
extern void set_lastlinkinfo (int mode, F_linkinfo *links);
extern void set_lastposition (int x, int y);
extern void set_latestarc (F_arc *arc);
extern void set_latestcompound (F_compound *compound);
extern void set_latestellipse (F_ellipse *ellipse);
extern void set_latestline (F_line *line);
extern void set_latestobjects (F_compound *objects);
extern void set_latestspline (F_spline *spline);
extern void set_latesttext (F_text *text);
extern void set_newposition (int x, int y);

#endif /* U_UNDO_H */
