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
				pcb->exitCode = finalizadoCorrectamente;
				pcb->estadoDeProceso = finalizado;
				modificarPCB(pcb);

				int* socket = malloc(sizeof(int));

				*socket = socketCPU;

				sem_wait(&mutex_cola_CPUs_libres);
						estaCPU->esperaTrabajo = true;
				sem_post(&mutex_cola_CPUs_libres);

			}break;

			//TE MANDO UN PCB QUE TERMINA PORQUE SE QUEDO SIN QUANTUM, ARREGLATE LAS COSAS DE MOVER DE UNA COLA A LA OTRA Y ESO
			case enviarPCBaReady:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU se quedo sin quamtum y el proceso pasa a ready: accion- %d!\n", enviarPCBaReady);

				pcb = deserializarPCB(stream);
				modificarPCB(pcb);

				queue_pop(cola_Exec);
				queue_push(cola_Ready, pcb);


				/// Revisar esto - y poner semaforos
			}break;

			//TE MANDO UNA ESTRUCTURA CON {PID, DESCRIPTOR, MENSAJE(CHAR*)} PARA QUE:  iF(DESCRIPTOR == 1) ESCRIBE EN LA CONSOLA QUE LE CORRESPONDE ; ELSE ESCRIBE EN EL ARCHIVO ASOCIADO A ESE DESCRIPTOR
			case mensajeParaEscribir:{
				printf("[Rutina rutinaCPU] - Entramos al Caso de que CPU me mande a imprimir algo a la consola: accion- %d!\n", mensajeParaEscribir);

				t_mensajeDeProceso msj = deserializarMensajeAEscribir(stream);


				//***Si el fileDescriptro es 1, se imprime por consola
				if(msj.descriptorArchivo == 1){
					void * stream2 = serializarMensajeAEscribir(msj,strlen(msj.mensaje)+1);
					int socketConsola = consola_buscarSocketConsola(msj.pid);
					int s =  tamanoMensajeAEscribir(strlen(msj.mensaje)+1);

					enviarMensaje(socketConsola,imprimirPorPantalla,stream2,s);
				}
				else{
					//*** Si es otro valor trabajar aca

					/// validar que ese proceso (el pid dentro de esta estructura) tenga los permisos
					//evniar algo a filesystem
				}
			}break;

			case abrirArchivo: {// crear archivo PD : NO PROBAR TODAVIA PORQUE NO ANDA, OK?
				//HAY QUE INICIALIZAR LAS TABLAS EN EL KERNEL
						t_crearArchivo estructura = deserializarCrearArchivo(stream);//TODO: Deserializar esta cosa
						 if(archivoExiste(estructura.path)){
							ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS auxiliar;
							int i = 0;
							bool sonDeIgualPath(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elementos){
									i++;
									return  elementos->path == estructura.path;
							}
							auxiliar = list_find(tablaGlobalDeArchivos,(void *)sonDeIgualPath);
							auxiliar->cantidad_aperturas ++;

							ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);
							nuevaEntradaTablaDeProceso(i,estructura.flags,aux->tablaProceso);
							enviarMensaje(socketCPU,envioDelFileDescriptor,list_size(aux->tablaProceso),sizeof(int));

						 }else if (string_contains(estructura.flags,"c")){
								enviarMensaje(socketFS,creacionDeArchivo,estructura.path,strlen(estructura.path)+1);
								void* stream2;
								recibirMensaje(socketFS,stream2);
								bool rta = (bool*)stream2;
								if (rta){
										ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS nuevaEntrada = {
												.path = estructura.path,
												.cantidad_de_aperturas = 1,
										};

										list_add(tablaGlobalDeArchivos,nuevaEntrada);
										ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);

										nuevaEntradaTablaDeProceso(list_size(tablaGlobalDeArchivos),estructura.flags,aux->tablaProceso);

										enviarMensaje(socketCPU,envioDelFileDescriptor,list_size(aux->tablaProceso),sizeof(int));
								}
						 }
						//else{EXITCODE}
					 }
						break;

					case leerArchivo: // leer PD : NO PROBAR TODAVIA PORQUE NO ANDA, OK?
					{
						t_archivo estructura = deserializart_Archivo(stream);

						ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);

						 ENTRADA_DE_TABLA_DE_PROCESO entrada_a_evaluar= list_get(aux->tablaProceso,estructura.fileDescriptor);
						if (string_contains(entrada_a_evaluar.flag,"r")){
							ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS entrada_de_archivo= list_get(tablaGlobalDeArchivos,entrada_a_evaluar.globalFD);
							void* pedido = serializarPedidoFS(strlen(entrada_de_archivo.path)+1, 0,entrada_de_archivo.path);//Patos, basicamente
							enviarMensaje(socketFS,obtenerDatosDeArchivo,(void *) pedido,strlen(entrada_de_archivo.path)+1 );
							void* contenido;

							if(recibirMensaje(socketFS,contenido) != respuestaBooleanaDeFs){
							//enviarMensaje(socketCPU,10,void * contenido,);algo
							}

						}

					}break;

			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN WAIT, ME DEBERIAS DECIR SI ME BLOQUEO O NO
			case waitSemaforo:{

				char* nombreSemaforo = leerString(stream);

				//***Verifico si existe el semaforo que cpu me mando
				if(sema_existeSemaforo(nombreSemaforo)){

					//***Si el semaforo existe realizo el wait para este semaforo
					sema_proceso_wait(nombreSemaforo);

					//***Le aviso a la cpu que la acabamos de bloquear
//					enviarMensaje(socketCPU,bloquearProceso,&accionCPU,sizeof(int)); // agregar al enum bloquearProceso . le paso accion cpu para mandarle algo

					//***Recibo el pcb que voy a bloquear
					recibirMensaje(socketCPU, &stream); // puedo reciclar el stream?
					pcb = deserializarPCB(stream);

					//***Cambio el estado de este pcb a bloqueado
					pcb->estadoDeProceso = bloqueado;

					//***Agrego a la lista de espera que el pid que acabo de recibir esta esperando por el semaforo que se me habia indicado
					//hacer semaforo para esta lista

					esperaDeSemaforo* esp_sem=malloc(sizeof(esperaDeSemaforo));

					esp_sem->pid=pcb->pid;
					esp_sem->sem=nombreSemaforo;

					list_add(listaDeEsperaSemaforos,esp_sem);
				}
				else{
					//***Sino existe le digo que me esta pidiendo cualqueir cosa

//					enviarMensaje(socketCPU,errorSemaforo,&accionCPU,sizeof(int)); // agregar al enum errorsemafor . le paso accion cpu para mandarle algo
				}


			}break;

			//TE MANDO UN NOMBRE DE UN SEMAFORO Y QUIERO QUE HAGAS UN SIGNAL, LE DEBERIAS INFORMAR A ALGUIEN SI ESTABA BLOQUEADO EN UN WAIT DE ESTE SEMAFORO
			case signalSemaforo:{
				char* nombreSemaforo = leerString(stream);

				//***Verifico si existe el semaforo que cpu me mando
				if(sema_existeSemaforo(nombreSemaforo)){

					//***Si el semaforo existe realizo el signal para este semaforo
					sema_proceso_signal(nombreSemaforo);

					//***Quito de la lista de espera que el pid que acabo de recibir esta esperando por el semaforo que se me habia indicado
					//hacer semaforo para esta lista

				}
				else{
					//***Sino existe le digo que me esta pidiendo cualqueir cosa

//					enviarMensaje(socketCPU,errorSemaforo,&accionCPU,sizeof(int)); // agregar al enum errorsemafor . le paso accion cpu para mandarle algo
				}

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

