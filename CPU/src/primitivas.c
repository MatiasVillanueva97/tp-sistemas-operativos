#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "commons/config.h"
#include "commons/string.h"
#include <parser/parser.h>


#include "compartidas.h"


typedef struct{
 int pid;
 char* mensaje;
} t_mensajeDeProceso;


t_puntero AnSISOP_definirVariable(t_nombre_variable identificador_variable){
	puts("AnSISOP_definirVariable");
	return 0x10;
}

t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable identificador_variable){
	puts("AnSISOP_obtenerPosicionVariable");
	return 0x10;
}

t_valor_variable AnSISOP_dereferenciar(t_puntero direccion_variable){
	puts("AnSISOP_dereferenciar");
	return 0;
}

void AnSISOP_asignar(t_puntero direccion_variable, t_valor_variable valor){
	puts("AnSISOP_asignar");

}

t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida variable){
	puts("AnSISOP_obtenerValorCompartida");
	return 0;
}

t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida variable,t_valor_variable valor){
	puts("AnSISOP_asignarValorCompartida");
	return 0;
}

void AnSISOP_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){
	puts("AnSISOP_irAlLabel");

}

void AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	puts("AnSISOP_llamarSinRetorno");

}

void AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,t_puntero donde_retornar){
	puts("AnSISOP_llamarConRetorno");

}

void AnSISOP_finalizar(void){
	puts("AnSISOP_finalizar");

}

void AnSISOP_retornar(t_valor_variable retorno){
	puts("AnSISOP_retornar");

}

//Operaciones de Kernel

void AnSISOP_wait(t_nombre_semaforo identificador_semaforo){
	puts("AnSISOP_wait");

}

void AnSISOP_signal(t_nombre_semaforo identificador_semaforo){
	puts("AnSISOP_signal");

}

t_puntero AnSISOP_reservar(t_valor_variable espacio){
	puts("AnSISOP_reservar");
	return 0x10;
}

void AnSISOP_liberar(t_puntero puntero){
	puts("AnSISOP_liberar");

}

t_descriptor_archivo AnSISOP_abrir(t_direccion_archivo direccion,t_banderas flags){
	puts("AnSISOP_abrir");
	return 0;
}

void AnSISOP_borrar(t_descriptor_archivo direccion){
	puts("AnSISOP_borrar");

}

void AnSISOP_cerrar(t_descriptor_archivo descriptor_archivo){
	puts("AnSISOP_cerrar");

}

void AnSISOP_moverCursor(t_descriptor_archivo descriptor_archivo,t_valor_variable posicion){
	puts("AnSISOP_moverCursor");

}

void AnSISOP_escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	puts("AnSISOP_escribir");
	t_mensajeDeProceso mensajeDeProceso;
	mensajeDeProceso.pid = pcb.pid;
	mensajeDeProceso.mensaje = informacion;
	printf("%d   %s",mensajeDeProceso.pid,mensajeDeProceso.mensaje);
	//enviarMensaje(socketKernel,4,&mensajeDeProceso,tamanio + sizeof(int));

	//intentar arreglar esto de otra forma
}

void AnSISOP_leer(t_descriptor_archivo descriptor_archivo,t_puntero informacion, t_valor_variable tamanio){
	puts("AnSISOP_leer");
}
