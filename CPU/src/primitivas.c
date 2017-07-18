#include "primitivas.h"

t_valor_variable pedirValorAMemoria(t_direccion);

void escribirValorEnMemoria(t_direccion, t_valor_variable);

t_puntero calcularPuntero(t_direccion);

t_direccion calcularDireccion(t_puntero);

t_direccion calcularNuevaDireccion();

t_direccion ultimaDireccionArg(int);

t_direccion ultimaDireccionVar(int);

bool mayor(t_direccion, t_direccion);

void asignarDireccionRespectoA(int, t_direccion*);

int paginaInicio();

int tamanoMensajeAEscribir(int tamanioContenido);

void* serializarMensajeAEscribir(t_mensajeDeProceso mensaje);

char* generarStringFlags(t_banderas flags);

void* serializarArchivo(t_crearArchivo archivo);

void escribirEnLaVariable(t_puntero informacion, void* stream, int tamanio);

t_puntero AnSISOP_definirVariable(t_nombre_variable identificador_variable) {
	log_info(logCPU,"AnSISOP_definirVariable\n");
	t_direccion direccion;
	t_variable* variable = malloc(sizeof(t_variable));

	direccion = calcularNuevaDireccion();

	variable->ID = identificador_variable;
	variable->direccion = direccion;

	if (direccion.page >= pcb->contPags_pcb) {
		log_info(logCPU,"STACK OVERFLOW page: %d , offset: %d , size: %d \n",direccion.page, direccion.offset, direccion.size);
		terminoPrograma = true;
		pcb->exitCode = -5;
		return -1;
	}

	//Si es un digito es un argumento debido a la sintaxis del lenguaje, entonces se debe almacenar en argumentos
	if (isdigit(identificador_variable)) {
		list_add(pcb->indiceStack[pcb->contextoActual].argumentos, variable);
	}

	//Si es una letra es una variable debido a la sintaxis del lenguaje, entonces se debe almacenar en variables
	if (isalpha(identificador_variable)) {
		list_add(pcb->indiceStack[pcb->contextoActual].variables, variable);
	}

	log_info(logCPU,"Defini %c le asigne la direccion: %d %d %d \n",
			identificador_variable, direccion.page, direccion.offset,direccion.size);

	return calcularPuntero(direccion);
}

t_puntero AnSISOP_obtenerPosicionVariable(
		t_nombre_variable identificador_variable) {
	log_info(logCPU,"AnSISOP_obtenerPosicionVariable\n");

	t_variable *variable;
	t_puntero aRetornar;

	//Si es un digito es un argumento debido a la sintaxis del lenguaje, entonces se debe buscar en argumentos
	if (identificador_variable >= '0' && identificador_variable <= '9') {
		//Es menos 48 porque '0' es 48 es ASCII y es para obtener el valor sin hacer mucho quilombo
		variable = list_get(pcb->indiceStack[pcb->contextoActual].argumentos,identificador_variable - 48);

		aRetornar = calcularPuntero(variable->direccion);

		log_info(logCPU,"Obtuve la posicion de %c esta es: %d \n",identificador_variable, aRetornar);

		return aRetornar;
	} else {
		//Si es una letra es una variable debido a la sintaxis del lenguaje, entonces se debe buscar en variables
		int j;
		for (j = 0;j < list_size(pcb->indiceStack[pcb->contextoActual].variables);j++) {
			variable = list_get(pcb->indiceStack[pcb->contextoActual].variables,j);
			if (variable->ID == identificador_variable) {

				aRetornar = calcularPuntero(variable->direccion);

				log_info(logCPU,"Obtuve la posicion de %c esta es: %d \n",identificador_variable, aRetornar);

				return aRetornar;
			}
		}
	}
	return -1;
}

t_valor_variable AnSISOP_dereferenciar(t_puntero direccion_variable) {
	log_info(logCPU,"AnSISOP_dereferenciar\n");

	//Se busca la direccion
	t_direccion direccion = calcularDireccion(direccion_variable);

	//Se pide el valor a memoria
	t_valor_variable valorVariable = pedirValorAMemoria(direccion);

	log_info(logCPU,"Desreferencie la direccion %d %d %d y el valor es %d \n",direccion.page, direccion.offset, direccion.size, valorVariable);

	return valorVariable;
}

