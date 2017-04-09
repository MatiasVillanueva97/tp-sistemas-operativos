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
}
int main(void) {


	copiarEnProyecto("Kernel");
	copiarEnProyecto("Consola");
	copiarEnProyecto("CPU");
	copiarEnProyecto("Memoria");
	copiarEnProyecto("FileSystem");


	return EXIT_SUCCESS;
}
