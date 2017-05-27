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
		pcb->cantidadDeEtiquetas = metadata->cantidad_de_etiquetas;

		pcb->indiceStack = NULL;

		pcb->cantidadDeEntradas = 0;
		pcb->cantidadDeInstrucciones = metadata->instrucciones_size;
		pcb->programCounter = metadata->instruccion_inicio;

	return pcb;
}


PCB_DATA * buscarPCBPorPidEnCola(t_queue * cola, int pid){
	bool busquedaDePid(PCB_DATA * pcb){
		return (pcb->pid == pid);
	}
	if(list_any_satisfy(cola->elements,busquedaDePid)){
		return (PCB_DATA*)list_find(cola->elements,busquedaDePid);
	}
	return NULL;
}

PCB_DATA * buscarPCBPorPidEnColaYBorrar(t_queue * cola, int pid){
	int i = 0;
	bool busquedaDePid(PCB_DATA * pcb){
		if(pcb->pid == pid)	return true;
		i++;
		return false;
	}
	if(list_any_satisfy(cola->elements,busquedaDePid)){
		list_remove(cola->elements,i);
		return (PCB_DATA*)list_find(cola->elements,busquedaDePid);
	}
	return NULL;
}

PCB_DATA * buscarPCBPorPidYBorrar(int pid){
	PCB_DATA * pcb;
	pcb = buscarPCBPorPidEnColaYBorrar(cola_New, pid);
	if(pcb != NULL) return pcb;
	pcb = buscarPCBPorPidEnColaYBorrar(cola_Ready, pid);
	if(pcb != NULL) return pcb;
	pcb = buscarPCBPorPidEnColaYBorrar(cola_Wait, pid);
	if(pcb != NULL) return pcb;
	pcb = buscarPCBPorPidEnColaYBorrar(cola_Finished, pid);
	if(pcb != NULL) return pcb;
	else{
		printf("No se pudo encontrar el PCB pedido");
		return NULL;
	}
}
