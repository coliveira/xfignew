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
extern "C" {
#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "paintop.h"
#include "f_util.h"
#include "d_text.h"
#include "u_create.h"
#include "u_fonts.h"
#include "u_list.h"
#include "u_search.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_drawprim.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_zoom.h"

#include "u_draw.h"
#include "u_markers.h"
#include "u_redraw.h"
#include "w_cmdpanel.h"
#include "w_cursor.h"
}

#include <sys/wait.h>  /* waitpid() */

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#ifdef SEL_TEXT
Boolean	text_selection_active;
#endif /* SEL_TEXT */

/* EXPORTS */
int	work_font;

/* LOCALS */

#define CTRL_A	'\001'		/* move to beginning of text */
#define CTRL_B	'\002'		/* move back one char */
#define CTRL_D	'\004'		/* delete right of cursor */
#define CTRL_E	'\005'		/* move to end of text */
#define CTRL_F	'\006'		/* move forward one char */
#define CTRL_H	'\010'		/* backspace */
#define CTRL_K	'\013'		/* kill to end of text */
#ifdef SEL_TEXT
#define CTRL_W	'\027'		/* delete selected text */
#endif /* SEL_TEXT */
#define CTRL_HAT  '\036'	/* start superscript or end subscript */
#define CTRL_UNDERSCORE  '\037'	/* start subscript or end superscript */

#define	MAX_SUPSUB 4			/* max number of nested super/subscripts */
#define CSUB_FRAC 0.75			/* fraction of current char size for super/subscript */
#define PSUB_FRAC (1.0-CSUB_FRAC)	/* amount of up/down shift */

#define NL	'\n'
#define ESC	'\033'
#define CR	'\r'
#define CTRL_X	24
#define SP	' '
#define DEL	127

#define		BUF_SIZE	400

char		prefix[BUF_SIZE],	/* part of string left of mouse click */
		suffix[BUF_SIZE];	/* part to right of click */
int		leng_prefix, leng_suffix;
static int	char_ht;
static int	base_x, base_y;
static int	supersub;		/* < 0 = currently subscripted, > 0 = superscripted */
static int	heights[MAX_SUPSUB];	/* keep prev char heights when super/subscripting */
static int	orig_x, orig_y;		/* to position next line */
static int	orig_ht;		/* to advance to next line */
static float	work_float_fontsize;	/* keep current font size in floating for roundoff */
static XFontStruct *canvas_zoomed_font;

static Boolean	is_newline;
static int	work_fontsize, work_flags,
		work_psflag, work_textjust, work_depth;
static Color	work_textcolor;
static XFontStruct *work_fontstruct;
static float	work_angle;		/* in RADIANS */
static double	sin_t, cos_t;		/* sin(work_angle) and cos(work_angle) */
static void	finish_n_start(int x, int y);
static void	init_text_input(int x, int y), cancel_text_input(void);
static F_text  *new_text(void);

static void	new_text_line(void);
static void     overlay_text_input(int x, int y);
static void	create_textobject(void);
static void	draw_cursor(int x, int y);
static void	move_cur(int dir, unsigned char c, float div);
static void	move_text(int dir, unsigned char c, float div);
static void	reload_compoundfont(F_compound *compounds);
static int	prefix_length(char *string, int where_p);
static void	initialize_char_handler(Window w, int (*cr) (/* ??? */), int bx, int by);
static void	terminate_char_handler(void);
static void	turn_on_blinking_cursor(void (*draw_cursor) (int,int), void (*erase_cursor) (int,int), int x, int y, long unsigned int msec);
static void	turn_off_blinking_cursor(void);
static void	move_blinking_cursor(int x, int y);

#ifdef SEL_TEXT
/* for text selection */
static void	track_text_select();
static Boolean	text_selection_showing = False;
static int	startp, endp;
static int 	prev_indx, lensel = 0;
static int	start_text_select = -1;
static int	start_sel_x, start_sel_y;
static Boolean	click_on_text = False;
static char	text_selection[500] = {'\0'};
static Boolean	selection_dir = 0;
#endif /* SEL_TEXT */

#ifdef I18N
#include <sys/wait.h>

XIM		xim_im = NULL;
XIC		xim_ic = NULL;
XIMStyle	xim_style = 0;
Boolean		xim_active = False;

static int	save_base_x, save_base_y;

/*
In EUC encoding, a character can 1 to 3 bytes long.
Although fig2dev-i18n will support only G0 and G1,
xfig-i18n will prepare for G2 and G3 here, too.
  G0: 0xxxxxxx                   (ASCII, for example)
  G1: 1xxxxxxx 1xxxxxxx          (JIS X 0208, for example)
  G2: 10001110 1xxxxxxx          (JIS X 0201, for example)
  G3: 10001111 1xxxxxxx 1xxxxxxx (JIS X 0212, for example)
*/
#define is_euc_multibyte(ch)  ((unsigned char)(ch) & 0x80)
#define EUC_SS3 '\217'  /* single shift 3 */

static int	i18n_prefix_tail(), i18n_suffix_head();

#ifdef I18N_USE_PREEDIT
static pid_t	preedit_pid = -1;
static char	preedit_filename[PATH_MAX] = "";
static 		open_preedit_proc(), close_preedit_proc(), paste_preedit_proc();
static Boolean	is_preedit_running();
#endif  /* I18N_USE_PREEDIT */
#endif  /* I18N */

/********************************************************/
/*							*/
/*			Procedures			*/
/*							*/
/********************************************************/


void move_pref_to_suf (void);
void move_suf_to_pref (void);

void
text_drawing_selected(void)
{
    canvas_kbd_proc = (FCallBack)null_proc;
    canvas_locmove_proc = null_proc;
    canvas_middlebut_proc = (FCallBack)null_proc;
    canvas_leftbut_proc = (FCallBack)init_text_input;
    canvas_rightbut_proc = (FCallBack)null_proc;
    set_mousefun("position cursor", "", "", "", "", "");
#ifdef I18N
#ifdef I18N_USE_PREEDIT
    if (appres.international && strlen(appres.text_preedit) != 0) {
      if (is_preedit_running()) {
	canvas_middlebut_proc = paste_preedit_proc;
	canvas_rightbut_proc = close_preedit_proc;
	set_mousefun("position cursor", "paste pre-edit", "close pre-edit", "", "", "");
      } else {
	canvas_rightbut_proc = open_preedit_proc;
	set_mousefun("position cursor", "", "open pre-edit", "", "", "");
      }
    }
#endif  /* I18N_USE_PREEDIT */
#endif  /* I18N */
    reset_action_on();
    clear_mousefun_kbd();
    set_cursor(text_cursor);
    is_newline = False;
}

static void
finish_n_start(int x, int y)
{
    create_textobject();
    /* reset text size after any super/subscripting */
    work_fontsize = cur_fontsize;
    work_float_fontsize = (float) work_fontsize;
    supersub = 0;
    is_newline = False;
    init_text_input(x, y);
}

void
finish_text_input(int x, int y, int shift)
{
    if (shift) {
	paste_primary_selection();
	return;
    }
    create_textobject();
    text_drawing_selected();
    /* reset text size after any super/subscripting */
    work_fontsize = cur_fontsize;
    work_float_fontsize = (float) work_fontsize;
    /* reset super/subscript */
    supersub = 0;
    draw_mousefun_canvas();
}

static void
cancel_text_input(void)
{
    /* reset text size after any super/subscripting */
    work_fontsize = cur_fontsize;
    work_float_fontsize = (float) work_fontsize;
    /* reset super/subscript */
    supersub = 0;
    erase_char_string();
    terminate_char_handler();
    reset_action_on();
    if (cur_t != NULL) {
	/* draw it and any objects that are on top */
	redisplay_text(cur_t);
    }
    text_drawing_selected();
    draw_mousefun_canvas();
}

static void
new_text_line(void)
{
    /* finish current text */
    create_textobject();
    /* restore x,y to point where user clicked first text or start
       of text if clicked on existing text */
    cur_x = orig_x;
    cur_y = orig_y;
    /* restore orig char height */
    char_ht = orig_ht;

    /* advance to next line */
    cur_x = round(cur_x + char_ht*cur_textstep*sin_t);
    cur_y = round(cur_y + char_ht*cur_textstep*cos_t);

    is_newline = True;
    init_text_input(cur_x, cur_y);
}

