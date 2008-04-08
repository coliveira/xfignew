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

extern Boolean	colors_are_swapped;
extern void	load_request(Widget w, XButtonEvent *ev);
extern void	do_save(Widget w, XButtonEvent *ev);
extern void	popup_open_panel(void);
extern void	popup_merge_panel(void);
extern void	popup_saveas_panel(void);
extern Boolean	query_save(char *msg);

extern Widget	preview_size, preview_widget, preview_name;
extern Pixmap	preview_land_pixmap, preview_port_pixmap;

extern Widget	file_panel;
extern Boolean	file_up;
extern Widget	cfile_text;
extern Widget	file_selfile;
extern Widget	file_mask;
extern Widget	file_dir;
extern Widget	file_flist;
extern Widget	file_dlist;
extern Widget	file_popup;

extern Boolean	check_cancel(void);
extern Boolean	cancel_preview;
extern Boolean	preview_in_progress;
extern void preview_figure (char *filename, Widget parent, Widget canvas, Widget size_widget, Pixmap port_pixmap, Pixmap land_pixmap);
extern int renamefile (char *file);
extern void file_panel_dismiss (void);
