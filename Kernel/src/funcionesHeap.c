/*
 * funcionesHeap.c
 *
 *  Created on: 14/6/2017
 *      Author: utnso
 */
#include "funcionesHeap.h"




void* serializarAlmacenarBytes2(t_escrituraMemoria almacenar){//No se que idiotez le pego a esto y no me dejaba ponerlo sin el 2.
	void* cosa = malloc(sizeof(int)*4+almacenar.direccion.size);
	memcpy(cosa,&almacenar.id,sizeof(int));
	memcpy(cosa+sizeof(int),&almacenar.direccion.page,sizeof(int));
	memcpy(cosa+sizeof(int)*2,&almacenar.direccion.offset,sizeof(int));
	memcpy(cosa+sizeof(int)*3,&almacenar.direccion.size,sizeof(int));
	memcpy(cosa+sizeof(int)*4,almacenar.valor,almacenar.direccion.size);
	return cosa;
}

int pedirPagina(int pid,int tamano){
	if(size_pagina <tamano){
		log_error(logKernel,"Se pidio un tamano invalido");
		return -1;

	}
	t_asignarPaginas asignar;
			asignar.cantPags = 1;
			asignar.pid = pid;
			enviarMensaje(socketMemoria,asignarPaginas,&asignar,sizeof(t_asignarPaginas));
			log_info(logKernel,"Se pide a memoria que se reserve %d paginas para el pid %d",1,pid);

			void* stream;
			recibirMensaje(socketMemoria,&stream);
			int pagina = leerInt(stream);
			if(!pagina){
				log_error(logKernel,"No se pudo reservar la pagina");
				return 0;
			}
			//log_info(logKernel,"Se recibe %d como respuesta,(Cantidad de paginas)",pagina );
			t_escrituraMemoria *w = malloc(sizeof(t_escrituraMemoria));
			void* cosaAMandar = malloc(tamano + sizeof(HeapMetadata)*2);
			HeapMetadata* heap1 = malloc(sizeof(HeapMetadata));
			heap1->isFree = false;
			heap1->size = tamano;


			w->valor = malloc(tamano + sizeof(HeapMetadata)*2);
			memcpy(w->valor,heap1,sizeof(HeapMetadata));
			log_info(logKernel,"Se reservan %d bytes  en la pagina pedida.",tamano );

			HeapMetadata* heap2 = malloc(sizeof(HeapMetadata));
			heap2->isFree = true;
			heap2->size = size_pagina-sizeof(HeapMetadata)*2 - tamano;
			memcpy(w->valor+sizeof(HeapMetadata)+tamano,heap2,sizeof(HeapMetadata));
			log_info(logKernel,"Se escribe el heapMetadata que informa cuanto tamaño queda (el ultimo)",tamano );
			free(heap1);
			free(heap2);
			w->id = pid;
			w->direccion.page = pagina;
			w->direccion.offset = 0;
			w->direccion.size = sizeof(HeapMetadata)*2 + tamano;
			cosaAMandar = serializarAlmacenarBytes2(*w);
			log_info(logKernel,"Se envia a memoria el siguiente pedido de escritura: Pagina-> %d, Pid->%d, tamano->%d,->offset->0",w->direccion.page,w->id,w->direccion.size);


			enviarMensaje(socketMemoria,almacenarBytes,cosaAMandar,sizeof(t_direccion)+sizeof(int)+w->direccion.size);//esta mal, necesito el deserealizador de spisso.
			recibirMensaje(socketMemoria,&stream);

			if(leerInt(stream)){
				log_info(logKernel,"Se almaceno correctamente los bytes en memoria.");
				filaTablaDeHeapMemoria* elemento = malloc(sizeof(filaTablaDeHeapMemoria));
				elemento->pagina = pagina;
				elemento->pid = pid;
				elemento->tamanoDisponible = size_pagina -sizeof(HeapMetadata)*2-tamano;
				list_add(tablaDeHeapMemoria,elemento);
				log_info(logKernel,"Se añade la pagina a la tabla de heap, y se retorna el offset de la variable pedida.");
				return tamanoHeader + pagina*size_pagina;
			}
			log_error(logKernel,"No se pudo almacenar los bytes pedidos por algun motivo. Se libera la pagina y se retorna el error."); // Puede pasar muy rara vez, todavia nunca nos paso. Pero mejor chequearlo.
			int pedidoDeLiberacion[] = {pid,pagina};
			enviarMensaje(socketMemoria,liberarUnaPagina,pedidoDeLiberacion,sizeof(int)*2);//esta mal, necesito el deserealizador de spisso.
			return 0;
}
int manejarLiberacionDeHeap(int pid,int offset){
	bool busqueda(filaTablaDeHeapMemoria* fila)
		{
			return fila->pid == pid && fila->pagina == offset/size_pagina;
		}
	filaTablaDeHeapMemoria* fila = list_find(tablaDeHeapMemoria,busqueda);
	if(fila == NULL){
		log_error(logKernel,"No hay una fila para liberar. Por lo que no hay variable para liberar. No existe el pid");

		return 0;
	}
	t_pedidoMemoria pedidoMemoria;
	pedidoMemoria.direccion.page = fila->pagina;
	pedidoMemoria.direccion.offset = 0;
	pedidoMemoria.direccion.size = size_pagina;
	pedidoMemoria.id = pid;
	log_info(logKernel,"Se pide a memoria para el pid %d, la pagina %d ",pedidoMemoria.id,pedidoMemoria.direccion.page);

	enviarMensaje(socketMemoria,solicitarBytes,&pedidoMemoria,sizeof(t_pedidoMemoria));
	void* stream;
	recibirMensaje(socketMemoria,&stream);
	if(stream){
		log_info(logKernel,"Se acepto el pedido a memoria y se recibio la pagina pedida");

		free(stream);
		recibirMensaje(socketMemoria,&stream);
		offsetTamanoYHeader* loQueTengoQueEscribir= liberarMemoriaHeap(offset%size_pagina,stream);
		if(loQueTengoQueEscribir == NULL){
			log_error(logKernel,"El pedido de liberacion fue invalido");
			return 0;
		}
		t_escrituraMemoria almacenamiento;
		almacenamiento.valor = &loQueTengoQueEscribir->header;//Dudoso
		almacenamiento.direccion.offset = loQueTengoQueEscribir->offset;
		almacenamiento.direccion.page = fila->pagina;
		almacenamiento.direccion.size = sizeof(HeapMetadata);
		almacenamiento.id = fila->pid;
		serializarAlmacenarBytes2(almacenamiento);
		log_info(logKernel,"Se pide almacenar para el pid %d en la pagina %d, en el offset %d, con el siguiente tamano %d.",almacenamiento.id,almacenamiento.direccion.page,almacenamiento.direccion.offset,almacenamiento.direccion.size);
		enviarMensaje(socketMemoria,almacenarBytes,serializarAlmacenarBytes2(almacenamiento),sizeof(int)*4+sizeof(HeapMetadata));
		free(stream);
		recibirMensaje(socketMemoria,&stream);
		if(fila->tamanoDisponible +loQueTengoQueEscribir->tamanoLibre < size_pagina-sizeof(HeapMetadata)){
			log_info(logKernel,"Se agrego %d a la pagina %d del pid %d",loQueTengoQueEscribir->tamanoLibre,fila->pagina,fila->pid);

			fila->tamanoDisponible+= loQueTengoQueEscribir->tamanoLibre;
		}
		else{
			bool busqueda2(filaTablaDeHeapMemoria* fila2)
				{
					if( fila2->pagina == fila->pagina &&fila2->pid == fila->pid){
						int x[2]={fila->pid,fila->pagina}; // Para ustedes que les gusta mas
						enviarMensaje(socketMemoria,liberarUnaPagina,x,sizeof(int)*2);
					}
				}

			list_remove_and_destroy_by_condition(tablaDeHeapMemoria,busqueda2,free);// falta un destroyer
			log_info(logKernel,"Se libero la pagina %d del pid %d.",fila->pagina,fila->pid);
		}
		return leerInt(stream);
	}
	log_error(logKernel,"No se acepto el pedido de la pagina a memoria.");
	free(stream);
	return 0;

}