static void
new_text_down(void)
{
    /* only so deep */
    if (supersub <= -MAX_SUPSUB)
	return;

    create_textobject();
    /* save current char height */
    heights[abs(supersub)] = char_ht;
    if (supersub-- > 0) {
	/* we were previously in a superscript, go back one */
	cur_x = round(cur_x + heights[supersub]*sin_t*PSUB_FRAC);
	cur_y = round(cur_y + heights[supersub]*cos_t*PSUB_FRAC);
	work_float_fontsize /= CSUB_FRAC;
    } else if (supersub < 0) {
	/* we were previously in a subscript, go deeper */
	cur_x = round(cur_x + char_ht*sin_t*PSUB_FRAC);
	cur_y = round(cur_y + char_ht*cos_t*PSUB_FRAC);
	work_float_fontsize *= CSUB_FRAC;
    }
    work_fontsize = round(work_float_fontsize);
    is_newline = False;
    overlay_text_input(cur_x, cur_y);
}

static void
new_text_up(void)
{
    /* only so deep */
    if (supersub >= MAX_SUPSUB)
	return;

    create_textobject();
    /* save current char height */
    heights[abs(supersub)] = char_ht;
    if (supersub++ < 0) {
	/* we were previously in a subscript, go back one */
	cur_x = round(cur_x - heights[-supersub]*sin_t*PSUB_FRAC);
	cur_y = round(cur_y - heights[-supersub]*cos_t*PSUB_FRAC);
	work_float_fontsize /= CSUB_FRAC;
    } else if (supersub > 0) {
	/* we were previously in a superscript, go deeper */
	cur_x = round(cur_x - char_ht*sin_t*PSUB_FRAC);
	cur_y = round(cur_y - char_ht*cos_t*PSUB_FRAC);
	work_float_fontsize *= CSUB_FRAC;
    }
    work_fontsize = round(work_float_fontsize);
    is_newline = False;
    overlay_text_input(cur_x, cur_y);
}



/* Version of init_text_input that allows overlaying.
 * Does not test for other text nearby.
 */

static void
overlay_text_input(int x, int y)
{
    cur_x = x;
    cur_y = y;

    set_action_on();
    set_mousefun("new text", "finish text", "cancel", "", "paste text", "");
    draw_mousefun_kbd();
    draw_mousefun_canvas();
    canvas_kbd_proc = (FCallBack)char_handler;
    canvas_middlebut_proc = finish_text_input;
    canvas_leftbut_proc = (FCallBack)finish_n_start;
    canvas_rightbut_proc = (FCallBack)cancel_text_input;

  /*
   * set working font info to current settings. This allows user to change
   * font settings while we are in the middle of accepting text without
   * affecting this text i.e. we don't allow the text to change midway
   * through
   */

    cur_t=NULL;
    leng_prefix = leng_suffix = 0;
    *suffix = 0;
    prefix[leng_prefix] = '\0';
    base_x = cur_x;
    base_y = cur_y;
#ifdef I18N
    save_base_x = base_x;
    save_base_y = base_y;
#endif /* I18N */

    if (is_newline) {	/* working settings already set */
	is_newline = False;
    } else {		/* set working settings from ind panel */
	work_textcolor = cur_pencolor;
	work_font     = using_ps ? cur_ps_font : cur_latex_font;
	work_psflag   = using_ps;
	work_flags    = cur_textflags;
	work_textjust = cur_textjust;
	work_depth    = cur_depth;
	work_angle    = cur_elltextangle*M_PI/180.0;
	while (work_angle < 0.0)
	    work_angle += M_2PI;
	sin_t = sin((double)work_angle);
	cos_t = cos((double)work_angle);

	/* load the X font and get its id for this font and size UNZOOMED */
	/* this is to get widths etc for the unzoomed chars */
	canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize);
	/* get the ZOOMED font for actually drawing on the canvas */
	canvas_zoomed_font = lookfont(x_fontnum(work_psflag, work_font), 
				  round(work_fontsize*display_zoomscale));
	/* save the working font structure */
	work_fontstruct = canvas_zoomed_font;
    }

    put_msg("Ready for text input (from keyboard)");
    char_ht = ZOOM_FACTOR * max_char_height(canvas_font);
    initialize_char_handler(canvas_win, (IntFunc)finish_text_input,
			  base_x, base_y);
    draw_char_string();
}

static void
create_textobject(void)
{
    PR_SIZE	    size;

    reset_action_on();
    erase_char_string();
    terminate_char_handler();

    if (cur_t == NULL) {	/* a brand new text */
	strcat(prefix, suffix);	/* re-attach any suffix */
	leng_prefix=strlen(prefix);
	if (leng_prefix == 0)
	    return;		/* nothing afterall */
	cur_t = new_text();
	add_text(cur_t);
    } else {			/* existing text modified */
	strcat(prefix, suffix);
	leng_prefix += leng_suffix;
	if (leng_prefix == 0) {
	    delete_text(cur_t);
	    return;
	}
	if (!strcmp(cur_t->cstring, prefix)) {
	    /* we didn't change anything */
	    /* draw it and any objects that are on top */
	    redisplay_text(cur_t);
#ifdef SEL_TEXT
	    /* if any text is selected, redraw those characters inverted */
	    if (lensel)
		draw_selection(start_sel_x, start_sel_y, text_selection);
	    return;
#endif /* SEL_TEXT */
	}
	new_t = copy_text(cur_t);
	change_text(cur_t, new_t);
	if (strlen(new_t->cstring) >= leng_prefix) {
	    strcpy(new_t->cstring, prefix);
	} else {		/* free old and allocate new */
	    free(new_t->cstring);
	    if ((new_t->cstring = new_string(leng_prefix)) != NULL)
		strcpy(new_t->cstring, prefix);
	}
	size = textsize(canvas_font, leng_prefix, prefix);
	new_t->ascent  = size.ascent;
	new_t->descent = size.descent;
	new_t->length  = size.length;
	cur_t = new_t;
    }
    /* draw it and any objects that are on top */
    redisplay_text(cur_t);
}

