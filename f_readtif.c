/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1999-2002 by Brian V. Smith
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
#include "w_msgpanel.h"

#include "f_readpcx.h"

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/

/* for some reason, tifftopnm requires a file and can't work in a pipe */



int
read_tif(char *filename, int filetype, F_pic *pic)
{
	char	 buf[2*PATH_MAX+40],pcxname[PATH_MAX];
	FILE	*tiftopcx;
	int	 stat;

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());

	/* make command to convert tif to pnm then to pcx into temp file */
	/* for some reason, tifftopnm requires a file and can't work in a pipe */
	sprintf(buf, "tifftopnm %s 2> /dev/null | ppmtopcx > %s 2> /dev/null",
		filename, pcxname);
	if ((tiftopcx = popen(buf,"w" )) == 0) {
	    file_msg("Cannot open pipe to tifftopnm or ppmtopcx\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return FileInvalid;
	}
	/* close pipe */
	pclose(tiftopcx);
	if ((tiftopcx = fopen(pcxname, "rb")) == NULL) {
	    file_msg("Can't open temp output file\n");
	    /* remove temp file */
	    unlink(pcxname);
	    return FileInvalid;
	}
	/* now call read_pcx to read the pcx file */
	stat = read_pcx(tiftopcx, filetype, pic);
	pic->pic_cache->subtype = T_PIC_TIF;
	/* remove temp file */
	unlink(pcxname);
	return stat;
}
