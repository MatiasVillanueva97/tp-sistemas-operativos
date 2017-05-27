/*
 * funcionesMemoria.c
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#include "funcionesMemoria.h"

//// ---- FUNCIONES MEMORIA ----///


/// *** Esta función esta probada y anda
char** memoria_dividirScriptEnPaginas(int cant_paginas, char *copiaScriptAnsisop)
{
	char * scriptDivididoEnPaginas[cant_paginas];
	int i;
	for(i=0;i<cant_paginas;i++){
		scriptDivididoEnPaginas[i] = malloc(size_pagina);
		memcpy(scriptDivididoEnPaginas[i],copiaScriptAnsisop+i*size_pagina,size_pagina);
		printf("[memoria_dividirScriptEnPaginas] - %s",scriptDivididoEnPaginas[i]);
	}
	if(strlen(scriptDivididoEnPaginas[i-1]) < size_pagina){
		char* x = string_repeat(' ',size_pagina-strlen(scriptDivididoEnPaginas[i-1]));
		string_append(&(scriptDivididoEnPaginas[i-1]),x);
	}
	return scriptDivididoEnPaginas;
}

/// *** Esta función esta probada y anda
int memoria_CalcularCantidadPaginas(char * scriptAnsisop)
{
  return  ceil(((double)(strlen(scriptAnsisop))/((double) size_pagina)));
}

//// ---- FIN FUNCIONES MEMORIA ---- ////
