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

int sema_indiceDeSemaforo(char* sem);

void sema_decrementarSEM(char* sem);

void sema_incrementarSEM(char* sem);

int sema_valorDelSemaforo(char * sem);

bool sema_existeSemaforo(char * sem);


int sema_proceso_wait(char* sem);

void sema_despertarProceso(char* sem);

void sema_proceso_signal(char* sem);


#endif /* FUNCIONESSEMAFOROS_H_ */
