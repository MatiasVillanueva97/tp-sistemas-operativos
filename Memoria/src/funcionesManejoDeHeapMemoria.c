/*
 * funcionesManejoDeHeapMemoria.c
 *
 *  Created on: 16/5/2017
 *      Author: utnso
 */

#include "EstructurasDeLaMemoria.h"
#include <stdbool.h>
#define tamanoHeader sizeof(HeapMetadata) // not sure si anda esto

void* leerMemoria(int posicion_dentro_de_la_pagina, int*tamanioStreamLeido){

	if (posicion_dentro_de_la_pagina<0){
		perror("ingreso una posicion de la pagina negativa.");
	}
	int i;
	int recorrido = 0;
	HeapMetadata x = *((HeapMetadata*) (memoriaTotal+recorrido));// Esto lee la primera el header de la pagina
	recorrido+= sizeof(x);//Tamanio del heap

	for (i=0;i<posicion_dentro_de_la_pagina&&recorrido<sizeOfPaginas;){//esto sigue hasta que llegue a la posicion de la pagina actual requerida o se pese del size
		recorrido+=x.size;// Al recorrido le aumento el tamanio con el heap
		x = *((HeapMetadata*) (memoriaTotal+recorrido)); // se mueve el heap
		recorrido+= sizeof(x);
		if (!x.isFree){
			i++;
		}
	}

	if(recorrido>=sizeOfPaginas){// manejo de error
		perror("pidio una posicion invalida, es decir, que es mayor al numero de posiciones dentro de la pagina"); // esto significa posicion invalida
	}
	else{
		void* contenido = malloc(x.size);//Reservo memoria para poder retornarlo
		memcpy(contenido,memoriaTotal+recorrido,x.size);//Agarro la memtotal y la copio en el contenido
		*tamanioStreamLeido=x.size;//modificas la posicion de memoria donde esta el tamaño
		return contenido;
	}
}
//Esto es para heap y sufrira modificaciones
void* escribirMemoria(void* contenido,int tamano,void* memoria){

	HeapMetadata headerAnterior = *((HeapMetadata*) memoria);
	void* buffer = malloc(tamano+sizeof(HeapMetadata)*2);
	int recorrido=0;
		recorrido = recorrido +tamanoHeader;//El recorrido se posiciona en donde termina el header.
	while(sizeOfPaginas > recorrido){
		if(headerAnterior.isFree &&headerAnterior.size>tamano+tamanoHeader){
				HeapMetadata header;
				header.isFree= false;
				header.size= tamano;
				int y = recorrido-tamanoHeader;
				int offset = recorrido;
				memcpy(memoria+y,&header,tamanoHeader);
				memcpy(memoria+recorrido,contenido,tamano);
				recorrido += tamano; //El recorrido se posiciona donde va el siguiente header
				header.isFree= true;
				header.size= headerAnterior.size-tamano-tamanoHeader; //Calculo el puntero donde esta, es decir, el tamaño ocupado, y se lo resto a lo que queda.
				memcpy(memoria+recorrido,&header,tamanoHeader);
				memcpy(buffer,memoria+offset-sizeof(HeapMetadata),tamano+sizeof(HeapMetadata)*2);
				if(offset == 5){
					offset = 0;
				}
				printf("El offset es: %d", offset);
				//escribir en la memoria
				return buffer;
		}
		else{
			recorrido+=headerAnterior.size;//el header se posiciona para leer el siguiente header.
		}
		headerAnterior = *((HeapMetadata*) (memoria+recorrido));
		recorrido+=tamanoHeader;
	}
	return 0;//no hay espacio suficiente
}
//Esto es para heap y sufrira modificaciones
void liberarMemoriaHeap(int offset,void* pagina){
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
	while(recorrido<sizeOfPaginas&&recorrido+sizeof(HeapMetadata)<offset){
		recorridoDesdeDondeTengoQueCopiar = recorrido-5;
		headerAnterior = headerActual;
		recorrido+=headerActual->size;
		headerActual = ((HeapMetadata*) (pagina+recorrido));
		recorrido+= tamanoHeader;
	}
		if(recorrido>=sizeOfPaginas){
			perror("pidio una posicion invalida, es decir, que es mayor al numero de posiciones dentro de la pagina"); // esto significa posicion invalida
	}
	else{
			headerActual->isFree= true;
			HeapMetadata* headerSiguiente;
			HeapMetadata  headerAEscribir = *headerActual;
			headerSiguiente = ((HeapMetadata*) (pagina+recorrido+headerActual->size));
			if(headerSiguiente->isFree){
				headerActual->size += headerSiguiente->size + tamanoHeader;
				offsetQueTengoQueDevolver = recorrido;
			}
			if(headerAnterior->isFree){
				headerAnterior->size += headerActual->size + tamanoHeader;
				offsetQueTengoQueDevolver = recorridoDesdeDondeTengoQueCopiar;
				headerAEscribir = *headerAnterior;
			}

			//tengo que devolver offsetQueTengoQueDevolver y el headerQueTengaQueEscribir
	}
}
