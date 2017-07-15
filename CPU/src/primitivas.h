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
#include "commons/log.h"
#include "commons/collections/list.h"

#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "compartidas.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "../../Nuestras/src/laGranBiblioteca/ProcessControlBlock.h"
#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"


	int recibirMensajeSeguro(int socketKernel, void ** stream);

	//IMPLEMENTADA
	t_direccion calcularDireccion(t_puntero puntero);
	t_puntero calcularPuntero(t_direccion direccion);


	//IMPLEMENTADA
	t_puntero AnSISOP_definirVariable(t_nombre_variable identificador_variable);

	//IMPLEMENTADA
	t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable identificador_variable);

	//IMPLEMENTADA
	t_valor_variable AnSISOP_dereferenciar(t_puntero direccion_variable);

	//IMPLEMENTADA
	void AnSISOP_asignar(t_puntero direccion_variable, t_valor_variable valor);

	//IMPLEMENTADA
	t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida variable);

	//IMPLEMENTADA
	t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida variable,t_valor_variable valor);

	//IMPLEMENTADA
	void AnSISOP_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);

	//IMPLEMENTADA
	void AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta);

	//IMPLEMENTADA
	void AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,t_puntero donde_retornar);

	//IMPLEMENTADA
	void AnSISOP_finalizar(void);

	//IMPLEMENTADA
	void AnSISOP_retornar(t_valor_variable retorno);

//Operaciones de Kernel

	//IMPLEMENTADA
	void AnSISOP_wait(t_nombre_semaforo identificador_semaforo);

	//IMPLEMENTADA
	void AnSISOP_signal(t_nombre_semaforo identificador_semaforo);

	//IMPLEMENTADA
	t_puntero AnSISOP_reservar(t_valor_variable espacio);

	//IMPLEMENTADA
	void AnSISOP_liberar(t_puntero puntero);

	//IMPLEMENTADA
	t_descriptor_archivo AnSISOP_abrir(t_direccion_archivo direccion,t_banderas flags);

//UNTESTED
	void AnSISOP_borrar(t_descriptor_archivo direccion);

	//IMPLEMENTADA
	void AnSISOP_cerrar(t_descriptor_archivo descriptor_archivo);

	//IMPLEMENTADA
	void AnSISOP_moverCursor(t_descriptor_archivo descriptor_archivo,t_valor_variable posicion);

//WORK IN PROGRESS
	void AnSISOP_escribir(t_descriptor_archivo descriptor_archivo,void* informacion, t_valor_variable tamanio);

	//IMPLEMENTADA
void AnSISOP_leer(t_descriptor_archivo descriptor_archivo,t_puntero informacion, t_valor_variable tamanio);
