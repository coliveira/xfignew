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
#include "figx.h"
#include "version.h"
#include "patchlevel.h"
#include "resources.h"
#include "object.h"
#include "main.h"
#include "mode.h"
#include "d_text.h"
#include "e_edit.h"
#include "f_read.h"
#include "f_util.h"
#include "u_error.h"
#include "u_fonts.h"
#include "u_redraw.h"
#include "u_undo.h"
#include "w_canvas.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_cursor.h"
#include "w_cmdpanel.h"
#include "w_digitize.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_fontpanel.h"
#include "w_export.h"
#include "w_help.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_library.h"
#include "w_msgpanel.h"
#include "w_modepanel.h"
#include "w_mousefun.h"
#include "w_print.h"
#include "w_rulers.h"
#include "w_srchrepl.h"
#include "w_setup.h"
#include "w_style.h"
#include "w_util.h"
#include "w_zoom.h"
#include "w_snap.h"
#include "f_load.h"
}
	
#include <X11/IntrinsicP.h>

/* input extensions for an input tablet */
#ifdef USE_TAB
#include "X11/extensions/XInput.h"
#endif /* USE_TAB */

#ifdef I18N
#include <X11/keysym.h>
#endif  /* I18N */

/* EXPORTS */

Boolean	    geomspec; 

/* LOCALS */

static int	screen_res;
static void	make_cut_buf_name(void);
static void	check_resource_ranges(void);
static void	set_icon_geom(void);
static void	set_max_image_colors(void);
static void	parse_canvas_colors(void);
static void	set_xpm_icon(void);
static void	resize_canvas(void);
static void check_refresh(XtPointer client_data, XtIntervalId *id);

/************** FIG options ******************/

DeclareStaticArgs(10);
char    *arg_filename = NULL;

static Boolean	TRue = True;
static Boolean	FAlse = False;
static float	Fzero = 0.0;
static float	Fone = 1.0;
static float	F100 = 100.0;
static float	FDef_arrow_wd = DEF_ARROW_WID;
static float	FDef_arrow_ht = DEF_ARROW_HT;

/* actions so that we may install accelerators at the top level */
static XtActionsRec	main_actions[] =
{
    {"LoadRecent",	(XtActionProc) acc_load_recent_file},
    {"DoSave",		(XtActionProc) do_save},
    {"Quit",		(XtActionProc) quit},
    {"New",		(XtActionProc) New},
    {"DeleteAll",	(XtActionProc) delete_all_cmd},
    {"SaveAs",		(XtActionProc) popup_saveas_panel},
    {"OpenFile",	(XtActionProc) popup_open_panel},
    {"MergeFile",	(XtActionProc) popup_merge_panel},
    {"PopupDigitize",	(XtActionProc) popup_digitize_panel},
    {"PopupExport",	(XtActionProc) popup_export_panel},
    {"ExportFile",	(XtActionProc) do_export},
    {"PopupPrint",	(XtActionProc) popup_print_panel},
    {"PrintFile",	(XtActionProc) do_print},
    {"PopupCharmap",	(XtActionProc) popup_character_map},
    {"PopupGlobals",	(XtActionProc) show_global_settings},
    {"Undo",		(XtActionProc) undo},
    {"Paste",		(XtActionProc) paste},
    {"SpellCheck",	(XtActionProc) spell_check},
    {"Search",		(XtActionProc) popup_search_panel},
    {"ChangeOrient",	(XtActionProc) change_orient},
    {"Redraw",		(XtActionProc) redisplay_canvas},
    {"LeaveCmdSw",	(XtActionProc) clear_mousefun},
    {"RefMan",		(XtActionProc) launch_refman},
    {"Man",		(XtActionProc) launch_man},
    {"HowToGuide",	(XtActionProc) launch_howto},
    {"AboutXfig",	(XtActionProc) launch_about},
    {"SpinnerUpDown",	(XtActionProc) spinner_up_down},
};

static XtResource application_resources[] = {
    {"geometry",  "XtCGeometry",    XtRString,  sizeof(char *),
      XtOffset(appresPtr, geometry), XtRString, (caddr_t) NULL},

    {"version",  "version",    XtRString,  sizeof(char *),
      XtOffset(appresPtr,version), XtRString, (caddr_t) NULL},
    {"zoom", "Zoom", XtRFloat, sizeof(float),
      XtOffset(appresPtr, zoom), XtRFloat, (caddr_t) & Fone},
    {"allownegcoords", "NegativeCoordinates",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, allownegcoords), XtRBoolean, (caddr_t) & TRue},
    {"showaxislines", "Axis",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, showaxislines), XtRBoolean, (caddr_t) & TRue},
    {"canvasbackground",  "Background",    XtRString,  sizeof(char *),
      XtOffset(appresPtr,canvasbackground), XtRString, (caddr_t) NULL},
    {"canvasforeground",  "Foreground",    XtRString,  sizeof(char *),
      XtOffset(appresPtr,canvasforeground), XtRString, (caddr_t) NULL},
    {"iconGeometry",  "IconGeometry",    XtRString,  sizeof(char *),
      XtOffset(appresPtr,iconGeometry), XtRString, (caddr_t) NULL},
    {"showallbuttons", "ShowAllButtons",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, showallbuttons), XtRBoolean, (caddr_t) & FAlse},
    {XtNjustify,   XtCJustify, XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, RHS_PANEL), XtRBoolean, (caddr_t) & FAlse},
    {"landscape",   XtCOrientation, XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, landscape), XtRBoolean, (caddr_t) & TRue},
    {"pwidth",   XtCWidth, XtRFloat, sizeof(float),
      XtOffset(appresPtr, tmp_width), XtRFloat, (caddr_t) & Fzero},
    {"pheight",   XtCHeight, XtRFloat, sizeof(float),
      XtOffset(appresPtr, tmp_height), XtRFloat, (caddr_t) & Fzero},
    {"trackCursor", "Track",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, tracking), XtRBoolean, (caddr_t) & TRue},
    {"inches", "Inches",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, INCHES), XtRBoolean, (caddr_t) & TRue},
    {"boldFont", "Font",   XtRString, sizeof(char *),
      XtOffset(appresPtr, boldFont), XtRString, (caddr_t) NULL},
    {"normalFont", "Font",   XtRString, sizeof(char *),
      XtOffset(appresPtr, normalFont), XtRString, (caddr_t) NULL},
    {"buttonFont", "Font",   XtRString, sizeof(char *),
      XtOffset(appresPtr, buttonFont), XtRString, (caddr_t) NULL},
    {"startarrowtype", "StartArrowType",   XtRInt, sizeof(int),
      XtOffset(appresPtr, startarrowtype), XtRImmediate, (caddr_t) 0},
    {"startarrowthick", "StartArrowThick",   XtRFloat, sizeof(float),
      XtOffset(appresPtr, startarrowthick), XtRFloat, (caddr_t) & Fone},
    {"startarrowwidth", "StartArrowWidth",   XtRFloat, sizeof(float),
      XtOffset(appresPtr, startarrowwidth), XtRFloat, (caddr_t) & FDef_arrow_wd},
    {"startarrowlength", "StartArrowLength",   XtRFloat, sizeof(float),
      XtOffset(appresPtr, startarrowlength), XtRFloat, (caddr_t) & FDef_arrow_ht},
    {"startlatexFont", "StartlatexFont",   XtRString, sizeof(char *),
      XtOffset(appresPtr, startlatexFont), XtRString, (caddr_t) NULL},
    {"startpsFont", "StartpsFont",   XtRString, sizeof(char *),
      XtOffset(appresPtr, startpsFont), XtRString, (caddr_t) NULL},
    {"startfontsize", "StartFontSize",   XtRFloat, sizeof(float),
      XtOffset(appresPtr, startfontsize), XtRFloat, (caddr_t) & Fzero},
    {"internalborderwidth", "InternalBorderWidth",   XtRInt, sizeof(int),
      XtOffset(appresPtr, internalborderwidth), XtRImmediate, (caddr_t) 0},
    {"starttextstep", "StartTextStep",   XtRFloat, sizeof(float),
      XtOffset(appresPtr, starttextstep), XtRFloat, (caddr_t) & Fzero},
    {"startfillstyle", "StartFillStyle",   XtRInt, sizeof(int),
      XtOffset(appresPtr, startfillstyle), XtRImmediate, (caddr_t) DEFAULT},
    {"startlinewidth", "StartLineWidth",   XtRInt, sizeof(int),
      XtOffset(appresPtr, startlinewidth), XtRImmediate, (caddr_t) 1},
    {"grid_color", "Color",   XtRString, sizeof(char *),
      XtOffset(appresPtr, grid_color), XtRImmediate, (caddr_t) "#ffcccc"},
    {"grid_unit", "UnitType",   XtRString, sizeof(char *),
      XtOffset(appresPtr, tgrid_unit), XtRImmediate, (caddr_t) "default"},
    {"startgridmode", "StartGridMode",   XtRInt, sizeof(int),
      XtOffset(appresPtr, startgridmode), XtRImmediate, (caddr_t) 0},
    {"startposnmode", "StartPosnMode",   XtRInt, sizeof(int),
      XtOffset(appresPtr, startposnmode), XtRImmediate, (caddr_t) 1 },
    {"latexfonts", "Latexfonts",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, latexfonts), XtRBoolean, (caddr_t) & FAlse},
    {"hiddentext", "HiddenText",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, hiddentext), XtRBoolean, (caddr_t) & FAlse},
    {"rigidtext", "RigidText",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, rigidtext), XtRBoolean, (caddr_t) & FAlse},
    {"specialtext", "SpecialText",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, specialtext), XtRBoolean, (caddr_t) & FAlse},
    {"scalablefonts", "ScalableFonts",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, scalablefonts), XtRBoolean, (caddr_t) & TRue},
    {"monochrome", "Monochrome",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, monochrome), XtRBoolean, (caddr_t) & FAlse},
    {"latexfonts", "Latexfonts",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, latexfonts), XtRBoolean, (caddr_t) & FAlse},
    {"keyFile", "KeyFile",   XtRString, sizeof(char *),
      XtOffset(appresPtr, keyFile), XtRString, (caddr_t) "CompKeyDB"},
    {"exportLanguage", "ExportLanguage",   XtRString, sizeof(char *),
      XtOffset(appresPtr, exportLanguage), XtRString, (caddr_t) "eps"},
    {"flushleft", "FlushLeft",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, flushleft), XtRBoolean, (caddr_t) & FAlse},
    {"userscale", "UserScale",   XtRFloat, sizeof(float),
      XtOffset(appresPtr, userscale), XtRFloat, (caddr_t) & Fone},
    {"userunit", "UserUnit",   XtRString, sizeof(char *),
      XtOffset(appresPtr, userunit), XtRString, (caddr_t) ""},
    {"but_per_row", "But_per_row",   XtRInt, sizeof(int),
      XtOffset(appresPtr, but_per_row), XtRImmediate, (caddr_t) 0},
    {"max_image_colors", "Max_image_colors", XtRInt, sizeof(int),
      XtOffset(appresPtr, max_image_colors), XtRImmediate, (caddr_t) 0},
    {"installowncmap", "Installcmap", XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, installowncmap), XtRBoolean, (caddr_t) & FAlse},
    {"dontswitchcmap", "Dontswitchcmap", XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, dontswitchcmap), XtRBoolean, (caddr_t) & FAlse},
    {"tablet", "Tablet", XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, tablet), XtRBoolean, (caddr_t) & FAlse},
    {"rulerthick", "RulerThick", XtRInt, sizeof(int),
      XtOffset(appresPtr, rulerthick), XtRImmediate, (caddr_t) 0},
    {"image_editor", "ImageEditor", XtRString, sizeof(char *),
      XtOffset(appresPtr, image_editor), XtRString, (caddr_t) "xv"},
    {"magnification", "Magnification", XtRFloat, sizeof(float),
      XtOffset(appresPtr, magnification), XtRFloat, (caddr_t) & F100},
    {"paper_size", "Papersize", XtRString, sizeof(char *),
      XtOffset(appresPtr, paper_size), XtRString, (caddr_t) NULL},
    {"multiple",   XtCOrientation, XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, multiple), XtRBoolean, (caddr_t) & FAlse},
    {"overlap",   XtCOrientation, XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, overlap), XtRBoolean, (caddr_t) & FAlse},
    {"showballoons",   "ShowBalloons", XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, showballoons), XtRBoolean, (caddr_t) & TRue},
    {"balloon_delay", "balloonDelay", XtRInt, sizeof(int),
      XtOffset(appresPtr, balloon_delay), XtRImmediate, (caddr_t) 500},
    {"spellcheckcommand", "spellCheckCommand",   XtRString, sizeof(char *),
      XtOffset(appresPtr, spellcheckcommand), XtRString, (caddr_t) "spell %f"},
    {"jpeg_quality", "Quality", XtRInt, sizeof(int),
      XtOffset(appresPtr, jpeg_quality), XtRImmediate, (caddr_t) 0},
    {"transparent", "Transparent", XtRInt, sizeof(int),
      XtOffset(appresPtr, transparent), XtRImmediate, (caddr_t) TRANSP_NONE },
    {"library_dir", "Directory", XtRString, sizeof(char *),
      XtOffset(appresPtr, library_dir), XtRString, (caddr_t) OBJLIBDIR},
    {"debug", "Debug",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, DEBUG), XtRBoolean, (caddr_t) & FAlse},
    {"showlengths", "Debug",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, showlengths), XtRBoolean, (caddr_t) & FAlse},
    {"shownums", "Debug",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, shownums), XtRBoolean, (caddr_t) & FAlse},
    {"showpageborder", "Debug",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, show_pageborder), XtRBoolean, (caddr_t) & TRue},
    {"pageborder", "Color",   XtRString, sizeof(char *),
      XtOffset(appresPtr, pageborder), XtRString, (caddr_t) "lightblue"},
    {"browser", "Browser", XtRString, sizeof(char *),
      XtOffset(appresPtr, browser), XtRString, (caddr_t) "netscape"},
    {"pdfviewer", "Viewer", XtRString, sizeof(char *),
      XtOffset(appresPtr, pdf_viewer), XtRString, (caddr_t) "acroread"},
    {"spinner_delay", "spinnerDelay",   XtRInt, sizeof(int),
      XtOffset(appresPtr, spinner_delay), XtRImmediate, (caddr_t) 500},
    {"spinner_rate", "spinnerRate",   XtRInt, sizeof(int),
      XtOffset(appresPtr, spinner_rate), XtRImmediate, (caddr_t) 100},
    {"export_margin", "Margin",   XtRInt, sizeof(int),
      XtOffset(appresPtr, export_margin), XtRImmediate, (caddr_t) DEF_EXPORT_MARGIN},
    {"showdepthmanager", "Hints",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, showdepthmanager), XtRBoolean, (caddr_t) & TRue},
    {"flipvisualhints", "Hints",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, flipvisualhints), XtRBoolean, (caddr_t) & FAlse},
    {"smooth_factor", "Smooth",   XtRInt, sizeof(int),
      XtOffset(appresPtr, smooth_factor), XtRImmediate, (caddr_t) 0},
    {"icon_view", "View",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, icon_view), XtRBoolean, (caddr_t) & TRue},
    {"library_icon_size", "Dimension",   XtRInt, sizeof(int),
      XtOffset(appresPtr, library_icon_size), XtRImmediate, (caddr_t) DEF_ICON_SIZE},
    {"splash", "View",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, splash), XtRBoolean, (caddr_t) & TRue},
    {"axislines", "Color",   XtRString, sizeof(char *),
      XtOffset(appresPtr, axislines), XtRString, (caddr_t) "pink"},
    {"freehand_resolution", "Hints",   XtRInt, sizeof(int),
      XtOffset(appresPtr, freehand_resolution), XtRImmediate, (caddr_t) 25},
    {"ghostscript", "Ghostscript",   XtRString, sizeof(char *),
      XtOffset(appresPtr, ghostscript), XtRString, (caddr_t) "gs"},
    {"correct_font_size", "Size",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, correct_font_size), XtRBoolean, (caddr_t) & TRue},
    {"encoding", "Encoding", XtRInt, sizeof(int),
      XtOffset(appresPtr, encoding), XtRImmediate, (caddr_t) 1},
    {"write_v40", "Format",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, write_v40), XtRBoolean, (caddr_t) & FAlse},
    {"crosshair", "Crosshair",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, crosshair), XtRBoolean, (caddr_t) & FAlse},
    {"autorefresh", "Refresh",   XtRBoolean, sizeof(Boolean),
      XtOffset(appresPtr, autorefresh), XtRBoolean, (caddr_t) & FAlse},

