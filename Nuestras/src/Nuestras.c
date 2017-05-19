/*
 ============================================================================
 Name        : Nuestras.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <stdint.h>


typedef struct{
 int page;
 int offset;
 int size;
}__attribute__((packed))t_direccion;

typedef struct{
 char ID;
 t_direccion direccion;
}__attribute__((packed))t_variable;

typedef struct{
 t_list* argumentos;
 t_list* variables;
 int retPos;
 t_direccion retVar;
}__attribute__((packed))t_entrada;



typedef struct{
 int pid;
 t_puntero_instruccion programCounter;

 int cantidadDeInstrucciones;//Sirve para el indice de codigo, Es un contador para meterlo en un for

 int contPags_pcb;
 int contextoActual;
 t_intructions* indiceCodigo;//Es un array
char* indiceEtiquetas; // ¡??¡?'¿¿'??¡ que es¡?
 int cantidadDeEntradas;//Es un contador para meterlo en un for
 int cantidadDeEtiquetas;
 t_entrada* indiceStack;//Es otro array


 int exitCode;
}__attribute__((packed)) PCB_DATA;



//-----CALCULO DE TAMAÑOS

int tamanoDeListaDe__t_variable(t_list * lista){
	//***Defino el tamaño de un elemento, que va a ser el tamaño de un t_variable
	int tamanoDeElemento = sizeof(t_variable);

	//***Defino el tamaño total de la lista que va a ser la cant de elementos * el tamaño de cada elemento,
	//***le sumo un int para poder leer en el deserializador el tamaño de la lista
	int tamanoTotalDeLista = (list_size(lista) * tamanoDeElemento);

	return tamanoTotalDeLista;
}

int tamanoDe__t_instructions(int cantidadDeInstrucciones){
	return sizeof(t_intructions)*cantidadDeInstrucciones;
}

int tamanoDe__t_entrada(t_entrada entrada){
	return    tamanoDeListaDe__t_variable(entrada.argumentos)
			+ tamanoDeListaDe__t_variable(entrada.variables)
			+ sizeof(uint32_t)
			+ sizeof(t_direccion)
			+ sizeof(uint32_t) * 2; //Por los headers de las listas
}

int tamanoDe__t_entradas(t_entrada* entradas, int cantidadDeEntradas){
	///***Esta asignacion es para poder guardar el tamaño de las entradas
	int tamano = sizeof(uint32_t);// * cantidadDeEntradas; //Para guardar los headers de las entradas
	int i;
	for(i = 0; i<cantidadDeEntradas ; i++){
		tamano += tamanoDe__t_entrada(entradas[i]);
	}
	return tamano;
}

int tamanoPCB(PCB_DATA * pcb){
	int tamanoEtiquetas;
	if(pcb->cantidadDeEtiquetas == 0){
		tamanoEtiquetas = 0;
	}
	else{
		tamanoEtiquetas = (strlen(pcb->indiceEtiquetas)+1);
	}
		return 	sizeof(uint32_t)  												//por el pid
				+sizeof(t_puntero_instruccion)  								//por el program counter
				+sizeof(uint32_t) 												//por el contador de paginas
				+sizeof(uint32_t) 												//por el contexto actual

				+sizeof(uint32_t) 												//por la cantidad de instrucciones
				+tamanoDe__t_instructions(pcb->cantidadDeInstrucciones)			//por las instrucciones

				+tamanoEtiquetas								//por el indice de stack ---- esto no se sabe si es un char*

				+sizeof(uint32_t) 												//por la cantidad de entradas
				+tamanoDe__t_entradas(pcb->indiceStack,pcb->cantidadDeEntradas)//por las entradas
				+sizeof(uint32_t)												//por la cantidadDeEtiquetas
				+sizeof(uint32_t)*2;												//por el exit code
				//+sizeof(uint32_t)*2; 											//Creo que es por los headers de las listas
}


int obtenerTamanoProximoBloque(void * stream, int *recorrido){//***Tiene doble funcionalidad, porque tambien corre el stream
	int tamanoProximoBloque;
	memcpy(&tamanoProximoBloque, stream + *recorrido, sizeof(uint32_t));
	*recorrido += sizeof(uint32_t);
	return tamanoProximoBloque;
}

uint32_t leerUINT32(void * stream, int* recorrido){//***Tiene doble funcionalidad, porque tambien corre el stream
	return obtenerTamanoProximoBloque(stream, recorrido);
}


void * serializarListaDe__t_variable(t_list * lista){

	int tamanoDeElemento = sizeof(t_variable);
	int tamanoTotalDeLista = tamanoDeListaDe__t_variable(lista);

	//***Hago un malloc donde asigno el tamaño a un chorro de bytes, y le copio el tamaño de la lista al principio
	void * stream = malloc(tamanoTotalDeLista + sizeof(uint32_t));
	memcpy(stream, &tamanoTotalDeLista, sizeof(uint32_t));

	//***Muevo el recorrido a la posicion siguiente
	int recorrido = sizeof(uint32_t);

	int i;
	for(i = 0; i < list_size(lista);i++){
		//***Consigo un elemento de la lista
		void * elementoACopiar = list_get(lista, i);

		//***Copio el elemento a la posicion siguiente
		memcpy(stream + recorrido, elementoACopiar, tamanoDeElemento);

		//***Muevo la posicion, agregando el tamaño del elemento que acabo de agregar
		recorrido += tamanoDeElemento;
	}

	return stream;
}

t_variable* copiarVariable(t_variable var){
	t_variable* aux = malloc(sizeof(t_variable*));
	*aux = var;
	return aux;
}

t_variable * leer__t_variable(t_variable * entrada, int pos){
	t_variable * valor = malloc(sizeof(t_variable));
	memcpy(valor, entrada+pos, sizeof(t_variable));
	return valor;
}

t_list * deserializarListaDe__t_variable(void * stream, int tamanoLista, int *posicion){

	int cantidadDeElementos = tamanoLista / sizeof(t_variable);

	t_list * lista = list_create();
	int i = 0;
	int recorrido = *posicion;

	t_variable **elemento = malloc(sizeof(t_variable)*cantidadDeElementos);

	int recorridoArray = 0;
	for(i = 0; i<cantidadDeElementos; i++){
		*elemento = malloc(sizeof(t_variable));
		memcpy(*elemento,stream + recorrido, sizeof(t_variable));
		list_add(lista, *elemento);
		recorrido += sizeof(t_variable);
	}
	free(elemento);
	return lista;
}


void * serializarUINT32(uint32_t valor){
	void * stream = malloc(sizeof(uint32_t));
	memcpy(stream, &valor, sizeof(uint32_t));
	return stream;
}

void * serializar__t_direccion(t_direccion valor){
	void * stream = malloc(sizeof(t_direccion));
	memcpy(stream, &valor, sizeof(t_direccion));
	return stream;
}

t_direccion deserializar__t_direccion(void *stream, int * posicion){
	t_direccion direccion;

	memcpy(&direccion, stream + *posicion, sizeof(t_direccion));
	*posicion += sizeof(t_direccion);

	return direccion;
}


void * serializarEntrada(t_entrada entrada){

	int tamanoEntrada = tamanoDe__t_entrada(entrada);
	//***Le asigno el tamaño del bloque mas su header
	void * stream = malloc(tamanoEntrada);//Deberia sumarle el header?
	//**Le copio el header
	//memcpy(stream, &tamanoEntrada, sizeof(uint32_t));
	//**Muevo El recorrido
	int recorrido = 0;

	void * listaDeArgumentosSerializada = serializarListaDe__t_variable(entrada.argumentos);
	int tamanoListaDeArgumentosSerializada = tamanoDeListaDe__t_variable(entrada.argumentos);

	memcpy(stream + recorrido, listaDeArgumentosSerializada, tamanoListaDeArgumentosSerializada + sizeof(uint32_t));
	free(listaDeArgumentosSerializada);//---Creo que hay un memoryLeak


	recorrido += tamanoListaDeArgumentosSerializada + sizeof(uint32_t);

	void * listaDeVariablesSerializada = serializarListaDe__t_variable(entrada.variables);
	int tamanoListaDeVariablesSerializada = tamanoDeListaDe__t_variable(entrada.variables);
	memcpy(stream + recorrido, listaDeVariablesSerializada, tamanoListaDeVariablesSerializada+ sizeof(uint32_t));
	free(listaDeVariablesSerializada);

	recorrido += tamanoListaDeVariablesSerializada + sizeof(uint32_t);

	void * retPosSerializado = serializarUINT32(entrada.retPos);
	memcpy(stream + recorrido, retPosSerializado, sizeof(uint32_t));
	free(retPosSerializado);

	recorrido += sizeof(uint32_t);

	void * retVarSerializado = serializar__t_direccion(entrada.retVar);
	memcpy(stream + recorrido, retVarSerializado, sizeof(t_direccion));
	free(retVarSerializado);

	return stream;
}

t_entrada deserializarEntrada(void* stream, int *posicion){

	t_entrada entrada;


	int tamanoListaDeArgumentos;
	tamanoListaDeArgumentos = obtenerTamanoProximoBloque(stream, posicion);
	entrada.argumentos = deserializarListaDe__t_variable(stream, tamanoListaDeArgumentos, posicion);

	/*uint32_t asd = ((t_variable*)list_get(entrada.argumentos, 0))->direccion.offset;
	printf("%d", asd);*/

	*posicion += tamanoListaDeArgumentos;

	int tamanoListaDeVariables = obtenerTamanoProximoBloque(stream, posicion);
	entrada.variables= deserializarListaDe__t_variable(stream, tamanoListaDeVariables, posicion);

	*posicion += tamanoListaDeArgumentos;

	entrada.retPos = leerUINT32(stream, posicion);//Esto ya corre el stream

	entrada.retVar = deserializar__t_direccion(stream, posicion);

	//*posicion -= 4;
	//obtenerTamanoProximoBloque(stream, posicion);

	return entrada;

}


