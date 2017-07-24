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
//Juli cambiale el nombre al semaforo aReady y aNew sino no va andar una mierda

PROCESOS* buscarProceso(int pid){
	return list_get(avisos,pid - 1);
}

PCB_DATA* quitarPCBDeCola(t_queue* cola, int pid){
	bool busqueda(PCB_DATA* pcb){
		return pcb->pid == pid;
	}
	return list_remove_by_condition(cola->elements, busqueda);
}


PCB_DATA* quitarPCBDeDondeEste(PCB_DATA* pcb){
	switch(pcb->estadoDeProceso){
	case 0:{
			PROCESOS* proceso = list_get(avisos, pcb->pid - 1);
			return proceso->pcb;
		}break;
	case new:{
		bool busqueda(PROCESOS* proceso){
				return proceso->pid == pcb->pid;
			}
			return list_remove_by_condition(cola_New->elements, busqueda);
		}break;
	case ready:{
			return quitarPCBDeCola(cola_Ready,pcb->pid);
		}break;
	case enCPU:
	case exec:{
			return quitarPCBDeCola(cola_Exec,pcb->pid);
		}break;
	case Wait:{
			return quitarPCBDeCola(cola_Wait,pcb->pid);
		}break;
	//por el finish no hace nada, no se saca xdxdxd
	default: return NULL; break;
	}
}

bool moverA(int pid, int movimientoACola){

	PROCESOS * proceso = buscarProceso(pid);
	if(proceso == NULL)
		return false;
	if(proceso->pcb->estadoDeProceso == finish)
		return false;

	quitarPCBDeDondeEste(proceso->pcb);

	switch(movimientoACola)
	{
		case aNew:{
			proceso->pcb->estadoDeProceso=new;
			queue_push(cola_New,proceso);

		}break;

		case aWait:{
			proceso->pcb->estadoDeProceso=Wait;
			queue_push(cola_Wait, proceso->pcb);
		}break;
		case aReady:{
			proceso->pcb->estadoDeProceso=ready;
			queue_push(cola_Ready, proceso->pcb);
		}break;
		case aExec:{
			proceso->pcb->estadoDeProceso=exec;
			queue_push(cola_Exec, proceso->pcb);
		}break;
		case aFinished:{
			proceso->pcb->estadoDeProceso=finish;
			queue_push(cola_Finished, proceso->pcb);
		}break;
		default:{
			log_warning(logKernel, "[MoverA]: se quiere hacer un movimiento a un estado inexistente");
		}break;
	}
	return true;
}


///***Esta función tiene que buscar en todas las colas y fijarse donde esta el procesos y cambiar su estado a estado finalizado
bool proceso_Finalizar_conAviso(int pid, int exitCode, bool conAvisoAConsola)
{
	bool flag=false;

	bool busqueda(PROCESOS * aviso)
	{
		return aviso->pid == pid;
	}


	PROCESOS* procesoAFianalizar = list_find(avisos, busqueda);
	if(procesoAFianalizar != NULL && procesoAFianalizar->pcb->estadoDeProceso == enCPU ){
		procesoAFianalizar->pcb->estadoDeProceso = aFinalizar;
	}

	else  if(procesoAFianalizar != NULL && procesoAFianalizar->pcb->estadoDeProceso != finish){

		if(procesoAFianalizar->pcb->estadoDeProceso != new){

			sem_wait(&mutex_gradoDeMultiprogramacion);
			numeroGradoDeMultiprogramacion++;
			sem_post(&mutex_gradoDeMultiprogramacion);
			sem_post(&gradoDeMultiprogramacion);
		}

		int socketConsola = procesoAFianalizar->socketConsola;

		proceso_liberarRecursos(procesoAFianalizar);

		procesoAFianalizar->pcb->exitCode = exitCode;
		moverA(procesoAFianalizar->pid,aFinished);

		enviarMensaje(socketMemoria,finalizarPrograma,&pid,sizeof(int));
		void* respuesta;
		recibirMensaje(socketMemoria,&respuesta);
		free(respuesta);

		if(conAvisoAConsola){
			proceso_avisarAConsola(socketConsola, pid, exitCode);
		}

		flag=true;
	}
	else{
		log_error(logKernel,"[proceso_Finalizar] - Hubo un error al finalizar el pid: %d", pid);
	}

	return flag;
}

