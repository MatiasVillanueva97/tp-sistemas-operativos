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
#include <arpa/inet.h>
#include "commons/txt.h"
#include "commons/string.h"
#include "commons/collections/list.h"
#include "../../Nuestras/src/sockets.h"

// aca poner la futura libreria

int main(int argc, char *argv[])
{
	if (argc != 2){
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	printf("Hola! Soy una cpu!:, conctese con el kernel, la ip y el puerto estn harcodeados, algun dia los ingresara: ");

	char* puerto = "3490";
	//char* ip = "127.0.0.1";

	char* mensaje = "Este es el menjasaque que enviaremos, aujerooosdasdasdfasd";

	int socket = conexionConKernel(puerto,argv[1]);

	enviarMensaje(mensaje, socket);

	close(socket);

	return 0;
}
