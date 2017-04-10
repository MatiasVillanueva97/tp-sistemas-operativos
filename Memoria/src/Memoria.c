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
	char* IP; // Lo agrego yo maiori, y le dejo la ip del localhost
}config_Memoria;

void configuracionInicialMemoria(char* PATH, config_Memoria *configMemoria){
	t_config *config;
	config = config_create(PATH);
	configMemoria->PORT = getStringFromConfig(config,"PUERTO");
	configMemoria->MARCOS = getStringFromConfig(config,"MARCOS");
	configMemoria->MARCO_SIZE = getStringFromConfig(config,"MARCO_SIZE");
	configMemoria->ENTRADAS_CACHE = getStringFromConfig(config,"ENTRADAS_CACHE");
	configMemoria->CACHE_X_PROC = getStringFromConfig(config,"CACHE_X_PROC");
	configMemoria->REEMPLAZO_CACHE=getStringFromConfig(config,"REEMPLAZO_CACHE");
	configMemoria->RETARDO_MEMORIA = getStringFromConfig(config,"RETARDO_MEMORIA");
	configMemoria->IP = getStringFromConfig(config,"IP");
	config_destroy(config);
}
void imprimirConfiguracionInicialMemoria(config_Memoria config){

	printf("PUERTO: %s\n", config.PORT);
	printf("MARCOS: %s \n", config.MARCOS);
	printf("MARCO_SIZE: %s\n", config.MARCO_SIZE);
	printf("ENTRADAS_CACHE: %s\n", config.ENTRADAS_CACHE);
	printf("CACHE_X_PROC: %s\n", config.CACHE_X_PROC);
	printf("REEMPLAZO_CACHE: %s\n", config.REEMPLAZO_CACHE);
	printf("RETARDO_MEMORIA: %s\n", config.RETARDO_MEMORIA);
	printf("IP: %s\n", config.IP);
}

int main(int argc, char *argv[]) {
	printf("Inicializando Memoria.....\n\n");

	config_Memoria config;

	/*if (argc != 2) {
		    fprintf(stderr,"usage: client hostname\n");
		    exit(1);
	}*/

 	printf("Configuracion Inicial: \n");

    configuracionInicialMemoria("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config",&config);
    imprimirConfiguracionInicialMemoria(config);

    printf("\n\nAca hará lo que la memoria haga: \n");

    return EXIT_SUCCESS;
}
