/*
 ** server.c -- a stream socket server demo
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
#include <math.h>
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "commons/collections/dictionary.h"
#include <semaphore.h>

#include <parser/parser.h>
#include <parser/metadata_program.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"
#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.c"

#include "datosGlobales.h"
#include "funcionesPCB.h"
#include "funcionesMemoria.h"

char * scr_ej="#!/usr/bin/ansisop"
		"begin"
		"	f"
		"end"
		""
		"function f"
		"	variables a"
		"	a=1"
		"	print a"
		"	g"
		"end"
		" "
		"function g"
		"	variables a"
		"	a=0"
		"	print a"
		"	f"
		"end";

///----INICIO SEMAFOROS----///
pthread_mutex_t mutex_HistoricoPcb; // deberia ser historico pid
pthread_mutex_t mutex_listaProcesos;
pthread_mutex_t mutex_cola_CPUs_libres;
pthread_mutex_t mutex_cola_New;
pthread_mutex_t mutex_cola_Ready;
pthread_mutex_t mutex_cola_Exec;
pthread_mutex_t mutex_cola_Finished;

sem_t* contadorDeCpus = 0;

sem_t sem_ConsolaKernelLenvantada;

void inicializarSemaforo(){
	sem_init(&mutex_HistoricoPcb,0,1);
	sem_init(&mutex_listaProcesos,0,1);
	sem_init(&mutex_cola_CPUs_libres,0,1);
	sem_init(&mutex_cola_New,0,1);
	sem_init(&mutex_cola_Exec,0,1);
	sem_init(&mutex_cola_Ready,0,1);
	sem_init(&mutex_cola_Finished,0,1);

	sem_init(&sem_ConsolaKernelLenvantada,0,0);
}
///----FIN SEMAFOROS----///



///-----FUNCIONES CONSOLA-------//


/// *** Falta probar! Necesitamos que ande el enviar mensajes
///*** Esta funcion le avisa a la consola que un cierto proceso (pid) ya termino
void consola_enviarAvisoDeFinalizacion(int socketConsola, int pid){
	printf("[Función consola_enviarAvisoDeFinalizacion] - Se Envía a Consola el pid: %d, porque ha finalizado!\n", pid);
	enviarMensaje(socketConsola,envioDelPidEnSeco,&pid,sizeof(int));
}


/// *** Funciona
/// *** Esta funcion finalizara todos los procesos que sean de una consola que se acaba de desconectar
void consola_finalizarTodosLosProcesos(int socketConsola){

	void cambiar(PROCESOS * process){
		if(process->socketConsola==socketConsola)
		{
			process->pcb->exitCode=-6;
			process->avisoAConsola=true;

			enviarMensaje(socketMemoria,finalizarPrograma,&process->pid,sizeof(int));


			int* joaquin;
			recibirMensaje(socketMemoria,&joaquin);
			free(joaquin);

			printf("Murio el proceso: %d\n", process->pid);
		}
	}

	bool busqueda(PROCESOS * process)	{
		return process->socketConsola==socketConsola;
	}

	list_iterate(avisos, cambiar);
}

////----FIN FUNCIONES CONSOLA-----///



///---FUNCIONES DEL KERNEL----//


/// *** Esta función Anda
//***Funciones de Planificador a largo plazo - Esta funcion te pasa los procesos, (que no hayan finalizado), a la cola de ready;
void newToReady(){

	printf("\n\n\nEstamos en la función newToReady a largo plazo!\n\n");

	//***Tomo el primer elemento de la cola sin sacarlo de ella
	sem_wait(&mutex_cola_New);
	PROCESOS* programaAnsisop = queue_peek(cola_New);

	//*** Valido si el programa ya fue finalizado! Si aun no fue finalizado, se procede a valirdar si se puede pasar a ready... En caso de estar finalizado ya se pasa a la cola de terminados
	if(programaAnsisop->pcb->exitCode == 53)
	{
		printf("Estructura:--\nPid: %d\nScript: %s\nSocketConsola:%d\n\n",programaAnsisop->pid,programaAnsisop->scriptAnsisop,programaAnsisop->socketConsola);

		//***Calculo cuantas paginas necesitara la memoria para este script
		int cant_paginas = memoria_CalcularCantidadPaginas(programaAnsisop->scriptAnsisop);

		INICIALIZAR_PROGRAMA dataParaMemoria;
		dataParaMemoria.cantPags=cant_paginas+stack_size;
		dataParaMemoria.pid=programaAnsisop->pid;

		//***Le Enviamos a memoria el pid con el que vamos a trabajar - Junto a la accion que vamos a realizar - Le envio a memeoria la cantidad de paginas que necesitaré reservar
		enviarMensaje(socketMemoria,inicializarPrograma, &dataParaMemoria, sizeof(int)*2); // Enviamos el pid a memoria

		int* ok=malloc(sizeof(int));
		recibirMensaje(socketMemoria, &ok); // Esperamos a que memoria me indique si puede guardar o no el stream
		if(*ok)
		{
			printf("\n\n[Funcion consola_recibirScript] - Memoria dio el Ok para el proceso recien enviado: Ok-%d\n", *ok);

			//***Quito el script de la cola de new
			queue_pop(cola_New);
			sem_post(&mutex_cola_New);

			//*** Divido el script en la cantidad de paginas necesarias
			char** scriptEnPaginas = memoria_dividirScriptEnPaginas(cant_paginas, programaAnsisop->scriptAnsisop);

			//***Le envio a memoria tiodo el scrip pagina a pagina
			int i;
			for(i=0; i<cant_paginas && *ok; i++)
			{
				enviarMensaje(socketMemoria,envioCantidadPaginas,scriptEnPaginas[i],size_pagina);
				printf("Envio una pagina: %d\n", i);

				recibirMensaje(socketMemoria,&ok);
			}

			//***Le envio a memoria las paginas del stack
			char * paginasParaElStack;
			// puto el que lee
			paginasParaElStack = string_repeat(' ',size_pagina);
			for(i=0; i<stack_size && *ok;i++)
			{
				enviarMensaje(socketMemoria,envioCantidadPaginas,paginasParaElStack,size_pagina);
				printf("Envio una pagina: %d\n", i);

				recibirMensaje(socketMemoria,&ok);
			}

			//***Termino de completar el PCB


			programaAnsisop->pcb->contPags_pcb= cant_paginas+stack_size;

			//***Añado el pcb a la cola de Ready
			queue_push(cola_Ready,programaAnsisop->pcb);
		}
		else
		{
			//***Como memoria no me puede guardar el programa, finalizo este proceso- Lo saco de la cola de new y lo mando a la cola de finished
			queue_pop(cola_New);
			programaAnsisop->pcb->exitCode=-1; // exit code por falta de memoria

			//***Le aviso a consola que se termino su programa por falta de memoria
			enviarMensaje(programaAnsisop->socketConsola,pidFinalizadoPorFaltaDeMemoria,&programaAnsisop->pid,sizeof(int));
			programaAnsisop->avisoAConsola=true;

			sem_post(&mutex_cola_New);

			sem_wait(&mutex_cola_Finished);
				queue_push(cola_Finished, programaAnsisop);
			sem_post(&mutex_cola_Finished);


			printf("[Funcion newToReady] - No hubo espacio para guardar en memoria!\n");
		}
		free(ok);
	}
	else
	{
		//***Como el proceso fue finalizado externamente se lo quita de la cola de new y se lo agrega a la cola de finalizados
		queue_pop(cola_New);

		sem_wait(&mutex_cola_Finished);
			queue_push(cola_Finished, programaAnsisop);
		sem_post(&mutex_cola_Finished);

		sem_post(&mutex_cola_New);
	}

	/// Que onda con programaAnsisop? No hay que liberarlo? Yo creo que no, onda perderia todas las referencias a los punteros... o no lo sé
}

///***Esta función tiene que buscar en todas las colas y fijarse donde esta el procesos y cambiar su estado a estado finalizado
bool proceso_finalizacionExterna(int pid, int exitCode)
{
	bool flag=false;
	sem_wait(&mutex_listaProcesos);

	bool busqueda(PROCESOS * aviso)
	{
		if(aviso->pid == pid)
			return true;

		return false;
	}
	PROCESOS* procesoAFianalizar =(PROCESOS*)list_find(avisos, busqueda);

	if( procesoAFianalizar != NULL){
		procesoAFianalizar->pcb->exitCode=exitCode;
		flag=true;
	}

	sem_post(&mutex_listaProcesos);

	return flag;
}




///---FIN FUNCIONES DEL KERNEL----//



///---RUTINAS DE HILOS----///

/// *** A esta función hay que probarle tuodo el sistema de envio de mensajes entre consola y kernel ( falla en el recivir mensaje que esta dentro del swich, nose porque no recibe mensajes
//***Esta rutina se levanta por cada consola que se cree. Donde se va a quedar escuchandola hasta que la misma se desconecte.
void *rutinaConsola(void * arg)
{

	int socketConsola = (int)arg;
	bool todaviaHayTrabajo = true;
	void * stream;
	printf("[Rutina rutinaConsola] - Entramos al hilo de la consola: %d!\n", socketConsola);

	while(todaviaHayTrabajo){
		int a = recibirMensaje(socketConsola,&stream);
	switch(a){
			case envioScriptAnsisop:{
				//***Estoy recibiendo un script para inicializar. Creo un neuvo proceso y ya comeizno a rellenarlo con los datos que ya tengo
				printf("[Rutina rutinaConsola] - Nuevo script recibido!\n");

				char* scripAnsisop = (char *)stream;
				printf("El stream es : %s /n",scripAnsisop);

				PROCESOS * nuevoPrograma = malloc(sizeof(PROCESOS));

				sem_wait(&mutex_HistoricoPcb);
					nuevoPrograma->pid= historico_pid;
					historico_pid++;
				sem_post(&mutex_HistoricoPcb);

				nuevoPrograma->scriptAnsisop = scripAnsisop;
				nuevoPrograma->socketConsola = socketConsola;
				nuevoPrograma->avisoAConsola = false;

				//***Creo el PCB
				PCB_DATA * pcbNuevo = crearPCB(nuevoPrograma->scriptAnsisop, nuevoPrograma->pid, 0);
				nuevoPrograma->pcb = pcbNuevo;

				//***Lo Agrego a la Cola de New
				sem_wait(&mutex_cola_New);
					queue_push(cola_New,nuevoPrograma);
				sem_post(&mutex_cola_New);

				//***Le envio a consola el pid del script que me acaba de enviar
				enviarMensaje(socketConsola,envioDelPidEnSeco,&nuevoPrograma->pid,sizeof(int));

				sem_wait(&mutex_listaProcesos);
					list_add(avisos,nuevoPrograma);
				sem_post(&mutex_listaProcesos);

				/* Cuando un consola envia un pid para finalizar, lo que vamos a hacer es una funci+on que cambie el estado de ese proceso a finalizado,
				 * de modo que en el mommento en que un proceso pase de cola en cola se valide como esta su estado, de estar en finalizado externamente
				 *  se pasa automaticamente ala cola de finalizados ---
				 *  Asi que se elimina la estructura de avisos de finalizacion y se agrega el elemento "finalizadoExternamente" a todos las estructuras
				 *  Tambien, cabe destacar que hay una sola estructura procesos
				 */
				break;
			}

			case finalizarCiertoScript:{

				//***Estoy recibiendo un script para finalizar, le digo a memoria que lo finalize y si sale bien le aviso a consola, sino tambien le aviso, pero que salio mal xd
				int pid = leerInt(stream);
				int* respuesta = malloc(sizeof(int));

				printf("Entramos a finalizar el script, del pid: %d\n", pid);

				//***Le digo a memoria que mate a este programa
				enviarMensaje(socketMemoria,finalizarPrograma, &pid,sizeof(int));//CAMBIAR

				//***Esta función actualizará el estado de finalizacion de un proceso
				if(proceso_finalizacionExterna(pid, -7) )
				{
					enviarMensaje(socketConsola,pidFinalizado, &pid,sizeof(int));

					//***Memoria me avisa si no encontro el pid
					recibirMensaje(socketMemoria, &respuesta);
					if(!respuesta){
						errorEn(respuesta, "[Rutina rutinaConsola] - La Memoria no pudo finalizar el proceso\n");
					}
				}
				else{
					errorEn(respuesta, "[Rutina rutinaConsola] - No existe el pid\n");
					enviarMensaje(socketConsola,errorFinalizacionPid, &pid,sizeof(int));
				}


				free(respuesta);
			}break;
			case desconectarConsola:{ // podria ser 0
				//***Se desconecta la consola
				//int pid = leerInt(stream);

				//***Ya no hay mas nada que hacer, entonces cambio el bool de todaviahaytrabajo a false, asi salgo del while
				todaviaHayTrabajo = false;

				consola_finalizarTodosLosProcesos(socketConsola);

			}break;
			case 0:{
				printf("[Rutina rutinaConsola] - La consola ha perecido\n");
						todaviaHayTrabajo=false;
			}break;
			default:{
				printf("[Rutina rutinaConsola] - Se recibio una accion que no esta contemplada:%d se cerrara el socket\n",a);
				todaviaHayTrabajo=false;
			}break;
		}
	}
	printf("Se decontecta a la consola socket: %d\n", socketConsola);
	close(socketConsola);
}


