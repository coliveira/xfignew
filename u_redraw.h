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

extern void	redisplay_canvas(void);
extern Boolean	request_redraw;		/* set in redisplay_region if called when
					   preview_in_progress is true */
extern void	clearcounts(void);		/* clear object counters for each depth */
extern void	clearallcounts(void);	/* clear all object counters for each depth */

/*
 * Support for rendering based on correct object depth.	 A simple depth based
 * caching scheme; anything more will require major surgery on the object
 * data structures that will percolate throughout program.
 *
 * One ``counts'' structure for each object type at each nesting depth from 0
 * to MAX_DEPTH - 1.  We track both the number of objects per type per depth,
 * as well as the number of objects drawn so far per type per depth to cut
 * down on search loop overhead.
 */

struct counts {
    unsigned	    num_arcs;		/* # arcs at this depth */
    unsigned	    num_lines;		/* # lines at this depth */
    unsigned	    num_ellipses;	/* # ellipses at this depth */
    unsigned	    num_splines;	/* # splines at this depth */
    unsigned	    num_texts;		/* # texts at this depth */
    unsigned	    cnt_arcs;		/* count of arcs drawn at this depth */
    unsigned	    cnt_lines;		/* count of lines drawn at this depth */
    unsigned	    cnt_ellipses;	/* count of ellipses drawn at this
					 * depth */
    unsigned	    cnt_splines;	/* count of splines drawn at this depth */
    unsigned	    cnt_texts;		/* count of texts drawn at this depth */
};

extern struct counts	counts[], saved_counts[];
extern void redisplay_arc (F_arc *a);
extern void redisplay_arcs (F_arc *a1, F_arc *a2);
extern void redisplay_compound (F_compound *c);
extern void redisplay_compoundobject (F_compound *compounds, int depth);
extern void redisplay_compounds (F_compound *c1, F_compound *c2);
extern void redisplay_ellipse (F_ellipse *e);
extern void redisplay_ellipses (F_ellipse *e1, F_ellipse *e2);
extern void redisplay_line (F_line *l);
extern void redisplay_lines (F_line *l1, F_line *l2);
extern void redisplay_objects (F_compound *active_objects);
extern void redisplay_pageborder (void);
extern void redisplay_spline (F_spline *s);
extern void redisplay_splines (F_spline *s1, F_spline *s2);
extern void redisplay_text (F_text *t);
extern void redisplay_texts (F_text *t1, F_text *t2);
extern void redisplay_zoomed_region (int xmin, int ymin, int xmax, int ymax);
extern void update_pageborder (void);

extern void redisplay_region (int xmin, int ymin, int xmax, int ymax);
extern void redisplay_regions (int xmin1, int ymin1, int xmax1, int ymax1, int xmin2, int ymin2, int xmax2, int ymax2);
