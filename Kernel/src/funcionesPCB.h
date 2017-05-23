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

PCB_DATA * buscarPCBPorPidEnCola(t_queue * , int );
/* ******** buscarPCBPorPidEnCola
 * función que dada una cola y un pid te devuelve el PCB completo
 *
 */


PCB_DATA * buscarPCBPorPidEnColaYBorrar(t_queue * , int );
/* ******** buscarPCBPorPidEnColaYBorrar
 * función que dada una cola y un pid te devuelve el PCB completo y lo remueve de la cola
 *
 */

PCB_DATA * buscarPCBPorPidYBorrar(int );
/* ******** buscarPCBPorPidEnColaYBorrar
 * función que dado un pid, busca en todas las colas si se encuentra tal pid y borra el PCB de la cola
 * busca entre las colas de ready, wait, y finished por ahora
 */

#endif /* FUNCIONESPCB_H_ */
