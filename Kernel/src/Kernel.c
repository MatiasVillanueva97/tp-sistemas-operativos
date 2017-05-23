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
pthread_mutex_t mutex_ListaDeAvisos;
pthread_mutex_t mutex_cola_New;
pthread_mutex_t nuevaCPU;

sem_t* contadorDeCpus = 0;

void inicializarSemaforo(){
	sem_init(&mutex_HistoricoPcb,0,1);
	sem_init(&mutex_ListaDeAvisos,0,1);
	sem_init(&mutex_cola_New,0,1);
	sem_init(&nuevaCPU,0,0);
}
///----FIN SEMAFOROS----///



///------FUNCIONES Y PROTOCOLOS COMUNES----//
/// *** Esto Funciona
/*void errorEn(int valor,char * donde){
	if(valor == -1)
		printf("%s\n", donde);
}*/

//---FIN FUNCIONES COMUNES---//





///-----FUNCIONES CONSOLA-------//

/// *** Falta probar!
//***Esta funcion busca todos los procesos que tiene una consola
t_list * avisosDeConsola(int socketConsola){
	bool buscarPorSocket(AVISO_FINALIZACION * aviso){
		return (aviso->socketConsola == socketConsola);
	}
	return list_filter(avisos, buscarPorSocket);
}

/// *** Falta probar! Necesitamos que ande el enviar mensajes
void consola_enviarAvisoDeFinalizacion(int socketConsola, int pid){
	printf("[Función consola_enviarAvisoDeFinalizacion] - Se Envía a Consola el pid: %d, porque ha finalizado!\n", pid);
	enviarMensaje(socketConsola,envioDelPidEnSeco,&pid,sizeof(int));
}

/// *** Falta probar! Necesitamos que ande el enviar mensajes
///***Me falto borrarlo de la lista de avisos
void consola_finalizacionPorDirectiva(int socketConsola, int pid, int idError){

	PCB_DATA * pcb = buscarPCBPorPidYBorrar(pid);
	if(pcb != NULL){
		pcb->exitCode = idError;
		queue_push(cola_Finished,pcb);
		//**Le mando un 1 diciendo que salio todao ok
		enviarMensaje(socketConsola,1,1,sizeof(int));
		consola_enviarAvisoDeFinalizacion(socketConsola, pid);
	}
	else
		//***Le mando un 0 diciendo que no pude encontrar el pcb
		enviarMensaje(socketConsola,1,0,sizeof(int));
}

/// *** Falta probar! Necesitamos que ande el enviar mensajes
void consola_finalizarTodosLosProcesos(int socketConsola){
	t_list * avisosConsola = avisosDeConsola(socketConsola);
	void modificar(AVISO_FINALIZACION * aviso){
		aviso->finalizado = true;
		consola_finalizacionPorDirectiva(socketConsola, aviso->pid, -6);
	}
	list_map(avisosConsola, modificar);
}
////----FIN FUNCIONES CONSOLA-----///




///---FUNCIONES DEL KERNEL----//

/// *** Esta función esta probada y quasi-anda --- no anda uno de los freee de de adentro, que ahora esta comentado.. El programa sigue andando pero podemos tener memory leaks
void liberar_Programa_En_New(PROGRAMAS_EN_NEW * programaNuevo)
{
	//free(programaNuevo->scriptAnsisop); El que no este esta cosa puede traer memory leaks
	free(programaNuevo);
}


