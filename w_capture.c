/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1995 Jim Daley (jdaley@cix.compulink.co.uk)
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

/*
  Screen capture functions - let user draw rectangle on screen
  and write a png file of the contents of that area.
*/

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "w_capture.h"
#include "w_msgpanel.h"

#include "f_util.h"
#include "f_wrpng.h"
#include "w_drawprim.h"
#include "w_util.h"

static Boolean	getImageData(int *w, int *h, int *type, int *nc, unsigned char *Red, unsigned char *Green, unsigned char *Blue);	  	/* returns zero on failure */
static Boolean	selectedRootArea(int *x_r, int *y_r, unsigned int *w_r, unsigned int *h_r, Window *cw);	/* returns zero on failure */
static void	drawRect(int x, int y, int w, int h, int draw);
static int	getCurrentColors(Window w, XColor *colors);	/* returns number of colors in map */

static unsigned char *data;		/* pointer to captured & converted data */

/* 
  statics which need to be set up before we can call
  drawRect - drawRect relies on GC being an xor so
  a second XDrawRectangle will erase the first
*/
static Window   rectWindow;
static GC       rectGC;



Boolean
captureImage(Widget window, char *filename)  	/* returns True on success */
              
               
{
    unsigned char	Red[MAX_COLORMAP_SIZE],
			Green[MAX_COLORMAP_SIZE],
			Blue[MAX_COLORMAP_SIZE];
    int      		numcols;
    int      		captured;
    int      		width, height;
    Boolean		status;

    FILE		*pngfile;
    int			 type;

    if (!ok_to_write(filename, "EXPORT") )
	return(False);

    /* unmap the xfig windows, capture a png then remap our windows */

    XtUnmapWidget(tool);
    XtUnmapWidget(window);
    app_flush();

    /* capture the screen area */
    status = getImageData(&width, &height, &type, &numcols, Red, Green, Blue);

    /* make sure server is ungrabbed if we're debugging */
    app_flush();
    /* map our windows again */
    XtMapWidget(tool);
    XtMapWidget(window);

    if ( status == False ) {
	put_msg("Nothing Captured.");
	app_flush();
	captured = False;
    } else {
	/* encode the image and write to the file */
	put_msg("Writing screenshot to PNG file...");

	app_flush();

	if ((pngfile = fopen(filename,"wb"))==0) {
	    file_msg("Cannot open PNG file %s for writing",filename);
	    put_msg("Cannot open PNG file %s for writing",filename);
	    captured = False;
	} else {
	    /* write the png file */
	    if (!write_png(pngfile, data, type, Red, Green, Blue, numcols, width, height))
		file_msg("Problem writing PNG file from screen capture");
	    fclose(pngfile);
	    captured = True;
	}

	free(data);
   }

   return ( captured );
}

/*
 * Get the image data from the screen
 * width returned in w, height in h
 * image type (IMAGE_RGB or IMAGE_PALETTE) stored in type
 * colormap for IMAGE_PALETTE stored in Red, Green, Blue with
 *		number of colors stored in nc
 *
 * Returns False on failure
 */

/* count how many bits to shift mask to the right to end up with a "1" in the lsb */

int
rshift(int mask)
{
    register int i;

    for (i=0; i<32; i++) {
	if (mask&1)
	    break;
	mask >>= 1;
    }
    return i;
}