void AnSISOP_asignar(t_puntero direccion_variable, t_valor_variable valor) {
	log_info(logCPU,"AnSISOP_asignar\n");

	//Calculo la direccion en memoria de la variable
	t_direccion direccion = calcularDireccion(direccion_variable);

	//Se escribe en Memoria sabiendo la posicion de memoria y el valor a escribir
	escribirValorEnMemoria(direccion, valor);

	log_info(logCPU,"Asigne a la direccion %d %d %d el valor es %d \n", direccion.page,direccion.offset, direccion.size, valor);
}

t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida variable) {
	log_info(logCPU,"AnSISOP_obtenerValorCompartida\n");

	t_valor_variable valor = -42;

	log_info(logCPU,"Le pido al Kernel que me de el valor de la variable compartida: %s\n",variable);

	enviarMensaje(socketKernel, pedirValorCompartida, variable,strlen(variable) + 1);

	void* stream;

	switch (recibirMensajeSeguro(socketKernel, &stream)) {
	case noExisteVarCompartida: {
		terminoPrograma = true;
		pcb->exitCode = -12;
		free(stream);
	}
		break;
	case envioValorCompartida: {
		valor = leerInt(stream);
	}
		break;
	default: {
		log_error(logCPU,"Error en la accion del mensaje maquinola\n");
		free(stream);
	}
	}

	return valor;
}

t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida variable,t_valor_variable valor) {
	log_info(logCPU,"AnSISOP_asignarValorCompartida\n");

	t_asignarValor mensaje = { .valor = valor, .variable = variable };

	log_info(logCPU,"Le pido al Kernel que asigne el valor: %d a la variable compartida: %s\n",valor, variable);
	enviarMensaje(socketKernel, asignarValorCompartida, mensaje.variable,strlen(mensaje.variable) + 1);

	void* stream;

	switch (recibirMensajeSeguro(socketKernel, &stream)) {
		case noExisteVarCompartida: {
			terminoPrograma = true;
			pcb->exitCode = -12;
			free(stream);
		}break;
		case envioValorCompartida: {
			valor = leerInt(stream);
			enviarMensaje(socketKernel, asignarValorCompartida, &(mensaje.valor),sizeof(int));
		}break;
		default: {
			log_error(logCPU,"Error en la accion del mensaje maquinola\n");
			free(stream);
		}
	}

	return valor;
}

void AnSISOP_irAlLabel(t_nombre_etiqueta nombre_etiqueta) {
	log_info(logCPU,"AnSISOP_irAlLabel\n");

	t_puntero_instruccion puntero;

	//Una etiqueta es como un identificador para las funciones del programa ANSISOP
	//El numero de instruccion al que esta asociada la etiqueta es el numero de la primera instruccion ejecutable de dicha funcion
	//La funcion devuelve el numero de instruccion al que esta asociada la etiqueta
	puntero = metadata_buscar_etiqueta(nombre_etiqueta, pcb->indiceEtiquetas,pcb->sizeEtiquetas);

	//Como es el numero de la siguiente instruccion a ejecutar se le asigna al ProgramCounter para que el programa siga a partir de ahi

	pcb->programCounter = puntero - 1;

}

void AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta) {
	log_info(logCPU,"AnSISOP_llamarSinRetorno\n");

	//Se incrementa el contexto actual para que cada vez que se pida un valor se vaya a buscar el valor en la entrada correspondiente
	pcb->contextoActual++;

	//Se reserva espacio para una entrada mas de stack y se inicializa ambas listas de argumentos y variables para su posterior utilizacion
	pcb->indiceStack = realloc(pcb->indiceStack,sizeof(t_entrada) * (pcb->contextoActual + 1));
	pcb->indiceStack[pcb->contextoActual].argumentos = list_create();
	pcb->indiceStack[pcb->contextoActual].variables = list_create();

	//Se guarda en el contexto actual cual es la posicion de la instruccion siguiente que debe ejecutar al volver de la funcion
	pcb->indiceStack[pcb->contextoActual].retPos = pcb->programCounter;

	//Se actualiza la cantidad de entradas en el Stack
	pcb->cantidadDeEntradas++;

	//Para que luego siga la ejecucion en la funcion
	AnSISOP_irAlLabel(etiqueta);

}

void AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,
		t_puntero donde_retornar) {
	log_info(logCPU,"AnSISOP_llamarConRetorno\n");

	//Se incrementa el contexto actual para que cada vez que se pida un valor se vaya a buscar el valor en la entrada correspondiente
	pcb->contextoActual++;

	//Se reserva espacio para una entrada mas de stack y se inicializa ambas listas de argumentos y variables para su posterior utilizacion
	pcb->indiceStack = realloc(pcb->indiceStack,sizeof(t_entrada) * (pcb->contextoActual + 1));
	pcb->indiceStack[pcb->contextoActual].argumentos = list_create();
	pcb->indiceStack[pcb->contextoActual].variables = list_create();

	//Se guarda en el contexto actual cual es la posicion de la instruccion siguiente que debe ejecutar al volver de la funcion
	pcb->indiceStack[pcb->contextoActual].retPos = pcb->programCounter;

	//Se guarda en el contexto actual cual es la direccion de la variable a la que se le asignara el valor que retornara esta funcion
	pcb->indiceStack[pcb->contextoActual].retVar = calcularDireccion(donde_retornar);

	//Se actualiza la cantidad de entradas en el Stack
	pcb->cantidadDeEntradas++;

	//Para que luego siga la ejecucion en la funcion
	AnSISOP_irAlLabel(etiqueta);

}

void AnSISOP_finalizar(void) {
	log_info(logCPU,"AnSISOP_finalizar\n");

	//En ambos casos deberia de liberar la memoria de alguna manera magico-fantastica
	//Si es que hay que liberarla lo que ahora comienzo a dudar, se vera con respecto al ultimatum con el pcb

	if (pcb->contextoActual == 0) {
		//Esto se da en el caso que se termine el programa
		log_info(logCPU,"Se finalizo la ultima instruccion del main\n");
		terminoPrograma = true;

		pcb->exitCode = 0;

		//Se libera la memoria de esa entrada
		list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].argumentos, free);
		list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].variables, free);

	} else {
		//En otro caso se vuelve al contexto anterior
		log_info(logCPU,"Se finalizo la ultima instruccion de una funcion\n");

		//Se cambia el ProgramCounter para que siga la ejecucion a partir de la siguiente instruccion de la funcion anterior
		pcb->programCounter = pcb->indiceStack[pcb->contextoActual].retPos;

		//Se libera la memoria de esa entrada y se vuelve a ajustar el tamanio del indice del stack para tener siempre el tamanio exacto
		list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].argumentos, free);
		list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].variables, free);
		pcb->indiceStack = realloc(pcb->indiceStack,sizeof(t_entrada) * (pcb->contextoActual));

		//Se actualiza cual es el Contexto Actual de Ejecucion
		pcb->contextoActual--;

	}

	//Se actualiza la cantidad de entradas en el Stack
	pcb->cantidadDeEntradas--;
}

void AnSISOP_retornar(t_valor_variable retorno) {
	log_info(logCPU,"AnSISOP_retornar\n");

	//Se escribe el valor devuelto por la funcion en la direccion de retorno
	escribirValorEnMemoria(pcb->indiceStack[pcb->contextoActual].retVar,retorno);

	//Se cambia el ProgramCounter para que siga la ejecucion a partir de la siguiente instruccion de la funcion anterior
	pcb->programCounter = pcb->indiceStack[pcb->contextoActual].retPos;

	//Se libera la memoria de esa entrada y se vuelve a ajustar el tamanio del indice del stack para tener siempre el tamanio exacto
	list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].argumentos, free);
	list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].variables, free);
	pcb->indiceStack = realloc(pcb->indiceStack,sizeof(t_entrada) * (pcb->contextoActual));

	//Se actualiza cual es el Contexto Actual de Ejecucion
	pcb->contextoActual--;

	//Se actualiza la cantidad de entradas en el Stack
	pcb->cantidadDeEntradas--;

}

//Operaciones de Kernel
void AnSISOP_wait(t_nombre_semaforo identificador_semaforo) {
	log_info(logCPU,"AnSISOP_wait\n");

	pcb->cantDeInstPrivilegiadas++;

	log_info(logCPU,"Le pido al Kernel que haga wait del semaforo: %s\n",identificador_semaforo);
	pcb->programCounter++;
	void * stream = serializarPCBYSemaforo(pcb, identificador_semaforo);
	enviarMensaje(socketKernel, waitSemaforo, stream,tamanoPCB(pcb) + sizeof(int) + strlen(identificador_semaforo) + 1);

	free(stream);
	pcb->programCounter--;
	bool respuestaDeKernel;
	if (recibirMensajeSeguro(socketKernel, &stream) == respuestaBooleanaKernel)
		respuestaDeKernel = *((bool*) stream);
	else {
		terminoPrograma = true;
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
	}
	if (!respuestaDeKernel){
		bloqueado = true;
		pcb->estadoDeProceso = 2; //Esto significa bloqueado
	}

}