static void
init_text_input(int x, int y)
{
    int		    length, posn;
    PR_SIZE	    tsize;
    float	    lensin, lencos;
    int		    prev_work_font;

    cur_x = x;
    cur_y = y;

    /* clear canvas loc move proc in case we were in text select mode */
    canvas_locmove_proc = null_proc;

    set_action_on();
    set_mousefun("new text", "finish text", "cancel", "", "paste text", "");
    draw_mousefun_kbd();
    draw_mousefun_canvas();
    canvas_kbd_proc = (FCallBack)char_handler;
    canvas_middlebut_proc = finish_text_input;
    canvas_leftbut_proc = (FCallBack)finish_n_start;
    canvas_rightbut_proc = (FCallBack)cancel_text_input;

    /*
     * set working font info to current settings. This allows user to change
     * font settings while we are in the middle of accepting text without
     * affecting this text i.e. we don't allow the text to change midway
     * through
     */

    if ((cur_t = text_search(cur_x, cur_y, &posn)) == NULL) {

	/******************/
	/* new text input */
	/******************/

	leng_prefix = leng_suffix = 0;
	*suffix = 0;
	prefix[leng_prefix] = '\0';

	/* set origin where mouse was clicked */
	base_x = orig_x = cur_x = x;
	base_y = orig_y = cur_y = y;
#ifdef I18N
	save_base_x = base_x;
	save_base_y = base_y;
#endif /* I18N */

	/* set working settings from ind panel */
	if (is_newline) {	/* working settings already set from previous text */
	    is_newline = False;
	} else {		/* set working settings from ind panel */
	    work_textcolor = cur_pencolor;
	    work_fontsize = cur_fontsize;
	    prev_work_font = work_font;
	    work_font     = using_ps ? cur_ps_font : cur_latex_font;
	    /* font changed, refresh character map panel if it is up */
	    if (prev_work_font != work_font)
		refresh_character_panel();
	    work_psflag   = using_ps;
	    work_flags    = cur_textflags;
	    work_textjust = cur_textjust;
	    work_depth    = cur_depth;
	    work_angle    = cur_elltextangle*M_PI/180.0;
	    while (work_angle < 0.0)
		work_angle += M_2PI;
	    sin_t = sin((double)work_angle);
	    cos_t = cos((double)work_angle);

	    /* load the X font and get its id for this font and size UNZOOMED */
	    /* this is to get widths etc for the unzoomed chars */
	    canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize);
	    /* get the ZOOMED font for actually drawing on the canvas */
	    canvas_zoomed_font = lookfont(x_fontnum(work_psflag, work_font), 
			   round(work_fontsize*display_zoomscale));
	    /* save the working font structure */
	    work_fontstruct = canvas_zoomed_font;
	} /* (is_newline) */

    } else {

	/*****************/
	/* existing text */
	/*****************/

	if (hidden_text(cur_t)) {
	    put_msg("Can't edit hidden text");
	    reset_action_on();
	    text_drawing_selected();
	    return;
	}
	/* update the working text parameters */
	work_textcolor = cur_t->color;
	prev_work_font = work_font;
	work_font = cur_t->font;
	/* font changed, refresh character map panel if it is up */
	if (prev_work_font != work_font)
	    refresh_character_panel();
	work_fontstruct = canvas_zoomed_font = cur_t->fontstruct;
	work_fontsize = cur_t->size;
	work_psflag   = cur_t->flags & PSFONT_TEXT;
	work_flags    = cur_t->flags;
	work_textjust = cur_t->type;
	work_depth    = cur_t->depth;
	work_angle    = cur_t->angle;
	while (work_angle < 0.0)
		work_angle += M_2PI;
	sin_t = sin((double)work_angle);
	cos_t = cos((double)work_angle);

	/* load the X font and get its id for this font, size and angle UNZOOMED */
	/* this is to get widths etc for the unzoomed chars */
	canvas_font = lookfont(x_fontnum(work_psflag, work_font), 
			   work_fontsize);

	toggle_textmarker(cur_t);
	draw_text(cur_t, ERASE);
	base_x = cur_t->base_x;
	base_y = cur_t->base_y;
	length = cur_t->length;
	lencos = length*cos_t;
	lensin = length*sin_t;
#ifdef I18N
	save_base_x = base_x;
	save_base_y = base_y;
#endif /* I18N */

	/* set origin to base of this text so newline will go there */
	orig_x = base_x;
	orig_y = base_y;

	switch (cur_t->type) {
	  case T_CENTER_JUSTIFIED:
	    base_x = round(base_x - lencos/2.0);
	    base_y = round(base_y + lensin/2.0);
	    break;

	  case T_RIGHT_JUSTIFIED:
	    base_x = round(base_x - lencos);
	    base_y = round(base_y + lensin);
	    break;
	} /* switch */

	leng_suffix = strlen(cur_t->cstring);
	/* leng_prefix is index of char in the text before the cursor */
	/* it is also used for text selection as the starting point */
	leng_prefix = prefix_length(cur_t->cstring, posn);

#ifdef SEL_TEXT
	/**********************************************/
	/* user has double-clicked, select whole word */
	/**********************************************/
	if (pointer_click == 2) {
	    /* if any text is selected from before, undraw the selection */
	    if (lensel && text_selection_showing)
		undraw_selection(start_sel_x, start_sel_y, text_selection);
	    startp = leng_prefix-1;
	    /* back up to the beginning of the word or string */
	    while (startp && cur_t->cstring[startp-1] != ' ')
		startp--;
	    endp = leng_prefix;
	    /* now go forward to the end of the word or string */
	    while (endp < leng_suffix && cur_t->cstring[endp] != ' ')
		endp++;
	    lensel = endp-startp;
	    /* copy into the selection */
	    strncpy(text_selection, &cur_t->cstring[startp], lensel);
	    text_selection[lensel] = '\0';
	    /* save starting point of selection */
	    start_text_select = prev_indx = startp;
	    /* prefix includes selected text */
	    leng_prefix = endp;
	    /* save starting x,y of select */
	    tsize = textsize(canvas_font, leng_prefix-lensel, prefix);
	    start_sel_x = round(base_x + tsize.length * cos_t);
	    start_sel_y = round(base_y - tsize.length * sin_t);
	    selection_dir =  1;		/* selecting to the right */
	} else {
	    /* save starting point of text selection */
	    start_text_select = prev_indx = leng_prefix;
	}
#endif /* SEL_TEXT */
	    
	leng_suffix -= leng_prefix;
	strncpy(prefix, cur_t->cstring, leng_prefix);
	prefix[leng_prefix]='\0';
	strcpy(suffix, &cur_t->cstring[leng_prefix]);
	tsize = textsize(canvas_font, leng_prefix, prefix);

	/* set current to character position of mouse click (end of prefix) */
	cur_x = round(base_x + tsize.length * cos_t);
	cur_y = round(base_y - tsize.length * sin_t);

#ifdef SEL_TEXT
	/* set text selection flag in case user moves pointer along text with button down */
	text_selection_active = True;
	/* set flag saying the user just clicked */
	click_on_text = True;
	/* and set canvas move procedure to keep track of text being selected */
	canvas_locmove_proc = track_text_select;
#endif /* SEL_TEXT */
    }
    /* save floating font size */
    work_float_fontsize = work_fontsize;
    /* reset super/subscript counter */
    supersub = 0;

    put_msg("Ready for text input (from keyboard)");
    /* save original char_ht for newline */
    orig_ht = char_ht = ZOOM_FACTOR * max_char_height(canvas_font);
    initialize_char_handler(canvas_win, (IntFunc)finish_text_input,
			    base_x, base_y);
#ifdef SEL_TEXT
    /* if any text is selected from before, undraw the selection */
    if (lensel && text_selection_showing) {
	undraw_selection(start_sel_x, start_sel_y, text_selection);
	text_selection_showing = False;
    }
#endif /* SEL_TEXT */
    draw_char_string();
#ifdef SEL_TEXT
    /* draw the selected word in inverse */
    if (pointer_click == 2) {
	draw_selection(start_sel_x, start_sel_y, text_selection);
	text_selection_showing = True;
    }
#endif /* SEL_TEXT */
}

static F_text *
new_text(void)
{
    F_text	   *text;
    PR_SIZE	    size;

    if ((text = create_text()) == NULL)
	return (NULL);

    if ((text->cstring = new_string(leng_prefix)) == NULL) {
	free((char *) text);
	return (NULL);
    }
    text->type = work_textjust;
    text->font = work_font;	/* put in current font number */
    text->fontstruct = work_fontstruct;
    text->zoom = zoomscale;
    text->size = work_fontsize;
    text->angle = work_angle;
    text->flags = work_flags;
    text->color = cur_pencolor;
    text->depth = work_depth;
    text->pen_style = -1;
    size = textsize(canvas_font, leng_prefix, prefix);
    text->length = size.length;
    text->ascent = size.ascent;
    text->descent = size.descent;
    text->base_x = base_x;
    text->base_y = base_y;
    strcpy(text->cstring, prefix);
    text->next = NULL;
    return (text);
}

/* return the index of the character in the string before the cursor (where_p) */

static int
prefix_length(char *string, int where_p)
{
    /* c stands for character unit and p for pixel unit */
    int		    l, len_c, len_p;
    int		    char_wid, where_c;
    PR_SIZE	    size;

    len_c = strlen(string);
    size = textsize(canvas_font, len_c, string);
    len_p = size.length;
    if (where_p >= len_p)
	return (len_c);		/* entire string is the prefix */

#ifdef I18N
    if (appres.international && is_i18n_font(canvas_font)) {
      where_c = 0;
      while (where_c < len_c) {
	size = textsize(canvas_font, where_c, string);
	if (where_p <= size.length) return where_c;
	if (appres.euc_encoding) {
	  if (string[where_c] == EUC_SS3) where_c = where_c + 2;
	  else if (is_euc_multibyte(string[where_c])) where_c = where_c + 1;
	}
	where_c = where_c + 1;
      }
      return len_c;
    }
#endif  /* I18N */
    char_wid = ZOOM_FACTOR * char_width(canvas_font);
    where_c = where_p / char_wid;	/* estimated char position */
    size = textsize(canvas_font, where_c, string);
    l = size.length;		/* actual length (pixels) of string of
				 * where_c chars */
    if (l < where_p) {
	do {			/* add the width of next char to l */
	    l += (char_wid = ZOOM_FACTOR * char_advance(canvas_font, 
				(unsigned char) string[where_c++]));
	} while (l < where_p);
	if (l - (char_wid >> 1) >= where_p)
	    where_c--;
    } else if (l > where_p) {
	do {			/* subtract the width of last char from l */
	    l -= (char_wid = ZOOM_FACTOR * char_advance(canvas_font, 
				(unsigned char) string[--where_c]));
	} while (l > where_p);
	if (l + (char_wid >> 1) <= where_p)
	    where_c++;
    }
    if (where_c < 0) {
	fprintf(stderr, "xfig file %s line %d: Error in prefix_length - adjusted\n", __FILE__, __LINE__);
	where_c = 0;
    }
    if ( where_c > len_c ) 
	return (len_c);
    return (where_c);
}

/*******************************************************************

	char handling routines

*******************************************************************/

#define			BLINK_INTERVAL	700	/* milliseconds blink rate */

static Window	pw;
static int	ch_height;
static int	cbase_x, cbase_y;
static float	rbase_x, rbase_y, rcur_x, rcur_y;

static int	(*cr_proc) ();

static void
draw_cursor(int x, int y)
{
    pw_vector(pw, x, y, 
		round(x-ch_height*sin_t),
		round(y-ch_height*cos_t),
		INV_PAINT, 1, RUBBER_LINE, 0.0, DEFAULT);
}

