/*
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include "commons/collections/list.h"
#include <stdint.h>

////-----PCB y Stack--------////
typedef struct{
uint32_t page;
uint32_t offset;
uint32_t size;
}t_direccion;

typedef struct{
 char ID;
 t_direccion direccion;
}__attribute__((packed))t_variable;

typedef struct{
 t_list* argumentos; //es una lista de t_variable
 t_list* variables;  //es una lista de t_variable
 uint32_t retPos;
 t_direccion retVar;
} t_entrada;

typedef struct{
 int pid;
 t_puntero_instruccion programCounter;

 uint32_t cantidadDeInstrucciones;//Sirve para el indice de codigo, Es un contador para meterlo en un for

 uint32_t contPags_pcb;
 uint32_t contextoActual;
 t_intructions* indiceCodigo;//Es un array

char* indiceEtiquetas; // ¡??¡?'¿¿'??¡ que es¡?

t_entrada* indiceStack;//Es otro array

 uint32_t cantidadDeEntradas;//Es un contador para meterlo en un for

 uint32_t exitCode;
}__attribute__((packed)) PCB_DATA;
////-----FIN PCB y Stack--------////


typedef struct{
	uint32_t tipo;
	uint32_t tamano;
} __attribute__((packed))
Header;

enum cosasDelPCB{
	tipoLista = 0,
	tipoEntrada = 1,
	tipoInt = 2,
	tipoT_direccion = 3
};



int tamanoDeListaDe__t_variable(t_list * lista){
	//***Defino el tamaño de un elemento, que va a ser el tamaño de un t_variable
	int tamanoDeElemento = sizeof(t_variable);

	//***Defino el tamaño total de la lista que va a ser la cant de elementos * el tamaño de cada elemento,
	//***le sumo un int para poder leer en el deserializador el tamaño de la lista
	int tamanoTotalDeLista = (list_size(lista) * tamanoDeElemento) + sizeof(uint32_t);

	return tamanoTotalDeLista;
}

int tamanoDe__t_instructions(int cantidadDeInstrucciones){
	return sizeof(t_intructions)*cantidadDeInstrucciones;
}

int tamanoDe__t_entrada(t_entrada entrada){
	return    tamanoDeListaDe__t_variable(entrada.argumentos)
			+ tamanoDeListaDe__t_variable(entrada.variables)
			+ sizeof(uint32_t)
			+ sizeof(t_direccion);
}

int tamanoDe__t_entradas(t_entrada* entradas, int cantidadDeEntradas){
	///***Esta asignacion es para poder guardar el tamaño de las entradas
	int tamano = sizeof(uint32_t);
	int i;
	for(i = 0; i<cantidadDeEntradas ; i++){
		tamano += tamanoDe__t_entrada(entradas[i]);
	}
	return tamano;
}
int tamanoPCB(PCB_DATA * pcb){
	int tamano = sizeof(uint32_t)  												//por el pid
				+sizeof(t_puntero_instruccion)  								//por el program counter
				+sizeof(uint32_t) 												//por el contador de paginas
				+sizeof(uint32_t) 												//por el contexto actual

				+sizeof(uint32_t) 												//por la cantidad de instrucciones
				+tamanoDe__t_instructions(pcb->cantidadDeInstrucciones)			//por las instrucciones

				+(strlen(pcb->indiceStack)+1)									//por el indice de stack ---- esto no se sabe si es un char*

				+sizeof(uint32_t) 												//por la cantidad de entradas
				+tamanoDe__t_entradas(pcb->indiceCodigo,pcb->cantidadDeEntradas)//por las entradas

				+sizeof(uint32_t); 												//por el exit code
	return tamano;
}

void * serializarListaDe__t_variable(t_list * lista){

	int tamanoDeElemento = sizeof(t_variable);
	int tamanoTotalDeLista = (list_size(lista) * tamanoDeElemento);

	//***Hago un malloc donde asigno el tamaño a un chorro de bytes, y le copio el tamaño de la lista al principio
	void * stream = malloc(tamanoTotalDeLista);
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

t_list * deserializarListaDe__t_variable(void * stream, int tamano){
	int cantidadDeElementos = tamano / sizeof(t_variable);
	t_list * lista = list_create();
	int i = 0;
	int recorrido = 0;
	for(i = 0; i<cantidadDeElementos; i++){
		t_variable * elemento = malloc(sizeof(t_variable));
		memcpy(elemento,stream + recorrido, sizeof(t_variable));
		list_add(lista, elemento);
		recorrido += sizeof(t_variable);
	}
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

void * serializarEntrada(t_entrada entrada){
	int tamanoEntrada = tamanoDe__t_entrada(entrada);
	void * stream = malloc(tamanoEntrada);

	void * listaDeArgumentosSerializada = serializarListaDe__t_variable(entrada.argumentos);
	int tamanoListaDeArgumentosSerializada = tamanoDeListaDe__t_variable(entrada.argumentos);
	memcpy(stream, listaDeArgumentosSerializada, tamanoListaDeArgumentosSerializada);
	free(listaDeArgumentosSerializada);


	int recorrido = tamanoListaDeArgumentosSerializada;

	void * listaDeVariablesSerializada = serializarListaDe__t_variable(entrada.variables);
	int tamanoListaDeVariablesSerializada = tamanoDeListaDe__t_variable(entrada.variables);
	memcpy(stream + recorrido, listaDeVariablesSerializada, tamanoListaDeVariablesSerializada);
	free(listaDeVariablesSerializada);

	recorrido += tamanoListaDeVariablesSerializada;

	void * retPosSerializado = serializarUINT32(entrada.retPos);
	memcpy(stream + recorrido, retPosSerializado, sizeof(uint32_t));
	free(retPosSerializado);

	recorrido += sizeof(uint32_t);

	void * retVarSerializado = serializar__t_direccion(entrada.retVar);
	memcpy(stream + recorrido, retPosSerializado, sizeof(t_direccion));
	free(retVarSerializado);

	return stream;
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

void *  serializarPCB(PCB_DATA * pcb){
	int recorrido = 0;
	void * stream = malloc(tamanoPCB(pcb));

	memcpy(stream, &(pcb->pid), sizeof(uint32_t));
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
	memcpy(stream + recorrido, pcb->indiceEtiquetas, strlen(pcb->indiceEtiquetas)+1);
	recorrido += strlen(pcb->indiceEtiquetas)+1;

	void* indiceStack = serializarEntradas(pcb->indiceStack, pcb->cantidadDeEntradas);
	int tamanoEntradas = tamanoDe__t_entradas(pcb->indiceStack, pcb->cantidadDeEntradas);
	memcpy(stream + recorrido, indiceStack, tamanoEntradas);
	free(indiceStack);
	recorrido+=tamanoEntradas;

	memcpy(stream + recorrido, &(pcb->cantidadDeEntradas), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	memcpy(stream + recorrido, &(pcb->exitCode), sizeof(uint32_t));
	recorrido += sizeof(uint32_t);

	 if(recorrido == tamanoPCB(pcb)) printf("LA TENGO ENORME");

	 return stream;
}
*/
