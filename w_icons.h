/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
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

#ifndef W_ICONS_H
#define W_ICONS_H

#ifdef USE_SMALL_ICONS
#define		MODE_SW_HT	22	/* height of a small mode switch icon */
#define		MODE_SW_WD	22	/* width of a small mode switch icon */
#else
#define		MODE_SW_HT	32	/* height of a mode switch icon */
#define		MODE_SW_WD	36	/* width of a mode switch icon */
#endif

typedef struct _icon_struct {
    int	    width, height;
    char   *bits;
}	icon_struct;

extern icon_struct	addpt_ic;
extern icon_struct	figure_ic;
extern char		*fig_full_c_icon_X[], *fig_reduced_c_icon_X[];

extern icon_struct	smartoff_ic;
extern icon_struct	smartmove_ic;
extern icon_struct	smartslide_ic;
extern icon_struct	arc_ic;
extern icon_struct	autoarrow_ic;
extern icon_struct	backarrow_ic;
extern icon_struct	box_ic;
extern icon_struct	regpoly_ic;
extern icon_struct	picobj_ic;
extern icon_struct	arc_box_ic;
extern icon_struct	cirrad_ic;
extern icon_struct	cirdia_ic;
extern icon_struct	c_spl_ic;
extern icon_struct	c_xspl_ic;
extern icon_struct	copy_ic;
extern icon_struct	glue_ic;
extern icon_struct	break_ic;
extern icon_struct	library_ic;
extern icon_struct	open_comp_ic;
extern icon_struct	join_split_ic;
extern icon_struct	chop_ic;
extern icon_struct	joinmiter_ic;
extern icon_struct	joinround_ic;
extern icon_struct	joinbevel_ic;
extern icon_struct	capbutt_ic;
extern icon_struct	capround_ic;
extern icon_struct	capproject_ic;
extern icon_struct	solidline_ic;
extern icon_struct	dashline_ic;
extern icon_struct	dottedline_ic;
extern icon_struct	dashdotline_ic;
extern icon_struct	dash2dotsline_ic;
extern icon_struct	dash3dotsline_ic;
extern icon_struct	deletept_ic;
extern icon_struct	ellrad_ic;
extern icon_struct	elldia_ic;
extern icon_struct	flip_x_ic;
extern icon_struct	flip_y_ic;
extern icon_struct	forarrow_ic;
extern icon_struct	grid1_ic, grid2_ic, grid3_ic, grid3_ic, grid4_ic;
extern icon_struct	intspl_ic;
extern icon_struct	c_intspl_ic;
extern icon_struct	line_ic;
extern icon_struct	fine_grid_ic;
extern icon_struct	unconstrained_ic;
extern icon_struct	latexline_ic;
extern icon_struct	latexarrow_ic;
extern icon_struct	mounthattan_ic;
extern icon_struct	manhattan_ic;
extern icon_struct	mountain_ic;
extern icon_struct	move_ic;
extern icon_struct	movept_ic;
extern icon_struct	polygon_ic;
extern icon_struct	delete_ic;
extern icon_struct	rotCW_ic;
extern icon_struct	rotCCW_ic;
extern icon_struct	scale_ic;
extern icon_struct	convert_ic;
extern icon_struct	spl_ic;
extern icon_struct	text_ic;
extern icon_struct	update_ic;
extern icon_struct	edit_ic;
extern icon_struct	halignl_ic;
extern icon_struct	halignr_ic;
extern icon_struct	halignc_ic;
extern icon_struct	haligndc_ic;
extern icon_struct	halignde_ic;
extern icon_struct	haligna_ic;
extern icon_struct	valignt_ic;
extern icon_struct	valignb_ic;
extern icon_struct	valignc_ic;
extern icon_struct	valigndc_ic;
extern icon_struct	valignde_ic;
extern icon_struct	valigna_ic;
extern icon_struct	align_ic;
extern icon_struct	any_ic;
extern icon_struct	none_ic;
extern icon_struct	fill_ic;
extern icon_struct	blank_ic;
extern icon_struct	textL_ic;
extern icon_struct	textC_ic;
extern icon_struct	textR_ic;
extern icon_struct	noarrows_ic;
extern icon_struct	farrows_ic;
extern icon_struct	barrows_ic;
extern icon_struct	fbarrows_ic;
extern icon_struct	open_arc_ic;
extern icon_struct	pie_wedge_arc_ic;
extern icon_struct	xspl_ic;
extern icon_struct	tangent_ic;
extern icon_struct	anglemeas_ic;
extern icon_struct	lenmeas_ic;
extern icon_struct	areameas_ic;

/* misc icons */

extern icon_struct	kbd_ic;
extern icon_struct	printer_ic;

extern icon_struct	no_arrow_ic;
extern icon_struct	arrow0_ic;
extern icon_struct	arrow1o_ic, arrow1f_ic;
extern icon_struct	arrow2o_ic, arrow2f_ic;
extern icon_struct	arrow3o_ic, arrow3f_ic;
extern icon_struct	arrow4o_ic, arrow4f_ic;
extern icon_struct	arrow5o_ic, arrow5f_ic;
extern icon_struct	arrow6o_ic, arrow6f_ic;
extern icon_struct	arrow7o_ic, arrow7f_ic;
extern icon_struct	arrow8o_ic, arrow8f_ic;
extern icon_struct	arrow9a_ic, arrow9b_ic;
extern icon_struct	arrow10o_ic, arrow10f_ic;
extern icon_struct	arrow11o_ic, arrow11f_ic;
extern icon_struct	arrow12o_ic, arrow12f_ic;
extern icon_struct	arrow13a_ic, arrow13b_ic;
extern icon_struct	arrow14a_ic, arrow14b_ic;
extern unsigned char	no_arrow_bits[];
extern unsigned char	arrow0_bits[];
extern unsigned char	arrow1o_bits[], arrow1f_bits[];
extern unsigned char	arrow2o_bits[], arrow2f_bits[];
extern unsigned char	arrow3o_bits[], arrow3f_bits[];
extern unsigned char	arrow4o_bits[], arrow4f_bits[];
extern unsigned char	arrow5o_bits[], arrow5f_bits[];
extern unsigned char	arrow6o_bits[], arrow6f_bits[];
extern unsigned char	arrow7o_bits[], arrow7f_bits[];
extern unsigned char	arrow8o_bits[], arrow8f_bits[];
extern unsigned char	arrow9a_bits[], arrow9b_bits[];
extern unsigned char	arrow10o_bits[], arrow10f_bits[];
extern unsigned char	arrow11o_bits[], arrow11f_bits[];
extern unsigned char	arrow12o_bits[], arrow12f_bits[];
extern unsigned char	arrow13a_bits[], arrow13b_bits[];
extern unsigned char	arrow14a_bits[], arrow14b_bits[];

/* for splash screen */

#ifdef USE_XPM
extern char		*spl_bckgnd_xpm[];
#endif /* USE_XPM */
extern icon_struct	letters_ic;
extern icon_struct	spl_bckgnd_ic;

#endif /* W_ICONS_H */
