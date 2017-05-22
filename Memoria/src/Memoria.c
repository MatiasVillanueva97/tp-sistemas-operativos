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
#include "commons/log.h"
#include "commons/string.h"
#include "commons/collections/list.h"


#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"
#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"
#include "EstructurasDeLaMemoria.h"
#include "funcionesDeTablaInvertida.h"
#include "funcionesDeCache.h"
#include "parser/metadata_program.h"
#include "parser/parser.h"
//Variables


sem_t sem_isKernelConectado; // meparece que es otro tipo de semaforo, no mutex



//Función de Hash
int funcionHash (int pid, int pagina){
	return 0; //Falta hacer una funcion de hash
}

//Funciones De Memoria
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
		return 0;
	}
	int frame = buscarFrameCorrespondiente(pid,pagina);
	int posicion = frame*getConfigInt("MARCO_SIZE");
	memcpy(memoriaTotal+posicion,contenido,getConfigInt("MARCO_SIZE"));
	return 1;
}
void imprimirContenidoMemoria(){
	FILE* archivo =  fopen ("file.txt", "w+");
	int i;
	for (i=0;i<cantidadDeMarcos;i++){
		fprintf(archivo, "Contenido de la pagina numero %s \n\n",  string_itoa(i+1));

		fwrite(memoriaTotal+i*sizeOfPaginas,sizeOfPaginas,1,archivo);
		fwrite("\n",1,1,archivo);

	}
	//fwrite(memoriaTotal,sizeOfPaginas,getConfigInt("MARCOS"),archivo);
	fclose(archivo);
}
bool hayEspacio(int pid,int pagina){
	int i;
	for(i=funcionHash(pid,pagina);getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == -1 && filaActual.pid == -1){
			return true;
		}
	}
	return false;
}

//Funciones Principales
void iniciarPrograma(int pid, int paginas){
	int i;
	for(i=1;i<=paginas;i++){
		reservarFrame(pid,i);
	}
	//Falta hacer algo?
}
int almacenarBytesEnPagina(int pid, int pagina, int desplazamiento, int tamano,void* buffer){
	if(desplazamiento + tamano > sizeOfPaginas){
		return -1;
	}
	void *contenidoDeLaPagina = malloc(tamano);
	contenidoDeLaPagina = memoriaTotal+buscarFrameCorrespondiente(pid,pagina)*getConfigInt("MARCO_SIZE");
	memcpy(contenidoDeLaPagina+desplazamiento, buffer,tamano);
	return 1;
}
void* solicitarBytesDeUnaPagina(int pid, int pagina, int desplazamiento, int tamano){
	void* contenidoDeLaPagina = malloc(sizeOfPaginas);
	contenidoDeLaPagina = leerMemoriaPosta(pid,pagina);
	void* contenidoADevolver = malloc(tamano);
	memcpy(contenidoADevolver,contenidoDeLaPagina+desplazamiento,tamano);
	free(contenidoDeLaPagina);
	return contenidoADevolver;

}
int asignarPaginasAUnProceso(int pid, int cantidadDePaginas){
	int paginaMaxima = cantidadDePaginasDeUnProcesoDeUnProceso(pid);
	int i ;
	for(i= 1 ;i <= cantidadDePaginas; i++){
		if(reservarFrame(pid,paginaMaxima+i) == -1){
			finalizarUnPrograma(pid);
			return -1;
		}
	}
	return 0;
}
int finalizarUnPrograma(int pid){
	int paginas = cantidadDePaginasDeUnProcesoDeUnProceso(pid);
	if(paginas == 0){
		return -1;
	}
	int i;
	for(i = 1; i<= paginas;i++){
		liberarPagina(pid,i);
	}
	return 1;

}

