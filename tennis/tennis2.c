/*****************************************************************************/
/*									     */
/*				     Tennis0.c				     */
/*									     */
/*  Programa inicial d'exemple per a les practiques 2 i 3 de FSO.	     */
/*     Es tracta del joc del tennis: es dibuixa un camp de joc rectangular   */
/*     amb una porteria a cada costat, una paleta per l'usuari, una paleta   */
/*     per l'ordinador i una pilota que va rebotant per tot arreu; l'usuari  */
/*     disposa de dues tecles per controlar la seva paleta, mentre que l'or- */
/*     dinador mou la seva automaticament (amunt i avall). Evidentment, es   */
/*     tracta d'intentar col.locar la pilota a la porteria de l'ordinador    */
/*     (porteria de la dreta), abans que l'ordinador aconseguixi col.locar   */
/*     la pilota dins la porteria de l'usuari (porteria de l'esquerra).      */
/*									     */
/*  Arguments del programa:						     */
/*     per controlar la posicio de tots els elements del joc, cal indicar    */
/*     el nom d'un fitxer de text que contindra la seguent informacio:	     */
/*		n_fil n_col m_por l_pal					     */
/*		pil_pf pil_pc pil_vf pil_vc pil_ret			     */
/*		ipo_pf ipo_pc po_vf pal_ret				     */
/*									     */
/*     on 'n_fil', 'n_col' son les dimensions del taulell de joc, 'm_por'    */
/*     es la mida de les dues porteries, 'l_pal' es la longitud de les dues  */
/*     paletes; 'pil_pf', 'pil_pc' es la posicio inicial (fila,columna) de   */
/*     la pilota, mentre que 'pil_vf', 'pil_vc' es la velocitat inicial,     */
/*     pil_ret es el percentatge respecte al retard passat per paràmetre;    */
/*     finalment, 'ipo_pf', 'ipo_pc' indicara la posicio del primer caracter */
/*     de la paleta de l'ordinador, mentre que la seva velocitat vertical    */
/*     ve determinada pel parametre 'po_fv', i pal_ret el percentatge de     */
/*     retard en el moviment de la paleta de l'ordinador.		     */
/*									     */
/*     A mes, es podra afegir un segon argument opcional per indicar el      */
/*     retard de moviment de la pilota i la paleta de l'ordinador (en ms);   */
/*     el valor d'aquest parametre per defecte es 100 (1 decima de segon).   */
/*									     */
/*  Compilar i executar:					  	     */
/*     El programa invoca les funcions definides en 'winsuport.o', les       */
/*     quals proporcionen una interficie senzilla per a crear una finestra   */
/*     de text on es poden imprimir caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':				     */
/*									     */
/*	   $ gcc tennis0.c winsuport.o -o tennis0 -lcurses		     */
/*	   $ tennis0 fit_param [retard]					     */
/*									     */
/*  Codis de retorn:						  	     */
/*     El programa retorna algun dels seguents codis al SO:		     */
/*	0  ==>  funcionament normal					     */
/*	1  ==>  numero d'arguments incorrecte 				     */
/*	2  ==>  fitxer no accessible					     */
/*	3  ==>  dimensions del taulell incorrectes			     */
/*	4  ==>  parametres de la pilota incorrectes			     */
/*	5  ==>  parametres d'alguna de les paletes incorrectes		     */
/*	6  ==>  no s'ha pogut crear el camp de joc (no pot iniciar CURSES)   */
/*****************************************************************************/



#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include "winsuport.h"		/* incloure definicions de funcions propies */
#include <time.h>
#include <pthread.h>
#include <stdint.h> /* definició de intptr_t per màquines de 64 bits */
#include <unistd.h>
#include <stdbool.h>

#define MIN_FIL 7		/* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80
#define MIN_PAL 3
#define MIN_VEL -1.0
#define MAX_VEL 1.0
#define MIN_RET 0.0
#define MAX_RET 5.0
#define MAX_PALETES 9 
#define MAX_THREADS 12