void cpu_actualizarPCBcolaExec(PCB_DATA *pcbDesdeCPU)
{
	sem_wait(&mutex_cola_Exec);

/*	bool busqueda(PCB_DATA* unPCB)
	{
		if(unPCB->pid == pcbDesdeCPU->pid)
			return true;

		return false;
	}
	PCB_DATA* pcbParaActualizar =(PCB_DATA*)list_find(cola_Exec, busqueda);
*/
	PCB_DATA* pcbParaActualizar = (PCB_DATA*)queue_peek(cola_Exec);

	if( pcbParaActualizar != NULL){

		pcbParaActualizar=pcbDesdeCPU;
	}

	sem_post(&mutex_cola_Exec);

}

/// *** Esta rutina se comenzará a hacer cuando podramos comenzar a enviar mensajes entre procesos
void *rutinaCPU(void * arg)
{
	int socketCPU = (int)arg;

	//*** Le envio a la CPU todos los datos qeu esta necesitara para poder trabajar, como el tamaño de una pagina de memoria, el quantum y la cantidad de paginas que ocupa un stack
	DATOS_PARA_CPU datosCPU;
	datosCPU.size_pag=size_pagina;
	datosCPU.quantum=quantumRR;
	datosCPU.size_stack=stack_size;
	enviarMensaje(socketCPU,enviarDatosCPU,&datosCPU,sizeof(int)*3);

	bool todaviaHayTrabajo = true;
	void * stream;
	int accionCPU;

	printf("[Rutina rutinaCPU] - Entramos al hilo de la CPU: %d!\n", socketCPU);

	//*** Voy a trabajar con esta CPU hasta que se deconecte
	while(todaviaHayTrabajo){

		//*** Recibo la accion por parte de la CPU
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			//*** La CPU me pide un PCB para poder trabajar
			case pedirPCB:{

				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);

				sem_wait(&mutex_cola_Exec);
					PCB_DATA* pcb = queue_peek(cola_Exec);
				sem_post(&mutex_cola_Exec);

				void* pcbSerializado = serializarPCB(pcb);

				enviarMensaje(socketCPU,envioPCB,pcbSerializado,tamanoPCB(pcb) + 4);

			}break;
			case enviarPCBaTerminado:{
				//TE MANDO UN PCB QUE YA TERMINE DE EJECUTAR POR COMPLETO, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO

				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU termino la ejecucion de un proceso: accion- %d!\n", enviarPCBaTerminado);

				PCB_DATA* pcb = deserializarPCB(stream);
				imprimirPCB(pcb);

				pcb->exitCode=0;
				cpu_actualizarPCBcolaExec(pcb);

				planificadorExtraLargoPlazo();
			}break;
			case enviarPCBaReady:{
				//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO

			}break;
			case mensajeParaEscribir:{
				//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:
				//IF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE
				//ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			}break;
			case waitSemaforo:{
				//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN WAIT, ME DEBERIAS DECIR SI ME BLOQUEO O NO
			}break;
			case signalSemaforo:{
				//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN SIGNAL, LE DEBERIAS INFORMAR A ALGUIEN SI ESTABA BLOQUEADO EN UN WAIT DE ESTE SEMAFORO
			}break;
			case asignarValorCompartida:{
				//TE MANDO UNA ESTRUCTURA CON {VALOR, NOMBRE_VARIABLE(CHAR*)} PARA QUE LE ASIGNES ESE VALOR A DICHA VARIABLE
			}break;
			case pedirValorCompartida:{
				//TE MANDO EL NOMBRE DE UNA VARIABLE COMPARTIDA Y ME DEBERIAS DEVOLVER SU VALOR
			}break;
			case 0:{
				printf("[Rutina rutinaCPU] - Desconecto la CPU N°: %d\n", socketCPU);
				todaviaHayTrabajo=false;
			}break;
			default:{
				printf("[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada: %d se cerrara el socket\n",accionCPU);
				todaviaHayTrabajo=false;
			}break;
		}
	}

	close(socketCPU);
}


