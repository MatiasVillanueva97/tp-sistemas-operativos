/*
 * funcionesHeap.c
 *
 *  Created on: 14/6/2017
 *      Author: utnso
 */
#include "funcionesHeap.h"

typedef struct{
	int pid;
	t_direccion direccion;
	void* buffer;
}t_almacenarBytes;


int pedirPagina(int pid){
	t_asignarPaginas asignar;
			asignar.cantPags = 1;
			asignar.pid = pid;
			enviarMensaje(socketMemoria,asignarPaginas,&asignar,sizeof(t_asignarPaginas));
			void* stream;
			recibirMensaje(socketMemoria,&stream); // Falta inicializar la pagina
			int pagina = leerInt(stream);
			if(!pagina){
				return false;
			}
			t_almacenarBytes w;
			HeapMetadata* heap = malloc(sizeof(HeapMetadata));
			heap->isFree = true;
			heap->size = size_pagina-sizeof(HeapMetadata);
			w.buffer = heap;
			w.pid = pid;
			w.direccion.page = pagina;
			w.direccion.offset = 0;
			w.direccion.size = sizeof(HeapMetadata);
			enviarMensaje(socketMemoria,almacenarBytes,&w,sizeof(t_direccion)+sizeof(int)+w.direccion.size);//esta mal, necesito el deserealizador de spisso.
			free(stream);
			recibirMensaje(socketMemoria,&stream);
			if(leerInt(stream)){
				filaTablaDeHeapMemoria* elemento = malloc(sizeof(filaTablaDeHeapMemoria));
				elemento->pagina = pagina;
				elemento->pid = pid;
				elemento->tamanoDisponible = size_pagina -5;
				list_add(tablaDeHeapMemoria,elemento);
				return sizeof(HeapMetadata);
			}
			free(stream);
			return 0;
}
int manejarLiberacionDeHeap(int pid,int offset){
	bool busqueda(filaTablaDeHeapMemoria* fila)
		{
			return fila->pid == pid&&fila->pagina == offset/size_pagina;
		}
	filaTablaDeHeapMemoria* fila = list_find(tablaDeHeapMemoria,busqueda);
	if(fila == NULL){
		return 0;
	}
	t_pedidoMemoria escritura;
	escritura.direccion.page = fila->pagina;
	escritura.direccion.offset = 0;
	escritura.direccion.size = size_pagina;
	escritura.id = pid;
	enviarMensaje(socketMemoria,solicitarBytes,&escritura,sizeof(t_pedidoMemoria));
	void* stream;
	recibirMensaje(socketMemoria,&stream);
	if(stream){
		free(stream);
		recibirMensaje(socketMemoria,&stream);
		offsetYHeader loQueTengoQueEscribir= liberarMemoriaHeap(offset,stream);
		t_almacenarBytes almacenamiento;
		almacenamiento.buffer = &loQueTengoQueEscribir.header;
		almacenamiento.direccion.offset = loQueTengoQueEscribir.offset;
		almacenamiento.direccion.page = fila->pagina;
		almacenamiento.direccion.size = sizeof(HeapMetadata);
		almacenamiento.pid = fila->pid;
		enviarMensaje(socketMemoria,almacenarBytes,&almacenamiento,sizeof(int)*4+sizeof(HeapMetadata));
		free(stream);
		recibirMensaje(socketMemoria,&stream);
		return leerInt(stream);
	}
	free(stream);
	return 0;

}

int manejarPedidoDeMemoria(int pid,int tamano){
	bool busqueda(filaTablaDeHeapMemoria* fila)
		{
			return fila->pid == pid;
		}
	t_list* listaFiltrada = list_filter(tablaDeHeapMemoria,busqueda);
	if(list_is_empty(listaFiltrada)){
		return pedirPagina(pid);
	}
	else{
		int i;
		int tamanoLista = list_size(tablaDeHeapMemoria);
		for(i=0;i<tamanoLista;i++){
			filaTablaDeHeapMemoria* fila=list_get(tablaDeHeapMemoria,i);
			if(fila->tamanoDisponible>tamano){//Si podria escribir en esa pagina, se chequea
				t_pedidoMemoria escritura;
				escritura.direccion.page = fila->pagina;
				escritura.direccion.offset = 0;
				escritura.direccion.size = size_pagina;
				escritura.id = pid;

				enviarMensaje(socketMemoria,solicitarBytes,&escritura,sizeof(t_pedidoMemoria));
				void* stream;
				recibirMensaje(socketMemoria,&stream);
				if(*(int*)stream ){//Si no hubo error en memoria
					free(stream);
					recibirMensaje(socketMemoria,&stream);
					offsetYBuffer x = escribirMemoria(tamano,stream);
					if(x.offset >=0){
						t_almacenarBytes w;
						w.buffer = x.buffer;
						w.pid = pid;
						w.direccion.page = fila->pagina;
						w.direccion.offset = x.offset;
						w.direccion.size = tamano;
						enviarMensaje(socketMemoria,almacenarBytes,&w,sizeof(t_almacenarBytes)); // esto esta mal, el size es otro
						void* stream2;
						recibirMensaje(socketMemoria,&stream2);
						fila->tamanoDisponible -= tamano;
						return x.offset;
					}
				}
			}
		}
		return pedirPagina(pid);//
	}
}


