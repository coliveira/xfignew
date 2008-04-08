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

#include "fig.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "u_fonts.h"
#include "w_indpanel.h"
#include "w_msgpanel.h"
#include "w_setup.h"

int		cur_mode = F_NULL;
int		cur_halign = ALIGN_NONE;
int		cur_valign = ALIGN_NONE;
int		manhattan_mode = 0;
int		mountain_mode = 0;
int		latexline_mode = 0;
int		latexarrow_mode = 0;
int		autoforwardarrow_mode = 0;
int		autobackwardarrow_mode = 0;
int		cur_gridmode, cur_gridunit, old_gridunit, grid_unit;
int		cur_pointposn;
int		posn_rnd[NUM_GRID_UNITS][P_GRID4 + 1] = {
		   { 0, PPCM/10, PPCM/5, PPCM/2, PPCM, PPCM*2},	/*   mm  mode */
		   { 0, PPI/16, PPI/8, PPI/4, PPI/2, PPI},	/* 1/16" mode */
		   { 0, PPI/20, PPI/10, PPI/5, PPI/2, PPI},	/* 1/10" mode */
		   };
int		posn_hlf[NUM_GRID_UNITS][P_GRID4 + 1] = {
		   { 0, PPCM/20, PPCM/10, PPCM/4, PPCM/2, PPCM},
		   { 0, PPI/32, PPI/16, PPI/8, PPI/4, PPI/2},
		   { 0, PPI/40, PPI/20, PPI/10, PPI/5, PPI/2},
		   };
int		grid_spacing[NUM_GRID_UNITS][GRID_4] = {
		   { PPCM/5, PPCM/2, PPCM, PPCM*2},		/* 2mm, 5mm, 10mm, 20mm */
		   { PPI/8, PPI/4, PPI/2, PPI},			/* 1/8", 1/4", 1/2", 1" */
		   { PPI/10, PPI/5, PPI/2, PPI},		/* 1/10", 1/5", 1/2", 1" */
		   };

		/* the first entry in each unit category is only used for point positioning */
char	       *grid_name[NUM_GRID_UNITS][GRID_4+1] = {
		   { "1mm", "2 mm", "5 mm", "10 mm", "20 mm"},
		   { "1/16 inch", "1/8 inch", "1/4 inch", "1/2 inch", "1 inch"},
		   { "0.05 inch", "0.1 inch", "0.2 inch", "0.5 inch", "1.0 inch"},
		   };

float		cur_rotnangle = 90.0;
int		cur_linkmode = 0;
int		cur_numsides = 6;
int		cur_numcopies = 1;
int		cur_numxcopies = 0;
int		cur_numycopies = 0;
int		action_on = 0;
int		highlighting = 0;
int		aborting = 0;
int		anypointposn = 0;
int		figure_modified = 0;
char		cur_fig_units[200];
char		cur_library_dir[PATH_MAX];
char		cur_image_editor[PATH_MAX];
char		cur_spellchk[PATH_MAX];
char		cur_browser[PATH_MAX];
char		cur_pdfviewer[PATH_MAX];
Boolean		warnexist = False;

/**********************	 global mode variables	************************/

int		num_point;
int		min_num_points;

/***************************  Export Settings  ****************************/

int		cur_exp_lang;		/* gets initialized in main.c */
Boolean		batch_exists = False;
char		batch_file[32];

/*******************************************************************/
/* If you change the order of the lang_items[] you must change the */
/* order of the lang_texts[] and the LANG_xxx items in mode.h      */
/*******************************************************************/

char	       *lang_items[] = {
	"ps",    "eps",   "eps_ascii", "eps_mono_tiff", "eps_color_tiff",
	"pdf",   "pspdf", "box", "latex", "epic", "eepic", "eepicemu",
	"pstex", "pdftex","pspdftex", "pictex", "hpl", "textyl",
	"tpic",  "pic",   "map",    "mf",
	"mp",    "mmp",   "cgm",    "bcgm", "emf",   "tk", "shape", "svg",
/* bitmap formats start here */
	"gif",  
	"jpeg",
	"pcx",  "png",   "ppm", "sld", "tiff", "xbm", 
#ifdef USE_XPM
	"xpm",
#endif /* USE_XPM */
    };

char	       *lang_texts[] = {
	"Postscript                          ",
	"EPS (Encapsulated Postscript)       ",
	"EPS with ASCII (EPSI) preview       ",
	"EPS with Monochrome TIFF preview    ",
	"EPS with Color TIFF preview         ",
	"PDF (Portable Document Format)      ",
	"EPS and PDF (two files)             ",
	"LaTeX box (figure boundary)         ",
	"LaTeX picture                       ",
	"LaTeX picture + epic macros         ",
	"LaTeX picture + eepic macros        ",
	"LaTeX picture + eepicemu macros     ",
	"Combined PS/LaTeX (both parts)      ",
	"Combined PDF/LaTeX (both parts)     ",
	"Combined PS/PDF/LaTeX (3 parts)     ",
	"PiCTeX macros                       ",
	"HPGL/2 (or IBMGL)                   ",
	"Textyl \\special commands            ",
	"TPIC                                ",
	"PIC                                 ",
	"HTML Image Map                      ",
	"MF  (MetaFont)                      ",
	"MP  (MetaPost)                      ",
	"MMP (Multi MetaPost)                ",
	"CGM (Computer Graphics Metafile)    ",
	"Binary CGM                          ",
	"EMF (Enhanced Metafile)             ",
	"Tk  (Tcl/Tk toolkit)                ",
	"SHAPE (ShapePar definition )        ", 
	"SVG (Scalable Vector Graphics; beta)",

	/*** bitmap formats follow ***/
	/* if you move GIF, change FIRST_BITMAP_LANG in mode.h */

	"GIF  (Graphics Interchange Format)  ",
	"JPEG (Joint Photo. Expert Group     ",
	"PCX  (PC Paintbrush)                ",
	"PNG  (Portable Network Graphics)    ",
	"PPM  (Portable Pixmap)              ",
	"SLD  (AutoCad Slide)                ",
	"TIFF (no compression)               ",
	"XBM  (X11 Bitmap)                   ",
#ifdef USE_XPM
	"XPM  (X11 Pixmap)                   ",
#endif /* USE_XPM */
    };

