/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
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
#include "commons/collections/list.h"

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"
#include "../../Nuestras/src/laGranBiblioteca/funcionesParaTodosYTodas.c"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

#include "EstructurasDeLaMemoria.h"
#include "funcionesDeTablaInvertida.h"
#include "funcionesDeCache.h"

//Variables
 // meparece que es otro tipo de semaforo, no mutex



//Función de Hash
int funcionHash (int pid, int pagina){
	char str1[20];
	char str2[20];
	sprintf(str1, "%d", pid);
	sprintf(str2, "%d", pagina);
	strcat(str1, str2);
	unsigned int indice = atoi(str1) % cantidadDeMarcos;
	return indice;
}


t_escrituraMemoria deserializarAlmacenarBytes(void* almacenar){
	t_escrituraMemoria x;
	memcpy(&x.id,almacenar,sizeof(int));
	memcpy(&x.direccion.page,almacenar+sizeof(int),sizeof(int));
	memcpy(&x.direccion.offset,almacenar+sizeof(int)*2,sizeof(int));
	memcpy(&x.direccion.size,almacenar+sizeof(int)*3,sizeof(int));
	x.valor = malloc(x.direccion.size);
	memcpy(x.valor,almacenar+sizeof(int)*4,x.direccion.size);
	return x;
}



//Funciones De Memoria
void* leerMemoriaPosta (int pid, int pagina ){
	int frame = buscarFrameCorrespondiente(pid,pagina); //checkear que no haya errores en buscarFrame.
	if(frame==-1){
		return 0;
	}
	void * contenido = malloc(getConfigInt("MARCO_SIZE"));
	memcpy(contenido,memoriaTotal+frame*getConfigInt("MARCO_SIZE"),getConfigInt("MARCO_SIZE"));
	log_info(logMemoria,"[Solicitar Bytes]-Se lee toda la pagina %d del pid &d.",pagina,pid);

	return contenido;
}
int escribirMemoriaPosta(int pid,int pagina,void* contenido){
	//Antes de poder escribir, se deben haber reservado los frame.
/*	if(strlen(contenido)>getConfigInt("MARCO_SIZE")){
		puts(contenido);
		puts(string_itoa(strlen(contenido)));
		perror("El tamaño del contenido es mayor al tamaño de una pagina con la configuracion actual idiota");
		return 0;
	}
*/
	int frame = buscarFrameCorrespondiente(pid,pagina);
	if (frame == -1){
		return 0;
	}
	int posicion = frame*getConfigInt("MARCO_SIZE");
	memcpy(memoriaTotal+posicion,contenido,getConfigInt("MARCO_SIZE"));
	log_info(logMemoria,"Se escribe el contenido en memoria: %s", (char*) contenido);
	return 1;
}
void imprimirContenidoCache(){
	//Los mutex esta antes ( Donde se llama la funcion).
	FILE* archivo =  fopen ("contenidoCache.txt", "w+");
	int i;
	log_info(logMemoria,"Se imprime el contenido de la cache.");
	if(list_is_empty(tablaDeEntradasDeCache))
	{
		printf("La cache esta vacía!" );
	}
	else {

		for (i=0;i<list_size(tablaDeEntradasDeCache);i++){
			lineaCache* linea = list_get(tablaDeEntradasDeCache,i);
			printf("La pagina %d, del pid %d, tiene este contenido:\n", linea->pagina,linea->pid );
			puts((char*)linea->contenido);
			//printf("%s", (char*) &linea->contenido );
			fprintf(archivo, "La pagina %d, del pid %d, tiene este contenido:\n ", linea->pagina,linea->pid );
			fwrite((char*)linea->contenido,sizeOfPaginas,1,archivo);
			fwrite("\n",1,1,archivo);
		}
	}
}
void imprimirContenidoMemoria(){
	FILE* archivo =  fopen ("file.txt", "w+");
	int i;
	log_info(logMemoria,"Se imprime el contenido de la memoria.");

	sem_wait(&mutex_Memoria);
	int w;
	for(i= 0; i < list_size(tablaConCantidadDePaginas);i++){
		filaTablaCantidadDePaginas fila = *(filaTablaCantidadDePaginas*)list_get(tablaConCantidadDePaginas,i);
		fprintf(archivo, "El pid %d tiene %d paginas. \n\n", fila.pid ,fila.paginaMaxima);
		printf("El pid %d tiene %d paginas. \n\n", fila.pid ,fila.paginaMaxima);
		for(w=0;w<fila.paginaMaxima;w++){
			char* contenido =leerMemoriaPosta(fila.pid,w);
			if(contenido != 0){
				fprintf(archivo, "\nContenido de la pagina numero %s: \n",  string_itoa(w));
				fwrite(contenido,sizeOfPaginas,1,archivo);
				printf( "\n Contenido de la pagina numero %s: \n ",  string_itoa(w));
				puts((char*)contenido);
			}
		}
	}
/*
	for (i=0;i<cantidadDeMarcos;i++){
		fprintf(archivo, "Contenido de la pagina numero %s \n\n",  string_itoa(i+1));

		fwrite(memoriaTotal+i*sizeOfPaginas,sizeOfPaginas,1,archivo);
		fwrite("\n",1,1,archivo);
	}
	*/
	sem_post(&mutex_Memoria);
	//fwrite(memoriaTotal,sizeOfPaginas,getConfigInt("MARCOS"),archivo);
	fclose(archivo);
}
/*
bool hayEspacio(int pid,int pagina){
	int i;
	for(i=funcionHash(pid,pagina);getConfigInt("MARCOS") > i;i++){
		filaDeTablaPaginaInvertida filaActual = tablaDePaginacionInvertida[i];
		if(filaActual.pagina == -1 && filaActual.pid == -1){
			return true;
		}
	}
	return false;
}
*/
//Funciones Principales

