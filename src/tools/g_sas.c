/*
 * $Id$
 * 
 *                This source code is part of
 * 
 *                 G   R   O   M   A   C   S
 * 
 *          GROningen MAchine for Chemical Simulations
 * 
 *                        VERSION 3.2.0
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2004, The GROMACS development team,
 * check out http://www.gromacs.org for more information.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 * 
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 * 
 * For more info, check our website at http://www.gromacs.org
 * 
 * And Hey:
 * Green Red Orange Magenta Azure Cyan Skyblue
 */
#include <math.h>
#include <stdlib.h>
#include "sysstuff.h"
#include "string.h"
#include "typedefs.h"
#include "smalloc.h"
#include "macros.h"
#include "vec.h"
#include "xvgr.h"
#include "pbc.h"
#include "copyrite.h"
#include "futil.h"
#include "statutil.h"
#include "index.h"
#include "nsc.h"
#include "pdbio.h"
#include "confio.h"
#include "rmpbc.h"
#include "names.h"
#include "atomprop.h"

typedef struct {
  atom_id  aa,ab;
  real     d2a,d2b;
} t_conect;

void add_rec(t_conect c[],atom_id i,atom_id j,real d2)
{
  if (c[i].aa == NO_ATID) {
    c[i].aa  = j;
    c[i].d2a = d2;
  }
  else if (c[i].ab == NO_ATID) {
    c[i].ab  = j;
    c[i].d2b = d2;
  }
  else if (d2 < c[i].d2a) {
    c[i].aa  = j;
    c[i].d2a = d2;
  }
  else if (d2 < c[i].d2b) {
    c[i].ab  = j;
    c[i].d2b = d2;
  }
  /* Swap them if necessary: a must be larger than b */
  if (c[i].d2a < c[i].d2b) {
    j        = c[i].ab;
    c[i].ab  = c[i].aa;
    c[i].aa  = j;
    d2       = c[i].d2b;
    c[i].d2b = c[i].d2a;
    c[i].d2a = d2;
  }
}

void do_conect(char *fn,int n,rvec x[])
{
  FILE     *fp;
  int      i,j;
  t_conect *c;
  rvec     dx;
  real     d2;
  
  fprintf(stderr,"Building CONECT records\n");
  snew(c,n);
  for(i=0; (i<n); i++) 
    c[i].aa = c[i].ab = NO_ATID;
  
  for(i=0; (i<n); i++) {
    for(j=i+1; (j<n); j++) {
      rvec_sub(x[i],x[j],dx);
      d2 = iprod(dx,dx);
      add_rec(c,i,j,d2);
      add_rec(c,j,i,d2);
    }
  }
  fp = ffopen(fn,"a");
  for(i=0; (i<n); i++) {
    if ((c[i].aa == NO_ATID) || (c[i].ab == NO_ATID))
      fprintf(stderr,"Warning dot %d has no conections\n",i+1);
    fprintf(fp,"CONECT%5d%5d%5d\n",i+1,c[i].aa+1,c[i].ab+1);
  }
  ffclose(fp);
  sfree(c);
}

