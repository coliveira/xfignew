/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2004 by Chris Moller
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

#include <sys/types.h>
#include <regex.h>
#include <alloca.h>
#include <string.h>
#include <math.h>

#include "fig.h"
#include "figx.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "w_canvas.h"
#include "w_setup.h"
#include "w_indpanel.h"
#include "w_util.h"
#include "w_keyboard.h"

#if defined(__CYGWIN__)
#define REG_NOERROR REG_OKAY
#endif

Boolean keyboard_input_available = False;
int keyboard_x;
int keyboard_y;
int keyboard_state;

/* popups for keyboard input */

typedef struct _keyboard_history_s {
  char * str;
  struct _keyboard_history_s * next;
  struct _keyboard_history_s * prior;
} keyboard_history_s;

static keyboard_history_s * keyboard_history = NULL;
static keyboard_history_s * keyboard_history_read_pointer;
static keyboard_history_s * keyboard_history_write_pointer;
static int keyboard_history_nr_etys;
#define KEYBOARD_HISTORY_MAX	32

void
next_keyboard_history(w, event)
     Widget w;
     XKeyEvent *event;
{
  if (keyboard_history) {
    char * str;
    keyboard_history_read_pointer = keyboard_history_read_pointer->next;
    str = keyboard_history_read_pointer->str;
    XtVaSetValues(w, XtNstring, str, XtNinsertPosition, strlen(str) , NULL);
  }
}

void
prior_keyboard_history(w, event)
     Widget w;
     XKeyEvent *event;
{
  if (keyboard_history) {
    char * str;
    keyboard_history_read_pointer = keyboard_history_read_pointer->prior;
    str = keyboard_history_read_pointer->str;
    XtVaSetValues(w, XtNstring, str, XtNinsertPosition, strlen(str) , NULL);
  }
     
}

void
ignore_keyboard_input(w, event)
     Widget w;
     XKeyEvent *event;
{
  popdown_keyboard_panel();
}

