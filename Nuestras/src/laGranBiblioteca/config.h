/*
 * config2.h
 *
 *  Created on: 22/4/2017
 *      Author: utnso
 */
#include "commons/config.h"
#include "commons/collections/list.h"

#ifndef LAGRANBIBLIOTECA_CONFIG_H_
#define LAGRANBIBLIOTECA_CONFIG_H_


t_list * listaDeConfig;//Variable global para guardar las configuraciones

void configuracionInicial(char*);
void imprimirConfiguracion();
void liberarConfiguracion();

//recibe una etiqueta y un indice y devuelve el valor en tipo string
char * configStringArrayElement(char *, int);

//recibe una etiqueta y devuelve su valor en tipo array
char ** configStringArray(char *);

//recibe una etiqueta y devuelve el valor en tipo string
char * configString(char *);

//recibe una etiqueta y devuelve su valor en tipo int
int configInt(char *);

//recibe una etiqueta y un indice y devuelve el valor en tipo int
int configIntArrayElement(char *, int);

#endif /* CARPETA_CONFIG2_H_ */