void * serializarEntradas(t_entrada* entradas, int cantidadDeEntradas){
	int tamanoEntradas = tamanoDe__t_entradas(entradas,cantidadDeEntradas);
	void * stream = malloc(tamanoEntradas);
	//***Agrego el tamaño
	memcpy(stream, &tamanoEntradas, sizeof(uint32_t));

	int i=0;
	int recorrido = sizeof(uint32_t);

	for(i = 0; i<cantidadDeEntradas; i++){
		int tamanoEntradaActual = tamanoDe__t_entrada(entradas[i]);

		void * aux = serializarEntrada(entradas[i]);
		memcpy(stream + recorrido, aux, tamanoEntradaActual);
		free(aux);

		recorrido += tamanoEntradaActual;
	}
	return stream;
}

t_entrada* deserializarEntradas(void * stream, int* pos, int cantidadDeEntradas){
	int i=0;
	int tamano = obtenerTamanoProximoBloque(stream, pos);
	//t_entrada * entradas = malloc(sizeof(t_entrada)*cantidadDeEntradas);
	t_entrada * entradas = malloc(sizeof(t_entrada)*cantidadDeEntradas);////--------------------esto fue lo ultimo que toque
	//int tamanoEntrada = obtenerTamanoProximoBloque(stream, pos);
	for(i=0; i< cantidadDeEntradas; i++){
		entradas[i] = deserializarEntrada(stream, pos);
		///obtenerTamanoProximoBloque(stream, pos); /////-------------PUEDE HABER UN PROBLEMA CON ESTA LINEA EN LA ULTIMA ITERACION
	}
	return entradas;
}

