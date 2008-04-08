/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1997 by T. Sato
 * Parts Copyright (c) 1997-2002 by Brian V. Smith
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

/************************************************************
Written by: T.Sato <VEF00200@niftyserve.or.jp> 4 March, 1997

This is the code for the spell check, search & replace features.
It is invoked by pressing <Meta>h in the canvas (Search() action).
This is defined in Fig.ad.

It provides the following features:

 - Search text objects which include given pattern.
   Comparison can be case-less or case-sensitive.
   If the pattern is empty, all text objects will listed.

 - Replace substring which match the given pattern.

 - Update attribute of text objects which include given pattern.
   If pattern is empty, all text objects will updated.
   Using this, users can change attribute such as size, font, etc
   of all text objects at one time.

 - Spell check all text objects and list misspelled words.

There is currently no way to undo replace/update operations.

****************************************************************/

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "d_text.h"
#include "e_update.h"
#include "f_util.h"
#include "w_drawprim.h"
#include "w_indpanel.h"
#include "w_listwidget.h"
#include "w_msgpanel.h"
#include "w_srchrepl.h"
#include "w_setup.h"
#include "w_util.h"
#include "u_create.h"

#include "mode.h"
#include "u_bound.h"
#include "u_fonts.h"
#include "u_redraw.h"
#include "w_canvas.h"
#include "w_color.h"

#include <stdarg.h>

#define MAX_MISSPELLED_WORDS	200
#define	SEARCH_WIDTH		496	/* width of search message and results */

static String search_panel_translations =
        "<Message>WM_PROTOCOLS: QuitSearchPanel()\n";
static String spell_panel_translations =
        "<Message>WM_PROTOCOLS: QuitSpellPanel()\n";

static String spell_text_translations =
	"<Key>Return: Correct()\n\
	Meta<Key>Q: QuitSpellPanel()\n\
	<Key>Escape: QuitSpellPanel()\n";
String  search_text_translations =
	"<Key>Return: SearchText()";
String  replace_text_translations =
	"<Key>Return: ReplaceText()";
String	search_results_translations = 
	"<Btn4Down>:	scroll-one-line-up()\n\
	<Btn5Down>:	scroll-one-line-down()";

static void	search_panel_dismiss(Widget widget, XtPointer closure, XtPointer call_data);
static void	search_and_replace_text(Widget widget, XtPointer closure, XtPointer call_data);
static Boolean	search_text_in_compound(F_compound *com, char *pattern, void (*proc) (/* ??? */));
static Boolean	replace_text_in_compound(F_compound *com, char *pattern, char *dst);
static void	found_text_panel_dismiss(void);
static void	do_replace(Widget widget, XtPointer closure, XtPointer call_data);
static void	show_search_result(char *format, ...);
static void	show_search_msg(char *format, ...);

static void	spell_panel_dismiss(Widget widget, XtPointer closure, XtPointer call_data);
static void	spell_select_word(Widget widget, XtPointer closure, XtPointer call_data);
static void	spell_correct_word(Widget widget, XtPointer closure, XtPointer call_data);
static void	show_spell_msg(char *format, ...);

static XtActionsRec search_actions[] =
{
    {"SearchText", (XtActionProc) search_and_replace_text},
    {"ReplaceText", (XtActionProc) do_replace},
    {"QuitSearchPanel", (XtActionProc) search_panel_dismiss},
    {"QuitFoundTextPanel", (XtActionProc) found_text_panel_dismiss},
};

static XtActionsRec spell_actions[] =
{
    {"QuitSpellPanel", (XtActionProc)spell_panel_dismiss},
    {"Correct", (XtActionProc) spell_correct_word},
};

static Widget	search_results_win;
static int	msg_length = 0;

static Widget	search_panel = None;
static Widget	search_text_widget, replace_text_widget;
static Widget	replace_text_label;
static Widget	search_button;

static Widget	found_text_panel = None;
static Widget	do_replace_button, do_update_button;
static int	found_text_cnt = 0;
static Widget	search_msg_win;

