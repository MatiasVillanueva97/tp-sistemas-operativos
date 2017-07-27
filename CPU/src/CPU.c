
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
#include "commons/log.h"

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

	logCPU = log_create("CPU.log","CPU",1,0);


	terminoPrograma = true;
	signal_SIGUSR1 = false;
	signal_sigint = false;
	hayPCB = false;
	quantumSleep = 0;

	signal(SIGUSR1, sigusr1_handler);
	signal(SIGINT, sigint_handler);

	log_info(logCPU,"Inicializando CPU.....\n\n");


	// ******* Configuracion Inicial de CPU

 	log_info(logCPU,"Configuracion Inicial: \n");

 	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config");

 	imprimirConfiguracion();

 	conectarConKernel();

 	conectarConMemoria();



 	void * stream;
 	if(recibirMensajeSeguro(socketKernel,(void*)&datosIniciales) != 7){
 		//recibirMensajeSeguro(socketKernel,(void*)&datosIniciales);

 		log_error(logCPU,"Kernel envio mal los datos iniciales, se desconecta la CPU\n");
 		close(socketMemoria);
 		close(socketKernel);
 		liberarConfiguracion();
 		log_destroy(logCPU);
 		exit(-1);

 	}

 	log_info(logCPU,"Datos recibidos de Kernel:\nsize_pag-%d\nquantum-%d\nsize_stack-%d\n", datosIniciales->size_pag, datosIniciales->quantum, datosIniciales->size_stack);

 	//ESTE GRAN WHILE(1) ESTA COMENTADO PORQUE EN REALIDAD ES PARA RECIBIR UN PCB ATRAS DE OTRO Y EJECUTARLOS HASTA QUE EL KERNEL ME DIGA MORITE HIPPIE

	while(!signal_SIGUSR1 && !signal_sigint){

 		int quantumRestante = datosIniciales->quantum;


		// Recepcion del pcb

 		pedidoPCB();

 		terminoPrograma = false;
 		bloqueado = false;

 		pcb->cantDeRafagasEjecutadas++;

 		confirmarQuantumSleep();

		while(!terminoPrograma && quantumRestante != 0 && !bloqueado){

			char* instruccion = pedirInstruccion();

			//Si se recibio una linea de codigo se analiza
			if(instruccion != NULL && !terminoPrograma){
				log_info(logCPU,"\nInstruccion leida: %s\n\n",instruccion);

				//Magia del Parser para llamar a las primitivas
				analizadorLinea(instruccion,&AnSISOP_funciones,&AnSISOP_funciones_kernel);
				pcb->programCounter++;
			}

			free(instruccion);
			quantumRestante--;
			usleep(quantumSleep * 1000);
		}

		//ACA AVISARLE A KERNEL QUE TERMINE QUE CON ESTE PROCESO
		if(!bloqueado){
			if(terminoPrograma){
				log_info(logCPU,"Envie el PCB al Kernel\n");
				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketKernel,enviarPCBaTerminado,pcbSerializado,tamanoPCB(pcb));
				free(pcbSerializado);
			}
			else{
				log_info(logCPU,"Envie el PCB al Kernel porque me quede sin quantum\n");
				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketKernel,enviarPCBaReady,pcbSerializado,tamanoPCB(pcb));
				free(pcbSerializado);
			}
		}
		terminoPrograma = true;
		hayPCB = false;




		//libera la memoria malloqueada por el PCB
		destruirPCB_Puntero(pcb);
	}


	close(socketKernel);
	close(socketMemoria);
	liberarConfiguracion();
	log_destroy(logCPU);
	free(datosIniciales);

	return 0;
}



//******************************************************************FUNCIONES PARA MODULARIZAR Y QUEDE UN LINDO CODIGO*********************************************************************\\

