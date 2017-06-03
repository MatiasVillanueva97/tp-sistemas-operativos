/*
 * datosGlobales.h
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "commons/collections/dictionary.h"

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"

#ifndef DATOSGLOBALES_H_
#define DATOSGLOBALES_H_

int socketMemoria;
int socketFS;
int historico_pid;
int stack_size;
int grado_multiprogramacion;
int size_pagina;
int quantumRR;
bool finPorConsolaDelKernel;


///------ESTRUCTURAS AUXILIARES ADMNISTRATIVAS------///


enum exitCode{
	noSePudoReservarRecursos = -1,
	accesoArchivoInexistente = -2,
	lecturaDenegada = -3,
	escrituraDenegada = -4,
	excepcionMemoria = -5,
	deconexionConsola = -6,
	finalizacionDesdeConsola = -7,
	reservarMasMemoriaQueTamanoPagina = -8,
	noSePuedenAsignarMasPaginas = -9,
	finalizacionDesdeKenel = -10,
	finalizadoCorrectamente =0,
	loEstaUsandoUnaCPU = 1,
	bloqueado = 2,
	paraEjecutar = 53,

};

//*** Esta lista es porque una consola puede tener una serie de procesos --- Igual creo que esto tiene que morir
t_list * avisos;

//*** Estructura que utilizamos para manjear el tema de tener que avisarle a la consola cuando su proceso finalizó
//*** Como los procesos pueden finalizar en cualquier momento, y en el pcb no contenemos a que consola pertenece cierto proceso
//*** Creamos esta estructura que nos contiene eso, el pid del proceso, la consola a quien le pertenece y el estado del proceso
typedef struct{
	int pid;
	char* scriptAnsisop;
	int socketConsola;
	bool avisoAConsola;
	PCB_DATA* pcb;
} PROCESOS;

//*** Esta es la escructura que le envio a memoria para que inicialize un programa
typedef struct{
int pid;
int cantPags;
}__attribute__((packed))INICIALIZAR_PROGRAMA;

//*** Estructura que le envio a la cpu al principio con todos lo datos que esta necesitará
typedef struct{
	int size_pag;
	int quantum;
	int size_stack;
}__attribute__((packed)) DATOS_PARA_CPU;


///------FIN ESTRUCTURAS AUXILIARES ADMNISTRATIVAS------///

///-------INICIO COLAS-------//
t_queue * cola_New;
t_queue * cola_Ready;
t_queue * cola_Wait;
t_queue * cola_Exec;
t_queue * cola_Finished;

t_queue * cola_CPUs_libres;

///-------FIN COLAS------//

///----INICIO SEMAFOROS----///
pthread_mutex_t mutex_HistoricoPcb; // deberia ser historico pid
pthread_mutex_t mutex_listaProcesos;
pthread_mutex_t mutex_cola_CPUs_libres;
pthread_mutex_t mutex_cola_New;
pthread_mutex_t mutex_cola_Ready;
pthread_mutex_t mutex_cola_Wait;
pthread_mutex_t mutex_cola_Exec;
pthread_mutex_t mutex_cola_Finished;

sem_t sem_ConsolaKernelLenvantada;


//***Estra esttructura me la manda la cpu cuando quiere escribir algo, ya sea por pantalla o en filesystem
typedef struct{
	int pid;
	int descriptorArchivo;
	char* mensaje;
}__attribute__((packed)) MENSAJE_PARA_ESCRIBIR_CPU;

typedef struct{
	int pid;
	char* mensaje;
}__attribute__((packed))MENSAJE_PARA_ESCRIBIR_CONSOLA;

#endif /* DATOSGLOBALES_H_ */
