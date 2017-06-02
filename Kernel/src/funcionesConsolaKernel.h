/*
 * funcionesConsolaKernel.h
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "commons/collections/list.h"
#include "commons/collections/queue.h"

#include "datosGlobales.h"

#ifndef FUNCIONESCONSOLAKERNEL_H_
#define FUNCIONESCONSOLAKERNEL_H_

void imprimirTodosLosProcesosEnColas();

void imprimirProcesosdeCola(t_queue* unaCola);

void * consolaKernel();


#endif /* FUNCIONESCONSOLAKERNEL_H_ */
