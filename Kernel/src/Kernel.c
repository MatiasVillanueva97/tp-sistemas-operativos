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
#include "iNotify.h"
#include <parser/parser.h>
#include <parser/metadata_program.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"
#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.c"
#include "../../Nuestras/src/laGranBiblioteca/funcionesParaTodosYTodas.c"

#include "datosGlobales.h"
#include "funcionesPCB.h"
#include "funcionesMemoria.h"
#include "funcionesConsolaKernel.h"
#include "funcionesSemaforosYCompartidas.h"
#include "funcionesCapaFS.h"
#include "funcionesCPU.h"


void inicializarSemaforo();
void conectarConMemoria();
void conectarConFS();

///----FIN SEMAFOROS----///


///---FUNCIONES DEL KERNEL----//


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
		procesoAFianalizar->pcb->estadoDeProceso=finalizado;
		flag=true;
	}

	sem_post(&mutex_listaProcesos);

	return flag;
}


///---FIN FUNCIONES DEL KERNEL----//




///---- FUNCIONES DE PLANIFICACION ----///



/// ********************************************************************************************************///
/// ***************************** PRIMER PARTE - PASAR PROCESOS DE NEW A READY *****************************///
/// ********************************************************************************************************///

//***Funciones de Planificador a largo plazo - Esta funcion te pasa los procesos, (que no hayan finalizado), a la cola de ready; - anda bien
void newToReady(){

	printf("\n\n\nEstamos en la función newToReady a largo plazo!\n\n");
	log_info(logKernel,"\n\n\nEstamos en la función newToReady a largo plazo!\n\n");
	//***Tomo el primer elemento de la cola sin sacarlo de ella
	sem_wait(&mutex_cola_New);
	sem_wait(&mutex_cola_Ready);
	PROCESOS* programaAnsisop = queue_peek(cola_New);

	//*** Valido si el programa ya fue finalizado! Si aun no fue finalizado, se procede a valirdar si se puede pasar a ready... En caso de estar finalizado ya se pasa a la cola de terminados
	if(programaAnsisop->pcb->estadoDeProceso == paraEjecutar)
	{
		log_info(logKernel,"Estructura:--\nPid: %d\nScript: %s\nSocketConsola:%d\n\n",programaAnsisop->pid,programaAnsisop->scriptAnsisop,programaAnsisop->socketConsola);

		//***Calculo cuantas paginas necesitara la memoria para este script
		int cant_paginas = memoria_CalcularCantidadPaginas(programaAnsisop->scriptAnsisop);
		char** scriptEnPaginas = memoria_dividirScriptEnPaginas(cant_paginas,programaAnsisop->scriptAnsisop);
		INICIALIZAR_PROGRAMA dataParaMemoria;
		dataParaMemoria.cantPags=cant_paginas+getConfigInt("STACK_SIZE");
		dataParaMemoria.pid=programaAnsisop->pid;

		//***Le Enviamos a memoria el pid con el que vamos a trabajar - Junto a la accion que vamos a realizar - Le envio a memeoria la cantidad de paginas que necesitaré reservar
		enviarMensaje(socketMemoria,inicializarPrograma, &dataParaMemoria, sizeof(int)*2); // Enviamos el pid a memoria

		int* ok;
		recibirMensaje(socketMemoria, &ok); // Esperamos a que memoria me indique si puede guardar o no el stream
		if(*ok)
		{
			log_info(logKernel,"\n\n[Funcion consola_recibirScript] - Memoria dio el Ok para el proceso recien enviado: Ok-%d\n", *ok);

			//***Quito el script de la cola de new
			queue_pop(cola_New);
			sem_post(&mutex_cola_New);

			//*** Divido el script en la cantidad de paginas necesarias


			//***Le envio a memoria tiodo el scrip pagina a pagina
			int i;

			for(i=0; i<cant_paginas ; i++)
			{
				enviarMensaje(socketMemoria,envioCantidadPaginas,scriptEnPaginas[i],size_pagina);
				log_info(logKernel,"Envio una pagina: %d\n", i);
				log_info(logKernel,"La pagina %d, contiene:",i);
				log_info(logKernel,scriptEnPaginas[i]);
				free(ok);
				recibirMensaje(socketMemoria,&ok);
				free(scriptEnPaginas[i]);
			}
			free(scriptEnPaginas);

			//***Le envio a memoria las paginas del stack
			char * paginasParaElStack;
			// puto el que lee
			paginasParaElStack = string_repeat(' ',size_pagina);
			paginasParaElStack[size_pagina-1]='\0';
			for(i=0; i<getConfigInt("STACK_SIZE")&&*ok;i++)
			{
				free(ok);
				enviarMensaje(socketMemoria,envioCantidadPaginas,paginasParaElStack,size_pagina);
				printf("Envio una pagina: %d\n", i+cant_paginas);

				recibirMensaje(socketMemoria,&ok);
			}
			free(ok);
			free(paginasParaElStack);
			//***Termino de completar el PCB

			filaEstadisticaDeHeap* fila = malloc(sizeof(filaEstadisticaDeHeap));
			fila->pid = programaAnsisop->pid;
			fila->tamanoAlocadoEnBytes = 0;
			fila->tamanoAlocadoEnOperaciones = 0;
			fila->tamanoLiberadoEnBytes = 0;
			fila->tamanoAlocadoEnOperaciones = 0;
			fila->cantidadDePaginasHistoricasPedidas =0;

			sem_wait(&mutex_tabla_estadistica_de_heap);
			list_add(tablaEstadisticaDeHeap,fila);
			sem_post(&mutex_tabla_estadistica_de_heap);
			programaAnsisop->pcb->contPags_pcb= cant_paginas+getConfigInt("STACK_SIZE");

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
			log_info(logKernel,"[Funcion newToReady] - No hubo espacio para guardar en memoria!\n");
		free(ok);
		}
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
	sem_post(&mutex_cola_Ready);

	/// Que onda con programaAnsisop? No hay que liberarlo? Yo creo que no, onda perderia todas las referencias a los punteros... o no lo sé
}


//***Esta Función lo que hace es sumar el size de todas las colas que determinan el grado de multiplicacion y devuelve la suma ///***Esta Función esta Probada y anda (falta meterle tres semaforos mutex)
int cantidadProgramasEnProcesamiento()
{
	sem_wait(&mutex_cola_Ready);
	sem_wait(&mutex_cola_Exec);
	sem_wait(&mutex_cola_Wait);
		int cantidadProcesosEnLasColas = queue_size(cola_Ready)+queue_size(cola_Wait)+queue_size(cola_Exec);
	sem_post(&mutex_cola_Wait);
	sem_post(&mutex_cola_Exec);
	sem_post(&mutex_cola_Ready);

	return cantidadProcesosEnLasColas;
}


//*** Esta función te dice si hay programas en new - anda bien
bool hayProgramasEnNew()
{
	sem_wait(&mutex_cola_New);
		bool valor = queue_size(cola_New) > 0;
	sem_post(&mutex_cola_New);

	return valor;
}


//*** Rutina que te pasa los procesos de new a ready - anda bien
void * estadoNEW()
{
	printf("\n[rutina estadoNEW] - Entramos al planificador de largo plazo!\n");
	log_info(logKernel,"\n[rutina estadoNEW] - Entramos al planificador de largo plazo!\n");
	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		//*** Validamos que haya programas en alguna de la cola de new y que la cantidad de procesos que haya entre las colas de ready-excec-bloq sea menor al grado de multiprogramacion permitida
		if(cantidadProgramasEnProcesamiento() <  getConfigInt("GRADO_MULTIPROG") && hayProgramasEnNew())
		{
			newToReady();
		}
	}

	return NULL;
}


