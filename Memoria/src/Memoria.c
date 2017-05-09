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
#include <semaphore.h>
#include "commons/config.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#define ID 2

int sizeOfPaginas;
void* memoriaTotal;
void* cache;
typedef struct{
	int pid;
	int pagina;
	int frame;
}filaDeTablaPaginaInvertida;

sem_t sem_isKernelConectado; // meparece que es otro tipo de semaforo, no mutex
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
//Esto es para heap y sufrira modificaciones
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
//Esto es para heap y sufrira modificaciones
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


int funcionHash (int pid, int pagina){
	return 0; //Falta hacer una funcion de hash
}
int buscarFrameCorrespondiente(int pidRecibido,int pagina)
{
	int posicionDadaPorElHash = funcionHash(pidRecibido,pagina);
	filaDeTablaPaginaInvertida filaActual;
	while (posicionDadaPorElHash < getConfigInt("MARCOS")){
		filaActual =tablaDePaginacionInvertida[posicionDadaPorElHash];
		if (filaActual.pid == pidRecibido && filaActual.pagina == pagina ){
			return filaActual.frame;
		}
		posicionDadaPorElHash++;
	}
	return -1;
}
int reservarFrame(int pid, int pagina){
	int i;
	for(i=funcionHash(pid,pagina);getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.frame == -1){
			tablaDePaginacionInvertida[i].pagina = pagina;
			tablaDePaginacionInvertida[i].pid = pid;
			return 1;
		}
	}
	return -1;
}

int liberarPagina(int pid, int pagina){
	int i;
	for(i=funcionHash(pid,pagina);getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == pagina && filaActual.pid == pid){
			tablaDePaginacionInvertida[i].pagina = -1;
			tablaDePaginacionInvertida[i].pid = -1;
			return 1;
		}
	}
	return -1;
}
void* leerMemoriaPosta (int pid, int pagina ){
	int frame = buscarFrameCorrespondiente(pid,pagina); //checkear que no haya errores en buscarFrame.
	void * contenido = malloc(getConfigInt("MARCO_SIZE"));
	memcpy(contenido,memoriaTotal+frame*getConfigInt("MARCO_SIZE"),getConfigInt("MARCO_SIZE"));
	return contenido;
}
int escribirMemoriaPosta(int pid,int pagina,void* contenido){
	//Antes de poder escribir, se deben haber reservado los frame.
	if(strlen(contenido)>getConfigInt("MARCO_SIZE")){
		perror("El tamaño del contenido es mayor al tamaño de una pagina con la configuracion actual");
		return -1;
	}
	int frame = buscarFrameCorrespondiente(pid,pagina);
	memcpy(memoriaTotal+frame*getConfigInt("MARCO_SIZE"),contenido,getConfigInt("MARCO_SIZE"));
	return 0;
}

int buscarPidEnTablaInversa(int pidRecibido) //DEJO ESTA PORQUE SINO SE ROMPE TODO MIENTRAS SE VA IMPLEMENTANDO LO OTRO.
{
	return 0;
}
void *rutinaCPU(void * arg)
{
	int socketCPU = ((int*)arg)[0] ;
	int pidRecibido, tamanioDatosAEnviarCPU;

	printf("[Rutina CPU] - Entro a la rutina CPU. Socket CPU: %d\n", socketCPU);

	recibirMensaje(socketCPU,&pidRecibido);

	void * datosAEnviarACPU = leerMemoria(buscarPidEnTablaInversa(pidRecibido),&tamanioDatosAEnviarCPU);

	puts((char*)datosAEnviarACPU);

	enviarMensaje(socketCPU,2,datosAEnviarACPU,tamanioDatosAEnviarCPU);

	printf("[Rutina CPU] - MensajeEnviado a CPU. Socket CPU: %s\n\n", datosAEnviarACPU);

	free(datosAEnviarACPU);
}

/*
    pthread_mutex_lock( &mutex_isKernelConectado );
	pthread_mutex_init()
	pthread_mutex_lock()
	pthread_mutex_unlock()
	pthread_mutex_destroy()
	pthread_mutex_unlock( &mutex );
*/


