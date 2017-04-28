/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
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
#include "commons/config.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"
#define ID 2
int sizeOfPaginas;

void* cache;

typedef struct{
	uint32_t size;
	bool isFree;
}__attribute__((packed))
HeapMetadata;

#define tamanoHeader sizeof(HeapMetadata) // not sure si anda esto

int escribirMemoria(void* contenido,int tamano,void* memoria){

	HeapMetadata headerAnterior = *((HeapMetadata*) memoria);
	int recorrido=0;
	while(sizeOfPaginas > recorrido){
		recorrido = recorrido +tamanoHeader;//El recorrido se posiciona en donde termina el header.
		if(headerAnterior.isFree &&headerAnterior.size>tamano+tamanoHeader){
				HeapMetadata header;
				header.isFree= false;
				header.size= tamano;
				int y = recorrido-tamanoHeader;
				memcpy(memoria+y,&header,tamanoHeader);
				memcpy(memoria+recorrido,contenido,tamano);
				recorrido += tamano; //El recorrido se posiciona donde va el siguiente header
				header.isFree= true;
				header.size= headerAnterior.size-tamano-tamanoHeader; //Calculo el puntero donde esta, es decir, el tamaño ocupado, y se lo resto a lo que queda.
				memcpy(memoria+recorrido,&header,tamanoHeader);
				//escribir en la memoria
				return 0;

		}
		else{
			recorrido+=headerAnterior.size;//el header se posiciona para leer el siguiente header.
		}
		headerAnterior = *((HeapMetadata*) (memoria+recorrido));
	}
	return 1;//no hay espacio suficiente

}
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
void* leerMemoria(int posicion_dentro_de_la_pagina,void* pagina){

	if (posicion_dentro_de_la_pagina<0){
		perror("ingreso una posicion de la pagina negativa.");
	}
	int i;
	int recorrido = 0;
	HeapMetadata x = *((HeapMetadata*) (pagina+recorrido));
	recorrido+= sizeof(x);
	for (i=0;i<posicion_dentro_de_la_pagina&&recorrido<sizeOfPaginas;){
		recorrido+=x.size;
		x = *((HeapMetadata*) (pagina+recorrido));
		recorrido+= sizeof(x);
		if (!x.isFree){
			i++;
		}

	}
	if(recorrido>=sizeOfPaginas){
		perror("pidio una posicion invalida, es decir, que es mayor al numero de posiciones dentro de la pagina"); // esto significa posicion invalida
	}
	else{
		void* contenido = malloc(x.size);// hay que liberarlo dsp de mandarlo
		memcpy(contenido,pagina+recorrido,x.size);
		return contenido;
	}
}

typedef struct{
	uint32_t pagina;
	uint32_t pid;
}headerCache;
bool tieneMenosDeTresProcesosEnLaCache(int pid){
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
typedef struct{
	uint32_t pagina;
	uint32_t pid;
	uint32_t frame;
}columnaTablaMemoria;

typedef struct{
	uint32_t frame;
	uint32_t pid;
	char* pagina;
}lineaCache;


int main(void) {

	printf("Inicializando Memoria.....\n\n");

	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;

	struct sockaddr_storage their_addr; // Estructura que contiene la informacion de la conexion

	int listener, nuevoSocket, rta_handshake;
	int aceptados[] = {0,3};
	char ip[INET6_ADDRSTRLEN];

	char* mensajeRecibido= string_new();


	// ******* Configuracion de la Memoria a partir de un archivo

	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config");
	imprimirConfiguracion();
	sizeOfPaginas=getConfigInt("MARCO_SIZE");
	void* memoriaTotal = malloc(sizeOfPaginas);
	//cache = malloc(getConfigInt("ENTRADAS_CACHE")*(sizeOfPaginas+sizeof(uint32_t)*2));
	HeapMetadata header;
	header.isFree= true;
	header.size= sizeOfPaginas-5;
	memcpy(memoriaTotal,&header,tamanoHeader);

	escribirMemoria((void*)"hola hijo de puta",strlen("hola hijo de puta")+1,memoriaTotal);
	escribirMemoria((void*)"hola hijo de",strlen("hola hijo de")+1,memoriaTotal);
	escribirMemoria((void*)"hola",strlen("hola")+1,memoriaTotal);


	char* x = (char*) leerMemoria(1,memoriaTotal);
	printf("hola esto es una prueba: %s", x);
	printf("hola esto es una prueba2: %c", *x);
	char* y = (char*) leerMemoria(2,memoriaTotal);
	printf("%s", y);
	liberarMemoria(0,memoriaTotal);
	escribirMemoria((void*)"hola",strlen("hola")+1,memoriaTotal);
	escribirMemoria((void*)"pruebo",strlen("prueba")+1,memoriaTotal);

	free(x);
	free(y);
	char * w = (char*) leerMemoria(0,memoriaTotal);

	char * z = (char*) leerMemoria(1,memoriaTotal);
	free(w);
	free(z);




	// ******* Conexiones obligatorias y necesarias
	listener = crearSocketYBindeo(getConfigString("PUERTO")); // asignar el socket principal
	escuchar(listener); // poner a escuchar ese socket
	char *memoriav1 = string_new(); // aca se guarda el script o cualquier cosa que llega


	int numero_pag=0; // variable que de dice el numero de pagina

	while (1) {
		sin_size = sizeof their_addr;
//esto deberia ser una funcion en la libreria de sockets que sea aceptar sockets.
		if ((nuevoSocket = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
			perror("Error en el Accept");
			continue;
		}

		inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server

		printf("Conexion con %s\n", ip);

		if (!fork()) { // this is the child process
			close(listener); // child doesn't need the listener

			if ((rta_handshake = handshakeServidor(nuevoSocket, ID, aceptados)) == -1) {
				perror("Error con el handshake: -1");
				close(nuevoSocket);
			}

			printf("Conexión exitosa con el Server(%i)!!\n",rta_handshake);

			if(recibirMensaje(nuevoSocket,mensajeRecibido)==-1){
				perror("Error en el Reciv");
			}

			printf("Mensaje desde el Kernel: %s\n\n", mensajeRecibido);
			//free(mensajeRecibido);//OJO!!!!!ESTO HAY QUE MEJORARLO -- Comentario 2 esto esta omcentado tiera violacion de segmento

			if(mensajeRecibido[0]=='0') // leo un 0, eso quiere decir que kernel me acaba de mandar un codigo ansisop para guardar
			{
				memoriav1=mensajeRecibido; // abstraccion pura
				numero_pag++;

				printf("Reservado: memoriav1: %s\nEnviado: numero_pag: %d\n\n", memoriav1, numero_pag);

				enviarMensaje(nuevoSocket, 1, (void *)&numero_pag, sizeof(int));
			}

			close(nuevoSocket);
			exit(0);
		}
		close(nuevoSocket);  // parent doesn't need this




	liberarConfiguracion();
	free(memoriav1);
	free(mensajeRecibido);
	free(memoriaTotal);
	return EXIT_SUCCESS;
}