void * serializar__t_instructions(t_intructions* indiceCodigo, uint32_t cantidadDeInstrucciones){
	void * stream = malloc(tamanoDe__t_instructions(cantidadDeInstrucciones));
	int i = 0;
	int recorrido = 0;
	for(i = 0; i<cantidadDeInstrucciones;i++){
		memcpy(stream + recorrido, &(indiceCodigo[i]), sizeof(t_intructions));
		recorrido += sizeof(t_intructions);
	}
	return stream;
}

t_intructions * deserializar__t_instructions(void* stream, int* pos, int cantidadDeInstrucciones){
	int tamanoInst = tamanoDe__t_instructions(cantidadDeInstrucciones);
	t_intructions* instrucciones = malloc(sizeof(t_intructions)*cantidadDeInstrucciones);
	int i = 0;
	for(i = 0; i<cantidadDeInstrucciones;i++){
		memcpy(&instrucciones[i],stream + *pos, sizeof(t_intructions));
		*pos += sizeof(t_intructions);
	}
	return instrucciones;
}

void *  serializarPCB(PCB_DATA * pcb){
	int recorrido = 0;
	int tamanoPcb = tamanoPCB(pcb);
	void * stream = malloc(tamanoPcb + sizeof(uint32_t));//el tamaño del pcb mas su header

	memcpy(stream, &tamanoPcb, sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	memcpy(stream + recorrido, &(pcb->cantidadDeEntradas), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);


	void* indiceStack = serializarEntradas(pcb->indiceStack, pcb->cantidadDeEntradas);
	int tamanoEntradas = tamanoDe__t_entradas(pcb->indiceStack, pcb->cantidadDeEntradas);
	memcpy(stream + recorrido, indiceStack, tamanoEntradas);
	free(indiceStack);
	recorrido+=tamanoEntradas;


	memcpy(stream+recorrido, &(pcb->pid), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	memcpy(stream + recorrido, &(pcb->programCounter), sizeof(t_puntero_instruccion));
	recorrido += sizeof(t_puntero_instruccion);

	memcpy(stream + recorrido, &(pcb->cantidadDeInstrucciones), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	memcpy(stream + recorrido, &(pcb->contPags_pcb), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	memcpy(stream + recorrido, &(pcb->contextoActual), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	void* indiceCodigo = serializar__t_instructions(pcb->indiceCodigo, pcb->cantidadDeInstrucciones);
	int tamanoInstrucciones = tamanoDe__t_instructions(pcb->cantidadDeInstrucciones);
	memcpy(stream+ recorrido,indiceCodigo, tamanoInstrucciones);
	free(indiceCodigo);
	recorrido += tamanoInstrucciones;


	//****Esto no se si hay que cambiarlo
	int largoIndiceEtiquetas = 0;
	if(pcb->cantidadDeEtiquetas != 0){
		largoIndiceEtiquetas = strlen(pcb->indiceEtiquetas)+1;
	}

	//int largoIndiceEtiquetas = strlen(pcb->indiceEtiquetas)+1;//es el largo del char*
	memcpy(stream + recorrido, &largoIndiceEtiquetas, sizeof(uint32_t));
	recorrido += sizeof(uint32_t);
	memcpy(stream + recorrido, pcb->indiceEtiquetas, largoIndiceEtiquetas);
	recorrido += largoIndiceEtiquetas;





	memcpy(stream + recorrido, &(pcb->cantidadDeEtiquetas), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);


	memcpy(stream + recorrido, &(pcb->exitCode), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	 if(recorrido == tamanoPCB(pcb)) printf("LA TENGO ENORME");

	 return stream;
}

PCB_DATA* deserializarPCB(void* stream){

	int recorrido = 0;
	int tamanoPCB = leerUINT32(stream, &recorrido);


	PCB_DATA * pcb = malloc(tamanoPCB);
	//PCB_DATA pcb;

	int cantidadEntradas = leerUINT32(stream, &recorrido);


	pcb->indiceStack = deserializarEntradas(stream, &recorrido, cantidadEntradas);
	pcb->cantidadDeEntradas = cantidadEntradas;


	pcb->pid = leerUINT32(stream, &recorrido);
	pcb->programCounter = leerUINT32(stream, &recorrido);
	pcb->cantidadDeInstrucciones = leerUINT32(stream,&recorrido);
	pcb->contPags_pcb = leerUINT32(stream,&recorrido);
	pcb->contextoActual = leerUINT32(stream,&recorrido);
	pcb->indiceCodigo = deserializar__t_instructions(stream,&recorrido,pcb->cantidadDeInstrucciones);

	int largoIndiceEtiquetas = leerUINT32(stream, &recorrido);
	pcb->indiceEtiquetas = malloc(largoIndiceEtiquetas);
	memcpy(pcb->indiceEtiquetas, stream + recorrido, largoIndiceEtiquetas);
	recorrido += largoIndiceEtiquetas;



	pcb->cantidadDeEtiquetas = leerUINT32(stream, &recorrido);

	pcb->exitCode = leerUINT32(stream,&recorrido);

	return pcb;
}


//----TEST DE SERIALIZACION---

/*
void pruebaSerializarLista(){
		t_variable * a = malloc(sizeof(t_variable));
		a->ID = 's';

		t_direccion c;
		c.offset = 234;
		c.page = 1234;
		c.size = 231;

		a->direccion = c;

		t_variable * b= malloc(sizeof(t_variable));
		b->ID = 'h';
		b->direccion = c;

		t_list * lista1= list_create();

		list_add(lista1,a);
		list_add(lista1,b);

		void * stream = serializarListaDe__t_variable(lista1);

		uint32_t var;
		memcpy(&var, stream, sizeof(uint32_t));

		int tamanoTotal = var + sizeof(uint32_t);

		int tamanoBloque = obtenerTamanoProximoBloque(stream, &tamanoTotal);

		t_list * lista2 = deserializarListaDe__t_variable(stream, tamanoBloque);

		printf("El tamaño es: %d", var);

		int algo = ((t_variable *)list_get(lista2,0))->direccion.offset;
		printf("LLegue al final y consegui a->offset: %d", algo);
}*/

void pruebaSerializarEntrada(){
		t_variable * a = malloc(sizeof(t_variable));
		a->ID = 's';

		t_direccion c;
		c.offset = 234;
		c.page = 1234;
		c.size = 231;

		a->direccion = c;

		t_variable * b= malloc(sizeof(t_variable));
		b->ID = 'h';
		b->direccion = c;

		t_list * lista1= list_create();

		list_add(lista1,a);
		list_add(lista1,b);

		t_entrada entrada;
		entrada.argumentos = lista1;
		entrada.variables = lista1;
		entrada.retPos = 42;
		entrada.retVar = c;

		void* stream = serializarEntrada(entrada);


		int pos = 0;
		int aas = obtenerTamanoProximoBloque(stream, &pos);
		t_entrada entrada2 = deserializarEntrada(stream, &pos);

		printf("\n\n\n\nEl valor del retPos es: %d", entrada2.retPos);
}

void pruebaSerializarEntradas(){
		t_variable * a = malloc(sizeof(t_variable));
		a->ID = 's';

		t_direccion c;
		c.offset = 234;
		c.page = 1234;
		c.size = 231;

		a->direccion = c;

		t_variable * b= malloc(sizeof(t_variable));
		b->ID = 'h';
		b->direccion = c;

		t_list * lista1= list_create();

		list_add(lista1,a);
		list_add(lista1,b);

		t_entrada entrada;
		entrada.argumentos = lista1;
		entrada.variables = lista1;
		entrada.retPos = 42;
		entrada.retVar = c;

		t_entrada * entradas = malloc(sizeof(entrada)*5);
		entradas[0] = entrada;
		entradas[1] = entrada;
		entradas[2] = entrada;
		entradas[3] = entrada;
		entradas[4] = entrada;

		int pos = 0;

		void* stream = serializarEntradas(entradas, 5);

		int tamanoEntradas = tamanoDe__t_entradas(entradas, 5);

		void*stream2 = malloc(tamanoEntradas + sizeof(int)+10);

		memcpy(stream2, stream, tamanoEntradas);

		uint32_t valor = 789;

		memcpy(stream2 + tamanoEntradas , &valor, sizeof(int));

		t_entrada * entradas2 = deserializarEntradas(stream2, &pos, 3);

		printf("\n\n\n\nEl valor del retPos es: %d\n", entradas2[2].retVar.size);

		uint32_t recuperoValor = leerUINT32(stream2, &pos);

		printf("\n\n\n\nEl valor recuperdo es: %d\n", recuperoValor);
}

void pruebaSerizalizar__t_instructions(){
	t_intructions instruccion;
	instruccion.offset=53;
	instruccion.start=123;

	t_intructions* instrucciones = malloc(sizeof(t_intructions)*2);
	instrucciones[0] = instruccion;
	instrucciones[1] = instruccion;

	void * stream = serializar__t_instructions(instrucciones, 2);

	t_intructions* instrucciones2 = deserializar__t_instructions(stream, 0, 2);

	printf("\n\n\nLa probabilidad es: %d\n\n\n",instrucciones2[1].offset);

}
/*
t_list * copiarListaDeTVariables(t_list * lista){
	for()
}
*/
PCB_DATA* pruebaSerializarPCB(){
	t_variable * a = malloc(sizeof(t_variable));
	a->ID = 's';

	t_direccion c;
	c.offset = 234;
	c.page = 1234;
	c.size = 231;

	a->direccion = c;

	t_variable * b= malloc(sizeof(t_variable));
	b->ID = 'h';
	b->direccion = c;

	t_list * lista1= list_create();

	list_add(lista1,a);
	list_add(lista1,b);

	t_entrada entrada;
	entrada.argumentos = lista1;
	entrada.variables = lista1;
	entrada.retPos = 42;
	entrada.retVar = c;

	t_entrada * entradas = malloc(sizeof(entrada)*4);
	entradas[0] = entrada;
	entradas[1] = entrada;
	entradas[2] = entrada;
	entradas[3] = entrada;

	t_intructions instruccion;
	instruccion.offset=53;
	instruccion.start=123;

	t_intructions* instrucciones = malloc(sizeof(t_intructions)*2);
	instrucciones[0] = instruccion;
	instrucciones[1] = instruccion;

	PCB_DATA pcb;
	pcb.cantidadDeEntradas = 4;
	pcb.indiceStack = entradas;
	pcb.cantidadDeInstrucciones = 2;
	pcb.indiceCodigo = instrucciones;
	pcb.pid = 1;
	pcb.contextoActual = 53;
	pcb.programCounter = 23;
	pcb.contPags_pcb = 6;
	pcb.cantidadDeEtiquetas = 9;
	pcb.exitCode = -9;
	pcb.indiceEtiquetas = string_duplicate("poronga");

	void * stream = serializarPCB(&pcb);

	free(instrucciones);
	free(a);
	free(b);
	free(entradas);
	char* aux = pcb.indiceEtiquetas;
	free(aux);

	PCB_DATA* pcb2 = deserializarPCB(stream);

	printf("\n\n%s\n\n", pcb2->indiceEtiquetas);
	printf("\n\n%d\n\n", pcb2->indiceCodigo->offset);
	printf("\n\n%d\n\n", pcb2->indiceStack->retPos);

	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->ID);
	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->direccion.offset);
	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->direccion.page);
	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->direccion.size);

	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->ID);
	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->direccion.offset);
	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->direccion.page);
	printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->direccion.size);

	printf("\n%d\n",pcb2->indiceStack[0].retPos);
	printf("\n%d\n",pcb2->indiceStack[1].retPos);
	printf("\n%d\n",pcb2->indiceStack[2].retPos);
	printf("\n%d\n",pcb2->indiceStack[3].retPos);

	printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[0].argumentos, 1))->ID);
	printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[1].argumentos, 0))->ID);
	t_list * lista = pcb2->indiceStack[3].argumentos;
	printf("\n%d\n", list_size(lista));
	printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[3].argumentos, 1))->ID);

	printf("\n\n%d\n\n", pcb2->contPags_pcb);
	printf("\n\n%d\n\n", pcb2->cantidadDeEntradas);
	//free(((t_variable*)list_get(pcb2.indiceStack[1].argumentos, 0)));

	return pcb2;
}

