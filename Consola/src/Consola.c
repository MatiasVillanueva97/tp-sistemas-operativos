/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "commons/config.h"
#include "commons/string.h"
#include "commons/temporal.h"
#include "commons/process.h"
#include "commons/collections/list.h"
#include "commons/log.h"
#include <semaphore.h>
#include <pthread.h>


#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"
#include "../../Nuestras/src/laGranBiblioteca/funcionesParaTodosYTodas.c"

#include <arpa/inet.h>

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

void* rutinaPrograma(void*);
char* diferencia(char*,char*);
void transformarFechaAInts(char*, int[4]);
void* rutinaEscucharKernel();
void switchManejoError(int exitCode);



typedef struct {
	int pid;
	bool estaTerminado;
	bool hayParaImprimir;
	char * mensajeAImprimir;
}t_Estado;


struct{
	int pid;
	char* mensaje;
} mensajeDeProceso;

int socketKernel;
char* mensajeActual;
t_list* listaEstadoPrograma;
bool error;
bool signal_terminar;
t_log* logConsola;


pthread_mutex_t mutex_lista;
pthread_mutex_t mutex_ordenDeEscritura;
pthread_mutex_t mutex_mensajeActual;
pthread_mutex_t mutex_espera;
pthread_mutex_t hayQueEscribir;
pthread_mutex_t yaSeEscribio;


size_t tamanoArchivo(FILE * archivo){
	size_t tamano;
	fseek(archivo,0,SEEK_END);//ACA FALTA HANDELEAR ERROR
	tamano = ftell(archivo);
	fseek(archivo,0,SEEK_SET);
return tamano;
}

void string_iterate(char* strings, void (*closure)(char)) {
	int i = 0;
	while (strings[i] != '\0') {
		closure(strings[i]);
		i++;
	}
}

char* procesarArchivo(char* archivo, int tamano){
	//char ** lineas = string_split(archivo, "\n");
	char* archivoProcesado = string_new();
	bool laAnteriorFueUnEnter = false;
	void agregar(char caracter){
			if(caracter != '\t'){
			char* aux = malloc(sizeof(char)*5);
			aux[0] = caracter;
			aux[1] = '\0';
			string_append(&archivoProcesado, aux);
			free(aux);
		}
	}
	void dejarSoloUnEnter(char caracter){
		if(caracter == '\n'){
			if(!laAnteriorFueUnEnter){
				agregar(caracter);
			}
			laAnteriorFueUnEnter = true;
		}
		else{
			agregar(caracter);
			if(caracter != '\t')
			laAnteriorFueUnEnter = false;
		}

	}
	string_iterate(archivo, dejarSoloUnEnter);
	return archivoProcesado;
}

char * generarScript(char * nombreDeArchivo){
	FILE* archivo;

    if ( !(archivo = fopen(nombreDeArchivo, "r")) )
    {
    	perror("Error: el archivo no esta en el directorio o no existe \n");
    	log_error(logConsola,"Error: el archivo %s no esta en el directorio o no existe ",nombreDeArchivo);
    	return NULL;// Va si o si , en caso de que exista el error sino tira segmentation fault
    }
	size_t tamano = tamanoArchivo(archivo);
	char* script = malloc(tamano+1);//lee el archivo y lo guarda en el script AnsiSOP
   fread(script,tamano,1,archivo);
	script[tamano] = '\0';
	char* scriptProcesado = procesarArchivo(script, tamano);
	free(script);
	fclose(archivo);
	log_info(logConsola,"Se creo el script correctamente este dice: %s",scriptProcesado);
	return scriptProcesado;
}
void transformarFechaAInts(char * fecha, int arrayFecha[4]){
	char** arrayCalendario = string_split(fecha,":");
	int i;
	for (i=0;i<4;i++){
		arrayFecha[i] = atoi(arrayCalendario[i]);
	}
	liberarArray(arrayCalendario);
}
char* transformarArrayAFecha(int arrayInt[4]){
		int i;
		char* fecha[4];
		char * aux;
		aux = string_itoa(arrayInt[0]);
		for(i=1;i<4;i++){
			fecha[i] = string_itoa(arrayInt[i]);
			string_append_with_format(&aux,":%s",fecha[i]);
			free(fecha[i]);
		}
		//string_append_with_format(&aux,":%s",fecha[1]);
		//string_append_with_format(&aux,":%s",fecha[2]);
		//string_append_with_format(&aux,":%s",fecha[3]);
		//aux = strcat(strcat(strcat(fecha[0], fecha[1]),fecha[2]), fecha[3]);

		return aux;
	}