void connelly_plot(char *fn,int ndots,real dots[],rvec x[],t_atoms *atoms,
		   t_symtab *symtab,matrix box,bool bSave)
{
  static char *atomnm="DOT";
  static char *resnm ="DOT";
  static char *title ="Connely Dot Surface Generated by g_sas";

  int  i,i0,r0,ii0,k;
  rvec *xnew;
  t_atoms aaa;

  if (bSave) {  
    i0 = atoms->nr;
    r0 = atoms->nres;
    srenew(atoms->atom,atoms->nr+ndots);
    srenew(atoms->atomname,atoms->nr+ndots);
    srenew(atoms->resname,r0+1);
    atoms->resname[r0]  = put_symtab(symtab,resnm);
    srenew(atoms->pdbinfo,atoms->nr+ndots);
    snew(xnew,atoms->nr+ndots);
    for(i=0; (i<atoms->nr); i++)
      copy_rvec(x[i],xnew[i]);
    for(i=k=0; (i<ndots); i++) {
      ii0 = i0+i;
      atoms->atomname[ii0] = put_symtab(symtab,atomnm);
      sprintf(atoms->pdbinfo[ii0].pdbresnr,"%d",r0+1);
      atoms->pdbinfo[ii0].type = epdbATOM;
      atoms->atom[ii0].chain = ' ';
      atoms->pdbinfo[ii0].atomnr= ii0;
      atoms->atom[ii0].resnr = r0;
      xnew[ii0][XX] = dots[k++];
      xnew[ii0][YY] = dots[k++];
      xnew[ii0][ZZ] = dots[k++];
      atoms->pdbinfo[ii0].bfac  = 0.0;
      atoms->pdbinfo[ii0].occup = 0.0;
    }
    atoms->nr   = i0+ndots;
    atoms->nres = r0+1;
    write_sto_conf(fn,title,atoms,xnew,NULL,box);
    atoms->nres = r0;
    atoms->nr   = i0;
  }
  else {
    init_t_atoms(&aaa,ndots,TRUE);
    snew(xnew,ndots);
    for(i=k=0; (i<ndots); i++) {
      ii0 = i;
      aaa.resname[ii0]  = put_symtab(symtab,resnm);
      aaa.atomname[ii0] = put_symtab(symtab,atomnm);
      strcpy(aaa.pdbinfo[ii0].pdbresnr,"1");
      aaa.pdbinfo[ii0].type = epdbATOM;
      aaa.atom[ii0].chain = ' ';
      aaa.pdbinfo[ii0].atomnr= ii0;
      aaa.atom[ii0].resnr = 0;
      xnew[ii0][XX] = dots[k++];
      xnew[ii0][YY] = dots[k++];
      xnew[ii0][ZZ] = dots[k++];
      aaa.pdbinfo[ii0].bfac  = 0.0;
      aaa.pdbinfo[ii0].occup = 0.0;
    }
    aaa.nr = ndots;
    write_sto_conf(fn,title,&aaa,xnew,NULL,box);
    do_conect(fn,ndots,xnew);
    free_t_atoms(&aaa);
  }
  sfree(xnew);
}

real calc_radius(char *atom)
{
  real r;
  
  switch (atom[0]) {
  case 'C':
    r = 0.16;
    break;
  case 'O':
    r = 0.13;
    break;
  case 'N':
    r = 0.14;
    break;
  case 'S':
    r = 0.2;
    break;
  case 'H':
    r = 0.1;
    break;
  default:
    r = 1e-3;
  }
  return r;
}

