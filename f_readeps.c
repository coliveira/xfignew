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

#include "fig.h"
#include "resources.h"
#include "object.h"
#include "f_picobj.h"
#include "w_msgpanel.h"
#include "w_setup.h"

#include "w_util.h"

int         _read_pcx(FILE *pcxfile, F_pic *pic);
Boolean	    bitmap_from_gs();

/* read a PDF file */


int read_epsf_pdf (FILE *file, int filetype, F_pic *pic, Boolean pdf_flag);
void lower (char *buf);
int hex (char c);

int
read_pdf(FILE *file, int filetype, F_pic *pic)
{
    return read_epsf_pdf(file, filetype, pic, True);
}

/* read an EPS file */

/* return codes:  PicSuccess   (1) : success
		  FileInvalid (-2) : invalid file
*/

int
read_epsf(FILE *file, int filetype, F_pic *pic)
{
    return read_epsf_pdf(file, filetype, pic, False);
}

int
read_epsf_pdf(FILE *file, int filetype, F_pic *pic, Boolean pdf_flag)
{
    int         nbitmap;
    Boolean     bitmapz;
    Boolean     foundbbx;
    int         nested;
    char       *cp;
    unsigned char *mp;
    unsigned int hexnib;
    int         flag;
    char        buf[300];
    int         llx, lly, urx, ury, bad_bbox;
    unsigned char *last;
    Boolean     useGS;

    useGS = False;

    llx = lly = urx = ury = 0;
    foundbbx = False;
    nested = 0;
    while (fgets(buf, 300, file) != NULL) {

	/* look for /MediaBox for pdf file */
	if (pdf_flag) {
	    if (!strncmp(buf, "/MediaBox", 8)) {	/* look for the MediaBox spec */
		char       *c;

		c = strchr(buf, '[') + 1;
		if (c && sscanf(c, "%d %d %d %d", &llx, &lly, &urx, &ury) < 4) {
		    llx = lly = 0;
		    urx = paper_sizes[0].width * 72 / PIX_PER_INCH;
		    ury = paper_sizes[0].height * 72 / PIX_PER_INCH;
		    file_msg("Bad MediaBox in header, assuming %s size",
			     appres.INCHES ? "Letter" : "A4");
		    app_flush();
		}
	    }
	    /* look for bounding box */
	} else if (!nested && !strncmp(buf, "%%BoundingBox:", 14)) {
	    if (!strstr(buf, "(atend)")) {	/* make sure doesn't say (atend) */
		float       rllx, rlly, rurx, rury;

		if (sscanf(strchr(buf, ':') + 1, "%f %f %f %f", &rllx, &rlly, &rurx, &rury) < 4) {
		    file_msg("Bad EPS file: %s", file);
		    close_picfile(file,filetype);
		    return FileInvalid;
		}
		foundbbx = True;
		llx = round(rllx);
		lly = round(rlly);
		urx = round(rurx);
		ury = round(rury);
		break;
	    }
	} else if (!strncmp(buf, "%%Begin", 7)) {
	    ++nested;
	} else if (nested && !strncmp(buf, "%%End", 5)) {
	    --nested;
	}
    }
    if (!pdf_flag && !foundbbx) {
	file_msg("No bounding box found in EPS file");
	close_picfile(file,filetype);
	return FileInvalid;
    }
    if ((urx - llx) == 0) {
	llx = lly = 0;
	urx = (appres.INCHES ? LETTER_WIDTH : A4_WIDTH) * 72 / PIX_PER_INCH;
	ury = (appres.INCHES ? LETTER_HEIGHT : A4_HEIGHT) * 72 / PIX_PER_INCH;
	file_msg("Bad %s, assuming %s size",
		 pdf_flag ? "/MediaBox" : "EPS bounding box",
		 appres.INCHES ? "Letter" : "A4");
	app_flush();
    }
    pic->hw_ratio = (float) (ury - lly) / (float) (urx - llx);

    pic->pic_cache->size_x = round((urx - llx) * PIC_FACTOR);
    pic->pic_cache->size_y = round((ury - lly) * PIC_FACTOR);
    /* make 2-entry colormap here if we use monochrome */
    pic->pic_cache->cmap[0].red = pic->pic_cache->cmap[0].green = pic->pic_cache->cmap[0].blue = 0;
    pic->pic_cache->cmap[1].red = pic->pic_cache->cmap[1].green = pic->pic_cache->cmap[1].blue = 255;
    pic->pic_cache->numcols = 0;

    if (bad_bbox = (urx <= llx || ury <= lly)) {
	file_msg("Bad values in %s",
		 pdf_flag ? "/MediaBox" : "EPS bounding box");
	close_picfile(file,filetype);
	return FileInvalid;
    }
    bitmapz = False;

    /* look for a preview bitmap */
    if (!pdf_flag) {
	while (fgets(buf, 300, file) != NULL) {
	    lower(buf);
	    if (!strncmp(buf, "%%beginpreview", 14)) {
		sscanf(buf, "%%%%beginpreview: %d %d %*d",
		       &pic->pic_cache->bit_size.x, &pic->pic_cache->bit_size.y);
		bitmapz = True;
		break;
	    }
	}
    }
#ifdef GSBIT
    /* if monochrome and a preview bitmap exists, don't use gs */
    if ((!appres.monochrome || !bitmapz) && !bad_bbox) {
	useGS = bitmap_from_gs(file, filetype, pic, urx, llx, ury, lly, pdf_flag);
    }
#endif /* GSBIT */
    if (!useGS) {
	if (!bitmapz) {
	    file_msg("EPS object read OK, but no preview bitmap found/generated");
	    close_picfile(file,filetype);
	    return PicSuccess;
	} else if (pic->pic_cache->bit_size.x <= 0 || pic->pic_cache->bit_size.y <= 0) {
	    file_msg("Strange bounding-box/bitmap-size error, no bitmap found/generated");
	    close_picfile(file,filetype);
	    return FileInvalid;
	} else {
	    nbitmap = (pic->pic_cache->bit_size.x + 7) / 8 * pic->pic_cache->bit_size.y;
	    pic->pic_cache->bitmap = (unsigned char *) malloc(nbitmap);
	    if (pic->pic_cache->bitmap == NULL) {
		file_msg("Could not allocate %d bytes of memory for %s bitmap\n",
			 pdf_flag ? "PDF" : "EPS", nbitmap);
		close_picfile(file,filetype);
		return PicSuccess;
	    }
	    /* for whatever reason, ghostscript wasn't available or didn't work but there
	     * is a preview bitmap - use that */
	    mp = pic->pic_cache->bitmap;
	    bzero((char *) mp, nbitmap);	/* init bitmap to zero */
	    last = pic->pic_cache->bitmap + nbitmap;
	    flag = True;
	    while (fgets(buf, 300, file) != NULL && mp < last) {
		lower(buf);
		if (!strncmp(buf, "%%endpreview", 12) ||
		    !strncmp(buf, "%%endimage", 10))
		    break;
		cp = buf;
		if (*cp != '%')
		    break;
		cp++;
		while (*cp != '\0') {
		    if (isxdigit(*cp)) {
			hexnib = hex(*cp);
			if (flag) {
			    flag = False;
			    *mp = hexnib << 4;
			} else {
			    flag = True;
			    *mp = *mp + hexnib;
			    mp++;
			    if (mp >= last)
				break;
			}
		    }
		    cp++;
		}
	    }
	}
    }
    /* put in type */
    pic->pic_cache->subtype = T_PIC_EPS;
    close_picfile(file,filetype);
    return PicSuccess;
}

