/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "commons/config.h"
#include "laGranBiblioteca/sockets.h"
#include "laGranBiblioteca/config.h"

#include <arpa/inet.h>

//#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define ID 3


int main(int argc, char *argv[])
{
	printf("Inicializando Consola.....\n\n");

	config_Consola config;
	struct addrinfo hints, *servinfo, *p;
	int sockfd, numbytes, rv;
	char buf[MAXDATASIZE];
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/*if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}*/

	// ******* Configuración inicial Consola

	printf("Confiruacion Inicial:\n");
	configuracionInicialConsola("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Consola/consola.config",&config);
	imprimirConfiguracionInicialConsola(config);

	// ******* Procesos de Consola-  por ahora enviar mensajitos

	int socket = conexionConServidor(config.PORT,config.IP);

	printf("handshake cliente%i \n",handshakeCliente(socket,ID));

	char* mensaje = string_new();
	printf("Ingrese mensaje a enviar: ");
	fgets(mensaje,100,stdin);
	enviarMensaje(mensaje,socket);

	close(sockfd);
	return 0;
}
