/*
 * config.c
 *
 *  Created on: 7/4/2017
 *      Author: utnso
 */
#include "commons/config.h"

char* getStringFromConfig(t_config *config, char*valor){
	char* aux = malloc(sizeof(char*));

	if(config_has_property(config, valor)){
		strcpy(aux,config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}
