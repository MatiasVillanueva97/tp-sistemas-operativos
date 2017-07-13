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


bool cpu_hayCPUDisponible(){
	bool condicion(t_CPU* CPU){
		return CPU->esperaTrabajo;
	}
	return list_any_satisfy(lista_CPUS,condicion);
}

void cpu_asignarPCBACPU(PCB_DATA* PCB){
	bool condicion(t_CPU* CPU){
		return CPU->esperaTrabajo;
	}
	t_CPU* CPU = list_find(lista_CPUS, condicion);
	CPU->pcbQueSeLlevo = PCB;

	void* pcbSerializado = serializarPCB(PCB);
	enviarMensaje(CPU->socketCPU,envioPCB,pcbSerializado,tamanoPCB(PCB));
	free(pcbSerializado);

}

t_CPU* cpu_buscarCPU(int socketCPU){
	bool busqueda(t_CPU* nodo){
			return nodo->socketCPU == socketCPU;
	}
	return list_find(lista_CPUS, busqueda);
}

bool cpu_laCpuExiste(int socketCPU){

	sem_wait(&mutex_cola_CPUs_libres);
		t_CPU* CPU =  cpu_buscarCPU(socketCPU);
	sem_post(&mutex_cola_CPUs_libres);
	return CPU != NULL;
}

//Esta funcion sirve para decirle a la lista de CPUs si la cpu esta disponible o no
void cpu_laCpuEsperaTrabajo(int socketCPU, bool valor){
	sem_wait(&mutex_cola_CPUs_libres);
		t_CPU* CPU =  cpu_buscarCPU(socketCPU);
		CPU->esperaTrabajo = valor;
	sem_post(&mutex_cola_CPUs_libres);

}

void cpu_BorradorDeCPUs(t_CPU* CPU){
	if(CPU->pcbQueSeLlevo != NULL){
		CPU->pcbQueSeLlevo = paraEjecutar;
	}
	free(CPU);
}

void cpu_quitarDeLista(int socketCPU){



	bool busqueda(t_CPU* nodo){
		return nodo->socketCPU == socketCPU;
	}


	sem_wait(&mutex_cola_CPUs_libres);
		list_remove_and_destroy_by_condition(lista_CPUS,busqueda,cpu_BorradorDeCPUs);
	sem_post(&mutex_cola_CPUs_libres);
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


	sem_wait(&mutex_cola_CPUs_libres);
			t_CPU* estaCPU = cpu_buscarCPU(socketCPU);
	sem_post(&mutex_cola_CPUs_libres);


	//*** Voy a trabajar con esta CPU hasta que se deconecte
	while(todaviaHayTrabajo){

		//*** Recibo la accion por parte de la CPU
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			//*** La CPU me pide un PCB para poder trabajar
			case pedirPCB:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);

				cpu_laCpuEsperaTrabajo(socketCPU, true);


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
				if(estaCPU->pcbQueSeLlevo->estadoDeProceso != finalizado){
					if(pcb->exitCode<0){
						finalizarPid(pcb,pcb->exitCode);
					}
					else{
						finalizarPid(pcb,0);
					}
				    modificarPCB(pcb);
				   /* sem_wait(&mutex_cola_Finished);
				    	proceso_moverProcesoDeExecA(pcb->pid, cola_Finished);
				    sem_post(&mutex_cola_Finished);*/
				}


				estaCPU->esperaTrabajo = true;
				estaCPU->pcbQueSeLlevo = NULL;

				free(stream);

			}break;

			//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaReady:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU se quedo sin quamtum y el proceso pasa a ready: accion- %d!\n", enviarPCBaReady);
				pcb = deserializarPCB(stream);


				//if(estaCPU->pcbQueSeLlevo != NULL){ ////ESTE IF ES UN PARCHE. CON ROUND ROBIN LA CPU MANDA 2 VECES EL PCB A TERMINADO
					if(estaCPU->pcbQueSeLlevo->estadoDeProceso != finalizado){
						pcb->estadoDeProceso = paraEjecutar;
						modificarPCB(pcb);

						/*sem_wait(&mutex_cola_Ready);
							proceso_moverProcesoDeExecA(pcb->pid, cola_Ready);
						sem_post(&mutex_cola_Ready);*/
					}
				//}

				estaCPU->esperaTrabajo = true;
				estaCPU->pcbQueSeLlevo = NULL;

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

				cpu_quitarDeLista(socketCPU);
				printf("\n\nLa CPU de socket: %d se ha desconectado\n\n", socketCPU);
				todaviaHayTrabajo=false;

			}break;

			//QUE PASA CUANDO SE MUERTE LA CPU
			default:{
				log_info(logKernel,"[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada: %d se cerrara el socket\n",accionCPU);
				cpu_quitarDeLista(socketCPU);
				printf("\n\nLa CPU de socket: %d se ha desconectado\n\n", socketCPU);
				todaviaHayTrabajo=false;
			}break;
		}

	}

	close(socketCPU);
}