void AnSISOP_signal(t_nombre_semaforo identificador_semaforo) {
	log_info(logCPU,"AnSISOP_signal\n");

	pcb->cantDeInstPrivilegiadas++;
	log_info(logCPU,"Le pido al Kernel que haga signal del semaforo: %s\n",identificador_semaforo);
	pcb->programCounter++;
	void * stream = serializarPCBYSemaforo(pcb, identificador_semaforo);
	enviarMensaje(socketKernel, signalSemaforo, stream,tamanoPCB(pcb) + sizeof(int) + strlen(identificador_semaforo) + 1);
	free(stream);
	pcb->programCounter--;
	bool respuestaDeKernel;
	if (recibirMensajeSeguro(socketKernel, &stream) == respuestaBooleanaKernel)
		respuestaDeKernel = *((bool*) stream);
	else {
		terminoPrograma = true;
		log_info(logCPU,"Error en el protocolo de comunicacion\n");
	}
	//Esto solo pasa en caso de que noexista el semaforo
	if (!respuestaDeKernel)
		bloqueado = true;
}

t_puntero AnSISOP_reservar(t_valor_variable espacio) {
	log_info(logCPU,"AnSISOP_reservar\n");

	pcb->cantDeInstPrivilegiadas++;
	int dosEnteros[2] = { espacio, pcb->pid };

	enviarMensaje(socketKernel, reservarVariable, (void*) dosEnteros,sizeof(int) * 2);

	void* stream;
	int offset = 0;

	switch (recibirMensajeSeguro(socketKernel, &stream)) {
	case enviarOffsetDeVariableReservada: {
		offset = *(int*) stream;
		free(stream);
		if (offset == 0) {
			log_info(logCPU,"No se pudo asignar otra pagina al proceso\n");
			terminoPrograma = true;
			pcb->exitCode = -9;
		} else {
			calcularDireccion(offset);
		}
	}break;
	case pedidoRechazadoPorPedirMas: { // en caso de que se pida alocar mas que el tamanio de una pagina
		log_info(logCPU,"Se intentó reservar más memoria que el tamaño de una página\n");
		terminoPrograma = true;
		pcb->exitCode = -8;
	}break;
	default:
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
	}

	return offset;
}

void AnSISOP_liberar(t_puntero puntero) {
	log_info(logCPU,"AnSISOP_liberar\n");

	pcb->cantDeInstPrivilegiadas++;

	int dosEnteros[2] = { puntero, pcb->pid };

	enviarMensaje(socketKernel, liberarVariable, dosEnteros, sizeof(int) * 2);

	void* stream;

	if (recibirMensajeSeguro(socketKernel, &stream) != enviarSiSePudoLiberar) {
		free(stream);
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
	} else {
		int entero = leerInt(stream);
		if (entero == 0) {
			log_info(logCPU,"Algo salio mal al liberar la variable\n");
			bloqueado = true;
			pcb->exitCode = -42;
		}else{
			log_info(logCPU,"Se libero correctamente la variable alocada\n");
		}

	}



}

t_descriptor_archivo AnSISOP_abrir(t_direccion_archivo direccion,t_banderas flags) {
	log_info(logCPU,"AnSISOP_abrir\n");

	pcb->cantDeInstPrivilegiadas++;

	t_crearArchivo archivo;
	archivo.pid = pcb->pid;
	archivo.path = direccion;
	archivo.flags = generarStringFlags(flags);

	if (strcmp(archivo.flags, "") == 0) {
		log_error(logCPU,"Quiere abrir un archivo sin permisos \n");
		//inserte error aqui
		return 0;
	}

	void* archivoSerializado = serializarArchivo(archivo);

	enviarMensaje(socketKernel, abrirArchivo, archivoSerializado,strlen(direccion) + 1 + strlen(archivo.flags) + 1+ sizeof(int) * 3);

	free(archivoSerializado);

	void* stream;

	switch (recibirMensajeSeguro(socketKernel, &stream)) {
		case envioDelFileDescriptor: {
			t_descriptor_archivo FD = leerInt(stream);
			log_info(logCPU,"El FD del archivo pedido es: %d \n", FD);
			return FD;
		}break;
		case respuestaBooleanaKernel: {
			free(stream);
			log_info(logCPU,"Algo salio mal con el archivo, se termina el programa\n");
			bloqueado = true;
		}break;
		default: {
			log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
		}
	}

	return 0;
}

