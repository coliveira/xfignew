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

#define DEF_ICON_SIZE	  60		/* size (square) of object icon */

typedef struct f_libobj {
    struct {
	    	int x, y;
		} corner;
    struct f_compound *compound;
}
	F_libobject;

extern F_libobject **lib_compounds;
extern char	   **library_objects_texts;

extern void	popup_library_panel(void);
extern void set_comments(char *comments);