/* variables globals */
pthread_t tid[MAX_THREADS];
float po_pf[MAX_PALETES];	                        /* pos. vertical de la paleta de l'ordinador, en valor real */
int ipopf[MAX_PALETES], ipopc[MAX_PALETES];      	/* posicio del la paleta de l'ordinador */
float vpal[MAX_PALETES];
float palret[MAX_PALETES];
int paletes;
int temps = 0;
bool pausa = false;

int n_fil, n_col, m_por;	/* dimensions del taulell i porteries */
int l_pal;			/* longitud de les paletes */
float v_pal; 			/* velocitat de la paleta del programa */
float pal_ret;			/* percentatge de retard de la paleta */

int ipu_pf, ipu_pc;      	/* posicio del la paleta d'usuari */
int ipo_pf, ipo_pc;      	/* posicio del la paleta de l'ordinador */

int ipil_pf, ipil_pc;		/* posicio de la pilota, en valor enter */
float pil_pf, pil_pc;		/* posicio de la pilota, en valor real */
float pil_vf, pil_vc;		/* velocitat de la pilota, en valor real*/
float pil_ret;			/* percentatge de retard de la pilota */

int retard;		/* valor del retard de moviment, en mil.lisegons */
int moviments, moviments_inicials;		/* numero max de moviments paletes per acabar el joc */