static void
initialize_char_handler(Window w, int (*cr) (/* ??? */), int bx, int by)
{
    pw = w;
    cr_proc = cr;
    rbase_x = cbase_x = bx;	/* keep real base so dont have roundoff */
    rbase_y = cbase_y = by;
    rcur_x = cur_x;
    rcur_y = cur_y;

    ch_height = ZOOM_FACTOR * canvas_font->max_bounds.ascent;
    turn_on_blinking_cursor(draw_cursor, draw_cursor,
			    cur_x, cur_y, (long unsigned int) BLINK_INTERVAL);
#ifdef I18N
    if (xim_ic != NULL && (appres.latin_keyboard || is_i18n_font(canvas_font))) {
      put_msg("Ready for text input (from keyboard with input-method)");
      XSetICFocus(xim_ic);
      xim_active = True;
      xim_set_spot(cur_x, cur_y);
    }
#endif /* I18N */
}

static void
terminate_char_handler(void)
{
    turn_off_blinking_cursor();
    cr_proc = NULL;
#ifdef I18N
    if (xim_ic != NULL) XUnsetICFocus(xim_ic);
    xim_active = False;
#endif /* I18N */
}

void do_char(unsigned char ch, int op)
{
    char	     c[2];

    c[0] = ch; c[1] = '\0';
    pw_text(pw, cur_x, cur_y, op, MAX_DEPTH+1, canvas_zoomed_font, 
	    work_angle, c, work_textcolor, COLOR_NONE);
}

void draw_char(unsigned char ch)
{
    do_char(ch, PAINT);
}

void erase_char(unsigned char ch)
{
    do_char(ch, ERASE);
}

void do_prefix(int op)
{
    if (leng_prefix)
	pw_text(pw, cbase_x, cbase_y, op, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, prefix, work_textcolor, COLOR_NONE);
}

void draw_prefix(void)
{
    do_prefix(PAINT);
}

void erase_prefix(void)
{
    do_prefix(ERASE);
}

void do_suffix(int op)
{
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, op, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor, COLOR_NONE);
}

void draw_suffix(void)
{
    do_suffix(PAINT);
}

void erase_suffix(void)
{
    do_suffix(ERASE);
}

void
erase_char_string(void)
{
    pw_text(pw, cbase_x, cbase_y, ERASE, MAX_DEPTH+1, canvas_zoomed_font, 
	    work_angle, prefix, work_textcolor, COLOR_NONE);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, ERASE, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor, COLOR_NONE);
}

void
draw_char_string(void)
{
#ifdef I18N
    if (appres.international && is_i18n_font(canvas_font)) {
	double cwidth;
	int direc, asc, des;
	XCharStruct overall;

	float mag = ZOOM_FACTOR / display_zoomscale;
	float prefix_width = 0, suffix_width = 0;
	if (0 < leng_prefix) {
	    i18n_text_extents(canvas_zoomed_font, prefix, leng_prefix,
			  &direc, &asc, &des, &overall);
	    prefix_width = (float)(overall.width) * mag;
	}
	if (0 < leng_suffix) {
	    i18n_text_extents(canvas_zoomed_font, suffix, leng_suffix,
			  &direc, &asc, &des, &overall);
	    suffix_width = (float)(overall.width) * mag;
	}

	cbase_x = save_base_x;
	cbase_y = save_base_y;
	switch (work_textjust) {
	    case T_LEFT_JUSTIFIED:
		break;
	    case T_RIGHT_JUSTIFIED:
		cbase_x = cbase_x - (prefix_width + suffix_width) * cos_t;
		cbase_y = cbase_y + (prefix_width + suffix_width) * sin_t;
		break;
	    case T_CENTER_JUSTIFIED:
		cbase_x = cbase_x - (prefix_width + suffix_width) * cos_t / 2;
		cbase_y = cbase_y + (prefix_width + suffix_width) * sin_t / 2;
		break;
	}
      
	pw_text(pw, cbase_x, cbase_y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
	      work_angle, prefix, work_textcolor, COLOR_NONE);
	cur_x = cbase_x + prefix_width * cos_t;
	cur_y = cbase_y - prefix_width * sin_t;
	if (leng_suffix)
	    pw_text(pw, cur_x, cur_y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor, COLOR_NONE);
	move_blinking_cursor(cur_x, cur_y);
	return;
    }
#endif /* I18N */
    pw_text(pw, cbase_x, cbase_y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
	    work_angle, prefix, work_textcolor, COLOR_NONE);
    if (leng_suffix)
	pw_text(pw, cur_x, cur_y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, suffix, work_textcolor, COLOR_NONE);
    move_blinking_cursor(cur_x, cur_y);
}

