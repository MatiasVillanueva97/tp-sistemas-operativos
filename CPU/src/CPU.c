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
void sigint_handler(int signal);
void pedidoValido(int*,void*,int);
char* pedirInstruccion();
void pedidoPCB();
void confirmarQuantumSleep();


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


	terminoPrograma = true;
	signal_SIGUSR1 = false;
	signal_sigint = false;
	hayPCB = false;

	signal(SIGUSR1, sigusr1_handler);
	signal(SIGINT, sigint_handler);

	printf("Inicializando CPU.....\n\n");


	// ******* Configuracion Inicial de CPU

 	printf("Configuracion Inicial: \n");

 	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config");

 	imprimirConfiguracion();

 	conectarConKernel();

 	conectarConMemoria();



 	void * stream;
 	if(recibirMensajeSeguro(socketKernel, &stream) != 7){
 		recibirMensajeSeguro(socketKernel,(void*)&datosIniciales);
 		/*
 		perror("Kernel envio mal los datos iniciales, se desconecta la CPU");
 		close(socketMemoria);
 		close(socketKernel);
 		liberarConfiguracion();
 		exit(-1);
 		*/
 	}
 	else
 		datosIniciales = stream;

 	printf("Datos recibidos de Kernel:\nsize_pag-%d\nquantum-%d\nsize_stack-%d\n", datosIniciales->size_pag, datosIniciales->quantum, datosIniciales->size_stack);

 	//ESTE GRAN WHILE(1) ESTA COMENTADO PORQUE EN REALIDAD ES PARA RECIBIR UN PCB ATRAS DE OTRO Y EJECUTARLOS HASTA QUE EL KERNEL ME DIGA MORITE HIPPIE

	while(!signal_SIGUSR1 && !signal_sigint){

 		int quantumRestante = datosIniciales->quantum;


		// Recepcion del pcb

 		pedidoPCB();

 		terminoPrograma = false;
 		bloqueado = false;

 		pcb->cantDeRafagasEjecutadas++;

 		confirmarQuantumSleep();

		while(!terminoPrograma && quantumRestante != 0 && !bloqueado && !signal_sigint){

			char* instruccion = pedirInstruccion();

			//Si se recibio una linea de codigo se analiza
			if(instruccion != NULL){
				printf("\nInstruccion leida: %s\n\n",instruccion);

				//Magia del Parser para llamar a las primitivas
				analizadorLinea(instruccion,&AnSISOP_funciones,&AnSISOP_funciones_kernel);
				pcb->programCounter++;
			}

			free(instruccion);
			quantumRestante--;
			usleep(quantumSleep * 1000);
		}

		//ACA AVISARLE A KERNEL QUE TERMINE QUE CON ESTE PROCESO
		if(terminoPrograma){
			printf("Envie el PCB al Kernel\n");
			void* pcbSerializado = serializarPCB(pcb);
			enviarMensaje(socketKernel,enviarPCBaTerminado,pcbSerializado,tamanoPCB(pcb));
			free(pcbSerializado);
		}

		else{
			if(quantumRestante == 0){
				printf("Envie el PCB al Kernel porque me quede sin quantum\n");
				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketKernel,enviarPCBaReady,pcbSerializado,tamanoPCB(pcb));
				free(pcbSerializado);
			}
			else{
				if(signal_sigint){
					printf("Envie el PCB al Kernel porque me desconectaron bruscamente\n");
					void* pcbSerializado = serializarPCB(pcb);
					enviarMensaje(socketKernel,enviarPCBaReady,pcbSerializado,tamanoPCB(pcb));
					free(pcbSerializado);
				}
			}
		}

		//libera la memoria malloqueada por el PCB
		destruirPCB_Puntero(pcb);
		hayPCB = false;
	}


	close(socketKernel);
	close(socketMemoria);
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
			puts("Error en el protocolo de comunicacion");
		}break;
	}
}

void sigusr1_handler(int signal) {
	signal_SIGUSR1 = true;

	if(terminoPrograma && !hayPCB){ //si NO esta ejecutando
			printf("Se recibio una SIGUSR1, se desconecta esta CPU\n");
			close(socketKernel);
			liberarConfiguracion();
			free(datosIniciales);
			exit(-1);

	}

	printf("Se recibio una SIGUSR1, la CPU se desconectara luego de terminada la rafaga\n");
	return;
}

void sigint_handler(int signal) {

	signal_sigint = true;

	if(terminoPrograma && !hayPCB){ //si NO esta ejecutando
		printf("Se recibio una SIGINT, se desconecta esta CPU\n");
		close(socketKernel);
		liberarConfiguracion();
		free(datosIniciales);
		exit(-1);
		return;
	}
	printf("Se recibio una SIGINT, la CPU se desconectara luego de terminada la instruccion\n");
	return;
}

void pedidoPCB(){
	enviarMensaje(socketKernel,pedirPCB,&(datosIniciales->quantum),sizeof(int));
	void* pcbSerializado;
	puts("esperando pcb\n");
	if(recibirMensajeSeguro(socketKernel,&pcbSerializado) != envioPCB){
		puts("Error en el protocolo de comunicacion");
	}else{
		hayPCB = true;
		pcb=deserializarPCB(pcbSerializado);
		free(pcbSerializado);
		puts("PCB recibido");
		//En el caso que esta sea la primera vez que el proceso entra en una CPU su indice de stack estara NULL porque no habia entradas anteriores, entonces se inicializa la primera entrada
		if(pcb->indiceStack == NULL){
			pcb->indiceStack = realloc(pcb->indiceStack,sizeof(t_entrada));
			pcb->indiceStack->argumentos = list_create();
			pcb->indiceStack->variables = list_create();
			pcb->cantidadDeEntradas++;
		}
	}
}

void confirmarQuantumSleep(){
	enviarMensaje(socketKernel,dameQuantumSleep,&(quantumSleep),sizeof(int));

	void* stream;

	if(recibirMensajeSeguro(socketKernel,&stream) == respuestaBooleanaKernel){
		quantumSleep = *(int*)stream;
		free(stream);
	}else{
		puts("Error en el protocolo de comunicacion");
		//algo mas?
	}

}

void recepcionCodigo(t_pedidoMemoria pedido,char* instruccion,int tamano){
	//Recepcion del codigo ANSISOP
	void* stream;
	int booleano;
	//Se recibe si tal pedido es valido o rompe por todos lados
	int accion = recibirMensajeSeguro(socketMemoria,&stream);
	pedidoValido(&booleano,stream,accion);
	//Si el pedido salio bien se pasa a pedir el codigo concretamente
	if(booleano){
		free(stream);
		if(recibirMensajeSeguro(socketMemoria,&stream) == lineaDeCodigo){
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
		segundo = pedido.direccion.offset + pedido.direccion.size - datosIniciales->size_pag;
		pedido.direccion.size = datosIniciales->size_pag - pedido.direccion.offset;
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
