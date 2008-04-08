/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
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
#include "mode.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_print.h"
#include "w_setup.h"

#include "f_save.h"
#include "f_util.h"
#include "w_cursor.h"
#include "w_drawprim.h"
#include "w_util.h"
#include "u_print.h"


static int	exec_prcmd(char *command, char *msg);
static char	layers[PATH_MAX];
static char	prcmd[2*PATH_MAX+200], tmpcmd[255];

Boolean	print_hpgl_pcl_switch;
Boolean	hpgl_specified_font;

/*
 * Protect a string by enclosing it in apostrophes. Escape any apostrophes
 * in the string with a backslash ('\').
 * Beware!  The string returned by this function is static and is
 * reused the next time the function is called!
 */


void build_layer_list (char *layers);
void append_group (char *list, char *num, int first, int last);

char *shell_protect_string(char *string)
{
    static char *buf = 0;
    static int buflen = 0;
    int len = 2 * strlen(string) + 1;
    char *cp, *cp2;

    if (strlen(string) == 0)
	return string;

    if (! buf) {
	buf = XtMalloc(len);
	buflen = len;
    }
    else if (buflen < len) {
	buf = XtRealloc(buf, len);
	buflen = len;
    }

    cp2 = buf;
    *cp2++ = '\'';
    for (cp = string; *cp; cp++) {
	if (*cp == '\'') {
	    /* an apostrophe in the string, close quotes, add \' and open again */
	    *cp2++ = '\'';
	    *cp2++ = '\\';
	    *cp2++ = '\'';
	}
	*cp2++ = *cp;
    }
    *cp2++ = '\'';

    *cp2 = '\0';

    return(buf);
}

