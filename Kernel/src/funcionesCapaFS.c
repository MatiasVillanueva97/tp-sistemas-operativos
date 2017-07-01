/*
 * funcionesCapaFS.c
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#include "funcionesCapaFS.h"



ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* encontrarElDeIgualPid(int pid){
			ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux;
			bool sonDeIgualPid(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * elementos){
				return  elementos->pid == pid;
				}
			aux = list_find(tablaGlobalDeArchivosDeProcesos,(void *)sonDeIgualPid);
						return aux;
}

ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* encontrarElDeIgualPath(char* path){
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * auxiliar;
bool sonDeIgualPath(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elementos){
				return  elementos->path == path;
		}
		auxiliar = list_find(tablaGlobalDeArchivos,(void *)sonDeIgualPath);
		return auxiliar;
}

bool archivoExiste(char* path){
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * auxiliar;
	auxiliar = encontrarElDeIgualPath(path);
	return auxiliar !=NULL;
}
int posicionEnTablaGlobalDeArchivos(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* auxiliar){
	int i = 0;
	bool buscarPosicion(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elementos){
					i++;
					return  elementos->path == auxiliar->path;
			}
			auxiliar = list_find(tablaGlobalDeArchivos,(void *)buscarPosicion);
			return i;
}

int posicionEnTablaGlobalArchivosDeProceso(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* auxiliar){
	int i = 0;
	bool buscarPosicion(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* elemento){
					i++;
					return  elemento->pid== auxiliar->pid;
			}
			auxiliar = list_find(tablaGlobalDeArchivosDeProcesos,(void *)buscarPosicion);
			return i;
}

/*int posicionEnUnaLista(t_list* lista, void* nodo,void(*condicion)(void*)){
	int i = 0;
	void * auxiliar;
	bool buscarPosicion(void * nodo){
			i++;
			return   condicion(nodo) == condicion(auxiliar);
				}
	auxiliar = list_find(lista,(void *)buscarPosicion);
	return i;
}*/

void agregarATablaDeProceso(int globalFD, char* flags, t_list* tablaProceso, int posicion){
	ENTRADA_DE_TABLA_DE_PROCESO* nuevaEntradaProceso = malloc(sizeof(ENTRADA_DE_TABLA_DE_PROCESO));
	nuevaEntradaProceso->globalFD = globalFD;
	nuevaEntradaProceso->flags = string_duplicate(flags);
	nuevaEntradaProceso->offset = 0;
	list_add(tablaProceso,nuevaEntradaProceso);
}

void agregarATablaGlobalDeArchivos(char* path,int aperturas){
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * nuevaEntrada = malloc(sizeof(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS));
	nuevaEntrada->path = string_duplicate(path);
	nuevaEntrada->cantidad_aperturas = aperturas;
	list_add(tablaGlobalDeArchivos,nuevaEntrada);
}

void* serializarPedidoFs(int size, int offset,char* path){
	void *contenido = malloc(4+strlen(path)+sizeof(int)*2);
	memcpy(contenido,&size,sizeof(int));
	memcpy(contenido+sizeof(int),&offset,sizeof(int));
	int tamanoRuta = strlen(path);
	memcpy(contenido+sizeof(int)*2,&tamanoRuta,sizeof(int));
	memcpy(contenido+sizeof(int)*3,path,strlen(path));
	return contenido;
}

//Espero que esto ande
t_crearArchivo deserializarCrearArchivo(void* stream){
	t_crearArchivo mensaje;
		memcpy(&mensaje.pid,stream,sizeof(int));
		int tamanoContenido;
		memcpy(&tamanoContenido,stream + sizeof(int), sizeof(int));
		char* contenidoAuxiliar = malloc(tamanoContenido);
		memcpy(contenidoAuxiliar,stream + sizeof(int) * 2, tamanoContenido);
		mensaje.flags = contenidoAuxiliar;
		int tamanoContenido2;
		memcpy(&tamanoContenido2,stream + sizeof(int)*2 + tamanoContenido, sizeof(int));
		char* contenidoAuxiliar2 = malloc(tamanoContenido2);
		memcpy(contenidoAuxiliar2,stream + sizeof(int)*3 +tamanoContenido, tamanoContenido2);
		mensaje.path = contenidoAuxiliar2;
		return mensaje;
}


void* serializarEscribirMemoria(int size, int offset,char* path, char* buffer){
	void *contenido = malloc(4+strlen(path)+size+sizeof(int)*2);
	memcpy(contenido,&size,sizeof(int));
	memcpy(contenido+sizeof(int),buffer,size);
	int tamanoRuta= strlen(path);
	memcpy(contenido+sizeof(int)+size,&tamanoRuta,sizeof(int));
	memcpy(contenido+sizeof(int)*2+size,path,tamanoRuta);
	memcpy(contenido+sizeof(int)*2+tamanoRuta+size,&offset,sizeof(int));
	return contenido;
}


void finalizarPid(PCB_DATA* pcb,int exitCode){
	pcb->exitCode = exitCode;
	pcb->estadoDeProceso = finalizado;
}
void liberarEntradaDeTablaProceso(ENTRADA_DE_TABLA_DE_PROCESO* entrada){

	free(entrada);
}

void liberarEntradaTablaGlobalDeArchivos(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada){
	free(entrada->cantidad_aperturas);
	free(entrada->path);
	free(entrada);
}

void liberarEntradaTablaGlobalDeArchivosDeProceso(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO*entrada){
	list_clean_and_destroy_elements(entrada->tablaProceso,liberarEntradaDeTablaProceso);
	free(entrada->pid);
	free(entrada);
}


void liberarRecursosArchivo(PCB_DATA* pcb){
	ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* entrada_a_eliminar = encontrarElDeIgualPid(pcb->pid);
	int i;
	int tamanoTabla=list_size(entrada_a_eliminar->tablaProceso);
	for(i=3;i<tamanoTabla;i++){
		ENTRADA_DE_TABLA_DE_PROCESO *entrada_de_tabla_proceso= list_get(entrada_a_eliminar->tablaProceso,i);
		ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo= list_get(tablaGlobalDeArchivos,entrada_de_tabla_proceso->globalFD);
		entrada_de_archivo->cantidad_aperturas--;
		if(entrada_de_archivo->cantidad_aperturas==0){
			list_remove_and_destroy_element(tablaGlobalDeArchivos,entrada_de_tabla_proceso->globalFD,liberarEntradaTablaGlobalDeArchivos);
		}
	}
	list_remove_and_destroy_element(tablaGlobalDeArchivosDeProcesos,posicionEnTablaGlobalArchivosDeProceso(entrada_a_eliminar),liberarEntradaTablaGlobalDeArchivosDeProceso);
}

