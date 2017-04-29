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


//Hola


#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/config.h"

#include <arpa/inet.h>


#define MAXDATASIZE 100 // max number of bytes we can get at once
#define ID 3


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
/*
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
*/

	//Verdadero codigo


	size_t len = 0;
	while(1){

		printf("\nIngrese Comando: \n");
		//fgets(mensaje,100,stdin);					//en algun momento hacer un realloc turbio por si 100 no fueron suficientes o estuvieron de mas
		getline(&mensaje,&len,stdin);

		char** comandoYParametros = NULL;
		comandoYParametros = string_split(mensaje, " "); // separa la entrada en un char**

		if(strcmp(comandoYParametros[0],"iniciarPrograma") == 0){
			char** path= string_split(comandoYParametros[1], "\n");
			FILE* archivo = fopen(path[0], "r");

			fseek(archivo,0,SEEK_END);
			len = ftell(archivo);
			fseek(archivo,0,SEEK_SET);
			char* script = malloc(len+1);
			fread(script,len,1,archivo);
			script[len] = '\0';
			printf("%s   %d",script,len);

			fclose(archivo);
			free(script);
			liberarArray(path);
			liberarArray(comandoYParametros);
			continue;
		}
		//if(strcmp(comandoYParametros[0],"iniciarPrograma") == 0)

		if(strcmp(comandoYParametros[0],"desconectarConsola\n") == 0){
			liberarArray(comandoYParametros);
			break;
		}
		puts("Comando Inválido!");
	}
	// Envio del mensaje
	/*if(enviarMensaje(socketConsola,2,(void*)mensaje,strlen(mensaje)+1)==-1){
		perror("Error en el Send");
	}*/

	//close(socketConsola);
	free(mensaje);
	liberarConfiguracion();
	return 0;
}
