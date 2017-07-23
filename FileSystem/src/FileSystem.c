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
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include "commons/config.h"
#include "commons/string.h"
#include "commons/bitarray.h"
#include "commons/log.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

t_log* logFS;
int cantidadDeBloques;
int bloqueSize;
t_bitarray* bitMap ;
t_list* listaDeArchivosAbiertos;
int socketKernel;
int fd;
void* serializarPedidoFs(int size, int offset,char* path){
	void *contenido = malloc(4+strlen(path)+sizeof(int)*2);
	memcpy(contenido,&size,sizeof(int));
	memcpy(contenido+sizeof(int),&offset,sizeof(int));
	int tamanoRuta = strlen(path);
	memcpy(contenido+sizeof(int)*2,&tamanoRuta,sizeof(int));
	memcpy(contenido+sizeof(int)*3,path,strlen(path));
	return contenido;
}

t_pedidoFS deseralizarPedidoFs(void* contenido){
	t_pedidoFS pedido;
	pedido.size= *(int*)contenido;
	int tamanoRuta =  *(int*)(contenido +sizeof(int)*2);
	pedido.path = malloc(tamanoRuta);
	memcpy(&pedido.offset,contenido+sizeof(int),sizeof(int));
	memcpy(pedido.path,contenido+sizeof(int)*3,tamanoRuta);

	return pedido;
}

void* serializarEscribirMemoria(int size, int offset,char* path, char* buffer){
	void *contenido = malloc(4+strlen(path)+size+sizeof(int)*2);
	memcpy(contenido,&size,sizeof(int));
	memcpy(contenido+sizeof(int),buffer,size);
	int tamanoRuta= strlen(path);
	memcpy(contenido+sizeof(int)+size,&tamanoRuta,sizeof(int));
	memcpy(contenido+sizeof(int)*2+size,path,tamanoRuta);
	memcpy(contenido+sizeof(int)*2+tamanoRuta+size,&offset,sizeof(int));
	return contenido;
}
t_escritura deserializarEscribirMemoria(void* contenido){
	t_escritura escritura;
	escritura.offset = *(int*)contenido;
	escritura.size= *(int*)(contenido+sizeof(int));
	//Lineas muy ricas
	//como la del diego
	//rica rica
	//amarga
	escritura.buffer= malloc(escritura.size);
	memcpy(escritura.buffer,(contenido+sizeof(int)+sizeof(int)),escritura.size);
	int tamanoRuta =  *(int*)(contenido +sizeof(int)+escritura.size+sizeof(int));
	escritura.path = malloc(tamanoRuta);
	memcpy(escritura.path,contenido+sizeof(int)*2+escritura.size+sizeof(int),tamanoRuta);

	escritura.path[tamanoRuta-1] = '\0';

	return escritura;
}
char* obtenerRutaTotal(char* path, char* tipoDeArchivo){

	char* pathTotal = string_new();
	char* w = string_duplicate(getConfigString("PUNTO_MONTAJE"));
	string_append_with_format(&pathTotal,"%s/%s/%s",w,tipoDeArchivo,path);
	free(w);
	return pathTotal;
}

int validarArchivo(char* path){
	char* pathTotal = string_new();
	char* x = getConfigString("PUNTO_MONTAJE");
	string_append(&pathTotal,x);
	string_append(&pathTotal,"/Archivos");
	string_append(&pathTotal,path);
	FILE* archivo = fopen(pathTotal,"r");
	int w = archivo != NULL;
	if(w==1){
		fclose(archivo);
	}
	free(pathTotal);
	return w;
}
int devolverTamano(char* path){
	char* pathTotal = obtenerRutaTotal(path,"Archivos");
	t_config* configuracion =config_create(pathTotal);
	int tamano =config_get_int_value(configuracion,"TAMANO");
	log_info(logFS,"El archivo tiene un tamaño de: %d", tamano);
	config_destroy(configuracion);
	free(pathTotal);
	return tamano;
}