int* almacenarBytesEnPagina(int pid, int pagina, int desplazamiento, int tamano,void* buffer){
	int* prueba=malloc(sizeof(int));
	*prueba= 0;
	if(desplazamiento + tamano > sizeOfPaginas){
		log_error(logMemoria,"Almacenamiento invalido!");
		return prueba;
	}
	void *contenidoDeLaPagina;

	sem_wait(&mutex_TablaDePaginasInvertida);
	int frame = buscarFrameCorrespondiente(pid,pagina);
	sem_post(&mutex_TablaDePaginasInvertida);
	if(frame ==-1){
		return prueba;
	}
	log_info(logMemoria,"La pagina %d del pid %d se corresponde con el frame %d",pagina,pid,frame);

	sem_wait(&mutex_Memoria);
	contenidoDeLaPagina = memoriaTotal+frame*getConfigInt("MARCO_SIZE");
	memcpy(contenidoDeLaPagina+desplazamiento, buffer,tamano);
	sem_wait(&mutex_cache);
	if(buscarEnLaCache(pid,pagina) !=NULL){
		log_info(logMemoria,"Se procede a actualizar la cache ya que la pagina modificada se encuentra en la cache");
		actualizarPaginaDeLaCache(pid,pagina,tamano,desplazamiento,buffer);
	}
	sem_post(&mutex_cache);
	sem_post(&mutex_Memoria);
	*prueba = 1;
	log_info(logMemoria,"Se devuelve %d ya que se almaceno correctamente",*prueba);

	return prueba;
}
void* solicitarBytesDeUnaPagina(int pid, int pagina, int desplazamiento, int tamano){
	void* contenidoDeLaPagina;
	sem_wait(&mutex_cache);
	if((contenidoDeLaPagina= buscarEnLaCache(pid,pagina))!=NULL){
		log_info(logMemoria,"[Solicitar Bytes]-La pagina %d del proceso %d esta en la cache, se procede a un cache hit",pid,pagina);

		cacheHit(pid,pagina);
		sem_post(&mutex_cache);
	}
	else{
		log_info(logMemoria,"[Solicitar Bytes]-La pagina %d del proceso %d no esta en la cache",pid,pagina);

		sem_post(&mutex_cache);

		sem_wait(&mutex_retardo);
		int retardoLocal = retardo;
		sem_post(&mutex_retardo);
		log_info(logMemoria,"[Solicitar Bytes]- Se procede a ejecutar el retardo por no estar en cache %d", retardoLocal);
		usleep(retardoLocal*1000);
		sem_wait(&mutex_Memoria); //No se si son 2 mutex distintos
		contenidoDeLaPagina = leerMemoriaPosta(pid,pagina);
		if ((int)contenidoDeLaPagina == 0){
			log_error(logMemoria,"[Solicitar Bytes]-La solicutd de la pagina %d del pid %d es invalida!",pagina,pid);
			free(contenidoDeLaPagina);
			return 0;
		}
		log_info(logMemoria,"[Solicitar Bytes]-Se encontro el contenido y se procede a ejecutar un cache miss");
		sem_wait(&mutex_cache);
		cacheMiss(pid,pagina,contenidoDeLaPagina);
		sem_post(&mutex_cache);
		sem_post(&mutex_Memoria);
	}
	void* contenidoADevolver = malloc(tamano);
	memcpy(contenidoADevolver,contenidoDeLaPagina+desplazamiento,tamano);
	free(contenidoDeLaPagina);
	return contenidoADevolver;

}
int buscarFilaEnTablaCantidadDePaginas(int pid){

	bool buscarPid(filaTablaCantidadDePaginas* fila){
			return (fila->pid== pid);
	}
	filaTablaCantidadDePaginas* x = list_find(tablaConCantidadDePaginas,buscarPid);
	return x;

}

