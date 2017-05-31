/*
 * funcionesDeCache.h
 *
 *  Created on: 17/5/2017
 *      Author: utnso
 */
#include <stdbool.h>
#include <stdint.h>
#ifndef FUNCIONESDECACHE_H_
#define FUNCIONESDECACHE_H_

bool tieneMenosDeNProcesos(int pid);
void cacheFlush();
void* cacheMiss(int pid, int pagina,void* contenido);
void cacheHit(int pid, int pagina);

#endif /* FUNCIONESDECACHE_H_ */
