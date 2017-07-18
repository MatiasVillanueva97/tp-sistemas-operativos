#include "funcionesPCB.h"


PCB_DATA* crearPCB(char * scriptAnsisop, int pid, int contPags){
	t_metadata_program *metadata = metadata_desde_literal(scriptAnsisop);
	PCB_DATA * pcb = malloc(sizeof(PCB_DATA));

		pcb->pid = pid;
		pcb->contPags_pcb = contPags;
		pcb->contextoActual = 0;
		pcb->exitCode = 53;
		pcb->indiceCodigo = metadata->instrucciones_serializado;
		pcb->indiceEtiquetas = metadata->etiquetas;
		pcb->cantidadDeEtiquetas = metadata->cantidad_de_etiquetas + metadata->cantidad_de_funciones;
		pcb->indiceStack = NULL;
		pcb->sizeEtiquetas = metadata->etiquetas_size;
		pcb->estadoDeProceso = 0;
		pcb->cantidadDeEntradas = 0;
		pcb->cantidadDeInstrucciones = metadata->instrucciones_size;
		pcb->programCounter = metadata->instruccion_inicio;
		pcb->cantDeRafagasEjecutadas = 0;
		pcb->cantDeInstPrivilegiadas = 0;

	return pcb;
}
//Sincronizame señor!Por favor!
PCB_DATA * buscarPCB (int pid){
	bool sonIguales(PROCESOS* elemento){
		return elemento->pid == pid;
	}
	return((PROCESOS*)list_find(avisos,sonIguales))->pcb;
}

PCB_DATA* modificarPCB(PCB_DATA * pcbNuevo){

	bool busqueda(PROCESOS * aviso)
	{
		if(aviso->pid == pcbNuevo->pid)
		{
			log_info(logKernel,"\ndentro de la busqueda");
			//imprimirPCB(aviso->pcb);
			return true;
		}


		return false;
	}
//	sem_wait(&mutex_listaProcesos);

	PCB_DATA * pcbViejo = ((PROCESOS*)list_find(avisos, busqueda))->pcb;

	if(pcbViejo != NULL){
		if(pcbViejo->estadoDeProceso != finalizado){
			destruirPCB_Local(*pcbViejo);
			pcbViejo->cantidadDeEntradas = pcbNuevo->cantidadDeEntradas;
			pcbViejo->cantidadDeEtiquetas = pcbNuevo->cantidadDeEtiquetas;
			pcbViejo->cantidadDeInstrucciones = pcbNuevo->cantidadDeInstrucciones;
			pcbViejo->contextoActual = pcbNuevo->contextoActual;
			pcbViejo->exitCode = pcbNuevo->exitCode;
			pcbViejo->indiceCodigo = pcbNuevo->indiceCodigo;
			pcbViejo->indiceEtiquetas = pcbNuevo->indiceEtiquetas;
			pcbViejo->indiceStack = pcbNuevo->indiceStack;
			pcbViejo->estadoDeProceso = pcbNuevo->estadoDeProceso;
			pcbViejo->programCounter = pcbNuevo->programCounter;
		}
	}

	//****Esta linea no se si va
	free(pcbNuevo);

//	sem_post(&mutex_listaProcesos);
	return pcbViejo;
}