static Boolean
getImageData(int *w, int *h, int *type, int *nc, unsigned char *Red, unsigned char *Green, unsigned char *Blue)
{
    XColor	colors[MAX_COLORMAP_SIZE];
    int		colused[MAX_COLORMAP_SIZE];
    int		mapcols[MAX_COLORMAP_SIZE];
    int		red, green, blue;
    int		red_mask, green_mask, blue_mask;
    int		red_shift, green_shift, blue_shift;

    int		x, y, width, height;
    Window	cw;
    static	XImage *image;

    int		i, j;
    int		numcols;
    int		bytes_per_pixel, bit_order, byte_order;
    int		byte_inc;
    int		pix;
    unsigned char *iptr, *rowptr, *dptr;

    sleep(1);   /* in case he'd like to click on something */
    beep();	/* signal user */
    if ( selectedRootArea( &x, &y, &width, &height, &cw ) == False )
	return False;

    image = XGetImage(tool_d, XDefaultRootWindow(tool_d),
				 x, y, width, height, AllPlanes, ZPixmap);
    if (!image || !image->data) {
	file_msg("Cannot capture %dx%d area - memory problems?",
							width,height);
	return False;
    }


    /* if we get here we got an image! */
    *w = width = image->width;
    *h = height = image->height;

    if (tool_vclass == TrueColor) {
	*type = IMAGE_RGB;
	bytes_per_pixel = 3;
    } else {
	/* PseudoColor, get color table */
	*type = IMAGE_PALETTE;
	bytes_per_pixel = 1;
	numcols = getCurrentColors(XDefaultRootWindow(tool_d), colors);
	if ( numcols <= 0 ) {  /* ought not to get here as capture button
			    should not appear for these displays */
	    file_msg("Cannot handle a display without a colormap.");
	    XDestroyImage( image );
	    return False;
	}
    }

    iptr = rowptr = (unsigned char *) image->data;
    dptr = data = (unsigned char *) malloc(height*width*bytes_per_pixel);
    if ( !dptr ) {
	file_msg("Insufficient memory to convert image.");
	XDestroyImage(image);
	return False;
    }
     
    if (tool_vclass == TrueColor) {
	byte_order = image->byte_order;			/* MSBFirst or LSBFirst */
	bit_order = image->bitmap_bit_order;		/* MSBFirst or LSBFirst */
	red_mask = image->red_mask;
	green_mask = image->green_mask;
	blue_mask = image->blue_mask;
	/* find how many bits we need to shift values */
	red_shift = rshift(red_mask);
	green_shift = rshift(green_mask);
	blue_shift = rshift(blue_mask);
	switch (image->bits_per_pixel) {
	    case 8: byte_inc = 1;
		    break;
	    case 16: byte_inc = 2;
		    break;
	    case 24: byte_inc = 3;
		    break;
	    case 32: byte_inc = 4;
		    break;
	    default: byte_inc = 4;
		    break;
	}

	for (i=0; i<image->height; i++) {
	    for (j=0; j<image->width; j++) {
		if (byte_order == MSBFirst) {
		    switch (byte_inc) {
			case 1:
				pix =  (unsigned char) *iptr;
				break;
			case 2: 
				pix =  (unsigned short) (*iptr << 8);
				pix += (unsigned char) *(iptr+1);
				break;
			case 3:
				pix =  (unsigned int) (*(iptr) << 16);
				pix += (unsigned short) (*(iptr+1) << 8);
				pix += (unsigned char) (*(iptr+2));
				break;
			case 4:
				pix =  (unsigned int) (*(iptr) << 24);
				pix += (unsigned int) (*(iptr+1) << 16);
				pix += (unsigned short) (*(iptr+2) << 8);
				pix += (unsigned char) (*(iptr+3));
				break;
		    }
		} else {
		    /* LSBFirst */
		    switch (byte_inc) {
			case 1:
				pix =  (unsigned char) *iptr;
				break;
			case 2: 
				pix =  (unsigned char) *iptr;
				pix += (unsigned short) (*(iptr+1) << 8);
				break;
			case 3:
				pix =  (unsigned char) *iptr;
				pix += (unsigned short) (*(iptr+1) << 8);
				pix += (unsigned int) (*(iptr+2) << 16);
				break;
			case 4:
				pix =  (unsigned char) *iptr;
				pix += (unsigned short) (*(iptr+1) << 8);
				pix += (unsigned int) (*(iptr+2) << 16);
				pix += (unsigned int) (*(iptr+3) << 24);
				break;
		    }
		} /* if (byte_order ...) */

		/* increment pixel pointer */
		iptr += byte_inc;

		/* now extract the red, green and blue values using the masks and shifting */

		red   = (pix & red_mask) >> red_shift;
		green = (pix & green_mask) >> green_shift;
		blue  = (pix & blue_mask) >> blue_shift;
		/* store in output data */
		*(dptr++) = (unsigned char) red;
		*(dptr++) = (unsigned char) green;
		*(dptr++) = (unsigned char) blue;
	    } /* for (j=0; j<image->width ... */

	    /* advance to next scanline row */
	    rowptr += image->bytes_per_line;
	    iptr = rowptr;
	} /* for (i=0; i<image->height ... */

    } else if (tool_cells > 2) { 
	/* color image with color table (PseudoColor) */
	for (i=0; i<numcols; i++) {
	    colused[i] = 0;
	}

	/* now map the pixel values to 0..numcolors */
	x = 0;
	for (i=0; i<image->bytes_per_line*height; i++, iptr++) {
	    if (x >= image->bytes_per_line)
		x=0;
	    if (x < width) {
		colused[*iptr] = 1;	/* mark this color as used */
		*dptr++ = *iptr;
	    }
	    x++;
	}

	/* count the number of colors used */
	*nc = numcols;
        numcols = 0;
	/* and put them in the Red, Green and Blue arrays */
	for (i=0; i< *nc; i++) {
	    if (colused[i]) {
		mapcols[i] =  numcols;
		Red[numcols]   = colors[i].red >> 8;
		Green[numcols] = colors[i].green >> 8;
		Blue[numcols]  = colors[i].blue >> 8;
		numcols++;
	    }
	}
	/* remap the pixels */
	for (i=0, dptr = data; i < width*height; i++, dptr++) {
	    *dptr = mapcols[*dptr];
	}
	*nc = numcols;

    /* monochrome, copy bits to bytes */
    } else {
	int	bitp;
	x = 0;
	for (i=0; i<image->bytes_per_line*height; i++, iptr++) {
	    if (x >= image->bytes_per_line*8)
		x=0;
	    if (image->bitmap_bit_order == LSBFirst) {
		for (bitp=1; bitp<256; bitp<<=1) {
		    if (x < width) {
			if (*iptr & bitp)
			    *dptr = 1;
			else
			    *dptr = 0;
			dptr++;
		    }
		    x++;
		}
	    } else {
		for (bitp=128; bitp>0; bitp>>=1) {
		    if (x < width) {
			if (*iptr & bitp)
			    *dptr = 1;
			else
			    *dptr = 0;
			dptr++;
		    }
		    x++;
		}
	    }
	}
	for (i=0; i<2; i++) {
	    Red[i]   = colors[i].red >> 8;
	    Green[i] = colors[i].green >> 8;
	    Blue[i]  = colors[i].blue >> 8;
	}
	numcols = 2;
	*nc = numcols;
    }
    /* free the image structure */
    XDestroyImage(image);
    return True;
}


