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

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "u_fonts.h"		/* printer font names */
#include "w_fontbits.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_fontpanel.h"
#include "w_setup.h"
#include "w_util.h"
#include "w_color.h"

/********************  local variables	***************************/

static int     *font_ps_sel;	/* ptr to store selected ps font in */
static int     *font_latex_sel; /* ptr to store selected latex font */
static int     *flag_sel;	/* pointer to store ps/latex flag */
static Widget	font_widget;	/* widget adr to store font image in */
static void	(*font_setimage) ();

static MenuItemRec ps_fontmenu_items[NUM_FONTS + 1];
static MenuItemRec latex_fontmenu_items[NUM_LATEX_FONTS];

static void	fontpane_select(Widget w, XtPointer closure, XtPointer call_data);
static void	fontpane_cancel(void);
static void	fontpane_swap(void);

static XtCallbackRec pane_callbacks[] =
{
    {fontpane_select, NULL},
    {NULL, NULL},
};

static String	fontpane_translations =
	"<Message>WM_PROTOCOLS: FontPaneCancel()\n\
	 <Key>Escape: FontPaneCancel()\n";
static XtActionsRec	fontpane_actions[] =
{
    {"FontPaneCancel", (XtActionProc) fontpane_cancel},
};

static Widget	ps_fontpanes, ps_form;
static Widget	latex_fontpanes, latex_form;
static Widget	ps_fontpane[NUM_FONTS+1];
static Widget	latex_fontpane[NUM_LATEX_FONTS];
static Boolean	first_fontmenu;



