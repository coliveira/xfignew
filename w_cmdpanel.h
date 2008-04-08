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

/* width of a command button */
#define CMD_BUT_WD 60
#define CMD_BUT_HT 22


/* def for menu */

typedef struct {
    char  *name;		/* name e.g. 'Save' */
    int	   u_line;		/* which character to underline (-1 means none) */
    void  (*func)();		/* function that is called for menu choice */
    Boolean checkmark;		/* whether a checkmark is put in the left bitmap space */
} menu_def ;

/* cmd panel menu definitions */

#define CMD_LABEL_LEN	16

typedef struct main_menu_struct {
    char	    label[CMD_LABEL_LEN];	/* label on the button */
    char	    menu_name[CMD_LABEL_LEN];	/* command name for resources */
    char	    hint[CMD_LABEL_LEN];	/* label for mouse func and balloon */
    menu_def	   *menu;			/* menu */
    Widget	    widget;			/* button widget */
    Widget	    menuwidget;			/* menu widget */
}	main_menu_info;

extern main_menu_info main_menus[];

extern void	init_cmd_panel();
extern void	add_cmd_actions(void);
extern void	quit(Widget w, XtPointer closure, XtPointer call_data);
extern void	New(Widget w, XtPointer closure, XtPointer call_data);
extern void	paste(Widget w, XtPointer closure, XtPointer call_data);
extern void	delete_all_cmd(Widget w, int closure, int call_data);
extern void	setup_cmd_panel();
extern void	change_orient();
extern void	setup_cmd_panel();
extern void	show_global_settings(Widget w);
extern void	acc_load_recent_file(Widget w, XEvent *event, String *params, Cardinal *nparams);
extern int	num_main_menus(void);
extern Widget	create_menu_item(main_menu_info *menup);
extern void	refresh_view_menu(void);
extern void	popup_character_map(void);
extern void	refresh_character_panel(void);
extern void     init_main_menus(Widget tool, char *filename);

extern void goodbye (Boolean abortflag);
extern void update_cur_filename (char *newname);
extern void rebuild_file_menu (Widget menu);
void setup_main_menus(void);
