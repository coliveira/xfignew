/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2002 by Brian V. Smith
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



#ifndef U_FONTS_H
#define U_FONTS_H

#define DEF_FONTSIZE		12		/* default font size in pts */
#define DEF_PS_FONT		0
#define DEF_LATEX_FONT		0
#define PS_FONTPANE_WD		290
#define LATEX_FONTPANE_WD	112
#define PS_FONTPANE_HT		20
#define LATEX_FONTPANE_HT	20
#define NUM_FONTS		35
#define NUM_LATEX_FONTS		6
/* font number for the "nil" font (when user wants tiny text) */
#define NILL_FONT NUM_FONTS

/* element of linked list for each font
   The head of list is for the different font NAMES,
   and the elements of this list are for each different
   point size of that font */

struct xfont {
    int		    size;	/* size in points */
    Font	    fid;	/* X font id */
    char	   *fname;	/* actual name of X font found */
    char	   *bname;	/* name of backup X font to try if first doesn't exist */
    XFontStruct	   *fstruct;	/* X font structure */
    struct xfont   *next;	/* next in the list */
};

struct _fstruct {
    char	   *name;	/* Postscript font name */
    int		    xfontnum;	/* template for locating X fonts */
};

struct _xfstruct {
    char	   *Template;	/* template for locating X fonts */
    struct xfont   *xfontlist;	/* linked list of X fonts for different point
				 * sizes */
};

extern int		psfontnum(char *font);
extern int		latexfontnum(char *font);

extern struct _xfstruct	x_fontinfo[], x_backup_fontinfo[];
extern struct _fstruct	ps_fontinfo[];
extern struct _fstruct	latex_fontinfo[];

int		x_fontnum(int psflag, int fnum);
#endif /* U_FONTS_H */