void
char_handler(XKeyEvent *kpe, unsigned char c, KeySym keysym)
{
    register int    i;
    unsigned char   ch;

    if (cr_proc == NULL)
	return;

#ifdef SEL_TEXT
    /* clear text selection flag since user typed a character */
    /* but only if not one of the selection editing chars */
    if (c != CTRL_W) {
	if (text_selection_active)
	    undraw_selection(start_sel_x, start_sel_y, text_selection);
	text_selection_active = False;
	/* and canvas loc move proc */
	canvas_locmove_proc = null_proc;
    }
#endif /* SEL_TEXT */

    if (c == ESC) {
	create_textobject();
	canvas_kbd_proc = (FCallBack)null_proc;
	canvas_middlebut_proc = (FCallBack)null_proc;
	canvas_leftbut_proc = (FCallBack)null_proc;
	canvas_rightbut_proc = (FCallBack)null_proc;
    } else if (c == CR || c == NL) {
	new_text_line();
    } else if (c == CTRL_UNDERSCORE) {
	/* subscript */
	new_text_down();
    } else if (c == CTRL_HAT) {
	/* superscript */
	new_text_up();

    /******************************************************/
    /* move cursor left - move char from prefix to suffix */
    /* Control-B and the Left arrow key both do this */
    /******************************************************/
    } else if (keysym == XK_Left || c == CTRL_B) {
#ifdef I18N
	if (leng_prefix > 0 && appres.international && is_i18n_font(canvas_font)) {
	    erase_char_string();
	    move_pref_to_suf();
	    draw_char_string();
	} else
#endif /* I18N */
	if (leng_prefix > 0) {
	    move_pref_to_suf();
	    move_cur(-1, suffix[0], 1.0);
	}

    /*******************************************************/
    /* move cursor right - move char from suffix to prefix */
    /* Control-F and Right arrow key both do this */
    /*******************************************************/
    } else if (keysym == XK_Right || c == CTRL_F) {
#ifdef I18N
	if (leng_suffix > 0 && appres.international && is_i18n_font(canvas_font)) {
	    erase_char_string();
	    move_suf_to_pref();
	    draw_char_string();
	} else
#endif /* I18N */
	if (leng_suffix > 0) {
	    move_suf_to_pref();
	    move_cur(1, prefix[leng_prefix-1], 1.0);
	}

    /***************************************************************/
    /* move cursor to beginning of text - put everything in suffix */
    /* Control-A and Home key both do this */
    /***************************************************************/
    } else if (keysym == XK_Home || c == CTRL_A) {
	if (leng_prefix > 0) {
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      erase_char_string();
	    else	      
#endif  /* I18N */
	    for (i=leng_prefix-1; i>=0; i--)
		move_cur(-1, prefix[i], 1.0);
	    strcat(prefix,suffix);
	    strcpy(suffix,prefix);
	    prefix[0]='\0';
	    leng_prefix=0;
	    leng_suffix=strlen(suffix);
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      draw_char_string();
#endif  /* I18N */
	}

    /*********************************************************/
    /* move cursor to end of text - put everything in prefix */
    /* Control-E and End key both do this */
    /*********************************************************/
    } else if (keysym == XK_End || c == CTRL_E) {
	if (leng_suffix > 0) {
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      erase_char_string();
	    else	      
#endif  /* I18N */
	    for (i=0; i<leng_suffix; i++)
		move_cur(1, suffix[i], 1.0);
	    strcat(prefix,suffix);
	    suffix[0]='\0';
	    leng_suffix=0;
	    leng_prefix=strlen(prefix);
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      draw_char_string();
#endif  /* I18N */
	}

    /******************************************/
    /* backspace - delete char left of cursor */
    /******************************************/
    } else if (c == CTRL_H) {
	ch = prefix[leng_prefix-1];
#ifdef I18N
	if (leng_prefix > 0
	      && appres.international && is_i18n_font(canvas_font)) {
	    int len;
	    erase_char_string();
	    len = i18n_prefix_tail(NULL);
	    leng_prefix-=len;
	    erase_char(ch);
	    prefix[leng_prefix]='\0';
	    draw_char_string();
	} else
#endif /* I18N */
	if (leng_prefix > 0) {
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    erase_suffix();
		    move_cur(-1, ch, 1.0);
		    erase_char(ch);
		    break;
		case T_CENTER_JUSTIFIED:
		    erase_char_string();
		    move_cur(-1, ch, 2.0);
		    move_text(1, ch, 2.0);
		    break;
		case T_RIGHT_JUSTIFIED:
		    erase_prefix();
		    move_text(1, ch, 1.0);
		    break;
		}
	    /* remove the character from the prefix */
	    prefix[--leng_prefix] = '\0';

	    /* now redraw stuff */
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    draw_suffix();
		    break;
		case T_CENTER_JUSTIFIED:
		    draw_char_string();
		    break;
		case T_RIGHT_JUSTIFIED:
		    draw_prefix();
		    break;
	    }
	}

    /*****************************************/
    /* delete char to right of cursor        */
    /* Control-D and Delete key both do this */
    /*****************************************/
    } else if (c == DEL || c == CTRL_D) {
#ifdef I18N
	if (leng_suffix > 0
	      && appres.international && is_i18n_font(canvas_font)) {
	    int len;
	    erase_char_string();
	    len = i18n_suffix_head(NULL);
	    for (i=0; i<=leng_suffix-len; i++)	/* copies null too */
		suffix[i]=suffix[i+len];
	    leng_suffix-=len;
	    draw_char_string();
	} else
#endif /* I18N */
	if (leng_suffix > 0) {
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    erase_suffix();
		    break;
		case T_CENTER_JUSTIFIED:
		    erase_char_string();
		    move_cur(1, suffix[0], 2.0);
		    move_text(1, suffix[0], 2.0);
		    break;
		case T_RIGHT_JUSTIFIED:
		    erase_prefix();
		    erase_char(suffix[0]);
		    move_cur(1, suffix[0], 1.0);
		    move_text(1, suffix[0], 1.0);
		    break;
		}
	    /* shift suffix left one char */
	    for (i=0; i<=leng_suffix; i++)	/* copies null too */
		suffix[i]=suffix[i+1];
	    leng_suffix--;
	    /* now redraw stuff */
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    draw_suffix();
		    break;
		case T_CENTER_JUSTIFIED:
		    draw_char_string();
		    break;
		case T_RIGHT_JUSTIFIED:
		    draw_prefix();
		    break;
	    }
	}

    /*******************************/
    /* delete to beginning of line */
    /*******************************/
    } else if (c == CTRL_X) {
	if (leng_prefix > 0) {
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      erase_char_string();
	    else
#endif  /* I18N */
	    switch (work_textjust) {
	        case T_LEFT_JUSTIFIED:
		    erase_char_string();
		    rcur_x = rbase_x;
		    cur_x = cbase_x = round(rcur_x);
		    rcur_y = rbase_y;
		    cur_y = cbase_y = round(rcur_y);
		    break;
	        case T_CENTER_JUSTIFIED:
		    erase_char_string();
		    while (leng_prefix--) {	/* subtract char width/2 per char */
			rcur_x -= ZOOM_FACTOR * cos_t*char_advance(canvas_font,
					(unsigned char) prefix[leng_prefix]) / 2.0;
			rcur_y += ZOOM_FACTOR * sin_t*char_advance(canvas_font,
					(unsigned char) prefix[leng_prefix]) / 2.0;
		    }
		    rbase_x = rcur_x;
		    cur_x = cbase_x = round(rbase_x);
		    rbase_y = rcur_y;
		    cur_y = cbase_y = round(rbase_y);
		    break;
	        case T_RIGHT_JUSTIFIED:
		    erase_prefix();
		    rbase_x = rcur_x;
		    cbase_x = cur_x = round(rbase_x);
		    rbase_y = rcur_y;
		    cbase_y = cur_y = round(rbase_y);
		    break;
	    }
	    leng_prefix = 0;
	    *prefix = '\0';
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    draw_char_string();
		    break;
		case T_CENTER_JUSTIFIED:
		    draw_char_string();
		    break;
		case T_RIGHT_JUSTIFIED:
		    break;
	    }
	    move_blinking_cursor(cur_x, cur_y);
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      draw_char_string();
#endif  /* I18N */
	}

    /*************************/
    /* delete to end of line */
    /*************************/
    } else if (c == CTRL_K) {
	if (leng_suffix > 0) {
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      erase_char_string();
	    else
#endif  /* I18N */
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    erase_suffix();
		    break;
		case T_CENTER_JUSTIFIED:
		    erase_char_string();
		    while (leng_suffix--) {
			move_cur(1, suffix[leng_suffix], 2.0);
			move_text(1, suffix[leng_suffix], 2.0);
		    }
		    break;
		case T_RIGHT_JUSTIFIED:
		    erase_char_string();
		    /* move cursor to end of (orig) string then move string over */
		    while (leng_suffix--) {
			move_cur(1, suffix[leng_suffix], 1.0);
			move_text(1, suffix[leng_suffix], 1.0    );
		    }
		    break;
		}
	    leng_suffix = 0;
	    *suffix = '\0';
	    /* redraw stuff */
#ifdef I18N
	    if (appres.international && is_i18n_font(canvas_font))
	      draw_char_string();
	    else
#endif  /* I18N */
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    break;
		case T_CENTER_JUSTIFIED:
		    draw_char_string();
		    break;
		case T_RIGHT_JUSTIFIED:
		    draw_char_string();
		    break;
	    }
	}
#ifdef SEL_TEXT
    /************************/
    /* delete selected text */
    /************************/
    } else if (c == CTRL_W) {
	/* only if active */
	if (lensel) {
	    /* simply delete lensel characters from the end of the prefix */
	    prefix[leng_prefix-lensel] = '\0';
	    leng_prefix = strlen(prefix);
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    erase_suffix();
		    break;
		case T_CENTER_JUSTIFIED:
		    erase_prefix();
		    erase_suffix();
		    break;
		case T_RIGHT_JUSTIFIED:
		    erase_prefix();
		    break;
	    }
	    /* move cursor and/or text (prefix) base */
	    for (i=0; i<lensel; i++) {
		switch (work_textjust) {
		  case T_LEFT_JUSTIFIED:
		    move_cur(-1, text_selection[i], 1.0);
		    break;
		  case T_CENTER_JUSTIFIED:
		    move_cur(-1, text_selection[i], 2.0);
		    move_text(1, text_selection[i], 2.0);
		    break;
		  case T_RIGHT_JUSTIFIED:
		    move_text(1, text_selection[i], 1.0);
		    break;
		}
	    }
	    /* turn off blinking cursor temporarily */
	    turn_off_blinking_cursor();
	    /* erase the selection characters */
	    pw_text(pw, start_sel_x, start_sel_y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, text_selection, CANVAS_BG, CANVAS_BG);
	    switch (work_textjust) {
		case T_LEFT_JUSTIFIED:
		    /* redraw suffix */
		    draw_suffix();
		    break;
		case T_CENTER_JUSTIFIED:
		    /* redraw both prefix and suffix */
		    draw_prefix();
		    draw_suffix();
		    break;
		case T_RIGHT_JUSTIFIED:
		    /* redraw prefix */
		    draw_prefix();
		    break;
	    }
	    /* turn on blinking cursor again */
	    turn_on_blinking_cursor(draw_cursor, draw_cursor,
			    cur_x, cur_y, (long) BLINK_INTERVAL);
	    /* clear the selection */
	    lensel = 0;
	    text_selection[0] = '\0';
	    text_selection_active = False;
	    text_selection_showing = False;
	}
#endif /* SEL_TEXT */
    } else if (c < SP) {
	put_msg("Invalid character ignored");
    } else if (leng_prefix + leng_suffix == BUF_SIZE) {
	put_msg("Text buffer is full, character is ignored");

    /*************************/
    /* normal text character */
    /*************************/
    } else {
	/* move pointer */
#ifdef I18N
	if (appres.international && is_i18n_font(canvas_font))
	  erase_char_string();
	else
#endif  /* I18N */
	switch (work_textjust) {
	    case T_LEFT_JUSTIFIED:
		erase_suffix();		/* erase any string after cursor */
		draw_char(c);		/* draw new char */
		move_cur(1, c, 1.0);
		break;
	    case T_CENTER_JUSTIFIED:
		erase_char_string();
		move_cur(1, c, 2.0);
		move_text(-1, c, 2.0);
		break;
	    case T_RIGHT_JUSTIFIED:
		erase_prefix();
		move_text(-1, c, 1.0);
		break;
	    }
	prefix[leng_prefix++] = c;
	prefix[leng_prefix] = '\0';
	move_blinking_cursor(cur_x, cur_y);
	/* redraw stuff */
#ifdef I18N
	if (appres.international && is_i18n_font(canvas_font))
	  draw_char_string();
	else
#endif  /* I18N */
	switch (work_textjust) {
	    case T_LEFT_JUSTIFIED:
		draw_suffix();
		break;
	    case T_CENTER_JUSTIFIED:
		draw_char_string();
		break;
	    case T_RIGHT_JUSTIFIED:
		draw_prefix();
		break;
	}
    }
}