bool proceso_Finalizar(int pid, int exitCode){
	return proceso_Finalizar_conAviso( pid, exitCode, true);
}

void proceso_avisarAConsola(int socketConsola, int pid, int exitCode){
		enviarMensaje(socketConsola, pidFinalizado, &pid, sizeof(int));
		log_info(logKernel,"Se acaba de mandar a la consola n°: %d, que el proceso %d acaba de finalizar con exit code: %d\n", socketConsola, pid, exitCode);
}


void liberarProcesoDeSemaforo(char* nombreSEM, PCB_DATA* pcb){

	t_semaforo* sem = buscarSemaforo(nombreSEM);

	if(sem == NULL){
		pcb->estadoDeProceso=exec;
		proceso_Finalizar(pcb->pid,intentoAccederAUnSemaforoInexistente);
	}

	sem->valor++;

	bool busqueda(int* pid){
		return *pid == pcb->pid;
	}

	list_remove_and_destroy_by_condition(sem->cola->elements,busqueda,free);
}

bool liberarSemaforo(PROCESOS* proceso){
	sem_wait(&mutex_semaforos_ANSISOP);
	if(proceso->semBloqueante != NULL){
		liberarProcesoDeSemaforo(proceso->semBloqueante,proceso->pcb);
	}
	sem_post(&mutex_semaforos_ANSISOP);

	return false;
}


void proceso_liberarRecursos(PROCESOS* proceso){

	if(liberarRecursosHeap(proceso->pid)== 0){
		log_info(logKernel,"No se liberaron los recursos del heap correctamenete del pid %d\n", proceso->pid);
	}
	else{
		log_info(logKernel,"Se liberaron correctamente los recursos del heap del pid %d\n",proceso->pid);
	}

	//(liberarRecursosArchivo(proceso->pcb->pid)){
	//	log_info(logKernel,"No se liberaron los recursos del de los archivos correctamenete del pid %d\n", proceso->pid);
	//}
	//else{
	//	log_info(logKernel,"Se liberaron correctamente de los archivos del heap del pid %d\n",proceso->pid);
	//}

	if(liberarSemaforo(proceso)){
		log_info(logKernel,"No se liberaron los recursos d correctamenete del pid %d\n", proceso->pid);
	}
	else{
		log_info(logKernel,"Se liberaron correctamente los recursos de los semaforos del pid %d\n", proceso->pid);
	}
}

bool proceso_EstaFinalizado(int pid)
{
	bool busqueda(PROCESOS * aviso){
	  return aviso->pid == pid;
	 }
	 PCB_DATA* pcb = ((PROCESOS*)list_find(avisos, busqueda))->pcb;
	 return pcb->estadoDeProceso == finish;
}

///---FIN FUNCIONES DEL KERNEL----//




///---- FUNCIONES DE PLANIFICACION ----///



/// ********************************************************************************************************///
/// ***************************** PRIMER PARTE - PASAR PROCESOS DE NEW A READY *****************************///
/// ********************************************************************************************************///

