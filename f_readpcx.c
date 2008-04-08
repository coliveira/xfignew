/*
 * FIG : Facility for Interactive Generation of figures
 * Based on (public domain) code from Russell Marks
 * Parts Copyright (c) 2000-2002 by Brian V. Smith
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
#include "f_picobj.h"
#include "f_util.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_msgpanel.h"
#include "w_setup.h"

/* This is based on: */

/* pcx2ppm 1.2 - convert pcx to ppm
 * based on zgv's readpcx.c
 * public domain by RJM
 *
 * this is incredibly messy
 *
 * 1999-03-16	updated to support 24-bit.
 */

typedef unsigned char byte;

struct pcxhed
  {
  byte manuf,ver,encod,bpp;		/* 0 - 3 gen format */
  byte x1lo,x1hi,y1lo,y1hi;		/* 4 - 11 size */
  byte x2lo,x2hi,y2lo,y2hi;
  byte unused1[4];			/* 12 - 15 scrn size */
  byte pal16[48];			/* 16 - 63 4-bit palette */
  byte reserved;			/* 64 reserved */
  byte nplanes;				/* 65 num of bitplanes */
  byte bytelinelo,bytelinehi;		/* 66 - 67 bytes per line */
  byte unused2[60];			/* 68 - 127 unused */
  };  /* palette info is after image data */


/* prototypes */
void dispbyte(unsigned char *ptr,int *xp,int *yp,int c,int w,int h,
              int real_bpp,int byteline,int *planep,int *pmaskp);

int	_read_pcx(FILE *pcxfile, F_pic *pic);


int
read_pcx(FILE *file, int filetype, F_pic *pic)
{
    int		    status;

    /* make scale factor smaller for metric */
    float scale = (appres.INCHES ?
		    (float)PIX_PER_INCH :
		    2.54*PIX_PER_CM)/(float)DISPLAY_PIX_PER_INCH;
    
    status = _read_pcx(file,pic);
    if (status != 1) {
	close_picfile(file,filetype);
	return FileInvalid;
    }

    pic->pixmap = None;
    pic->hw_ratio = (float) pic->pic_cache->bit_size.y / pic->pic_cache->bit_size.x;
    pic->pic_cache->subtype = T_PIC_PCX;
    pic->pic_cache->size_x = pic->pic_cache->bit_size.x * scale;
    pic->pic_cache->size_y = pic->pic_cache->bit_size.y * scale;
    /* if monochrome display map bitmap */
    if (tool_cells <= 2 || appres.monochrome)
	map_to_mono(pic);

    close_picfile(file,filetype);
    return PicSuccess;
}

/* _read_pcx() is called from read_pcx() and read_epsf().
   The latter is because the output of ghostscript is to a PCX
   file (actually a pipe).
*/

void pcx_decode();

