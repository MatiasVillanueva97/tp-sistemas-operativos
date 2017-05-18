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
int escribirMemoria(void* contenido,int tamano,void* memoria){

	HeapMetadata headerAnterior = *((HeapMetadata*) memoriaTotal);
	int recorrido=0;
		recorrido = recorrido +tamanoHeader;//El recorrido se posiciona en donde termina el header.
	while(sizeOfPaginas > recorrido){
		if(headerAnterior.isFree &&headerAnterior.size>tamano+tamanoHeader){
				HeapMetadata header;
				header.isFree= false;
				header.size= tamano;
				int y = recorrido-tamanoHeader;
				memcpy(memoriaTotal+y,&header,tamanoHeader);
				memcpy(memoriaTotal+recorrido,contenido,tamano);
				recorrido += tamano; //El recorrido se posiciona donde va el siguiente header
				header.isFree= true;
				header.size= headerAnterior.size-tamano-tamanoHeader; //Calculo el puntero donde esta, es decir, el tamaño ocupado, y se lo resto a lo que queda.
				memcpy(memoriaTotal+recorrido,&header,tamanoHeader);
				//escribir en la memoria
				return 1;
		}
		else{
			recorrido+=headerAnterior.size;//el header se posiciona para leer el siguiente header.
		}
		headerAnterior = *((HeapMetadata*) (memoria+recorrido));
	}
	return 0;//no hay espacio suficiente
}
//Esto es para heap y sufrira modificaciones
void liberarMemoria(int posicion_dentro_de_la_pagina,void* pagina){
	if (posicion_dentro_de_la_pagina<0){
			perror("ingreso una posicion de la pagina negativa.");
	}
	int i;
	int recorrido = 0;
	HeapMetadata x = *((HeapMetadata*) (pagina+recorrido));
	recorrido+= sizeof(x);
	for (i=0;i<posicion_dentro_de_la_pagina&&recorrido<sizeOfPaginas;i++){
			recorrido+=x.size;
			x = *((HeapMetadata*) (pagina+recorrido));
			recorrido+= sizeof(x);

	}
	if(recorrido>=sizeOfPaginas){
			perror("pidio una posicion invalida, es decir, que es mayor al numero de posiciones dentro de la pagina"); // esto significa posicion invalida
	}
	else{
			x.isFree= true;
			memcpy(pagina+recorrido-sizeof(x),&x,sizeof(x));
	}
}