//***Funciones de Planificador a largo plazo - Esta funcion te pasa los procesos, (que no hayan finalizado), a la cola de ready; - anda bien
void newToReady(){

	log_info(logKernel,"\n\n\nEstamos en la función newToReady a largo plazo!\n\n");
	//***Tomo el primer elemento de la cola sin sacarlo de ella

	//***Quito el script de la cola de new
	sem_wait(&mutex_listaProcesos);
		PROCESOS* programaAnsisop = queue_peek(cola_New);
	//*** Valido si el programa ya fue finalizado! Si aun no fue finalizado, se procede a valirdar si se puede pasar a ready... En caso de estar finalizado ya se pasa a la cola de terminados
	if(programaAnsisop != NULL){
		if(programaAnsisop->pcb->estadoDeProceso == new)
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


				//*** Divido el script en la cantidad de paginas necesarias
				programaAnsisop->pcb->contPags_pcb= cant_paginas+getConfigInt("STACK_SIZE");
				programaAnsisop->pcb->cantPaginasDeCodigo = cant_paginas;

				//***Añado el pcb a la cola de Ready
				moverA(programaAnsisop->pid,aReady);
				sem_post(&mutex_listaProcesos);
				sem_wait(&mutex_gradoDeMultiprogramacion);
				numeroGradoDeMultiprogramacion--;
				sem_post(&mutex_gradoDeMultiprogramacion);
				//***Le envio a memoria tiodo el scrip pagina a pagina
				int i;

				for(i=0; i<cant_paginas ; i++)
				{
					enviarMensaje(socketMemoria,envioCantidadPaginas,scriptEnPaginas[i],size_pagina);
					free(ok);
					recibirMensaje(socketMemoria,&ok);
					free(scriptEnPaginas[i]);
				}
				free(scriptEnPaginas);


				//***Le envio a memoria las paginas del stack
				char * paginasParaElStack;
				paginasParaElStack = string_repeat(' ',size_pagina);
				paginasParaElStack[size_pagina-1]='\0';
				for(i=0; i<getConfigInt("STACK_SIZE")&&*ok;i++)
				{
					free(ok);
					enviarMensaje(socketMemoria,envioCantidadPaginas,paginasParaElStack,size_pagina);
					//printf("Envio una pagina: %d\n", i+cant_paginas);

					recibirMensaje(socketMemoria,&ok);
				}
				free(ok);
				free(paginasParaElStack);
				//***Termino de completar el PCB

				filaEstadisticaDeHeap* fila = malloc(sizeof(filaEstadisticaDeHeap));
				fila->pid = programaAnsisop->pid;
				fila->tamanoAlocadoEnBytes = 0;
				fila->tamanoAlocadoEnOperaciones = 0;
				fila->cantidadDePaginasHistoricasPedidas =0;
				fila->tamanoLiberadoEnBytes = 0;
				fila->tamanoLiberadoEnOperaciones = 0;
				sem_wait(&mutex_tabla_estadistica_de_heap);
				list_add(tablaEstadisticaDeHeap,fila);
				sem_post(&mutex_tabla_estadistica_de_heap);

				sem_post(&cantidadDeProgramasEnReady);
			}
			else
			{
				//***Como memoria no me puede guardar el programa, finalizo este proceso
				proceso_Finalizar(programaAnsisop->pid, pidFinalizadoPorFaltaDeMemoria);

				sem_post(&mutex_listaProcesos);
				log_info(logKernel,"[Funcion newToReady] - No hubo espacio para guardar en memoria el proceso pid: %d\n", programaAnsisop->pid);
				free(ok);
			}
		}
		else
		{
			//***Como el proceso fue finalizado externamente se lo agrega a la cola de finalizados
			moverA(programaAnsisop->pid,aFinished);
			sem_post(&mutex_listaProcesos);
		}
	}
	else{
		sem_post(&mutex_listaProcesos);
		sem_wait(&mutex_gradoDeMultiprogramacion);
		numeroGradoDeMultiprogramacion++;
		sem_post(&mutex_gradoDeMultiprogramacion);
		sem_post(&gradoDeMultiprogramacion);
	}
}


//***Esta Función lo que hace es sumar el size de todas las colas que determinan el grado de multiplicacion y devuelve la suma ///***Esta Función esta Probada y anda (falta meterle tres semaforos mutex)



