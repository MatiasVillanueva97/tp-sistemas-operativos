/*
 * funcionesCapaFS.c
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#include "funcionesCapaFS.h"


bool archivoExiste(char* path){

	bool sonDeIgualPath(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elementos){
								return  elementos->path == path;
	}
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* auxiliar;
	auxiliar = list_find(tablaGlobalDeArchivos,(void *)sonDeIgualPath);
	if(auxiliar == NULL){
			return false;}
	else{return true; }
}

ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * encontrarElDeIgualPid(int pid){
			ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux;
			bool sonDeIgualPid(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * elementos){
				return  elementos->pid == pid;
				}
			aux = list_find(tablaGlobalDeArchivosDeProcesos,(void *)sonDeIgualPid);
						return aux;
}

void agregarATablaDeProceso(int df, char* flags, t_list* tablaProceso){
ENTRADA_DE_TABLA_DE_PROCESO * nuevaEntradaProceso = malloc(sizeof(ENTRADA_DE_TABLA_DE_PROCESO));
nuevaEntradaProceso->globalFD = df;
nuevaEntradaProceso->flags = string_duplicate(flags);
list_add(tablaProceso, nuevaEntradaProceso);
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
		memcpy(&tamanoContenido2,stream + sizeof(int)*3 + tamanoContenido, sizeof(int));
		char* contenidoAuxiliar2 = malloc(tamanoContenido2);
		memcpy(contenidoAuxiliar2,stream + sizeof(int) * 4 +tamanoContenido, tamanoContenido2);
		mensaje.path = contenidoAuxiliar2;
		return mensaje;
}
void finalizarPid(PCB_DATA* pcb,int exitCode){
	pcb->exitCode = exitCode;
	pcb->estadoDeProceso = finalizado;
}
void liberarEntradaDeTablaProceso(ENTRADA_DE_TABLA_DE_PROCESO* entrada){
	free(entrada->flags);
	free(entrada->globalFD);
	free(entrada);
}

void liberarEntradaTablaGlobalDeArchivos(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada){
	free(entrada->cantidad_aperturas);
	free(entrada->path);
	free(entrada);
}