void AnSISOP_borrar(t_descriptor_archivo direccion) {
	log_info(logCPU,"AnSISOP_borrar\n");

	pcb->cantDeInstPrivilegiadas++;

	t_archivo archivo;
	archivo.pid = pcb->pid;
	archivo.fileDescriptor = direccion;

	enviarMensaje(socketKernel, borrarArchivoCPU, (void*) &archivo,sizeof(int) * 2);

	void* stream;

	if (recibirMensajeSeguro(socketKernel, &stream) == respuestaBooleanaKernel) {
		int rtaKernel = *(int*) stream;
		free(stream);
		if (rtaKernel == 0) {
			log_info(logCPU,"Quiere borrar un archivo que nunca abrio\n");
			terminoPrograma = true;
			pcb->exitCode = -13;
		} else {
			log_info(logCPU,"Archivo borrado con exito\n");
		}
	} else {
		free(stream);
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
		terminoPrograma = true;
		pcb->exitCode = -42;
	}

}

void AnSISOP_cerrar(t_descriptor_archivo descriptor_archivo) {
	log_info(logCPU,"AnSISOP_cerrar\n");

	pcb->cantDeInstPrivilegiadas++;

	t_archivo archivo;
	archivo.pid = pcb->pid;
	archivo.fileDescriptor = descriptor_archivo;

	enviarMensaje(socketKernel, cerrarArchivo, (void*) &archivo,sizeof(int) * 2);

	void* stream;

	if (recibirMensajeSeguro(socketKernel, &stream) == respuestaBooleanaKernel) {
		int rtaKernel = *(int*) stream;
		free(stream);
		if (rtaKernel == 0) {
			log_info(logCPU,"Quiere cerrar un archivo que nunca abrio\n");
			terminoPrograma = true;
			pcb->exitCode = -13;
		} else {
			log_info(logCPU,"Archivo cerrado con exito\n");
		}
	} else {
		free(stream);
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
		terminoPrograma = true;
		pcb->exitCode = -42;
	}

}

void AnSISOP_moverCursor(t_descriptor_archivo descriptor_archivo,t_valor_variable posicion) {
	log_info(logCPU,"AnSISOP_moverCursor\n");

	pcb->cantDeInstPrivilegiadas++;

	t_moverCursor archivo;
	archivo.pid = pcb->pid;
	archivo.fileDescriptor = descriptor_archivo;
	archivo.posicion = posicion;

	enviarMensaje(socketKernel, moverCursorArchivo, (void*) &archivo,sizeof(int) * 3);

	void* stream;

	if (recibirMensajeSeguro(socketKernel, &stream) == respuestaBooleanaKernel) {
		int rtaKernel = *(int*) stream;
		free(stream);
		if (rtaKernel == 0) {
			log_info(logCPU,"Quiere mover el cursor de un archivo que nunca abrio\n");
			terminoPrograma = true;
			pcb->exitCode = -13;
		} else {
			log_info(logCPU,"Cursor movido con exito\n");
		}
	} else {
		free(stream);
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
		terminoPrograma = true;
		pcb->exitCode = -42;
	}
}

void AnSISOP_escribir(t_descriptor_archivo descriptor_archivo,void* informacion, t_valor_variable tamanio) {
	log_info(logCPU,"AnSISOP_escribir\n");

	pcb->cantDeInstPrivilegiadas++;

	t_mensajeDeProceso mensajeDeProceso = {
			.pid = pcb->pid,
			.descriptorArchivo = descriptor_archivo,
			.tamanio = tamanio,
			.mensaje =(char*) informacion
	};


	void* mensajeSerializado = serializarMensajeAEscribir(mensajeDeProceso);

	enviarMensaje(socketKernel, mensajeParaEscribir, mensajeSerializado,tamanoMensajeAEscribir(tamanio));

	free(mensajeSerializado);

	void* stream;

	if(recibirMensajeSeguro(socketKernel, &stream) == respuestaBooleanaKernel) {
		int  rtaKernel = *(int*)stream;
		if(rtaKernel){
			log_info(logCPU,"La escritura fue exitosa\n");
		}else{
			log_info(logCPU,"Quiere escribir un archivo que nunca abrio\n");
			terminoPrograma = true;
			pcb->exitCode = -13;
		}
	}else{
		log_error(logCPU,"Error en el protocolo de comunicacion\n");
		terminoPrograma = true;
		pcb->exitCode = -42;
	}
	free(stream);

}

