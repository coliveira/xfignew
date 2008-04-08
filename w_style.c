/*
 * FIG : Facility for Interactive Generation of figures
 * This file Copyright (c) 2002 Stephane Mancini
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
#include "object.h"
#include "mode.h"
#include "paintop.h"
#include "d_text.h"
#include "e_edit.h"
#include "e_update.h"
#include "u_draw.h"
#include "u_fonts.h"
#include "w_drawprim.h"
#include "w_file.h"
#include "w_fontbits.h"
#include "w_indpanel.h"
#include "w_color.h"
#include "w_listwidget.h"
#include "w_mousefun.h"
#include "w_msgpanel.h"
#include "w_setup.h"
#include "w_style.h"
#include "w_util.h"
#include "w_zoom.h"

#include "w_modepanel.h"

/* EXPORTS */

void	popup_manage_style_panel(void);
void	close_style(Widget w, XButtonEvent *ev);
Boolean	style_dirty_flag = False;

/* LOCALS */

static Style_family current_family_set[MAX_STYLE_FAMILY_SET];
int     current_family = -1, current_style = -1;
Widget  style_popup, style_main_form, style_main_label;
Widget  style_family_form, style_style_form;
Widget  style_family_label, style_family_list_viewport,
	style_family_choice_label, style_family_list, style_family_name,
	style_add_family, style_delete_family;
Widget  style_style_label, style_style_list_viewport,
	style_style_choice_label, style_style_list, style_style_name,
	style_add_style, style_delete_style;
Widget  style_save_style, style_load_style, style_close_style;
char   *family_text[MAX_STYLE_FAMILY_SET + 1];
char   *style_text[MAX_STYLE_FAMILY + 1];

DeclareStaticArgs (20);

Element style_reference[] = {
    "depth", Tint, "", &cur_depth, I_DEPTH,
    "text_step", Tfloat, NULL, &cur_textstep, I_TEXTSTEP,
    "font_size", Tint, NULL, &cur_fontsize, I_FONTSIZE,
    "font", Tint, NULL, &cur_latex_font, I_FONT,
    "font_ps", Tint, NULL, &cur_ps_font, I_FONT,
    "font_latex", Tint, NULL, &cur_latex_font, I_FONT,
    "text_flags", Tint, NULL, &cur_textflags, I_FONT | I_TEXTFLAGS,
    "text_just", Tint, NULL, &cur_textjust, I_TEXTJUST,
    "line_width", Tint, NULL, &cur_linewidth, I_LINEWIDTH,
    "line_style", Tint, NULL, &cur_linestyle, I_LINESTYLE,
    "join_style", Tint, NULL, &cur_joinstyle, I_JOINSTYLE,
    "cap_style", Tint, NULL, &cur_capstyle, I_CAPSTYLE,
    "dash_length", Tfloat, NULL, &cur_dashlength, I_CAPSTYLE,
    "dot_gap", Tfloat, NULL, &cur_dotgap, I_CAPSTYLE,
    "pen_color", TColor, NULL, &cur_pencolor, I_PEN_COLOR,
    "fill_color", TColor, NULL, &cur_fillcolor, I_FILL_COLOR,
    "box_radius", Tint, NULL, &cur_boxradius, I_BOXRADIUS,
    "fill_style", Tint, NULL, &cur_fillstyle, I_FILLSTYLE,
    "arrow_mode", Tint, NULL, &cur_arrowmode, I_ARROWMODE,
    "arrow_type", Tint, NULL, &cur_arrowtype, I_ARROWTYPE,
    "arrow_width", Tfloat, NULL, &cur_arrowwidth, I_ARROWSIZE,
    "arrow_height", Tfloat, NULL, &cur_arrowheight, I_ARROWSIZE,
    "arrow_thick", Tfloat, NULL, &cur_arrowthick, I_ARROWSIZE,
    "arrow_mult_width", Tfloat, NULL, &cur_arrow_multwidth, I_ARROWSIZE,
    "arrow_mult_height", Tfloat, NULL, &cur_arrow_multheight, I_ARROWSIZE,
    "arrow_mult_thick", Tfloat, NULL, &cur_arrow_multthick, I_ARROWSIZE,
    "arc_type", Tint, NULL, &cur_arctype, I_ARCTYPE,
    "ellipse_text_angle", Tfloat, NULL, &cur_elltextangle, I_ELLTEXTANGLE,
    NULL, 0, NULL, NULL, 0
};



