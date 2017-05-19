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
#include "sockets.h"


#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BACKLOG 10	 // how many pending connections queue will hold


#define CASO_OK 0
#define CASO_INT 1
#define CASO_DINAMICO 2



// get sockaddr, IPv4 or IPv6:

typedef struct{
	int tipo;
	int cantidad;
}t_mensajeByte;

/*
void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}*/


void *getSin_Addr(struct sockaddr *sa)
{
	/*if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);*/

	return &(((struct sockaddr_in*)sa)->sin_addr); //IPV4
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

	inet_ntop(p->ai_family, getSin_Addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("Conexion con ip %s - ", s);

	freeaddrinfo(servinfo); // all done with this structure


	return sockfd;
}


int crearSocketYBindeo(char* puerto)
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
	if (listen(sockfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}

		/*sa.sa_handler = sigchld_handler; // reap all dead processes
		sigemptyset(&sa.sa_mask); //manejo de errores
		sa.sa_flags = SA_RESTART;
		if (sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror("sigaction");
			exit(1);
		}
		*/
		printf("\n\nEstableciendo Conexiones:\n\n");
}


void *deserializador(Header header,int socket)
{
	void* contenido;
	int* numero=malloc(sizeof(uint32_t));
	int* yes =malloc(sizeof(uint32_t));

	switch(header.tipo)
	{
		case CASO_OK:
		{
			*yes=0;
			return yes; // Vos no tenes que entender esto, dejalo. DIE' de  DIE'
		}
		case CASO_INT: //Devuelve un int, deberiamos hacer un case por cada estrutuctura que querramos recibir con tamaño fijo.
		{
			if(recv(socket,numero,4,0) == -1){
				perror("Error al Recibir entero");
				break;
			}
			return numero;
		}
		case CASO_DINAMICO: // en este caso agarramos todo el contenido de cualquie chorro de bytes y lo devolvemos asi como vino sin asco,
		{
			contenido = malloc(header.tamano);
			if(recv(socket,contenido,header.tamano,0)==-1){
				perror("Error al recibir mensajes dinamicos");
				break;
			}
			return contenido;
		case 3:{
					struct{
						int pid;
						int contPags_pcb;
					}__attribute__((packed)) *PCB_DATA = malloc(sizeof(int)*2);
					if(recv(socket,&PCB_DATA->pid,sizeof(int),0)==-1){
						perror("Error al recibir mensajes dinamicos");
						break;
					}
					if(recv(socket,&PCB_DATA->contPags_pcb,sizeof(int),0)==-1){
						perror("Error al recibir mensajes dinamicos");
						break;
					}
					return PCB_DATA;
				}
				case 4:{
					struct{
						int pid;
						char* mensaje;
					}*paraImprimir = malloc(header.tamano);
					if(recv(socket,&paraImprimir->pid,sizeof(int),0)==-1){
						perror("Error al recibir mensajes dinamicos");
						break;
					}
					if(recv(socket,&paraImprimir->mensaje,header.tamano-sizeof(int),0)==-1){
						perror("Error al recibir mensajes dinamicos");
						break;
					}
					return paraImprimir;
				}
		}
		default:
			perror("Error al recibir mensajes");
			exit(-1);
	}
	return NULL;
}

int recibirMensaje(int socket,void* stream) // Toda esta funcion deberá ccambiar en el momento qeu definamos el protocolo de paquetes de mensajes :)
{
	Header header;
	int cantidad;
	if((cantidad=recv(socket,&header,8,0))==-1){
		perror("Error en el recibir");
	}

	if(cantidad == 0) return 0;

	stream = malloc(header.tamano); ////ACA HICE UN MALLOC
	memcpy(stream, deserializador(header,socket), header.tamano);
	return header.tamano;
}



//////////////////////////////////////////////////////////////////

// stream tiene: """ un int con su tipo (4 bytes) + un int que dice el tanaño (4bytes) """ + *(puede estar o no) el contenido que peude ser variable o no

//

void* serializar (int tipoMensaje, void* contenido, int tamanioMensaje)
{

	void * stream=malloc(tamanioMensaje+(sizeof(int)*2));
	memcpy(stream, &tipoMensaje, sizeof(int));

	switch(tipoMensaje)
	{
		case CASO_OK: // el caso de una respuesta, que solo da a entender una validacion por ok
		{
			memcpy(stream+sizeof(int), &tamanioMensaje, sizeof(int));
		}break;
		case CASO_INT: /// 3-150
		{
			memcpy(stream+sizeof(int), &tamanioMensaje, sizeof(int));
			memcpy(stream+(sizeof(int)*2), contenido, sizeof(int));
		}break;
		case CASO_DINAMICO:
		{
			memcpy(stream+sizeof(int), &tamanioMensaje, sizeof(int));
			memcpy(stream+(sizeof(int)*2), contenido, tamanioMensaje);
		}break;
		case 3:{
				memcpy(stream+sizeof(int), &tamanioMensaje, sizeof(int));
				memcpy(stream+(sizeof(int)*2), contenido, tamanioMensaje);
				break;
			}
			case 4:{
				memcpy(stream+sizeof(int), &tamanioMensaje, sizeof(int));
				memcpy(stream+(sizeof(int)*2), contenido, tamanioMensaje);
				break;
			}
		default:{
			perror("Error al serializar el chorro de bytes");
			exit(-1);
		}break;


	}
	return stream;
}

int enviarMensaje(int socket, int tipoMensaje, void* contenido, int tamanioMensaje)
{
	int total = 0;
	int n;
	int bytesleft;
	int longitud = 2*sizeof(uint32_t) + tamanioMensaje;
	bytesleft = longitud;
	void* auxiliar;
	auxiliar = serializar(tipoMensaje, contenido, tamanioMensaje);
	/*if(send(socket, auxiliar , ((2*sizeof(uint32_t))+tamanioMensaje), 0) == -1 ){
			perror("recv");
			return -1;
	}*/
	while(total < longitud){ /// NO OLVIDAR //// 32 añso 1974
		n = send(socket, auxiliar + total, longitud - total,0);
		if(n == -1)
			return -1;
		total += n;
		bytesleft -= n;
	}

	free(auxiliar);

	return 0;
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
	int id_servidor;

	//Se envia el id del cliente al Servidor
	if (send(socket, &id, sizeof(id), 0) == -1)
		perror("send");


	//Se recibe el id del Servidor

	if ((recv(socket, &id_servidor, sizeof(id_servidor), 0)) == -1)
			perror("recv");

	if(id_servidor == -1){
		perror("No se pudo establecer la conexcion: id_receptor=-1");
		close(socket);
	}

	return id_servidor;
}

//Devuelve el id del cliente con quien se conecto
int handshakeServidor(int socket,int id, int permitidos[])
{
	int id_cliente;
	int rta;

	//Se recibe el id del emisor mediante la conexion

	if ((recv(socket, &id_cliente, sizeof(id_cliente), 0)) == -1) {
		perror("recv");
		exit(1);
	}

	rta = conexionPosible(id_cliente,permitidos) ? id : -1; // se comprueba que puedan conectarse

	//Se envia el id del servidor al cliente en caso de que se acepte la conexion

	if (send(socket, &rta, sizeof(rta), 0) == -1){
			perror("send");
			exit(1);
	}

	if(rta == -1)
		return -1;


	return id_cliente;
}
