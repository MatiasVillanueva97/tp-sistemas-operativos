/*
 * funcionesParaTodosYTodas.h
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include "commons/collections/list.h"
#include "datosGobalesGenerales.h"
#include "ProcessControlBlock.h"

#ifndef LAGRANBIBLIOTECA_FUNCIONESPARATODOSYTODAS_H_
#define LAGRANBIBLIOTECA_FUNCIONESPARATODOSYTODAS_H_


t_mensajeDeProceso deserializarMensajeAEscribir(void* stream);

int sum(t_list *lista,int(* funcion) (void*));

int tamanoMensajeAEscribir(int tamanioContenido);

void* serializarMensajeAEscribir(t_mensajeDeProceso mensaje, int tamanio);


#endif /* LAGRANBIBLIOTECA_FUNCIONESPARATODOSYTODAS_H_ */
