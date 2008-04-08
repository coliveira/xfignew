/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2004 by Chris Moller
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static int
quadratic(double * cv, double * zr, double * zi)
{

  double dum1;
  double discrim;
#define qa cv[2]
#define qb cv[1]
#define qc cv[0]
  
  if (qa == 0.0){
    if (qb == 0.0)
      return 0;
    else {
      zr[0] = -qc/qb;
      zi[0] = 0;
      return 1;
    }
  }

  if (qb == 0.0){
    dum1 = -qc/qa;
    if (dum1 < 0.0) {
      dum1 = -dum1;
      dum1 = sqrt(dum1);
      zr[1] = zr[0] = 0.0;
      zi[1] = zi[0] = dum1;
    }
    else{
      dum1 = sqrt(dum1);
      zr[0] = dum1;
      zr[1] = -dum1;
      zi[0] = zi[1] = 0.0;
    }
    return 2;
  }

  if (!qc){
    zi[1] = zi[0] = zr[0] = 0.0;
    zr[1] = -qb/qa;
    return 2;
  }

  qb /= 2.0*qa;
  qc /= qa;

  discrim = -qc + pow(qb, 2.0);

  if (discrim < 0.0) {
    discrim = -discrim;
    zr[1] = zr[0] = -qb;
    zi[1] = zi[0] = sqrt(discrim);
  } 
  else {
    if (qb < 0.0){
      qb = -qb;
      qa = qb + sqrt(discrim);
    }
    else {
      qa = qb + sqrt(discrim);
      qa = -qa;
    }
    zr[0] = qc/qa;
    zr[1] = qa;
    zi[0] = zi[1] = 0.0; 
  }
  return 2;
}


static int
cubic(double * cv, double * zr, double * zi)
{

#define ca cv[3]
#define cb cv[2]
#define cc cv[1]
#define cd cv[0]
  
  double discrim, q, r, dum1, s, t, term1, r13;
  int i;
  
  if (ca == 0.0){
    return quadratic(cv, zr, zi);
  }

  if (cd == 0.0){
    int n;
    fprintf(stderr, "doing quadratic\n");
    n =  quadratic(&cv[1], zr, zi);
    zr[n] = zi[n] = 0.0;
    return n+1;
  }

  if (ca != 1.0) {
    for (i = 0; i < 3; i++) cv[i] /= cv[3];
  }

  q = (3.0 * cc - pow(cb, 2.0))/9.0;
  r = -(27.0 * cd) + cb * ((9.0 * cc) - (2.0 * pow(cb, 2.0)));
  r /= 54.0;

  discrim = pow(q, 3.0) + pow(r, 2.0);
  zi[0] =  0.0; 		/* The first root is always real.*/
  term1 = (cb/3.0);

  if (discrim > 0.0) {		 /* one root real, two are complex */
    s = r +sqrt(discrim);
    s = ((s < 0) ? -pow(-s, (1.0/3.0)) : pow(s, (1.0/3.0)));
    t = r - sqrt(discrim);
    t = ((t < 0) ? -pow(-t, (1.0/3.0)) : pow(t, (1.0/3.0)));
    zr[0] = -term1 + s + t;
    term1 += (s + t)/2.0;
    zr[2] = zr[1] = -term1;
    term1 = sqrt(3.0)*(-t + s)/2;
    zi[1] = term1;
    zi[2] = -term1;
    return 3;
  }

  /* The remaining options are all real */
  zi[2] = zi[1] = 0.0;

  if (discrim == 0.0){ 			/* All roots real, at least two are equal. */
    r13 = ((r < 0) ? -pow(-r,(1.0/3.0)) : pow(r,(1.0/3.0)));
    zr[0] = -term1 + 2.0*r13;
    zr[2] = zr[1] = -(r13 + term1);
    return 3;
  }

  /* Only option left is that all roots are real and unequal (to get here, q < 0) */
  q = -q;
  dum1 = pow(q, 3.0);
  dum1 = acos(r/sqrt(dum1));
  r13 = 2.0 * sqrt(q);
  zr[0] = -term1 + r13 * cos(dum1/3.0);
  zr[1] = -term1 + r13*cos((dum1 + 2.0*M_PI)/3.0);
  zr[2] = -term1 + r13*cos((dum1 + 4.0*M_PI)/3.0);
  return 3;
}

