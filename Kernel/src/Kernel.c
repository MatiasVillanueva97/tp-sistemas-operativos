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
#include "commons/collections/list.h"
#include "commons/collections/queue.h"
#include "commons/collections/dictionary.h"
#include <semaphore.h>

#include <parser/parser.h>
#include <parser/metadata_program.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#include "datosGlobales.h"
#include "funcionesPCB.h"

enum tiposMensajes{
	enviarPid = 1,
	enviarCantidadPaginas = 2,
	enviarPaginaMemoria = 3
};

///----INICIO SEMAFOROS----///
pthread_mutex_t mutex_HistoricoPcb; // deberia ser historico pid
pthread_mutex_t mutex_ListaDeAvisos;
pthread_mutex_t mutex_cola_New;

sem_t* contadorDeCpus = 0;

void inicializarSemaforo(){
	sem_init(&mutex_HistoricoPcb,0,1);
	sem_init(&mutex_ListaDeAvisos,0,1);
	sem_init(&mutex_cola_New,0,1);
}
///----FIN SEMAFOROS----///



///------FUNCIONES Y PROTOCOLOS COMUNES----//
void errorEn(int valor,char * donde){
	if(valor == -1)
		printf("%s\n", donde);
}

int socketKernel;

void teDigoQueHayQueHacer(int queHaces){
	enviarMensaje(socketKernel,1,queHaces,sizeof(int));
}

int queHago(int socket){
	int recibido;
	recibirMensaje(socket,&recibido);
	return recibido;
}
//---FIN FUNCIONES COMUNES---//

t_list * avisosDeConsola(int socketConsola){
	bool buscarPorSocket(AVISO_FINALIZACION * aviso){
		return (aviso->socketConsola == socketConsola);
	}
	return list_filter(avisos, buscarPorSocket);
}

///-----FUNCIONES CONSOLA-------//

void consola_enviarAvisoDeFinalizacion(int socketConsola, int pid){
	char * mensaje = string_from_format("El Proceso con pid %d, ha finalizado",pid);
	enviarMensaje(socketConsola,2,mensaje,strlen(mensaje)+1);
	free(mensaje);
}

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

//***Funciones de Avisos a Consola


int size_pagina=256;

void liberar_Programa_En_New(PROGRAMAS_EN_NEW * programaNuevo)
{
	//free(programaNuevo->scriptAnsisop); El que no este esta cosa puede traer memory leaks
	free(programaNuevo);
}
unsigned int redondearDivison(unsigned int dividend, unsigned int divisor)
{
    return (dividend + (divisor / 2)) / divisor; /// revisar laburo para santi
}

char ** memoria_dividirScriptEnPaginas(int cant_paginas, char *copiaScriptAnsisop)
{
	char ** scriptParticionado= malloc(strlen(copiaScriptAnsisop)+(sizeof(char*)*cant_paginas));

	//while(){}

	return 15;
}

int memoria_CalcularCantidadPaginas(char * scriptAnsisop)
{
	return 11;// redondearDivision((strlen(scriptAnsisop)+1), 256) + getConfigInt("STACK_SIZE");
}


