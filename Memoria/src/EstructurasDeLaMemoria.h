/*
 * EstructurasDeLaMemoria.h
 *
 *  Created on: 16/5/2017
 *      Author: utnso
 */

#include <stdbool.h>
#include <stdint.h>
#include "commons/collections/list.h"


#ifndef ESTRUCTURASDELAMEMORIA_H_
#define ESTRUCTURASDELAMEMORIA_H_
int retardo;
int sizeOfPaginas;
int cantidadDeMarcos;
void* memoriaTotal;
t_list* cache;
t_list* tablaConCantidadDePaginas;

typedef struct{
	int pid;
	int cantidadDePaginas;
}filaTablaCantidadDePaginas;
typedef struct{
	int pid;
	int pagina;
	int frame;
}filaDeTablaPaginaInvertida;

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
typedef struct{
	uint32_t size;
	bool isFree;
}__attribute__((packed))HeapMetadata;

#endif /* ESTRUCTURASDELAMEMORIA_H_ */