/// *** Esta función esta probada y anda
//*** Esta funcion te divide el scriptAnsisop en una cantidad de paginas dadas, el size de cada pagina esta en el config
/*char** memoria_dividirScriptEnPaginas(int cant_paginas, char *copiaScriptAnsisop)
{
	char * scriptDivididoEnPaginas[cant_paginas];
	int i;
	for(i=0;i<cant_paginas;i++){
		scriptDivididoEnPaginas[i] = malloc(size_pagina);
		memcpy(scriptDivididoEnPaginas[i],copiaScriptAnsisop+i*size_pagina,size_pagina);
		printf("[memoria_dividirScriptEnPaginas] - %s",scriptDivididoEnPaginas[i]);
	}
	return scriptDivididoEnPaginas;
}*/
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
	PROGRAMAS_EN_NEW* programaAnsisop = queue_peek(cola_New);

	if(!programaAnsisop->finalizado)
	{
		printf("Estructura:--\nPid: %d\nScript: %s\nSocketConsola:%d\n\n",programaAnsisop->pid_provisorio,programaAnsisop->scriptAnsisop,programaAnsisop->socketConsola);

		//***Calculo cuantas paginas necesitara la memoria para este script
		int cant_paginas = memoria_CalcularCantidadPaginas(programaAnsisop->scriptAnsisop);

		INICIALIZAR_PROGRAMA dataParaMemoria;
		dataParaMemoria.cantPags=cant_paginas;
		dataParaMemoria.pid=programaAnsisop->pid_provisorio;

		//***Le Enviamos a memoria el pid con el que vamos a trabajar - Junto a la accion que vamos a realizar - Le envio a memeoria la cantidad de paginas que necesitaré reservar
		enviarMensaje(socketMemoria,inicializarPrograma, &dataParaMemoria, sizeof(int)*2); // Enviamos el pid a memoria

		int* ok;
		recibirMensaje(socketMemoria, &ok); // Esperamos a que memoria me indique si puede guardar o no el stream
		if(*ok)
		{
			printf("\n\n[Funcion consola_recibirScript] - Memoria dio el Ok para el proceso recien enviado: Ok-%d\n", *ok);

			char* copiaScriptAnsisop = strdup(programaAnsisop->scriptAnsisop);
			int pid= programaAnsisop->pid_provisorio;
			int copiasocketConsola = programaAnsisop->socketConsola;

			//***Quito el script de la cola de new
			queue_pop(cola_New);
		//	liberar_Programa_En_New(programaAnsisop); --- Arreglar esto

			sem_post(&mutex_cola_New);


			//*** Divido el script en la cantidad de paginas necesarias
			char** scriptEnPaginas = memoria_dividirScriptEnPaginas(cant_paginas, copiaScriptAnsisop);

			//***Le envio a memoria todo el scrip pagina a pagina
			int i=0;

			for(i; i<cant_paginas && ok; i++)
			{
				enviarMensaje(socketMemoria,envioCantidadPaginas,scriptEnPaginas[i],strlen(scriptEnPaginas[i]));//// Cambiar strlen rellenar con /0
				printf("Envio una pagina: %d\n", i);
	//enviarMensaje(socketMemoria,envioCantidadPaginas,"FASDFSDF",strlen("FASDFSDF"));//// Cambiar strlen rellenar con /0

				recibirMensaje(socketMemoria,&ok);
			}

			//***Creo el PCB
			PCB_DATA * pcbNuevo = crearPCB(copiaScriptAnsisop, cant_paginas, pid);
			free(copiaScriptAnsisop);

			//***Añado el pcb a la cola de Ready
			queue_push(cola_Ready,pcbNuevo);
		}
		else
		{
			PROGRAMAS_EN_NEW * programaNuevoABorrar = queue_peek(cola_New);
			queue_pop(cola_New);

			printf("Estructura a borrar:--\nPid: %d\nScript: %s\nSocketConsola:%d\n\n",programaNuevoABorrar->pid_provisorio,programaNuevoABorrar->scriptAnsisop,programaNuevoABorrar->socketConsola);

		//	consola_finalizacionPorNoMemoria(programaNuevoABorrar->socketConsola, programaNuevoABorrar->pid_provisorio, -1); /// arreglar esto

			liberar_Programa_En_New(programaNuevoABorrar);

			sem_post(&mutex_cola_New);

			printf("[Funcion consola_recibirScript] - No hubo espacio para guardar en memoria!\n");
		}
	}
	else
	{
	}
}


