/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1989-2002 by Brian V. Smith
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

/* structure of a PCX header */

typedef struct _pcxhead
{
    unsigned char	id;		/* 00h Manufacturer ID */
    unsigned char	vers;		/* 01h version */
    unsigned char	format;		/* 02h Encoding Scheme */
    unsigned char	bppl;		/* 03h Bits/Pixel/Plane */
    unsigned short	xmin;		/* 04h X Start (upper left) */
    unsigned short	ymin;		/* 06h Y Start (top) */
    unsigned short	xmax;		/* 08h X End (lower right) */
    unsigned short	ymax;		/* 0Ah Y End (bottom) */
    unsigned short	hdpi;		/* 0Ch Horizontal Res. */
    unsigned short	vdpi;		/* 0Eh Vertical Res. */
    unsigned char	egapal[48];	/* 10h 16-Color EGA Palette */
    unsigned char	reserv;		/* 40h reserv */
    unsigned char	nplanes;	/* 41h Number of Color Planes */
    unsigned short	blp;		/* 42h Bytes/Line/Plane */
    unsigned short	palinfo;	/* 44h Palette Interp. */
    unsigned short	hscrnsiz;	/* 46h Horizontal Screen Size */
    unsigned short	vscrnsiz;	/* 48h Vertical Screen Size */
    unsigned char	fill[54];	/* 4Ah reserv */
} pcxheadr;


