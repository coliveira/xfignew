/*
 * FIG : Facility for Interactive Generation of figures
 * This software is copyright (C) 1991-1996, Thomas G. Lane.
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * All Rights Reserved except as specified below.
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

/* The following code is from the example.c file in the JPEG distribution */

/* Here is their copyright notice:
 *
 * The authors make NO WARRANTY or representation, either express or implied,
 * with respect to this software, its quality, accuracy, merchantability, or
 * fitness for a particular purpose.  This software is provided "AS IS", and you,
 * its user, assume the entire risk as to its quality and accuracy.
 * 
 * This software is copyright (C) 1991-1996, Thomas G. Lane.
 * All Rights Reserved except as specified below.
 * 
 * Permission is hereby granted to use, copy, modify, and distribute this
 * software (or portions thereof) for any purpose, without fee, subject to these
 * conditions:
 * (1) If any part of the source code for this software is distributed, then this
 * README file must be included, with this copyright and no-warranty notice
 * unaltered; and any additions, deletions, or changes to the original files
 * must be clearly indicated in accompanying documentation.
 * (2) If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the work of
 * the Independent JPEG Group".
 * (3) Permission for use of this software is granted only if the user accepts
 * full responsibility for any undesirable consequences; the authors accept
 * NO LIABILITY for damages of any kind.
 * 
 * These conditions apply to any software derived from or based on the IJG code,
 * not just to the unmodified library.  If you use our work, you ought to
 * acknowledge us.
 * 
 * Permission is NOT granted for the use of any IJG author's name or company name
 * in advertising or publicity relating to this software or products derived from
 * it.  This software may be referred to only as "the Independent JPEG Group's
 * software".
 * 
 * We specifically permit and encourage the use of this software as the basis of
 * commercial products, provided that all warranty or liability claims are
 * assumed by the product vendor.
 *
 */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "f_picobj.h"
#include "f_util.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include <setjmp.h>
#include <jpeglib.h>

static	Boolean	read_JPEG_file(FILE *file);

static	F_pic	   *pict;
static	unsigned char *bitmapptr;

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/

int
read_jpg(FILE *file, int filetype, F_pic *pic)
{
	/* make scale factor smaller for metric */
	float scale = (appres.INCHES ?
				(float)PIX_PER_INCH :
				2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;
	pict = pic;
	if (!read_JPEG_file(file)) {
	    close_picfile(file,filetype);
	    return FileInvalid;
	}

	/* number of colors is put in by read_JPEG_file() */
	pic->pixmap = None;
	pic->pic_cache->subtype = T_PIC_JPEG;
	pic->pic_cache->size_x = pic->pic_cache->bit_size.x * scale;
	pic->pic_cache->size_y = pic->pic_cache->bit_size.y * scale;
	pic->hw_ratio = (float) pic->pic_cache->bit_size.y/pic->pic_cache->bit_size.x;

	/* we have the image here, see if we need to map it to monochrome display */
	if (tool_cells <= 2 || appres.monochrome)
	    map_to_mono(pic);

	close_picfile(file,filetype);
	return PicSuccess;
}

/* These static variables are needed by the error routines. */

static	jmp_buf setjmp_buffer;		/* for return to caller */
static	void	error_exit(j_common_ptr cinfo);
static	void	error_output(j_common_ptr cinfo);

struct error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct error_mgr * error_ptr;

/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the JPEG file is already open and that all
 * decompression parameters can be default values.
 * The routine returns True if successful, False if not.
 */

static Boolean
read_JPEG_file (FILE *file)
{
	int i;

	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;

	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	struct error_mgr jerr;	/* error handler */

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = error_exit;
	jerr.pub.output_message = error_output;

	/* Establish the setjmp return context for error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
	  /* If we get here, the JPEG code has signaled an error.
	   * We need to clean up the JPEG object and return.
	   */
	  jpeg_destroy_decompress(&cinfo);
	  return False;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (i.e. our file) */

	jpeg_stdio_src(&cinfo, file);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, True);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* We want a colormapped color space */
	/* Let the jpeg library do a two-pass over the image to make nice colors */
	cinfo.quantize_colors = True;

	/* Now fill in the pict parameters */

	pict->pic_cache->bit_size.x = cinfo.image_width;
	pict->pic_cache->bit_size.y = cinfo.image_height;
	if ((pict->pic_cache->bitmap = (unsigned char *) 
	     malloc(cinfo.image_width * cinfo.image_height)) == NULL)
		error_exit("Can't alloc memory for JPEG image");
	bitmapptr = pict->pic_cache->bitmap;

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	jpeg_start_decompress(&cinfo);

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */ 
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
	  (void) jpeg_read_scanlines(&cinfo, buffer, 1);
	  for (i = 0; i < row_stride; i++)
		*bitmapptr++ = (unsigned char) buffer[0][i];
	}

	/* Step 7: fill up the colortable in the pict object */
	/* (Must do this before jpeg_finish_decompress or jpeg_destroy_decompress) */

	pict->pic_cache->numcols = cinfo.actual_number_of_colors;
	for (i = 0; i < pict->pic_cache->numcols; i++) {
	    pict->pic_cache->cmap[i].red   = cinfo.colormap[0][i];
	    /* set other colors to first if grayscale */
	    if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
		pict->pic_cache->cmap[i].green = pict->pic_cache->cmap[i].blue = pict->pic_cache->cmap[i].red;
	    } else {
		pict->pic_cache->cmap[i].green = cinfo.colormap[1][i];
		pict->pic_cache->cmap[i].blue  = cinfo.colormap[2][i];
	    }
	}

	/* Step 8: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 9: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return True;
}

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
error_exit (j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    /* cinfo->err really points to a error_mgr struct, so coerce pointer */
    error_ptr err = (error_ptr) cinfo->err;

    /* Display the message if it is NOT "Not a JPEG file" */
    /* However, since the error number is not public we have to parse the string */

    /* Format the message */
    (*cinfo->err->format_message) (cinfo, buffer);

    if (strncmp(buffer,"Not a JPEG file",15)!=0)
	file_msg("%s", buffer);

    /* Return control to the setjmp point */
    longjmp(err->setjmp_buffer, 1);
}

static void
error_output(j_common_ptr cinfo)
{
  char	buffer[JMSG_LENGTH_MAX];

  (*cinfo->err->format_message)(cinfo, buffer);

   file_msg("%s", buffer);
}
