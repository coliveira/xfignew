/*
 * FIG : Facility for Interactive Generation of figures
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

extern "C" {
#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "f_util.h"
}
#include <xpm.h>

	
	
/* attempt to read a XPM (color pixmap) file */
/* the filename is passed since XpmReadFileToXpmImage needs a name 
   instead of a FILE pointer.  This is fine because it handles compressed files */

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/

int read_xpm(char *file, int filetype, F_pic *pic)
{
    int		    status;
    XpmImage	    image;
    int		    i;
    char	   *c;
    XColor	    exact_def;

    /* make scale factor smaller for metric */
    float scale = (appres.INCHES ?
			(float)PIX_PER_INCH :
			2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;

    status = XpmReadFileToXpmImage(file, &image, NULL);
    /* if out of colors, try switching colormaps and reading again */
    if (status == XpmColorFailed) {
	if (!switch_colormap())
	    return PicSuccess;
	status = XpmReadFileToXpmImage(file, &image, NULL);
    }
    if (status == XpmSuccess) {
	/* now look up the colors in the image and put them in the pic colormap */
	for (i=0; i<image.ncolors; i++) {
	    c = (image.colorTable + i)->c_color;
	    if (c == NULL || *c == '\0') {	/* use white for null color */
		c = "white";
		file_msg("white used for *NULL color");
	    }
	    if (XParseColor(tool_d, tool_cm, c, &exact_def) == 0) {
		file_msg("Error parsing color %s",c);
		exact_def.red = exact_def.green = exact_def.blue = 65535;
	    }
	    pic->pic_cache->cmap[i].red = exact_def.red >> 8;
	    pic->pic_cache->cmap[i].green = exact_def.green >> 8;
	    pic->pic_cache->cmap[i].blue = exact_def.blue >> 8;
	}
	pic->pic_cache->subtype = T_PIC_XPM;
	pic->pic_cache->numcols = image.ncolors;
	pic->pixmap = None;
	pic->pic_cache->bitmap = (char *) 
					malloc(image.width*image.height*sizeof(unsigned char));
	if (pic->pic_cache->bitmap == NULL) {
	    file_msg("cannot allocate space for XPM image");
	    return PicSuccess;
	}
	for (i=0; i<image.width*image.height; i++)
	    pic->pic_cache->bitmap[i] = (unsigned char) image.data[i]; /* int to unsigned char */
	pic->hw_ratio = (float) image.height / image.width;
	pic->pic_cache->bit_size.x = image.width;
	pic->pic_cache->bit_size.y = image.height;
	pic->pic_cache->size_x = image.width * scale;
	pic->pic_cache->size_y = image.height * scale;
	XpmFreeXpmImage(&image);	/* get rid of the image */
	/* if monochrome display map bitmap */
	if (tool_cells <= 2 || appres.monochrome)
	    map_to_mono(pic);

	return PicSuccess;
    }
    return FileInvalid;
}
