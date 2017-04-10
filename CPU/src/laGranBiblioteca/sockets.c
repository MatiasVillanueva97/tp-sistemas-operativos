/*
 ============================================================================
 Name        : sockects.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "commons/txt.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include <sys/wait.h>
#include <signal.h>

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BACKLOG 10	 // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:

typedef struct{
	int tipo;
	int cantidad;
}t_mensajeByte;


void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int conexionConServidor(char* puerto, char* ip)
{
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *servinfo;//=malloc(sizeof(struct addrinfo*));
	struct addrinfo *p;///=malloc(sizeof(struct addrinfo*));
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(ip, puerto, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}


	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "CPU: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("CPU: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure


	return sockfd;
}


int crearSocketYBindeo(int puerto)
{
	int sockfd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	int yes=1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, puerto, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
	if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

			break;
		}

		freeaddrinfo(servinfo); // all done with this structure

		if (p == NULL)  {
				fprintf(stderr, "server: failed to bind\n");
				exit(1);
			}

		return sockfd;

}


void escuchar(int sockfd)
{
	int  new_fd,numbytes;  // listen on sock_fd, new connection on new_fd
		struct addrinfo hints, *servinfo, *p;
		struct sockaddr_storage their_addr; // connector's address information
		socklen_t sin_size;
		struct sigaction sa;
		char s[INET6_ADDRSTRLEN];
		char buf[100];


	if (listen(sockfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}

		sa.sa_handler = sigchld_handler; // reap all dead processes
		sigemptyset(&sa.sa_mask); //manejo de errores
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}

		printf("server: waiting for connections...\n");
}


int definirBytesDelMensaje(int header)
{
	switch(header){
		case 0:
			return sizeof(int);
			break;
		case 1:
			return 100;
			break;
		case 2:
			return sizeof(char);
			break;
		case 3:
			return sizeof(float);
			break;
		default:
			return -1;
	}
}

int recibirMensaje(int socket,char* mensaje) // Toda esta funcion deberá ccambiar en el momento qeu definamos el protocolo de paquetes de mensajes :)
{
	int longitud;
	int recibido;
	if ((recibido = recv(socket, &longitud, sizeof(int), 0)) == -1) {
		perror("recv");
		return -1;
		//exit(1);
	}
	if (recibido == 0){
		return 0;
	}

	if (recv(socket, mensaje, longitud, 0) == -1) {
		perror("recv");
		return -1;
		//exit(1);
	}

	*(mensaje + longitud) = '\0';

	return longitud;
}

void enviarMensaje(char* contenido, int socket)
{
	int total = 0;
	int n;
	int bytesleft;
	int longitud = strlen(contenido) + 1;
	bytesleft = longitud;

	if(send(socket, &longitud , sizeof(int), 0) == -1 ){
			perror("recv");
			exit(1);
	}

	while(total < longitud){
		n = send(socket, contenido + total, bytesleft, 0);
		if(n == -1) break;
		total += n;
		bytesleft -= n;
	}
}


int conexionPosible(int id, int permitidos[])
{
	int i;
	for(i = 0; i < 4; i++){
		if(id == permitidos[i])
			return 1;
	}
	return 0;
}

//Esta funcion devuelve el id del Servidor al que se conecta

int handshakeCliente(int socket, int id)
{
	int id_receptor;

	//Se envia el id del cliente al Servidor

	if (send(socket, &id, sizeof(id), 0) == -1){
		perror("send");
		exit(1);
	}

	//Se recibe el id del Servidor

	if ((recv(socket, &id_receptor, sizeof(id_receptor), 0)) == -1) {
			perror("recv");
			exit(1);
	}

	return id_receptor;
}


//Devuelve el id o -1 si rechazo la conexion

int handshakeServidor(int socket,int id, int permitidos[])
{
	int id_emisor;
	int rta;

	//Se recibe el id del emisor mediante la conexion

	if ((recv(socket, &id_emisor, sizeof(id_emisor), 0)) == -1) {
		perror("recv");
		exit(1);
	}

	rta = conexionPosible(id_emisor,permitidos) ? id : -1; // se comprueba que puedan conectarse

	//Se envia el id del servidor al cliente en caso de que se acepte la conexion

	if (send(socket, &rta, sizeof(rta), 0) == -1){
			perror("send");
			exit(1);
	}

	return rta;
}
