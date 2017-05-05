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
int pcbHistorico = 0;
t_queue* colaDeReady ;
sem_t* contadorDeCpus = 0;

typedef enum {
	Kernel, // 0
	CPU, // 1
	Memoria, // 2
	Consola, // 3
	FileSystem, //  4
}modulos;


typedef struct
{
	int id_pcb;
	int contPags_pcb;
}__attribute__((packed)) PCB_DATA;



void *rutinaCPU(void * arg)
{
	int socketCPU = ((int*)arg)[0];
	while(1){//Villereada
		while(!queue_is_empty(colaDeReady)){
			PCB_DATA *pcbAEjecutar = queue_pop(colaDeReady);
			enviarMensaje(socketCPU,3,pcbAEjecutar,sizeof(PCB_DATA)); // falta hacer este tipo.
			recibirMensaje(socketCPU, pcbAEjecutar);
		}
	}
}
void *rutinaConsola(void * arg)
{
	int socketConsola = ((int*)arg)[0] ;
	char* mensajeDeConsola = malloc(20);
	recibirMensaje(socketConsola, mensajeDeConsola);
	enviarMensaje(socketMemoria,2,mensajeDeConsola,strlen(mensajeDeConsola)+1);
	free(mensajeDeConsola);
	//aca spisso dice que van los diccionarios.
	char* respuestaDeMemoria = malloc(3);
	recibirMensaje(socketMemoria,respuestaDeMemoria);
	if(strcmp(respuestaDeMemoria,"Ok")== 0){
		PCB_DATA pcb;
		pcb.id_pcb = pcbHistorico;
		pcbHistorico++;
		enviarMensaje(socketMemoria,2,&pcb,sizeof(pcb));
		enviarMensaje(socketConsola,1,&(pcb.id_pcb),sizeof(int));
		queue_push(colaDeReady,&pcb);
		sem_wait(contadorDeCpus);
		sem_post(contadorDeCpus);


		//wait();
		//signal();
	}
	else{
		enviarMensaje(socketConsola,1,-1,sizeof(int));
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

	int	historico_pcb = 0;
	socklen_t sin_size;
	modulos id_mod;
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
	conectarConFS();

	listener = crearSocketYBindeo(getConfigString("PUERTO_PROG"));
//	escuchar(listener);	 // Pone el listener (socket principal) a escuchar las peticiones





	// ******* Recibir datos desde las Consolas

	char *scripAnsisop = string_new();
	scripAnsisop = "Soy un codigo AsiSop"; // me llega esto dsede la consola







	// ******* Enviar Datos a Memoria

	int sizeCodigoAnsisop= strlen(scripAnsisop)+1;
	enviarMensaje(socketMemoria,2,scripAnsisop,sizeCodigoAnsisop); // Le envio el stream a memoria

	int ok=0;
	recibirMensaje(socketMemoria, &ok); // Esperamos a que memoria me indique si puede guardar o no el stream
	if(ok)
	{
		printf("\n\nMemoria dio el Ok para el proceso recien enviado\n");
		PCB_DATA pcb;
		historico_pcb++;

		pcb.id_pcb=historico_pcb; // asigno un pid al pcb
		enviarMensaje(socketMemoria,2,&pcb.id_pcb,sizeof(int)); // Enviamos el pid a memoria

		enviarMensaje(socketMemoria,1,&sizeCodigoAnsisop,sizeof(int)); // Enviamos el size del stream a memoria

		int nuevo_contPags_pcb;
		if(recibirMensaje(socketMemoria, &nuevo_contPags_pcb)==-1) // REcibimos el numero de pagina o contador de pagina o lo que sea necesario de pagina
		{
			perror("Error en el reciv del contador de paginas del pcb desde memoria");
		}

		pcb.contPags_pcb=nuevo_contPags_pcb; // asigno el contador de pagns al pcb

		printf("Pcb Despues de recibir la pagina y el ok:\n*-id_pcb: %d\n*-contPags_pcb: %d\n\n", pcb.id_pcb, pcb.contPags_pcb);
	}







	// ******* Planificar procesos para las CPUs







	liberarConfiguracion();
	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	return 0;
}
