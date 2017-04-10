/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
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

#include <arpa/inet.h>


typedef struct{
	char * PORT;
	char * PUNTO_MONTAJE;
}config_FileSystem;


void configuracionInicialFileSystem(char*PATH, config_FileSystem* conFs){
	t_config * config;
	config = config_create(PATH);

	conFs->PORT = getStringFromConfig(config,"PUERTO");
	conFs->PUNTO_MONTAJE = getStringFromConfig(config,"PUNTO_MONTAJE");

	config_destroy(config);
}

void imprimirConfiguracionInicialFileSystem(config_FileSystem config){
	printf("PORT: %s\n", config.PORT);
	printf("PUNTO_MONTAJE: %s \n", config.PUNTO_MONTAJE);

}


int main(int argc, char* argv[]) {
	printf("Inicializando FileSystem.....\n\n");
	config_FileSystem config;

	/*if (argc != 2) {
		    printf(stderr,"usage: client hostname\n");
		    exit(1);
		}
	 */

	printf("Configuracion Inicial: \n");
	configuracionInicialFileSystem("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/FileSystem/filesystem.config",&config);
	imprimirConfiguracionInicialFileSystem(config);

	printf("\n\nAca se hará lo que sea que haga el FileSystem:\n");

	return EXIT_SUCCESS;
}
