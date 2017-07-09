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


void borraDeLaCache(int pid){
	bool buscarPid(lineaCache* fila){
				return (fila->pid== pid);
	}
	log_info(logMemoria,"Se procede a borrar todos las paginas cacheadas del pid %d ya que fue finalizado.", pid);
	list_remove_and_destroy_by_condition(tablaDeEntradasDeCache,buscarPid,free);//faltaria un destroyer decente

}

bool tieneMenosDeNProcesos(int pid){
	bool igualPid(lineaCache* fila){
		return fila->pid == pid;
	}
	int x= list_count_satisfying(tablaDeEntradasDeCache,igualPid);
	log_info(logMemoria,"El proceso %d tiene %d paginas en cache",pid,x);

	return x < getConfigInt("CACHE_X_PROC");
}

void destroyerLineaCache(lineaCache* linea ){
	free(linea->contenido);
	free(linea);
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
	log_info(logMemoria,"[Solicitar Bytes- Cache Hit]-Se actualizo mediante el cacheHit. Es decir, se puso al principio de la lista la linea de cache que produjo el hit.");
	list_add_in_index(tablaDeEntradasDeCache,0,elemento);
}
lineaCache* buscarLinea(int pid, int pagina){
	bool igualPid(lineaCache* fila){
				return fila->pid == pid&& fila->pagina == pagina;
		}
	lineaCache* linea = list_find(tablaDeEntradasDeCache,igualPid);
	log_info(logMemoria,"La busqueda en la cache dio como resultado %b",linea!=NULL);
	return linea;
}

void actualizarPaginaDeLaCache(int pid, int pagina, int tamano, int desplazamiento, void* contenidoModificado) {
	lineaCache* linea = buscarLinea(pid,pagina);
	if(linea == NULL){
		log_warning(logMemoria,"La pagina %d del pid %d  no esta en al cache.",pagina,pid);
	}
	else{
	void* contenido = linea->contenido;
	memcpy(contenido+desplazamiento,contenidoModificado,tamano);
	log_info(logMemoria,"Se actualizo la pagina %d del pid %d",pagina,pid);
	}
}
void* buscarEnLaCache(int pid,int pagina){
	lineaCache* linea = buscarLinea(pid,pagina);
	if(linea == NULL){
		log_warning(logMemoria,"La pagina %d del pid %d no esta en la cache.",pagina,pid);
		return NULL;
	}
	log_info(logMemoria,"La pagina %d del pid %d esta en al cache",pagina,pid);
	void* contenidoDeLaPagina = malloc(sizeOfPaginas);
	memcpy(contenidoDeLaPagina,linea->contenido,sizeOfPaginas);
	return contenidoDeLaPagina;
}

void cacheMiss(int pid, int pagina,void* contenido){
	int cantidadDeEntradas= getConfigInt("ENTRADAS_CACHE");
	if(tieneMenosDeNProcesos(pid)){
		log_info(logMemoria,"[Solicitar Bytes-Cache  Miss]-El pid %d tiene menos de %d paginas.",pid,getConfigInt("CACHE_X_PROC"));
		int cantidadDeElementos = list_size(tablaDeEntradasDeCache);
		if(cantidadDeElementos == cantidadDeEntradas){
			log_info(logMemoria,"[Solicitar Bytes-Cache  Miss]-La cache esta llena, por lo que se procede a borrar el ultimo de la lista");
			list_remove_and_destroy_element(tablaDeEntradasDeCache,cantidadDeEntradas-1,destroyerLineaCache);// POSIBLEMENTE TENGA QUE HACER UN DESTROYER
		}
	}
	else{
		log_info(logMemoria,"Tiene el maximo de paginas permitidas para la cache.");
		int posicion=0;
		int posicionMaxima=0;
		void obtenerDondeEstaElLRUdeUnProceso(lineaCache* fila){
				if( fila->pid == pid&&posicion >posicionMaxima){
					posicionMaxima = posicion;
				}
				posicion++;
		}
		list_iterate(tablaDeEntradasDeCache,obtenerDondeEstaElLRUdeUnProceso);
		log_info(logMemoria,"[Solicitar Bytes-Cache  Miss]-La ultima pagina cacheada del pid %d esta en al posicion %d",pid,posicionMaxima);

		list_remove_and_destroy_element(tablaDeEntradasDeCache,posicionMaxima,destroyerLineaCache);
	}
	lineaCache* linea = malloc(sizeof(lineaCache));
	linea->pagina = pagina;
	linea->pid = pid;
	linea->contenido = malloc(sizeOfPaginas);;
	memcpy(linea->contenido,contenido,sizeOfPaginas);
	log_info(logMemoria,"[Solicitar Bytes-Cache  Miss]-Se agrega esa fila a la cache.");
	list_add_in_index(tablaDeEntradasDeCache,0,linea);
}

void cacheFlush(){
	log_info(logMemoria,"[Cache Flush]-Se limpia (Flush) la cache");
	list_clean_and_destroy_elements(tablaDeEntradasDeCache,destroyerLineaCache);
}