void
construct_style (Style * style, unsigned long flag)
{
    int     i = 0, j = 0;

    while (style_reference[i].name != NULL) {
	if (flag & style_reference[i].flag) {
	    style->element[j].name = style_reference[i].name;
	    style->element[j].type = style_reference[i].type;
	    style->element[j].toset = style_reference[i].toset;
	    style->element[j].flag = style_reference[i].flag;
	    switch (style_reference[i].type) {
		case Tint:
		    style->element[j].value = malloc (sizeof (int));
		    *((int *) style->element[j].value) =
			*((int *) style_reference[i].toset);
		    break;
		case Tfloat:
		    style->element[j].value = malloc (sizeof (float));
		    *((float *) style->element[j].value) =
			*((float *) style_reference[i].toset);
		    break;
		case TColor:
		    style->element[j].value = malloc (sizeof (int));
		    *((int *) style->element[j].value) =
			*((int *) style_reference[i].toset);
		    break;
	    }
	    j++;
	}
	i++;
    }
    if (j < MAX_STYLE_ELEMENT)
	style->element[j].name = NULL;
}

unsigned long
set_style (Style * style)
{
    int     i = 0;
    unsigned long flag = 0;

    while (style->element[i].name != NULL) {
	flag = flag | style->element[i].flag;
	switch (style->element[i].type) {
	    case Tint:
		*((int *) style->element[i].toset) = *((int *) style->element[i].value);
		break;
	    case Tfloat:
		*((float *) style->element[i].toset) =
		    *((float *) style->element[i].value);
		break;
	    case TColor:
		*((int *) style->element[i].toset) = *((int *) style->element[i].value);
		break;
	}
	i++;
    }
    return flag;
}

static int indent;

void print_indent (FILE * file)
{
    int     i;

    for (i = 0; i < indent; i++)
	fprintf (file, " ");
}

void
print_style (FILE * file, Style * style)
{
    int     i = 0;

    print_indent (file);
    fprintf (file, "%s [\n", style[i].name);
    indent += 2;
    while (style->element[i].name != NULL) {
	print_indent (file);
	fprintf (file, "%s : ", style->element[i].name);
	switch (style->element[i].type) {
	    case Tint:
		fprintf (file, "%d\n", *((int *) style->element[i].value));
		break;
	    case Tfloat:
		fprintf (file, "%f\n", *((float *) style->element[i].value));
		break;
	    case TColor:
		fprintf (file, "%d\n", *((int *) style->element[i].value));
		break;
	}
	i++;
    }
    indent -= 2;
    print_indent (file);
    fprintf (file, "]\n");
}

void
print_style_family (FILE * file, Style_family * family)
{
    int     i = 0;

    print_indent (file);
    fprintf (file, "%s {\n", family->name);
    indent += 2;
    while (family->style[i].name != NULL) {
	print_style (file, &family->style[i]);
	i++;
    }
    indent -= 2;
    print_indent (file);
    fprintf (file, "}\n");
}

void
print_family_set (FILE * file, Style_family * family)
{
    int     i = 0;

    indent = 0;
    while (family[i].name != NULL) {
	print_style_family (file, &family[i]);
	i++;
    }
}

/* return the next non-blank line */

char   *
get_nstyle_line (char *string, int buflen, FILE * file)
{
    int     i;

    while (1) {
	if (fgets (string, buflen, file) == NULL)
	    return NULL;
	if (strlen (string) == 1)
	    continue;		/* empty (except for newline) - read next */
	for (i = strlen (string); i >= 0; i--)
	    if (string[i] != '\t' && string[i] != ' ')
		return string;	/* found at least one non-blank char */
    }
}

/* remove leading and trailing blanks */

