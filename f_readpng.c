/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 2000-2002 by Brian V. Smith
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
#include "f_neuclrtab.h"
#include "f_util.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_msgpanel.h"
#include "w_setup.h"

#include "f_picobj.h"

#include <png.h>



int
read_png(FILE *file, int filetype, F_pic *pic)
{
    register int    i, j;
    png_structp	    png_ptr;
    png_infop	    info_ptr;
    png_infop	    end_info;
    png_uint_32	    w, h, rowsize;
    int		    bit_depth, color_type, interlace_type;
    int		    compression_type, filter_type;
    png_bytep	   *row_pointers;
    char	   *ptr;
    int		    num_palette;
    png_colorp	    palette;
    png_color_16    background;

    /* make scale factor smaller for metric */
    float scale = (appres.INCHES ?
		    (float)PIX_PER_INCH :
		    2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;
    
    /* read the png file here */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		(png_voidp) NULL, NULL, NULL);
    if (!png_ptr) {
	close_picfile(file,filetype);
	return FileInvalid;
    }
		
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
	close_picfile(file,filetype);
	return FileInvalid;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
	close_picfile(file,filetype);
	return FileInvalid;
    }

    /* set long jump recovery here */
    if (setjmp(png_ptr->jmpbuf)) {
	/* if we get here there was a problem reading the file */
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	close_picfile(file,filetype);
	return FileInvalid;
    }
	
    /* set up the input code */
    png_init_io(png_ptr, file);

    /* now read the file info */
    png_read_info(png_ptr, info_ptr);

    /* get width, height etc */
    png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
	&interlace_type, &compression_type, &filter_type);

    if (info_ptr->valid & PNG_INFO_gAMA)
	png_set_gamma(png_ptr, 2.2, info_ptr->gamma);
    else
	png_set_gamma(png_ptr, 2.2, 0.45);

    if (info_ptr->valid & PNG_INFO_bKGD)
	/* set the background to the one supplied */
	png_set_background(png_ptr, &info_ptr->background,
		PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
    else {
	/* blend the canvas background using the alpha channel */
	background.red   = x_bg_color.red >> 8;
	background.green = x_bg_color.green >> 8;
	background.blue  = x_bg_color.blue >> 8;
	background.gray  = 0;
	png_set_background(png_ptr, &background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 2.2);
    }

    /* set order to BGR (default is RGB) */
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_bgr(png_ptr);

    /* strip 16-bit RGB values down to 8-bit */
    if (bit_depth == 16)
	png_set_strip_16(png_ptr);

    /* force to 8-bits per pixel if less than 8 */
    if (bit_depth < 8)
	png_set_packing(png_ptr);

    /* dither rgb files down to 8 bit palette */
    num_palette = 0;
    pic->pic_cache->transp = TRANSP_NONE;
    if (color_type & PNG_COLOR_MASK_COLOR) {
	png_uint_16p	histogram;

#ifdef USE_ALPHA
	/* we need to somehow give libpng a background color that it can
	   combine with the alpha information in each pixel to make the image */

	if (color_type & PNG_COLOR_MASK_ALPHA)
	    pic->pic_cache->transp = TRANSP_BACKGROUND;
#endif /* USE_ALPHA */

	if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
	    png_get_hIST(png_ptr, info_ptr, &histogram);
	    png_set_dither(png_ptr, palette, num_palette, 256, histogram, 0);
	}
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
	/* expand to full range */
	png_set_expand(png_ptr);
	/* make a gray colormap */
	num_palette = 256;
	for (i = 0; i < num_palette; i++)
	    pic->pic_cache->cmap[i].red = pic->pic_cache->cmap[i].green = pic->pic_cache->cmap[i].blue = i;
    } else {
	/* transfer the palette to the object's colormap */
	for (i=0; i<num_palette; i++) {
	    pic->pic_cache->cmap[i].red   = palette[i].red;
	    pic->pic_cache->cmap[i].green = palette[i].green;
	    pic->pic_cache->cmap[i].blue  = palette[i].blue;
	}
    }
    rowsize = w;
    if (color_type == PNG_COLOR_TYPE_RGB ||
	color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	    rowsize = w*3;

    /* allocate the row pointers and rows */
    row_pointers = (png_bytep *) malloc(h*sizeof(png_bytep));
    for (i=0; i<h; i++) {
	if ((row_pointers[i] = malloc(rowsize)) == NULL) {
	    for (j=0; j<i; j++)
		free(row_pointers[j]);
	    close_picfile(file,filetype);
	    return FileInvalid;
	}
    }

    /* finally, read the file */
    png_read_image(png_ptr, row_pointers);

    /* allocate the bitmap */
    if ((pic->pic_cache->bitmap=malloc(rowsize*h))==NULL) {
	close_picfile(file,filetype);
	return FileInvalid;
    }

    /* copy it to our bitmap */
    ptr = pic->pic_cache->bitmap;
    for (i=0; i<h; i++) {
	bcopy(row_pointers[i], ptr, rowsize);
	ptr += rowsize;
    }
    /* put in width, height */
    pic->pic_cache->bit_size.x = w;
    pic->pic_cache->bit_size.y = h;

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
	/* no palette, must use neural net to reduce to 256 colors with palette */
	if (!map_to_palette(pic)) {
	    close_picfile(file,filetype);
	    return FileInvalid;		/* out of memory or something */
	}
	pic->pic_cache->numcols = 256;
    } else {
	pic->pic_cache->numcols = num_palette;
    }

    /* clean up */
    png_read_end(png_ptr, end_info);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    for (i=0; i<h; i++)
	free(row_pointers[i]);

    pic->pic_cache->subtype = T_PIC_PNG;
    pic->pixmap = None;
    pic->hw_ratio = (float) pic->pic_cache->bit_size.y / pic->pic_cache->bit_size.x;
    pic->pic_cache->size_x = pic->pic_cache->bit_size.x * scale;
    pic->pic_cache->size_y = pic->pic_cache->bit_size.y * scale;
    /* if monochrome display map bitmap */
    if (tool_cells <= 2 || appres.monochrome)
	map_to_mono(pic);

    close_picfile(file,filetype);
    return PicSuccess;
}
