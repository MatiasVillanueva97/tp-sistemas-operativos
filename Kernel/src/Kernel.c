/*
 ** server.c -- a stream socket server demo
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
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include <semaphore.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10// how many pending connections queue will hold

#define ID 0

int socketMemoria;
int socketFS;
int	historico_pcb = 0;

t_queue* colaDeReady ;

sem_t* contadorDeCpus = 0;

enum id_Modulos{
	Kernel = 0,
	CPU = 1,
	Memoria = 2,
	Consola = 3,
	FileSystem = 4
};


typedef struct
{
	int pid;
	int contPags_pcb;
}__attribute__((packed)) PCB_DATA;

typedef struct{
	PCB_DATA pcb;
	int socket;
}pcb_Consola;

t_list* tablaConsolaPcb;
void agregarATablaConsolaPcb(PCB_DATA* pcb, int* socket){
	pcb_Consola pcb_Consola;
	pcb_Consola.pcb = *pcb;
	pcb_Consola.socket = *socket;
	list_add(tablaConsolaPcb, &pcb_Consola);
}

int obtenerSocketConsola(PCB_DATA* pcb){
	bool busqueda(PCB_DATA* pcb2){
		return pcb->pid == pcb2->pid;
	}
	if (!list_any_satisfy(tablaConsolaPcb,busqueda)){
		pcb_Consola* pcbActual =list_find(tablaConsolaPcb,busqueda);
		return pcbActual->socket;
	}
	else
		return -1;
}
void *rutinaCPU(void * arg)
{
	int listener = (int)arg ;
	int socketCPU;
	int aceptados[] = {CPU};
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;

	escuchar(listener); // poner a escuchar ese socket

	if ((socketCPU = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {// estas lines tienen que ser una funcion
		perror("Error en el Accept");
		//continue;
	}
	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server
	printf("[Rutina CPU] - Conexion con %s\n", ip);

	int id_clienteConectado;

	if ((id_clienteConectado = handshakeServidor(socketCPU, ID, aceptados)) == -1) {
		perror("Error con el handshake: -1");
		close(socketCPU);
	}
	printf("[Rutina CPU] - CPU conectado exitosamente\n");


	while(1){  //Villereada
		while(!queue_is_empty(colaDeReady)){
			PCB_DATA *pcbAEjecutar = queue_pop(colaDeReady);
			enviarMensaje(socketCPU,3,pcbAEjecutar,sizeof(PCB_DATA)); // falta hacer este tipo.
			void* resultado = malloc(100);
			int tamano = recibirMensaje(socketCPU, resultado);
			int socketConsola = obtenerSocketConsola(pcbAEjecutar);
			if(socketConsola!= -1){
				enviarMensaje(socketConsola,1,&(pcbAEjecutar->pid),sizeof(int));//esto no estoy seguro si anda
				enviarMensaje(socketConsola,2,resultado,tamano); // falta hacer este tipo.
				printf("Termine la cpu, buscando nuevos procesos para realizar");
			}
			else{
				printf("No pude obtener el socketConsola de la lista de sockets");
				exit(-56);
			}
		}
	}
}


void *rutinaConsola(void * arg)
{
	int listener = (int)arg ;
	int socketConsola;
	int aceptados[] = {CPU};
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;
	escuchar(listener); // poner a escuchar ese socket



	if ((socketConsola = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {// estas lines tienen que ser una funcion
		perror("Error en el Accept");
		//continue;
	}
	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server
	printf("[Rutina Consola] - Conexion con %s\n", ip);

	int id_clienteConectado;

	if ((id_clienteConectado = handshakeServidor(socketConsola, ID, aceptados)) == -1) {
		perror("Error con el handshake: -1");
		close(socketConsola);
	}
	printf("[Rutina Consola] - Consola conectada exitosamente\n");


	char* scripAnsisop = malloc(120);

	// Consola:
	// Nos tiene que mandar un script - chorrrodebytes
	// le devolvemos al pid
	// -------- viene todo el proceso
	// le enviamos el "resultad" , un stream - chorrodebytes

	// CPU:
	// Nosotros le mandamos el pcb a la cpu
	// ---- proceso ---
	// esperamos que nos mande el resultado , con el pid

	int sizeCodigoAnsisop = recibirMensaje(socketConsola, scripAnsisop);

	enviarMensaje(socketMemoria,2,scripAnsisop,sizeCodigoAnsisop); // Le envio el stream a memoria
	enviarMensaje(socketMemoria,1,&sizeCodigoAnsisop,sizeof(int)); // Enviamos el size del stream a memoria

	int ok=0;
	recibirMensaje(socketMemoria, &ok); // Esperamos a que memoria me indique si puede guardar o no el stream
	if(ok)
	{
		printf("\n\nMemoria dio el Ok para el proceso recien enviado\n");
		PCB_DATA pcb;
		historico_pcb++;
		agregarATablaConsolaPcb(&pcb,&socketConsola);
		pcb.pid=historico_pcb; // asigno un pid al pcb
		enviarMensaje(socketMemoria,2,&pcb.pid,sizeof(int)); // Enviamos el pid a memoria

		int nuevo_contPags_pcb;
		if(recibirMensaje(socketMemoria, &nuevo_contPags_pcb)==-1) // REcibimos el numero de pagina o contador de pagina o lo que sea necesario de pagina
			perror("Error en el reciv del contador de paginas del pcb desde memoria");

		pcb.contPags_pcb=nuevo_contPags_pcb; // asigno el contador de pagns al pcb

		printf("Pcb Despues de recibir la pagina y el ok:\n*-id_pcb: %d\n*-contPags_pcb: %d\n\n", pcb.pid, pcb.contPags_pcb);

		queue_push(colaDeReady, &pcb); // agregamos el pcb a la cola de redys
	}
	else
		printf("No hubo espacio para guardar en memoria!\n");

}

void *aceptarConexiones( void *arg ){
	int listener = (int)arg;
	int nuevoSocket;
	int aceptados[] = {CPU, Consola};
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;


	escuchar(listener); // poner a escuchar ese socket

	while(1)
	{
		socklen_t sin_size = sizeof their_addr;

		if ((nuevoSocket = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {// estas lines tienen que ser una funcion
			perror("Error en el Accept");
			//continue;
		}

		inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server
		printf("Conexion con %s\n", ip);

		int id_clienteConectado;
		id_clienteConectado = handshakeServidor(nuevoSocket, ID, aceptados);

		pthread_t hilo_M;

		switch(id_clienteConectado)
		{
			case CPU: // Si el cliente conectado es el cpu
			{
				printf("Entro una CPU\n");
				pthread_create(&hilo_M, NULL, rutinaCPU, nuevoSocket);
			}break;
			case Consola: // Si es un cliente conectado es una CPU
			{
				printf("\nNueva CPU Conectada!\nSocket cpu %d\n\n", nuevoSocket);
				pthread_create(&hilo_M, NULL, rutinaConsola, nuevoSocket);
			}break;
			default:
			{
				printf("Papi, fijate se te esta conectado cualquier cosa\n");
				close(nuevoSocket);
			}
		}
	}
}

void conectarConMemoria()
{
	int rta_conexion;
	socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria
	if (socketMemoria == 1){
			perror("Falla en el protocolo de comunicación");
			exit(1);
	}
	if (socketMemoria == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketMemoria, ID)) == -1) {
				perror("Error en el handshake con Memoria");
				close(socketMemoria);
	}
	printf("Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);
}
void conectarConFS()
{
	int rta_conexion;
	socketFS = conexionConServidor(getConfigString("PUERTO_FS"),getConfigString("IP_FS")); // Asignación del socket que se conectara con el filesytem
	if (socketFS == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketFS == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketFS, ID)) == -1) {
		perror("Error en el handshake con FileSystem");
		close(socketFS);
	}
	printf("Conexión exitosa con el FileSystem(%i)!!\n",rta_conexion);
}

int main(void) {
	printf("Inicializando Kernel.....\n\n");

	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;
	struct sockaddr_storage their_addr; // connector's address information

	int id_cliente, rta_conexion, nbytes, socketAEnviarMensaje = 0, socketSeleccionado = 0;
	int aceptados[] = { 1, 2, 3, 4 };

	char ip_suponemos[INET6_ADDRSTRLEN]; // esto es una ip
	char mensajeRecibido[100];

	// Variables para el while que contiene el select
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	fd_set write_fds;

	int fdmax;        // Maximo numero del FileDescriptor
	int listener;     // Socket principal
	int nuevoSocket;  // Socket donde se asignan las peticiones
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);



	// ******* Configuracion del Kernel a partir de un archivo

	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config");
	imprimirConfiguracion();




	// ******* Conexiones obligatorias y necesarias del Kernel - FileSystem y Memoria y preparar listener para poder oir consolas y cpus

	printf("\n\n\nEsperando conexiones:\n-FileSystem\n-Memoria\n");
	conectarConMemoria();
//	conectarConFS();

	listener = crearSocketYBindeo(getConfigString("PUERTO_PROG"));





	// ******* Recibir datos desde las Consolas -- rutina consola

	pthread_t hilo_rutinaConsola;

	pthread_create(&hilo_rutinaConsola, NULL, rutinaConsola, listener);




	// ******* Enviar Datos a Memoria




	// ******* Planificar procesos para las CPUs

	pthread_t hilo_rutinaCPU;
	pthread_create(&hilo_rutinaCPU, NULL, rutinaConsola, listener);



	pthread_join(hilo_rutinaCPU, NULL);
	pthread_join(hilo_rutinaConsola, NULL);
	liberarConfiguracion();


	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	return 0;
}