/***************************  Mode Settings  ****************************/

int		cur_objmask = M_NONE;
int		cur_updatemask = I_UPDATEMASK;
int		new_objmask = M_NONE;
/* start depth at 50 to make it easier to put new objects on top without
   having to remember to increase the depth at startup */
int		cur_depth = DEF_DEPTH;

/***************************  Texts ****************************/

int		hidden_text_length;
float		cur_textstep = 1.0;
int		cur_fontsize = DEF_FONTSIZE;
int		cur_latex_font	= 0;
int		cur_ps_font	= 0;
int		cur_textjust	= T_LEFT_JUSTIFIED;
int		cur_textflags	= PSFONT_TEXT;

/***************************  Lines ****************************/

int		cur_linewidth	= 1;
int		cur_linestyle	= SOLID_LINE;
int		cur_joinstyle	= JOIN_MITER;
int		cur_capstyle	= CAP_BUTT;
float		cur_dashlength	= DEF_DASHLENGTH;
float		cur_dotgap	= DEF_DOTGAP;
float		cur_styleval	= 0.0;
Color		cur_pencolor	= BLACK;
Color		cur_fillcolor	= WHITE;
int		cur_boxradius	= DEF_BOXRADIUS;
int		cur_fillstyle	= UNFILLED;
int		cur_arrowmode	= L_NOARROWS;
int		cur_arrowtype	= 0;
float		cur_arrowthick	= 1.0;			/* pixels */
float		cur_arrowwidth	= DEF_ARROW_WID;	/* pixels */
float		cur_arrowheight	= DEF_ARROW_HT;		/* pixels */
float		cur_arrow_multthick = 1.0;		/* when using multiple of width */
float		cur_arrow_multwidth = DEF_ARROW_WID;
float		cur_arrow_multheight = DEF_ARROW_HT;
Boolean		use_abs_arrowvals = False;		/* start with values prop. to width */
int		cur_arctype	= T_OPEN_ARC;
float		cur_tangnormlen = 1.0;			/* current tangent/normal line length */

/*************************** Dimension lines ****************************/

int		cur_dimline_thick = 1;		/* main line and tick thickness */
int		cur_dimline_style = SOLID_LINE;	/* main line style */
int		cur_dimline_color = BLACK;	/* color */
int		cur_dimline_leftarrow = 2;	/* arrow type (black triangle) */
int		cur_dimline_rightarrow = 2;	/* other end */
float		cur_dimline_arrowlength = DEF_ARROW_HT;	/* as a multiple of line thickness */
float		cur_dimline_arrowwidth = DEF_ARROW_WID;	/* as a multiple of line thickness */
Boolean		cur_dimline_ticks = True;	/* Include ticks */
int		cur_dimline_tickthick = 1;	/* thickness of ticks */
int		cur_dimline_boxthick = 1;	/* thickness of text box line */
int		cur_dimline_boxcolor = WHITE;	/* start with white-filled text box */
int		cur_dimline_textcolor = BLACK;	/* text color */
int		cur_dimline_font = 0;		/* Times Roman */
int		cur_dimline_fontsize = DEF_FONTSIZE; /* size of text */
int		cur_dimline_psflag = 1;
Boolean		cur_dimline_fixed = False;	/* text is adjusted to report line length */
int		cur_dimline_prec = 1;		/* number of dec points in dimension line text */

/**************************** Miscellaneous *****************************/

float		cur_elltextangle = 0.0;		/* text/ellipse input angle */
char		EMPTY_PIC[8]	= "<empty>";

/***************************  File Settings  ****************************/

char		cur_file_dir[PATH_MAX];
char		cur_export_dir[PATH_MAX];
char		cur_filename[PATH_MAX] = "";
char		save_filename[PATH_MAX] = "";	/* to undo load */
char		file_header[32];
char		cut_buf_name[PATH_MAX];		/* path of .xfig cut buffer file */
char		xfigrc_name[PATH_MAX];		/* path of .xfigrc file */

/*************************** routines ***********************/

void
reset_modifiedflag(void)
{
    figure_modified = 0;
}

void
set_modifiedflag(void)
{
    figure_modified = 1;
}

void
set_action_on(void)
{
    action_on = 1;
}

void
reset_action_on(void)
{
    action_on = 0;
    /* reset this so next show_linelengths will work properly */
    first_lenmsg = True;
}