///---FIN FUNCIONES DEL KERNEL----//



//// El recibir mensaje recibe (int socketk, void stream) y devuelve el tipo de mensaje



///---RUTINAS DE HILOS----///

/// *** A esta función hay que probarle tuodo el sistema de envio de mensajes entre consola y kernel
//***Esta rutina se levanta por cada consola que se cree. Donde se va a quedar escuchandola hasta que la misma se desconecte.
void *rutinaConsola(void * arg)
{

	int socketConsola = (int)arg;
	bool todaviaHayTrabajo = true;
	void * stream;

	printf("[Rutina rutinaConsola] - Entramos al hilo de la consola: %d!\n", socketConsola);

	while(todaviaHayTrabajo){
		switch(recibirMensaje(socketConsola,&stream)){
			case envioScriptAnsisop:{

				//***Como para el caso de recibir un script
				char* scripAnsisop = (char *)stream;
				int sizeCodigoAnsisop = strlen(scripAnsisop);

				PROGRAMAS_EN_NEW * nuevoPrograma;

				sem_wait(&mutex_HistoricoPcb);
					nuevoPrograma->pid_provisorio= historico_pid;
					historico_pid++;
				sem_post(&mutex_HistoricoPcb);

				nuevoPrograma->scriptAnsisop = scripAnsisop;
				nuevoPrograma->socketConsola = socketConsola;
				nuevoPrograma->finalizado = false;

				//***Lo Agrego a la Cola de New - Aca hay que poner un semaforo
				sem_wait(&mutex_cola_New);
					queue_push(cola_New,nuevoPrograma);
				sem_post(&mutex_cola_New);

				//***Añado al proceso a la cola de avisos -- FAlta un semaforo par aviso de finalizacion
		/*		AVISO_FINALIZACION * aviso = malloc(sizeof(AVISO_FINALIZACION));

				aviso->pid = nuevoPrograma->pid_provisorio;
				aviso->socketConsola = socketConsola;
				aviso->finalizado = false;

				list_add(avisos,aviso); // esto no va aca
		*/
				/* Cuando un consola envia un pid para finalizar, lo que vamos a hacer es una funci+on que cambie el estado de ese proceso a finalizado,
				 * de modo que en el mommento en que un proceso pase de cola en cola se valide como esta su estado, de estar en finalizado se pasa automaticamente ala cola
				 * de finalizados --- Asi que se elimina la estructura de avisos de finalizacion y se agrega el elemento estado a todos las estructuras de als colas
				 */

			}break;

			case finalizarCiertoScript:{
				int pid = (int)stream;
				int respuesta;

				//***Le digo a memoria que mate a este programa
				enviarMensaje(socketMemoria,envioDelPidEnSeco, &pid,sizeof(int));

				//***Memoria me avisa que salio todobon
				recibirMensaje(socketConsola, &respuesta);
				errorEn(respuesta, "[Funcion consola_FinalizacionPorDirectiva] - La Memoria no pudo finalizar el proceso");

				consola_finalizacionPorDirectiva(socketConsola, pid, -7);
			}break;
			case desconectarConsola:{

				todaviaHayTrabajo = false;
				consola_finalizarTodosLosProcesos(socketConsola);

				// matar todos los procesos en memoria tambien
			}break;

			default:{
				perror("Se recibio una accion que no esta contemplada, se cerrara el socket\n");
				close(socketConsola);
			}
		}
	}
	close(socketConsola);
}

typedef struct{
	int socketCPU;
	bool ocupada;
};


/// *** Esta rutina se comenzará a hacer cuando podramos comenzar a enviar mensajes entre procesos
void *rutinaCPU(void * arg)
{
	int socketCPU = (int)arg;
}





