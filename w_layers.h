/*
 * This layer code written by Tim Love <tpl@eng.cam.ac.uk>  June 25, 1998
 *
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

extern Boolean	active_layers[MAX_DEPTH +1];
extern int	object_depths[MAX_DEPTH +1], saved_depths[MAX_DEPTH +1];
extern int	saved_min_depth, saved_max_depth;
extern Boolean	save_layers[MAX_DEPTH+1];
extern Widget	layer_form;
extern Boolean	gray_layers;

#define active_layer(layer) active_layers[layer]

extern void	init_depth_panel(Widget parent);
extern void	setup_depth_panel(void);
extern void	update_layers(void);
extern void	toggle_show_depths(void);
extern	void	save_active_layers(void), restore_active_layers(void);
extern	void	save_counts(void), save_counts_and_clear(void), restore_counts(void);
extern	void	save_depths(void), restore_depths(void);

extern Boolean	any_active_in_compound(F_compound *cmpnd);
extern void	reset_layers(void);
extern void	reset_depths(void);

extern int	LAYER_WD, LAYER_HT;
extern void update_layerpanel ();
