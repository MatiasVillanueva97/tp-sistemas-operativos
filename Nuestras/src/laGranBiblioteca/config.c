/*
 * config2.c

 *
 *  Created on: 22/4/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"

//--------FUNCIONES DE SEGURIDAD-----
char* getStringFromConfig(t_config *config, char*valor){
	char* aux;

	if(config_has_property(config, valor)){
		aux = string_duplicate(config_get_string_value(config, valor));
	}

	else perror("Archivo config mal hecho");

	return aux;
}



char** getStringArrayFromConfig(t_config *config, char*valor){
	char** aux;

	if(config_has_property(config, valor)){
		aux = config_get_array_value(config, valor);
	}

	else perror("Archivo config mal hecho");

	return aux;
}


typedef struct{
	char * etiqueta;
	void* contenido;
	int tipo;//0 = char*, 1=char**
}parametro;

//Sirve para liberar un char** por completo
void liberarArray(char ** lista){
	int i = 0;
	for (i = 0; i < countSplit(lista); i++)
			free(lista[i]);
	free(lista);
}

///---------------LIBERACION DE VARIABLES-------------
void liberarParametro(parametro * param){
	if(param->tipo == 0)
		free(param->contenido);
	else
		liberarArray(param->contenido);
	free(param->etiqueta);
	free(param);
}

void liberarConfiguracion(){

	list_clean_and_destroy_elements(listaDeConfig, liberarParametro);
	list_destroy(listaDeConfig);
}


//---------FUNCIONES PARA BUSCAR LOS PARAMETROS EN EL CONFIG
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

//---------FUNCIONES DE BUSQUEDA
parametro * buscarParametro(char * etiqueta){
	bool busqueda(parametro * parametro){
		return strcmp(etiqueta, parametro->etiqueta) == 0;
	}
	if(!list_any_satisfy(listaDeConfig, busqueda)){
		perror("No se encontro el parametro buscado en el archivo de configuracion");
		exit(-1);
	}
	parametro * par = list_find(listaDeConfig, busqueda);
	return par;
}
int buscarIdParametro(char * etiqueta){
	int cont = 0;
	bool busqueda(parametro * parametro){
		cont++;
		return strcmp(etiqueta, parametro->etiqueta) == 0;
	}
	parametro * par = list_find(listaDeConfig, busqueda);
	return cont-1;
}

//------------FUNCIONES EXTRA PARA EL FUNCIONAMIENTO DE LOS SETTERS-------
parametro * parametroModificado(parametro * viejo, char* contenido){
	parametro * nuevo = malloc(sizeof(parametro));
	nuevo->tipo = viejo->tipo;
	nuevo->etiqueta = string_duplicate(viejo->etiqueta);
	nuevo->contenido = string_duplicate(contenido);
	return nuevo;
}
char ** nuevoArrayConModificacion(parametro * viejo, char* contenidoNuevo, int indice){
	int i = 0;
	char * salida = string_new();
	string_append(&salida,"[");
	for(i=0;((char**)viejo->contenido)[i] != NULL;i++){
		if(i != indice)
			string_append(&salida,getConfigStringArrayElement(viejo->etiqueta,i));
		else
			string_append(&salida,contenidoNuevo);
		if(((char**)viejo->contenido)[i+1] != NULL)
			string_append(&salida,",");
	}
	string_append(&salida,"]");

	char ** aux = string_get_string_as_array(salida);
	free(salida);
	return aux;
}
parametro * parametroArrayModificado(parametro * viejo, char* contenidoNuevo, int indice){
	parametro * nuevo = malloc(sizeof(parametro));
	nuevo->tipo = viejo->tipo;
	nuevo->etiqueta = string_duplicate(viejo->etiqueta);
	nuevo->contenido = nuevoArrayConModificacion(viejo,contenidoNuevo,indice);
	return nuevo;
}

///------------------------GETTERS-----------------------
char * getConfigStringArrayElement(char * etiqueta, int indice){
	parametro * par = buscarParametro(etiqueta);//no hay que hacer un free aca o se rompe la lista
	return ((char**)par->contenido)[indice];
}
char ** getConfigStringArray(char * etiqueta){
	parametro * par = buscarParametro(etiqueta);//no hay que hacer un free aca o se rompe la lista
	return (char**)par->contenido;
}
char * getConfigString(char * etiqueta){
	parametro * par = buscarParametro(etiqueta);//no hay que hacer un free aca o se rompe la lista
	return (char*)par->contenido;
}

int getConfigInt(char * etiqueta){
	return atoi(getConfigString(etiqueta));
}

int getConfigIntArrayElement(char * etiqueta, int indice){
	return atoi(getConfigStringArrayElement(etiqueta, indice));
}



///----------------------------SETTERS------------------------
//tienen logica repetida, si querés arreglarlos te invito xdxdxd
void setConfigString(char * etiqueta, char * contenidoNuevo){
	parametro * viejo = buscarParametro(etiqueta);
	int indiceParam = buscarIdParametro(etiqueta);
	char * aux = string_duplicate(contenidoNuevo);
	parametro * nuevo = parametroModificado(viejo, contenidoNuevo);
	free(aux);
	list_replace_and_destroy_element(listaDeConfig, indiceParam, nuevo, liberarParametro);
}

void setConfigStringArrayElement(char * etiqueta, int indice, char * contenidoNuevo){
	parametro * viejo = buscarParametro(etiqueta);
	int indiceParam = buscarIdParametro(etiqueta);
	parametro * nuevo = parametroArrayModificado(viejo, contenidoNuevo, indice);
	list_replace_and_destroy_element(listaDeConfig, indiceParam, nuevo, liberarParametro);
}

void setConfigInt(char * etiqueta, int valor){
	parametro * viejo = buscarParametro(etiqueta);
	int indiceParam = buscarIdParametro(etiqueta);
	char * aux = string_itoa(valor);
	parametro * nuevo = parametroModificado(viejo, aux);
	free(aux);
	list_replace_and_destroy_element(listaDeConfig, indiceParam, nuevo, liberarParametro);
}

void setConfigIntArrayElement(char * etiqueta, int indice, int valor){
	parametro * viejo = buscarParametro(etiqueta);
	int indiceParam = buscarIdParametro(etiqueta);
	char * aux = string_itoa(valor);
	parametro * nuevo = parametroArrayModificado(viejo, aux, indice);
	free(aux);
	list_replace_and_destroy_element(listaDeConfig, indiceParam, nuevo, liberarParametro);
}

//Convierte un archivo en un char*
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

//Cuenta las lineas de un array
int countSplit(char ** array){
	int size;
	for (size = 0; array[size] != NULL; size++);
	return size;
}


//Decide si un parametro es un array o un string/int y lo obtiene para que este sea agregado a la lista
parametro * agregarParametro(char * linea, t_config * config){
	char ** cadenas = string_split(linea,"=");
	parametro * param =
			cadenas[1][0] == '[' ? obtenerParametroArray(config, string_duplicate(cadenas[0])) : obtenerParametroString(config, string_duplicate(cadenas[0]));
	liberarArray(cadenas);
	return param;
}

///--------------INICIALIZACION-------------

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
	liberarArray(cadenas);
	free(contenidoArchivo);
	fclose(archivo);

	config_destroy(config);
	listaDeConfig = lista;//listaDeConfig es una variable publica que guarda las configuraciones
}


///-----------IMPRESIONES-----------


void imprimirParametroString(parametro* par){
	printf("%s: %s\n", par->etiqueta, (char*)par->contenido);
}
void imprimirParametroArray(parametro* par){//no hay que hacer free o se borra de la lista
	printf("%s: [", par->etiqueta);
	int i = 0;
	for(i=0;((char**)par->contenido)[i] != NULL;i++){
		printf("%s",getConfigStringArrayElement(par->etiqueta,i));
		if(((char**)par->contenido)[i+1] != NULL) printf(",");
	}
	printf("]\n");
}
//El mejor invento de la galaxia
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