static Boolean	case_sensitive = False;

/* spell checker vars */

static Widget	 spell_check_panel = None;
static Widget	 spell_msg_win;
static Widget	 spell_viewport;
static Widget	 word_list, correct_word, correct_button, recheck_button;
static char	*miss_word_list[MAX_MISSPELLED_WORDS];
static char	 selected_word[200];

static Boolean	 do_replace_called;

DeclareStaticArgs(14);


static Boolean compare_string(char *str, char *pattern)
{
  if (case_sensitive) {
    return strncmp(str, pattern, strlen(pattern)) == 0;
  } else {
    return strncasecmp(str, pattern, strlen(pattern)) == 0;
  }
}

static void
do_replace(Widget widget, XtPointer closure, XtPointer call_data)
{
  int cnt;

  if (found_text_cnt > 0) {
    if (!do_replace_called &&
	strlen(panel_get_value(replace_text_widget)) == 0) {
      do_replace_called = True;
      show_search_msg("Click \"Replace\" again if you want remove \"Search for\" string");
      beep();
      return;
    }
    replace_text_in_compound(&objects, panel_get_value(search_text_widget),
				panel_get_value(replace_text_widget));
    found_text_panel_dismiss();
    redisplay_canvas();
    set_modifiedflag();
    cnt = found_text_cnt;
    search_and_replace_text(None, NULL, NULL);
    show_search_msg("%d object%s replaced", cnt, (cnt != 1)? "s":"");
  }
}

static Boolean 
replace_text_in_compound(F_compound *com, char *pattern, char *dst)
{
  F_compound	*c;
  F_text	*t;
  PR_SIZE	 size;
  Boolean	 replaced, processed;
  int		 pat_len, i, j;
  char		 str[300];

  pat_len = strlen(pattern);
  if (pat_len == 0) 
	return False;

  processed = False;
  for (c = com->compounds; c != NULL; c = c->next) {
    if (replace_text_in_compound(c, pattern, dst)) 
	processed = True;
  }
  for (t = com->texts; t != NULL; t = t->next) {
    replaced = False;
    if (pat_len <= strlen(t->cstring)) {
      str[0] = '\0';
      j = 0;
      for (i = 0; i <= strlen(t->cstring) - pat_len; i++) {
        if (compare_string(&t->cstring[i], pattern)) {
          if (strlen(str) + strlen(dst) < sizeof(str)) {
            strncat(str, &t->cstring[j], i - j);
            strcat(str, dst);
            i += pat_len - 1;
            j = i + 1;
            replaced = True;
          } else {  /* string becomes too long; don't replace it */
            replaced = False;
          }
        }
      }
      if (replaced && j < strlen(t->cstring)) {
        if (strlen(str) + strlen(&t->cstring[j]) < sizeof(str)) {
          strcat(str, &t->cstring[j]);
        } else {
          replaced = False;
        }
      }
      if (replaced) {  /* replace the text object */
        if (strlen(t->cstring) != strlen(str)) {
          free(t->cstring);
          t->cstring = new_string(strlen(str));
        }
        strcpy(t->cstring, str);
        size = textsize(lookfont(x_fontnum(psfont_text(t), t->font),
				t->size), strlen(t->cstring), t->cstring);
        t->ascent = size.ascent;
        t->descent = size.descent;
        t->length = size.length;
        processed = True;
      }
    }
  }
  if (processed)
    compound_bound(com, &com->nwcorner.x, &com->nwcorner.y,
			&com->secorner.x, &com->secorner.y);
  return processed;
}

static void 
do_update(Widget widget, XtPointer closure, XtPointer call_data)
{
  if (found_text_cnt > 0) {
    search_text_in_compound(&objects,
            panel_get_value(search_text_widget), update_text);
    found_text_panel_dismiss();
    redisplay_canvas();
    set_modifiedflag();
    show_search_msg("%d object%s updated", 
	found_text_cnt, (found_text_cnt != 1)? "s":"");
  }
}

