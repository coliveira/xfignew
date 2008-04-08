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
#include "figx.h"
#include "resources.h"
#include "paintop.h"

#include "w_util.h"

#define magnify_width 16
#define magnify_height 16
#define magnify_x_hot 6
#define magnify_y_hot 6
static unsigned char magnify_bits[] = {
   0x40, 0x00, 0xf8, 0x03, 0x4c, 0x06, 0x46, 0x0c, 0x42, 0x08, 0x42, 0x08,
   0xff, 0x1f, 0x42, 0x08, 0x42, 0x08, 0x46, 0x0c, 0x4c, 0x1e, 0xf8, 0x3f,
   0x40, 0x7c, 0x00, 0xf8, 0x00, 0xf0, 0x00, 0x60};



void
init_cursor(void)
{
    register Display *d = tool_d;
    register Pixmap  mag_pixmap;

    arrow_cursor	= XCreateFontCursor(d, XC_left_ptr);
    bull_cursor		= XCreateFontCursor(d, XC_circle);
    buster_cursor	= XCreateFontCursor(d, XC_pirate);
    crosshair_cursor	= XCreateFontCursor(d, XC_crosshair);
    null_cursor		= XCreateFontCursor(d, XC_tcross);
    text_cursor		= XCreateFontCursor(d, XC_xterm);
    pick15_cursor	= XCreateFontCursor(d, XC_dotbox);
    pick9_cursor	= XCreateFontCursor(d, XC_hand1);
    wait_cursor		= XCreateFontCursor(d, XC_watch);
    panel_cursor	= XCreateFontCursor(d, XC_icon);
    lr_arrow_cursor	= XCreateFontCursor(d, XC_sb_h_double_arrow);
    l_arrow_cursor	= XCreateFontCursor(d, XC_sb_left_arrow);
    r_arrow_cursor	= XCreateFontCursor(d, XC_sb_right_arrow);
    ud_arrow_cursor	= XCreateFontCursor(d, XC_sb_v_double_arrow);
    u_arrow_cursor	= XCreateFontCursor(d, XC_sb_up_arrow);
    d_arrow_cursor	= XCreateFontCursor(d, XC_sb_down_arrow);

    /* we must make our on magnifying glass cursor as there is none
	in the cursor font */
    mag_pixmap		= XCreateBitmapFromData(tool_d, DefaultRootWindow(tool_d),
				(char *) magnify_bits, magnify_width, magnify_height);
    magnify_cursor	= XCreatePixmapCursor(d, mag_pixmap, mag_pixmap,
				&x_fg_color, &x_bg_color, magnify_x_hot, magnify_y_hot);
    XFreePixmap(tool_d, mag_pixmap);

    cur_cursor		= arrow_cursor;  /* current cursor */
}

void
recolor_cursors(void)
{
    register Display *d = tool_d;

    XRecolorCursor(d, arrow_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, bull_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, buster_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, crosshair_cursor, &x_fg_color, &x_bg_color);
    XRecolorCursor(d, null_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, text_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pick15_cursor,    &x_fg_color, &x_bg_color);
    XRecolorCursor(d, pick9_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, wait_cursor,      &x_fg_color, &x_bg_color);
    XRecolorCursor(d, panel_cursor,     &x_fg_color, &x_bg_color);
    XRecolorCursor(d, l_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, r_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, lr_arrow_cursor,  &x_fg_color, &x_bg_color);
    XRecolorCursor(d, u_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, d_arrow_cursor,   &x_fg_color, &x_bg_color);
    XRecolorCursor(d, ud_arrow_cursor,  &x_fg_color, &x_bg_color);
    XRecolorCursor(d, magnify_cursor,   &x_fg_color, &x_bg_color);
}

static Cursor active_cursor = None;

void
reset_cursor(void)
{
    if (active_cursor != cur_cursor) {
	active_cursor = cur_cursor;
	XDefineCursor(tool_d, main_canvas, cur_cursor);
	app_flush();
    }
}

void
set_temp_cursor(Cursor cursor)
{
  if (active_cursor != cursor) {
    active_cursor = cursor;
    XDefineCursor(tool_d, main_canvas, cursor);
    app_flush();
  }
}

void
set_cursor(Cursor cursor)
{
  cur_cursor = cursor;
  if (active_cursor != cur_cursor) {
    active_cursor = cur_cursor;
    XDefineCursor(tool_d, main_canvas, cursor);
    app_flush();
  }
}
