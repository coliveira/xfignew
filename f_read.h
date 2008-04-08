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

/* errors from read_figc in addition to those in errno.h (e.g. ENOENT) */
#define	BAD_FORMAT		-1
#define	EMPTY_FILE		-2
#define NO_VERSION		-3

#define MERGE			True
#define DONT_MERGE		False

#define REMAP_IMAGES		True
#define DONT_REMAP_IMAGES	False

extern int	 defer_update_layers;	/* if != 0, update_layers() doesn't update */
extern int	 line_no;
extern int	 num_object;
extern char	*read_file_name;

/* structure which is filled by readfp_fig */

typedef struct {
    Boolean	landscape;
    Boolean	flushleft;
    Boolean	units;
    int		grid_unit;
    int		papersize;
    float	magnification;
    Boolean	multiple;
    int		transparent;
    } fig_settings;

extern Boolean	 uncompress_file(char *name);
extern int	 read_figc(char *file_name, F_compound *obj, Boolean merge, Boolean remapimages, int xoff, int yoff, fig_settings *settings);
extern int	 read_fig(char *file_name, F_compound *obj, Boolean merge, int xoff, int yoff, fig_settings *settings);
extern int parse_papersize(char *size);
extern void fix_angle (float *angle);
extern void swap_colors (void);