/// ********************************************************************************************************///
/// ***************************************** FIN PRIMERA PARTE ********************************************///
/// ********************************************************************************************************///







/// ********************************************************************************************************///
/// **************************** SEGUNDA PARTE - PASAR PROCESOS DE READY A EXEC ****************************///
/// ********************************************************************************************************///


///*** Quito el primer elemento de la cola de ready, valido que no haya sido finalizado y lo pongo en la cola de exec //**** Esta funcion anda bien,
PCB_DATA* readyToExec()
{
	sem_wait(&mutex_cola_Ready);
	sem_wait(&mutex_cola_Exec);

	//*** Tomo el primer elemento de la cola de ready y lo quito
	PCB_DATA* pcb = queue_pop(cola_Ready);


	//*** Valido que el proceso no haya sido terminado ya
	if(pcb->estadoDeProceso == finalizado)
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
			return readyToExec();
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


//*** Esta funcion te dice si hay programas en ready
bool hayProcesosEnReady(){
 sem_wait(&mutex_cola_Ready);
  bool valor = queue_size(cola_Ready) > 0;
 sem_post(&mutex_cola_Ready);

 return valor;
}


//*** Esta funcion te dice si hay cpus disponibles
bool hayCpusDisponibles(){

	//SI LA CPU ESTA ESPERANDO TRABAJO es porque esta disponible y espera un PCB para trabajar
	int sumaSi(t_CPU* cpu){
		if(cpu->esperaTrabajo)
			return 1;
		return 0;
	}

	sem_wait(&mutex_cola_CPUs_libres);
		bool valor = sum(lista_CPUS,sumaSi) > 0;
	sem_post(&mutex_cola_CPUs_libres);

	return valor;
}


//*** pasar procesos de ready a exec
void * estadoReady()
{
	printf("\n[Rutina planificadorCortoPlazo] - Entramos al planificador de corto plazo!\n");
	log_info(logKernel,"\n[Rutina planificadorCortoPlazo] - Entramos al planificador de corto plazo!\n");
	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		if(hayCpusDisponibles() && hayProcesosEnReady())
		{
			///*** Quito el primer elemento de la cola de ready, valido que no haya sido finalizado y lo pongo en la cola de exec - en caso de no encontrar uno para poder trabajar no hago nada
			readyToExec();
		}
	}
	return NULL;
}


