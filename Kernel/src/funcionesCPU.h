/*
 * funcionesCPU.h
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


#ifndef FUNCIONESCPU_H_
#define FUNCIONESCPU_H_


void cpu_crearHiloDetach(int nuevoSocket);

void *rutinaCPU(void * arg);


#endif /* FUNCIONESCPU_H_ */
