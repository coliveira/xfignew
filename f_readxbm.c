/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright, 1987, Massachusetts Institute of Technology
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
#include "object.h"
#include "f_picobj.h"
#include "w_setup.h"

/* attempt to read a bitmap file */

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/


int ReadDataFromBitmapFile (FILE *file, unsigned int *width, unsigned int *height, char **data_ret);

int
read_xbm(FILE *file, int filetype, F_pic *pic)
{
    int status;
    unsigned int x, y;
    /* make scale factor smaller for metric */
    float scale = (appres.INCHES ?
			(float)PIX_PER_INCH :
			2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;

    /* first try for a X Bitmap file format */
    status = ReadDataFromBitmapFile(file, &x, &y, &pic->pic_cache->bitmap);
    if (status == BitmapSuccess) {
	pic->pic_cache->subtype = T_PIC_XBM;
	pic->hw_ratio = (float) y / x;
	pic->pic_cache->numcols = 0;
	pic->pic_cache->bit_size.x = x;
	pic->pic_cache->bit_size.y = y;
	pic->pic_cache->size_x = x * scale;
	pic->pic_cache->size_y = y * scale;
	close_picfile(file,filetype);
	return PicSuccess;
    }
    close_picfile(file,filetype);
    /* Non Bitmap file */
    return FileInvalid;
}

/* The following is a modified version of the
   XReadBitmapFromFile() routine from the X11R5 distribution.
   This version reads the XBM file into a data array rather
   than creating the pixmap directly.
   Also, it will uncompress or gunzip the file if necessary.
*/

/* $XConsortium: XRdBitF.c,v 1.15 91/02/01 16:34:46 gildea Exp $ */
/* Copyright, 1987, Massachusetts Institute of Technology */

/*
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish and/or distribute copies of
 * the Software, and to permit persons who receive copies from any such 
 * party to do so, with the only requirement being that this copyright 
 * notice remain intact.
*/

/*
 *	Code to read bitmaps from disk files. Interprets
 *	data from X10 and X11 bitmap files.
 *
 *	Modified for speedup by Jim Becker, changed image
 *	data parsing logic (removed some fscanf()s).
 *	Aug 5, 1988
 *
 * Note that this file and ../Xmu/RdBitF.c look very similar....  Keep them
 * that way (but don't use common source code so that people can have one
 * without the other).
 */

#define MAX_SIZE 255

/* shared data for the image read/parse logic */
static short hexTable[256];		/* conversion value */
static Bool initialized = False;	/* easier to fill in at run time */


/*
 *	Table index for the hex values. Initialized once, first time.
 *	Used for translation value or delimiter significance lookup.
 */
static void
initHexTable(void)
{
    /*
     * We build the table at run time for several reasons:
     *
     *     1.  portable to non-ASCII machines.
     *     2.  still reentrant since we set the init flag after setting table.
     *     3.  easier to extend.
     *     4.  less prone to bugs.
     */
    hexTable['0'] = 0;	hexTable['1'] = 1;
    hexTable['2'] = 2;	hexTable['3'] = 3;
    hexTable['4'] = 4;	hexTable['5'] = 5;
    hexTable['6'] = 6;	hexTable['7'] = 7;
    hexTable['8'] = 8;	hexTable['9'] = 9;
    hexTable['A'] = 10;	hexTable['B'] = 11;
    hexTable['C'] = 12;	hexTable['D'] = 13;
    hexTable['E'] = 14;	hexTable['F'] = 15;
    hexTable['a'] = 10;	hexTable['b'] = 11;
    hexTable['c'] = 12;	hexTable['d'] = 13;
    hexTable['e'] = 14;	hexTable['f'] = 15;

    /* delimiters of significance are flagged w/ negative value */
    hexTable[' '] = -1;	hexTable[','] = -1;
    hexTable['}'] = -1;	hexTable['\n'] = -1;
    hexTable['\t'] = -1;
	
    initialized = True;
}

/*
 *	read next hex value in the input stream, return -1 if EOF
 */
static int
NextInt(FILE *fstream)
{
    int	ch;
    int	value = 0;
    int	ret_value = 0;
    int gotone = 0;
    int done = 0;

    /* loop, accumulate hex value until find delimiter  */
    /* skip any initial delimiters found in read stream */

    while (!done) {
	ch = getc(fstream);
	if (ch == EOF) {
	    value	= -1;
	    done++;
	} else {
	    /* trim high bits, check type and accumulate */
	    ch &= 0xff;
	    if (isascii(ch) && isxdigit(ch)) {
		value = (value << 4) + hexTable[ch];
		gotone++;
	    } else if ((hexTable[ch]) < 0 && gotone)
	      done++;
	}
    }

    ret_value = 0;
    if (value & 0x80)
	ret_value |= 0x01;
    if (value & 0x40)
	ret_value |= 0x02;
    if (value & 0x20)
	ret_value |= 0x04;
    if (value & 0x10)
	ret_value |= 0x08;
    if (value & 0x08)
	ret_value |= 0x10;
    if (value & 0x04)
	ret_value |= 0x20;
    if (value & 0x02)
	ret_value |= 0x40;
    if (value & 0x01)
	ret_value |= 0x80;
    return ret_value;

}

int
ReadDataFromBitmapFile(FILE *file, unsigned int *width, unsigned int *height, char **data_ret)
               
                                 	/* RETURNED */
                    			/* RETURNED */
{
    char *data = NULL;			/* working variable */
    char line[MAX_SIZE];		/* input line from file */
    int size;				/* number of bytes of data */
    char name_and_type[MAX_SIZE];	/* an input line */
    char *type;				/* for parsing */
    int value;				/* from an input line */
    int version10p;			/* boolean, old format */
    int padding;			/* to handle alignment */
    int bytes_per_line;			/* per scanline of data */
    unsigned int ww = 0;		/* width */
    unsigned int hh = 0;		/* height */

    /* first time initialization */
    if (initialized == False)
	initHexTable();

    /* error cleanup and return macro	*/
#define	RETURN(code)  { if (data) free (data); \
			return code; }

    while (fgets(line, MAX_SIZE, file)) {
	if (strlen(line) == MAX_SIZE-1) {
	    RETURN (BitmapFileInvalid);
	}
	if (sscanf(line,"#define %s %d",name_and_type,&value) == 2) {
	    if (!(type = strrchr(name_and_type, '_')))
	      type = name_and_type;
	    else
	      type++;

	    if (!strcmp("width", type))
	      ww = (unsigned int) value;
	    if (!strcmp("height", type))
	      hh = (unsigned int) value;
	    continue;
	}

	if (sscanf(line, "static short %s = {", name_and_type) == 1)
	  version10p = 1;
	else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
	  version10p = 0;
	else if (sscanf(line, "static char %s = {", name_and_type) == 1)
	  version10p = 0;
	else
	  continue;

	if (!(type = strrchr(name_and_type, '_')))
	  type = name_and_type;
	else
	  type++;

	if (strcmp("bits[]", type))
	  continue;

	if (!ww || !hh)
	  RETURN (BitmapFileInvalid);

	if ((ww % 16) && ((ww % 16) < 9) && version10p)
	  padding = 1;
	else
	  padding = 0;

	bytes_per_line = (ww+7)/8 + padding;

	size = bytes_per_line * hh;
	data = (char *) malloc ((unsigned int) size);
	if (!data)
	  RETURN (BitmapNoMemory);

	if (version10p) {
	    char *ptr;
	    int bytes;

	    for (bytes=0, ptr=data; bytes<size; (bytes += 2)) {
		if ((value = NextInt(file)) < 0)
		  RETURN (BitmapFileInvalid);
		*(ptr++) = value;
		if (!padding || ((bytes+2) % bytes_per_line))
		  *(ptr++) = value >> 8;
	    }
	} else {
	    char *ptr;
	    int bytes;

	    for (bytes=0, ptr=data; bytes<size; bytes++, ptr++) {
		if ((value = NextInt(file)) < 0)
		  RETURN (BitmapFileInvalid);
		*ptr=value;
	    }
	}
    }					/* end while */

    if (data == NULL) {
	RETURN (BitmapFileInvalid);
    }

    *data_ret = data;
    *width = ww;
    *height = hh;

    /* don't free the data pointer */
    return (BitmapSuccess);
}
