/*
 * config2.c

 *
 *  Created on: 22/4/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"
#include "commons/collections/list.h"
#include "config.h"


char* getStringFromConfig(t_config *config, char*valor){
	char* aux;

	if(config_has_property(config, valor)){
		aux = string_duplicate(config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}


typedef struct{
	char * etiqueta;
	void* contenido;
	int tipo;//0 = char*, 1=char**
}parametro;


void liberarArray(char** array){
 int i= 0;
 while(array[i]!= NULL){
  free(array[i]);
  i++;
 }
 free(array);
}

void liberarParametro(parametro * param){
/*
	if(param->tipo == 0)
		free(param->contenido);
	else
		liberarArray(param->contenido);*/
	free(param->contenido);
	free(param->etiqueta);
	free(param);
}

parametro * obtenerParametroString(t_config * config, char * etiqueta){
	parametro * parametro = malloc(sizeof(*parametro));
	parametro->contenido = getStringFromConfig(config,etiqueta);
	parametro->tipo = 0;
	parametro->etiqueta = etiqueta;
	return parametro;
}
parametro * obtenerParametroArray(t_config * config, char * etiqueta){
	parametro * parametro = malloc(sizeof(*parametro)) ;
	parametro->contenido = config_get_array_value(config,etiqueta);
	parametro->tipo = 1;
	parametro->etiqueta = etiqueta;
	return parametro;
}

parametro * buscarParametro(char * etiqueta){
	bool busqueda(parametro * parametro){
		return strcmp(etiqueta, parametro->etiqueta) == 0;
	}
	parametro * par = list_find(listaDeConfig, busqueda);
	return par;
}

//-------
char * configStringArrayElement(char * etiqueta, int indice){
	parametro * par = buscarParametro(etiqueta);//no hay que hacer un free aca o se rompe la lista
	return ((char**)par->contenido)[indice];
}
char ** configStringArray(char * etiqueta){
	parametro * par = buscarParametro(etiqueta);//no hay que hacer un free aca o se rompe la lista
	return (char**)par->contenido;
}
char * configString(char * etiqueta){
	parametro * par = buscarParametro(etiqueta);//no hay que hacer un free aca o se rompe la lista
	return (char*)par->contenido;
}

int configInt(char * etiqueta){
	return atoi(configString(etiqueta));
}

int configIntArrayElement(char * etiqueta, int indice){
	return atoi(configStringArrayElement(etiqueta, indice));
}
//-------

char * file_to_string(FILE * stream){
	char *contents;
	fseek(stream, 0L, SEEK_END);
	long fileSize = ftell(stream);
	fseek(stream, 0L, SEEK_SET);

	//Allocate enough memory (add 1 for the \0, since fread won't add it)
	contents = malloc(fileSize+1);

	//Read the file
	size_t size=fread(contents,1,fileSize,stream);
	contents[size]=0; // Add terminating zero.

	//No lo lei y me chupa un huevo, es copy-paste de internet
	return contents;
}

int countSplit(char ** array){
	int size;
	for (size = 0; array[size] != NULL; size++);
	return size;
}

//Sirve para liberar un char** por completo
void liberarLista(char ** lista){
	int i = 0;
	for (i = 0; i < countSplit(lista); i++)
			free(lista[i]);
	free(lista);
}

parametro * agregarParametro(char * linea, t_config * config){
	char ** cadenas = string_split(linea,"=");
	parametro * param =
			cadenas[1][0] == '[' ? obtenerParametroArray(config, string_duplicate(cadenas[0])) : obtenerParametroString(config, string_duplicate(cadenas[0]));
	liberarLista(cadenas);
	return param;
}

void configuracionInicial(char*PATH){
	t_config * config;
	t_list * lista = list_create();
	config = config_create(PATH);

	FILE* archivo = fopen(PATH,"rb");
	char * contenidoArchivo = file_to_string(archivo);		//Se carga el contenido del archivo en una sola cadena
	char ** cadenas = string_split(contenidoArchivo,"\n");
	int i = 0;
	for (i = 0; i < countSplit(cadenas); i++)
			list_add(lista,agregarParametro(cadenas[i], config));//Se agregan a la lista todos los elementos

		//Se liberan las cosas que ya no uso y se cierra el archivo
	liberarLista(cadenas);
	free(contenidoArchivo);
	fclose(archivo);

	config_destroy(config);
	listaDeConfig = lista;//listaDeConfig es una variable publica que guarda las configuraciones
}
void imprimirParametroString(parametro* par){
	printf("%s: %s\n", par->etiqueta, (char*)par->contenido);
}
void imprimirParametroArray(parametro* par){//no hay que hacer free o se borra de la lista
	printf("%s: [", par->etiqueta);
	int i = 0;
	for(i=0;((char**)par->contenido)[i] != NULL;i++){
		printf("%s",configStringArrayElement(par->etiqueta,i));
		if(((char**)par->contenido)[i+1] != NULL) printf(",");
	}
	printf("]\n");
}
void list_forEach(t_list * self, void (*funcion) (void*)){
	t_link_element *element = self->head;
	while(element != NULL ){
		funcion(element->data);
		element = element->next;
	}
}
void imprimirParametro(parametro* par){
	if(par->tipo == 0) imprimirParametroString(par);
	if(par->tipo == 1) imprimirParametroArray(par);
}

void imprimirConfiguracion(){
	list_forEach(listaDeConfig, imprimirParametro);
}

void liberarConfiguracion(){

	list_clean_and_destroy_elements(listaDeConfig, liberarParametro);
	list_destroy(listaDeConfig);
}