#ifdef I18N
    {"international", "International", XtRBoolean, sizeof(Boolean),
       XtOffset(appresPtr, international), XtRBoolean, (caddr_t) & false},
    {"fontMenulanguage", "Language", XtRString, sizeof(char *),
       XtOffset(appresPtr, font_menu_language), XtRString, (caddr_t) ""},
    {"eucEncoding", "EucEncoding", XtRBoolean, sizeof(Boolean),
       XtOffset(appresPtr, euc_encoding), XtRBoolean, (caddr_t) & true},
    {"latinKeyboard", "Latinkeyboard", XtRBoolean, sizeof(Boolean),
       XtOffset(appresPtr, latin_keyboard), XtRBoolean, (caddr_t) & false},
    {"alwaysUseFontSet", "AlwaysUseFontSet", XtRBoolean, sizeof(Boolean),
       XtOffset(appresPtr, always_use_fontset), XtRBoolean, (caddr_t) & false},
    {"fixedFontSet", "FontSet", XtRFontSet, sizeof(XFontSet),
       XtOffset(appresPtr, fixed_fontset), XtRString,
       (caddr_t) "-*-times-medium-r-normal--16-*-*-*-*-*-*-*,"
	 "-*-*-medium-r-normal--16-*-*-*-*-*-*-*,*--16-*" },
    {"normalFontSet", "NormalFontSet", XtRFontSet, sizeof(XFontSet),
       XtOffset(appresPtr, normal_fontset), XtRString,
       (caddr_t) "-*-times-medium-r-normal--16-*-*-*-*-*-*-*,"
	 "-*-*-medium-r-normal--16-*-*-*-*-*-*-*,"
	 "-*-*-*-r-*--16-*-*-*-*-*-*-*" },
    {"boldFontSet", "BoldFontSet", XtRFontSet, sizeof(XFontSet),
       XtOffset(appresPtr, bold_fontset), XtRString,
       (caddr_t) "-*-times-bold-r-normal--16-*-*-*-*-*-*-*,"
	 "-*-*-bold-r-normal--16-*-*-*-*-*-*-*,"
	 "-*-*-*-r-*--16-*-*-*-*-*-*-*" },
    {"fontSetSize", "FontSetSize", XtRInt, sizeof(int),
       XtOffset(appresPtr, fontset_size), XtRImmediate, (caddr_t)0 },
    {"inputStyle", "InputStyle", XtRString, sizeof(char *),
       XtOffset(appresPtr, xim_input_style), XtRString, (caddr_t) "OffTheSpot"},
    {"fig2devLocalizeOption", "Fig2devLocalizeOption", XtRString, sizeof(char *),
       XtOffset(appresPtr, fig2dev_localize_option), XtRString, (caddr_t) "-j"},
#ifdef I18N_USE_PREEDIT
    {"textPreedit", "TextPreedit", XtRString, sizeof(char *),
       XtOffset(appresPtr, text_preedit), XtRString, (caddr_t) ""},
#endif /* I18N_USE_PREEDIT */
#endif  /* I18N */
};

/* BE SURE TO UPDATE THE -help COMMAND OPTION LIST IF ANY CHANGES ARE MADE HERE */

