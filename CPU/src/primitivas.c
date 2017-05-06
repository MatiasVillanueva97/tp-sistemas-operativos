#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "commons/config.h"
#include "commons/string.h"
#include <parser/metadata_program.h>
#include <parser/parser.h>

#include "../../Nuestras/src/laGranBiblioteca/sockets.h"
#include "compartidas.h"

typedef struct{
 int pid;
 char* mensaje;
} t_mensajeDeProceso;

void AnSISOP_escribir(t_descriptor_archivo descriptor_archivo,void* informacion, t_valor_variable tamanio){
 t_mensajeDeProceso mensajeDeProceso;
 mensajeDeProceso.pid = pcb.pid;
 mensajeDeProceso.mensaje = informacion;
 enviarMensaje(socketKernel,4,&mensajeDeProceso,tamanio + sizeof(int));
}