//*** Rutina que te pasa los procesos de new a ready - anda bien
void * estadoNEW()
{
	//printf("\n[rutina estadoNEW] - Entramos al planificador de largo plazo!\n");
	log_info(logKernel,"\n[rutina estadoNEW] - Entramos al planificador de largo plazo!\n");
	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{
		sem_wait(&programasEnNew);
		sem_wait(&gradoDeMultiprogramacion);
		sem_wait(&mutex_gradoDeMultiprogramacion);

		if(numeroGradoDeMultiprogramacion>0){
			sem_post(&mutex_gradoDeMultiprogramacion);

			newToReady();
		}
		else{
			sem_post(&mutex_gradoDeMultiprogramacion);
			sem_post(&programasEnNew);
		}
		//*** Validamos que haya programas en alguna de la cola de new y que la cantidad de procesos que haya entre las colas de ready-excec-bloq sea menor al grado de multiprogramacion permitida
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
void readyToExec()
{
	//*** Tomo el primer elemento de la cola de ready y lo quito
	sem_wait(&mutex_listaProcesos);

	PCB_DATA* pcb = queue_peek(cola_Ready);

	if(pcb != NULL)
			moverA(pcb->pid, aExec);
	sem_post(&cantidadDeProgramasEnExec);

	sem_post(&mutex_listaProcesos);
}


//*** pasar procesos de ready a exec
void * estadoReady()
{
//	printf("\n[Rutina planificadorCortoPlazo] - Entramos al planificador de corto plazo!\n");
	log_info(logKernel,"\n[Rutina planificadorCortoPlazo] - Entramos al planificador de corto plazo!\n");
	//*** el booleano finPorConsolaDelKernel esta en false desde el inicio, en el momento en el que el kernel quiera frenar la planificiacion esta variable pasara a true, y se frenara la planificacion
	while(!finPorConsolaDelKernel)
	{

		sem_wait(&cantidadDeProgramasEnReady);
		sem_wait(&cpuDisponible);

		///*** Quito el primer elemento de la cola de ready, valido que no haya sido finalizado y lo pongo en la cola de exec - en caso de no encontrar uno para poder trabajar no hago nada
		readyToExec();
	}
	return NULL;
}


/// ********************************************************************************************************///
/// ***************************************** FIN SEGUNDA PARTE ********************************************///
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

				pthread_t hilo_rutinaCPU;

				cpu_crearHiloDetach(nuevoSocket);
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

		//***Lectura e impresion de los archivos de configuracion
		printf("Configuracion Inicial: \n");
		configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config");
		inicializarSemaforo();
		imprimirConfiguracion();
		cargarSemaforosDesdeConfig();
		cargarVariablesGlobalesDesdeConfig();

		quantumRR = (strcmp("FIFO",getConfigString("ALGORITMO")) == 0)? -1 : getConfigInt("QUANTUM");
		quantumSleep = getConfigInt("QUANTUM_SLEEP");
		numeroGradoDeMultiprogramacion = getConfigInt("GRADO_MULTIPROG");
	///-----------------------------////



	//---CONECTANDO CON FILESYSTEM Y MEMORIA
	log_info(logKernel,"\n\n\nEsperando conexiones:\n-FileSystem\n-Memoria\n");

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


	pthread_t hilo_Inotify;
	pthread_create(&hilo_Inotify, NULL,INotify, NULL);


	//----ME PONGO A ESCUCHAR CONEXIONES---//
	pthread_t hilo_aceptarConexiones_Cpu_o_Consola;
	pthread_create(&hilo_aceptarConexiones_Cpu_o_Consola, NULL, aceptarConexiones_Cpu_o_Consola, (void*)listener);


	pthread_join(hilo_aceptarConexiones_Cpu_o_Consola, NULL);

	pthread_join(hilo_estadoNEW, NULL);

	pthread_join(hilo_consolaKernel, NULL);

	log_destroy(logKernel);

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

	sem_init(&mutex_tablaDeHeap,0,1);

	sem_init(&mutex_tablaGlobalDeArchivos,0,1);
	sem_init(&mutex_tablaGlobalDeArchivosDeProcesos,0,1);

	sem_init(&mutex_semaforos_ANSISOP,0,1);
	sem_init(&mutex_variables_compartidas,0,1);
	sem_init(&mutex_Quantum_Sleep,0,1);
	sem_init(&mutex_tabla_estadistica_de_heap,0,1);


	sem_init(&programasEnNew,0,0);
	sem_init(&gradoDeMultiprogramacion,0,getConfigInt("GRADO_MULTIPROG"));
	sem_init(&cantidadDeProgramasEnExec,0,0);
	sem_init(&cantidadDeProgramasEnReady,0,0);

	sem_init(&mutex_gradoDeMultiprogramacion,0,1);
	sem_init(&cpuDisponible,0,0);

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
			log_info(logKernel,"Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);

			int* sizePag;
			recibirMensaje(socketMemoria, &sizePag);

			size_pagina = *sizePag;

			log_info(logKernel,"[Conexion con Memoria] - El Size pag es: %d", size_pagina);

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
	log_info(logKernel,"Conexión exitosa con el FileSystem(%i)!!\n",rta_conexion);
}

///---- FIN FUNCIONES DE CONEXIONES------////