void sas_plot(int nfile,t_filenm fnm[],real solsize,int ndots,
	      real qcut,bool bSave,real minarea,bool bPBC,
	      real dgs_default,bool bFindex)
{
  FILE         *fp,*fp2,*fp3=NULL;
  char         *legend[] = { "Hydrophobic", "Hydrophilic", 
			     "Total", "D Gsolv" };
  real         t;
  void         *atomprop=NULL;
  int          status,ndefault;
  int          i,j,ii,nfr,natoms,flag,nsurfacedots,res;
  rvec         *x;
  matrix       box;
  t_topology   *top;
  t_atoms      *atoms;
  bool         *bPhobic;
  bool         bConnelly;
  bool         bResAt,bITP,bDGsol;
  real         *radius,*dgs_factor=NULL,*area=NULL,*surfacedots=NULL;
  real         at_area,*atom_area=NULL,*atom_area2=NULL;
  real         *res_a=NULL,*res_area=NULL,*res_area2=NULL;
  real         totarea,totvolume,harea,tarea,fluc2;
  atom_id      *index,*findex;
  int          nx,nphobic,npcheck;
  char         *grpname,*fgrpname;
  real         dgsolv;

  bITP   = opt2bSet("-i",nfile,fnm);
  bResAt = opt2bSet("-or",nfile,fnm) || opt2bSet("-oa",nfile,fnm) || bITP;

  if ((natoms=read_first_x(&status,ftp2fn(efTRX,nfile,fnm),
			   &t,&x,box))==0)
    fatal_error(0,"Could not read coordinates from statusfile\n");

  top   = read_top(ftp2fn(efTPX,nfile,fnm));
  atoms = &(top->atoms);
  bDGsol = strcmp(*(atoms->atomtype[0]),"?") != 0;
  if (!bDGsol) 
    fprintf(stderr,"Warning: your tpr file is too old, will not compute "
	    "Delta G of solvation\n");
  atomprop = get_atomprop();
  
  fprintf(stderr,"Select group for calculation of surface and for output:\n");
  get_index(&(top->atoms),ftp2fn_null(efNDX,nfile,fnm),1,&nx,&index,&grpname);

  if (bFindex) {
    fprintf(stderr,"Select group of hydrophobic atoms:\n");
    get_index(&(top->atoms),ftp2fn_null(efNDX,nfile,fnm),1,&nphobic,&findex,&fgrpname);
  }
  
  /* Now compute atomic readii including solvent probe size */
  snew(radius,natoms);
  snew(bPhobic,nx);
  if (bResAt) {
    snew(atom_area,nx);
    snew(atom_area2,nx);
    snew(res_a,atoms->nres);
    snew(res_area,atoms->nres);
    snew(res_area2,atoms->nres);
  }
  if (bDGsol)
    snew(dgs_factor,nx);

  /* Get a Van der Waals radius for each atom */
  ndefault = 0;
  for(i=0; (i<natoms); i++) {
    if (!query_atomprop(atomprop,epropVDW,
			*(top->atoms.resname[top->atoms.atom[i].resnr]),
			*(top->atoms.atomname[i]),&radius[i]))
      ndefault++;
    /* radius[i] = calc_radius(*(top->atoms.atomname[i])); */
    radius[i] += solsize;
  }
  if (ndefault > 0)
    fprintf(stderr,"WARNING: could not find a Van der Waals radius for %d atoms\n",ndefault);
  /* Determine which atom is counted as hydrophobic */
  if (bFindex) {
    npcheck = 0;
    for(i=0; (i<nx); i++) {
      ii = index[i];
      for(j=0; (j<nphobic); j++) {
	if (findex[j] == ii) {
	  bPhobic[i] = TRUE;
	  npcheck++;
	}
      }
    }
    if (npcheck != nphobic)
      fatal_error(0,"Consistency check failed: not all %d atoms in the hydrophobic index\n"
		  "found in the normal index selection (%d atoms)",nphobic,npcheck);
  }
  else
    nphobic = 0;
    
  for(i=0; (i<nx); i++) {
    ii = index[i];
    if (!bFindex) {
      bPhobic[i] = fabs(atoms->atom[ii].q) <= qcut;
      if (bPhobic[i])
	nphobic++;
    }
    if (bDGsol)
      if (!query_atomprop(atomprop,epropDGsol,
			  *(atoms->resname[atoms->atom[ii].resnr]),
			  *(atoms->atomtype[ii]),&(dgs_factor[i])))
	dgs_factor[i] = dgs_default;
    if (debug)
      fprintf(debug,"Atom %5d %5s-%5s: q= %6.3f, r= %6.3f, dgsol= %6.3f, hydrophobic= %s\n",
	      ii+1,*(atoms->resname[atoms->atom[ii].resnr]),
	      *(atoms->atomname[ii]),
	      atoms->atom[ii].q,radius[ii]-solsize,dgs_factor[i],
	      BOOL(bPhobic[i]));
  }
  fprintf(stderr,"%d out of %d atoms were classified as hydrophobic\n",
	  nphobic,nx);
  
  done_atomprop(&atomprop);
  
  fp=xvgropen(opt2fn("-o",nfile,fnm),"Solvent Accessible Surface","Time (ps)",
	      "Area (nm\\S2\\N)");
  xvgr_legend(fp,asize(legend) - (bDGsol ? 0 : 1),legend);
  
  nfr=0;
  do {
    if (bPBC)
      rm_pbc(&top->idef,natoms,box,x,x);
    
    bConnelly = (nfr==0 && opt2bSet("-q",nfile,fnm));
    if (bConnelly)
      flag = FLAG_ATOM_AREA | FLAG_DOTS;
    else
      flag = FLAG_ATOM_AREA;
    
    if (debug)
      write_sto_conf("check.pdb","pbc check",atoms,x,NULL,box);

    if (nsc_dclm2(x,radius,nx,index,ndots,flag,&totarea,
		  &area,&totvolume,&surfacedots,&nsurfacedots,
		  bPBC ? box : NULL))
      fatal_error(0,"Something wrong in nsc_dclm2");
    
    if (bConnelly)
      connelly_plot(ftp2fn(efPDB,nfile,fnm),
		    nsurfacedots,surfacedots,x,atoms,
		    &(top->symtab),box,bSave);
    
    harea  = 0; 
    tarea  = 0;
    dgsolv = 0;
    if (bResAt)
      for(i=0; i<atoms->nres; i++)
	res_a[i] = 0;
    for(i=0; (i<nx); i++) {
      ii = index[i];
      at_area = area[i];
      if (bResAt) {
	atom_area[i] += at_area;
	atom_area2[i] += sqr(at_area);
	res_a[atoms->atom[ii].resnr] += at_area;
      }
      tarea += at_area;
      if (bDGsol)
	dgsolv += at_area*dgs_factor[i];
      if (bPhobic[i])
	harea += at_area;
    }
    if (bResAt)
      for(i=0; i<atoms->nres; i++) {
	res_area[i] += res_a[i];
	res_area2[i] += sqr(res_a[i]);
      }
    fprintf(fp,"%10g  %10g  %10g  %10g",t,harea,tarea-harea,tarea);
    if (bDGsol)
      fprintf(fp,"  %10g\n",dgsolv);
    else
      fprintf(fp,"\n");
    
    if (area) {
      sfree(area);
      area = NULL;
    }
    if (surfacedots) {
      sfree(surfacedots);
      surfacedots = NULL;
    }
    nfr++;
  } while (read_next_x(status,&t,natoms,x,box));
  
  fprintf(stderr,"\n");
  close_trj(status);
  fclose(fp);
  
  /* if necessary, print areas per atom to file too: */
  if (bResAt) {
    for(i=0; i<atoms->nres; i++) {
      res_area[i] /= nfr;
      res_area2[i] /= nfr;
    }
    for(i=0; i<nx; i++) {
      atom_area[i] /= nfr;
      atom_area2[i] /= nfr;
    }
    fprintf(stderr,"Printing out areas per atom\n");
    fp  = xvgropen(opt2fn("-or",nfile,fnm),"Area per residue","Residue",
		   "Area (nm\\S2\\N)");
    fp2 = xvgropen(opt2fn("-oa",nfile,fnm),"Area per atom","Atom #",
		   "Area (nm\\S2\\N)");
    if (bITP) {
      fp3 = ftp2FILE(efITP,nfile,fnm,"w");
      fprintf(fp3,"[ position_restraints ]\n"
	      "#define FCX 1000\n"
	      "#define FCY 1000\n"
	      "#define FCZ 1000\n"
	      "; Atom  Type  fx   fy   fz\n");
    }
    for(i=0; i<nx; i++) {
      ii = index[i];
      res = atoms->atom[ii].resnr;
      if (i==nx-1 || res!=atoms->atom[index[i+1]].resnr) {
	fluc2 = res_area2[res]-sqr(res_area[res]);
	if (fluc2 < 0)
	  fluc2 = 0;
	fprintf(fp,"%10d  %10g %10g\n",res+1,res_area[res],sqrt(fluc2));
      }
      fluc2 = atom_area2[i]-sqr(atom_area[i]);
      if (fluc2 < 0)
	fluc2 = 0;
      fprintf(fp2,"%d %g %g\n",index[i]+1,atom_area[i],sqrt(fluc2));
      if (bITP && (atom_area[i] > minarea))
	fprintf(fp3,"%5d   1     FCX  FCX  FCZ\n",ii+1);
    }
    if (bITP)
      fclose(fp3);
    fclose(fp);
  }

  sfree(x);
}

