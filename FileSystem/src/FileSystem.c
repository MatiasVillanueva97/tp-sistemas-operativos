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
#include "commons/config.h"
#include "commons/string.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#define ID 4

int main(void) {
	printf("Inicializando FileSystem.....\n\n");

	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;

	struct sockaddr_storage their_addr; // connector's address information

	int id_cliente, rta_handshake, nuevoSocket, listener;// listen on sock_fd, new connection on new_fd
	int aceptados[] = {0};
	char ip[INET6_ADDRSTRLEN];
	char* mensajeRecibido = string_new();//El valgrind dice que hay un problema acá

	// ******* Configuracion del FileSystem a partir de un archivo

	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/FileSystem/fileSystem.config");
	imprimirConfiguracion();


	// ******* Conexiones obligatorias y necesarias

	listener = crearSocketYBindeo(getConfigString("PUERTO"));
	escuchar(listener);

	sin_size = sizeof their_addr;

	if ((nuevoSocket = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
		perror("Error en el Accept");
	}

	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server

	printf("Conexion con %s\n", ip);


	close(listener); // child doesn't need the listener

	if ((rta_handshake = handshakeServidor(nuevoSocket, ID, aceptados)) == -1) {
		perror("Error con el handshake: -1");
		close(nuevoSocket);
	}

	printf("Conexión exitosa con el Server(%i)!!\n",rta_handshake);


	char* mensaje;
	if(recibirMensaje(nuevoSocket,(void *)mensajeRecibido)==-1){
		perror("Error en el Reciv");
	}
	printf("Mensaje desde el Kernel: %s\n\n", mensaje);

	close(nuevoSocket);  // parent doesn't need this

	liberarConfiguracion();
	// free(mensajeRecibido); esto tira violacion de segmento
	return EXIT_SUCCESS;
}