XrmOptionDescRec options[] =
{
    {"-visual", "*visual", XrmoptionSepArg, NULL},
    {"-depth", "*depth", XrmoptionSepArg, NULL},

    {"-allownegcoords", ".allownegcoords", XrmoptionNoArg, "True"},
    {"-autorefresh", ".autorefresh", XrmoptionNoArg, "True"},
    {"-balloon_delay", ".balloon_delay", XrmoptionSepArg, 0},
    {"-boldFont", ".boldFont", XrmoptionSepArg, 0},
    {"-buttonFont", ".buttonFont", XrmoptionSepArg, 0},
    {"-but_per_row", ".but_per_row", XrmoptionSepArg, 0},
    {"-cbg", ".canvasbackground", XrmoptionSepArg, (caddr_t) NULL},
    {"-center", ".flushleft", XrmoptionNoArg, "False"},
    {"-centimeters", ".inches", XrmoptionNoArg, "False"},
    {"-cfg", ".canvasforeground", XrmoptionSepArg, (caddr_t) NULL},
    {"-correct_font_size", ".correct_font_size", XrmoptionNoArg, "True"},
    {"-crosshair", ".crosshair", XrmoptionNoArg, "True"},
    {"-debug", ".debug", XrmoptionNoArg, "True"},
    {"-dontallownegcoords", ".allownegcoords", XrmoptionNoArg, "False"},
    {"-dontshowaxislines", ".showaxislines", XrmoptionNoArg, "False"},
    {"-dontshowballoons", ".showballoons", XrmoptionNoArg, "False"},
    {"-dontshowlengths", ".showlengths", XrmoptionNoArg, "False"},
    {"-dontshownums", ".shownums", XrmoptionNoArg, "False"},
    {"-dontshowpageborder", ".showpageborder", XrmoptionNoArg, "False"},
    {"-dontswitchcmap", ".dontswitchcmap", XrmoptionNoArg, "True"},
    {"-encoding", ".encoding", XrmoptionSepArg, 0},
    {"-exportLanguage", ".exportLanguage", XrmoptionSepArg, 0},
    {"-export_margin", ".export_margin", XrmoptionSepArg, 0},
    {"-flipvisualhints", ".flipvisualhints", XrmoptionNoArg, "True"},
    {"-noflipvisualhints", ".flipvisualhints", XrmoptionNoArg, "False"},
    {"-flushleft", ".flushleft", XrmoptionNoArg, "True"},
    {"-freehand_resolution", ".freehand_resolution", XrmoptionSepArg, 0},
    {"-ghostscript", ".ghostscript", XrmoptionSepArg, "gs"},
    {"-grid_color", ".grid_color", XrmoptionSepArg, "lightblue"},
    {"-grid_unit", ".grid_unit", XrmoptionSepArg, "default"},
    {"-hiddentext", ".hiddentext", XrmoptionNoArg, "True"},
    {"-dontshowdepthmanager", ".showdepthmanager", XrmoptionNoArg, "False"},
    {"-iconGeometry", ".iconGeometry", XrmoptionSepArg, (caddr_t) NULL},
    {"-icon_view", ".icon_view", XrmoptionNoArg, "True"},
    {"-image_editor", ".image_editor", XrmoptionSepArg, 0},
    {"-imperial", ".inches", XrmoptionNoArg, "True"},
    {"-inches", ".inches", XrmoptionNoArg, "True"},
    {"-installowncmap", ".installowncmap", XrmoptionNoArg, "True"},
    {"-internalBW", ".internalborderwidth", XrmoptionSepArg, 0},
    {"-jpeg_quality", ".jpeg_quality", XrmoptionSepArg, 0},
    {"-keyFile", ".keyFile", XrmoptionSepArg, 0},
    {"-Landscape", ".landscape", XrmoptionNoArg, "True"},
    {"-landscape", ".landscape", XrmoptionNoArg, "True"},
    {"-latexfonts", ".latexfonts", XrmoptionNoArg, "True"},
    {"-left", ".justify", XrmoptionNoArg, "False"},
    {"-library_dir", ".library_dir", XrmoptionSepArg, 0},
    {"-library_icon_size", ".library_icon_size", XrmoptionSepArg, 0},
    {"-list_view", ".icon_view", XrmoptionNoArg, "False"},
    {"-magnification", ".magnification", XrmoptionSepArg, 0},
    {"-max_image_colors", ".max_image_colors", XrmoptionSepArg, 0},
    {"-metric", ".inches", XrmoptionNoArg, "False"},
    {"-monochrome", ".monochrome", XrmoptionNoArg, "True"},
    {"-multiple", ".multiple", XrmoptionNoArg, "True"},
    {"-nooverlap", ".overlap", XrmoptionNoArg, "False"},
    {"-normalFont", ".normalFont", XrmoptionSepArg, 0},
    {"-noscalablefonts", ".scalablefonts", XrmoptionNoArg, "False"},
    {"-nosplash", ".splash", XrmoptionNoArg, "False"},
    {"-notrack", ".trackCursor", XrmoptionNoArg, "False"},
    {"-overlap", ".overlap", XrmoptionNoArg, "True"},
    {"-pageborder", ".pageborder", XrmoptionSepArg, (caddr_t) NULL},
    {"-paper_size", ".paper_size", XrmoptionSepArg, (caddr_t) NULL},
    {"-pheight", ".pheight", XrmoptionSepArg, 0},
    {"-Portrait", ".landscape", XrmoptionNoArg, "False"},
    {"-portrait", ".landscape", XrmoptionNoArg, "False"},
    {"-pwidth", ".pwidth", XrmoptionSepArg, 0},
    {"-right", ".justify", XrmoptionNoArg, "True"},
    {"-rigidtext", ".rigidtext", XrmoptionNoArg, "True"},
    {"-rulerthick", ".rulerthick", XrmoptionSepArg, 0},
    {"-scalablefonts", ".scalablefonts", XrmoptionNoArg, "True"},
    {"-scale_factor", ".scale_factor", XrmoptionSepArg, 0},
    {"-showallbuttons", ".showallbuttons", XrmoptionNoArg, "True"},
    {"-showballoons", ".showballoons", XrmoptionNoArg, "True"},
    {"-showdepthmanager", ".showdepthmanager", XrmoptionNoArg, "True"},
    {"-showlengths", ".showlengths", XrmoptionNoArg, "True"},
    {"-shownums", ".shownums", XrmoptionNoArg, "True"},
    {"-showpageborder", ".showpageborder", XrmoptionNoArg, "True"},
    {"-showaxislines", ".showaxislines", XrmoptionNoArg, "True"},
    {"-single", ".multiple", XrmoptionNoArg, "False"},
    {"-smooth_factor", ".smooth_factor", XrmoptionSepArg, 0},
    {"-specialtext", ".specialtext", XrmoptionNoArg, "True"},
    {"-spellcheckcommand", ".spellcheckcommand", XrmoptionSepArg, 0},
    {"-spinner_delay", ".spinner_delay", XrmoptionSepArg, 0},
    {"-spinner_rate", ".spinner_rate", XrmoptionSepArg, 0},
    {"-splash", ".splash", XrmoptionNoArg, "True"},
    {"-startfillstyle", ".startfillstyle", XrmoptionSepArg, 0},
    {"-startarrowtype", ".startarrowtype",  XrmoptionSepArg, 0},
    {"-startarrowthick", ".startarrowthick", XrmoptionSepArg, 0},
    {"-startarrowwidth", ".startarrowwidth", XrmoptionSepArg, 0},
    {"-startarrowlength", ".startarrowlength", XrmoptionSepArg, 0},
    {"-startFontSize", ".startfontsize", XrmoptionSepArg, 0},
    {"-startfontsize", ".startfontsize", XrmoptionSepArg, 0},
    {"-startgridmode", ".startgridmode",  XrmoptionSepArg, 0},
    {"-startlatexFont", ".startlatexFont", XrmoptionSepArg, 0},
    {"-startlinewidth", ".startlinewidth", XrmoptionSepArg, 0},
    {"-startposnmode", ".startposnmode",  XrmoptionSepArg, 0},
    {"-startpsFont", ".startpsFont", XrmoptionSepArg, 0},
    {"-starttextstep", ".starttextstep",  XrmoptionSepArg, 0},
    {"-tablet", ".tablet", XrmoptionNoArg, "True"},
    {"-track", ".trackCursor", XrmoptionNoArg, "True"},
    {"-transparent_color", ".transparent", XrmoptionSepArg, 0},
    {"-userscale", ".userscale", XrmoptionSepArg, 0},
    {"-write_v40", ".write_v40", XrmoptionNoArg, "True"},
    {"-userunit", ".userunit", XrmoptionSepArg, 0},
    {"-axislines", ".axislines", XrmoptionSepArg, "pink"},
    {"-zoom", ".zoom", XrmoptionSepArg, 0},
#ifdef I18N
    {"-international", ".international", XrmoptionNoArg, "True"},
    {"-inputStyle", ".inputStyle", XrmoptionSepArg, 0},
#endif  /* I18N */
};

char *help_list[] = {
	"[-allownegcoords] ",
	"[-autorefresh] ",
	"[-balloon_delay <delay>] ",
	"[-boldFont <font>] ",
	"[-but_per_row <number>] ",
	"[-buttonFont <font>] ",
	"[-cbg <color>] ",
	"[-center] ",
	"[-cfg <color>] ",
	"[-centimeters] ",
	"[-correct_font_size] ",
	"[-debug] ",
	"[-depth <visual_depth>] ",
	"[-dontallownegcoords] ",
	"[-dontshowaxislines] ",
	"[-dontshowballoons] ",
	"[-dontshowlengths] ",
	"[-dontshowpageborder] ",
	"[-dontshownums] ",
	"[-dontswitchcmap] ",
	"[-encoding <ISO-8859 encoding>] ",
	"[-exportLanguage <language>] ",
	"[-export_margin <pixels>] ",
	"[-flipvisualhints] ",
	"[-flushleft] ",
	"[-freehand_resolution <Fig_units>] ",
	"[-geometry] ",
	"[-ghostscript <gsname>] ",
	"[-grid_color <grid_color>] ",
	"[-grid_unit <grid_unit>] ",
	"[-hiddentext] ",
	"[-dontshowdepthmanager] ",
	"[-iconGeometry <geom>] ",
	"[-icon_view] ",
	"[-image_editor <editor>] ",
	"[-imperial] ",
	"[-inches] ",
	"[-installowncmap] ",
	"[-internalBW <width>] ",
	"[-jpeg_quality <quality>] ",
	"[-keyFile <file>] ",
	"[-landscape] ",
	"[-latexfonts] ",
	"[-left] ",
	"[-library_dir <directory>] ",
	"[-library_icon_size <size>] ",
	"[-list_view] ",
	"[-magnification <print/export_mag>] ",
	"[-max_image_colors <number>] ",
	"[-metric] ",
	"[-monochrome] ",
	"[-multiple] ",
	"[-normalFont <font>] ",
	"[-noscalablefonts] ",
	"[-nosplash] ",
	"[-notrack] ",
	"[-overlap] ",
	"[-pageborder <color>] ",
	"[-paper_size <size>] ",
	"[-pheight <height>] ",
	"[-portrait] ",
	"[-pwidth <width>] ",
	"[-right] ",
	"[-rigidtext] ",
	"[-rulerthick <width>] ",
	"[-scale_factor <factor>] ",
	"[-scalablefonts] ",
	"[-showallbuttons] ",
	"[-showballoons] ",
	"[-showdepthmanager] ",
	"[-showlengths] ",
	"[-shownums] ",
	"[-showpageborder] ",
	"[-showaxislines] ",
	"[-single] ",
	"[-smooth_factor <factor>] ",
	"[-specialtext] ",
	"[-spellcheckcommand <command>] ",
	"[-spinner_delay <delay>] ",
	"[-spinner_rate <rate>] ",
	"[-splash] ",
	"[-startfillstyle <style>] ",
	"[-startfontsize <size>] ",
	"[-startgridmode <number>] ",
	"[-startlatexFont <font>] ",
	"[-startlinewidth <width>] ",
	"[-startposnmode <number>] ",
	"[-startpsFont <font>] ",
	"[-starttextstep <number>] ",
	"[-tablet] ",
	"[-track] ",
	"[-transparent_color <color number>] ",
	"[-update file1 file2 ...] ",
	"[-userscale <scale>] ",
	"[-userunit <units>] ",
	"[-visual <visual>] ",
	"[-axislines <color>] ",
	"[-zoom <zoom scale>] ",
#ifdef I18N
	"[-international] ",
	"[-inputStyle <OffTheSpot|OverTheSpot|Root>] ",
#endif /* I18N */
	"  [file] ",
	NULL } ;

