#include "primitivas.h"


t_valor_variable pedirValorAMemoria(t_direccion);

void escribirirValorEnMemoria(t_direccion , t_valor_variable );

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
	t_variable variable;
	variable.ID = identificador_variable;

	direccion = calcularNuevaDireccion();
	variable.direccion = direccion;

	//Si es un digito es un argumento debido a la sintaxis del lenguaje, entonces se debe almacenar en argumentos
	if(isdigit(identificador_variable)){
		list_add(pcb.indiceStack[pcb.contextoActual].argumentos,&variable);
	}

	//Si es una letra es una variable debido a la sintaxis del lenguaje, entonces se debe almacenar en variables
	if(isalpha(identificador_variable)){
		list_add(pcb.indiceStack[pcb.contextoActual].variables,&variable);
	}

	if(identificador_variable == 'a') dirA = direccion;
	if(identificador_variable == 'b') dirB = direccion;

	printf("Defini %c le asigne la direccion: %d %d %d \n",identificador_variable,direccion.page,direccion.offset,direccion.size);

	return calcularPuntero(direccion);
}

t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable identificador_variable){
	puts("AnSISOP_obtenerPosicionVariable");

	t_variable *variable;
	t_puntero aRetornar;

	bool condition(t_variable* variableCondicion){

		return variableCondicion->ID == identificador_variable;
	}

	//Si es un digito es un argumento debido a la sintaxis del lenguaje, entonces se debe buscar en argumentos
	if(identificador_variable >= '0' && identificador_variable <= '9'){
		variable = list_find(pcb.indiceStack[pcb.contextoActual].argumentos,condition);
	}else{
		//Si es una letra es una variable debido a la sintaxis del lenguaje, entonces se debe buscar en variables
		variable = list_find(pcb.indiceStack[pcb.contextoActual].variables,condition);
	}

	printf("%c %d %d %d",variable->ID,variable->direccion.page,variable->direccion.offset,variable->direccion.size);

	aRetornar = calcularPuntero(variable->direccion);

	printf("Obtuve la posicion de %c esta es: %d \n",identificador_variable,aRetornar);

	return aRetornar;
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
	escribirirValorEnMemoria(direccion,valor);

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
	puntero = metadata_buscar_etiqueta(nombre_etiqueta,pcb.indiceEtiquetas,pcb.cantidadDeEtiquetas);

	//Como es el numero de la siguiente instruccion a ejecutar se le asigna al ProgramCounter para que el programa siga a partir de ahi
	pcb.programCounter = puntero;

}

void AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	puts("AnSISOP_llamarSinRetorno");
	pcb.contextoActual++;
	//CLARAMENTE ACA HABRIA QUE HACER UN REALLOC PARA GUARDAR EL TAMANIO DE ESA SIGUIENTE ENTRADA

	//Se guarda en el contexto actual cual es la posicion de la instruccion siguiente que debe ejecutar al volver de la funcion
	pcb.indiceStack[pcb.contextoActual].retPos = pcb.programCounter + 1;

	//Para que luego siga la ejecucion en la funcion
	AnSISOP_irAlLabel(etiqueta);
}

void AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,t_puntero donde_retornar){
	puts("AnSISOP_llamarConRetorno");

	pcb.contextoActual++;
	//CLARAMENTE ACA HABRIA QUE HACER UN REALLOC PARA GUARDAR EL TAMANIO DE ESA SIGUIENTE ENTRADA

	//Se guarda en el contexto actual cual es la posicion de la instruccion siguiente que debe ejecutar al volver de la funcion
	pcb.indiceStack[pcb.contextoActual].retPos = pcb.programCounter + 1;

	//Se guarda en el contexto actual cual es la direccion de la variable a la que se le asignara el valor que retornara esta funcion
	pcb.indiceStack[pcb.contextoActual].retVar = calcularDireccion(donde_retornar);

	//Para que luego siga la ejecucion en la funcion
	AnSISOP_irAlLabel(etiqueta);
}

void AnSISOP_finalizar(void){
	puts("AnSISOP_finalizar");

	//En ambos casos deberia de liberar la memoria de alguna manera magico-fantastica
	//Si es que hay que liberarla lo que ahora comienzo a dudar, se vera con respecto al ultimatum con el pcb

	if(pcb.contextoActual == 0){
		//Esto se da en el caso que se termine el programa
		puts("Se finalizo la ultima instruccion del main");
		terminoPrograma = true;
	}else{
		//En otro caso se vuelve al contexto anterior
		puts("Se finalizo la ultima instruccion de una funcion");
		pcb.contextoActual--;
	}
}

void AnSISOP_retornar(t_valor_variable retorno){
	puts("AnSISOP_retornar");

	//Se escribe el valor devuelto por la funcion en la direccion de retorno
	escribirirValorEnMemoria(pcb.indiceStack[pcb.contextoActual].retVar , retorno );

	//Se deberia liberar la memoria de alguna manera magico-fantastica
	//Si es que hay que liberarla lo que ahora comienzo a dudar, se vera con respecto al ultimatum con el pcb
	pcb.contextoActual--;
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
	mensajeDeProceso.pid = pcb.pid;
	mensajeDeProceso.mensaje = informacion;
	printf("%d   %s",mensajeDeProceso.pid,mensajeDeProceso.mensaje);
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
			.id = pcb.pid,
			.direccion = direccion
	};