int manejarPedidoDeMemoria(int pid,int tamano){
	bool busqueda(filaTablaDeHeapMemoria* fila)
		{
			return fila->pid == pid;
		}
	t_list* listaFiltrada = list_filter(tablaDeHeapMemoria,busqueda);
	if(list_is_empty(listaFiltrada)){
		log_info(logKernel,"No tiene paginas de heap. Se procede a pedir una pagina.");
		return pedirPagina(pid,tamano);

	}
	else{
		int i;
		int tamanoLista = list_size(listaFiltrada);
		log_info(logKernel,"El proceso tiene %d paginas de heap",tamanoLista);
		for(i=0;i<tamanoLista;i++){
			filaTablaDeHeapMemoria* fila=list_get(listaFiltrada,i);
			if(fila->tamanoDisponible>tamano){//Si podria escribir en esa pagina, se chequea
				log_info(logKernel,"Tiene tamano sufieciente para guardarlo aparentemente la pagina %d del pid %d",fila->pagina,fila->pid);
				t_pedidoMemoria escritura;
				escritura.direccion.page = fila->pagina;
				escritura.direccion.offset = 0;
				escritura.direccion.size = size_pagina;
				escritura.id = pid;
				log_info(logKernel,"Se pide  del pid %d,  la pagina %d, en el offset %d, con el siguiente tamano %d.",escritura.id,escritura.direccion.page,escritura.direccion.offset,escritura.direccion.size);

				enviarMensaje(socketMemoria,solicitarBytes,&escritura,sizeof(t_pedidoMemoria));
				void* stream;
				recibirMensaje(socketMemoria,&stream);
				if(*(int*)stream ){//Si no hubo error en memoria
					log_info(logKernel,"Se acepto el solicitar Bytes y se recibio el contenido.");

					free(stream);
					recibirMensaje(socketMemoria,&stream);

					offsetYBuffer x = escribirMemoria(tamano,stream);
					if(x.offset >=0){
						log_info(logKernel,"Se pudo guardar en esta pagina");

						t_escrituraMemoria w;
						w.valor = x.buffer;
						w.id = pid;
						w.direccion.page = fila->pagina;
						w.direccion.offset = x.offset-sizeof(HeapMetadata);
						w.direccion.size = tamano; // creo que va sizeof(HeapMetadata)
						log_info(logKernel,"Se escribe el metadata Heap necesario");

						enviarMensaje(socketMemoria,almacenarBytes,serializarAlmacenarBytes2(w),sizeof(int)*4+w.direccion.size); // esto esta mal, el size es otro
						void* stream2;
						recibirMensaje(socketMemoria,&stream2);
						fila->tamanoDisponible -= tamano-tamanoHeader;
						log_info(logKernel,"Se cambia el tamano disponible, dejandolo en %d", fila->tamanoDisponible);

						return x.offset +fila->pagina*size_pagina;
					}
				}
			}
		}
		log_info(logKernel,"Se recorrio todas las paginas de heap del proceso y no se pudo guardar. Por lo tanto, se pide una pagina nueva.");

		return pedirPagina(pid,tamano);//
	}
}


