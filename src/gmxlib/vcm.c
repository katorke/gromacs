/*
 *       @(#) copyrgt.c 1.12 9/30/97
 *
 *       This source code is part of
 *
 *        G   R   O   M   A   C   S
 *
 * GROningen MAchine for Chemical Simulations
 *
 *            VERSION 2.0b
 * 
 * Copyright (c) 1990-1997,
 * BIOSON Research Institute, Dept. of Biophysical Chemistry,
 * University of Groningen, The Netherlands
 *
 * Please refer to:
 * GROMACS: A message-passing parallel molecular dynamics implementation
 * H.J.C. Berendsen, D. van der Spoel and R. van Drunen
 * Comp. Phys. Comm. 91, 43-56 (1995)
 *
 * Also check out our WWW page:
 * http://rugmd0.chem.rug.nl/~gmx
 * or e-mail to:
 * gromacs@chem.rug.nl
 *
 * And Hey:
 * Great Red Oystrich Makes All Chemists Sane
 */
#include "macros.h"
#include "vcm.h"
#include "vec.h"
#include "smalloc.h"
 
void calc_vcm(FILE *log,int homenr,int start,real mass[],rvec v[],rvec vcm)
{
  int    i;
  real   m0;
  real   x,y,z;
  
  /* Calculate */
  x=y=z=0;
  for(i=start; (i<start+homenr); i++) {
    m0=mass[i];
    x+=m0*v[i][XX];
    y+=m0*v[i][YY];
    z+=m0*v[i][ZZ];
  }
  vcm[XX]=x;
  vcm[YY]=y;
  vcm[ZZ]=z;
}

void do_stopcm(FILE *log,int homenr,int start,rvec v[],rvec mvcm,real tm)
{
  int  i;
  real x,y,z;
  rvec vcm;
  
  vcm[XX]=mvcm[XX]/tm;
  vcm[YY]=mvcm[YY]/tm;
  vcm[ZZ]=mvcm[ZZ]/tm;

  for(i=start; (i<start+homenr); i++) 
    rvec_dec(v[i],vcm);
}

void check_cm(FILE *log,rvec mvcm,real tm)
{
  int    m;
  rvec   vcm;
  real   ekcm=0,max_vcm=0;

  for(m=0; (m<DIM); m++) {
    vcm[m]=mvcm[m]/tm;
    max_vcm=max(max_vcm,fabs(vcm[m]));
    ekcm+=vcm[m]*vcm[m];
  }
  if (max_vcm > 0.1) {
    ekcm*=0.5*tm;
    fprintf(log,"Large VCM: (%12.5f,  %12.5f,  %12.5f), ekin-cm: %12.5f\n",
	    vcm[XX],vcm[YY],vcm[ZZ],ekcm);
  }
}

void do_stoprot(FILE *log, int natoms, rvec box, rvec x[], real mass[])
{
  int i;
  static int *index=NULL;
  rvec half_box;
  static rvec *old_x=NULL;
  
  for (i=0; (i<DIM); i++)
    half_box[i]=box[i]*0.5;
  if (!index) {
    snew(index,natoms);
    for (i=0; (i<natoms); i++)
      index[i]=i;
  }
  
  /* first center on origin (do_fit does not do this!) */
  reset_x(natoms,index,natoms,index,x,mass);
  
  if (old_x) {
    /* do least squares fit to previous structure */
    do_fit(natoms,mass,old_x,x);
  } else {
    /* no previous structure, make room to copy this one */
    snew(old_x, natoms);
  }
  /* save structure for next fit and move structure back to center of box */
  for (i=0; (i<natoms); i++) {
    copy_rvec(x[i],old_x[i]);
    rvec_inc(x[i],half_box);
  }
}