int buscarPosicionLibre(){
	int i;
	for(i=0;i<cantidadDeBloques;i++){
		if(!bitarray_test_bit(bitMap,i)){
			log_info(logFS,"[Buscar frame libre]-Se encontro la posicion %d en el bitmap libre.",i);
			return i;
		}
	}
	return -1;

}

int crearElArchivo(char* path){
	 DIR *dirp;
	char* rutaTotal=obtenerRutaTotal("","Archivos");
	//strcpy(rutaTotal,getConfigString("PUNTO_MONTAJE"));
	char** directorios = string_split(path,"/");
	int i;
	for(i=0;*(directorios+i+1) != NULL;i++){
		string_append(&rutaTotal,*(directorios+i));
		dirp = opendir(rutaTotal);
	 if (dirp == NULL){
		 mkdir(rutaTotal,0700);
			log_info(logFS,"[Crear Archivo]-Se creo el directorio %s",*(directorios+i));
	}
	string_append(&rutaTotal,"/");
	}
	string_append(&rutaTotal,*(directorios+i));
	FILE* archivo = fopen(rutaTotal,"w");
	int posicionDelBitMap = buscarPosicionLibre();
	if(posicionDelBitMap == -1){
		log_error(logFS,"[Crear Archivo]-No hay bloques disponibles ");
		return 0;
	}
	bitarray_set_bit(bitMap,posicionDelBitMap);

	fprintf(archivo,"TAMANO=0\nBLOQUES=[%d]\n",posicionDelBitMap);
	log_info(logFS,"[Crear Archivo]-Se creo en la ruta: %s el archivo con TAMANO=0\nBLOQUES=[%d]\n", rutaTotal ,posicionDelBitMap);
	fclose(archivo);
	free(rutaTotal);
	liberarArray(directorios);
	return 1;
	//falta la parte de escribir el archivo con el tamano de archivo(Seria 0) y asignarle el bloque en el archivo
}
int borrarUnArchivo(char* path){

	char* ruta;
	ruta = obtenerRutaTotal(path, "Archivos");
	t_config* configuracionFileSystem = config_create(ruta);
	char** bloques = config_get_array_value(configuracionFileSystem,"BLOQUES");
	int i;
	for(i = 0;*(bloques+i)!=NULL;i++){
		int x = atoi(*(bloques+i));
		bitarray_clean_bit(bitMap,x);
		log_info(logFS,"[Borrar Archivo]-Se desligo el bloque numero: %d ", x);
	}
	if (remove(ruta)==0){
		free(ruta);
		return 1;
	}
	free(ruta);
	return 0;
}