PCB_DATA *global;

void otraPrueba(PCB_DATA* pcb2){
	printf("\n\n%s\n\n", pcb2->indiceEtiquetas);
		printf("\n\n%d\n\n", pcb2->indiceCodigo->offset);
		printf("\n\n%d\n\n", pcb2->indiceStack->retPos);

		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->ID);
		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->direccion.offset);
		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->direccion.page);
		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 0))->direccion.size);

		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->ID);
		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->direccion.offset);
		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->direccion.page);
		printf("\n%d\n", ((t_variable*)list_get(pcb2->indiceStack->argumentos, 1))->direccion.size);

		printf("\n%d\n",pcb2->indiceStack[0].retPos);
		printf("\n%d\n",pcb2->indiceStack[1].retPos);
		printf("\n%d\n",pcb2->indiceStack[2].retPos);
		printf("\n%d\n",pcb2->indiceStack[3].retPos);

		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[0].argumentos, 1))->ID);
		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[1].argumentos, 0))->ID);
		t_list * lista = pcb2->indiceStack[3].argumentos;
		printf("\n%d\n", list_size(lista));
		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[3].argumentos, 1))->ID);

		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[0].argumentos, 1))->ID);
		((t_variable*)list_get(pcb2->indiceStack[0].argumentos, 1))->ID = 'z';
		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[0].argumentos, 1))->ID);
		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[0].argumentos, 1))->ID);


		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[1].argumentos, 1))->ID);
		printf("\n%c\n", ((t_variable*)list_get(pcb2->indiceStack[2].argumentos, 1))->ID);

		printf("\n\n%d\n\n", pcb2->contPags_pcb);
		printf("\n\n%d\n\n", pcb2->cantidadDeEntradas);

		printf("\n\n%d\n\n", pcb2->indiceCodigo[0].offset);
		printf("\n\n%d\n\n", pcb2->indiceCodigo[1].offset);
		printf("\n\n%d\n\n", pcb2->indiceCodigo[1].start);
		printf("\n\n%d\n\n", pcb2->indiceCodigo[0].start);

		printf("PIJA");
}