static void 
found_text_panel_dismiss(void)
{
  if (found_text_panel != None) 
	XtDestroyWidget(found_text_panel);
  found_text_panel = None;

  XtSetSensitive(search_button, True);
}


static void 
show_search_result(char *format,...)
{
  va_list ap;
  XawTextBlock block;
  static char tmpstr[300];

  va_start(ap, format);
  vsprintf(tmpstr, format, ap );
  va_end(ap);

  strcat(tmpstr,"\n");
  /* append this message to the file message widget string */
  block.firstPos = 0;
  block.ptr = tmpstr;
  block.length = strlen(tmpstr);
  block.format = FMT8BIT;
  /* make editable to add new message */
  FirstArg(XtNeditType, XawtextEdit);
  SetValues(search_results_win);
  /* insert the new message after the end */
  (void) XawTextReplace(search_results_win, msg_length, msg_length, &block);
  (void) XawTextSetInsertionPoint(search_results_win, msg_length);

  /* make read-only again */
  FirstArg(XtNeditType, XawtextRead);
  SetValues(search_results_win);
  msg_length += block.length;
}

/* show a one-line message in the search message (label) widget */

static void 
show_search_msg(char *format,...)
{
  va_list ap;
  static char tmpstr[300];

  va_start(ap, format);
  vsprintf(tmpstr, format, ap );
  va_end(ap);

  FirstArg(XtNlabel, tmpstr);
  SetValues(search_msg_win);
}

static void 
show_text_object(F_text *t)
{
  float x, y;
  char *unit;
  if (appres.INCHES) {
    unit = "in";
    x = (float)t->base_x / (float)(PIX_PER_INCH);
    y = (float)t->base_y / (float)(PIX_PER_INCH);
  } else {
    unit = "cm";
    x = (float)t->base_x / (float)(PIX_PER_CM);
    y = (float)t->base_y / (float)(PIX_PER_CM);
  }
  show_search_result("[x=%4.1f%s y=%4.1f%s] %s", x, unit, y, unit, t->cstring);
  found_text_cnt++;
}

static void 
search_and_replace_text(Widget widget, XtPointer closure, XtPointer call_data)
{
  char	*string;

  show_search_msg("Searching text...");

  XtSetSensitive(do_replace_button, False);
  XtSetSensitive(do_update_button, False);

  /* clear any old search results first */
  found_text_cnt = 0;
  msg_length = 0;
  FirstArg(XtNstring, "\0");
  SetValues(search_results_win);
  do_replace_called = False;

  string = panel_get_value(search_text_widget);
  if (strlen(string)!=0)
    search_text_in_compound(&objects, string, show_text_object);

  if (found_text_cnt == 0) 
	show_search_msg("No match");
  else 
	show_search_msg("%d line%s match%s", found_text_cnt, 
		(found_text_cnt != 1)? "s":"",
		(found_text_cnt == 1)? "es":"");

  if (found_text_cnt > 0) {
    XtSetSensitive(replace_text_label, True);
    XtSetSensitive(do_replace_button, True);
    XtSetSensitive(do_update_button, True);
  }
}

static Boolean 
search_text_in_compound(F_compound *com, char *pattern, void (*proc) (/* ??? */))
{
  F_compound *c;
  F_text *t;
  Boolean match, processed;
  int pat_len, i;
  processed = False;
  for (c = com->compounds; c != NULL; c = c->next) {
    if (search_text_in_compound(c, pattern, proc)) 
	processed = True;
  }
  pat_len = strlen(pattern);
  for (t = com->texts; t != NULL; t = t->next) {
    match = False;
    if (pat_len == 0) {
      match = True;
    } else if (pat_len <= strlen(t->cstring)) {
      for (i = 0; !match && i <= strlen(t->cstring) - pat_len; i++) {
        if (compare_string(&t->cstring[i], pattern)) 
	    match = True;
      }
    }
    if (match) {
      (*proc)(t);
      if (proc != show_text_object) 
	  processed = True;
    }
  }
  if (processed)
    compound_bound(com, &com->nwcorner.x, &com->nwcorner.y,
			&com->secorner.x, &com->secorner.y);
  return processed;
}

