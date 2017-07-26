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


void cpu_crearHiloDetach(int nuevoSocket){
	pthread_attr_t attr;
	pthread_t hilo_rutinaCPU ;

	//Hilos detachables cpn manejo de errores tienen que ser logs
	int  res;
	res = pthread_attr_init(&attr);
	if (res != 0) {
		log_info(logKernel,"Error en los atributos del hilo");
	}

	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (res != 0) {
		log_info(logKernel,"Error en el seteado del estado de detached");
	}

	res = pthread_create (&hilo_rutinaCPU ,&attr,rutinaCPU, (void *)nuevoSocket);
	if (res != 0) {
		log_info(logKernel,"Error en la creacion del hilo");
	}

	pthread_attr_destroy(&attr);
}


//***Esa funcion devuelve un un PCB que este listo para mandarse a ejecutar , en caso de que ninguno este listo retorna null
PCB_DATA * cpu_pedirPCBDeExec(){
	bool condicion(PCB_DATA* pcb){
		return pcb->estadoDeProceso == exec;
	}
	return list_find(cola_Exec->elements, condicion);
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

	//*** Voy a trabajar con esta CPU hasta que se deconecte
	while(todaviaHayTrabajo){

		//*** Recibo la accion por parte de la CPU
		accionCPU = recibirMensaje(socketCPU,&stream);

		switch(accionCPU){
			//*** La CPU me pide un PCB para poder trabajar
			case pedirPCB:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);
				pcb = NULL;
				while(pcb == NULL){
				sem_post(&cpuDisponible);
					sem_wait(&cantidadDeProgramasEnExec);
					pcb = cpu_pedirPCBDeExec();

				}

					sem_wait(&mutex_listaProcesos);

				if(pcb == NULL){
					//Segun santi, aca va un while asqueroso
					printf("Se rompio muy feo en rutina CPU, el pcb aparecio como NULL, se cierra el Kernel");
					exit(-1);
				}

				pcb->estadoDeProceso = enCPU;

				void* pcbSerializado = serializarPCB(pcb);


				enviarMensaje(socketCPU,envioPCB,pcbSerializado,tamanoPCB(pcb));

				free(stream);

				if(recibirMensaje(socketCPU, &stream)!= respuestaBooleanaKernel){
					moverA(pcb->pid,aReady);
					sem_post(&cantidadDeProgramasEnReady);
				}else{
					free(stream);
				}
				free(pcbSerializado);

				sem_post(&mutex_listaProcesos);
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

				printf("CPU - Se manda a finalizar este pid: %d\n", pcb->pid);

				sem_wait(&mutex_listaProcesos);
				if(!proceso_EstaFinalizado(pcb->pid)){
					PROCESOS* proceso = buscarProceso(pcb->pid);
					proceso->pcb->estadoDeProceso = exec;
					if(pcb->exitCode<0){
						proceso_Finalizar(pcb->pid, pcb->exitCode);
					}
					else{
						printf("CPU - salio todo bien pid:%d\n", pcb->pid);
						proceso_Finalizar(pcb->pid, pcb->exitCode);

					}

				    modificarPCB(pcb);
				}
				else{
					destruirPCB_Puntero(pcb);
				}
				sem_post(&mutex_listaProcesos);

				free(stream);

			}break;

			//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaReady:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU se quedo sin quamtum y el proceso pasa a ready: accion- %d!\n", enviarPCBaReady);
				pcb = deserializarPCB(stream);

				sem_wait(&mutex_listaProcesos);
				PROCESOS* proceso = buscarProceso(pcb->pid);
				if(proceso->pcb->estadoDeProceso != aFinalizar){
					pcb = modificarPCB(pcb);

					moverA(pcb->pid, aReady);
					sem_post(&cantidadDeProgramasEnReady);
				}
				else{
					proceso->pcb->estadoDeProceso = exec;
					proceso_Finalizar(pcb->pid,finalizacionDesdeConsola);
					destruirPCB_Puntero(pcb);
				}

				sem_post(&mutex_listaProcesos);

				free(stream);
			}break;

			//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:  iF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE ; ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			case mensajeParaEscribir:{
				//printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU me mande a imprimir algo a la consola: accion- %d!\n", mensajeParaEscribir);

				t_mensajeDeProceso msj = deserializarMensajeAEscribir(stream);

					int tamanoDelBuffer =  msj.tamanio + sizeof(int)*3;
					int respuestaACPU = false;

				//***Si el fileDescriptro es 1, se imprime por consola
				if(msj.descriptorArchivo == 1){
					void * stream2 = serializarMensajeAEscribir(msj,msj.tamanio);
					int socketConsola = consola_buscarSocketConsola(msj.pid);

					enviarMensaje(socketConsola,imprimirPorPantalla,stream2,tamanoDelBuffer);

					respuestaACPU = true;
					free(stream2);
					enviarMensaje(socketCPU,respuestaBooleanaKernel,&respuestaACPU,sizeof(int));

				}
				else{
					respuestaACPU = escribirEnUnArchivo(msj);
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
			//	printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide wait de un semaforo: accion- %d!\n", waitSemaforo);
				bool respuestaParaCPU = false;
				char* nombreSemaforo;

				PCB_DATA* pcbRecibido = deserializarPCBYSemaforo(stream, &nombreSemaforo);

				//Validar que el proceso no haya sido finalizado, responder siempre a la CPU

				sem_wait(&mutex_listaProcesos);
				PROCESOS* proceso = buscarProceso(pcbRecibido->pid);
				if(proceso->pcb->estadoDeProceso == aFinalizar ){
					proceso_Finalizar(proceso->pcb->pid,proceso->pcb->exitCode);
				}else{
					PCB_DATA* pcbDelProcesoActual = modificarPCB(pcbRecibido);
					sem_wait(&mutex_semaforos_ANSISOP);
					respuestaParaCPU = (proceso->pcb->estadoDeProceso == finish) ? false : SEM_wait(nombreSemaforo, pcbDelProcesoActual);
					sem_post(&mutex_semaforos_ANSISOP);
				}

				sem_post(&mutex_listaProcesos);

				free(nombreSemaforo);

				enviarMensaje(socketCPU,respuestaBooleanaKernel, &respuestaParaCPU, sizeof(bool));
			}break;

			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN SIGNAL, LE DEBERIAS INFORMAR A ALGUIEN SI ESTABA BLOQUEADO EN UN WAIT DE ESTE SEMAFORO
			case signalSemaforo:{

				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide signal de un semaforo: accion- %d!\n", signalSemaforo);



				char* nombreSemaforo;
				PCB_DATA* pcbRecibido = deserializarPCBYSemaforo(stream, &nombreSemaforo);

				sem_wait(&mutex_listaProcesos);
				PCB_DATA* pcbDelProcesoActual = modificarPCB(pcbRecibido);

				sem_wait(&mutex_semaforos_ANSISOP);
					bool respuestaParaCPU = SEM_signal(nombreSemaforo, pcbDelProcesoActual);
				sem_post(&mutex_semaforos_ANSISOP);

				sem_post(&mutex_listaProcesos);

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
					int cosaRandom = 12345;
					enviarMensaje(socketCPU,noExisteVarCompartida,&cosaRandom,sizeof(int));
				}else{
					enviarMensaje(socketCPU,envioValorCompartida,&(varGlob->valor),sizeof(int));
					free(stream);
					if(recibirMensaje(socketCPU,&stream) == asignarValorCompartida){
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

				if(varGlob == NULL)
					enviarMensaje(socketCPU,noExisteVarCompartida,NULL,0);
				else
					enviarMensaje(socketCPU,envioValorCompartida,&(varGlob->valor),sizeof(int));
				

				sem_post(&mutex_variables_compartidas);
				free(stream);

			}break;

			case reservarVariable:{
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide reservar una variable compartida: accion- %d!\n", reservarVariable);


				int tamano = *(int*) stream;
				int pid =  *(int*) (stream+4);
				free(stream);
				sem_wait(&mutex_tablaDeHeap);
				int offset = manejarPedidoDeMemoria(pid,tamano);
				sem_post(&mutex_tablaDeHeap);
				if(offset == 0){
					PCB_DATA* pcb = buscarPCB(pid);//ESTA LINEA NO HACE FALTA
					//ESTA VALIDACION ES AL PEDO, LA CPU NUNCA TE VA A MANDAR UN PID QUE NO EXISTE
					if(pcb != NULL){
						finalizarPid(pid,-9);
					}
					else{
						liberarRecursosHeap(pid);
					}
				}
				if(offset == -1){
					PCB_DATA* pcb = buscarPCB(pid);//ESTA LINEA NO HACE FALTA
					offset= 0;

					//ESTA VALIDACION ES AL PEDO, LA CPU NUNCA TE VA A MANDAR UN PID QUE NO EXISTE
					if(pcb != NULL){
						finalizarPid(pid,-8);
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
				log_info(logKernel,"[Rutina rutinaCPU] - Entramos al Caso de que CPU pide liberar una variable compartida: accion- %d!\n", liberarVariable);

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
						finalizarPid(pid,-7);
					}
					else{
						liberarRecursosHeap(pid);
					}
				}
				enviarMensaje(socketCPU,enviarSiSePudoLiberar,&x,sizeof(int));
			}break;

			//QUE PASA SI SE DESCONECTA LA CPU
			case 0:{
				log_info(logKernel,"[Rutina rutinaCPU] -Se desconecto la CPU de socket: %d\n", socketCPU);
				todaviaHayTrabajo=false;

			}break;

			//QUE PASA CUANDO SE MUERTE LA CPU
			default:{
				log_info(logKernel,"[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada: %d se cerrara el socket\n",accionCPU);
				todaviaHayTrabajo=false;

			}break;
		}

	}

	//close(socketCPU);
}

