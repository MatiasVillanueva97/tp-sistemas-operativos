#include "funcionesPCB.h"


PCB_DATA* crearPCB(char * scriptAnsisop, int pid, int contPags){
	t_metadata_program *metadata = metadata_desde_literal(scriptAnsisop);
	PCB_DATA * pcb = malloc(sizeof(PCB_DATA));

		pcb->pid = pid;
		pcb->contPags_pcb = contPags;

		pcb->contextoActual = 0;
		pcb->exitCode = 53; //***Por ahora el exit code 53 significa que no esta terminado

		pcb->indiceCodigo = metadata->instrucciones_serializado;
		pcb->indiceEtiquetas = metadata->etiquetas;
		pcb->cantidadDeEtiquetas = metadata->cantidad_de_etiquetas + metadata->cantidad_de_funciones;

		pcb->indiceStack = NULL;

		pcb->sizeEtiquetas = metadata->etiquetas_size;

		//CAMBIAR ESTO ACA JAJA



		pcb->estadoDeProceso = 0;




		//SISI LO DE ALLA ARRIBA

		pcb->cantidadDeEntradas = 0;
		pcb->cantidadDeInstrucciones = metadata->instrucciones_size;
		pcb->programCounter = metadata->instruccion_inicio;

	return pcb;
}

void modificarPCB(PCB_DATA * pcbNuevo){


	bool busqueda(PROCESOS * aviso)
	{
		if(aviso->pid == pcbNuevo->pid)
		{
			puts("\ndentro de la busqueda");
			imprimirPCB(aviso->pcb);
			return true;
		}


		return false;
	}
	sem_wait(&mutex_listaProcesos);


	PCB_DATA * pcb2 = ((PROCESOS*)list_find(avisos, busqueda))->pcb;


	if(pcb2 != NULL){
	puts("\n\nDentro del if\n");
	destruirPCB_Local(*pcb2);
		pcb2->cantidadDeEntradas = pcbNuevo->cantidadDeEntradas;
		pcb2->cantidadDeEtiquetas = pcbNuevo->cantidadDeEtiquetas;
		pcb2->cantidadDeInstrucciones = pcbNuevo->cantidadDeInstrucciones;
		pcb2->contextoActual = pcbNuevo->contextoActual;
		pcb2->exitCode = pcbNuevo->exitCode;
		pcb2->indiceCodigo = pcbNuevo->indiceCodigo;
		pcb2->indiceEtiquetas = pcbNuevo->indiceEtiquetas;
		pcb2->indiceStack = pcbNuevo->indiceStack;
		pcb2->programCounter = pcbNuevo->programCounter;
	}

	//****Esta linea no se si va
	free(pcbNuevo);

	sem_post(&mutex_listaProcesos);
}
