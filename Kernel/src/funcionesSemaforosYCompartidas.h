/*
 * funcionesSemaforos.h
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include "commons/collections/list.h"
#include "commons/collections/queue.h"

#include "datosGlobales.h"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

#ifndef FUNCIONESSEMAFOROS_H_
#define FUNCIONESSEMAFOROS_H_

typedef struct{
	char* nombre;
	int valor;
}t_variableGlobal;

t_list* listaDeVariablesGlobales;

typedef struct{
	char* nombre;
	int valor;
	t_queue * cola;
}t_semaforo;

t_list * listaDeSemaforos;

//funciones de variables compartidas
void cargarVariablesGlobalesDesdeConfig();

t_variableGlobal* buscarVariableGlobal(char* nombreVarGlob);

//Hacen lo que dice su nombre
void cargarSemaforosDesdeConfig();

bool SEM_wait(char* nombreSEM, PCB_DATA * pcb);

bool SEM_signal(char* nombreSEM, PCB_DATA* pcb);

#endif /* FUNCIONESSEMAFOROS_H_ */