void conectarConMemoria(){
	int rta_conexion;
	socketMemoria = conexionConServidor(getConfigString("PUERTO_MEMORIA"),getConfigString("IP_MEMORIA")); // Asignación del socket que se conectara con la memoria
	if (socketMemoria == 1){
		log_error(logCPU,"Falla en el protocolo de comunicación\n");
		liberarConfiguracion();
		log_destroy(logCPU);
		enviarMensaje(socketKernel,7777,NULL,0);
		exit(-1);
	}
	if (socketMemoria == 2){
		log_error(logCPU,"No se conectado con el Memoria, asegurese de que este abierto el proceso\n");
		liberarConfiguracion();
		log_destroy(logCPU);
		enviarMensaje(socketKernel,7777,NULL,0);
		exit(-1);
	}
	if ( (rta_conexion = handshakeCliente(socketMemoria, CPU)) == -1) {
		log_error(logCPU,"Error en el handshake con Memoria\n");
		enviarMensaje(socketKernel,7777,NULL,0);
		close(socketMemoria);
		liberarConfiguracion();
		log_destroy(logCPU);
		exit(-1);
	}
	log_info(logCPU,"Conexión exitosa con el Memoria(%i)!!\n",rta_conexion);
}

void conectarConKernel()
{
	int rta_conexion;
	socketKernel = conexionConServidor(getConfigString("PUERTO_KERNEL"),getConfigString("IP_KERNEL")); // Asignación del socket que se conectara con la memoria
	if (socketKernel == 1){
		log_error(logCPU,"Falla en el protocolo de comunicación\n");
		liberarConfiguracion();
		log_destroy(logCPU);
		exit(-1);
	}
	if (socketKernel == 2){
		log_error(logCPU,"No se conectado con el Kernel, asegurese de que este abierto el proceso\n");
		liberarConfiguracion();
		log_destroy(logCPU);
		exit(-1);
	}
	if ( (rta_conexion = handshakeCliente(socketKernel, CPU)) == -1) {
		log_error(logCPU,"Error en el handshake con Kernel\n");
				liberarConfiguracion();
				log_destroy(logCPU);
				exit(-1);
	}
	log_info(logCPU,"Conexión exitosa con el Kernel(%i)!!\n",rta_conexion);
}

void pedidoValido(int* booleano,void* stream, int accion){
	switch(accion){
		case RespuestaBooleanaDeMemoria:{
			memcpy(booleano,stream,sizeof(int));
		}break;
		default:{
			log_error(logCPU,"Error en el protocolo de comunicacion\n");
		}break;
	}
}

void sigusr1_handler(int signal) {
	signal_SIGUSR1 = true;

	if(terminoPrograma && !hayPCB){ //si NO esta ejecutando
			log_info(logCPU,"Se recibio una SIGUSR1, se desconecta esta CPU\n");
			close(socketKernel);
			liberarConfiguracion();
			log_destroy(logCPU);
			free(datosIniciales);
			exit(-1);

	}

	log_info(logCPU,"Se recibio una SIGUSR1, la CPU se desconectara luego de terminada la rafaga\n");
	return;
}

void sigint_handler(int signal) {

	signal_sigint = true;

	if(terminoPrograma && !hayPCB){ //si NO esta ejecutando
		log_info(logCPU,"Se recibio una SIGINT, se desconecta esta CPU\n");
		//close(socketKernel);
		liberarConfiguracion();
		log_destroy(logCPU);
		free(datosIniciales);
		exit(-1);
		return;
	}
	log_info(logCPU,"Se recibio una SIGINT, la CPU se desconectara luego de terminada la rafaga\n");
	return;
}

void pedidoPCB(){
	enviarMensaje(socketKernel,pedirPCB,&(datosIniciales->quantum),sizeof(int));
	void* pcbSerializado;
	log_info(logCPU,"esperando pcb\n");
	int esto = recibirMensajeSeguro(socketKernel,&pcbSerializado);
	if(esto != envioPCB){
		printf("%d\n",esto);
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
		liberarConfiguracion();
		log_destroy(logCPU);
		exit(-1);
	}else{
		enviarMensaje(socketKernel,respuestaBooleanaKernel,&quantumSleep,sizeof(int));
		hayPCB = true;
		pcb=deserializarPCB(pcbSerializado);
		free(pcbSerializado);
		log_info(logCPU,"PCB recibido\n");
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
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
		liberarConfiguracion();
		log_destroy(logCPU);
		exit(-1);
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
