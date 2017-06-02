/*
 * funcionesConsola.h
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
#include <pthread.h>
#include "commons/collections/list.h"
#include "commons/collections/queue.h"

#include "datosGlobales.h"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

void consola_enviarAvisoDeFinalizacion(int socketConsola, int pid);

void consola_finalizarTodosLosProcesos(int socketConsola);

void consola_crearHiloDetach(int nuevoSocket);

void *rutinaConsola(void * arg);

#endif /* FUNCIONESCONSOLA_H_ */
