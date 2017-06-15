/*
 * funcionesHeap.h
 *
 *  Created on: 14/6/2017
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include "datosGlobales.h"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"
typedef struct{
	int offset;
	void* buffer;
}__attribute__((packed))offsetYBuffer;


typedef struct{
	uint32_t size;
	bool isFree;
}__attribute__((packed))HeapMetadata;
typedef struct{
	int offset;
	HeapMetadata header;
}offsetYHeader;
#define tamanoHeader sizeof(HeapMetadata) // not sure si anda esto

typedef struct {
	int pid;
	int cantPags;
}__attribute__((packed)) t_asignarPaginas;


#ifndef FUNCIONESHEAP_H_
#define FUNCIONESHEAP_H_
int manejarPedidoDeMemoria(int pid,int tamano);
offsetYBuffer escribirMemoria(int tamano,void* memoria);
offsetYHeader liberarMemoriaHeap(int offset,void* pagina);
#endif /* FUNCIONESHEAP_H_ */
