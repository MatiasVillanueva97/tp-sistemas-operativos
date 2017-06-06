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
#include "commons/config.h"
#include "commons/string.h"
#include "commons/bitarray.h"
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.c"
#include "../../Nuestras/src/laGranBiblioteca/config.c"

#include "../../Nuestras/src/laGranBiblioteca/datosGobalesGenerales.h"

int cantidadDeBloques;
int bloqueSize;
t_bitarray* bitMap ;
t_list* listaDeArchivosAbiertos;
int socketKernel;
void* serializarEstaVerga(int size, int offset,char* path, char* buffer){
	void *contenido = malloc(4+strlen(path)+size+sizeof(int)*2);
	memcpy(contenido,&size,sizeof(int));
	memcpy(contenido+sizeof(int),buffer,size);
	int tamanoRuta= strlen(path);
	memcpy(contenido+sizeof(int)+size,&tamanoRuta,sizeof(int));
	memcpy(contenido+sizeof(int)*2+size,path,tamanoRuta);
	memcpy(contenido+sizeof(int)*2+tamanoRuta+size,&offset,sizeof(int));
	return contenido;
}
t_escritura deserializaEstaPoronga(void* contenido){
	t_escritura escritura;
	escritura.size= *(int*)contenido;
	escritura.buffer= malloc(escritura.size);
	memcpy(escritura.buffer,(contenido+sizeof(int)),escritura.size);
	int tamanoRuta =  *(int*)(contenido +sizeof(int)+escritura.size);
	escritura.path = malloc(tamanoRuta);
	memcpy(escritura.path,contenido+sizeof(int)*2+escritura.size,tamanoRuta);
	memcpy(&escritura.offset,contenido+sizeof(int)*2+tamanoRuta+escritura.size,sizeof(int));

	return escritura;
}
char* obtenerRutaTotal(char* path, char* tipoDeArchivo){

	char* pathTotal = string_new();
	char* w = string_duplicate(getConfigString("PUNTO_MONTAJE"));
	string_append_with_format(&pathTotal,"%s/%s/%s",w,tipoDeArchivo,path);
	return pathTotal;
}

bool validarArchivo(char* path){
	char* pathTotal = string_new();
	char* x = getConfigString("PUNTO_MONTAJE");
	string_append(&pathTotal,x);
	string_append(&pathTotal,"/Archivos");
	string_append(&pathTotal,path);
	FILE* archivo = fopen(pathTotal,"r");
	close(archivo);
	return archivo != NULL;
}
int devolverTamano(char* path){
	char* pathTotal = obtenerRutaTotal(path,"Archivos");
	t_config* configuracion =config_create(pathTotal);
	int w =config_get_int_value(configuracion,"TAMANIO");
	config_destroy(configuracion);
	free(pathTotal);
	return w;
}

int buscarPosicionLibre(){
	int i;
	for(i=0;i<cantidadDeBloques;i++){
		if(!bitarray_test_bit(bitMap,i)){
			return i;
		}
	}
	return -1;

}

void crearElArchivo(char* path){
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
	}
	}
	string_append(&rutaTotal,"/");
	string_append(&rutaTotal,*(directorios+i));
	FILE* archivo = fopen(rutaTotal,"w");
	int posicionDelBitMap = buscarPosicionLibre();
	bitarray_set_bit(bitMap,posicionDelBitMap);

	fprintf(archivo,"TAMANIO=123\nBLOQUES=[%d]\n",posicionDelBitMap);

	fclose(archivo);

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
		bitarray_clean_bit(bitMap,x-1);
	}
	return !remove(ruta);
}