void list_forEach2(t_list * self, void (*funcion) (void*)){
	t_link_element *element = self->head;
	while(element != NULL ){
		funcion(element->data);
		element = element->next;
	}
}



void imprimirListaDe__t_variables(t_list* lista, char* nombreLista){
	//printf("La lista de %s contiene",nombreLista);
	int indice = 0;
	void imprimir__t_variable(t_variable * variable){
		printf("\nPara el valor %d de la lista de %s:",indice, nombreLista);
		printf("\n\tEl id es: %c",variable->ID);
		printf("\n\tLos valores de la direccion son:");
		printf("\n\t\tPage: %d",variable->direccion.page);
		printf("\n\t\tOffset: %d",variable->direccion.offset);
		printf("\n\t\tSize: %d",variable->direccion.size);
		indice++;
	}
	list_forEach2(lista, imprimir__t_variable);
}

void imprimirEntradas(t_entrada* indiceStack, int cantidadDeEntradas){
	int i;
	for(i=0;i<cantidadDeEntradas;i++){
		printf("\n\nPara la entrada numero %d", i);
		printf("\nSu retPos es %d",indiceStack[i].retPos);
		printf("\nSu retVar.offset es %d",indiceStack[i].retVar.offset);
		printf("\nSu retVar.page es %d",indiceStack[i].retVar.page);
		printf("\nSu retVar.size es %d",indiceStack[i].retVar.size);
		imprimirListaDe__t_variables(indiceStack[i].argumentos,"argumentos");
		imprimirListaDe__t_variables(indiceStack[i].variables,"variables");
	}
}

