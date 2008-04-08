/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by Brian Boyter
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

/* GS bitmap generation added: 13 Nov 1992, by Michael C. Grant
*  (mcgrant@rascals.stanford.edu) adapted from Marc Goldburg's
*  (marcg@rascals.stanford.edu) original idea and code. */

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "paintop.h"
#include "f_picobj.h"
#include "f_util.h"
#include "u_create.h"
#include "u_elastic.h"
#include "w_canvas.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "mode.h"

#include "w_file.h"
#include "w_util.h"

extern	int	read_gif(FILE *file, int filetype, F_pic *pic);
extern	int	read_pcx(FILE *file, int filetype, F_pic *pic);
extern	int	read_epsf(FILE *file, int filetype, F_pic *pic);
extern	int	read_pdf(FILE *file, int filetype, F_pic *pic);
extern	int	read_png(FILE *file, int filetype, F_pic *pic);
extern	int	read_ppm(FILE *file, int filetype, F_pic *pic);
extern	int	read_tif(char *filename, int filetype, F_pic *pic);
extern	int	read_xbm(FILE *file, int filetype, F_pic *pic);

#ifdef USE_JPEG
extern	int	read_jpg(FILE *file, int filetype, F_pic *pic);
#endif /* USE_JPEG */
#ifdef USE_XPM
extern	int	read_xpm(FILE *file, int filetype, F_pic *pic);
#endif /* USE_XPM */

#define MAX_SIZE 255

static	 struct hdr {
	    char	*type;
	    char	*bytes;
	    int		 nbytes;
	    int		(*readfunc)();
	    Boolean	pipeok;
	}
	headers[]= {    {"GIF", "GIF",		    3, read_gif,	True},
			{"PCX", "\012\005\001",	    3, read_pcx,	True},
			{"EPS", "%!",		    2, read_epsf,	True},
			{"PDF", "%PDF",		    2, read_pdf,	True},
			{"PPM", "P3",		    2, read_ppm,	True},
			{"PPM", "P6",		    2, read_ppm,	True},
			{"TIFF", "II*\000",	    4, read_tif,	False},
			{"TIFF", "MM\000*",	    4, read_tif,	False},
			{"XBM", "#define",	    7, read_xbm,	True},
#ifdef USE_JPEG
			{"JPEG", "\377\330\377\340", 4, read_jpg,	True},
			{"JPEG", "\377\330\377\341", 4, read_jpg,       True},
#endif /* USE_JPEG */
			{"PNG", "\211\120\116\107\015\012\032\012", 8, read_png, True},
#ifdef USE_XPM
			{"XPM", "/* XPM */",	    9, read_xpm,	False},
#endif /* USE_XPM */
			};

#define NUMHEADERS sizeof(headers)/sizeof(headers[0])

/*
 * Check through the pictures repository to see if "file" is already there.
 * If so, set the pic->pic_cache pointer to that repository entry and set
 * "existing" to True.
 * If not, read the file via the relevant reader and add to the repository
 * and set "existing" to False.
 * If "force" is true, read the file unconditionally.
 */



void read_picobj(F_pic *pic, char *file, int color, Boolean force, Boolean *existing)
{
    FILE	   *fd;
    int		    type;
    int		    i,j,c;
    char	    buf[20],realname[PATH_MAX];
    Boolean	    found, reread;
    struct _pics   *pics, *lastpic;
    time_t	    mtime;

    pic->color = color;
    /* don't touch the flipped flag - caller has already set it */
    pic->pixmap = (Pixmap) NULL;
    pic->hw_ratio = 0.0;
    pic->pix_rotation = 0;
    pic->pix_width = 0;
    pic->pix_height = 0;
    pic->pix_flipped = 0;

    /* check if user pressed cancel button */
    if (check_cancel())
	return;

    put_msg("Reading Picture object file...");
    app_flush();

    /* look in the repository for this filename */
    lastpic = pictures;
    reread = False;
    for (pics = pictures; pics; pics = pics->next) {
	if (strcmp(pics->file, file)==0) {
	    /* found it - make sure the timestamp is >= the timestamp of the file  */
	    /* check both the "realname" and the original name */
	    if ((mtime = file_timestamp(pics->realname) < 0))
		mtime = file_timestamp(pics->file);
	    if (mtime < 0) {
		/* oops, doesn't exist? */
		file_msg("Error %s on %s",strerror(errno),file);
		return;
	    }
	    /* or if force is true then reread it */
	    if (force || (mtime > pics->time_stamp)) {
		reread = True;
		break;			/* no, re-read the file */
	    }
	    pic->pic_cache = pics;
	    pics->refcount++;
	    if (appres.DEBUG)
		fprintf(stderr,"Found stored picture %s, count=%d\n",file,pics->refcount);
	    /* if there is a bitmap, return, otherwise fall through and reread the file */
	    if (pics->bitmap != NULL) {
		*existing = True;
		put_msg("Reading Picture object file...found cached picture");
		/* must set the h/w ratio here */
		pic->hw_ratio = (float) pic->pic_cache->bit_size.y/pic->pic_cache->bit_size.x;
		return;
	    }
	    if (appres.DEBUG)
		fprintf(stderr,"Re-reading file\n");
	}
	/* keep pointer to last entry */
	lastpic = pics;
    }
    *existing = False;
    if (reread) {
	if (appres.DEBUG)
	    fprintf(stderr,"Timestamp changed, reread file %s\n",file);
    } else if (pics == NULL) {
	/* didn't find it in the repository, add it */
	pics = create_picture_entry();
	if (lastpic) {
	    /* add to list */
	    lastpic->next = pics;
	    pics->prev = lastpic;
	} else {
	    /* first one */
	    pictures = pics;
	}
	pics->file = strdup(file);
	pics->refcount = 1;
	pics->bitmap = (unsigned char *) NULL;
	pics->subtype = T_PIC_NONE;
	pics->numcols = 0;
	pics->size_x = 0;
	pics->size_y = 0;
	pics->bit_size.x = 0;
	pics->bit_size.y = 0;
	if (appres.DEBUG)
	    fprintf(stderr,"New picture %s\n",file);
    }
    /* put it in the pic */
    pic->pic_cache = pics;
    pic->pixmap = (Pixmap) NULL;

    /* open the file and read a few bytes of the header to see what it is */
    if ((fd=open_picfile(file, &type, PIPEOK, realname)) == NULL) {
	file_msg("No such picture file: %s",file);
	return;
    }
    /* get the modified time and save it */
    pics->time_stamp = file_timestamp(file);
    /* and save the realname (it may be compressed) */
    pics->realname = strdup(realname);

    /* read some bytes from the file */
    for (i=0; i<15; i++) {
	if ((c=getc(fd))==EOF)
	    break;
	buf[i]=(char) c;
    }
    close_picfile(fd,type);

    /* now find which header it is */
    for (i=0; i<NUMHEADERS; i++) {
	found = True;
	for (j=headers[i].nbytes-1; j>=0; j--)
	    if (buf[j] != headers[i].bytes[j]) {
		found = False;
		break;
	    }
	if (found)
	    break;
    }
    if (found) {
	if (headers[i].pipeok) {
	    /* open it again (it may be a pipe so we can't just rewind) */
	    fd=open_picfile(file, &type, headers[i].pipeok, realname);
	    if ( (*headers[i].readfunc)(fd,type,pic) == FileInvalid) {
		file_msg("%s: Bad %s format",file, headers[i].type);
	    }
	} else {
	    /* those routines that can't take a pipe (e.g. xpm) get the real filename */
	    if ( (*headers[i].readfunc)(realname,type,pic) == FileInvalid) {
		file_msg("%s: Bad %s format",file, headers[i].type);
	    }
	}
	put_msg("Reading Picture object file...Done");
	return;
    }

    /* none of the above */
    file_msg("%s: Unknown image format",file);
    put_msg("Reading Picture object file...Failed");
    app_flush();
}