//Rutinas
void *rutinaKernel(void *arg){
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

		if ((id_clienteConectado = handshakeServidor(socketKernel, Memoria, aceptados)) == -1) {
			perror("Error con el handshake: -1");
			close(socketKernel);
		}
		printf("[Rutina Kernel] - Kernel conectado exitosamente\n");

		enviarMensaje(socketKernel,enviarTamanoPaginas,&sizeOfPaginas,sizeof(int));

		sem_post(&sem_isKernelConectado);//Semaforo que indica si solo hay un kernel conectado

		recibirMensajesMemoria(socketKernel);
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
void *rutinaConsolaMemoria(void* x){
	size_t len = 0;
		char* mensaje = NULL;
		while(1){
				printf("\nIngrese Comando: \n");
				getline(&mensaje,&len,stdin);
				char** comandoConsola = NULL;
				comandoConsola = string_split(mensaje," ");
				if(strcmp(comandoConsola[0],"retardo") == 0){
					comandoConsola = string_split(comandoConsola[1], "\n");
					retardo = atoi(comandoConsola[0]); // el que hizo esto es un forro c:
					puts(comandoConsola[0]);
					comandoConsola = NULL;
				}
				if(strcmp(comandoConsola[0],"dump") == 0){
					if(strcmp(comandoConsola[1],"memoria\n")== 0){
						imprimirContenidoMemoria();
						continue;

					}
					if(strcmp(comandoConsola[1],"estructuras\n")== 0){
						imprimirTablaDePaginasInvertida();
						continue;

					}
						//imprimirEstructuras();
						//imprimirContenidoCache();
				}

				if(strcmp(comandoConsola[0],"flush\n") == 0){
						continue;
				}
				if(strcmp(comandoConsola[0],"size") == 0){
						if(strcmp(comandoConsola[1],"memoria\n")== 0){
							printf("Su tamaño en frames: %d\n",cantidadDeMarcos);
							int cantidadDeMarcosLibres = memoriaFramesLibres();
							printf("Marcos libres: %d\n", cantidadDeMarcosLibres);
							printf("Marcos ocupado %d\n", cantidadDeMarcos -cantidadDeMarcosLibres);

						}
						if(strcmp(comandoConsola[1],"PID:")== 0){
							comandoConsola = string_split(comandoConsola[2], "\n");
							int pidPedido = atoi(comandoConsola[0]);
							printf("El proceso %d tiene %d\n",pidPedido,cantidadDePaginasDeUnProcesoDeUnProceso(pidPedido));
						}
				}
			}


}

typedef struct{
int pid;
int cantPags;
}t_inicializarPrograma;

typedef struct{
	int pid;
	t_direccion direccion;
	void* buffer;
}t_almacenarBytes;

typedef struct {
	int pid;
	int cantPags;
}t_asignarPaginas;
//Funciones de Conexion
void recibirMensajesMemoria(void* arg){
	int socket = (int)arg;
	void* stream;
	int operacion;
	while (1){

		operacion = recibirMensaje(socket,&stream);

		switch(operacion)
		{
				case inicializarPrograma:{ //inicializarPrograma
					t_inicializarPrograma* estructura = stream;
					int x=1;
					if(asignarPaginasAUnProceso(estructura->pid,estructura->cantPags)==-1){
						x=0;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));

					int t=1;
					char* contenidoPag;
					int rta_escribir_Memoria;

					for(t;t<=estructura->cantPags;t++)
					{
					   recibirMensaje(socket,&contenidoPag);
					   rta_escribir_Memoria=escribirMemoriaPosta(estructura->pid,t,contenidoPag);

					   enviarMensaje(socket,RespuestaBooleanaDeMemoria,&rta_escribir_Memoria,sizeof(int));
					}
					free(contenidoPag);

					sleep(5);

					break;
				}
				case solicitarBytes :{ //solicitar bytes de una pagina
					t_pedidoMemoria* estructura = stream;
					void* contenidoDeLaPagina= solicitarBytesDeUnaPagina(estructura->id,estructura->direccion.page,estructura->direccion.offset,estructura->direccion.size);
					enviarMensaje(socket,lineaDeCodigo,contenidoDeLaPagina,estructura->direccion.size);
					//cambiar por linea de codigo (enum)
					//Controla errores forro.
					break;
				}

				case almacenarBytes://almacenarBytes en una pagina
				{
					t_almacenarBytes* estructura = stream;
					int x=1;
					if(almacenarBytesEnPagina(estructura->pid,estructura->direccion.page,estructura->direccion.offset,estructura->direccion.size,estructura->buffer)==-1){
						x=-1;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));

					break;
				}
				case asignarPaginas:{//PedirMasPaginas
					t_asignarPaginas* estructura = stream;
					int x=1;
					if(asignarPaginasAUnProceso(estructura->pid,estructura->cantPags)==-1){
							x=-1;
					}
					break;
				}
				case finalizarPrograma:{//FinalzarPrograma

					printf("finalizar PRograma entroasjdfasjfas \n");

					int* pid = stream;
					int x=1;
					if(finalizarUnPrograma(*pid)){
							x=-1;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));
					break;
				}
				default:{
					perror("Error de comando");
				}
		}


	}
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
		if ((id_clienteConectado = handshakeServidor(nuevoSocketCpu, Memoria, aceptados)) == -1) {
			perror("Error con el handshake: -1");
			close(nuevoSocketCpu);
		}
		printf("[AceptarConexionesCPU] - Nueva CPU Conectada! Socket CPU: %d\n", nuevoSocketCpu);

		pthread_create(&hilo_nuevaCPU, NULL, recibirMensajesMemoria,  nuevoSocketCpu);
	}
}


