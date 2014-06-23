
#ifndef BSAL_MEMORY_H
#define BSAL_MEMORY_H

#include <stdlib.h>

#include <stdint.h>

void *bsal_malloc(size_t size);
void bsal_free(void *pointer);

uint64_t bsal_get_heap_size();

#endif
