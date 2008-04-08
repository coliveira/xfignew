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

#ifndef W_SETUP_H
#define W_SETUP_H

#define		PIX_PER_INCH		1200
#define		PIX_PER_CM		450	/* closest to correct (472.4) and still
						   have the rulers look good */
						 
/* shorthand */
#define		PPI PIX_PER_INCH
#define		PPCM PIX_PER_CM

#define DISPLAY_PIX_PER_INCH 80

/* Portrait dimensions */
#define		DEF_CANVAS_HT_PORT	9.5*DISPLAY_PIX_PER_INCH+1
#define		DEF_CANVAS_WD_PORT	8.5*DISPLAY_PIX_PER_INCH+1

/* Landscape dimensions */
#define		DEF_CANVAS_HT_LAND	8.5*DISPLAY_PIX_PER_INCH+1
#define		DEF_CANVAS_WD_LAND	11*DISPLAY_PIX_PER_INCH+1

#define		DEF_RULER_WD		24
#define		UNITBOX_WD		95

#ifndef MAX_TOPRULER_WD
#define		MAX_TOPRULER_WD		1600
#endif
#ifndef MAX_SIDERULER_HT
#define		MAX_SIDERULER_HT	1280
#endif
#define		MOUSEFUN_WD		275

#define		DEF_SW_PER_ROW		2	/* def num of buttons per row in mode panel */

#define		DEF_INTERNAL_BW		1
#define		POPUP_BW		2
#define		DEF_LAYER_WD		58	/* default width of the depth panel */

extern int	TOOL_WD, TOOL_HT;
extern int	CMDFORM_WD, CMDFORM_HT;
extern int	NAMEPANEL_WD;
extern int	MODEPANEL_WD, MODEPANEL_HT;
extern int	MODEPANEL_SPACE;
extern int	MSGPANEL_WD, MSGPANEL_HT;
extern int	MOUSEFUN_HT;
extern int	INDPANEL_WD;
extern int	CANVAS_WD, CANVAS_HT;
extern int	CANVAS_WD_LAND, CANVAS_HT_LAND;
extern int	CANVAS_WD_PORT, CANVAS_HT_PORT;
extern int	INTERNAL_BW;
extern int	TOPRULER_WD, TOPRULER_HT;
extern int	SIDERULER_WD, SIDERULER_HT;
extern int	LAYER_WD, LAYER_HT;
extern int	SW_PER_ROW;
extern int	NUM_MODE_SW;

extern void setup_sizes (int new_canv_wd, int new_canv_ht);

#endif /* W_SETUP_H */
