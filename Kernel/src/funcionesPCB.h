/*
 * funcionesPCB.h
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
#include "commons/collections/list.h"
#include "commons/collections/queue.h"

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"
#include "datosGlobales.h"

#ifndef FUNCIONESPCB_H_
#define FUNCIONESPCB_H_


PCB_DATA* crearPCB(char * scriptAnsisop, int pid, int contPags);
/* ******** CrearPCB
 * función que crea el pcb a partir de un scriptAnsisop, un pid y el contador de las paginas asignadas a memoria
 *
 */
PCB_DATA* buscarPCB (int pid);

void modificarPCB(PCB_DATA * pcbNuevo);

#endif /* FUNCIONESPCB_H_ */
