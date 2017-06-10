/*
 * funcionesFS.c
 *
 *  Created on: 9/6/2017
 *      Author: utnso
 */

#include "funcionesFS.h"


void* rutinaFS(){
	int operacion = 1;
	void * stream;
	bool rta;
	while(operacion){

		operacion = recibirMensaje(socketFS,stream);
		switch(operacion){
		case(1111):{//Se creo bien!
			rta = (bool) stream;

			break;
		}
		}
		//La misma bola de siempre
	}
}