int
quartic(double * ck, double * zr, double * zi)
{
  
#define a ck[4]
#define b ck[3]
#define c ck[2]
#define d ck[1]
#define e ck[0]

  double cl, cm, cn;  /* Coefficients for use with cubic solver */
  double discrim, q, r, RRe, RIm, DRe, DIm, dum1, ERe, EIm, s, t, term1,
    r13, sqR, y1, z1Re, z1Im, z2Re;
  int i;

  if (a == 0.0) {
    return cubic(ck, zr, zi);
  } 

  if (e == 0.0) {
    int n;
    fprintf(stderr, "doing cubic\n");
    n =  cubic(&ck[1], zr, zi);
    zr[n] = zi[n] = 0.0;
    return n+1;
  }

  if (a != 1.0) {
    for (i = 0; i < 4; i++) ck[i] /= ck[4];
  }

  cl = -c;
  cm = (-4.0 * e) + (d * b);
  cn = -(pow(b, 2.0) * e + pow(d, 2.0)) + (4.0 * c * e);

  if (cn == 0.0) return 0;	/* not sure why */

  /* Solve the resolvant cubic for y1 */
  q = (3.0*cm - (cl*cl))/9.0;
  r = -(27.0*cn) + cl*(9.0*cm - 2.0*(cl*cl));
  r /= 54.0;
  discrim = q*q*q + r*r;
  term1 = (cl/3.0);

  if (discrim > 0.0) { /* one root real, two are complex */
    s = r + sqrt(discrim);
    s = ((s < 0.0) ? -pow(-s, (1.0/3.0)) : pow(s, (1.0/3.0)));
    t = r - sqrt(discrim);
    t = ((t < 0.0) ? -pow(-t, (1.0/3.0)) : pow(t, (1.0/3.0)));
    y1 = -term1 + s + t;
  }
  else {
    if (discrim == 0.0) {
      r13 = ((r < 0.0) ? -pow(-r,(1.0/3.0)) : pow(r,(1.0/3.0)));
      y1 = -term1 + 2.0*r13;
    }
    else {
      q = -q;
      dum1 = q*q*q;
      dum1 = acos(r/sqrt(dum1));
      r13 = 2.0*sqrt(q);
      y1 = -term1 + r13*cos(dum1/3.0);
    }
  }

  /* At this point, we have determined y1, a real root of the resolvent cubic. */
  /* Carry on to solve the original quartic equation */

  term1 = b/4.0;
  sqR = -c + term1*b + y1;  /* R-squared */

  RRe = RIm = DRe = DIm = ERe = EIm = z1Re = z1Im = z2Re = 0.0;

  if (sqR >= 0.0) {
    if (sqR == 0.0) {
      dum1 = -(4.0*e) + y1*y1;
      if (dum1 < 0.0) 		/* D and E will be complex */
	z1Im = 2.0*sqrt(-dum1);
      else {
	z1Re = 2.0*sqrt(dum1);
	z2Re = -z1Re;
      }
    }
    else {
      RRe = sqrt(sqR);
      z1Re = -(8.0*d + b*b*b)/4.0 + b*c;
      z1Re /= RRe;
      z2Re = -z1Re;
    }
  }
  else {
    RIm = sqrt(-sqR);
    z1Im = -(8.0*d + b*b*b)/4.0 + b*c;
    z1Im /= RIm;
    z1Im = -z1Im;
  }

  z1Re += -(2.0*c + sqR) + 3.0*b*term1;
  z2Re += -(2.0*c + sqR) + 3.0*b*term1;

  /* At this point, z1 and z2 should be the terms under the square root for D and E */

  if (z1Im == 0.0){ 		/* Both z1 and z2 real */
    if (z1Re >= 0.0) DRe = sqrt(z1Re);
    else             DIm = sqrt(-z1Re);
    if (z2Re >= 0.0) ERe = sqrt(z2Re);
    else             EIm = sqrt(-z2Re);
  }
  else { 		/* else (zIm != 0); calculate root of a complex number******** */
    r = sqrt(z1Re*z1Re + z1Im*z1Im); 	/* Calculate r, the magnitude */
    r = sqrt(r);

    dum1 = atan2(z1Im, z1Re); 		/* Calculate the angle between the two vectors */
    dum1 /= 2; 				/* Divide this angle by 2 */
    ERe = DRe = r*cos(dum1);		 /* Form the new complex value */
    DIm = r*sin(dum1);
    EIm = -DIm; 
  } 

  zr[0] = -term1 + (RRe + DRe)/2.0;
  zi[0] = (RIm + DIm)/2.0;
  zr[1] = -(term1 + DRe/2.0) + RRe/2.0;
  zi[1] = (-DIm + RIm)/2.0;
  zr[2] = -(term1 + RRe/2.0) + ERe/2.0;
  zi[2] = (-RIm + EIm)/2.0;
  zr[3] = -(term1 + (RRe + ERe)/2.0);
  zi[3] = -(RIm + EIm)/2.0;

  return 4;
} 
