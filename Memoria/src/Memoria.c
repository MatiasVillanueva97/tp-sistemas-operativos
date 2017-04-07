/*
 ============================================================================
 Name        : Memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "commons/config.h"

char* getStringFromConfig(t_config *config, char*valor){
	char* aux = malloc(sizeof(char*));

	if(config_has_property(config, valor)){
		strcpy(aux,config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}

void configuracionInicial(char*PATH,char**PORT,char**MARCOS,char**MARCO_SIZE,char**ENTRADAS_CACHE,char**CACHE_X_PROC,char**RETARDO_MEMORIA,char**REEMPLAZO_CACHE){
	t_config *config;
	config = config_create(PATH);
	*PORT = getStringFromConfig(config,"PUERTO");
	*MARCOS = getStringFromConfig(config,"MARCOS");
	*MARCO_SIZE = getStringFromConfig(config,"MARCO_SIZE");
	*ENTRADAS_CACHE = getStringFromConfig(config,"ENTRADAS_CACHE");
	*CACHE_X_PROC = getStringFromConfig(config,"CACHE_X_PROC");
	*REEMPLAZO_CACHE=getStringFromConfig(config,"REEMPLAZO_CACHE");
	*RETARDO_MEMORIA = getStringFromConfig(config,"RETARDO_MEMORIA");
	config_destroy(config);
}

int main(int argc, char *argv[]) {
    char* PORT,MARCOS,MARCO_SIZE,ENTRADAS_CACHE ,CACHE_X_PROC,RETARDO_MEMORIA,REEMPLAZO_CACHE;
    configuracionInicial(argv[1],PORT,MARCOS,MARCO_SIZE,ENTRADAS_CACHE ,CACHE_X_PROC,RETARDO_MEMORIA,REEMPLAZO_CACHE);
	puts("16 GB DE RAM"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}
