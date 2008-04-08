/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Change function implemented by Frank Schmuck (schmuck@svax.cs.cornell.edu)
 * X version by Jon Tombs <jon@uk.ac.oxford.robots>
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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

extern char	*xf_basename(char *filename);
extern int	 emptyfigure(void);
extern char	*safe_strcpy(char *p1, char *p2);
extern Boolean	 uncompress_file(char *name);
extern char	*build_command(char *program, char *filename);
extern Boolean	 map_to_palette(F_pic *pic);
extern Boolean	 dimline_components(F_compound *dimline, F_line **line, F_line **tick1, F_line **tick2, F_line **poly);
extern int	 find_largest_depth(F_compound *compound);
extern int	 find_smallest_depth(F_compound *compound);
extern void	 get_grid_spec(char *grid, Widget minor_grid_panel, Widget major_grid_panel);
extern time_t	 file_timestamp(char *file);
extern void map_to_mono(F_pic *pic);
extern void read_xfigrc(void);
extern void init_settings(void);
extern int change_directory(char *path);
extern void beep (void);
extern int emptyfigure_msg (char *msg);
extern int emptyname (char *name);
extern int emptyname_msg (char *name, char *msg);
extern int get_directory (char *direct);
extern int ok_to_write (char *file_name, char *op_name);
extern void remap_imagecolors (void);
extern void update_recent_files (void);
extern void update_xfigrc (char *name, char *string);
int		update_fig_files(int argc, char** argv);
