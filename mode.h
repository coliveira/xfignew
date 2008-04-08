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

#ifndef MODE_H
#define MODE_H

#define		F_NULL			0
#define	    FIRST_DRAW_MODE	    F_CIRCLE_BY_RAD
#define		F_CIRCLE_BY_RAD		1
#define		F_CIRCLE_BY_DIA		2
#define		F_ELLIPSE_BY_RAD	3
#define		F_ELLIPSE_BY_DIA	4
#define		F_CIRCULAR_ARC		5
#define		F_POLYLINE		6
#define		F_BOX			7
#define		F_POLYGON		8
#define		F_TEXT			9
#define		F_APPROX_SPLINE		10
#define		F_CLOSED_APPROX_SPLINE	11
#define		F_INTERP_SPLINE		12
#define		F_CLOSED_INTERP_SPLINE	13
#define		F_ARCBOX		14
#define		F_REGPOLY		15
#define		F_PICOBJ		16
#define		F_PLACE_LIB_OBJ		17

#define	    FIRST_EDIT_MODE	    F_GLUE
#define		F_GLUE			30
#define		F_BREAK			31
#define		F_SCALE			32
#define		F_ADD			33
#define		F_COPY			34
#define		F_MOVE			35
#define		F_DELETE		36
#define		F_MOVE_POINT		37
#define		F_DELETE_POINT		38
#define		F_ADD_POINT		39
#define		F_DELETE_ARROW_HEAD	40
#define		F_ADD_ARROW_HEAD	41
#define		F_FLIP			42
#define		F_ROTATE		43
#define		F_AUTOARROW		44
#define		F_CONVERT		45
#define		F_EDIT			46
#define		F_UPDATE		47
#define		F_ALIGN			48
#define		F_ZOOM			49
#define		F_LOAD			50
#define		F_ENTER_COMP		51
#define		F_EXIT_COMP		52
#define		F_EXIT_ALL_COMP		53
#define		F_OPEN_CLOSE		54
#define		F_SPLIT			55
#define		F_JOIN			56
#define		F_TANGENT		57
#define		F_ANGLEMEAS		58
#define		F_LENMEAS		59
#define		F_AREAMEAS		60
#define		F_PASTE			61
#define		F_CHOP			62

extern int	cur_mode;

/* alignment mode */
#define		ALIGN_NONE		0
#define		ALIGN_LEFT		1
#define		ALIGN_TOP		1
#define		ALIGN_CENTER		2
#define		ALIGN_RIGHT		3
#define		ALIGN_BOTTOM		3
#define		ALIGN_DISTRIB_C		4
#define		ALIGN_DISTRIB_E		5
#define		ALIGN_ABUT		6

extern int	cur_halign;
extern int	cur_valign;

/* angle geometry */
#define		L_UNCONSTRAINED		0
#define		L_LATEXLINE		1
#define		L_LATEXARROW		2
#define		L_MOUNTHATTAN		3
#define		L_MANHATTAN		4
#define		L_MOUNTAIN		5

extern int	manhattan_mode;
extern int	mountain_mode;
extern int	latexline_mode;
extern int	latexarrow_mode;

/* arrow mode */
#define		L_NOARROWS		0
#define		L_FARROWS		1
#define		L_FBARROWS		2
#define		L_BARROWS		3

extern int	autoforwardarrow_mode;
extern int	autobackwardarrow_mode;

/* grid subunit modes (mm, 1/16", 1/10") */
#define		NUM_GRID_UNITS		3
enum 		{ MM_UNIT, FRACT_UNIT, TENTH_UNIT };

/* grid mode */
#define		GRID_0			0
#define		GRID_1			1
#define		GRID_2			2
#define		GRID_3			3
#define		GRID_4			4

extern int	cur_gridmode, cur_gridunit, old_gridunit, grid_unit;
extern int	grid_spacing[NUM_GRID_UNITS][GRID_4];
extern char    *grid_name[NUM_GRID_UNITS][GRID_4+1];

/* point position */
#define		P_ANY			0
#define		P_MAGNET		1
#define		P_GRID1			2
#define		P_GRID2			3
#define		P_GRID3			4
#define		P_GRID4			5

extern int	cur_pointposn;
extern int	posn_rnd[NUM_GRID_UNITS][P_GRID4+1];
extern int	posn_hlf[NUM_GRID_UNITS][P_GRID4+1];

/* rotn axis */
#define		UD_FLIP			1
#define		LR_FLIP			2

extern float	cur_rotnangle;

/* smart link mode */
#define		SMART_OFF		0
#define		SMART_MOVE		1
#define		SMART_SLIDE		2

extern int	cur_linkmode;

/* misc */
extern int	action_on;
extern int	highlighting;
extern int	aborting;
extern int	anypointposn;
extern int	figure_modified;
extern int	cur_numsides;
extern int	cur_numcopies;
extern int	cur_numxcopies;
extern int	cur_numycopies;
extern char	cur_fig_units[200];
extern char	cur_library_dir[PATH_MAX];
extern char	cur_image_editor[PATH_MAX];
extern char	cur_spellchk[PATH_MAX];
extern char	cur_browser[PATH_MAX];
extern char	cur_pdfviewer[PATH_MAX];
extern Boolean	warnexist;