int buscarCantidadDePaginas(int pid){
	sem_wait(&mutex_TablaDeCantidadDePaginas);
	filaTablaCantidadDePaginas* x= buscarFilaEnTablaCantidadDePaginas(pid);
	sem_post(&mutex_TablaDeCantidadDePaginas);
	if (x==NULL){
		return 0;
	}
	return x->cantidadDePaginasReales;

}


int asignarPaginasAUnProceso(int pid, int cantidadDePaginas){
//	int paginaMaxima = cantidadDePaginasDeUnProcesoDeUnProceso(pid);
	int i;


	int paginaMaxima = buscarCantidadDePaginas(pid);
	log_info(logMemoria,"[Asignar Paginas]-El proceso %d tiene %d paginas en memoria y pidio %d paginas nuevas",pid,paginaMaxima,cantidadDePaginas);

	sem_wait(&mutex_TablaDePaginasInvertida);
	for(i= 0 ;i < cantidadDePaginas; i++){
		if(reservarFrame(pid,paginaMaxima+i) == 0){
			log_error(logMemoria,"[Asignar Paginas]-No se pudo asignar paginas al proceso, se procede a liberar las paginas");
			int w ;
			for(w = 0;w<i;w++){
				liberarPagina(pid,paginaMaxima+w);
			}
			sem_post(&mutex_TablaDePaginasInvertida);
			return -1;
		}
	}
	sem_post(&mutex_TablaDePaginasInvertida);
	sem_wait(&mutex_TablaDeCantidadDePaginas);
	if(paginaMaxima == 0){
		log_warning(logMemoria,"[Asignar Paginas]-Hay que crear una nueva entrada a la tabla cantidad de paginas");

		filaTablaCantidadDePaginas * x = malloc(sizeof(filaTablaCantidadDePaginas));
		x->paginaMaxima= cantidadDePaginas;
		x->cantidadDePaginasReales = cantidadDePaginas;
		x->pid = pid;
		list_add(tablaConCantidadDePaginas,x);
	}
	else{
		log_info(logMemoria,"[Asignar Paginas]-Se encontro una página. Se procede a modificar el elemento de la lista de cantidad de paginas.");

		filaTablaCantidadDePaginas * elemento = buscarFilaEnTablaCantidadDePaginas(pid);
		elemento->paginaMaxima += cantidadDePaginas;
		elemento->cantidadDePaginasReales += cantidadDePaginas;
	}
	sem_post(&mutex_TablaDeCantidadDePaginas);
	return 1;
}