/// ********************************************************************************************************///
/// ***************************************** FIN SEGUNDA PARTE ********************************************///
/// ********************************************************************************************************///








/// ********************************************************************************************************///
/// *********************** TERCERA PARTE - PASAR PROCESOS DE EXEC A FINISHED O BLOQ ***********************///
/// ********************************************************************************************************///


//*** Esta funcion pasa todos los procesos que esten finalizados dentro de la cola de exec a la cola de Finished, y retorna el pid del proceso que se acaba de mover de cola, sino encontro ninguno retorna -1
void execTo()
{
	//**Tomo el primer elemento de la lista,
	PCB_DATA* pcb=queue_peek(cola_Exec);

	if(pcb != NULL){
		//***Valido que el proceso haya finalizado
		if(pcb->estadoDeProceso == finalizado){
			queue_pop(cola_Exec);
			//*** si el proceso ya finalizo lo paso a la cola de finished
			sem_wait(&mutex_cola_Finished);
				queue_push(cola_Finished,pcb);
			sem_post(&mutex_cola_Finished);

		}
		else{
			//*** Valido que el proceso este bloqueado, si lo esta lo mando a wait
			if(pcb->estadoDeProceso == bloqueado){
				queue_pop(cola_Exec);
				//*** si el proceso esta  bloqueado lo paso ala cola de bloqueado
				sem_wait(&mutex_cola_Wait);
					queue_push(cola_Wait,pcb);
				sem_post(&mutex_cola_Wait);

			}
		}
	}
}


