/*
 * funcionesCPU.c
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#include "funcionesCPU.h"

t_mensajeDeProceso deserializarMensajeAEscribir(void* stream);

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

	PCB_DATA* pcb;

	printf("[Rutina rutinaCPU] - Entramos al hilo de la CPU cuyo socket es: %d.\n", socketCPU);

	//*** Voy a trabajar con esta CPU hasta que se deconecte
	while(todaviaHayTrabajo){

		//*** Recibo la accion por parte de la CPU
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			//*** La CPU me pide un PCB para poder trabajar
			case pedirPCB:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);

				pcb = cpu_pedirPCBDeExec();

				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketCPU,envioPCB,pcbSerializado,tamanoPCB(pcb) + 4);

			}break;

			//TE MANDO UN PCB QUE YA TERMINE DE EJECUTAR POR COMPLETO, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaTerminado:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU termino la ejecucion de un proceso: accion- %d!\n", enviarPCBaTerminado);

				pcb = deserializarPCB(stream);
				pcb->exitCode = 0;
				modificarPCB(pcb);

				int* socket = malloc(sizeof(int));

				*socket = socketCPU;

				sem_wait(&mutex_cola_CPUs_libres);
					queue_push(cola_CPUs_libres,socket);
				sem_post(&mutex_cola_CPUs_libres);

			}break;

			//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaReady:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU se quedo sin quamtum y el proceso pasa a ready: accion- %d!\n", enviarPCBaReady);

				pcb = deserializarPCB(stream);
				modificarPCB(pcb);

				queue_pop(cola_Exec);
				queue_push(cola_Ready, pcb);
			}break;

			//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:  iF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE ; ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			case mensajeParaEscribir:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU me mande a imprimir algo a la consola: accion- %d!\n", mensajeParaEscribir);

				t_mensajeDeProceso msj = deserializarMensajeAEscribir(stream);

				puts(msj.mensaje);
				printf("Descriptor: %d \n",msj.descriptorArchivo);
				printf("Pid: %d \n",msj.pid);
				if(msj.descriptorArchivo == 0){
					void * stream2 = serializarMensajeAEscribir(msj,strlen(msj.mensaje)+1);
					int socketConsola = consola_buscarSocketConsola(msj.pid);
					int s =  tamanoMensajeAEscribir(strlen(msj.mensaje)+1);
					printf("Lo que envia %d \n",s);

					printf("%d \n",socketConsola);
					enviarMensaje(socketConsola,imprimirPorPantalla,stream2,s);
				}
				else{
					//evniar algo a filesystem
				}

			}break;

			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN WAIT, ME DEBERIAS DECIR SI ME BLOQUEO O NO
			case waitSemaforo:{

				char* etiquetaSemaforo = leerString(stream);
/*
				int i;
				for(i=0; i < getArraySize("SEM_IDS"); i++){

					char* sem= getConfigStringArrayElement("SEM_IDS", i);
					//**** me fijo cual es el semaforo
					if( strcmp(sem, etiquetaSemaforo) == 0 ){

						//**** si entro aca, significa que encontre el semaforo y lo decrementeo
						setConfigIntArrayElement("SEM_INIT", i, getConfigIntArrayElement("SEM_INIT", i)-1 );

						if( getConfigIntArrayElement("SEM_INIT", i) < 0)
						{
							pcb->exitCode= bloqueado;
							queue_push(cola_Wait, pcb);
						}
					}
				}*/

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