void AnSISOP_leer(t_descriptor_archivo descriptor_archivo,t_puntero informacion, t_valor_variable tamanio) {
	log_info(logCPU,"AnSISOP_leer\n");

	pcb->cantDeInstPrivilegiadas++;

	int tresEnteros[] = { pcb->pid, descriptor_archivo, tamanio };

	enviarMensaje(socketKernel, leerArchivo, tresEnteros, sizeof(int) * 3);

	void* stream;
	switch (recibirMensajeSeguro(socketKernel, &stream)) {
		case respuestaBooleanaKernel: {
			/*si nunca habia abierto el archivo*/
			log_info(logCPU,"Se produjo un error al intentar leer el archivo\n");
			terminoPrograma = true;
		}break;
		case respuestaLectura: {

			log_info(logCPU,"Se leyo con exito el archivo\n");
			escribirEnLaVariable(informacion,stream,tamanio);
			//Aca hacer algo, pero sinceramente nose que espera el parser que haga por el


		}break;
		default: {
			log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
		}
	}
	free(stream);
}

/**************************************************************************FUNCIONES PRIVADAS**********************************************************************************************/

t_valor_variable pedirValorAMemoria(t_direccion direccion) {

	t_valor_variable valorVariable;
	t_pedidoMemoria pedido = { .id = pcb->pid, .direccion = direccion };

	//Se pide a Memoria el contenido de esa posicion que es el valor de la variable
	enviarMensaje(socketMemoria, solicitarBytes, (void *) &pedido,sizeof(pedido));

	log_info(logCPU,"Se pide el valor de la variable en la direccion de memoria: %d %d %d \n",direccion.page, direccion.offset, direccion.size);

	//se recibe el valor de la variable
	void* stream;
	int* valor;
	int* booleano;

	int accion = recibirMensajeSeguro(socketMemoria, &stream);

	switch (accion) {
		case RespuestaBooleanaDeMemoria: {
			booleano = stream;
		}break;
		default: {
			log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
		}break;
	}
	if (*booleano) {
		free(stream);
		accion = recibirMensajeSeguro(socketMemoria, &stream);
		switch (accion) {
			case lineaDeCodigo: {
				valor = stream;
				valorVariable = *valor;
			}break;
			default: {
				log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
			}break;
		}
	} else {
		terminoPrograma = true;
		pcb->exitCode = -5;		//Excepcion de Memoria STACK OVERFLOW
	}

	free(stream);

	return valorVariable;
}

void escribirValorEnMemoria(t_direccion direccion, t_valor_variable valor) {

	//Se crea el void* que contiene el pid, la direccion y el valor a escribir, que son los datos necesarios para que la memoria escriba el valor
	int escrituraA[] = { pcb->pid, direccion.page, direccion.offset,direccion.size, valor };
	//se pide a memoria que escriba el valor enviado en la posicion de memoria tambien enviada
	enviarMensaje(socketMemoria, almacenarBytes, escrituraA,sizeof(int) * 4 + direccion.size);

	log_info(logCPU,"Se escribio el valor %d en la direccion de memoria: %d %d %d \n",valor, direccion.page, direccion.offset, direccion.size);

//	free(auxiliar);

	//Devuelve un OK o mata con un Stack Overflow
	void* stream;
	int* respuesta;
	int accion = recibirMensajeSeguro(socketMemoria, &stream);
	switch (accion) {
		case RespuestaBooleanaDeMemoria: {
			respuesta = stream;
			break;
		}
		default: {
			log_error(logCPU,"Error en el protocolo de comunicacion\n");
			terminoPrograma = true;
			pcb->exitCode = -42;
		}
	}

	if (*respuesta != 1) {
		terminoPrograma = true;
		pcb->exitCode = -5;
	}

	free(stream);
}

