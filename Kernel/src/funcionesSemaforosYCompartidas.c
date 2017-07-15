/*
 * funcionesSemaforos.c
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */

#include "funcionesSemaforosYCompartidas.h"


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
//Si no lo encuentra termina el proceso.
bool SEM_wait(char* nombreSEM, PCB_DATA * pcb){
	t_semaforo* sem = buscarSemaforo(nombreSEM);
	if(sem != NULL){
		sem_wait(&mutex_listaProcesos);
		bool buscar(PROCESOS* proceso){
			return proceso->pid == pcb->pid;
		}
		PROCESOS* proceso = list_find(avisos, buscar);
		proceso->semaforoTomado = string_duplicate(nombreSEM);
		sem_post(&mutex_listaProcesos);
		if(sem->valor <= 0){
			sem_wait(&mutex_cola_Exec);
			pcb->estadoDeProceso = bloqueado;
			sem_post(&mutex_cola_Exec);
			queue_push(sem->cola, pcb);
		}
		sem->valor--;
	}
	else{
		pcb->estadoDeProceso = finalizado;
		pcb->exitCode = intentoAccederAUnSemaforoInexistente;
	}
	return (sem != NULL) && (sem->valor >= 0);//Si se cumplen estas 2 condiciones, la CPU puede seguir ejecutando el PCB
}

void despertarProceso(t_semaforo * sem){
	PCB_DATA* procesoADespertar;
	int i = 0;
	int f = queue_size(sem->cola);
	for(i;i<f; i++){
		procesoADespertar = queue_pop(sem->cola);
		if(procesoADespertar->estadoDeProceso == bloqueado){
			procesoADespertar->estadoDeProceso = paraEjecutar;
			break;
		}
	}

}

bool SEM_signal(char* nombreSEM, PCB_DATA* pcb){
	t_semaforo* sem = buscarSemaforo(nombreSEM);
	if(sem != NULL){
		sem->valor++;
		if(queue_size(sem->cola) > 0){
			sem_wait(&mutex_cola_Wait);
			despertarProceso(sem);
			sem_post(&mutex_cola_Wait);
		}
	}
	else{
		pcb->estadoDeProceso = finalizado;
		pcb->exitCode = intentoAccederAUnSemaforoInexistente;
	}
	return (sem != NULL);//Retorna si el semaforo existe, le permite o no a la CPU seguir ejecutando
}

