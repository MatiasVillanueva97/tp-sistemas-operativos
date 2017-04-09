/*
 * config.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */
#include "commons/config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
char* getStringFromConfig(t_config *config, char*valor){
	char* aux = malloc(sizeof(char*));

	if(config_has_property(config, valor)){
		strcpy(aux,config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}
void imprimirArraysDeStrings(char** lista)
{
	printf("[ %s",*lista);
	while(*(lista+1) != NULL) //Mostrar a gabi; //lenguaje de mierda.
	{
		lista++;
		printf(", %s ", *lista);
	}
	printf(" ]\n");
}



