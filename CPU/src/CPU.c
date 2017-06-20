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
#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.c"
#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#include "primitivas.h"
#include "compartidas.h"

void conectarConMemoria();
void conectarConKernel();
void sigusr1_handler(int signal);
void pedidoValido(int*,void*,int);
char* pedirInstruccion();
void pedidoPCB();


int main(void)
{
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



	signal_SIGUSR1 = false;

	signal(SIGUSR1, sigusr1_handler);

	printf("Inicializando CPU.....\n\n");


	// ******* Configuracion Inicial de CPU

 	printf("Configuracion Inicial: \n");

 	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config");

 	imprimirConfiguracion();

 	conectarConKernel();

 	conectarConMemoria();



 	if(recibirMensaje(socketKernel,(void*)&datosIniciales) != 7) perror("Kernel que es esta basura? Dame mis datos para comenzar");

 	printf("Datos recibidos de Kernel:\nsize_pag-%d\nquantum-%d\nsize_stack-%d\n", datosIniciales->size_pag, datosIniciales->quantum, datosIniciales->size_stack);

 	//ESTE GRAN WHILE(1) ESTA COMENTADO PORQUE EN REALIDAD ES PARA RECIBIR UN PCB ATRAS DE OTRO Y EJECUTARLOS HASTA QUE EL KERNEL ME DIGA MORITE HIPPIE

	while(!signal_SIGUSR1){

 		int quantumRestante = datosIniciales->quantum;

 		terminoPrograma = false;
 		bloqueado = false;

		// Recepcion del pcb

 		pedidoPCB();

		while(!terminoPrograma && quantumRestante != 0 && !bloqueado){

			char* instruccion = pedirInstruccion();

			//Si se recibio una linea de codigo se analiza
			if(instruccion != NULL){
				printf("\n%s\n\n",instruccion);

				//Magia del Parser para llamar a las primitivas
				analizadorLinea(instruccion,&AnSISOP_funciones,&AnSISOP_funciones_kernel);
				pcb->programCounter++;
			}

			free(instruccion);
			quantumRestante--;
		}

		//ACA AVISARLE A KERNEL QUE TERMINE QUE CON ESTE PROCESO
		if(terminoPrograma){
			printf("Envie el PCB al Kernel porque termine toda su ejecucion\n");
			void* pcbSerializado = serializarPCB(pcb);
			enviarMensaje(socketKernel,enviarPCBaTerminado,pcbSerializado,tamanoPCB(pcb));
			free(pcbSerializado);
		}

		if(quantumRestante == 0){
			printf("Envie el PCB al Kernel porque me quede sin quantum\n");
			void* pcbSerializado = serializarPCB(pcb);
			enviarMensaje(socketKernel,enviarPCBaReady,pcbSerializado,tamanoPCB(pcb));
			free(pcbSerializado);
		}

		//libera la memoria malloqueada por el PCB
		destruirPCB_Puntero(pcb);

	}


	close(socketKernel);
	liberarConfiguracion();
	free(datosIniciales);

	return 0;
}



//******************************************************************FUNCIONES PARA MODULARIZAR Y QUEDE UN LINDO CODIGO*********************************************************************\\


void conectarConMemoria()
{
	int rta_conexion;
	socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria
	if (socketMemoria == 1){
		perror("Falla en el protocolo de comunicación");
	}
	if (socketMemoria == 2){
		perror("No se conectado con el Memoria, asegurese de que este abierto el proceso");
	}
	if ( (rta_conexion = handshakeCliente(socketMemoria, CPU)) == -1) {
				perror("Error en el handshake con Memoria");
				close(socketMemoria);
	}
	printf("Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);
}

void conectarConKernel()
{
	int rta_conexion;
	socketKernel = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL")); // Asignación del socket que se conectara con la memoria
	if (socketKernel == 1){
		perror("Falla en el protocolo de comunicación");
	}
	if (socketKernel == 2){
		perror("No se conectado con el Kernel, asegurese de que este abierto el proceso");
	}
	if ( (rta_conexion = handshakeCliente(socketKernel, CPU)) == -1) {
				perror("Error en el handshake con Kernel");
				close(socketMemoria);
	}
	printf("Conexión exitosa con el Kernel(%i)!!\n",rta_conexion);
}

void pedidoValido(int* booleano,void* stream, int accion){
	switch(accion){
		case RespuestaBooleanaDeMemoria:{
			memcpy(booleano,stream,sizeof(int));
		}break;
		default:{
			perror("Error en la accion maquinola");
		}break;
	}
}

void sigusr1_handler(int signal) {
	signal_SIGUSR1 = true;
	puts("Se recibio una SIGUSR1, la CPU se desconectara luego de terminada la ejecucion");
	return;
}

void pedidoPCB(){
	enviarMensaje(socketKernel,pedirPCB,&(datosIniciales->quantum),sizeof(int));
	void* pcbSerializado;
	puts("esperando pcb\n");
	if(recibirMensaje(socketKernel,&pcbSerializado) != envioPCB) perror("Error en la accion maquinola");
	pcb=deserializarPCB(pcbSerializado);
	free(pcbSerializado);
	//En el caso que esta sea la primera vez que el proceso entra en una CPU su indice de stack estara NULL porque no habia entradas anteriores, entonces se inicializa la primera entrada
	if(pcb->indiceStack == NULL){
		pcb->indiceStack = realloc(pcb->indiceStack,sizeof(t_entrada));
		pcb->indiceStack->argumentos = list_create();
		pcb->indiceStack->variables = list_create();
		pcb->cantidadDeEntradas++;
	}
}

void recepcionCodigo(t_pedidoMemoria pedido,char* instruccion,int tamano){
	//Recepcion del codigo ANSISOP
	void* stream;
	int booleano;
	//Se recibe si tal pedido es valido o rompe por todos lados
	int accion = recibirMensaje(socketMemoria,&stream);
	pedidoValido(&booleano,stream,accion);
	//Si el pedido salio bien se pasa a pedir el codigo concretamente
	if(booleano){
		free(stream);
		if(recibirMensaje(socketMemoria,&stream) == lineaDeCodigo){
			memcpy(instruccion,stream,tamano);
			free(stream);
		}
	}else{
		free(stream);
		terminoPrograma = true;
		pcb->exitCode = -5;		//Excepcion de Memoria STACK OVERFLOW
	}
}

char* pedirInstruccion(){
	t_pedidoMemoria pedido;
	pedido.id = pcb->pid;
	pedido.direccion = calcularDireccion(pcb->indiceCodigo[pcb->programCounter].start);
	pedido.direccion.size = pcb->indiceCodigo[pcb->programCounter].offset;

	int total = pedido.direccion.size;
	int segundo = 0;
	char* instruccion = malloc(total + 1);

	if(pedido.direccion.offset + pedido.direccion.size >= datosIniciales->size_pag){
		pedido.direccion.size = datosIniciales->size_pag - pedido.direccion.offset;
		segundo = pedido.direccion.offset + pedido.direccion.size - datosIniciales->size_pag;
	}

	enviarMensaje(socketMemoria,solicitarBytes,(void *)&pedido, sizeof(pedido));

	recepcionCodigo(pedido,instruccion,total - segundo);

	if(segundo > 0){
		pedido.direccion.page++;
		pedido.direccion.offset = 0;
		pedido.direccion.size = segundo;

		enviarMensaje(socketMemoria,solicitarBytes,(void *)&pedido, sizeof(pedido));

		recepcionCodigo(pedido,instruccion + total - segundo,segundo);

	}

	instruccion[total - 1] = '\0';

	return instruccion;

}