void imprimirIndiceDeCodigo(t_intructions* indiceDeCodigo, int cantidadDeInstrucciones){
	int i;
	printf("\nLista de indiceDeCodigo:");
	for(i=0;i<cantidadDeInstrucciones;i++){
		printf("\n\tEn la posicion %d el indiceDeCodigo.offset vale: %d ", i, indiceDeCodigo[i].offset);
		printf("\n\tEn la posicion %d el indiceDeCodigo.start vale: %d", i, indiceDeCodigo[i].start);
	}
}

void imprimirPCB(PCB_DATA * pcb){
	printf("\nSu cantidadEntradas es: %d",pcb->cantidadDeEntradas);
	printf("\nSu pid es: %d",pcb->pid);
	printf("\nSu programCounter es: %d",pcb->programCounter);
	printf("\nSu cantidadDeInstrucciones es: %d",pcb->cantidadDeInstrucciones);
	printf("\nSu contPags_pcb es: %d",pcb->contPags_pcb);
	printf("\nSu indiceEtiquetas es: %s",pcb->indiceEtiquetas);
	printf("\nSu cantidadDeEtiquetas es: %d",pcb->cantidadDeEtiquetas);
	printf("\nSu exitCode es: %d",pcb->exitCode);
	printf("\nSu contextoActual es: %d",pcb->contextoActual);

	imprimirIndiceDeCodigo(pcb->indiceCodigo, pcb->cantidadDeInstrucciones);

	imprimirEntradas(pcb->indiceStack, pcb->cantidadDeEntradas);

}

