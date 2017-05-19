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





int main(void) {

	/*PCB_DATA pcb;

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

	printf("%d\n",pcb.pid);
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