Atom wm_protocols[2];

static String	tool_translations =
			"<Message>WM_PROTOCOLS:Quit()\n";
static String	form_translations =
			"<ConfigureNotify>:ResizeForm()\n";
XtActionsRec	form_actions[] =
{
    {"ResizeForm", (XtActionProc) check_for_resize},
    {"Quit", (XtActionProc) my_quit},
};

static XtActionsRec text_panel_actions[] =
{
    {"PastePanelKey", (XtActionProc) paste_panel_key} ,
    {"EmptyTextKey", (XtActionProc) clear_text_key} ,
};

struct geom {
	int wid,ht;
	};

#define NCHILDREN	9

static Arg	    args[10];
static int	    cnt;

/* to get any visual the user specifies */

typedef struct
{
	Visual	*visual;
	int	depth;
} OptionsRec;

OptionsRec	Options;

XtResource resources[] =
{
	{"visual", "Visual", XtRVisual, sizeof (Visual *),
	XtOffsetOf (OptionsRec, visual), XtRImmediate, NULL},
	{"depth", "Depth", XtRInt, sizeof (int),
	XtOffsetOf (OptionsRec, depth), XtRImmediate, NULL},
};

XtTimerCallbackProc manage_layer_buttons();

int	xargc;		/* keeps copies of the command-line arguments */
char  **xargv;
int	xpm_icon_status; /* status from reading the xpm icon */
struct  geom   geom;


int setup_visual (int *argc_p, char **argv, Arg *args);
void get_pointer_mapping (void);


void updateFigFilesToCurrentVersion(int argc, char **argv)
{
    exit(update_fig_files(argc, argv));
}