int gmx_sas(int argc,char *argv[])
{
  static char *desc[] = {
    "g_sas computes hydrophobic, hydrophilic and total solvent accessible surface area.",
    "As a side effect the Connolly surface can be generated as well in",
    "a pdb file where the nodes are represented as atoms and the vertices",
    "connecting the nearest nodes as CONECT records. The area can be plotted",
    "per residue and atom as well (options [TT]-or[tt] and [TT]-oa[tt]).",
    "In combination with the latter option an [TT]itp[tt] file can be",
    "generated (option [TT]-i[tt])",
    "which can be used to restrain surface atoms.[PAR]",
    "By default, periodic boundary conditions are taken into account,",
    "this can be turned off using the [TT]-pbc[tt] option."
  };

  static real solsize = 0.14;
  static int  ndots   = 24;
  static real qcut    = 0.2;
  static real minarea = 0.5, dgs_default=0;
  static bool bSave   = TRUE,bPBC=TRUE,bFindex=FALSE;
  t_pargs pa[] = {
    { "-solsize", FALSE, etREAL, {&solsize},
      "Radius of the solvent probe (nm)" },
    { "-ndots",   FALSE, etINT,  {&ndots},
      "Number of dots per sphere, more dots means more accuracy" },
    { "-qmax",    FALSE, etREAL, {&qcut},
      "The maximum charge (e, absolute value) of a hydrophobic atom" },
    { "-f_index", FALSE, etBOOL, {&bFindex},
      "Determine from a group in the index file what are the hydrophobic atoms rather than from the charge" },
    { "-minarea", FALSE, etREAL, {&minarea},
      "The minimum area (nm^2) to count an atom as a surface atom when writing a position restraint file  (see help)" },
    { "-pbc",     FALSE, etBOOL, {&bPBC},
      "Take periodicity into account" },
    { "-prot",    FALSE, etBOOL, {&bSave},
      "Output the protein to the connelly pdb file too" },
    { "-dgs",     FALSE, etREAL, {&dgs_default},
      "default value for solvation free energy per area (kJ/mol/nm^2)" }
  };
  t_filenm  fnm[] = {
    { efTRX, "-f",   NULL,       ffREAD },
    { efTPX, "-s",   NULL,       ffREAD },
    { efXVG, "-o",   "area",     ffWRITE },
    { efXVG, "-or",  "resarea",  ffOPTWR },
    { efXVG, "-oa",  "atomarea", ffOPTWR },
    { efPDB, "-q",   "connelly", ffOPTWR },
    { efNDX, "-n",   "index",    ffOPTRD },
    { efITP, "-i",   "surfat",   ffOPTWR }
  };
#define NFILE asize(fnm)

  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,PCA_CAN_VIEW | PCA_CAN_TIME | PCA_BE_NICE,
		    NFILE,fnm,asize(pa),pa,asize(desc),desc,0,NULL);
  if (solsize <= 0) {
    solsize=1e-3;
    fprintf(stderr,"Solsize too small, setting it to %g\n",solsize);
  }
  if (ndots < 20) {
    ndots = 20;
    fprintf(stderr,"Ndots too small, setting it to %d\n",ndots);
  }

  please_cite(stderr,"Eisenhaber95");
    
  sas_plot(NFILE,fnm,solsize,ndots,qcut,bSave,minarea,bPBC,dgs_default,bFindex);
  
  do_view(opt2fn("-o",NFILE,fnm),NULL);
  do_view(opt2fn_null("-or",NFILE,fnm),NULL);
  do_view(opt2fn_null("-oa",NFILE,fnm),NULL);

  thanx(stderr);
  
  return 0;
}