/// *** Esta Función esta probada y anda
//***Esta rutina solo revisa una lista de procesos y si algun terminó, se lo avisa a su consola correspondiente
void * revisarFinalizados(){
	while(1){

		sleep(10);
		printf("[Rutina revisarFinalizados] - Entro al while 1 de la rutina de finalizados\n");

		int pos = 0;

		bool busqueda(AVISO_FINALIZACION * aviso)
		{
			if(aviso->finalizado){
				return true;
			}
			pos++;
			return false;
		}

		sem_wait(&mutex_ListaDeAvisos);
		//**Si algun elemento de la lista se encuentra en estado finalizado, se avisa a la consola y se elimina el proceso de la lista
		if(list_any_satisfy(avisos, busqueda)){
			AVISO_FINALIZACION* aux =(AVISO_FINALIZACION*)list_find(avisos, busqueda);
			//**Le avisamos a la consola apropiada a traves del socket que esta en la estructura de avisos, que cierto pid esta en estado finalizado
			consola_enviarAvisoDeFinalizacion(aux->socketConsola, aux->pid);

			//**Eliminamos el nodo de la lista
			list_remove_and_destroy_element(avisos,pos,free);//***No se si esto tiene memory leaks-- revisar
		}
		sem_post(&mutex_ListaDeAvisos);
	}
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
		id_clienteConectado = aceptarConexiones(listener, &nuevoSocket, Kernel, &aceptados);

		pthread_t hilo_M;

		switch(id_clienteConectado)
		{
			case Consola: // Si es un cliente conectado es una CPU
			{
				printf("\nNueva Consola Conectada!\nSocket Consola %d\n\n", nuevoSocket);
				//pthread_create(&hilo_M, NULL, rutinaConsola, nuevoSocket);

				rutinaConsola(nuevoSocket);
			}break;

			case CPU: // Si el cliente conectado es el cpu
			{
				printf("Nueva CPU Conectada\nSocket CPU %d\n\n", nuevoSocket);
				//pthread_create(&hilo_M, NULL, rutinaCPU, nuevoSocket);
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

//*** Rutina que
void * planificadorLargoPlazo()
{
	while(1)
	{
		printf("Entramos al planificador de largo plazo!\n");

		if(cantidadProgramasEnProcesamiento() < getConfigInt("GRADO_MULTIPROG") && hayProgramasEnNew())
		{
			newToReady();
		}
		printf("No nos da el grado de multi programación o hay programas en new!\n");
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
	int rta_conexion;
	socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria
	if (socketMemoria == 1){
		perror("Falla en el protocolo de comunicación");
	}
	if (socketMemoria == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
	}
	if ( (rta_conexion = handshakeCliente(socketMemoria, Kernel)) != Memoria) {
				perror("Error en el handshake con Memoria");
				close(socketMemoria);
	}
	printf("Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);

	int* sizePag;
	recibirMensaje(socketMemoria, &sizePag);

	size_pagina = *sizePag;

	printf("[Conexion con Memoria] - El Size pag es: %d", size_pagina);

	free(sizePag);
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

	//---ABRO EL HILO DE PROCESOS FINALIZADOS---//
//	pthread_t hilo_revisarFinalizados;
//	pthread_create(&hilo_revisarFinalizados, NULL, revisarFinalizados, NULL);

	//---ABRO EL HILO DEL PLANIFICADOR A LARGO PLAZO---//
//	pthread_t hilo_planificadorLargoPlazo;
//	pthread_create(&hilo_planificadorLargoPlazo, NULL, planificadorLargoPlazo, NULL);

	//----ME PONGO A ESCUCHAR CONEXIONES---//
	pthread_t hilo_aceptarConexiones_Cpu_o_Consola;
	pthread_create(&hilo_aceptarConexiones_Cpu_o_Consola, NULL, aceptarConexiones_Cpu_o_Consola, listener);


	pthread_join(hilo_aceptarConexiones_Cpu_o_Consola, NULL);
//	pthread_join(hilo_planificadorLargoPlazo, NULL);
//	pthread_join(hilo_revisarFinalizados, NULL);

	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	///-----LIBERO LA CONFIGURACION
	liberarConfiguracion();

	return 0;
}
