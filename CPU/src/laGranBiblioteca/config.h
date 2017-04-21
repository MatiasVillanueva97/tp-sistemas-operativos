/*
 * config.h
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "commons/config.h"

char* getStringFromConfig(t_config*, char*);

#endif /* CONFIG_H_ */


// Para el kernel

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

// esta funcion imprime la configuracion inicial
void imprimirConfiguracionInicialKernel(config_Kernel); // Yo gabriel maiori, dije explicitamente que esto es una terrible NEGRADA, pero como yo soy el tosco del team, no puedo quejarme

// esta funcion setea en una estructura ttodo lo que provenga por el archivo config
void configuracionInicialKernel(char*,config_Kernel*);

void liberarConfiguracionKernel(config_Kernel* );


// Para la Consola

typedef struct{
	char *PORT;
	char *IP;
}config_Consola;

void configuracionInicialConsola(char*,config_Consola *);

void imprimirConfiguracionInicialConsola(config_Consola);

void liberarConfiguracionConsola(config_Consola *);
//Para el File System

typedef struct{
	char * PORT;
	char * PUNTO_MONTAJE;
}config_FileSystem;

void configuracionInicialFileSystem(char*, config_FileSystem*);

void imprimirConfiguracionInicialFileSystem(config_FileSystem);

void liberarConfiguracionFileSystem(config_FileSystem * conFs);
//Para la Memoria

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

void configuracionInicialMemoria(char*, config_Memoria *);

void imprimirConfiguracionInicialMemoria(config_Memoria);

void liberarConfiguracionMemoria(config_Memoria *);

//Para la CPU

typedef struct{
	char *IP_KERNEL;
	char *PORT_KERNEL;
	char *IP_MEMORIA;
	char *PORT_MEMORIA;

}config_CPU;

void configuracionInicialCPU(char*PATH,config_CPU *);

void imprimirConfiguracionInicialCPU(config_CPU);

void liberarConfiguracionCPU(config_CPU *);
