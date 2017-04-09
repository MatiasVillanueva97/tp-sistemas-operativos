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
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "commons/config.h"
#include "laGranBiblioteca/sockets.h"

#include <arpa/inet.h>


typedef struct{
	char *PORT;
	char *PUNTO_MONTAJE;
}config_FileSystem;


void configuracionInicial(char*PATH, config_FileSystem * configFS){
	t_config *config;
	config = config_create(PATH);
	configFS->PORT = getStringFromConfig(config,"PUERTO");
	configFS->PUNTO_MONTAJE = getStringFromConfig(config,"PUNTO_MONTAJE");
	config_destroy(config);
}

void imprimirConfiguracionInicial(config_FileSystem config){

	printf("PORT: %s\n", config.PORT);
	printf("PUNTO_MONTAJE: %s \n", config.PUNTO_MONTAJE);

}


int main(int argc, char* argv[]) {

	config_FileSystem config;

	if (argc != 2) {
		    printf(stderr,"usage: client hostname\n");
		    exit(1);
		}

	configuracionInicial(argv[1],&config);
	imprimirConfiguracionInicial(config);

	return EXIT_SUCCESS;
}
