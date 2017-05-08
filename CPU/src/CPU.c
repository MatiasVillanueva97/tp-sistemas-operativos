/*
** client.c -- a stream socket client demo
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

#include "parser/metadata_program.h"
#include "parser/parser.h"
#include "commons/string.h"

#include "primitivas.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#define ID 1

enum id_Modulos{
	Kernel = 0,
	CPU = 1,
	Memoria = 2,
	Consola = 3,
	FileSystem = 4
};

typedef struct
{
	int pid;
	int contPags_pcb;
}__attribute__((packed)) PCB_DATA;

int socketKernel;
int socketMemoria;



int main(void)
{
	printf("Inicializando CPU.....\n\n");

	int rta_conexion;
	int i;
	PCB_DATA pcb;
	AnSISOP_funciones AnSISOP_funciones = {
			.AnSISOP_definirVariable = AnSISOP_definirVariable,
			.AnSISOP_obtenerPosicionVariable = AnSISOP_obtenerPosicionVariable,
			.AnSISOP_dereferenciar = AnSISOP_dereferenciar,
			.AnSISOP_asignar = AnSISOP_asignar,
			.AnSISOP_obtenerValorCompartida = AnSISOP_obtenerValorCompartida,
			.AnSISOP_asignarValorCompartida = AnSISOP_asignarValorCompartida,
			.AnSISOP_irAlLabel = AnSISOP_irAlLabel,
			.AnSISOP_llamarSinRetorno = AnSISOP_llamarSinRetorno,
			.AnSISOP_llamarConRetorno = AnSISOP_llamarConRetorno,
			.AnSISOP_finalizar = AnSISOP_finalizar,
			.AnSISOP_retornar = AnSISOP_retornar
	};
	AnSISOP_kernel AnSISOP_funciones_kernel = {
			.AnSISOP_wait = AnSISOP_wait,
			.AnSISOP_signal = AnSISOP_signal,
			.AnSISOP_reservar = AnSISOP_reservar,
			.AnSISOP_liberar = AnSISOP_liberar,
			.AnSISOP_abrir = AnSISOP_abrir,
			.AnSISOP_borrar = AnSISOP_borrar,
			.AnSISOP_cerrar = AnSISOP_cerrar,
			.AnSISOP_moverCursor = AnSISOP_moverCursor,
			.AnSISOP_escribir = AnSISOP_escribir,
			.AnSISOP_leer = AnSISOP_leer
	};
	// ******* Configuracion Inicial de CPU

 	printf("Configuracion Inicial: \n");

 	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config");
 	imprimirConfiguracion();

	// ******* Procesos de la CPU - por ahora solo recibir un mensaje


	socketKernel = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL")); // Asignación del socket que se conectara con el filesytem


	// validacion de un correcto handshake
	if (socketKernel == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketKernel == 2){
		perror("No se conectado con el Kernel, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketKernel, ID)) == -1) {
		perror("Error en el handshake con el Servidor");
		close(socketKernel);
	}
	printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);

	socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA"));

	// validacion de un correcto hadnshake
	if (socketMemoria == 1){
		perror("Falla en el protocolo de comunicación");
		exit(1);
	}
	if (socketMemoria == 2){
		perror("No se conectado con el Kernel, asegurese de que este abierto el proceso");
		exit(1);
	}
	if ( (rta_conexion = handshakeCliente(socketMemoria, ID)) == -1) {
		perror("Error en el handshake con el Servidor");
		close(socketMemoria);
	}
	printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);

	while(1){
		char* script = string_new();
		// Recepcion del pcb
		puts("esperando pcb\n");
		if(recibirMensaje(socketKernel,(void*)&pcb)==-1){
			perror("Error en el Reciv");
		}
		printf("%d\n",pcb.pid);

		// Pedido de Codigo
		enviarMensaje(socketMemoria,1,(void *)&pcb.pid, sizeof(int));
		//Recepcion del codigo ANSISOP
		if(recibirMensaje(socketMemoria,(void*)script)==-1){
			perror("Error en el Reciv");
		}
//Lo siguiente es provisorio y será reemplazado por un codigo similar al que hay en el dummy-cpu en el runner.c

		char** lineasDelScript= string_split(script,"\n");
		i = 0;
		/*while(lineasDelScript[i] != NULL){
		analizadorLinea(lineasDelScript[2],&AnSISOP_funciones,&AnSISOP_funciones_kernel);
			i++;
		}*/
		/*if (strcmp(lineasDelScript[1],TEXT_BEGIN)){
		      perror("Error en el begin");
		  }
		  while(lineasDelScript[i]!= NULL){
			  char ** algo = string_split(lineasDelScript[i]," s");
			  if(algo[2]!=NULL){
				  puts(algo[2]);

			  }



		   i++;
		  }
		  */
		puts(script);
	}
	close(socketKernel);
	liberarConfiguracion();
	// free(mensajeRecibido); --- esto tira cviolacion de segmentos
	return 0;
}
