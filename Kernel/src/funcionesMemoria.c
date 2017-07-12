/*
 * funcionesMemoria.c
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#include "funcionesMemoria.h"

//// ---- FUNCIONES MEMORIA ----///

char** memoria_dividirScriptEnPaginas4(int *cant_paginas, char* copiaScriptAnsisop) //Devuelve el script dividio y en el int que pasas por parametro la cantidadDePaginas
{
 char ** scriptDivididoEnPaginas = malloc(sizeof(char*));
 char* pagina = malloc(size_pagina);
 char** instrucciones = string_split(copiaScriptAnsisop,"\n");
 int numeroPaginaActual = 0;
 int tamano= 0;
 int i;
 for(i=0;*(instrucciones+i);i++){
  if(strlen(*(instrucciones+i)) > size_pagina){ // Por si en algun momento te tiran una instruccion de mas del size de pagina
	   free(instrucciones);
	   free(pagina);
	   free(scriptDivididoEnPaginas);
	   return NULL;
  }
  if(tamano+strlen(*(instrucciones+i)) >size_pagina){
	   *(scriptDivididoEnPaginas+numeroPaginaActual) = malloc(size_pagina);
	   memcpy((scriptDivididoEnPaginas+numeroPaginaActual),pagina,size_pagina);
	   numeroPaginaActual++;
	   tamano = 0;
	   memcpy(pagina,*(instrucciones+i),strlen(*(instrucciones+i)));
	   memcpy(pagina+strlen(*(instrucciones+i)),"\n",sizeof(char));
  }
  else{
	   memcpy(pagina+tamano,*(instrucciones+i),strlen(*(instrucciones+i)));
	   memcpy(pagina+strlen(*(instrucciones+i))+tamano,"\n",sizeof(char));
	   tamano +=strlen(*(instrucciones+i))+1;
  }
 }
 if (tamano!= 0){
	  *(scriptDivididoEnPaginas+numeroPaginaActual) = malloc(size_pagina);
	  memcpy(*(scriptDivididoEnPaginas+numeroPaginaActual),pagina,tamano);
	  char* aux = string_repeat('\0',size_pagina-tamano);
	  memcpy((*(scriptDivididoEnPaginas+numeroPaginaActual))+tamano,aux,size_pagina-tamano);
	  free(aux);
 }
 *cant_paginas = numeroPaginaActual+1; // Empieza en 0 la variable, por eso le sumo 1
 free(pagina);
 free(instrucciones);
 return scriptDivididoEnPaginas;
}




/// *** Esta función esta probada y anda
char** memoria_dividirScriptEnPaginas(int cant_paginas, char *copiaScriptAnsisop)
{
	char ** scriptDivididoEnPaginas = malloc(sizeof(char*)*cant_paginas);
	int i;
	for(i=0;i<cant_paginas;i++){
		scriptDivididoEnPaginas[i] = malloc(size_pagina);
		memcpy(scriptDivididoEnPaginas[i],copiaScriptAnsisop+i*size_pagina,size_pagina);
		log_info(logKernel,"[memoria_dividirScriptEnPaginas] - %s",scriptDivididoEnPaginas[i]);
	}
	if(strlen(scriptDivididoEnPaginas[i-1]) < size_pagina){
		char* x = string_repeat('\0',size_pagina-strlen(scriptDivididoEnPaginas[i-1]));
		string_append(&(scriptDivididoEnPaginas[i-1]),x);
		free(x);
	}
	return scriptDivididoEnPaginas;
}

/// *** Esta función esta probada y anda
int memoria_CalcularCantidadPaginas(char * scriptAnsisop)
{
  return  ceil(((double)(strlen(scriptAnsisop))/((double) size_pagina)));
}

//// ---- FIN FUNCIONES MEMORIA ---- ////
