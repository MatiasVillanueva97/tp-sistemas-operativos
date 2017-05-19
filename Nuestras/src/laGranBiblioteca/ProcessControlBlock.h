/*
 * ProcessControlBlock.h
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */
#include <parser/metadata_program.h>


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
 t_entrada* indiceStack;//Es otro array


 int exitCode;
}__attribute__((packed)) PCB_DATA;


////-----FIN PCB y Stack--------////


#endif /* LAGRANBIBLIOTECA_PROCESSCONTROLBLOCK_H_ */