//Esto es para heap y sufrira modificaciones
offsetYBuffer escribirMemoria(int tamano,void* memoria){

	HeapMetadata headerAnterior = *((HeapMetadata*) memoria);
	void* buffer = malloc(tamano+sizeof(HeapMetadata)*2);
	int recorrido=0;
		recorrido = recorrido +tamanoHeader;//El recorrido se posiciona en donde termina el header.
	while(size_pagina > recorrido){
		if(headerAnterior.isFree &&headerAnterior.size>tamano+tamanoHeader){
				HeapMetadata header;
				header.isFree= false;
				header.size= tamano;
				int y = recorrido-tamanoHeader;
				int offset = recorrido;
				memcpy(memoria+y,&header,tamanoHeader);
				recorrido += tamano; //El recorrido se posiciona donde va el siguiente header
				header.isFree= true;
				header.size= headerAnterior.size-tamano-tamanoHeader; //Calculo el puntero donde esta, es decir, el tamaño ocupado, y se lo resto a lo que queda.
				memcpy(memoria+recorrido,&header,tamanoHeader);
				memcpy(buffer,memoria+offset-sizeof(HeapMetadata),tamano+sizeof(HeapMetadata)*2);
				if(offset == 5){
					offset = 0;
				}
				printf("El offset es: %d", offset);
				offsetYBuffer x;
				x.buffer = buffer;
				x.offset = offset;

				//escribir en la memoria
				return x;
		}
		else{
			recorrido+=headerAnterior.size;//el header se posiciona para leer el siguiente header.
		}
		headerAnterior = *((HeapMetadata*) (memoria+recorrido));
		recorrido+=tamanoHeader;
	}
	offsetYBuffer x;
	x.offset = -1;
	x.buffer = NULL;
	return x;//no hay espacio suficiente
}
//Esto es para heap y sufrira modificaciones


offsetYHeader liberarMemoriaHeap(int offset,void* pagina){
	if (offset<0){
			perror("ingreso una posicion de la pagina negativa.");
	}
	int i;
	int recorrido = 0;
	HeapMetadata *headerActual = ((HeapMetadata*) (pagina+recorrido));
	HeapMetadata *headerAnterior = malloc(tamanoHeader);

	int offsetQueTengoQueDevolver = 0;

	headerAnterior->isFree = false;
	int recorridoDesdeDondeTengoQueCopiar=recorrido;
	recorrido+= tamanoHeader;
	while(recorrido<size_pagina&&recorrido+sizeof(HeapMetadata)<offset){
		headerAnterior = headerActual;
		recorrido+=headerActual->size;
		recorridoDesdeDondeTengoQueCopiar = recorrido-5;
		headerActual = ((HeapMetadata*) (pagina+recorrido));
		recorrido+= tamanoHeader;
	}
	recorridoDesdeDondeTengoQueCopiar = recorrido-5;
		if(recorrido>=size_pagina){
			perror("pidio una posicion invalida, es decir, que es mayor al numero de posiciones dentro de la pagina"); // esto significa posicion invalida
	}

	else{
			headerActual->isFree= true;
			HeapMetadata* headerSiguiente;
			HeapMetadata  headerAEscribir = *headerActual;
			headerSiguiente = ((HeapMetadata*) (pagina+recorrido+headerActual->size));
			if(headerSiguiente->isFree){
				headerAEscribir.size += headerSiguiente->size + tamanoHeader;
			}
			offsetQueTengoQueDevolver = recorridoDesdeDondeTengoQueCopiar;
			if(headerAnterior->isFree){
				offsetQueTengoQueDevolver = recorridoDesdeDondeTengoQueCopiar-headerActual->size - tamanoHeader;
				headerAnterior->size += headerAEscribir.size + tamanoHeader;
				headerAEscribir = *headerAnterior;
			}
			offsetYHeader x;
			x.header =headerAEscribir;
			x.offset = offsetQueTengoQueDevolver;
			return x;
			//tengo que devolver offsetQueTengoQueDevolver y el headerQueTengaQueEscribir
	}
		return *(offsetYHeader*)NULL;
}