char* diferencia(char* fechaInicio,char* fechaFin){
	int resultadoInt[4];
	int arrayInicio[4];
	transformarFechaAInts(fechaInicio, arrayInicio);
	int arrayFin[4];
	transformarFechaAInts(fechaFin,arrayFin);
	int i;
	for(i=0;i<4;i++){
		resultadoInt[i] = arrayFin[i]-arrayInicio[i];
		if(resultadoInt[i] < 0){
			resultadoInt[i-1]--;
			if(i != 3) {
				resultadoInt[i] = 60 + resultadoInt[i];
			}else {
				resultadoInt[i] = 1000 + resultadoInt[i];
			};
		}
	}
	char* diferencia = transformarArrayAFecha(resultadoInt);

	return diferencia;
}


void conectarConKernel(){
	int rta_conexion;
	socketKernel = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL"));

		// validacion de un correcto hadnshake
		if (socketKernel== 1){
			log_error(logConsola,"Falla en el protocolo de comunicación,en conectarConKernel");
			perror("Falla en el protocolo de comunicación");
			exit(1);
		}
		if (socketKernel == 2){
			log_error(logConsola,"No se ha conecto con Kernel , en conectarConKernel");
			perror("No se conecto con Kernel");
			exit(1);
		}
		if ( (rta_conexion = handshakeCliente(socketKernel, Consola)) == -1) {
			log_error(logConsola,"Error en el handshake con el Servidor, en conectarConKernel");
			perror("Error en el handshake con el Servidor");
			close(socketKernel);
		}
		log_info(logConsola,"Se conecto la Consola y el Kernel");
		printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);
}

void agregarAListaEstadoPrograma(int pid, bool estado){
	t_Estado*	aux = malloc(sizeof(t_Estado));
	aux->pid = pid;
	aux->estaTerminado = estado;
	aux->hayParaImprimir = false;
	aux->mensajeAImprimir = NULL;
	sem_wait(&mutex_lista);
	list_add(listaEstadoPrograma, aux);
	sem_post(&mutex_lista);
}
t_Estado* encontrarElDeIgualPid(int pid){
		t_Estado * programaEstado;
		bool sonIguales(t_Estado * elementos){
			return  elementos->pid == pid;
		}
		sem_wait(&mutex_lista);
		programaEstado = list_find(listaEstadoPrograma, (void*) sonIguales);
		sem_post(&mutex_lista);
		return programaEstado;
	}
void matarHiloPrograma(int pid){
	bool sonIguales(t_Estado * elementos){
		return  elementos->pid == pid;
	}
	t_Estado * ret;
	ret = encontrarElDeIgualPid(pid);
	if(ret == NULL){
		perror("Error: el pid no existe o ya ha finalizado el programa");
		log_error(logConsola,"Error: el pid no existe o ya ha finalizado el programa");
	}
	else{
		sem_wait(&mutex_lista);
		ret->estaTerminado=true;
		sem_post(&mutex_lista);
		sem_post(&hayQueEscribir);
		sem_wait(&yaSeEscribio);
		log_info(logConsola,"Se cambio el estado del pid %d a terminado",pid);
	}
}