void* leerBloque(FILE* archivo,int offset, int size){
	char* x = malloc(bloqueSize);
	fread(x,sizeof(char),bloqueSize,archivo);
	void* contenido = malloc(size);
	memcpy(contenido,x+offset,size);
	log_info(logFS,"[Leer Bloque]-Se leyo el bloque y se obtuvo: %s ",(char*) contenido);
	free(x);
	return contenido;
}
int* obtenerBloques (char* path, int* cantidad){
	char* pathconfiguracion = obtenerRutaTotal(path,"Archivos");
	t_config *configuracion = config_create(pathconfiguracion);
	free(pathconfiguracion);
	if(configuracion == NULL){
		log_error(logFS,"[Obtener Bloques]-No se pudo crear la configuración");
		return 0;
	}
	char** bloquesEnString = config_get_array_value(configuracion,"BLOQUES");
	int i;
	int* bloques= malloc(cantidadDeElementosDeUnArray(bloquesEnString)*4);// Terriblemente turbio, pero bueno
	log_info(logFS,"[Obtener Bloques]-El archivo tiene %d bloques",cantidadDeElementosDeUnArray(bloquesEnString));//x2
	for(i=0;bloquesEnString[i]!=NULL;i++){
		*(bloques+i) = atoi(bloquesEnString[i]);
		log_info(logFS,"[Obtener Bloques]-Se obtuvo el bloque numero: %s ", bloquesEnString[i]);
	}
	*cantidad = i;
	log_info(logFS,"[Obtener Bloques]-Se obtuvieron los bloques del archivo %s", path);
	config_destroy(configuracion);
	liberarArray(bloquesEnString);
	return bloques;

}
FILE* aperturaDeArchivo (int numeroDeBloque){
	char* nombreDeBloque = string_itoa(numeroDeBloque);
	char* pathTotal = obtenerRutaTotal(nombreDeBloque,"Bloques");
	free(nombreDeBloque);
	string_append(&pathTotal,".bin");
	FILE* archivo = fopen(pathTotal,"r");
	free(pathTotal);
	return archivo;
}
int cantidadDeElementosDeUnArray (int* array){
	int i = 0;
	while (array[i]!=NULL){
		i++;
	}
	return i;
}
void* obtenerDatos(char* path,int offset, int size){
	int cantidad;
	int *bloques = obtenerBloques(path,&cantidad);
	int bloqueInicial = offset / bloqueSize;
	if(offset+size > devolverTamano(path)){
		log_error(logFS,"[Obtener datos]-Se pidio un offset mas grande que el tamano del archivo");
		return NULL;
	}
	offset -= bloqueSize*bloqueInicial;
	int sizeLeido = 0;
	void* contenido = malloc(size);
	while(size!=0){ // revisarEsto
			FILE * archivo = aperturaDeArchivo(*(bloques+bloqueInicial));
			if(archivo == NULL){
				log_error(logFS,"[Obtener datos]-El bloque %d.bin no existe más",bloqueInicial);
				char* pathTotal = obtenerRutaTotal(string_itoa(*(bloques+bloqueInicial)),"Bloques");
				string_append(&pathTotal,".bin");
				log_warning(logFS,"[Obtener datos]-Se procede a crear nuevamente el bloque para escribirlo",bloqueInicial);
				archivo= fopen(pathTotal,"w+");
				free(pathTotal);
			}
			else{
				log_info(logFS,"[Obtener datos]-Se abrio el archivo %d.bin",*(bloques+bloqueInicial));
			}
			if(offset+size <= bloqueSize){
				void* contenidoDelBloque= leerBloque(archivo,offset,size);
				memcpy(contenido+sizeLeido,contenidoDelBloque,size);
				//string_append(&contenido,leerBloque(archivo,offset,size));
				size=0;
				sizeLeido += size;
				free(contenidoDelBloque);
			}
			else{
				void* contenidoDelBloque=leerBloque(archivo,offset,bloqueSize-offset);

				memcpy(contenido+sizeLeido,contenidoDelBloque,bloqueSize-offset);
				log_info(logFS,"[Obtener datos]-Se copiaron %d bytes del bloque %d.bin",bloqueSize-offset,*(bloques+bloqueInicial));
				size -= bloqueSize-offset;
				log_info(logFS,"[Obtener datos]-Se copiaron %d bytes del bloque %d.bin",bloqueSize-offset,*(bloques+bloqueInicial));

				log_info(logFS,"[Obtener datos]-Quedan por leer %d bytes",size);
				sizeLeido += bloqueSize-offset;
				log_info(logFS,"[Obtener datos]-Ya se leyeron %d bytes",sizeLeido);

				offset=0;
				free(contenidoDelBloque);

			}
			fclose(archivo);
		bloqueInicial++;
	}
	free(bloques);

	log_info(logFS,"[Obtener datos]-Se obtuvo el buffer: %s",(char*) contenido);
	return contenido;
}
void escribirBloque(int bloque,int offset,int size, void* buffer ){
	char* x =string_itoa(bloque);
	char* ruta =obtenerRutaTotal(x,"Bloques");
	string_append(&ruta,".bin");
	FILE* archivo = fopen(ruta,"r+");
	fseek(archivo,offset,SEEK_SET);
	fwrite(buffer,size,1,archivo);
	log_info(logFS,"[Escribir Bloque]-Se escribio en el archivo: %s",(char*)buffer);
	free(ruta);
	free(x);
	fclose(archivo);
}
char* crearStringBloques(int* bloques,int cantidad){
	char* x = string_new();
	int i;
	for(i=0;i<cantidad;i++){
		string_append_with_format(&x,",%d",*(bloques+i));
	}
	char* bloquesString =string_substring_from(x,1);
	free(x);
	log_info(logFS,"[Crear String Bloques]-El conjunto de bloques nuevos es: [%s]",bloquesString);
	return bloquesString;
}
int guardarDatos(char* path,int offset, int size,void* buffer){
	int cantidadDeBloques;
	int *bloques = obtenerBloques(path,&cantidadDeBloques);
	if(bloques == NULL){
		log_error(logFS,"[Guardar Datos]-No existia el archivo con la path: %s",path);
		return 0;
	}
	int bloqueInicialDondeQuiereEscribir = offset/bloqueSize;
	int bloqueQueQuiereEscribir = (size+offset) / bloqueSize;
	int i;
	int tamano= devolverTamano(path);
	log_info(logFS,"[Guardar Datos]-El tamaño del archivo es de: %s", path);
	int sizeLeido = 0;
	if(tamano < offset+size){
		tamano = offset+size;
	}
		for(i=0;i<=bloqueQueQuiereEscribir&&sizeLeido!=size;i++){
			if(i>=cantidadDeBloques){
				int posicionLibre =buscarPosicionLibre();//Falta chequear errores aca
				if(posicionLibre == -1){
					log_error(logFS,"[Guardar Datos]-No hay bloques disponibles para seguir escribiendo.");

					return 0;
				}
				log_info(logFS,"[Guardar Datos]-Se tuvo que pedir otro bloque");
				int* tmp = realloc(bloques,cantidadDeBloques*sizeof(int)+4);
				if(tmp == NULL){
					log_error(logFS,"[Guardar Datos]-Error en un realloc. Falla en la memoria principal.");
					return 0;
				}

				else{
					bloques= tmp;
				}
				bitarray_set_bit(bitMap,posicionLibre);
				*(bloques+i) = posicionLibre;
				log_info(logFS,"[Guardar Datos]-Se asigna el bloque %d.bin",posicionLibre);
				cantidadDeBloques+=1;
				log_info(logFS,"[Guardar Datos]-Ahora el archivo tiene %d cantidad de bloques.",cantidadDeBloques);

			}
			if(i>=bloqueInicialDondeQuiereEscribir){
				log_info(logFS,"[Guardar Datos]-Llegue al bloque %d.bin para escribir", i);
				if(bloqueSize-offset > size-sizeLeido){

					log_info(logFS,"[Guardar Datos]-Es el ultimo bloque. Voy a escribir %d bytes",size-sizeLeido);
					void* contenido = malloc((size-sizeLeido));
					memcpy(contenido,buffer+sizeLeido,(size-sizeLeido));
					escribirBloque(*(bloques+i),offset,size-sizeLeido,contenido);
					size=0;
					free(contenido);
				}
				else{
					log_info(logFS,"[Guardar Datos]-Todavia me faltan bloques para escribir, este es un bloque intermedio i.bin ");
					void* contenido = malloc(bloqueSize-offset);
					memcpy(contenido,buffer+sizeLeido,bloqueSize-offset);
					escribirBloque(*(bloques+i),offset,bloqueSize-offset,contenido);
					sizeLeido += bloqueSize-offset;
					free(contenido);
				}

			}

			if(offset>bloqueSize){
				offset -= bloqueSize;
			}
			else{
				offset = 0;
			}
		}
		//Falta escribirlo ACA o al final en el archivo del archivoPropiamenteDicho
//	}

	char* rutaDelArchivo = obtenerRutaTotal(path,"Archivos");
	FILE* archivoDondeEscriboLosNuevosDatos =fopen(rutaDelArchivo,"w");
	char* bloquesAEscribir = crearStringBloques(bloques,cantidadDeBloques);
	fprintf(archivoDondeEscriboLosNuevosDatos,"TAMANO=%d\nBLOQUES=[%s]",tamano,bloquesAEscribir);
	log_info(logFS,"[Guardar Datos]-Se escribio el archivo. Su tamaño es: %d,y tiene los siguientes BLOQUES: [%s]",tamano,bloquesAEscribir);
	fclose(archivoDondeEscriboLosNuevosDatos);
	free(bloquesAEscribir);
	free(rutaDelArchivo);
	free(bloques);
	return 1;
}


