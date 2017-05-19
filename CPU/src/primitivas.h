#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "commons/string.h"
#include "commons/collections/list.h"

#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "compartidas.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"


int a,b;
t_direccion dirA,dirB;

//HARDCODEADISIMO PAPU
#define tamanioPagina 64

typedef struct{
 int pid;
 char* mensaje;
} t_mensajeDeProceso;

typedef struct{
	int id;
	t_direccion direccion;
}t_pedidoMemoria;

typedef struct{
	int id;
	t_direccion direccion;
	t_valor_variable valor;
}t_escrituraMemoria;

	t_direccion calcularDireccion(t_puntero puntero);

	t_puntero AnSISOP_definirVariable(t_nombre_variable identificador_variable);

	t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable identificador_variable);

	t_valor_variable AnSISOP_dereferenciar(t_puntero direccion_variable);

	void AnSISOP_asignar(t_puntero direccion_variable, t_valor_variable valor);

	t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida variable);

	t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida variable,t_valor_variable valor);

	void AnSISOP_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);

	void AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta);

	void AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,t_puntero donde_retornar);

	void AnSISOP_finalizar(void);

	void AnSISOP_retornar(t_valor_variable retorno);

//Operaciones de Kernel

	void AnSISOP_wait(t_nombre_semaforo identificador_semaforo);

	void AnSISOP_signal(t_nombre_semaforo identificador_semaforo);

	t_puntero AnSISOP_reservar(t_valor_variable espacio);

	void AnSISOP_liberar(t_puntero puntero);

	t_descriptor_archivo AnSISOP_abrir(t_direccion_archivo direccion,t_banderas flags);

	void AnSISOP_borrar(t_descriptor_archivo direccion);

	void AnSISOP_cerrar(t_descriptor_archivo descriptor_archivo);

	void AnSISOP_moverCursor(t_descriptor_archivo descriptor_archivo,t_valor_variable posicion);

	void AnSISOP_escribir(t_descriptor_archivo descriptor_archivo,void* informacion, t_valor_variable tamanio);

	void AnSISOP_leer(t_descriptor_archivo descriptor_archivo,t_puntero informacion, t_valor_variable tamanio);