/// *** Esta Función esta probada y anda
void * aceptarConexiones_Cpu_o_Consola( void *arg ){
	//----DECLARACION DE VARIABLES ------//
	int listener = (int)arg;
	int id_clienteConectado, nuevoSocket;
	int aceptados[] = {CPU, Consola};
	///-----FIN DECLARACION----///

	while(1)
	{
		//***Acepto una conexion
		id_clienteConectado = aceptarConexiones(listener, &nuevoSocket, Kernel, &aceptados, 2);

		pthread_t hilo_M;

		switch(id_clienteConectado)
		{
			case Consola: // Si es un cliente conectado es una CPU
			{
				printf("\n[rutina aceptarConexiones] - Nueva Consola Conectada!\nSocket Consola %d\n\n", nuevoSocket);

				/// CAmbiar este hilo a uno desetachable
				pthread_create(&hilo_M, NULL, rutinaConsola, nuevoSocket);

			}break;

			case CPU: // Si el cliente conectado es el cpu
			{
				printf("\n[rutina aceptarConexiones] - Nueva CPU Conectada\nSocket CPU %d\n\n", nuevoSocket);

				sem_wait(&mutex_cola_CPUs_libres);
					queue_push(cola_CPUs_libres,nuevoSocket);
				sem_post(&mutex_cola_CPUs_libres);

			}break;

			default:
			{
				printf("Papi, fijate se te esta conectado cualquier cosa\n");
				close(nuevoSocket);
			}
		}
	}
}
///--- FIN RUTINAS DE HILOS----///