pthread_mutex_t stop = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pantalla = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t variables = PTHREAD_MUTEX_INITIALIZER;
int cont, tec;
int segs, min;

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins un fitxer de text, el nom del qual es passa per referencia en   */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris del principi del programa).		*/
void carrega_parametres(const char *nom_fit)
{
  FILE *fit;

  fit = fopen(nom_fit,"rt");		/* intenta obrir fitxer */
  if (fit == NULL)
  {	fprintf(stderr,"No s'ha pogut obrir el fitxer \'%s\'\n",nom_fit);
  	exit(2);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %d %d\n",&n_fil,&n_col,&m_por,&l_pal);
  if ((n_fil < MIN_FIL) || (n_fil > MAX_FIL) ||
	(n_col < MIN_COL) || (n_col > MAX_COL) || (m_por < 0) ||
	 (m_por > n_fil-3) || (l_pal < MIN_PAL) || (l_pal > n_fil-3))
  {
	fprintf(stderr,"Error: dimensions del camp de joc incorrectes:\n");
	fprintf(stderr,"\t%d =< n_fil (%d) =< %d\n",MIN_FIL,n_fil,MAX_FIL);
	fprintf(stderr,"\t%d =< n_col (%d) =< %d\n",MIN_COL,n_col,MAX_COL);
	fprintf(stderr,"\t0 =< m_por (%d) =< n_fil-3 (%d)\n",m_por,(n_fil-3));
	fprintf(stderr,"\t%d =< l_pal (%d) =< n_fil-3 (%d)\n",MIN_PAL,l_pal,(n_fil-3));
	fclose(fit);
	exit(3);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %f %f %f\n",&ipil_pf,&ipil_pc,&pil_vf,&pil_vc,&pil_ret);
  if ((ipil_pf < 1) || (ipil_pf > n_fil-3) ||
	(ipil_pc < 1) || (ipil_pc > n_col-2) ||
	(pil_vf < MIN_VEL) || (pil_vf > MAX_VEL) ||
	(pil_vc < MIN_VEL) || (pil_vc > MAX_VEL) ||
	(pil_ret < MIN_RET) || (pil_ret > MAX_RET))
  {
	fprintf(stderr,"Error: parametre pilota incorrectes:\n");
	fprintf(stderr,"\t1 =< ipil_pf (%d) =< n_fil-3 (%d)\n",ipil_pf,(n_fil-3));
	fprintf(stderr,"\t1 =< ipil_pc (%d) =< n_col-2 (%d)\n",ipil_pc,(n_col-2));
	fprintf(stderr,"\t%.1f =< pil_vf (%.1f) =< %.1f\n",MIN_VEL,pil_vf,MAX_VEL);
	fprintf(stderr,"\t%.1f =< pil_vc (%.1f) =< %.1f\n",MIN_VEL,pil_vc,MAX_VEL);
	fprintf(stderr,"\t%.1f =< pil_ret (%.1f) =< %.1f\n",MIN_RET,pil_ret,MAX_RET);
	fclose(fit);
	exit(4);
  }

paletes = 0;
while (!feof(fit) && paletes < MAX_PALETES){

  fscanf(fit,"%d %d %f %f\n",&ipo_pf,&ipo_pc,&v_pal,&pal_ret);
  ipopf[paletes] = ipo_pf;
  ipopc[paletes] = ipo_pc;
  vpal[paletes] = v_pal;
  palret[paletes] = pal_ret;

  if ((ipopf[paletes] < 1) || (ipopf[paletes]+l_pal > n_fil-2) ||
	(ipo_pc < 5) || (ipo_pc > n_col-2) ||
	(v_pal < MIN_VEL) || (v_pal > MAX_VEL) ||
	(pal_ret < MIN_RET) || (pal_ret > MAX_RET))
    {
	fprintf(stderr,"Error: parametres paleta ordinador incorrectes:\n");
	fprintf(stderr,"\t1 =< ipo_pf (%d) =< n_fil-l_pal-3 (%d)\n",ipo_pf,(n_fil-l_pal-3));
	fprintf(stderr,"\t5 =< ipo_pc (%d) =< n_col-2 (%d)\n",ipo_pc,(n_col-2));
	fprintf(stderr,"\t%.1f =< v_pal (%.1f) =< %.1f\n",MIN_VEL,v_pal,MAX_VEL);
	fprintf(stderr,"\t%.1f =< pal_ret (%.1f) =< %.1f\n",MIN_RET,pal_ret,MAX_RET);
	fclose(fit);
	exit(5);
    }
    paletes++;
}
  fclose(fit);			/* fitxer carregat: tot OK! */

}


/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
int inicialitza_joc(void)
{
  int i, i_port, f_port, retwin;
  char strin[51];

  retwin = win_ini(&n_fil,&n_col,'+',INVERS);   /* intenta crear taulell */

  if (retwin < 0)       /* si no pot crear l'entorn de joc amb les curses */
  { fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (retwin)
    {   case -1: fprintf(stderr,"camp de joc ja creat!\n");
                 break;
        case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n");
 		 break;
        case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n");
                 break;
        case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n");
                 break;
     }
     return(retwin);
  }

  i_port = n_fil/2 - m_por/2;	    /* crea els forats de la porteria */
  if (n_fil%2 == 0) i_port--;
  if (i_port == 0) i_port=1;
  f_port = i_port + m_por -1;
  for (i = i_port; i <= f_port; i++)
  {	pthread_mutex_lock(&pantalla);
    win_escricar(i,0,' ',NO_INV);
	win_escricar(i,n_col-1,' ',NO_INV);
  pthread_mutex_unlock(&pantalla);
  }

  ipu_pf = n_fil/2; ipu_pc = 3;		/* inicialitzar pos. paletes */
  if (ipu_pf+l_pal >= n_fil-3) ipu_pf = 1;
  for (i=0; i< l_pal; i++)	    /* dibuixar paleta inicialment */
  {	 pthread_mutex_lock(&pantalla);
    win_escricar(ipu_pf +i, ipu_pc, '0',INVERS);
    pthread_mutex_unlock(&pantalla);
	  for (int x=0; x<paletes; x++){
      pthread_mutex_lock(&pantalla);
      win_escricar(ipopf[x] +i, ipopc[x], '1',INVERS);
      pthread_mutex_unlock(&pantalla);
      pthread_mutex_lock(&variables);
      po_pf[x] = ipopf[x];
      pthread_mutex_unlock(&variables);
    }
  }

  pthread_mutex_lock(&variables);
  pil_pf = ipil_pf; pil_pc = ipil_pc;	/* fixar valor real posicio pilota */
  pthread_mutex_unlock(&variables);
  pthread_mutex_lock(&pantalla);
  win_escricar(ipil_pf, ipil_pc, '.',INVERS);	/* dibuix inicial pilota */
  pthread_mutex_unlock(&pantalla);
  return(0);
}


/* funcio per moure la pilota; retorna un valor amb alguna d'aquestes	*/
/* possibilitats:							*/
/*	-1 ==> la pilota no ha sortit del taulell			*/
/*	 0 ==> la pilota ha sortit per la porteria esquerra		*/
/*	>0 ==> la pilota ha sortit per la porteria dreta		*/
void * moure_pilota(void * cap)
{
  int f_h, c_h;
  char rh,rv,rd,pd;

do{
  pthread_mutex_lock(&stop);
  pthread_mutex_unlock(&stop);
  f_h = pil_pf + pil_vf;		/* posicio hipotetica de la pilota */
  c_h = pil_pc + pil_vc;
  cont = -1;		/* inicialment suposem que la pilota no surt */
  rh = rv = rd = pd = ' ';
  if ((f_h != ipil_pf) || (c_h != ipil_pc))
  {		/* si posicio hipotetica no coincideix amb la pos. actual */
    if (f_h != ipil_pf)		/* provar rebot vertical */
    { 
      pthread_mutex_lock(&pantalla);	
      rv = win_quincar(f_h,ipil_pc);	/* veure si hi ha algun obstacle */
      pthread_mutex_unlock(&pantalla);
	    if (rv != ' ')			/* si no hi ha res */
	        {   pil_vf = -pil_vf;		/* canvia velocitat vertical */
	        f_h = pil_pf+pil_vf;	/* actualitza posicio hipotetica */
	        }
    }
    if (c_h != ipil_pc)		/* provar rebot horitzontal */
    {	
      pthread_mutex_lock(&pantalla); 
      rh = win_quincar(ipil_pf,c_h);	/* veure si hi ha algun obstacle */
      pthread_mutex_unlock(&pantalla);
        if (rh != ' ')			/* si no hi ha res */
          {    pil_vc = -pil_vc;		/* canvia velocitat horitzontal */
              c_h = pil_pc+pil_vc;	/* actualitza posicio hipotetica */
          }
    }
    if ((f_h != ipil_pf) && (c_h != ipil_pc))	/* provar rebot diagonal */
      {	pthread_mutex_lock(&pantalla); rd = win_quincar(f_h,c_h);
      pthread_mutex_unlock(&pantalla);
        if (rd != ' ')				/* si no hi ha obstacle */
        {    pil_vf = -pil_vf; pil_vc = -pil_vc;	/* canvia velocitats */
            f_h = pil_pf+pil_vf;
            c_h = pil_pc+pil_vc;		/* actualitza posicio entera */
        }
    }
    pthread_mutex_lock(&pantalla);
    if (win_quincar(f_h,c_h) == ' ')	/* verificar posicio definitiva */
    {						/* si no hi ha obstacle */
	win_escricar(ipil_pf,ipil_pc,' ',NO_INV);	/* esborra pilota */
	pil_pf += pil_vf; pil_pc += pil_vc;
	ipil_pf = f_h; ipil_pc = c_h;		/* actualitza posicio actual */
	if ((ipil_pc > 0) && (ipil_pc <= n_col))	/* si no surt */
		win_escricar(ipil_pf,ipil_pc,'.',INVERS); /* imprimeix pilota */
	else{
    if (ipil_pc > n_col) cont = 0;
    else if(ipil_pc < 0) cont = 1;
  }
    }
    pthread_mutex_unlock(&pantalla);
  }
  else { pil_pf += pil_vf; pil_pc += pil_vc; }

  win_retard(retard);
  
}while ((tec != TEC_RETURN) && (cont==-1) && ((moviments > 0) || moviments == -1));

}


/* funcio per moure la paleta de l'usuari en funcio de la tecla premuda */
void * mou_paleta_usuari(void * cap)
{
  do{
  pthread_mutex_lock(&variables);
  tec=win_gettec();
  pthread_mutex_unlock(&variables);
  if ((tec == TEC_AVALL) && (win_quincar(ipu_pf+l_pal,ipu_pc) == ' ') && !pausa)
  {
    pthread_mutex_lock(&pantalla);
    win_escricar(ipu_pf,ipu_pc,' ',NO_INV);	   /* esborra primer bloc */
    ipu_pf++;					   /* actualitza posicio */
    win_escricar(ipu_pf+l_pal-1,ipu_pc,'0',INVERS); /* impri. ultim bloc */
    pthread_mutex_unlock(&pantalla);
    if (moviments > 0) moviments--;    /* he fet un moviment de la paleta */
  }
  if ((tec == TEC_AMUNT) && (win_quincar(ipu_pf-1,ipu_pc) == ' ') && !pausa)
  {
    pthread_mutex_lock(&pantalla);
    win_escricar(ipu_pf+l_pal-1,ipu_pc,' ',NO_INV); /* esborra ultim bloc */
    ipu_pf--;					    /* actualitza posicio */
    win_escricar(ipu_pf,ipu_pc,'0',INVERS);	    /* imprimeix primer bloc */
    pthread_mutex_unlock(&pantalla);
    if (moviments > 0) moviments--;    /* he fet un moviment de la paleta */
  }
  if (tec == TEC_ESPAI){
    //do{
    //  tec = win_gettec();
    //}while (tec != TEC_ESPAI);
    if (!pausa) pthread_mutex_lock(&stop);
    else pthread_mutex_unlock(&stop);
    pausa = !pausa;
  }
  if (tec == TEC_RETURN){
    win_fi();
    printf("Joc terminat correctament.\n");
    printf("Has jugat %d:%d minuts\n", min, segs);
    exit(0);
  }
  win_retard(retard);

  }while((tec != TEC_RETURN) && (cont==-1) && ((moviments > 0) || moviments == -1));
}


/* funcio per moure la paleta de l'ordinador autonomament, en funcio de la */
/* velocitat de la paleta (variable global v_pal) */
void * mou_paleta_ordinador(void * index)
{
  int f_h[MAX_PALETES];
 /* char rh,rv,rd; */

  int i = (intptr_t) index;
  do{
      pthread_mutex_lock(&stop);
      pthread_mutex_unlock(&stop);
      pthread_mutex_lock(&variables);
      f_h[i] = po_pf[i] + vpal[i];		/* posicio hipotetica de la paleta */
      if (f_h[i] != ipopf[i])	/* si pos. hipotetica no coincideix amb pos. actual */
      {
      if (vpal[i] > 0.0)			/* verificar moviment cap avall */
      {
	    if (win_quincar(f_h[i]+l_pal-1,ipopc[i]) == ' ')   /* si no hi ha obstacle */
	    {
        pthread_mutex_lock(&pantalla);
  	    win_escricar(ipopf[i],ipopc[i],' ',NO_INV);      /* esborra primer bloc */
	      po_pf[i] += (vpal[i]/**palret[i]*/); ipopf[i] = po_pf[i];		/* actualitza posicio */
	      win_escricar(ipopf[i]+l_pal-1,ipopc[i],'1',INVERS); /* impr. ultim bloc */
        pthread_mutex_unlock(&pantalla);
      }
	    else		/* si hi ha obstacle, canvia el sentit del moviment */
	      vpal[i] = -vpal[i];
      }
      else			/* verificar moviment cap amunt */
      {
	      if (win_quincar(f_h[i],ipopc[i]) == ' ')        /* si no hi ha obstacle */
	      {
          pthread_mutex_lock(&pantalla);
	        win_escricar(ipopf[i]+l_pal-1,ipopc[i],' ',NO_INV); /* esbo. ultim bloc */
	        po_pf[i] += (vpal[i]/**palret[i]*/); ipopf[i] = po_pf[i];		/* actualitza posicio */
	        win_escricar(ipopf[i],ipopc[i],'1',INVERS);	/* impr. primer bloc */
          pthread_mutex_unlock(&pantalla);
        }
	    else		/* si hi ha obstacle, canvia el sentit del moviment */
	      vpal[i] = -vpal[i];
      }
      }
      else{
        po_pf[i] += vpal[i]; 	/* actualitza posicio vertical real de la paleta */
      }
      pthread_mutex_unlock(&variables);
      win_retard(retard*palret[i]);

  }while((tec != TEC_RETURN) && (cont==-1) && ((moviments > 0) || moviments == -1));
  
}

void * rellotge(void * cap) {
    cont = -1;
    char missatge[100];
    while (1) {
        temps++;
        sleep(1);
        segs = temps%60;
        min = temps/60;

        if (moviments == -1){
          sprintf(missatge, "Movs: ilimitats|Temps: %d:%d", min, segs);
        pthread_mutex_lock(&pantalla);
        win_escristr(missatge);
        pthread_mutex_unlock(&pantalla);
        }else{
          sprintf(missatge, "Movs: %d restants|%d realitzats|Temps: %d:%d",
                  moviments, moviments_inicials-moviments, min, segs);
          pthread_mutex_lock(&pantalla);
          win_escristr(missatge);
          pthread_mutex_unlock(&pantalla);
        }

        if (moviments == 0 || cont != -1) {
            win_fi();
            if (cont == 1)
                printf("Ha guanyat l'ordinador!\n");
            else if (cont == 0)
                printf("Ha guanyat l'usuari!\n");
            printf("Has jugat %d:%d minuts\n", min, segs);
            exit(0);
        }
        win_retard(retard);
    }
    return NULL;
}

/* programa principal*/
int main(int n_args, const char *ll_args[])
{
  if ((n_args != 3) && (n_args !=4))
  {	fprintf(stderr,"Comanda: tennis0 fit_param moviments [retard]\n");
  	exit(1);
  }
  carrega_parametres(ll_args[1]);
  moviments=atoi(ll_args[2]);
  if (moviments == 0) moviments = -1; 
  moviments_inicials = moviments;

  if (n_args == 4) retard=atoi(ll_args[3]);
  else {
    retard=100;
  }

  if (inicialitza_joc() !=0)    // intenta crear el taulell de joc
    exit(4);                    // aborta si hi ha algun problema amb taulell

  pthread_mutex_init(&variables, NULL);     //Es crea el semafor
  pthread_mutex_init(&pantalla, NULL);     //Es crea el semafor
  pthread_mutex_init(&stop, NULL);     //Es crea el semafor

  for (int i=0; i<paletes; i++){
    pthread_create(&tid[i], NULL, mou_paleta_ordinador, (void *)(intptr_t)i);
  }
  pthread_create(&tid[paletes], NULL, mou_paleta_usuari, NULL);
  pthread_create(&tid[paletes+1], NULL, moure_pilota, NULL);
  pthread_create(&tid[paletes+2], NULL, rellotge, NULL);
  
  for (int i=0; i<paletes+3; i++){
    pthread_join(tid[i], NULL);
  }
  
  win_fi();

  pthread_mutex_destroy(&variables);    //Es destrueix el semafor
  pthread_mutex_destroy(&pantalla);    //Es destrueix el semafor
  pthread_mutex_destroy(&stop);
  return(0);
}
