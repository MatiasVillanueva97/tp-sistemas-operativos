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
#include "laGranBiblioteca/sockets.h"
#include "laGranBiblioteca/config.h"
#define ID 4

int main(int argc, char* argv[]) {
	printf("Inicializando FileSystem.....\n\n");
	config_FileSystem config;
	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;

	struct sockaddr_storage their_addr; // connector's address information

	int id_cliente; // listen on sock_fd, new connection on new_fd
	int aceptados[] = {0};
	int numbytes;
	char s[INET6_ADDRSTRLEN];
	char buf[100];

	int socketFS;
	int socketMemoria;

	//Variables para el while que contiene el select
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	fd_set write_fds;

	int fdmax;        // maximum file descriptor number
	int listener;     // listening socket descriptor
	int newfd;
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	/*if (argc != 2) {
	 printf(stderr,"usage: client hostname\n");
	 exit(1);
	 }
	 */

	int new_fd;
	printf("Configuracion Inicial: \n");
	configuracionInicialFileSystem(
			"/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/FileSystem/fileSystem.config",
			&config);
	imprimirConfiguracionInicialFileSystem(config);

	listener = crearSocketYBindeo(config.PORT);
	escuchar(listener);

		sin_size = sizeof their_addr;
		if ((new_fd = accept(listener, (struct sockaddr *) &their_addr,
				&sin_size)) == -1) {
			perror("accept");
		}
		inet_ntop(their_addr.ss_family,
				getSin_Addr((struct sockaddr *) &their_addr), s, sizeof s); // para poder imprimir la ip del server
		printf("server: got connection from %s\n", s);
		 // this is the child process
			close(listener); // child doesn't need the listener
			int resHanS;
			if ((resHanS = handshakeServidor(new_fd, ID, aceptados)) == -1) {
				close(new_fd);
			}
			printf("Respuesta del handsacke del server: %d\n", resHanS);
			char* mensaje= string_new();
			//while(strcmp(mensaje, "fin")){
				recibirMensaje(new_fd,mensaje);
				printf("Mensaje del kernel %s\n", mensaje);
			//}

			close(new_fd);
			exit(0);

		close(new_fd);  // parent doesn't need this


	return EXIT_SUCCESS;
}
