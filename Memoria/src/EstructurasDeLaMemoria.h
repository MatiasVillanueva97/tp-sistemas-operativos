/*
 * EstructurasDeLaMemoria.h
 *
 *  Created on: 16/5/2017
 *      Author: utnso
 */

#include <stdbool.h>
#include <stdint.h>
#include "commons/collections/list.h"
#include <pthread.h>
#include <semaphore.h>

#ifndef ESTRUCTURASDELAMEMORIA_H_
#define ESTRUCTURASDELAMEMORIA_H_
int retardo;
int sizeOfPaginas;
int cantidadDeMarcos;
void* memoriaTotal;
t_list* tablaDeEntradasDeCache;
t_list* tablaConCantidadDePaginas;
sem_t mutex_Memoria;
sem_t mutex_TablaDePaginasInvertida;
sem_t mutex_TablaDeCantidadDePaginas;

sem_t mutex_retardo;
sem_t mutex_cache;

sem_t sem_isKernelConectado;

typedef struct{
	int pid;
	int paginaMaxima;
	int cantidadDePaginasReales;
}filaTablaCantidadDePaginas;
typedef struct{
	int pid;
	int pagina;
	int frame;
}filaDeTablaPaginaInvertida;

typedef struct{
	int pid;
	int pagina;
	int orden;
}filaDeTablaOrdenCache;

filaDeTablaPaginaInvertida* tablaDePaginacionInvertida; //Podria ser un t_list
typedef struct{
	uint32_t pagina;
	uint32_t pid;
}headerCache;

typedef struct{
	uint32_t pagina;
	uint32_t pid;
	uint32_t frame;
}columnaTablaMemoria;
typedef struct{
	uint32_t pid;
	uint32_t pagina;
	void* contenido;
	}lineaCache;


#endif /* ESTRUCTURASDELAMEMORIA_H_ */
