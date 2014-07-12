
#include "memory.h"

#include "tracer.h"

#include <string.h>
#include <stdio.h>

/*
 * bound memory allocations in order
 * to detect provided negative numbers
 * size_t value of 18446744073709551615 corresponds to int value -1)
 *
 */

/*
   * Use System Programming Interface on the IBM Blue Gene/Q to get memory usage.
   */
#ifdef __bgq__
#include <spi/include/kernel/memory.h>
#endif

/* minimum is 1 byte
 */
#define BSAL_MEMORY_MINIMUM 1

/*
 * maximum is 1000 * 1000 * 1000 * 1000 bytes (1 terabyte)
 */
#define BSAL_MEMORY_MAXIMUM 1000000000000

void *bsal_memory_allocate_private(size_t size, const char *function, const char *file, int line)
{
    void *pointer;

    /*
    printf("DEBUG bsal_memory_allocate size: %zu (as int: %d)\n", size, (int)size);
    */
    pointer = NULL;

    if (size < BSAL_MEMORY_MINIMUM) {
        printf("DEBUG Error bsal_memory_allocate received a number below the minimum: %zu bytes\n", size);

        if (file != NULL) {
            printf("BSAL_MEMORY_DEBUG bsal_memory_allocate %d bytes %p %s %s %d\n",
                    (int)size, pointer, function, file, line);
        }
        bsal_tracer_print_stack_backtrace();
        exit(1);
    }

    if (size > BSAL_MEMORY_MAXIMUM) {
        printf("DEBUG Error bsal_memory_allocate received a number above the maximum: %zu bytes (int value: %d)\n", size,
                        (int)size);
        if (file != NULL) {
            printf("BSAL_MEMORY_DEBUG bsal_memory_allocate %d bytes %p %s %s %d\n",
                    (int)size, pointer, function, file, line);
        }
        bsal_tracer_print_stack_backtrace();
        exit(1);
    }

#ifdef BSAL_MEMORY_DEBUG_TRACK_TARGET
    char target[] = "bsal_vector_reserve";
    if (strcmp(function, target) == 0) {
        printf("TRACER: call to bsal_memory_allocate in %s\n", function);
        bsal_tracer_print_stack_backtrace();
    }
#endif

    pointer = malloc(size);

#ifdef BSAL_MEMORY_DEBUG_DETAIL
    if (file != NULL) {
        printf("BSAL_MEMORY_DEBUG bsal_memory_allocate %d bytes %p %s %s %d\n",
                    (int)size, pointer, function, file, line);
    }

    /*
     * Ask the tracer to print a stack
     */
    if (size == 4) {
        bsal_tracer_print_stack_backtrace();
    }


#endif

    /*
     * On Linux, this does not happen.
     *
     * \see http://www.win.tue.nl/~aeb/linux/lk/lk-9.html
     * \see http://stackoverflow.com/questions/16674370/why-does-malloc-or-new-never-return-null
     */
    if (pointer == NULL) {
        printf("DEBUG Error bsal_memory_allocate returned %p, %zu bytes\n", pointer, size);
        bsal_tracer_print_stack_backtrace();
        exit(1);
    }

    return pointer;
}

void bsal_memory_free_private(void *pointer, const char *function, const char *file, int line)
{
#ifdef BSAL_MEMORY_DEBUG_DETAIL
    if (file != NULL) {
        printf("BSAL_MEMORY_DEBUG bsal_memory_free %p %s %s %d\n",
                   pointer, function, file, line);
    }
#endif

    if (pointer == NULL) {
        return;
    }

#ifdef BSAL_MEMORY_DEBUG
#endif

    free(pointer);
}

uint64_t bsal_get_heap_size()
{
    uint64_t bytes;
    bytes = 0;

#if defined(__bgq__)
    Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAP,&bytes);

#elif defined(__linux__)

    FILE *descriptor;
    char buffer [1024];
    int heap_size;
    int expected;
    int actual;

    expected = 1;
    descriptor = fopen("/proc/self/status", "r");

    while (!feof(descriptor)) {
        actual = fscanf(descriptor, "%s", buffer);

        if (actual == expected
                        && strcmp(buffer, "VmData:") == 0) {
            actual = fscanf(descriptor, "%d", &heap_size);

#ifdef BSAL_MEMORY_DEBUG_HEAP
            printf("Scanned %d\n", heap_size);
#endif

            if (actual == expected) {
                bytes = (uint64_t)heap_size * 1024;
            }
            break;
        }
    }

    fclose(descriptor);
#endif

    return bytes;
}

size_t bsal_memory_align(size_t unaligned)
{
/* enable alignment only if alignment is greater than 0
 */
#ifdef BSAL_MEMORY_ALIGNMENT_ENABLED
    return bsal_memory_align_private(unaligned, BSAL_MEMORY_ALIGNMENT_DEFAULT);
#else
    return unaligned;
#endif
}

size_t bsal_memory_align_private(size_t unaligned, size_t alignment)
{
    size_t aligned;

    if (alignment == 0) {
        return unaligned;
    }

    aligned = unaligned + (alignment - (unaligned % alignment));

#ifdef BSAL_DNA_KMER_DEBUG_ALIGNMENT
    printf("ALIGNMENT %d unaligned %d aligned %d\n",
                    alignment, unaligned, aligned);
#endif

    return aligned;
}
