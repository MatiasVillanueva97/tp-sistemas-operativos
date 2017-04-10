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
#include "laGranBiblioteca/sockets.h"
#include "laGranBiblioteca/config.h"


#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10// how many pending connections queue will hold

#define ID 0


int main(void)
{
 	printf("Inizializando Kernel.....\n\n");

 	// ******* Declaración de la mayoria de las variables a utilizar

	socklen_t sin_size;
	config_Kernel config;

	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	struct sigaction sa;

	int socket, new_fd, numbytes, rv;  // listen on sock_fd, new connection on new_fd
	int yes=1;
	int aceptados[] = {1,2,3,4};

	char s[INET6_ADDRSTRLEN];
	char buf[100];

	// Variables para el while que contiene el select
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number
	int listener;     // listening socket descriptor
	int newfd;
	FD_ZERO(&master);    // clear the master and temp sets
 	FD_ZERO(&read_fds);

 	// ******* Configuracion del Kernel a partir de un archivo

 	printf("Configuracion Inicial: \n");

	configuracionInicialKernel("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config",&config);
	imprimirConfiguracionInicialKernel(config);

	printf("\n\n\nEstableciendo Conexiones:\n");


	// ******* Proceso de conectar al Kernel con otros modulos que le realizen peticiones

	listener = crearSocketYBindeo(config.PUERTO_PROG);

	escuchar(listener);
	// añadir la listener para la setear maestro   -add the listener to the master set
	  FD_SET(listener, &master);

	    // keep track of the biggest file descriptor
	    fdmax = listener; // so far, it's this one

	int i=0, nbytes, j=0;

	// *******

	while(1)
	 {
	    read_fds = master; // copy it
	    if (select(fdmax+1, &read_fds, NULL, NULL, 0) == -1) {
	            perror("select");
	            exit(4);
	        }

	        // run through the existing connections looking for data to read
	        for(i = 0; i <= fdmax; i++) {
	            if (FD_ISSET(i, &read_fds)) { // we got one!!
	                if (i == listener) {
	                    // handle new connections
	                	sin_size = sizeof their_addr;
						newfd = accept(listener,
							(struct sockaddr *)&their_addr,
							&sin_size);

						if (newfd == -1) {
	                        perror("accept");
	                    } else {
	                        FD_SET(newfd, &master); // add to master set
	                        if (newfd > fdmax) {    // keep track of the max
	                            fdmax = newfd;
	                        }

	                        int algo;
	                        algo=handshakeServidor(newfd, ID, aceptados);
	                        printf("Respesuesta HdServidor: %d\n", algo);


	                        printf("selectserver: new connection from %s on "
	                            "socket %d\n",
								inet_ntop(their_addr.ss_family,
									get_in_addr((struct sockaddr*)&their_addr),
									s, INET6_ADDRSTRLEN),
								newfd);
	                    }
	                } else {
	                    // handle data from a client
	                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
	                        // got error or connection closed by client
	                        if (nbytes == 0) {
	                    	printf("1 Esto es aabajo de nbytes: %s\n", buf);
	                            // connection closed
	                            printf("selectserver: socket %d hung up\n", i);
	                        } else {
	                            perror("recv");
	                        }
	                    	printf("2 esto es antes del close : %s\n ", buf);
	                        close(i); // bye!
	                        FD_CLR(i, &master); // remove from master set
	                    } else {
	                    	printf("3 esto es dentro del else: %s\n", buf);
	                        // we got some data from a client
	                        for(j = 0; j <= fdmax; j++) {
	                            // send to everyone!
	                            if (FD_ISSET(j, &master)) {
	                                // except the listener and ourselves
	                                if (j != listener && j != i) {
	                                    if (send(j, buf, nbytes, 0) == -1) {
	                                        perror("send");
	                                    }
	                                }
	                            }
	                        }
	                    }
	                } // END handle data from client
	            } // END got new incoming connection
	        } // END looping through file descriptors
	    } // END while(1)--and you thought it would never end!


	/*while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(socket, (struct sockaddr *)&their_addr, &sin_size);

		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		 inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s); // para poder imprimir la ip del server
		 printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(socket); // child doesn't need the listener

			int resHanS;
			if((resHanS=handshakeServidor(new_fd,ID,aceptados)) == -1){
				close(new_fd);
			}

			printf("Respuesta del handsacke del server: %d\n",resHanS);



			if (send(new_fd, "Hello patonC!", 13, 0) == -1){
				perror("send");
				exit(1);
			}
			if ((numbytes = recv(new_fd, buf, 13, 0)) == -1) {
				    perror("recv");
				    exit(1);
			}

			buf[numbytes]= '\0';
			printf("Kernel: received %s \n",buf);

			char* aux = recibir(new_fd);
			printf("mensaje recibido:  %s \n",aux);

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}*/

	return 0;
}