void* configurarTodo(){
	char* rutaConfig = obtenerRutaTotal("Metadata.bin","Metadata");
	t_config* configuracion = config_create(rutaConfig);
	free(rutaConfig);

	if(configuracion == NULL){
		log_error(logFS,"[Configurar Todo]-No se obtuvo el archivo Metadata.bin, por lo que se procede a cerrar el FileSytem");
		perror("Te falta el archivo de metadata.bin papu");
		exit(-1);
	}
	log_info(logFS,"Se cargo el archivo Metadata.bin");

	bloqueSize = config_get_int_value(configuracion,"TAMANIO_BLOQUES");
	cantidadDeBloques = config_get_int_value(configuracion,"CANTIDAD_BLOQUES");
	log_info(logFS,"[Configurar Todo]-Tamaño de bloque: %d",bloqueSize);
	log_info(logFS,"[Configurar Todo]-Cantidad de bloques: %d",cantidadDeBloques);
	char* rutaDeDirectorioBloques = string_duplicate(getConfigString("PUNTO_MONTAJE"));
	string_append(&rutaDeDirectorioBloques,"/Bloques");
	DIR* directorio=opendir(rutaDeDirectorioBloques);
	if(directorio==NULL){
		mkdir(rutaDeDirectorioBloques,0700);
		log_info(logFS,"[Configurar Todo]-Creacion del directorio de Bloques");
	}
	else{
		free(directorio);
	}
	free(rutaDeDirectorioBloques);
	char* rutaDeDirectorioArchivos = string_duplicate(getConfigString("PUNTO_MONTAJE"));
	string_append(&rutaDeDirectorioArchivos,"/Archivos");
	DIR* directorio2 = opendir(rutaDeDirectorioArchivos);
	if(directorio2==NULL){
		mkdir(rutaDeDirectorioArchivos,0700);
		log_info(logFS,"[Configurar Todo]-Creacion del directorio de Archivos");
	}
	else{
		closedir(directorio2);
	}
	free(rutaDeDirectorioArchivos);

	int i;
	char* nombre;
	for(i = 0;i< cantidadDeBloques;i++){
		nombre = string_new();
		char* nombreBloque = string_itoa(i);
		string_append(&nombre,nombreBloque);
		string_append(&nombre,".bin");
		char* rutaTotal = obtenerRutaTotal(nombre,"Bloques");
		FILE* Bloque = fopen(rutaTotal,"r+");
		if(Bloque == NULL){
			log_info(logFS,"[Configurar Todo]-Se tuvo que crear el bloque %d.bin",i);
			Bloque = fopen(rutaTotal,"w+");
		}
		fclose(Bloque);
		free(nombreBloque);
		free(rutaTotal);
		free(nombre);
	}
	if(!strcmp(config_get_string_value(configuracion,"MAGIC_NUMBER"),"SADICA")){
		perror("Ingreso mal el punto de montaje");
		exit(-2);
	}
	int opcion;
	printf("Ingrese 1 si quiere crear de 0 el bitmap. Sino, ingrese 0:\n");
	scanf("%d",opcion);
	if(opcion == 1){
		char* rutaDelArchivoDeBitmap2 = obtenerRutaTotal("Bitmap.bin","Metadata");
		remove(rutaDelArchivoDeBitmap2);
		free(rutaDelArchivoDeBitmap2);
	}
	char* rutaDelArchivoDeBitmap = obtenerRutaTotal("Bitmap.bin","Metadata");
	FILE* archivoDeBitmap = fopen(rutaDelArchivoDeBitmap,"r+");
	if(archivoDeBitmap == NULL){
		log_info(logFS,"[Configurar Todo]-Se tuvo que crear un bitmap nuevo, ya que no habia un bitmap anterior.");
		archivoDeBitmap =fopen(rutaDelArchivoDeBitmap,"w+");
		int cantidad = ceil(((double)cantidadDeBloques)/8.0);
		char* cosa = string_repeat('\0',cantidad);
		fwrite(cosa,1,cantidadDeBloques,archivoDeBitmap);
		free(cosa);
	}
	else{
		log_info(logFS,"Se cargo el bitmap al FileSystem");
	}
	fclose(archivoDeBitmap);
	fd = open(rutaDelArchivoDeBitmap,O_RDWR);
	free(rutaDelArchivoDeBitmap);

	struct stat scriptMap;
	fstat(fd, &scriptMap);

	char* bitmap2 = mmap(0, scriptMap.st_size, PROT_WRITE |PROT_READ , MAP_SHARED, fd,  0);
	bitMap= bitarray_create(bitmap2, ceil(((double)cantidadDeBloques)/8.0));
	log_info(logFS,"[Configurar Todo]-Se creo correctamente el bitmap");
	config_destroy(configuracion);
}

