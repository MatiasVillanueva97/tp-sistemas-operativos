#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"
#include "commons/log.h"

#include <parser/metadata_program.h>

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"

#ifndef ESTACOSA
#define ESTACOSA

typedef struct{
 int size_pag;
 int quantum;
 int size_stack;
}__attribute__((packed)) t_datosIniciales;

t_log* logCPU;
int socketKernel;
int socketMemoria;
PCB_DATA* pcb;
bool terminoPrograma;
bool bloqueado;
bool signal_SIGUSR1;
bool signal_sigint;
bool hayPCB;
t_datosIniciales* datosIniciales;
int quantumSleep;
#endif //ESTACOSA