#define PTR_BUTTON_STATE( wx, wy, msk ) \
 ( XQueryPointer(tool_d, rectWindow, &root_r, &child_r, &root_x, &root_y, \
					&wx, &wy, &msk),    \
     msk & (Button1Mask | Button2Mask | Button3Mask) )

	
/*
  let user mark which bit of the window we want, UI follows xfig:
  	button1  marks start point, any other cancels
  	button1 again marks end point - any other cancels
*/

static Boolean 
selectedRootArea(int *x_r, int *y_r, unsigned int *w_r, unsigned int *h_r, Window *cw)
{
    int		x1, y1;			/* start point of user rect */
    int		x, y, width, height;	/* current values for rect */

    Window	root_r, child_r;	/* parameters for xQueryPointer */
    int		root_x, root_y;
    int		last_x, last_y;
    int		win_x,  win_y;
    unsigned	int dum;
    unsigned	int mask;
    XGCValues	gcv;
    unsigned long gcmask;

    /* set up our local globals for drawRect */ 
    rectWindow = XDefaultRootWindow(tool_d);

    XGrabPointer(tool_d, rectWindow, False, 0L,
	 	GrabModeAsync, GrabModeSync, None,
 			crosshair_cursor, CurrentTime);
    while (PTR_BUTTON_STATE( win_x, win_y, mask ) == 0) 
	;

    /* button 1 pressed, get whole window under pointer */
    if ( (mask & Button1Mask ) ) {
	/* after user releases button */
	while (PTR_BUTTON_STATE( win_x, win_y, mask ) != 0) 
	    ;
	XUngrabPointer(tool_d, CurrentTime);
	if (child_r == None)
	    child_r = root_r;
	/* get the geometry right into the return vars */
	XGetGeometry(tool_d, child_r, &root_r, x_r, y_r, w_r, h_r, &dum, &dum);
	/* make sure area is on screen */
	if (*x_r < 0)
	    *x_r = 0;
	else if (*x_r + *w_r > WidthOfScreen(tool_s))
	    *w_r = WidthOfScreen(tool_s)-*x_r;
	if (*y_r < 0)
	    *y_r = 0;
	else if (*y_r + *h_r > HeightOfScreen(tool_s))
	    *h_r = HeightOfScreen(tool_s)-*y_r;
	*cw = child_r;
	return True;
    }

	
    /* button 2 pressed, wait for release */
    if ( !(mask & Button2Mask ) ) {
	XUngrabPointer(tool_d, CurrentTime);
	return False; 
    } else {
	while (PTR_BUTTON_STATE( win_x, win_y, mask ) != 0) 
	    ;
    }

    /* if we're here we got a button 2 press  & release */
    /* so initialise for tracking box across display    */ 

    last_x = x1 = x = win_x;  
    last_y = y1 = y = win_y;  
    width = 0;
    height = 0;

    /* Nobble our GC to let us draw a box over everything */
    gcv.foreground = x_color(BLACK) ^ x_color(WHITE);
    gcv.background = x_color(WHITE);
    gcv.function = GXxor;
    gcmask = GCFunction | GCForeground | GCBackground;
    rectGC = XCreateGC(tool_d, XtWindow(canvas_sw), gcmask, &gcv);
    XSetSubwindowMode(tool_d, rectGC, IncludeInferiors);

    /* Wait for button press while tracking rectangle on screen */
    while ( PTR_BUTTON_STATE( win_x, win_y, mask ) == 0 ) {
	if (win_x != last_x || win_y != last_y) {   
	    drawRect(x, y, width, height, False);	/* remove any existing rectangle */

	    x = min2(x1, win_x);
	    y = min2(y1, win_y);
	    width  = abs(win_x - x1);
	    height = abs(win_y - y1);

	    last_x = win_x;
	    last_y = win_y;

	    if ((width > 1) && (height > 1))
	    drawRect(x, y, width, height, True);	/* display rectangle */
	}
    }
 
    drawRect(x, y, width, height, False);		/*  remove any remaining rect */
    XUngrabPointer(tool_d, CurrentTime);		/*  & let go the pointer */

    /* put GC back to normal */
    XSetFunction(tool_d, rectGC, GXcopy);
    XSetSubwindowMode(tool_d, rectGC, ClipByChildren);

    if (width == 0 || height == 0 || !(mask & Button2Mask) )  
	return False;	/* cancelled or selected nothing */

    /* we have a rectangle - set up the return parameters */    
    *x_r = x;     *y_r = y;
    *w_r = width; *h_r = height;
    if ( child_r == None )
	*cw = root_r;
    else
	*cw = child_r;

    return True;
}


