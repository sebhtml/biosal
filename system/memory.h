
#ifndef BSAL_MEMORY_H
#define BSAL_MEMORY_H

#include <stdlib.h>

void *bsal_malloc(size_t size);
void bsal_free(void *pointer);

#endif