int main(int argc, char **argv)
{
    Widget	    children[NCHILDREN];
    XEvent	    event;
    int		    ichild;
    int		    init_canv_wd, init_canv_ht;
    XWMHints	   *wmhints;
    int		    i,j;
    XColor	    dumcolor;
    char	    version[30];
    char	   *dval;
    char	    tmpstr[PATH_MAX];

    geomspec = False;

    /* generate version string for various uses */
    sprintf(xfig_version, "Xfig %s patchlevel %s (Protocol %s)",
	   FIG_VERSION, PATCHLEVEL, PROTOCOL_VERSION);

    /* ratio of Fig units to display resolution (80ppi) */
    ZOOM_FACTOR = PIX_PER_INCH/DISPLAY_PIX_PER_INCH;

    /* make a copy of the version */
    (void) strcpy(tool_name, xfig_version);
    (void) sprintf(file_header, "#FIG %s", PROTOCOL_VERSION);

    /* clear update Fig flag */
    update_figs = False;

    /* get the TMPDIR environment variable for temporary files */
    if ((TMPDIR = getenv("XFIGTMPDIR"))==NULL)
	TMPDIR = "/tmp";

    /* first check args to see if user wants to scale the figure as it is
	read in and make sure it is a resonable (positive) number */
    for (i=1; i<argc-1; i++)
	if (strcasecmp(argv[i], "-scale_factor")==0)
	    break;
    if (i < argc-1)
	scale_factor = atof(argv[i+1]);
    if (scale_factor <= 0.0)
	scale_factor = 1.0;

    if (argc > 1 && (strcasecmp(argv[1],"-update")==0)) {
    	updateFigFilesToCurrentVersion(argc, argv);
    } else if (argc > 1) {
	char *p1,*p2,*p;
	/* first check for either -help or -version */
	for (i=1; i<argc; i++)
	    if (strcasecmp(argv[i],"-v")==0 ||
		    ((strcmp(argv[i],"-help")==0) || (strcmp(argv[i],"-h")==0))) {

		/*******************************/
		/* help or version number only */
		/*******************************/
		/* print the version info in both cases */
		fprintf(stderr,"%s\n",xfig_version);
		if ((strcmp(argv[i],"-help")==0) || (strcmp(argv[i],"-h")==0)) {
		    int len, col;

		    fprintf(stderr,"Usage:\n");
		    fprintf(stderr,"xfig [-h[elp]] [-v[ersion]]\n");
		    fprintf(stderr,"     ");
		    col=5;
		    for (i = 0; help_list[i] != NULL; i++) {
			len = strlen(help_list[i]);
			if (col+len > 85) {
			    col = 5;
			    fprintf(stderr,"\n     ");
			}
			fprintf(stderr, "%s", help_list[i]);
			col += len;
		    }
	 	    fprintf(stderr,"\n  Note: all options may be abbreviated to minimum unique string.\n");
		}
		/* exit after -h or -v */
		exit(0);
	    }

	/*********************************************************************************/
	/* If the user uses the -geom argument parse out the position from the size.     */
	/* We'll allow the system to use the position part, but we'll set the size later */
	/* when the window sizes have been computed correctly.                           */
	/*********************************************************************************/

	for (i=1; i<argc; i++) {
	    if (strncmp(argv[i],"-ge",3)==0) {
		if (i+1 < argc) {
		    if ((argv[i+1][0]=='+') || (argv[i+1][0]=='-'))
			break;			/* yes, starts with +/-x+/-y */
		}
		geomspec = True;
		sscanf(argv[i+1],"%dx%d",&geom.wid,&geom.ht);
		/* now, if there is an offset, (+/-x+/-y) then move it to beginning of arg */
		p1=strchr(argv[i+1],'+');
		p2=strchr(argv[i+1],'-');
		if (p1 || p2) {
		    if (p1 == 0)
			p = p2;
		    else if (p2 == 0)
			p = p1;
		    else
			p = min2(p1,p2);
		    safe_strcpy(argv[i+1],p);
		    break;
		}
		/* only specified geometry and not offset, get rid of it altogether */
		for (j=i+2; j<argc; i++, j++) {
		    argv[i] = argv[j];
		}
		argc -= 2;
		break;
	    }
	}
    }

#ifdef I18N
    setlocale(LC_ALL, "");
    XtSetLanguageProc(NULL, NULL, NULL);
#endif  /* I18N */

    /*
     * save the command line arguments
     */

    xargc = argc;
    xargv = (char **) XtMalloc ((argc+1) * sizeof (char *));
    bcopy ((char *) argv, (char *) xargv, argc * sizeof (char *));

    /* for some reason Solaris version of XtAppCreateShell references the following */
    xargv[argc]=NULL;

    /* get all the visual stuff and depth */
    cnt = setup_visual(&argc, argv, args);

    /*
     * Now create the real toplevel widget.
     */
    XtSetArg (args[cnt], XtNargv, xargv); ++cnt;
    XtSetArg (args[cnt], XtNargc, xargc); ++cnt;
    tool = XtAppCreateShell ((String) "xfig", (String) "Fig",
				applicationShellWidgetClass,
				tool_d, args, cnt);

    /* get width, height of screen */
    screen_wd = WidthOfScreen(XtScreen(tool));
    screen_ht = HeightOfScreen(XtScreen(tool));

    /* get the FIG2DEV_DIR environment variable (if any is set) for the path
       to fig2dev, in case the user wants one not in the normal search path */
    if ((fig2dev_path = getenv("FIG2DEV_DIR"))==NULL)
	strcpy(fig2dev_cmd, "fig2dev");
    else
	sprintf(fig2dev_cmd,"%s/fig2dev",fig2dev_path);

    /* install actions to get to the functions with accelerators */
    XtAppAddActions(tool_app, main_actions, XtNumber(main_actions));

    /* add "string to float" and "integer to float" converters */
    fix_converters();

    /* flip the mouse hints if the pointer mapping is reversed */
    get_pointer_mapping();

    /***********************************************************************/
    /* get the application resources again now that we have the new visual */
    /***********************************************************************/
    XtGetApplicationResources(tool, &appres, application_resources,
			      XtNumber(application_resources), NULL, 0);

    /* start dimension line fonts same as user's request */
    cur_dimline_psflag = appres.latexfonts? 0:1;

    /* check if the user specified Fig.geometry in resources */
    /* If he did not, there will be +0+0.  Allow him to specify position */
    if (appres.geometry && appres.geometry[0] != '+' && appres.geometry[0] != '-') {
	fprintf(stderr,"Don't specify Fig.geometry in the resources.");
	fprintf(stderr,"  The xfig window may not appear\n");
	fprintf(stderr,"correctly if this resource is specified.\n");
	fprintf(stderr,"Use -geometry on the command-line instead.\n");
    }

    /* create filename for cut buffer */
    make_cut_buf_name();

    /* Now read the .xfigrc file from user home directory to get previous settings */

    read_xfigrc();

    /**************************************************************/
    /* All option args have now been deleted, leaving other args. */
    /**************************************************************/

    /* the Fig filename is the only option now */
    if (argc > 1) {
		if (argv[1][0] == '-') {
		  fprintf(stderr, "xfig: unknown option or missing parameter: %s\n", argv[1]);
		  exit(0);
		} else if (argc > 2) {
		  fprintf(stderr, "xfig: too many parameters: %s\n", argv[2]);
		  exit(0);
		}
		arg_filename = argv[1];
    }

#ifdef I18N
    /************************************************************/
    /* if the international option has been set, set the locale */
    /************************************************************/

    if (appres.international)
      (void) sprintf(&tool_name[strlen(tool_name)], " [locale: %s]",
		     setlocale(LC_CTYPE, NULL));
#endif  /* I18N */

    /* get the number of colormap cells for the screen */
    tool_cells = CellsOfScreen(tool_s);

    /* get the screen resolution for use with any input tablet */
    screen_res = (int) ((float) WidthOfScreen(tool_s) /
			((appres.INCHES) ?
			    ((float) WidthMMOfScreen(tool_s)/25.4) :
				     WidthMMOfScreen(tool_s) ));

    /*****************************************/
    /* make sure some resources are in range */
    /*****************************************/

    check_resource_ranges();

    /* PIC_FACTOR is the number of Fig units (1200dpi) per printer's points (1/72 inch) */
    /* it changes with Metric and Imperial */
    PIC_FACTOR  = (appres.INCHES ? (float)PIX_PER_INCH : 2.54*PIX_PER_CM)/72.0;

    /* start with zoom = 1.0 for scaling patterns in buttons, etc */
    display_zoomscale = 1.0;
    zoomscale=display_zoomscale/ZOOM_FACTOR;

    /* parse any paper size the user wants */
    if (appres.paper_size) {
	appres.papersize = parse_papersize(appres.paper_size);
    } else {
	/* default paper size; letter for imperial and A4 for Metric */
	appres.papersize = (appres.INCHES? PAPER_LETTER: PAPER_A4);
    }

    /* filled in later */
    tool_w = (Window) NULL;

    /*************************/
    /* set the icon geometry */
    /*************************/

    set_icon_geom();

    /* set maximum number of colors for imported images */
    set_max_image_colors();

    /* allocate black and white in case we aren't using the default colormap */
    /* (in which case we could have just used BlackPixelOfScreen...) */

    XAllocNamedColor(tool_d, tool_cm, (String) "white", &dumcolor, &dumcolor);
    white_color = dumcolor;
    XAllocNamedColor(tool_d, tool_cm, (String) "black", &dumcolor, &dumcolor);
    black_color = dumcolor;

    /* copy initial appres settings to current variables */
    init_settings();

    /* initialize font information */
    init_font();

    /* initialize the active_layers array */
    reset_layers();
    /* and the depth counters */
    reset_depths();
    /* object counters for depths */
    clearallcounts();

    /* make the top-level widget */
    FirstArg(XtNinput, True);
    NextArg(XtNdefaultDistance, 0);
    NextArg(XtNresizable, False);	/* don't allow children to resize themselves */

    /* create the main form to put everything in */
    tool_form = XtCreateManagedWidget("form", formWidgetClass, tool,
				 Args, ArgCount);

    if (INTERNAL_BW == 0)
	INTERNAL_BW = appres.internalborderwidth;
    if (INTERNAL_BW <= 0)
	INTERNAL_BW = DEF_INTERNAL_BW;

    /* get the desired number of buttons per row for the mode panel */
    SW_PER_ROW = appres.but_per_row;
    if (SW_PER_ROW <= 0)
	SW_PER_ROW = DEF_SW_PER_ROW;
    else if (SW_PER_ROW > 6)
	SW_PER_ROW = 6;

    /*********************/
    /* setup canvas size */
    /*********************/

    init_canv_wd = appres.tmp_width *
	(appres.INCHES ? PIX_PER_INCH : PIX_PER_CM)/ZOOM_FACTOR + 1;
    init_canv_ht = appres.tmp_height *
	(appres.INCHES ? PIX_PER_INCH : PIX_PER_CM)/ZOOM_FACTOR + 1;

    RULER_WD = appres.rulerthick;
    if (RULER_WD < DEF_RULER_WD)
	RULER_WD = DEF_RULER_WD;

    /* if the user didn't specify anything in the resources for width/height */
    if (init_canv_wd < 10) {
	if (appres.landscape) {
	    init_canv_wd = DEF_CANVAS_WD_LAND;
	} else {
	    init_canv_wd = DEF_CANVAS_WD_PORT;
	}
    }
    if (init_canv_ht < 10) {
	if (appres.landscape) {
	    init_canv_ht = DEF_CANVAS_HT_LAND;
	} else {
	    init_canv_ht = DEF_CANVAS_HT_PORT;
	}
    }
    if (appres.landscape) {
	CANVAS_WD_LAND = init_canv_wd;
	CANVAS_HT_LAND = init_canv_ht;
	CANVAS_WD_PORT = DEF_CANVAS_WD_PORT;
	CANVAS_HT_PORT = DEF_CANVAS_HT_PORT;
    } else {
	CANVAS_WD_PORT = init_canv_wd;
	CANVAS_HT_PORT = init_canv_ht;
	CANVAS_WD_LAND = DEF_CANVAS_WD_LAND;
	CANVAS_HT_LAND = DEF_CANVAS_HT_LAND;
    }

    /* setup the other widgets now */
    setup_sizes(init_canv_wd, init_canv_ht);

    /* if canvas size was not specified explicitly, shrink the
       canvas so that xfig window can fit into the screen */
    if (appres.but_per_row <= 0
	|| appres.tmp_height <= 0 || appres.tmp_width <= 0) {
      int message_ht = CMDFORM_HT;  /* assume all height are same... */
      int scrollbar_ht = CMDFORM_HT;
      int margin_ht = 30;  /* margin for window decoration */
      int margin_wd = 20;
      int max_ht, max_wd, min_sw_per_row;
      Boolean resize = FALSE;

      if (appres.tmp_height <= 0) {
	max_ht = screen_ht -
	    CMDFORM_HT - message_ht - TOPRULER_HT - DEF_IND_SW_HT -
	    scrollbar_ht - INTERNAL_BW * 4 - margin_ht;
	CANVAS_HT_LAND = min2(max_ht, CANVAS_HT_LAND);
	CANVAS_HT_PORT = min2(max_ht, CANVAS_HT_PORT);
	if (max_ht < init_canv_ht) {
	  init_canv_ht = max_ht;
	  resize = TRUE;
	}
      }

      /* decide SW_PER_ROW so that they can fit into the window */
      min_sw_per_row = (MODE_SW_HT + INTERNAL_BW) * NUM_MODE_SW / init_canv_ht + 1;
      SW_PER_ROW = max2(min_sw_per_row, SW_PER_ROW);

      if (appres.tmp_width <= 0) {
	max_wd = screen_wd
	   - INTERNAL_BW * 2
	   - (MODE_SW_WD + INTERNAL_BW) * SW_PER_ROW
	   - SIDERULER_WD - margin_wd
	   - (appres.showdepthmanager? LAYER_WD:0);
	CANVAS_WD_LAND = min2(max_wd, CANVAS_WD_LAND);
	CANVAS_WD_PORT = min2(max_wd, CANVAS_WD_PORT);
	if (max_wd < init_canv_wd) {
	  init_canv_wd = max_wd;
	  resize = TRUE;
	}
      }

      if (resize)
	setup_sizes(init_canv_wd, init_canv_ht);
    }

    /* add actions now or some of the other init_ procs complain about not finding actions */
    add_canvas_actions();
    add_cmd_actions();
    add_style_actions();
    add_ind_actions();

    /* now initialize the panels */
    init_main_menus(tool_form, arg_filename);
    init_msg(tool_form);
    init_mousefun(tool_form);
    init_mode_panel(tool_form);
    init_topruler(tool_form);
    init_canvas(tool_form);

    /*
     * check if the NUM_STD_COLS drawing colors could be allocated and have
     * different palette entries
     */
    check_colors();

    /*
     * parse any canvas background or foreground color the user wants
     */

    parse_canvas_colors();

    /* setup the cursors */
    init_cursor();

    /* and recolor the cursors */
    recolor_cursors();

    init_fontmenu(tool_form);	/* printer font menu */
    init_unitbox(tool_form);	/* units box where rulers meet */
    init_sideruler(tool_form);	/* side ruler */
    init_ind_panel(tool_form);	/* bottom indicator panel */
    init_snap_panel(tool_form);	/* snap mode -- must precede the depth panel */
    init_depth_panel(tool_form);/* active layer panel to the right of the side ruler */
    init_manage_style_panel();	/* the named style panel */

    ichild = 0;
    children[ichild++] = cmd_form;	/* command buttons */
    children[ichild++] = mousefun;	/* labels for mouse fns */
    children[ichild++] = msg_panel;	/* message window panel */
    children[ichild++] = mode_panel;	/* current mode */
    children[ichild++] = topruler_sw;	/* top ruler */
    children[ichild++] = unitbox_sw;	/* box containing units */
    children[ichild++] = sideruler_sw;	/* side ruler */
    children[ichild++] = canvas_sw;	/* main drawing canvas */
    children[ichild++] = ind_panel;	/* current settings indicators */

    /*
     * until the following XtRealizeWidget() is called, there are NO windows
     * in existence
     */

    XtManageChildren(children, NCHILDREN);
    XtRealizeWidget(tool);
    tool_w = XtWindow(tool);

    /* get the current directory so we can go back here on abort */
    get_directory(orig_dir);

    /* keep main_canvas for the case when we set a temporary cursor and
       the canvas_win is set the figure preview (when loading figures) */
    main_canvas = canvas_win = XtWindow(canvas_sw);

    /* create some global bitmaps like arrows, etc */
    create_bitmaps();

#ifdef I18N
    if (appres.international) {
      xim_initialize(canvas_sw);
      if (xim_ic != NULL)
	xim_set_ic_geometry(xim_ic, CANVAS_WD, CANVAS_HT);
    }
#endif  /* I18N */

    /* make sure we have the most current colormap */
    if (!swapped_cmap)
	set_cmap(tool_w);

    /* get this one for other sub windows too */
    wm_delete_window = XInternAtom(tool_d, "WM_DELETE_WINDOW", False);

    /* for the main window trap delete window and save_yourself (my_quit) is called */
    i = 0;
    wm_protocols[i++] = wm_delete_window;
    /* remove WM_SAVE_YOURSELF until I do the "right" thing with it */
#ifdef WHEN_SAVE_YOURSELF_IS_FIXED
    wm_protocols[i++] = XInternAtom(tool_d, "WM_SAVE_YOURSELF", False);
#endif /* WHEN_SAVE_YOURSELF_IS_FIXED */
    (void) XSetWMProtocols(tool_d, tool_w, wm_protocols, i);

    /*************************************************/
    /* setup the XPM color icon if compiled to do so */
    /*************************************************/

    set_xpm_icon();

    /* Set the input field to true to allow keyboard input */
    wmhints = XGetWMHints(tool_d, tool_w);
    wmhints->flags |= InputHint;/* add in input hint */
    wmhints->input = True;
    XSetWMHints(tool_d, tool_w, wmhints);
    XFree((char *) wmhints);

    if (appres.RHS_PANEL) {	/* side button panel is on right size */
	FirstArg(XtNfromHoriz, 0);
	NextArg(XtNhorizDistance, SIDERULER_WD + INTERNAL_BW);
	SetValues(topruler_sw);

	/* put side ruler on left side of canvas */
	FirstArg(XtNfromHoriz, 0);
	NextArg(XtNhorizDistance, 0);
	NextArg(XtNfromVert, topruler_sw);
	NextArg(XtNleft, XtChainLeft);	/* chain to left of form */
	NextArg(XtNright, XtChainLeft);
	SetValues(sideruler_sw);

	/* same with unit box */
	FirstArg(XtNfromHoriz, 0);
	NextArg(XtNhorizDistance, 0);
	NextArg(XtNfromVert, msg_panel);
	NextArg(XtNleft, XtChainLeft);	/* chain to left of form */
	NextArg(XtNright, XtChainLeft);
	SetValues(unitbox_sw);

	/* move the mode button panel to the RIGHT of the canvas */
	XtUnmanageChild(mode_panel);
	XtUnmanageChild(canvas_sw);
	FirstArg(XtNfromHoriz, sideruler_sw);	/* canvas goes right of side ruler */
	SetValues(canvas_sw);
	FirstArg(XtNfromHoriz, canvas_sw);	/* panel goes right of canvas */
	NextArg(XtNhorizDistance, -INTERNAL_BW);
	NextArg(XtNfromVert, mousefun);
	NextArg(XtNleft, XtChainRight);
	NextArg(XtNright, XtChainRight);
	SetValues(mode_panel);
	XtManageChild(canvas_sw);
	XtManageChild(mode_panel);

	/* move the depth manager to its right */
	FirstArg(XtNfromHoriz, mode_panel)
	SetValues(layer_form);

    }

    /* initialize the GCs */

    init_gc();

    /* now that widgets have been realized, do some final setups */

    setup_main_menus();
    setup_msg();
    setup_canvas();
    setup_rulers();
    setup_mode_panel();
    setup_mousefun();
    setup_fontmenu();		/* setup bitmaps in printer font menu */
    setup_manage_style_panel();	/* the named style panel */

    setup_ind_panel();
    setup_depth_panel();	/* resize layer form now that we have the height of the rest */

    /* now that all the windows are up, make a busy cursor */
    set_temp_cursor(wait_cursor);

    /* now that we have fill patterns etc, set zoom factor for resolution chosen */
    if (appres.zoom <= 0.0)
	appres.zoom = 1.0;		/* user didn't specify starting zoom, use 1.0 */
    display_zoomscale = appres.zoom;
    zoomscale=display_zoomscale/ZOOM_FACTOR;
    /* set zoom indicator */
    update_current_settings();

    /* if the user specified a geometry, change canvas size to fit */
    resize_canvas();

    /*********************************************************************/
    /* get the current directory for both file and export operations     */
    /* and library_dir if the user has specified that as a relative path */
    /*********************************************************************/
    get_directory(cur_file_dir);
    get_directory(cur_export_dir);

    if (appres.library_dir[0] == '~' && userhome != NULL) {
	sprintf(tmpstr, "%s%s", userhome, &appres.library_dir[1]);
	appres.library_dir = strdup(tmpstr);
    } else if (appres.library_dir[0] != '/') {
	sprintf(tmpstr, "%s/%s", cur_file_dir, appres.library_dir);
	appres.library_dir = strdup(tmpstr);
    }

    /********************************************/
    /* save any filename passed in cur_filename */
    /********************************************/

    if (arg_filename == NULL)
	cur_filename[0] = '\0';
    else
	strcpy(cur_filename, arg_filename);

    /* save path if specified in filename */
    if (dval=strrchr(cur_filename, '/')) {
	strcpy(cur_file_dir, cur_filename);
	/* remove path from filename */
	strcpy(cur_filename, dval+1);
	if (dval=strrchr(cur_file_dir, '/'))
	    *dval = '\0';  /* terminate path at the last "/" */
	change_directory(cur_file_dir);		/* go there */
	/* and get back the canonical (absolute) path */
	get_directory(cur_file_dir);
	get_directory(cur_export_dir);
    }

    /* update the name panel with the filename */
    update_cur_filename(cur_filename);

    /**************************************/
    /* parse the export language resource */
    /**************************************/

    for (i=0; i<NUM_EXP_LANG; i++)
	if (strcasecmp(appres.exportLanguage, lang_items[i])==0)
	    break;
    /* found it set the language number */
    if (i < NUM_EXP_LANG)
	cur_exp_lang = i;
    else
	file_msg("Unknown export language: %s, using default: %s",
		appres.exportLanguage, lang_items[cur_exp_lang]);

    /* install the accelerators - cmd_form, ind_panel and mode_panel
	accelerators are installed in their respective setup_xxx procedures */
    XtInstallAllAccelerators(canvas_sw, tool);
    XtInstallAllAccelerators(mousefun, tool);
    XtInstallAllAccelerators(msg_panel, tool);
    XtInstallAllAccelerators(topruler_sw, tool);
    XtInstallAllAccelerators(sideruler_sw, tool);
    XtInstallAllAccelerators(unitbox_sw, tool);
    XtInstallAllAccelerators(ind_panel, tool);
    XtInstallAllAccelerators(mode_panel, tool);

    if (!appres.DEBUG) {
	XSetErrorHandler(X_error_handler);
	XSetIOErrorHandler((XIOErrorHandler) X_error_handler);
    }

    (void) signal(SIGHUP, error_handler);
    (void) signal(SIGFPE, error_handler);
#ifdef SIGBUS
    (void) signal(SIGBUS, error_handler);
#endif /* SIGBUS */
    (void) signal(SIGSEGV, error_handler);
    (void) signal(SIGINT, SIG_IGN);	/* in case user accidentally types ctrl-c */
    (void) signal(SIGPIPE, SIG_IGN);	/* because we use popen()  */

    /* now add actions */
    XtAppAddActions(tool_app, form_actions, XtNumber(form_actions));
    XtAppAddActions(tool_app, text_panel_actions, XtNumber(text_panel_actions));
    XtOverrideTranslations(tool, XtParseTranslationTable(tool_translations));
    XtOverrideTranslations(tool_form, XtParseTranslationTable(form_translations));

    /* let things settle down */
    process_pending();

    /* now that everything is up, check the version number in the app-defaults */
    sprintf(version,"%s.%s",FIG_VERSION,PATCHLEVEL);
    if (!appres.version || strcasecmp(appres.version,version) < 0) {
	if (!appres.version) {
	    file_msg("Either you have a very old app-defaults file installed (Fig),");
	    file_msg("or there is none installed at all.");
	} else {
	    file_msg("The app-defaults file (version: %s) is older", appres.version);
	    file_msg("    than this version of xfig (%s).",version);
	}
	file_msg("You should install the correct version or you may lose some features.");
	file_msg("This may be done with \"make install\" in the xfig source directory.");
	beep();
	beep();
    }

    /*****************************/
    /*  do the splash screen now */
    /*****************************/

    if (appres.splash)
	splash_screen();

    /************************************************/
    /* if the user passed a filename to us, load it */
    /************************************************/

    if (strlen(cur_filename))
	load_file(cur_filename, 0, 0);

    /* reset the cursor */
    reset_cursor();

    /* add a timeout proc to check if the fig file has changed to redisplay it */
    /* this is only done if the user has requested -autorefresh */
    if (appres.autorefresh) {
	set_autorefresh();
    }

    /* If the user requests a tablet then do the set up for it */
    /*   and handle the tablet XInput extension events */
    /*   in a custom XtAppMainLoop gjl */

    if (appres.tablet) {

#ifndef USE_TAB
	file_msg("Input tablet not compiled in xfig - option ignored");
	appres.tablet = False;
#else

#define TABLETINCHES 11.7
#define SETBUTEVT(d, e) ((d).serial = (e)->serial, \
	(d).window 	= (e)->window, (d).root = (e)->root, \
	(d).subwindow 	= (e)->subwindow, (d).time = (e)->time, \
	(d).x 		= (e)->axis_data[0] / max2(tablet_res, 0.1), \
	(d).state 	= (e)->state, \
	(d).y 		= ((int) ht - (e)->axis_data[1] / max2(tablet_res, 0.1)), \
	(d).button 	= (e)->button)
/* Switch buttons because of the layout of the buttons on the mouse */
#define SWITCHBUTTONS(d) ((d).button = ((d).button == Button2) ? Button1 : \
        ((d).button == Button1) ? Button2 : \
        ((d).button == Button4) ? Button3 : Button4)

	XEventClass	    eventList[3];
	XDeviceMotionEvent *devmotevt;
	XDeviceButtonEvent *devbutevt;
	XDevice		   *tablet;
	XDeviceState	   *tabletState;
	XValuatorState	   *valState;
	int		   i, numDevs, motiontype, butprstype, butreltype, dum;
	long		   minval, maxval;

	/* Get the device list */
	XDeviceInfo *devPtr, *devInfo;
	/* tablet_res is ratio between the tablet res and the screen res */
	float tablet_res = 10.0;
	/* ht is the height of the tablet at 100dpi */
	Dimension ht, wd;

	XButtonEvent xprs, xrel;

	xprs.type = ButtonPress, xrel.type = ButtonRelease;
	xprs.send_event = xprs.same_screen =
	  xrel.send_event = xrel.same_screen = True;
	xprs.button = xrel.button = Button1;

	/* check if the XInputExtension exists */
	if (!XQueryExtension(tool_d, INAME, &dum, &dum, &dum))
		goto notablet;

	/* now search the device list for the tablet */
        devInfo = XListInputDevices(tool_d, &numDevs);
	if (numDevs == 0)
		goto notablet;

	/* Open the tablet device and select the event types */
	for (i = 0, devPtr = devInfo; i < numDevs; i++, devPtr++)
	  if (! strcmp(devPtr->name, XI_TABLET)) {
	    if ((tablet = XOpenDevice(tool_d, devPtr->id)) == NULL)
	      printf("Unable to open tablet\n");

	    else {
	      DeviceMotionNotify(tablet,  motiontype, eventList[0]);
	      DeviceButtonPress(tablet,   butprstype, eventList[1]);
	      DeviceButtonRelease(tablet, butreltype, eventList[2]);

	      if (XSelectExtensionEvent(tool_d,
	             XtWindow(canvas_sw), eventList, 3))
	        printf("Bad status on XSelectExtensionEvent\n");
	    }
	  }

	XFreeDeviceList(devInfo);

	/* Get the valuator data which should give the resolution */
	/*   of the tablet in absolute mode (the default / what we want) */
	/*   Problem with sgi array index (possibly word size related ) */
	tabletState = XQueryDeviceState(tool_d, tablet);
	valState = (XValuatorState *) tabletState->data;
	for (i = 0; i < tabletState->num_classes; i++)
	  if ((int) valState->class == ValuatorClass)
	  {
	    if (valState->num_valuators)
	    {
#if sgi
	      minval = valState->valuators[4];
	      maxval = valState->valuators[5];
#else
	      minval = valState->valuators[0];
	      maxval = valState->valuators[1];
#endif /* sgi */
	      tablet_res = ((float) maxval / TABLETINCHES / screen_res);
	      if (tablet_res <= 0.0 || tablet_res > 100.0)
	        tablet_res = 12.0;

              if (appres.DEBUG)
	        printf("TABLET: Res: %f %d %d %ld %ld\n", tablet_res,
	          valState->valuators[8], valState->valuators[10],
	          minval, maxval);
	    }
	  }
	  else
	    valState = (XValuatorState *)
	      ((long) valState + (int) valState->length);

	XFreeDeviceState(tabletState);

	xprs.display = xrel.display = tool_d;
        FirstArg(XtNheight, &ht);
        NextArg(XtNwidth, &wd);
        GetValues(canvas_sw);
	
	/* "XtAppMainLoop" customized for extension events */
        /* For tablet puck motion events use the location */
        /* info to warp the cursor to the corresponding screen */
        /* position.  For puck button events switch the buttons */
        /* to correspond to the mouse buttons and send a mouse */
        /* button event to the server so the program will just */
        /* think it is getting a mouse button event and act */
        /* appropriately */
	while (1) {
	  XtAppNextEvent(tool_app, &event);
	  if (event.type == motiontype) {
	    devmotevt = (XDeviceMotionEvent *) &event;
            devmotevt->axis_data[0] /= tablet_res;
            devmotevt->axis_data[1] /= tablet_res;

            /* Keep the pointer within the canvas window */
            FirstArg(XtNheight, &ht);
            NextArg(XtNwidth, &wd);
            GetValues(canvas_sw);
	    XWarpPointer(tool_d, None, XtWindow(canvas_sw), None, None, 0, 0,
		 min2(devmotevt->axis_data[0], (int) wd),
		 max2((int) ht - devmotevt->axis_data[1], 0));
	  } else if (event.type == butprstype) {
	    devbutevt = (XDeviceButtonEvent *) &event;
	    SETBUTEVT(xprs, devbutevt);
            SWITCHBUTTONS(xprs);
	    XSendEvent(tool_d, PointerWindow, True,
	       ButtonPressMask, (XEvent *) &xprs);
	  } else if (event.type == butreltype) {
	    devbutevt = (XDeviceButtonEvent *) &event;
	    SETBUTEVT(xrel, devbutevt);
            SWITCHBUTTONS(xrel);
	    XSendEvent(tool_d, PointerWindow, True,
	       ButtonReleaseMask, (XEvent *) &xrel);
	  } else {
	    if (splash_onscreen) {
		/* if user presses key or mouse button, clear splash */
		if (event.type == KeyPress || event.type == ButtonPress)
		    clear_splash();
	    }
	    XtDispatchEvent(&event);
	  }
	}
