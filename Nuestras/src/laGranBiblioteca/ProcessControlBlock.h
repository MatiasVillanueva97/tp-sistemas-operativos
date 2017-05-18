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
}t_direccion;

typedef struct{
 char ID;
 t_direccion direccion;
}t_variable;

typedef struct{
 t_list* argumentos;
 t_list* variables;
 int retPos;
 t_direccion retVar;
} t_entrada;

typedef struct{
 int pid;
 t_puntero_instruccion programCounter;
 int contPags_pcb;
 int contextoActual;
 t_intructions* indiceCodigo;
char* indiceEtiquetas; // ¡??¡?'¿¿'??¡ que es¡?
 t_entrada* indiceStack;
 int exitCode;
}__attribute__((packed)) PCB_DATA;
////-----FIN PCB y Stack--------////


#endif /* LAGRANBIBLIOTECA_PROCESSCONTROLBLOCK_H_ */
