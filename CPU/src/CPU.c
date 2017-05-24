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
void pedidoValido(int*,void*,int);
void recibirInstruccion(void*,char**,int,int*);


char* script = "begin\nvariables a, b\na = 3\nb = 5\na = b + 12\nend\n";


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

	pcb = malloc(sizeof(PCB_DATA));

	printf("Inicializando CPU.....\n\n");


	// ******* Configuracion Inicial de CPU

 	printf("Configuracion Inicial: \n");

 	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/CPU/cpu.config");

 	imprimirConfiguracion();

 	//conectarConKernel();

 	conectarConMemoria();

 	//ESTE GRAN WHILE(1) ESTA COMENTADO PORQUE EN REALIDAD ES PARA RECIBIR UN PCB ATRAS DE OTRO Y EJECUTARLOS HASTA QUE EL KERNEL ME DIGA MORITE HIPPIE

	//while(1){
		terminoPrograma = false;

		// Recepcion del pcb
		puts("esperando pcb\n");
		/*
		void* stream;
		int accion = recibirMensaje(socketKernel,stream);
		switch(accion){
			case envioPCB:{
				pcb = deserealizarPCB(stream);
				break;
			}
			default:{
				perror("Error en la accion maquinola");
			}
		}*/

		t_metadata_program *metadata = metadata_desde_literal(script);

		pcb->pid = 1;
		pcb->contPags_pcb = 1;
		pcb->contextoActual = 0;
		pcb->exitCode = 0;
		pcb->indiceCodigo = metadata->instrucciones_serializado;
		pcb->indiceEtiquetas = metadata->etiquetas;
		pcb->cantidadDeEtiquetas = metadata->cantidad_de_etiquetas; /// HAY QUE VER ESTO; ACAAAAAAA MIRENME SOY UN COMENTARIO WUOWUOWUWOUWOWUOWUWOWWWOOO

		pcb->indiceStack = malloc(sizeof(t_entrada));
		pcb->indiceStack->argumentos = list_create();
		pcb->indiceStack->variables = list_create();

		pcb->cantidadDeEntradas = 1;
		pcb->cantidadDeInstrucciones = metadata->instrucciones_size;
		pcb->programCounter = metadata->instruccion_inicio;


		while(!terminoPrograma){

			t_pedidoMemoria pedido;
			pedido.id = pcb->pid;
			pedido.direccion = calcularDireccion(pcb->indiceCodigo[pcb->programCounter].start);
			pedido.direccion.size = pcb->indiceCodigo[pcb->programCounter].offset;


			// Pedido de Codigo
			enviarMensaje(socketMemoria,solicitarBytes,(void *)&pedido, sizeof(pedido));


			//Recepcion del codigo ANSISOP
			void* stream;
			int booleano;

			//Se recibe si tal pedido es valido o rompe por todos lados
			int accion = recibirMensaje(socketMemoria,&stream);

			pedidoValido(&booleano,stream,accion);


			//Si el pedido salio bien se pasa a pedir el codigo concretamente
			char* instruccion;


			if(booleano){
				recibirInstruccion(stream,&instruccion,pedido.direccion.size,&accion);
			}else{
				terminoPrograma = true;
				pcb->exitCode = -5;		//Excepcion de Memoria STACK OVERFLOW
			}

			//Si se recibio una linea de codigo se analiza
			if(accion == lineaDeCodigo){
				printf("\n%s\n\n",instruccion);

				//Magia del Parser para llamar a las primitivas
				analizadorLinea(instruccion,&AnSISOP_funciones,&AnSISOP_funciones_kernel);
				pcb->programCounter++;
			}

			free(instruccion);
			free(stream);

		}

		//NO SE LIBERA EL MATADATA PORQUE REALMENTE NO VA A ESTAR ACA SINO EN KERNEL Y SOLO TENDRIA QUE LIBERAR MI HERMOSO PCB
		//libera la memoria malloqueada por el PCB
		destruirPCB_Puntero(pcb);

	//}


	close(socketKernel);
	liberarConfiguracion();


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

void recibirInstruccion(void *stream, char** instruccion, int size, int* accion){
	*accion = recibirMensaje(socketMemoria,&stream);
	switch(*accion){
		case lineaDeCodigo:{
			*instruccion = stream;
			(*instruccion)[size - 1] = '\0';
		}break;
		default:{
			perror("Error en la accion maquinola");
		}break;
	}
}