void
handle_keyboard_input(w, event)
     Widget w;
     XKeyEvent *event;
{

#define WS "[[:space:]]*"  

#define RELABS WS "([rRaA])?" WS

#define SLASH_DELIM WS "/" WS

  /* the comma is optional */
#define COMMA_DELIM  WS ",?" WS

#define ANGLE_DELIM WS "<" WS

#define FP_NR_TYPE1 "([+-]?[[:digit:]]+(\\.[[:digit:]]+)?)"
#define FP_NR_TYPE2 "([+-]?\\.[[:digit:]]+)"
#define FP_VALUE "(" FP_NR_TYPE1 "|" FP_NR_TYPE2 ")"

#define ANGLE  FP_VALUE "([rRdDpP]?)"

#define UNSIGNED_INT_VALUE "([[:digit:]]+)"
#define SIGNED_INT_VALUE "([+-]?[[:digit:]]+)"

#define FRACTION UNSIGNED_INT_VALUE SLASH_DELIM UNSIGNED_INT_VALUE

#define INT_VALUE "(" SIGNED_INT_VALUE "(-" FRACTION "))"

#define MAG "(" INT_VALUE "|" FP_VALUE ")"

#define POLAR_COORD "(" WS MAG ANGLE_DELIM ANGLE ")"

#define RECT_COORD "(" RELABS MAG COMMA_DELIM RELABS MAG ")"

#define COORD   POLAR_COORD "|" RECT_COORD
  
  char * coord = {COORD};
  regex_t preg;
  regmatch_t * pmatch = NULL;
  int rc;
  int origin_x, origin_y;

  DeclareArgs(2);
  char *val;
  double pix_per_unit = (double)(appres.INCHES? PIX_PER_INCH : PIX_PER_CM);
  
  if (NULL == pmatch) {
#if 1  
    regcomp(&preg, coord, REG_EXTENDED);
#else  
    fprintf(stderr, "coord = \"%s\"\n\n", coord);
    if (REG_NOERROR != (rc = regcomp(&preg, coord, REG_EXTENDED))) {
#define ERRBUF_SIZE 256    
      char * errbuf = alloca(ERRBUF_SIZE);
      regerror(rc, &preg, errbuf, ERRBUF_SIZE);
      fprintf(stderr, "regcomp error: %s\n", errbuf);
    }
#endif

    pmatch = malloc((1 + preg.re_nsub) * sizeof(regmatch_t));
  }

#define M_POLAR_COORD			 1
#define M_POLAR_MAG			 2
#define M_POLAR_MAG_I_FORM		 3
#define M_POLAR_MAG_I_FORM_INT		 4
#define M_POLAR_MAG_I_FORM_FRAC		 5
#define M_POLAR_MAG_I_FORM_FRAC_N	 6    
#define M_POLAR_MAG_I_FORM_FRAC_D	 7
#define M_POLAR_MAG_F			 8    
#define M_POLAR_PHASE			12
#define M_POLAR_PHASE_UNITS		16
    
#define M_RECT_COORD			17
#define M_RECT_COORD_X_RA		18
#define M_RECT_COORD_X			19
#define M_RECT_COORD_X_I_FORM		20
#define M_RECT_COORD_X_I_FORM_INT	21
#define M_RECT_COORD_X_I_FORM_FRAC	22
#define M_RECT_COORD_X_I_FORM_FRAC_N	23
#define M_RECT_COORD_X_I_FORM_FRAC_D	24
#define M_RECT_COORD_X_F		25
  
#define M_RECT_COORD_Y_RA		29
#define M_RECT_COORD_Y			30
#define M_RECT_COORD_Y_I_FORM		31
#define M_RECT_COORD_Y_I_FORM_INT	32
#define M_RECT_COORD_Y_I_FORM_FRAC	33
#define M_RECT_COORD_Y_I_FORM_FRAC_N	34
#define M_RECT_COORD_Y_I_FORM_FRAC_D	35
#define M_RECT_COORD_Y_F		36

#define IS_POLAR			(-1 != pmatch[M_POLAR_MAG].rm_so)
#define IS_RECT				(-1 != pmatch[M_RECT_COORD].rm_so)
  
#define IS_POLAR_MAG_I_FORM		(-1 != pmatch[M_POLAR_MAG_I_FORM].rm_so)
#define HAS_POLAR_MAG_I_FORM_FRAC	(-1 != pmatch[M_POLAR_MAG_I_FORM].rm_so)
#define POLAR_MAG_I_FORM_INT		(&val[pmatch[M_POLAR_MAG_I_FORM_INT].rm_so])
#define POLAR_MAG_I_FORM_FRAC_N		(&val[pmatch[M_POLAR_MAG_I_FORM_FRAC_N].rm_so])
#define POLAR_MAG_I_FORM_FRAC_D		(&val[pmatch[M_POLAR_MAG_I_FORM_FRAC_D].rm_so])
#define POLAR_MAG_F			(&val[pmatch[M_POLAR_MAG_F].rm_so])
#define POLAR_PHASE			(&val[pmatch[M_POLAR_PHASE].rm_so])
#define HAS_POLAR_PHASE_UNITS		(-1 != pmatch[M_POLAR_PHASE_UNITS].rm_so)
#define POLAR_PHASE_UNITS		(val[pmatch[M_POLAR_PHASE_UNITS].rm_so])
  
#define HAS_RECT_X_RA			(-1 != pmatch[M_RECT_COORD_X_RA].rm_so)
#define RECT_X_RA			(val[pmatch[M_RECT_COORD_X_RA].rm_so])
#define IS_RECT_X_I_FORM		(-1 != pmatch[M_RECT_COORD_X_I_FORM].rm_so)
#define HAS_RECT_X_I_FORM_FRAC		(-1 != pmatch[M_RECT_COORD_X_I_FORM_FRAC].rm_so)
#define RECT_X_I_FORM_INT		(&val[pmatch[M_RECT_COORD_X_I_FORM_INT].rm_so])
#define RECT_X_I_FORM_FRAC_N		(&val[pmatch[M_RECT_COORD_X_I_FORM_FRAC_N].rm_so])
#define RECT_X_I_FORM_FRAC_D		(&val[pmatch[M_RECT_COORD_X_I_FORM_FRAC_D].rm_so])
#define RECT_X_F			(&val[pmatch[M_RECT_COORD_X_F].rm_so])
  
#define HAS_RECT_Y_RA			(-1 != pmatch[M_RECT_COORD_Y_RA].rm_so)
#define RECT_Y_RA			(val[pmatch[M_RECT_COORD_Y_RA].rm_so])
#define IS_RECT_Y_I_FORM		(-1 != pmatch[M_RECT_COORD_Y_I_FORM].rm_so)
#define HAS_RECT_Y_I_FORM_FRAC		(-1 != pmatch[M_RECT_COORD_Y_I_FORM_FRAC].rm_so)
#define RECT_Y_I_FORM_INT		(&val[pmatch[M_RECT_COORD_Y_I_FORM_INT].rm_so])
#define RECT_Y_I_FORM_FRAC_N		(&val[pmatch[M_RECT_COORD_Y_I_FORM_FRAC_N].rm_so])
#define RECT_Y_I_FORM_FRAC_D		(&val[pmatch[M_RECT_COORD_Y_I_FORM_FRAC_D].rm_so])
#define RECT_Y_F			(&val[pmatch[M_RECT_COORD_Y_F].rm_so])
  
    
  FirstArg(XtNstring, &val);
  GetValues(w);

  if ((F_POLYLINE == cur_mode) ||
      (F_POLYGON  == cur_mode)) {
    if (cur_point) {
      origin_x = cur_point->x;
      origin_y = cur_point->y;
    }
    else {
      origin_x = -1;
      origin_y = -1;
    }
  }
  else {
    origin_x = fix_x;
    origin_y = fix_y;
  }
  
  if (REG_NOERROR == (rc = regexec(&preg, val, preg.re_nsub, pmatch, 0))) {
#if 0    
    int i;
    for (i = 0; i < 1 + preg.re_nsub; i++) {
      if (-1 != pmatch[i].rm_so) {
	fprintf(stderr, "match %2d: \"%.*s\"\n",
		i,
		pmatch[i].rm_eo - pmatch[i].rm_so,
		&val[pmatch[i].rm_so]);
      }
    }
#endif    

    if (IS_POLAR) {
      if ((0 <= origin_x) && (0 <= origin_y)) {
	double mag;
	double pha;
	double pha_conversion;
	if (IS_POLAR_MAG_I_FORM) {
	  mag = strtod(POLAR_MAG_I_FORM_INT, NULL);
	  if (HAS_POLAR_MAG_I_FORM_FRAC)
	    mag +=
	      strtod(POLAR_MAG_I_FORM_FRAC_N, NULL)/
	      strtod(POLAR_MAG_I_FORM_FRAC_D, NULL);
	}
	else mag = strtod(POLAR_MAG_F, NULL);
	pha = strtod(POLAR_PHASE, NULL);
	pha_conversion = M_PI/180.0;
	if (HAS_POLAR_PHASE_UNITS) {
	  switch(POLAR_PHASE_UNITS) {
	  case 'd':
	  case 'D':		/* degrees */
	    pha_conversion = M_PI/180.0;
	    break;
	  case 'r':
	  case 'R':		/* radians */
	    pha_conversion = 1.0;
	    break;
	  case 'p':
	  case 'P':		/* pi radians */
	    pha_conversion = M_PI;
	    break;
	  }
	}
	pha *= pha_conversion;

	keyboard_x =  origin_x + (int)lrint(pix_per_unit * mag * cos(pha));
	keyboard_y =  origin_y + (int)lrint(pix_per_unit * mag * sin(pha));
	keyboard_state = event->state;
	keyboard_input_available = True;
      }
      else {
	put_msg("Polar coordinates must be relative to a current point.");
	beep();
      }
    }
    else if (IS_RECT) {
      double xv, yv;
      Boolean x_absolute = True;
      Boolean y_absolute = True;
      
      if (HAS_RECT_X_RA) {
	switch(RECT_X_RA) {
	case 'r':
	case 'R':
	  x_absolute = False;
	  break;
	case 'a':
	case 'A':
	  x_absolute = True;
	  break;
	}
      }
      
      if (HAS_RECT_Y_RA) {
	switch(RECT_Y_RA) {
	case 'r':
	case 'R':
	  y_absolute = False;
	  break;
	case 'a':
	case 'A':
	  y_absolute = True;
	  break;
	}
      }
      else {	/* if x_abs has been set and y_abs is default, make y follow x */
	if (HAS_RECT_X_RA) y_absolute = x_absolute;
      }
      
      if (((0 > origin_x) || (0 > origin_y)) &&
	  ((False == x_absolute) || (False == y_absolute))) {
	put_msg("Relative coordinates require a current point.");
	beep();
      }
      else {
	if (IS_RECT_X_I_FORM) {
	  xv = strtod(RECT_X_I_FORM_INT, NULL);
	  if (HAS_RECT_X_I_FORM_FRAC)
	    xv +=
	      strtod(RECT_X_I_FORM_FRAC_N, NULL)/
	      strtod(RECT_X_I_FORM_FRAC_D, NULL);
	}
	else xv = strtod(RECT_X_F, NULL);
	
	if (IS_RECT_Y_I_FORM) {
	  yv = strtod(RECT_Y_I_FORM_INT, NULL);
	  if (HAS_RECT_Y_I_FORM_FRAC)
	    yv +=
	      strtod(RECT_Y_I_FORM_FRAC_N, NULL)/
	      strtod(RECT_Y_I_FORM_FRAC_D, NULL);
	}
	else yv = strtod(RECT_Y_F, NULL);

	keyboard_x = (int)lrint(pix_per_unit * xv);
	keyboard_y = (int)lrint(pix_per_unit * yv);
	if (False == x_absolute) keyboard_x += origin_x;
	if (False == y_absolute) keyboard_y += origin_y;
	keyboard_state = event->state;
	keyboard_input_available = True;
      }
    }
    else {
      put_msg("Keyboard input screw-up.  Flame Chris Moller <moller@mollerware.com>");
      beep();
    }

    if (True == keyboard_input_available) {
      if (NULL == keyboard_history) {
	keyboard_history = malloc(KEYBOARD_HISTORY_MAX * sizeof(keyboard_history_s));
	bzero(keyboard_history, KEYBOARD_HISTORY_MAX * sizeof(keyboard_history_s));
	keyboard_history_nr_etys = 1;
	keyboard_history_read_pointer = keyboard_history_write_pointer = &keyboard_history[0];
	keyboard_history[0].next  = keyboard_history_read_pointer;
	keyboard_history[0].prior = keyboard_history_read_pointer;
	keyboard_history[0].str = strdup(val);
      }
      else {
	if (strcmp(keyboard_history_read_pointer->str, val)) {		/* don't duplicate cur ety */
	  if (KEYBOARD_HISTORY_MAX <= keyboard_history_nr_etys) { /* start recycling */
	    keyboard_history_read_pointer
	      = keyboard_history_write_pointer
	      = keyboard_history_write_pointer->next;
	  }
	  else {	/* new entry */
	    keyboard_history[0].prior = 
	      keyboard_history_write_pointer->next =
	      keyboard_history_read_pointer =
	      keyboard_history_write_pointer =
	      &keyboard_history[keyboard_history_nr_etys];
	    keyboard_history_write_pointer->next  = &keyboard_history[0];
	    keyboard_history_write_pointer->prior = &keyboard_history[keyboard_history_nr_etys - 1];
	    keyboard_history_nr_etys++;
	  }
	  if (NULL != keyboard_history_write_pointer->str) free(keyboard_history_write_pointer->str);
	  keyboard_history_write_pointer->str = strdup(val);
	}
      }
      
    }
    
  }
  
  popdown_keyboard_panel();
}