void
init_fontmenu(Widget tool)
{
    Widget	    tmp_but, ps_cancel, latex_cancel;
    register int    i;
    register MenuItemRec *mi;
    XtTranslations  pane_actions;

    DeclareArgs(8);

    first_fontmenu = True;

    FirstArg(XtNborderWidth, POPUP_BW);
    NextArg(XtNmappedWhenManaged, False);
    NextArg(XtNtitle, "Xfig: Font menu");

    ps_fontmenu = XtCreatePopupShell("ps_font_menu",
				     transientShellWidgetClass, tool,
				     Args, ArgCount);
    XtOverrideTranslations(ps_fontmenu,
			XtParseTranslationTable(fontpane_translations));
    latex_fontmenu = XtCreatePopupShell("latex_font_menu",
					transientShellWidgetClass, tool,
					Args, ArgCount);
    XtOverrideTranslations(latex_fontmenu,
			XtParseTranslationTable(fontpane_translations));
    XtAppAddActions(tool_app, fontpane_actions, XtNumber(fontpane_actions));

    FirstArg(XtNwidth, PS_FONTPANE_WD*2);
    NextArg(XtNdefaultDistance, INTERNAL_BW);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    ps_form = XtCreateManagedWidget("ps_form", formWidgetClass,
				       ps_fontmenu, Args, ArgCount);
    XtOverrideTranslations(ps_form,
			XtParseTranslationTable(fontpane_translations));

    FirstArg(XtNwidth, LATEX_FONTPANE_WD*2);
    NextArg(XtNdefaultDistance, INTERNAL_BW);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    latex_form = XtCreateManagedWidget("latex_form", formWidgetClass,
					  latex_fontmenu, Args, ArgCount);
    XtOverrideTranslations(latex_fontmenu,
			XtParseTranslationTable(fontpane_translations));

    /* box with PostScript font image buttons */
    FirstArg(XtNvSpace, -INTERNAL_BW);
    NextArg (XtNhSpace, -INTERNAL_BW);
    NextArg (XtNwidth, PS_FONTPANE_WD*2 + INTERNAL_BW*4);	/* two across */
    NextArg (XtNhSpace, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    ps_fontpanes = XtCreateManagedWidget("menu", boxWidgetClass,
					 ps_form, Args, ArgCount);

    /* box with LaTeX font image buttons */
    FirstArg(XtNvSpace, -INTERNAL_BW);
    NextArg (XtNhSpace, -INTERNAL_BW);
    NextArg (XtNwidth, LATEX_FONTPANE_WD*2 + INTERNAL_BW*4);	/* two across */
    NextArg (XtNhSpace, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    latex_fontpanes = XtCreateManagedWidget("menu", boxWidgetClass,
					    latex_form, Args, ArgCount);

    for (i = 0; i < NUM_FONTS + 1; i++) {
	ps_fontmenu_items[i].type = MENU_IMAGESTRING;		/* put the fontnames in
								 * menu */
	ps_fontmenu_items[i].label = ps_fontinfo[i].name;
	ps_fontmenu_items[i].info = (caddr_t) (i - 1);		/* index for font # */
    }

    for (i = 0; i < NUM_LATEX_FONTS; i++) {
	latex_fontmenu_items[i].type = MENU_IMAGESTRING;	/* put the fontnames in
								 * menu */
	latex_fontmenu_items[i].label = latex_fontinfo[i].name;
	latex_fontmenu_items[i].info = (caddr_t) i;		/* index for font # */
    }

    pane_actions = XtParseTranslationTable("<EnterWindow>:set()\n\
		<Btn1Up>:notify()unset()\n");

    /* make the PostScript font image buttons */
    FirstArg(XtNwidth, PS_FONTPANE_WD);
    NextArg(XtNheight, PS_FONTPANE_HT+4);
    NextArg(XtNcallback, pane_callbacks);
    NextArg(XtNbitmap, NULL);
    NextArg(XtNinternalWidth, 0);	/* space between pixmap and edge */
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNresize, False);	/* don't allow resize */

    for (i = 0; i < NUM_FONTS + 1; ++i) {
	mi = &ps_fontmenu_items[i];
	pane_callbacks[0].closure = (caddr_t) mi;
	ps_fontpane[i] = XtCreateManagedWidget("pane", commandWidgetClass,
					       ps_fontpanes, Args, ArgCount);
	XtOverrideTranslations(ps_fontpane[i], pane_actions);
    }
    /* Cancel */
    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, ps_fontpanes);
    NextArg(XtNhorizDistance, 4);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    ps_cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				    ps_form, Args, ArgCount);
    XtAddEventHandler(ps_cancel, ButtonReleaseMask, False,
		      fontpane_cancel, (XtPointer) NULL);

    /* button to switch to the LaTeX fonts */
    FirstArg(XtNlabel, " Use LaTex Fonts ");
    NextArg(XtNfromVert, ps_fontpanes);
    NextArg(XtNfromHoriz, ps_cancel);
    NextArg(XtNhorizDistance, 10);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    tmp_but = XtCreateManagedWidget("use_latex_fonts", commandWidgetClass,
				    ps_form, Args, ArgCount);
    XtAddEventHandler(tmp_but, ButtonReleaseMask, False,
		      fontpane_swap, (XtPointer) NULL);

    /* now make the LaTeX font image buttons */
    FirstArg(XtNwidth, LATEX_FONTPANE_WD);
    NextArg(XtNheight, LATEX_FONTPANE_HT+4);
    NextArg(XtNcallback, pane_callbacks);
    NextArg(XtNbitmap, NULL);
    NextArg(XtNinternalWidth, 0);	/* space between pixmap and edge */
    NextArg(XtNinternalHeight, 0);
    NextArg(XtNborderWidth, INTERNAL_BW);
    NextArg(XtNresize, False);	/* don't allow resize */

    for (i = 0; i < NUM_LATEX_FONTS; ++i) {
	mi = &latex_fontmenu_items[i];
	pane_callbacks[0].closure = (caddr_t) mi;
	latex_fontpane[i] = XtCreateManagedWidget("pane", commandWidgetClass,
					   latex_fontpanes, Args, ArgCount);
	XtOverrideTranslations(latex_fontpane[i], pane_actions);
    }
    /* Cancel */
    FirstArg(XtNlabel, "Cancel");
    NextArg(XtNfromVert, latex_fontpanes);
    NextArg(XtNhorizDistance, 4);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    latex_cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				    latex_form, Args, ArgCount);
    XtAddEventHandler(latex_cancel, ButtonReleaseMask, False,
		      fontpane_cancel, (XtPointer) NULL);

    /* button to switch to the PostScript fonts */
    FirstArg(XtNlabel, " Use PostScript Fonts ");
    NextArg(XtNfromVert, latex_fontpanes);
    NextArg(XtNfromHoriz, latex_cancel);
    NextArg(XtNhorizDistance, 10);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    tmp_but = XtCreateManagedWidget("use_postscript_fonts", commandWidgetClass,
				    latex_form, Args, ArgCount);
    XtAddEventHandler(tmp_but, ButtonReleaseMask, False,
		      fontpane_swap, (XtPointer) NULL);

    XtInstallAccelerators(ps_form, ps_cancel);
    XtInstallAccelerators(latex_form, latex_cancel);
}

/* create the bitmaps for the font menu */

