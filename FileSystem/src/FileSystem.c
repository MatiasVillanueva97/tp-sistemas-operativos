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

#include <arpa/inet.h>


typedef struct{
	char *PORT;
	char *PUNTO_MONTAJE;
}config_FileSystem;

char* getStringFromConfig(t_config *config, char*valor){
	char* aux = malloc(sizeof(char*));

	if(config_has_property(config, valor)){
		strcpy(aux,config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}

void configuracionInicial(char*PATH, config_FileSystem * configFS){
	t_config *config;
	config = config_create(PATH);
	configFS->PORT = getStringFromConfig(config,"PUERTO");
	configFS->PUNTO_MONTAJE = getStringFromConfig(config,"PUNTO_MONTAJE");
	config_destroy(config);
}


int main(int argc, char* argv[]) {

	config_FileSystem config;

	if (argc != 2) {
		    fprintf(stderr,"usage: client hostname\n");
		    exit(1);
		}

	configuracionInicial(argv[1],&config);


	return EXIT_SUCCESS;
}
