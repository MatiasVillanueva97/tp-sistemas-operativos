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


#define MAXDATASIZE 100 // max number of bytes we can get at once
#define ID 3


int main(void)
{
	printf("Inicializando Consola.....\n\n");

	int socketConsola, rta_conexion;
	char* mensaje =malloc(100);
	//char s[INET6_ADDRSTRLEN];


	// ******* Configuración inicial Consola

	printf("Configuracion Inicial:\n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Consola/consola.config");
	imprimirConfiguracion();


	// ******* Procesos de Consola-  por ahora enviar mensajitos

	socketConsola = conexionConServidor(configString("PUERTO_KERNEL"),configString("IP_KERNEL"));

	// validacion de un correcto hadnshake
	if (socketConsola == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketConsola == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketConsola, ID)) == -1) {
		perror("Error en el handshake con el Servidor");
		close(socketConsola);
	}
	printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);


	printf("\n\nIngrese mensaje a enviar: ");
	fgets(mensaje,100,stdin);

	// Envio del mensaje
	if(enviarMensaje(mensaje,socketConsola)==-1){
		perror("Error en el Send");
	}

	close(socketConsola);
	free(mensaje);
	liberarConfiguracion();
	return 0;
}