///---- FUNCIONES DE PLANIFICACION ----///

///***Esta Función esta Probada y anda (falta meterle tres semaforos mutex)
//***Esta Función lo que hace es sumar el size de todas las colas que determinan el grado de multiplicacion y devuelve la suma
int cantidadProgramasEnProcesamiento()
{
	//HAcer Semaforos para todas las colas
	int cantidadProcesosEnLasColas = queue_size(cola_Ready)+queue_size(cola_Wait)+queue_size(cola_Exec);
	return cantidadProcesosEnLasColas;
}

//*** Esta función te dice si hay programas en new
bool hayProgramasEnNew()
{
	sem_wait(&mutex_cola_New);
		bool valor = queue_size(cola_New) > 0;
	sem_post(&mutex_cola_New);

	return valor;
}

//*** Rutina que te pasa los procesos de new a ready - anda bien
void * planificadorLargoPlazo()
{
	printf("\n[rutina planificadorLargoPlazo] - Entramos al planificador de largo plazo!\n");

	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		//*** Validamos que haya programas en alguna de la cola de new y que la cantidad de procesos que haya entre las colas de ready-excec-bloq sea menor al grado de multiprogramacion permitida
		if(cantidadProgramasEnProcesamiento() < grado_multiprogramacion && hayProgramasEnNew())
		{
			newToReady();
		}
//		sleep(5);
	}
}





