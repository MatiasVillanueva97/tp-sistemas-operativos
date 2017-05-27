#include "primitivas.h"


t_valor_variable pedirValorAMemoria(t_direccion);

void escribirValorEnMemoria(t_direccion , t_valor_variable );

t_puntero calcularPuntero(t_direccion);

t_direccion calcularDireccion(t_puntero);

t_direccion calcularNuevaDireccion();

t_direccion ultimaDireccionArg(int);

t_direccion ultimaDireccionVar(int);

bool mayor(t_direccion,t_direccion);

void asignarDireccionRespectoA(int, t_direccion*);

int paginaInicio();


t_puntero AnSISOP_definirVariable(t_nombre_variable identificador_variable){
	puts("AnSISOP_definirVariable");
	t_direccion direccion;
	t_variable* variable = malloc(sizeof(t_variable));

	//En el caso que esta sea la primera vez que el proceso entra en una CPU su indice de stack estara NULL porque no habia entradas anteriores, entonces se inicializa la primera entrada
	if(pcb->indiceStack == NULL){
		pcb->indiceStack = malloc(sizeof(t_entrada));
		pcb->indiceStack->argumentos = list_create();
		pcb->indiceStack->variables = list_create();
	}

	direccion = calcularNuevaDireccion();

	variable->ID = identificador_variable;
	variable->direccion = direccion;

	//Si es un digito es un argumento debido a la sintaxis del lenguaje, entonces se debe almacenar en argumentos
	if(isdigit(identificador_variable)){
		list_add(pcb->indiceStack[pcb->contextoActual].argumentos,variable);
	}

	//Si es una letra es una variable debido a la sintaxis del lenguaje, entonces se debe almacenar en variables
	if(isalpha(identificador_variable)){
		list_add(pcb->indiceStack[pcb->contextoActual].variables,variable);
	}

	printf("Defini %c le asigne la direccion: %d %d %d \n",identificador_variable,direccion.page,direccion.offset,direccion.size);

	return calcularPuntero(direccion);
}

t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable identificador_variable){
	puts("AnSISOP_obtenerPosicionVariable");

	t_variable *variable;
	t_puntero aRetornar;

	//Si es un digito es un argumento debido a la sintaxis del lenguaje, entonces se debe buscar en argumentos
	if(identificador_variable >= '0' && identificador_variable <= '9'){
		//Es menos 48 porque '0' es 48 es ASCII y es para obtener el valor sin hacer mucho quilombo
		variable = list_get(pcb->indiceStack[pcb->contextoActual].argumentos,identificador_variable - 48);

		aRetornar = calcularPuntero(variable->direccion);

		printf("Obtuve la posicion de %c esta es: %d \n",identificador_variable,aRetornar);

		return aRetornar;
	}else{
		//Si es una letra es una variable debido a la sintaxis del lenguaje, entonces se debe buscar en variables
		int j;
		for(j = 0; j < list_size(pcb->indiceStack[pcb->contextoActual].variables); j++){
			variable = list_get(pcb->indiceStack[pcb->contextoActual].variables,j);
			if(variable->ID == identificador_variable){

				aRetornar = calcularPuntero(variable->direccion);

				printf("Obtuve la posicion de %c esta es: %d \n",identificador_variable,aRetornar);

				return aRetornar;
			}
		}
	}
	return -1;
}

t_valor_variable AnSISOP_dereferenciar(t_puntero direccion_variable){
	puts("AnSISOP_dereferenciar");

	//Se busca la direccion
	t_direccion direccion = calcularDireccion(direccion_variable);

	//Se pide el valor a memoria
	t_valor_variable valorVariable = pedirValorAMemoria(direccion);

	printf("Desreferencie la direccion %d %d %d y el valor es %d \n",direccion.page,direccion.offset,direccion.size,valorVariable);

	return valorVariable;
}

void AnSISOP_asignar(t_puntero direccion_variable, t_valor_variable valor){
	puts("AnSISOP_asignar");

	//Calculo la direccion en memoria de la variable
	t_direccion direccion = calcularDireccion(direccion_variable);

	//Se escribe en Memoria sabiendo la posicion de memoria y el valor a escribir
	escribirValorEnMemoria(direccion,valor);

	printf("Asigne a la direccion %d %d %d el valor es %d \n",direccion.page,direccion.offset,direccion.size,valor);
}

t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida variable){
	puts("AnSISOP_obtenerValorCompartida");
	return 0;
}

t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida variable,t_valor_variable valor){
	puts("AnSISOP_asignarValorCompartida");
	return 0;
}

