#ifndef MEMORY_MANAGER
#define MEMORY_MANAGER
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>


typedef unsigned char uint8_t ;
/*typedef unsigned int uint32_t;*/

void* myMalloc(unsigned int size);

void myFree(void* ptr);

void memoryMap();

#endif