void
trim (char *string)
{
    int     i, j, k, len;

    len = strlen (string);
    for (i = 0; i < len; i++)
	if (string[i] != ' ')
	    break;
    for (j = strlen (string) - 1; j > i; j--)
	if (string[j] != ' ')
	    break;
    if (j)
	string[j + 1] = '\0';
    /* now move string left if i>0 */
    if (i)
	for (k = 0; k <= j; i++, k++)
	    string[k] = string[i];
}

/* parse a single style */
int
parse_style (Style * style, FILE * file)
{
    int     i = 0, j = 0;
    char    name[256], value[256], string[256];
    long    pos;

    pos = ftell (file);
    if (get_nstyle_line (string, 256, file) == NULL)
	return 1;
    if ((sscanf (string, "%[^[^\n] [", name) != 1) || string[0] == '}') {
	fseek (file, pos, 0);
	return 1;
    }
    /* remove leading and trailing blanks */
    trim (name);
    style->name = my_strdup (name);
    if (get_nstyle_line (string, 256, file) == NULL)
	return 1;
    while (string[0] != ']' &&
	   i < MAX_STYLE_ELEMENT && sscanf (string, "%s : %s", name, value) == 2) {
	while (style_reference[j].name != NULL && strcmp (style_reference[j].name, name))
	    j++;
	if (style_reference[j].name != NULL) {
	    style->element[i].name = style_reference[j].name;
	    style->element[i].type = style_reference[j].type;
	    style->element[i].toset = style_reference[j].toset;
	    style->element[i].flag = style_reference[j].flag;
	    switch (style->element[i].type) {
		case Tint:
		    style->element[i].value = malloc (sizeof (int));
		    *((int *) style->element[i].value) = atoi (value);
		    break;

		case Tfloat:
		    style->element[i].value = malloc (sizeof (float));
		    *((float *) style->element[i].value) = atof (value);
		    break;

		case TColor:
		    style->element[i].value = malloc (sizeof (int));
		    *((int *) style->element[i].value) = atoi (value);
		    break;
	    }
	    i++;
	}
	if (get_nstyle_line (string, 256, file) == NULL)
	    return 1;
    }
    style->element[i].name = NULL;
    return 0;
}

int
parse_family_style (Style_family * family, FILE * file)
{
    int     i = 0, r;
    char    name[256], string[256];

    if (get_nstyle_line (string, 256, file) == NULL)
	return EOF;

    r = sscanf (string, "%[^{^\n] {", name);
    /* remove leading and trailing blanks */
    trim (name);
    family->name = my_strdup (name);
    while (i < MAX_STYLE_FAMILY && !parse_style (&family->style[i], file)) {
	i++;
    }
    if (get_nstyle_line (string, 256, file) == NULL)
	return 1;
    if (string[0] != '}') {
	i = 0;
	return 1;
    }
    if (i < MAX_STYLE_FAMILY)
	family->style[i].name = NULL;
    return 0;
}

void
parse_family_set (Style_family * family, FILE * file)
{
    int     i = 0, ok;

    while ((ok = parse_family_style (&family[i], file)) == 0)
	i++;
    if (ok == 1)
	i = 0;
    if (i < MAX_STYLE_FAMILY_SET)
	family[i].name = NULL;
}

void
load_family_set (Style_family * family)
{
    FILE   *file;
    char    name[PATH_MAX];

    strcpy (name, xfigrc_name);
    strcat (name, "style");
    if ((file = fopen (name, "r")) != NULL) {
	parse_family_set (family, file);
	fclose (file);
    }
    else {
	family[0].name = NULL;
    }
    /* clear dirty flag */
    style_dirty_flag = False;
}

void
save_family_set (Style_family * family)
{
    FILE   *file;
    char    name[PATH_MAX];

    strcpy (name, xfigrc_name);
    strcat (name, "style");
    file = fopen (name, "w");
    print_family_set (file, family);
    fclose (file);
    /* clear dirty flag */
    style_dirty_flag = False;
}

