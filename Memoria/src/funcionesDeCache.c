/*
 * funcionesDeCache.c
 *
 *  Created on: 17/5/2017
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "EstructurasDeLaMemoria.h"

#define tamanoHeader sizeof(HeapMetadata)// no se si anda



bool tieneMenosDeNProcesos(int pid){
	bool igualPid(lineaCache* fila){
		return fila->pid == pid;
	}
	int x= list_count_satisfying(tablaDeEntradasDeCache,igualPid);
	return x < getConfigInt("CACHE_X_PROC");
}


//Funciones Cache
/*void agregarAlPrincipio(lineaCache lineaCache1){
	list_remove(cache,list_size(cache));
	list_add_in_index(cache,&lineaCache1,0);
}*/

//void eliminarPaginas(){}
void cacheHit(int pid, int pagina){
	bool busquedaCache(lineaCache* linea){
		return linea->pagina == pagina &&pid ==linea->pid;
	}
	lineaCache* elemento = list_remove_by_condition(tablaDeEntradasDeCache,busquedaCache);
	list_add_in_index(tablaDeEntradasDeCache,0,elemento);
}
lineaCache* buscarLinea(int pid, int pagina){
	bool igualPid(lineaCache* fila){
				return fila->pid == pid&& fila->pagina == pagina;
		}
		t_list* x = tablaDeEntradasDeCache;
		lineaCache* y = list_find(tablaDeEntradasDeCache,igualPid);
	return y	;
}

void actualizarPaginaDeLaCache(int pid, int pagina, int tamano, int desplazamiento, void* contenidoModificado) {
	lineaCache* linea = buscarLinea(pid,pagina);
	puts("hola");
	void* contenido = linea->contenido;
	memcpy(contenido+desplazamiento,contenidoModificado,tamano);
}
void* buscarEnLaCache(int pid,int pagina){
	lineaCache* linea = buscarLinea(pid,pagina);
	if(linea == NULL){
		return NULL;
	}
	void* contenidoDeLaPagina = malloc(sizeOfPaginas);
	memcpy(contenidoDeLaPagina,linea->contenido,sizeOfPaginas);
	return contenidoDeLaPagina;
}

void cacheMiss(int pid, int pagina,void* contenido){
	int cantidadDeEntradas= getConfigInt("ENTRADAS_CACHE");
	if(tieneMenosDeNProcesos(pid)){
		int cantidadDeElementos = list_size(tablaDeEntradasDeCache);
		if(cantidadDeElementos == cantidadDeEntradas){
			list_remove_and_destroy_element(tablaDeEntradasDeCache,cantidadDeEntradas-1,free);// POSIBLEMENTE TENGA QUE HACER UN DESTROYER
		}
	}
	else{
		int posicion=0;
		int posicionMaxima=0;
		void obtenerDondeEstaElLRUdeUnProceso(lineaCache* fila){
				if( fila->pid == pid&&posicion >posicionMaxima){
					posicionMaxima = posicion;
				}
				posicion++;
		}
		list_iterate(tablaDeEntradasDeCache,obtenerDondeEstaElLRUdeUnProceso);
		list_remove_and_destroy_element(tablaDeEntradasDeCache,posicionMaxima,free);
	}
	lineaCache* linea = malloc(sizeof(lineaCache));
	linea->pagina = pagina;
	linea->pid = pid;
	void* contenido2 = malloc(sizeOfPaginas);
	linea->contenido = contenido2;
	memcpy(linea->contenido,contenido,sizeOfPaginas);
	list_add_in_index(tablaDeEntradasDeCache,0,linea);
}

void cacheFlush(){
	list_clean(tablaDeEntradasDeCache);
}

