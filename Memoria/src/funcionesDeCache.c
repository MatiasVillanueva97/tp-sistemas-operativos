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




//Funciones Cache
void agregarAlPrincipio(lineaCache lineaCache1){
	list_remove(cache,list_size(cache));
	list_add_in_index(cache,&lineaCache1,0);
}
//void inicializarCache(){}
//void eliminarPaginas(){}
void actualizarCache(lineaCache lineaCache1){
	bool busquedaDeCache(lineaCache* lineasDeCache){
		return lineasDeCache->pagina==lineaCache1.pagina && lineasDeCache->pid ==lineaCache1.pid;
	}
	lineaCache* algo =list_remove_by_condition(cache,busquedaDeCache);
	list_add_in_index(cache,0,algo);
}
bool tieneMenosDeXProcesosEnLaCache(int pid){
	int i;
	int contador = 0;
	for(i=0;i<getConfigInt("ENTRADAS_CACHE");i++){
		headerCache x = *((headerCache*) (cache+(sizeOfPaginas+sizeof(uint32_t)*2)*i));
		if(x.pid == pid){
			contador++;
		}
	}
	return contador < getConfigInt("ENTRADAS_CACHE");
}