//*** esta funcion te pasa las cosas de excet a finshed, sea el caso que sea y te manda el mensaje a cnsola de cada cosa que acaba de mover -- Aunque esta accion de enviar a consola las cosas terminadas no deberia estar aca.. este hilo va a cambiar muchisimo
void* estadoEXEC(){
	int pidParaAvisar;

	bool busqueda(PROCESOS * aviso)
	{
		if(aviso->pid == pidParaAvisar)
			return true;

		return false;
	}

	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		sem_wait(&mutex_cola_Exec);

		//***Validamos que haya procesos en la cola de exec
		if(queue_size(cola_Exec)>0){

			//***Llamamos a la funcion excetToReady que pasa los procesos que ya hallan finalizado a la cola de finalizados, y retorna el pid del proceso que acaba de pasar. No haber encotrado ninguno retorna -1
			execTo();
			sem_post(&mutex_cola_Exec);
		}
		else{
			sem_post(&mutex_cola_Exec);
		}
	}
	return NULL;
}


/// ********************************************************************************************************///
/// ***************************************** FIN TERCERA PARTE ********************************************///
/// ********************************************************************************************************///





/// ********************************************************************************************************///
/// **************************** CUARTA PARTE - PASAR PROCESOS DE WAIT A READY *****************************///
/// ********************************************************************************************************///

void* estadoWAIT(){
	int pidParaAvisar;

	bool busqueda(PROCESOS * aviso)
	{
		if(aviso->pid == pidParaAvisar)
			return true;

		return false;
	}

	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		sem_wait(&mutex_listaProcesos);
		sem_wait(&mutex_cola_Wait);

		//***Validamos que haya procesos en la cola de exec
		if(queue_size(cola_Wait)>0){

			PCB_DATA* pcbDeWait = queue_pop(cola_Wait);

			sem_post(&mutex_cola_Wait);

			if(pcbDeWait->estadoDeProceso == paraEjecutar)
			{
				sem_wait(&mutex_cola_Ready);
					queue_push(cola_Ready, pcbDeWait);
				sem_post(&mutex_cola_Ready);
			}
			else{
				if(pcbDeWait->estadoDeProceso == finalizado){
					sem_wait(&mutex_cola_Finished);
						queue_push(cola_Finished,pcbDeWait);
					sem_post(&mutex_cola_Finished);
				}
				else{
					sem_wait(&mutex_cola_Wait);
						queue_push(cola_Wait, pcbDeWait);
					sem_post(&mutex_cola_Wait);
				}
			}
		}
		else{
			sem_post(&mutex_cola_Wait);
		}
		sem_post(&mutex_listaProcesos);

	}
	return NULL;
}


/// ********************************************************************************************************///
/// ***************************************** FIN CUARTA PARTE ********************************************///
/// ********************************************************************************************************///







/// ********************************************************************************************************///
/// **************************** QUINTA PARTE - TRABAJAR PROCESOS EN FINISHED ******************************///
/// ********************************************************************************************************///

void proceso_avisarAConsola(){

	bool busqueda(PROCESOS * process)	{
		return (process->pcb->estadoDeProceso == finalizado && !process->avisoAConsola);
	}

	sem_wait(&mutex_listaProcesos);

	// Si la consola no habia sido avisada le envio el mensaje del pid que acaba de finalizar
	PROCESOS* procesoFinalizado = list_find(avisos, busqueda);

	sem_post(&mutex_listaProcesos);

	if(procesoFinalizado != NULL)
	{
		//proceso_liberarRecursos(procesoFinalizado->pcb);
		enviarMensaje(procesoFinalizado->socketConsola, pidFinalizado, &procesoFinalizado->pid, sizeof(int));
		log_info(logKernel,"Se acaba de mandar a la consola n°: %d, que el proceso %d acaba de finalizar con exit code: %d\n", procesoFinalizado->socketConsola, procesoFinalizado->pid, procesoFinalizado->pcb->exitCode);
		printf("Se acaba de mandar a la consola n°: %d, que el proceso %d acaba de finalizar con exit code: %d\n", procesoFinalizado->socketConsola, procesoFinalizado->pid, procesoFinalizado->pcb->exitCode);

		// y ahora pongo que el este proceso ya le aviso a su consola
		procesoFinalizado->avisoAConsola=true;
	}

}