/* the name should not contain blanks */
int
add_family_to_family_set (Style_family * family, char *name)
{
    int     i, ok;

    /* search if the family already exists */
    i = 0;
    while (i < MAX_STYLE_FAMILY_SET &&
	   family[i].name != NULL && (ok = (strcmp (family[i].name, name) != 0)))
	i++;

    if ((i < MAX_STYLE_FAMILY_SET) && (family[i].name == NULL)) {
	family[i].name = my_strdup (name);
	family[i].style[0].name = NULL;
	if (i + 1 < MAX_STYLE_FAMILY_SET)
	    family[i + 1].name = NULL;
	/* set dirty flag */
	style_dirty_flag = True;
    }
    return i;
}

Boolean
add_style_to_family (Style_family * family,
		     int *iii, int *jjj,
		     char *family_name, char *style_name, unsigned long flag)
{
    int		i, j, ok;
    Boolean	status;

    i = add_family_to_family_set (family, family_name);
    j = 0;
    while (j < MAX_STYLE_FAMILY &&
	   family[i].style[j].name != NULL &&
	   (ok = (strcmp (family[i].style[j].name, style_name) != 0)))
	j++;

    if (j < MAX_STYLE_FAMILY) {
	if (family[i].style[j].name == NULL) {
	    family[i].style[j].name = my_strdup (style_name);
	    family[i].style[j].element[0].name = NULL;
	    if (j + 1 < MAX_STYLE_FAMILY)
		family[i].style[j + 1].name = NULL;
	    /* set dirty flag */
	    style_dirty_flag = True;
	}
	construct_style (&family[i].style[j], flag);
	*iii = i;
	*jjj = j;
	status = True;
    } else {
	*iii = -1;
	*jjj = -1;
	status = False;
    }
    return status;
}

void delete_family_from_family_set (Style_family * family, char *name)
{
    int		i, ii, j, k, ok;

    /* search if the family  exists */
    i = 0;
    while (i < MAX_STYLE_FAMILY_SET &&
	   family[i].name != NULL && (ok = (strcmp (family[i].name, name) != 0)))
	i++;
    /* it exists */
    if (!ok) {
	free (family[i].name);
	j = 0;
	while (j < MAX_STYLE_FAMILY && family[i].style[j].name != NULL) {
	    free (family[i].style[j].name);
	    k = 0;
	    while (family[i].style[j].element[k].name != NULL) {
		free (family[i].style[j].element[k].value);
		k++;
	    }
	    j++;
	}
	for (ii = i + 1; ii < MAX_STYLE_FAMILY_SET && family[ii].name != NULL; ii++) {
	    family[ii - 1].name = family[ii].name;
	    j = 0;
	    while (j < MAX_STYLE_FAMILY && family[ii].style[j].name != NULL) {
		family[ii - 1].style[j].name = family[ii].style[j].name;
		k = 0;
		while (family[ii].style[j].element[k].name != NULL) {
		    family[ii - 1].style[j].element[k] = family[ii].style[j].element[k];
		    k++;
		}
		family[ii - 1].style[j].element[k].name = NULL;
		j++;
	    }
	    family[ii - 1].style[j].name = NULL;
	}
	family[ii - 1].name = NULL;
	/* set dirty flag */
	style_dirty_flag = True;
    }
}

void delete_style_from_family (Style_family * family, char *family_name, char *style_name)
{
    int     i, j, jj, k, ok;

    /* search the family */
    i = 0;
    while (i < MAX_STYLE_FAMILY_SET &&
	   family[i].name != NULL && (ok = (strcmp (family[i].name, family_name) != 0)))
	i++;
    /* it exists */
    if (!ok) {
	/* search the style */
	j = 0;
	while (j < MAX_STYLE_FAMILY &&
	       family[i].style[j].name != NULL &&
	       (ok = (strcmp (family[i].style[j].name, style_name) != 0)))
	    j++;
	if (!ok) {
	    free (family[i].style[j].name);
	    k = 0;
	    while (family[i].style[j].element[k].name != NULL) {
		free (family[i].style[j].element[k].value);
		k++;
	    }
	    for (jj = j + 1;
		 jj < MAX_STYLE_FAMILY && family[i].style[jj].name != NULL; jj++) {
		family[i].style[jj - 1].name = family[i].style[jj].name;
		k = 0;
		while (family[i].style[jj].element[k].name != NULL) {
		    family[i].style[jj - 1].element[k] = family[i].style[jj].element[k];
		    k++;
		}
		family[i].style[jj].element[k].name = NULL;
	    }
	    family[i].style[jj - 1].name = NULL;
	    /* set dirty flag */
	    style_dirty_flag = True;
	}
    }
}