notablet:
	file_msg("No input tablet present");

#endif /* USE_TAB */

    }

#ifdef I18N
    /* I use this code instead of XtAppMainLoop(), bacause there was
         a problem when used with kinput2.
	 It is probably not good idea to mix calls of XtDispatchEvent()
	 and canvas_selected(), but I couldn't find a better solution. -T.S.
    */
    if (xim_ic != NULL) {
      while (1) {
	XtAppNextEvent(tool_app, &event);
	if (splash_onscreen) {
	    /* if user presses key or mouse button, clear splash */
	    if (event.type == KeyPress || event.type == ButtonPress)
		clear_splash();
	}
	if (!xim_active) {
	  XtDispatchEvent(&event);
	} else if (!XFilterEvent(&event, XtWindow(canvas_sw))) {
	  if (event.type == KeyPress
	      && XtWindow(canvas_sw) == ((XKeyPressedEvent *)&event)->window) {
	    KeySym key = XLookupKeysym((XKeyPressedEvent *)&event, 0);
	    if (XK_F1 <= key && key <= XK_F35) {
	      XtDispatchEvent(&event);
	    } else {
	      canvas_selected(canvas_sw, &event, NULL, NULL);
	    }
	  } else {
	    XtDispatchEvent(&event);
	  }
	}
      }
    }