/* 
   Open the file 'name' and return its type (pipe or real file) in 'type'.
   Return the full name in 'retname'.  This will have a .gz or .Z if the file is
   zipped/compressed.
   The return value is the FILE stream.
*/

FILE *
open_picfile(char *name, int *type, Boolean pipeok, char *retname)
{
    char	 unc[PATH_MAX+20];	/* temp buffer for gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	 status;
    char	*gzoption;

    *type = 0;
    *retname = '\0';
    if (pipeok)
	gzoption = "-c";		/* tell gunzip to output to stdout */
    else
	gzoption = "";

    /* see if the filename ends with .Z or with .z or .gz */
    /* if so, generate gunzip command and use pipe (filetype = 1) */
    if ((strlen(name) > 3 && !strcmp(".gz", name + (strlen(name)-3))) ||
	       (strlen(name) > 2 && !strcmp(".Z", name + (strlen(name)-3))) ||
	       (strlen(name) > 2 && !strcmp(".z", name + (strlen(name)-2)))) {
	sprintf(unc,"gunzip -q %s %s",gzoption,name);
	*type = 1;
    /* none of the above, see if the file with .Z or .gz or .z appended exists */
    } else {
	strcpy(retname, name);
	strcat(retname, ".Z");
	if (!stat(retname, &status)) {
	    sprintf(unc, "gunzip %s %s",gzoption,retname);
	    *type = 1;
	    name = retname;
	} else {
	    strcpy(retname, name);
	    strcat(retname, ".z");
	    if (!stat(retname, &status)) {
		sprintf(unc, "gunzip %s %s",gzoption,retname);
		*type = 1;
		name = retname;
	    } else {
		strcpy(retname, name);
		strcat(retname, ".gz");
		if (!stat(retname, &status)) {
		    sprintf(unc, "gunzip %s %s",gzoption,retname);
		    *type = 1;
		    name = retname;
		}
	    }
	}
    }
    /* if a pipe, but the caller needs a file, uncompress the file now */
    if (*type == 1 && !pipeok) {
	char *p;
	system(unc);
	if (p=strrchr(name,'.')) {
	    *p = '\0';		/* terminate name before last .gz, .z or .Z */
	}
	strcpy(retname, name);
	/* force to plain file now */
	*type = 0;
    }

    /* no appendages, just see if it exists */
    /* and restore the original name */
    strcpy(retname, name);
    if (stat(name, &status) != 0) {
	fstream = NULL;
    } else {
	switch (*type) {
	  case 0:
	    fstream = fopen(name, "rb");
	    break;
	  case 1:
	    fstream = popen(unc,"r");
	    break;
	}
    }
    return fstream;
}

void
close_picfile(FILE *file, int type)
{
    char	 line[MAX_SIZE];
    int		 stat;

    if (file == 0)
	return;
    if (type == 0) {
	if ((stat=fclose(file)) != 0)
	    file_msg("Error closing picture file: %s",strerror(errno));
    } else {
	/* for a pipe, must read everything or we'll get a broken pipe message */
        while(fgets(line,MAX_SIZE,file))
		;
	pclose(file);
    }
}