//*** Esta funcion pasa todos los procesos que esten finalizados dentro de la cola de exec a la cola de Finished, y retorna el pid del proceso que se acaba de mover de cola, sino encontro ninguno retorna -1
int execToFinished()
{
	//**Tomo el primer elemento de la lista,
	PCB_DATA* pcb=queue_peek(cola_Exec);

	//Valido que el proceso haya finalizado

	/// DEBERIA VALIDAR ADEMAS DE ESTO; SI EL PROCESO YA FUE FINALIZADO!!!! TENGO QUE VER TODOS LOS EXITCODE de FINALIZACIONES QUE NO SEAN DE CPU Y PONERLOS EN EL IF
	if(pcb->exitCode != 53){

		//*** si el proceso ya finalizo, lo saco de la cola de exec y lo paso a la cola de finished
		queue_pop(cola_Exec);

		sem_wait(&mutex_cola_Finished);
		queue_push(cola_Finished,pcb);
		sem_post(&mutex_cola_Finished);

		//*** Retorno el pid del proceso que acabo de psar a finalizado
		return  pcb->pid;
	}

	return -1;
}


void planificadorExtraLargoPlazo(){
	int pidParaAvisar;

	bool busqueda(PROCESOS * aviso)
	{
		if(aviso->pid == pidParaAvisar)
			return true;

		return false;
	}

	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
//	while(!finPorConsolaDelKernel)
	{
		sem_wait(&mutex_cola_Exec);

		//***Validamos que haya procesos en la cola de exec
		if(queue_size(cola_Exec)>0){

			//***Llamamos a la funcion excetToReady que pasa los procesos que ya hallan finalizado a la cola de finalizados, y retorna el pid del proceso que acaba de pasar. No haber encotrado ninguno retorna -1
			pidParaAvisar = execToFinished();
			sem_post(&mutex_cola_Exec);

			//*** Si hay un pid para avisar
			if(pidParaAvisar>0){

				//*** Buscamos el procesos en la lista de procesos, a travez del pid que acaba de ser psado a finalizado
				PROCESOS* procesoFinalizado =(PROCESOS*)list_find(avisos, busqueda);

				//*** En caso de haber encontrado el proceso con el pid finalizado
				if( procesoFinalizado != NULL){

					//*** Valido que no haya sido avisada la consola anteriormente
					if(!procesoFinalizado->avisoAConsola)
					{

					// Si la consola no habia sido avisada le envio el mensaje del pid que acaba de finalizar
//					enviarMensaje(procesoFinalizado->socketConsola,procesoFinalizado,&procesoFinalizado->pid,sizeof(int));	ACA FALTa VER COMO LO RECIBE JULY (consola)

					printf("Se acaba de mandar a la consola n°: %d, que el proceso %d acaba de finalizar exitosamente!\n", procesoFinalizado->socketConsola, procesoFinalizado->pid);

					// y ahora pongo que el este proceso ya le aviso a su consola
					procesoFinalizado->avisoAConsola=true;
					}
				}
			}

		}
		else{
			sem_post(&mutex_cola_Exec);
		}

//		sleep(5);
	}
}






//*** Esta funcion te dice si hay cpus disponibles
bool hayCpusDisponibles(){
	sem_wait(&mutex_cola_CPUs_libres);
		bool valor = queue_size(cola_CPUs_libres) > 0;
	sem_post(&mutex_cola_CPUs_libres);

	return valor;
}

