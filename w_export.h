/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2002 by Brian V. Smith
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

extern Widget	export_popup;	/* the main export popup */
extern Widget	export_panel;	/* the form it's in */
extern Widget	exp_selfile,	/* output (selected) file widget */
		exp_mask,	/* mask widget */
		exp_dir,	/* current directory widget */
		exp_flist,	/* file list widget */
		exp_dlist;	/* dir list widget */
extern Boolean	export_up;

extern char	default_export_file[];

extern Widget	export_orient_panel;
extern Widget	export_just_panel;
extern Widget	export_papersize_panel;
extern Widget	export_multiple_panel;
extern Widget	export_overlap_panel;
extern Widget	export_mag_text;
extern void	export_update_figure_size(void);
extern Widget	export_transp_panel;
extern Widget	export_background_panel;

extern Widget	export_grid_minor_text, export_grid_major_text;
extern Widget	export_grid_minor_menu_button, export_grid_minor_menu;
extern Widget	export_grid_major_menu_button, export_grid_major_menu;
extern Widget	export_grid_unit_label;
extern void	export_grid_minor_select(Widget w, XtPointer new_grid_choice, XtPointer call_data);
extern void	export_grid_major_select(Widget w, XtPointer new_grid_choice, XtPointer call_data);

extern void	popup_export_panel(Widget w);
extern void	do_export(Widget w);
extern void update_def_filename (void);