/*
  draw or erase an on screen rectangle - dependant on value of draw
*/

static void
drawRect(int x, int y, int w, int h, int draw)
{
static int onscreen = False;

if ( onscreen != draw )
  {
  if ((w>1) && (h >1))
     {
     XDrawRectangle( tool_d, rectWindow, rectGC, x, y, w-1, h-1 );
     onscreen = draw;
     }
  }
}

/*
  in picking up the color map I'm making the assumption that the user
  has arranged the captured screen to appear as he wishes - ie that
  whatever colors he wants are displayed - this means that if the
  chosen window color map is not installed then we need to pick
  the one that is - rather than the one appropriate to the window
     The catch is that there may be several installed maps
     so we do need to check the window -  rather than pick up
     the current installed map.

  ****************  This code based on xwd.c *****************
  ********* Here is the relevant copyright notice: ***********

  The following copyright and permission notice  outlines  the
  rights  and restrictions covering most parts of the standard
  distribution of the X Window System from MIT.   Other  parts
  have additional or different copyrights and permissions; see
  the individual source files.

  Copyright 1984, 1985, 1986, 1987, 1988, Massachusetts Insti-
  tute of Technology.

  Permission  to  use,  copy,  modify,  and  distribute   this
  software  and  its documentation for any purpose and without
  fee is hereby granted, provided  that  the  above  copyright
  notice  appear  in  all  copies and that both that copyright
  notice and this permission notice appear in supporting docu-
  mentation,  and  that  the  name  of  M.I.T.  not be used in
  advertising or publicity pertaining to distribution  of  the
  software without specific, written prior permission.  M.I.T.
  makes no  representations  about  the  suitability  of  this
  software  for  any  purpose.  It is provided "as is" without
  express or implied warranty.

  This software is not subject to any license of the  American
  Telephone  and  Telegraph  Company  or of the Regents of the
  University of California.

*/