//Esto es para heap y sufrira modificaciones
offsetYBuffer escribirMemoria(int tamano,void* memoria){

	HeapMetadata headerAnterior = *((HeapMetadata*) memoria);
	void* buffer = malloc(tamano+sizeof(HeapMetadata)*2);
	int recorrido=0;
		recorrido = recorrido +tamanoHeader;//El recorrido se posiciona en donde termina el header.
	log_info(logKernel,"Empiezo a recorrer la pagina");
	while(size_pagina > recorrido){
		if(headerAnterior.isFree &&headerAnterior.size>tamano+tamanoHeader){

				HeapMetadata header;
				header.isFree= false;
				header.size= tamano;
				log_info(logKernel,"Se escribe el heapMetadata con tamano %d, y no esta libre",tamano);

				int y = recorrido-tamanoHeader;
				int offset = recorrido;
				memcpy(memoria+y,&header,tamanoHeader);
				recorrido += tamano; //El recorrido se posiciona donde va el siguiente header
				header.isFree= true;
				header.size= headerAnterior.size-tamano-tamanoHeader; //Calculo el puntero donde esta, es decir, el tamaño ocupado, y se lo resto a lo que queda.
				log_info(logKernel,"Se escribe el heapMetadata con tamano %d, y  esta libre",header.size);

				memcpy(memoria+recorrido,&header,tamanoHeader);
				memcpy(buffer,memoria+offset-sizeof(HeapMetadata),tamano+sizeof(HeapMetadata)*2);
				log_info(logKernel,"Se copia el contenido para enviarselo a memoria");
				if(offset == 5){
					offset = 0;
				}
				offsetYBuffer x;
				x.buffer = buffer;
				x.offset = offset;
				log_info(logKernel,"Se devuelve el offset %d y los siguientes heapMetadata:",x.offset);

				//escribir en la memoria
				return x;
		}
		else{
			log_info(logKernel,"El heap leido no alcanza para el tamano pedido o estaba reservado, tenia %d como tamaño y %b como boleano",headerAnterior.size,headerAnterior.isFree);
			recorrido+=headerAnterior.size;//el header se posiciona para leer el siguiente header.
			log_info(logKernel,"El recorrido ahora es %d", recorrido);
		}
		headerAnterior = *((HeapMetadata*) (memoria+recorrido));
		log_info(logKernel,"El heapMetadata tiene  %d como tamaño y %b como boleano",headerAnterior.size,headerAnterior.isFree);
		recorrido+=tamanoHeader;
		log_info(logKernel,"El recorrido ahora es %d", recorrido);
	}
	offsetYBuffer x;
	x.offset = -1;
	x.buffer = NULL;
	log_warning(logKernel,"No hay espacio suficiente. Se devuelve el offset -1.");
	return x;//no hay espacio suficiente
}
//Esto es para heap y sufrira modificaciones


