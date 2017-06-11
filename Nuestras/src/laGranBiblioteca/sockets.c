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

typedef struct{
	int tipo;
	int cantidad;
}t_mensajeByte;


void *getSin_Addr(struct sockaddr *sa)
{
	return &(((struct sockaddr_in*)sa)->sin_addr); //IPV4
}

void errorEn(int valor,char * donde){
	if(valor == -1)
		printf("%s\n", donde);
}

int aceptarConexiones(int listener, int* nuevoSocket, int procesoQueAcepta, int* aceptados,int cantidadDePermitidos)
{
	int socketNuevo;
	struct sockaddr_storage their_addr;
	socklen_t sin_size = sizeof their_addr;//No preguntes porque, pero sin esto no anda nada
	char ip[INET6_ADDRSTRLEN];

	socketNuevo = accept(listener, (struct sockaddr *) &their_addr, &sin_size);
	errorEn(nuevoSocket,"ACCEPT");

	//***Toda esta negrada para imprimir una ip
	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip);
	printf("Conexion con %s\n", ip);

	//***Hago el handShake con la nueva conexion
	int id_clienteConectado;
	id_clienteConectado = handshakeServidor(socketNuevo, procesoQueAcepta, aceptados,cantidadDePermitidos);

	*nuevoSocket = socketNuevo;

	return id_clienteConectado;
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
			}
		return sockfd;

}


void escuchar(int sockfd)
{
	if (listen(sockfd, BACKLOG) == -1) {
			perror("listen");
			exit(1);
		}
		printf("\n\nEstableciendo Conexiones:\n\n");
}

int leerInt(void* stream)
{
	int axuluar = *((int*)stream);
	free(stream);
	return axuluar;
}

char* leerString(void * stream)
{
	return (char*)stream;
}


void *deserializador(Header header,int socket)
{
	void* stream = malloc(header.tamano);

	if(recv(socket,stream,header.tamano,0) == -1){
		perror("Error al Recibir mensaje");
	}
	return stream;
}

int recibirMensaje(int socket,void** stream) // Toda esta funcion deberá ccambiar en el momento qeu definamos el protocolo de paquetes de mensajes :)
{
	Header header;
	int cantidad;
	if((cantidad=recv(socket,&header,8,0))==-1){
		perror("Error en el recibir");
	}

	if(cantidad == 0) return 0;

	*stream = malloc(header.tamano); ////ACA HICE UN MALLOC
	void* auxiliarStream = deserializador(header,socket);
	memcpy(*stream, auxiliarStream, header.tamano);
	free(auxiliarStream);

	return header.tipo;
}



//////////////////////////////////////////////////////////////////


void* serializar (int tipoDeOperacion, void* contenido, int tamanioMensaje)
{

	void * stream=malloc(tamanioMensaje+(sizeof(int)*2));
	memcpy(stream, &tipoDeOperacion, sizeof(int));

	memcpy(stream+sizeof(int), &tamanioMensaje, sizeof(int));
	memcpy(stream+sizeof(int)*2, contenido, tamanioMensaje);

	return stream;
}

