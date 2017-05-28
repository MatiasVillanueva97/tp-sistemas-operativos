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
#include <semaphore.h>
#include <pthread.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#include <arpa/inet.h>

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

void* rutinaPrograma(void*);
char* diferencia(char*,char*);
void transformarFechaAInts(char*, int[4]);
typedef struct {
	int pid;
	int tid;
}pidtid;

struct{
	int pid;
	char* mensaje;
} mensajeDeProceso;

int socketKernel;
int hayMensajeNuevo = 1;
t_list* listaPidTid;

pthread_mutex_t mutex_lista;
pthread_mutex_t mutex_escribirEnPantalla;

size_t tamanoArchivo(FILE * archivo){
	size_t tamano;
	fseek(archivo,0,SEEK_END);//ACA FALTA HANDELEAR ERROR
	tamano = ftell(archivo);
	fseek(archivo,0,SEEK_SET);
return tamano;
}

char * generarScript(char * nombreDeArchivo){
	FILE* archivo;

    if ( !(archivo = fopen(nombreDeArchivo, "r")) )
    {
    	perror("Error: el archivo no esta en el directorio o no existe \n");
    	exit(-1);// Va si o si , en caso de que exista el error sino tira segmentation fault
    }
	size_t tamano = tamanoArchivo(archivo);
	char* script = malloc(tamano+1);
	fread(script,tamano,1,archivo);//lee el archivo y lo guarda en el script AnsiSOP
    if(tamano*1 != fread(script,tamano,1,archivo))
    {
        printf("\n Error : fallo la lectura del archivo \n");
        exit(-1);
    }
	script[tamano] = '\0';
	fclose(archivo);
	return script;
}
void transformarFechaAInts(char * fecha, int arrayFecha[4]){
	char** arrayCalendario = string_split(fecha,":");
	int i;
	for (i=0;i<3;i++){
		arrayFecha[i] = atoi(arrayCalendario[i]);
	}

}
char* diferencia(char* fechaInicio,char* fechaFin){
	int resultadoInt[4];
	int arrayInicio[4];
	transformarFechaAInts(fechaInicio, arrayInicio);
	int arrayFin[4];
	transformarFechaAInts(fechaFin,arrayFin);
	int i;
	char* resultadoChar[4];
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
		resultadoChar[i] = string_itoa(resultadoInt[i]);
		strcat(resultadoChar[i], ":");

	}

	char* diferencia = strcat(strcat(strcat(resultadoChar[0], resultadoChar[1]),resultadoChar[2]), resultadoChar[3]);

	return diferencia;
}


void conectarConKernel(){
	int rta_conexion;
	socketKernel = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL"));

		// validacion de un correcto hadnshake
		if (socketKernel== 1){
			perror("Falla en el protocolo de comunicación");
			exit(1);
		}
		if (socketKernel == 2){
			perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
			exit(1);
		}
		if ( (rta_conexion = handshakeCliente(socketKernel, Consola)) == -1) {
			perror("Error en el handshake con el Servidor");
			close(socketKernel);
		}
		printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);
}
void agregarAListaDePidTid(int pid, int tid){
	pidtid	aux;
	aux.pid = pid;
	aux.tid = tid;
	list_add(listaPidTid, &aux);

}
void matarHiloPrograma(int pid){
	bool sonIguales(pidtid * elementos){
		return  elementos->pid == pid;
	}
list_find(listaPidTid, sonIguales);
	//if(pthread_kill(hiloAMatar.tid,SIGKILL) != 0){
		//perror("Error al matar el hilo");
	//}else{
	list_remove_by_condition(listaPidTid, sonIguales);
}
//}
void crearHiloDetach( char* script){
	pthread_attr_t attr;
				pthread_t hilo ;
				//Hilos detachables cpn manejo de errores tienen que ser logs
				int  res;
				  res = pthread_attr_init(&attr);
				    if (res != 0) {
				        perror("Error en los atributos del hilo");

				    }
				    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
				    if (res != 0) {
				        perror("Error en el seteado del estado de detached");

				    }

				    res = pthread_create (&hilo ,&attr,rutinaPrograma, (void *) script);
				    if (res != 0) {
				        perror("Error en la creacion del hilo");

				    }
				    pthread_attr_destroy(&attr);
}