//*** Esta funcion te dice si hay programas en ready
bool hayProcesosEnReady(){
	sem_wait(&mutex_cola_Ready);
		bool valor = queue_size(cola_Ready) > 0;
	sem_post(&mutex_cola_Ready);

	return valor;
}


//**** Esta funcion anda bien, el tema es que tengo comentados todos los semaforos, nose porque tiran error
///*** Quito el primer elemento de la cola de ready, valido que no haya sido finalizado y lo pongo en la cola de exec
PCB_DATA* readyToExec()
{
	sem_wait(&mutex_cola_Ready);
	sem_wait(&mutex_cola_Exec);

	//*** Tomo el primer elemento de la cola de ready y lo quito
	PCB_DATA* pcb = queue_peek(cola_Ready);
	queue_pop(cola_Ready);

	//*** Valido que el proceso no haya sido terminado ya
	if(pcb->exitCode != 53)
	{
		//*** Si ya fue finalizado lo paso a la cola de finalizados
		sem_wait(&mutex_cola_Finished);
			queue_push(cola_Finished,pcb);
		sem_post(&mutex_cola_Finished);

		//*** valido si aun quedan procesos en la cola de ready para seguir buscando un pcb para trabajar
		if(queue_size(cola_Ready)>0){

			//*** Si como aun quedan porcesos en la cola de ready vuelvo a llamar a la funcion tomarPCBdeReady
			sem_post(&mutex_cola_Exec);
			sem_post(&mutex_cola_Ready);
			pcb = readyToExec();
		}
		else
		{
			//** En caso de que ya no haya mas procesos para trabajar, devuelvo null
			sem_post(&mutex_cola_Exec);
			sem_post(&mutex_cola_Ready);
			return NULL;
		}
	}

	//*** Como el proceso que encontre no esta terminado, entonces lo pongo en la cola de excec y lo retorno
	queue_push(cola_Exec,pcb);

	sem_post(&mutex_cola_Exec);
	sem_post(&mutex_cola_Ready);
	return pcb;
}

void * planificadorCortoPlazo()
{
	printf("\n[Rutina planificadorCortoPlazo] - Entramos al planificador de corto plazo!\n");

	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		if(hayCpusDisponibles() && hayProcesosEnReady())
		{
			PCB_DATA* pcb;
			///*** Quito el primer elemento de la cola de ready, valido que no haya sido finalizado y lo pongo en la cola de exec - en caso de no encontrar uno para poder trabajar no hago nada
			if((pcb = readyToExec()) != NULL)
			{
				//*** Agarro una cpu que este disponible
				sem_wait(&mutex_cola_CPUs_libres);
					int cpuParaTrabajar = queue_peek(cola_CPUs_libres);
					queue_pop(cola_CPUs_libres);
				sem_post(&mutex_cola_CPUs_libres);

				printf("[Rutina readyToExec] - Se crea un nuevo hilo para que la CPU N°: %d trabaje con el proceso N°: %d\n", cpuParaTrabajar, pcb->pid);


				//*** Llamo a una rutina CPU que va a estar trabajando con la cpu que le paso por parametro -- Cambiar tipo de hilo
				pthread_t hilo_rutinaCPU;
				pthread_create(&hilo_rutinaCPU, NULL, rutinaCPU, cpuParaTrabajar);
			}

		}

//		sleep(5);
	}
}



///---- FIN FUNCIONES DE PLANIFICACION ----///


///---- CONSOLA DEL KERNEL -----////

//**Esta funcion anda
///***Esta funcion imprime todos los pids de sistema
void imprimirTodosLosProcesosEnColas(){

	void imprimir (PROCESOS * aviso)
	{
		printf("Pid: %d\n", aviso->pid);

		if(aviso->pcb->exitCode == 53)
			printf("Estado: en procesamiento\n");
		else
			printf("Estado: finalizado (%d)\n",aviso->pcb->exitCode);
	}

	list_iterate(avisos, imprimir);
}

//*** probar esta funcion - no anda, arreglar
///*** Esta funcion dada una cola te imprime todos los procesos que esta contenga
void imprimirProcesosdeCola(t_queue* unaCola)
{
	void imprimir(PCB_DATA * pcb){
		printf("Pid: %d\n", pcb->pid);
	}

	list_iterate(unaCola, imprimir);
}