int _read_pcx(FILE *pcxfile, F_pic *pic)
{
	int		 i,w,h,bytepp,x,y,yy,byteline,plane,pmask;
	unsigned char	*pal;
	struct pcxhed	 header;
	int		 count, cnt;
	long		 bytemax,bytesdone;
	byte		 inbyte;
	int		 real_bpp;		/* how many bpp file really is */

	pic->pic_cache->bitmap=NULL;

	fread(&header,1,sizeof(struct pcxhed),pcxfile);
	if (header.manuf!=10 || header.encod!=1)
	    return FileInvalid;

	/* header.bpp=1, header.nplanes=1 = 1-bit.
	 * header.bpp=1, header.nplanes=2 = 2-bit.   - added B.V.Smith 6/00
	 * header.bpp=1, header.nplanes=3 = 3-bit.   - added B.V.Smith 6/00
	 * header.bpp=1, header.nplanes=4 = 4-bit.
	 * header.bpp=8, header.nplanes=1 = 8-bit.
	 * header.bpp=8, header.nplanes=3 = 24-bit.
	 * anything else gives an `unsupported' error.
	 */
	real_bpp=0;
	bytepp = 1;
	switch(header.bpp) {
	  case 1:
	    switch(header.nplanes) {
	      case 1: real_bpp=1; break;
	      case 2: real_bpp=2; break;
	      case 3: real_bpp=3; break;
	      case 4: real_bpp=4; break;
	    }
	    break;
	  
	  case 8:
	    switch(header.nplanes) {
	      case 1: real_bpp=8; break;
	      case 3: real_bpp=24; bytepp = 3; break;
	    }
	    break;
	  }

	if (!real_bpp)
	    return FileInvalid;

	if ((pal=calloc(768,1))==NULL)
	    return FileInvalid;

	w=(header.x2lo+256*header.x2hi)-(header.x1lo+256*header.x1hi)+1;
	h=(header.y2lo+256*header.y2hi)-(header.y1lo+256*header.y1hi)+1;
	byteline=header.bytelinelo+256*header.bytelinehi;

	if (w==0 || h==0)
	    return FileInvalid;

	x=0; y=0;
	bytemax=w*h;
	if (real_bpp==1 || real_bpp==4)
	    bytemax=(1<<30);	/* we use a 'y<h' test instead for these files */

	if ((pic->pic_cache->bitmap=malloc(w*(h+2)*bytepp))==NULL)
	    return FileInvalid;

	/* need this if more than one bitplane */
	memset(pic->pic_cache->bitmap,0,w*h*bytepp);

	bytesdone=0;

	/* start reading image */
	for (yy=0; yy<h; yy++) {
	  plane = 0;
	  pmask = 1;
	  
	  y = yy;
	  x = 0;
	  while (y == yy) {
	    inbyte=fgetc(pcxfile);
	    if ((inbyte & 0xC0) != 0xC0) {
	      dispbyte(pic->pic_cache->bitmap,&x,&y,inbyte,w,h,real_bpp,
		byteline,&plane,&pmask);
	      bytesdone++;
	    } else {
	      cnt = inbyte & 0x3F;
	      inbyte = fgetc(pcxfile);
	      for (count=0; count<cnt; count++)
		dispbyte(pic->pic_cache->bitmap,&x,&y,inbyte,w,h,real_bpp,
			byteline,&plane,&pmask);
	      bytesdone += cnt;
	    }
	  }
	}

	pic->pic_cache->bit_size.x = w;
	pic->pic_cache->bit_size.y = h;

	/* read palette */
	switch(real_bpp) {
	    case 1:
		pic->pic_cache->cmap[0].red = pic->pic_cache->cmap[0].green = pic->pic_cache->cmap[0].blue = 0;
		pic->pic_cache->cmap[1].red = pic->pic_cache->cmap[1].green = pic->pic_cache->cmap[1].blue = 255;
		pic->pic_cache->numcols = 2;
		break;
	  
	    case 2:
	    case 3:
	    case 4:
		/* 2-,3-, and 4-bit, palette is embedded in header */
		pic->pic_cache->numcols = (1<<real_bpp);
		for (x=0; x < pic->pic_cache->numcols; x++) {
		    pic->pic_cache->cmap[x].red   = header.pal16[x*3  ];
		    pic->pic_cache->cmap[x].green = header.pal16[x*3+1];
		    pic->pic_cache->cmap[x].blue  = header.pal16[x*3+2];
		}
		break;

	    case 8:
		/* 8-bit */
		fseek(pcxfile, -768L, SEEK_END);/* locate colormap in last 768 bytes of file */
		for (x=0; x<256; x++) {
		    pic->pic_cache->cmap[x].red   = fgetc(pcxfile);
		    pic->pic_cache->cmap[x].green = fgetc(pcxfile);
		    pic->pic_cache->cmap[x].blue  = fgetc(pcxfile);
		}
		/* start with 256 */
		pic->pic_cache->numcols = 256;
		/* ignore duplicate white entries after real colors */
		for (i = pic->pic_cache->numcols-1; i >=0 ; i--) {
		if (pic->pic_cache->cmap[i].red != 255 ||
		    pic->pic_cache->cmap[i].green != 255 ||
		    pic->pic_cache->cmap[i].blue != 255)
			break;
		}
		if (i < pic->pic_cache->numcols-2)
		    pic->pic_cache->numcols = i+2;
		break;
	  
	    case 24:
		/* no palette, must use neural net to reduce to 256 colors with palette */
		if (!map_to_palette(pic))
		    return FileInvalid;		/* out of memory or something */
		break;
	}
	return PicSuccess;  
}


void
dispbyte(unsigned char *ptr,int *xp,int *yp,int c,int w,int h,
              int real_bpp,int byteline,int *planep,int *pmaskp)
{
	int f;
	unsigned char *dstptr;

	switch(real_bpp) {
	  case 1:
	  case 2:
	  case 3:
	  case 4:
		/* mono or 4-bit */

		if ((*yp)>=h)
		    return;

		dstptr=ptr+(*yp)*w+*xp;
		w=byteline*8;

		for (f=0; f<8; f++) {
		   *dstptr++|=(c&(0x80>>(f&7)))?(*pmaskp):0;
		   (*xp)++;
		   if (*xp>=w) {
			if (real_bpp==1) {
			    (*xp)=0,(*yp)++;
			    return;
			}
			(*xp)=0;
			(*planep)++;
			(*pmaskp)<<=1;
			if (real_bpp==2) {
			    if (*planep==2) {
				(*yp)++;
				return;
			    }
			} else if (real_bpp==3) {
			    if (*planep==3) {
				(*yp)++;
				return;
			    }
			} else {
			    /* otherwise, it's 4 bpp */
			    if (*planep==4) {
				(*yp)++;
				return;
			    }
			}
		    }
		    if ((*yp)>=h)
			return;
		}
		break;
	  
	  case 8:
		*(ptr+(*yp)*w+*xp)=c;
		(*xp)++;
		if (*xp>=byteline) {
		    (*xp)=0;
		    (*yp)++;
		}
		break;
	  
	  case 24:
		*(ptr+((*yp)*w+*xp)*3+(2-(*planep)))=c;
		(*xp)++;
		if (*xp>=byteline) {
		    (*xp)=0;
		    (*planep)++; /* no need to change pmask */
		    if (*planep==3) {
			(*yp)++;
			return;
		    }
		}
		break;
	  }
}