t_direccion calcularNuevaDireccion() {
	t_direccion direccion;
	t_list* argumentos = pcb->indiceStack[pcb->contextoActual].argumentos;
	t_list* variables = pcb->indiceStack[pcb->contextoActual].variables;

	//Si es la primera vez que se pide una direccion para este contexto
	if (list_size(argumentos) == 0 && list_size(variables) == 0) {

		//Si es la primera vez que se pide una direccion para este proceso se le asigna la primera posicion de su stack
		if (pcb->contextoActual == 0) {
			direccion.page = paginaInicio();
			direccion.offset = 0;
		} else {

			//Si es la primera vez que se pide una direccion para este contexto se debe revisar cual fue la ultima posicion asignada en el contexto anterior para comenzar desde ahi
			asignarDireccionRespectoA(pcb->contextoActual - 1, &direccion);
		}
	} else {
		asignarDireccionRespectoA(pcb->contextoActual, &direccion);
	}

	direccion.size = 4;

	if (direccion.offset + direccion.size > datosIniciales->size_pag) {
		direccion.page++;
		direccion.offset = 0;
	}

	//printf("Calcule una nueva direccion jaja saludos, esta es: %d %d %d \n",direccion.page, direccion.offset, direccion.size);

	return direccion;
}

t_puntero calcularPuntero(t_direccion direccion) {
	t_puntero puntero = direccion.page * datosIniciales->size_pag+ direccion.offset;
	//printf("En base a la direccion %d %d %d calculo la posicion %d \n",direccion.page, direccion.offset, direccion.size, puntero);
	return puntero;
}

t_direccion calcularDireccion(t_puntero puntero) {
	t_direccion direccion;

	if (datosIniciales->size_pag > puntero) {
		direccion.page = 0;
		direccion.offset = puntero;
	} else {
		direccion.page = puntero / datosIniciales->size_pag;
		direccion.offset = puntero % datosIniciales->size_pag;
	}
	direccion.size = 4;

	//printf("En base a la posicion %d calculo la direccion %d %d %d \n", puntero,direccion.page, direccion.offset, direccion.size);

	return direccion;
}

void asignarDireccion(t_direccion* direccion, void* stream) {
	direccion = stream;
}

t_direccion ultimaDireccionArg(int contexto) {

	t_direccion direccion;
	t_variable* variable;

	//Si no hay argumentos declarados en ningun contexto se devuelve una direccion erronea
	if (contexto == -1) {
		direccion.page = -1;
		direccion.offset = -1;
		direccion.size = -1;
	} else {
		//Si no hay elementos en la lista se busca en el contexto anterior
		if (list_is_empty(pcb->indiceStack[contexto].argumentos)) {
			direccion = ultimaDireccionArg(contexto - 1);
		} else {
			//Devuelve el ultimo elemento de la lista de argumentos
			variable = list_get(pcb->indiceStack[contexto].argumentos,list_size(pcb->indiceStack[contexto].argumentos) - 1);
			direccion = variable->direccion;
		}
	}

	return direccion;
}

t_direccion ultimaDireccionVar(int contexto) {

	t_direccion direccion;
	t_variable* variable;

	//Si no hay variables declaradas en ningun contexto se devuelve una direccion erronea
	if (contexto == -1) {
		direccion.page = -1;
		direccion.offset = -1;
		direccion.size = -1;
	} else {
		//Si no hay elementos en la lista se busca en el contexto anterior
		if (list_is_empty(pcb->indiceStack[contexto].variables)) {
			direccion = ultimaDireccionVar(contexto - 1);
		} else {
			//Devuelve el ultimo elemento de la lista de variables
			variable = list_get(pcb->indiceStack[contexto].variables,list_size(pcb->indiceStack[contexto].variables) - 1);
			direccion = variable->direccion;
		}
	}
	return direccion;
}

//Devuelve si la primer direccion es mayor a la segunda
bool mayor(t_direccion unaDireccion, t_direccion otraDireccion) {

	if (unaDireccion.page == otraDireccion.page) {
		return unaDireccion.offset > otraDireccion.offset;
	} else {
		return unaDireccion.page > otraDireccion.page;
	}

}

bool direccionInvalida(t_direccion direccion) {
	return (direccion.page == -1 && direccion.offset == -1 && direccion.size == -1);
}

//Esto es para no repetir codigo, era muy asqueroso sino
void asignarDireccionRespectoA(int contexto, t_direccion* direccion) {

	t_direccion variable = ultimaDireccionVar(contexto);
	t_direccion argumento = ultimaDireccionArg(contexto);

	if (mayor(variable, argumento)) {
		direccion->page = variable.page;
		direccion->offset = variable.offset + 4;
	} else {
		direccion->page = argumento.page;
		direccion->offset = argumento.offset + 4;
	}
	if (direccionInvalida(variable) && direccionInvalida(argumento)) {
		direccion->page = paginaInicio();
		direccion->offset = 0;
	}
}