void AnSISOP_irAlLabel(t_nombre_etiqueta nombre_etiqueta){
	puts("AnSISOP_irAlLabel");

	t_puntero_instruccion puntero;

	//Una etiqueta es como un identificador para las funciones del programa ANSISOP
	//El numero de instruccion al que esta asociada la etiqueta es el numero de la primera instruccion ejecutable de dicha funcion
	//La funcion devuelve el numero de instruccion al que esta asociada la etiqueta
	puntero = metadata_buscar_etiqueta(nombre_etiqueta,pcb->indiceEtiquetas,pcb->cantidadDeEtiquetas);

	//Como es el numero de la siguiente instruccion a ejecutar se le asigna al ProgramCounter para que el programa siga a partir de ahi

	//ESTO ES LO QUE DEBERIA ANDAR
	//pcb->programCounter = puntero-1;


	//HARDCODEO ASQUEROSO PARA DEMOSTRAR QUE ANDA EL RESTO DE COSAS SALVO ESTA PARTE
	pcb->programCounter = 5;


}

void AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	puts("AnSISOP_llamarSinRetorno");

	//Se incrementa el contexto actual para que cada vez que se pida un valor se vaya a buscar el valor en la entrada correspondiente
	pcb->contextoActual++;

	//Se reserva espacio para una entrada mas de stack y se inicializa ambas listas de argumentos y variables para su posterior utilizacion
	pcb->indiceStack = realloc(pcb->indiceStack, sizeof(t_entrada) * (pcb->contextoActual + 1));
	pcb->indiceStack[pcb->contextoActual].argumentos = list_create();
	pcb->indiceStack[pcb->contextoActual].variables = list_create();


	//Se guarda en el contexto actual cual es la posicion de la instruccion siguiente que debe ejecutar al volver de la funcion
	pcb->indiceStack[pcb->contextoActual].retPos = pcb->programCounter;

	//Se actualiza la cantidad de entradas en el Stack
	pcb->cantidadDeEntradas++;

	//Para que luego siga la ejecucion en la funcion
	AnSISOP_irAlLabel(etiqueta);


}

void AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,t_puntero donde_retornar){
	puts("AnSISOP_llamarConRetorno");

	//Se incrementa el contexto actual para que cada vez que se pida un valor se vaya a buscar el valor en la entrada correspondiente
	pcb->contextoActual++;

	//Se reserva espacio para una entrada mas de stack y se inicializa ambas listas de argumentos y variables para su posterior utilizacion
	pcb->indiceStack = realloc(pcb->indiceStack, sizeof(t_entrada) * (pcb->contextoActual + 1));
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

void AnSISOP_finalizar(void){
	puts("AnSISOP_finalizar");

	//En ambos casos deberia de liberar la memoria de alguna manera magico-fantastica
	//Si es que hay que liberarla lo que ahora comienzo a dudar, se vera con respecto al ultimatum con el pcb

	if(pcb->contextoActual == 0){
		//Esto se da en el caso que se termine el programa
		puts("Se finalizo la ultima instruccion del main");
		terminoPrograma = true;

		//Se libera la memoria de esa entrada
		list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].argumentos, free);
		list_destroy_and_destroy_elements(pcb->indiceStack[pcb->contextoActual].variables, free);


	}else{
		//En otro caso se vuelve al contexto anterior
		puts("Se finalizo la ultima instruccion de una funcion");

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

void AnSISOP_retornar(t_valor_variable retorno){
	puts("AnSISOP_retornar");

	//Se escribe el valor devuelto por la funcion en la direccion de retorno
	escribirValorEnMemoria(pcb->indiceStack[pcb->contextoActual].retVar , retorno );

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

void AnSISOP_wait(t_nombre_semaforo identificador_semaforo){
	puts("AnSISOP_wait");

}

void AnSISOP_signal(t_nombre_semaforo identificador_semaforo){
	puts("AnSISOP_signal");

}

t_puntero AnSISOP_reservar(t_valor_variable espacio){
	puts("AnSISOP_reservar");
	return 0x10;
}

void AnSISOP_liberar(t_puntero puntero){
	puts("AnSISOP_liberar");

}

t_descriptor_archivo AnSISOP_abrir(t_direccion_archivo direccion,t_banderas flags){
	puts("AnSISOP_abrir");
	return 0;
}

void AnSISOP_borrar(t_descriptor_archivo direccion){
	puts("AnSISOP_borrar");

}

void AnSISOP_cerrar(t_descriptor_archivo descriptor_archivo){
	puts("AnSISOP_cerrar");

}

void AnSISOP_moverCursor(t_descriptor_archivo descriptor_archivo,t_valor_variable posicion){
	puts("AnSISOP_moverCursor");

}

void AnSISOP_escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	puts("AnSISOP_escribir");
	t_mensajeDeProceso mensajeDeProceso;
	mensajeDeProceso.pid = pcb->pid;
	mensajeDeProceso.mensaje = informacion;
	printf("%s \n",mensajeDeProceso.mensaje);
	//enviarMensaje(socketKernel,4,&mensajeDeProceso,tamanio + sizeof(int));

	//intentar arreglar esto de otra forma
}

void AnSISOP_leer(t_descriptor_archivo descriptor_archivo,t_puntero informacion, t_valor_variable tamanio){
	puts("AnSISOP_leer");
}




/**************************************************************************FUNCIONES PRIVADAS**********************************************************************************************/



t_valor_variable pedirValorAMemoria(t_direccion direccion){

	t_valor_variable valorVariable;
	t_pedidoMemoria pedido = {
			.id = pcb->pid,
			.direccion = direccion
	};


	//Se pide a Memoria el contenido de esa posicion que es el valor de la variable
	enviarMensaje(socketMemoria,solicitarBytes,(void *)&pedido, sizeof(pedido));

	printf("Se pide el valor de la variable en la direccion de memoria: %d %d %d \n",direccion.page,direccion.offset,direccion.size);

	//se recibe el valor de la variable
	void* stream;
	int* valor;
	int* booleano;

	int accion = recibirMensaje(socketMemoria,&stream);

		switch(accion){
				case RespuestaBooleanaDeMemoria:{
					booleano = stream;
				}break;
				default:{
					perror("Error en la accion maquinola");
				}break;
			}
		if(*booleano){
			free(stream);
			accion = recibirMensaje(socketMemoria,&stream);
				switch(accion){
					case lineaDeCodigo:{
						valor = stream;
						valorVariable = *valor;
					}break;
					default:{
						perror("Error en la accion maquinola");
					}break;
				}
		}else{
			terminoPrograma = true;
			pcb->exitCode = -5;		//Excepcion de Memoria STACK OVERFLOW
		}

	free(stream);

	return valorVariable;

	//ESTO PARECIA MEJOR PERO NO ANDA, DESPUES REVISALO

/*
	if(recibirMensaje(socketMemoria,&stream) == RespuestaBooleanaDeMemoria){
		booleano = stream;
	}
	else{
		perror("Error en la accion maquinola");
	};

	if(*booleano != 1){
		free(stream);
		if(recibirMensaje(socketMemoria,&stream) == lineaDeCodigo){
			valor = stream;
			valorVariable = *valor;
		}else{
			perror("Error en la accion maquinola");
		}
	}else{
		terminoPrograma = true;
		pcb->exitCode = -5;		//Excepcion de Memoria STACK OVERFLOW
	}
*/
}

void escribirValorEnMemoria(t_direccion direccion, t_valor_variable valor){

	//Se crea el void* que contiene el pid, la direccion y el valor a escribir, que son los datos necesarios para que la memoria escriba el valor
	void* auxiliar = malloc(sizeof(t_escrituraMemoria));
	memcpy(auxiliar,&pcb->pid,sizeof(int));
	memcpy(auxiliar + sizeof(int),&direccion,sizeof(t_direccion));
	memcpy(auxiliar + sizeof(int) * 4,&valor,sizeof(t_valor_variable));

	//se pide a memoria que escriba el valor enviado en la posicion de memoria tambien enviada
	enviarMensaje(socketMemoria,almacenarBytes,auxiliar,sizeof(t_escrituraMemoria));

	printf("Se escribio el valor %d en la direccion de memoria: %d %d %d \n",valor,direccion.page,direccion.offset,direccion.size);

	free(auxiliar);


	//Devuelve un OK o mata con un Stack Overflow
	void* stream;
	int* respuesta;
	int accion = recibirMensaje(socketMemoria,&stream);
	switch(accion){
		case RespuestaBooleanaDeMemoria:{
			respuesta = stream;
			break;
		}
		default:{
			perror("Error en la accion maquinola");
		}
	}
//ESTO PARECIA MEJOR PERO NO ANDA, DESPUES REVISALO
/*	if(recibirMensaje(socketMemoria,&stream) == RespuestaBooleanaDeMemoria){
		respuesta = stream;
	}else{
		perror("Error en la accion maquinola");
	}
*/
	if(*respuesta != 1){
		terminoPrograma = true;
		pcb->exitCode = -5;
	}

	free(stream);
}






t_direccion calcularNuevaDireccion(){
	t_direccion direccion;
	t_list* argumentos = pcb->indiceStack[pcb->contextoActual].argumentos;
	t_list* variables = pcb->indiceStack[pcb->contextoActual].variables;

	//Si es la primera vez que se pide una direccion para este contexto
	if(list_size(argumentos)==0 && list_size(variables)==0){

		//Si es la primera vez que se pide una direccion para este proceso se le asigna la primera posicion de su stack
		if(pcb->contextoActual == 0){
			direccion.page = paginaInicio();
			direccion.offset = 0;
		}else{

		//Si es la primera vez que se pide una direccion para este contexto se debe revisar cual fue la ultima posicion asignada en el contexto anterior para comenzar desde ahi
			asignarDireccionRespectoA(pcb->contextoActual-1, &direccion);
		}
	}else{
		asignarDireccionRespectoA(pcb->contextoActual, &direccion);
	}

	if(direccion.offset > datosIniciales->size_pag){
		direccion.page++;
		direccion.offset = 0;
	}

	direccion.size = 4;

	printf("Calcule una nueva direccion jaja saludos, esta es: %d %d %d \n",direccion.page,direccion.offset,direccion.size);

	return direccion;
}

//NO HARDCODEAR EL TAMANIO DE LA PAGINA
t_puntero calcularPuntero(t_direccion direccion){
	t_puntero puntero = direccion.page * datosIniciales->size_pag + direccion.offset;
	printf("En base a la direccion %d %d %d calculo la posicion %d \n",direccion.page,direccion.offset,direccion.size,puntero);
	return puntero;
}

//NO HARDCODEAR EL TAMANIO DE LA PAGINA
t_direccion calcularDireccion(t_puntero puntero){
	t_direccion direccion;

	if(datosIniciales->size_pag > puntero){
		direccion.page = 1;
		direccion.offset = puntero;
	}else{
		direccion.page = puntero/datosIniciales->size_pag;
		direccion.offset = puntero % datosIniciales->size_pag;
	}
	direccion.size = 4;

	printf("En base a la posicion %d calculo la direccion %d %d %d \n",puntero,direccion.page,direccion.offset,direccion.size);

	return direccion;
}

void asignarDireccion(t_direccion* direccion,void* stream){
	direccion = stream;
}

t_direccion ultimaDireccionArg(int contexto){

	t_direccion direccion;
	t_variable* variable;

	//Si no hay elementos en la lista devuelve una direccion erronea
	if(list_is_empty(pcb->indiceStack[contexto].argumentos)){
		direccion.page = -1;
		direccion.offset = -1;
		direccion.size = -1;
	}else{
		//Devuelve el ultimo elemento de la lista de argumentos
		variable = list_get(pcb->indiceStack[contexto].argumentos,list_size(pcb->indiceStack[contexto].argumentos)-1);
		direccion = variable->direccion;
	}

	return direccion;
}

t_direccion ultimaDireccionVar(int contexto){

	t_direccion direccion;
	t_variable* variable;

	//Si no hay elementos en la lista devuelve una direccion erronea
	if(list_is_empty(pcb->indiceStack[contexto].variables)){
		direccion.page = -1;
		direccion.offset = -1;
		direccion.size = -1;
	}else{
		//Devuelve el ultimo elemento de la lista de variables
		variable = list_get(pcb->indiceStack[contexto].variables,list_size(pcb->indiceStack[contexto].variables)-1);
		direccion = variable->direccion;
	}

	return direccion;
}

//Devuelve si la primer direccion es mayor a la segunda
bool mayor(t_direccion unaDireccion,t_direccion otraDireccion){

	if(unaDireccion.page == otraDireccion.page){
		return unaDireccion.offset > otraDireccion.offset;
	}else{
		return unaDireccion.page > otraDireccion.page;
	}

}


//Esto es para no repetir codigo, era muy asqueroso sino
void asignarDireccionRespectoA(int contexto, t_direccion* direccion){

	t_direccion variable = ultimaDireccionVar(contexto);
	t_direccion argumento = ultimaDireccionArg(contexto);

	if(mayor(variable,argumento)){
		direccion->page = variable.page;
		direccion->offset = variable.offset + 4;
	}else{
		direccion->page = argumento.page;
		direccion->offset = argumento.offset + 4;
	}
}

//Devuelve la pagina donde comienza en el stack
int paginaInicio(){
	return pcb->contPags_pcb - datosIniciales->size_stack + 1;
}