extern void	reset_modifiedflag(void);
extern void	set_modifiedflag(void);
extern void	reset_action_on(void);
extern void	set_action_on(void);

/**********************	 global mode variables	************************/

extern int	num_point;
extern int	min_num_points;

/***************************  Export Settings  ****************************/

/*********************************************************************/
/* If you change the order of the LANG_xxx you must change the order */
/* of the lang_texts[] and the LANG_items[] items in mode.c          */
/*********************************************************************/

/* position of languages starting from 0 */
enum {
	LANG_PS,
	LANG_EPS,
	LANG_EPS_ASCII,
	LANG_EPS_MONO_TIFF,
	LANG_EPS_COLOR_TIFF,
	LANG_PDF,
	LANG_PSPDF,
	LANG_BOX,
	LANG_LATEX,
	LANG_EPIC,
	LANG_EEPIC,
	LANG_EEPICEMU,
	LANG_PSTEX,
	LANG_PDFTEX,
	LANG_PSPDFTEX,
	LANG_PICTEX,
	LANG_IBMGL,
	LANG_TEXTYL,
	LANG_TPIC,
	LANG_PIC,
	LANG_MAP,
	LANG_MF,
	LANG_MP,
	LANG_MMP,
	LANG_CGM,
	LANG_BCGM,	/* binary cgm */
	LANG_EMF,
	LANG_TK,
	LANG_SHAPE,	/* ShapePar definition */
	LANG_SVG,
/* bitmap formats should follow here, starting with GIF */
	LANG_GIF,
	LANG_JPEG,
	LANG_PCX,
	LANG_PNG,
	LANG_PPM,
	LANG_SLD,
	LANG_TIFF,
	LANG_XBM,
#ifdef USE_XPM
	LANG_XPM,
#endif
	END_OF_LANGS
};

/* important for the menu dividing line */
#define FIRST_BITMAP_LANG LANG_GIF

/* number of export languages */

#define NUM_EXP_LANG	END_OF_LANGS

extern int	cur_exp_lang;
extern char    *lang_items[NUM_EXP_LANG];
extern char    *lang_texts[NUM_EXP_LANG];
extern Boolean  batch_exists;
extern char     batch_file[];

/***************************  Mode Settings  ****************************/

extern int	cur_objmask;
extern int	cur_updatemask;
extern int	new_objmask;
extern int	cur_depth;

/***************************  Text Settings  ****************************/

extern int	hidden_text_length;
extern float	cur_textstep;
extern int	cur_fontsize;
extern int	cur_latex_font;
extern int	cur_ps_font;
extern int	cur_textjust;
extern int	cur_textflags;

/*******************************  Lines *********************************/

extern int	cur_linewidth;
extern int	cur_linestyle;
extern int	cur_joinstyle;
extern int	cur_capstyle;
extern float	cur_dashlength;
extern float	cur_dotgap;
extern float	cur_styleval;
extern Color	cur_pencolor;
extern Color	cur_fillcolor;
extern int	cur_boxradius;
extern int	cur_fillstyle;
extern int	cur_arrowmode;
extern int	cur_arrowtype;
extern float	cur_arrowwidth;
extern float	cur_arrowheight;
extern float	cur_arrowthick;
extern float	cur_arrow_multwidth;
extern float	cur_arrow_multheight;
extern float	cur_arrow_multthick;
extern Boolean	use_abs_arrowvals;
extern int	cur_arctype;
extern float	cur_tangnormlen;

/*************************** Dimension lines ****************************/

extern int	cur_dimline_thick;
extern int	cur_dimline_style;
extern int	cur_dimline_color;
extern int	cur_dimline_leftarrow;
extern int	cur_dimline_rightarrow;
extern float	cur_dimline_arrowlength;
extern float	cur_dimline_arrowwidth;
extern Boolean	cur_dimline_ticks;
extern int	cur_dimline_tickthick;
extern int	cur_dimline_boxthick;
extern int	cur_dimline_boxcolor;
extern int	cur_dimline_textcolor;
extern int	cur_dimline_font;
extern int	cur_dimline_fontsize;
extern int	cur_dimline_psflag;
extern Boolean	cur_dimline_fixed;
extern int	cur_dimline_prec;

/**************************** Miscellaneous *****************************/

extern float	cur_elltextangle;	/* text/ellipse input angle */
extern char	EMPTY_PIC[8];

/***************************  File Settings  ****************************/

extern char	cur_file_dir[];
extern char	cur_export_dir[];
extern char	cur_filename[];
extern char	save_filename[];	/* to undo load or "new" command */
extern char	file_header[];
extern char	cut_buf_name[];		/* path of .xfig cut buffer file */
extern char	xfigrc_name[];		/* path of .xfigrc file */

#endif /* MODE_H */
