/*
 * funcionesSemaforos.c
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */

#include "funcionesSemaforosYCompartidas.h"

void liberarSemaforosYCompartidas(){
	void liberarSemaforo(t_semaforo* semaforo){
		free(semaforo->nombre);
		queue_destroy_and_destroy_elements(semaforo->cola,free);
		free(semaforo);
	}
	void liberarCompartidas(t_variableGlobal* variable){
		if(variable->nombre != NULL)
			free(variable->nombre-1);
		free(variable);
	}
	list_destroy_and_destroy_elements(listaDeSemaforos,liberarSemaforo);
	list_destroy_and_destroy_elements(listaDeVariablesGlobales,liberarCompartidas);
}

void cargarVariablesGlobalesDesdeConfig(){
	listaDeVariablesGlobales = list_create();
	int i;
	for(i=0; i<getArraySize("SHARED_VARS"); i++){
		t_variableGlobal * varGlob = malloc(sizeof(t_variableGlobal));
		varGlob->nombre = getConfigStringArrayElement("SHARED_VARS", i) + sizeof(char);
		varGlob->valor = 0;
		list_add(listaDeVariablesGlobales, varGlob);
	}
}

t_variableGlobal* buscarVariableGlobal(char* nombreVarGlob){
	bool busqueda(t_variableGlobal* varGlob){
		return strcmp(varGlob->nombre, nombreVarGlob) == 0;
	}
	return list_find(listaDeVariablesGlobales, busqueda);
}


void cargarSemaforosDesdeConfig(){
	listaDeSemaforos = list_create();
	int i;
	for(i=0; i<getArraySize("SEM_IDS"); i++){
		t_semaforo * sem = malloc(sizeof(t_semaforo));
		sem->nombre = getConfigStringArrayElement("SEM_IDS", i);
		sem->valor = getConfigIntArrayElement("SEM_INIT", i);
		sem->cola = queue_create();
		list_add(listaDeSemaforos, sem);
	}
}

t_semaforo* buscarSemaforo(char* nombreSEM){
	bool busqueda(t_semaforo* sem){
		return strcmp(sem->nombre, nombreSEM) == 0;
	}
	return list_find(listaDeSemaforos, busqueda);
}


//La idea es que la CPU me pase el PCB siempre que quiera hace un wait o signal, en base a eso yo puedo decidir que hacer

//Si encuentra el semaforo pedido hace lo que haria un wait normal.
//Si no lo encuentra  termina el proceso.
bool SEM_wait(char* nombreSEM, PCB_DATA * pcb){

	t_semaforo* sem = buscarSemaforo(nombreSEM);

	if(sem == NULL){
		log_error(logKernel,"Se intento acceder a un semaforo inexistente (el semaforo %s). Se finaliza el proceso: %d", nombreSEM, pcb->pid);

		pcb->estadoDeProceso=aFinalizar;
		proceso_Finalizar(pcb->pid, intentoAccederAUnSemaforoInexistente);

		return false;
	}

	sem->valor--;

	PROCESOS* proceso = list_get(avisos, pcb->pid-1);

	if(sem->valor < 0){
		if(pcb->estadoDeProceso != Wait){
			moverA(pcb->pid,aWait);
		}

		proceso->semBloqueante = string_duplicate(nombreSEM);

		int* pid=malloc(sizeof(int));
		*pid=pcb->pid;
		queue_push(sem->cola,pid);

		return false;
	}

	return true;
}

void despertarProceso(t_semaforo * sem){

	int* elTortu = queue_pop(sem->cola);

	PROCESOS* proceso = buscarProceso(*elTortu);

	free(proceso->semBloqueante);
	proceso->semBloqueante = NULL;
	moverA(*elTortu,aReady);

	sem_post(&cantidadDeProgramasEnReady);
	free(elTortu);
}

bool SEM_signal(char* nombreSEM, PCB_DATA* pcb){

	t_semaforo* sem = buscarSemaforo(nombreSEM);

	if(sem == NULL){
		pcb->estadoDeProceso=aFinalizar;
		proceso_Finalizar(pcb->pid,intentoAccederAUnSemaforoInexistente);
		return false;
	}

	sem->valor++;

	if(sem->valor <= 0)
		despertarProceso(sem);

	return true;
}

