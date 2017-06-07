/*
 * ProcessControlBlock.h
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <parser/metadata_program.h>
#include "commons/collections/list.h"


#ifndef LAGRANBIBLIOTECA_PROCESSCONTROLBLOCK_H_
#define LAGRANBIBLIOTECA_PROCESSCONTROLBLOCK_H_


////-----PCB y Stack--------////

typedef struct{
 int page;
 int offset;
 int size;
}__attribute__((packed))t_direccion;

typedef struct{
 char ID;
 t_direccion direccion;
}__attribute__((packed))t_variable;

typedef struct{
 t_list* argumentos;
 t_list* variables;
 int retPos;
 t_direccion retVar;
}__attribute__((packed))t_entrada;



typedef struct{
 int pid;
 t_puntero_instruccion programCounter;

 int cantidadDeInstrucciones;//Sirve para el indice de codigo, Es un contador para meterlo en un for

 int contPags_pcb;
 int contextoActual;
 t_intructions* indiceCodigo;//Es un array
char* indiceEtiquetas; // ¡??¡?'¿¿'??¡ que es¡?
 int cantidadDeEntradas;//Es un contador para meterlo en un for
 int cantidadDeEtiquetas;
 int sizeEtiquetas;
 int estadoDeProceso;
 t_entrada* indiceStack;//Es otro array


 int exitCode;
}__attribute__((packed)) PCB_DATA;


////-----FIN PCB y Stack--------////

int tamanoPCB(PCB_DATA * pcb);

void* serializarPCB(PCB_DATA * pcb);

PCB_DATA* deserializarPCB(void* stream);

void imprimirPCB(PCB_DATA * pcb);

void harcodeoAsquerosoDePCB();

void destruirPCB_Puntero(PCB_DATA * pcb);


#endif /* LAGRANBIBLIOTECA_PROCESSCONTROLBLOCK_H_ */
