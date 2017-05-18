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
#include "commons/collections/list.h"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"
//#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"

#include "primitivas.h"
#include "compartidas.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"


char* script =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";


int main(void)
{
	printf("Inicializando CPU.....\n\n");

	int rta_conexion;
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
	if ( (rta_conexion = handshakeCliente(socketKernel, CPU)) == -1) {
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
	if ( (rta_conexion = handshakeCliente(socketMemoria, CPU)) == -1) {
		perror("Error en el handshake con el Servidor");
		close(socketMemoria);
	}
	printf("Conexión exitosa con el Servidor(%i)!!\n",rta_conexion);

	while(1){
		terminoPrograma = false;
		// Recepcion del pcb
		puts("esperando pcb\n");

		/*
		if(recibirMensaje(socketKernel,(void*)&pcb)==-1){
			perror("Error en el Reciv");
		}
		*/

		t_metadata_program *metadata = metadata_desde_literal(script);

		pcb.pid = 0;
		pcb.contPags_pcb = 1;
		pcb.contextoActual = -1;
		pcb.exitCode = 0;
		pcb.indiceCodigo = metadata->instrucciones_serializado;
		pcb.indiceEtiquetas = metadata->etiquetas;
		pcb.cantidadEtiquetas = metadata->cantidad_de_etiquetas; /// HAY QUE VER ESTO; ACAAAAAAA MIRENME SOY UN COMENTARIO WUOWUOWUWOUWOWUOWUWOWWWOOO
		pcb.indiceStack = malloc(sizeof(t_entrada));
		pcb.indiceStack->argumentos = list_create();
		pcb.indiceStack->variables = list_create();

		printf("%d\n",pcb.pid);
/*
		// Pedido de Codigo
		enviarMensaje(socketMemoria,1,(void *)&pcb.pid, sizeof(int));
		//Recepcion del codigo ANSISOP
		if(recibirMensaje(socketMemoria,(void*)script)==-1){
			perror("Error en el Reciv");
		}
*/
		char** lineas = string_split(script,"\n");
		int i = 0;

		while(!terminoPrograma){

			char* instruccion = lineas[i];
			puts(instruccion);
			/*t_pedidoMemoria pedido;
			pedido.id = pcb.pid;
			pedido.direccion = calcularDireccion(pcb.indiceCodigo[pcb.programCounter].start);
			pedido.direccion.size = pcb.indiceCodigo[pcb.programCounter].offset;
*/
/*
			// Pedido de Codigo
			//Falta crear un caso de enviarMensaje para este tipo de pedidos
			enviarMensaje(socketMemoria,5,(void *)&pedido, sizeof(pedido));
			//Recepcion del codigo ANSISOP
			if(recibirMensaje(socketMemoria,(void*)instruccion)==-1){
				perror("Error en el Reciv");
			}
*/



			//Magia del Parser para llamar a las primitivas
			analizadorLinea(instruccion,&AnSISOP_funciones,&AnSISOP_funciones_kernel);

			free(instruccion);
			i++;
		}
		free(script);
	}
	close(socketKernel);
	liberarConfiguracion();
	// free(mensajeRecibido); --- esto tira cviolacion de segmentos
	return 0;
}
