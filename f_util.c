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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "mode.h"
#include "f_neuclrtab.h"
#include "f_read.h"
#include "f_util.h"
#include "u_create.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_util.h"
#include "w_icons.h"
#include "w_msgpanel.h"
#include "version.h"

#include "f_save.h"
#include "u_fonts.h"
#include "w_cursor.h"

/* LOCALS */

int	count_colors(void);
int	count_pixels(void);


/* PROCEDURES */


void beep (void);
void alloc_imagecolors (int num);
void add_all_pixels (void);
void remap_image_colormap (void);
void extract_cmap (void);
void readjust_cmap (void);
void free_pixmaps (F_compound *obj);
void add_recent_file (char *file);
int strain_out (char *name);
void finish_update_xfigrc (void);

int
emptyname(char *name)
{
    if  (name == NULL || *name == '\0') {
	return (1);
    } else {
	return (0);
    }
}

int
emptyname_msg(char *name, char *msg)
{
    int		    returnval;

    if (returnval = emptyname(name)) {
	put_msg("No file name specified, %s command ignored", msg);
	beep();
    }
    return (returnval);
}

int
emptyfigure(void)
{
    if (objects.texts != NULL)
	return (0);
    if (objects.lines != NULL)
	return (0);
    if (objects.ellipses != NULL)
	return (0);
    if (objects.splines != NULL)
	return (0);
    if (objects.arcs != NULL)
	return (0);
    if (objects.compounds != NULL)
	return (0);
    return (1);
}

int
emptyfigure_msg(char *msg)
{
    int		    returnval;

    if (returnval = emptyfigure()) {
	put_msg("Empty figure, %s command ignored", msg);
	beep();
    }
    return (returnval);
}

int
change_directory(char *path)
{
    if (path == NULL || *path == '\0')
	return 0;
    if (chdir(path) == -1) {
	file_msg("Can't go to directory %s, : %s", path, strerror(errno));
	return 1;
    }
    return 0;
}