int
hex(char c)
{
    if (isdigit(c))
	return (c - 48);
    else
	return (c - 87);
}

void lower(char *buf)
{
    while (*buf) {
	if (isupper(*buf))
	    *buf = (char) tolower(*buf);
	buf++;
    }
}

#ifdef GSBIT
/* if GhostScript */

/* Read bitmap from gs, return True if success */
Boolean
bitmap_from_gs(file, filetype, pic, urx, llx, ury, lly, pdf_flag)
    FILE       *file;
    int         filetype;
    F_pic      *pic;
    int         urx, llx, ury, lly;
    int         pdf_flag;
{
    static	tempseq = 0;
    char        buf[300];
    FILE       *tmpfp, *pixfile, *gsfile;
    char       *psnam, *driver;
    int         status, wid, ht, nbitmap;
    char        tmpfile[PATH_MAX],
		pixnam[PATH_MAX],
		errnam[PATH_MAX],
		gscom[2 * PATH_MAX];

    wid = urx - llx;
    ht = ury - lly;

    strcpy(tmpfile, pic->pic_cache->file);
    /* is the file a pipe? (This would mean that it is compressed) */
    if (filetype == 1) {	/* yes, now we have to uncompress the file into a temp
				 * file */
	/* re-open the pipe */
	close_picfile(file, filetype);
	file = open_picfile(tmpfile, &filetype, PIPEOK, pixnam);
	sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-eps", getpid());
	if ((tmpfp = fopen(tmpfile, "wb")) == NULL) {
	    file_msg("Couldn't open tmp file %s, %s", tmpfile, strerror(errno));
	    return False;
	}
	while (fgets(buf, 300, file) != NULL)
	    fputs(buf, tmpfp);
	fclose(tmpfp);
    }
    /* make name /TMPDIR/xfig-pic######.pix */
    sprintf(pixnam, "%s/%s%06d.pix", TMPDIR, "xfig-pic", tempseq);
    /* and file name for any error messages from gs */
    sprintf(errnam, "%s/%s%06d.err", TMPDIR, "xfig-pic", tempseq);
    tempseq++;

    /* generate gs command line */
    /* for monochrome, use pbm */
    if (tool_cells <= 2 || appres.monochrome) {
	/* monochrome output */
	driver = "pbmraw";
    } else {
	/* for color, use pcx */
	driver = "pcx256";
    }
    /* avoid absolute paths (for Cygwin with gswin32) by changing directory */
    if (tmpfile[0] == '/') {
	psnam = strrchr(tmpfile, '/');
	*psnam = 0;
	sprintf(gscom, "cd \"%s/\";", tmpfile);
	*psnam++ = '/';		/* Restore name for unlink() below */
    } else {
	psnam = tmpfile;
	gscom[0] = '\0';
    }
    sprintf(&gscom[strlen(gscom)],
	    "%s -r72x72 -dSAFER -sDEVICE=%s -g%dx%d -sOutputFile=%s -q - > %s 2>&1",
	    appres.ghostscript, driver, wid, ht, pixnam, errnam);
    if (appres.DEBUG)
	fprintf(stderr,"calling: %s\n",gscom);
    if ((gsfile = popen(gscom, "w")) == 0) {
	file_msg("Cannot open pipe with command: %s\n", gscom);
	return False;
    }
    /*********************************************
    gs commands (New method)

    W is the width in pixels and H is the height
    gs -dSAFER -sDEVICE=pbmraw(or pcx256) -gWxH -sOutputFile=/tmp/xfig-pic%%%.pix -q -

    -llx -lly translate
    % mark dictionary (otherwise fails for tiger.ps (e.g.):
    % many ps files don't 'end' their dictionaries)
    countdictstack
    mark
    /oldshowpage {showpage} bind def
    /showpage {} def
    /initgraphics {} def	<<< this nasty command should never be used!
    /initmmatrix {} def		<<< this one too
    (psfile) run
    oldshowpage
    % clean up stacks and dicts
    cleartomark
    countdictstack exch sub { end } repeat
    quit
    *********************************************/

    fprintf(gsfile, "%d %d translate\n", -llx, -lly);
    fprintf(gsfile, "countdictstack\n");
    fprintf(gsfile, "mark\n");
    fprintf(gsfile, "/oldshowpage {showpage} bind def\n");
    fprintf(gsfile, "/showpage {} def\n");
    fprintf(gsfile, "/initgraphics {} def\n");
    fprintf(gsfile, "/initmatrix {} def\n");
    fprintf(gsfile, "(%s) run\n", psnam);
    fprintf(gsfile, "oldshowpage\n");
    fprintf(gsfile, "cleartomark\n");
    fprintf(gsfile, "countdictstack exch sub { end } repeat\n");
    fprintf(gsfile, "quit\n");

    status = pclose(gsfile);

    if (filetype == 1)
	unlink(tmpfile);
    /* error return from ghostscript, look in error file */
    if (status != 0 || (pixfile = fopen(pixnam, "rb")) == NULL) {
	FILE       *errfile = fopen(errnam, "r");

	file_msg("Could not parse %s file with ghostscript: %s",
		 pdf_flag ? "PDF" : "EPS", file);
	if (errfile) {
	    file_msg("ERROR from ghostscript:");
	    while (fgets(buf, 300, errfile) != NULL) {
		buf[strlen(buf) - 1] = '\0';	/* strip newlines */
		file_msg("%s", buf);
	    }
	    fclose(errfile);
	    unlink(errnam);
	}
	unlink(pixnam);
	return False;
    }
    pic->pic_cache->bit_size.x = wid;
    pic->pic_cache->bit_size.y = ht;
    if (tool_cells <= 2 || appres.monochrome) {
	pic->pic_cache->numcols = 0;
	nbitmap = (pic->pic_cache->bit_size.x + 7) / 8 * pic->pic_cache->bit_size.y;
	pic->pic_cache->bitmap = (unsigned char *) malloc(nbitmap);
	if (pic->pic_cache->bitmap == NULL) {
	    file_msg("Could not allocate %d bytes of memory for %s bitmap\n",
		     pdf_flag ? "PDF" : "EPS", nbitmap);
	    return False;
	}
	fgets(buf, 300, pixfile);
	/* skip any comments */
	/* the last line read is the image size */
	do
	    fgets(buf, 300, pixfile);
	while (buf[0] == '#');
	if (fread(pic->pic_cache->bitmap, nbitmap, 1, pixfile) != 1) {
	    file_msg("Error reading output (%s problems?): %s",
		     pdf_flag ? "PDF" : "EPS", pixnam);
	    file_msg("Look in %s for errors", errnam);
	    fclose(pixfile);
	    unlink(pixnam);
	    pic->pic_cache->bitmap = NULL;
	    return False;
	}
    } else {
	FILE       *pcxfile;
	int         filtyp;

	/* now read the pcx file just produced by gs */
	/* don't need bitmap - _read_pcx() will allocate a new one */
	/* save picture width/height because read_pcx will overwrite it */
	wid = pic->pic_cache->size_x;
	ht = pic->pic_cache->size_y;
	pcxfile = open_picfile(pixnam, &filtyp, PIPEOK, tmpfile);
	status = _read_pcx(pcxfile, pic);
	/* restore width/height */
	pic->pic_cache->size_x = wid;
	pic->pic_cache->size_y = ht;
	if (status != 1) {
	    file_msg("Error reading output from ghostscript (%s problems?): %s",
		     pdf_flag ? "PDF" : "EPS", pixnam);
	    file_msg("Look in %s for errors", errnam);
	    unlink(pixnam);
	    if (pic->pic_cache->bitmap)
		free((char *) pic->pic_cache->bitmap);
	    pic->pic_cache->bitmap = NULL;
	    return False;
	}
    }
    fclose(pixfile);
    unlink(pixnam);
    unlink(errnam);
    return True;		/* Success */
}

#endif /* GSBIT */
