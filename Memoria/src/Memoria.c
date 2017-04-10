/*
 ============================================================================
 Name        : Memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "commons/config.h"
#include "laGranBiblioteca/config.h"



int main(int argc, char *argv[]) {
	printf("Inicializando Memoria.....\n\n");

	config_Memoria config;

	/*if (argc != 2) {
		    fprintf(stderr,"usage: client hostname\n");
		    exit(1);
	}*/

 	printf("Configuracion Inicial: \n");

    configuracionInicialMemoria("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config",&config);
    imprimirConfiguracionInicialMemoria(config);

    printf("\n\nAca hará lo que la memoria haga: \n");

    return EXIT_SUCCESS;
}
