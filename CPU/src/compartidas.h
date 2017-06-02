#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"

#include <parser/metadata_program.h>

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"

#ifndef ESTACOSA
#define ESTACOSA

typedef struct{
 int size_pag;
 int quantum;
 int size_stack;
}__attribute__((packed)) t_datosIniciales;

int socketKernel;
int socketMemoria;
PCB_DATA* pcb;
bool terminoPrograma;
bool bloqueado;
t_datosIniciales* datosIniciales;
#endif //ESTACOSA