void liberarSemaforo(int pid){
	sem_wait(&mutex_listaProcesos);
			bool buscar(PROCESOS* proceso){
				return proceso->pid == pid;
			}
			PROCESOS* proceso = list_find(avisos, buscar);
			if(proceso->semaforoTomado != NULL)
				SEM_signal(proceso->semaforoTomado, proceso->pcb);
			sem_post(&mutex_listaProcesos);
}

void proceso_liberarRecursos(PCB_DATA* pcb){

	if(liberarRecursosHeap(pcb->pid)== 0){
		printf("No se liberaron los recursos del heap correctamenete del pid %d\n", pcb->pid);
	}
	else{
		printf("Se liberaron correctamente los recursos del heap del pid %d\n",pcb->pid);
	}
	liberarRecursosArchivo(pcb);
	liberarSemaforo(pcb->pid);//deberian ser varios
	enviarMensaje(socketMemoria,finalizarPrograma,&pcb->pid,sizeof(int));
	void* respuesta;
	recibirMensaje(socketMemoria,&respuesta);
	free(respuesta);

}

///avisa a al consola que un proceso termino
void * estadoFINISHED()
{


	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		///***Reviso si algun proceso que esta en finalizado aun no le aviso a su consola que ya finalizo
		proceso_avisarAConsola();

	}
	return NULL;
}


/// ********************************************************************************************************///
/// ***************************************** FIN QUINTA PARTE ********************************************///
/// ********************************************************************************************************///








///---- FIN FUNCIONES DE PLANIFICACION ----///




/// *** Esta Función esta probada y anda - Te acepta CPUs y las encola, o te hacepta consolas y te la levanta en hilos dettachables
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
				log_info(logKernel,"\n[rutina aceptarConexiones] - Nueva Consola Conectada!\nSocket Consola %d\n\n", nuevoSocket);

				consola_crearHiloDetach(nuevoSocket);

			}break;

			case CPU: // Si el cliente conectado es el cpu
			{
				log_info(logKernel,"\n[rutina aceptarConexiones] - Nueva CPU Conectada\nSocket CPU %d\n\n", nuevoSocket);

				t_CPU* nuevaCPU = malloc(sizeof(t_CPU));

				nuevaCPU->socketCPU = nuevoSocket;
				nuevaCPU->esperaTrabajo = true;


				sem_wait(&mutex_cola_CPUs_libres);
					list_add(lista_CPUS,nuevaCPU);
				sem_post(&mutex_cola_CPUs_libres);

				pthread_t hilo_rutinaCPU;
				cpu_crearHiloDetach(nuevaCPU->socketCPU);
			}break;

			default:
			{
				log_error(logKernel,"[rutina aceptarConexiones_CPU_o_Consola] - Se esta conectado cualquier cosa, algo que no es ni cpu ni consola\n");
				close(nuevoSocket);
			}
		}
	}
}

