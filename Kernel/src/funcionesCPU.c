/*
 * funcionesCPU.c
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#include "funcionesCPU.h"
#include "funcionesHeap.h"
#include "funcionesCapaFS.h"

t_mensajeDeProceso deserializarMensajeAEscribir(void* stream);


void cpu_quitarDeLista(socketCPU){

	bool busqueda(t_CPU* nodo)
	{
		return nodo->socketCPU == socketCPU;
	}

	list_remove_and_destroy_by_condition(lista_CPUS,busqueda,free);
}

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
		sem_wait(&mutex_cola_Exec);
		//***Me fijo la cantidad de procesos que hay en la cola de exec
		cantidadProcesos = queue_size(cola_Exec);

		//***Voy a iterar tantas veces como elementos tenga en la cola de exec
		for(i=0; i < cantidadProcesos; i++)
		{
			//***Tomo el primer pcb de la cola
			pcb = queue_pop(cola_Exec);

			//*** Valido si el pcb se puede mandar a ejecutar
			if(pcb->estadoDeProceso == paraEjecutar)
			{
				//***Esta listo para ejecutar, le cambio el exitcode
				pcb->estadoDeProceso = loEstaUsandoUnaCPU;

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
		sem_post(&mutex_cola_Exec);
	}

	return pcb;
}


bool proceso_EstaFinalizado(int pid)
{
	bool busqueda(PROCESOS * aviso){
	  return aviso->pid == pid;
	 }
	 PCB_DATA* pcb = ((PROCESOS*)list_find(avisos, busqueda))->pcb;
	 return pcb->estadoDeProceso == finalizado;
}
void agregarATablaEstadistica(int pid,int tamano,bool esAlocar){
	bool encontrarPorPid(filaEstadisticaDeHeap* fila2){
		return fila2->pid == pid;
	}
	sem_wait(&mutex_tabla_estadistica_de_heap);
	filaEstadisticaDeHeap* fila = list_find(tablaEstadisticaDeHeap,encontrarPorPid);
	if(esAlocar){
		fila->tamanoAlocadoEnBytes+=tamano;
		fila->tamanoAlocadoEnOperaciones++;
	}
	else{
		fila->tamanoLiberadoEnBytes+=tamano;
		fila->tamanoLiberadoEnOperaciones++;
	}
	sem_post(&mutex_tabla_estadistica_de_heap);

}

void agregarPedirPaginaATablaEstadistica(int pid){
	bool encontrarPorPid(filaEstadisticaDeHeap* fila2){
		return fila2->pid == pid;
	}
	sem_wait(&mutex_tabla_estadistica_de_heap);
	filaEstadisticaDeHeap* fila = list_find(tablaEstadisticaDeHeap,encontrarPorPid);
	fila->cantidadDePaginasHistoricasPedidas++;
	sem_post(&mutex_tabla_estadistica_de_heap);
}

t_CPU* cpu_buscarCPUDisponible(){
	bool busqueda(t_CPU* cpu){
		return cpu->esperaTrabajo;
	}
	return list_find(lista_CPUS,busqueda);
}


void *rutinaCPU(void * arg)
{
	int socketCPU = (int)arg;

	//*** Le envio a la CPU todos los datos qeu esta necesitara para poder trabajar, como el tamaño de una pagina de memoria, el quantum y la cantidad de paginas que ocupa un stack
	DATOS_PARA_CPU datosCPU;
	datosCPU.size_pag=size_pagina;
	datosCPU.quantum=quantumRR;
	datosCPU.size_stack=getConfigInt("STACK_SIZE");
	enviarMensaje(socketCPU,enviarDatosCPU,&datosCPU,sizeof(int)*3);

	bool todaviaHayTrabajo = true;
	void * stream;
	int accionCPU;

	PCB_DATA* pcb;

	log_info(logKernel,"[Rutina rutinaCPU] - Entramos al hilo de la CPU cuyo socket es: %d.\n", socketCPU);

	bool busqueda(t_CPU* cpu){
		return cpu->socketCPU == socketCPU;
	}

	sem_wait(&mutex_cola_CPUs_libres);
			t_CPU* estaCPU = list_find(lista_CPUS,busqueda);
	sem_post(&mutex_cola_CPUs_libres);


	//*** Voy a trabajar con esta CPU hasta que se deconecte
	while(todaviaHayTrabajo){

		//*** Recibo la accion por parte de la CPU
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			//*** La CPU me pide un PCB para poder trabajar
			case pedirPCB:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);

				/*pcb = cpu_pedirPCBDeExec();

				free(stream);

				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketCPU,envioPCB,pcbSerializado,tamanoPCB(pcb));
				free(pcbSerializado);
				*/
				estaCPU->esperaTrabajo=true;

				free(stream);

			}break;

			case dameQuantumSleep:{
				sem_wait(&mutex_Quantum_Sleep);
					enviarMensaje(socketCPU,respuestaBooleanaKernel,&quantumSleep,sizeof(int));
				sem_post(&mutex_Quantum_Sleep);

				free(stream);

			}break;

			//TE MANDO UN PCB QUE YA TERMINE DE EJECUTAR POR COMPLETO, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaTerminado:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU termino la ejecucion de un proceso: accion- %d!\n", enviarPCBaTerminado);

				pcb = deserializarPCB(stream);

				// aca como que deberiamos validar que no haya sido finalizado ya este procesito
				if(!proceso_EstaFinalizado(pcb->pid)){
					if(pcb->exitCode<0){
						finalizarPid(pcb,pcb->exitCode);
					}
					else{
						finalizarPid(pcb,0);
					}
				    modificarPCB(pcb);
				}
				sem_wait(&mutex_cola_CPUs_libres);
				   	estaCPU->esperaTrabajo = true;
					estaCPU->pcbQueSeLlevo = NULL;

				sem_post(&mutex_cola_CPUs_libres);
				free(stream);

			}break;

			//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaReady:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU se quedo sin quamtum y el proceso pasa a ready: accion- %d!\n", enviarPCBaReady);
				pcb = deserializarPCB(stream);
				if(pcb->estadoDeProceso != bloqueado){
					pcb->estadoDeProceso = moverAReady;
				}
				sem_wait(&mutex_cola_CPUs_libres);
				estaCPU->esperaTrabajo = true;
				estaCPU->pcbQueSeLlevo = NULL;
				sem_post(&mutex_cola_CPUs_libres);
				sem_wait(&mutex_cola_Exec);
				 modificarPCB(pcb);
				 sem_post(&mutex_cola_Exec);
