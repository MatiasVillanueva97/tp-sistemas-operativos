/*
 * funcionesDeTablaInvertida.h
 *
 *  Created on: 17/5/2017
 *      Author: utnso
 */
#include <stdbool.h>
#include <stdint.h>
#ifndef FUNCIONESDETABLAINVERTIDA_H_
#define FUNCIONESDETABLAINVERTIDA_H_


//Funciones Tabla De Paginación Invertida
int buscarPidEnTablaInversa(int pidRecibido);
void iniciarTablaDePaginacionInvertida();
void imprimirTablaDePaginasInvertida();

//Funciones Frame
int buscarFrameCorrespondiente(int pidRecibido,int pagina);
int reservarFrame(int pid, int pagina);
int memoriaFramesLibres();

//Funciones Paginas
int cantidadDePaginasDeUnProcesoDeUnProceso(int pid);
int liberarPagina(int pid, int pagina);
#endif /* FUNCIONESDETABLAINVERTIDA_H_ */
