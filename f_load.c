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
#include "figx.h"
#include "resources.h"
#include "mode.h"
#include "object.h"
#include "f_read.h"
#include "f_util.h"
#include "u_create.h"
#include "u_undo.h"
#include "w_cmdpanel.h"
#include "w_export.h"
#include "w_file.h"
#include "w_indpanel.h"
#include "w_layers.h"
#include "w_msgpanel.h"
#include "w_rulers.h"
#include "w_print.h"
#include "w_util.h"
#include "w_setup.h"

#include "e_compound.h"
#include "u_bound.h"
#include "u_draw.h"
#include "u_list.h"
#include "u_redraw.h"
#include "w_cursor.h"
#include "w_grid.h"

/* LOCAL declarations */

void	read_fail_message(char *file, int err);

/* load Fig file.

   Call with:
	file		= name of Fig file
	xoff, yoff	= offset from 0 (Fig units)
*/


void update_settings (fig_settings *settings);
void update_recent_list (char *file);

int
load_file(char *file, int xoff, int yoff)
{
    int		    s;
    F_compound	    c;
    fig_settings    settings;

    put_msg("Loading file %s...",file);
    c.parent = NULL;
    c.GABPtr = NULL;
    c.arcs = NULL;
    c.compounds = NULL;
    c.ellipses = NULL;
    c.lines = NULL;
    c.splines = NULL;
    c.texts = NULL;
    c.comments = NULL;
    c.next = NULL;
    set_temp_cursor(wait_cursor);

    /* initialize the active_layers array */
    reset_layers();
    /* and depth counters */
    reset_depths();
    /* object counters for depths */
    clearallcounts();

    s = read_figc(file, &c, DONT_MERGE, REMAP_IMAGES, xoff, yoff, &settings);
    defer_update_layers = 1;	/* so update_layers() won't update for each object */
    add_compound_depth(&c);	/* count objects at each depth */
    defer_update_layers = 0;
    update_layers();

    if (s == 0) {		/* Successful read */
	clean_up();
	(void) strcpy(save_filename, cur_filename);
	update_cur_filename(file);
	/* in case the user is inside any compounds */
	close_all_compounds();
	saved_objects = objects;
	objects = c;

	/* update the settings in appres.xxx from the settings struct returned from read_fig */
	update_settings(&settings);

	/* and draw the figure on the canvas*/
	redisplay_canvas();

	put_msg("Current figure \"%s\" (%d objects)", file, num_object);
	set_action(F_LOAD);
	reset_cursor();
	/* reset modified flag in case any change in orientation set it */
	reset_modifiedflag();
	/* update the recent list */
	update_recent_list(file);
	return 0;
    } else if (s == ENOENT || s == EMPTY_FILE) {
	char fname[PATH_MAX];

	clean_up();
	saved_objects = objects;
	objects = c;
	redisplay_canvas();
	put_msg("Current figure \"%s\" (new file)", file);
	(void) strcpy(save_filename, cur_filename);
	/* update current file name with null */
	update_cur_filename(file);
	/* update title bar with "(new file)" */
	strcpy(fname,file);
	strcat(fname," (new file)");
	update_wm_title(fname);
	set_action(F_LOAD);
	reset_cursor();
	reset_modifiedflag();
	return 0;
    }
    read_fail_message(file, s);
    reset_modifiedflag();
    reset_cursor();
    return 1;
}

void update_settings(fig_settings *settings)
{
	DeclareArgs(4);
	char    buf[30];

	/* set landscape flag oppositely and change_orient() will toggle it */
	if (settings->landscape != (int) appres.landscape) {
	    /* change_orient toggles */
	    appres.landscape = ! ((Boolean) settings->landscape);
	    /* now change the orientation of the canvas */
	    change_orient();
	    /* and flush to let things settle out */
	    app_flush();
	}
	/* set the printer and export justification labels */
	appres.flushleft = settings->flushleft;
	FirstArg(XtNlabel, just_items[settings->flushleft]);
	if (export_popup)
	    SetValues(export_just_panel);
	if (print_popup)
	    SetValues(print_just_panel);

	/* set the rulers/grid accordingly */
	if (settings->units != (int) appres.INCHES || cur_gridunit != settings->grid_unit) {
	    /* reset the units string for the length messages */
	    strcpy(cur_fig_units, settings->units ? "in" : "cm");
	    /* and grid unit (1/16th, 1/10th, etc) */
	    cur_gridunit = settings->grid_unit;

	    appres.INCHES = (Boolean) settings->units;
	    /* PIC_FACTOR is the number of Fig units per printer points (1/72 inch) */
	    /* it changes with Metric and Imperial */
	    PIC_FACTOR  = (appres.INCHES ? (float)PIX_PER_INCH : 2.54*PIX_PER_CM)/72.0;
	    /* make sure we aren't previewing a figure in the file panel */
	    if (canvas_win == main_canvas) {
		reset_rulers();
		init_grid();
		setup_grid();
		/* change the label in the units widget */
		FirstArg(XtNlabel, appres.INCHES ? "in" : "cm");
		SetValues(unitbox_sw);
	    }
	}

	/* set the unit indicator in the upper-right corner */
	set_unit_indicator(False);

	/* paper size */
	if ((appres.papersize = settings->papersize) < 0) {
	    file_msg("Illegal paper size in file, using default");
	    appres.papersize = (appres.INCHES? PAPER_LETTER: PAPER_A4);
	}
	/* and the print and export paper size menus */
	FirstArg(XtNlabel, paper_sizes[appres.papersize].fname);
	if (export_popup)
	    SetValues(export_papersize_panel);
	if (print_popup)
	    SetValues(print_papersize_panel);

	appres.magnification = settings->magnification;
	/* set the magnification in the export and print panels */
	sprintf(buf,"%.2f",appres.magnification);
	FirstArg(XtNstring, buf);
	if (export_popup)
	    SetValues(export_mag_text);
	if (print_popup) {
	    SetValues(print_mag_text);
	    print_update_figure_size();
	}

	/* multi-page setting */
	appres.multiple = settings->multiple;
	FirstArg(XtNlabel, multiple_pages[appres.multiple]);
	if (export_popup)
	    SetValues(export_multiple_panel);
	if (print_popup)
	    SetValues(print_multiple_panel);

	/* GIF transparent color */
	appres.transparent = settings->transparent;
	/* make colorname from number */
	set_color_name(appres.transparent, buf);
	FirstArg(XtNlabel, buf);
	if (export_popup)
	    SetValues(export_transp_panel);
}

