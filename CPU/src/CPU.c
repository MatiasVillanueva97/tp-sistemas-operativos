/*
** client.c -- a stream socket client demo
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

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#define ID 1

int main(void)
{
	printf("Inicializando CPU.....\n\n");

	int socketCPU, rta_conexion;
	char* mensajeRecibido =string_new();

	// ******* Configuracion Inicial de CPU

 	printf("Configuracion Inicial: \n");

 	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config");
 	imprimirConfiguracion();

	// ******* Porcesos de la CPU - por ahora solo recibir un mensaje

	printf("\n\nHola! Soy una cpu! Aca estan mis procesos:\n\n");

	socketCPU = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL")); // Asignación del socket que se conectara con el filesytem

	// validacion de un correcto hadnshake
	if (socketCPU == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketCPU == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketCPU, ID)) == -1) {
		perror("Error en el handshake con el Servidor");
		close(socketCPU);
	}
	printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);

	// Recepcion del mensaje
	if(recibirMensaje(socketCPU,(void*)mensajeRecibido)==-1){
		perror("Error en el Reciv");
	}
	printf("Mensaje desde el Kernel: %s\n\n", mensajeRecibido);

	close(socketCPU);
	liberarConfiguracion();
	// free(mensajeRecibido); --- esto tira cviolacion de segmentos
	return 0;
}
