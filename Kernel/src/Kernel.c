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

#include "datosGlobales.h"
#include "funcionesPCB.h"



///----INICIO SEMAFOROS----///
pthread_mutex_t mutex_HistoricoPcb; // deberia ser historico pid
pthread_mutex_t mutex_listaProcesos;
pthread_mutex_t mutex_cola_New;
pthread_mutex_t nuevaCPU;
pthread_mutex_t mutex_cola_Finished;

sem_t* contadorDeCpus = 0;

void inicializarSemaforo(){
	sem_init(&mutex_HistoricoPcb,0,1);
	sem_init(&mutex_listaProcesos,0,1);
	sem_init(&mutex_cola_New,0,1);
	sem_init(&nuevaCPU,0,0);
	sem_init(&mutex_cola_Finished,0,1);
}
///----FIN SEMAFOROS----///



///-----FUNCIONES CONSOLA-------//



/// *** Falta probar! Necesitamos que ande el enviar mensajes
void consola_enviarAvisoDeFinalizacion(int socketConsola, int pid){
	printf("[Función consola_enviarAvisoDeFinalizacion] - Se Envía a Consola el pid: %d, porque ha finalizado!\n", pid);
	enviarMensaje(socketConsola,envioDelPidEnSeco,&pid,sizeof(int));
}