void * consolaKernel()
{
	int opcion;
	printf("Hola Bienvenido al Kernel!\n\n"
			"Aca esta el menu de todas las opciones que tiene para hacer:\n"
			"1- Obtener el listado de procesos del sistema de alguna cola.\n"
			"2- Obtener datos sobre un proceso.\n"
			"3- Obtener la tabla global de archivos.\n"
			"4- Modificar el grado de multiprogramación del sistema.\n"
			"5- Finalizar un proceso.\n"
			"6- Detener la planificación.\n"
			"7- Reactivar la planificación.\n"
			"8- Imprimir de nuevo el menu.\n\n"
			"Elija el numero de su opcion: ");
	sem_post(&sem_ConsolaKernelLenvantada);
	scanf("%d", &opcion);


	while(1)
	{
		switch(opcion){
			case 1:{
				printf("Selecione la cola que quiere imprimir:\n"
						"1- Cola de New.\n"
						"2- Cola de Ready.\n"
						"3- Cola de Exec.\n"
						"4- Cola de Bloq.\n"
						"5- Cola de Finish.\n"
						"6- Todas las colas.\n"
						"Elija el numero de su opcion: ");
						scanf("%d", &opcion);

				switch(opcion){
					case 1:{
						printf("Procesos de la cola de New:\n");

					}break;
					case 2:{
						printf("Procesos de la cola de Ready:\n");


					}break;
					case 3:{
						printf("Procesos de la cola de Exec:\n");


					}break;
					case 4:{
						printf("Procesos de la cola de Bloq:\n");


					}break;
					case 5:{
						printf("Procesos de la cola de Finish:\n");

					}break;
					case 6:{
						printf("Estos son todos los procesos:\n");

						sem_wait(&mutex_listaProcesos);
							imprimirTodosLosProcesosEnColas();
						sem_post(&mutex_listaProcesos);
					}break;
					default:{
						printf("Opcion invalida! Intente nuevamente.\n");
					}break;
				}


			}break;
			case 2:{
				int pid;
				printf("Ingrese pid del proceso a finalizar: ");
				scanf("%d",&pid);
/*
				2. Obtener para un proceso dado:
				a. La cantidad de rafagas ejecutadas.
				b. La cantidad de operaciones privilegiadas que ejecutó.
				c. Obtener la tabla de archivos abiertos por el proceso.
				d. La cantidad de páginas de Heap utilizadas
				i. Cantidad de acciones alocar realizadas en cantidad de operaciones y en
				bytes
				ii. Cantidad de acciones liberar realizadas en cantidad de operaciones y en
				bytes
				e. Cantidad de syscalls ejecutadas*/
			}break;
			case 3:{
				/// ni idea que es esto
			}break;
			case 4:{
				int gradoNuevo;
				printf("Ingrese nuevo Grado de multiprogramacion: ");
				scanf("%d",&gradoNuevo);

				//probablemente tengamos qeu poner un semaforo para la variable global de grado multiprogramacion
				grado_multiprogramacion=gradoNuevo;
			}break;
			case 5:{
				int pid;
				printf("Ingrese pid del proceso a finalizar: ");
				scanf("%d",&pid);

				if(proceso_finalizacionExterna( pid,  -50681)) //cambiar el numero del exit code, por el que sea el correcto
					printf("Finalizacion exitosa.\n");
				else
					printf("El Pid %d es Incorrecto! Reeintente con un nuevo pid.\n",pid);

			}break;
			case 6:{
				finPorConsolaDelKernel=true;
				printf("Planificacion detenida.\n");
			}break;
			case 7:{
				finPorConsolaDelKernel=false;
				printf("Planificacion reactivada.\n");
			}break;
			case 8:{
				printf("Hola Bienvenido al Kernel!\n\n"
						"Aca esta el menu de todas las opciones que tiene para hacer:\n"
						"1- Obtener el listado de procesos del sistema.\n"
						"2- Obtener datos sobre un proceso.\n"
						"3- Obtener la tabla global de archivos.\n"
						"4- Modificar el grado de multiprogramación del sistema.\n"
						"5- Finalizar un proceso.\n"
						"6- Detener la planificación.\n"
						"7- Imprimir de nuevo el menu.\n\n");
			}break;
			default:{

			}break;
		}

		printf("Elija el numero de su opcion: ");
		scanf("%d", &opcion);
	}
}

///---- FIN CONSOLA KERNEL----////


