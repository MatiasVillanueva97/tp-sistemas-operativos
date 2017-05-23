/*
 * funcionesParaTodosYTodas.c
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include "funcionesParaTodosYTodas.h"

int sum(t_list *lista,int(* funcion) (void*)){
	int i;
	int contador=0;
	for( i = 0; i< list_size(lista);i++){
		contador += funcion(list_get(lista,i));
	}
	return contador;
}