#define lowbit(x) ((x) & (~(x) + 1))

static int
getCurrentColors(Window w, XColor *colors)
{
  XWindowAttributes xwa;
  int i, ncolors;
  Colormap map;

  XGetWindowAttributes(tool_d, w, &xwa);

  if (xwa.visual->class == TrueColor) {
    file_msg("TrueColor visual No colormap.");
    return 0;
  }

  else if (!xwa.colormap) {
    XGetWindowAttributes(tool_d, XDefaultRootWindow(tool_d), &xwa);
    if (!xwa.colormap) {
       file_msg("no Colormap available.");
       return 0;
    }
  }

  ncolors = xwa.visual->map_entries;

  if (xwa.visual->class == DirectColor) {
    Pixel red, green, blue, red1, green1, blue1;


    red = green = blue = 0;
    red1   = lowbit(xwa.visual->red_mask);
    green1 = lowbit(xwa.visual->green_mask);
    blue1  = lowbit(xwa.visual->blue_mask);
    for (i=0; i<ncolors; i++) {
      colors[i].pixel = red|green|blue;
      colors[i].pad = 0;
      red += red1;
      if (red > xwa.visual->red_mask)     red = 0;
      green += green1;
      if (green > xwa.visual->green_mask) green = 0;
      blue += blue1;
      if (blue > xwa.visual->blue_mask)   blue = 0;
    }
  }
  else {
    for (i=0; i<ncolors; i++) {
      colors[i].pixel = i;
      colors[i].pad = 0;
    }
  }

  if ( ( xwa.colormap ) && ( xwa.map_installed ) )
     map = xwa.colormap;

  else
     {
     Colormap *maps;
     int count;

     maps = XListInstalledColormaps(tool_d, XDefaultRootWindow(tool_d), &count);
     if ( count > 0 )   map = maps[0];
     else               map = tool_cm;  /* last resort! */
     XFree( maps );
     }
  XQueryColors(tool_d, map, colors, ncolors);

  return(ncolors);
}


/* 
  returns True if we can handle XImages from the visual class
  The current Image write functions & our image conversion routines
  require us to produce a colormapped byte per pixel image 
  pointed to by data
*/

Boolean
canHandleCapture(Display *d)
{
    XWindowAttributes xwa;
 
    XGetWindowAttributes(d, XDefaultRootWindow(d), &xwa);

    if (!xwa.colormap) {
	file_msg("Can't capture screen because no colormap found");
	return False;
    } else if (MAX_COLORMAP_SIZE < xwa.visual->map_entries) {
	file_msg("Can't capture screen because colormap (%d) is larger than %d",
				       xwa.visual->map_entries, MAX_COLORMAP_SIZE);
	return False;
    } else {
	return True;
    }
}
