/*
 * funcionesCPU.c
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#include "funcionesCPU.h"

void cpu_crearHiloDetach(int nuevoSocket){
	pthread_attr_t attr;
	pthread_t hilo_rutinaCPU ;

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

	res = pthread_create (&hilo_rutinaCPU ,&attr,rutinaCPU, (void *)nuevoSocket);
	if (res != 0) {
		perror("Error en la creacion del hilo");
	}

	pthread_attr_destroy(&attr);
}


//***Esa funcion devuelve un un PCB que este listo para mandarse a ejecutar , en caso de que ninguno este listo retorna null
PCB_DATA * cpu_pedirPCBDeExec(){

	bool encontroPCB = false;
	PCB_DATA* pcb;
	int i, cantidadProcesos=0;

	//***Voy a estar buscando en la cola de exec hasta que encuentre alguno
	while(!encontroPCB)
	{
		//sem_wait(&cola_Exec);
		//***Me fijo la cantidad de procesos que hay en la cola de exec
		cantidadProcesos = queue_size(cola_Exec);

		//***Voy a iterar tantas veces como elementos tenga en la cola de exec
		for(i=0; i < cantidadProcesos; i++)
		{
			//***Tomo el primer pcb de la cola
			pcb = queue_pop(cola_Exec);

			//*** Valido si el pcb se puede mandar a ejecutar
			if(pcb->exitCode == paraEjecutar)
			{
				//***Esta listo para ejecutar, le cambio el exitcode
				pcb->exitCode = loEstaUsandoUnaCPU;

				//***Lo agrego al final de la cola de exec
				queue_push(cola_Exec, pcb);

				//***Cambio el booleano a true, porque acabo de encontrar un pcb y asi cortar el while y hago el break del for
				encontroPCB=true;
				break;
			}
			else{
				queue_push(cola_Exec, pcb);
			}
		}
		//sem_post(&cola_Exec);
	}

	return pcb;
}

void *rutinaCPU(void * arg)
{
	int socketCPU = (int)arg;

	//*** Le envio a la CPU todos los datos qeu esta necesitara para poder trabajar, como el tamaño de una pagina de memoria, el quantum y la cantidad de paginas que ocupa un stack
	DATOS_PARA_CPU datosCPU;
	datosCPU.size_pag=size_pagina;
	datosCPU.quantum=quantumRR;
	datosCPU.size_stack=stack_size;
	enviarMensaje(socketCPU,enviarDatosCPU,&datosCPU,sizeof(int)*3);

	bool todaviaHayTrabajo = true;
	void * stream;
	int accionCPU;

	printf("[Rutina rutinaCPU] - Entramos al hilo de la CPU: %d!\n", socketCPU);

	//*** Voy a trabajar con esta CPU hasta que se deconecte
	while(todaviaHayTrabajo){

		//*** Recibo la accion por parte de la CPU
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			//*** La CPU me pide un PCB para poder trabajar
			case pedirPCB:{

				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);


				PCB_DATA* pcb = cpu_pedirPCBDeExec();

				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketCPU,envioPCB,pcbSerializado,tamanoPCB(pcb) + 4);

			}break;
			case enviarPCBaTerminado:{
				//TE MANDO UN PCB QUE YA TERMINE DE EJECUTAR POR COMPLETO, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO

				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU termino la ejecucion de un proceso: accion- %d!\n", enviarPCBaTerminado);

				PCB_DATA* pcb = deserializarPCB(stream);
				imprimirPCB(pcb);

				pcb->exitCode = 0;

				modificarPCB(pcb);


				//queue_pop(cola_Exec);
				//queue_push(cola_Finished, pcb);

				//proceso_GestorDeColaExec();

				//planificadorExtraLargoPlazo();
			}break;
			case enviarPCBaReady:{
				//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO

			}break;
			case mensajeParaEscribir:{
				//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:
				//IF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE
				//ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			}break;
			case waitSemaforo:{
				//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN WAIT, ME DEBERIAS DECIR SI ME BLOQUEO O NO
			}break;
			case signalSemaforo:{
				//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN SIGNAL, LE DEBERIAS INFORMAR A ALGUIEN SI ESTABA BLOQUEADO EN UN WAIT DE ESTE SEMAFORO
			}break;
			case asignarValorCompartida:{
				//TE MANDO UNA ESTRUCTURA CON {VALOR, NOMBRE_VARIABLE(CHAR*)} PARA QUE LE ASIGNES ESE VALOR A DICHA VARIABLE
			}break;
			case pedirValorCompartida:{
				//TE MANDO EL NOMBRE DE UNA VARIABLE COMPARTIDA Y ME DEBERIAS DEVOLVER SU VALOR
			}break;
			case 0:{
				printf("[Rutina rutinaCPU] - Desconecto la CPU N°: %d\n", socketCPU);
				todaviaHayTrabajo=false;
			}break;
			default:{
				printf("[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada: %d se cerrara el socket\n",accionCPU);
				todaviaHayTrabajo=false;
			}break;
		}
	}

	close(socketCPU);
}
