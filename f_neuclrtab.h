/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1994 Anthony Dekker
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

#ifndef F_NEUCLRTAB_H
#define F_NEUCLRTAB_H

/*
 * Neural-Net quantization algorithm based on work of Anthony Dekker
 */

/*
 *  color.h - header for routines using pixel color values.
 *
 *     12/31/85
 *
 *  Two color representations are used, one for calculation and
 *  another for storage.  Calculation is done with three floats
 *  for speed.  Stored color values use 4 bytes which contain
 *  three single byte mantissas and a common exponent.
 */

#define  N_RED		0
#define  N_GRN		1
#define  N_BLU		2

typedef unsigned char  BYTE;	/* 8-bit unsigned integer */
typedef BYTE	 COLR[4];	/* red, green, blue, exponent */
extern	BYTE	 clrtab[][3];

extern int neu_init(long int npixels);
extern int neu_init2 (long int npixels);
extern int neu_clrtab(int ncolors);
extern void neu_pixel(register BYTE *col);
extern int neu_map_pixel(register BYTE *col);


#define MIN_NEU_SAMPLES	600	/* min number of samples (npixels/samplefac) needed for network */
#endif /* F_NEUCLRTAB_H */