void print_to_printer(char *printer, char *backgrnd, float mag, Boolean print_all_layers, char *grid, char *params)
{
    char	    syspr[2*PATH_MAX+200];
    char	    tmpfile[PATH_MAX];
    char	   *name;

    sprintf(tmpfile, "%s/%s%06d", TMPDIR, "xfig-print", getpid());
    warnexist = False;
    init_write_tmpfile();
    if (write_file(tmpfile, False)) {
      end_write_tmpfile();
      return;
    }
    end_write_tmpfile();

    /* if the user only wants the active layers, build that list */
    build_layer_list(layers);

    if (strlen(cur_filename) == 0)
	name = tmpfile;
    else
	name = shell_protect_string(cur_filename);

#ifdef I18N
    /* set the numeric locale to C so we get decimal points for numbers */
    setlocale(LC_NUMERIC, "C");
    sprintf(tmpcmd, "%s %s -L ps -z %s -m %f %s -n %s",
	    fig2dev_cmd, appres.international ? appres.fig2dev_localize_option : "",
    /* reset to original locale */
#else
    sprintf(tmpcmd, "%s -L ps -z %s -m %f %s -n %s",
	    fig2dev_cmd,
#endif /* I18N */
	    paper_sizes[appres.papersize].sname, mag/100.0,
	    appres.landscape ? "-l xxx" : "-p xxx", name);
#ifdef I18N
    setlocale(LC_NUMERIC, "");
#endif /* I18N */

    if (appres.correct_font_size)
	strcat(tmpcmd," -F ");
    if (!appres.multiple && !appres.flushleft)
	strcat(tmpcmd," -c ");
    if (appres.multiple)
	strcat(tmpcmd," -M ");
    if (strlen(grid) && strcasecmp(grid,"none") !=0 ) {
	strcat(tmpcmd," -G ");
	strcat(tmpcmd,grid);
    }
    if (backgrnd[0]) {
	strcat(tmpcmd," -g \\");	/* must escape the #rrggbb color spec */
	strcat(tmpcmd,backgrnd);
    }
    /* add the -D +list if user doesn't want all layers printed */
    if (!print_all_layers)
	strcat(tmpcmd, layers);

    /* make the print command with no filename (it will be in stdin) */
    gen_print_cmd(syspr, "", printer, params);

    /* make up the whole translate/print command */
    sprintf(prcmd, "%s %s | %s", tmpcmd, tmpfile, syspr);
    if (exec_prcmd(prcmd, "PRINT") == 0) {
	if (emptyname(printer))
	    put_msg("Printing on default printer with %s paper size in %s mode ... done",
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
	else
	    put_msg("Printing on \"%s\" with %s paper size in %s mode ... done",
		printer, paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    unlink(tmpfile);
}

void strsub(prcmd,find,repl,result, global)
	char *prcmd,*find, *repl,*result;
    int global;
{
	char *loc;

	do {
		loc = strstr(prcmd,find);
		if(loc == NULL)
			break;

		while((prcmd != loc) && *prcmd) /* copy prcmd into result up to loc */
			*result++ = *prcmd++;
		strcpy(result,repl);
		result += strlen(repl);
		prcmd += strlen(find);
	} while(global);

	strcpy(result,prcmd);
}
/* xoff, yoff, and border are in fig2dev print units (1/72 inch) */

int print_to_file(char *file, char *lang, float mag, int xoff, int yoff, char *backgrnd, char *transparent, Boolean use_transp_backg, Boolean print_all_layers, int border, Boolean smooth, char *grid, Boolean overlap)
{
    char	    tmp_name[PATH_MAX];
    char	    tmp_fig_file[PATH_MAX];
    char	   *outfile, *name, *real_lang;
    char	   *suf;

    /* if file exists, ask if ok */
    if (!ok_to_write(file, "EXPORT"))
	return (1);

    sprintf(tmp_fig_file, "%s/%s%06d", TMPDIR, "xfig-fig", getpid());
    /* write the fig objects to a temporary file */
    warnexist = False;
    init_write_tmpfile();
    if (write_file(tmp_fig_file, False)) {
      end_write_tmpfile();
      return (1);
    }
    end_write_tmpfile();

    /* if the user only wants the active layers, build that list */
    build_layer_list(layers);

    outfile = my_strdup(shell_protect_string(file));
    if (strlen(cur_filename) == 0)
	name = my_strdup(file);
    else
	name = my_strdup(shell_protect_string(cur_filename));

    put_msg("Exporting to file \"%s\" in %s mode ...     ",
	    file, appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    app_flush();		/* make sure message gets displayed */
   
    /* change "hpl" to "ibmgl" */
    if (!strcmp(lang, "hpl"))
	lang = "ibmgl";

    real_lang = lang;
    /* if lang is eps_ascii, eps_mono_tiff or eps_color_tiff, call fig2dev with just "eps" */
    if (!strncmp(lang, "eps", 3))
	real_lang = "eps";

    /* if lang is pspdf, call first with just "eps" */
    if (!strncmp(lang, "pspdf", 5))
	real_lang = "eps";

    /* if binary CGM, real language is cgm */
    if (!strcmp(lang, "bcgm"))
	real_lang = "cgm";

    /* start with the command, language and internationalization, if applicable */
#ifdef I18N
    /* set the numeric locale to C so we get decimal points for numbers */
    setlocale(LC_NUMERIC, "C");
    sprintf(prcmd, "%s %s -L %s ", 
		fig2dev_cmd, appres.international ? appres.fig2dev_localize_option : "",
		real_lang, mag/100.0);
    /* reset to original locale */
    setlocale(LC_NUMERIC, "");
#else
    sprintf(prcmd, "%s -L %s ", fig2dev_cmd, real_lang);
#endif  /* I18N */

    /* Add in magnification */
    sprintf(&prcmd[strlen(prcmd)], "-m %f", mag/100.);

    /* fig2dev for -L ibmgl also take option -m "mag,x0,y0", where the
       offset is in inches.
    */
    if (!strcmp(lang, "ibmgl") && (xoff || yoff)) {
	sprintf(&prcmd[strlen(prcmd)], ",%.4f,%.4f ", xoff/72., yoff/72.);
    } else
	strcat(prcmd, " ");

    /* add the -D +list if user doesn't want all layers printed */
    if (!print_all_layers)
	strcat(prcmd, layers);

    /* any grid spec */
    if (strlen(grid) && strcasecmp(grid,"none") !=0 ) {
	strcat(prcmd," -G ");
	strcat(prcmd,grid);
	strcat(prcmd," ");
    }

    /* PostScript or PDF output */
    if (!strcmp(lang, "ps") || !strcmp(lang, "pdf")) {
	/* add -O if user wants pages overlapped (multiple page mode) */
	if (overlap)
	    strcat(prcmd, " -O ");

	sprintf(tmpcmd, "-z %s %s -n %s -x %d -y %d -b %d", 
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "-l xxx" : "-p xxx", 
		name, xoff, yoff, border);
	strcat(prcmd, tmpcmd);

	if (appres.correct_font_size)
	    strcat(prcmd," -F ");
	if (!appres.multiple && !appres.flushleft)
	    strcat(prcmd," -c ");
	if (appres.multiple)
	    strcat(prcmd," -M ");
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* EPS (Encapsulated PostScript) output */
    } else if (!strncmp(lang, "eps", 3)) {	
	/* matches all "eps", "eps_ascii", "eps_mono_tiff" and "eps_color_tiff" */
	sprintf(tmpcmd, "-b %d -n %s", border, name);
	strcat(prcmd, tmpcmd);
	if (appres.correct_font_size)
	    strcat(prcmd," -F ");
	/* add -A, -T or -C for ASCII or mono or color TIFF preview respectively */
	if (!strcmp(lang, "eps_ascii"))
	    strcat(prcmd," -A");
	if (!strcmp(lang, "eps_mono_tiff"))
	    strcat(prcmd," -T");
	else if (!strcmp(lang, "eps_color_tiff"))
	    strcat(prcmd," -C dummy");

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* IBMGL (we know it as "hpl" but it was changed above) */
    } else if (!strcmp(lang, "ibmgl")) {
	if (hpgl_specified_font)
	    strcat(prcmd," -F");
	if (print_hpgl_pcl_switch)
	    strcat(prcmd, " -k");
	sprintf(tmpcmd, " -z %s %s %s %s",
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "" : "-P",
		tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);
    } else if (!strcmp(lang,"pspdftex")) {
	/* first generate postscript then PDF.  */
	sprintf(tmpcmd, "-n %s", outfile);
	strcat(prcmd, tmpcmd);

	if (backgrnd[0]) {
		strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
		strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");

	strsub(outfile,".","_",tmp_name,1);
	strcat(prcmd,tmp_name);

	/* make it suitable for pstex. */
	strsub(prcmd,"pspdftex","pstex",tmpcmd,0);
	strcat(tmpcmd,".eps");
	(void) exec_prcmd(tmpcmd, "EXPORT of PostScript part");

	/* make it suitable for pdftex. */
	strsub(prcmd,"eps","pdf",tmpcmd,0);
	strsub(tmpcmd,"pspdftex","pdftex",prcmd,0);
	strcat(prcmd,".pdf");
	(void) exec_prcmd(prcmd, "EXPORT of PDF part");

	/* and then the tex code. */
#ifdef I18N
	/* set the numeric locale to C so we get decimal points for numbers */
	setlocale(LC_NUMERIC, "C");
	sprintf(prcmd, "fig2dev %s -L %s -p %s -m %f %s %s",
		appres.international ?  appres.fig2dev_localize_option : "",
#else
		sprintf(prcmd, "fig2dev -L %s -p %s -m %f %s %s",
#endif  /* I18N */
				"pstex_t", tmp_name, mag/100.0, tmp_fig_file, outfile);
#ifdef I18N
	/* reset to original locale */
	setlocale(LC_NUMERIC, "");
#endif  /* I18N */

    /* PSTEX and PDFTEX */
    } else if (!strcmp(lang, "pstex") || !strcmp(lang, "pdftex")) {
	/* do both EPS (or PDF) part and text part */
	/* first the EPS/PDF part */
	sprintf(tmpcmd, "-b %d -n %s", border, name);
	strcat(prcmd, tmpcmd);
	if (appres.correct_font_size)
	    strcat(prcmd," -F ");

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

	if (!strcmp(lang, "pstex"))
	    (void) exec_prcmd(prcmd, "EXPORT of EPS part");
	else
	    (void) exec_prcmd(prcmd, "EXPORT of PDF part");

	/* now the text part */
	/* add "_t" to the output filename and put in tmp_name */
	strcpy(tmp_name,outfile);
	strcat(tmp_name,"_t");
	/* make it automatically input the postscript/pdf part (-p option) */
#ifdef I18N
	/* set the numeric locale to C so we get decimal points for numbers */
	setlocale(LC_NUMERIC, "C");
	sprintf(prcmd, "%s %s -L %s -E %d -p %s -m %f -b %d ",
		fig2dev_cmd, appres.international ? appres.fig2dev_localize_option : "",
#else
	sprintf(prcmd, "%s -L %s -E %d -p %s -m %f -b %d ",
		fig2dev_cmd,
#endif  /* I18N */
		!strcmp(lang,"pstex")? "pstex_t": "pdftex_t",
		appres.encoding, outfile, mag/100.0, border);
#ifdef I18N
	/* reset to original locale */
	setlocale(LC_NUMERIC, "");
#endif  /* I18N */
	/* add the -D +list if user doesn't want all layers printed */
	if (!print_all_layers)
	    strcat(prcmd, layers);
	/* finally, append the filenames */
	strcat(prcmd, tmp_fig_file);
	strcat(prcmd, " ");
	strcat(prcmd, tmp_name);

    /* PSPDF */
    } else if (!strcmp(lang, "pspdf")) {
	sprintf(tmpcmd, "-b %d -n %s", border, name);
	strcat(prcmd, tmpcmd);
	if (appres.correct_font_size)
	    strcat(prcmd," -F ");

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	/* add output file name */
	strcat(prcmd,outfile);

	(void) exec_prcmd(prcmd, "EXPORT of EPS part");
	/* start over with the command, language and internationalization, if applicable */
#ifdef I18N
	/* set the numeric locale to C so we get decimal points for numbers */
	setlocale(LC_NUMERIC, "C");
	sprintf(prcmd, "%s %s -L pdf -m %f ", 
		fig2dev_cmd, appres.international ? appres.fig2dev_localize_option : "",
		mag/100.0);
	/* reset to original locale */
	setlocale(LC_NUMERIC, "");
#else
	sprintf(prcmd, "%s -L pdf -m %f ", fig2dev_cmd, mag/100.0);
#endif  /* I18N */

	/* add the -D +list if user doesn't want all layers printed */
	if (!print_all_layers)
	    strcat(prcmd, layers);

	/* any grid spec */
	if (strlen(grid) && strcasecmp(grid,"none") !=0 ) {
	    strcat(prcmd," -G ");
	    strcat(prcmd,grid);
	    strcat(prcmd," ");
	}
	sprintf(tmpcmd, "-b %d -n %s", border, name);
	strcat(prcmd, tmpcmd);
	if (appres.correct_font_size)
	    strcat(prcmd," -F ");

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");

	/* now change the output file name to xxx.pdf */
	/* strip off current suffix, if any */
	if ((suf=strrchr(outfile, '.')))
	    *suf = '\0';
	/* and make .pdf */
	strcat(outfile,".pdf'");
	strcat(prcmd, outfile);
	/* and fall through to export */
    	
    /* JPEG */
    } else if (!strcmp(lang, "jpeg")) {
	/* set the image quality for JPEG export */
	sprintf(tmpcmd, "-b %d -q %d -S %d", 
		border, appres.jpeg_quality, smooth);
	strcat(prcmd, tmpcmd);

	if (appres.correct_font_size)
	    strcat(prcmd," -F ");
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}

    /* GIF */
    } else if (!strcmp(lang, "gif")) {
	sprintf(tmpcmd, "-b %d -S %d",
		border, smooth);
	strcat(prcmd, tmpcmd);

	/* select the transparent color, if any */
	if (transparent) {
	    /* if user wants background transparent, set the background
	        to the transparrent color */
	    if (use_transp_backg)
		backgrnd = transparent;
	    strcat(prcmd," -t \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,transparent);
	}
	if (appres.correct_font_size)
	    strcat(prcmd," -F ");
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* MAP */
    } else if (!strcmp(lang, "map")) {
        /* HTML map needs border option */
	sprintf(tmpcmd, "-b %d %s %s",
		border, tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);

    /* PCX, PNG, TIFF, XBM, XPM, and PPM */
    } else if (!strcmp(lang, "pcx") || !strcmp(lang, "png") || !strcmp(lang, "tiff") ||
		!strcmp(lang, "xbm") || !strcmp(lang, "xpm") || !strcmp(lang, "ppm")) {
        /* bitmap formats need border option */
	sprintf(tmpcmd, "-b %d -S %d",
		border, smooth);
	strcat(prcmd, tmpcmd);
	if (appres.correct_font_size)
	    strcat(prcmd," -F ");

	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* binary CGM? */
    } else if (!strcmp(lang, "bcgm")) {
	/* yes, append -b option */
	strcat(prcmd," -b dum ");
	strcat(prcmd,tmp_fig_file);
	strcat(prcmd," ");
	strcat(prcmd,outfile);

    /* epic, eepic, eepicemu, latex, pictex */
    } else if (!strcmp(lang, "epic") || !strcmp(lang, "eepic") || !strcmp(lang, "eepicemu") ||
		!strcmp(lang, "latex") || !strcmp(lang, "pictex")) {
	sprintf(tmpcmd, "-E %d %s %s",
		appres.encoding, tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);
    /* tk? */
    } else if (!strcmp(lang, "tk")) {
	/* yes, append background option */
	if (backgrnd[0]) {
	    strcat(prcmd," -g \\");	/* must escape the #rrggbb color spec */
	    strcat(prcmd,backgrnd);
	}
	strcat(prcmd," ");
	sprintf(tmpcmd, "%s %s",
		tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);
    /* Everything else */
    } else {
	sprintf(tmpcmd, "%s %s",
		tmp_fig_file, outfile);
	strcat(prcmd, tmpcmd);
    }

    /* make a busy cursor */
    set_temp_cursor(wait_cursor);

    /* now execute fig2dev */
    if (exec_prcmd(prcmd, "EXPORT") == 0)
	put_msg("Export to \"%s\" done", file);

    /* and reset the cursor */
    reset_cursor();

    /* free tempnames */
    free(name);
    free(outfile);

    unlink(tmp_fig_file);
    return (0);
}

void gen_print_cmd(char *cmd, char *file, char *printer, char *pr_params)
{
    if (emptyname(printer)) {	/* send to default printer */
#if (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR)
	sprintf(cmd, "lp %s %s", 
		pr_params,
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr %s %s",
		pr_params,
		shell_protect_string(file));
#endif /* (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR) */
	put_msg("Printing on default printer with %s paper size in %s mode ...     ",
		paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    } else {
#if (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR)
	sprintf(cmd, "lp %s -d%s %s",
		pr_params,
		printer, 
		shell_protect_string(file));
#else
	sprintf(cmd, "lpr %s -P%s %s", 
		pr_params,
		printer,
		shell_protect_string(file));
#endif /* (defined(SYSV) || defined(SVR4)) && !defined(BSDLPR) */
	put_msg("Printing on \"%s\" with %s paper size in %s mode ...     ",
		printer, paper_sizes[appres.papersize].sname,
		appres.landscape ? "LANDSCAPE" : "PORTRAIT");
    }
    app_flush();		/* make sure message gets displayed */
}

int
exec_prcmd(char *command, char *msg)
{
    char   errfname[PATH_MAX];
    FILE  *errfile;
    char   str[400];
    int	   status;

    /* make temp filename for any errors */
    sprintf(errfname, "%s/xfig-export%06d.err", TMPDIR, getpid());
    /* direct any output from fig2dev to this file */
    strcat(command, " 2> "); 
    strcat(command, errfname); 
    if (appres.DEBUG)
	fprintf(stderr,"Execing: %s\n",command);
    status=system(command);
    /* check if error file has anything in it */
    if ((errfile = fopen(errfname, "r")) == NULL) {
	if (status != 0)
	    file_msg("Error during %s. No messages available.",msg);
    } else {
	if (fgets(str,sizeof(str)-1,errfile) != NULL) {
	    rewind(errfile);
	    file_msg("Error during %s.  Messages:",msg);
	    while (fgets(str,sizeof(str)-1,errfile) != NULL) {
		/* remove trailing newlines */
		str[strlen(str)-1] = '\0';
		file_msg(" %s",str);
	    }
	}
	fclose(errfile);
    }
    unlink(errfname);
    return status;
}

/* 
   make an rgb string from color (e.g. #31ab12)
   if the color is < 0, make empty string
*/

void make_rgb_string(int color, char *rgb_string)
{
	XColor xcolor;
	if (color >= 0) {
	    xcolor.pixel = x_color(color);
	    XQueryColor(tool_d, tool_cm, &xcolor);
	    sprintf(rgb_string,"#%02x%02x%02x",
				xcolor.red>>8,
				xcolor.green>>8,
				xcolor.blue>>8);
	} else {
	    rgb_string[0] = '\0';	/* no background wanted by user */
	}
}

/* make up the -D option to fig2dev if user wants to print only active layers */

void build_layer_list(char *layers)
{
    char	 list[PATH_MAX], notlist[PATH_MAX], num[10];
    int		 layer, len, notlen;
    int		 firstyes, lastyes, firstno, lastno;

    layers[0] = '\0';

    if (print_all_layers)
	return;

    list[0] = notlist[0] = '\0';
    len = notlen = 0;

    /* build up two lists - layers TO print and layers to NOT print */
    /* use the smaller of the two in the final command */

    firstyes = firstno = -1;
    for (layer=min_depth; layer<=max_depth; layer++) {
	if (active_layers[layer] && object_depths[layer]) {
	    if (firstyes == -1)
		firstyes = lastyes = layer;
	    /* see if there is a contiguous set */
	    if (layer-lastyes <= 1) {
		lastyes = layer;	/* so far, yes */
		continue;
	    }
	    append_group(list, num, firstyes, lastyes);
	    firstyes = lastyes = layer;
	    if (len+strlen(list) >= PATH_MAX-5)
		continue;		/* list is too long, don't append */
	    strcat(list,num);
	    len += strlen(list)+1;
	} else if (object_depths[layer]) {
	    if (firstno == -1)
		firstno = lastno = layer;
	    /* see if there is a contiguous set */
	    if (layer-lastno <= 1) {
		lastno = layer;		/* so far, yes */
		continue;
	    }
	    if (firstno == -1)
		firstno = layer;
	    append_group(notlist, num, firstno, lastno);
	    firstno = lastno = layer;
	    if (notlen+strlen(notlist) >= PATH_MAX-5)
		continue;		/* list is too long, don't append */
	    strcat(notlist,num);
	    notlen += strlen(notlist)+1;
	}
    }
    if (firstyes != -1) {
	append_group(list, num, firstyes, lastyes);
	if (len+strlen(list) < PATH_MAX-5) {
	    strcat(list,num);
	    len += strlen(list)+1;
	}
    }
    if (firstno != -1) {
	append_group(notlist, num, firstno, lastno);
	if (notlen+strlen(notlist) < PATH_MAX-5) {
	    strcat(notlist,num);
	    notlen += strlen(notlist)+1;
	}
    }
    if (len < notlen && firstyes != -1) {
	/* use list of layers TO print */
	sprintf(layers," -D +%s ",list);
    } else if (firstno != -1){
	/* use list of layers to NOT print */
	sprintf(layers," -D -%s ",notlist);
    }
}

void append_group(char *list, char *num, int first, int last)
{
    if (list[0])
	strcat(list,",");
    if (first==last)
	sprintf(num,"%0d",first);
    else
	sprintf(num,"%0d:%d",first,last);
}

