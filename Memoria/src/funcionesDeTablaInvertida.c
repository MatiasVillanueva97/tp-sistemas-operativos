/*
 * funcionesDeTablaInvertida.c
 *
 *  Created on: 17/5/2017
 *      Author: utnso
 */
#include "funcionesDeTablaInvertida.h"



//Funciones Tabla De Paginación Invertida

void iniciarTablaDePaginacionInvertida(){
	//tablaDePaginacionInvertida = malloc (sizeof(filaDeTablaPaginaInvertida)*getConfigInt("MARCOS"));//falta calcular la cantidadDe filas que tiene la tabla.
	tablaDePaginacionInvertida = memoriaTotal;

	int cantidad = ceil(((double)((getConfigInt("MARCOS")*sizeof(filaDeTablaPaginaInvertida)))/((double) sizeOfPaginas)));
	log_info(logMemoria,"La tabla de paginacion invertida ocupa %d paginas para administracion",cantidad );
	int i;

	for(i=0;i<cantidad;i++){
		tablaDePaginacionInvertida[i].frame = i;
		tablaDePaginacionInvertida[i].pid = 0;
		tablaDePaginacionInvertida[i].pagina = i;
	}
	filaTablaCantidadDePaginas* nuevaFila= malloc(sizeof(filaTablaCantidadDePaginas));
	nuevaFila->pid = 0;
	nuevaFila->cantidadDePaginasReales = cantidad;
	nuevaFila->paginaMaxima = cantidad;
	log_info(logMemoria,"Se agrego las paginas administrativas a la tabla de cantidad de paginas");

	list_add(tablaConCantidadDePaginas,nuevaFila);

	for(i = cantidad;getConfigInt("MARCOS")> i;i++){
		tablaDePaginacionInvertida[i].frame = i;
		tablaDePaginacionInvertida[i].pid = -1;
		tablaDePaginacionInvertida[i].pagina = -1;
	}
	log_info(logMemoria,"Se le dio formato a la tabla de paginacion invertida");

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
	sem_wait(&mutex_TablaDePaginasInvertida);
	for (i=0;i<cantidadDeMarcos;i++){
		if(tablaDePaginacionInvertida[i].pid !=-1 &&tablaDePaginacionInvertida[i].pagina != -1){
			fprintf(archivo, "Fila numero %d:    %d |%d  |%d \n\n",  i+1,tablaDePaginacionInvertida[i].pid, tablaDePaginacionInvertida[i].pagina,tablaDePaginacionInvertida[i].frame);
			printf("Fila numero %d:    %d |%d  |%d \n\n",  i+1,tablaDePaginacionInvertida[i].pid, tablaDePaginacionInvertida[i].pagina,tablaDePaginacionInvertida[i].frame);
		}

	}
	sem_post(&mutex_TablaDePaginasInvertida);
	fclose(archivo);

}