/* move the cursor left (-1) or right (1) by the width of char c divided by div */

static void
move_cur(int dir, unsigned char c, float div)
{
    double	    cwidth;
    double	    cwsin, cwcos;

    cwidth = (float) (ZOOM_FACTOR * char_advance(canvas_font, c));
    cwsin = cwidth/div*sin_t;
    cwcos = cwidth/div*cos_t;

    rcur_x += dir*cwcos;
    rcur_y -= dir*cwsin;
    cur_x = round(rcur_x);
    cur_y = round(rcur_y);
    move_blinking_cursor(cur_x, cur_y);
}

/* move the base of the text left (-1) or right (1) by the width of
   char c divided by div */

static void
move_text(int dir, unsigned char c, float div)
{
    double	    cwidth;
    double	    cwsin, cwcos;

    cwidth = (float) (ZOOM_FACTOR * char_advance(canvas_font, c));
    cwsin = cwidth/div*sin_t;
    cwcos = cwidth/div*cos_t;

    rbase_x += dir*cwcos;
    rbase_y -= dir*cwsin;
    cbase_x = round(rbase_x);
    cbase_y = round(rbase_y);
}

#ifdef SEL_TEXT

/********************************************************/
/*							*/
/*		Text selection procedures		*/
/*							*/
/********************************************************/

/* keep track of text being selected by button down pointer movement */

static void
track_text_select(x, y)
    int		    x, y;
{
    int		    indx, posn;
    int		    i, dir;
    char	    substr[200];

    /* if user released button, turn off canvas locmove proc */
    if (!text_selection_active) {
	canvas_locmove_proc = null_proc;
	return;
    }

    /* if the pointer is not in the string (plus a little to the right) anymore, return */
    if (!in_text_bound(cur_t, x, y, &posn, True))
	return;

    indx = prefix_length(cur_t->cstring, posn);
    /* if on same character, return */
    if (indx == prev_indx)
	return;

    /* if the user just clicked and has now moved the pointer */
    if (click_on_text) {
	/* save starting point of selection for refresing if user clicks elsewhere */
	start_sel_x = cur_x;
	start_sel_y = cur_y;
	/* clear the selection */
	text_selection[0] = '\0';
	lensel = 0;
	if (indx < prev_indx)
	    selection_dir = -1;		/* selecting to the left */
	else
	    selection_dir =  1;		/* selecting to the right */
	click_on_text = False;
    }
	
    if (indx > start_text_select) {
	/* selecting right */
	if (selection_dir == -1) {
	    /* but we were selecting left, switch */
	    selection_dir = 1;
	    for (i=0; i<lensel; i++)
		move_cur(1, text_selection[i], 1.0);
	    /* and draw the characters normal */
	    undraw_selection(cur_x, cur_y, text_selection);
	    text_selection[0] = '\0';
	    lensel = 0;
	    /* restart prev_index */
	    prev_indx = start_text_select;
	} 
	if (indx > prev_indx) {
	    /* we selected right, and are still moving right */
	    for (i=0; i < (indx-prev_indx); i++) {
		substr[i] = cur_t->cstring[i+prev_indx];
		/* move char to prefix */
		move_suf_to_pref();
		/* update currently selected text */
		text_selection[lensel++] = substr[i];
	    }
	    substr[i] = '\0';
	    text_selection[lensel] = '\0';
	    text_selection[indx-start_text_select] = '\0';
	    /* draw the characters inverted */
	    draw_selection(cur_x, cur_y, substr);
	    /* and move the cursor to the current position */
	    for (i=0; i<strlen(substr); i++)
		move_cur(1, substr[i], 1.0);
	} else {
	    /* we started selecting right, but are unselecting left */
	    for (i=0; i < (prev_indx-indx); i++) {
		substr[i] = cur_t->cstring[i+indx];
		/* put char back in suffix */
		move_pref_to_suf();
		/* update currently selected text */
		lensel--;
	    }
	    substr[i] = '\0';
	    text_selection[lensel] = '\0';
	    /* move the cursor to the current position */
	    for (i=0; i<strlen(substr); i++)
		move_cur(-1, substr[i], 1.0);
	    /* and draw the characters normal */
	    undraw_selection(cur_x, cur_y, substr);
	}

    } else {
	/* selecting left */
	if (selection_dir == 1) {
	    /* but we were selecting right, switch */
	    selection_dir = -1;
	    for (i=0; i<lensel; i++)
		move_cur(-1, text_selection[i], 1.0);
	    /* and draw the characters normal */
	    undraw_selection(cur_x, cur_y, text_selection);
	    text_selection[0] = '\0';
	    lensel = 0;
	    /* restart prev_index */
	    prev_indx = start_text_select;
	}
	if (indx < prev_indx) {
	    /* we selected left, and are still moving left */
	    for (i=0; i < (prev_indx-indx); i++) {
		substr[i] = cur_t->cstring[i+indx];
		/* move char to suffix */
		move_pref_to_suf();
	    }
	    substr[i] = '\0';
	    /* update currently selected text */
	    lensel += prev_indx-indx;
	    strncpy(text_selection, &cur_t->cstring[indx], lensel);
	    text_selection[lensel] = '\0';
	    /* move the cursor to the current position */
	    for (i=0; i<strlen(substr); i++)
		move_cur(-1, substr[i], 1.0);
	    /* and draw the characters inverted */
	    draw_selection(cur_x, cur_y, substr);
	} else {
	    /* we started selecting left, but are unselecting right */
	    for (i=0; i < (indx-prev_indx); i++) {
		substr[i] = cur_t->cstring[i+prev_indx];
		/* put char back in prefix */
		move_suf_to_pref();
	    }
	    substr[i] = '\0';
	    /* update currently selected text */
	    lensel -= indx-prev_indx;
	    strncpy(text_selection, &cur_t->cstring[indx], lensel);
	    text_selection[lensel] = '\0';
	    /* draw the characters normal */
	    undraw_selection(cur_x, cur_y, substr);
	    /* and move the cursor to the current position */
	    for (i=0; i<strlen(substr); i++)
		move_cur(1, substr[i], 1.0);
	}
    }
    text_selection_showing = True;
    prev_indx = indx;
}
#endif /* SEL_TEXT */

/* move last char of prefix to first of suffix */

void move_pref_to_suf(void)
{
    int		    i;

#ifdef I18N
    int		    len;
    if (leng_prefix > 0 && appres.international && is_i18n_font(canvas_font)) {
	len = i18n_prefix_tail(NULL);
	for (i=leng_suffix+len; i>0; i--)	/* copies null too */
	    suffix[i]=suffix[i-len];
	for (i=0; i<len; i++)
	    suffix[i]=prefix[leng_prefix-len+i];
	prefix[leng_prefix-len]='\0';
	leng_prefix-=len;
	leng_suffix+=len;
    } else
#endif /* I18N */
    if (leng_prefix > 0) {
	for (i=leng_suffix+1; i>0; i--)	/* copies null too */
	    suffix[i]=suffix[i-1];
	suffix[0]=prefix[leng_prefix-1];
	prefix[leng_prefix-1]='\0';
	leng_prefix--;
	leng_suffix++;
    }
}

/* move first char of suffix to last of prefix */

void move_suf_to_pref(void)
{
    int		    i;

#ifdef I18N
    int		    len;
    if (leng_suffix > 0 && appres.international && is_i18n_font(canvas_font)) {
	len = i18n_suffix_head(NULL);
	for (i=0; i<len; i++)
	   prefix[leng_prefix+i]=suffix[i];
	prefix[leng_prefix+len]='\0';
	for (i=0; i<=leng_suffix-len; i++)	/* copies null too */
	    suffix[i]=suffix[i+len];
	leng_suffix-=len;
	leng_prefix+=len;
    } else
#endif /* I18N */
    if (leng_suffix > 0) {
	prefix[leng_prefix] = suffix[0];
	prefix[leng_prefix+1]='\0';
	for (i=0; i<=leng_suffix; i++)	/* copies null too */
	    suffix[i]=suffix[i+1];
	leng_suffix--;
	leng_prefix++;
    }
}

#ifdef SEL_TEXT

/* draw string from x, y in inverse (selected) color */

draw_selection(x, y, string)
    int		    x, y;
    char	   *string;
{
    /* turn off blinking cursor temporarily */
    turn_off_blinking_cursor();
    pw_text(pw, x, y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, string, CANVAS_BG, work_textcolor);
    /* turn on blinking cursor again */
    turn_on_blinking_cursor(draw_cursor, draw_cursor,
		    cur_x, cur_y, (long) BLINK_INTERVAL);
}