static
XtActionsRec keyboard_input_actions[] =
{
    {"HandleKeyboardInput",  (XtActionProc) handle_keyboard_input} ,
    {"IgnoreKeyboardInput",  (XtActionProc) ignore_keyboard_input} ,
    {"NextKeyboardHistory",  (XtActionProc) next_keyboard_history} ,
    {"PriorKeyboardHistory", (XtActionProc) prior_keyboard_history} ,
};

static Widget keyboard_panel = None;
static Widget active_keyboard_panel = None;
static Widget keyboard_input = None;

String  keyboard_translations =
        "<Key>Return: HandleKeyboardInput()\n\
        <Key>Escape:  IgnoreKeyboardInput()\n\
        Ctrl<Key>n:   NextKeyboardHistory()\n\
        <Key>Down:    NextKeyboardHistory()\n\
        Ctrl<Key>p:   PriorKeyboardHistory()\n\
        <Key>Up:      PriorKeyboardHistory()\n";

static void
create_keyboard_panel()
{
  Widget keyboard_form, entry;
  Widget label, usage_hint;
  icon_struct *icon;
  Widget up = None, left = None;
  int max_wd = 150, wd = 0;
  int i;
  DeclareArgs(10);
  char * str;

  keyboard_panel = XtVaCreatePopupShell("keyboard_menu", transientShellWidgetClass, tool,
					XtNtitle, "Keyboard Input", NULL);
  keyboard_form = XtVaCreateManagedWidget("form", formWidgetClass, keyboard_panel,
					  XtNdefaultDistance, 0, NULL);

  FirstArg(XtNlabel, "Shift => Button2; Control => Button3");
  NextArg(XtNjustify, XtJustifyLeft);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  usage_hint = XtCreateManagedWidget("label", labelWidgetClass,
				     keyboard_form, Args, ArgCount);

  FirstArg(XtNlabel, "Coordinate:");
  NextArg(XtNjustify, XtJustifyLeft);
  NextArg(XtNborderWidth, 0);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  NextArg(XtNfromVert, usage_hint);
  label = XtCreateManagedWidget("label", labelWidgetClass,
				keyboard_form, Args, ArgCount);

  str = malloc(80);
  str[0] = 0;
  FirstArg(XtNstring, str);
  NextArg(XtNinsertPosition, strlen(str));
  NextArg(XtNeditType, XawtextEdit);
  NextArg(XtNfromHoriz, label);
  NextArg(XtNfromVert, usage_hint);
  NextArg(XtNwidth, 320);
  NextArg(XtNtop, XtChainTop);
  NextArg(XtNbottom, XtChainTop);
  NextArg(XtNleft, XtChainLeft);
  NextArg(XtNright, XtChainLeft);
  keyboard_input = XtCreateManagedWidget("keyboard_input", asciiTextWidgetClass,
					 keyboard_form, Args, ArgCount);
  XtSetKeyboardFocus(keyboard_form, keyboard_input);
  XtAppAddActions(tool_app, keyboard_input_actions, XtNumber(keyboard_input_actions));
  XtOverrideTranslations(keyboard_input, XtParseTranslationTable(keyboard_translations));

}

void
popup_keyboard_panel(Widget widget,
		     XButtonEvent * event,
		     String * params,
		     Cardinal * num_params)
{
  Dimension wd, ht;

  /* make sure we're in a drawing mode first */
  if (cur_mode >= FIRST_EDIT_MODE || cur_mode == F_PLACE_LIB_OBJ)
  	return;

  if (keyboard_panel == None) {
    create_keyboard_panel();
    XtRealizeWidget(keyboard_panel);
    XSetWMProtocols(tool_d, XtWindow(keyboard_panel), &wm_delete_window, 1);
  }
  XtVaGetValues( keyboard_panel, XtNwidth, &wd, XtNheight, &ht, NULL);
  XtVaSetValues( keyboard_panel, XtNx, event->x_root - wd / 2,
		XtNy, event->y_root - ht * 2 / 3, NULL);
  XtPopup( keyboard_panel, XtGrabNone);
  active_keyboard_panel =  keyboard_panel;
}

void
popdown_keyboard_panel()
{
  if (active_keyboard_panel != None) {
    XtPopdown(active_keyboard_panel);
  }
  active_keyboard_panel = None;
}
