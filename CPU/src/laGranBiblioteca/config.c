/*
 * config.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */
#include "commons/config.h"
#include "commons/string.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
char * getStringFromConfig(t_config * config, char*valor){
	char* aux;

	if(config_has_property(config, valor)){
		aux = string_duplicate(config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");
	return aux;
}
/*char* getStringFromConfig(t_config *config, char*valor){
	char* aux =  malloc(strlen(config_get_string_value(config, valor)));
	if(config_has_property(config, valor)){
		strcpy(aux,config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}*/
void imprimirArraysDeStrings(char** lista)
{
	printf("[ %s",*lista);
	while(*(lista+1) != NULL) //Mostrar a gabi; //lenguaje de mierda.
	{
		lista++;
		printf(", %s ", *lista);
	}
	printf(" ]\n");
}

void liberarArray(char** array){
	int i= 0;
	while(array[i]!= NULL){
		free(array[i]);
		i++;
	}
	free(array);
}


//   ************************* Funciones para la configuracion del Kernel   ****************************

typedef struct {
	char * PUERTO_PROG;
	char * PUERTO_CPU;
	char * IP_MEMORIA;
	char * PUERTO_MEMORIA;
	char * IP_FS;
	char * PUERTO_FS;
	char * QUANTUM;
	char * QUANTUM_SLEEP;
	char * ALGORITMO;
	char * GRADO_MULTIPROG;
	char ** SEM_IDS;
	char ** SEM_INIT;
	char ** SHARED_VARS;
	char * STACK_SIZE;
}config_Kernel;

void configuracionInicialKernel(char*PATH, config_Kernel*est){
	t_config * config;
	config = config_create(PATH);
	est->PUERTO_PROG = getStringFromConfig(config,"PUERTO_PROG");
	est->PUERTO_CPU = getStringFromConfig(config,"PUERTO_CPU");
	est->IP_MEMORIA = getStringFromConfig(config,"IP_MEMORIA");
	est->PUERTO_MEMORIA = getStringFromConfig(config,"PUERTO_MEMORIA");
	est->IP_FS = getStringFromConfig(config,"IP_FS");
	est->PUERTO_FS = getStringFromConfig(config,"PUERTO_FS");
	est->QUANTUM = getStringFromConfig(config,"QUANTUM");
	est->QUANTUM_SLEEP = getStringFromConfig(config,"QUANTUM_SLEEP");
	est->ALGORITMO = getStringFromConfig(config,"ALGORITMO");
	est->GRADO_MULTIPROG = getStringFromConfig(config,"GRADO_MULTIPROG");
	est->SEM_IDS = config_get_array_value(config,"SEM_IDS");
	est->SEM_INIT = config_get_array_value(config,"SEM_INIT");
	est->SHARED_VARS = config_get_array_value(config,"SHARED_VARS");
	est->STACK_SIZE = getStringFromConfig(config,"STACK_SIZE");

	config_destroy(config);
}
void liberarConfiguracionKernel(config_Kernel* est){
		free(est->PUERTO_PROG);
		free(est->PUERTO_CPU);
		free(est->IP_MEMORIA);
		free(est->PUERTO_MEMORIA);
		free(est->IP_FS);
		free(est->PUERTO_FS);
		free(est->QUANTUM);
		free(est->QUANTUM_SLEEP);
		free(est->ALGORITMO);
		free(est->GRADO_MULTIPROG);
		liberarArray(est->SEM_IDS);
		liberarArray(est->SEM_INIT);
		liberarArray(est->SHARED_VARS);
		free(est->STACK_SIZE);
}
void imprimirConfiguracionInicialKernel(config_Kernel config) // Yo gabriel maiori, dije explicitamente que esto es una terrible NEGRADA, pero como yo soy el tosco del team, no puedo quejarme
{
	printf("PUERTO_PROG: %s \n", config.PUERTO_PROG);
	printf("PUERTO_CPU: %s \n", config.PUERTO_CPU);
	printf("IP_MEMORIA: %s \n", config.IP_MEMORIA);
	printf("PUERTO_MEMORIA: %s \n", config.PUERTO_MEMORIA);
	printf("IP_FS: %s \n", config.IP_FS);
	printf("PUERTO_FS: %s \n", config.PUERTO_FS);
	printf("QUANTUM: %s \n", config.QUANTUM);
	printf("QUANTUM_SLEEP: %s \n", config.QUANTUM_SLEEP);
	printf("ALGORITMO: %s\n", config.ALGORITMO);
	printf("GRADO_MULTIPROG: %s \n", config.GRADO_MULTIPROG);


	printf("SEM_IDS: ");

	imprimirArraysDeStrings(config.SEM_IDS);
	printf("SEM_INIT: ");

	imprimirArraysDeStrings(config.SEM_INIT);
	printf("SHARED_VARS: ");

	imprimirArraysDeStrings(config.SHARED_VARS);


	printf("STACK_SIZE: %s \n", config.STACK_SIZE);
}

//   ************************* Funciones para la configuracion de la Consola   ****************************

typedef struct{
	char *PORT;
	char *IP;
}config_Consola;

void configuracionInicialConsola(char*PATH,config_Consola * configConsola){
	t_config *config;
	config = config_create(PATH);
	configConsola->PORT = getStringFromConfig(config,"PUERTO_KERNEL");
	configConsola->IP = getStringFromConfig(config,"IP_KERNEL");
	config_destroy(config);
}

void imprimirConfiguracionInicialConsola(config_Consola config){

	printf("IP_KERNEL: %s\n", config.IP);
	printf("PUERTO_KERNEL: %s \n", config.PORT);

}

void liberarConfiguracionConsola(config_Consola * configConsola){
	free(configConsola->IP);
	free(configConsola->PORT);
}

//   ************************* Funciones para la configuracion del File System   ****************************

typedef struct{
	char * PORT;
	char * PUNTO_MONTAJE;
}config_FileSystem;

void configuracionInicialFileSystem(char*PATH, config_FileSystem* conFs){
	t_config * config;
	config = config_create(PATH);

	conFs->PORT = getStringFromConfig(config,"PUERTO");
	//char** aux = config_get_array_value(config,"PUNTO_MONTAJE");//cuando arreglemos el error hay que borrar esta linea
	conFs->PUNTO_MONTAJE = getStringFromConfig(config,"PUNTO_MONTAJE");//esta tambien
	//liberarArray(aux);//esta tambien


	config_destroy(config);
}

void imprimirConfiguracionInicialFileSystem(config_FileSystem config){
	printf("PORT: %s\n", config.PORT);
	printf("PUNTO_MONTAJE: %s \n", config.PUNTO_MONTAJE);

}

void liberarConfiguracionFileSystem(config_FileSystem * conFs){
	free(conFs->PORT);
	free(conFs->PUNTO_MONTAJE);
}

//   ************************* Funciones para la configuracion de la Memoria   ****************************

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
void liberarConfiguracionMemoria(config_Memoria * configMemoria){
	free(configMemoria->PORT);
	free(configMemoria->MARCOS);
	free(configMemoria->MARCO_SIZE);
	free(configMemoria->ENTRADAS_CACHE);
	free(configMemoria->CACHE_X_PROC);
	free(configMemoria->REEMPLAZO_CACHE);
	free(configMemoria->RETARDO_MEMORIA);
	free(configMemoria->IP);
}

//   ************************* Funciones para la configuracion de la CPU   ****************************

typedef struct{
	char *IP_KERNEL;
	char *PORT_KERNEL;
	char *IP_MEMORIA;
	char *PORT_MEMORIA;

}config_CPU;

void configuracionInicialCPU(char*PATH,config_CPU * configCPU){
	t_config * config;
	config = config_create(PATH);
	configCPU->PORT_KERNEL = getStringFromConfig(config,"PUERTO_KERNEL");
	configCPU->IP_KERNEL = getStringFromConfig(config,"IP_KERNEL");
	configCPU->PORT_MEMORIA = getStringFromConfig(config,"PUERTO_MEMORIA");
	configCPU->IP_MEMORIA = getStringFromConfig(config,"IP_MEMORIA");
	config_destroy(config);
}

void imprimirConfiguracionInicialCPU(config_CPU config){

	printf("IP_KERNEL: %s\n", config.IP_KERNEL);
	printf("PORT_KERNEL: %s \n", config.PORT_KERNEL);
	printf("IP_MEMORIA: %s\n", config.IP_MEMORIA);
	printf("PORT_MEMORIA: %s \n", config.PORT_MEMORIA);

}
void liberarConfiguracionCPU(config_CPU * configCPU){
	free(configCPU->PORT_KERNEL);
	free(configCPU->IP_KERNEL);
	free(configCPU->PORT_MEMORIA);
	free(configCPU->IP_MEMORIA);
}
