#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"

#include <parser/metadata_program.h>

#ifndef ESTACOSA
#define ESTACOSA


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

typedef struct
{
	int pid;
	int programCounter;
	int contPags_pcb;
	int contextoActual;
	t_intructions* indiceCodigo;
	int cantidadEtiquetas;
	char* indiceEtiquetas;
	t_entrada* indiceStack;
	int exitCode;
}__attribute__((packed)) PCB_DATA;


int socketKernel;
int socketMemoria;
PCB_DATA pcb;
bool terminoPrograma;
#endif //ESTACOSA