//Devuelve la pagina donde comienza en el stack
int paginaInicio() {
	return pcb->contPags_pcb - datosIniciales->size_stack;
}

char* generarStringFlags(t_banderas flags) {
	char* ret = string_new();

	if (flags.lectura)
		strcat(ret, "r\n");
	if (flags.escritura)
		strcat(ret, "w\n");
	if (flags.creacion)
		strcat(ret, "c\n");

	return ret;

}

int tamanoMensajeAEscribir(int tamanioContenido) {
	return sizeof(int) * 3 + tamanioContenido;
}

void* serializarMensajeAEscribir(t_mensajeDeProceso mensaje) {
	void* stream = malloc(tamanoMensajeAEscribir(mensaje.tamanio));

	memcpy(stream, &mensaje.pid, sizeof(int));

	memcpy(stream + sizeof(int), &mensaje.descriptorArchivo, sizeof(int));

	memcpy(stream + sizeof(int) * 2, &mensaje.tamanio, sizeof(int));

	memcpy(stream + sizeof(int) * 3, mensaje.mensaje, mensaje.tamanio);

	return stream;
}

void* serializarArchivo(t_crearArchivo archivo) {
	void* stream = malloc(
			sizeof(int) * 3 + strlen(archivo.flags) + 1 + strlen(archivo.path) + 1);

	memcpy(stream, &archivo.pid, sizeof(int));

	int aux1 = strlen(archivo.flags) + 1;

	memcpy(stream + sizeof(int), &aux1, sizeof(int));

	memcpy(stream + sizeof(int) * 2, archivo.flags, aux1);

	int aux2 = strlen(archivo.path) + 1;

	memcpy(stream + sizeof(int) * 2 + aux1, &aux2, sizeof(int));

	memcpy(stream + sizeof(int) * 3 + aux1, archivo.path, aux2);

	return stream;
}

void escribirEnLaVariable(t_puntero puntero,void* cosaDelFS, int tamanio){

 t_direccion direccion = calcularDireccion(puntero);
 direccion.size = tamanio;
 //Se crea el void* que contiene el pid, la direccion y el valor a escribir, que son los datos necesarios para que la memoria escriba el valor
 void* aEnviar = malloc(sizeof(int)*4 + direccion.size);

 memcpy(aEnviar,&(pcb->pid),sizeof(int));
 memcpy(aEnviar + sizeof(int),&(direccion.page),sizeof(int));
 memcpy(aEnviar + sizeof(int)*2,&(direccion.offset),sizeof(int));
 memcpy(aEnviar + sizeof(int)*3,&(direccion.size),sizeof(int));
 memcpy(aEnviar + sizeof(int)*4,cosaDelFS,direccion.size);

 //se pide a memoria que escriba el valor enviado en la posicion de memoria tambien enviada
 enviarMensaje(socketMemoria, almacenarBytes, aEnviar,sizeof(int) * 4 + direccion.size);

 free(aEnviar);

 //Devuelve un OK o mata con un Stack Overflow
 void* stream;
 int* respuesta;
 int accion = recibirMensajeSeguro(socketMemoria, &stream);
 switch (accion) {
  case RespuestaBooleanaDeMemoria: {
   respuesta = stream;
   break;
  }
  default: {
	  log_error(logCPU,"Error en el protocolo de comunicacion\n");
   terminoPrograma = true;
   pcb->exitCode = -42;
  }
 }

 if (*respuesta != 1) {
  terminoPrograma = true;
  pcb->exitCode = -5;
 }

 free(stream);
}


int recibirMensajeSeguro(int socket, void ** stream){
	int valor = recibirMensaje(socket, stream);
	if(valor == 0){
		if(socket==socketKernel)
			log_info(logCPU,"El Kernel se ha desconectado, se finalizara esta CPU\n");
		else
			log_info(logCPU,"La Memoria se ha desconectado, se finalizara esta CPU\n");
		close(socketKernel);
		close(socketMemoria);
		liberarConfiguracion();
		log_destroy(logCPU);
		if(hayPCB) 	destruirPCB_Puntero(pcb);
		free(datosIniciales);
		exit(-1);
	}
	return valor;
}
