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

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10// how many pending connections queue will hold

#define ID 0

typedef struct
{
	int id_pcb;
	int contPags_pcb;
}__attribute__((packed)) PCB_DATA;

int compartida = 0;

void *sumarMilVeces( void *argumento ){
  char *nombre = (char *) argumento;

  int i = 0;
  for (i; i<1000; i++) {
    compartida = compartida + 1;
    printf("%s : %d\n",nombre,compartida);
    //sleep(1);
  }
}

int main(void) {
	printf("Inicializando Kernel.....\n\n");

	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;

	struct sockaddr_storage their_addr; // connector's address information

	int id_cliente, socketFS, socketMemoria, rta_conexion, nbytes, socketAEnviarMensaje = 0, socketSeleccionado = 0;
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

	pthread_t h1, h2, h3, h4, h5;


	pthread_create( &h1, NULL, sumarMilVeces,  "Hilo 1, Gabriel Maiori");

	pthread_create(&h2, NULL, sumarMilVeces,  "Hilo 2, Gabriel Maiori");

	pthread_create(&h3, NULL, sumarMilVeces,  "Hilo 3, Gabriel Maiori");

	pthread_create(&h4, NULL, sumarMilVeces,  "Hilo 4, Gabriel Maiori");

	pthread_create(&h5, NULL, sumarMilVeces,  "Hilo 5, Gabriel Maiori");


	 pthread_join(h1 , NULL);
	 pthread_join(h2 , NULL);
	 pthread_join(h3 , NULL);
	 pthread_join(h4 , NULL);
	 pthread_join(h5 , NULL);





	// ******* Conexiones obligatorias y necesarias del Kernel - FileSystem y Memoria
/*
	printf("\n\n\nEsperando conexiones:\n-FileSystem\n-Memoria\n");
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
	FD_SET(socketMemoria, &write_fds);  // Agregamos el FileDescriptor de la Memoria al set del write (lo ponemos como que al wachin le vamos a escribir)
*/

/*
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

	FD_SET(socketFS, &write_fds); // Agregamos el FileDescriptor del fileSystem al set del write (lo ponemos como que al wachin le vamos a escribir)
*/
	//Delegar aca.


	//int x= 3;
	//enviarMensaje(socketFS, 2,(void*)&x, sizeof(int));

	//enviarMensaje(socketFS, 2,(void*)"hola negro, esto deberia anda", strlen("hola negro, esto deberia anda")+1);









/*
    // 1 - llega un proceso desde la consola
	// 2 - se le mandan los datos a memoria
	// 3 - se recibe el ok de memoria
	// 4 - se la asiga un pcb a esta instruccion
	// 5 - se aunmenta el contador de de paginas en memoria . Esto no se hace aca pero lo pongo igual para no olvidarnos

	// 6 - esto ya es cosa mia, pero me imagino que al cpu hay que indicarle que tiene un procesito para ejecutar pasandole un pcb

	// Primer Parte: Llega un proceso desde la consola . tendria que ir un reciv pero no voy a tocar consola asi que solo voy a trabajar en kernel harcodeando datos

	char *scripAnsisop = string_new();
	scripAnsisop = "0 - scrip hiper rudimentario, imprimir algo por pantalla"; // ahora que estoy probando si leo un 0 es un script en el primer caracter imprimire algo por pantalla

	printf("%s", scripAnsisop);

	int	historico_pcb = 87;
	PCB_DATA pcb;

	pcb.id_pcb=0;
	pcb.contPags_pcb=0;
	historico_pcb++;
	printf("\n\nPcb Inicial:\nid_pcb: %d", pcb.id_pcb);
	printf("\ncontPags_pcb: %d\n\n", pcb.contPags_pcb);

	enviarMensaje(socketMemoria, 2,(void*)scripAnsisop, strlen(scripAnsisop)+1);

	int nuevo_contPags_pcb;


	if(recibirMensaje(socketMemoria, &nuevo_contPags_pcb)==-1)
	{
		perror("Error en el reciv del ok de la memoria");
	}

	pcb.id_pcb=historico_pcb;
	pcb.contPags_pcb=nuevo_contPags_pcb;
	printf("\n\nPcb DesPues de recibir la pagina y el ok:\nid_pcb: %d\ncontPags_pcb: %d\n\n", pcb.id_pcb, pcb.contPags_pcb);

/*
	// ******* Proceso de conectar al Kernel con otros modulos que le realizen peticiones

/*	listener = crearSocketYBindeo(getConfigString("PUERTO_PROG"));

	escuchar(listener);	 // Pone el listener (socket principal) a escuchar las peticiones
	FD_SET(listener, &master); // agrega al master el socket

	fdmax = listener; // por algun motivo desconocido por nosotros, el select necesita tener siempre apuntando al ultimo socket del master (el ultimo que se abre)
*/

	liberarConfiguracion();



	/*while (1) {
		read_fds = master;

		if (select(fdmax + 1, &read_fds, NULL, NULL, 0) == -1) {  // Como pasa por referencia el set de leer, los modifica, por eso hay que setearlos antes
			// aca esta el Select que recibe : el ultimo socket abierto+1, el set de todos los que lee, el set de los que escribe(no implementado), execpciones no implementados y 0 .Cap 7.2 beej en ingles
			perror("Error en el Select");
			exit(4);
		}

		for (socketSeleccionado = 0; socketSeleccionado <= fdmax; socketSeleccionado++) {  // Este for corre mientras este buscando a alguien para leer
			if (FD_ISSET(socketSeleccionado, &read_fds)){ // entra a este if cuando encuentra uno
					if (socketSeleccionado == listener){
					sin_size = sizeof their_addr;
					nuevoSocket = accept(listener, (struct sockaddr *) &their_addr,&sin_size); // Aqui esta el accept

					if (nuevoSocket == -1) {
						perror("Error en el Accept");
					}
					else {
						FD_SET(nuevoSocket, &master); // Se agrega al master el socket creado
						if (nuevoSocket > fdmax) {    // keep track of the max
							fdmax = nuevoSocket;
						}
						if ((id_cliente = handshakeServidor(nuevoSocket, ID, aceptados)) == -1) {
							perror("Error en el handshake");
							close(nuevoSocket);
						}
						else{
							printf("Nueva conexión de:\nIP = %s\nSocket = %d\n", inet_ntop(their_addr.ss_family,getSin_Addr((struct sockaddr *)&their_addr), ip_suponemos, INET6_ADDRSTRLEN), nuevoSocket);
						}
						if(id_cliente != 0 && id_cliente != 3){ // valida si el cliente es o no una consola o el mismo kernel (Quienes NO deben recibir el mensaje)- Para la primer entrega
							FD_SET(nuevoSocket, &write_fds);
						}
						else{
							FD_SET(nuevoSocket, &read_fds);
						}
					}
				}
				else {
					// handle data from a client
					if ((nbytes = recibirMensaje(socketSeleccionado, (void*)mensajeRecibido)) <= 0) { // aca esta el reciv // got error or connection closed by client
						if (nbytes == 0) {  // Solo se cumplira esta condicion cuando se haya cerrado el socket del lado del cliente
							printf("Fin de conexion con socket %d.\n", socketSeleccionado);
						}
						else {
							perror("Error en el Reciv");
						}

						close(socketSeleccionado); // Se cierra el socket Seleccionado

						FD_CLR(socketSeleccionado, &master);
						FD_CLR(socketSeleccionado, &read_fds);// remove from master set
						FD_CLR(socketSeleccionado, &write_fds);
					}
					else {
						printf("Mensaje recibido: %s\n", mensajeRecibido);   // we got some data from a client

						for (socketAEnviarMensaje = 0; socketAEnviarMensaje <= fdmax; socketAEnviarMensaje++) {   // send to everyone!
							if (FD_ISSET(socketAEnviarMensaje, &write_fds) && socketAEnviarMensaje != listener && socketAEnviarMensaje != socketSeleccionado){
								if( enviarMensaje(socketAEnviarMensaje,2,(void*)mensajeRecibido,strlen(mensajeRecibido)+1)==-1 ){  //valida cosas except the listener and ourselves
									perror("send");
									}
								FD_CLR(socketAEnviarMensaje, &write_fds);
								}
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END while(1)--and you thought it would never end!
*/
	return 0;
}