/*
	//Se pide a Memoria el contenido de esa posicion que es el valor de la variable
	enviarMensaje(socketMemoria,5,(void *)&pedido, sizeof(pedido));
	//se recibe el valor de la variable
	recibirMensaje(socketMemoria,(void*)&valorVariable);
*/
	if(direccion.page == dirA.page && direccion.offset == dirA.offset && direccion.size == dirA.size) return a;
	if(direccion.page == dirB.page && direccion.offset == dirB.offset && direccion.size == dirB.size) return b;

	return valorVariable;
}

void escribirirValorEnMemoria(t_direccion direccion, t_valor_variable valor){

	t_escrituraMemoria escritura = {
			.id = pcb.pid,
			.direccion = direccion,
			.valor = valor
	};

	if(direccion.page == dirA.page && direccion.offset == dirA.offset && direccion.size == dirA.size) a = valor;
	if(direccion.page == dirB.page && direccion.offset == dirB.offset && direccion.size == dirB.size) b = valor;

/*
	//se pide a memoria que escriba el valor enviado en la posicion de memoria tambien enviada
	enviarMensaje(socketMemoria,6,(void*)&escritura,sizeof(escritura));
*/
}






t_direccion calcularNuevaDireccion(){
	t_direccion direccion;
	t_list* argumentos = pcb.indiceStack[pcb.contextoActual].argumentos;
	t_list* variables = pcb.indiceStack[pcb.contextoActual].variables;

	//Si es la primera vez que se pide una direccion para este contexto
	if(list_size(argumentos)==0 && list_size(variables)==0){

		//Si es la primera vez que se pide una direccion para este proceso se le asigna la primera posicion de su stack
		if(pcb.contextoActual == 0){
			direccion.page = paginaInicio();
			direccion.offset = 0;
		}else{

		//Si es la primera vez que se pide una direccion para este contexto se debe revisar cual fue la ultima posicion asignada en el contexto anterior para comenzar desde ahi
			asignarDireccionRespectoA(pcb.contextoActual-1, &direccion);
		}
	}else{
		asignarDireccionRespectoA(pcb.contextoActual, &direccion);
	}

	if(direccion.offset > tamanioPagina){
		direccion.page++;
		direccion.offset = 0;
	}

	direccion.size = 4;

	printf("Calcule una nueva direccion jaja saludos, esta es: %d %d %d \n",direccion.page,direccion.offset,direccion.size);

	return direccion;
}

//NO HARDCODEAR EL TAMANIO DE LA PAGINA
t_puntero calcularPuntero(t_direccion direccion){
	t_puntero puntero = direccion.page * tamanioPagina + direccion.offset;
	printf("En base a la direccion %d %d %d calculo la posicion %d \n",direccion.page,direccion.offset,direccion.size,puntero);
	return puntero;
}

//NO HARDCODEAR EL TAMANIO DE LA PAGINA
t_direccion calcularDireccion(t_puntero puntero){
	t_direccion direccion;

	if(tamanioPagina > puntero){
		direccion.page = 0;
		direccion.offset = puntero;
	}else{
		direccion.page = puntero/tamanioPagina;
		direccion.offset = puntero % tamanioPagina;
	}
	direccion.size = 4;

	printf("En base a la posicion %d calculo la direccion %d %d %d \n",direccion.page,direccion.offset,direccion.size,puntero);

	return direccion;
}

void asignarDireccion(t_direccion* direccion,void* stream){
	direccion = stream;
}

//MODIFICAR CUANDO HAYA SERIALIZADOR PROBABLEMENTE, SINO REALIZAR LIST_LAST(T_LIST*)
t_direccion ultimaDireccionArg(int contexto){

	t_direccion direccion;
	t_variable* variable;

	//Si no hay elementos en la lista devuelve una direccion erronea
	if(list_is_empty(pcb.indiceStack[contexto].argumentos)){
		direccion.page = -1;
		direccion.offset = -1;
		direccion.size = -1;
	}else{
		//Devuelve el ultimo elemento de la lista de argumentos
		variable = list_get(pcb.indiceStack[contexto].argumentos,list_size(pcb.indiceStack[contexto].argumentos)-1);
		direccion = variable->direccion;
	}

	return direccion;
}
//MODIFICAR CUANDO HAYA SERIALIZADOR PROBABLEMENTE, SINO REALIZAR LIST_LAST(T_LIST*)
t_direccion ultimaDireccionVar(int contexto){

	t_direccion direccion;
	t_variable* variable;

	//Si no hay elementos en la lista devuelve una direccion erronea
	if(list_is_empty(pcb.indiceStack[contexto].variables)){
		direccion.page = -1;
		direccion.offset = -1;
		direccion.size = -1;
	}else{
		//Devuelve el ultimo elemento de la lista de variables
		variable = list_get(pcb.indiceStack[contexto].variables,list_size(pcb.indiceStack[contexto].variables)-1);
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
	//ASUMIENDO QUE ESE CAMPO DEL PCB TIENE EL VALOR DE LA CANTIDAD DE PAGINAS DE CODIGO, HABLAR CON LOS CHICOS QUE ES  CONSULTAR
	return pcb.contPags_pcb + 1;
}
