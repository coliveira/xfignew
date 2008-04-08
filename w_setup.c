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
#include "w_cmdpanel.h"
#include "w_icons.h"
#include "w_indpanel.h"
#include "w_setup.h"
#include "w_util.h"

#define	NUM_DRAW_SW 17 /* kludge - shouldn't have to edit this by hand */

int		TOOL_WD, TOOL_HT;
int		CMDFORM_WD, CMDFORM_HT = CMD_BUT_HT;
int		NAMEPANEL_WD;
int		MODEPANEL_WD;
int		MODEPANEL_SPACE;
int		MSGPANEL_WD;
int		MSGPANEL_HT = 18;
int		MOUSEFUN_HT;
int		INDPANEL_WD;
int		CANVAS_WD, CANVAS_HT;
int		CANVAS_WD_LAND, CANVAS_HT_LAND;
int		CANVAS_WD_PORT, CANVAS_HT_PORT;
int		INTERNAL_BW;
int		TOPRULER_WD, TOPRULER_HT;
int		SIDERULER_WD, SIDERULER_HT;
int		SW_PER_ROW, SW_PER_COL;

void setup_sizes(int new_canv_wd, int new_canv_ht)
{
    int		    NUM_CMD_MENUS;

    /*
     * make the width of the mousefun panel about 1/3 of the size of the
     * canvas width and the cmdpanel the remaining width. Be sure to set it
     * up so that cmdpanel buttons can be allocated a size which divides
     * evenly into the remaining space.
     */
    CANVAS_WD = new_canv_wd;
    if (CANVAS_WD < 10)
	CANVAS_WD = 10;
    CANVAS_HT = new_canv_ht;
    if (CANVAS_HT < 10)
	CANVAS_HT = 10;

    if (appres.RHS_PANEL) {
	SIDERULER_WD = UNITBOX_WD - 3;	  /* must make side ruler wider to show unitbox */
	TOPRULER_WD = CANVAS_WD-UNITBOX_WD+SIDERULER_WD - 2*INTERNAL_BW + 4;
    } else {
	SIDERULER_WD = DEF_RULER_WD + 20;  /* allow for 100's and decimals at large zooms */
	TOPRULER_WD = CANVAS_WD-UNITBOX_WD+SIDERULER_WD - 2*INTERNAL_BW + 2;
    }
    TOPRULER_HT = RULER_WD;
    SIDERULER_HT = CANVAS_HT;
    if (TOPRULER_WD > MAX_TOPRULER_WD)
	TOPRULER_WD = MAX_TOPRULER_WD;
    if (SIDERULER_HT > MAX_SIDERULER_HT)
	SIDERULER_HT = MAX_SIDERULER_HT;

    /* side mode panel */
    MODEPANEL_WD = (MODE_SW_WD + INTERNAL_BW) * SW_PER_ROW + INTERNAL_BW;

    NUM_CMD_MENUS = num_main_menus();	/* kludge - NUM_CMD_MENUS local to w_cmdpanel.c */
    /* width of the command menu button form */
    CMDFORM_WD = NUM_CMD_MENUS*(CMD_BUT_WD+INTERNAL_BW);

    /* filename panel to the right of the command menu buttons */
    NAMEPANEL_WD = MODEPANEL_WD + CANVAS_WD + SIDERULER_WD - CMDFORM_WD - 
			MOUSEFUN_WD - INTERNAL_BW;
    if (NAMEPANEL_WD < 75)
	NAMEPANEL_WD = 75;

    /* message window under command buttons */
    MSGPANEL_WD = CMDFORM_WD + NAMEPANEL_WD - INTERNAL_BW;

    /* lower indicator panel */
    INDPANEL_WD = MODEPANEL_WD + CANVAS_WD + SIDERULER_WD;

    /* space for both modepanel titles (Drawing modes and Editing modes) */
    MODEPANEL_SPACE = 1 + CANVAS_HT + RULER_WD - 
	(MODE_SW_HT + INTERNAL_BW) * (ceil((double)NUM_DRAW_SW/SW_PER_ROW) +
			ceil((double)(NUM_MODE_SW-NUM_DRAW_SW)/SW_PER_ROW));
    if (MODEPANEL_SPACE < 2)
	MODEPANEL_SPACE = 2;
}