int finalizarUnPrograma(int pid){
	int paginas = buscarCantidadDePaginas(pid);
	if(paginas == 0){
		log_warning(logMemoria,"[Finalizar Programa]-No se encontraba ninguna pagina del proceso en memoria.");
		return 0;
	}
	int i;

	for(i = 0; i< paginas;i++){
		liberarPagina(pid,i);
	}
	sem_wait(&mutex_TablaDeCantidadDePaginas);
	bool buscarPid(filaTablaCantidadDePaginas* fila){
			return (fila->pid== pid);
	}
	list_remove_and_destroy_by_condition(tablaConCantidadDePaginas,buscarPid,free);//faltaria un destroyer decente
	sem_post(&mutex_TablaDeCantidadDePaginas);
	sem_wait(&mutex_cache);
	log_info(logMemoria,"[Finalizar Programa]-Se borra de la cache las paginas del proceso.");
	borraDeLaCache(pid);
	sem_post(&mutex_cache);
	return 1;
}

//Rutinas
void *rutinaKernel(void *arg){
		int listener = (int)arg;
		int aceptados[] = {Kernel}; // hacer un enum de esto
		int socketKernel;
		escuchar(listener); // poner a escuchar ese socket
		int id_clienteConectado;
		id_clienteConectado = aceptarConexiones(listener, &socketKernel, Memoria, &aceptados,1);
		if(id_clienteConectado == -1){
			log_error(logMemoria,"Se conecto otra cosa que no es un kernel");
			close(socketKernel);
		}
		else{
			log_info(logMemoria,"[Rutina Kernel] - Kernel conectado exitosamente\n");
			enviarMensaje(socketKernel,enviarTamanoPaginas,&sizeOfPaginas,sizeof(int));
			sem_post(&sem_isKernelConectado);//Semaforo que indica si solo hay un kernel conectado
			recibirMensajesMemoria(socketKernel);
		}



}

void *rutinaConsolaMemoria(void* x){
	size_t len = 0;
		char* mensaje = NULL;
		while(1){
				printf("\nIngrese Comando: \n");
				getline(&mensaje,&len,stdin);
				char** comandoConsola = NULL;
				comandoConsola = string_split(mensaje," ");
				if(strcmp(comandoConsola[0],"retardo") == 0){
					log_info(logMemoria,"Entramos en retardo");
					comandoConsola = string_split(comandoConsola[1], "\n");
					retardo = atoi(comandoConsola[0]); // el que hizo esto es un forro c:
					puts(comandoConsola[0]);
					comandoConsola = NULL;
				}
				if(strcmp(comandoConsola[0],"dump") == 0){
					if(strcmp(comandoConsola[1],"memoria\n")== 0){
						log_info(logMemoria,"Entramos en dump memoria");
						imprimirContenidoMemoria();
						continue;

					}
					if(strcmp(comandoConsola[1],"estructuras\n")== 0){
						log_info(logMemoria,"Entramos en dump estructuras administrativas");
						imprimirTablaDePaginasInvertida();
						continue;

					}
					if(strcmp(comandoConsola[1],"cache\n")== 0){
						log_info(logMemoria,"Entramos en dump cache");
						sem_wait(&mutex_cache);
						imprimirContenidoCache();
						sem_post(&mutex_cache);
						continue;

					}
						//imprimirEstructuras();
				}

				if(strcmp(comandoConsola[0],"flush\n") == 0){
					log_info(logMemoria,"Entramos en cache flush");
					cacheFlush();
					continue;
				}

				if(strcmp(comandoConsola[0],"size") == 0){
						if(strcmp(comandoConsola[1],"memoria\n")== 0){
							log_info(logMemoria,"Entramos en size memoria");
							log_info(logMemoria,"Su tamaño en frames: %d\n",cantidadDeMarcos);
							printf("Su tamaño en frames: %d\n",cantidadDeMarcos);

							int cantidadDeMarcosLibres = memoriaFramesLibres();
							log_info(logMemoria,"Marcos ocupados: %d\n", cantidadDeMarcosLibres);

							printf("Marcos ocupados: %d\n", cantidadDeMarcosLibres);
							log_info(logMemoria,"Marcos libres: %d\n", cantidadDeMarcos -cantidadDeMarcosLibres);
							printf("Marcos libres: %d\n", cantidadDeMarcos -cantidadDeMarcosLibres);

						}
						if(strcmp(comandoConsola[1],"PID:")== 0){
							log_info(logMemoria,"Entramos en size PID:");
							comandoConsola = string_split(comandoConsola[2], "\n");
							int pidPedido = atoi(comandoConsola[0]);
							log_info(logMemoria,"El proceso %d tiene %d\n",pidPedido,((filaTablaCantidadDePaginas*)buscarFilaEnTablaCantidadDePaginas(pidPedido))->cantidadDePaginasReales);
							printf("El proceso %d tiene %d\n",pidPedido,((filaTablaCantidadDePaginas*)buscarFilaEnTablaCantidadDePaginas(pidPedido))->cantidadDePaginasReales);
						}
				}
				liberarArray(comandoConsola);
			}
}