#endif  /* I18N */

    /* all finished with setup */
    put_msg("READY. Select a mode or load a file");

    /*******************************************************/
    /* Main loop when there is no input tablet and no I18N */
    /* receive and process X events                        */
    /*******************************************************/

    while (1) {
	XtAppNextEvent(tool_app, &event);
	if (splash_onscreen) {
	    /* if user presses key or mouse button, clear splash */
	    if (event.type == KeyPress || event.type == ButtonPress)
		clear_splash();
	}
	XtDispatchEvent(&event);
    }
}

/* setup all the visual and depth stuff */

int
setup_visual(int *argc_p, char **argv, Arg *args)
{
	int	       i, n, cnt;
	int	       count;		/* number of matches (only 1?) */
	XPixmapFormatValues *pmf;
	XVisualInfo    vinfo;		/* template for find visual */
	XVisualInfo   *vinfo_list;	/* returned list of visuals */

	/*
	 * The following creates a _dummy_ toplevel widget so we can
	 * retrieve the appropriate visual resource.
	 */
	tool = XtAppInitialize (&tool_app, "Fig", options, XtNumber (options), argc_p, argv,
			       (String *) NULL, args, 0);
	/* save important info */
	tool_d = XtDisplay(tool);
	tool_s = XtScreen(tool);
	tool_sn = DefaultScreen(tool_d);

	XtGetApplicationResources (tool, &Options, resources,
				   XtNumber (resources), args, 0);
	cnt = 0;
	if ((Options.visual && Options.visual != DefaultVisualOfScreen (tool_s)) ||
	    (Options.depth && Options.depth != DefaultDepthOfScreen (tool_s))) {
		if (! Options.visual) {
		    /* no visual specified by the user, use default */
		    Options.visual = DefaultVisual(tool_d,tool_sn);
		}
		if (Options.depth == 0) {
		    /* no depth specified by the user, use default */
		    Options.depth = DefaultDepthOfScreen(tool_s);
		}

		XtSetArg (args[cnt], XtNvisual, Options.visual); ++cnt;
		/*
		 * Now we create an appropriate colormap.  We could
		 * use a default colormap based on the class of the
		 * visual; we could examine some property on the
		 * rootwindow to find the right colormap; we could
		 * do all sorts of things...
		 */
		tool_cm = XCreateColormap (tool_d,
					    RootWindowOfScreen (tool_s),
					    Options.visual,
					    AllocNone);
		XtSetArg (args[cnt], XtNcolormap, tool_cm); ++cnt;

		/*
		 * Now find some information about the visual.
		 */
		vinfo.visualid = XVisualIDFromVisual (Options.visual);
		vinfo_list = XGetVisualInfo (tool_d, VisualIDMask, &vinfo, &count);
		if (vinfo_list && count > 0) {
			XtSetArg (args[cnt], XtNdepth, vinfo_list[0].depth);
			++cnt;
			XFree ((XVisualInfo *) vinfo_list);
			/* save the depth of the visual */
			tool_dpth = vinfo_list[0].depth;
		}
		/* save the visual */
		tool_v = Options.visual;
	} else {
		/* no visual specified by the user, use default */
		tool_v = DefaultVisual(tool_d,tool_sn);
		/* same for colormap */
		tool_cm = DefaultColormapOfScreen(tool_s);
		tool_dpth = DefaultDepthOfScreen(tool_s);
	}
	/* and save the class */
	tool_vclass = tool_v->c_class;

/***** THIS IS NOT THE PLACE FOR IT *****/
	/* see if the user wants a private colormap */
	if (appres.installowncmap)
	    switch_colormap();

	/* now get the pixmap formats supported and find the bits-per-pixel
	   for the depth we are using */
	
	pmf = XListPixmapFormats (tool_d, &n);
	image_bpp = 0;
	if (pmf) {
	    for (i = 0; i < n; i++) {
		if (pmf[i].depth == tool_dpth) {
		    /* calculate bytes per pixel from bits per pixel */
		    if ((image_bpp = pmf[i].bits_per_pixel/8)==0)
			image_bpp = 1;
		}
	    }
	    XFree ((char *) pmf);
	} else {
	    fprintf(stderr,"*????* NO supported pixmap formats (from XListPixmapFormats)?\n");
	    fprintf(stderr,"Report this to your vendor\n");
	}
	if (image_bpp == 0) {
	    file_msg("Can't find bits per pixel from Pixmap formats, using 8");
	    image_bpp = 1;
	}

	XtDestroyWidget (tool);

	/* return arg pointer */
	return cnt;

}

/************************************************************************/
/* Decide on filename for cut buffer: first try users HOME directory to */
/* allow cutting and pasting between sessions. If this fails, create    */
/* unique filename in /tmp dir                                          */
/************************************************************************/

static void
make_cut_buf_name(void)
{
    userhome = getenv("HOME");
    if ((userhome == NULL) || (*userhome == '\0')) {
	userhome = ".";
    }
    if (userhome != NULL && *strcpy(cut_buf_name, userhome) != '\0') {
	strcat(cut_buf_name, "/.xfig");
    } else {
	sprintf(cut_buf_name, "%s/xfig%06d", TMPDIR, getpid());
    }
}

/*****************************************/
/* make sure some resources are in range */
/*****************************************/

static void
check_resource_ranges(void)
{
    /* make sure smooth_factor is 0, 2 or 4 */
    switch (appres.smooth_factor) {
	case 0: break;
	case 2: break;
	case 4: break;
	default: fprintf(stderr,
		     "smooth_factor must be 0, 2 or 4 - setting to 0 (no smoothing)\n");
		break;
    }

    /* make sure balloon_delay is non-negative */
    if (appres.balloon_delay < 0)
	appres.balloon_delay = 0;

    /* set print/export magnification to user selection (if any) and fix any bad value */
    if (appres.magnification <= 0.0 || appres.magnification > 100.0)
	appres.magnification = 100.0;	/* user didn't specify or chose bad value */

    /* set jpeg quality to user selection (if any) and fix any bad value */
    if (appres.jpeg_quality <= 0 || appres.jpeg_quality > 100)
	appres.jpeg_quality = DEF_JPEG_QUALITY;	/* user didn't specify or chose bad value */
}

/* set the icon geometry */