static void 
search_panel_dismiss(Widget widget, XtPointer closure, XtPointer call_data)
{
  found_text_panel_dismiss();
  if (search_panel != None) 
	XtDestroyWidget(search_panel);
  search_panel = None;
}

/* create and popup the search panel */

void 
popup_search_panel(void)
{
    static Boolean actions_added = False;
    Widget below = None;
    Widget form, label, dismiss_button;
    int rx,ry;

    /* turn off Compose key LED */
    setCompLED(0);

    /* don't paste if in the middle of drawing/editing */
    if (check_action_on())
	return;

    /* don't make another one if one already exists */
    if (search_panel) {
	return;
    }

    put_msg("Search & Replace");

    get_pointer_root_xy(&rx, &ry);

    FirstArg(XtNx, (Position) rx);
    NextArg(XtNy, (Position) ry);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNtitle, "Xfig: Search & Replace");
    
    search_panel = XtCreatePopupShell("search_panel",
				transientShellWidgetClass, tool,
				Args, ArgCount);
    XtOverrideTranslations(search_panel,
		XtParseTranslationTable(search_panel_translations));
    if (!actions_added) {
	XtAppAddActions(tool_app, search_actions, XtNumber(search_actions));
	actions_added = True;
    }
    
    form = XtCreateManagedWidget("form", formWidgetClass, search_panel, NULL, 0) ;
    
    FirstArg(XtNlabel, "  Search for:");
    NextArg(XtNborderWidth, 0);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    label = XtCreateManagedWidget("search_lab", labelWidgetClass,
				form, Args, ArgCount);
    
    FirstArg(XtNfromHoriz, label);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 200);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    search_text_widget = XtCreateManagedWidget("search_text", asciiTextWidgetClass,
				form, Args, ArgCount);
    XtOverrideTranslations(search_text_widget,
			XtParseTranslationTable(search_text_translations));

    /* search button */

    FirstArg(XtNlabel, "Search ");
    NextArg(XtNfromHoriz, search_text_widget);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    search_button = XtCreateManagedWidget("search", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(search_button, XtNcallback,
                (XtCallbackProc) search_and_replace_text, (XtPointer) NULL);
  
    (void) CreateCheckbutton("Case sensitive", "case_sensitive", 
			form, NULL, search_button, MANAGE, SMALL_CHK, &case_sensitive, 0, 0);
    below = label;

    FirstArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 6);
    NextArg(XtNborderWidth, 0);
    NextArg(XtNlabel, "Replace with:");
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    replace_text_label = XtCreateManagedWidget("replace_lab", labelWidgetClass,
				form, Args, ArgCount);

    FirstArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 6);
    NextArg(XtNfromHoriz, replace_text_label);
    NextArg(XtNeditType, XawtextEdit);
    NextArg(XtNwidth, 200);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    replace_text_widget = XtCreateManagedWidget("replace_text", asciiTextWidgetClass,
				form, Args, ArgCount);
    XtOverrideTranslations(replace_text_widget,
				XtParseTranslationTable(replace_text_translations));
    
    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, replace_text_widget);
    NextArg(XtNlabel, "Replace");
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    do_replace_button = XtCreateManagedWidget("do_replace", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(do_replace_button, XtNcallback,
			(XtCallbackProc) do_replace, (XtPointer) NULL);

    FirstArg(XtNfromVert, below);
    NextArg(XtNfromHoriz, do_replace_button);
    NextArg(XtNlabel, "UPDATE  settings");
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    do_update_button = XtCreateManagedWidget("dismiss", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(do_update_button, XtNcallback,
			(XtCallbackProc) do_update, (XtPointer) NULL);

    below = replace_text_widget;

    /* make a label to report if no match for search */

    FirstArg(XtNlabel, "Enter search string and press \"Search\"");
    NextArg(XtNfromVert, below);
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNwidth, SEARCH_WIDTH);
    NextArg(XtNheight, 20);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    search_msg_win = XtCreateManagedWidget("search_msg_win", labelWidgetClass, 
		    form, Args, ArgCount);
    
    below = search_msg_win;

    /* make a text window to hold search results */

    FirstArg(XtNwidth, SEARCH_WIDTH);
    NextArg(XtNfromVert, below);
    NextArg(XtNheight, 200);
    NextArg(XtNeditType, XawtextRead);
    NextArg(XtNdisplayCaret, False);
    NextArg(XtNscrollHorizontal, XawtextScrollWhenNeeded);
    NextArg(XtNscrollVertical, XawtextScrollAlways);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    search_results_win = XtCreateManagedWidget("search_results_win", asciiTextWidgetClass,
			form, Args, ArgCount);
    XtOverrideTranslations(search_results_win,
				XtParseTranslationTable(search_results_translations));
    
    below = search_results_win;

    /* make a dismiss button */

    FirstArg(XtNfromVert, below);
    NextArg(XtNlabel, "Dismiss");
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    dismiss_button = XtCreateManagedWidget("dismiss", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(dismiss_button, XtNcallback,
			(XtCallbackProc) search_panel_dismiss, (XtPointer) NULL);

    /* make update/replace buttons insensitive to start */
    XtSetSensitive(replace_text_label, False);
    XtSetSensitive(do_replace_button, False);
    XtSetSensitive(do_update_button, False);

    XtPopup(search_panel, XtGrabNone);

    XSetWMProtocols(tool_d, XtWindow(search_panel), &wm_delete_window, 1);
    set_cmap(XtWindow(search_panel));
}

/***********************/
/* spell check section */
/***********************/

void 
popup_spell_check_panel(char **list, int nitems)
{
  static Boolean actions_added = False;
  Widget form, dismiss_button, below, label;
  int x_val, y_val;
  
  /* if panel already exists, just replace the list of words */
  if (spell_check_panel != None) {
    XawListChange(word_list, list, nitems, 0, False);
  } else {
    /* must create it */
    get_pointer_root_xy(&x_val, &y_val);

    FirstArg(XtNx, (Position) x_val);
    NextArg(XtNy, (Position) y_val);
    NextArg(XtNcolormap, tool_cm);
    NextArg(XtNtitle, "Xfig: Misspelled words");
    spell_check_panel = XtCreatePopupShell("spell_check_panel",
				transientShellWidgetClass,
				tool, Args, ArgCount);
    XtOverrideTranslations(spell_check_panel,
			XtParseTranslationTable(spell_panel_translations));
    if (!actions_added) {
	XtAppAddActions(tool_app, spell_actions, XtNumber(spell_actions));
	actions_added = True;
    }
    
    form = XtCreateManagedWidget("form", formWidgetClass,
				spell_check_panel, NULL, ZERO);

    /* make a label to report either "No misspelled words" or "Misspelled words:" */

    FirstArg(XtNlabel, "Spell checker");
    NextArg(XtNjustify, XtJustifyLeft);
    NextArg(XtNwidth, 375);
    NextArg(XtNheight, 20);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    spell_msg_win = XtCreateManagedWidget("spell_msg_win", labelWidgetClass, 
				form, Args, ArgCount);
    
    /* labels for list and correct word entry */
    FirstArg(XtNlabel, "Misspelled words    ");
    NextArg(XtNfromVert, spell_msg_win);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    label = XtCreateManagedWidget("misspelled_label", labelWidgetClass,
				form, Args, ArgCount);
    below = label;

    FirstArg(XtNlabel, "Correction");
    NextArg(XtNfromVert, spell_msg_win);
    NextArg(XtNfromHoriz, label);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    label = XtCreateManagedWidget("correction_label", labelWidgetClass,
				form, Args, ArgCount);

    /* make a viewport to hold the list widget containing the misspelled words */

    FirstArg(XtNallowVert, True);
    NextArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 1);
    NextArg(XtNwidth, 150);
    NextArg(XtNheight, 200);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainRight);
    spell_viewport = XtCreateManagedWidget("spellvport", viewportWidgetClass,
					 form, Args, ArgCount);

    /* now make the list widget */
    FirstArg(XtNlist, list);
    NextArg(XtNnumberStrings, nitems);
    NextArg(XtNforceColumns, True);		/* force to one column */
    NextArg(XtNdefaultColumns, 1);		/* ditto */
    NextArg(XtNwidth, 150);
    NextArg(XtNheight, 200);
    word_list = XtCreateManagedWidget("word_list", figListWidgetClass,
				     spell_viewport, Args, ArgCount);
    XtAddCallback(word_list, XtNcallback, spell_select_word, (XtPointer) NULL);

    /* now an ascii widget to put the correct word into */
    FirstArg(XtNeditType, XawtextRead);		/* make uneditable until user selects a misspelled word */
    NextArg(XtNsensitive, False);		/* start insensitive */
    NextArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 1);
    NextArg(XtNfromHoriz, spell_viewport);	/* to the right of the viewport */
    NextArg(XtNwidth, 150);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    correct_word = XtCreateManagedWidget("correct_word", asciiTextWidgetClass,
				     form, Args, ArgCount);

    /* "Return" corrects word */
    XtOverrideTranslations(correct_word,
		XtParseTranslationTable(spell_text_translations));

    /* focus keyboard on text widget */
    XtSetKeyboardFocus(form, correct_word);

    /* now "Correct" button to the right */
    FirstArg(XtNlabel, "Correct");
    NextArg(XtNsensitive, False);		/* start insensitive */
    NextArg(XtNfromVert, below);
    NextArg(XtNvertDistance, 1);
    NextArg(XtNfromHoriz, correct_word);
    NextArg(XtNtop, XtChainTop);
    NextArg(XtNbottom, XtChainTop);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    correct_button = XtCreateManagedWidget("correct", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(correct_button, XtNcallback,
		(XtCallbackProc) spell_correct_word, (XtPointer) NULL);

    /* make a re-check spelling button at bottom of whole panel */

    FirstArg(XtNlabel, "Recheck");
    NextArg(XtNfromVert, spell_viewport);
    NextArg(XtNsensitive, False);		/* insensitive to start */
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    recheck_button = XtCreateManagedWidget("recheck", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(recheck_button, XtNcallback,
		(XtCallbackProc) spell_check, (XtPointer) NULL);

    /* make dismiss button to the right of the recheck button */

    FirstArg(XtNlabel, "Dismiss");
    NextArg(XtNfromVert, spell_viewport);
    NextArg(XtNfromHoriz, recheck_button);
    NextArg(XtNtop, XtChainBottom);
    NextArg(XtNbottom, XtChainBottom);
    NextArg(XtNleft, XtChainLeft);
    NextArg(XtNright, XtChainLeft);
    dismiss_button = XtCreateManagedWidget("dismiss", commandWidgetClass,
				form, Args, ArgCount);
    XtAddCallback(dismiss_button, XtNcallback,
		(XtCallbackProc) spell_panel_dismiss, (XtPointer) NULL);

    /* install accelerators for the dismiss function */
    XtInstallAccelerators(form, dismiss_button);
    XtInstallAccelerators(word_list, dismiss_button);
  }

  XtPopup(spell_check_panel, XtGrabExclusive);
  /* if the file message window is up add it to the grab */
  file_msg_add_grab();
  XSetWMProtocols(tool_d, XtWindow(spell_check_panel), &wm_delete_window, 1);
  set_cmap(XtWindow(spell_check_panel));

}

