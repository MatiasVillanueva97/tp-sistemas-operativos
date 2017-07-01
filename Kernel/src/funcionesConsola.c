/*
 * funcionesConsola.c
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#include "funcionesConsola.h"



///-----FUNCIONES CONSOLA-------//


int consola_buscarSocketConsola(int pid){

	bool busqueda(PROCESOS * process)	{
		return process->pid==pid;
	}


	PROCESOS* proceso = list_find(avisos,busqueda);


	if(proceso != NULL){
		return proceso->socketConsola;
	}

	return -1;
}

/// *** Falta probar! Necesitamos que ande el enviar mensajes
///*** Esta funcion le avisa a la consola que un cierto proceso (pid) ya termino
void consola_enviarAvisoDeFinalizacion(int socketConsola, int pid){
	printf("[Función consola_enviarAvisoDeFinalizacion] - Se Envía a Consola el pid: %d, porque ha finalizado!\n", pid);
	enviarMensaje(socketConsola,envioDelPidEnSeco,&pid,sizeof(int));
}



/// *** Funciona
/// *** Esta funcion finalizara todos los procesos que sean de una consola que se acaba de desconectar
void consola_finalizarTodosLosProcesos(int socketConsola){

	void cambiar(PROCESOS * process){
		if(process->socketConsola==socketConsola)
		{
			process->pcb->exitCode=-6;
			process->avisoAConsola=true;

			enviarMensaje(socketMemoria,finalizarPrograma,&process->pid,sizeof(int));


			int* joaquin;
			recibirMensaje(socketMemoria,&joaquin);
			free(joaquin);

			printf("Murio el proceso: %d\n", process->pid);
		}
	}

	bool busqueda(PROCESOS * process)	{
		return process->socketConsola==socketConsola;
	}

	list_iterate(avisos, cambiar);
}


void consola_crearHiloDetach(int nuevoSocket){
	pthread_attr_t attr;
	pthread_t hilo_M ;

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

	res = pthread_create (&hilo_M ,&attr,rutinaConsola, (void *)nuevoSocket);
	if (res != 0) {
		perror("Error en la creacion del hilo");
	}

	pthread_attr_destroy(&attr);
}



/// *** A esta función hay que probarle tuodo el sistema de envio de mensajes entre consola y kernel ( falla en el recivir mensaje que esta dentro del swich, nose porque no recibe mensajes
//***Esta rutina se levanta por cada consola que se cree. Donde se va a quedar escuchandola hasta que la misma se desconecte.
void *rutinaConsola(void * arg)
{

	int socketConsola = (int)arg;
	bool todaviaHayTrabajo = true;
	void * stream;
	printf("[Rutina rutinaConsola] - Entramos al hilo de la consola: %d!\n", socketConsola);


	while(todaviaHayTrabajo){
		int a = recibirMensaje(socketConsola,&stream);

		switch(a){
			case envioScriptAnsisop:{
				//***Estoy recibiendo un script para inicializar. Creo un neuvo proceso y ya comeizno a rellenarlo con los datos que ya tengo
				printf("[Rutina rutinaConsola] - Nuevo script recibido!\n");

				char* scripAnsisop = (char *)stream;
				printf("El stream es : %s /n",scripAnsisop);

				PROCESOS * nuevoPrograma = malloc(sizeof(PROCESOS));

				sem_wait(&mutex_HistoricoPcb);
					nuevoPrograma->pid= historico_pid;
					historico_pid++;
				sem_post(&mutex_HistoricoPcb);

				nuevoPrograma->scriptAnsisop = scripAnsisop;
				nuevoPrograma->socketConsola = socketConsola;
				nuevoPrograma->avisoAConsola = false;

				//***Creo el PCB
				PCB_DATA * pcbNuevo = crearPCB(nuevoPrograma->scriptAnsisop, nuevoPrograma->pid, 0);
				nuevoPrograma->pcb = pcbNuevo;
				//**Le doy una tabla para sus archivos abiertos


				ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * nuevaEntrada = malloc(sizeof(int)+4);
				nuevaEntrada->pid = nuevoPrograma->pid;
				nuevaEntrada->tablaProceso = list_create();

				agregarATablaDeProceso(0," ",nuevaEntrada->tablaProceso);
				agregarATablaDeProceso(0," ",nuevaEntrada->tablaProceso);
				agregarATablaDeProceso(0," ",nuevaEntrada->tablaProceso);

				list_add(tablaGlobalDeArchivosDeProcesos,nuevaEntrada);

				//***Lo Agrego a la Cola de New
				sem_wait(&mutex_cola_New);
					queue_push(cola_New,nuevoPrograma);
				sem_post(&mutex_cola_New);

				//***Le envio a consola el pid del script que me acaba de enviar
				enviarMensaje(socketConsola,envioDelPidEnSeco,&nuevoPrograma->pid,sizeof(int));

				sem_wait(&mutex_listaProcesos);
					list_add(avisos,nuevoPrograma);
				sem_post(&mutex_listaProcesos);

				/* Cuando un consola envia un pid para finalizar, lo que vamos a hacer es una funci+on que cambie el estado de ese proceso a finalizado,
				 * de modo que en el mommento en que un proceso pase de cola en cola se valide como esta su estado, de estar en finalizado externamente
				 *  se pasa automaticamente ala cola de finalizados ---
				 *  Asi que se elimina la estructura de avisos de finalizacion y se agrega el elemento "finalizadoExternamente" a todos las estructuras
				 *  Tambien, cabe destacar que hay una sola estructura procesos
				 */
				break;
			}

			case finalizarCiertoScript:{

				//***Estoy recibiendo un script para finalizar, le digo a memoria que lo finalize y si sale bien le aviso a consola, sino tambien le aviso, pero que salio mal xd
				int pid = leerInt(stream);
				int* respuesta = malloc(sizeof(int));

				printf("Entramos a finalizar el script, del pid: %d\n", pid);

				//***Le digo a memoria que mate a este programa
				enviarMensaje(socketMemoria,finalizarPrograma, &pid,sizeof(int));//CAMBIAR

				//***Esta función actualizará el estado de finalizacion de un proceso
				if(proceso_finalizacionExterna(pid, -7) )
				{
					enviarMensaje(socketConsola,pidFinalizado, &pid,sizeof(int));

					//***Memoria me avisa si no encontro el pid
					recibirMensaje(socketMemoria, &respuesta);
					if(!respuesta){
						errorEn(respuesta, "[Rutina rutinaConsola] - La Memoria no pudo finalizar el proceso\n");
					}
				}
				else{
					errorEn(respuesta, "[Rutina rutinaConsola] - No existe el pid\n");
					enviarMensaje(socketConsola,errorFinalizacionPid, &pid,sizeof(int));
				}


				free(respuesta);
			}break;
			case desconectarConsola:{ // podria ser 0
				//***Se desconecta la consola
				//int pid = leerInt(stream);

				//***Ya no hay mas nada que hacer, entonces cambio el bool de todaviahaytrabajo a false, asi salgo del while
				todaviaHayTrabajo = false;

				consola_finalizarTodosLosProcesos(socketConsola);

			}break;
			case 0:{
				printf("[Rutina rutinaConsola] - La consola ha perecido\n");
						todaviaHayTrabajo=false;
			}break;
			default:{
				printf("[Rutina rutinaConsola] - Se recibio una accion que no esta contemplada:%d se cerrara el socket\n",a);
				todaviaHayTrabajo=false;
			}break;
		}
	}
	printf("Se decontecta a la consola socket: %d\n", socketConsola);
	close(socketConsola);
}



////----FIN FUNCIONES CONSOLA-----///
