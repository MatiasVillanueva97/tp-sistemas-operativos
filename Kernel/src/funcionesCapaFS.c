/*
 * funcionesCapaFS.c
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#include "funcionesCapaFS.h"


int recibirBooleanoDeFS(int pid, int exitcode);
int agregarNuevaAperturaDeArchivo(char* path, int pid, char* flags);
int crearArchivo(char* path, int pid, char* flags);
ENTRADA_DE_TABLA_DE_PROCESO* getEntradaEnTablaProceso(int pid, int FD);

ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* encontrarElDeIgualPid(int pid) {
	ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux;
	bool sonDeIgualPid(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * elementos) {
		return elementos->pid == pid;
	}
	sem_wait(&mutex_tablaGlobalDeArchivosDeProcesos);
	aux = list_find(tablaGlobalDeArchivosDeProcesos, (void *) sonDeIgualPid);
	sem_post(&mutex_tablaGlobalDeArchivosDeProcesos);
	return aux;
}

ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* encontrarElDeIgualPath(char* path) {
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * auxiliar;
	bool sonDeIgualPath(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elemento) {
		return !strcmp(elemento->path, path);
	}
	sem_wait(&mutex_tablaGlobalDeArchivos);
	auxiliar = list_find(tablaGlobalDeArchivos, sonDeIgualPath);
	sem_post(&mutex_tablaGlobalDeArchivos);
	return auxiliar;
}

bool archivoExiste(char* path) {
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * auxiliar;
	auxiliar = encontrarElDeIgualPath(path);
	return auxiliar != NULL;
}
int posicionEnTablaGlobalDeArchivos(
		ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* auxiliar) {
	int i = -1;
	bool buscarPosicion(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * elemento) {
		i++;
		return !strcmp(elemento->path, auxiliar->path);
	}
	sem_wait(&mutex_tablaGlobalDeArchivos);
	auxiliar = list_find(tablaGlobalDeArchivos, (void *) buscarPosicion);
	sem_post(&mutex_tablaGlobalDeArchivos);
	return i;
}

int posicionEnTablaGlobalArchivosDeProceso(
		ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* auxiliar) {
	int i = -1;
	bool buscarPosicion(ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* elemento) {
		i++;
		return elemento->pid == auxiliar->pid;
	}
	sem_wait(&mutex_tablaGlobalDeArchivosDeProcesos);
	auxiliar = list_find(tablaGlobalDeArchivosDeProcesos,
			(void *) buscarPosicion);
	sem_post(&mutex_tablaGlobalDeArchivosDeProcesos);
	return i;
}

void agregarATablaDeProceso(int globalFD, char* flags, t_list* tablaProceso) {
	ENTRADA_DE_TABLA_DE_PROCESO* nuevaEntradaProceso = malloc(
			sizeof(ENTRADA_DE_TABLA_DE_PROCESO));
	nuevaEntradaProceso->globalFD = globalFD;
	nuevaEntradaProceso->flags = string_duplicate(flags);
	nuevaEntradaProceso->offset = 0;
	list_add(tablaProceso, nuevaEntradaProceso);
}

void agregarATablaGlobalDeArchivos(char* path, int aperturas) {
	ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS * nuevaEntrada = malloc(
			sizeof(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS));
	nuevaEntrada->path = string_duplicate(path);
	nuevaEntrada->cantidad_aperturas = aperturas;
	sem_wait(&mutex_tablaGlobalDeArchivos);
	list_add(tablaGlobalDeArchivos, nuevaEntrada);
	sem_post(&mutex_tablaGlobalDeArchivos);
}
void agregarATablaGlobalDeArchivosDeProcesos(int pid, t_list* tablaProceso) {
	ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO * nuevaEntrada = malloc(sizeof(int) + 4);
	nuevaEntrada->pid = pid;
	nuevaEntrada->tablaProceso = tablaProceso;
	sem_wait(&mutex_tablaGlobalDeArchivosDeProcesos);
	list_add(tablaGlobalDeArchivosDeProcesos, nuevaEntrada);
	sem_post(&mutex_tablaGlobalDeArchivosDeProcesos);
}

void* serializarPedidoFs(int size, int offset, char* path) {
	void *contenido = malloc(4 + strlen(path) + sizeof(int) * 2 + 1);
	memcpy(contenido, &size, sizeof(int));
	memcpy(contenido + sizeof(int), &offset, sizeof(int));
	int tamanoRuta = strlen(path) + 1;
	memcpy(contenido + sizeof(int) * 2, &tamanoRuta, sizeof(int));
	memcpy(contenido + sizeof(int) * 3, path, tamanoRuta);
	return contenido;
}

//Espero que esto ande
t_crearArchivo deserializarCrearArchivo(void* stream) {
	t_crearArchivo mensaje;
	memcpy(&mensaje.pid, stream, sizeof(int));
	int tamanoContenido;
	memcpy(&tamanoContenido, stream + sizeof(int), sizeof(int));
	char* contenidoAuxiliar = malloc(tamanoContenido);
	memcpy(contenidoAuxiliar, stream + sizeof(int) * 2, tamanoContenido);
	mensaje.flags = contenidoAuxiliar;
	int tamanoContenido2;
	memcpy(&tamanoContenido2, stream + sizeof(int) * 2 + tamanoContenido,
			sizeof(int));
	char* contenidoAuxiliar2 = malloc(tamanoContenido2);
	memcpy(contenidoAuxiliar2, stream + sizeof(int) * 3 + tamanoContenido,
			tamanoContenido2);
	mensaje.path = contenidoAuxiliar2;
	return mensaje;
}

void* serializarEscribirMemoria(int size, int offset, char* path, char* buffer) {
	void *contenido = malloc(
			sizeof(int) + strlen(path) + 1 + size + sizeof(int) * 2);
	memcpy(contenido, &offset, sizeof(int));
	memcpy(contenido + sizeof(int), &size, sizeof(int));
	memcpy(contenido + sizeof(int) + sizeof(int), buffer, size);
	int tamanoRuta = strlen(path) + 1;
	memcpy(contenido + sizeof(int) + size + sizeof(int), &tamanoRuta,
			sizeof(int));
	memcpy(contenido + sizeof(int) * 2 + size + sizeof(int), path, tamanoRuta);
	return contenido;
}



void finalizarPid(int pid, int exitCode) {
	 sem_wait(&mutex_listaProcesos);
 	 PROCESOS * proceso = buscarProceso(pid);
 	 proceso->pcb->estadoDeProceso = exec;
 	 proceso_Finalizar(pid,exitCode);
 	 sem_post(&mutex_listaProcesos);
}


void liberarEntradaDeTablaProceso(ENTRADA_DE_TABLA_DE_PROCESO* entrada) {
	free(entrada->flags);
	free(entrada);
}

void liberarEntradaTablaGlobalDeArchivos(
		ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada) {
	free(entrada->path);
	free(entrada);
}

void liberarEntradaTablaGlobalDeArchivosDeProceso(
		ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO*entrada) {
	list_clean_and_destroy_elements(entrada->tablaProceso,
			liberarEntradaDeTablaProceso);
	free(entrada);
}

bool liberarRecursosArchivo(int pid) {
	ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* entrada_a_eliminar =	encontrarElDeIgualPid(pid);
	int i;
	int tamanoTabla = list_size(entrada_a_eliminar->tablaProceso);

	if (tamanoTabla > 3) {
		for (i = 3; i < tamanoTabla; i++) {
			ENTRADA_DE_TABLA_DE_PROCESO* entrada_de_tabla_proceso = list_get(entrada_a_eliminar->tablaProceso, i);
			sem_wait(&mutex_tablaGlobalDeArchivos);
			ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo = list_get(tablaGlobalDeArchivos, entrada_de_tabla_proceso->globalFD);
			sem_post(&mutex_tablaGlobalDeArchivos);
			borrarEnTablaGlobalDeArchivo( entrada_de_archivo,entrada_de_tabla_proceso->globalFD);
			}
	}
	int posicion = posicionEnTablaGlobalArchivosDeProceso(entrada_a_eliminar);
	sem_wait(&mutex_tablaGlobalDeArchivosDeProcesos);
	list_remove_and_destroy_element(tablaGlobalDeArchivosDeProcesos, posicion,
			liberarEntradaTablaGlobalDeArchivosDeProceso);
	sem_post(&mutex_tablaGlobalDeArchivosDeProcesos);
	return true;
}

int borrarArchivoPermanente(t_archivo estructura) {

	ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);

	ENTRADA_DE_TABLA_DE_PROCESO* entrada_a_evaluar = list_get(aux->tablaProceso,estructura.fileDescriptor);

	int rtaCPU = 0;

	if (entrada_a_evaluar != NULL) {
		sem_wait(&mutex_tablaGlobalDeArchivos);
		ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo = list_get(tablaGlobalDeArchivos, entrada_a_evaluar->globalFD);
		sem_post(&mutex_tablaGlobalDeArchivos);
		if (entrada_de_archivo != NULL) {
			if (entrada_de_archivo->cantidad_aperturas == 1) {
				enviarMensaje(socketFS, borrarArchivo, entrada_de_archivo->path,strlen(entrada_de_archivo->path) + 1);
				rtaCPU = recibirBooleanoDeFS(estructura.pid,borradoFallidoPorFileSystem);
			} else {
				finalizarPid(estructura.pid, borradoFallidoOtroProcesoLoEstaUtilizando); //No se pudo borrar, porque otro proceso lo esta usando
			}
		} else {
			finalizarPid(estructura.pid, archivoInexistente);
		}
	} else {
		finalizarPid(estructura.pid, falloEnElFileDescriptor);
	}
	return rtaCPU;
}

void borrarEnTablaGlobalDeArchivo(ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo, int globalFD) {
	entrada_de_archivo->cantidad_aperturas--;
	if (entrada_de_archivo->cantidad_aperturas == 0) {
		sem_wait(&mutex_tablaGlobalDeArchivosDeProcesos);
		list_remove_and_destroy_element(tablaGlobalDeArchivos, globalFD,liberarEntradaTablaGlobalDeArchivos);
		sem_post(&mutex_tablaGlobalDeArchivosDeProcesos);


	}
}
	int cerrarArchivoPermanente(t_archivo estructura) {

		int rtaCPU = 0;
		ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(estructura.pid);
		ENTRADA_DE_TABLA_DE_PROCESO* entrada_de_tabla_proceso = list_get(aux->tablaProceso, estructura.fileDescriptor);
		if (entrada_de_tabla_proceso != NULL) {

			sem_wait(&mutex_tablaGlobalDeArchivos);
			ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo = list_get(tablaGlobalDeArchivos, entrada_de_tabla_proceso->globalFD);
			sem_post(&mutex_tablaGlobalDeArchivos);

			if (entrada_de_archivo != NULL) {
				borrarEnTablaGlobalDeArchivo(entrada_de_archivo, entrada_de_tabla_proceso->globalFD);
				list_remove_and_destroy_element(aux->tablaProceso,estructura.fileDescriptor, free);
				rtaCPU = 1;
			} else {
				finalizarPid(estructura.pid, archivoInexistente); //Por ahora
			}
		} else {
			finalizarPid(estructura.pid, falloEnElFileDescriptor);
		}
		return rtaCPU;
	}

	int escribirEnUnArchivo(t_mensajeDeProceso msj) {
		int rtaCPU = 0;

		ENTRADA_DE_TABLA_DE_PROCESO* entrada_de_tabla_proceso = getEntradaEnTablaProceso(msj.pid, msj.descriptorArchivo);

		if (entrada_de_tabla_proceso != NULL) {
			sem_wait(&mutex_tablaGlobalDeArchivos);
			ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo = list_get(tablaGlobalDeArchivos, entrada_de_tabla_proceso->globalFD);
			sem_post(&mutex_tablaGlobalDeArchivos);
			if (entrada_de_archivo != NULL) {
				if (string_contains(entrada_de_tabla_proceso->flags, "w")) {
					void* pedidoEscritura = serializarEscribirMemoria(msj.tamanio, entrada_de_tabla_proceso->offset,entrada_de_archivo->path, msj.mensaje);
					enviarMensaje(socketFS, guardarDatosDeArchivo,pedidoEscritura,sizeof(int) + strlen(entrada_de_archivo->path)+ msj.tamanio+ sizeof(int) * 2);
					free(pedidoEscritura);
					rtaCPU = recibirBooleanoDeFS(msj.pid,escrituraDenegadaPorFileSystem);
				} else {
					finalizarPid(msj.pid, escrituraDenegadaPorFaltaDePermisos);
				}
			} else {
				finalizarPid(msj.pid, archivoInexistente);
			}
		} else {
			finalizarPid(msj.pid, falloEnElFileDescriptor);
		}
		return rtaCPU;
	}

	int moverUnCursor(t_moverCursor estructura) {

		int rtaCPU = 0;

		ENTRADA_DE_TABLA_DE_PROCESO* entrada_de_tabla_proceso = getEntradaEnTablaProceso(estructura.pid, estructura.fileDescriptor);

		if (entrada_de_tabla_proceso != NULL) {
			sem_wait(&mutex_tablaGlobalDeArchivos);
			ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo = list_get(tablaGlobalDeArchivos, entrada_de_tabla_proceso->globalFD);
			sem_post(&mutex_tablaGlobalDeArchivos);
			if (entrada_de_archivo != NULL) {
				entrada_de_tabla_proceso->offset = estructura.posicion;
				rtaCPU = 1;
			} else {
				finalizarPid(estructura.pid, archivoInexistente);
			}
		} else {
			finalizarPid(estructura.pid, falloEnElFileDescriptor);
		}
		return rtaCPU;
	}

	void abrirArchivoPermanente(bool existeArchivo, t_crearArchivo estructura,
			int socketCPU) {

		int rtaCPU = 0;
		if (existeArchivo) {
			int fileDescriptor = agregarNuevaAperturaDeArchivo(estructura.path,estructura.pid, estructura.flags);
			enviarMensaje(socketCPU, envioDelFileDescriptor, &fileDescriptor,sizeof(int));
		} else if (string_contains(estructura.flags, "c")) {
			int fileDescriptor = crearArchivo(estructura.path, estructura.pid,estructura.flags);
			if (fileDescriptor) {
				enviarMensaje(socketCPU, envioDelFileDescriptor,&fileDescriptor, sizeof(int));
			} else {
				finalizarPid(estructura.pid, noSeCreoElArchivoPorFileSystem);
				enviarMensaje(socketCPU, respuestaBooleanaKernel, &rtaCPU,sizeof(int));
			}
		} else {
			finalizarPid(estructura.pid, archivoInexistente);
			enviarMensaje(socketCPU, respuestaBooleanaKernel, &rtaCPU,sizeof(int));
		}
	}

	void leerEnUnArchivo(t_lectura estructura, int socketCPU) {

		int rtaCPU = 0;

		ENTRADA_DE_TABLA_DE_PROCESO* entrada_de_tabla_proceso = getEntradaEnTablaProceso(estructura.pid, estructura.fileDescriptor);

		if (entrada_de_tabla_proceso != NULL) {
			sem_wait(&mutex_tablaGlobalDeArchivos);
			ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* entrada_de_archivo = list_get(tablaGlobalDeArchivos, entrada_de_tabla_proceso->globalFD);
			sem_post(&mutex_tablaGlobalDeArchivos);
			if (entrada_de_archivo != NULL) {
				if (string_contains(entrada_de_tabla_proceso->flags, "r")) {

					void* pedidoDeLectura = serializarPedidoFs(estructura.size,entrada_de_tabla_proceso->offset,entrada_de_archivo->path); //Patos, basicamente
					enviarMensaje(socketFS, obtenerDatosDeArchivo,pedidoDeLectura,4 + strlen(entrada_de_archivo->path)+ sizeof(int) * 2 + 1);
					void* contenido;
					if (recibirMensaje(socketFS, &contenido)== respuestaConContenidoDeFs) {

						enviarMensaje(socketCPU, respuestaLectura, contenido,estructura.size);
						entrada_de_tabla_proceso->offset += estructura.size;
					} else {
						finalizarPid(estructura.pid, lecturaDenegadaPorFileSystem); //Respuesta Mala de FS, no hay que leer en el archivo
						enviarMensaje(socketCPU, respuestaBooleanaKernel,&rtaCPU, sizeof(int));
					}
					free(contenido);
				} else {
					finalizarPid(estructura.pid, lecturaDenegadaPorFaltaDePermisos);
					enviarMensaje(socketCPU, respuestaBooleanaKernel, &rtaCPU,sizeof(int));
				}
			} else {
				finalizarPid(estructura.pid, archivoInexistente);
				enviarMensaje(socketCPU, respuestaBooleanaKernel, &rtaCPU,sizeof(int));
			}
		} else {
			finalizarPid(estructura.pid, falloEnElFileDescriptor);
			enviarMensaje(socketCPU, respuestaBooleanaKernel, &rtaCPU,sizeof(int));
		}
	}

///////Privadas

	int recibirBooleanoDeFS(int pid, int exitcode) {
		void* stream2;
		recibirMensaje(socketFS, &stream2);
		int rtaFS = (*(int*) stream2);
		free(stream2);
		if (rtaFS) {
			return 1;
		} else {
			finalizarPid(pid, exitcode);
			return 0;
		}
	}

	int crearArchivo(char* path, int pid, char* flags) {
		enviarMensaje(socketFS, creacionDeArchivo, path, strlen(path) + 1);
		void* stream;
		recibirMensaje(socketFS, &stream);
		int seCreoArchivo = (*(int*) stream);
		if (seCreoArchivo) {
			agregarATablaGlobalDeArchivos(path, 1);
			ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(pid);
			int fileDescriptor = list_size(aux->tablaProceso);
			sem_wait(&mutex_tablaGlobalDeArchivos);
			int globalFD = list_size(tablaGlobalDeArchivos) - 1;
			sem_post(&mutex_tablaGlobalDeArchivos);
			agregarATablaDeProceso(globalFD, flags, aux->tablaProceso);
			return fileDescriptor;
		} else {
			return 0;
		}
	}

	int agregarNuevaAperturaDeArchivo(char* path, int pid, char* flags) {
		ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(pid);
		ENTRADA_DE_TABLA_GLOBAL_DE_ARCHIVOS* archivo = encontrarElDeIgualPath(path);
		if (archivo == NULL) {
			agregarATablaGlobalDeArchivos(path, 1);
			archivo = encontrarElDeIgualPath(path);
		} else {
			sem_wait(&mutex_tablaGlobalDeArchivos);
			archivo->cantidad_aperturas++;
			sem_post(&mutex_tablaGlobalDeArchivos);
		}
		int fileDescriptor = list_size(aux->tablaProceso);
		int posicion = posicionEnTablaGlobalDeArchivos(archivo);
		agregarATablaDeProceso(posicion, flags, aux->tablaProceso);
		return fileDescriptor;
	}

ENTRADA_DE_TABLA_DE_PROCESO* getEntradaEnTablaProceso(int pid, int FD){
		ENTRADA_DE_TABLA_GLOBAL_DE_PROCESO* aux = encontrarElDeIgualPid(pid);
		return  list_get(aux->tablaProceso, FD);

	}