static void 
spell_panel_dismiss(Widget widget, XtPointer closure, XtPointer call_data)
{
  if (spell_check_panel != None) 
	XtDestroyWidget(spell_check_panel);
  spell_check_panel = None;
}

static void write_text_from_compound(FILE *fp, F_compound *com);

void 
spell_check(void)
{
  char	  filename[PATH_MAX];
  char	 *cmd;
  char	  str[300];
  FILE	 *fp;
  int	  len, i;
  Boolean done = FALSE;
  static int lines = 0;

  /* turn off Compose key LED */
  setCompLED(0);

  put_msg("Spell checking...");

  /* free any strings from the previous spelling */
  for (i=0; i<lines; i++) {
    free(miss_word_list[i]);
    miss_word_list[i] = 0;
  }
  lines = 0;

  sprintf(filename, "%s/xfig-spell.%d", TMPDIR, (int)getpid());
  fp = fopen(filename, "w");
  if (fp == NULL) {
    file_msg("Can't open temporary file: %s: %s\n", filename, strerror(errno));
  } else {
    /* locate all text objects and write them to file fp */
    write_text_from_compound(fp, &objects);
    fclose(fp);

    /* replace the %f in the spellcheckcommand with the filename to check */
    cmd = build_command(appres.spellcheckcommand, filename);
    /* "spell %f", "ispell -l < %f | sort -u" or equivalent */
    fp = popen(cmd, "r");
    if (fp != NULL) {
      while (fgets(str, sizeof(str), fp) != NULL) {
        len = strlen(str);
        if (str[len - 1] == '\n') 
	    str[len - 1] = '\0';
	/* save the word in the list */
        miss_word_list[lines] = my_strdup(str);
        lines++;
	if (lines >= MAX_MISSPELLED_WORDS)
	    break;
      }
      if (pclose(fp) == 0) 
	  done = TRUE;
    }
    unlink(filename);

    /* put up the panel to show the results */
    popup_spell_check_panel(miss_word_list, lines);

    if (!done) 
	show_spell_msg("Can't exec \"%s\": %s", cmd, strerror(errno));
    else if (lines == 0) 
	show_spell_msg("No misspelled words found");
    else if (lines >= MAX_MISSPELLED_WORDS)
	show_spell_msg("%d (limit) misspelled words found. There may be more.",
				lines);
    else
	show_spell_msg("%d misspelled words found", lines);

    /* free command string allocated by build_command() */
    free(cmd);
  }

  if (!done) {
	show_spell_msg("Spell check: Internal error");
	beep();
  }
  /* make recheck button sensitive */
  XtSetSensitive(recheck_button, True);
}

