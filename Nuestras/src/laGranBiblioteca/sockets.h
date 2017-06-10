#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
int aceptarConexiones(int listener, int* nuevoSocket, int procesoQueAcepta, int* aceptados, int cantidadDePermitidos);
/**
* @NAME: aceptarConexiones
* @DESC: Acepta una conexión, realiza el handshake con este y devuelve el id de quien se coneto, y setea en la variable "nuevoSocket" el valor del socket con quien se conecto
* 		 Recibe por parametro el listener de donde escuchara el accept interno, el socketNuevo que devolverá el acept interno, el porceso que llama a la función, por ejemplo,
* 		 Si estoy llamando a esta función en el kernel, le pasaré al momento de llamarla, el id del kernel, y por ultimo la lista de procesos con quienes me puedo conectar,
* 		 siguiendo con el ejemplo, el kernel le pasará el array [1 ,3], porque son los id con quienes puede hacer handshake.
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
int handshakeServidor(int socket,int id, int permitidos[],int cantidadDePermitidos);
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

void enviarPaquete(int socket, int cantInts, int cantStrings, ...);

void recibirPaquete(int socket, ...);

#endif
