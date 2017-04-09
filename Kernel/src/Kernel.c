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

typedef struct {
	char * PUERTO_PROG;
	char * PUERTO_CPU;
	char * IP_MEMORIA;
	char * PUERTO_MEMORIA;
	char * IP_FS;
	char * PUERTO_FS;
	char * QUANTUM;
	char * QUANTUM_SLEEP;
	char * ALGORITMO;
	char * GRADO_MULTIPROG;
	char ** SEM_IDS;
	char ** SEM_INIT;
	char ** SHARED_VARS;
	char * STACK_SIZE;
}config_Kernel;

void configuracionInicial(char*PATH, config_Kernel * est){
	t_config * config;
	config = config_create(PATH);
	est->PUERTO_PROG = getStringFromConfig(config,"PUERTO_PROG");
	est->PUERTO_CPU = getStringFromConfig(config,"PUERTO_CPU");
	est->IP_MEMORIA = getStringFromConfig(config,"IP_MEMORIA");
	est->PUERTO_MEMORIA = getStringFromConfig(config,"PUERTO_MEMORIA");
	est->IP_FS = getStringFromConfig(config,"IP_FS");
	est->PUERTO_FS = getStringFromConfig(config,"PUERTO_FS");
	est->QUANTUM = getStringFromConfig(config,"QUANTUM");
	est->QUANTUM_SLEEP = getStringFromConfig(config,"QUANTUM_SLEEP");
	est->ALGORITMO = getStringFromConfig(config,"ALGORITMO");
	est->GRADO_MULTIPROG = getStringFromConfig(config,"GRADO_MULTIPROG");
	est->SEM_IDS = config_get_array_value(config,"SEM_IDS");
	est->SEM_INIT = config_get_array_value(config,"SEM_INIT");
	est->SHARED_VARS = config_get_array_value(config,"SHARED_VARS");
	est->STACK_SIZE = getStringFromConfig(config,"STACK_SIZE");

	config_destroy(config);
}

void imprimirConfiguracionInicial(config_Kernel config) // Yo gabriel maiori, dije explicitamente que esto es una terrible NEGRADA, pero como yo soy el tosco del team, no puedo quejarme
{
	printf("PUERTO_PROG: %s \n", config.PUERTO_PROG);
	printf("PUERTO_CPU: %s \n", config.PUERTO_CPU);
	printf("IP_MEMORIA: %s \n", config.IP_MEMORIA);
	printf("PUERTO_MEMORIA: %s \n", config.PUERTO_MEMORIA);
	printf("IP_FS: %s \n", config.IP_FS);
	printf("PUERTO_FS: %s \n", config.PUERTO_FS);
	printf("QUANTUM: %s \n", config.QUANTUM);
	printf("QUANTUM_SLEEP: %s \n", config.QUANTUM_SLEEP);
	printf("ALGORITMO: %s\n", config.ALGORITMO);
	printf("GRADO_MULTIPROG: %s \n", config.GRADO_MULTIPROG);


	printf("SEM_IDS: ");

	imprimirArraysDeStrings(config.SEM_IDS);
	printf("SEM_INIT: ");

	imprimirArraysDeStrings(config.SEM_INIT);
	printf("SHARED_VARS: ");

	imprimirArraysDeStrings(config.SHARED_VARS);


	printf("STACK_SIZE: %s \n", config.STACK_SIZE);
}



int main(void)
{
	int socket, new_fd,numbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[100];
	int aceptados[] = {1,2,3,4};

	config_Kernel config;

	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config",&config);

	imprimirConfiguracionInicial(config);

	socket=crearSocketYBindeo(config.PUERTO_PROG);

	escuchar(socket);

	while(1) {  // main accept() loop
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
			if(resHanS=(handshakeServidor(new_fd,ID,aceptados)) == -1){
				close(new_fd);
			}

			printf("Respuesta del handsacke del server: %d\n",resHanS);



			if (send(new_fd, "Hello patonC!", 13, 0) == -1){
				perror("send");
				exit(1);
			}
			if ((numbytes = recv(new_fd, buf, 100-1, 0)) == -1) {
				    perror("recv");
				    exit(1);
			}

			buf[numbytes]= '\0';
			printf("Kernel: received '%s'\n",buf);

			char* aux;
			aux = recibir(new_fd);
			printf("mensaje recibido:  %s",aux);

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}
