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
#include <pthread.h>
#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#include <arpa/inet.h>

#define ID 3

enum id_Modulos{
	Kernel = 0,
	CPU = 1,
	Memoria = 2,
	Consola = 3,
	FileSystem = 4
};


typedef struct {
	int socket;
	size_t tamanioScript;
	char * script;
} t_parametrosHiloPrograma;

#define MAXDATASIZE 100 // max number of bytes we can get at once

void* laFuncionMagicaDeConsola(void*);

int hayMensajeNuevo = 1;

struct{
	int pid;
	char* mensaje;
} mensajeDeProceso;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
	printf("Inicializando Consola.....\n\n");

	int socketConsola, rta_conexion;
	char* mensaje = NULL;
	//char s[INET6_ADDRSTRLEN];


	// ******* Configuración inicial Consola

	printf("Configuracion Inicial:\n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Consola/consola.config");
	imprimirConfiguracion();


	// ******* Procesos de Consola-  por ahora enviar mensajitos

	socketConsola = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL"));

	// validacion de un correcto hadnshake
	if (socketConsola == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketConsola == 2){
		perror("No se conectado con el FileSystem, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketConsola, ID)) == -1) {
		perror("Error en el handshake con el Servidor");
		close(socketConsola);
	}
	printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);


	//Verdadero codigo


	size_t len = 0;
	while(1){//Ciclo donde se ejecutan los comandos principales.

		printf("\nIngrese Comando: \n");

		getline(&mensaje,&len,stdin);//Aca es donde la persona mete por teclado el comando que desea ejecutar
									//Por ahora no hay directorio, se guarda en la carpeta del Debug.
									//Para probar usar algo.txt

		char** comandoConsola = NULL;//Esta variable es para cortar el mensaje en 2.
		comandoConsola = string_split(mensaje, " "); // separa la entrada en un char**

		if(strcmp(comandoConsola[0],"iniciarPrograma") == 0){//Primer Comando iniciarPrograma
			char** nombreDeArchivo= string_split(comandoConsola[1], "\n");//Toma el parametro que contiene el archivo y le quita el \n
			FILE* archivo = fopen(nombreDeArchivo[0], "r");

			fseek(archivo,0,SEEK_END);
			len = ftell(archivo);
			fseek(archivo,0,SEEK_SET);//De fseek a fseek sirve para conocer el tamaño del archivo
			//len = 4;
			char* script = malloc(len+1);
			fread(script,len,1,archivo);//lee el archivo y lo guarda en el script AnsiSOP
			//script = "hola";
			script[len] = '\0';//Importante!

			printf("%s",script);


			t_parametrosHiloPrograma parametrosHiloPrograma;
			parametrosHiloPrograma.socket = socketConsola;
			parametrosHiloPrograma.tamanioScript = len+1;
			parametrosHiloPrograma.script = script;

			pthread_attr_t attr;
			pthread_t h1 ;
			int  res;
			  res = pthread_attr_init(&attr);
			    if (res != 0) {
			        perror("Attribute init failed");
			        exit(EXIT_FAILURE);
			    }
			    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			    if (res != 0) {
			        perror("Setting detached state failed");
			        exit(EXIT_FAILURE);
			    }

			    res = pthread_create (&h1 ,&attr,laFuncionMagicaDeConsola, (void *) &parametrosHiloPrograma);
			    if (res != 0) {
			        perror("Creation of thread failed");
			        exit(EXIT_FAILURE);
			    }
			    pthread_attr_destroy(&attr);

			fclose(archivo);

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

		if(strcmp(comandoConsola[0],"desconectarConsola\n") == 0){
			liberarArray(comandoConsola);
			break;
		}
		puts("Comando Inválido!");
	}

	close(socketConsola);
	free(mensaje);
	liberarConfiguracion();
	return 0;
}

void* laFuncionMagicaDeConsola(void* parametros){
	int *pid = malloc(4);
	t_parametrosHiloPrograma *parametrosHiloPrograma = parametros;
	enviarMensaje(parametrosHiloPrograma->socket,2,(void *)parametrosHiloPrograma->script, parametrosHiloPrograma->tamanioScript);
	recibirMensaje(parametrosHiloPrograma->socket,(void *)pid);
	int aux = *pid;

	printf("%d\n",aux);

	pthread_mutex_lock( &mutex );

	/*if(hayMensajeNuevo){
		recibirMensaje(parametrosHiloPrograma->socket,(void *)&mensajeDeProceso);
		hayMensajeNuevo = 0;
	}
	if(mensajeDeProceso.pid == *pid){
		printf("%s",mensajeDeProceso.mensaje);
		hayMensajeNuevo = 1;
	}
	*/
	pthread_mutex_unlock( &mutex );

	free(pid);
	free(parametrosHiloPrograma->script);
}