typedef struct{
int pid;
int cantPags;
}t_inicializarPrograma;

typedef struct{
	int pid;
	t_direccion direccion;
	void* buffer;
}t_almacenarBytes;

typedef struct {
	int pid;
	int cantPags;
}__attribute__((packed)) t_asignarPaginas;
//Funciones de Conexion
void recibirMensajesMemoria(void* arg){
	int socket = (int)arg;
	log_info(logMemoria,"[recibirMensajesMemoria]-Entramos a recibirMensajesMemoria con %d como socket", socket);
	void* stream;
	int operacion=1;//Esto es para que si lee 0, se termine el while.
	while (operacion){

		operacion = recibirMensaje(socket,&stream);
		log_info(logMemoria,"[recibirMensajesMemoria]-Llego la operacion %d", operacion);
		switch(operacion)
		{
				case inicializarPrograma:{ //inicializarPrograma
					t_inicializarPrograma* estructura = stream;
					log_info(logMemoria,"[Inicializar Programa] - El kernel quiere iniciar el pid %d con %d paginas iniciales", estructura->pid,estructura->cantPags);
					int x=1;
					if(asignarPaginasAUnProceso(estructura->pid,estructura->cantPags)==0){
						x=0;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));
					if(x){
						log_info(logMemoria,"[Inicializar Programa]-Se acepto asignar las paginas, por lo que se procede a iniciar el proceso", estructura->pid,estructura->cantPags);
					      int t;
					      char* contenidoPag ;
					      char* contenidoDeLaPaginaPosta= malloc(sizeOfPaginas);
					      int rta_escribir_Memoria;
					      sem_wait(&mutex_Memoria);
					      for(t=0;t<estructura->cantPags ;t++)
					      {
					    	  recibirMensaje(socket,&contenidoPag);
					    	  if(contenidoPag == NULL){
					    		  operacion = 0;
					    		  break;
					    	  }
					    	  log_info(logMemoria,"[Inicializar Programa]-Se recibio el contenido de  la pagina numero %d y se depositara el contenido: %s",t,(char*) contenidoPag);
					    	  memcpy(contenidoDeLaPaginaPosta,contenidoPag,sizeOfPaginas);
					    	  free(contenidoPag);
					    	  //rta_escribir_Memoria=escribirMemoriaPosta(estructura->pid,t,contenidoPag);
					    	  rta_escribir_Memoria=escribirMemoriaPosta(estructura->pid,t,contenidoDeLaPaginaPosta);
					    	  log_info(logMemoria,"[Inicializar Programa]-Se envio %d a kernel como respuesta",rta_escribir_Memoria);
					    	  enviarMensaje(socket,RespuestaBooleanaDeMemoria,&rta_escribir_Memoria,sizeof(int));
					      }
					      free(contenidoDeLaPaginaPosta);
					      sem_post(&mutex_Memoria);
					}


					break;
				}
				case solicitarBytes :{ //solicitar bytes de una pagina
					t_pedidoMemoria* estructura = stream;
					int respuesta= 1;

					void* contenidoDeLaPagina= solicitarBytesDeUnaPagina(estructura->id,estructura->direccion.page,estructura->direccion.offset,estructura->direccion.size);

					if(!contenidoDeLaPagina){
						respuesta =0;
						enviarMensaje(socket,RespuestaBooleanaDeMemoria,&respuesta,sizeof(int));
						 log_info(logMemoria,"[Solicitar Bytes]-Se envio %b a kernel como respuesta",respuesta);
					}
					else{

						enviarMensaje(socket,RespuestaBooleanaDeMemoria,&respuesta,sizeof(int));
						 log_info(logMemoria,"[Solicitar Bytes]-Se envio %b a kernel como respuesta",respuesta);
						 enviarMensaje(socket,lineaDeCodigo,contenidoDeLaPagina,estructura->direccion.size);
						 log_info(logMemoria,"[Solicitar Bytes]-Se envio el siguiente contenido a kernel: %s",(char*)contenidoDeLaPagina);
					}
					free(contenidoDeLaPagina);
					break;
				}

				case almacenarBytes://almacenarBytes en una pagina
				{
					t_escrituraMemoria estructura = deserializarAlmacenarBytes(stream);
					int x=1;
					if(almacenarBytesEnPagina(estructura.id,estructura.direccion.page,estructura.direccion.offset,estructura.direccion.size,estructura.valor)==0){
						x=0;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));
					log_info(logMemoria,"[Almacenar Bytes]-Se envio % a kernel como respuesta",x);

					break;
				}
				case asignarPaginas:{//PedirMasPaginas
					t_asignarPaginas* estructura = stream;
					int x;
					log_info(logMemoria,"[Asignar Paginas] Se quieren asignar %d paginas para el pid %d",estructura->cantPags,estructura->pid);
					if(asignarPaginasAUnProceso(estructura->pid,estructura->cantPags)==-1){
					  							x=0;
					}
					else{
					 						x=buscarCantidadDePaginas(estructura->pid) -1;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));
					log_info(logMemoria,"[Asignar Paginas]-Se envio %d a kernel como respuesta",x);
					break;
				}
				case finalizarPrograma:{//FinalzarPrograma

					int* pid = stream;
					int x=0;
					log_info(logMemoria,"[Finalizar Programa]-Entramos a Finalizar Programa. Pid: %d \n", *pid);
					if(finalizarUnPrograma(*pid)){
							x=1;
					}
					enviarMensaje(socket,RespuestaBooleanaDeMemoria,&x,sizeof(int));
					log_info(logMemoria,"[Finalizar Programa]-Se envio %d a kernel como respuesta",x);
					break;
				}
				case liberarUnaPagina:{
				     int* pid = stream;
				     int* pagina = stream+sizeof(int);
				     log_info(logMemoria,"[Liberar Pagina]-Entramos a liberar pagina.Pid:%d Pagina: %d\n", *pid,*pagina);
				     liberarPagina(*pid,*pagina);
				     break;
				}
				case 0:{
					log_info(logMemoria,"[Rip Kernel]-Se cerro el kernel");
					close(socket);
					break;
				}

				default:{
					perror("Error de comando");
				}
		}
		if(operacion != 0) free(stream);


	}
}
void *aceptarConexionesCpu( void *arg ){ // aca le sacamos el asterisco, porque esto era un void*
	sem_wait(&sem_isKernelConectado);
	int listener = (int)arg;
	int nuevoSocketCpu;
	int aceptados[] = {CPU};
	escuchar(listener); // poner a escuchar ese socket
	pthread_t hilo_nuevaCPU;

	log_info(logMemoria,"[AceptarConexionesCPU] - Ya se ha establecido Conexion con un Kernel, ahora si se pueden conectar CPUs: \n");

	while (1)
	{

		int id_clienteConectado;
		id_clienteConectado = aceptarConexiones(listener, &nuevoSocketCpu, Memoria, &aceptados,1);
		if(id_clienteConectado == -1){
			log_error(logMemoria,"Se rechazo una conexion invalida.");
				close(nuevoSocketCpu);
		}
		else{
			log_info(logMemoria,"[AceptarConexionesCPU] - Nueva CPU Conectada! Socket CPU: %d\n", nuevoSocketCpu);
			pthread_create(&hilo_nuevaCPU, NULL, recibirMensajesMemoria,  nuevoSocketCpu);
		}

	}
}



