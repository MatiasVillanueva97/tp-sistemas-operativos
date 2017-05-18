#include "funcionesPCB.h"

PCB_DATA* crearPCB(const char* script, int contadorPaginas, int unPid)
{
	PCB_DATA * pcb = malloc(sizeof(PCB_DATA));

	t_metadata_program* metadataScrip;
	metadataScrip = metadata_desde_literal(script);

//	pcb->indiceCodigo = malloc(sizeof(t_intructions)*(metadataScrip->instrucciones_size));

	pcb->pid = unPid;

//	pcb->programCounter = metadataScrip->instruccion_inicio;

	pcb->contPags_pcb = contadorPaginas;

	pcb->contextoActual=0;

//	pcb->indiceCodigo =  metadataScrip->instrucciones_serializado;
//	pcb->indiceEtiquetas= metadataScrip->etiquetas;
	pcb->indiceStack = malloc(sizeof(t_entrada));
	pcb->exitCode=1;  /// esta en 1 , es mera instalación

	//metadata_destruir(metadataScrip);// debería estar
} // creo que tiene  memori leaks


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