void merge_file(char *file, int xoff, int yoff)
{
    F_compound	   *c, *c2;
    int		    s;
    fig_settings    settings;

    if ((c = create_compound()) == NULL)
	return;
    c->arcs = NULL;
    c->compounds = NULL;
    c->ellipses = NULL;
    c->lines = NULL;
    c->splines = NULL;
    c->texts = NULL;
    c->comments = NULL;
    c->next = NULL;

    set_temp_cursor(wait_cursor);

    /* clear picture object read flag */
    pic_obj_read = False;

    /* read merged file into compound */
    s = read_figc(file, c, MERGE, DONT_REMAP_IMAGES, xoff, yoff, &settings);

    if (s == 0) {			/* Successful read */
	/* only if there are objects other than user colors */
	if (c) {
	    /* if there are no objects other than one compound, don't encapsulate
		   it in another compound */
	    if (c->arcs == 0 && c->ellipses == 0 && c->lines == 0 && 
		c->splines == 0 && c->texts == 0 &&
		(c->compounds != 0 && c->compounds->next == 0)) {
		    /* save ptr to embedded compound */
		    c2 = c->compounds;
		    /* free the toplevel */
		    free((char *) c);
	            c = c2;
	    }
	    compound_bound(c, &c->nwcorner.x, &c->nwcorner.y,
		   &c->secorner.x, &c->secorner.y);
	    clean_up();
	    /* add the depths of the objects besides adding the objects to the main list */
	    list_add_compound(&objects.compounds, c);
	    set_latestcompound(c);
	}
	/* must remap all EPS/GIF/XPMs now if any new pic objects were read */
	if (pic_obj_read)
	    remap_imagecolors();
	redraw_images(&objects);
	if (c)
	    redisplay_zoomed_region(c->nwcorner.x, c->nwcorner.y, 
				c->secorner.x, c->secorner.y);
	put_msg("%d object(s) read from \"%s\"", num_object, file);
	set_action_object(F_ADD, O_COMPOUND);
	reset_cursor();
	set_modifiedflag();
    }
    read_fail_message(file, s);
    reset_cursor();
}

/* update the recent list */

void update_recent_list(char *file)
{
    int    i;
    char   *name, path[PATH_MAX], num[3];

    /* prepend path if not already in name */
    if (file[0] != '/') {
	/* prepend path if not already there */
	get_directory(path);
	strcat(path, "/");
	strcat(path, file);
	file = path;
    }
    /* if this name is already in the list, delete it and move list up */
    for (i=0; i<num_recent_files; i++) {
	if (strcmp(file,&recent_files[i].name[2])==0)	 /* point past number in menu entry */
	    break;
    }
    /* already there? */
    if (i < num_recent_files) {
	/* yes, move others up */
	for ( ; i < num_recent_files-1; i++)
	    recent_files[i] = recent_files[i+1];
	num_recent_files--;
    }

    /* first, push older entries down one slot */
    for (i=num_recent_files; i>0; i--) {
	if (i >= max_recent_files) {
	    /* pushing one off the end, free it's name */
	    free(recent_files[i-1].name);
	    continue;
	}
	/* shift down */
	recent_files[i] = recent_files[i-1];
	/* renumber entry */
	sprintf(num,"%1d",i+1);
	strncpy(recent_files[i].name,num,1);
    }

    /* put new entry in first slot */
    /* prepend with file number (1) */
    name = new_string(strlen(file)+4);
    sprintf(name,"1 %s",file);
    recent_files[0].name = name;
    if (num_recent_files < max_recent_files)
	num_recent_files++;

    /* now recreate the File menu with the new filenames */
    rebuild_file_menu(None);
    /* now update the user's .xfigrc file with the file list */
    update_recent_files();
}

void
read_fail_message(char *file, int err)
{
    if (err == 0)		/* Successful read */
	return;
#ifdef ENAMETOOLONG
    else if (err == ENAMETOOLONG)
	file_msg("File name \"%s\" is too long", file);
#endif /* ENAMETOOLONG */
    else if (err == ENOENT)
	file_msg("File \"%s\" does not exist.", file);
    else if (err == ENOTDIR)
	file_msg("A name in the path \"%s\" is not a directory.", file);
    else if (err == EACCES)
	file_msg("Read access to file \"%s\" is blocked.", file);
    else if (err == EISDIR)
	file_msg("File \"%s\" is a directory.", file);
    else if (err == NO_VERSION)
	file_msg("File \"%s\" has no version number in header.", file);
    else if (err == EMPTY_FILE)
	file_msg("File \"%s\" is empty.", file);
    else if (err == BAD_FORMAT)
	/* Format error; relevant error message is already delivered */
	;
    else
	file_msg("File \"%s\" is not accessable; %s.", file, strerror(err));
}