/// *** Falta probar! Necesitamos que ande el enviar mensajes
void consola_finalizarTodosLosProcesos(int socketConsola){

	void cambiar(PROCESOS * process){
		if(process->socketConsola==socketConsola)
		{
			process->pcb->exitCode=-6;

			process->finalizadoExternamente=true;

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

/// *** Esta función esta probada y quasi-anda --- no anda uno de los freee de de adentro, que ahora esta comentado.. El programa sigue andando pero podemos tener memory leaks
/*void liberar_Programa_En_New(PROGRAMAS_EN_NEW * programaNuevo)
{
	free(programaNuevo->scriptAnsisop); El que no este esta cosa puede traer memory leaks
	free(programaNuevo);
}*/

/// *** Esta función esta probada y anda
//*** Esta funcion te divide el scriptAnsisop en una cantidad de paginas dadas, el size de cada pagina esta en el config
char** memoria_dividirScriptEnPaginas(int cant_paginas, char *copiaScriptAnsisop)
{
	char * scriptDivididoEnPaginas[cant_paginas];
	int i;
	for(i=0;i<cant_paginas;i++){
		scriptDivididoEnPaginas[i] = malloc(size_pagina);
		memcpy(scriptDivididoEnPaginas[i],copiaScriptAnsisop+i*size_pagina,size_pagina);
		printf("[memoria_dividirScriptEnPaginas] - %s",scriptDivididoEnPaginas[i]);
	}
	if(strlen(scriptDivididoEnPaginas[i-1]) < size_pagina){
		char* x = string_repeat(' ',size_pagina-strlen(scriptDivididoEnPaginas[i-1]));
		string_append(&(scriptDivididoEnPaginas[i-1]),x);
	}
	return scriptDivididoEnPaginas;
}

/// *** Esta función esta probada y anda
//***Esta Funcion te devuelve la cantidad de paginas que se van a requerir para un scriptAsisop dado
int memoria_CalcularCantidadPaginas(char * scriptAnsisop)
{
  return  ceil(((double)(strlen(scriptAnsisop))/((double) size_pagina)));
}


/// *** A esta función solo le faltan dos cosas, 1- la funcion consola_finalizacionPorNoMemoria ; (no esta desarrollada) y porbar tuodos los enviar y recibir mensajes con memoria
//***Funciones de Planificador
void newToReady(){

	printf("\n\n\nEstamos en la función newToReady a largo plazo!\n\n");

	//***Tomo el primer elemento de la cola sin sacarlo de ella
	sem_wait(&mutex_cola_New);
	PROCESOS* programaAnsisop = queue_peek(cola_New);

	//*** Valido si el programa ya fue finalizado! Si aun no fue finalizado, se procede a valirdar si se puede pasar a ready... En caso de estar finalizado ya se pasa a la cola de terminados
	if(!programaAnsisop->finalizadoExternamente)
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
			programaAnsisop->finalizadoExternamente=true;
			programaAnsisop->pcb->exitCode=-1; // exit code por falta de memoria

			sem_wait(&mutex_cola_Finished);
				queue_push(cola_Finished, programaAnsisop);
			sem_post(&mutex_cola_Finished);

			//***Le aviso a consola que se termino su programa por falta de memoria
			enviarMensaje(programaAnsisop->socketConsola,pidFinalizadoPorFaltaDeMemoria,&programaAnsisop->pid,sizeof(int));

			sem_post(&mutex_cola_New);
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
		procesoAFianalizar->finalizadoExternamente=true;
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
				nuevoPrograma->finalizadoExternamente = false;

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

typedef struct{
	int socketCPU;
	bool ocupada;
}CPUS_EN_USO;

/// *** Esta rutina se comenzará a hacer cuando podramos comenzar a enviar mensajes entre procesos
void *rutinaCPU(void * arg)
{
	int socketCPU = (int)arg;

	bool todaviaHayTrabajo = true;
	void * stream;
	int accionCPU;

	printf("[Rutina rutinaCPU] - Entramos al hilo de la CPU: %d!\n", socketCPU);

	while(todaviaHayTrabajo){
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			case pedirPCB:{

				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);

				PCB_DATA* pcb = queue_peek(cola_Ready);

				enviarMensaje(socketCPU,envioPCB,pcb,sizeof(PCB_DATA));

				queue_pop(cola_Ready);
				queue_push(cola_Exec,pcb);

				// POner semaforos

			}break;

			case 0:{
				printf("[Rutina rutinaCPU] - Desconecta la CPU\n");
				todaviaHayTrabajo=false;
			}break;
			default:{
				printf("[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada:%d se cerrara el socket\n",accionCPU);
				todaviaHayTrabajo=false;
			}break;
		}
	}

	close(socketCPU);
}

typedef struct{
	int size_pag;
	int quantum;
	int size_stack;
}__attribute__((packed)) DATOS_PARA_CPU;

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
				printf("\nNueva Consola Conectada!\nSocket Consola %d\n\n", nuevoSocket);
				pthread_create(&hilo_M, NULL, rutinaConsola, nuevoSocket);

			}break;

			case CPU: // Si el cliente conectado es el cpu
			{
				printf("Nueva CPU Conectada\nSocket CPU %d\n\n", nuevoSocket);

				DATOS_PARA_CPU datosCPU;

				datosCPU.size_pag=size_pagina;
				datosCPU.quantum=quantumRR;
				datosCPU.size_stack=stack_size;

				enviarMensaje(nuevoSocket,enviarDatosCPU,&datosCPU,sizeof(int)*3);

				printf("Socket CPU %d\n\n", nuevoSocket);

				rutinaCPU(nuevoSocket);
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
	printf("Entramos al planificador de largo plazo!\n");
	while(1)
	{
		if(cantidadProgramasEnProcesamiento() < getConfigInt("GRADO_MULTIPROG") && hayProgramasEnNew())
		{
			newToReady();
		}
		sleep(5);
	}
}

void * planificadorCortoPlazo()
{
	while(1)
	{
		sem_wait(&nuevaCPU);
		sleep(1);
		printf("Entramos al planificador de corto plazo!\n");


		printf("No nos da el grado de multi programación ni hay programas en new!\n");
		//getConfigIntArrayElement("SEM_IDS",2);
		//	setConfigInt("GRADO_MULTIPROG",3);
	}
}



///---- FIN FUNCIONES DE PLANIFICACION ----///


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

int main(void) {
	printf("Inicializando Kernel.....\n\n");

	///------INICIALIZO TO.DO-------------///
		historico_pid=1;

		//***Inicializo las listas
		avisos = list_create();

		//***Inicializo las colas
		cola_New = queue_create();
		cola_Ready = queue_create();
		cola_Wait = queue_create();
		cola_Exec = queue_create();
		cola_Finished = queue_create();

		//***Inicializo los semaforos
		inicializarSemaforo();

		//***Lectura e impresion de los archivos de configuracion
		printf("Configuracion Inicial: \n");
		configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Kernel/kernel.config");
		imprimirConfiguracion();

		stack_size = getConfigInt("STACK_SIZE");
		quantumRR = -1 ;//getConfigInt("QUANTUM");
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



	//---ABRO EL HILO DEL PLANIFICADOR A LARGO PLAZO---//
	pthread_t hilo_planificadorLargoPlazo;
	pthread_create(&hilo_planificadorLargoPlazo, NULL, planificadorLargoPlazo, NULL);

	//----ME PONGO A ESCUCHAR CONEXIONES---//
	pthread_t hilo_aceptarConexiones_Cpu_o_Consola;
	pthread_create(&hilo_aceptarConexiones_Cpu_o_Consola, NULL, aceptarConexiones_Cpu_o_Consola, listener);


	pthread_join(hilo_aceptarConexiones_Cpu_o_Consola, NULL);
	pthread_join(hilo_planificadorLargoPlazo, NULL);

	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	///-----LIBERO LA CONFIGURACION
	liberarConfiguracion();

	return 0;
}
