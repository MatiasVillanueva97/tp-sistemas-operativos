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
#include "laGranBiblioteca/sockets.h"
#include "laGranBiblioteca/config.h"

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10// how many pending connections queue will hold

#define ID 0

/*
typedef struct
{
	int socket;
	int id_cliente;
}conexion_establecida;
*/

int main(void) {
	printf("Inicializando Kernel.....\n\n");

	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;
	config_Kernel config;

	struct sockaddr_storage their_addr; // connector's address information

	int id_cliente; // listen on sock_fd, new connection on new_fd
	int aceptados[] = { 1, 2, 3, 4 };

	char s[INET6_ADDRSTRLEN];
	char buf[100];

	int socketFS;
	int socketMemoria;

	// Variables para el while que contiene el select
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	fd_set write_fds;

	int fdmax;        // maximum file descriptor number
	int listener;     // listening socket descriptor
	int newfd;
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	// ******* Configuracion del Kernel a partir de un archivo

	printf("Configuracion Inicial: \n");

	configuracionInicialKernel("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config",&config);

	imprimirConfiguracionInicialKernel(config);

	printf("Esperando conexion con File System \n");

	socketFS = conexionConServidor(config.PUERTO_FS,config.IP_FS);
	if (socketFS == 1){
		perror("Falla en el protocolo");
		exit(1);
	}
	if (socketFS == 2){
		perror("Conecta el fileSystem imbecil");
		exit(1);
	}
	int rta;

	if ( (rta=handshakeCliente(socketFS, ID)) == -1) {
			perror("error en el handshake");
			close(socketFS);
	}
	printf("handshake con File System Bien Hecho %i",rta);

	FD_SET(socketFS, &write_fds);




	printf("Esperando conexion con Memoria \n");
	socketMemoria = conexionConServidor(config.PUERTO_MEMORIA,config.IP_MEMORIA);
	if (socketFS == 1){
			perror("Falla en el protocolo");
			exit(1);
		}
		if (socketFS == 2){
			perror("Conecta el fileSystem imbecil");
			exit(1);
		}
	if ( (rta=handshakeCliente(socketMemoria, ID)) == -1) {
				perror("error en el handshake");
				close(socketMemoria);
		}
	printf("handshake con Memoria Bien Hecho %i",rta);

	FD_SET(socketMemoria, &write_fds);

	//Delegar aca.


	printf("\n\n\nEstableciendo Conexiones:\n");

	// ******* Proceso de conectar al Kernel con otros modulos que le realizen peticiones

	listener = crearSocketYBindeo(config.PUERTO_PROG);

	escuchar(listener);
	// añadir la listener para la setear maestro   -add the listener to the master set
	FD_SET(listener, &master);

	// keep track of the biggest file descriptor
	fdmax = listener; // so far, it's this one

	int i = 0, nbytes, j = 0;

	while (1) {
	read_fds = master; // copy it
		if (select(fdmax + 1, &read_fds, NULL, NULL, 0) == -1) {
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // we got one!!
				if (i == listener) {
					// handle new connections
					sin_size = sizeof their_addr;
					newfd = accept(listener, (struct sockaddr *) &their_addr,&sin_size); // Aqui esta el accept

					if (newfd == -1) {
						perror("accept");
					}
					else {
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) {    // keep track of the max
							fdmax = newfd;
						}
						if ((id_cliente = handshakeServidor(newfd, ID, aceptados)) == -1) {
							perror("error en el handshake");
							close(newfd);
						} else {
							printf("selectserver: new connection from %s on ""socket %d\n", inet_ntop(their_addr.ss_family,getSin_Addr((struct sockaddr *)&their_addr),s, INET6_ADDRSTRLEN), newfd);
						}
						if(id_cliente != 0 && id_cliente != 3){ // sea distinto a el kernel o consola
							FD_SET(newfd, &write_fds);
						}
						else{
							FD_SET(newfd, &read_fds);
						}
					}
				}
				else {
					// handle data from a client
					if ((nbytes = recibirMensaje(i, buf)) <= 0) { // aca esta el reciv // got error or connection closed by client
						if (nbytes == 0) { // connection closed
							printf("selectserver: socket %d hung up\n", i);
						} else {
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master);
						FD_CLR(i, &read_fds);// remove from master set
						FD_CLR(i, &write_fds);
						}
					else {
						printf("Recibido de: %s\n", buf);
						// we got some data from a client
						for (j = 0; j <= fdmax; j++) { // send to everyone!
							if (FD_ISSET(j, &write_fds) && j != listener && j != i){
								if( enviarMensaje(buf,j)==-1 ){  //valida cosas except the listener and ourselves
									perror("send");
								}
								FD_CLR(j, &write_fds);
							}
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END while(1)--and you thought it would never end!

	return 0;
}