void crearHiloDetachPrograma(int* pid){
	pthread_attr_t attr;
	pthread_t hilo ;
	//Hilos detachables cpn manejo de errores tienen que ser logs
	int  res;
	res = pthread_attr_init(&attr);
	if (res != 0) {
		log_error(logConsola,"Error en los atributos del hilo, en crearHiloDetachable");
		perror("Error en los atributos del hilo");

	}
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	 if (res != 0) {
		 log_error(logConsola,"Error en el seteado del estado de detached, en crearHiloDetachable");
		 perror("Error en el seteado del estado de detached");

	 }

	res = pthread_create (&hilo ,&attr,rutinaPrograma, (void *) pid);
	if (res != 0) {
		log_error(logConsola,"Error en la creacion del hilo, en crearHiloDetachable");
		perror("Error en la creacion del hilo");
	}
	log_info(logConsola,"Se creo un hilo Detachable para el proceso %d",*pid);
	pthread_attr_destroy(&attr);
}
void inicializarSemaforos(){
	log_info(logConsola,"Se inicializan los semaforos");
	sem_init(&mutex_lista,0,1);
	sem_init(&mutex_ordenDeEscritura,0,0);
	sem_init(&mutex_mensajeActual,0,1);
	sem_init(&mutex_espera,0,0);
	sem_init(&hayQueEscribir,0,0);
	sem_init(&yaSeEscribio,0,0);
}
void liberarNodoListaEstado(t_Estado* nodo){
	free(nodo->mensajeAImprimir);
	free(nodo);
}
void sigint_handler(int signal) {
	printf("Se recibio una SIGINT, se finalizará el proceso\n" );
	log_warning(logConsola,"Se recibio una SIGINT, se finalizará el proceso");
	enviarMensaje(socketKernel,desconectarConsola, &socketKernel, 4);

	list_destroy_and_destroy_elements(listaEstadoPrograma,liberarNodoListaEstado);
	close(socketKernel);
	exit(-1);
	return;
}


int main(void)
{
	logConsola = log_create("Consola.log","Consola",0,0);
	printf("Inicializando Consola.....\n\n");

	//char* gensc = generarScript("for");
	//puts(gensc);

	size_t len = 0;
	char* mensaje = NULL;
	listaEstadoPrograma = list_create();
	error = false;
	signal_terminar = false;
	inicializarSemaforos();
	signal(SIGINT, sigint_handler);

	// ******* Configuración inicial Consola

	printf("Configuracion Inicial:\n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Consola/consola.config");
	imprimirConfiguracion();

	conectarConKernel();


	pthread_t hiloMaster ;
	pthread_create(hiloMaster, NULL,rutinaEscucharKernel, NULL);
	while(!error && !signal_terminar){//Ciclo donde se ejecutan los comandos principales.

		printf("\nIngrese Comando: \n");

		getline(&mensaje,&len,stdin);
		log_info(logConsola,"Se escribió un comando en la consola");

		char** comandoConsola = NULL;//Esta variable es para cortar el mensaje en 2.
		comandoConsola = string_split(mensaje, " "); // separa la entrada en un char**

		if(strcmp(comandoConsola[0],"desconectarConsola\n") == 0){
			liberarArray(comandoConsola);
			enviarMensaje(socketKernel,desconectarConsola, &socketKernel, 4);
			log_info(logConsola,"Se desconecto a Consola. Se le envio un mensaje al Kernel");
			break;
		}
		if(strcmp(comandoConsola[0],"limpiarMensajes\n") == 0){
			system("clear");
			log_info(logConsola,"Se limpiaron los mensajes de la consola");
			liberarArray(comandoConsola);
			continue;
		}
		if(strcmp(comandoConsola[0],"iniciarPrograma") == 0){//Primer Comando iniciarPrograma
			char** nombreDeArchivo= string_split(comandoConsola[1], "\n");//Toma el parametro que contiene el archivo y le quita el \n
			char* script = generarScript(nombreDeArchivo[0]);
			if (script==NULL){
				continue;}
			enviarMensaje(socketKernel,envioScriptAnsisop, script,strlen(script)+1);
			log_info(logConsola,"Se le envio el script al Kernel que dice : %s", script);
			liberarArray(nombreDeArchivo);
			liberarArray(comandoConsola);
			free(script);
			sem_wait(&mutex_ordenDeEscritura);
			continue;
		}
		if(strcmp(comandoConsola[0],"finalizarPrograma") == 0){
			char** stream= string_split(comandoConsola[1],"\n");
			int *pid = malloc(sizeof(int));
			*pid = atoi(*stream);
			if(encontrarElDeIgualPid(*pid)==NULL){
			 	printf(" el pid es incorrecto \n");
			 }
			 else{
			 	enviarMensaje(socketKernel,finalizarCiertoScript ,pid, sizeof(int));
			 	log_info(logConsola,"Se le envia el pid %d para que Kernel lo finalice",pid);
			}
			liberarArray(comandoConsola);
			liberarArray(stream);
			continue;
		}
		if (error){
			liberarArray(comandoConsola);
			break;
		}

	puts("Comando Inválido!\n");
	}


	sem_wait(&mutex_lista);
	list_destroy_and_destroy_elements(listaEstadoPrograma,liberarNodoListaEstado);
	sem_post(&mutex_lista);
	close(socketKernel);
	free(mensaje);
	liberarConfiguracion();
	log_info(logConsola,"Se liberan los recursos");
	log_destroy(logConsola);
	return 0;
}





