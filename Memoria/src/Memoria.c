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

/////////////////////
/////////////            HACER ENUM los DIFAINS negros que estan en la biblioteca de sockets
/////////////////




#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"
#define ID 2

int sizeOfPaginas;
void* memoriaTotal;
void* cache;

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
	uint32_t frame;
	uint32_t pid;
	char* pagina;
	}lineaCache;
typedef struct{
	uint32_t size;
	bool isFree;
}__attribute__((packed))HeapMetadata;

#define tamanoHeader sizeof(HeapMetadata) // not sure si anda esto
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
void* leerMemoria(int posicion_dentro_de_la_pagina, int*tamanioStreamLeido){

	if (posicion_dentro_de_la_pagina<0){
		perror("ingreso una posicion de la pagina negativa.");
	}
	int i;
	int recorrido = 0;
	HeapMetadata x = *((HeapMetadata*) (memoriaTotal+recorrido));
	recorrido+= sizeof(x);
	for (i=0;i<posicion_dentro_de_la_pagina&&recorrido<sizeOfPaginas;){
		recorrido+=x.size;
		x = *((HeapMetadata*) (memoriaTotal+recorrido));
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
		memcpy(contenido,memoriaTotal+recorrido,x.size);
		*tamanioStreamLeido=x.size;
		return contenido;
	}
}


int buscarPidEnTablaInversa(int pidRecibido)
{
	return 0;
}

void *rutinaCPU( void * arg)
{
	int socketCPU=(int)arg;
	int pidRecibido, tamanioDatosAEnviarCPU;

	puts("Entro a la rutina CPU");
	recibirMensaje(socketCPU,&pidRecibido);

	void * datosAEnviarACPU = leerMemoria(buscarPidEnTablaInversa(pidRecibido),&tamanioDatosAEnviarCPU);

	puts((char*)datosAEnviarACPU);

	enviarMensaje(socketCPU,2,datosAEnviarACPU,tamanioDatosAEnviarCPU);

	free(datosAEnviarACPU);
}

void *aceptarConexiones( void *arg ){ // aca le sacamos el asterisco, porque esto era un void*
	int listener = (int)arg;
	socklen_t sin_size;
	int nuevoSocket;
	int aceptados[] = {0,1};
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];

	escuchar(listener); // poner a escuchar ese socket

	if ((nuevoSocket = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {// estas lines tienen que ser una funcion
		perror("Error en el Accept");
		//continue;
	}
	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server
	printf("Conexion con %s\n", ip);

	int id_clienteConectado;
	if ((id_clienteConectado = handshakeServidor(nuevoSocket, ID, aceptados)) == -1) {
		perror("Error con el handshake: -1");
		close(nuevoSocket);
	}
	printf("Conexión exitosa con el Cliente(%i)!!\n",id_clienteConectado);

	pthread_t hilo_nuevaCPU;

	switch(id_clienteConectado)
	{
		case 0:{ // Si el cliente conectado es el kernel
			printf("ENTRO POR EL KERNEL");
			// haces un hilo para el kernel
			}break;
		case 1:{ // Si es un cliente conectado es una CPU
			printf("\nNueva CPU Conectada!\n");
			rutinaCPU(nuevoSocket);
			//pthread_create(&hilo_nuevaCPU, NULL, rutinaCPU, &nuevoSocket);
			}break;
		default:{
			close(nuevoSocket);
			}
	}
	pthread_join(hilo_nuevaCPU, NULL);
}


/*
 * rutina kernel
 * while 1
 * {
 * haces un reciv
 * recivis el pid
 * hasces weas con el pid que por ahora no hacen nada
 * lees - escribis - liberas memoria
 * }
 */


/*
 * rutina Cpu
 * while 1
 * reciv --- while 0, esto viene de la mano de que cuando la cpu se vaya, te tira un close socket y de ahi aca recivis el 0 de fin de conexion
 * lees pid
 * de acuierdo al pid devuelve el contenido que espera
 * chau
 */

int main(void) {

	printf("Inicializando Memoria.....\n\n");

	// ******* Declaración de la mayoria de las variables a utilizar

	int listener, nuevoSocket, id_clienteConectado;


	char* mensajeRecibido= string_new();


	// ******* Configuracion de la Memoria a partir de un archivo

	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config");
	imprimirConfiguracion();

	sizeOfPaginas=getConfigInt("MARCO_SIZE");
	memoriaTotal = malloc(sizeOfPaginas);


	HeapMetadata header;
	header.isFree= true;
	header.size= sizeOfPaginas-5;
	memcpy(memoriaTotal,&header,tamanoHeader);


	escribirMemoria((void*)"hola hijo de puta",strlen("hola hijo de puta")+1,memoriaTotal);
	escribirMemoria((void*)"hola hijo de",strlen("hola hijo de")+1,memoriaTotal);
	escribirMemoria((void*)"hola",strlen("hola")+1,memoriaTotal);

/*
	//cache = malloc(getConfigInt("ENTRADAS_CACHE")*(sizeOfPaginas+sizeof(uint32_t)*2));
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
*/



	char *memoriav1 = string_new(); // aca se guarda el script o cualquier cosa que llega
	// ******* Conexiones obligatorias y necesarias

	listener = crearSocketYBindeo(getConfigString("PUERTO")); // asignar el socket principal


	aceptarConexiones(NULL);

/*	pthread_t hilo_AceptarConexiones;

	pthread_create(&hilo_AceptarConexiones, NULL, aceptarConexiones,  listener);

	pthread_join(hilo_AceptarConexiones , NULL);

/*	while (1) {
		sin_size = sizeof their_addr;
		//esto deberia ser una funcion en la libreria de sockets que sea aceptar sockets.

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
*/

	close(listener);
	liberarConfiguracion();
	free(memoriav1);
	free(mensajeRecibido);
	free(memoriaTotal);
	return EXIT_SUCCESS;
}

