#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include <time.h>
#include <stdint.h> /* definició de intptr_t per màquines de 64 bits */
#include <unistd.h>
#include "memoria.h"

#define MAX_PALETES 9

int main (int n_args, char *ll_args[]){
  
  int f_h;

  int *ipopf, *ipopc;
  float *vpal, *p_palret, *po_pf;
  int id_ipopf, id_ipopc, id_vpal, id_popf, id_palret;
  int id_cont, id_moviments, id_tec, id_lpal, id_ret, id_fin;
  int *cont, *moviments, *tec, *l_pal, *retard, *p_fin, *palret;
  int index;
  
 /* char rh,rv,rd; */

  if (n_args < 15)
  {   fprintf(stderr,"proces: moure_paletes_ordinador  tecla  cont  moviments valorsDePaletes(5)\n");
	exit(0);
  }
  id_tec = atoi(ll_args[1]);
  tec = map_mem(id_tec);

  id_ret = atoi(ll_args[2]);
  retard = map_mem(id_ret);

  id_cont = atoi(ll_args[3]);
  cont = map_mem(id_cont);

  id_moviments = atoi(ll_args[4]);
  moviments = map_mem(id_moviments);

  id_lpal = atoi(ll_args[5]), id_ipopc = atoi(ll_args[6]), id_ipopf = atoi(ll_args[7]);
  l_pal = map_mem(id_lpal), ipopc = map_mem(id_ipopc), ipopf = map_mem(id_ipopf);

  id_vpal = atof(ll_args[9]), id_popf = atof(ll_args[8]);
  vpal = map_mem(id_vpal), po_pf = map_mem(id_popf);

  index = atoi(ll_args[10]);

  id_fin = atoi(ll_args[11]);
  p_fin = map_mem(id_fin);

  int n_fil = atoi(ll_args[12]), n_col = atoi(ll_args[13]);

  id_palret = atof(ll_args[14]);
  palret = map_mem(id_palret);
  
  win_set(p_fin, n_fil, n_col);
  do{
      f_h = po_pf[index] + vpal[index];		/* posicio hipotetica de la paleta */
      
      if (f_h != ipopf[index])	/* si pos. hipotetica no coincideix amb pos. actual */
      {
      if (vpal[index] > 0.0)			/* verificar moviment cap avall */
      {
	    if (win_quincar(f_h+(*l_pal)-1,ipopc[index]) == ' ')   /* si no hi ha obstacle */
	    {
  	    win_escricar(ipopf[index],ipopc[index],' ',NO_INV);      /* esborra primer bloc */
	      po_pf[index] = po_pf[index] + vpal[index]; ipopf[index] = po_pf[index];		/* actualitza posicio */
	      win_escricar(ipopf[index]+(*l_pal)-1,ipopc[index],'1',INVERS); /* impr. ultim bloc */
      }
	    else{		/* si hi ha obstacle, canvia el sentit del moviment */
        vpal[index] = -vpal[index];
      }
      }
      else			/* verificar moviment cap amunt */
      {
	      if (win_quincar(f_h,ipopc[index]) == ' ')        /* si no hi ha obstacle */
	      {
	        win_escricar(ipopf[index]+(*l_pal)-1,ipopc[index],' ',NO_INV); /* esbo. ultim bloc */
	        po_pf[index] = po_pf[index] + vpal[index]; ipopf[index] = po_pf[index];		/* actualitza posicio */
	        win_escricar(ipopf[index],ipopc[index],'1',INVERS);	/* impr. primer bloc */
        }
	    else{		/* si hi ha obstacle, canvia el sentit del moviment */
        vpal[index] = -vpal[index];
      }
      }
      }
      else{
        po_pf[index] = po_pf[index] + vpal[index]; 	/* actualitza posicio vertical real de la paleta */
      }
      win_retard(*retard);
  }while((*tec != TEC_RETURN) && (*cont==-1) && ((*moviments > 0) || *moviments == -1));
  //}while(1);
  return(0);
}