void
setup_fontmenu(void)
{
    register int    i;

    DeclareArgs(2);

    Pixel	    bg, fg;

    /* get the foreground/background of the widget */
    FirstArg(XtNforeground, &fg);
    NextArg(XtNbackground, &bg);
    GetValues(ps_fontpane[0]);

    /* Create the bitmaps */

#ifdef I18N
    if (appres.international) {
      char *lang;
      lang = appres.font_menu_language;
      if (lang[0] == '\0') lang = setlocale(LC_CTYPE, NULL);
      if (strncasecmp(lang, "japanese", 2) == 0) {
	extern unsigned char Japanese_Times_Roman_bits[], Japanese_Roman_bits[];
	extern unsigned char Japanese_Times_Bold_bits[], Japanese_Bold_bits[];
	psfont_menu_bits[1] = Japanese_Times_Roman_bits;
	latexfont_menu_bits[1] = Japanese_Roman_bits;
	psfont_menu_bits[3] = Japanese_Times_Bold_bits;
	latexfont_menu_bits[2] = Japanese_Bold_bits;
      } else if (strncasecmp(lang, "korean", 2) == 0) {
	extern unsigned char Korean_Times_Roman_bits[], Korean_Roman_bits[];
	extern unsigned char Korean_Times_Bold_bits[], Korean_Bold_bits[];
	psfont_menu_bits[1] = Korean_Times_Roman_bits;
	latexfont_menu_bits[1] = Korean_Roman_bits;
	psfont_menu_bits[3] = Korean_Times_Bold_bits;
	latexfont_menu_bits[2] = Korean_Bold_bits;
      }
    }
#endif  /* I18N */
    for (i = 0; i < NUM_FONTS + 1; i++)
	psfont_menu_bitmaps[i] = XCreatePixmapFromBitmapData(tool_d,
				   XtWindow(ind_panel), (char *) psfont_menu_bits[i],
				     PS_FONTPANE_WD, PS_FONTPANE_HT, fg, bg,
				      tool_dpth);

    for (i = 0; i < NUM_LATEX_FONTS; i++)
	latexfont_menu_bitmaps[i] = XCreatePixmapFromBitmapData(tool_d,
				     XtWindow(ind_panel), (char *) latexfont_menu_bits[i],
				      LATEX_FONTPANE_WD, LATEX_FONTPANE_HT, fg, bg,
				       tool_dpth);

    /* Store the bitmaps in the menu panes */
    for (i = 0; i < NUM_FONTS + 1; i++) {
	FirstArg(XtNbitmap, psfont_menu_bitmaps[i]);
	SetValues(ps_fontpane[i]);
    }
    for (i = 0; i < NUM_LATEX_FONTS; i++) {
	FirstArg(XtNbitmap, latexfont_menu_bitmaps[i]);
	SetValues(latex_fontpane[i]);
    }

    XtRealizeWidget(ps_fontmenu);
    XtRealizeWidget(latex_fontmenu);
    /* at this point the windows are realized but not drawn */
    XDefineCursor(tool_d, XtWindow(ps_fontpanes), arrow_cursor);
    XDefineCursor(tool_d, XtWindow(latex_fontpanes), arrow_cursor);
}

void
fontpane_popup(int *psfont_adr, int *latexfont_adr, int *psflag_adr, void (*showfont_fn) (/* ??? */), Widget show_widget)
{
    DeclareArgs(2);
    Position	    xposn, yposn;
    Widget	    widg;

    font_ps_sel = psfont_adr;
    font_latex_sel = latexfont_adr;
    flag_sel = psflag_adr;
    font_setimage = showfont_fn;
    font_widget = show_widget;
    if (first_fontmenu) {
	first_fontmenu = False;	/* don't reposition it if user has already popped it */
	XtTranslateCoords(tool, CANVAS_WD/4, CANVAS_HT/4, &xposn, &yposn);
	FirstArg(XtNx, xposn);	/* position about 1/4 from upper-left corner of canvas */
	NextArg(XtNy, yposn);
	SetValues(ps_fontmenu);
	SetValues(latex_fontmenu);
    }
    widg = *flag_sel ? ps_fontmenu : latex_fontmenu;
    XtPopup(widg, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(widg));
    XSetWMProtocols(tool_d, XtWindow(widg), &wm_delete_window, 1);
}

static void
fontpane_select(Widget w, XtPointer closure, XtPointer call_data)
{
    MenuItemRec	   *mi = (MenuItemRec *) closure;
    char	   *font_name = mi->label;

    if (*flag_sel)
	*font_ps_sel = (int) mi->info;	/* set ps font to one selected */
    else
	*font_latex_sel = (int) mi->info;	/* set latex font to one
						 * selected */
    put_msg("Font: %s", font_name);
    /* put image of font in indicator window */
    (*font_setimage) (font_widget);
    XtPopdown(*flag_sel ? ps_fontmenu : latex_fontmenu);
}

static void
fontpane_cancel(void)
{
    XtPopdown(*flag_sel ? ps_fontmenu : latex_fontmenu);
}

static void
fontpane_swap(void)
{
    Widget widg;

    XtPopdown(*flag_sel ? ps_fontmenu : latex_fontmenu);
    *flag_sel = 1 - *flag_sel;
    /* put image of font in indicator window */
    (*font_setimage) (font_widget);
    widg = *flag_sel ? ps_fontmenu : latex_fontmenu;
    XtPopup(widg, XtGrabExclusive);
    /* if the file message window is up add it to the grab */
    file_msg_add_grab();
    /* insure that the most recent colormap is installed */
    set_cmap(XtWindow(widg));
    XSetWMProtocols(tool_d, XtWindow(widg), &wm_delete_window, 1);
}