int main(void)
{
	printf("Inicializando Consola.....\n\n");

	size_t len = 0;
	char* mensaje = NULL;
	listaPidTid = list_create();
	sem_init(&mutex_lista,0,1);

	sem_init(&mutex_escribirEnPantalla,0,0);
	// ******* Configuración inicial Consola

	printf("Configuracion Inicial:\n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Consola/consola.config");
	imprimirConfiguracion();

	conectarConKernel();

	while(1){//Ciclo donde se ejecutan los comandos principales.

		printf("\nIngrese Comando: \n");

		getline(&mensaje,&len,stdin);

		char** comandoConsola = NULL;//Esta variable es para cortar el mensaje en 2.
		comandoConsola = string_split(mensaje, " "); // separa la entrada en un char**

		if(strcmp(comandoConsola[0],"iniciarPrograma") == 0){//Primer Comando iniciarPrograma
			char** nombreDeArchivo= string_split(comandoConsola[1], "\n");//Toma el parametro que contiene el archivo y le quita el \n
			char* script = generarScript(nombreDeArchivo[0]);// lee el archivo y guarda en script.
			crearHiloDetach(script); //Crea el hilo del programa

			liberarArray(nombreDeArchivo);
			liberarArray(comandoConsola);
			sem_wait(&mutex_escribirEnPantalla);
			continue;
		}
		if(strcmp(comandoConsola[0],"limpiarMensajes\n") == 0){
			system("clear");
			liberarArray(comandoConsola);
			continue;
		}
		if(strcmp(comandoConsola[0],"finalizarPrograma") == 0){
			char** stream= string_split(comandoConsola[1],"\n");
			int *pid;
			*pid = atoi(*stream);
			enviarMensaje(socketKernel,finalizarCiertoScript ,pid, sizeof(int));

			int respuesta = recibirMensaje(socketKernel, &pid);

			sem_wait(&mutex_lista);
			//matarHiloPrograma(pid); DEBE IMPLEMENTARSE
			sem_post( &mutex_lista );
			//Futuro wait del mutex_escribirEnPantalla
			if (respuesta == pidFinalizado)
				printf("Se finalizo correctamente el pid: %d", *pid);
			else if (respuesta == errorFinalizacionPid)
				printf("No se ha finalizado correctamente el pid: %d", *pid);
			continue;
		}
		if(strcmp(comandoConsola[0],"desconectarConsola\n") == 0){
			liberarArray(comandoConsola);
			enviarMensaje(socketKernel,desconectarConsola, NULL, 0);

			break;
		}
		puts("Comando Inválido!");
	}

	close(socketKernel);
	free(mensaje);
	liberarConfiguracion();
	return 0;
}

void* rutinaPrograma(void* parametro){
	int* pid= malloc(4);
	char* tiempoInicio = temporal_get_string_time();

	char* script = (char*) parametro;
	enviarMensaje(socketKernel,envioScriptAnsisop, script,strlen(script)+1);


	if (recibirMensaje(socketKernel, (void*) &pid) != envioDelPidEnSeco){
			perror("Error al recibir el pid");
	}
	printf("Tiempo de inicio :%s\n",tiempoInicio);

	int tid= process_get_thread_id();

	sem_wait(&mutex_lista);
	agregarAListaDePidTid(*pid,tid);
	sem_post( &mutex_lista );

	printf("Numero de pid: %d \n",*pid);


/*	pthread_mutex_lock( &mutex );
	if(hayMensajeNuevo){
		recibirMensaje(socketKernel,(void *)&mensajeDeProceso);
		hayMensajeNuevo = 0;
	}
	if(mensajeDeProceso.pid == *pid){
		printf("%s",mensajeDeProceso.mensaje);
		hayMensajeNuevo = 1;
	}
	pthread_mutex_unlock( &mutex );*/



	char* tiempoFin = temporal_get_string_time();

	printf("Tiempo de Finalización: %s\n",tiempoFin);

	printf("Tiempo de Ejecución: %s\n",diferencia(tiempoInicio,tiempoFin));

	free(pid);
	free(script);

	sem_post(&mutex_escribirEnPantalla);
}


//FALTA TERMINAR Y HACER MAS LINDA CUANDO NO SEAN LAS 5 AM hh:mm:ss:mmmm

//ARREGLAR

