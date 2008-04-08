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
#include "f_picobj.h"
#include "w_msgpanel.h"

#include "f_readpcx.h"

#define BUFLEN 1024

/* return codes:  PicSuccess (1) : success
		  FileInvalid (-2) : invalid file
*/



int
read_ppm(FILE *file, int filetype, F_pic *pic)
{
	char	 buf[BUFLEN],pcxname[PATH_MAX];
	FILE	*giftopcx;
	int	 stat, size;

	/* make name for temp output file */
	sprintf(pcxname, "%s/%s%06d.pix", TMPDIR, "xfig-pcx", getpid());
	/* make command to convert gif to pcx into temp file */
	sprintf(buf, "ppmtopcx > %s 2> /dev/null", pcxname);
	if ((giftopcx = popen(buf,"w" )) == 0) {
	    file_msg("Cannot open pipe to ppmtopcx\n");
	    close_picfile(file,filetype);
	    return FileInvalid;
	}
	while ((size=fread(buf, 1, BUFLEN, file)) != 0) {
	    fwrite(buf, size, 1, giftopcx);
	}
	/* close pipe */
	pclose(giftopcx);
	if ((giftopcx = fopen(pcxname, "rb")) == NULL) {
	    file_msg("Can't open temp output file\n");
	    close_picfile(file,filetype);
	    return FileInvalid;
	}
	/* now call read_pcx to read the pcx file */
	stat = read_pcx(giftopcx, filetype, pic);
	pic->pic_cache->subtype = T_PIC_PPM;
	/* remove temp file */
	unlink(pcxname);
	close_picfile(file,filetype);
	return stat;
}