int main(void) {
	//
	/*char* a = "hola estoy de paro";
	void* pagina = malloc(100);
	sizeOfPaginas = 100;
	HeapMetadata h;
	h.isFree = true;
	h.size = 95;
	memcpy(pagina,&h,sizeof(HeapMetadata));
	void * prueba =  escribirMemoria(a,strlen(a),pagina);
	 prueba =  escribirMemoria(a,strlen(a),pagina);
	 prueba =  escribirMemoria(a,strlen(a),pagina);
	liberarMemoriaHeap(5,pagina);
	liberarMemoriaHeap(23,pagina);
	liberarMemoriaHeap(46,pagina);

*/
	logMemoria = log_create("Memoria.log","Memoria",0,0);
	printf("Inicializando Memoria.....\n\n");
	sem_init(&mutex_Memoria,0,1);
	sem_init(&mutex_TablaDeCantidadDePaginas,0,1);
	sem_init(&mutex_TablaDePaginasInvertida,0,1);
	sem_init(&mutex_cache,0,1);
	sem_init(&mutex_retardo,0,1);


	// ******* Configuracion de la Memoria a partir de un archivo
	tablaDeEntradasDeCache = list_create();
	tablaConCantidadDePaginas = list_create();
	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/Memoria/memoria.config");
	imprimirConfiguracion();
	retardo = getConfigInt("RETARDO_MEMORIA");
	sizeOfPaginas=getConfigInt("MARCO_SIZE");
	cantidadDeMarcos = getConfigInt("MARCOS");
	memoriaTotal = malloc(sizeOfPaginas*cantidadDeMarcos);
	int i;
	char* joaco;
	joaco = string_repeat(' ',sizeOfPaginas);
	for(i=0;i<cantidadDeMarcos;i++){//Chequearlo despues
		memcpy(memoriaTotal+i*sizeOfPaginas,joaco,sizeOfPaginas);
	}
	log_info(logMemoria,"Se inicializo la memoria");
	free(joaco); //No me toma el free por algun motivo __	 O	___
												//		  \__|__/

	iniciarTablaDePaginacionInvertida();
	log_info(logMemoria,"Se inicializo la tabla de paginacion invertida");
/*
	cacheMiss(1,2,joaco);
	cacheMiss(1,3,joaco);
	cacheMiss(1,4,joaco);
	cacheMiss(1,5,joaco);
	lineaCache* linea;
	linea =list_get(tablaDeEntradasDeCache,0);
	linea = list_get(tablaDeEntradasDeCache,1);

	linea =list_get(tablaDeEntradasDeCache,2);
	int x = list_size(tablaDeEntradasDeCache);
	/*	void* pagina = buscarEnLaCache(1,2);
	cacheMiss(1,3,hijodeputa);
	cacheMiss(2,2,hijodeputa);
	cacheMiss(2,2,hijodeputa);
	cacheMiss(2,2,hijodeputa);
	cacheMiss(3,2,hijodeputa);
	cacheMiss(3,2,hijodeputa);
	cacheMiss(3,2,hijodeputa);

	cacheMiss(4,2,hijodeputa);
	cacheMiss(4,2,hijodeputa);
	cacheMiss(4,2,hijodeputa);

	cacheMiss(5,2,hijodeputa);
	cacheMiss(5,2,hijodeputa);
	cacheMiss(5,2,hijodeputa);

	cacheMiss(23,2,hijodeputa);
	char* pagina2 = buscarEnLaCache(1,2);
//	cacheFlush();
	lineaCache linea;
	linea.pid = 2;
	linea.pagina = 2;
	//estabaEnCache(linea);
	pagina2 = buscarEnLaCache(2,2);
*/
	// PRUEBAS

//	asignarPaginasAUnProceso(1,2);

//	char* script = "begin\nvariables a, b\na = 3\nb = 5\na = b + 12\nprints l \"Hola Mundo\"\nend\n";
//	almacenarBytesEnPagina(1,1,0,strlen(script),(void*)script);
//	int x = 3;
//	almacenarBytesEnPagina(1,2,0,sizeof(int),(void*)&x);
//	finalizarUnPrograma(1);
//	asignarPaginasAUnProceso(5,3);

/*	char* stream = solicitarBytesDeUnaPagina(1,1,0,strlen(script));

	char* stream2 = solicitarBytesDeUnaPagina(1,312451516,0,strlen(script));
*/


	//printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[0].frame,tablaDePaginacionInvertida[0].pagina,tablaDePaginacionInvertida[0].pid);
	//printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[1].frame,tablaDePaginacionInvertida[1].pagina,tablaDePaginacionInvertida[1].pid);
	//printf("El frame es %i, la pagina es %i, y  la pagina del pid es %i\n",tablaDePaginacionInvertida[2].frame,tablaDePaginacionInvertida[2].pagina,tablaDePaginacionInvertida[2].pid);
	/*reservarFrame(0,0);
	reservarFrame(0,1);
	int x= 15481;
	char* mensaje1 = string_new();
	string_append(&mensaje1, "hola como andas");
	char* mensaje2 ="hola infeliz";
	almacenarBytesEnPagina(0,0,0,strlen(mensaje2),mensaje2);
	almacenarBytesEnPagina(0,1,25,strlen(mensaje1),mensaje1);
	almacenarBytesEnPagina(0,0,15,sizeof(int),&x);
	char* y = solicitarBytesDeUnaPagina(0, 0, 0, strlen(mensaje1));

	char* w = solicitarBytesDeUnaPagina(0, 1, 0, strlen(mensaje1));
	puts(w);
*/
	/*asignarPaginasAUnProceso(23,23);
	printf("cantidad de paginas : %d ", cantidadDePaginasDeUnProcesoDeUnProceso(23));
	size_t len = 0;

	char* mensaje = NULL;
	int* z = solicitarBytesDeUnaPagina(0, 0, 4, 4);
*/
	// ******* Declaración de la mayoria de las variables a utilizar

	pthread_t hilo_consolaMemoria;
	pthread_create(&hilo_consolaMemoria, NULL, rutinaConsolaMemoria,  NULL);
	log_info(logMemoria,"Se inicio el hilo consola de la memoria");

//	pthread_join(hilo_consolaMemoria, NULL);
	int listener;
	//char* mensajeRecibido= string_new();


	// ******* Conexiones obligatorias y necesarias

	listener = crearSocketYBindeo(getConfigString("PUERTO")); // asignar el socket principal
	log_info(logMemoria,"Se bindeo el listener");
	pthread_t hilo_AceptarConexionesCPU, hilo_Kernel;
	sem_init(&sem_isKernelConectado,0,0);

	pthread_create(&hilo_Kernel, NULL, rutinaKernel,  listener);
	log_info(logMemoria,"Se inicio el hilo que atiende al kernel");
	pthread_create(&hilo_AceptarConexionesCPU, NULL, aceptarConexionesCpu, listener);
	log_info(logMemoria,"Se inicio el hilo aceptar conexiones cpu");

	printf("\nMensaje desde la ram principal del programa!\n");
	pthread_join(hilo_Kernel, NULL);
	close(listener);
	liberarConfiguracion();

	free(memoriaTotal);
	log_info(logMemoria,"Se finalizo el modulo Memoria.");

	return EXIT_SUCCESS;

}

