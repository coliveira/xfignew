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
#include "main.h"
#include "mode.h"
#include "patchlevel.h"
#include "version.h"

#include "object.h"
#include "f_save.h"
#include "f_util.h"
#include "w_cmdpanel.h"

#define MAXERRORS 6
#define MAXERRMSGLEN 512

static int	error_cnt = 0;

void emergency_quit (Boolean abortflag);

void error_handler(int err_sig)
{
    fprintf(stderr,"\nxfig%s.%s: ",FIG_VERSION,PATCHLEVEL);
    switch (err_sig) {
    case SIGHUP:
	fprintf(stderr, "SIGHUP signal trapped\n");
	break;
    case SIGFPE:
	fprintf(stderr, "SIGFPE signal trapped\n");
	break;
#ifdef SIGBUS
    case SIGBUS:
	fprintf(stderr, "SIGBUS signal trapped\n");
	break;
#endif /* SIGBUS */
    case SIGSEGV:
	fprintf(stderr, "SIGSEGV signal trapped\n");
	break;
    default:
	fprintf(stderr, "Unknown signal (%d)\n", err_sig);
	break;
    }
    emergency_quit(True);
}

void X_error_handler(Display *d, XErrorEvent *err_ev)
{
    char	    err_msg[MAXERRMSGLEN];
    char	    ernum[10];

    /* uninstall error handlers so we don't recurse if another error happens! */
    XSetErrorHandler(NULL);
    XSetIOErrorHandler((XIOErrorHandler) NULL);
    XGetErrorText(tool_d, (int) (err_ev->error_code), err_msg, MAXERRMSGLEN - 1);
    (void) fprintf(stderr,
	   "xfig%s.%s: X error trapped - error message follows:\n%s\n", 
		FIG_VERSION,PATCHLEVEL,err_msg);
    (void) sprintf(ernum, "%d", (int)err_ev->request_code);
    XGetErrorDatabaseText(tool_d, "XRequest", ernum, "<Unknown>", err_msg, MAXERRMSGLEN);
    (void) fprintf(stderr, "Request code: %s\n",err_msg);
    emergency_quit(True);
}

void emergency_quit(Boolean abortflag)
{
    if (++error_cnt > MAXERRORS) {
	fprintf(stderr, "xfig: too many errors - giving up.\n");
	exit(-1);
    }
    signal(SIGHUP, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif /* SIGBUS */
    signal(SIGSEGV, SIG_DFL);

    aborting = abortflag;
    if (figure_modified && !emptyfigure()) {
	fprintf(stderr, "xfig: attempting to save figure\n");
	if (emergency_save("SAVE.fig") == -1)
	    if (emergency_save(strcat(TMPDIR,"/SAVE.fig")) == -1)
		fprintf(stderr, "xfig: unable to save figure\n");
    } else
	fprintf(stderr, "xfig: figure empty or not modified - exiting\n");

    goodbye(abortflag);	/* finish up and exit */
}

/* ARGSUSED */
void
my_quit(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    if (event && event->type == ClientMessage &&
#ifdef WHEN_SAVE_YOURSELF_IS_FIXED
	((event->xclient.data.l[0] != wm_protocols[0]) &&
	 (event->xclient.data.l[0] != wm_protocols[1])))
#else
	(event->xclient.data.l[0] != wm_protocols[0]))
#endif /* WHEN_SAVE_YOURSELF_IS_FIXED */
    {
	return;
    }
    /* quit after asking whether user wants to save figure */
    quit(w, 0, 0);
}
