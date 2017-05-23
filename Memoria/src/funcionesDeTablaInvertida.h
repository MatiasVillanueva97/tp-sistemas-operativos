/*
 * funcionesDeTablaInvertida.h
 *
 *  Created on: 17/5/2017
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
#include <pthread.h>
#include <semaphore.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"

#include "EstructurasDeLaMemoria.h"

#include "parser/metadata_program.h"
#include "parser/parser.h"

#ifndef FUNCIONESDETABLAINVERTIDA_H_
#define FUNCIONESDETABLAINVERTIDA_H_


//Funciones Tabla De Paginación Invertida
int buscarPidEnTablaInversa(int pidRecibido);
void iniciarTablaDePaginacionInvertida();
void imprimirTablaDePaginasInvertida();

//Funciones Frame
int buscarFrameCorrespondiente(int pidRecibido,int pagina);
int reservarFrame(int pid, int pagina);
int memoriaFramesLibres();

//Funciones Paginas
int cantidadDePaginasDeUnProcesoDeUnProceso(int pid);
int liberarPagina(int pid, int pagina);
#endif /* FUNCIONESDETABLAINVERTIDA_H_ */