/* draw string from x, y in normal (unselected) color */

undraw_selection(x, y, string)
    int		    x, y;
    char	   *string;
{
    /* turn off blinking cursor temporarily */
    turn_off_blinking_cursor();
    pw_text(pw, x, y, PAINT, MAX_DEPTH+1, canvas_zoomed_font, 
		work_angle, string, work_textcolor, CANVAS_BG);
    /* turn on blinking cursor again */
    turn_on_blinking_cursor(draw_cursor, draw_cursor,
		    cur_x, cur_y, (long) BLINK_INTERVAL);
}

/* convert selection to string */
/* this the callback w_canvas.c: canvas_selected() when user pastes the selection */

Boolean
ConvertSelection(w, selection, target, type, value, length, format)
    Widget	    w;
    Atom	   *selection, *target, *type;
    XtPointer	   *value;
    unsigned long  *length;
    int		   *format;
{
    /* if nothing, return */
    if (lensel == 0)
	return False;

    if (*target == XA_STRING ||
	*target == XA_TEXT(tool_d) ||
	*target == XA_COMPOUND_TEXT(tool_d)) {
	if (*target == XA_COMPOUND_TEXT(tool_d))
	    *type = *target;
	else
	    *type = XA_STRING;
	*value = text_selection;
	*length = lensel;
	*format = 8;
	return True;
    }
}

void
LoseSelection(w, selection)
    Widget	    w;
    Atom	   *selection;
{
    lensel = 0;
    text_selection[0] = '\0';
    /* unhighlight text */
    if (cur_t)
	redisplay_text(cur_t);
}

void
TransferSelectionDone(w, selection, target)
    Widget	    w;
    Atom	   *selection, *target;
{
    /* nothing to do */
}
#endif /* SEL_TEXT */

/****************************************************************/
/*								*/
/*		Blinking cursor handling routines		*/
/*								*/
/****************************************************************/

static int	cursor_on, cursor_is_moving;
static int	cursor_x, cursor_y;
static void	(*erase) (int, int);
static void	(*draw) (int, int);
static XtTimerCallbackProc blink(XtPointer client_data, XtIntervalId *id);
static unsigned long blink_timer;
static int	stop_blinking = False;
static int	cur_is_blinking = False;

static void
turn_on_blinking_cursor(void (*draw_cursor) (int, int), void (*erase_cursor) (int, int), int x, int y, long unsigned int msec)
{
    draw = draw_cursor;
    erase = erase_cursor;
    cursor_is_moving = 0;
    cursor_x = x;
    cursor_y = y;
    blink_timer = msec;
    draw(x, y);
    cursor_on = 1;
    if (!cur_is_blinking) {	/* if we are already blinking, don't request
				 * another */
	(void) XtAppAddTimeOut(tool_app, blink_timer, (XtTimerCallbackProc) blink,
				  (XtPointer) NULL);
	cur_is_blinking = True;
    }
    stop_blinking = False;
}

static void
turn_off_blinking_cursor(void)
{
    if (cursor_on)
	erase(cursor_x, cursor_y);
    stop_blinking = True;
}

static		XtTimerCallbackProc
blink(XtPointer client_data, XtIntervalId *id)
{
    if (!stop_blinking) {
	if (cursor_is_moving)
	    return (0);
	if (cursor_on) {
	    erase(cursor_x, cursor_y);
	    cursor_on = 0;
	} else {
	    draw(cursor_x, cursor_y);
	    cursor_on = 1;
	}
	(void) XtAppAddTimeOut(tool_app, blink_timer, (XtTimerCallbackProc) blink,
				  (XtPointer) NULL);
    } else {
	stop_blinking = False;	/* signal that we've stopped */
	cur_is_blinking = False;
    }
    return (0);
}

static void
move_blinking_cursor(int x, int y)
{
    cursor_is_moving = 1;
    if (cursor_on)
	erase(cursor_x, cursor_y);
    cursor_x = x;
    cursor_y = y;
    draw(cursor_x, cursor_y);
    cursor_on = 1;
    cursor_is_moving = 0;
#ifdef I18N
    if (xim_active) xim_set_spot(x, y);
#endif /* I18N */
}

/*
 * Reload the font structure for all texts, the saved texts and the 
   current work_fontstruct.
 */

void
reload_text_fstructs(void)
{
    F_text	   *t;

    /* reload the compound objects' texts */
    reload_compoundfont(objects.compounds);
    /* and the separate texts */
    for (t=objects.texts; t != NULL; t = t->next)
	reload_text_fstruct(t);
}

/*
 * Reload the font structure for texts in compounds.
 */

static void
reload_compoundfont(F_compound *compounds)
{
    F_compound	   *c;
    F_text	   *t;

    for (c = compounds; c != NULL; c = c->next) {
	reload_compoundfont(c->compounds);
	for (t=c->texts; t != NULL; t = t->next)
	    reload_text_fstruct(t);
    }
}

void
reload_text_fstruct(F_text *t)
{
    t->fontstruct = lookfont(x_fontnum(psfont_text(t), t->font), 
			round(t->size*display_zoomscale));
    t->zoom = zoomscale;
}


/****************************************************************/
/*								*/
/*		Internationalization utility procedures		*/
/*								*/
/****************************************************************/

#ifdef I18N

static void
GetPreferredGeomerty(ic, name, area)
     XIC ic;
     char *name;
     XRectangle **area;
{
  XVaNestedList list;
  list = XVaCreateNestedList(0, XNAreaNeeded, area, NULL);
  XGetICValues(ic, name, list, NULL);
  XFree(list);
}

static void
SetGeometry(ic, name, area)
     XIC ic;
     char *name;
     XRectangle *area;
{
  XVaNestedList list;
  list = XVaCreateNestedList(0, XNArea, area, NULL);
  XSetICValues(ic, name, list, NULL);
  XFree(list);
}

xim_set_ic_geometry(ic, width, height)
     XIC ic;
     int width;
     int height;
{
  XRectangle preedit_area, *preedit_area_ptr;
  XRectangle status_area, *status_area_ptr;

  if (xim_ic == NULL) return;

  if (appres.DEBUG)
    fprintf(stderr, "xim_set_ic_geometry(%d, %d)\n", width, height);

  if (xim_style & XIMStatusArea) {
    GetPreferredGeomerty(ic, XNStatusAttributes, &status_area_ptr);
    status_area.width = status_area_ptr->width;
    if (width / 2 < status_area.width) status_area.width = width / 2;
    status_area.height = status_area_ptr->height;
    status_area.x = 0;
    status_area.y = height - status_area.height;
    SetGeometry(xim_ic, XNStatusAttributes, &status_area);
    if (appres.DEBUG) fprintf(stderr, "status geometry: %dx%d+%d+%d\n",
			      status_area.width, status_area.height,
			      status_area.x, status_area.y);
  }
  if (xim_style & XIMPreeditArea) {
    GetPreferredGeomerty(ic, XNPreeditAttributes, &preedit_area_ptr);
    preedit_area.width = preedit_area_ptr->width;
    if (preedit_area.width < width - status_area.width)
      preedit_area.width = width - status_area.width;
    if (width < preedit_area.width)
      preedit_area.width = width;
    preedit_area.height = preedit_area_ptr->height;
    preedit_area.x = width - preedit_area.width;
    preedit_area.y = height - preedit_area.height;
    SetGeometry(xim_ic, XNPreeditAttributes, &preedit_area);
    if (appres.DEBUG) fprintf(stderr, "preedit geometry: %dx%d+%d+%d\n",
			      preedit_area.width, preedit_area.height,
			      preedit_area.x, preedit_area.y);
  }
}

