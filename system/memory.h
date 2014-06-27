
#ifndef BSAL_MEMORY_H
#define BSAL_MEMORY_H

#include <stdlib.h>

#include <stdlib.h>
#include <stdint.h>

/*
 * \see http://stackoverflow.com/questions/15884793/how-to-get-the-name-or-file-and-line-of-caller-method
 */

/*
 */
#define BSAL_MEMORY_DEBUG

#ifdef BSAL_MEMORY_DEBUG

#define bsal_allocate(size) \
        bsal_allocate_private(size, __func__, __FILE__, __LINE__)

#define bsal_free(pointer) \
        bsal_free_private(pointer, __func__, __FILE__, __LINE__)

#else

#define bsal_allocate(size) \
        bsal_allocate_private(size, NULL, NULL, -1)

#define bsal_free(pointer) \
        bsal_free_private(pointer, NULL, NULL, -1)

#endif

void *bsal_allocate_private(size_t size, const char *function, const char *file, int line);
void bsal_free_private(void *pointer, const char *function, const char *file, int line);

uint64_t bsal_get_heap_size();

#endif