//Funciones Frame
int buscarFrameCorrespondiente(int pidRecibido,int pagina)
{
	int posicionDadaPorElHash = funcionHash(pidRecibido,pagina);
	filaDeTablaPaginaInvertida filaActual;
	int i;
	for (i=0;i < getConfigInt("MARCOS");i++){
		filaActual =tablaDePaginacionInvertida[i];
		if (filaActual.pid == pidRecibido && filaActual.pagina == pagina ){

			log_info(logMemoria,"Se devolvio el frame %d de la busqueda en la tabla",filaActual.frame);
			return filaActual.frame;
		}
	}
	for (i=0;posicionDadaPorElHash >0;i++){
		filaActual =tablaDePaginacionInvertida[i];
		if (filaActual.pid == pidRecibido && filaActual.pagina == pagina ){
			log_info(logMemoria,"Se devolvio el frame %d de la busqueda en la tabla",filaActual.frame);
			return filaActual.frame;
		}
	}
	log_error(logMemoria,"La pagina no estaba en la tabla de paginacion invertida.");
	return -1;
}
int reservarFrame(int pid, int pagina){
	int i;
	int posicionEnLatabla = funcionHash(pid,pagina);
	for(i=posicionEnLatabla;getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == -1 && filaActual.pid == -1){
			tablaDePaginacionInvertida[i].pagina = pagina;
			tablaDePaginacionInvertida[i].pid = pid;
			log_info(logMemoria,"Se reservo el frame %d para la pagina %d del pid %d",tablaDePaginacionInvertida[i].frame,pid,pagina);

			return 1;
		}
	}
	for(i=0;posicionEnLatabla> i;i++){
			filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
			if(filaActual.pagina == -1 && filaActual.pid == -1){
				filaTablaCantidadDePaginas* fila = malloc(sizeof(filaTablaCantidadDePaginas));
				list_add(tablaConCantidadDePaginas,fila);
				tablaDePaginacionInvertida[i].pagina = pagina;
				tablaDePaginacionInvertida[i].pid = pid;
				log_info(logMemoria,"Se reservo el frame %d para la pagina %d del pid %d",tablaDePaginacionInvertida[i].frame,pid,pagina);
				return 1;
		}
	}
	log_error(logMemoria,"No se pudo reservar ningun frame, no habia ninguno disponible");

	return 0;
}
/*
int sum(t_list *lista,int(* funcion) (void*)){
	int i;
	int contador=0;
	for( i = 0; i< list_size(lista);i++){
		contador += funcion(list_get(lista,i));
	}
	return contador;
}*/
int getCantidadDePaginas(filaTablaCantidadDePaginas * fila){
	return fila->cantidadDePaginasReales;
}
int memoriaFramesLibres(){
	int i = 0;
	int libres = 0;
	sem_wait(&mutex_TablaDeCantidadDePaginas);
	int prueba = sum(tablaConCantidadDePaginas,getCantidadDePaginas);
	sem_post(&mutex_TablaDeCantidadDePaginas);
	log_info(logMemoria,"Hay %d frames disponibles en memoria",prueba);

	return prueba;
	/*for(i;getConfigInt("MARCOS") > i;i++){
			filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
			if(filaActual.pagina == -1 && filaActual.pid == -1){

				tablaDePaginacionInvertida[i].pagina = -1;
				tablaDePaginacionInvertida[i].pid = -1;
				libres++;
			}
		}
	return libres;
	*/
}

//Funciones Paginas
/*
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
}*/
int liberarPagina(int pid, int pagina){ //Esta sincronizado en finalizarPrograma.
	int i;
	int posicionEnLaTabla =funcionHash(pid,pagina);
	log_info(logMemoria,"[Liberar Pagina]-La funcion de hash aproxima a la posicion %d de la tabla de paginacion invetida", posicionEnLaTabla);
	sem_wait(&mutex_TablaDeCantidadDePaginas);
	bool buscarPid(filaTablaCantidadDePaginas* fila){
			return (fila->pid== pid);
	}
	filaTablaCantidadDePaginas* fila =list_find(tablaConCantidadDePaginas,buscarPid);
	if(fila == NULL){
		log_error(logMemoria,"[Liberar Pagina]-No existe la entrada en la tabla de cantidad de paginas.Por lo tanto, se ingreso un pid invalido");
		sem_post(&mutex_TablaDeCantidadDePaginas);
		return 0;
	}
	fila->cantidadDePaginasReales-=1;
	sem_post(&mutex_TablaDeCantidadDePaginas);
	sem_wait(&mutex_TablaDePaginasInvertida);
	for(i=posicionEnLaTabla;getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == pagina && filaActual.pid == pid){
			tablaDePaginacionInvertida[i].pagina = -1;
			tablaDePaginacionInvertida[i].pid = -1;
			sem_post(&mutex_TablaDePaginasInvertida);
			log_info(logMemoria,"[Liberar Pagina]-Se libero el frame %d de la pagina %d del pid %d",tablaDePaginacionInvertida[i].frame,pagina,pid);
			return 1;
		}
	}
	for(i=0;posicionEnLaTabla> i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == pagina && filaActual.pid == pid){
			tablaDePaginacionInvertida[i].pagina = -1;
			tablaDePaginacionInvertida[i].pid = -1;
			sem_post(&mutex_TablaDePaginasInvertida);
			log_info(logMemoria,"[Liberar Pagina]-Se libero el frame %d de la pagina %d del pid %d",tablaDePaginacionInvertida[i].frame,pagina,pid);
			return 1;
		}
	}
	sem_post(&mutex_TablaDePaginasInvertida);
	log_error(logMemoria,"[Liberar Pagina]-No se encontro la pagina %d del pid %d en la tabla de paginacion invertida",pagina,pid);
	fila->cantidadDePaginasReales+=1; // Subido ultimamente
	return 0;
}
