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

#include "datosGobalesGenerales.h"

#ifndef LAGRANBIBLIOTECA_SOCKETS_H_
#define LAGRANBIBLIOTECA_SOCKETS_H_

typedef struct{
	uint32_t tipo;
	uint32_t tamano;
} __attribute__((packed))
Header;

int leerInt(void* stream);

char* leerString(void * stream);

void sigchld_handler(int s);
/**
* @NAME: getSin_Addr
* @DESC: Crea un string vacio
*/
void *getSin_Addr(struct sockaddr *sa);
/**
* @NAME: conexionConServidor
* @DESC: Crea un string vacio
*/
int conexionConServidor(char* puerto, char* ip);
/**
* @NAME: crearSocketYBindeo
* @DESC: Crea un string vacio
*/
int crearSocketYBindeo(char* puerto);
/**
* @NAME: escuchar
* @DESC: Crea un string vacio
*/
void escuchar(int);

/**
* @NAME: recibirMensaje
* @DESC: Crea un string vacio
*/
int recibirMensaje(int socket,void** stream); // Toda esta funcion deberá ccambiar en el momento qeu defininamos el protocolo de paquetes de mensajes :)
/**
* @NAME: enviarMensaje
* @DESC: Crea un string vacio
*/
int enviarMensaje(int socket, int tipoMensaje, void* contenido, int tamanioMensaje);
/**
* @NAME: handshakeCliente
* @DESC: Crea un string vacio
*/
int handshakeCliente(int socket, int id);
/**
* @NAME: handshakeServidor
* @DESC: Crea un string vacio
*/
int handshakeServidor(int socket,int id, int permitidos[]);
/**
* @NAME: deserializador
* @DESC: Crea un string vacio
*/
void *deserializador(Header header,int socket);
/**
* @NAME: serializar
* @DESC: Crea un string vacio
*/
void* serializar (int tipoMensaje, void* contenido, int tamanioMensaje);

#endif