offsetTamanoYHeader* liberarMemoriaHeap(int offset,void* pagina){
	if (offset<0){
		log_warning(logKernel,"Se ingreso un offset %d, el cual es negativo. Se procede a denegar la liberacion de heap.",offset);
		return NULL;

	}
	int i;
	int recorrido = 0;
	HeapMetadata *headerActual = ((HeapMetadata*) (pagina+recorrido));
	log_info(logKernel,"El offset es: %d y el HeapMetadata es: %d tamano y esta %b como booleano ",offset,headerActual->size,headerActual->isFree);

	HeapMetadata *headerAnterior = malloc(tamanoHeader);

	int offsetQueTengoQueDevolver = 0;

	headerAnterior->isFree = false;
	int recorridoDesdeDondeTengoQueCopiar=recorrido;
	recorrido+= tamanoHeader;
	log_info(logKernel,"Se empieza a recorrer la pagina para buscar el heapMetadata donde se pueda guardar");
	while(recorrido<size_pagina&&recorrido+sizeof(HeapMetadata)<offset){
		recorridoDesdeDondeTengoQueCopiar = recorrido-5;
		headerAnterior = headerActual;
		log_info(logKernel,"El HeapMetadata Anterior es: %d tamano y esta %d como booleano ",offset,headerAnterior->size,headerAnterior->isFree);
		recorrido+=headerActual->size;
		headerActual = ((HeapMetadata*) (pagina+recorrido));
		log_info(logKernel,"El HeapMetadata Actual es: %d tamano y esta %d como booleano ",offset,headerActual->size,headerActual->isFree);

		recorrido+= tamanoHeader;
	}
	if(recorrido>=size_pagina){
			log_error(logKernel,"No se pudo guardar en esta pagina");
			return NULL;
	}
	else{
		offsetQueTengoQueDevolver = offset-sizeof(HeapMetadata);
		offsetTamanoYHeader* x = malloc(sizeof(offsetTamanoYHeader));
			x->tamanoLibre = headerActual->size;
			headerActual->isFree= true;
			HeapMetadata* headerSiguiente;
			HeapMetadata  headerAEscribir = *headerActual;
			log_info(logKernel,"El HeapMetadata a escribir es: %d tamano y esta %b como booleano ",offset,headerAEscribir.size,headerAEscribir.isFree);
			headerSiguiente = ((HeapMetadata*) (pagina+recorrido+headerActual->size));
			log_info(logKernel,"El HeapMetadata Siguiente es: %d tamano y esta %b como booleano ",offset,headerActual->size,headerActual->isFree);
			if(headerSiguiente->isFree){
				log_info(logKernel,"El siguiente heapMetadata estaba libre, por lo que se ajusta para evitar la fragmentacion");
				headerActual->size += headerSiguiente->size + tamanoHeader;
				offsetQueTengoQueDevolver = recorridoDesdeDondeTengoQueCopiar;
				x->tamanoLibre	+= 5;
				headerAEscribir = *headerActual;
				log_info(logKernel,"El HeapMetadata a escribir es: %d tamano y esta %b como booleano ",offset,headerAEscribir.size,headerAEscribir.isFree);
			}
			if(headerAnterior->isFree){
				log_info(logKernel,"El  heapMetadata anterior estaba libre, por lo que se ajusta para evitar la fragmentacion");
				headerAnterior->size += headerActual->size + tamanoHeader;
				offsetQueTengoQueDevolver = recorridoDesdeDondeTengoQueCopiar;
				headerAEscribir = *headerAnterior;
				log_info(logKernel,"El HeapMetadata a escribir es: %d tamano y esta %b como booleano ",offset,headerAEscribir.size,headerAEscribir.isFree);
				x->tamanoLibre	+= 5;
				log_info(logKernel,"");
			}
			x->header = headerAEscribir;
			x->offset = offsetQueTengoQueDevolver;
			log_info(logKernel,"Se devuelve el offset %d, el tamaño libre %d y el header, con el size %d y booleano %b",x->offset,x->tamanoLibre,x->header.size,x->header.isFree);
			return x;
			//tengo que devolver offsetQueTengoQueDevolver y el headerQueTengaQueEscribir
	}
	log_error(logKernel,"No se pudo liberar la variable.");
	return NULL;
}

int liberarRecursosHeap(int pid){

	bool busqueda(filaTablaDeHeapMemoria* fila)
	{
			return fila->pid == pid;
	}
	int i;
	sem_wait(&mutex_tablaDeHeap);
	int cantidad = list_count_satisfying(tablaDeHeapMemoria,busqueda);
	if(cantidad == 0){
		sem_post(&mutex_tablaDeHeap);
		return 1;
	}
	for(i=0;i<cantidad;i++){
		filaTablaDeHeapMemoria* fila =list_remove_by_condition(tablaDeHeapMemoria,busqueda);
		int estructuras[] = {fila->pid,fila->pagina};
		enviarMensaje(socketMemoria,liberarUnaPagina,estructuras,sizeof(int)*2);
	}
	sem_post(&mutex_tablaDeHeap);
	log_info(logKernel,"Se removieron todos las paginas");
	return 0;
}