/*
-SOLUCIONAR LO DE LAS ETIQUETAS.
-HACER EN EL KERNEL TODAS LAS OPERACIONES QUE PIDE EL CPU:
-WAIT
-SIGNAL
-VALOR COMPARTIDAS
-ASIGNAR COMPARTIDAS
-ESCRIBIR (POR AHORA SOLO POR CONSOLA)





-MANEJAR LOS PRINTS DE CONSOLA
-MANEJAR LA FINALIZACION DE PROGRAMAS POR CONSOLA

*/
///--------FUNCIONES DE CONEXIONES-------////
//***Estas funciones andan
//*** Esta función se conecta con memoria y recibe de ella el tamaño de las paginas
void conectarConMemoria()
{
	bool flag=true;

	while(flag)
	{
		int rta_conexion;
		socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria
		if (socketMemoria == 1){
			perror("Falla en el protocolo de comunicación");
		}
		if (socketMemoria == 2){
			perror("No se conectado con Memoria, asegurese de que este abierto el proceso");
		}
		rta_conexion = handshakeCliente(socketMemoria, Kernel);

		if ( rta_conexion != Memoria) {
			perror("Error en el handshake con Memoria");
			close(socketMemoria);
			exit(-1);
		}
		else{
			printf("Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);

			int* sizePag;
			recibirMensaje(socketMemoria, &sizePag);

			size_pagina = *sizePag;

			printf("[Conexion con Memoria] - El Size pag es: %d", size_pagina);

			free(sizePag);

			flag=false;
		}
	}
}
void conectarConFS()
{
	int rta_conexion;
	socketFS = conexionConServidor(getConfigString("PUERTO_FS"),getConfigString("IP_FS")); // Asignación del socket que se conectara con el filesytem
	if (socketFS == 1){
		perror("Falla en el protocolo de comunicación");
	}
	if (socketFS == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
	}
	if ( (rta_conexion = handshakeCliente(socketFS, Kernel)) == -1) {
		perror("Error en el handshake con FileSystem");
		close(socketFS);
	}
	printf("Conexión exitosa con el FileSystem(%i)!!\n",rta_conexion);
}
///---- FIN FUNCIONES DE CONEXIONES------////



/*  SEM_IDS=[SEM1, SEM2, SEM3]
	SEM_INIT=[1, 1 ,1]
*/

int saberTamanoArray()
{
	return 3;
}

char** ids_semaforos;
int** init_semaforos;
int cant_semaforos;

int main(void) {
	printf("Inicializando Kernel.....\n\n");


	///------INICIALIZO TO.DO-------------///
		historico_pid=1;
		finPorConsolaDelKernel=false;

		//***Inicializo las listas
		avisos = list_create();

		//***Inicializo las colas
		cola_New = queue_create();
		cola_Ready = queue_create();
		cola_Wait = queue_create();
		cola_Exec = queue_create();
		cola_Finished = queue_create();

		cola_CPUs_libres = queue_create();

		//***Inicializo los semaforos
		inicializarSemaforo();

		//***Lectura e impresion de los archivos de configuracion
		printf("Configuracion Inicial: \n");
		configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config");
		imprimirConfiguracion();

		grado_multiprogramacion = getConfigInt("GRADO_MULTIPROG");
		stack_size = getConfigInt("STACK_SIZE");
		quantumRR = (strcmp("FIFO",getConfigString("ALGORITMO")) == 0)? -1 : getConfigInt("QUANTUM"); // con amor, niñita


		// Es muy bueno sizeof(a)/sizeof(*a)
		ids_semaforos = getConfigStringArray("SEM_IDS");
		cant_semaforos= 3; // tendriamos qeu tener una cuenta o algo asi


	///-----------------------------////



	//---CONECTANDO CON FILESYSTEM Y MEMORIA
	printf("\n\n\nEsperando conexiones:\n-FileSystem\n-Memoria\n");
	conectarConMemoria();
//	conectarConFS();
	///----------------------///

	///---CREO EL LISTENER Y LO PONGO A ESCUCHAR POR PRIMERA VEZ-----///
	int listener;     // Socket principal
	listener = crearSocketYBindeo(getConfigString("PUERTO_PROG"));
	escuchar(listener); // poner a escuchar ese socket
	///----------------------//

	//---ABRO LA CONSOLA DEL KERNEL---//
	pthread_t hilo_consolaKernel;
	pthread_create(&hilo_consolaKernel, NULL, consolaKernel, NULL);
	sem_wait(&sem_ConsolaKernelLenvantada);


	//---ABRO EL HILO DEL PLANIFICADOR A LARGO PLAZO---//
	pthread_t hilo_planificadorLargoPlazo;
	pthread_create(&hilo_planificadorLargoPlazo, NULL, planificadorLargoPlazo, NULL);


	//---ABRO EL HILO DEL PLANIFICADOR A CORTO PLAZO---//
	pthread_t hilo_planificadorCortoPlazo;
	pthread_create(&hilo_planificadorCortoPlazo, NULL, planificadorCortoPlazo, NULL);


	//----ME PONGO A ESCUCHAR CONEXIONES---//
	pthread_t hilo_aceptarConexiones_Cpu_o_Consola;
	pthread_create(&hilo_aceptarConexiones_Cpu_o_Consola, NULL, aceptarConexiones_Cpu_o_Consola, listener);


	pthread_join(hilo_aceptarConexiones_Cpu_o_Consola, NULL);
	pthread_join(hilo_planificadorCortoPlazo, NULL);
	pthread_join(hilo_planificadorLargoPlazo, NULL);
	pthread_join(hilo_consolaKernel, NULL);

	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	///-----LIBERO LA CONFIGURACION
	liberarConfiguracion();

	return 0;
}
