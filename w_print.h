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

extern Widget	print_popup;	/* the main print popup */
extern Widget	print_panel;	/* the form it's in */
extern Widget	print_orient_panel;
extern Widget	print_just_panel;
extern Widget	print_papersize_panel;
extern Widget	print_multiple_panel;
extern Widget	print_overlap_panel;
extern Widget	print_mag_text;
extern Widget	print_background_panel;
extern Widget	make_layer_choice(char *label_all, char *label_active, Widget parent, Widget below, Widget beside, int hdist, int vdist);

extern void	print_update_figure_size(void);
extern void	popup_print_panel(Widget w);
extern void	do_print(Widget w);
extern void	do_print_batch(Widget w);
extern Boolean	print_all_layers;

extern Widget	print_grid_minor_text, print_grid_major_text;
extern Widget	print_grid_minor_menu_button, print_grid_minor_menu;
extern Widget	print_grid_major_menu_button, print_grid_major_menu;
extern Widget	print_grid_unit_label;
extern void	print_grid_minor_select(Widget w, XtPointer new_grid_choice, XtPointer garbage);
extern void	print_grid_major_select(Widget w, XtPointer new_grid_choice, XtPointer garbage);
