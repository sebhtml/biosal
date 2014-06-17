
#include "memory.h"

#include <stdio.h>

void *bsal_malloc(size_t size)
{
    void *pointer;

    pointer = malloc(size);

    if (pointer == NULL) {
        printf("DEBUG Error bsal_malloc returned %p, %d bytes\n", pointer, (int)size);
        exit(1);
    }

    return pointer;
}

void bsal_free(void *pointer)
{
    if (pointer == NULL) {
        return;
    }

    free(pointer);
}
