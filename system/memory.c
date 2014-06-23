
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

void *bsal_malloc(size_t size)
{
    void *pointer;

    /*
    printf("DEBUG bsal_malloc size: %zu (as int: %d)\n", size, (int)size);
    */

    if (size < BSAL_MEMORY_MINIMUM) {
        printf("DEBUG Error bsal_malloc received a number below the minimum: %zu bytes\n", size);
        bsal_tracer_print_stack_backtrace();
        exit(1);
    }

    if (size > BSAL_MEMORY_MAXIMUM) {
        printf("DEBUG Error bsal_malloc received a number above the maximum: %zu bytes (int value: %d)\n", size,
                        (int)size);
        bsal_tracer_print_stack_backtrace();
        exit(1);
    }

    pointer = malloc(size);

    if (pointer == NULL) {
        printf("DEBUG Error bsal_malloc returned %p, %zu bytes\n", pointer, size);
        bsal_tracer_print_stack_backtrace();
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

#ifdef BSAL_MEMORY_DEBUG
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