static void
build_family_text (Style_family * family)
{
    int     i = 0;

    while (i < MAX_STYLE_FAMILY_SET && family[i].name != NULL) {
	family_text[i] = family[i].name;
	i++;
    }
    family_text[i] = NULL;
}

static void
build_style_text (Style_family * family, int f)
{
    int     i = 0;

    if (f >= 0) {
	while (i < MAX_STYLE_FAMILY && family[f].style[i].name != NULL) {
	    style_text[i] = family[f].style[i].name;
	    i++;
	}
	style_text[i] = NULL;
    }
    else
	style_text[0] = NULL;
}

static void
style_update (void)
{
    build_family_text (current_family_set);
    build_style_text (current_family_set, current_family);
    XawListChange (style_family_list, family_text, 0, 0, True);
    XawListChange (style_style_list, style_text, 0, 0, True);
    if (current_family >= 0)
	panel_set_value (style_family_name, family_text[current_family]);
    else
	panel_set_value (style_family_name, "");

    if (current_style >= 0)
	panel_set_value (style_style_name, style_text[current_style]);
    else
	panel_set_value (style_style_name, "");
}

static void
family_select (Widget w, XtPointer closure, XtPointer call_data)
{

    XawListReturnStruct *ret_struct = (XawListReturnStruct *) call_data;

    current_family = ret_struct->list_index;
    current_style = -1;
    style_update ();
}

static void
style_select (Widget w, XtPointer closure, XtPointer call_data)
{
    XawListReturnStruct *ret_struct = (XawListReturnStruct *) call_data;

    current_style = ret_struct->list_index;
    style_update ();
    cur_updatemask = set_style (&current_family_set[current_family].style[current_style]);

    tog_selective_update (cur_updatemask);
    update_current_settings ();
}

void
add_family (Widget w, XButtonEvent *ev)
{
    char   *fval;

    FirstArg (XtNstring, &fval);
    GetValues (style_family_name);
    if (strlen (fval)) {
	current_family = add_family_to_family_set (current_family_set, fval);
	current_style = -1;
	style_update ();
    }
}

void
delete_family (Widget w, XButtonEvent *ev)
{
    char   *fval;

    FirstArg (XtNstring, &fval);
    GetValues (style_family_name);
    if (strlen (fval)) {
	delete_family_from_family_set (current_family_set, fval);
	current_family = -1;
	current_style = -1;
	style_update ();
    }
}

void
add_style (Widget w, XButtonEvent *ev)
{
    char   *fval, *sval, ftemp[256], stemp[256];

    FirstArg (XtNstring, &fval);
    GetValues (style_family_name);
    strcpy (ftemp, fval);
    FirstArg (XtNstring, &sval);
    GetValues (style_style_name);
    strcpy (stemp, sval);
    if (strlen (ftemp) && strlen (stemp)) {
	if (!add_style_to_family (current_family_set,
			     &current_family, &current_style,
			     ftemp, stemp, cur_updatemask))
	    file_msg("Sorry, no more room available in this family : use a new one");
	style_update ();
    }
}



void
delete_style (Widget w, XButtonEvent *ev)
{
    char   *fval, *sval;

    FirstArg (XtNstring, &fval);
    GetValues (style_family_name);
    FirstArg (XtNstring, &sval);
    GetValues (style_style_name);
    if (strlen (fval) && strlen (sval)) {
	delete_style_from_family (current_family_set, fval, sval);
	current_style = -1;
	style_update ();
    }
}

void
save_style (Widget w, XButtonEvent *ev)
{
    save_family_set (current_family_set);
}

void
load_style (Widget w, XButtonEvent *ev)
{
    load_family_set (current_family_set);
    current_family = current_style = -1;
    style_update ();
}

