
#include "memory.h"

#include "tracer.h"

#include <stdio.h>

/*
 * bound memory allocations in order
 * to detect provided negative numbers
 * size_t value of 18446744073709551615 corresponds to int value -1)
 *
 */

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