void* rutinaPrograma(void* parametro){
	int pid ;
	char* tiempoInicio = temporal_get_string_time();
	int cantImpresiones = 0;
	pid = *((int*)parametro);

	t_Estado * programaEstado ;
	agregarAListaEstadoPrograma(pid,false);
	programaEstado = encontrarElDeIgualPid(pid);

		printf("Numero de pid del proces: %d \n",pid);
		log_info(logConsola,"Se inicializo rutinaPrograma %d en el tiempo %s",pid,tiempoInicio);
		sem_post(&mutex_ordenDeEscritura);
		sem_post(&mutex_espera);

		while(1){
			sem_wait(&hayQueEscribir);
			sem_wait(&mutex_lista);

		if(programaEstado->hayParaImprimir){
			cantImpresiones++;
			printf("Mensaje del pid %d: %s\n", pid, programaEstado->mensajeAImprimir);
			log_info(logConsola,"Se imprimio el mensaje del pid %d: %s\n", pid, programaEstado->mensajeAImprimir);
			programaEstado->hayParaImprimir = false;
			sem_post(&yaSeEscribio);

		}else if(programaEstado->estaTerminado){
				sem_post(&yaSeEscribio);
				sem_post(&mutex_lista);
				break;
		}else{
			sem_post(&hayQueEscribir);
		}
			sem_post(&mutex_lista);
		}

	char* tiempoFin = temporal_get_string_time();
	char* diferencia1 = diferencia(tiempoInicio,tiempoFin);
	log_info(logConsola,"Finalizo el pid: %d", pid);
	printf("\nAcaba de finalizar el pid: %d\n", pid);
	log_info(logConsola,"Tiempo de inicio: %s\n",tiempoInicio);
	printf("Tiempo de inicio: %s\n",tiempoInicio);
	log_info(logConsola,"Tiempo de Finalización: %s\n",tiempoFin);
	printf("Tiempo de Finalización: %s\n",tiempoFin);
	log_info(logConsola,"Tiempo de Ejecución: %s\n",diferencia1);
	printf("Tiempo de Ejecución: %s\n",diferencia1);
	log_info(logConsola,"Cantidad de impresiones del programa ansisop: %d\n",cantImpresiones);
	printf("Cantidad de impresiones del programa ansisop: %d\n",cantImpresiones);
	free(diferencia1);
	free(tiempoFin);
	free(tiempoInicio);
	log_info(logConsola,"Se liberaron los recursos del hilo detachable asociado al pid %d",pid);

	bool sonIguales(t_Estado * elementos){
				return  elementos->pid == pid;
			}
	sem_wait(&mutex_lista);
	list_remove_and_destroy_by_condition(listaEstadoPrograma,sonIguales,liberarNodoListaEstado);
	sem_post(&mutex_lista);
}