int main(void) {
	//
	printf("Inicializando Memoria.....\n\n");



	// ******* Configuracion de la Memoria a partir de un archivo
	cache=list_create();
	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config");
	imprimirConfiguracion();
	retardo = getConfigInt("RETARDO_MEMORIA");
	sizeOfPaginas=getConfigInt("MARCO_SIZE");
	cantidadDeMarcos = getConfigInt("MARCOS");
	memoriaTotal = malloc(sizeOfPaginas*cantidadDeMarcos);
	int i;
	char hijodeputa = ' ';
	for(i=0;i<sizeOfPaginas*cantidadDeMarcos;i++){
		memcpy(memoriaTotal+i,&hijodeputa,1);
	}
	// PRUEBAS
	iniciarTablaDePaginacionInvertida();

	//printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[0].frame,tablaDePaginacionInvertida[0].pagina,tablaDePaginacionInvertida[0].pid);
	//printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[1].frame,tablaDePaginacionInvertida[1].pagina,tablaDePaginacionInvertida[1].pid);
	//printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[2].frame,tablaDePaginacionInvertida[2].pagina,tablaDePaginacionInvertida[2].pid);
	/*reservarFrame(0,0);
	reservarFrame(0,1);
	int x= 15481;
	char* mensaje1 = string_new();
	string_append(&mensaje1, "hola como andas");
	char* mensaje2 ="hola infeliz";
	almacenarBytesEnPagina(0,0,0,strlen(mensaje2),mensaje2);
	almacenarBytesEnPagina(0,1,25,strlen(mensaje1),mensaje1);
	almacenarBytesEnPagina(0,0,15,sizeof(int),&x);
	char* y = solicitarBytesDeUnaPagina(0, 0, 0, strlen(mensaje1));

	char* w = solicitarBytesDeUnaPagina(0, 1, 0, strlen(mensaje1));
	puts(w);
*/
	/*asignarPaginasAUnProceso(23,23);
	printf("cantidad de paginas : %d ", cantidadDePaginasDeUnProcesoDeUnProceso(23));
	size_t len = 0;

	char* mensaje = NULL;
	int* z = solicitarBytesDeUnaPagina(0, 0, 4, 4);
*/
	// ******* Declaración de la mayoria de las variables a utilizar

	pthread_t hilo_consolaMemoria;
	pthread_create(&hilo_consolaMemoria, NULL, rutinaConsolaMemoria,  NULL);
	//pthread_join(hilo_consolaMemoria, NULL);
	int listener;
	//char* mensajeRecibido= string_new();


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
	free(memoriaTotal);

	return EXIT_SUCCESS;

}

