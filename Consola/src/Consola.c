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

#include <arpa/inet.h>

//#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define ID 3

typedef struct{
	char *PORT;
	char *IP;
}config_Consola;

// get sockaddr, IPv4 or IPv6:
void configuracionInicial(char*PATH,config_Consola * configConsola){
	t_config *config;
	config = config_create(PATH);
	configConsola->PORT = getStringFromConfig(config,"PUERTO_KERNEL");
	configConsola->IP = getStringFromConfig(config,"IP_KERNEL");
	config_destroy(config);
}

void imprimirConfiguracionInicial(config_Consola config){

	printf("IP_KERNEL: %s\n", config.IP);
	printf("PUERTO_KERNEL: %s \n", config.PORT);

}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	config_Consola config;



	/*if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}*/

	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Consola/consola.config",&config);
	imprimirConfiguracionInicial(config);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(config.IP, config.PORT, &hints, &servinfo)) != 0) {
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
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	printf("handshake cliente%i \n",handshakeCliente(sockfd,ID));



	/*if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}*/

	//buf[numbytes] = '\0';

	//printf("client: received '%s'\n",buf);

	if (send(sockfd, "Patos, world!", 13, 0) == -1){
		perror("send");
	}

	//enviarMensaje("LA RE CONCHA DE TU MADRE RAUL",sockfd);

	close(sockfd);

	return 0;
}
