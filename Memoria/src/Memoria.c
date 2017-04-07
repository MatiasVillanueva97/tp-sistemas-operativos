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

typedef struct{
	char* PORT;
	char* MARCOS;
	char* MARCO_SIZE;
	char* ENTRADAS_CACHE ;
	char* CACHE_X_PROC;
	char* RETARDO_MEMORIA;
	char* REEMPLAZO_CACHE;
}config_Memoria;


char* getStringFromConfig(t_config *config, char*valor){
	char* aux = malloc(sizeof(char*));

	if(config_has_property(config, valor)){
		strcpy(aux,config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}

void configuracionInicial(char* PATH, config_Memoria *configMemoria){
	t_config *config;
	config = config_create(PATH);
	configMemoria->PORT = getStringFromConfig(config,"PUERTO");
	configMemoria->MARCOS = getStringFromConfig(config,"MARCOS");
	configMemoria->MARCO_SIZE = getStringFromConfig(config,"MARCO_SIZE");
	configMemoria->ENTRADAS_CACHE = getStringFromConfig(config,"ENTRADAS_CACHE");
	configMemoria->CACHE_X_PROC = getStringFromConfig(config,"CACHE_X_PROC");
	configMemoria->REEMPLAZO_CACHE=getStringFromConfig(config,"REEMPLAZO_CACHE");
	configMemoria->RETARDO_MEMORIA = getStringFromConfig(config,"RETARDO_MEMORIA");
	config_destroy(config);
}

int main(int argc, char *argv[]) {
	config_Memoria config;

	if (argc != 2) {
		    fprintf(stderr,"usage: client hostname\n");
		    exit(1);
	}

    configuracionInicial(argv[1],&config);
    puts("16 GB DE RAM"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}