int enviarMensaje(int socket, int TipoDeOperacion, void* contenido, int tamanioMensaje)
{
	int total = 0;
	int n;
	int bytesleft;
	int longitud = 2*sizeof(uint32_t) + tamanioMensaje;
	bytesleft = longitud;
	void* auxiliar;

	auxiliar = serializar(TipoDeOperacion, contenido, tamanioMensaje);

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


int conexionPosible(int id, int permitidos[],int cantidadDePermitidos)
{
	int i;
	for(i = 0; i<cantidadDePermitidos; i++){
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
int handshakeServidor(int socket,int id, int permitidos[],int cantidadDePermitidos)
{
	int id_cliente;
	int rta;

	//Se recibe el id del emisor mediante la conexion

	if ((recv(socket, &id_cliente, sizeof(id_cliente), 0)) == -1) {
		perror("Error en el recv del HandshakeServidor");
	}

	rta = conexionPosible(id_cliente,permitidos,cantidadDePermitidos) ? id : -1; // se comprueba que puedan conectarse

	//Se envia el id del servidor al cliente en caso de que se acepte la conexion

	if (send(socket, &rta, sizeof(rta), 0) == -1){
			perror("Error en el send del HandshakeServidor");
	}

	if(rta == -1)
		return -1;


	return id_cliente;
}

int calcularTamanoDePaquete(int cantInts, int cantStrings, va_list arguments){
	va_list largoStrings;
	va_copy(largoStrings, arguments);
	//va_start(largoStrings, cantStrings);
	int i;
	int cantCaracteres = 0;
	for(i = 0; i < cantStrings + cantInts; i++){
		if(i > cantInts-1){
			char* aux = va_arg(largoStrings, char*);
			cantCaracteres += strlen(aux) + 1;
		}
		if(i < cantInts){
			int aux = va_arg(largoStrings, int);
			int al =23;
		}

	}
	va_end(largoStrings);


	return (cantInts + cantStrings + 2) * sizeof(int) + cantCaracteres;//Sumo 2 para saber la cantidad de ints y de strings
}

void* serializarPaquete(int tamanoDelPack, int cantInts, int cantStrings, va_list parametros){

	va_list arguments;
	va_copy(arguments, parametros);

	void* stream = malloc(tamanoDelPack);

	int recorrido = 0;
	memcpy(stream, &cantInts, sizeof(int));
	recorrido+= sizeof(int);
	memcpy(stream+recorrido, &cantStrings, sizeof(int));
	recorrido+= sizeof(int);


	int i;
	for(i = 0; i<cantInts; i++){
		int valor = va_arg(arguments, int);
		memcpy(stream+recorrido, &valor, sizeof(int));
		recorrido+= sizeof(int);
	}

	for(; i<cantStrings + cantInts; i++){
		char* copia = va_arg(arguments, char*);
		int largo = strlen(copia) + 1;
		memcpy(stream+recorrido, &largo, sizeof(int));
		recorrido+= sizeof(int);
		memcpy(stream+recorrido, copia, largo);
		recorrido += largo;
	}
	va_end(arguments);

	return stream;

}

int obtenerTamanoProximoBloquePack(void * stream, int *recorrido){
	int tamanoProximoBloque;
	memcpy(&tamanoProximoBloque, stream + *recorrido, sizeof(int));
	*recorrido += sizeof(int);
	return tamanoProximoBloque;
}

int leerINTPack(void * stream, int* recorrido){
	return obtenerTamanoProximoBloquePack(stream, recorrido);
}

void deserializarPaquete(void* stream, va_list parametros){
	int *pos = malloc(sizeof(int));
	*pos = 0;
	int cantInts = leerINTPack(stream, pos);
	int cantStrings = leerINTPack(stream, pos);

	va_list arguments;
	va_copy(arguments, parametros);
	int i;
	for(i=0; i<cantInts; i++){
		int* entero = va_arg(arguments, int*);
		int valor = leerINTPack(stream, pos);
		memcpy(&entero[0], &valor, sizeof(int));
	}

	for(i=0; i<cantStrings; i++){
		int tamanoCadena = leerINTPack(stream, pos);
		char** cadena = va_arg(arguments, char*);
		*cadena = malloc(tamanoCadena);
		memcpy(*cadena, stream + (*pos), tamanoCadena);
		*pos += tamanoCadena;
	}
	free(pos);
	free(stream);
}

void enviarPaquete(int socket,int tipoDeMensaje, int cantInts, int cantStrings, ...){
	va_list arguments;

	va_start(arguments, cantStrings);
		int tamanoDelPack = calcularTamanoDePaquete(cantInts, cantStrings, arguments);
		void* stream = serializarPaquete(tamanoDelPack, cantInts, cantStrings, arguments);
	va_end(arguments);


	enviarMensaje(socket,tipoDeMensaje, stream, tamanoDelPack);
	free(stream);
}

int recibirPaquete(int socket, ...){
	va_list arguments;
	va_start(arguments, socket);
	void * stream;
	int rta = recibirMensaje(socket, &stream);
	deserializarPaquete(stream, arguments);
	va_end(arguments);
	return rta;
}
