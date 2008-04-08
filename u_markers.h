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

#ifndef U_MARKERS_H
#define U_MARKERS_H

extern void          toggle_pointmarker(int x, int y);
extern int anyline_in_mask (void);
extern int anyspline_in_mask (void);
extern int anytext_in_mask (void);
extern int arc_in_mask (void);
extern void center_marker (int x, int y);
extern int compound_in_mask (void);
extern int ellipse_in_mask (void);
extern void mask_toggle_arcmarker (F_arc *a);
extern void mask_toggle_compoundmarker (F_compound *c);
extern void mask_toggle_ellipsemarker (F_ellipse *e);
extern void mask_toggle_linemarker (F_line *l);
extern void mask_toggle_splinemarker (F_spline *s);
extern void mask_toggle_textmarker (F_text *t);
extern void toggle_all_compoundmarkers (void);
extern void toggle_archighlight (F_arc *a);
extern void toggle_arcmarker (F_arc *a);
extern void toggle_compoundhighlight (F_compound *c);
extern void toggle_compoundmarker (F_compound *c);
extern void toggle_csrhighlight (int x, int y);
extern void toggle_ellipsehighlight (F_ellipse *e);
extern void toggle_ellipsemarker (F_ellipse *e);
extern void toggle_linehighlight (F_line *l);
extern void toggle_linemarker (F_line *l);
extern void toggle_markers_in_compound (F_compound *cmpnd);
extern void toggle_splinehighlight (F_spline *s);
extern void toggle_splinemarker (F_spline *s);
extern void toggle_texthighlight (F_text *t);
extern void toggle_textmarker (F_text *t);
extern void update_markers (int mask);
extern int validline_in_mask (F_line *l);
extern int validspline_in_mask (F_spline *s);
extern int validtext_in_mask (F_text *t);

#endif /* U_MARKERS_H */