Boolean
xim_initialize(w)
     Widget w;
{
  const XIMStyle style_notuseful = 0;
  const XIMStyle style_over_the_spot = XIMPreeditPosition | XIMStatusArea;
  const XIMStyle style_old_over_the_spot = XIMPreeditPosition | XIMStatusNothing;
  const XIMStyle style_off_the_spot = XIMPreeditArea | XIMStatusArea;
  const XIMStyle style_root = XIMPreeditNothing | XIMStatusNothing;
  static long int im_event_mask = 0;
  XIMStyles	*styles;
  XIMStyle	 preferred_style;
  int		 i;
  XVaNestedList  preedit_att, status_att;
  XPoint	 spot;
  char		*modifier_list;

  preferred_style = style_notuseful;
  if (strncasecmp(appres.xim_input_style, "OverTheSpot", 3) == 0)
    preferred_style = style_over_the_spot;
  else if (strncasecmp(appres.xim_input_style, "OldOverTheSpot", 6) == 0)
    preferred_style = style_old_over_the_spot;
  else if (strncasecmp(appres.xim_input_style, "OffTheSpot", 3) == 0)
    preferred_style = style_off_the_spot;
  else if (strncasecmp(appres.xim_input_style, "Root", 3) == 0)
    preferred_style = style_root;
  else if (strncasecmp(appres.xim_input_style, "None", 3) != 0)
    fprintf(stderr, "xfig: inputStyle should OverTheSpot, OffTheSpot, or Root\n");

  if (preferred_style == style_notuseful) return;

  if (appres.DEBUG) fprintf(stderr, "initialize_input_method()...\n");
  if ((modifier_list = XSetLocaleModifiers("")) == NULL || *modifier_list == '\0') {
	/* printf("Warning: XSetLocaleModifiers() failed.\n"); */
  }

  xim_im = XOpenIM(XtDisplay(w), NULL, NULL, NULL);
  if (xim_im == NULL) {
    fprintf(stderr, "xfig: can't open input-method\n");
    return False;
  }
  XGetIMValues(xim_im, XNQueryInputStyle, &styles, NULL, NULL);
  for (i = 0; i < styles->count_styles; i++) {
    if (appres.DEBUG)
      fprintf(stderr, "styles[%d]=%lx\n", i, styles->supported_styles[i]);
    if (styles->supported_styles[i] == preferred_style) {
      xim_style = preferred_style;
    } else if (styles->supported_styles[i] == style_root) {
      if (xim_style == 0) xim_style = style_root;
    }
  }
  if (xim_style != preferred_style) {
    fprintf(stderr, "xfig: this input-method doesn't support %s input style\n",
	    appres.xim_input_style);
    if (xim_style == 0) {
      fprintf(stderr, "xfig: it don't support ROOT input style, too...\n");
      return False;
    } else {
      fprintf(stderr, "xfig: using ROOT input style instead.\n");
    }
  }
  if (appres.DEBUG) {
    char *s;
    if (xim_style == style_over_the_spot) s = "OverTheSpot";
    else if (xim_style == style_off_the_spot) s = "OffTheSpot";
    else if (xim_style == style_root) s = "Root";
    else s = "unknown";
    fprintf(stderr, "xfig: selected input style: %s\n", s);
  }

  spot.x = 20;  /* dummy */
  spot.y = 20;
  preedit_att = XVaCreateNestedList(0, XNFontSet, appres.fixed_fontset,
				    XNSpotLocation, &spot,
				    NULL);
  status_att = XVaCreateNestedList(0, XNFontSet, appres.fixed_fontset,
				   NULL);
  xim_ic = XCreateIC(xim_im, XNInputStyle , xim_style,
		     XNClientWindow, XtWindow(w),
		     XNFocusWindow, XtWindow(w),
		     XNPreeditAttributes, preedit_att,
		     XNStatusAttributes, status_att,
		     NULL, NULL);
  XFree(preedit_att);
  XFree(status_att);
  if (xim_ic == NULL) {
    fprintf(stderr, "xfig: can't create input-context\n");
    return False;
  }

  if (appres.DEBUG) fprintf(stderr, "input method initialized\n");

  return True;
}

xim_set_spot(x, y)
     int x, y;
{
  static XPoint spot;
  XVaNestedList preedit_att;
  int x1, y1;
  if (xim_ic != NULL) {
    if (xim_style & XIMPreeditPosition) {
      if (appres.DEBUG) fprintf(stderr, "xim_set_spot(%d,%d)\n", x, y);
      preedit_att = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
      x1 = ZOOMX(x) + 1;
      y1 = ZOOMY(y);
      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      spot.x = x1;
      spot.y = y1;
      XSetICValues(xim_ic, XNPreeditAttributes, preedit_att, NULL);
      XFree(preedit_att);
    }
  }
}

static int
i18n_prefix_tail(char *s1)
{
  int len, i;
  if (appres.euc_encoding && is_euc_multibyte(prefix[leng_prefix-1])) {
    if (leng_prefix > 2 && prefix[leng_prefix-3] == EUC_SS3)
      len = 3;
      /* G3: 10001111 1xxxxxxx 1xxxxxxx (JIS X 0212, for example) */
    else if (leng_prefix > 1)
      len = 2;
      /* G2: 10001110 1xxxxxxx (JIS X 0201, for example) */
      /* G1: 1xxxxxxx 1xxxxxxx (JIS X 0208, for example) */
    else
      len = 1;  /* this can't happen */
  } else {
    len = 1;  /* G0: 0xxxxxxx (ASCII, for example) */
  }
  if (s1 != NULL) {
    for (i = 0; i < len; i++) s1[i] = prefix[leng_prefix - len + i];
    s1[len] = '\0';
  }
  return len;
}

static int
i18n_suffix_head(char *s1)
{
  int len, i;
  if (appres.euc_encoding && is_euc_multibyte(suffix[0])) {
    if (leng_suffix > 2 && suffix[0] == EUC_SS3) len = 3;  /* G3 */
    else if (leng_suffix > 1) len = 2;  /* G1 or G2 */
    else len = 1;  /* this can't happen */
  } else {
    len = 1;  /* G0 */
  }
  if (s1 != NULL) {
    for (i = 0; i < len; i++) s1[i] = suffix[i];
    s1[len] = '\0';
  }
  return len;
}

i18n_char_handler(str)
     unsigned char *str;
{
  int i;
  erase_char_string();	/* erase chars after the cursor */
  for (i = 0; str[i] != '\0'; i++)
	prefix_append_char(str[i]);
  draw_char_string();	/* draw new suffix */
}

prefix_append_char(ch)
     unsigned char ch;
{
  if (leng_prefix + leng_suffix < BUF_SIZE) {
    prefix[leng_prefix++] = ch;
    prefix[leng_prefix] = '\0';
  } else {
    put_msg("Text buffer is full, character is ignored");
  }
}

#ifdef I18N_USE_PREEDIT
static Boolean
is_preedit_running()
{
  pid_t pid;
  sprintf(preedit_filename, "%s/%s%06d", TMPDIR, "xfig-preedit", getpid());
#if defined(_POSIX_SOURCE) || defined(SVR4)
  pid = waitpid(-1, NULL, WNOHANG);
#else
  pid = wait3(NULL, WNOHANG, NULL);
#endif /* defined(_POSIX_SOURCE) || defined(SVR4) */
  if (0 < preedit_pid && pid == preedit_pid) preedit_pid = -1;
  return (0 < preedit_pid && access(preedit_filename, R_OK) == 0);
}

kill_preedit()
{
  if (0 < preedit_pid) {
    kill(preedit_pid, SIGTERM);
    preedit_pid = -1;
  }
}

static
close_preedit_proc(x, y)
     int x, y;
{
  if (is_preedit_running()) {
    kill_preedit();
    put_msg("Pre-edit window closed");
  }
  text_drawing_selected();
  draw_mousefun_canvas();
}

static
open_preedit_proc(x, y)
     int x, y;
{
  int i;
  if (!is_preedit_running()) {
    put_msg("Opening pre-edit window...");
    draw_mousefun_canvas();
    set_temp_cursor(wait_cursor);
    preedit_pid = fork();
    if (preedit_pid == -1) {  /* cannot fork */
        fprintf(stderr, "Can't fork the process: %s\n", strerror(errno));
    } else if (preedit_pid == 0) {  /* child process; execute xfig-preedit */
        execlp(appres.text_preedit, appres.text_preedit, preedit_filename, NULL);
        fprintf(stderr, "Can't execute %s\n", appres.text_preedit);
        exit(-1);
    } else {  /* parent process; wait until xfig-preedit is up */
        for (i = 0; i < 10 && !is_preedit_running(); i++) sleep(1);
    }
    if (is_preedit_running()) 
	put_msg("Pre-edit window opened");
    else
	put_msg("Can't open pre-edit window");
    reset_cursor();
  }
  text_drawing_selected();
  draw_mousefun_canvas();
}

static
paste_preedit_proc(x, y)
     int x, y;
{
  FILE *fp;
  char ch;
  if (!is_preedit_running()) {
    open_preedit_proc(x, y);
  } else if ((fp = fopen(preedit_filename, "r")) != NULL) {
    init_text_input(x, y);
    while ((ch = getc(fp)) != EOF) {
      if (ch == '\\') 
	new_text_line();
      else
	prefix[leng_prefix++] = ch;
    }
    prefix[leng_prefix] = '\0';
    finish_text_input(0,0,0);
    fclose(fp);
    put_msg("Text pasted from pre-edit window");
  } else {
    put_msg("Can't get text from pre-edit window");
  }
  text_drawing_selected();
  draw_mousefun_canvas();
}
#endif  /* I18N_USE_PREEDIT */

#endif /* I18N */
