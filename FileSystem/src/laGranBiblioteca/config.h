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
