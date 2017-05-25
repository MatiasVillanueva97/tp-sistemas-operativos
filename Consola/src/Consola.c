/*
** client.c -- a stream socket client demo
*/
//Hola estoy brancheando_lololo
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

#include <pthread.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#include <arpa/inet.h>

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"


#define MAXDATASIZE 100 // max number of bytes we can get at once

void* rutinaIniciarPrograma(void*);
char* diferencia(char*,char*);
int* transformarFechaAInts(char*);

int socketKernel;
int hayMensajeNuevo = 1;

struct{
	int pid;
	char* mensaje;
} mensajeDeProceso;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

size_t tamanoArchivo(FILE * archivo){
	size_t tamano;
	fseek(archivo,0,SEEK_END);
	tamano = ftell(archivo);
	fseek(archivo,0,SEEK_SET);
return tamano;
}

char * generarScript(char * nombreDeArchivo){
	FILE* archivo = fopen(nombreDeArchivo, "r");
	size_t tamano = tamanoArchivo(archivo);
	char* script = malloc(tamano+1);
	fread(script,tamano,1,archivo);//lee el archivo y lo guarda en el script AnsiSOP
	script[tamano] = '\0';
	fclose(archivo);
	return script;
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



int main(void)
{
	printf("Inicializando Consola.....\n\n");

	size_t len = 0;
	char* mensaje = NULL;

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

			pthread_attr_t attr;
			pthread_t hilo ;
			//Hilos detachables cpn manejo de errores tienen que ser logs
			int  res;
			  res = pthread_attr_init(&attr);
			    if (res != 0) {
			        perror("Error en los atributos del hilo");
			        continue;
			    }
			    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			    if (res != 0) {
			        perror("Error en el seteado del estado de detached");
			        continue;
			    }

			    res = pthread_create (&hilo ,&attr,rutinaIniciarPrograma, (void *) script);
			    if (res != 0) {
			        perror("Error en la creacion del hilo");
			        continue;
			    }
			    pthread_attr_destroy(&attr);
			//free(script); 				ESTO ES LO QUE ROMPIA TO DO
			liberarArray(nombreDeArchivo);
			liberarArray(comandoConsola);
			continue;
		}
		if(strcmp(comandoConsola[0],"limpiarMensajes\n") == 0){
			system("clear");
			liberarArray(comandoConsola);
			continue;
		}
		if(strcmp(comandoConsola[0],"finalizarPrograma") == 0){
			char** stream= string_split(comandoConsola[1], "\n");
			int pid;
			pid = string_itoa(stream[0]);
			enviarMensaje(socketKernel,finalizarCiertoScript ,&pid, sizeof(int));
		}
		if(strcmp(comandoConsola[0],"desconectarConsola\n") == 0){
			liberarArray(comandoConsola);
			break;
		}
		puts("Comando Inválido!");
	}

	close(socketKernel);
	free(mensaje);
	liberarConfiguracion();
	return 0;
}

void* rutinaIniciarPrograma(void* parametro){
	int* pid= malloc(4);
	char* tiempoInicio = temporal_get_string_time();

	char* script = (char*) parametro;

	enviarMensaje(socketKernel,envioScriptAnsisop, script,strlen(script)+1);

	if (recibirMensaje(socketKernel, (void*) &pid) != envioDelPidEnSeco){
			perror("Error al recibir el pid");
	}
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

	printf("%s\n",tiempoInicio);

	char* tiempoFin = temporal_get_string_time();

	printf("%s\n",tiempoFin);

	//printf("%s\n",diferencia(tiempoInicio,tiempoFin));

	free(pid);
	free(script);
}


//FALTA TERMINAR Y HACER MAS LINDA CUANDO NO SEAN LAS 5 AM hh:mm:ss:mmmm
int* transformarFechaAInts(char * fecha){
	char** arrayCalendario = string_split(fecha,":");
	int* fechaInt;
	int i;
	for (i=0;i<4;i++){
		fechaInt[i] = atoi(arrayCalendario[i]);
	}
	return fechaInt;
}
//ARREGLAR
char* diferencia(char* fechaInicio,char* fechaFin){
	int resultadoInt[4];
	int* arrayInicio = transformarFechaAInts(fechaInicio);
	int* arrayFin = transformarFechaAInts(fechaFin);
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
	}
//HACELO CON GANAS NO SEAS FORRO

	char* diferencia = strcat(strcat(strcat(strcat(resultadoChar[0], ":"), strcat(resultadoChar[1], ":")), strcat(resultadoChar[2], ":")), resultadoChar[3]);
	return diferencia;
}

