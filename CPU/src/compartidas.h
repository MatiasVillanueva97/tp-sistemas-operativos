#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct{
 int pid;
 int contPags_pcb;
}__attribute__((packed)) PCB_DATA;

int socketKernel;
int socketMemoria;
PCB_DATA pcb;
