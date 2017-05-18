/*
 * funcionesDeTablaInvertida.c
 *
 *  Created on: 17/5/2017
 *      Author: utnso
 */
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
#include <pthread.h>
#include <semaphore.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"


#include "EstructurasDeLaMemoria.h"

#include "parser/metadata_program.h"
#include "parser/parser.h"



//Funciones Tabla De Paginación Invertida
int buscarPidEnTablaInversa(int pidRecibido) //DEJO ESTA PORQUE SINO SE ROMPE TODO MIENTRAS SE VA IMPLEMENTANDO LO OTRO.
{
	return 0;
}
void iniciarTablaDePaginacionInvertida(){
	//tablaDePaginacionInvertida = malloc (sizeof(filaDeTablaPaginaInvertida)*getConfigInt("MARCOS"));//falta calcular la cantidadDe filas que tiene la tabla.
	tablaDePaginacionInvertida = malloc(getConfigInt("MARCOS")*sizeof(filaDeTablaPaginaInvertida));
	int i;
	for(i = 0;getConfigInt("MARCOS")> i;i++){
		tablaDePaginacionInvertida[i].frame = i;
		tablaDePaginacionInvertida[i].pid = -1;
		tablaDePaginacionInvertida[i].pagina = -1;
	}

	/*	filaDeTablaPaginaInvertida* auxiliarParaGuardar = malloc(sizeOfPaginas);
	int paginasNecesarias = cantidadDeMarcos*sizeof(filaDeTablaPaginaInvertida) / sizeOfPaginas;
	if (cantidadDeMarcos*sizeof(filaDeTablaPaginaInvertida) % sizeOfPaginas != 0){
		paginasNecesarias++;
	}
	//printf("paginas necesarias %d ",paginasNecesarias);
	for(i=0; i< paginasNecesarias; i++){
		reservarFrame(0,i+1);
		auxiliarParaGuardar = tablaDePaginacionInvertida+i*sizeOfPaginas;
		escribirMemoriaPosta(0,i+1,auxiliarParaGuardar);
	}*/
}
void imprimirTablaDePaginasInvertida(){
	FILE* archivo =  fopen ("tablaDePaginas.dat", "w+");
	int i;
	for (i=0;i<cantidadDeMarcos;i++){
		if(tablaDePaginacionInvertida[i].pid !=-1 &&tablaDePaginacionInvertida[i].pagina != -1){
			fprintf(archivo, "Fila numero %d:    %d |%d  |%d \n\n",  i+1,tablaDePaginacionInvertida[i].pid, tablaDePaginacionInvertida[i].pagina,tablaDePaginacionInvertida[i].frame);
			printf("Fila numero %d:    %d |%d  |%d \n\n",  i+1,tablaDePaginacionInvertida[i].pid, tablaDePaginacionInvertida[i].pagina,tablaDePaginacionInvertida[i].frame);
		}

	}
	fclose(archivo);

}

//Funciones Frame
int buscarFrameCorrespondiente(int pidRecibido,int pagina)
{
	int posicionDadaPorElHash = funcionHash(pidRecibido,pagina);
	filaDeTablaPaginaInvertida filaActual;
	while (posicionDadaPorElHash < getConfigInt("MARCOS")){
		filaActual =tablaDePaginacionInvertida[posicionDadaPorElHash];
		if (filaActual.pid == pidRecibido && filaActual.pagina == pagina ){
			return filaActual.frame;
		}
		posicionDadaPorElHash++;
	}
	return -1;
}
int reservarFrame(int pid, int pagina){
	int i;
	for(i=funcionHash(pid,pagina);getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == -1 && filaActual.pid == -1){
			tablaDePaginacionInvertida[i].pagina = pagina;
			tablaDePaginacionInvertida[i].pid = pid;
			return 1;
		}
	}
	return -1;
}
int memoriaFramesLibres(){
	int i = 0;
	int libres = 0;
	for(i;getConfigInt("MARCOS") > i;i++){
			filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
			if(filaActual.pagina == -1 && filaActual.pid == -1){
				tablaDePaginacionInvertida[i].pagina = -1;
				tablaDePaginacionInvertida[i].pid = -1;
				libres++;
			}
		}
	return libres;
}

//Funciones Paginas
int cantidadDePaginasDeUnProcesoDeUnProceso(int pid){
	int i = 0;
	int paginas = 0;
	for(i;getConfigInt("MARCOS") > i;i++){
			filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
			if(filaActual.pid == pid){
				paginas++;
			}
		}
	return paginas;
}
int liberarPagina(int pid, int pagina){
	int i;
	for(i=funcionHash(pid,pagina);getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == pagina && filaActual.pid == pid){
			tablaDePaginacionInvertida[i].pagina = -1;
			tablaDePaginacionInvertida[i].pid = -1;
			return 1;
		}
	}
	return -1;
}