int main(void) {
	printf("Inicializando Kernel.....\n\n");

	logKernel= log_create("Kernel.log","Kernel",0,0);
	///------INICIALIZO TO.DO-------------///
		historico_pid=1;
		finPorConsolaDelKernel=false;

		//***Inicializo las listas
		avisos = list_create();
		lista_CPUS = list_create();
		//listaDeEsperaSemaforos = list_create();
		tablaDeHeapMemoria = list_create();
		tablaGlobalDeArchivos = list_create();
		tablaGlobalDeArchivosDeProcesos= list_create();
		tablaEstadisticaDeHeap = list_create();
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
		cargarSemaforosDesdeConfig();
		cargarVariablesGlobalesDesdeConfig();

		quantumRR = (strcmp("FIFO",getConfigString("ALGORITMO")) == 0)? -1 : getConfigInt("QUANTUM");
		quantumSleep = getConfigInt("QUANTUM_SLEEP");

	///-----------------------------////



	//---CONECTANDO CON FILESYSTEM Y MEMORIA
	printf("\n\n\nEsperando conexiones:\n-FileSystem\n-Memoria\n");
	conectarConMemoria();
	conectarConFS();
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



	pthread_t hilo_estadoNEW;
	pthread_create(&hilo_estadoNEW, NULL, estadoNEW, NULL);


	pthread_t hilo_estadoReady;
	pthread_create(&hilo_estadoReady, NULL, estadoReady, NULL);


	pthread_t hilo_estadoEXEC;
	pthread_create(&hilo_estadoEXEC, NULL, estadoEXEC, NULL);


	pthread_t hilo_estadoWAIT;
	pthread_create(&hilo_estadoWAIT, NULL, estadoWAIT, NULL);


	pthread_t hilo_estadoFINISHED;
	pthread_create(&hilo_estadoFINISHED, NULL, estadoFINISHED, NULL);

	pthread_t hilo_Inotify;
	pthread_create(&hilo_Inotify, NULL,INotify, NULL);
	//----ME PONGO A ESCUCHAR CONEXIONES---//
	pthread_t hilo_aceptarConexiones_Cpu_o_Consola;
	pthread_create(&hilo_aceptarConexiones_Cpu_o_Consola, NULL, aceptarConexiones_Cpu_o_Consola, (void*)listener);


	pthread_join(hilo_aceptarConexiones_Cpu_o_Consola, NULL);
	pthread_join(hilo_estadoReady, NULL);
	pthread_join(hilo_estadoNEW, NULL);
	pthread_join(hilo_estadoEXEC, NULL);
	pthread_join(hilo_estadoWAIT, NULL);
	pthread_join(hilo_estadoFINISHED,NULL);
	pthread_join(hilo_consolaKernel, NULL);

	log_destroy(logKernel);
	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	///-----LIBERO LA CONFIGURACION
	liberarConfiguracion();

	return 0;
}



// *************************************************************************************************************************************************** //
// ****************************************************** COSAS MOLESTAS PERO NECESARIAS ************************************************************* //
// *************************************************************************************************************************************************** //

void inicializarSemaforo(){
	sem_init(&mutex_HistoricoPcb,0,1);
	sem_init(&mutex_listaProcesos,0,1);
	sem_init(&mutex_cola_CPUs_libres,0,1);
	sem_init(&mutex_cola_New,0,1);
	sem_init(&mutex_cola_Ready,0,1);
	sem_init(&mutex_cola_Wait,0,1);
	sem_init(&mutex_cola_Exec,0,1);
	sem_init(&mutex_cola_Finished,0,1);
	sem_init(&mutex_tablaDeHeap,0,1);

	sem_init(&mutex_tablaGlobalDeArchivos,0,1);
	sem_init(&mutex_tablaGlobalDeArchivosDeProcesos,0,1);

	sem_init(&mutex_semaforos_ANSISOP,0,1);
	sem_init(&mutex_variables_compartidas,0,1);
	sem_init(&mutex_Quantum_Sleep,0,1);
	sem_init(&mutex_tabla_estadistica_de_heap,0,1);


	sem_init(&sem_ConsolaKernelLenvantada,0,0);
}


///--------FUNCIONES DE CONEXIONES-------////

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
		exit(-1);
	}
	if (socketFS == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		perror("Error en el handshake con FileSystem");
		exit(-1);
	}
	if ( (rta_conexion = handshakeCliente(socketFS, Kernel)) != 4) {
		perror("Error en el handshake con FileSystem");
		close(socketFS);
	}
	printf("Conexión exitosa con el FileSystem(%i)!!\n",rta_conexion);
}

///---- FIN FUNCIONES DE CONEXIONES------////