void* leerBloque(FILE* archivo,int offset, int size){
	char* x = malloc(bloqueSize);
	fread(x,sizeof(char),bloqueSize,archivo);
	void* contenido = malloc(size);
	memcpy(contenido,x+offset,size);
	return contenido;
}
int* obtenerBloques (char* path){

	t_config *configuracion = config_create(obtenerRutaTotal(path,"Archivos"));

	char** bloquesEnString = config_get_array_value(configuracion,"BLOQUES");
	int i;
	int* bloques= malloc(4);
	int sizeAcumulado=0;
	for(i=0;bloquesEnString[i]!=NULL;i++){
		void* tmp = realloc(bloques,sizeAcumulado+4);
		bloques = tmp;
		*(bloques+i) = atoi(bloquesEnString[i]);
		sizeAcumulado+=4;
	}
	return bloques;

}
FILE* abrirArchivo (int numeroDeBloque){
	char* pathTotal = obtenerRutaTotal(string_itoa(numeroDeBloque),"Bloques");
	string_append(&pathTotal,".bin");
	return fopen(pathTotal,"r");
}
int cantidadDeElementosDeUnArray (int* array){
	int i = 0;
	while (array[i]!=NULL){
		i++;
	}
	return i;
}
void* obtenerDatos(char* path,int offset, int size){
	int *bloques = obtenerBloques(path);
	int bloqueInicial = offset / bloqueSize;
	if(offset+size > cantidadDeElementosDeUnArray(bloques)*bloqueSize){
		return NULL;
	}
	offset -= bloqueSize*bloqueInicial;
	int sizeLeido = 0;
	void* contenido = malloc(size);
	while(bloques[bloqueInicial] != NULL&&size!=0){ // revisarEsto
			FILE * archivo = abrirArchivo(*(bloques+bloqueInicial));
			if(offset+size <= bloqueSize){
				void* contenidoDelBloque= leerBloque(archivo,offset,size);
				memcpy(contenido+sizeLeido,contenidoDelBloque,size);
				//string_append(&contenido,leerBloque(archivo,offset,size));
				size=0;
				sizeLeido += size;
			}
			else{
				void* contenidoDelBloque=leerBloque(archivo,offset,bloqueSize-offset);
				memcpy(contenido+sizeLeido,contenidoDelBloque,bloqueSize-offset);
				size -= bloqueSize-offset;
				sizeLeido += bloqueSize-offset;
				offset=0;


			}
			fclose(archivo);
		bloqueInicial++;
	}
	free(bloques);
	return contenido;
}
void escribirBloque(int bloque,int offset,int size, void* buffer ){
	char* x =string_itoa(bloque);
	char* ruta =obtenerRutaTotal(x,"Bloques");
	string_append(&ruta,".bin");
	FILE* archivo = fopen(ruta,"w");
	fseek(archivo,offset,SEEK_SET);
	fwrite(buffer,size,1,archivo);	fclose(archivo);
}
char* crearStringBloques(int* bloques,int cantidad){
	char* x = string_new();
	int i;
	for(i=0;i<cantidad;i++){
		string_append_with_format(&x,",%d",*(bloques+i));
	}
	char* bloquesString =string_substring_from(x,1);
	free(x);
	return bloquesString;
}
bool guardarDatos(char* path,int offset, int size,void* buffer){
	int *bloques = obtenerBloques(path);
	int cantidadDeBloques = cantidadDeElementosDeUnArray(bloques);
	int bloqueInicialDondeQuiereEscribir = offset/bloqueSize;
	int bloqueQueQuiereEscribir = (size+offset) / bloqueSize;
	int i;
	int tamano= devolverTamano(path);
	int sizeLeido = 0;
	if(tamano < offset+size){
		tamano = offset+size;
	}
//	if(cantidadDeBloques >bloqueQueQuiereEscribir&&size+offset<=bloqueSize){
//		escribirBloque(*(bloques+bloqueQueQuiereEscribir),offset,size,buffer);
//	}
//	else{
		for(i=0;i<=bloqueQueQuiereEscribir&&sizeLeido!=size;i++){
			if(i>=cantidadDeBloques){
				int posicionLibre =buscarPosicionLibre();//Falta chequear errores aca
				if(posicionLibre == -1){
					return false;
				}
				bitarray_set_bit(bitMap,posicionLibre);
				*(bloques+i) = posicionLibre+1;//posibleRealloc
			}
			if(i>=bloqueInicialDondeQuiereEscribir){
				if(bloqueSize-offset > size-sizeLeido){
					void* contenido = malloc((size-sizeLeido));
					memcpy(contenido,buffer+sizeLeido,(size-sizeLeido));
					escribirBloque(*(bloques+i),offset,size-sizeLeido,contenido);
					size=0;
					free(contenido);
				}
				else{
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
	fprintf(archivoDondeEscriboLosNuevosDatos,"TAMANO= %d\nBLOQUES= [%s ]",tamano,crearStringBloques(bloques,cantidadDeBloques));
	fclose(archivoDondeEscriboLosNuevosDatos);
	return true;
}


void* configurarTodo(){
	t_config* configuracion = config_create(obtenerRutaTotal("Metadata.bin","Metadata"));
	if(configuracion == NULL){
		perror("Te falta el archivo de metadata.bin papu");
		exit(-1);
	}
	bloqueSize = config_get_int_value(configuracion,"TAMANIO_BLOQUES");
	cantidadDeBloques = config_get_int_value(configuracion,"CANTIDAD_BLOQUES");

	if(!strcmp(config_get_string_value(configuracion,"MAGIC_NUMBER"),"SADICA")){
		perror("Ingreso mal el punto de montaje");
		exit(-2);
	}
	FILE* archivoDeBitmap = fopen(obtenerRutaTotal("Bitmap.bin","Metadata"),"rb");
	int fd = fileno(archivoDeBitmap);
	void* bitmap2 = malloc(cantidadDeBloques);
	mmap(bitmap2, cantidadDeBloques, PROT_READ, MAP_SHARED, fd, (off_t) 0);
	bitMap= bitarray_create(bitmap2,cantidadDeBloques);
	int pos = buscarPosicionLibre();
	bitarray_set_bit(bitMap,pos);
	munmap( bitmap2, cantidadDeBloques );
	fclose(archivoDeBitmap);


}

void tramitarPeticionesDelKernel(int socketKernel){
	void* stream;
	int operacion=1;//Esto es para que si lee 0, se termine el while.
	while (operacion){
			operacion = recibirMensaje(socket,&stream);

			switch(operacion)
			{
					case validacionDerArchivo:{
						char* path = stream;
						bool respuesta = validarArchivo(path);
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						break;
					}
					case creacionDeArchivo:{
						char* path = stream;
						 crearElArchivo(path);
						bool respuesta = true;
						 enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						break;
					}
					case borrarArchivo:{
						char* path = stream;
						bool respuesta= borrarUnArchivo(path);
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						break;
					}
					case obtenerDatosDeArchivo:{
						t_pedidoFS pedido =*(t_pedidoFS*) stream;
						void *contenido= obtenerDatos(pedido.path,pedido.offset,pedido.size);
						if(contenido != NULL){
							enviarMensaje(socketKernel,respuestaConContenidoDeFs,&contenido,pedido.size);
						}
						else{
							int respuesta = false;
							enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,1);
						}
						break;
					}
					case guardarDatosDeArchivo:{
						t_escritura escritura =*(t_escritura*) stream;
						bool respuesta= guardarDatos(escritura.path,escritura.offset,escritura.size,escritura.buffer);
						enviarMensaje(socketKernel,respuestaBooleanaDeFs,&respuesta,sizeof(respuesta));
						break;
					}


					case 0:{
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
int main(void) {
	int x = 24141;
	void* w =
			serializarEstaVerga(sizeof(int),123,"hola.bin",&x);
	deserializaEstaPoronga(w);
	printf("Inicializando FileSystem.....\n\n");
	// ******* Declaración de la mayoria de las variables a utilizar
	listaDeArchivosAbiertos = list_create();

	// ******* Configuracion del FileSystem a partir de un archivo

	printf("Configuracion Inicial: \n");
	configuracionInicial("/home/utnso/workspace/tp-2017-1c-While-1-recursar-grupo-/FileSystem/fileSystem.config");
	imprimirConfiguracion();
	configurarTodo();

	int listener = crearSocketYBindeo(getConfigString("PUERTO"));
	escuchar(listener);
	int aceptados[] = {Kernel};
	if(aceptarConexiones(listener,&socketKernel,FileSystem,aceptados,1)!=Kernel){
			perror("Conectaste cualquier cosa menos un kernel pa");
	}
	tramitarPeticionesDelKernel(socketKernel);

	//t_config* configuracionFileSystem = config_create(getConfigString("PUNTO_MONTAJE"));
	//cantidadDeBloques = config_get_int_value(configuracionFileSystem,"CANTIDAD_BLOQUES");

	//crearElArchivo("passwords/hola.bin");
	//char* x = "hola como andashola como andashola como andashola como andashola como andas hola como andashola como andashola como andashola como andashola como andas";
	char* rutaDelArchivo = obtenerRutaTotal("passwords/hola2.bin","Archivos");
	//FILE* archivoDondeEscriboLosNuevosDatos = conseguirFileAbierto(rutaDelArchivo);

	//int x = 1235;
	//guardarDatos("passwords/hola.bin",48,strlen(x),x);
	//guardarDatos("passwords/hola.bin",64,strlen(x),x);
	//guardarDatos("passwords/hola.bin",128,strlen(x),x);
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
	return EXIT_SUCCESS;

}