int
confirm_close_style (void)
{
    int      status;

    if (style_dirty_flag) {
	status = popup_query(QUERY_YESNOCAN, "Do you wish to save the changes you made to the styles?");
	if (status == RESULT_YES)
	    save_style((Widget) 0, (XEvent *) 0);
	return status;
    }
    return RESULT_YES;
}

void
close_style (Widget w, XButtonEvent *ev)
{
    if (confirm_close_style() == RESULT_CANCEL)
	return;
    XtPopdown (style_popup);
}

static String style_family_list_translations = "<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	 <Btn1Up>(2): family_select()\n\
	 <Key>Escape: CloseStyle()\n\
	 <Key>Return: family_select()\n";

static String style_family_name_translations = "<Key>Escape: CloseStyle()\n\
	 <Key>Return: add_family()\n";

static String style_style_list_translations = "<Btn1Down>,<Btn1Up>: Set()Notify()\n\
	 <Btn1Up>(2): style_select()\n\
	 <Key>Escape: CloseStyle()\n\
	 <Key>Return: style_select()\n";

static String style_style_name_translations = "<Key>Escape: CloseStyle()\n\
	 <Key>Return: add_style()\n";

static String style_translations = "<Message>WM_PROTOCOLS: CloseStyle()\n\
	 <Key>Escape: CloseStyle()\n";

static XtActionsRec style_actions[] = {
    "ShowNamedStyles", (XtActionProc) popup_manage_style_panel,
    "add_family", (XtActionProc) add_family,
    "delete_family", (XtActionProc) delete_family,
    "add_style", (XtActionProc) add_style,
    "delete_style", (XtActionProc) delete_style,
    "family_select", (XtActionProc) family_select,
    "style_select", (XtActionProc) style_select,
    "CloseStyle", (XtActionProc) close_style
};

/**********************************/
/* create the style manager panel */
/**********************************/