int get_directory(char *direct)
{
#if defined(SYSV) || defined(SVR4) || defined(_POSIX_SOURCE)
    extern char	   *getcwd(char *, size_t);

#else
    extern char	   *getwd();

#endif /* defined(SYSV) || defined(SVR4) || defined(_POSIX_SOURCE) */

#if defined(SYSV) || defined(SVR4) || defined(_POSIX_SOURCE)
    if (getcwd(direct, PATH_MAX) == NULL) {	/* get current working dir */
	file_msg("Can't get current directory");
	beep();
#else
    if (getwd(direct) == NULL) {		/* get current working dir */
	file_msg("%s", direct);			/* err msg is in direct var */
	beep();
#endif /* defined(SYSV) || defined(SVR4) || defined(_POSIX_SOURCE) */
	*direct = '\0';
	return 0;
    }
    return 1;
}

#ifndef S_IWUSR
#define S_IWUSR 0000200
#endif /* S_IWUSR */

#ifndef S_IWGRP
#define S_IWGRP 0000020
#endif /* S_IWGRP */

#ifndef S_IWOTH
#define S_IWOTH 0000002
#endif /* S_IWOTH */

int
ok_to_write(char *file_name, char *op_name)
{
    struct stat	    file_status;
    char	    string[180];

    if (stat(file_name, &file_status) == 0) {	/* file exists */
	if (file_status.st_mode & S_IFDIR) {
	    put_msg("\"%s\" is a directory", file_name);
	    beep();
	    return 0;
	}
	if (file_status.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) {
	    /* writing is permitted by SOMEONE */
	    if (access(file_name, W_OK)) {
		put_msg("Write permission for \"%s\" is denied", file_name);
		beep();
		return 0;
	    } else {
		if (warnexist) {
		    sprintf(string, "\"%s\" already exists.\nDo you want to overwrite it?", file_name);
		    if (popup_query(QUERY_YESNO, string) != RESULT_YES) {
			put_msg("%s cancelled", op_name);
			return 0;
		    }
		} else {
		    return 1;
		}
	    }
	} else {
	    put_msg("\"%s\" is read only", file_name);
	    beep();
	    return 0;
	}
    } else {
	if (errno != ENOENT)
	    return 0;		/* file does exist but stat fails */
    }

    return  1;
}

/* for systems without basename() (e.g. SunOS 4.1.3) */
/* strip any path from filename */

char *
xf_basename(char *filename)
{
    char	   *p;
    if (filename == NULL || *filename == '\0')
	return filename;
    if (p=strrchr(filename,'/')) {
	return ++p;
    } else {
	return filename;
    }
}

static int	  scol, ncolors;
static int	  num_oldcolors = -1;
static Boolean	  usenet;
static int	  npixels;

#define REMAP_MSG	"Remapping picture colors..."
#define REMAP_MSG2	"Remapping picture colors...Done"

/* remap the colors for all the pictures in the picture repository */

void remap_imagecolors(void)
{
    int		    i;

    /* if monochrome, return */
    if (tool_cells <= 2 || appres.monochrome)
	return;

    npixels = 0;

    /* first see if there are enough colorcells for all image colors */
    usenet = False;

    /* see if the total number of colors will fit without using the neural net */
    ncolors = count_colors();
    if (ncolors == 0)
	return;

    put_msg(REMAP_MSG);
    set_temp_cursor(wait_cursor);
    app_flush();

    if (ncolors > appres.max_image_colors) {
	if (appres.DEBUG) 
		fprintf(stderr,"More colors (%d) than allowed (%d), using neural net\n",
				ncolors,appres.max_image_colors);
	ncolors = appres.max_image_colors;
	usenet = True;
    }

    /* if this is the first image, allocate the number of colorcells we need */
    if (num_oldcolors != ncolors) {
	if (num_oldcolors != -1) {
	    unsigned long   pixels[MAX_USR_COLS];
	    for (i=0; i<num_oldcolors; i++)
		pixels[i] = image_cells[i].pixel;
	    if (tool_vclass == PseudoColor)
		XFreeColors(tool_d, tool_cm, pixels, num_oldcolors, 0);
	}
	alloc_imagecolors(ncolors);
	/* hmm, we couldn't get that number of colors anyway; use the net, Luke */
	if (ncolors > avail_image_cols) {
	    usenet = True;
	    if (appres.DEBUG) 
		fprintf(stderr,"More colors (%d) than available (%d), using neural net\n",
				ncolors,avail_image_cols);
	}
	num_oldcolors = avail_image_cols;
	if (avail_image_cols < 2 && ncolors >= 2) {
	    file_msg("Cannot allocate even 2 colors for pictures");
	    reset_cursor();
	    num_oldcolors = -1;
	    reset_cursor();
	    put_msg(REMAP_MSG2);
	    app_flush();
	    return;
	}
    }
    reset_cursor();

    if (usenet) {
	int	stat;
	int	mult = 1;

	/* check if user pressed cancel button (in file preview) */
	if (check_cancel())
	    return;

	/* count total number of pixels in all the pictures */
	npixels = count_pixels();

	/* check if user pressed cancel button */
	if (check_cancel())
	    return;

	/* initialize the neural network */
	/* -1 means can't alloc memory, -2 or more means must have that many times
		as many pixels */
	set_temp_cursor(wait_cursor);
	if ((stat=neu_init(npixels)) <= -2) {
	    mult = -stat;
	    npixels *= mult;
	    /* try again with more pixels */
	    stat = neu_init2(npixels);
	}
	if (stat == -1) {
	    /* couldn't alloc memory for network */
	    fprintf(stderr,"Can't alloc memory for neural network\n");
	    reset_cursor();
	    put_msg(REMAP_MSG2);
	    app_flush();
	    return;
	}
	/* now add all pixels to the samples */
	for (i=0; i<mult; i++)
	    add_all_pixels();

	/* make a new colortable with the optimal colors */
	avail_image_cols = neu_clrtab(avail_image_cols);

	/* now change the color cells with the new colors */
	/* clrtab[][] is the colormap produced by neu_clrtab */
	for (i=0; i<avail_image_cols; i++) {
	    image_cells[i].red   = (unsigned short) clrtab[i][N_RED] << 8;
	    image_cells[i].green = (unsigned short) clrtab[i][N_GRN] << 8;
	    image_cells[i].blue  = (unsigned short) clrtab[i][N_BLU] << 8;
	}
	YStoreColors(tool_cm, image_cells, avail_image_cols);
	reset_cursor();

	/* check if user pressed cancel button */
	if (check_cancel())
	    return;

	/* get the new, mapped indices for the image colormap */
	remap_image_colormap();
    } else {
	/*
	 * Extract the RGB values from the image's colormap and allocate
	 * the appropriate X colormap entries.
	 */
	scol = 0;	/* global color counter */
	set_temp_cursor(wait_cursor);
	extract_cmap();
	for (i=0; i<scol; i++) {
	    image_cells[i].flags = DoRed|DoGreen|DoBlue;
	}
	YStoreColors(tool_cm, image_cells, scol);
	scol = 0;	/* global color counter */
	readjust_cmap();
	if (appres.DEBUG) 
	    fprintf(stderr,"Able to use %d colors without neural net\n",scol);
	reset_cursor();
    }
    put_msg(REMAP_MSG2);
    app_flush();
}

/* allocate the color cells for the pictures */

void alloc_imagecolors(int num)
{
    int		    i;

    /* see if we can get all user wants */
    avail_image_cols = num;
    for (i=0; i<avail_image_cols; i++) {
	image_cells[i].flags = DoRed|DoGreen|DoBlue;
	if (!alloc_color_cells(&image_cells[i].pixel, 1)) {
	    break;
	}
    }
    avail_image_cols = i;
}

/* count the number of colors in all the pictures in the picture repository */

int
count_colors(void)
{
    int		    ncolors;
    struct _pics   *pics;

    ncolors = 0;
    for (pics = pictures; pics; pics = pics->next)
	if (pics->bitmap != NULL)
		ncolors += pics->numcols;
    return ncolors;
}

int
count_pixels(void)
{
    int		    npixels;
    struct _pics   *pics;

    npixels = 0;
    for (pics = pictures; pics; pics = pics->next)
	if (pics->bitmap != NULL && pics->numcols > 0)
	    npixels += pics->bit_size.x * pics->bit_size.y;
    return npixels;
}

void readjust_cmap(void)
{
    struct _pics   *pics;
    int		   i, j;

    /* first adjust the colormaps in the repository */
    for (pics = pictures; pics; pics = pics->next)
	if (pics->bitmap != NULL && pics->numcols > 0) {
	    for (i=0; i<pics->numcols; i++) {
		j = pics->cmap[i].pixel;
		pics->cmap[i].pixel = image_cells[j].pixel;
		scol++;
	    }
	}

    /* now free up all pixmaps in picture objects */
    /* start with main list */
    free_pixmaps(&objects);
}

void free_pixmaps(F_compound *obj)
{
    F_line	   *l;
    F_compound	   *c;

    /* traverse the compounds in this compound */
    for (c = obj->compounds; c != NULL; c = c->next) {
	free_pixmaps(c);
    }
    for (l = obj->lines; l != NULL; l = l->next) {
	if (l->type == T_PICTURE) {
	    if (l->pic->pixmap != (Pixmap) NULL) {
		if (l->pic->pixmap)
		    XFreePixmap(tool_d, l->pic->pixmap);
		l->pic->pixmap = 0;		/* this will force regeneration of the pixmap */
		if (l->pic->mask != 0)
		    XFreePixmap(tool_d, l->pic->mask);
		l->pic->mask = 0;
	    }
	}
    }
}

void extract_cmap(void)
{
    struct _pics   *pics;
    int		    i;

    /* extract the colormaps in the repository */
    for (pics = pictures; pics; pics = pics->next)
	if (pics->bitmap != NULL && pics->numcols > 0) {
	    for (i=0; i<pics->numcols; i++) {
		image_cells[scol].red   = pics->cmap[i].red << 8;
		image_cells[scol].green = pics->cmap[i].green << 8;
		image_cells[scol].blue  = pics->cmap[i].blue << 8;
		pics->cmap[i].pixel = scol;
		scol++;
	    }
	}
    /* now free up the pixmaps */
    free_pixmaps(&objects);
}

void add_all_pixels(void)
{
    struct _pics   *pics;
    BYTE	   col[3];
    int		   i, npix;
    register unsigned char byte;

    for (pics = pictures; pics; pics = pics->next)
	if (pics->bitmap != NULL && pics->numcols > 0) {
	    /* now add each pixel to the sample list */
	    npix = pics->bit_size.x * pics->bit_size.y;
	    for (i=0; i < npix; i++) {
		/* check if user pressed cancel button */
		if (i%1000==0 && check_cancel())
		    return;
		byte = pics->bitmap[i];
		col[N_RED] = pics->cmap[byte].red;
		col[N_GRN] = pics->cmap[byte].green;
		col[N_BLU] = pics->cmap[byte].blue;
		neu_pixel(col);
	    }
	}
}

void remap_image_colormap(void)
{
    struct _pics   *pics;
    BYTE	   col[3];
    int		   i;
    int		   p;

    for (pics = pictures; pics; pics = pics->next)
	if (pics->bitmap != NULL && pics->numcols > 0) {
	    for (i=0; i<pics->numcols; i++) {
		/* real color from the image */
		col[N_RED] = pics->cmap[i].red;
		col[N_GRN] = pics->cmap[i].green;
		col[N_BLU] = pics->cmap[i].blue;
		/* X color index from the mapping */
		p = neu_map_pixel(col);
		pics->cmap[i].pixel = image_cells[p].pixel;
	    }
	}
    free_pixmaps(&objects);
}

/* map the bytes in pic->pic_cache->bitmap to bits for monochrome display */
/* DESTROYS original pic->pic_cache->bitmap */
/* uses a Floyd-Steinberg algorithm from the pbmplus package */

/* Here is the copyright notice:
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
*/

#define FS_SCALE 1024
#define HALF_FS_SCALE 512
#define MAXVAL (256*256)

void map_to_mono(F_pic *pic)
{
	unsigned char *dptr = pic->pic_cache->bitmap;	/* 8-bit wide data pointer */
	unsigned char *bptr;			/* 1-bit wide bitmap pointer */
	int	   bitp;
	int	   col, row, limitcol;
	int	   width, height;
	long	   grey, threshval, sum;
	long	  *thiserr, *nexterr, *temperr;
	unsigned char *cP, *bP;
	Boolean	   fs_direction;
	int	   sbit;

	width = pic->pic_cache->bit_size.x;
	height = pic->pic_cache->bit_size.y;

	/* allocate space for 1-bit bitmap */
	if ((bptr = (unsigned char*) 
	     malloc(sizeof(unsigned char) * (width+7)/8*height)) == NULL)
		return;
	thiserr = (long *) malloc(sizeof(long) * (width+2));
	nexterr = (long *) malloc(sizeof(long) * (width+2));
	/* initialize random seed */
	srandom( (int) (time(0)^getpid()) );
	for (col=0; col<width+2; col++) {
	    /* (random errors in [-FS_SCALE/8 .. FS_SCALE/8]) */
	    thiserr[col] = ( random() % FS_SCALE - HALF_FS_SCALE) / 4;
	}
	fs_direction = True;
	threshval = FS_SCALE/2;

	/* starting bit for left-hand scan */
	sbit = 1 << (7-((width-1)%8));

	for (row=0; row<height; row++) {
	    for (col=0; col<width+2; col++)
		nexterr[col] = 0;
	    if (fs_direction) {
		col = 0;
		limitcol = width;
		cP = &dptr[row*width];
		bP = &bptr[row*(int)((width+7)/8)];
		bitp = 0x80;
	    } else {
		col = width - 1;
		limitcol = -1;
		cP = &dptr[row*width+col];
		bP = &bptr[(row+1)*(int)((width+7)/8) - 1];
		bitp = sbit;
	    }
	    do {
		grey =  pic->pic_cache->cmap[*cP].red   * 77  +	/* 0.30 * 256 */
			pic->pic_cache->cmap[*cP].green * 151 +	/* 0.59 * 256 */
			pic->pic_cache->cmap[*cP].blue  * 28;	/* 0.11 * 256 */
		sum = ( grey * FS_SCALE ) / MAXVAL + thiserr[col+1];
		if (sum >= threshval) {
		    *bP |= bitp;		/* white bit */
		    sum = sum - threshval - HALF_FS_SCALE;
		} else {
		    *bP &= ~bitp;		/* black bit */
		}
		if (fs_direction) {
		    bitp >>= 1;
		    if (bitp <= 0) {
			bP++;
			bitp = 0x80;
		    }
		} else { 
		    bitp <<= 1;
		    if (bitp > 0x80) {
			bP--;
			bitp = 0x01;
		    }
		}
		if ( fs_direction )
		    {
		    thiserr[col + 2] += ( sum * 7 ) / 16;
		    nexterr[col    ] += ( sum * 3 ) / 16;
		    nexterr[col + 1] += ( sum * 5 ) / 16;
		    nexterr[col + 2] += ( sum     ) / 16;

		    ++col;
		    ++cP;
		    }
		else
		    {
		    thiserr[col    ] += ( sum * 7 ) / 16;
		    nexterr[col + 2] += ( sum * 3 ) / 16;
		    nexterr[col + 1] += ( sum * 5 ) / 16;
		    nexterr[col    ] += ( sum     ) / 16;

		    --col;
		    --cP;
		    }
		}
	    while ( col != limitcol );
	    temperr = thiserr;
	    thiserr = nexterr;
	    nexterr = temperr;
	    fs_direction = ! fs_direction;
	}
	free((char *) pic->pic_cache->bitmap);
	free((char *) thiserr);
	free((char *) nexterr);
	pic->pic_cache->bitmap = bptr;
	/* monochrome */
	pic->pic_cache->numcols = 0;
		
	return;
}

void beep(void)
{
	XBell(tool_d,0);
}

#ifdef NOSTRSTR

char *strstr(s1, s2)
    char *s1, *s2;
{
    int len2;
    char *stmp;

    len2 = strlen(s2);
    for (stmp = s1; *stmp != NULL; stmp++)
	if (strncmp(stmp, s2, len2)==0)
	    return stmp;
    return NULL;
}
#endif /* NOSTRSTR */

/* strncasecmp and strcasecmp by Fred Appelman (Fred.Appelman@cv.ruu.nl) */

#ifdef HAVE_NO_STRNCASECMP
int
strncasecmp(const char* s1, const char* s2, int n)
{
   char c1,c2;

   while (--n>=0)
   {
	  /* Check for end of string, if either of the strings
	   * is ended, we can terminate the test
	   */
	  if (*s1=='\0' && *s2!='\0') return -1; /* s1 ended premature */
	  if (*s1!='\0' && *s2=='\0') return +1; /* s2 ended premature */

	  c1=toupper(*s1++);
	  c2=toupper(*s2++);
	  if (c1<c2) return -1; /* s1 is "smaller" */
	  if (c1>c2) return +1; /* s2 is "smaller" */
   }
   return 0;
}

#endif /* HAVE_NO_STRNCASECMP */

#ifdef HAVE_NO_STRCASECMP
int
strcasecmp(const char* s1, const char* s2)
{
   char c1,c2;

   while (*s1 && *s2)
   {
	  c1=toupper(*s1++);
	  c2=toupper(*s2++);
	  if (c1<c2) return -1; /* s1 is "smaller" */
	  if (c1>c2) return +1; /* s2 is "smaller" */
   }
   /* Check for end of string, if not both the strings ended they are 
    * not the same. 
	*/
   if (*s1=='\0' && *s2!='\0') return -1; /* s1 ended premature */
   if (*s1!='\0' && *s2=='\0') return +1; /* s2 ended premature */
   return 0;
}

#endif /* HAVE_NO_STRCASECMP */

/* this routine will safely copy overlapping strings */
/* p2 is copied to p1 and p1 is returned */
/* p1 must be < p2 */

char *
safe_strcpy(char *p1, char *p2)
{
    char *c1;
    c1 = p1;
    for ( ; *p2; )
	*p1++ = *p2++;
    *p1 = '\0';
    return c1;
}

/* gunzip file if necessary */

Boolean
uncompress_file(char *name)
{
    char	    plainname[PATH_MAX];
    char	    dirname[PATH_MAX];
    char	    tmpfile[PATH_MAX];
    char	    unc[PATH_MAX+20];	/* temp buffer for uncompress/gunzip command */
    char	   *c;
    struct stat	    status;

    strcpy(tmpfile, name);		/* save original name */
    strcpy(plainname, name);
    c = strrchr(plainname, '.');
    if (c) {
      if (strcmp(c, ".gz") == 0 || strcmp(c, ".Z") == 0 || strcmp(c, ".z") == 0)
	*c = '\0';
    }

    if (stat(name, &status)) {    /* first see if file exists AS NAMED */
      /* no, try without .gz etc suffix */
      strcpy(name, plainname);
      if (stat(name, &status)) {
	/* no, try with a .z */
	sprintf(name, "%s.z", plainname);
	if (stat(name, &status)) {
	  /* no, try with .Z suffix */
	  sprintf(name, "%s.Z", plainname);
	  if (stat(name, &status)) {
	    /* no, try .gz */
	    sprintf(name, "%s.gz", plainname);
	    if (stat(name, &status)) {
	      /* none of the above, return original name and False status */
	      strcpy(name, tmpfile);
	      return False;
	    }
	  }
	}
      }
    }
    /* file doesn't have .gz etc suffix anymore, return modified name */
    if (strcmp(name, plainname) == 0) return True;

    strcpy(dirname, name);
    c = strrchr(dirname, '/');
    if (c) *c = '\0';
    else strcpy(dirname, ".");

    if (access(dirname, W_OK) == 0) {  /* OK - the directory is writable */
      sprintf(unc, "gunzip -q %s", name);
      if (system(unc) != 0)
	file_msg("Couldn't uncompress the file: \"%s\"", unc);
      strcpy(name, plainname);
    } else {  /* the directory is not writable */
      /* create a path to TMPDIR in case we need to uncompress a read-only file to there */
      c = strrchr(plainname, '/');
      if (c)
	  sprintf(tmpfile, "%s%s", TMPDIR, c);
      else
	  sprintf(tmpfile, "%s/%s", TMPDIR, plainname);
      sprintf(unc, "gunzip -q -c %s > %s", name, tmpfile);
      if (system(unc) != 0)
	  file_msg("Couldn't uncompress the file: \"%s\"", unc);
      file_msg ("Uncompressing file %s in %s because it is in a read-only directory",
		name, TMPDIR);
      strcpy(name, tmpfile);
    }
    return True;
}

/*************************************************************/
/* Read the user's .xfigrc file in his home directory to get */
/* settings from previous session.                           */
/*************************************************************/

#define RC_BUFSIZ 1000

void read_xfigrc(void)
{
    char  line[RC_BUFSIZ+1], *word, *opnd;
    int	  i, len;
    FILE *xfigrc;

    num_recent_files = 0;
    max_recent_files = DEF_RECENT_FILES;	/* default if none found in .xfigrc */

    /* make the filename from the user's home path and ".xfigrc" */
    strcpy(xfigrc_name,userhome);
    strcat(xfigrc_name,"/.xfigrc");
    xfigrc = fopen(xfigrc_name,"r");
    if (xfigrc == 0)
	return;		/* no .xfigrc file */
    
    /* there must not be any whitespace between word and ":" */
    while (fgets(line, RC_BUFSIZ, xfigrc) != NULL) {
	word = strtok(line, ": \t");
	opnd = strtok(NULL, "\n");		/* parse operand and remove newline */
	if (!word || !opnd)
	    continue;
	/* find first non-blank */
	for (i=0, len=strlen(opnd); i<len; i++, opnd++)
	    if (*opnd != ' ' && *opnd != '\t')
		break;
	/* nothing, do next */
	if (i==len)
	    continue;
	if (strcasecmp(word, "max_recent_files") == 0)
	    max_recent_files = min2(MAX_RECENT_FILES, atoi(opnd));
	else if (strcasecmp(word, "file") == 0)
	    add_recent_file(opnd);
    }
    fclose(xfigrc);
}

/* add next token from 'line' to recent file list */

void add_recent_file(char *file)
{
    char  *name;

    if (file == NULL)
	return;
    if (num_recent_files >= MAX_RECENT_FILES || num_recent_files >= max_recent_files)
	return;
    name = new_string(strlen(file)+3);	/* allow for file number (1), blank (1) and NUL */
    sprintf(name,"%1d %s",num_recent_files+1,file);
    recent_files[num_recent_files].name = name;
    num_recent_files++;
}


static FILE	*xfigrc, *tmpf;
static char	tmpname[PATH_MAX];

/* rewrite .xfigrc file with current list of recent files */

void update_recent_files(void)
{
    int	    i;

    /* copy all lines without the "file" spec into a temp file */
    if (strain_out("file") != 0) {
	if (tmpf != 0)
	    fclose(tmpf);
	if (xfigrc != 0)
	    fclose(xfigrc);
	return;	/* problem creating temp file */
    }

    /* now append file list */
    for (i=0; i<num_recent_files; i++)
    	fprintf(tmpf, "file: %s\n",&recent_files[i].name[2]);  /* point past the number */
    /* close and rename the files */
    finish_update_xfigrc();
}

/* update a named entry in the user's .xfigrc file */

void update_xfigrc(char *name, char *string)
{
    /* first copy all lines except the one we want to the temp file */
    strain_out(name);
    /* add the new name/string value to the file */
    fprintf(tmpf, "%s: %s\n",name,string);
    /* close and rename the files */
    finish_update_xfigrc();
}

/* copy all lines from .xfigrc without the "name" spec into a temp file (global tmpf) */
/* temp file is left open after copy */

int strain_out(char *name)
{
    char    line[RC_BUFSIZ+1], *tok;

    /* make a temp filename in the user's home directory so we
       can just rename it to .xfigrc after creating it */
    sprintf(tmpname, "%s/%s%06d", userhome, "xfig-xfigrc", getpid());
    tmpf = fopen(tmpname,"wb");
    if (tmpf == 0) {
	file_msg("Can't make temporary file for .xfigrc - error: %s",strerror(errno));
	return -1;	
    }
    /* read the .xfigrc file and write all to temp file except file names */
    xfigrc = fopen(xfigrc_name,"r");
    /* does the .xfigrc file exist? */
    if (xfigrc == 0) {
	/* no, create one */
	xfigrc = fopen(xfigrc_name,"wb");
	if (xfigrc == 0) {
	    file_msg("Can't create ~/.xfigrc - error: %s",strerror(errno));
	    return -1;
	}

	fclose(xfigrc);
	xfigrc = (FILE *) 0;
	return 0;
    }
    while (fgets(line, RC_BUFSIZ, xfigrc) != NULL) {
	/* look for sane input */
	if (line[0] == '\0')
	    break;
	/* make copy of line to look for token because strtok modifies the line */
	tok = my_strdup(line);
	if (strcasecmp(strtok(tok, ": \t"), name) == 0)
	    continue;			/* match, skip */
	fputs(line, tmpf);
	free(tok);
    }
    return 0;
}

void finish_update_xfigrc(void)
{
    fclose(tmpf);
    if (xfigrc != 0)
	fclose(xfigrc);
    /* delete original .xfigrc and move temp file to .xfigrc */
    if (unlink(xfigrc_name) != 0) {
	file_msg("Can't update your .xfigrc file - error: %s",strerror(errno));
	return;
    }
    if (rename(tmpname, xfigrc_name) != 0)
	file_msg("Can't rename %s to .xfigrc - error: %s",tmpname, strerror(errno));
}

/************************************************/
/* Copy initial appres settings to current vars */
/************************************************/

void init_settings(void)
{
    /* also initialize print/export grid strings */
    minor_grid[0] = major_grid[0] = '\0';

    if (appres.startfontsize >= 1.0)
	cur_fontsize = round(appres.startfontsize);

    /* allow "Modern" for "Sans Serif" and allow "SansSerif" (no space) */
    if (appres.startlatexFont) {
        if (strcmp(appres.startlatexFont,"Modern")==0 ||
	    strcmp(appres.startlatexFont,"SansSerif")==0)
	      cur_latex_font = latexfontnum ("Sans Serif");
    } else {
	    cur_latex_font = latexfontnum (appres.startlatexFont);
    }

    cur_ps_font = psfontnum (appres.startpsFont);

    if (appres.startarrowtype >= 0)
	cur_arrowtype = appres.startarrowtype;

    if (appres.startarrowthick > 0.0) {
	use_abs_arrowvals = True;
	cur_arrowthick = appres.startarrowthick;
    }

    if (appres.startarrowwidth > 0.0) {
	use_abs_arrowvals = True;
	cur_arrowwidth = appres.startarrowwidth;
    }

    if (appres.startarrowlength > 0.0) {
	use_abs_arrowvals = True;
	cur_arrowheight = appres.startarrowlength;
    }

    if (appres.starttextstep > 0.0)
	cur_textstep = appres.starttextstep;

    if (appres.startfillstyle >= 0)
	cur_fillstyle = min2(appres.startfillstyle,NUMFILLPATS-1);

    if (appres.startlinewidth >= 0)
	cur_linewidth = min2(appres.startlinewidth,MAX_LINE_WIDTH);

    if (appres.startgridmode >= 0)
	cur_gridmode = min2(appres.startgridmode,GRID_4);

    if (appres.startposnmode >= 0)
	cur_pointposn = min2(appres.startposnmode,P_GRID4);

    /* choose grid units (1/16", 1/10", MM) */
    /* default */
    grid_unit = FRACT_UNIT;

    /* if inches is desired set grid unit to user spec (1/16" or 1/10") */
    if (appres.INCHES) {
	/* make sure user specified one */
	if (strcasecmp(appres.tgrid_unit, "default") != 0) {
	    if (strcasecmp(appres.tgrid_unit, "tenth") == 0 || 
		strcasecmp(appres.tgrid_unit, "ten") == 0 ||
		strcasecmp(appres.tgrid_unit, "1/10") == 0 ||
		strcasecmp(appres.tgrid_unit, "10") == 0)
		    grid_unit = TENTH_UNIT;
		else
		    grid_unit = FRACT_UNIT;
	}
    } else {
	grid_unit = MM_UNIT;
    }

    /* set current and "old" grid unit */
    old_gridunit = cur_gridunit = grid_unit;

    /* turn off PSFONT_TEXT flag if user specified -latexfonts */
    if (appres.latexfonts)
	cur_textflags = cur_textflags & (~PSFONT_TEXT);
    if (appres.specialtext)
	cur_textflags = cur_textflags | SPECIAL_TEXT;
    if (appres.rigidtext)
	cur_textflags = cur_textflags | RIGID_TEXT;
    if (appres.hiddentext)
	cur_textflags = cur_textflags | HIDDEN_TEXT;

    /* turn off PSFONT_TEXT flag if user specified -latexfonts */
    if (appres.latexfonts)
	cur_textflags = cur_textflags & (~PSFONT_TEXT);

    if (appres.userunit)
	strncpy(cur_fig_units, appres.userunit, sizeof(cur_fig_units)-1);
    else
	cur_fig_units[0] = '\0';

    strcpy(cur_library_dir, appres.library_dir);
    strcpy(cur_spellchk, appres.spellcheckcommand);
    strcpy(cur_image_editor, appres.image_editor);
    strcpy(cur_browser, appres.browser);
    strcpy(cur_pdfviewer, appres.pdf_viewer);

    /* assume color to start */
    all_colors_available = True;

    /* check if monochrome screen */
    if (tool_cells == 2 || appres.monochrome)
	all_colors_available = False;

} /* init_settings() */

/* This is called to read a list of Fig files specified in the command line
   and write them back (renaming the original to xxxx.fig.bak) so that they
   are updated to the current version.
   If the file is already in the current version it is untouched.
*/

int
update_fig_files(int argc, char **argv)
{
    fig_settings    settings;
    char	    file[PATH_MAX];
    int		    i,col;
    Boolean	    status;
    int		    allstat;

    /* overall status - if any one file can't be read, return status is 1 */
    allstat = 0;

    update_figs = True;

    for (i=1; i<argc; i++) {
	/* skip any other options the user may have given */
	if (argv[i][0] == '-') {
	    continue;
	}
	strcpy(file,argv[i]);
	fprintf(stderr,"* Reading %s ... ",file);
	/* reset user colors */
	for (col=0; col<MAX_USR_COLS; col++)
	    n_colorFree[col] = True;
	/* read Fig file but don't import any images */
	status = read_fig(file, &objects, DONT_MERGE, 0, 0, &settings);
	if (status != 0) {
	    fprintf(stderr," *** Error in reading, not updating this file\n");
	    allstat = 1;
	} else {
	    fprintf(stderr,"Ok. Renamed to %s.bak. ",file);
	    /* now rename original file to file.bak */
	    renamefile(file);
	    fprintf(stderr,"Writing as protocol %s ... ",PROTOCOL_VERSION);
	    /* first update the settings from appres */
	    appres.landscape = settings.landscape;
	    appres.flushleft = settings.flushleft;
	    appres.INCHES = settings.units;
	    appres.papersize = settings.papersize;
	    appres.magnification = settings.magnification;
	    appres.multiple = settings.multiple;
	    appres.transparent = settings.transparent;
	    /* copy user colors */
	    for (col=0; col<MAX_USR_COLS; col++) {
		colorUsed[col] = !n_colorFree[col];
		user_colors[col].red = n_user_colors[col].red;
		user_colors[col].green = n_user_colors[col].green;
		user_colors[col].blue = n_user_colors[col].blue;
	    }
	    /* now write out the new one */
	    num_usr_cols = MAX_USR_COLS;
	    write_file(file, False);
	    fprintf(stderr,"Ok\n");
	}
    }
    return allstat;
}

/* replace all "%f" in "program" with value in filename */

char *
build_command(char *program, char *filename)
{
    char *cmd = new_string(PATH_MAX*2);
    char  cmd2[PATH_MAX*2];
    char *c1;
    Boolean repl = False;

    if (!cmd)
	return (char *) NULL;
    strcpy(cmd,program);
    while (c1=strstr(cmd,"%f")) {
	repl = True;
	strcpy(cmd2, c1+2);		/* save tail */
	strcpy(c1, filename);		/* change %f to filename */
	strcat(c1, cmd2);		/* append tail */
    }
    /* if no %f was found in the resource, just append the filename to the command */
    if (!repl) {
	strcat(cmd," ");
	strcat(cmd,filename);
    }
    /* append "2> /dev/null" to send stderr to /dev/null and "&" to run in background */
    strcat(cmd," 2> /dev/null &");
    return cmd;
}

/* define the strerror() function to return str_errlist[] if 
   the system doesn't have the strerror() function already */

#ifdef NEED_STRERROR
char *
strerror(e)
	int e;
{
	return sys_errlist[e];
}
#endif /* NEED_STRERROR */

/**************************************************************************/
/* Routine to mimic `my_strdup()' (some machines don't have it)           */
/**************************************************************************/

char
*my_strdup(char *str)
{
    char *s;
    
    if (str==NULL)
	return NULL;
    
    s = new_string(strlen(str)+1);
    if (s!=NULL) 
	strcpy(s, str);
    
    return s;
}

/* for images with no palette, we'll use neural net to reduce to 256 colors with palette */

Boolean
map_to_palette(F_pic *pic)
{
	int	 w,h,x,y;
	int	 mult, neu_stat, size;
	unsigned char *old;
	BYTE	 col[3];

	w = pic->pic_cache->bit_size.x;
	h = pic->pic_cache->bit_size.y;

	mult = 1;
	if ((neu_stat=neu_init(w*h)) <= -2) {
	    mult = -neu_stat;
	    /* try again with more pixels */
	    neu_stat = neu_init2(w*h*mult);
	}
	if (neu_stat == -1) {
	    /* couldn't alloc memory for network */
	    fprintf(stderr,"Can't alloc memory for neural network\n");
	    free(pic->pic_cache->bitmap);
	    return False;
	}
	/* now add all pixels to the samples */
	size = w*h*3;
	for (x=0; x<size;) {
	    col[N_BLU] = pic->pic_cache->bitmap[x++];
	    col[N_GRN] = pic->pic_cache->bitmap[x++];
	    col[N_RED] = pic->pic_cache->bitmap[x++];
	    for (y=0; y<mult; y++) {
		neu_pixel(col);
	    }
	}

	/* make a new colortable with the optimal colors */
	pic->pic_cache->numcols = neu_clrtab(256);

	/* now change the color cells with the new colors */
	/* clrtab[][] is the colormap produced by neu_clrtab */
	for (x=0; x<pic->pic_cache->numcols; x++) {
	    pic->pic_cache->cmap[x].red   = (unsigned short) clrtab[x][N_RED];
	    pic->pic_cache->cmap[x].green = (unsigned short) clrtab[x][N_GRN];
	    pic->pic_cache->cmap[x].blue  = (unsigned short) clrtab[x][N_BLU];
	}

	/* now alloc a 1-byte/per/pixel array for the final colormapped image */
	/* save orig */
	old = pic->pic_cache->bitmap;
	if ((pic->pic_cache->bitmap=malloc(w*(h+2)))==NULL)
	    return False;

	/* and change the 3-byte pixels to the 1-byte */
	for (x=0, y=0; x<size; x+=3, y++) {
	    col[N_BLU] = old[x];
	    col[N_GRN] = old[x+1];
	    col[N_RED] = old[x+2];
	    pic->pic_cache->bitmap[y] = neu_map_pixel(col);
	}
	/* free 3-byte/pixel array */
	free(old);
	return True;
}

/* return pointers to the line components of a dimension line. 
   If passed dimline is not a dimension line, the result is False */

Boolean
dimline_components(F_compound *dimline, F_line **line, F_line **tick1, F_line **tick2, F_line **poly)
{
    F_line *l;

    if (!dimline->comments || strncmp(dimline->comments,"Dimension line:",15) !=0 )
	return False;

    *line = *tick1 = *tick2 = *poly = (F_line *) NULL;
    for (l = dimline->lines; l; l=l->next) {
	if (!l->comments)
		continue;
	if (strcmp(l->comments,"main dimension line")==0)
	    *line = l;
	else if (strcmp(l->comments,"text box")==0)
	    *poly = l;
	else if (strcmp(l->comments,"tick")==0) {
	    if (*tick1 == 0)
		*tick1 = l;
	    else
		*tick2 = l;
	}
    }
    return True;
}

int
find_smallest_depth(F_compound *compound)
{
	F_line	 *l;
	F_spline	 *s;
	F_ellipse	 *e;
	F_arc	 *a;
	F_text	 *t;
	F_compound *c;
	int	  smallest, d1;

	smallest = MAX_DEPTH;
	for (l = compound->lines; l != NULL; l = l->next) {
	    if (l->depth < smallest) smallest = l->depth;
	}
	for (s = compound->splines; s != NULL; s = s->next) {
	    if (s->depth < smallest) smallest = s->depth;
	}
	for (e = compound->ellipses; e != NULL; e = e->next) {
	    if (e->depth < smallest) smallest = e->depth;
	}
	for (a = compound->arcs; a != NULL; a = a->next) {
	    if (a->depth < smallest) smallest = a->depth;
	}
	for (t = compound->texts; t != NULL; t = t->next) {
	    if (t->depth < smallest) smallest = t->depth;
	}
	for (c = compound->compounds; c != NULL; c = c->next) {
	    d1 = find_smallest_depth(c);
	    if (d1 < smallest)
		smallest = d1;
	}
	return smallest;
}

int
find_largest_depth(F_compound *compound)
{
	F_line	 *l;
	F_spline	 *s;
	F_ellipse	 *e;
	F_arc	 *a;
	F_text	 *t;
	F_compound *c;
	int	  largest, d1;

	largest = MIN_DEPTH;
	for (l = compound->lines; l != NULL; l = l->next) {
	    if (l->depth > largest) largest = l->depth;
	}
	for (s = compound->splines; s != NULL; s = s->next) {
	    if (s->depth > largest) largest = s->depth;
	}
	for (e = compound->ellipses; e != NULL; e = e->next) {
	    if (e->depth > largest) largest = e->depth;
	}
	for (a = compound->arcs; a != NULL; a = a->next) {
	    if (a->depth > largest) largest = a->depth;
	}
	for (t = compound->texts; t != NULL; t = t->next) {
	    if (t->depth > largest) largest = t->depth;
	}
	for (c = compound->compounds; c != NULL; c = c->next) {
	    d1 = find_largest_depth(c);
	    if (d1 > largest)
		largest = d1;
	}
	return largest;
}

/* get grid params and assemble into fig2dev parm */
void
get_grid_spec(char *grid, Widget minor_grid_panel, Widget major_grid_panel)
{
	char	   *c1;

	/* if panel hasn't been up yet */
	if (minor_grid_panel == (Widget) 0 || major_grid_panel == (Widget) 0) {
	    sprintf(grid,"%s:%s%s", 
			    	strlen(minor_grid)==0? "0": minor_grid, 
				strlen(major_grid)==0? "0": major_grid,
				appres.INCHES? "in":"mm");
	    return;
	}

	/* get minor grid spec from panel */
	strcpy(grid, panel_get_value(minor_grid_panel));
	if (strcasecmp(grid,"none") == 0) {
	    grid[0]='\0';
	}
	/* now major */
	c1 = panel_get_value(major_grid_panel);
	if (strcasecmp(c1,"none") != 0 && strlen(c1)) {
	    strcat(grid,":");
	    strcat(grid,c1);
	}
	if (strlen(grid))
	    strcat(grid,appres.INCHES? "in":"mm");
}

/* get the timestamp (mtime) of the filename passed */

time_t
file_timestamp(char *file)
{
    struct stat	    file_status;

    if (stat(file, &file_status) != 0)
	return -1;
    return file_status.st_mtime;
}
