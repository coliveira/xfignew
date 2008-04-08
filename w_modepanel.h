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

extern void	force_positioning(void);
extern void	force_nopositioning(void);
extern void	force_anglegeom(void);
extern void	force_noanglegeom(void);
extern void	init_mode_panel(Widget tool);


#include "w_icons.h"

#define MAX_MODEMSG_LEN 80
typedef struct mode_switch_struct {
    icon_struct	   *icon;		/* icon (xxx_ic struct) */
    int		    mode;		/* mode (e.g. F_CIRCLE_BY_RAD) */
    void	    (*setmode_func) ();	/* function called when button is released */
    int		    objmask;		/* mask of objects that may be affected by this */
    unsigned long   indmask;		/* mask to display indicators for this func */
    char	    modemsg[MAX_MODEMSG_LEN];  /* message for function */
    Boolean	    popup;		/* true for commands that popup something */
    Widget	    widget;
    Pixmap	    pixmap, reversePM;
}               mode_sw_info;

extern void change_mode (icon_struct *icon);
extern void turn_off_current (void);
extern void update_modepanel ();
extern void setup_mode_panel(void);