/* locate all text objects and write them to file fp */

static void 
write_text_from_compound(FILE *fp, F_compound *com)
{
    F_compound *c;
    F_text *t;
    for (c = com->compounds; c != NULL; c = c->next) {
	write_text_from_compound(fp, c);
    }
    for (t = com->texts; t != NULL; t = t->next) {
	fprintf(fp, "%s\n", t->cstring);
    }
}

/* user has selected a word from the list */

static void
spell_select_word(Widget widget, XtPointer closure, XtPointer call_data)
{
    XawListReturnStruct *ret_struct = (XawListReturnStruct *) call_data;

    /* make correct button and correction entry sensitive */
    XtSetSensitive(correct_button, True);
    XtSetSensitive(correct_word, True);

    /* save the selected word */
    strcpy(selected_word, ret_struct->string);
    /* copy the word to the correct_word ascii widget */
    FirstArg(XtNstring, ret_struct->string);
    NextArg(XtNeditType, XawtextEdit);		/* make editable now */
    NextArg(XtNsensitive, True);		/* and sensitive */
    SetValues(correct_word);
}

/* correct word that user has selected */

static void
spell_correct_word(Widget widget, XtPointer closure, XtPointer call_data)
{
    char	   *corrected_word;

    /* get the correct word from the ascii widget */
    FirstArg(XtNstring, &corrected_word);
    GetValues(correct_word);
    replace_text_in_compound(&objects, selected_word, corrected_word);
    redisplay_canvas();
}

/* show a one-line message in the spelling message (label) widget */

static void 
show_spell_msg(char *format,...)
{
  va_list ap;
  static char tmpstr[300];

  va_start(ap, format);
  vsprintf(tmpstr, format, ap );
  va_end(ap);

  FirstArg(XtNlabel, tmpstr);
  SetValues(spell_msg_win);
}