/*

				sem_wait(&mutex_cola_Exec);
					queue_pop(cola_Exec);
				sem_post(&mutex_cola_Exec);

				sem_wait(&mutex_cola_Ready);
					queue_push(cola_Ready, modificarPCB(pcb));
				sem_post(&mutex_cola_Ready);
*/
				/// Revisar esto - y poner semaforos

				 free(stream);


			}break;

			//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:  iF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE ; ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			case mensajeParaEscribir:{
				//printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU me mande a imprimir algo a la consola: accion- %d!\n", mensajeParaEscribir);

				t_mensajeDeProceso msj = deserializarMensajeAEscribir(stream);

					int tamanoDelBuffer =  msj.tamanio + sizeof(int)*3;
					bool respuestaACPU = false;

				//***Si el fileDescriptro es 1, se imprime por consola
				if(msj.descriptorArchivo == 1){
					void * stream2 = serializarMensajeAEscribir(msj,msj.tamanio);
					int socketConsola = consola_buscarSocketConsola(msj.pid);

					enviarMensaje(socketConsola,imprimirPorPantalla,stream2,tamanoDelBuffer);

					respuestaACPU = true;
					free(stream2);
					enviarMensaje(socketCPU,respuestaBooleanaKernel,&respuestaACPU,sizeof(bool));

				}
				else{
					respuestaACPU = escribirEnUnArchivo(msj,msj.tamanio);
					enviarMensaje(socketCPU,respuestaBooleanaKernel,&respuestaACPU,sizeof(bool));
				}
				free(msj.mensaje);
				free(stream);

			}break;

		case abrirArchivo: {// No se que tan bien funcionan los deserializar y serializa
					t_crearArchivo estructura = deserializarCrearArchivo(stream);
					enviarMensaje(socketFS,validacionDerArchivo,estructura.path,strlen(estructura.path)+1);
					void * stream2;
					recibirMensaje(socketFS,&stream2);
					bool existeArchivo = *(bool*) stream2;
					abrirArchivoPermanente(existeArchivo,estructura, socketCPU);
					free(stream);
					free(stream2);
			 }
			break;

		case cerrarArchivo:{
			t_archivo estructura;
			estructura = *((t_archivo*)stream);

			int rtaCPU = cerrarArchivoPermanente(estructura);

			enviarMensaje(socketCPU,respuestaBooleanaKernel,&rtaCPU,sizeof(int));

			free(stream);


		}break;

		case borrarArchivoCPU:{
			t_archivo estructura;
			estructura = *((t_archivo*)stream);

			int rtaCPU = borrarArchivoPermanente(estructura);

			enviarMensaje(socketCPU,respuestaBooleanaKernel,&rtaCPU,sizeof(int));

			free(stream);

		}break;

		case leerArchivo:{

			t_lectura estructura;
			estructura = *((t_lectura*)stream);
			leerEnUnArchivo(estructura,socketCPU);

			free(stream);

		}break;
		case moverCursorArchivo:{
			t_moverCursor estructura;

			estructura = *((t_moverCursor*)stream);

			int rtaCPU = moverUnCursor(estructura);

			enviarMensaje(socketCPU,respuestaBooleanaKernel,&rtaCPU,sizeof(int));

			free(stream);

		}break;


			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN WAIT, ME DEBERIAS DECIR SI ME BLOQUEO O NO
			case waitSemaforo:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide wait de un semaforo: accion- %d!\n", waitSemaforo);

				puts("Entro al waitSemaforo\n");
				char* nombreSemaforo;

				PCB_DATA* pcbRecibido = deserializarPCBYSemaforo(stream, &nombreSemaforo);

				//Validar que el proceso no haya sido finalizado, responder siempre a la CPU si
				PCB_DATA* pcbDelProcesoActual = modificarPCB(pcbRecibido);

				sem_wait(&mutex_semaforos_ANSISOP);
					bool respuestaParaCPU = SEM_wait(nombreSemaforo, pcbDelProcesoActual);
				sem_post(&mutex_semaforos_ANSISOP);

				free(nombreSemaforo);

				enviarMensaje(socketCPU,respuestaBooleanaKernel, &respuestaParaCPU, sizeof(bool));
			}break;

			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN SIGNAL, LE DEBERIAS INFORMAR A ALGUIEN SI ESTABA BLOQUEADO EN UN WAIT DE ESTE SEMAFORO
			case signalSemaforo:{

				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide signal de un semaforo: accion- %d!\n", signalSemaforo);

				char* nombreSemaforo;
				PCB_DATA* pcbRecibido = deserializarPCBYSemaforo(stream, &nombreSemaforo);
				PCB_DATA* pcbDelProcesoActual = modificarPCB(pcbRecibido);

				sem_wait(&mutex_semaforos_ANSISOP);
					bool respuestaParaCPU = SEM_signal(nombreSemaforo, pcbDelProcesoActual);
				sem_post(&mutex_semaforos_ANSISOP);

				free(nombreSemaforo);
				enviarMensaje(socketCPU,respuestaBooleanaKernel, &respuestaParaCPU, sizeof(bool));
			}break;

			//TE MANDO UNA ESTRUCTURA CON {VALOR, NOMBRE_VARIABLE(CHAR*)} PARA QUE LE ASIGNES ESE VALOR A DICHA VARIABLE
			case asignarValorCompartida:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU asigna valor a una variable compartida: accion- %d!\n", asignarValorCompartida);
				char* nombreVarGlob = leerString(stream);

				sem_wait(&mutex_variables_compartidas);
				t_variableGlobal* varGlob = buscarVariableGlobal(nombreVarGlob);

				if(varGlob == NULL){
					free(stream);
					sem_post(&mutex_variables_compartidas);
					enviarMensaje(socketCPU,noExisteVarCompartida,NULL,sizeof(NULL));
				}else{
					enviarMensaje(socketCPU,envioValorCompartida,&(varGlob->valor),sizeof(int));
					free(stream);
					if(recibirMensaje(socketCPU,stream) == asignarValorCompartida){
						varGlob->valor = *((int*)stream);
					}
					sem_post(&mutex_variables_compartidas);
					free(stream);
				}

			}break;

			//TE MANDO EL NOMBRE DE UNA VARIABLE COMPARTIDA Y ME DEBERIAS DEVOLVER SU VALOR
			case pedirValorCompartida:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide el valor de una variable compartida: accion- %d!\n", pedirValorCompartida);

				char* nombreVarGlob = leerString(stream);

				sem_wait(&mutex_variables_compartidas);
				
				t_variableGlobal* varGlob = buscarVariableGlobal(nombreVarGlob);
				if(varGlob == NULL){
					enviarMensaje(socketCPU,noExisteVarCompartida,NULL,sizeof(NULL));
				}else{
					enviarMensaje(socketCPU,envioValorCompartida,&(varGlob->valor),sizeof(int));
				}
				
				sem_post(&mutex_variables_compartidas);
				free(stream);

			}break;

			case reservarVariable:{
				int tamano = *(int*) stream;
				int pid =  *(int*) (stream+4);
				free(stream);
				sem_wait(&mutex_tablaDeHeap);
				int offset = manejarPedidoDeMemoria(pid,tamano);
				sem_post(&mutex_tablaDeHeap);
				if(offset == 0){
					PCB_DATA* pcb = buscarPCB(pid);
					if(pcb != NULL){
						finalizarPid(pcb,-9);
					}
					else{
						liberarRecursosHeap(pid);
					}
				}
				if(offset == -1){
					PCB_DATA* pcb = buscarPCB(pid);
					offset= 0;
					if(pcb != NULL){
						finalizarPid(pcb,-8);
					}
					else{
						liberarRecursosHeap(pid);
					}
					enviarMensaje(socketCPU,pedidoRechazadoPorPedirMas,&offset,sizeof(int));
				}
				else{
					enviarMensaje(socketCPU,enviarOffsetDeVariableReservada,&offset,sizeof(offset)); // Negro tene cuidado. Si te tiro un 0, es que rompio. Nunca te puedo dar el 0, porque va el metadata.
					agregarATablaEstadistica(pid,tamano,true);
				}
			}break;
			case liberarVariable:{
				int offset = *(int*) stream;
				int pid = *(int*) (stream+4);
				free(stream);
				sem_wait(&mutex_tablaDeHeap);
				int x = manejarLiberacionDeHeap(pid,offset);
				sem_post(&mutex_tablaDeHeap);
				if(x== 0){
					PCB_DATA* pcb = buscarPCB(pid);
					offset= 0;
					if(pcb != NULL){
						finalizarPid(pcb,-7);
					}
					else{
						liberarRecursosHeap(pid);
					}
				}
				enviarMensaje(socketCPU,enviarSiSePudoLiberar,&x,sizeof(int));
			}break;

			//QUE PASA SI SE DESCONECTA LA CPU
			case 0:{
				log_info(logKernel,"[Rutina rutinaCPU] - Desconecto la CPU N°: %d\n", socketCPU);
				sem_wait(&mutex_cola_CPUs_libres);
				if(estaCPU->pcbQueSeLlevo!=NULL){
					log_info(logKernel,"La CPU de socket c");
					sem_wait(&mutex_cola_Ready);
					queue_push(cola_Ready, pcb);
					sem_post(&mutex_cola_Ready);

				}
				cpu_quitarDeLista(socketCPU);
				todaviaHayTrabajo=false;
				sem_post(&mutex_cola_CPUs_libres);

			}break;

			//QUE PASA CUANDO SE MUERTE LA CPU
			default:{
				log_info(logKernel,"[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada: %d se cerrara el socket\n",accionCPU);

				cpu_quitarDeLista(socketCPU);
				todaviaHayTrabajo=false;
			}break;
		}

	}

	//close(socketCPU);
}

