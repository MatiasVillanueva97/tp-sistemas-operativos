/*
 * funcionesSemaforos.c
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */

#include "funcionesSemaforos.h"



//---Estas funciones son solo para el kernel---
int sema_indiceDeSemaforo(char* sem){
	int i = 0;
	char** array = getConfigStringArray(("SEM_IDS"));
	for(i = 0; i< countSplit(array); i++)
		if(strcmp(sem, array[i]) == 0){
			liberarArray(array);
			return i;
		}
	return NULL;
}

//Recorro el diccionario 2 veces, porque no me queda otra. Uno es para conseguir el id y el otro es para cambiar el valor
void sema_decrementarSEM(char* sem){
	decrementarConfigArray("SEM_INIT", sema_indiceDeSemaforo(sem));
}

void sema_incrementarSEM(char* sem){
	incrementarConfigArray("SEM_INIT", sema_indiceDeSemaforo(sem));
}

int sema_valorDelSemaforo(char * sem){
	return getConfigIntArrayElement("SEM_INIT", sema_indiceDeSemaforo(sem));
}

bool sema_existeSemaforo(char * sem){
	return string_contains(getConfigString("SEM_IDS"),sem);
}



int sema_proceso_wait(char* sem){
	if(sema_valorDelSemaforo(sem) <= 0){
		//agregarSemaforoAListaDeEspera();
	}
	sema_decrementarSEM(sem);

	return sema_valorDelSemaforo(sem);
}



void sema_despertarProceso(char* sem){
	int cont=-1;
	bool buscarPorSem(esperaDeSemaforo* e_sem)
	{
		cont++;
		return (strcmp(sem,e_sem->sem)==0);
	}


	/*pcb = list_find(listaDeEspera, buscarPorSem)->pcb;

	if(pcv->estado != finalizado)
	{
		pcb->estado = listoParaEjecutar;

	list_remove_and_destroy_element(listaDeEspera,cont, free);

	}
	else
	{

	}


	//--Tambien hay que sacar el pcb de la lista de alguna manera
*/
}
//Hay que contemplar el caso en el que un proceso muera externamente y haya hecho un wait
//si pasa, ¿hay que sumar un signal al semaforo? (Deberia, cuando un proceso muere libera sus recursos)

void sema_proceso_signal(char* sem){
	if(sema_valorDelSemaforo(sem) < 0)
		sema_despertarProceso(sem);
	sema_incrementarSEM(sem);
}

//No se si hay que validar que hagan un wait de un semaforo que no existe