void
init_manage_style_panel (void)
{
    load_family_set (current_family_set);
    FirstArg (XtNtitle, "Xfig: Manage Styles");
    NextArg (XtNcolormap, tool_cm);

    style_popup = XtCreatePopupShell ("style_manage_panel",
				      transientShellWidgetClass, tool, Args, ArgCount);

    XtOverrideTranslations (style_popup, XtParseTranslationTable (style_translations));
    FirstArg (XtNborderWidth, 0);

    style_main_form = XtCreateManagedWidget ("style_main_form",
					     formWidgetClass, style_popup,
					     Args, ArgCount);
    XtOverrideTranslations (style_main_form,
			    XtParseTranslationTable (style_translations));

    /* label for title - "Manage Styles" */
    FirstArg (XtNlabel, "Manage Styles");
    NextArg (XtNborderWidth, 0);
    style_main_label =
	XtCreateManagedWidget ("style_main_label", labelWidgetClass,
			       style_main_form, Args, ArgCount);

    /* frame for Family stuff */

    FirstArg (XtNborderWidth, 1);
    NextArg (XtNfromVert, style_main_label);
    style_family_form = XtCreateManagedWidget ("style_family_form",
					       formWidgetClass,
					       style_main_form, Args, ArgCount);
    /* Put family stuff */
    FirstArg (XtNlabel, "Family");
    NextArg (XtNborderWidth, 0);
    style_family_label =
	XtCreateManagedWidget ("style_family_label", labelWidgetClass,
			       style_family_form, Args, ArgCount);

    FirstArg (XtNallowVert, True);
    NextArg (XtNfromHoriz, style_family_label);
    NextArg (XtNborderWidth, INTERNAL_BW);
    NextArg (XtNwidth, 180);
    NextArg (XtNheight, 100);
    NextArg (XtNtop, XtChainBottom);
    NextArg (XtNbottom, XtChainBottom);
    NextArg (XtNleft, XtChainLeft);
    NextArg (XtNright, XtChainRight);
    style_family_list_viewport =
	XtCreateManagedWidget ("style_family_list_viewport",
			       viewportWidgetClass, style_family_form, Args, ArgCount);
    FirstArg (XtNlist, family_text);
    NextArg (XtNforceColumns, True);
    NextArg (XtNdefaultColumns, 1);
    NextArg (XtNallowVert, True);
    style_family_list =
	XtCreateManagedWidget ("family_list", figListWidgetClass,
			       style_family_list_viewport, Args, ArgCount);

    XawListChange (style_family_list, family_text, 0, 0, True);
    XtAddCallback (style_family_list, XtNcallback, family_select, (XtPointer) NULL);
    XtAugmentTranslations (style_family_list,
			   XtParseTranslationTable (style_family_list_translations));

    FirstArg (XtNlabel, "Choice");
    NextArg (XtNfromVert, style_family_list_viewport);
    NextArg (XtNborderWidth, 0);
    style_family_choice_label =
	XtCreateManagedWidget ("family_choice_label", labelWidgetClass,
			       style_family_form, Args, ArgCount);
    FirstArg (XtNeditType, XawtextEdit);
    NextArg (XtNfromVert, style_family_list_viewport);
    NextArg (XtNfromHoriz, style_family_choice_label);
    NextArg (XtNwidth, 180);
    NextArg (XtNheight, 30);
    NextArg (XtNstring, "\0");
    NextArg (XtNscrollHorizontal, XawtextScrollWhenNeeded);
    style_family_name = XtCreateManagedWidget ("style_family_name",
					       asciiTextWidgetClass,
					       style_family_form, Args, ArgCount);
    XtOverrideTranslations (style_family_name,
			    XtParseTranslationTable (style_family_name_translations));

    FirstArg (XtNlabel, "Add");
    NextArg (XtNfromVert, style_family_name);
    style_add_family =
	XtCreateManagedWidget ("style_add_family", commandWidgetClass,
			       style_family_form, Args, ArgCount);
    XtAddCallback (style_add_family, XtNcallback, (XtCallbackProc) add_family,
		   (XtPointer) NULL);

    FirstArg (XtNlabel, "Delete");
    NextArg (XtNfromVert, style_family_name);
    NextArg (XtNfromHoriz, style_add_family);
    style_delete_family = XtCreateManagedWidget ("style_delete_family",
						 commandWidgetClass,
						 style_family_form, Args, ArgCount);
    XtAddCallback (style_delete_family, XtNcallback,
		   (XtCallbackProc) delete_family, (XtPointer) NULL);


    /* frame for Style stuff inside family frame */

    FirstArg (XtNborderWidth, 1);
    NextArg (XtNfromVert, style_add_family);
    style_style_form = XtCreateManagedWidget ("style_style_form",
					      formWidgetClass,
					      style_family_form, Args, ArgCount);
    FirstArg (XtNlabel, "Style ");
    NextArg (XtNborderWidth, 0);
    style_style_label =
	XtCreateManagedWidget ("style_style_label", labelWidgetClass,
			       style_style_form, Args, ArgCount);
    FirstArg (XtNallowVert, True);
    NextArg (XtNfromHoriz, style_style_label);
    NextArg (XtNborderWidth, INTERNAL_BW);
    NextArg (XtNwidth, 180);
    NextArg (XtNheight, 100);
    NextArg (XtNtop, XtChainBottom);
    NextArg (XtNbottom, XtChainBottom);
    NextArg (XtNleft, XtChainLeft);
    NextArg (XtNright, XtChainRight);
    style_style_list_viewport =
	XtCreateManagedWidget ("style_family_list_viewport",
			       viewportWidgetClass, style_style_form, Args, ArgCount);


    FirstArg (XtNlist, style_text);
    NextArg (XtNforceColumns, True);
    NextArg (XtNdefaultColumns, 1);
    style_style_list =
	XtCreateManagedWidget ("style_list", figListWidgetClass,
			       style_style_list_viewport, Args, ArgCount);

    XawListChange (style_style_list, style_text, 0, 0, True);
    XtAddCallback (style_style_list, XtNcallback, style_select, (XtPointer) NULL);
    XtAugmentTranslations (style_style_list,
			   XtParseTranslationTable (style_style_list_translations));


    FirstArg (XtNlabel, "Choice");
    NextArg (XtNfromVert, style_style_list_viewport);
    NextArg (XtNborderWidth, 0);
    style_style_choice_label =
	XtCreateManagedWidget ("style_choice_label", labelWidgetClass,
			       style_style_form, Args, ArgCount);
    FirstArg (XtNeditType, XawtextEdit);
    NextArg (XtNfromVert, style_style_list_viewport);
    NextArg (XtNfromHoriz, style_style_choice_label);
    NextArg (XtNwidth, 180);
    NextArg (XtNheight, 30);
    NextArg (XtNstring, "\0");
    NextArg (XtNscrollHorizontal, XawtextScrollWhenNeeded);
    style_style_name = XtCreateManagedWidget ("style_style_name",
					      asciiTextWidgetClass,
					      style_style_form, Args, ArgCount);
    XtOverrideTranslations (style_style_name,
			    XtParseTranslationTable (style_style_name_translations));

    FirstArg (XtNlabel, "Add");
    NextArg (XtNfromVert, style_style_name);
    style_add_style =
	XtCreateManagedWidget ("style_add_family", commandWidgetClass,
			       style_style_form, Args, ArgCount);
    XtAddCallback (style_add_style, XtNcallback, (XtCallbackProc) add_style,
		   (XtPointer) NULL);

    FirstArg (XtNlabel, "Delete");
    NextArg (XtNfromVert, style_style_name);
    NextArg (XtNfromHoriz, style_add_style);
    style_delete_style = XtCreateManagedWidget ("style_delete_family",
						commandWidgetClass,
						style_style_form, Args, ArgCount);
    XtAddCallback (style_delete_style, XtNcallback,
		   (XtCallbackProc) delete_style, (XtPointer) NULL);

    /* overall save/load/close buttons */

    FirstArg (XtNlabel, "Save settings");
    NextArg (XtNfromVert, style_family_form);
    style_save_style = XtCreateManagedWidget ("style_save",
					      commandWidgetClass,
					      style_main_form, Args, ArgCount);
    XtAddCallback (style_save_style, XtNcallback, (XtCallbackProc) save_style,
		   (XtPointer) NULL);

    FirstArg (XtNlabel, "Reload settings");
    NextArg (XtNfromVert, style_family_form);
    NextArg (XtNfromHoriz, style_save_style);
    style_load_style = XtCreateManagedWidget ("style_load",
					      commandWidgetClass,
					      style_main_form, Args, ArgCount);
    XtAddCallback (style_load_style, XtNcallback, (XtCallbackProc) load_style,
		   (XtPointer) NULL);

    FirstArg (XtNlabel, "Close");
    NextArg (XtNfromHoriz, style_load_style);
    NextArg (XtNfromVert, style_family_form);
    style_close_style = XtCreateManagedWidget ("style_close",
					       commandWidgetClass,
					       style_main_form, Args, ArgCount);
    XtAddCallback (style_close_style, XtNcallback,
		   (XtCallbackProc) close_style, (XtPointer) NULL);

    style_update ();
}

void
add_style_actions(void)
{
    XtAppAddActions (tool_app, style_actions, XtNumber (style_actions));
}

/* now that the main tool has been created we can position (although it
   is not popped up yet) the style panel */

void
setup_manage_style_panel (void)
{
    Position x_val, y_val;
    Dimension width, height;

    FirstArg (XtNwidth, &width);
    NextArg (XtNheight, &height);
    GetValues (tool);
    /* position the popup 1/6 in from left and 1/4 down from top */
    XtTranslateCoords (tool, (Position) (width / 6), (Position) (height / 4),
		       &x_val, &y_val);

    FirstArg (XtNx, x_val);
    NextArg (XtNy, y_val);
    SetValues (style_popup);
}

void
popup_manage_style_panel (void)
{
    /* first make sure we're in update mode */
    change_mode (&update_ic);
    /* use GrabNone so user can choose styles while drawing */
    XtPopup (style_popup, XtGrabNone);
    (void) XSetWMProtocols (tool_d, XtWindow (style_popup), &wm_delete_window, 1);
}
