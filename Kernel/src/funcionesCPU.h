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
#include "funcionesSemaforosYCompartidas.h"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"



#ifndef FUNCIONESCPU_H_
#define FUNCIONESCPU_H_


void cpu_crearHiloDetach(int nuevoSocket);

PCB_DATA * cpu_pedirPCBDeExec();

void *rutinaCPU(void * arg);

void* serializarPedidoFs(int size, int offset,char* path);

t_crearArchivo deserializarCrearArchivo(void* stream);


#endif /* FUNCIONESCPU_H_ */


/*
t_mensajeDeProceso deserializarMensajeAEscribir(void* stream){
	t_mensajeDeProceso mensaje;

	memcpy(&mensaje.pid,stream,sizeof(int));

	memcpy(&mensaje.descriptorArchivo,stream + sizeof(int),sizeof(int));

	int tamanoContenido;

	memcpy(&tamanoContenido,stream + sizeof(int) * 2,sizeof(int));

	char* contenidoAuxiliar = malloc(tamanoContenido);

	memcpy(contenidoAuxiliar,stream + sizeof(int) * 3, tamanoContenido);

	mensaje.mensaje = contenidoAuxiliar;

	return mensaje;
}*/
