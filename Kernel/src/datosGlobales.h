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
#include "commons/log.h"

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"

#ifndef DATOSGLOBALES_H_
#define DATOSGLOBALES_H_

int socketMemoria;
int socketFS;
int historico_pid;
int size_pagina;
int quantumRR;
int quantumSleep;
bool finPorConsolaDelKernel;
t_log* logKernel;




///------ESTRUCTURAS AUXILIARES ADMNISTRATIVAS------///


enum exitCode{
	finalizadoCorrectamente =0,
	noSePudoReservarRecursos = -1,
	archivoInexistente = -2,
	lecturaDenegadaPorFaltaDePermisos = -3,
	escrituraDenegadaPorFaltaDePermisos = -4,
	excepcionMemoria = -5,
	deconexionConsola = -6,
	finalizacionDesdeConsola = -7,
	reservarMasMemoriaQueTamanoPagina = -8,
	noSePuedenAsignarMasPaginas = -9,
	finalizacionDesdeKenel = -10,
	intentoAccederAUnSemaforoInexistente = -11,
	intentoAccederAUnaVariableCompartidaInexistente = -12,
	lecturaDenegadaPorFileSystem = -13,
	escrituraDenegadaPorFileSystem = -14,
	noSeCreoElArchivoPorFileSystem = -15,
	falloEnElFileDescriptor = -16
};

enum estadosProcesos{
	finalizado = 0,
	loEstaUsandoUnaCPU = 1,
	bloqueado = 2,
	paraEjecutar = 53,
	moverAReady = 42
};

//ESTRUCTURA PARA MANEJAR LAS CPUS

typedef struct{
	int socketCPU;
	bool esperaTrabajo;
	PCB_DATA* pcbQueSeLlevo;
}t_CPU;

t_list* lista_CPUS;
t_list* tablaDeHeapMemoria;
t_list* tablaEstadisticaDeHeap;
typedef struct{
	int pid;
	int tamanoAlocadoEnOperaciones;
	int tamanoAlocadoEnBytes;
	int tamanoLiberadoEnOperaciones;
	int tamanoLiberadoEnBytes;
	int cantidadDePaginasHistoricasPedidas;
}filaEstadisticaDeHeap;
typedef struct{
	int pid;
	int pagina;
	int tamanoDisponible;

}filaTablaDeHeapMemoria;

/// Estructura para manejar los semaforos


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
	char* semaforoTomado;
	//archivos tomados
	//elementos de heap tomados
} PROCESOS;

//***Tabla de archivos para cada proceso

//*** Tabla global de procesos
typedef struct {
	int globalFD;// seria algo asi como la posicion del archivo en la tabla global de archivo pero no se me ocurrio un nombre
	char* flags;
	int offset;
}ENTRADA_DE_TABLA_DE_PROCESO;

typedef struct{
	int pid ;
	t_list* tablaProceso;
}ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO;

t_list * tablaGlobalDeArchivosDeProcesos;


//**Tabla global de archivos
typedef struct {
	char * path;
	int cantidad_aperturas;//si esta en 0 entonces sale de la lista
}ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS;
t_list * tablaGlobalDeArchivos;

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
	int quantum_sleep;
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
pthread_mutex_t mutex_tablaGlobalDeArchivos;
pthread_mutex_t mutex_tablaGlobalDeArchivosDeProcesos;

pthread_mutex_t activarHilo_Exec;


pthread_mutex_t mutex_semaforos_ANSISOP;
pthread_mutex_t mutex_variables_compartidas;
pthread_mutex_t mutex_tablaDeHeap;
pthread_mutex_t mutex_Quantum_Sleep;
pthread_mutex_t mutex_tabla_estadistica_de_heap;


sem_t sem_ConsolaKernelLenvantada;

sem_t aNew;
sem_t aReady;
sem_t aWait;
sem_t aBloq;
sem_t aFinished;


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