//***Funciones de Planificador
void newToReady(){

	printf("\n\n\nEstamos en el planificador a largo plazo!\n\n");

	//***Tomo el primer elemento de la cola sin sacarlo de ella
	sem_wait(&mutex_cola_New);
	PROGRAMAS_EN_NEW* programaAnsisop = queue_peek(cola_New);

	printf("Estructura:--\nPid: %d\nScript: %s\nSocketConsola:%d\n\n",programaAnsisop->pid_provisorio,programaAnsisop->scriptAnsisop,programaAnsisop->socketConsola);


	//***Le Enviamos a memoria el pid con el que vamos a trabajar - Junto a la accion que vamos a realizar
//	enviarMensaje(socketMemoria,enviarPid, &programaAnsisop->pid_provisorio, sizeof(int)); // Enviamos el pid a memoria
	printf("Pid Enviado a memoria, pid: %d\n", programaAnsisop->pid_provisorio);

	//***Calculo cuantas paginas necesitara la memoria para este script
	int cant_paginas = memoria_CalcularCantidadPaginas(programaAnsisop->scriptAnsisop); // hacer esta funcion-------------------------------------------------FALTA

	//***Le envio a memeoria la cantidad de paginas que necesitaré reservar
//	enviarMensaje(socketMemoria,enviarCantidadPaginas,&cant_paginas,sizeof(int));
	printf("Cantidad De Paginas Enviadas a memoria, cantidad de paginas: %d\n", cant_paginas);

	int ok=0;
	//recibirMensaje(socketMemoria, &ok); // Esperamos a que memoria me indique si puede guardar o no el stream
	//printf("Recibimos el Ok de memoria\n");
	//ok=1;

	if(ok)
	{
		char* copiaScriptAnsisop = strdup(programaAnsisop->scriptAnsisop);
		int pid= programaAnsisop->pid_provisorio;
		int copiasocketConsola = programaAnsisop->socketConsola;

		//***Quito el script de la cola de new
		queue_pop(cola_New);
		liberar_Programa_En_New(programaAnsisop);
		sem_post(&mutex_cola_New);

		printf("\n\n[Funcion consola_recibirScript] - Memoria dio el Ok para el proceso recien enviado\n");

		//*** Divido el script en la cantidad de paginas necesarias
		//char** scriptEnPaginas = memoria_dividirScriptEnPaginas(cant_paginas, copiaScriptAnsisop); // hacer esta otra funcion-------------------------------------------FALTA

		//***Le envio a memoria todo el scrip pagina a pagina
		int i=0;
		for(i; i<cant_paginas; i++)
		{
			//enviarMensaje(socketMemoria,enviarPaginaMemoria,scriptEnPaginas[i],sizeof(int));
			printf("Envio una pagina: %d\n", i);
		}

		//***Creo el PCB
		PCB_DATA * pcbNuevo = crearPCB(copiaScriptAnsisop, cant_paginas, pid);
		free(copiaScriptAnsisop);

		//***Añado el pcb a la cola de Ready
		queue_push(cola_Ready,pcbNuevo);

		//***Añado al proceso a la cola de avisos
		AVISO_FINALIZACION * aviso = malloc(sizeof(AVISO_FINALIZACION));

		aviso->pid = pcbNuevo->pid;
		aviso->socketConsola = copiasocketConsola;
		aviso->finalizado = false;

		list_add(avisos,aviso);

		printf("Se agrego en la lista de avisos: pid- %d , socketConsola-%d \n", aviso->pid,aviso->socketConsola);
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


///---FIN FUNCIONES DEL KERNEL----//

//// El recibir mensaje recibe (int socketk, void stream) y devuelve el tipo de mensaje

///---RUTINAS DE HILOS----///
void *rutinaConsola(void * arg)
{

	int socketConsola = (int)arg;
	bool todaviaHayTrabajo = true;
	void * stream;

	printf("[Rutina rutinaConsola] - Entramos al hilo de la consola: %d!\n", socketConsola);

	while(todaviaHayTrabajo){
		switch(recibirMensaje(socketConsola,&stream)){
			case recibirScript:{

				//***Como para el caso de recibir un script
				char* scripAnsisop = (char *)stream;
				int sizeCodigoAnsisop = strlen(scripAnsisop);

				/* Ni bien llega un programa, ya tengo que ponerlo una estructura que tiene a los programitas dando vuelta,
				 * fijate que ya necesitariamos un pid, porque ya en cualquier momento este proceso se puede finalizar y para eso
				 * tengo qeu podes avisarle a una consola que su proceso finalizó, justamente para esto tenemos esta estructura. aviso qe esta re peruano esto
				 */

				PROGRAMAS_EN_NEW * nuevoPrograma;

				sem_wait(&mutex_HistoricoPcb);
					nuevoPrograma->pid_provisorio= historico_pid;
					historico_pid++;
				sem_post(&mutex_HistoricoPcb);

				nuevoPrograma->scriptAnsisop = scripAnsisop;
				nuevoPrograma->socketConsola = socketConsola;

				//***Lo Agrego a la Cola de New - Aca hay que poner un semaforo
				sem_wait(&mutex_cola_New);
					queue_push(cola_New,nuevoPrograma);
				sem_post(&mutex_cola_New);
			}break;

			case finalizarScript:{
				int pid;
				int respuesta = (int)stream;

				//***Le digo a memoria que mate a este programa
				enviarMensaje(socketMemoria,1, &pid,sizeof(int));

				//***Memoria me avisa que salio todobon
				respuesta = recibirMensaje(socketConsola, &pid);
				errorEn(respuesta, "[Funcion consola_FinalizacionPorDirectiva] - La Memoria no pudo finalizar el proceso"); // y algo mas deveriamos hacer

				consola_finalizacionPorDirectiva(socketConsola, pid, -7);
			}break;
			case desconectar:{

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

void *rutinaCPU(void * arg)
{
	int socketCPU = (int)arg;
}


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

void *aceptarConexiones( void *arg ){
	//----DECLARACION DE VARIABLES ------//
	int listener = (int)arg;
	int nuevoSocket;
	int aceptados[] = {CPU, Consola};
	struct sockaddr_storage their_addr;
	char ip[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;
	int id_clienteConectado;
	///-----FIN DECLARACION----///

	while(1)
	{
		socklen_t sin_size = sizeof their_addr;//No preguntes porque, pero sin esto no anda nada

		//***Acepto una conexion
		nuevoSocket = accept(listener, (struct sockaddr *) &their_addr, &sin_size);
		errorEn(nuevoSocket,"ACCEPT");


		//***Toda esta negrada para imprimir una ip
		inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip);
		printf("Conexion con %s\n", ip);

		//***Hago el handShake con la nueva conexion
		id_clienteConectado = handshakeServidor(nuevoSocket, Kernel, aceptados);

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



void conectarConMemoria()
{
	int rta_conexion;
	socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria
	if (socketMemoria == 1){
			perror("Falla en el protocolo de comunicación");
			exit(1);
	}
	if (socketMemoria == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketMemoria, Kernel)) == -1) {
				perror("Error en el handshake con Memoria");
				close(socketMemoria);
	}
	printf("Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);
}
void conectarConFS()
{

	int rta_conexion;
	socketFS = conexionConServidor(getConfigString("PUERTO_FS"),getConfigString("IP_FS")); // Asignación del socket que se conectara con el filesytem
	if (socketFS == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketFS == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketFS, Kernel)) == -1) {
		perror("Error en el handshake con FileSystem");
		close(socketFS);
	}
	printf("Conexión exitosa con el FileSystem(%i)!!\n",rta_conexion);
}


int main(void) {
	printf("Inicializando Kernel.....\n\n");

	///------INICIALIZO TODO-------------///
		historico_pid=0;

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
	pthread_t hilo_revisarFinalizados;
	pthread_create(&hilo_revisarFinalizados, NULL, revisarFinalizados, NULL);

	//----ME PONGO A ESCUCHAR CONEXIONES---//
	pthread_t hilo_aceptarConexiones;
	pthread_create(&hilo_aceptarConexiones, NULL, aceptarConexiones, listener);
	pthread_join(hilo_aceptarConexiones, NULL);

	pthread_join(hilo_revisarFinalizados, NULL);


	while(1)
	{
		printf("Hola hago tiempo!\n");
		sleep(5);
	}

	///-----LIBERO LA CONFIGURACION
	liberarConfiguracion();

	return 0;
}
