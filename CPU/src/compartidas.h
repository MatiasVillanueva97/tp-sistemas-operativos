#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"

#include <parser/metadata_program.h>

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"

#ifndef ESTACOSA
#define ESTACOSA

int socketKernel;
int socketMemoria;
PCB_DATA pcb;
bool terminoPrograma;
#endif //ESTACOSA