void tramitarPeticionesDelKernel(int socketKernel){
	void* stream;
	int operacion=1;//Esto es para que si lee 0, se termine el while.
	while (operacion){
			operacion = recibirMensaje(socketKernel,&stream);
			log_info(logFS,"[Tramitar Peticiones Del Kernel]-Llego un mensaje del kernel con operacion %d",operacion);
			switch(operacion)
			{
					case validacionDerArchivo:{
						log_info(logFS,"[Validar Archivo]-El kernel quiere validar un archivo");
						char* path = stream;
						int respuesta;
						if(path == NULL){
							respuesta = 0;
							log_error(logFS,"[Validar Archivo]- Me llego un NULL.");
						}else{
							respuesta = validarArchivo(path);
						}
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						log_info(logFS,"La respuesta enviada al kernel es: %d ", respuesta);
						break;
					}
					case creacionDeArchivo:{
						char* path = stream;
						int respuesta;
						if(path == NULL){
							respuesta = 0;
							log_error(logFS,"[Validar Archivo]- Me llego un NULL.");
						}else{
							log_info(logFS,"El kernel quiere crear un archivo con el siguiente path: ", path);
							respuesta = crearElArchivo(path);
						}
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						log_info(logFS,"La respuesta enviada al kernel es: %b ", respuesta);
						break;
					}
					case borrarArchivo:{
						char* path = stream;
						int respuesta;
						if(path == NULL){
							respuesta = 0;
							log_error(logFS,"[Validar Archivo]- Me llego un NULL.");
						}
						else{log_info(logFS,"[Validar Archivo]-El kernel quiere crear un archivo con el siguiente path: %s",path);
						respuesta= borrarUnArchivo(path);
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(int));
						log_info(logFS,"[Validar Archivo]-La respuesta enviada al kernel es: %d ", respuesta);
						break;
					}
					case obtenerDatosDeArchivo:{

						t_pedidoFS pedido = deseralizarPedidoFs(stream);
						log_info(logFS,"[Obtener datos de Archivo]-El kernel quiere obtener %d bytes desde %d, de la path: %s",pedido.size,pedido.offset,pedido.path);
						void *contenido= obtenerDatos(pedido.path,pedido.offset,pedido.size);
						if(contenido != NULL){
							enviarMensaje(socketKernel,respuestaConContenidoDeFs,contenido,pedido.size);
							log_info(logFS,"[Obtener datos de Archivo]-Se encontraron los datos y se envia al kernel: %s",(char*) contenido);
						}
						else{
							bool respuesta = false;
							log_info(logFS,"[Obtener datos de Archivo]-La respuesta enviada al kernel es: %b ", respuesta);
							enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,1);
						}
						free(contenido);
						free(pedido.path);
						break;
					}
					case guardarDatosDeArchivo:{
						t_escritura escritura = deserializarEscribirMemoria(stream);
						log_info(logFS,"[Guardar datos de Archivo]-El kernel quiere escribir %d bytes desde %d, de la path: %s con el siguiente contenido: %s",escritura.size,escritura.offset,escritura.path,(char*)escritura.buffer);
						int respuesta= guardarDatos(escritura.path,escritura.offset,escritura.size,escritura.buffer);
						log_info(logFS,"[Guardar datos de Archivo]-Se envia al kernel: %d", respuesta);
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						free(escritura.buffer);
						free(escritura.path);
						break;
					}


					case 0:{
						log_info(logFS,"Se cerro el kernel y se procede a cerrar el socket");
						close(socketKernel);
						break;
					}

					default:{
						log_info(logFS,"El kernel mando un codigo de operacion invalido");
						perror("Error de comando");
					}
			}


		}
			if(operacion != 0) free(stream);
	}
}
int main(void) {

	logFS = log_create("FileSystem.log","FileSystem",0,0);
	log_info(logFS,"Inicializando FileSystem.....\n\n");


	// ******* Configuracion del FileSystem a partir de un archivo
	log_info(logFS,"Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/FileSystem/fileSystem.config");
	imprimirConfiguracion();
	configurarTodo();

	int listener = crearSocketYBindeo(getConfigString("PUERTO"));
	escuchar(listener);
	int aceptados[] = {Kernel};
	if(aceptarConexiones(listener,&socketKernel,FileSystem,aceptados,1)!=Kernel){
		log_error(logFS,"Configuracion Inicial: \n");
		exit(-1);
	}
	tramitarPeticionesDelKernel(socketKernel);

	liberarConfiguracion();
	close(socketKernel);
	close(fd);
	free(bitMap->bitarray);
	bitarray_destroy(bitMap);

/*
	crearElArchivo("passwords/hola.bin");
	char* x = "Hola";
	guardarDatos("passwords/hola.bin",48,strlen(x)+1,x);
	guardarDatos("passwords/hola.bin",64,strlen(x)+1,x);
	guardarDatos("passwords/hola.bin",128,strlen(x)+1,x);
	puts(obtenerDatos("passwords/hola.bin",128,strlen(x)+1));
// bitarray_set_bit(bitMap,2);
//	bitarray_set_bit(bitMap,1);
//	obtenerRutaTotal("hola.bin","Archivos");

	// ******* Conexiones obligatorias y necesarias

/*
	listener = crearSocketYBindeo(getConfigString("PUERTO"));
	escuchar(listener);

	sin_size = sizeof their_addr;

	if ((nuevoSocket = accept(listener, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
		perror("Error en el Accept");
	}

	inet_ntop(their_addr.ss_family, getSin_Addr((struct sockaddr *) &their_addr), ip, sizeof ip); // para poder imprimir la ip del server

	printf("Conexion con %s\n", ip);


	close(listener); // child doesn't need the listener

	if ((rta_handshake = handshakeServidor(nuevoSocket, FileSystem, aceptados,1)) == -1) {
		perror("Error con el handshake: -1");
		close(nuevoSocket);
	}
	printf("Conexión exitosa con el Kernel(%i)!!\n",rta_handshake);

/*
	char* mensaje;
	if(recibirMensaje(nuevoSocket,(void *)mensajeRecibido)==-1){
		perror("Error en el Reciv");
	}
	printf("Mensaje desde el Kernel: %s\n\n", mensaje);
*/
//	close(nuevoSocket);  // parent doesn't need this

	//DIR* directorio =opendir("/home/utnso");
	//main2();
/*	obtenerDatos("hola.bin",10,55);
	crearElArchivo("passwords/hola.bin");
	int w = devolverTamano("passwords/hola.bin");
	borrarUnArchivo("hola.bin");
	//existeElArchivo("/hola.bin");
	liberarConfiguracion();
	// free(mensajeRecibido); esto tira violacion de segmento
	*/
	log_destroy(logFS);
	close(fd);
	return EXIT_SUCCESS;

}