static void
set_icon_geom(void)
{
    int scr, x, y, junk;

    if (appres.iconGeometry != (char *) 0) {

        for(scr = 0; tool_s != ScreenOfDisplay(tool_d, scr); scr++)
		;

        XGeometry(tool_d, scr, appres.iconGeometry,
                  "", 0, 0, 0, 0, 0, &x, &y, &junk, &junk);
        FirstArg(XtNiconX, x);
        NextArg(XtNiconY, y);
        SetValues(tool);
    }
}

/* set maximum number of colors for imported images */

static void
set_max_image_colors(void)
{
    /* for any of these visual classes, allow total number of cmap entries for image colors */
    switch (tool_vclass) {
	case GrayScale:
	case PseudoColor:
	    /* if the user hasn't specified a limit to the number of colors for images */
	    if (appres.max_image_colors <= 0)
		appres.max_image_colors = DEF_MAX_IMAGE_COLS;
	    break;
	case StaticGray:
	case StaticColor:	/* number of colors = number of map_entries */
	    appres.max_image_colors = tool_v->map_entries;
	    break;
	case DirectColor:
	case TrueColor:		/* set number of colors at max */
	    appres.max_image_colors = MAX_COLORMAP_SIZE;
	    break;
	default:
	    break;
    }
}

/*
 * parse any canvas background or foreground color the user wants
 */

static void
parse_canvas_colors(void)
{
    Pixel	pix;

    /* we had to wait until the canvas was created to get any color the
       user set through resources */
    if (appres.canvasbackground) {
	XParseColor(tool_d, tool_cm, appres.canvasbackground, &x_bg_color);
	if (XAllocColor(tool_d, tool_cm, &x_bg_color)==0) {
	    fprintf(stderr,"Can't allocate background color for canvas\n");
	    appres.canvasbackground = (char*) NULL;
	}
    } else {
	FirstArg(XtNbackground, &pix);
	GetValues(canvas_sw);
	x_bg_color.pixel = pix;
	/* get the rgb values for it */
	XQueryColor(tool_d, tool_cm, &x_bg_color);
    }

    if (appres.canvasforeground) {
	XParseColor(tool_d, tool_cm, appres.canvasforeground, &x_fg_color);
	if (XAllocColor(tool_d, tool_cm, &x_fg_color)==0) {
	    fprintf(stderr,"Can't allocate background color for canvas\n");
	    appres.canvasforeground = (char*) NULL;
	}
    } else {
	FirstArg(XtNforeground, &pix);
	GetValues(canvas_sw);
	x_fg_color.pixel = pix;
	/* get the rgb values for it */
	XQueryColor(tool_d, tool_cm, &x_fg_color);
    }
    /* now set the canvas to the user's choice, if any */
    FirstArg(XtNbackground, x_bg_color.pixel);
    NextArg(XtNforeground, x_fg_color.pixel);
    SetValues(canvas_sw);
}

/*************************************************/
/* setup the XPM color icon if compiled to do so */
/*************************************************/

static void
set_xpm_icon(void)
{
    xpm_icon_status = -1;

#ifdef USE_XPM_ICON
    /********************************************/
    /* use the XPM color icon for color display */
    /********************************************/
    if (all_colors_available) {
	Pixmap		dum;
	Window		iconWindow;

	/*  make a window for the icon */
	iconWindow = XCreateSimpleWindow(tool_d, DefaultRootWindow(tool_d),
					 0, 0, 1, 1, 0,
					black_color.pixel, black_color.pixel);
	xfig_icon_attr.valuemask = XpmReturnPixels;
	xfig_icon_attr.colormap = tool_cm;
	/* use full color icon if TrueColor display */
	if (tool_vclass == TrueColor) {
	    xpm_icon_status = XpmCreatePixmapFromData(tool_d, iconWindow,
				     fig_full_c_icon_X, &fig_icon, &dum, &xfig_icon_attr);
	} else {
	    /* else use reduced color version */
	    xpm_icon_status = XpmCreatePixmapFromData(tool_d, iconWindow,
				     fig_reduced_c_icon_X, &fig_icon, &dum, &xfig_icon_attr);
	}
	/* if all else fails, use standard monochrome bitmap for icon */
	if (xpm_icon_status == XpmSuccess) {
	    XResizeWindow(tool_d, iconWindow,
			  xfig_icon_attr.width,
			  xfig_icon_attr.height);
	    XSetWindowBackgroundPixmap(tool_d, iconWindow, fig_icon);
	    XtVaSetValues(tool, XtNiconWindow, iconWindow, NULL);
	} else {
	    fig_icon = XCreateBitmapFromData(tool_d, tool_w, (char *) figure_ic.bits,
				figure_ic.width, figure_ic.height);
	}
    } else {
#endif /* USE_XPM_ICON */
	fig_icon = XCreateBitmapFromData(tool_d, tool_w, (char *) figure_ic.bits,
				figure_ic.width, figure_ic.height);
#ifdef USE_XPM_ICON
    }
#endif /* USE_XPM_ICON */

    FirstArg(XtNtitle, tool_name);
    NextArg(XtNiconPixmap, fig_icon);
    SetValues(tool);
}

/* if the user specified a geometry, change canvas size to fit */

static void
resize_canvas(void)
{
    Dimension	    w, h, w1, h1;

    /* get the size of the whole shebang */
    FirstArg(XtNwidth, &w);
    NextArg(XtNheight, &h);
    GetValues(tool);
    TOOL_WD = (int) w;
    TOOL_HT = (int) h;

    if (geomspec) {
	/* set the tool size to user's request */
	TOOL_HT = geom.ht;
	TOOL_WD = geom.wid;
	XtResizeWidget(tool, TOOL_WD, TOOL_HT, 0);

	/* get width of mode panel and side ruler */
	FirstArg(XtNwidth, &w1);
	GetValues(mode_panel);
	w = w1;
	GetValues(sideruler_sw);
	w += w1;
	GetValues(layer_form);
	w += w1;
	/* subtract them from the total xfig window to get the width of the canvas */
	CANVAS_WD_PORT = CANVAS_WD_LAND = geom.wid - w;
	/* now get height of the cmd panel, msg panel, top ruler and ind panel */
	FirstArg(XtNheight, &h1);
	GetValues(cmd_form);
	h = h1;
	GetValues(msg_panel);
	h += h1;
	GetValues(topruler_sw);
	h += h1;
	GetValues(ind_panel);
	h += h1;
	h += INTERNAL_BW*2;
	/* finally whole tool */
	GetValues(tool);
	CANVAS_HT_LAND = CANVAS_HT_PORT = geom.ht - h;
	resize_all((int) (CANVAS_WD_LAND), (int) (CANVAS_HT_LAND));
    } else {
	    /* this really settles things to the right size */
	    resize_all(CANVAS_WD, CANVAS_HT);
    }
}

/* flip the mouse hints if the pointer mapping is reversed */

void get_pointer_mapping(void)
{
	unsigned char mapping[3];
	int	      nmap;

	nmap = XGetPointerMapping(tool_d, mapping, 3);
	/* could happen, I guess */
	if (nmap == 0)
	    return;
	if (mapping[0] != 1)
	    appres.flipvisualhints = True;
}

XtIntervalId refresh_timeout_id = 0;
static	Widget	refresh_indicator = (Widget) NULL;
static	Dimension	refresh_w = 0;
static	Dimension	msg_w = 0;
static	Dimension	new_msg_width;

/* Turn on autorefresh mode 
 * Add a AppTimeOut and insert a label widget to the left of the
 * message window with a red background saying "Autorefresh Mode"
 */

void
set_autorefresh(void)
{
	DeclareArgs(10);

	/* get the initial timestamp */
	figure_timestamp = file_timestamp(cur_filename);
	refresh_timeout_id = XtAppAddTimeOut(tool_app, CHECK_REFRESH_TIME, 
			(XtTimerCallbackProc) check_refresh, (XtPointer) NULL);
	XtUnmanageChild(msg_panel);
	if (!refresh_indicator) {
	    FirstArg(XtNlabel, "Autorefresh Mode");
	    NextArg(XtNfromVert, cmd_form);
	    NextArg(XtNborderWidth, 0);
	    NextArg(XtNbackground, x_color(RED));
	    refresh_indicator = XtCreateWidget("autorefresh", labelWidgetClass,
				    tool_form, Args, ArgCount);
	}
	XtManageChild(refresh_indicator);
	if (msg_w == 0) {
	    FirstArg(XtNwidth, &refresh_w);
	    GetValues(refresh_indicator);	/* get width of refresh indicator */
	    FirstArg(XtNwidth, &msg_w);
	    GetValues(msg_panel);		/* and message panel */
	    new_msg_width = msg_w - refresh_w;	/* calculate new width of message panel */
	}
	FirstArg(XtNfromHoriz, refresh_indicator);
	NextArg(XtNwidth, new_msg_width);	/* set width of message panel */
	SetValues(msg_panel);
	XtManageChild(msg_panel);
	put_msg("Autorefresh mode ON");
}

/* Cancel the autorefresh mode
 * remove the timer and the indicator to the left of the message window
 */

void
cancel_autorefresh(void)
{
	DeclareArgs(4);

	XtRemoveTimeOut(refresh_timeout_id);
	refresh_timeout_id = 0;
	put_msg("Autorefresh mode OFF");
	XtUnmanageChild(msg_panel);
	XtUnmanageChild(refresh_indicator);
	/* reposition message panel to the hard left in the main window */
	FirstArg(XtNfromHoriz, (Widget) NULL);
	NextArg(XtNwidth, msg_w);		/* restore original width if message panel */
	SetValues(msg_panel);
	XtManageChild(msg_panel);
}

void
toggle_refresh_mode(void)
{
	appres.autorefresh = !appres.autorefresh;
	if (appres.autorefresh) {
	    set_autorefresh();
	} else if (refresh_timeout_id) {
	    cancel_autorefresh();
	}
	/* update the View menu */
	refresh_view_menu();
}

/* check if the file timestamp has changed since last displayed and redisplay it */
/* This is called by XtAppAddTimeOut */

static void
check_refresh(XtPointer client_data, XtIntervalId *id)
{
	time_t	    cur_timestamp;

	/* get current timestamp and reload if newer */
	cur_timestamp = file_timestamp(cur_filename);
	if (cur_timestamp > figure_timestamp)
	    load_file(cur_filename, 0, 0);
	figure_timestamp = cur_timestamp;

	/* keep being called */
	(void) XtAppAddTimeOut(tool_app, CHECK_REFRESH_TIME, 
			(XtTimerCallbackProc) check_refresh, (XtPointer) NULL);
	return;
}