void* rutinaEscucharKernel() {

	while (!error) {
		int operacion;
		void * stream;
		operacion = recibirMensaje(socketKernel, &stream);
		switch (operacion) {
		case (envioDelPidEnSeco): {
			int pid;
			pid = *((int*) stream);
			crearHiloDetachPrograma(&pid);
			sem_wait(&mutex_espera);
			break;
		}
		case (imprimirPorPantalla): {
			t_mensajeDeProceso aux = deserializarMensajeAEscribir(stream);
			t_Estado* programaEstado;
			programaEstado = encontrarElDeIgualPid(aux.pid);
			sem_wait(&mutex_lista);
			programaEstado->mensajeAImprimir = string_duplicate(aux.mensaje);
			programaEstado->hayParaImprimir = true;
			sem_post(&mutex_lista);
			sem_post(&hayQueEscribir);
			sem_wait(&yaSeEscribio);
			free(aux.mensaje);

			break;
		}
		case (pidFinalizado): {
			int pid;
			pid = (*(int*) stream);
			matarHiloPrograma(pid);
			log_info(logConsola,"Se finalizo el pid %d correctamente",pid);
			printf("Se ha finalizado correctamente el pid: %d \n", pid);
			break;
		}

		case (errorFinalizacionPid): {
			int pid;
			int exitCode;
			pid = (*(int*) stream);
			exitCode = (*(int*) (stream+4));
			matarHiloPrograma(pid);
			log_warning(logConsola,"No se ha finalizado correctamente el pid: %d \n", pid,exitCode);
			printf("No se ha finalizado correctamente el pid: %d \n", pid);
			switchManejoError(exitCode);
			break;
		}

		case (0):{
			log_error(logConsola,"Se desconecto el kernel");
			printf("Se desconecto el kernel\n");
			sem_wait(&mutex_lista);
			list_destroy_and_destroy_elements(listaEstadoPrograma,liberarNodoListaEstado);
			sem_post(&mutex_lista);
			error = true;
			exit(-1);
			break;
		}

		case (pidFinalizadoPorFaltaDeMemoria): {
			int pid;
			pid = (*(int*) stream);

			matarHiloPrograma(pid);
			log_error(logConsola,"No se ha finalizado correctamente el pid %d por falta de memoria",pid);
			printf("\nNo se ha finalizado correctamente el pid %d por falta de memoria\n",pid);
			break;

		}

		default: {
			perror("Error: no se pudo obtener mensaje \n");
		}
	}

		if (operacion != 0)
			free(stream);

	}
}

void switchManejoError(int exitCode){
	switch(exitCode){
		case noSePudoReservarRecursos :{
			printf("Se finalizó porque no se pudo reservar recursos.\n");
			}break;
		case archivoInexistente :{
			printf("Se finalizó porque el archivo es inexistente.\n");
			}break;
		case lecturaDenegadaPorFaltaDePermisos :{
			printf("Se finalizó porque no se tienen permisos de lectura para el archivo.\n");
				}break;
		case escrituraDenegadaPorFaltaDePermisos :{
			printf("Se finalizó porque no se tienen permisos de escritura para el archivo.\n");
				}break;
		case excepcionMemoria :{
			printf("Se finalizó porque se lanzo una excepción de memoria.\n");
				}break;

		case finalizacionDesdeConsola :{
			printf("Se finalizó porque se utilizo el comando de finalizacion por consola.\n");
				}break;
		case reservarMasMemoriaQueTamanoPagina:{
			printf("Se finalizó porque se reservó más memoria que tamanio de pagina.\n");
				}break;
		case noSePuedenAsignarMasPaginas :{
			printf("Se finalizó porque no se pueden asignar mas páginas.\n");
				}break;
		case finalizacionDesdeKenel :{
			printf("Se finalizó porque se utilizo el comando de finalizacion por consola del Kernel.\n");
				}break;
		case intentoAccederAUnSemaforoInexistente:{
			printf("Se finalizó porque se intento acceder a un semaforo inexistente.\n");
				}break;
		case intentoAccederAUnaVariableCompartidaInexistente:{
			printf("Se finalizó porque se intento acceder a una variable compartida inexistente.\n");
				}break;
		case lecturaDenegadaPorFileSystem:{
			printf("Se finalizó porque se denego la lectura por el FileSystem.\n");
				}break;
		case escrituraDenegadaPorFileSystem:{
			printf("Se finalizó porque se denego el escritura por el FileSystem.\n");
				}break;

		case noSeCreoElArchivoPorFileSystem:{
			printf("Se finalizó porque se denego la creacion por el FileSystem.\n");
				}break;
		case falloEnElFileDescriptor:{
			printf("Se finalizó porque ocurrio un fallo en el File Descriptor.\n");
				}break;
		case borradoFallidoOtroProcesoLoEstaUtilizando:{
			printf("Se finalizó porque no se pudo borrar el archivo debido a que otro proceso lo esta utilizando.\n");
				}break;
		case borradoFallidoPorFileSystem:{
			printf("Se finalizó porque se denego el borrado por FileSystem.\n");
				}break;
		case seQuiereUtilizarUnaVariableNoDeclarada:{
			printf("Se finalizó porque se quiso utilizar una variable no declarado.\n");
				}break;
		default:{
		}
	}
}
