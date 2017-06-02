/*
 * funcionesConsolaKernel.c
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#include "funcionesConsolaKernel.h"


///---- CONSOLA DEL KERNEL -----////

//**Esta funcion anda
///***Esta funcion imprime todos los pids de sistema
void imprimirTodosLosProcesosEnColas(){

	void imprimir (PROCESOS * aviso)
	{
		printf("Pid: %d\n", aviso->pid);

		imprimirPCB(aviso->pcb);

		if(aviso->pcb->exitCode == 53)
			printf("Estado: en procesamiento\n");
		else
			printf("Estado: finalizado (%d)\n",aviso->pcb->exitCode);
	}

	list_iterate(avisos, imprimir);
}

//*** probar esta funcion - no anda, arreglar
///*** Esta funcion dada una cola te imprime todos los procesos que esta contenga
void imprimirProcesosdeCola(t_queue* unaCola)
{
	void imprimir(PCB_DATA * pcb){
		printf("Pid: %d\n",pcb->pid);
		//imprimirPCB(pcb);
	}

	int a=queue_size(unaCola);
	printf("kiusaiz: %d\n",a);//,(char*)unaCola->elements->head->data);

	if(a>0 )
		list_iterate(unaCola->elements, imprimir);
	else
		printf("No hay elementos en esta cola.\n");
}




void * consolaKernel()
{
	int opcion;
	printf("Hola Bienvenido al Kernel!\n\n"
			"Aca esta el menu de todas las opciones que tiene para hacer:\n"
			"1- Obtener el listado de procesos del sistema de alguna cola.\n"
			"2- Obtener datos sobre un proceso.\n"
			"3- Obtener la tabla global de archivos.\n"
			"4- Modificar el grado de multiprogramación del sistema.\n"
			"5- Finalizar un proceso.\n"
			"6- Detener la planificación.\n"
			"7- Reactivar la planificación.\n"
			"8- Imprimir de nuevo el menu.\n\n"
			"Elija el numero de su opcion: ");
	sem_post(&sem_ConsolaKernelLenvantada);
	scanf("%d", &opcion);


	while(1)
	{
		switch(opcion){
			case 1:{
				printf("Selecione la cola que quiere imprimir:\n"
						"1- Cola de New.\n"
						"2- Cola de Ready.\n"
						"3- Cola de Exec.\n"
						"4- Cola de Bloq.\n"
						"5- Cola de Finish.\n"
						"6- Todas las colas.\n"
						"Elija el numero de su opcion: ");
						scanf("%d", &opcion);

				switch(opcion){
					case 1:{
						printf("Procesos de la cola de New:\n");

						sem_wait(&mutex_cola_New);
							imprimirProcesosdeCola(cola_New);
						sem_post(&mutex_cola_New);
					}break;
					case 2:{
						printf("Procesos de la cola de Ready:\n");

						sem_wait(&mutex_cola_Ready);
							imprimirProcesosdeCola(cola_Ready);
						sem_post(&mutex_cola_Ready);

					}break;
					case 3:{
						printf("Procesos de la cola de Exec:\n");

						sem_wait(&mutex_cola_Exec);
							imprimirProcesosdeCola(cola_Exec);
						sem_post(&mutex_cola_Exec);
					}break;
					case 4:{
						printf("Procesos de la cola de Bloq:\n");

						sem_wait(&mutex_cola_Wait);
							imprimirProcesosdeCola(cola_Wait);
						sem_post(&mutex_cola_Wait);
					}break;
					case 5:{
						printf("Procesos de la cola de Finish:\n");

						sem_wait(&mutex_cola_Finished);
							imprimirProcesosdeCola(cola_Finished);
						sem_post(&mutex_cola_Finished);
					}break;
					case 6:{
						printf("Estos son todos los procesos:\n");

						sem_wait(&mutex_listaProcesos);
							imprimirTodosLosProcesosEnColas();
						sem_post(&mutex_listaProcesos);
					}break;
					default:{
						printf("Opcion invalida! Intente nuevamente.\n");
					}break;
				}


			}break;
			case 2:{
				int pid;
				printf("Ingrese pid del proceso a finalizar: ");
				scanf("%d",&pid);
/*
				2. Obtener para un proceso dado:
				a. La cantidad de rafagas ejecutadas.
				b. La cantidad de operaciones privilegiadas que ejecutó.
				c. Obtener la tabla de archivos abiertos por el proceso.
				d. La cantidad de páginas de Heap utilizadas
				i. Cantidad de acciones alocar realizadas en cantidad de operaciones y en
				bytes
				ii. Cantidad de acciones liberar realizadas en cantidad de operaciones y en
				bytes
				e. Cantidad de syscalls ejecutadas*/
			}break;
			case 3:{
				/// ni idea que es esto
			}break;
			case 4:{
				int gradoNuevo;
				printf("Ingrese nuevo Grado de multiprogramacion: ");
				scanf("%d",&gradoNuevo);

				//probablemente tengamos qeu poner un semaforo para la variable global de grado multiprogramacion
				grado_multiprogramacion=gradoNuevo;
			}break;
			case 5:{
				int pid;
				printf("Ingrese pid del proceso a finalizar: ");
				scanf("%d",&pid);

				if(proceso_finalizacionExterna(pid,  -50681)) //cambiar el numero del exit code, por el que sea el correcto
					printf("Finalizacion exitosa.\n");
				else
					printf("El Pid %d es Incorrecto! Reeintente con un nuevo pid.\n",pid);

			}break;
			case 6:{
				finPorConsolaDelKernel=true;
				printf("Planificacion detenida.\n");
			}break;
			case 7:{
				finPorConsolaDelKernel=false;
				printf("Planificacion reactivada.\n");
			}break;
			case 8:{
				printf("Hola Bienvenido al Kernel!\n\n"
						"Aca esta el menu de todas las opciones que tiene para hacer:\n"
						"1- Obtener el listado de procesos del sistema.\n"
						"2- Obtener datos sobre un proceso.\n"
						"3- Obtener la tabla global de archivos.\n"
						"4- Modificar el grado de multiprogramación del sistema.\n"
						"5- Finalizar un proceso.\n"
						"6- Detener la planificación.\n"
						"7- Imprimir de nuevo el menu.\n\n");
			}break;
			default:{

			}break;
		}

		printf("Elija el numero de su opcion: ");
		scanf("%d", &opcion);
	}
}

///---- FIN CONSOLA KERNEL----////

