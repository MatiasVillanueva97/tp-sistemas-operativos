/*
 * funcionesCPU.c
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#include "funcionesCPU.h"

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
bool archivoExiste(char* path){

	bool sonDeIgualPath(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elementos){
								return  elementos->path == path;
	}
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* auxiliar;
	auxiliar = list_find(tablaGlobalDeArchivos,(void *)sonDeIgualPath);
	if(auxiliar == NULL){
			return false;}
	else{return true; }
}

ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * encontrarElDeIgualPid(int pid){
			ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux;
			bool sonDeIgualPid(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * elementos){
				return  elementos->pid == pid;
				}
			aux = list_find(tablaGlobalDeArchivosDeProcesos,(void *)sonDeIgualPid);
						return aux;
}

void agregarATablaDeProceso(int df, char* flags, t_list* tablaProceso){
ENTRADA_DE_TABLA_DE_PROCESO * nuevaEntradaProceso = malloc(sizeof(ENTRADA_DE_TABLA_DE_PROCESO));
nuevaEntradaProceso->globalFD = df;
nuevaEntradaProceso->flags = string_duplicate(flags);
list_add(tablaProceso, nuevaEntradaProceso);
}

void* serializarPedidoFs(int size, int offset,char* path){
	void *contenido = malloc(4+strlen(path)+sizeof(int)*2);
	memcpy(contenido,&size,sizeof(int));
	memcpy(contenido+sizeof(int),&offset,sizeof(int));
	int tamanoRuta = strlen(path);
	memcpy(contenido+sizeof(int)*2,&tamanoRuta,sizeof(int));
	memcpy(contenido+sizeof(int)*3,path,strlen(path));
	return contenido;
}
//Espero que esto ande
t_crearArchivo deserializarCrearArchivo(void* stream){
	t_crearArchivo mensaje;
		memcpy(&mensaje.pid,stream,sizeof(int));
		int tamanoContenido;
		memcpy(&tamanoContenido,stream + sizeof(int), sizeof(int));
		char* contenidoAuxiliar = malloc(tamanoContenido);
		memcpy(contenidoAuxiliar,stream + sizeof(int) * 2, tamanoContenido);
		mensaje.flags = contenidoAuxiliar;
		int tamanoContenido2;
		memcpy(&tamanoContenido2,stream + sizeof(int)*3 + tamanoContenido, sizeof(int));
		char* contenidoAuxiliar2 = malloc(tamanoContenido2);
		memcpy(contenidoAuxiliar2,stream + sizeof(int) * 4 +tamanoContenido, tamanoContenido2);
		mensaje.path = contenidoAuxiliar2;
		return mensaje;
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

	printf("[Rutina rutinaCPU] - Entramos al hilo de la CPU cuyo socket es: %d.\n", socketCPU);

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
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU pide un pcb: accion- %d!\n", pedirPCB);

				pcb = cpu_pedirPCBDeExec();

				void* pcbSerializado = serializarPCB(pcb);
				enviarMensaje(socketCPU,envioPCB,pcbSerializado,tamanoPCB(pcb));

			}break;

			//TE MANDO UN PCB QUE YA TERMINE DE EJECUTAR POR COMPLETO, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaTerminado:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU termino la ejecucion de un proceso: accion- %d!\n", enviarPCBaTerminado);

				pcb = deserializarPCB(stream);

				// aca como que deberiamos validar que no haya sido finalizado ya este procesito
				//ACA HAY QUE CAMBIAR ESTO
				if(pcb->exitCode != excepcionMemoria && !proceso_EstaFinalizado(pcb->pid))
				{
					pcb->exitCode = finalizadoCorrectamente;
				}
				pcb->estadoDeProceso = finalizado;
				if(!proceso_EstaFinalizado(pcb->pid)){
					modificarPCB(pcb);
				}
				sem_wait(&mutex_cola_CPUs_libres);
						estaCPU->esperaTrabajo = true;
				sem_post(&mutex_cola_CPUs_libres);

			}break;

			//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaReady:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU se quedo sin quamtum y el proceso pasa a ready: accion- %d!\n", enviarPCBaReady);

				pcb = deserializarPCB(stream);
				modificarPCB(pcb);

				sem_wait(&mutex_cola_Exec);
					queue_pop(cola_Exec);
				sem_post(&mutex_cola_Exec);

				sem_wait(&mutex_cola_Ready);
					queue_push(cola_Ready, pcb);
				sem_post(&mutex_cola_Ready);

				/// Revisar esto - y poner semaforos
			}break;

			//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:  iF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE ; ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			case mensajeParaEscribir:{
				//printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU me mande a imprimir algo a la consola: accion- %d!\n", mensajeParaEscribir);

				t_mensajeDeProceso msj = deserializarMensajeAEscribir(stream);

					int tamanoDelBuffer =  tamanoMensajeAEscribir(strlen(msj.mensaje)+1);
					PCB_DATA* pcbaux;
					pcbaux = buscarPCB(msj.pid);
					bool respuestaACPU = false;

				//***Si el fileDescriptro es 1, se imprime por consola
				if(msj.descriptorArchivo == 1){
					void * stream2 = serializarMensajeAEscribir(msj,strlen(msj.mensaje)+1);
					int socketConsola = consola_buscarSocketConsola(msj.pid);

					enviarMensaje(socketConsola,imprimirPorPantalla,stream2,tamanoDelBuffer);
				}
				else{
					ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(msj.pid);

					ENTRADA_DE_TABLA_DE_PROCESO *entrada_a_evaluar= list_get(aux->tablaProceso,msj.descriptorArchivo);

					ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo= list_get(tablaGlobalDeArchivos,entrada_a_evaluar->globalFD);
					if (entrada_de_archivo != NULL){
						if (string_contains(entrada_a_evaluar->flags,"r")){
							int offset = 0;//DEBE CAMBIAR.QUE ES UN CURSOR?
							//void* pedidoEscritura = serializarPedidoDeEscritura(entrada_de_archivo->path,offset,tamanoDelBuffer,msj.mensaje);
							//enviarMensaje(socketFS,guardarDatosDeArchivo,(void *) pedidoEscritura,tamanoDelBuffer);
							//enviarPaquete(socketFS,2,2,offset,tamanoDelBuffer,entrada_de_archivo->path,msj.mensaje);
							void* contenido;
							recibirMensaje(socketFS,contenido);
							int respuestaFS = leerInt(contenido);
							respuestaACPU = respuestaFS;
							if(!respuestaFS){
								pcbaux->exitCode = -1;
								pcbaux->estadoDeProceso = finalizado;
							}
						}else{
							pcbaux->exitCode = -3;
							pcbaux->estadoDeProceso = finalizado;
						}
					}else{
						pcbaux->exitCode = -2;
						pcbaux->estadoDeProceso = finalizado;
					}
					//enviarMensaje(socketCPU,respuestaEscritura,respuestaACPU,sizeof(bool));
				}
			}break;

		case abrirArchivo: {// crear archivo PD : NO PROBAR TODAVIA PORQUE NO ANDA, OK?
					t_crearArchivo estructura = deserializarCrearArchivo(stream);
					enviarMensaje(socketFS,validacionDerArchivo,estructura.path,sizeof(int));
					void * stream2;
					recibirMensaje(socketFS,stream2);
					int existeArchivo = leerInt(stream2);
					 if(existeArchivo){
						ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * auxiliar;
						int i = 0;
						bool sonDeIgualPath(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elementos){
								i++;
								return  elementos->path == estructura.path;
						}
						auxiliar = list_find(tablaGlobalDeArchivos,(void *)sonDeIgualPath);
						auxiliar->cantidad_aperturas++;

						ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);
						agregarATablaDeProceso(i,estructura.flags,aux->tablaProceso);
						enviarMensaje(socketCPU,envioDelFileDescriptor,list_size(aux->tablaProceso),sizeof(int));

						}else if (string_contains(estructura.flags,"c")){
								enviarMensaje(socketFS,creacionDeArchivo,estructura.path,strlen(estructura.path)+1);
								void* stream2;
								recibirMensaje(socketFS,stream2);
								bool rta = (bool*)stream2;
								if (rta){
									ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * nuevaEntrada;
									nuevaEntrada->path = estructura.path;
									nuevaEntrada->cantidad_aperturas = 1;

									list_add(tablaGlobalDeArchivos,nuevaEntrada);
									ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);

									agregarATablaDeProceso(list_size(tablaGlobalDeArchivos),estructura.flags,aux->tablaProceso);

									enviarMensaje(socketCPU,envioDelFileDescriptor,list_size(aux->tablaProceso),sizeof(int));
											}
									 }
						else{
							PCB_DATA* pcbaux;
							pcbaux = buscarPCB(estructura.pid);
							pcbaux->exitCode = -2;
							pcbaux->estadoDeProceso = finalizado;
						}
			 }
			break;

			case leerArchivo:{ // leer PD : NO PROBAR TODAVIA PORQUE NO ANDA, OK?

				t_archivo estructura;
				estructura = *((t_archivo*)stream);
				PCB_DATA* pcbaux;
				pcbaux = buscarPCB(estructura.pid);
				ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);


				ENTRADA_DE_TABLA_DE_PROCESO* entrada_a_evaluar= list_get(aux->tablaProceso,estructura.fileDescriptor);

				ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo= list_get(tablaGlobalDeArchivos,entrada_a_evaluar->globalFD);
				if (entrada_de_archivo != NULL){
					if (string_contains(entrada_a_evaluar->flags,"r")){

						int tamanioDelPedido =strlen(entrada_de_archivo->path)+1 ;
						int offset = 0;//DEBE CAMBIAR.QUE ES UN CURSOR?
						void* pedidoDeLectura = serializarPedidoFs(tamanioDelPedido,offset,entrada_de_archivo->path);//Patos, basicamente
						enviarMensaje(socketFS,obtenerDatosDeArchivo,(void *) pedidoDeLectura,tamanioDelPedido);
						void* contenido;

						if(recibirMensaje(socketFS,contenido) == respuestaConContenidoDeFs){
							//enviarMensaje(socketCPU,respuestaLectura, contenido,tamanioDelPedido)
							}
						else{
								pcbaux->exitCode = -1;
								pcbaux->estadoDeProceso = finalizado;
								}
							}else{
								pcbaux->exitCode = -3;
								pcbaux->estadoDeProceso = finalizado;
							}
							}else{
							pcbaux->exitCode = -2;
							pcbaux->estadoDeProceso = finalizado;
				}
				//enviarMensaje(socketCPU,noLaburesCPU,,sizeof(bool));
			}break;


			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN WAIT, ME DEBERIAS DECIR SI ME BLOQUEO O NO
			case waitSemaforo:{

							puts("Entro al waitSemaforo\n");
							char* nombreSemaforo;

							PCB_DATA* pcbRecibido = deserializarPCBYSemaforo(stream, &nombreSemaforo);
							//Validar que el proceso no haya sido finalizado, responder siempre a la CPU si
							PCB_DATA* pcbDelProcesoActual = modificarPCB(pcbRecibido);
							bool respuestaParaCPU = SEM_wait(nombreSemaforo, pcbDelProcesoActual);
							free(nombreSemaforo);

							enviarMensaje(socketCPU,respuestaBooleanaKernel, &respuestaParaCPU, sizeof(bool));

						}break;

			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN SIGNAL, LE DEBERIAS INFORMAR A ALGUIEN SI ESTABA BLOQUEADO EN UN WAIT DE ESTE SEMAFORO
			case signalSemaforo:{
					puts("Entro al signalSemaforo\n");
					char* nombreSemaforo;
					PCB_DATA* pcbRecibido = deserializarPCBYSemaforo(stream, &nombreSemaforo);
					PCB_DATA* pcbDelProcesoActual = modificarPCB(pcbRecibido);
					bool respuestaParaCPU = SEM_signal(nombreSemaforo, pcbDelProcesoActual);
					free(nombreSemaforo);
					enviarMensaje(socketCPU,respuestaBooleanaKernel, &respuestaParaCPU, sizeof(bool));
				}break;

			//TE MANDO UNA ESTRUCTURA CON {VALOR, NOMBRE_VARIABLE(CHAR*)} PARA QUE LE ASIGNES ESE VALOR A DICHA VARIABLE
			case asignarValorCompartida:{
			}break;

			//TE MANDO EL NOMBRE DE UNA VARIABLE COMPARTIDA Y ME DEBERIAS DEVOLVER SU VALOR
			case pedirValorCompartida:{
			}break;


			//QUE PASA SI SE DESCONECTA LA CPU
			case 0:{
				printf("[Rutina rutinaCPU] - Desconecto la CPU N°: %d\n", socketCPU);
				todaviaHayTrabajo=false;

				cpu_quitarDeLista(socketCPU);

			}break;

			//QUE PASA CUANDO SE MUERTE LA CPU
			default:{
				printf("[Rutina rutinaCPU] - Se recibio una accion que no esta contemplada: %d se cerrara el socket\n",accionCPU);
				todaviaHayTrabajo=false;

				cpu_quitarDeLista(socketCPU);
			}break;
		}
	}

	close(socketCPU);
}