char* script =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";


int main(void) {

	PCB_DATA pcb;

	t_metadata_program *metadata = metadata_desde_literal(script);

	pcb.pid = 0;
	pcb.contPags_pcb = 1;
	pcb.contextoActual = -1;
	pcb.exitCode = 0;
	pcb.indiceCodigo = metadata->instrucciones_serializado;
	pcb.cantidadDeInstrucciones = metadata->instrucciones_size;

	pcb.indiceEtiquetas =  metadata->etiquetas;

	//string_append(&(pcb.indiceEtiquetas),"\0");

	pcb.cantidadDeEtiquetas = metadata->cantidad_de_etiquetas;



	pcb.indiceStack = malloc(sizeof(t_entrada));
	pcb.indiceStack->argumentos = list_create();
	pcb.indiceStack->variables = list_create();

	pcb.cantidadDeEntradas = 0;
	pcb.programCounter = metadata->instruccion_inicio;


	void* pcbSerializado = serializarPCB(&pcb);

	PCB_DATA *pcb2 = deserializarPCB(pcbSerializado);

	imprimirPCB(pcb2);

	/*printf("%d\n",pcb.pid);
	global = pruebaSerializarPCB();
	((t_variable*)list_get(global->indiceStack[3].argumentos, 1))->ID = 'k';
	otraPrueba(global);*/

	//imprimirPCB(global);

}













/*

void copiar(char * ruta){
	char * comando = string_new();
	string_append(&comando, "cp -R src/laGranBiblioteca/ ");
	string_append(&comando, ruta);
	string_append(&comando, "/src/");
	system(comando);
}
void copiarEnProyecto(char * proyecto){
	char * comando = string_new();
	string_append(&comando, "../");
	string_append(&comando, proyecto);
	copiar(comando);
	puts("Proceso terminado maquinola");
}


int main(void) {




	/*
	copiarEnProyecto("Kernel");
	copiarEnProyecto("Consola");
	copiarEnProyecto("CPU");
	copiarEnProyecto("Memoria");
	copiarEnProyecto("FileSystem");

*/

/*	printf("Hace algun tiempo, este codigo era lo mas ranciamente hermoso que podiamos haber esperado del gran programadior, G.H.S\nSal2.\n\n\n");

	return EXIT_SUCCESS;
}
*/

