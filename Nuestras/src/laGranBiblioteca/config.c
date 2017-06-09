#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"
#include "commons/collections/list.h"
#include "commons/string.h"
#include "string.h"
#include "commons/collections/dictionary.h"


void liberarArray(char ** lista){
	int i = 0;
	for (i = 0; i < countSplit(lista); i++)
			free(lista[i]);
	free(lista);
}
t_config * config;

//Cuenta las lineas de un array
int countSplit(char ** array){
	int size;
	for (size = 0; array[size] != NULL; size++);
	return size;
}

bool esArray(char* valor){
	return string_contains(valor, "[");
}

char * getConfigString(char * etiqueta){
	return config_get_string_value(config, etiqueta);
}

char ** getConfigStringArray(char * etiqueta){
	return config_get_array_value(config, etiqueta);
}

char * getConfigStringArrayElement(char * etiqueta, int indice){
	char** array =  getConfigStringArray(etiqueta);
	char* copia = string_duplicate(array[indice]);
	liberarArray(array);
	return copia;
}

int getConfigInt(char * etiqueta){
	return atoi(getConfigString(etiqueta));
}

int getConfigIntArrayElement(char * etiqueta, int indice){
	char* elemento = getConfigStringArrayElement(etiqueta, indice);
	int elementoParseado = atoi(elemento);
	free(elemento);
	return elementoParseado;
}

int getArraySize(char * etiqueta){
	char** array = getConfigStringArray(etiqueta);
	int cant = countSplit(array);
	liberarArray(array);
	return cant;
}
////------------FIN GETTERS-------

void modificarArray(char** array, int indiceAModificar, char * contenidoNuevo){
	free(array[indiceAModificar]);
	array[indiceAModificar] = string_duplicate(contenidoNuevo);
}

char* arrayFormateado(char** array){
	char* nuevo = string_duplicate("[");
	int i;
	int cantElementos = countSplit(array);
	for(i = 0; i< cantElementos; i++){
		string_append(&nuevo, array[i]);
		if(i < cantElementos -1) string_append(&nuevo, ", ");
	}
	string_append(&nuevo, "]");
	return nuevo;
}

////------------SETTERS-------

void setConfigString(char * etiqueta, char * valor){
	dictionary_remove_and_destroy(config->properties, etiqueta, free);
	dictionary_put(config->properties, etiqueta, string_duplicate(valor));
}

void setConfigStringArrayElement(char * etiqueta, int indice, char * valor){
	char** array = getConfigStringArray(etiqueta);
	modificarArray(array, indice, valor);
	char* arrayFormatead = arrayFormateado(array);
	liberarArray(array);
	setConfigString(etiqueta, arrayFormatead);
	free(arrayFormatead);
}

void setConfigInt(char * etiqueta, int valor){
	char * aux = string_itoa(valor);
	setConfigString(etiqueta, aux);
	free(aux);
}

void setConfigIntArrayElement(char * etiqueta, int indice, int valor){
	char * aux = string_itoa(valor);
	setConfigStringArrayElement(etiqueta, indice, aux);
	free(aux);
}


void imprimirConfiguracion(){
	void impresion(char * etiqueta, char* valor){
		printf("%s: %s\n", etiqueta, valor);
	}
	dictionary_iterator(config->properties, impresion);
}

void liberarConfiguracion(){
	config_destroy(config);
}

void configuracionInicial(char*PATH){
	config = config_create(PATH);
}


//El mejor invento de la galaxia
void list_forEach(t_list * self, void (*funcion) (void*)){
	t_link_element *element = self->head;
	while(element != NULL ){
		funcion(element->data);
		element = element->next;
	}
}

///////


//--Hice esta funcion porque sino tenia que recorrer el diccionario 2 veces
void modConfigIntArrayElem(char* etiqueta, int indice, int mod){
	int valor = getConfigIntArrayElement(etiqueta, indice) + mod;
	setConfigIntArrayElement(etiqueta, indice, valor);
}

void decrementarConfigArray(char* etiqueta, int indice){
	modConfigIntArrayElem(etiqueta, indice, -1);
}

void incrementarConfigArray(char* etiqueta, int indice){
	modConfigIntArrayElem(etiqueta, indice, 1);
}



