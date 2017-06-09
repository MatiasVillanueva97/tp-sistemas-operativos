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

void configuracionInicial(char*PATH);
void imprimirConfiguracion();
void liberarConfiguracion();


//-- Getters de parametros

//recibe una etiqueta y un indice y devuelve el valor en tipo string
char * getConfigStringArrayElement(char * etiqueta, int indice);

//recibe una etiqueta y devuelve su valor en tipo array
char ** getConfigStringArray(char * etiqueta);

//recibe una etiqueta y devuelve el valor en tipo string
char * getConfigString(char * etiqueta);

//recibe una etiqueta y devuelve su valor en tipo int
int getConfigInt(char * etiqueta);

//recibe una etiqueta y un indice y devuelve el valor en tipo int
int getConfigIntArrayElement(char *, int);

//recibe una etiqueta y devuelve la cantidad de elementos que tiene
int getArraySize(char * etiqueta);


//--Setters de parametros

//Sirve para editar un string
void setConfigString(char * etiqueta, char * valor);

//Sirve para editar un string dentro de un array
void setConfigStringArrayElement(char * etiqueta, int indice, char * valor);

//Sirve para editar un int
void setConfigInt(char * etiqueta, int valor);

//Sirve para editar un int dentro de un array
void setConfigIntArrayElement(char * etiqueta, int indice, int valor);

void liberarArray(char ** lista);

// por ahora esto lo voy a dejar esto por aca

void modConfigIntArrayElem(char* etiqueta, int indice, int mod);

void decrementarConfigArray(char* etiqueta, int indice);

void incrementarConfigArray(char* etiqueta, int indice);


#endif /* CARPETA_CONFIG2_H_ */
