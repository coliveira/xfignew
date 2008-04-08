/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1987 Christopher A. Kent
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
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

#ifndef W_COLOR_H
#define W_COLOR_H

#define USE_EXISTING_COLOR	True
#define DONT_USE_EXISTING_COLOR	False

extern	void	show_pencolor(void), next_pencolor(ind_sw_info *sw), prev_pencolor(ind_sw_info *sw);
extern	void	show_fillcolor(void), next_fillcolor(ind_sw_info *sw), prev_fillcolor(ind_sw_info *sw);
extern	void	count_user_colors(void);
extern void YStoreColors (Colormap colormap, XColor *color, int ncolors);
extern int add_color_cell (Boolean use_exist, int indx, int r, int g, int b);
extern void color_borders (void);
extern void create_color_panel (Widget form, Widget label, Widget cancel, ind_sw_info *isw);
extern void del_color_cell (int indx);
extern void pen_fill_activate (int func);
extern void pick_contrast (XColor color, Widget widget);
extern void restore_mixed_colors (void);
extern void set_cmap (Window window);


extern	Widget	delunusedColors;

/* 
 * color.h - color definitions
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Sun Dec 13 1987
 */

Boolean switch_colormap(void);
Boolean alloc_color_cells(Pixel *pixels, int n);

/*
 * $Log: w_color.h,v $
 * Revision 1.1  1995/02/28  15:40:16  feuille
 * Initial revision
 *
 * Revision 1.2  90/06/30  14:33:12  rlh2
 * patchlevel 1
 * 
 * Revision 1.1  90/05/10  11:16:54  rlh2
 * Initial revision
 * 
 * Revision 1.2  88/06/30  09:58:56  mikey
 * Handles CMY also.
 * 
 * Revision 1.1  88/06/30  09:10:53  mikey
 * Initial revision
 * 
 */

typedef	struct _RGB {
	unsigned short r, g, b;
} RGB;

typedef	struct _HSV {
	float	h, s, v;	/* [0, 1] */
} HSV;

typedef struct _CMY {
	unsigned short c, m, y;
} CMY;

extern RGB	RGBWhite, RGBBlack;

RGB	MixRGB();
RGB	MixHSV();
RGB	HSVToRGB(HSV hsv);
HSV	RGBToHSV(RGB rgb);
float	RGBDist();
RGB	PctToRGB(float rr, float gg, float bb);
HSV	PctToHSV();
RGB	CMYToRGB();
CMY	RGBToCMY();
#endif /* W_COLOR_H */
