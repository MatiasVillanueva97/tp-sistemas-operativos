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
#include "commons/config.h"
#include "commons/collections/list.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10// how many pending connections queue will hold

#define ID 0

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



	// ******* Conexiones obligatorias y necesarias del Kernel - FileSystem y Memoria

/*	printf("\n\n\nEsperando conexiones:\n-FileSystem\n-Memoria\n");
	socketMemoria = conexionConServidor(configString("PUERTO_MEMORIA"),configString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria

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


	socketFS = conexionConServidor(configString("PUERTO_FS"),configString("IP_FS")); // Asignación del socket que se conectara con el filesytem
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

	//Delegar aca.



	enviarMensaje(socketFS, 2, (void *)"LA CONCHA DE TU MADRE ROBERTO",  strlen("LA CONCHA DE TU MADRE ROBERTO"));



	// ******* Proceso de conectar al Kernel con otros modulos que le realizen peticiones

	listener = crearSocketYBindeo(configString("PUERTO_PROG"));

	escuchar(listener);	 // Pone el listener (socket principal) a escuchar las peticiones
	FD_SET(listener, &master); // agrega al master el socket

	fdmax = listener; // por algun motivo desconocido por nosotros, el select necesita tener siempre apuntando al ultimo socket del master (el ultimo que se abre)


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
					if ((nbytes = recibirMensaje(socketSeleccionado, mensajeRecibido)) <= 0) { // aca esta el reciv // got error or connection closed by client
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
								if( enviarMensaje(mensajeRecibido,socketAEnviarMensaje)==-1 ){  //valida cosas except the listener and ourselves
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
