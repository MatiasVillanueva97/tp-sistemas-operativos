/*
 * funcionesMemoria.h
 *
 *  Created on: 27/5/2017
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
#include <math.h>

#include "datosGlobales.h"

#ifndef FUNCIONESMEMORIA_H_
#define FUNCIONESMEMORIA_H_



//*** Esta funcion te divide el scriptAnsisop en una cantidad de paginas dadas, el size de cada pagina esta en el config
char** memoria_dividirScriptEnPaginas(int cant_paginas, char *copiaScriptAnsisop);


//***Esta Funcion te devuelve la cantidad de paginas que se van a requerir para un scriptAsisop dado
int memoria_CalcularCantidadPaginas(char * scriptAnsisop);

#endif /* FUNCIONESMEMORIA_H_ */