void *rutinaKernel(void *arg){ // no se si tiene que ser void
	int listener = (int)arg;
	int socketKernel;
	int aceptados[] = {0}; // hacer un enum de esto
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;

	escuchar(listener); // poner a escuchar ese socket

	if ((socketKernel = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {// estas lines tienen que ser una funcion
		perror("Error en el Accept");
		//continue;
	}
	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server
	printf("[Rutina Kernel] - Conexion con %s\n", ip);

	int id_clienteConectado;

	if ((id_clienteConectado = handshakeServidor(socketKernel, ID, aceptados)) == -1) {
		perror("Error con el handshake: -1");
		close(socketKernel);
	}
	printf("[Rutina Kernel] - Kernel conectado exitosamente\n");

	sem_post(&sem_isKernelConectado);

	char* stream = malloc(100);
	recibirMensaje(socketKernel,stream); // recibir el stream
	printf("[Rutina Kernel] - Mensaje Enviado desde Kernel: %s\n",stream);

	int tamano;
	recibirMensaje(socketKernel,&tamano); // El Kernel me envia al tamanio del stream
	printf("[Rutina Kernel] - Tamanio Enviado desde Kernel: %d\n",tamano);

    int okSePuedeGuardarEsteCodigo; // hay que validar que se pueda guardar en memoria

	if ((okSePuedeGuardarEsteCodigo=escribirMemoria(stream, tamano, memoriaTotal)))
		printf("[Rutina Kernel] - Se Almaceno Correctamente el mensaje en memoria\n");
	else
		printf("[Rutina Kernel] - No se pudo almacenar espacio\n");

	enviarMensaje(socketKernel,1,&okSePuedeGuardarEsteCodigo,sizeof(int));   //Respuesta de que salio todo ok en memoria asi el kernel puede avanzar
	printf("[Rutina Kernel] - Respuesta Enviada a Kernel: %d\n",okSePuedeGuardarEsteCodigo);

	int pidRecibido;
	recibirMensaje(socketKernel,&pidRecibido); // El Kernel Me envia el pid
	printf("[Rutina Kernel] - Pid Enviado desde Kernel: %d\n",pidRecibido);

	//Hay que hacer que se guarden cosas en la memoria!
	if (escribirMemoria(stream, tamano, memoriaTotal)!=-1)		//mensaje = "Se almaceno correctamente";
		printf("[Rutina Kernel] - Se Almaceno Correctamente el mensaje en memoria\n");
	else
		printf("[Rutina Kernel] - No se pudo almacenar espacio\n");

	//Aca hay que almacenarlo en la tabla (Cuando lo escribis deberia ser)
	//enviarMensaje(socketKernel,2,mensaje,strlen(mensaje+1));

	int nroPagina = 1; // devolvemos 1 porque solo hay una pagina por ahora
	enviarMensaje(socketKernel,1,&nroPagina,sizeof(int));   //Respuesta de que salio todo ok en memoria asi el kernel puede avanzar
	printf("[Rutina Kernel] - nroPagina Enviada a Kernel: %d\n",nroPagina);

	free(stream);
}

void *aceptarConexionesCpu( void *arg ){ // aca le sacamos el asterisco, porque esto era un void*
	int listener = (int)arg;
	int nuevoSocketCpu;
	int aceptados[] = {1};
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;
	escuchar(listener); // poner a escuchar ese socket

	pthread_t hilo_nuevaCPU;

	sem_wait(&sem_isKernelConectado);
	printf("[AceptarConexionesCPU] - Ya se ha establecido Conexion con un Kernel, ahora si se pueden conectar CPUs: \n");

	while (1)
	{
		sin_size = sizeof their_addr;

		if ((nuevoSocketCpu = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {// estas lines tienen que ser una funcion
			perror("Error en el Accept");
			//continue;
		}
		inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server
		printf("[AceptarConexionesCPU] - Conexion con %s\n", ip);

		int id_clienteConectado;
		if ((id_clienteConectado = handshakeServidor(nuevoSocketCpu, ID, aceptados)) == -1) {
			perror("Error con el handshake: -1");
			close(nuevoSocketCpu);
		}
		printf("[AceptarConexionesCPU] - Nueva CPU Conectada! Socket CPU: %d\n", nuevoSocketCpu);

		pthread_create(&hilo_nuevaCPU, NULL, rutinaCPU,  &nuevoSocketCpu);
	}
}

void iniciarTablaDePaginacionInvertida(){
	//tablaDePaginacionInvertida = malloc (sizeof(filaDeTablaPaginaInvertida)*getConfigInt("MARCOS"));//falta calcular la cantidadDe filas que tiene la tabla.
	tablaDePaginacionInvertida = malloc(getConfigInt("MARCOS")*sizeof(filaDeTablaPaginaInvertida));
	int i;
	for(i = 0;getConfigInt("MARCOS")> i;i++){
		tablaDePaginacionInvertida[i].frame = i;
		tablaDePaginacionInvertida[i].pid = -1;
		tablaDePaginacionInvertida[i].pagina = -1;


	}

}

int main(void) {



	//



	printf("Inicializando Memoria.....\n\n");
	// ******* Configuracion de la Memoria a partir de un archivo

	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config");
	imprimirConfiguracion();

	// PRUEBAS
	iniciarTablaDePaginacionInvertida();
	printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[0].frame,tablaDePaginacionInvertida[0].pagina,tablaDePaginacionInvertida[0].pid);
	printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[1].frame,tablaDePaginacionInvertida[1].pagina,tablaDePaginacionInvertida[1].pid);
	printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[2].frame,tablaDePaginacionInvertida[2].pagina,tablaDePaginacionInvertida[2].pid);




	// ******* Declaración de la mayoria de las variables a utilizar


	int listener;
	char* mensajeRecibido= string_new();


	sizeOfPaginas=getConfigInt("MARCO_SIZE");
	memoriaTotal = malloc(sizeOfPaginas);


	HeapMetadata header;
	header.isFree= true;
	header.size= sizeOfPaginas-5;
	memcpy(memoriaTotal,&header,tamanoHeader);

	char *memoriav1 = string_new();

	// ******* Conexiones obligatorias y necesarias

	listener = crearSocketYBindeo(getConfigString("PUERTO")); // asignar el socket principal

	pthread_t hilo_AceptarConexionesCPU, hilo_Kernel;

	//semaforoTrucho = -100;
	sem_init(&sem_isKernelConectado,0,0);

	pthread_create(&hilo_Kernel, NULL, rutinaKernel,  listener);
	pthread_create(&hilo_AceptarConexionesCPU, NULL, aceptarConexionesCpu, listener);

	printf("\nMensaje desde la ram principal del programa!\n");

	pthread_join(hilo_Kernel, NULL);
	pthread_join(hilo_AceptarConexionesCPU , NULL);
	close(listener);
	liberarConfiguracion();
	free(memoriav1);
	free(mensajeRecibido);
	free(memoriaTotal);

	return EXIT_SUCCESS;

}

