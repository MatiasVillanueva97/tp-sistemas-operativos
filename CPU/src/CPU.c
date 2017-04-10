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
#include "laGranBiblioteca/sockets.h"
#include "laGranBiblioteca/config.h"


int main(int argc, char *argv[])
{
	printf("Inicializando CPU.....\n\n");

	config_CPU config;

	/*if (argc != 2){
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}*/

	// ******* Configuracion Inicial de CPU

 	printf("Configuracion Inicial: \n");
	configuracionInicialCPU("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config",&config);
	imprimirConfiguracionInicialCPU(config);

	// ******* Porcesos de la CPU - por ahora , solo un envio de mensajes

	printf("\n\nHola! Soy una cpu!:, conctese con el kernel, la ip y el puerto estn harcodeados, algun dia los ingresara: ");

	char* mensaje = "Este es el menjasaque que enviaremos, aujerooosdasdasdfasd";

	int socket = conexionConKernel(config.PORT,config.IP);

	enviarMensaje(mensaje, socket);

	close(socket);
}
